/*!
    \file    generic_server.c
    \brief   Implementation of BLE mesh generic server.

    \version 2024-09-09, V1.0.2, firmware for GD32VW55x
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

#include "app_cfg.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mesh_cfg.h"
#include "api/mesh.h"
#include "mesh_kernel.h"

#include "models.h"
#include "model_utils.h"
#include "transition.h"
#include "generic_server.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include "api/mesh_log.h"

static int gen_onoff_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    struct bt_mesh_gen_onoff_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_ONOFF_STATUS, 3);
    bt_mesh_model_msg_init(&msg, OP_GEN_ONOFF_STATUS);
    net_buf_simple_add_u8(&msg, srv->state.onoff);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_u8(&msg, srv->state.target_onoff);
        net_buf_simple_add_u8(&msg, srv->transition.remain_time);
    }

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int gen_onoff_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    struct bt_mesh_gen_onoff_srv *srv = model->rt->user_data;

    if (srv->cb != NULL && srv->cb->get != NULL)
        srv->cb->get(srv->cb->user_data, BT_MESH_SRV_GEN_ONOFF_EVT, &srv->state);

    return gen_onoff_status_send(model, ctx);
}

static int gen_onoff_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint8_t onoff, tid;
    struct bt_mesh_gen_onoff_srv *srv = model->rt->user_data;

    if ((buf->len != 2U) && (buf->len != 4U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    onoff = net_buf_simple_pull_u8(buf);
    if (onoff > 1) {
        LOG_ERR("Invalid OnOff value %u", onoff);
        return -EINVAL;
    }

    tid = net_buf_simple_pull_u8(buf);
    if (bt_mesh_tid_check_and_update(&srv->pre_tid, tid, ctx->addr, ctx->recv_dst))
        return 0;

    bt_mesh_server_stop_transition(&srv->transition);
    srv->state.target_onoff = onoff;

    if (srv->cb != NULL && srv->cb->set != NULL)
        srv->cb->set(srv->cb->user_data, BT_MESH_SRV_GEN_ONOFF_EVT, srv);

    if (srv->state.target_onoff == srv->state.onoff)
        return 0;

    bt_mesh_srv_transition_get(model, &srv->transition, buf);
    LOG_DBG("onoff:%u transition_time:%u delay:%u", onoff, srv->transition.transition_time, srv->transition.delay);

    set_transition_values(&srv->transition);
    bt_mesh_server_start_transition(&srv->transition);

    return 0;
}

static int gen_onoff_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    int ret = gen_onoff_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return gen_onoff_status_send(model, ctx);
}

const struct bt_mesh_model_op bt_mesh_gen_onoff_srv_op[] = {
    { OP_GEN_ONOFF_GET,                 BT_MESH_LEN_EXACT(0),       gen_onoff_get },
    { OP_GEN_ONOFF_SET,                 BT_MESH_LEN_MIN(2),         gen_onoff_set },
    { OP_GEN_ONOFF_SET_UNACK,           BT_MESH_LEN_MIN(2),         gen_onoff_set_unack },
    BT_MESH_MODEL_OP_END,
};

static int gen_onoff_pub_update(const struct bt_mesh_model *model)
{
    struct bt_mesh_gen_onoff_srv *srv = model->rt->user_data;

    bt_mesh_model_msg_init(model->pub->msg, OP_GEN_ONOFF_STATUS);
    net_buf_simple_add_u8(model->pub->msg, srv->state.onoff);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_u8(model->pub->msg, srv->state.target_onoff);
        net_buf_simple_add_u8(model->pub->msg, srv->transition.remain_time);
    }

    return 0;
}

void gen_onoff_config(struct bt_mesh_gen_onoff_srv *srv, uint8_t onoff)
{
    LOG_INF("%u", onoff);
    srv->state.onoff = onoff;
}

void gen_onoff_status_publish(const struct bt_mesh_model *model)
{
    LOG_INF("");
    gen_onoff_pub_update(model);
    bt_mesh_model_publish(model);
}

static void gen_onoff_work_handler(struct k_work *work)
{
    struct bt_mesh_gen_onoff_srv *srv = CONTAINER_OF(work, struct bt_mesh_gen_onoff_srv, transition.timer.work);

    LOG_DBG("just_started:%u counter:%u, quo_tt:%u", srv->transition.just_started, srv->transition.counter,
        srv->transition.quo_tt);

    if (srv->transition.just_started) {
        srv->transition.just_started = false;

        if (srv->transition.counter == 0U) {
            gen_onoff_config(srv, srv->state.target_onoff);
            gen_onoff_status_publish(srv->model);
            if (srv->cb != NULL && srv->cb->state_change != NULL)
                srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_GEN_ONOFF_EVT, &srv->state);
        } else {
            srv->transition.start_timestamp = k_uptime_get();
            k_work_reschedule(&srv->transition.timer, K_MSEC(srv->transition.quo_tt));
        }
        return;
    }

    if (srv->transition.counter == 0 || (--srv->transition.counter) == 0) {
        gen_onoff_config(srv, srv->state.target_onoff);
        gen_onoff_status_publish(srv->model);
        if (srv->cb != NULL && srv->cb->state_change != NULL)
            srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_GEN_ONOFF_EVT, &srv->state);
    } else {
        k_work_reschedule(&srv->transition.timer, K_MSEC(srv->transition.quo_tt));
    }
}

static int bt_mesh_gen_onoff_srv_init(const struct bt_mesh_model *model)
{
    LOG_INF("");
    struct bt_mesh_gen_onoff_srv *srv = model->rt->user_data;
    if (!srv) {
        LOG_ERR("No Server context provided");
        return -EINVAL;
    }

    if (!model->pub) {
        LOG_ERR("No publication support");
        return -EINVAL;
    }

    srv->model = model;
    model->pub->update = gen_onoff_pub_update;

    k_work_init_delayable(&srv->transition.timer, gen_onoff_work_handler);
    return 0;
}

const struct bt_mesh_model_cb bt_mesh_gen_onoff_srv_cb = {
    .init = bt_mesh_gen_onoff_srv_init,
};

static int gen_level_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    struct bt_mesh_gen_level_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_LEVEL_STATUS, 5);
    bt_mesh_model_msg_init(&msg, OP_GEN_LEVEL_STATUS);
    net_buf_simple_add_le16(&msg, srv->state.level);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_le16(&msg, srv->state.target_level);
        net_buf_simple_add_u8(&msg, srv->transition.remain_time);
    }

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int gen_level_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    struct bt_mesh_gen_onoff_srv *srv = model->rt->user_data;

    if (srv->cb != NULL && srv->cb->get != NULL)
        srv->cb->get(srv->cb->user_data, BT_MESH_SRV_GEN_LEVEL_EVT, &srv->state);

    return gen_level_status_send(model, ctx);
}

static int gen_level_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    int16_t level;
    uint8_t tid;
    struct bt_mesh_gen_level_srv *srv = model->rt->user_data;

    if ((buf->len != 3U) && (buf->len != 5U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    level = net_buf_simple_pull_le16(buf);
    tid = net_buf_simple_pull_u8(buf);

    if (bt_mesh_tid_check_and_update(&srv->pre_tid, tid, ctx->addr, ctx->recv_dst))
        return 0;

    bt_mesh_server_stop_transition(&srv->transition);
    srv->state.target_level = level;

    if (srv->cb != NULL && srv->cb->set != NULL)
        srv->cb->set(srv->cb->user_data, BT_MESH_SRV_GEN_LEVEL_EVT, srv);

    if (srv->state.target_level == srv->state.level)
        return 0;

    bt_mesh_srv_transition_get(model, &srv->transition, buf);
    LOG_DBG("level:%d transition_time:%u delay:%u", level, srv->transition.transition_time, srv->transition.delay);

    set_transition_values(&srv->transition);
    srv->state.delta_level = ((float)(srv->state.target_level - srv->state.level) / srv->transition.counter);
    bt_mesh_server_start_transition(&srv->transition);
    return 0;
}

static int gen_level_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    int ret = gen_level_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return gen_level_status_send(model, ctx);
}

static int gen_delta_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint8_t tid;
    int32_t delta_level;
    struct bt_mesh_gen_level_srv *srv = model->rt->user_data;

    if ((buf->len != 5U) && (buf->len != 7U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    delta_level = net_buf_simple_pull_le32(buf);
    tid = net_buf_simple_pull_u8(buf);

    if (bt_mesh_tid_check_and_update(&srv->pre_tid, tid, ctx->addr, ctx->recv_dst) != 0) {
        srv->state.target_level = CLAMP(srv->state.last_level + delta_level, (int16_t)0x8000, (int16_t)0x7FFF);
    } else {
        srv->state.last_level = srv->state.level;
        srv->state.target_level = CLAMP(srv->state.level + delta_level, (int16_t)0x8000, (int16_t)0x7FFF);
    }

    bt_mesh_server_stop_transition(&srv->transition);

    if (srv->cb != NULL && srv->cb->set != NULL)
        srv->cb->set(srv->cb->user_data, BT_MESH_SRV_GEN_LEVEL_EVT, srv);

    if (srv->state.target_level == srv->state.level)
        return 0;

    bt_mesh_srv_transition_get(model, &srv->transition, buf);
    LOG_DBG("delta:%d transition_time:%u delay:%u", delta_level, srv->transition.transition_time, srv->transition.delay);

    set_transition_values(&srv->transition);
    srv->state.delta_level = ((float) (srv->state.target_level - srv->state.level) / srv->transition.counter);
    bt_mesh_server_start_transition(&srv->transition);

    return 0;
}

static int gen_delta_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    int ret = gen_delta_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return gen_level_status_send(model, ctx);
}

static int gen_move_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint8_t tid;
    int16_t delta_level;
    struct bt_mesh_gen_level_srv *srv = model->rt->user_data;

    if ((buf->len != 3U) && (buf->len != 5U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    delta_level = net_buf_simple_pull_le16(buf);
    if (delta_level == 0)
        return 0;

    tid = net_buf_simple_pull_u8(buf);
    if (bt_mesh_tid_check_and_update(&srv->pre_tid, tid, ctx->addr, ctx->recv_dst))
        return 0;

    if (delta_level > 0) {
        srv->state.target_level = (int16_t)0x7FFF;
    } else {
        srv->state.target_level = (int16_t)0x8000;
    }

    bt_mesh_server_stop_transition(&srv->transition);

    if (srv->cb != NULL && srv->cb->set != NULL)
        srv->cb->set(srv->cb->user_data, BT_MESH_SRV_GEN_LEVEL_EVT, srv);

    if (srv->state.target_level == srv->state.level)
        return 0;

    bt_mesh_srv_transition_get(model, &srv->transition, buf);
    LOG_INF("delta:%d transition_time:%u delay:%u", delta_level, srv->transition.transition_time, srv->transition.delay);

    srv->transition.type = TRANSITION_TYPE_MOVE;
    set_transition_values(&srv->transition);
    srv->state.delta_level = delta_level;
    srv->transition.counter = (srv->state.target_level + delta_level - 1 - srv->state.level) / delta_level;
    bt_mesh_server_start_transition(&srv->transition);

    return 0;
}

static int gen_move_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    int ret = gen_move_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return gen_level_status_send(model, ctx);
}

const struct bt_mesh_model_op bt_mesh_gen_level_srv_op[] = {
    { OP_GEN_LEVEL_GET,                 BT_MESH_LEN_EXACT(0),       gen_level_get },
    { OP_GEN_LEVEL_SET,                 BT_MESH_LEN_MIN(3),         gen_level_set },
    { OP_GEN_LEVEL_SET_UNACK,           BT_MESH_LEN_MIN(3),         gen_level_set_unack },
    { OP_GEN_LEVEL_DELTA_SET,           BT_MESH_LEN_MIN(5),         gen_delta_set },
    { OP_GEN_LEVEL_DELTA_SET_UNACK,     BT_MESH_LEN_MIN(5),         gen_delta_set_unack },
    { OP_GEN_LEVEL_MOVE_SET,            BT_MESH_LEN_MIN(3),         gen_move_set },
    { OP_GEN_LEVEL_MOVE_SET_UNACK,      BT_MESH_LEN_MIN(3),         gen_move_set_unack },
    BT_MESH_MODEL_OP_END,
};

static int gen_level_pub_update(const struct bt_mesh_model *model)
{
    struct bt_mesh_gen_level_srv *srv = model->rt->user_data;

    bt_mesh_model_msg_init(model->pub->msg, OP_GEN_LEVEL_STATUS);
    net_buf_simple_add_le16(model->pub->msg, srv->state.level);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_le16(model->pub->msg, srv->state.target_level);
        net_buf_simple_add_u8(model->pub->msg, srv->transition.remain_time);
    }

    return 0;
}

void gen_level_config(struct bt_mesh_gen_level_srv *srv, int16_t level)
{
    LOG_INF("%d", level);
    srv->state.level = level;
}

void gen_level_status_publish(const struct bt_mesh_model *model)
{
    LOG_INF("");
    gen_level_pub_update(model);
    bt_mesh_model_publish(model);
}

static void gen_level_work_handler(struct k_work *work)
{
    struct bt_mesh_gen_level_srv *srv = CONTAINER_OF(work, struct bt_mesh_gen_level_srv, transition.timer.work);

    LOG_DBG("just_started:%u counter:%u, quo_tt:%u, delta_level:%d", srv->transition.just_started,
        srv->transition.counter, srv->transition.quo_tt, srv->state.delta_level);

    if (srv->transition.just_started) {
        srv->transition.just_started = false;

        if (srv->transition.counter == 0U) {
            gen_level_config(srv, srv->state.target_level);
            gen_level_status_publish(srv->model);

            if (srv->cb != NULL && srv->cb->state_change != NULL)
                srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_GEN_LEVEL_EVT, &srv->state);
        } else {
            srv->transition.start_timestamp = k_uptime_get();
            k_work_reschedule(&srv->transition.timer, K_MSEC(srv->transition.quo_tt));
        }

        return;
    }

    if (srv->transition.counter == 0 || (--srv->transition.counter) == 0) {
        gen_level_config(srv, srv->state.target_level);
        gen_level_status_publish(srv->model);

        if (srv->cb != NULL && srv->cb->state_change != NULL)
            srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_GEN_LEVEL_EVT, &srv->state);
    } else {
        gen_level_config(srv, srv->state.level + srv->state.delta_level);
        gen_level_status_publish(srv->model);
        k_work_reschedule(&srv->transition.timer, K_MSEC(srv->transition.quo_tt));

        if (srv->cb != NULL && srv->cb->state_change != NULL)
            srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_GEN_LEVEL_EVT, &srv->state);
    }
}

static int bt_mesh_gen_level_srv_init(const struct bt_mesh_model *model)
{
    struct bt_mesh_gen_level_srv *srv = model->rt->user_data;

    LOG_INF("");

    if (!srv) {
        LOG_ERR("No Server context provided");
        return -EINVAL;
    }

    if (!model->pub) {
        LOG_ERR("No publication support");
        return -EINVAL;
    }

    srv->model = model;
    model->pub->update = gen_level_pub_update;

    k_work_init_delayable(&srv->transition.timer, gen_level_work_handler);
    return 0;
}

const struct bt_mesh_model_cb bt_mesh_gen_level_srv_cb = {
    .init = bt_mesh_gen_level_srv_init,
};

static int gen_def_trans_time_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    struct bt_mesh_gen_def_trans_time_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_DEF_TRANS_TIME_STATUS, 1);
    bt_mesh_model_msg_init(&msg, OP_GEN_DEF_TRANS_TIME_STATUS);
    net_buf_simple_add_u8(&msg, srv->state.transition_time);

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int gen_def_trans_time_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    return gen_def_trans_time_status_send(model, ctx);
}

static int gen_def_trans_time_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint8_t transition_time;
    struct bt_mesh_gen_def_trans_time_srv *srv = model->rt->user_data;

    transition_time = net_buf_simple_pull_u8(buf);
    if (transition_time == srv->state.transition_time) {
        return 0;
    }

    LOG_DBG("%u", transition_time);

    srv->state.transition_time = transition_time;

    if (srv->cb != NULL && srv->cb->set != NULL)
        srv->cb->set(srv->cb->user_data, BT_MESH_SRV_GEN_DEF_TRANS_TIME_EVT, srv);

    gen_def_trans_time_status_publish(model);
    return 0;
}

static int gen_def_trans_time_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    int ret = gen_def_trans_time_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return gen_def_trans_time_status_send(model, ctx);
}

const struct bt_mesh_model_op bt_mesh_gen_def_trans_time_srv_op[] = {
    { OP_GEN_DEF_TRANS_TIME_GET,        BT_MESH_LEN_EXACT(0),       gen_def_trans_time_get },
    { OP_GEN_DEF_TRANS_TIME_SET,        BT_MESH_LEN_EXACT(1),       gen_def_trans_time_set },
    { OP_GEN_DEF_TRANS_TIME_SET_UNACK,  BT_MESH_LEN_EXACT(1),       gen_def_trans_time_set_unack },
    BT_MESH_MODEL_OP_END,
};

static int gen_def_trans_time_pub_update(const struct bt_mesh_model *model)
{
    struct bt_mesh_gen_def_trans_time_srv *srv = model->rt->user_data;

    bt_mesh_model_msg_init(model->pub->msg, OP_GEN_DEF_TRANS_TIME_STATUS);
    net_buf_simple_add_u8(model->pub->msg, srv->state.transition_time);

    return 0;
}

void gen_def_trans_time_config(struct bt_mesh_gen_def_trans_time_srv *srv, uint8_t transition_time)
{
    LOG_INF("%u", transition_time);
    srv->state.transition_time = transition_time;
}

void gen_def_trans_time_status_publish(const struct bt_mesh_model *model)
{
    LOG_INF("");
    gen_def_trans_time_pub_update(model);
    bt_mesh_model_publish(model);
}

static int bt_mesh_gen_def_trans_time_srv_init(const struct bt_mesh_model *model)
{
    LOG_INF("");
    struct bt_mesh_gen_def_trans_time_srv *srv = model->rt->user_data;
    if (!srv) {
        LOG_ERR("No Server context provided");
        return -EINVAL;
    }

    if (!model->pub) {
        LOG_ERR("No publication support");
        return -EINVAL;
    }

    srv->model = model;
    model->pub->update = gen_def_trans_time_pub_update;

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_gen_def_trans_time_srv_cb = {
    .init = bt_mesh_gen_def_trans_time_srv_init,
};

static int gen_onpowerup_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    struct bt_mesh_gen_power_onoff_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_ONPOWERUP_STATUS, 1);
    bt_mesh_model_msg_init(&msg, OP_GEN_ONPOWERUP_STATUS);
    net_buf_simple_add_u8(&msg, srv->state.onpowerup);

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int gen_onpowerup_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    return gen_onpowerup_status_send(model, ctx);
}

static int gen_onpowerup_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint8_t onpowerup;
    struct bt_mesh_gen_power_onoff_srv *srv = model->rt->user_data;

    onpowerup = net_buf_simple_pull_u8(buf);
    if (onpowerup == srv->state.onpowerup) {
        return 0;
    }

    srv->state.onpowerup = onpowerup;
    LOG_DBG("%u", srv->state.onpowerup);

    if (srv->cb != NULL && srv->cb->set != NULL)
        srv->cb->set(srv->cb->user_data, BT_MESH_SRV_GEN_POWER_ONOFF_EVT, srv);

    gen_onpowerup_status_publish(srv->model);
    return 0;
}

static int gen_onpowerup_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    int ret = gen_onpowerup_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return gen_onpowerup_status_send(model, ctx);
}

const struct bt_mesh_model_op bt_mesh_gen_power_onoff_srv_op[] = {
    { OP_GEN_ONPOWERUP_GET,             BT_MESH_LEN_EXACT(0),       gen_onpowerup_get },
    BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op bt_mesh_gen_power_onoff_setup_srv_op[] = {
    { OP_GEN_ONPOWERUP_SET,             BT_MESH_LEN_EXACT(1),       gen_onpowerup_set },
    { OP_GEN_ONPOWERUP_SET_UNACK,       BT_MESH_LEN_EXACT(1),       gen_onpowerup_set_unack },
    BT_MESH_MODEL_OP_END,
};

static int gen_onpowerup_pub_update(const struct bt_mesh_model *model)
{
    struct bt_mesh_gen_power_onoff_srv *srv = model->rt->user_data;

    bt_mesh_model_msg_init(model->pub->msg, OP_GEN_ONPOWERUP_STATUS);
    net_buf_simple_add_u8(model->pub->msg, srv->state.onpowerup);

    return 0;
}

void gen_onpowerup_config(struct bt_mesh_gen_power_onoff_srv *srv, uint8_t onpowerup)
{
    LOG_INF("%u ", onpowerup);
    srv->state.onpowerup = onpowerup;
}

void gen_onpowerup_status_publish(const struct bt_mesh_model *model)
{
    LOG_INF("");
    gen_onpowerup_pub_update(model);
    bt_mesh_model_publish(model);
}

void gen_power_onoff_config(struct bt_mesh_gen_power_onoff_srv *srv, uint8_t onoff)
{
    LOG_INF("%u ", onoff);
    gen_onoff_config(&srv->onoff, onoff);
}

static void bt_mesh_gen_power_onoff_cb_get(void *user_data, enum bt_mesh_srv_callback_evt evt, void *state)
{
    struct bt_mesh_gen_power_onoff_srv *cur_srv = user_data;

    if (cur_srv->cb != NULL && cur_srv->cb->get != NULL)
        cur_srv->cb->get(cur_srv->cb->user_data, evt, state);
}

static void bt_mesh_gen_power_onoff_cb_set(void *user_data, enum bt_mesh_srv_callback_evt evt, void *srv)
{
    struct bt_mesh_gen_power_onoff_srv *cur_srv = user_data;

    if (cur_srv->cb != NULL && cur_srv->cb->set != NULL)
        cur_srv->cb->set(cur_srv->cb->user_data, evt, srv);
}

static void bt_mesh_gen_power_onoff_cb_state_change(void *user_data, enum bt_mesh_srv_callback_evt evt, void *state)
{
    struct bt_mesh_gen_power_onoff_srv *srv = user_data;

    if (srv->cb != NULL && srv->cb->state_change != NULL)
        srv->cb->state_change(srv->cb->user_data, evt, state);
}

static struct bt_mesh_srv_callbacks bt_mesh_gen_power_onoff_cb = {
    .get = bt_mesh_gen_power_onoff_cb_get,
    .set = bt_mesh_gen_power_onoff_cb_set,
    .state_change = bt_mesh_gen_power_onoff_cb_state_change
};

static int bt_mesh_gen_power_onoff_srv_init(const struct bt_mesh_model *model)
{
    struct bt_mesh_gen_power_onoff_srv *srv = model->rt->user_data;

    LOG_INF("");

    if (!srv) {
        LOG_ERR("No Server context provided");
        return -EINVAL;
    }

    if (!model->pub) {
        LOG_ERR("No publication support");
        return -EINVAL;
    }

    srv->model = model;
    model->pub->update = gen_def_trans_time_pub_update;

    bt_mesh_gen_power_onoff_cb.user_data = srv;
    srv->onoff.cb = &bt_mesh_gen_power_onoff_cb;
    srv->def_trans_time.cb = &bt_mesh_gen_power_onoff_cb;

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_gen_power_onoff_srv_cb = {
    .init = bt_mesh_gen_power_onoff_srv_init,
};

static int bt_mesh_gen_power_onoff_setup_srv_init(const struct bt_mesh_model *model)
{
    LOG_INF("");
    struct bt_mesh_gen_power_onoff_srv *srv = model->rt->user_data;
    if (!srv) {
        LOG_ERR("No Server context provided");
        return -EINVAL;
    }

    srv->setup_model = model;

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_gen_power_onoff_setup_srv_cb = {
    .init = bt_mesh_gen_power_onoff_setup_srv_init,
};

static int gen_power_level_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    struct bt_mesh_gen_power_level_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_POWER_LEVEL_STATUS, 5);
    bt_mesh_model_msg_init(&msg, OP_GEN_POWER_LEVEL_STATUS);
    net_buf_simple_add_le16(&msg, srv->state.actual);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_le16(&msg, srv->state.target_actual);
        net_buf_simple_add_u8(&msg, srv->transition.remain_time);
    }

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int gen_power_level_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_power_level_srv *srv = model->rt->user_data;

    if (srv->cb != NULL && srv->cb->get != NULL)
        srv->cb->get(srv->cb->user_data, BT_MESH_SRV_GEN_POWER_LEVEL_EVT, &srv->state);

    return gen_power_level_status_send(model, ctx);
}

static int gen_power_level_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint16_t power;
    uint8_t tid;
    struct bt_mesh_gen_power_level_srv *srv = model->rt->user_data;

    if ((buf->len != 3U) && (buf->len != 5U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    power = net_buf_simple_pull_le16(buf);
    tid = net_buf_simple_pull_u8(buf);
    if (bt_mesh_tid_check_and_update(&srv->pre_tid, tid, ctx->addr, ctx->recv_dst))
        return 0;

    bt_mesh_server_stop_transition(&srv->transition);

    if (power) {
        if (srv->state.range_min && power < srv->state.range_min) {
            power = srv->state.range_min;
        } else if (srv->state.range_max && power > srv->state.range_max) {
            power = srv->state.range_max;
        }
    }

    srv->state.target_actual = power;

    if (srv->cb != NULL && srv->cb->set != NULL)
        srv->cb->set(srv->cb->user_data, BT_MESH_SRV_GEN_POWER_LEVEL_EVT, srv);

    if (srv->state.target_actual == srv->state.actual)
        return 0;

    bt_mesh_srv_transition_get(model, &srv->transition, buf);
    LOG_DBG("power:%u transition_time:%u delay:%u", power, srv->transition.transition_time, srv->transition.delay);

    srv->state.last = srv->state.actual;
    set_transition_values(&srv->transition);
    srv->state.delta_power = ((float)(srv->state.target_actual - srv->state.actual) / srv->transition.counter);
    bt_mesh_server_start_transition(&srv->transition);
    return 0;
}

static int gen_power_level_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    int ret = gen_power_level_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return gen_power_level_status_send(model, ctx);
}

static int gen_power_last_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_power_level_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_POWER_LAST_STATUS, 2);
    bt_mesh_model_msg_init(&msg, OP_GEN_POWER_LAST_STATUS);
    net_buf_simple_add_le16(&msg, srv->state.last);

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int gen_power_default_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    struct bt_mesh_gen_power_level_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_POWER_DEFAULT_STATUS, 2);
    bt_mesh_model_msg_init(&msg, OP_GEN_POWER_DEFAULT_STATUS);
    net_buf_simple_add_le16(&msg, srv->state.def);

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int gen_power_default_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    return gen_power_default_status_send(model, ctx);
}

static int gen_power_default_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint16_t power;
    struct bt_mesh_gen_power_level_srv *srv = model->rt->user_data;

    power = net_buf_simple_pull_le16(buf);
    if (power == 0)
        power = srv->state.last;

    LOG_DBG("%u", srv->state.def);
    srv->state.def = power;
    return 0;
}

static int gen_power_default_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    int ret = gen_power_default_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return gen_power_default_status_send(model, ctx);
}

static int gen_power_range_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, uint8_t status)
{
    struct bt_mesh_gen_power_level_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_POWER_RANGE_STATUS, 5);
    bt_mesh_model_msg_init(&msg, OP_GEN_POWER_RANGE_STATUS);
    net_buf_simple_add_u8(&msg, status);

    if (status != 0) {
        return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
    }

    net_buf_simple_add_le16(&msg, srv->state.range_min);
    net_buf_simple_add_le16(&msg, srv->state.range_max);

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int gen_power_range_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    return gen_power_range_status_send(model, ctx, 0);
}

static int gen_power_range_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint16_t range_min, range_max;
    struct bt_mesh_gen_power_level_srv *srv = model->rt->user_data;

    range_min = net_buf_simple_pull_le16(buf);
    range_max = net_buf_simple_pull_le16(buf);

    LOG_DBG("%u %u", range_min, range_max);

    if (range_min == 0 || range_max == 0 || range_min > range_max)
        return -EINVAL;

    srv->state.range_min = range_min;
    srv->state.range_max = range_max;

    return 0;
}

static int gen_power_range_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    int ret = gen_power_range_set_unack(model, ctx, buf);
    if (ret < 0 || ret >= BT_MESH_STATUS_UNKNOWN)
        return ret;

    return gen_power_range_status_send(model, ctx, ret);
}

const struct bt_mesh_model_op bt_mesh_gen_power_level_srv_op[] = {
    { OP_GEN_POWER_LEVEL_GET,           BT_MESH_LEN_EXACT(0),       gen_power_level_get },
    { OP_GEN_POWER_LEVEL_SET,           BT_MESH_LEN_MIN(3),         gen_power_level_set },
    { OP_GEN_POWER_LEVEL_SET_UNACK,     BT_MESH_LEN_MIN(3),         gen_power_level_set_unack },
    { OP_GEN_POWER_LAST_GET,            BT_MESH_LEN_EXACT(0),       gen_power_last_get },
    { OP_GEN_POWER_DEFAULT_GET,         BT_MESH_LEN_EXACT(0),       gen_power_default_get },
    { OP_GEN_POWER_RANGE_GET,           BT_MESH_LEN_EXACT(0),       gen_power_range_get },
    BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op bt_mesh_gen_power_level_setup_srv_op[] = {
    { OP_GEN_POWER_DEFAULT_SET,         BT_MESH_LEN_EXACT(2),       gen_power_default_set },
    { OP_GEN_POWER_DEFAULT_SET_UNACK,   BT_MESH_LEN_EXACT(2),       gen_power_default_set_unack },
    { OP_GEN_POWER_RANGE_SET,           BT_MESH_LEN_EXACT(4),       gen_power_range_set },
    { OP_GEN_POWER_RANGE_SET_UNACK,     BT_MESH_LEN_EXACT(4),       gen_power_range_set_unack },
    BT_MESH_MODEL_OP_END,
};

static int gen_power_level_pub_update(const struct bt_mesh_model *model)
{
    struct bt_mesh_gen_power_level_srv *srv = model->rt->user_data;

    bt_mesh_model_msg_init(model->pub->msg, OP_GEN_POWER_LEVEL_STATUS);
    net_buf_simple_add_le16(model->pub->msg, srv->state.actual);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_le16(model->pub->msg, srv->state.target_actual);
        net_buf_simple_add_u8(model->pub->msg, srv->transition.remain_time);
    }

    return 0;
}

void gen_power_level_config(struct bt_mesh_gen_power_level_srv *srv, uint16_t power)
{
    LOG_INF("%u", power);
    srv->state.actual = power;
    gen_level_config(&srv->level, gen_power_actual_to_gen_level(power));
    gen_power_onoff_config(&srv->power_onoff, gen_power_actual_to_gen_onoff(power));
}

void gen_power_level_status_publish(const struct bt_mesh_model *model)
{
    LOG_INF("");
    gen_power_level_pub_update(model);
    bt_mesh_model_publish(model);
}

static void gen_power_level_work_handler(struct k_work *work)
{
    struct bt_mesh_gen_power_level_srv *srv = CONTAINER_OF(work, struct bt_mesh_gen_power_level_srv, transition.timer.work);

    LOG_DBG("just_started:%u counter:%u, quo_tt:%u  %u", srv->transition.just_started, srv->transition.counter,
        srv->transition.quo_tt, srv->state.delta_power);

    if (srv->transition.just_started) {
        srv->transition.just_started = false;

        if (srv->transition.counter == 0U) {
            gen_power_level_config(srv, srv->state.target_actual);
            gen_power_level_status_publish(srv->model);

            if (srv->cb != NULL && srv->cb->state_change != NULL)
                srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_GEN_POWER_LEVEL_EVT, &srv->state);
        } else {
            srv->transition.start_timestamp = k_uptime_get();
            k_work_reschedule(&srv->transition.timer, K_MSEC(srv->transition.quo_tt));
        }

        return;
    }

    if (srv->transition.counter == 0 || (--srv->transition.counter) == 0) {
        gen_power_level_config(srv, srv->state.target_actual);
        gen_power_level_status_publish(srv->model);

        if (srv->cb != NULL && srv->cb->state_change != NULL)
            srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_GEN_POWER_LEVEL_EVT, &srv->state);
    } else {
        gen_power_level_config(srv, srv->state.actual + srv->state.delta_power);
        gen_power_level_status_publish(srv->model);
        k_work_reschedule(&srv->transition.timer, K_MSEC(srv->transition.quo_tt));

        if (srv->cb != NULL && srv->cb->state_change != NULL)
            srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_GEN_POWER_LEVEL_EVT, &srv->state);
    }
}

static void bt_mesh_gen_power_level_cb_get(void *user_data, enum bt_mesh_srv_callback_evt evt, void *state)
{
    struct bt_mesh_gen_power_level_srv *cur_srv = user_data;

    LOG_DBG("%u", evt);

    if (cur_srv->cb != NULL && cur_srv->cb->get != NULL)
        cur_srv->cb->get(cur_srv->cb->user_data, BT_MESH_SRV_GEN_POWER_LEVEL_EVT, &cur_srv->state);

    switch (evt) {
    case BT_MESH_SRV_GEN_ONOFF_EVT: {
        struct bt_mesh_gen_onoff_state *onoff = state;
        onoff->onoff = gen_power_actual_to_gen_onoff(cur_srv->state.actual);
    }
        break;
    case BT_MESH_SRV_GEN_LEVEL_EVT: {
        struct bt_mesh_gen_level_state *level = state;
        level->level = gen_power_actual_to_gen_level(cur_srv->state.actual);
    }
        break;
    default:
        break;
    }
}

static void bt_mesh_gen_power_level_cb_set(void *user_data, enum bt_mesh_srv_callback_evt evt, void *srv)
{
    struct bt_mesh_gen_power_level_srv *cur_srv = user_data;

    bt_mesh_server_stop_transition(&cur_srv->transition);

    LOG_DBG("%u", evt);

    switch (evt) {
    case BT_MESH_SRV_GEN_ONOFF_EVT: {
        struct bt_mesh_gen_onoff_srv *onoff = srv;
        if (onoff->state.target_onoff == 1) {
            cur_srv->state.target_actual = cur_srv->state.def == 0 ? cur_srv->state.last : cur_srv->state.def;
        } else {
            cur_srv->state.target_actual = 0;
        }
    }
        break;
    case BT_MESH_SRV_GEN_LEVEL_EVT: {
        struct bt_mesh_gen_level_srv *level = srv;
        cur_srv->state.target_actual = gen_level_to_gen_power_actual(level->state.target_level);
        if (cur_srv->state.target_actual != 0) {
            if (cur_srv->state.range_min && cur_srv->state.target_actual < cur_srv->state.range_min) {
                cur_srv->state.target_actual = cur_srv->state.range_min;
            } else if (cur_srv->state.range_max && cur_srv->state.target_actual > cur_srv->state.range_max) {
                cur_srv->state.target_actual = cur_srv->state.range_max;
            }
        }
    }
        break;
    default:
        break;
    }

    if (cur_srv->cb != NULL && cur_srv->cb->set != NULL)
        cur_srv->cb->set(cur_srv->cb->user_data, BT_MESH_SRV_GEN_POWER_LEVEL_EVT, cur_srv);

    switch (evt) {
    case BT_MESH_SRV_GEN_ONOFF_EVT: {
        struct bt_mesh_gen_onoff_srv *onoff = srv;
        onoff->state.target_onoff = gen_power_actual_to_gen_onoff(cur_srv->state.target_actual);
        cur_srv->transition.child = &onoff->transition;
    }
        break;
    case BT_MESH_SRV_GEN_LEVEL_EVT: {
        struct bt_mesh_gen_level_srv *level = srv;
        level->state.target_level = gen_power_actual_to_gen_level(cur_srv->state.target_actual);
        cur_srv->transition.child = &level->transition;
    }
        break;
    default:
        break;
    }
}

static void bt_mesh_gen_power_level_cb_state_change(void *user_data, enum bt_mesh_srv_callback_evt evt, void *state)
{
    struct bt_mesh_gen_power_level_srv *srv = user_data;

    LOG_DBG("%u", evt);

    switch (evt) {
    case BT_MESH_SRV_GEN_ONOFF_EVT: {
        struct bt_mesh_gen_onoff_state *onoff = state;
        if (onoff->onoff == 1) {
            srv->state.actual = srv->state.def == 0 ? srv->state.last : srv->state.def;
        } else {
            srv->state.last = srv->state.actual;
            srv->state.actual = 0;
        }
        gen_level_config(&srv->level, gen_power_actual_to_gen_level(srv->state.actual));
    }
        break;
    case BT_MESH_SRV_GEN_LEVEL_EVT: {
        struct bt_mesh_gen_level_state *level = state;
        uint16_t power = gen_level_to_gen_power_actual(level->level);
        srv->state.last = srv->state.actual;
        if (power == 0) {
            srv->state.actual = 0;
        } else if (srv->state.range_min && power < srv->state.range_min) {
            srv->state.actual = srv->state.range_min;
        } else {
            srv->state.actual = power;
        }

        gen_power_onoff_config(&srv->power_onoff, gen_power_actual_to_gen_onoff(srv->state.actual));
    }
        break;
    default:
        break;
    }

    if (srv->cb != NULL && srv->cb->state_change != NULL)
        srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_GEN_POWER_LEVEL_EVT, &srv->state);
}

static struct bt_mesh_srv_callbacks bt_mesh_gen_power_level_cb = {
    .get = bt_mesh_gen_power_level_cb_get,
    .set = bt_mesh_gen_power_level_cb_set,
    .state_change = bt_mesh_gen_power_level_cb_state_change
};

static int bt_mesh_gen_power_level_srv_init(const struct bt_mesh_model *model)
{
    struct bt_mesh_gen_power_level_srv *srv = model->rt->user_data;

    LOG_INF("");

    if (!srv) {
        LOG_ERR("No Server context provided");
        return -EINVAL;
    }

    if (!model->pub) {
        LOG_ERR("No publication support");
        return -EINVAL;
    }

    srv->model = model;
    model->pub->update = gen_power_level_pub_update;

    bt_mesh_gen_power_level_cb.user_data = srv;
    srv->level.cb = &bt_mesh_gen_power_level_cb;
    srv->power_onoff.cb = &bt_mesh_gen_power_level_cb;

    k_work_init_delayable(&srv->transition.timer, gen_power_level_work_handler);

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_gen_power_level_srv_cb = {
    .init = bt_mesh_gen_power_level_srv_init,
};

static int bt_mesh_gen_power_level_setup_srv_init(const struct bt_mesh_model *model)
{
    struct bt_mesh_gen_power_level_srv *srv = model->rt->user_data;

    LOG_INF("");

    if (!srv) {
        LOG_ERR("No Server context provided");
        return -EINVAL;
    }

    srv->setup_model = model;

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_gen_power_level_setup_srv_cb = {
    .init = bt_mesh_gen_power_level_setup_srv_init,
};

static int gen_battery_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    struct bt_mesh_gen_battery_srv *srv = model->rt->user_data;

    if (srv->cb != NULL && srv->cb->get != NULL)
        srv->cb->get(srv->cb->user_data, BT_MESH_SRV_GEN_BATTERY_EVT, &srv->state);

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_BATTERY_STATUS, 8);
    bt_mesh_model_msg_init(&msg, OP_GEN_BATTERY_STATUS);
    net_buf_simple_add_le32(&msg, srv->state.time_to_discharge << 8 | srv->state.battery_level);
    net_buf_simple_add_le32(&msg, srv->state.battery_flags << 24 | srv->state.time_to_charge);

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

const struct bt_mesh_model_op bt_mesh_gen_battery_srv_op[] = {
    { OP_GEN_BATTERY_GET,               BT_MESH_LEN_EXACT(0),       gen_battery_get },
    BT_MESH_MODEL_OP_END,
};

static int gen_battery_pub_update(const struct bt_mesh_model *model)
{
    struct bt_mesh_gen_battery_srv *srv = model->rt->user_data;

    if (srv->cb != NULL && srv->cb->get != NULL)
        srv->cb->get(srv->cb->user_data, BT_MESH_SRV_GEN_BATTERY_EVT, &srv->state);

    bt_mesh_model_msg_init(model->pub->msg, OP_GEN_BATTERY_STATUS);
    net_buf_simple_add_le32(model->pub->msg, srv->state.time_to_discharge << 8 | srv->state.battery_level);
    net_buf_simple_add_le32(model->pub->msg, srv->state.battery_flags << 24 | srv->state.time_to_charge);

    return 0;
}

void gen_battery_status_publish(const struct bt_mesh_model *model)
{
    LOG_INF("");
    gen_battery_pub_update(model);
    bt_mesh_model_publish(model);
}

static int bt_mesh_gen_battery_srv_init(const struct bt_mesh_model *model)
{
    LOG_INF("");
    struct bt_mesh_gen_battery_srv *srv = model->rt->user_data;
    if (!srv) {
        LOG_ERR("No Server context provided");
        return -EINVAL;
    }

    if (!model->pub) {
        LOG_ERR("No publication support");
        return -EINVAL;
    }

    srv->model = model;
    model->pub->update = gen_battery_pub_update;

    /* Initialize state to unknown */
    memset(&srv->state, 0xff, sizeof(srv->state));

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_gen_battery_srv_cb = {
    .init = bt_mesh_gen_battery_srv_init,
};

static int gen_location_global_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    struct bt_mesh_gen_location_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_LOCATION_GLOBAL_STATUS, 10);
    bt_mesh_model_msg_init(&msg, OP_GEN_LOCATION_GLOBAL_STATUS);
    net_buf_simple_add_le32(&msg, srv->state.global_latitude);
    net_buf_simple_add_le32(&msg, srv->state.global_longitude);
    net_buf_simple_add_le16(&msg, srv->state.global_altitude);

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int gen_location_global_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_location_srv *srv = model->rt->user_data;

    if (srv->cb != NULL && srv->cb->get != NULL)
        srv->cb->get(srv->cb->user_data, BT_MESH_SRV_GEN_LOCATION_GLOBAL_EVT, &srv->state);

    return gen_location_global_status_send(model, ctx);
}

static int gen_location_global_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_location_srv *srv = model->rt->user_data;

    srv->state.global_latitude = net_buf_simple_pull_le32(buf);
    srv->state.global_longitude = net_buf_simple_pull_le32(buf);
    srv->state.global_altitude = net_buf_simple_pull_le16(buf);

    if (srv->cb != NULL && srv->cb->set != NULL)
        srv->cb->set(srv->cb->user_data, BT_MESH_SRV_GEN_LOCATION_GLOBAL_EVT, srv);

    gen_location_status_publish(srv->model, OP_GEN_LOCATION_GLOBAL_STATUS);
    return 0;
}

static int gen_location_global_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    int ret = gen_location_global_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return gen_location_global_status_send(model, ctx);
}

static int gen_location_local_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    struct bt_mesh_gen_location_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_LOCATION_LOCAL_STATUS, 9);
    bt_mesh_model_msg_init(&msg, OP_GEN_LOCATION_LOCAL_STATUS);
    net_buf_simple_add_le16(&msg, srv->state.local_north);
    net_buf_simple_add_le16(&msg, srv->state.local_east);
    net_buf_simple_add_le16(&msg, srv->state.local_altitude);
    net_buf_simple_add_u8(&msg, srv->state.floor_number);
    net_buf_simple_add_le16(&msg, srv->state.uncertainty);

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int gen_location_local_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_location_srv *srv = model->rt->user_data;

    if (srv->cb != NULL && srv->cb->get != NULL)
        srv->cb->get(srv->cb->user_data, BT_MESH_SRV_GEN_LOCATION_LOCAL_EVT, &srv->state);

    return gen_location_local_status_send(model, ctx);
}

static int gen_location_local_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_location_srv *srv = model->rt->user_data;

    srv->state.local_north = net_buf_simple_pull_le16(buf);
    srv->state.local_east = net_buf_simple_pull_le16(buf);
    srv->state.local_altitude = net_buf_simple_pull_le16(buf);
    srv->state.floor_number = net_buf_simple_pull_u8(buf);
    srv->state.uncertainty = net_buf_simple_pull_le16(buf);

    if (srv->cb != NULL && srv->cb->set != NULL)
        srv->cb->set(srv->cb->user_data, BT_MESH_SRV_GEN_LOCATION_LOCAL_EVT, srv);

    gen_location_status_publish(srv->model, OP_GEN_LOCATION_LOCAL_STATUS);
    return 0;
}

static int gen_location_local_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    int ret = gen_location_local_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return gen_location_local_status_send(model, ctx);
}

const struct bt_mesh_model_op bt_mesh_gen_location_srv_op[] = {
    { OP_GEN_LOCATION_GLOBAL_GET,       BT_MESH_LEN_EXACT(0),       gen_location_global_get },
    { OP_GEN_LOCATION_LOCAL_GET,        BT_MESH_LEN_EXACT(0),       gen_location_local_get },
    BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op bt_mesh_gen_location_setup_srv_op[] = {
    { OP_GEN_LOCATION_GLOBAL_SET,       BT_MESH_LEN_EXACT(10),      gen_location_global_set },
    { OP_GEN_LOCATION_GLOBAL_SET_UNACK, BT_MESH_LEN_EXACT(10),      gen_location_global_set_unack },
    { OP_GEN_LOCATION_LOCAL_SET,        BT_MESH_LEN_EXACT(9),       gen_location_local_set },
    { OP_GEN_LOCATION_LOCAL_SET_UNACK,  BT_MESH_LEN_EXACT(9),       gen_location_local_set_unack },
    BT_MESH_MODEL_OP_END,
};

static int gen_location_pub_update(const struct bt_mesh_model *model)
{
    struct bt_mesh_gen_location_srv *srv = model->rt->user_data;

    if (srv->pub_opcode == OP_GEN_LOCATION_GLOBAL_STATUS) {
        bt_mesh_model_msg_init(model->pub->msg, OP_GEN_LOCATION_GLOBAL_STATUS);
        net_buf_simple_add_le32(model->pub->msg, srv->state.global_latitude);
        net_buf_simple_add_le32(model->pub->msg, srv->state.global_longitude);
        net_buf_simple_add_le16(model->pub->msg, srv->state.global_altitude);
    } else if (srv->pub_opcode == OP_GEN_LOCATION_LOCAL_STATUS) {
        bt_mesh_model_msg_init(model->pub->msg, OP_GEN_LOCATION_LOCAL_STATUS);
        net_buf_simple_add_le16(model->pub->msg, srv->state.local_north);
        net_buf_simple_add_le16(model->pub->msg, srv->state.local_east);
        net_buf_simple_add_le16(model->pub->msg, srv->state.local_altitude);
        net_buf_simple_add_u8(model->pub->msg, srv->state.floor_number);
        net_buf_simple_add_le16(model->pub->msg, srv->state.uncertainty);
    }

    return 0;
}

void gen_location_config(struct bt_mesh_gen_location_srv *srv, struct bt_mesh_gen_location_state *state)
{
    memcpy(&srv->state, state, sizeof(struct bt_mesh_gen_location_state));
}

void gen_location_status_publish(const struct bt_mesh_model *model, uint16_t opcode)
{
    struct bt_mesh_gen_location_srv *srv = model->rt->user_data;

    if (opcode != OP_GEN_LOCATION_GLOBAL_STATUS && opcode != OP_GEN_LOCATION_LOCAL_STATUS) {
        LOG_ERR("opcode error");
        return;
    }

    LOG_INF("");
    srv->pub_opcode = opcode;
    gen_location_pub_update(model);
    bt_mesh_model_publish(model);
}

static int bt_mesh_gen_location_srv_init(const struct bt_mesh_model *model)
{
    LOG_INF("");
    struct bt_mesh_gen_location_srv *srv = model->rt->user_data;
    if (!srv) {
        LOG_ERR("No Server context provided");
        return -EINVAL;
    }

    if (!model->pub) {
        LOG_ERR("No publication support");
        return -EINVAL;
    }

    srv->model = model;
    model->pub->update = gen_location_pub_update;

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_gen_location_srv_cb = {
    .init = bt_mesh_gen_location_srv_init,
};

static int bt_mesh_gen_location_setup_srv_init(const struct bt_mesh_model *model)
{
    LOG_INF("");
    struct bt_mesh_gen_location_srv *srv = model->rt->user_data;
    if (!srv) {
        LOG_ERR("No Server context provided");
        return -EINVAL;
    }

    srv->setup_model = model;

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_gen_location_setup_srv_cb = {
    .init = bt_mesh_gen_location_setup_srv_init,
};

static int gen_admin_properties_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    int i;
    struct bt_mesh_gen_property_srv *srv = model->rt->user_data;

    NET_BUF_SIMPLE_DEFINE(msg, BT_MESH_TX_SDU_MAX);
    bt_mesh_model_msg_init(&msg, OP_GEN_ADMIN_PROPS_STATUS);
    for (i = 0; i < srv->property_cnt; i++) {
        if (net_buf_simple_tailroom(&msg) < (BT_MESH_MIC_SHORT + 2))
            break;

        net_buf_simple_add_le16(&msg, srv->state[i].property_id);
    }

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int gen_admin_property_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_property_srv *srv = model->rt->user_data;
    uint16_t property_id = net_buf_simple_pull_le16(buf);
    struct bt_mesh_gen_property_state *property;

    if (property_id == MESH_PROPERTY_ID_PROHIBITED) {
        return -EINVAL;
    }

    NET_BUF_SIMPLE_DEFINE(msg, BT_MESH_TX_SDU_MAX);
    bt_mesh_model_msg_init(&msg, OP_GEN_ADMIN_PROP_STATUS);
    net_buf_simple_add_le16(&msg, property_id);

    property = gen_property_get(srv, property_id);
    if (property == NULL) {
        goto exit;
    }

    net_buf_simple_add_u8(&msg, property->access);

    property->val = &msg;
    if (srv->cb != NULL && srv->cb->get != NULL)
        srv->cb->get(property, BT_MESH_SRV_GEN_ADMIN_PROPERTY_EVT, srv);
    property->val = NULL;
exit:
    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int gen_admin_property_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_property_srv *srv = model->rt->user_data;
    uint16_t property_id = net_buf_simple_pull_le16(buf);
    uint16_t access = net_buf_simple_pull_u8(buf);
    struct bt_mesh_gen_property_state *property;

    if (property_id == MESH_PROPERTY_ID_PROHIBITED || access > GENERIC_USER_PROPERTY_READ_AND_WRITTEN) {
        return -EINVAL;
    }

    property = gen_property_get(srv, property_id);
    if (property == NULL) {
        return 0;
    }

    property->access = access;
    property->val = buf;
    if (srv->cb != NULL && srv->cb->set != NULL)
        srv->cb->set(property, BT_MESH_SRV_GEN_ADMIN_PROPERTY_EVT, srv);
    property->val = NULL;

    return 0;
}

static int gen_admin_property_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_property_srv *srv = model->rt->user_data;
    uint16_t property_id = net_buf_simple_pull_le16(buf);
    uint16_t access = net_buf_simple_pull_u8(buf);
    struct bt_mesh_gen_property_state *property;

    if (property_id == MESH_PROPERTY_ID_PROHIBITED || access > GENERIC_USER_PROPERTY_READ_AND_WRITTEN) {
        return -EINVAL;
    }

    NET_BUF_SIMPLE_DEFINE(msg, BT_MESH_TX_SDU_MAX);
    bt_mesh_model_msg_init(&msg, OP_GEN_ADMIN_PROP_STATUS);
    net_buf_simple_add_le16(&msg, property_id);

    property = gen_property_get(srv, property_id);
    if (property == NULL) {
        goto exit;
    }

    property->access = access;
    net_buf_simple_add_u8(&msg, property->access);

    property->val = buf;
    if (srv->cb != NULL && srv->cb->set != NULL)
        srv->cb->set(property, BT_MESH_SRV_GEN_ADMIN_PROPERTY_EVT, srv);

    property->val = &msg;
    if (srv->cb != NULL && srv->cb->get != NULL)
        srv->cb->get(property, BT_MESH_SRV_GEN_ADMIN_PROPERTY_EVT, srv);
    property->val = NULL;

exit:
    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

const struct bt_mesh_model_op bt_mesh_gen_admin_property_srv_op[] = {
    { OP_GEN_ADMIN_PROPS_GET,           BT_MESH_LEN_EXACT(0),       gen_admin_properties_get },
    { OP_GEN_ADMIN_PROP_GET,            BT_MESH_LEN_EXACT(2),       gen_admin_property_get },
    { OP_GEN_ADMIN_PROP_SET,            BT_MESH_LEN_MIN(3),         gen_admin_property_set },
    { OP_GEN_ADMIN_PROP_SET_UNACK,      BT_MESH_LEN_MIN(3),         gen_admin_property_set_unack },
    BT_MESH_MODEL_OP_END,
};

static int bt_mesh_gen_admin_property_srv_init(const struct bt_mesh_model *model)
{
    LOG_INF("");
    struct bt_mesh_gen_property_srv *srv = model->rt->user_data;

    if (!srv) {
        LOG_ERR("No Server context provided");
        return -EINVAL;
    }

    srv->model = model;
    return 0;
}

const struct bt_mesh_model_cb bt_mesh_gen_admin_property_srv_cb = {
    .init = bt_mesh_gen_admin_property_srv_init,
};

static int gen_mfr_properties_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    int i;
    struct bt_mesh_gen_property_srv *srv = model->rt->user_data;

    NET_BUF_SIMPLE_DEFINE(msg, BT_MESH_TX_SDU_MAX);
    bt_mesh_model_msg_init(&msg, OP_GEN_MFR_PROPS_STATUS);
    for (i = 0; i < srv->property_cnt; i++) {
        if (net_buf_simple_tailroom(&msg) < (BT_MESH_MIC_SHORT + 2))
            break;

        net_buf_simple_add_le16(&msg, srv->state[i].property_id);
    }

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int gen_mfr_property_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_property_srv *srv = model->rt->user_data;
    uint16_t property_id = net_buf_simple_pull_le16(buf);
    struct bt_mesh_gen_property_state *property;

    if (property_id == MESH_PROPERTY_ID_PROHIBITED) {
        return -EINVAL;
    }

    NET_BUF_SIMPLE_DEFINE(msg, BT_MESH_TX_SDU_MAX);
    bt_mesh_model_msg_init(&msg, OP_GEN_MFR_PROP_STATUS);
    net_buf_simple_add_le16(&msg, property_id);

    property = gen_property_get(srv, property_id);
    if (property == NULL) {
        goto exit;
    }

    net_buf_simple_add_u8(&msg, property->access);
    property->val = &msg;
    if (srv->cb != NULL && srv->cb->get != NULL)
        srv->cb->get(property, BT_MESH_SRV_GEN_MFR_PROPERTY_EVT, srv);
    property->val = NULL;
exit:
    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int gen_mfr_property_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_property_srv *srv = model->rt->user_data;
    uint16_t property_id = net_buf_simple_pull_le16(buf);
    uint16_t access = net_buf_simple_pull_u8(buf);
    struct bt_mesh_gen_property_state *property;

    if (property_id == MESH_PROPERTY_ID_PROHIBITED || access > GENERIC_USER_PROPERTY_READ) {
        return -EINVAL;
    }

    property = gen_property_get(srv, property_id);
    if (property == NULL) {
        return 0;
    }

    property->access = access;
    if (srv->cb != NULL && srv->cb->set != NULL)
        srv->cb->set(property, BT_MESH_SRV_GEN_MFR_PROPERTY_EVT, srv);
    return 0;
}

static int gen_mfr_property_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_property_srv *srv = model->rt->user_data;
    uint16_t property_id = net_buf_simple_pull_le16(buf);
    uint16_t access = net_buf_simple_pull_u8(buf);
    struct bt_mesh_gen_property_state *property;

    if (property_id == MESH_PROPERTY_ID_PROHIBITED || access > GENERIC_USER_PROPERTY_READ) {
        return -EINVAL;
    }

    NET_BUF_SIMPLE_DEFINE(msg, BT_MESH_TX_SDU_MAX);
    bt_mesh_model_msg_init(&msg, OP_GEN_MFR_PROP_STATUS);
    net_buf_simple_add_le16(&msg, property_id);

    property = gen_property_get(srv, property_id);
    if (property == NULL) {
        goto exit;
    }

    property->access = access;
    net_buf_simple_add_u8(&msg, property->access);

    if (srv->cb != NULL && srv->cb->set != NULL)
        srv->cb->set(property, BT_MESH_SRV_GEN_MFR_PROPERTY_EVT, srv);
    property->val = &msg;
    if (srv->cb != NULL && srv->cb->get != NULL)
        srv->cb->get(property, BT_MESH_SRV_GEN_MFR_PROPERTY_EVT, srv);
    property->val = NULL;

exit:
    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

const struct bt_mesh_model_op bt_mesh_gen_mfr_property_srv_op[] = {
    { OP_GEN_MFR_PROPS_GET,             BT_MESH_LEN_EXACT(0),       gen_mfr_properties_get },
    { OP_GEN_MFR_PROP_GET,              BT_MESH_LEN_EXACT(2),       gen_mfr_property_get },
    { OP_GEN_MFR_PROP_SET,              BT_MESH_LEN_MIN(3),         gen_mfr_property_set },
    { OP_GEN_MFR_PROP_SET_UNACK,        BT_MESH_LEN_MIN(3),         gen_mfr_property_set_unack },
    BT_MESH_MODEL_OP_END,
};

static int bt_mesh_gen_mfr_property_srv_init(const struct bt_mesh_model *model)
{
    LOG_INF("");
    struct bt_mesh_gen_property_srv *srv = model->rt->user_data;

    if (!srv) {
        LOG_ERR("No Server context provided");
        return -EINVAL;
    }

    srv->model = model;
    return 0;
}

const struct bt_mesh_model_cb bt_mesh_gen_mfr_property_srv_cb = {
    .init = bt_mesh_gen_mfr_property_srv_init,
};

static int gen_user_properties_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    int i;
    struct bt_mesh_gen_user_property_srv *srv = model->rt->user_data;

    NET_BUF_SIMPLE_DEFINE(msg, BT_MESH_TX_SDU_MAX);
    bt_mesh_model_msg_init(&msg, OP_GEN_USER_PROPS_STATUS);
    if (srv->admin_property != NULL) {
        for (i = 0; i< srv->admin_property->property_cnt; i++) {
            if (net_buf_simple_tailroom(&msg) < (BT_MESH_MIC_SHORT + 2))
                goto exit;

            if (srv->admin_property->state[i].access != NOT_GENERIC_USER_PROPERTY)
                net_buf_simple_add_le16(&msg, srv->admin_property->state[i].property_id);
        }
    }

    if (srv->mfr_property != NULL) {
        for (i = 0; i< srv->mfr_property->property_cnt; i++) {
            if (net_buf_simple_tailroom(&msg) < (BT_MESH_MIC_SHORT + 2))
                goto exit;

            if (srv->mfr_property->state[i].access != NOT_GENERIC_USER_PROPERTY)
                net_buf_simple_add_le16(&msg, srv->mfr_property->state[i].property_id);
        }
    }

exit:
    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int gen_user_property_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_user_property_srv *srv = model->rt->user_data;
    uint16_t property_id = net_buf_simple_pull_le16(buf);
    struct bt_mesh_gen_property_state *property;

    if (property_id == MESH_PROPERTY_ID_PROHIBITED) {
        return -EINVAL;
    }

    NET_BUF_SIMPLE_DEFINE(msg, BT_MESH_TX_SDU_MAX);
    bt_mesh_model_msg_init(&msg, OP_GEN_USER_PROP_STATUS);
    net_buf_simple_add_le16(&msg, property_id);

    property = gen_property_get(srv->admin_property, property_id);
    if (property != NULL && (property->access & GENERIC_USER_PROPERTY_READ)) {
        net_buf_simple_add_u8(&msg, property->access);
        property->val = &msg;
        if (srv->admin_property->cb != NULL && srv->admin_property->cb->get != NULL)
            srv->admin_property->cb->get(property, BT_MESH_SRV_GEN_ADMIN_PROPERTY_EVT, srv);
        property->val = NULL;
        goto exit;
    }

    property = gen_property_get(srv->mfr_property, property_id);
    if (property != NULL && (property->access & GENERIC_USER_PROPERTY_READ)) {
        net_buf_simple_add_u8(&msg, property->access);
        property->val = &msg;
        if (srv->mfr_property->cb != NULL && srv->mfr_property->cb->get != NULL)
            srv->mfr_property->cb->get(property, BT_MESH_SRV_GEN_MFR_PROPERTY_EVT, srv);
        property->val = NULL;
    }
exit:
    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int gen_user_property_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_user_property_srv *srv = model->rt->user_data;
    uint16_t property_id = net_buf_simple_pull_le16(buf);
    struct bt_mesh_gen_property_state *property;

    if (property_id == MESH_PROPERTY_ID_PROHIBITED) {
        return -EINVAL;
    }

    property = gen_property_get(srv->admin_property, property_id);
    if (property != NULL) {
        property->val = buf;
        if (srv->admin_property->cb != NULL && srv->admin_property->cb->set != NULL)
            srv->admin_property->cb->set(property, BT_MESH_SRV_GEN_ADMIN_PROPERTY_EVT, srv);
        property->val = NULL;
        return 0;
    }

    property = gen_property_get(srv->mfr_property, property_id);
    if (property != NULL) {
        property->val = buf;
        if (srv->mfr_property->cb != NULL && srv->mfr_property->cb->set != NULL)
            srv->mfr_property->cb->set(property, BT_MESH_SRV_GEN_MFR_PROPERTY_EVT, srv);
        property->val = NULL;
    }

    return 0;
}

static int gen_user_property_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_user_property_srv *srv = model->rt->user_data;
    uint16_t property_id = net_buf_simple_pull_le16(buf);
    struct bt_mesh_gen_property_state *property;

    if (property_id == MESH_PROPERTY_ID_PROHIBITED) {
        return -EINVAL;
    }

    NET_BUF_SIMPLE_DEFINE(msg, BT_MESH_TX_SDU_MAX);
    bt_mesh_model_msg_init(&msg, OP_GEN_USER_PROP_STATUS);
    net_buf_simple_add_le16(&msg, property_id);

    property = gen_property_get(srv->admin_property, property_id);
    if (property != NULL) {
        net_buf_simple_add_u8(&msg, property->access);
        property->val = buf;
        if (srv->admin_property->cb != NULL && srv->admin_property->cb->set != NULL)
            srv->admin_property->cb->set(property, BT_MESH_SRV_GEN_ADMIN_PROPERTY_EVT, srv);
        property->val = &msg;
        if (srv->admin_property->cb != NULL && srv->admin_property->cb->get != NULL)
            srv->admin_property->cb->get(property, BT_MESH_SRV_GEN_ADMIN_PROPERTY_EVT, srv);
        property->val = NULL;
        goto exit;
    }

    property = gen_property_get(srv->mfr_property, property_id);
    if (property != NULL) {
        net_buf_simple_add_u8(&msg, property->access);
        property->val = buf;
        if (srv->mfr_property->cb != NULL && srv->mfr_property->cb->set != NULL)
            srv->mfr_property->cb->set(property, BT_MESH_SRV_GEN_MFR_PROPERTY_EVT, srv);
        property->val = &msg;
        if (srv->mfr_property->cb != NULL && srv->mfr_property->cb->get != NULL)
            srv->mfr_property->cb->get(property, BT_MESH_SRV_GEN_MFR_PROPERTY_EVT, srv);
        property->val = NULL;
    }

exit:
    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

const struct bt_mesh_model_op bt_mesh_gen_user_property_srv_op[] = {
    { OP_GEN_USER_PROPS_GET,            BT_MESH_LEN_EXACT(0),       gen_user_properties_get },
    { OP_GEN_USER_PROP_GET,             BT_MESH_LEN_EXACT(2),       gen_user_property_get },
    { OP_GEN_USER_PROP_SET,             BT_MESH_LEN_MIN(2),         gen_user_property_set },
    { OP_GEN_USER_PROP_SET_UNACK,       BT_MESH_LEN_MIN(2),         gen_user_property_set_unack },
    BT_MESH_MODEL_OP_END,
};

static int bt_mesh_gen_user_property_srv_init(const struct bt_mesh_model *model)
{
    LOG_INF("");
    struct bt_mesh_gen_user_property_srv *srv = model->rt->user_data;

    if (!srv) {
        LOG_ERR("No Server context provided");
        return -EINVAL;
    }

    srv->model = model;
    return 0;
}

const struct bt_mesh_model_cb bt_mesh_gen_user_property_srv_cb = {
    .init = bt_mesh_gen_user_property_srv_init,
};

static int gen_client_properties_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    int i;
    struct bt_mesh_gen_client_property_srv *srv = model->rt->user_data;
    uint16_t start_property_id = net_buf_simple_pull_le16(buf);

    NET_BUF_SIMPLE_DEFINE(msg, BT_MESH_TX_SDU_MAX);
    bt_mesh_model_msg_init(&msg, OP_GEN_CLIENT_PROPS_STATUS);
    for (i = 0; i < srv->property_cnt; i++) {
        if (srv->properties[i] < start_property_id)
            continue;

        if (net_buf_simple_tailroom(&msg) < (BT_MESH_MIC_SHORT + 2))
            break;

        net_buf_simple_add_le16(&msg, srv->properties[i]);
    }

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

const struct bt_mesh_model_op bt_mesh_gen_client_property_srv_op[] = {
    { OP_GEN_CLIENT_PROPS_GET,          BT_MESH_LEN_EXACT(2),       gen_client_properties_get },
    BT_MESH_MODEL_OP_END,
};

static int bt_mesh_gen_client_property_srv_init(const struct bt_mesh_model *model)
{
    int i;
    struct bt_mesh_gen_client_property_srv *srv = model->rt->user_data;

    LOG_INF("");

    if (!srv) {
        LOG_ERR("No Server context provided");
        return -EINVAL;
    }

    for (i = 1; i < srv->property_cnt; i++) {
        if (srv->properties[i] >= srv->properties[i - 1])
            continue;

        LOG_ERR("property id is not an ascending order.");
        return -EINVAL;
    }

    srv->model = model;
    return 0;
}

const struct bt_mesh_model_cb bt_mesh_gen_client_property_srv_cb = {
    .init = bt_mesh_gen_client_property_srv_init,
};

