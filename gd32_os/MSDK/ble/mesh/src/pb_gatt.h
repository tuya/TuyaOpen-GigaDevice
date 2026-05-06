/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2022 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int bt_mesh_pb_gatt_start(uint8_t conn_idx);
int bt_mesh_pb_gatt_close(uint8_t conn_idx);
int bt_mesh_pb_gatt_recv(uint8_t conn_idx, struct net_buf_simple *buf);

int bt_mesh_pb_gatt_cli_open(uint8_t conn_idx);
int bt_mesh_pb_gatt_cli_start(uint8_t conn_idx);
