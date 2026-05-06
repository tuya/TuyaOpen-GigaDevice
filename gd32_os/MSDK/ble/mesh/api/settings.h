/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MESH_INCLUDE_SETTINGS_SETTINGS_H_
#define MESH_INCLUDE_SETTINGS_SETTINGS_H_

#include "stddef.h"
#include <sys/types.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MESH_NAME_SPACE         "BLE_MESH"

#define SETTINGS_MAX_DIR_DEPTH	8	/* max depth of settings tree */
#define SETTINGS_MAX_NAME_LEN	(8 * SETTINGS_MAX_DIR_DEPTH)
#define SETTINGS_MAX_VAL_LEN	256
#define SETTINGS_NAME_SEPARATOR	'/'
#define SETTINGS_NAME_END '='

typedef int32_t (*settings_read_cb) (void *cb_arg, void *data, size_t len);

typedef int (*settings_load_direct_cb)(const char      *key, size_t   len, settings_read_cb read_cb,
	          void *cb_arg, void *param);

struct settings_handler_static {

	const char *name;
	/**< Name of subtree. */

	int (*h_get)(const char *key, char *val, int val_len_max);
	/**< Get values handler of settings items identified by keyword names.
	 *
	 * Parameters:
	 *  - key[in] the name with skipped part that was used as name in
	 *    handler registration
	 *  - val[out] buffer to receive value.
	 *  - val_len_max[in] size of that buffer.
	 *
	 * Return: length of data read on success, negative on failure.
	 */

	int (*h_set)(const char *key, size_t len, settings_read_cb read_cb,
		     void *cb_arg);
	/**< Set value handler of settings items identified by keyword names.
	 *
	 * Parameters:
	 *  - key[in] the name with skipped part that was used as name in
	 *    handler registration
	 *  - len[in] the size of the data found in the backend.
	 *  - read_cb[in] function provided to read the data from the backend.
	 *  - cb_arg[in] arguments for the read function provided by the
	 *    backend.
	 *
	 * Return: 0 on success, non-zero on failure.
	 */

	int (*h_commit)(void);
	/**< This handler gets called after settings has been loaded in full.
	 * User might use it to apply setting to the application.
	 */

	int (*h_export)(int (*export_func)(const char *name, const void *val,
					   size_t val_len));
	/**< This gets called to dump all current settings items.
	 *
	 * This happens when @ref settings_save tries to save the settings.
	 * Parameters:
	 *  - export_func: the pointer to the internal function which appends
	 *   a single key-value pair to persisted settings. Don't store
	 *   duplicated value. The name is subtree/key string, val is the string
	 *   with value.
	 *
	 * @remarks The User might limit a implementations of handler to serving
	 * only one keyword at one call - what will impose limit to get/set
	 * values using full subtree/key name.
	 *
	 * Return: 0 on success, non-zero on failure.
	 */

#if (CONFIG_MESH_CB_REGISTERED)
  struct settings_handler_static *next;
#endif
} __attribute__((aligned(4)));


#if (CONFIG_MESH_CB_REGISTERED)
void bt_mesh_settings_cb_register(struct settings_handler_static *p_settings_cb);
#else
#define SETTINGS_CB_SECTION __attribute__((section(".MESH_SETTINGS_CBS"))) __attribute__((aligned(4))) __attribute__((used))
/**
 * Define a static handler for settings items
 *
 * @param _hname handler name
 * @param _tree subtree name
 * @param _get get routine (can be NULL)
 * @param _set set routine (can be NULL)
 * @param _commit commit routine (can be NULL)
 * @param _export export routine (can be NULL)
 *
 * This creates a variable _hname prepended by settings_handler_.
 *
 */
#define SETTINGS_STATIC_HANDLER_DEFINE(_hname, _tree, _get, _set, _commit,   \
				       _export)				     \
	static const struct settings_handler_static		     \
				      settings_handler_ ## _hname SETTINGS_CB_SECTION = {       \
		.name = _tree,						     \
		.h_get = _get,						     \
		.h_set = _set,						     \
		.h_commit = _commit,					     \
		.h_export = _export,					     \
	}

#ifdef CONFIG_BT_SETTINGS
#define BT_MESH_SETTINGS_DEFINE(_hname, _subtree, _set)                                            \
	static int pre_##_set(const char *name, size_t len_rd, settings_read_cb read_cb,           \
			      void *cb_arg)                                                        \
	{                                                                                          \
		if (!atomic_test_bit(bt_mesh.flags, BT_MESH_INIT)) {                               \
			return 0;                                                                  \
		}                                                                                  \
		return _set(name, len_rd, read_cb, cb_arg);                                        \
	}                                                                                          \
	SETTINGS_STATIC_HANDLER_DEFINE(bt_mesh_##_hname, _subtree, NULL, pre_##_set,    \
				       NULL, NULL)

#else
/* Declaring non static settings handler helps avoid unnecessary ifdefs
 * as well as unused function warning. Since the declared handler structure is
 * unused, linker will discard it.
 */
#define BT_MESH_SETTINGS_DEFINE(_hname, _subtree, _set)\
	static const struct settings_handler settings_handler_bt_mesh_ ## _hname SETTINGS_CB_SECTION = {\
		.h_set = _set,						     \
	}
#endif

#endif

int settings_save_one(const char *name, const void *value, size_t val_len);

int settings_delete(const char *name);

int settings_name_next(const char *name, const char **next);

int settings_load_subtree_direct(
	const char             *subtree,
	settings_load_direct_cb cb,
	void                   *param);

/* Pending storage actions. */
enum bt_mesh_settings_flag {
	BT_MESH_SETTINGS_RPL_PENDING,
	BT_MESH_SETTINGS_NET_KEYS_PENDING,
	BT_MESH_SETTINGS_APP_KEYS_PENDING,
	BT_MESH_SETTINGS_NET_PENDING,
	BT_MESH_SETTINGS_IV_PENDING,
	BT_MESH_SETTINGS_SEQ_PENDING,
	BT_MESH_SETTINGS_HB_PUB_PENDING,
	BT_MESH_SETTINGS_CFG_PENDING,
	BT_MESH_SETTINGS_MOD_PENDING,
	BT_MESH_SETTINGS_VA_PENDING,
	BT_MESH_SETTINGS_CDB_PENDING,
	BT_MESH_SETTINGS_SRPL_PENDING,
	BT_MESH_SETTINGS_SSEQ_PENDING,
	BT_MESH_SETTINGS_COMP_PENDING,
	BT_MESH_SETTINGS_DEV_KEY_CAND_PENDING,
	BT_MESH_SETTINGS_BRG_PENDING,
	BT_MESH_SETTINGS_TEST_PENDING,
	BT_MESH_SETTINGS_FLAG_COUNT,
};

void bt_mesh_settings_init(void);
void bt_mesh_settings_store_schedule(enum bt_mesh_settings_flag flag);
void bt_mesh_settings_store_cancel(enum bt_mesh_settings_flag flag);
void bt_mesh_settings_store_pending(void);
int bt_mesh_settings_set(settings_read_cb read_cb, void *cb_arg,
			 void *out, size_t read_len);

#ifdef __cplusplus
}
#endif

#endif
