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
#include "pb_gatt.h"
#include "beacon.h"
#include "foundation.h"
#include "access.h"
#include "proxy.h"
#include "proxy_msg.h"
#include "pb_gatt_srv.h"

#include "ble_conn.h"
#include "ble_gatts.h"

#define LOG_LEVEL CONFIG_BT_MESH_PROV_LOG_LEVEL
#include "api/mesh_log.h"

#if (CONFIG_BT_MESH_PB_GATT)
#define FAST_ADV_TIME (60LL * MSEC_PER_SEC)

static int64_t fast_adv_timestamp;

static int gatt_send(uint8_t conn_idx, void *data, uint16_t len, bt_gatt_complete_func_t end, void *user_data);

static struct bt_mesh_proxy_role *cli;
static bool service_registered;
static uint16_t pb_gatt_cccd;

static void proxy_msg_recv(struct bt_mesh_proxy_role *role)
{
	switch (role->msg_type) {
	case BT_MESH_PROXY_PROV:
		LOG_DBG("Mesh Provisioning PDU");
		bt_mesh_pb_gatt_recv(role->conn_idx, &role->buf);
		break;

	default:
		LOG_WRN("Unhandled Message Type 0x%02x", role->msg_type);
		break;
	}
}

static int gatt_recv(uint8_t conn_idx, const void *buf, uint16_t len)
{
	const uint8_t *data = buf;

	if (len < 1) {
		LOG_WRN("Too small Proxy PDU");
		return -EINVAL;
	}

	if (PDU_TYPE(data) != BT_MESH_PROXY_PROV) {
		LOG_WRN("Proxy PDU type doesn't match GATT service");
		return -EINVAL;
	}

	return bt_mesh_proxy_msg_recv(conn_idx, buf, len);
}

static void gatt_connected(uint8_t conn_idx, uint8_t role)
{
	if (role != BLE_SLAVE || !service_registered || bt_mesh_is_provisioned() || cli)  {
		return;
	}

	cli = bt_mesh_proxy_role_setup(conn_idx, gatt_send, proxy_msg_recv);
	pb_gatt_cccd = 0;

	LOG_INF("conn_idx %u role %u", conn_idx, role);
}

static void gatt_disconnected(uint8_t conn_idx, uint8_t reason)
{
	if (!service_registered || !cli || cli->conn_idx != conn_idx) {
		return;
	}

	LOG_INF("conn_idx %u reason %x", conn_idx, reason);

	bt_mesh_proxy_role_cleanup(cli);
	cli = NULL;

	bt_mesh_pb_gatt_close(conn_idx);

	bt_mesh_adv_gatt_update();

	if (bt_mesh_is_provisioned()) {
		(void)bt_mesh_pb_gatt_srv_disable();
	}
}

static void bt_mesh_prov_srv_conn_evt_handler(ble_conn_evt_t event, ble_conn_data_u *p_data)
{
    switch (event) {
    case BLE_CONN_EVT_STATE_CHG:
        if (p_data->conn_state.state == BLE_CONN_STATE_DISCONNECTD) {
            gatt_disconnected(p_data->conn_state.info.discon_info.conn_idx, p_data->conn_state.info.discon_info.reason);
        } else if (p_data->conn_state.state == BLE_CONN_STATE_CONNECTED) {
            gatt_connected(p_data->conn_state.info.conn_info.conn_idx, p_data->conn_state.info.conn_info.role);
        }
        break;
    default:
        break;
    }
}

static int prov_ccc_write(uint8_t conn_idx, uint16_t value)
{
	LOG_INF("value 0x%04x", value);

	if (value != BLE_GATT_CCCD_NTF_BIT) {
		LOG_WRN("Client wrote 0x%04x instead enabling notify", value);
		return -1;
	}

	pb_gatt_cccd = value;
	bt_mesh_pb_gatt_start(conn_idx);

	return 0;
}

static void prov_srv_send_rsp(uint8_t conn_idx, uint16_t status)
{
    if (cli->end != NULL) {
        cli->end(conn_idx, status, cli->user_data);
        cli->end = NULL;
        cli->user_data = NULL;
    }
}

enum mesh_proxy_att_idx
{
    MESH_PROV_IDX_PRIM_SVC,
    MESH_PROV_IDX_CHAR_WRITE,
    MESH_PROV_IDX_WRITE,
    MESH_PROV_IDX_CHAR_NTF,
    MESH_PROV_IDX_NTF,
    MESH_PROV_IDX_NTF_CFG,

    MESH_PROV_IDX_NUMBER,
};

static uint8_t mesh_prov_prf_id;
const ble_gatt_attr_desc_t mesh_prov_att_db[MESH_PROV_IDX_NUMBER] = {
    [MESH_PROV_IDX_PRIM_SVC]   = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_PRIMARY_SERVICE),     PROP(RD),            0                                 },
    [MESH_PROV_IDX_CHAR_WRITE] = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),      PROP(RD),            0                                 },
    [MESH_PROV_IDX_WRITE]      = {UUID_16BIT_TO_ARRAY(BLE_GATT_CHAR_MESH_PROV_DATA_IN),   PROP(WC),            CONFIG_BT_MESH_PROXY_MSG_LEN      },
    [MESH_PROV_IDX_CHAR_NTF]   = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),      PROP(RD),            0                                 },
    [MESH_PROV_IDX_NTF]        = {UUID_16BIT_TO_ARRAY(BLE_GATT_CHAR_MESH_PROV_DATA_OUT),  PROP(NTF),           CONFIG_BT_MESH_PROXY_MSG_LEN      },
    [MESH_PROV_IDX_NTF_CFG]    = {UUID_16BIT_TO_ARRAY(BLE_GATT_DESC_CLIENT_CHAR_CFG),     PROP(RD) | PROP(WR), OPT(NO_OFFSET) | sizeof(uint16_t) },
};

static ble_status_t bt_mesh_prov_gatts_msg_cb(ble_gatts_msg_info_t *p_srv_msg_info)
{
    uint8_t  att_idx, conn_idx;
    uint8_t *data;
    uint16_t data_len;

    if (p_srv_msg_info->srv_msg_type == BLE_SRV_EVT_GATT_OPERATION) {
        conn_idx = p_srv_msg_info->msg_data.gatts_op_info.conn_idx;
        if (cli == NULL || cli->conn_idx != conn_idx) {
            LOG_ERR("No PB-GATT Client found");
            return -1;
        }

        if (p_srv_msg_info->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_WRITE_REQ) {
            att_idx = p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.write_req.att_idx;
            data = p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.write_req.p_val;
            data_len = p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.write_req.val_len;

            switch (att_idx) {
            case MESH_PROV_IDX_WRITE:
                gatt_recv(conn_idx, data, data_len);
                break;
            case MESH_PROV_IDX_NTF_CFG:
                if (data_len != sizeof(uint16_t))
                    return BLE_ERR_NO_ERROR;
                prov_ccc_write(conn_idx, *(uint16_t *)data);
                break;
            default:
                break;
            }
        } else if (p_srv_msg_info->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_NTF_IND_SEND_RSP) {
            prov_srv_send_rsp(conn_idx, p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.ntf_ind_send_rsp.status);
        } else if (p_srv_msg_info->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_READ_REQ) {
            ble_gatts_read_req_t *p_req = &p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.read_req;
            switch (p_req->att_idx) {
            case MESH_PROV_IDX_NTF_CFG:
                p_req->val_len = sizeof(uint16_t);
                p_req->att_len = sizeof(uint16_t);
                memcpy(p_req->p_val, &pb_gatt_cccd, p_req->val_len);
                break;
            default:
                break;
            }
        }
    }

    return BLE_ERR_NO_ERROR;
}

int bt_mesh_pb_gatt_srv_enable(void)
{
	uint8_t mesh_prov_svc_uuid[16] = UUID_16BIT_TO_ARRAY(BLE_GATT_SVC_MESH_PROVISIONING);

	LOG_DBG("");

	if (bt_mesh_is_provisioned()) {
		return -ENOTSUP;
	}

	if (service_registered) {
		return -EBUSY;
	}

	ble_gatts_svc_add(&mesh_prov_prf_id, mesh_prov_svc_uuid, 0, 0, mesh_prov_att_db, MESH_PROV_IDX_NUMBER, bt_mesh_prov_gatts_msg_cb);

	service_registered = true;
	fast_adv_timestamp = k_uptime_get();

	ble_conn_callback_register(bt_mesh_prov_srv_conn_evt_handler);

	return 0;
}

int bt_mesh_pb_gatt_srv_disable(void)
{
	LOG_DBG("");

	if (!service_registered) {
		return -EALREADY;
	}

	ble_gatts_svc_rmv(mesh_prov_prf_id);
	service_registered = false;

	ble_conn_callback_unregister(bt_mesh_prov_srv_conn_evt_handler);

	bt_mesh_adv_gatt_update();

	return 0;
}

static uint8_t prov_svc_data[20] = {
	BT_UUID_16_ENCODE(BT_UUID_MESH_PROV_VAL),
};

static const struct bt_data prov_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_MESH_PROV_VAL)),
	BT_DATA(BT_DATA_SVC_DATA16, prov_svc_data, sizeof(prov_svc_data)),
};

static size_t gatt_prov_adv_create(struct bt_data prov_sd[2])
{
	size_t prov_sd_len = 0;

	const struct bt_mesh_prov *prov = bt_mesh_prov_get();
	size_t uri_len;

	memcpy(prov_svc_data + 2, prov->uuid, 16);
	sys_put_be16(prov->oob_info, prov_svc_data + 18);

#if defined(CONFIG_BT_MESH_PB_GATT_USE_DEVICE_NAME)
	prov_sd[0].type = BT_DATA_NAME_COMPLETE;
	prov_sd[0].data_len = sizeof(CONFIG_BT_DEVICE_NAME) - 1;
	prov_sd[0].data = (const uint8_t *)CONFIG_BT_DEVICE_NAME;

	prov_sd_len += 1;
#endif

	if (!prov->uri) {
		return prov_sd_len;
	}

	uri_len = strlen(prov->uri);
	if (uri_len > 29) {
		/* There's no way to shorten an URI */
		LOG_WRN("Too long URI to fit advertising packet");
		return 0;
	}

	prov_sd[1].type = BT_DATA_URI;
	prov_sd[1].data_len = uri_len;
	prov_sd[1].data = (const uint8_t *)prov->uri;

	prov_sd_len += 1;

	return prov_sd_len;
}

static int gatt_send(uint8_t conn_idx, void *data, uint16_t len, bt_gatt_complete_func_t end, void *user_data)
{
	LOG_DBG("conn_idx %u   data %u bytes: %s", conn_idx, len, bt_hex(data, len));

	if (cli == NULL || cli->conn_idx != conn_idx || pb_gatt_cccd != BLE_GATT_CCCD_NTF_BIT) {
		LOG_ERR("No PB-GATT Client found");
		return -ENOTCONN;
	}

	cli->end = end;
	cli->user_data = user_data;

	ble_gatts_ntf_ind_send(conn_idx, mesh_prov_prf_id, MESH_PROV_IDX_NTF, data, len, BLE_GATT_NOTIFY);

	return 0;
}

int bt_mesh_pb_gatt_srv_adv_start(void)
{
	int ret;

	LOG_DBG("");

	if (!service_registered || bt_mesh_is_provisioned() || !bt_mesh_proxy_has_avail_conn() || cli != NULL) {
		return SYS_FOREVER_MS;
	}

	struct ble_mesh_adv_param_t fast_adv_param = {
		.own_addr_type = BLE_GAP_LOCAL_ADDR_STATIC,
		.prop = BLE_GAP_ADV_PROP_UNDIR_CONN,
		ADV_FAST_INT,
	};
	struct ble_mesh_adv_param_t slow_adv_param = {
		.own_addr_type = BLE_GAP_LOCAL_ADDR_STATIC,
		.prop = BLE_GAP_ADV_PROP_UNDIR_CONN,
		ADV_SLOW_INT,
	};
	struct bt_data prov_sd[2];
	size_t prov_sd_len;
	int64_t timestamp = fast_adv_timestamp;
	int64_t elapsed_time = k_uptime_delta(&timestamp);

	prov_sd_len = gatt_prov_adv_create(prov_sd);
	if (prov_sd_len == 0) {
		slow_adv_param.prop = BLE_GAP_ADV_PROP_CONNECTABLE_BIT;
		fast_adv_param.prop = BLE_GAP_ADV_PROP_CONNECTABLE_BIT;
	}

	if (elapsed_time > FAST_ADV_TIME) {
		slow_adv_param.timeout = 0 /* Forever */;
		ret = bt_mesh_adv_gatt_start(&slow_adv_param, prov_ad, ARRAY_SIZE(prov_ad), prov_sd, prov_sd_len);
		if (ret) {
			LOG_WRN("Failed to advertise");
		}

		return SYS_FOREVER_MS;
	}

	fast_adv_param.timeout = (FAST_ADV_TIME - elapsed_time);
	LOG_DBG("remaining fast adv time (%d ms)", (FAST_ADV_TIME - elapsed_time));
	/* Advertise 60 seconds using fast interval */
	ret = bt_mesh_adv_gatt_start(&fast_adv_param, prov_ad, ARRAY_SIZE(prov_ad), prov_sd, prov_sd_len);
	if (ret) {
		LOG_WRN("Failed to advertise");
		return SYS_FOREVER_MS;
	}

	return fast_adv_param.timeout;
}

#if (CONFIG_BT_TESTING)
int bt_mesh_pb_gatt_get_attr(struct bt_uuid_16 search_uuid, struct bt_gatt_attr *p_gatt_attr)
{
    struct bt_gatt_attr gatt_attr;
    uint16_t start_handle = 0;

    if (ble_gatts_get_start_hdl(mesh_prov_prf_id, &start_handle) != BLE_ERR_NO_ERROR) {
        return -ESRCH;
    }

    if (search_uuid.val == BLE_GATT_CHAR_MESH_PROV_DATA_IN) {
        p_gatt_attr->handle = start_handle + MESH_PROV_IDX_WRITE;
        p_gatt_attr->perm = BT_GATT_PERM_WRITE;
    } else if (search_uuid.val == BLE_GATT_CHAR_MESH_PROV_DATA_OUT) {
        p_gatt_attr->handle = start_handle + MESH_PROV_IDX_NTF;
        p_gatt_attr->perm = 0;
    } else {
        return -EINVAL;
    }
    return 0;
}
#endif
#endif // CONFIG_BT_MESH_PB_GATT
