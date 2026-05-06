/** @file
 *  @brief Proxy APIs.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_PROXY_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_PROXY_H_

#include <stdint.h>

#include "mesh_kernel.h"

/**
 * @brief Proxy
 * @defgroup bt_mesh_proxy Proxy
 * @ingroup bt_mesh
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Callbacks for the Proxy feature.
 *
 *  Should be instantiated with @ref BT_MESH_PROXY_CB_DEFINE.
 */
struct bt_mesh_proxy_cb {
	/** @brief Started sending Node Identity beacons on the given subnet.
	 *
	 *  @param net_idx Network index the Node Identity beacons are running
	 *                 on.
	 */
	void (*identity_enabled)(uint16_t net_idx);
	/** @brief Stopped sending Node Identity beacons on the given subnet.
	 *
	 *  @param net_idx Network index the Node Identity beacons were running
	 *                 on.
	 */
	void (*identity_disabled)(uint16_t net_idx);

#if (CONFIG_MESH_CB_REGISTERED)
	struct bt_mesh_proxy_cb *next;
#endif
} __attribute__((aligned(4)));

/**
 *  @brief Register a callback structure for Proxy events.
 *
 *  Registers a structure with callback functions that gets called on various
 *  Proxy events.
 *
 *  @param _name Name of callback structure.
 */
#if (CONFIG_MESH_CB_REGISTERED)
void bt_mesh_proxy_cb_register(struct bt_mesh_proxy_cb *p_proxy_cb);
#else
#define PROXY_CB_SECTION __attribute__((section(".MESH_PROXY_CBS"))) __attribute__((aligned(4))) __attribute__((used))
#define BT_MESH_PROXY_CB_DEFINE(_name)    \
	static const struct bt_mesh_proxy_cb bt_mesh_proxy_cb_ ## _name PROXY_CB_SECTION
#endif

/** @brief Enable advertising with Node Identity.
 *
 *  This API requires that GATT Proxy support has been enabled. Once called
 *  each subnet will start advertising using Node Identity for the next
 *  60 seconds.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_proxy_identity_enable(void);

/** @brief Enable advertising with Private Node Identity.
 *
 *  This API requires that GATT Proxy support has been enabled. Once called
 *  each subnet will start advertising using Private Node Identity for the next
 *  60 seconds.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_proxy_private_identity_enable(void);

/** @brief Allow Proxy Client to auto connect to a network.
 *
 *  This API allows a proxy client to auto-connect a given network.
 *
 *  @param net_idx Network Key Index
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_proxy_connect(uint16_t net_idx);

/** @brief Disallow Proxy Client to auto connect to a network.
 *
 *  This API disallows a proxy client to connect a given network.
 *
 *  @param net_idx Network Key Index
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_proxy_disconnect(uint16_t net_idx);

/** @brief Schedule advertising of Solicitation PDUs.
 *
 *  Once called, the device will schedule advertising Solicitation PDUs for the amount of time
 *  defined by @c adv_int * (@c CONFIG_BT_MESH_SOL_ADV_XMIT + 1), where @c adv_int is 20ms
 *  for Bluetooth v5.0 or higher, or 100ms otherwise.
 *
 *  If the number of advertised Solicitation PDUs reached 0xFFFFFF, the advertisements will
 *  no longer be started until the node is reprovisioned.
 *
 *  @param net_idx  Network Key Index
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_proxy_solicit(uint16_t net_idx);

int bt_mesh_proxy_filter_set(uint16_t net_idx, uint8_t type, int *status);

int bt_mesh_proxy_filter_addr_op(uint16_t net_idx, uint16_t *addrs, uint8_t addr_size, bool add, int *status);

int bt_mesh_proxy_connect_node_id(uint16_t net_idx, uint16_t addr);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_PROXY_H_ */
