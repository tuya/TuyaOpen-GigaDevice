/*!
    \file    adv.c
    \brief   Implementation of BLE mesh advertising adapter.

    \version 2024-05-24, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2023, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include "mesh_cfg.h"
#include "net/buf.h"
#include "api/mesh.h"
#include "bluetooth/bt_str.h"
#include "net.h"
#include "statistic.h"
#include "src/solicitation.h"
#include "prov.h"

#include <stdint.h>
#include "ble_gap.h"
#include "ble_adv.h"
#include "ble_adv_data.h"
#include "wrapper_os.h"
#include "ble_utils.h"
#include "ble_init.h"
#include "proxy.h"
#include "pb_gatt_srv.h"

#define LOG_LEVEL CONFIG_BT_MESH_ADV_LOG_LEVEL
#include "api/mesh_log.h"

struct ble_mesh_adv_msg {
    void *arg;
};

struct ble_mesh_adv_env {
    uint8_t             start_flag;
    uint8_t             ad[BLE_GAP_LEGACY_ADV_MAX_LEN];
    uint16_t            ad_len;
    uint8_t             sd[BLE_GAP_LEGACY_ADV_MAX_LEN];
    uint16_t            sd_len;
    ble_adv_state_t     adv_state;
    uint8_t             adv_idx;
    os_sema_t           sema;
    uint16_t            duration;
    uint16_t            reason;
    uint8_t             gatt_flag;
    uint8_t             gatt_start_pending;
    uint8_t             gatt_stop_pending;
    struct bt_mesh_adv  *adv;
};

#define BLE_MESH_ADV_QUEUE_SIZE     16

const uint8_t bt_mesh_adv_type[BT_MESH_ADV_TYPES] = {
    [BT_MESH_ADV_PROV]   = BLE_AD_TYPE_MESH_PROV,
    [BT_MESH_ADV_DATA]   = BLE_AD_TYPE_MESH_MESSAGE,
    [BT_MESH_ADV_BEACON] = BLE_AD_TYPE_MESH_BEACON,
    [BT_MESH_ADV_URI]    = BLE_AD_TYPE_URI,
};

struct ble_mesh_adv_env g_adv_env = {0};

static os_task_t bt_mesh_adv_task = NULL;

static os_queue_t bt_mesh_adv_queue;

static bool bt_adv_enabled;

static struct ble_mesh_adv_env *ble_mesh_adv_env_get(void)
{
    return &g_adv_env;
}

/** @brief Send an advertising message start callback.
 */
void bt_mesh_adv_send_start(uint16_t duration, int err, struct bt_mesh_adv_ctx *ctx)
{
    if (!ctx->started) {
        ctx->started = 1;

        if (ctx->cb && ctx->cb->start) {
            ctx->cb->start(duration, err, ctx->cb_data);
        }

        if (err) {
            ctx->cb = NULL;
            LOG_ERR("duration %d, err %d", duration, err);
        } else if (IS_ENABLED(CONFIG_BT_MESH_STATISTIC)) {
            bt_mesh_stat_succeeded_count(ctx);
        }
    }
}

/** @brief Send an advertising message end callback.
 */
void bt_mesh_adv_send_end(int err, struct bt_mesh_adv_ctx *ctx)
{
    if (ctx->started && ctx->cb && ctx->cb->end) {
        ctx->cb->end(err, ctx->cb_data);
    }
    ctx->started = 0;
}

struct bt_mesh_adv *bt_mesh_adv_ref(struct bt_mesh_adv *adv)
{
    __ASSERT_NO_MSG(adv->__ref < 0xff);

    adv->__ref++;

    return adv;
}

void bt_mesh_adv_unref(struct bt_mesh_adv *adv)
{
    __ASSERT_NO_MSG(adv->__ref > 0);

    if (--adv->__ref > 0) {
        return;
    }

    sys_mfree((void *)adv);
}

struct bt_mesh_adv *bt_mesh_adv_create(enum bt_mesh_adv_type type, enum bt_mesh_adv_tag tag, uint8_t xmit,
                                            k_timeout_t timeout)
{
    struct bt_mesh_adv_ctx *ctx;
    struct bt_mesh_adv *adv;

    if (atomic_test_bit(bt_mesh.flags, BT_MESH_SUSPENDED)) {
        LOG_WRN("Refusing to allocate buffer while suspended");
        return NULL;
    }

    adv = sys_malloc(sizeof(struct bt_mesh_adv));
    if (adv == NULL) {
        return NULL;
    }

    adv->__ref = 1;

    net_buf_simple_init_with_data(&adv->b, adv->__bufs, BT_MESH_ADV_DATA_SIZE);
    net_buf_simple_reset(&adv->b);

    ctx = &adv->ctx;

    sys_memset(ctx, 0, sizeof(*ctx));

    ctx->type = type;
    ctx->tag  = tag;
    ctx->xmit = xmit;

    return adv;
}

void bt_mesh_adv_send(struct bt_mesh_adv *adv, const struct bt_mesh_send_cb *cb, void *cb_data)
{
    struct ble_mesh_adv_msg msg;
    LOG_DUMP("send type 0x%02x len %u: %s", adv->ctx.type, adv->b.len, bt_hex(adv->b.data, adv->b.len));

    if (atomic_test_bit(bt_mesh.flags, BT_MESH_SUSPENDED)) {
        LOG_WRN("Sending advertisement while suspended");
    }

    adv->ctx.cb = cb;
    adv->ctx.cb_data = cb_data;
    adv->ctx.busy = 1U;

    if (IS_ENABLED(CONFIG_BT_MESH_STATISTIC)) {
        bt_mesh_stat_planned_count(&adv->ctx);
    }

    msg.arg = (void *)bt_mesh_adv_ref(adv);

    sys_queue_write(&bt_mesh_adv_queue, &msg, 0, false);
}

/** @brief If supported proxy server or pb-gatt server, will send connectable advertising.
 */
static int bt_mesh_adv_gatt_send(void)
{
    struct ble_mesh_adv_env *adv_env = ble_mesh_adv_env_get();

    if (!bt_adv_enabled) {
        return SYS_FOREVER_MS;
    }

    if (adv_env->gatt_flag == 1) {
        adv_env->gatt_start_pending = 1;
        return adv_env->duration == 0 ? SYS_FOREVER_MS : adv_env->duration;
    }

    if (bt_mesh_is_provisioned()) {
        if (IS_ENABLED(CONFIG_BT_MESH_PROXY_SOLICITATION)) {
            int duration = bt_mesh_sol_send();
            if (duration > 0) {
                return duration;
            }
        }

        if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY)) {
            LOG_DBG("Proxy Advertising");
            return bt_mesh_proxy_adv_start();
        }
    } else if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
        LOG_DBG("PB-GATT Advertising");
        return bt_mesh_pb_gatt_srv_adv_start();
    }

    return SYS_FOREVER_MS;
}

void bt_mesh_adv_gatt_update(void)
{
    struct ble_mesh_adv_msg msg = {
        .arg = NULL
    };

    if (atomic_test_bit(bt_mesh.flags, BT_MESH_SUSPENDED)) {
        LOG_WRN("Sending advertisement while suspended");
    }

    LOG_DBG("");

    sys_queue_write(&bt_mesh_adv_queue, &msg, 0, false);
}

static int bt_mesh_adv_env_reset(struct ble_mesh_adv_env *adv_env)
{
    if (adv_env->adv != NULL) {
        if (adv_env->start_flag == 0) {
            bt_mesh_adv_send_start(adv_env->duration, adv_env->reason, &adv_env->adv->ctx);
        } else {
            bt_mesh_adv_send_end(adv_env->reason, &adv_env->adv->ctx);
        }

        bt_mesh_adv_unref(adv_env->adv);
    }

    adv_env->adv_idx = BLE_ADV_INVALID_IDX;
    adv_env->start_flag = 0;
    adv_env->ad_len = 0;
    adv_env->sd_len = 0;
    adv_env->adv = NULL;
    adv_env->gatt_flag = 0;
    adv_env->gatt_stop_pending = 0;
    if (adv_env->gatt_start_pending) {
        adv_env->gatt_start_pending = 0;
        bt_mesh_adv_gatt_update();
    }

    return 0;
}

static void bt_mesh_adv_evt_hdlr(ble_adv_evt_t adv_evt, void *p_data, void *p_context)
{
    ble_adv_data_set_t adv;
    ble_adv_data_set_t scan_rsp;
    ble_data_t adv_data;
    ble_data_t adv_scanrsp_data;
    struct ble_mesh_adv_env *adv_env = (struct ble_mesh_adv_env *)p_context;

    if (adv_evt == BLE_ADV_EVT_STATE_CHG) {
        ble_adv_state_chg_t *p_chg = (ble_adv_state_chg_t *)p_data;
        ble_adv_state_t old_state = adv_env->adv_state;

        LOG_DBG("adv state change 0x%x ==> 0x%x, reason 0x%x", old_state, p_chg->state, p_chg->reason);
        adv_env->adv_state = p_chg->state;

        if (p_chg->state == BLE_ADV_STATE_IDLE) {
            if (old_state == BLE_ADV_STATE_CREATING) {
                LOG_ERR("adv create error, state change 0x%x ==> 0x%x, reason 0x%x", old_state, p_chg->state, p_chg->reason);
            }

            adv_env->reason = (p_chg->reason != BLE_ERR_NO_ERROR) && (adv_env->reason == BLE_ERR_NO_ERROR) ? p_chg->reason : adv_env->reason;
            bt_mesh_adv_env_reset(adv_env);
            sys_sema_up(&adv_env->sema);
            return;
        }

        if (p_chg->reason != BLE_ERR_NO_ERROR && p_chg->reason != BLE_GAP_ERR_TIMEOUT) {
            adv_env->reason = adv_env->reason == BLE_ERR_NO_ERROR ? p_chg->reason : adv_env->reason;
            LOG_ERR("adv error, state change 0x%x ==> 0x%x, reason 0x%x", old_state, p_chg->state, p_chg->reason);
            ble_adv_remove(p_chg->adv_idx);
            return;
        }

        if (adv_env->gatt_stop_pending != 0 && (p_chg->state == BLE_ADV_STATE_START)) {
            ble_adv_stop(adv_env->adv_idx);
            return;
        }

        if ((p_chg->state == BLE_ADV_STATE_CREATE) && (old_state == BLE_ADV_STATE_CREATING)) {
            adv_data.len = adv_env->ad_len;
            adv_data.p_data = adv_env->ad;
            adv.data_force = true;
            adv.data.p_data_force = &adv_data;

            if (adv_env->sd_len > 0) {
                adv_scanrsp_data.len = adv_env->sd_len;
                adv_scanrsp_data.p_data = adv_env->sd;
                scan_rsp.data_force = true;
                scan_rsp.data.p_data_force = &adv_scanrsp_data;
            }

            if (ble_adv_start(p_chg->adv_idx, &adv, adv_env->sd_len == 0 ? NULL : &scan_rsp, NULL)) {
                LOG_ERR("adv start error");
                ble_adv_remove(p_chg->adv_idx);
                return;
            }

            adv_env->adv_idx = p_chg->adv_idx;
            adv_env->start_flag = 1;

            if (adv_env->adv != NULL)
                bt_mesh_adv_send_start(adv_env->duration, p_chg->reason, &adv_env->adv->ctx);
        } else if ((p_chg->state == BLE_ADV_STATE_CREATE) && (old_state == BLE_ADV_STATE_START)) {
            ble_adv_remove(p_chg->adv_idx);
        }
    }
}

/** @brief Call the host api to send an advertising.
 */
static int bt_mesh_adv_start(struct ble_mesh_adv_env *adv_env, struct ble_mesh_adv_param_t *param,
                           const struct bt_data *ad, uint8_t ad_len, const struct bt_data *sd, uint8_t sd_len)
{
    uint8_t i;
    ble_adv_param_t adv_param = {0};

    if (adv_env == NULL || param == NULL || ad == NULL || ad_len == 0) {
        LOG_ERR("param error");
        return -1;
    }

    adv_param.param.type = BLE_GAP_ADV_TYPE_LEGACY;
    adv_param.param.ch_map = BLE_GAP_ADV_CHANN_37 | BLE_GAP_ADV_CHANN_38 | BLE_GAP_ADV_CHANN_39;
    adv_param.param.primary_phy = BLE_GAP_PHY_1MBPS;
    adv_param.param.own_addr_type = param->own_addr_type;
    adv_param.param.prop = param->prop;
    adv_param.param.adv_intv_min = MAX(param->interval_min, BLE_GAP_EXT_ADV_INTERVAL_MIN);
    adv_param.param.adv_intv_max = MIN(MAX(param->interval_max, BLE_GAP_EXT_ADV_INTERVAL_MIN), BLE_GAP_EXT_ADV_INTERVAL_MAX);
    adv_param.param.max_adv_evt = param->max_adv_evt;
    adv_param.param.duration = (param->timeout + 9) / 10 /* ms to 10ms */;

    adv_env->ad_len = 0;
    for (i = 0; i < ad_len; i++) {
        if (ad[i].data_len + adv_env->ad_len + AD_DATA_HDR_SIZE > BLE_GAP_LEGACY_ADV_MAX_LEN) {
            LOG_ERR("ad len error");
            return -2;
        }

        adv_env->ad[adv_env->ad_len] = ad[i].data_len + 1;
        adv_env->ad[adv_env->ad_len + 1] = ad[i].type;
        sys_memcpy(&adv_env->ad[adv_env->ad_len + 2], ad[i].data, ad[i].data_len);
        adv_env->ad_len += ad[i].data_len + AD_DATA_HDR_SIZE;
    }

    adv_env->sd_len = 0;
    for (i = 0; i < sd_len; i++) {
        if (sd[i].data_len + adv_env->sd_len + AD_DATA_HDR_SIZE > BLE_GAP_LEGACY_ADV_MAX_LEN) {
            LOG_ERR("sd len error");
            return -3;
        }

        adv_env->sd[adv_env->sd_len] = sd[i].data_len + 1;

        adv_env->sd[adv_env->sd_len + 1] = sd[i].type;
        sys_memcpy(&adv_env->sd[adv_env->sd_len + 2], sd[i].data, sd[i].data_len);
        adv_env->sd_len += sd[i].data_len + AD_DATA_HDR_SIZE;
    }

    LOG_DUMP("len %u: %s", adv_env->ad_len, bt_hex(adv_env->ad, adv_env->ad_len));

    return ble_adv_create(&adv_param, bt_mesh_adv_evt_hdlr, (void *)adv_env);
}

static int bt_adv_send(struct bt_mesh_adv *adv)
{
    struct ble_mesh_adv_env *adv_env = ble_mesh_adv_env_get();
    struct ble_mesh_adv_param_t param = {0};
    struct bt_data ad;
    int ret;

    sys_sema_down(&adv_env->sema, 0);

    param.own_addr_type = adv->ctx.priv ? BLE_GAP_LOCAL_ADDR_NONE_RESOLVABLE : BLE_GAP_LOCAL_ADDR_STATIC;
    param.prop = BLE_GAP_ADV_PROP_NON_CONN_NON_SCAN;
    param.interval_min = BLE_GAP_ADV_SCAN_UNIT(BT_MESH_TRANSMIT_INT(adv->ctx.xmit));
    param.interval_max = param.interval_min + BLE_GAP_ADV_SCAN_UNIT(10);
    param.max_adv_evt = BT_MESH_TRANSMIT_COUNT(adv->ctx.xmit) + 1;

    ad.type = bt_mesh_adv_type[adv->ctx.type];
    ad.data_len = adv->b.len;
    ad.data = adv->b.data;

    adv_env->adv = adv;
    adv_env->duration = BT_MESH_TRANSMIT_INT(adv->ctx.xmit) * param.max_adv_evt;

    ret = bt_mesh_adv_start(adv_env, &param, &ad, 1, NULL, 0);
    if (ret != 0) {
        LOG_ERR("adv start error: %d", ret);
        adv_env->reason = ret;
        bt_mesh_adv_env_reset(adv_env);
        sys_sema_up(&adv_env->sema);
        return -1;
    }

    bt_mesh_adv_ref(adv);
    return 0;
}

int bt_mesh_adv_bt_data_send(uint8_t num_events, uint16_t adv_interval, const struct bt_data *ad, size_t ad_len)
{
    struct ble_mesh_adv_env *adv_env = ble_mesh_adv_env_get();
    struct ble_mesh_adv_param_t param = {0};

    sys_sema_down(&adv_env->sema, 0);

    param.own_addr_type = BLE_GAP_LOCAL_ADDR_STATIC;
    param.prop = BLE_GAP_ADV_PROP_NON_CONN_NON_SCAN;
    param.interval_min = BLE_GAP_ADV_SCAN_UNIT(adv_interval);
    param.interval_max = param.interval_min + BLE_GAP_ADV_SCAN_UNIT(10);
    param.max_adv_evt = num_events;

    return bt_mesh_adv_start(adv_env, &param, ad, ad_len, NULL, 0);
}

int bt_mesh_adv_gatt_start(struct ble_mesh_adv_param_t *param, const struct bt_data *ad, size_t ad_len,
                                 const struct bt_data *sd, size_t sd_len)
{
    struct ble_mesh_adv_env *adv_env = ble_mesh_adv_env_get();

    sys_sema_down(&adv_env->sema, 0);
    adv_env->gatt_flag = 1;
    adv_env->duration = param->timeout;

    return bt_mesh_adv_start(adv_env, param, ad, ad_len, sd, sd_len);
}

static int bt_mesh_adv_gatt_stop(void)
{
    struct ble_mesh_adv_env *adv_env = ble_mesh_adv_env_get();

    if (adv_env->gatt_flag == 0) {
        return 0;
    }

    if (adv_env->adv_state == BLE_ADV_STATE_START) {
        return ble_adv_stop(adv_env->adv_idx);
    } else {
        adv_env->gatt_stop_pending = 1;
    }

    return 0;
}

static void bt_mesh_adv_thread(void *param)
{
    struct ble_mesh_adv_msg msg = {0};
    struct bt_mesh_adv **adv = (struct bt_mesh_adv **)&msg.arg;
    int timeout;

    ble_wait_ready();

    for (;;) {
        *adv = NULL;

        sys_queue_read(&bt_mesh_adv_queue, &msg, 0, false);
        while (!(*adv)) {
            timeout = bt_mesh_adv_gatt_send();
            sys_queue_read(&bt_mesh_adv_queue, &msg, timeout, false);
            bt_mesh_adv_gatt_stop();
        }

        if (*adv == NULL) {
            continue;
        }

        if (!bt_adv_enabled) {
            bt_mesh_adv_unref(*adv);
            continue;
        }

        /* busy == 0 means this was canceled */
        if (!(*adv)->ctx.busy) {
            bt_mesh_adv_unref(*adv);
            continue;
        }

        (*adv)->ctx.busy = 0U;
        bt_adv_send(*adv);
        bt_mesh_adv_unref(*adv);
    }
}

int bt_mesh_adv_enable(void)
{
    bt_adv_enabled = true;

    bt_mesh_adv_gatt_update();

    return 0;
}

int bt_mesh_adv_disable(void)
{
    bt_adv_enabled = false;

    bt_mesh_adv_gatt_update();

    return 0;
}

int bt_mesh_adv_terminate(struct bt_mesh_adv *adv)
{
    struct ble_mesh_adv_env *adv_env = ble_mesh_adv_env_get();

    if (adv == NULL || adv_env->adv != adv) {
        return 0;
    }

    /* Do not call `cb:end`, since this user action */
    adv->ctx.cb = NULL;

    if (adv_env->adv_state == BLE_ADV_STATE_START) {
        return ble_adv_stop(adv_env->adv_idx);
    }

    return 0;
}

int bt_mesh_adv_init(void)
{
    if (sys_queue_init(&bt_mesh_adv_queue, BLE_MESH_ADV_QUEUE_SIZE, sizeof(struct ble_mesh_adv_msg))) {
        return -1;
    }

    sys_sema_init_ext(&g_adv_env.sema, 1, 1);

    /* The adv task must have a higher priority than the app task to avoid parallel processing of the adv task and the adv event handler */
    bt_mesh_adv_task = sys_task_create_dynamic((const uint8_t *)"BLE mesh adv", CONFIG_BT_MESH_ADV_STACK_SIZE,
                                                OS_TASK_PRIORITY(CONFIG_BT_MESH_ADV_PRIO), bt_mesh_adv_thread, NULL);
    if (bt_mesh_adv_task == NULL) {
        sys_queue_free(&bt_mesh_adv_queue);
        sys_sema_free(&g_adv_env.sema);
        return -2;
    }

    return 0;
}

