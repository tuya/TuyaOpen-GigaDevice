/** @file
 *  @brief Keys APIs.
 */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_KEYS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_KEYS_H_

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/** The structure that keeps representation of key. */
struct bt_mesh_key {
	/** tinycrypt key representation is the pure key value. */
	uint8_t key[16];
};


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_KEYS_H_ */
