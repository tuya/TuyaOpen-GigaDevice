/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mesh_cfg.h"
#include "mesh_kernel.h"
#include "mesh_errno.h"
#include "mesh_util.h"

#include "api/mesh.h"
#include "bluetooth/mesh_uuid.h"

#include "mesh.h"
#include "subnet.h"
#include "app_keys.h"
#include "net.h"
#include "cdb.h"
#include "crypto.h"
#include "rpl.h"
#include "transport.h"
#include "heartbeat.h"
#include "access.h"
#include "proxy.h"
#include "pb_gatt_srv.h"
#include "api/settings.h"
#include "cfg.h"
#include "brg_cfg.h"
#include "solicitation.h"
#include "va.h"

#include "wrapper_os.h"
#include "nvds_flash.h"
#include "src/dfu_slot.h"

#define LOG_LEVEL CONFIG_BT_MESH_SETTINGS_LOG_LEVEL
#include "api/mesh_log.h"

struct settings_key_cb
{
    char key_name[16];
    uint16_t  val_len;
    struct settings_key_cb *next;
};

#if (CONFIG_BT_SETTINGS)

struct mesh_settings_cb
{
#if CONFIG_BT_MESH_SETTINGS_WORKQ
    os_task_t task_handle;
    os_sema_t list_sema;
    os_mutex_t mutex;
    os_timer_t delay_timer;
    uint32_t flags;
    uint32_t start_time_ms;
    k_ticks_t timer_period;
#endif
    struct settings_key_cb *key_list;
};

static struct mesh_settings_cb mesh_settings = {
#if CONFIG_BT_MESH_SETTINGS_WORKQ
    .task_handle = NULL,
    .list_sema = NULL,
    .mutex = NULL,
    .delay_timer = NULL,
    .flags = 0,
#endif
    .key_list = NULL,
};

#if (!CONFIG_BT_MESH_SETTINGS_WORKQ)
static struct k_work_delayable pending_store;
#endif

//static ATOMIC_DEFINE(pending_flags, BT_MESH_SETTINGS_FLAG_COUNT);
static atomic_t pending_flags[ATOMIC_BITMAP_SIZE(BT_MESH_SETTINGS_FLAG_COUNT)];


#ifdef CONFIG_BT_MESH_RPL_STORE_TIMEOUT
#define RPL_STORE_TIMEOUT CONFIG_BT_MESH_RPL_STORE_TIMEOUT
#else
#define RPL_STORE_TIMEOUT (-1)
#endif

#if (CONFIG_MESH_CB_REGISTERED)
static struct settings_handler_static *p_settings_handlers = NULL;
#else
extern uint32_t _settings_cbs[];
extern uint32_t _esettings_cbs[];
#endif


static int32_t bt_mesh_settings_read_cb(void *cb_arg, void *data, size_t len)
{
    int ret = nvds_data_get(NULL, MESH_NAME_SPACE, cb_arg, data, (uint32_t *)&len);

    if (ret != NVDS_OK) {
        LOG_ERR("settings Failed to read %s value (err %d), length %d", cb_arg, ret, len);
        return -EINVAL;
    }

    return len;
}

int bt_mesh_settings_set(settings_read_cb read_cb, void *cb_arg, void *out, size_t read_len)
{
    ssize_t len;

    len = read_cb(cb_arg, out, read_len);
    if (len < 0) {
        LOG_ERR("Failed to read %s value (err %d)", cb_arg, len);
        return len;
    }

    LOG_HEXDUMP_DBG(out, len, "val");

    if (len != read_len) {
        LOG_ERR("Unexpected %s value length (%u != %u)", cb_arg, len, read_len);
        return -EINVAL;
    }

    return 0;
}

static int mesh_commit(void)
{
    if (!atomic_test_bit(bt_mesh.flags, BT_MESH_INIT)) {
        return 0;
    }

    if (!bt_mesh_subnet_next(NULL)) {
        /* Nothing to do since we're not yet provisioned */
        LOG_WRN(" Nothing to do since we're not yet provisioned");
        return 0;
    }

    if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
        (void)bt_mesh_pb_gatt_srv_disable();
    }

    bt_mesh_net_settings_commit();
    bt_mesh_model_settings_commit();

    atomic_set_bit(bt_mesh.flags, BT_MESH_VALID);

    bt_mesh_start();

    return 0;
}


#if (CONFIG_MESH_CB_REGISTERED)
static struct settings_handler_static settings_handler_bt_mesh = {
    .name = "bt/mesh",
    .h_get = NULL,
    .h_set = NULL,
    .h_commit = mesh_commit,
    .h_export = NULL,
    .next = NULL,
};
#else
SETTINGS_STATIC_HANDLER_DEFINE(bt_mesh, "bt/mesh", NULL, NULL, mesh_commit, NULL);
#endif

/* Pending flags that use K_NO_WAIT as the storage timeout */
#define NO_WAIT_PENDING_BITS (BIT(BT_MESH_SETTINGS_NET_PENDING) |           \
                              BIT(BT_MESH_SETTINGS_IV_PENDING)  |           \
                              BIT(BT_MESH_SETTINGS_SEQ_PENDING) |           \
                              BIT(BT_MESH_SETTINGS_CDB_PENDING))

/* Pending flags that use CONFIG_BT_MESH_STORE_TIMEOUT */
#define GENERIC_PENDING_BITS                                                                       \
	(BIT(BT_MESH_SETTINGS_NET_KEYS_PENDING) | BIT(BT_MESH_SETTINGS_APP_KEYS_PENDING) |         \
	 BIT(BT_MESH_SETTINGS_HB_PUB_PENDING) | BIT(BT_MESH_SETTINGS_CFG_PENDING) |                \
	 BIT(BT_MESH_SETTINGS_MOD_PENDING) | BIT(BT_MESH_SETTINGS_VA_PENDING) |                    \
	 BIT(BT_MESH_SETTINGS_SSEQ_PENDING) | BIT(BT_MESH_SETTINGS_COMP_PENDING) |                 \
	 BIT(BT_MESH_SETTINGS_DEV_KEY_CAND_PENDING) | BIT(BT_MESH_SETTINGS_BRG_PENDING))

void bt_mesh_settings_store_schedule(enum bt_mesh_settings_flag flag)
{
    uint32_t timeout_ms, remaining_ms;
    uint32_t cur_time = sys_current_time_get();
#if CONFIG_BT_MESH_SETTINGS_WORKQ
    uint32_t delta = cur_time - mesh_settings.start_time_ms;
#endif

    atomic_set_bit(pending_flags, flag);

    if (atomic_get(pending_flags) & NO_WAIT_PENDING_BITS) {
        timeout_ms = 0;
    } else if (IS_ENABLED(CONFIG_BT_MESH_RPL_STORAGE_MODE_SETTINGS) && RPL_STORE_TIMEOUT >= 0 &&
               (atomic_test_bit(pending_flags, BT_MESH_SETTINGS_RPL_PENDING) ||
                atomic_test_bit(pending_flags, BT_MESH_SETTINGS_SRPL_PENDING)) &&
               !(atomic_get(pending_flags) & GENERIC_PENDING_BITS)) {
        timeout_ms = RPL_STORE_TIMEOUT * MSEC_PER_SEC;
    } else {
        timeout_ms = CONFIG_BT_MESH_STORE_TIMEOUT * MSEC_PER_SEC;
    }


#if CONFIG_BT_MESH_SETTINGS_WORKQ
    remaining_ms = delta > mesh_settings.timer_period ? 0 : (mesh_settings.timer_period - delta);
#else
    remaining_ms = k_ticks_to_ms_floor32(k_work_delayable_remaining_get(&pending_store));
#endif
    LOG_DBG("Waiting %u ms vs rem %u ms", timeout_ms, remaining_ms);

    /* If the new deadline is sooner, override any existing
     * deadline; otherwise schedule without changing any existing
     * deadline.
     */
    if (timeout_ms < remaining_ms) {
#if CONFIG_BT_MESH_SETTINGS_WORKQ
        bool added = false;
        sys_mutex_get(&mesh_settings.mutex);
        if (mesh_settings.flags & K_WORK_DELAYABLE) {
            mesh_settings.flags &= ~(K_WORK_DELAYABLE);
            sys_timer_stop(&mesh_settings.delay_timer, false);
        }

        if (timeout_ms == 0) {
            if ((mesh_settings.flags & BIT(K_WORK_QUEUED_BIT)) == 0U) {
                mesh_settings.flags |= BIT(K_WORK_QUEUED_BIT);
                added = true;
            }
        } else {
            mesh_settings.flags |= K_WORK_DELAYABLE;
            mesh_settings.start_time_ms = sys_current_time_get();
            mesh_settings.timer_period = timeout_ms;
            sys_timer_start_ext(&mesh_settings.delay_timer, timeout_ms, false);
        }
        sys_mutex_put(&mesh_settings.mutex);

        if (added) {
            sys_sema_up(&mesh_settings.list_sema);
        }
#else
        k_work_reschedule(&pending_store, K_MSEC(timeout_ms));
#endif
    } else {
#if CONFIG_BT_MESH_SETTINGS_WORKQ
        bool added = false;
        sys_mutex_get(&mesh_settings.mutex);
        if (timeout_ms == 0) {
            if ((mesh_settings.flags & BIT(K_WORK_QUEUED_BIT)) == 0U) {
                mesh_settings.flags |= BIT(K_WORK_QUEUED_BIT);
                added = true;
            }
        } else if ((mesh_settings.flags & K_WORK_DELAYABLE) == 0) {
            mesh_settings.flags |= K_WORK_DELAYABLE;
            mesh_settings.start_time_ms = sys_current_time_get();
            mesh_settings.timer_period = timeout_ms;
            sys_timer_start_ext(&mesh_settings.delay_timer, timeout_ms, false);
        }
        sys_mutex_put(&mesh_settings.mutex);

        if (added) {
            sys_sema_up(&mesh_settings.list_sema);
        }
#else
        k_work_schedule(&pending_store, K_MSEC(timeout_ms));
#endif
    }
}

void bt_mesh_settings_store_cancel(enum bt_mesh_settings_flag flag)
{
    atomic_clear_bit(pending_flags, flag);
}

static void store_pending(struct k_work *work)
{
    LOG_DBG("");

    if (atomic_test_and_clear_bit(pending_flags, BT_MESH_SETTINGS_RPL_PENDING)) {
        bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);
    }

    if (atomic_test_and_clear_bit(pending_flags,
                                  BT_MESH_SETTINGS_NET_KEYS_PENDING)) {
        bt_mesh_subnet_pending_store();
    }

    if (atomic_test_and_clear_bit(pending_flags,
                                  BT_MESH_SETTINGS_APP_KEYS_PENDING)) {
        bt_mesh_app_key_pending_store();
    }

    if (atomic_test_and_clear_bit(pending_flags,
                                  BT_MESH_SETTINGS_NET_PENDING)) {
        bt_mesh_net_pending_net_store();
    }

    if (atomic_test_and_clear_bit(pending_flags,
                                  BT_MESH_SETTINGS_IV_PENDING)) {
        bt_mesh_net_pending_iv_store();
    }

    if (atomic_test_and_clear_bit(pending_flags,
                                  BT_MESH_SETTINGS_SEQ_PENDING)) {
        bt_mesh_net_pending_seq_store();
    }

    if (atomic_test_and_clear_bit(pending_flags,
                                  BT_MESH_SETTINGS_DEV_KEY_CAND_PENDING)) {
        bt_mesh_net_pending_dev_key_cand_store();
    }

    if (atomic_test_and_clear_bit(pending_flags,
                                  BT_MESH_SETTINGS_HB_PUB_PENDING)) {
        bt_mesh_hb_pub_pending_store();
    }

    if (atomic_test_and_clear_bit(pending_flags,
                                  BT_MESH_SETTINGS_CFG_PENDING)) {
        bt_mesh_cfg_pending_store();
    }

    if (atomic_test_and_clear_bit(pending_flags,
                                  BT_MESH_SETTINGS_COMP_PENDING)) {
        bt_mesh_comp_data_pending_clear();
    }

    if (atomic_test_and_clear_bit(pending_flags,
                                  BT_MESH_SETTINGS_MOD_PENDING)) {
        bt_mesh_model_pending_store();
    }

    if (atomic_test_and_clear_bit(pending_flags,
                                  BT_MESH_SETTINGS_VA_PENDING)) {
        bt_mesh_va_pending_store();
    }

    if (IS_ENABLED(CONFIG_BT_MESH_CDB) &&
        atomic_test_and_clear_bit(pending_flags,
                                  BT_MESH_SETTINGS_CDB_PENDING)) {
        bt_mesh_cdb_pending_store();
    }

    if (IS_ENABLED(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV) &&
        atomic_test_and_clear_bit(pending_flags,
                                  BT_MESH_SETTINGS_SRPL_PENDING)) {
        bt_mesh_srpl_pending_store();
    }

    if (IS_ENABLED(CONFIG_BT_MESH_PROXY_SOLICITATION) &&
        atomic_test_and_clear_bit(pending_flags,
                                  BT_MESH_SETTINGS_SSEQ_PENDING)) {
        bt_mesh_sseq_pending_store();
    }
    if (IS_ENABLED(CONFIG_BT_MESH_BRG_CFG_SRV) &&
	    atomic_test_and_clear_bit(pending_flags, BT_MESH_SETTINGS_BRG_PENDING)) {
        bt_mesh_brg_cfg_pending_store();
    }

}

#if CONFIG_BT_MESH_SETTINGS_WORKQ
static void mesh_settings_task(void *param)
{
    bool need_store = false;
    for (;;) {
        sys_sema_down(&mesh_settings.list_sema, 0);

        sys_mutex_get(&mesh_settings.mutex);

        need_store = (mesh_settings.flags & BIT(K_WORK_QUEUED_BIT)) != 0U;
        if (!need_store) {
            sys_mutex_put(&mesh_settings.mutex);
            continue;
        }

        mesh_settings.flags &= ~(BIT(K_WORK_QUEUED_BIT));
        mesh_settings.flags |= BIT(K_WORK_RUNNING_BIT);

        sys_mutex_put(&mesh_settings.mutex);

        store_pending(NULL);

        sys_mutex_get(&mesh_settings.mutex);
        mesh_settings.flags &= ~(BIT(K_WORK_RUNNING_BIT));
        sys_mutex_put(&mesh_settings.mutex);
    }
}

static void mesh_settings_work_timeout(void *p_tmr, void *p_arg)
{
    bool need_store = false;
    sys_mutex_get(&mesh_settings.mutex);
    if (sys_timer_pending(&mesh_settings.delay_timer)) {
        // timer is active, it must be rescheduled before timeout function is completed
        LOG_ERR("mesh_work_timeout timer is still pending\r\n");
    } else if (mesh_settings.flags & K_WORK_DELAYABLE) {
        mesh_settings.flags &= ~(K_WORK_DELAYABLE);
        mesh_settings.timer_period = 0;

        if ((mesh_settings.flags & BIT(K_WORK_QUEUED_BIT)) == 0U) {
            mesh_settings.flags |= BIT(K_WORK_QUEUED_BIT);
            need_store = true;
        }
    }
    sys_mutex_put(&mesh_settings.mutex);

    if (need_store) {
        sys_sema_up(&mesh_settings.list_sema);
    }
}
#endif

static void mesh_settings_found_keys_cb(const char *namespace, const char *key, uint16_t val_len)
{
    struct settings_key_cb *alloc_key = sys_malloc(sizeof(struct settings_key_cb));

    if (alloc_key) {
        strncpy(alloc_key->key_name, key, 16);
        alloc_key->val_len = val_len;
        alloc_key->next = NULL;
        if (mesh_settings.key_list == NULL) {
            mesh_settings.key_list = alloc_key;
        } else {
            struct settings_key_cb *cur_key = mesh_settings.key_list;
            // FIX TODO we shall have a rank for saved settings
            if (strstr(alloc_key->key_name, "Va/")) {
                alloc_key->next = mesh_settings.key_list;
                mesh_settings.key_list = alloc_key;
                return;
            }
            while (cur_key->next) {
                cur_key = cur_key->next;
            }
            cur_key->next = alloc_key;
        }
    }
}

void mesh_settings_load(void)
{
    struct settings_key_cb *cur_key;

    nvds_find_keys_by_namespace(NULL, MESH_NAME_SPACE, mesh_settings_found_keys_cb);
    cur_key = mesh_settings.key_list;

    while (cur_key) {
        //printf("load key name %s, length %d \r\n", cur_key->key_name, cur_key->val_len);
        int len = settings_name_next(cur_key->key_name, NULL);
#if (CONFIG_MESH_CB_REGISTERED)
        struct settings_handler_static *p_cur_settings_cb = p_settings_handlers;

        while (p_cur_settings_cb) {
            //printf("load settings name %s \r\n", p_cur_settings_cb->name);
            if (len == strlen(p_cur_settings_cb->name) &&
                strncmp(p_cur_settings_cb->name, cur_key->key_name, len) == 0) {
                if (p_cur_settings_cb->h_set) {
                    const char *key_next = NULL;
                    settings_name_next(cur_key->key_name, &key_next);

                    p_cur_settings_cb->h_set(key_next, cur_key->val_len, bt_mesh_settings_read_cb, cur_key->key_name);
                }
            }
            p_cur_settings_cb = p_cur_settings_cb->next;
        }
#else
        struct settings_handler_static *p_cur_settings_cb = (struct settings_handler_static *)_settings_cbs;

        while (p_cur_settings_cb != (struct settings_handler_static *)_esettings_cbs) {
            //printf("load settings name %s \r\n", p_cur_settings_cb->name);
            if (len == strlen(p_cur_settings_cb->name) &&
                strncmp(p_cur_settings_cb->name, cur_key->key_name, len) == 0) {
                if (p_cur_settings_cb->h_set) {
                    const char *key_next = NULL;
                    settings_name_next(cur_key->key_name, &key_next);

                    p_cur_settings_cb->h_set(key_next, cur_key->val_len, bt_mesh_settings_read_cb, cur_key->key_name);
                }
            }
            p_cur_settings_cb++;
        }
#endif
        cur_key = cur_key->next;
        sys_mfree(mesh_settings.key_list);
        mesh_settings.key_list = cur_key;
    }

    mesh_settings.key_list = NULL;
}

void bt_mesh_settings_init(void)
{
#if (CONFIG_MESH_CB_REGISTERED)
    bt_mesh_settings_cb_register(&settings_handler_bt_mesh);
    bt_mesh_access_settings_init();
    bt_mesh_app_key_settings_init();
    bt_mesh_cdb_settings_init();
    bt_mesh_cfg_settings_init();
    bt_mesh_hb_pub_settings_init();
    bt_mesh_net_settings_init();

#if (CONFIG_BT_MESH_SOLICITATION)
    bt_mesh_sol_settings_init();
#endif
    bt_mesh_subnet_settings_init();
    bt_mesh_va_settings_init();
    bt_mesh_dfu_slot_settings_init();
#if (CONFIG_BT_MESH_RPL_STORAGE_MODE_SETTINGS)
    bt_mesh_rpl_settings_init();
#endif
#if (CONFIG_BT_MESH_BRG_CFG_SRV)
    bt_mesh_brg_cfg_settings_init();
#endif
#endif

#if CONFIG_BT_MESH_SETTINGS_WORKQ
    int32_t status = sys_sema_init(&mesh_settings.list_sema, 0);

    if (sys_sema_init(&mesh_settings.list_sema, 0) != OS_OK) {
        LOG_ERR("bt_mesh_settings_init sema init fail");
        return;
    }

    if (sys_mutex_init(&mesh_settings.mutex) != OS_OK) {
        LOG_ERR("bt_mesh_settings_init mutex init fail");
        return;
    }

    mesh_settings.task_handle = sys_task_create_dynamic((const uint8_t *)"mesh settings task", CONFIG_BT_MESH_SETTINGS_WORKQ_STACK_SIZE,
                                                        OS_TASK_PRIORITY(CONFIG_BT_MESH_SETTINGS_WORKQ_PRIO), mesh_settings_task, NULL);

    if (mesh_settings.task_handle == NULL) {
        LOG_ERR("bt_mesh_settings_init mesh task create fail");
    }

    sys_timer_init(&mesh_settings.delay_timer, (const uint8_t *)"", 1, 0, mesh_settings_work_timeout,
                   NULL);

#else
    k_work_init_delayable(&pending_store, store_pending);
#endif

    mesh_settings_load();

    mesh_commit();
}

void bt_mesh_settings_store_pending(void)
{
#if CONFIG_BT_MESH_SETTINGS_WORKQ
    sys_mutex_get(&mesh_settings.mutex);

    if (mesh_settings.flags & K_WORK_DELAYABLE) {
        mesh_settings.flags &= ~(K_WORK_DELAYABLE);
        sys_timer_stop(&mesh_settings.delay_timer, false);
    }

    mesh_settings.flags &= ~(BIT(K_WORK_QUEUED_BIT));
    sys_mutex_put(&mesh_settings.mutex);
    store_pending(NULL);
#else
    (void)k_work_cancel_delayable(&pending_store);
    store_pending(&pending_store.work);
#endif

}

int settings_save_one(const char *name, const void *value, size_t val_len)
{
    return nvds_data_put(NULL, MESH_NAME_SPACE, name, (uint8_t *)value, (uint32_t)val_len);
}

int settings_delete(const char *name)
{
    int err = nvds_data_del(NULL, MESH_NAME_SPACE, name);
    if (err == NVDS_E_NOT_FOUND) {
        return NVDS_OK;
    }
    return err;
}

int settings_name_next(const char *name, const char **next)
{
    int rc = 0;

    if (next) {
        *next = NULL;
    }

    if (!name) {
        return 0;
    }

    /* name might come from flash directly, in flash the name would end
     * with '=' or '\0' depending how storage is done. Flash reading is
     * limited to what can be read
     */
    while ((*name != '\0') && (*name != SETTINGS_NAME_END) &&
           (*name != SETTINGS_NAME_SEPARATOR)) {
        rc++;
        name++;
    }

    if (*name == SETTINGS_NAME_SEPARATOR) {
        if (next) {
            *next = name + 1;
        }
        return rc;
    }

    return rc;
}

int settings_load_subtree_direct(const char            *subtree, settings_load_direct_cb cb,
                                 void *param)
{
    uint32_t len = 0;
    if (!cb) {
        return -EINVAL;
    }

    if (nvds_data_get(NULL, MESH_NAME_SPACE, subtree, NULL, &len) != NVDS_OK) {
        return -ESRCH;
    }

    return cb(subtree, len, bt_mesh_settings_read_cb, (void *)subtree, param);
}


#if (CONFIG_MESH_CB_REGISTERED)
void bt_mesh_settings_cb_register(struct settings_handler_static *p_settings_cb)
{
    struct settings_handler_static *p_cur_settings_cb = NULL;

    if (p_settings_cb) {
        p_settings_cb->next = NULL;
        if (p_settings_handlers == NULL) {
            p_settings_handlers = p_settings_cb;
        } else {
            p_cur_settings_cb = p_settings_handlers;
            while (p_cur_settings_cb->next != NULL) {
                p_cur_settings_cb = p_cur_settings_cb->next;
            }
            p_cur_settings_cb->next = p_settings_cb;
        }
    }
}
#endif
#else
int settings_save_one(const char *name, const void *value, size_t val_len)
{
    return NVDS_E_FAIL;
}

int settings_delete(const char *name)
{
    return NVDS_E_FAIL;
}
#endif // CONFIG_BT_SETTINGS
