/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_MESH_PB_GATT_SRV_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_MESH_PB_GATT_SRV_H_

int bt_mesh_pb_gatt_srv_enable(void);
int bt_mesh_pb_gatt_srv_disable(void);

int bt_mesh_pb_gatt_srv_adv_start(void);

#if (CONFIG_BT_TESTING)
int bt_mesh_pb_gatt_get_attr(struct bt_uuid_16 search_uuid, struct bt_gatt_attr *p_gatt_attr);
#endif
#endif /* ZEPHYR_SUBSYS_BLUETOOTH_MESH_PB_GATT_SRV_H_ */
