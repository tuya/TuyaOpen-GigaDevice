/*
 * Copyright (c) 2021 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mesh_cfg.h"
#include "mesh_kernel.h"
#include "sys/byteorder.h"

#include "net/buf.h"
#include "bluetooth/mesh_bluetooth.h"
#include "bluetooth/mesh_uuid.h"
#include "api/mesh.h"

#include "bluetooth/bt_str.h"

#include "ble_gap.h"
#include "ble_gattc.h"
#include "ble_error.h"
#include "ble_conn.h"
#include "ble_scan.h"

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
#include "proxy_cli.h"
#include "gatt_cli.h"
#include "pb_gatt_cli.h"
#include "scan.h"

#define LOG_LEVEL CONFIG_BT_MESH_PROXY_LOG_LEVEL
#include "api/mesh_log.h"

#if (CONFIG_BT_MESH_GATT_CLIENT)
static struct bt_mesh_gatt_server {
    uint8_t conn_idx;
    ble_gap_addr_t addr;
    bool connecting;
    uint16_t data_in_handle;
    uint16_t data_out_cccd_handle;
    const struct bt_mesh_gatt_cli *gatt;
    void *user_data;
    bt_gatt_complete_func_t end;
    void *send_user_data;
} servers[CONFIG_BT_MAX_CONN] = {
    [0 ... (CONFIG_BT_MAX_CONN - 1)] = {
        .conn_idx = BLE_CONN_CONIDX_INVALID,
    },
};

static struct bt_mesh_gatt_server *get_server_by_addr(ble_gap_addr_t *addr)
{
    for (int i = 0; i < ARRAY_SIZE(servers); i++) {
        if (!memcmp(&servers[i].addr, addr, sizeof(ble_gap_addr_t))) {
            return &servers[i];
        }
    }

    return NULL;
}

static struct bt_mesh_gatt_server *get_server_by_conidx(uint8_t conn_idx)
{
    for (int i = 0; i < ARRAY_SIZE(servers); i++) {
        if (servers[i].conn_idx == conn_idx) {
            return &servers[i];
        }
    }

    return NULL;
}

static struct bt_mesh_gatt_server *alloc_server(ble_gap_addr_t *addr)
{
    for (int i = 0; i < ARRAY_SIZE(servers); i++) {
        if (servers[i].conn_idx == BLE_CONN_CONIDX_INVALID && !servers[i].connecting) {
            memcpy(&servers[i].addr, addr, sizeof(ble_gap_addr_t));
            return &servers[i];
        }
    }

    return NULL;
}

static uint8_t notify_func(uint8_t conn_idx, uint16_t handle, void *data, uint16_t length)
{
    const uint8_t *val = data;

    if (!data) {
        LOG_WRN("[UNSUBSCRIBED]");
        return -1;
    }

    if (length < 1) {
        LOG_WRN("Too small Proxy PDU");
        return -1;
    }

    (void)bt_mesh_proxy_msg_recv(conn_idx, val, length);

    return 0;
}

static void notify_enabled(uint8_t conn_idx)
{
    struct bt_mesh_gatt_server *server = get_server_by_conidx(conn_idx);

    if (server == NULL) {
        LOG_WRN("get server fail conn_idx:%u", conn_idx);
        return;
    }

    LOG_DBG("[SUBSCRIBED]");

    server->gatt->link_open(conn_idx);
}

static void write_resp(uint8_t conn_idx, uint16_t handle, ble_status_t status)
{
    struct bt_mesh_gatt_server *server = get_server_by_conidx(conn_idx);

    if (server == NULL) {
        LOG_WRN("get server fail conn_idx:%u", conn_idx);
        return;
    }

    if (handle == server->data_out_cccd_handle && status == BLE_ERR_NO_ERROR) {
        notify_enabled(conn_idx);
    } else if (handle == server->data_in_handle) {
        if (server->end != NULL) {
            server->end(conn_idx, status, server->send_user_data);
            server->end = NULL;
            server->send_user_data = NULL;
        }
    }
}

int bt_mesh_gatt_send(uint8_t conn_idx, void *data, uint16_t len, bt_gatt_complete_func_t end, void *user_data)
{
    int ret;
    struct bt_mesh_gatt_server *server = get_server_by_conidx(conn_idx);
    if (server == NULL) {
        LOG_WRN("get server fail conn_idx:%u", conn_idx);
        return -1;
    }

    LOG_DBG("%u bytes: %s", len, bt_hex(data, len));

    server->end = end;
    server->send_user_data = user_data;

    ret = ble_gattc_write_cmd(conn_idx, server->data_in_handle, len, data);
    if(ret != BLE_ERR_NO_ERROR) {
        LOG_ERR("gattc write faile");
        return -1;
    }

    return 0;
}

ble_status_t bt_proxy_cli_cb(ble_gattc_msg_info_t *p_cli_msg_info)
{
    if (p_cli_msg_info->cli_msg_type == BLE_CLI_EVT_GATT_OPERATION) {
        if (p_cli_msg_info->msg_data.gattc_op_info.gattc_op_sub_evt == BLE_CLI_EVT_WRITE_RSP) {
            ble_gattc_write_rsp_t *write_rsp = &p_cli_msg_info->msg_data.gattc_op_info.gattc_op_data.write_rsp;
            write_resp(p_cli_msg_info->msg_data.gattc_op_info.conn_idx, write_rsp->handle, write_rsp->status);
        } else if (p_cli_msg_info->msg_data.gattc_op_info.gattc_op_sub_evt == BLE_CLI_EVT_NTF_IND_RCV) {
            ble_gattc_ntf_ind_t *ntf_ind = &p_cli_msg_info->msg_data.gattc_op_info.gattc_op_data.ntf_ind;
            notify_func(p_cli_msg_info->msg_data.gattc_op_info.conn_idx, ntf_ind->handle, ntf_ind->p_value, ntf_ind->length);
        }
    }

    return BLE_ERR_NO_ERROR;
}

static void bt_gatt_discovery_callback(uint8_t conn_idx, uint16_t status)
{
    ble_gattc_uuid_info_t srv_uuid_info = {0};
    ble_gattc_uuid_info_t char_uuid_info = {0};
    ble_gattc_uuid_info_t desc_uuid_info = {0};
    ble_status_t ret = BLE_ERR_NO_ERROR;
    uint16_t out_cccd_handle  = 0, in_handle = 0;
    uint8_t cccd_buf[BLE_GATT_UUID_16_LEN] = {1, 0};
    struct bt_mesh_gatt_server *server;

    if (status != BLE_ERR_NO_ERROR) {
        LOG_ERR("discovery fail. conn_idx:%u status:%x");
        return;
    }

    server = get_server_by_conidx(conn_idx);
    if (server == NULL) {
        LOG_WRN("get server fail conn_idx:%u", conn_idx);
        return;
    }

    srv_uuid_info.instance_id = 0;
    srv_uuid_info.ble_uuid = server->gatt->srv_uuid;
    char_uuid_info.instance_id = 0;
    char_uuid_info.ble_uuid = server->gatt->data_out_uuid;
    desc_uuid_info.instance_id = 0;
    desc_uuid_info.ble_uuid = server->gatt->data_out_cccd_uuid;
    ret = ble_gattc_find_desc_handle(conn_idx, &srv_uuid_info, &char_uuid_info, &desc_uuid_info, &out_cccd_handle);
    if(ret != BLE_ERR_NO_ERROR) {
        LOG_ERR("can't find proxy out uuid");
        return;
    }

    srv_uuid_info.instance_id = 0;
    srv_uuid_info.ble_uuid = server->gatt->srv_uuid;
    char_uuid_info.instance_id = 0;
    char_uuid_info.ble_uuid = server->gatt->data_in_uuid;
    ret = ble_gattc_find_char_handle(conn_idx, &srv_uuid_info, &char_uuid_info, &in_handle);
    if(ret != BLE_ERR_NO_ERROR) {
        LOG_ERR("can't find proxy in uuid");
        return;
    }

    server->data_out_cccd_handle = out_cccd_handle;
    server->data_in_handle = in_handle;

    LOG_DBG("discovery out cccd handle: %x, in handle: %x", out_cccd_handle, in_handle);

    ble_gattc_svc_reg((ble_uuid_t *)&server->gatt->srv_uuid, bt_proxy_cli_cb);

    ret = ble_gattc_write_req(conn_idx, out_cccd_handle, BLE_GATT_UUID_16_LEN, cccd_buf);
    if(ret != BLE_ERR_NO_ERROR) {
        LOG_ERR("gattc write faile");
    }
}

static void gatt_connected(ble_gap_addr_t *peer_addr, uint8_t conn_idx, uint8_t role)
{
    struct bt_mesh_gatt_server *server = get_server_by_addr(peer_addr);

    if (server == NULL || role != BLE_MASTER) {
        return;
    }

    LOG_INF("conn_idx %d role %d", conn_idx, role);

    server->conn_idx = conn_idx;

    server->gatt->connected(conn_idx, server->user_data);

    server->connecting = false;

    ble_gattc_start_discovery(conn_idx, bt_gatt_discovery_callback);
}

static void gatt_disconnected(uint8_t conn_idx, uint8_t reason)
{
    struct bt_mesh_gatt_server *server = get_server_by_conidx(conn_idx);
    if (server == NULL) {
        return;
    }

    server->gatt->disconnected(conn_idx);

    LOG_INF("conn_idx %d reason %x", conn_idx, reason);

    (void)memset(server, 0, sizeof(struct bt_mesh_gatt_server));
    server->conn_idx = BLE_CONN_CONIDX_INVALID;
}

int bt_mesh_gatt_cli_connect(const bt_addr_le_t *addr, const struct bt_mesh_gatt_cli *gatt, void *user_data)
{
    int err;
    struct bt_mesh_gatt_server *server;

    server = get_server_by_addr((ble_gap_addr_t *)addr);
    if (server != NULL) {
        return 0;
    }

    server = alloc_server((ble_gap_addr_t *)addr);
    if (server == NULL) {
        LOG_ERR("alloc server fail");
        return -EALREADY;
    }

    LOG_DBG("Try to connect services");

    err = ble_conn_connect(NULL, BLE_GAP_LOCAL_ADDR_STATIC, (ble_gap_addr_t *)addr, false);
    if (err) {
        LOG_ERR("Connection failed (err:%d)", err);
        return err;
    }

    server->gatt = gatt;
    server->user_data = user_data;
    server->connecting = true;

    return 0;
}

static void gatt_advertising_recv(const struct bt_le_scan_recv_info *info,
				   struct net_buf_simple *buf)
{
	uint16_t uuid;

	if (buf->len < 3) {
		return;
	}

	uuid = net_buf_simple_pull_le16(buf);
	switch (uuid) {
#if defined(CONFIG_BT_MESH_PROXY_CLIENT)
	case BT_UUID_MESH_PROXY_VAL:
		bt_mesh_proxy_cli_adv_recv(info, buf);
		break;
#endif
#if defined(CONFIG_BT_MESH_PB_GATT_CLIENT)
	case BT_UUID_MESH_PROV_VAL:
		bt_mesh_pb_gatt_cli_adv_recv(info, buf);
		break;
#endif

	default:
		break;
	}
}

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	if (info->adv_type != BT_GAP_ADV_TYPE_ADV_IND) {
		return;
	}

	if (!bt_mesh_proxy_has_avail_conn()) {
		return;
	}

	while (buf->len > 1) {
		struct net_buf_simple_state state;
		uint8_t len, type;

		len = net_buf_simple_pull_u8(buf);
		/* Check for early termination */
		if (len == 0U) {
			return;
		}

		if (len > buf->len) {
			LOG_WRN("AD malformed");
			return;
		}

		net_buf_simple_save(buf, &state);

		type = net_buf_simple_pull_u8(buf);

		buf->len = len - 1;

		switch (type) {
		case BT_DATA_SVC_DATA16:
			gatt_advertising_recv(info, buf);
			break;
		default:
			break;
		}

		net_buf_simple_restore(buf, &state);
		net_buf_simple_pull(buf, len);
	}
}

void ble_mesh_proxy_cli_conn_evt_handler(ble_conn_evt_t event, ble_conn_data_u *p_data)
{
    switch (event) {
    case BLE_CONN_EVT_STATE_CHG: {
        if (p_data->conn_state.state == BLE_CONN_STATE_DISCONNECTD) {
            gatt_disconnected(p_data->conn_state.info.discon_info.conn_idx, p_data->conn_state.info.discon_info.reason);
        } else if (p_data->conn_state.state == BLE_CONN_STATE_CONNECTED) {
            gatt_connected(&p_data->conn_state.info.conn_info.peer_addr, p_data->conn_state.info.conn_info.conn_idx, p_data->conn_state.info.conn_info.role);
        }
    }
        break;
    default:
        break;
    }
}

static struct bt_le_scan_cb scan_cb = {
    .recv = scan_recv,
};

void bt_mesh_gatt_client_init(void)
{
    LOG_DBG("");
    bt_le_scan_cb_register(&scan_cb);
    ble_conn_callback_register(ble_mesh_proxy_cli_conn_evt_handler);
}

void bt_mesh_gatt_client_deinit(void)
{
    bt_le_scan_cb_unregister(&scan_cb);
    ble_conn_callback_unregister(ble_mesh_proxy_cli_conn_evt_handler);
}
#endif // CONFIG_BT_MESH_GATT_CLIENT
