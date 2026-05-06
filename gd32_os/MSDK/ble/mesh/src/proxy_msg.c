/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mesh_cfg.h"
#include "mesh_kernel.h"
#include "sys/byteorder.h"

#include "net/buf.h"
#include "bluetooth/mesh_bluetooth.h"
#include "api/mesh.h"

#include "bluetooth/bt_str.h"

#include "mesh.h"
#include "net.h"
#include "rpl.h"
#include "transport.h"
#include "prov.h"
#include "beacon.h"
#include "foundation.h"
#include "access.h"
#include "proxy.h"
#include "proxy_msg.h"

#include "ble_conn.h"
#include "ble_gattc.h"
#include "ble_gatts.h"

#define LOG_LEVEL CONFIG_BT_MESH_PROXY_LOG_LEVEL
#include "api/mesh_log.h"

#if (CONFIG_BT_MESH_GATT_CLIENT | CONFIG_BT_MESH_GATT_SERVER)
#define PDU_SAR(data)      (data[0] >> 6)

/* MshPRTv1.1: 6.3.2.2:
 * "The timeout for the SAR transfer is 20 seconds. When the timeout
 *  expires, the Proxy Server shall disconnect."
 */
#define PROXY_SAR_TIMEOUT  K_SECONDS(20)

#define SAR_COMPLETE       0x00
#define SAR_FIRST          0x01
#define SAR_CONT           0x02
#define SAR_LAST           0x03

#define PDU_HDR(sar, type) (sar << 6 | (type & BIT_MASK(6)))

static uint8_t __noinit bufs[CONFIG_BT_MAX_CONN * CONFIG_BT_MESH_PROXY_MSG_LEN];

static struct bt_mesh_proxy_role roles[CONFIG_BT_MAX_CONN] = {
    [0 ... (CONFIG_BT_MAX_CONN - 1)] = {
        .conn_idx = BLE_CONN_CONIDX_INVALID,
    },
};

static int conn_count = 0;

static struct bt_mesh_proxy_role *bt_mesh_proxy_role_alloc(uint8_t conn_idx)
{
    for (int i = 0; i < ARRAY_SIZE(roles); i++) {
        if (roles[i].conn_idx == BLE_CONN_CONIDX_INVALID) {
            roles[i].conn_idx = conn_idx;
            return &roles[i];
        }
    }

    LOG_ERR("proxy role alloc fail, reuse roles[0]");
    roles[0].conn_idx = conn_idx;
    return &roles[0];
}

static struct bt_mesh_proxy_role *bt_mesh_proxy_role_get(uint8_t conn_idx)
{
    for (int i = 0; i < ARRAY_SIZE(roles); i++) {
        if (roles[i].conn_idx == conn_idx) {
            return &roles[i];
        }
    }

    return NULL;
}

static void proxy_sar_timeout(struct k_work *work)
{
	struct bt_mesh_proxy_role *role;
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);

	LOG_WRN("Proxy SAR timeout");

	role = CONTAINER_OF(dwork, struct bt_mesh_proxy_role, sar_timer);
	if (role->conn_idx != BLE_CONN_CONIDX_INVALID) {
		ble_conn_disconnect(role->conn_idx, BLE_ERROR_HL_TO_HCI(BLE_LL_ERR_REMOTE_USER_TERM_CON));
	}
}

int bt_mesh_proxy_msg_recv(uint8_t conn_idx, const void *buf, uint16_t len)
{
	const uint8_t *data = buf;
	struct bt_mesh_proxy_role *role = bt_mesh_proxy_role_get(conn_idx);

	if (role == NULL) {
		LOG_WRN("Proxy role NULL conn_idx:%u", conn_idx);
		return -EINVAL;
	}

	LOG_DBG("conn %u len %u: %s", conn_idx, len, bt_hex(buf, len));

	if (net_buf_simple_tailroom(&role->buf) < len - 1) {
		LOG_WRN("Proxy role buffer overflow");
		return -EINVAL;
	}

	switch (PDU_SAR(data)) {
	case SAR_COMPLETE:
		if (role->buf.len) {
			LOG_WRN("Complete PDU while a pending incomplete one");
			return -EINVAL;
		}

		role->msg_type = PDU_TYPE(data);
		net_buf_simple_add_mem(&role->buf, data + 1, len - 1);
		role->cb.recv(role);
		net_buf_simple_reset(&role->buf);
		break;

	case SAR_FIRST:
		if (role->buf.len) {
			LOG_WRN("First PDU while a pending incomplete one");
			return -EINVAL;
		}

		k_work_reschedule(&role->sar_timer, PROXY_SAR_TIMEOUT);
		role->msg_type = PDU_TYPE(data);
		net_buf_simple_add_mem(&role->buf, data + 1, len - 1);
		break;

	case SAR_CONT:
		if (!role->buf.len) {
			LOG_WRN("Continuation with no prior data");
			return -EINVAL;
		}

		if (role->msg_type != PDU_TYPE(data)) {
			LOG_WRN("Unexpected message type in continuation");
			return -EINVAL;
		}

		k_work_reschedule(&role->sar_timer, PROXY_SAR_TIMEOUT);
		net_buf_simple_add_mem(&role->buf, data + 1, len - 1);
		break;

	case SAR_LAST:
		if (!role->buf.len) {
			LOG_WRN("Last SAR PDU with no prior data");
			return -EINVAL;
		}

		if (role->msg_type != PDU_TYPE(data)) {
			LOG_WRN("Unexpected message type in last SAR PDU");
			return -EINVAL;
		}

		/* If this fails, the work handler exits early, as there's no
		 * active SAR buffer.
		 */
		(void)k_work_cancel_delayable(&role->sar_timer);
		net_buf_simple_add_mem(&role->buf, data + 1, len - 1);
		role->cb.recv(role);
		net_buf_simple_reset(&role->buf);
		break;
	}

	return len;
}


uint16_t bt_gatt_get_mtu(uint8_t conn_idx, uint8_t role)
{
    uint16_t mtu;

    if (role == BLE_SLAVE) {
        ble_gatts_mtu_get(conn_idx, &mtu);
    } else {
        ble_gattc_mtu_get(conn_idx, &mtu);
    }

    return mtu;
}

int bt_mesh_proxy_msg_send(uint8_t conn_idx, uint8_t type, struct net_buf_simple *msg,
                                    bt_gatt_complete_func_t end, void *user_data)
{
	int err;
	uint16_t mtu;
	struct bt_mesh_proxy_role *role = bt_mesh_proxy_role_get(conn_idx);

	if (role == NULL) {
		LOG_WRN("Proxy role NULL conn_idx:%u", conn_idx);
		return -EINVAL;
	}

	LOG_DBG("conn_idx %u type 0x%02x len %u: %s", conn_idx, type, msg->len,
		bt_hex(msg->data, msg->len));

	/* ATT_MTU - OpCode (1 byte) - Handle (2 bytes) */
	mtu = bt_gatt_get_mtu(conn_idx, role->role) - 3;
	if (mtu > msg->len) {
		net_buf_simple_push_u8(msg, PDU_HDR(SAR_COMPLETE, type));
		return role->cb.send(conn_idx, msg->data, msg->len, end, user_data);
	}

	net_buf_simple_push_u8(msg, PDU_HDR(SAR_FIRST, type));
	err = role->cb.send(conn_idx, msg->data, mtu, NULL, NULL);
	if (err) {
		return err;
	}

	net_buf_simple_pull(msg, mtu);

	while (msg->len) {
		if (msg->len + 1 <= mtu) {
			net_buf_simple_push_u8(msg, PDU_HDR(SAR_LAST, type));
			err = role->cb.send(conn_idx, msg->data, msg->len, end, user_data);
			if (err) {
				return err;
			}

			break;
		}

		net_buf_simple_push_u8(msg, PDU_HDR(SAR_CONT, type));
		err = role->cb.send(conn_idx, msg->data, mtu, NULL, NULL);
		if (err) {
			return err;
		}

		net_buf_simple_pull(msg, mtu);
	}

	return 0;
}

static void buf_send_end(uint8_t conn_idx, uint16_t status, void *user_data)
{
	struct bt_mesh_proxy_role *role = bt_mesh_proxy_role_get(conn_idx);

	if (role == NULL) {
		LOG_WRN("Proxy role NULL conn_idx:%u", conn_idx);
		return;
	}

	if (status != 0) {
		LOG_WRN("Send failed, status: 0x%x", status);
		return;
	}

	bt_mesh_adv_unref((struct bt_mesh_adv *)user_data);
}

int bt_mesh_proxy_relay_send(uint8_t conn_idx, struct bt_mesh_adv *adv)
{
	int err;

	NET_BUF_SIMPLE_DEFINE(msg, 1 + BT_MESH_NET_MAX_PDU_LEN);

	/* Proxy PDU sending modifies the original buffer,
	 * so we need to make a copy.
	 */
	net_buf_simple_reserve(&msg, 1);
	net_buf_simple_add_mem(&msg, adv->b.data, adv->b.len);

	err = bt_mesh_proxy_msg_send(conn_idx, BT_MESH_PROXY_NET_PDU,
				     &msg, buf_send_end, bt_mesh_adv_ref(adv));

	bt_mesh_adv_send_start(0, err, &adv->ctx);
	if (err) {
		LOG_ERR("Failed to send proxy message (err %d)", err);

		/* If segment_and_send() fails the buf_send_end() callback will
		 * not be called, so we need to clear the user data (net_buf,
		 * which is just opaque data to segment_and send) reference given
		 * to segment_and_send() here.
		 */
		bt_mesh_adv_unref(adv);
	}

	return err;
}

static void proxy_msg_init(struct bt_mesh_proxy_role *role)
{
	/* Check if buf has been allocated, in this way, we no longer need
	 * to repeat the operation.
	 */
	if (role->buf.__buf) {
		net_buf_simple_reset(&role->buf);
		return;
	}

	net_buf_simple_init_with_data(&role->buf,
				      &bufs[ARRAY_INDEX_FLOOR(roles, role) *
					    CONFIG_BT_MESH_PROXY_MSG_LEN],
				      CONFIG_BT_MESH_PROXY_MSG_LEN);

	net_buf_simple_reset(&role->buf);

	k_work_init_delayable(&role->sar_timer, proxy_sar_timeout);

	role->end = NULL;
	role->user_data = NULL;
}

struct bt_mesh_proxy_role *bt_mesh_proxy_role_setup(uint8_t conn_idx, proxy_send_cb_t send, proxy_recv_cb_t recv)
{
	struct bt_mesh_proxy_role *role;

	conn_count++;

	role = bt_mesh_proxy_role_alloc(conn_idx);

	proxy_msg_init(role);

	role->cb.recv = recv;
	role->cb.send = send;

	return role;
}

void bt_mesh_proxy_role_cleanup(struct bt_mesh_proxy_role *role)
{
	/* If this fails, the work handler exits early, as
	 * there's no active connection.
	 */
	(void)k_work_cancel_delayable(&role->sar_timer);
	role->conn_idx = BLE_CONN_CONIDX_INVALID;

	conn_count--;

	bt_mesh_adv_gatt_update();
}

bool bt_mesh_proxy_has_avail_conn(void)
{
	return conn_count < CONFIG_BT_MESH_MAX_CONN;
}
#endif // (CONFIG_BT_MESH_GATT_CLIENT | CONFIG_BT_MESH_GATT_SERVER)