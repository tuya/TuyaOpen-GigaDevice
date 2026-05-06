/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mesh_cfg.h"
#include "api/mesh.h"
#include "bluetooth/bt_str.h"

#include "ble_gap.h"
#include "ble_conn.h"
#include "ble_error.h"

#include "net.h"
#include "proxy.h"
#include "prov.h"
#include "pb_gatt.h"
#include "proxy_msg.h"
#include "pb_gatt_srv.h"
#include "pb_gatt_cli.h"

#define LOG_LEVEL CONFIG_BT_MESH_PROV_LOG_LEVEL
#include "api/mesh_log.h"

#if (CONFIG_BT_MESH_PB_GATT_COMMON)
enum {
	GATT_LINK_ACTIVE,    	/* Link has been opened */
	GATT_PROVISIONER,    	/* The link was opened as provisioner */

	GATT_NUM_FLAGS,
};

struct prov_bearer_send_cb {
	prov_bearer_send_complete_t cb;
	void *cb_data;
};

struct prov_link {
	uint8_t conn_idx;
	atomic_t flags[ATOMIC_BITMAP_SIZE(GATT_NUM_FLAGS)];
	const struct prov_bearer_cb *cb;
	void *cb_data;
	struct prov_bearer_send_cb comp;
	struct k_work_delayable prot_timer;
};

static struct prov_link link;

static void reset_state(void)
{
	link.conn_idx = BLE_CONN_CONIDX_INVALID;
	atomic_clear(link.flags);

	/* If this fails, the protocol timeout handler will exit early. */
	(void)k_work_cancel_delayable(&link.prot_timer);
}

static void link_closed(enum prov_bearer_link_status status)
{
	const struct prov_bearer_cb *cb = link.cb;
	void *cb_data = link.cb_data;

	reset_state();

	cb->link_closed(&bt_mesh_pb_gatt, cb_data, status);
}

static void protocol_timeout(struct k_work *work)
{
	if (!atomic_test_bit(link.flags, GATT_LINK_ACTIVE)) {
		return;
	}

	/* If connection failed or timeout, not allow establish connection */
	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT_CLIENT) &&
	    atomic_test_bit(link.flags, GATT_PROVISIONER)) {
		if (link.conn_idx != BLE_CONN_CONIDX_INVALID) {
			ble_conn_disconnect(link.conn_idx, BLE_ERROR_HL_TO_HCI(BLE_LL_ERR_REMOTE_USER_TERM_CON));
		} else {
			(void)bt_mesh_pb_gatt_cli_setup(NULL);
		}
	}

	LOG_DBG("Protocol timeout");

	link_closed(PROV_BEARER_LINK_STATUS_TIMEOUT);
}

int bt_mesh_pb_gatt_recv(uint8_t conn_idx, struct net_buf_simple *buf)
{
	LOG_DBG("%u bytes: %s", buf->len, bt_hex(buf->data, buf->len));

	if (link.conn_idx != conn_idx || !link.cb) {
		LOG_WRN("Data for unexpected connection");
		return -ENOTCONN;
	}

	if (buf->len < 1) {
		LOG_WRN("Too short provisioning packet (len %u)", buf->len);
		return -EINVAL;
	}

	k_work_reschedule(&link.prot_timer, bt_mesh_prov_protocol_timeout_get());

	link.cb->recv(&bt_mesh_pb_gatt, link.cb_data, buf);

	return 0;
}

int bt_mesh_pb_gatt_start(uint8_t conn_idx)
{
	LOG_DBG("conn_idx %u", conn_idx);

	if (link.conn_idx != BLE_CONN_CONIDX_INVALID) {
		return -EBUSY;
	}

	atomic_set_bit(link.flags, GATT_LINK_ACTIVE);
	link.conn_idx = conn_idx;
	k_work_reschedule(&link.prot_timer, bt_mesh_prov_protocol_timeout_get());

	link.cb->link_opened(&bt_mesh_pb_gatt, link.cb_data);

	return 0;
}

int bt_mesh_pb_gatt_close(uint8_t conn_idx)
{
	LOG_DBG("conn_idx %u", conn_idx);

	if (link.conn_idx != conn_idx) {
		LOG_DBG("Not connected");
		return -ENOTCONN;
	}

	link_closed(PROV_BEARER_LINK_STATUS_SUCCESS);

	return 0;
}

#if defined(CONFIG_BT_MESH_PB_GATT_CLIENT)
int bt_mesh_pb_gatt_cli_start(uint8_t conn_idx)
{
	LOG_DBG("conn_idx %u", conn_idx);

	if (link.conn_idx != BLE_CONN_CONIDX_INVALID) {
		return -EBUSY;
	}

	link.conn_idx = conn_idx;
	k_work_reschedule(&link.prot_timer, bt_mesh_prov_protocol_timeout_get());

	return 0;
}

int bt_mesh_pb_gatt_cli_open(uint8_t conn_idx)
{
	LOG_DBG("conn_idx %u", conn_idx);

	if (link.conn_idx != conn_idx) {
		LOG_DBG("Not connected");
		return -ENOTCONN;
	}

	link.cb->link_opened(&bt_mesh_pb_gatt, link.cb_data);

	return 0;
}

static int prov_link_open(const uint8_t uuid[16], uint8_t timeout, const struct prov_bearer_cb *cb, void *cb_data)
{
	LOG_DBG("uuid %s, timeout %d", bt_hex(uuid, 16), timeout);

	if (atomic_test_and_set_bit(link.flags, GATT_LINK_ACTIVE)) {
		LOG_ERR("Ignoring bearer open: link already active");
		return -EBUSY;
	}

	link.cb = cb;
	link.cb_data = cb_data;

	atomic_set_bit(link.flags, GATT_PROVISIONER);
	k_work_reschedule(&link.prot_timer, K_SECONDS(timeout));

	return bt_mesh_pb_gatt_cli_setup(uuid);
}

static void prov_link_close(enum prov_bearer_link_status status)
{
	if (link.conn_idx == BLE_CONN_CONIDX_INVALID) {
		link_closed(status);
	} else {
		ble_conn_disconnect(link.conn_idx, BLE_ERROR_HL_TO_HCI(BLE_LL_ERR_REMOTE_USER_TERM_CON));
	}
}
#endif

#if defined(CONFIG_BT_MESH_PB_GATT)
static int link_accept(const struct prov_bearer_cb *cb, void *cb_data)
{
	int err;

	err = bt_mesh_adv_enable();
	if (err) {
		LOG_ERR("Failed enabling advertiser");
		return err;
	}

	(void)bt_mesh_pb_gatt_srv_enable();
	bt_mesh_adv_gatt_update();

	link.cb = cb;
	link.cb_data = cb_data;

	return 0;
}

static void link_cancel(void)
{
	(void)bt_mesh_pb_gatt_srv_disable();
}
#endif

static void buf_send_end(uint8_t conn_idx, uint16_t status, void *user_data)
{
	if (link.conn_idx != conn_idx) {
		LOG_WRN("Not connected");
		return;
	}

	if (status != 0) {
		LOG_WRN("Send failed, status: 0x%x", status);
		return;
	}

	if (link.comp.cb) {
		link.comp.cb(0, link.comp.cb_data);
	}
}

static int buf_send(struct net_buf_simple *buf, prov_bearer_send_complete_t cb,
		    void *cb_data)
{
	if (link.conn_idx == BLE_CONN_CONIDX_INVALID) {
		return -ENOTCONN;
	}

	link.comp.cb = cb;
	link.comp.cb_data = cb_data;

	k_work_reschedule(&link.prot_timer, bt_mesh_prov_protocol_timeout_get());

	return bt_mesh_proxy_msg_send(link.conn_idx, BT_MESH_PROXY_PROV, buf, buf_send_end, NULL);
}

static void clear_tx(void)
{
	/* No action */
}

void bt_mesh_pb_gatt_init(void)
{
	link.conn_idx = BLE_CONN_CONIDX_INVALID;
	atomic_clear(link.flags);
	k_work_init_delayable(&link.prot_timer, protocol_timeout);
}

void bt_mesh_pb_gatt_reset(void)
{
	reset_state();
}

const struct prov_bearer bt_mesh_pb_gatt = {
	.type = BT_MESH_PROV_GATT,
#if defined(CONFIG_BT_MESH_PB_GATT_CLIENT)
	.link_open = prov_link_open,
	.link_close = prov_link_close,
#endif
#if defined(CONFIG_BT_MESH_PB_GATT)
	.link_accept = link_accept,
	.link_cancel = link_cancel,
#endif
	.send = buf_send,
	.clear_tx = clear_tx,
};

#endif // CONFIG_BT_MESH_PB_GATT_COMMON