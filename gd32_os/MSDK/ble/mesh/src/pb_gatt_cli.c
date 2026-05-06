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

#include "ble_gap.h"
#include "ble_gatt.h"

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
#include "gatt_cli.h"
#include "proxy_msg.h"

#define LOG_LEVEL CONFIG_BT_MESH_PROV_LOG_LEVEL
#include "api/mesh_log.h"

#if (CONFIG_BT_MESH_PB_GATT_CLIENT)

static struct {
	bool target_set;
	uint8_t target_uuid[16];
	struct bt_mesh_proxy_role *srv;
} server;

static void pb_gatt_msg_recv(struct bt_mesh_proxy_role *role)
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

static void pb_gatt_connected(uint8_t conn_idx, void *user_data)
{
	server.srv = bt_mesh_proxy_role_setup(conn_idx, bt_mesh_gatt_send, pb_gatt_msg_recv);

	server.target_set = false;

	bt_mesh_pb_gatt_cli_start(conn_idx);
}

static void pb_gatt_link_open(uint8_t conn_idx)
{
	bt_mesh_pb_gatt_cli_open(conn_idx);
}

static void pb_gatt_disconnected(uint8_t conn_idx)
{
	bt_mesh_pb_gatt_close(conn_idx);

	bt_mesh_proxy_role_cleanup(server.srv);

	server.srv = NULL;
}

static const struct bt_mesh_gatt_cli pbgatt = {
	.srv_uuid		= BLE_UUID_INIT_16(BLE_GATT_SVC_MESH_PROVISIONING),
	.data_in_uuid		= BLE_UUID_INIT_16(BLE_GATT_CHAR_MESH_PROV_DATA_IN),
	.data_out_uuid		= BLE_UUID_INIT_16(BLE_GATT_CHAR_MESH_PROV_DATA_OUT),
	.data_out_cccd_uuid	= BLE_UUID_INIT_16(BT_UUID_GATT_CCC_VAL),

	.connected		= pb_gatt_connected,
	.link_open		= pb_gatt_link_open,
	.disconnected		= pb_gatt_disconnected
};

int bt_mesh_pb_gatt_cli_setup(const uint8_t uuid[16])
{
	if (server.srv) {
		return -EBUSY;
	}

	memcpy(server.target_uuid, uuid, 16);
	server.target_set = true;
	return 0;
}

void bt_mesh_pb_gatt_cli_adv_recv(const struct bt_le_scan_recv_info *info,
				  struct net_buf_simple *buf)
{
	uint8_t *uuid;
	bt_mesh_prov_oob_info_t oob_info;

	if (server.srv) {
		return;
	}

	if (buf->len != 18) {
		return;
	}

	uuid = net_buf_simple_pull_mem(buf, 16);

	if (server.target_set &&
	    !memcmp(server.target_uuid, uuid, 16)) {
		(void)bt_mesh_gatt_cli_connect(info->addr, &pbgatt, NULL);
		return;
	}

	if (!bt_mesh_prov->unprovisioned_beacon_gatt) {
		return;
	}

	oob_info = (bt_mesh_prov_oob_info_t)net_buf_simple_pull_le16(buf);

	bt_mesh_prov->unprovisioned_beacon_gatt(uuid, oob_info);
}

#endif // CONFIG_BT_MESH_PB_GATT_CLIENT