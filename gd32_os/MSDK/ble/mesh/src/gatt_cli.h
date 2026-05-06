/*
 * Copyright (c) 2021 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


struct bt_mesh_gatt_cli {
	ble_uuid_t srv_uuid;
	ble_uuid_t data_in_uuid;
	ble_uuid_t data_out_uuid;
	ble_uuid_t data_out_cccd_uuid;

	void (*connected)(uint8_t conn_idx, void *user_data);
	void (*link_open)(uint8_t conn_idx);
	void (*disconnected)(uint8_t conn_idx);
};

int bt_mesh_gatt_cli_connect(const bt_addr_le_t *addr, const struct bt_mesh_gatt_cli *gatt, void *user_data);

int bt_mesh_gatt_send(uint8_t conn_idx, void *data, uint16_t len, bt_gatt_complete_func_t end, void *user_data);


void bt_mesh_gatt_client_init(void);

void bt_mesh_gatt_client_deinit(void);
