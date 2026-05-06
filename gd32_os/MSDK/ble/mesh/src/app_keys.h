/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_MESH_APP_KEYS_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_MESH_APP_KEYS_H_

#include "api/mesh.h"
#include "subnet.h"

/** Appkey callback. Instantiate with @ref BT_MESH_APP_KEY_CB */
struct bt_mesh_app_key_cb {
	void (*evt_handler)(uint16_t app_idx, uint16_t net_idx,
			    enum bt_mesh_key_evt evt);

#if (CONFIG_MESH_CB_REGISTERED)
  struct bt_mesh_app_key_cb *next;
#endif
} __attribute__((aligned(4)));

/**
 *  @brief Register an AppKey event callback.
 *
 *  @param _handler Handler function, see @ref bt_mesh_app_key_cb::evt_handler.
 */
#if (CONFIG_MESH_CB_REGISTERED)
void bt_mesh_app_key_cb_register(struct bt_mesh_app_key_cb *p_key_cb);
#else
#define APP_KEYS_CB_SECTION __attribute__((section(".MESH_APPKEYS_CBS"))) __attribute__((aligned(4))) __attribute__((used))
#define BT_MESH_APP_KEY_CB_DEFINE(_handler)                                    \
  static const struct bt_mesh_app_key_cb bt_mesh_app_key_cb_ ## _handler APP_KEYS_CB_SECTION = {       \
		.evt_handler = (_handler),                                     \
	}
#endif

/** @brief Reset the app keys module. */
void bt_mesh_app_keys_reset(void);

/** @brief Initialize a new application key with the given parameters.
 *
 *  @param app_idx AppIndex.
 *  @param net_idx NetIndex the application is bound to.
 *  @param old_key Current application key.
 *  @param new_key Updated application key, or NULL if not known.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_app_key_set(uint16_t app_idx, uint16_t net_idx,
			const struct bt_mesh_key *old_key, const struct bt_mesh_key *new_key);

/** @brief Resolve the message encryption keys, given a message context.
 *
 *  Will use the @c ctx::app_idx and @c ctx::net_idx fields to find a pair of
 *  message encryption keys. If @c ctx::app_idx represents a device key, the
 *  @c ctx::net_idx will be used to determine the net key. Otherwise, the
 *  @c ctx::net_idx parameter will be ignored.
 *
 *  @param ctx     Message context.
 *  @param sub     Subnet return parameter.
 *  @param app_key Application return parameter.
 *  @param aid     Application ID return parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_keys_resolve(struct bt_mesh_msg_ctx *ctx,
			 struct bt_mesh_subnet **sub,
			 const struct bt_mesh_key **app_key, uint8_t *aid);

/** @brief Iterate through all matching application keys and call @c cb on each.
 *
 *  @param dev_key Whether to return device keys.
 *  @param aid     7 bit application ID to match.
 *  @param rx      RX structure to match against.
 *  @param cb      Callback to call for every valid app key.
 *  @param cb_data Callback data to pass to the callback.
 *
 *  @return The AppIdx that yielded a 0-return from the callback.
 */
uint16_t bt_mesh_app_key_find(bool dev_key, uint8_t aid,
			      struct bt_mesh_net_rx *rx,
			      int (*cb)(struct bt_mesh_net_rx *rx,
					const struct bt_mesh_key *key, void *cb_data),
			      void *cb_data);

/** @brief Store pending application keys in persistent storage. */
void bt_mesh_app_key_pending_store(void);

#if (CONFIG_MESH_CB_REGISTERED)
void bt_mesh_app_key_settings_init(void);

void bt_mesh_app_keys_init(void);

void bt_mesh_app_keys_subnet_cb_init(void);
#endif

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_MESH_APP_KEYS_H_ */
