/*!
    \file    lighting_server.c
    \brief   Implementation of BLE mesh lighting server.

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
#include "lighting_server.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include "api/mesh_log.h"

static int light_lightness_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    uint16_t op_code, present, target;
    struct bt_mesh_light_lightness_srv *srv = model->rt->user_data;

    if (ctx->recv_op == OP_LIGHT_LIGHTNESS_GET || ctx->recv_op == OP_LIGHT_LIGHTNESS_SET) {
        op_code = OP_LIGHT_LIGHTNESS_STATUS;
        present = srv->state.actual;
        target = srv->state.target_actual;
    } else {
        op_code = OP_LIGHT_LIGHTNESS_LINEAR_STATUS;
        present = light_actual_to_linear(srv->state.actual);
        target = light_actual_to_linear(srv->state.target_actual);
    }

    BT_MESH_MODEL_BUF_DEFINE(msg, op_code, 5);
    bt_mesh_model_msg_init(&msg, op_code);
    net_buf_simple_add_le16(&msg, present);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_le16(&msg, target);
        net_buf_simple_add_u8(&msg, srv->transition.remain_time);
    }

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int light_lightness_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_lightness_srv *srv = model->rt->user_data;

    if (srv->cb != NULL && srv->cb->get != NULL)
        srv->cb->get(srv->cb->user_data, BT_MESH_SRV_LIGHT_LIGHTNESS_EVT, &srv->state);

    return light_lightness_status_send(model, ctx);
}

static int light_lightness_set_ext(const struct bt_mesh_model *model, struct net_buf_simple *buf, uint16_t lightness)
{
    struct bt_mesh_light_lightness_srv *srv = model->rt->user_data;

    bt_mesh_server_stop_transition(&srv->transition);

    if (lightness) {
        if (srv->state.range_min && lightness < srv->state.range_min) {
            lightness = srv->state.range_min;
        } else if (srv->state.range_max && lightness > srv->state.range_max) {
            lightness = srv->state.range_max;
        }
    }

    srv->state.target_actual = lightness;

    if (srv->cb != NULL && srv->cb->set != NULL)
        srv->cb->set(srv->cb->user_data, BT_MESH_SRV_LIGHT_LIGHTNESS_EVT, srv);

    if (srv->state.target_actual == srv->state.actual)
        return 0;

    bt_mesh_srv_transition_get(model, &srv->transition, buf);
    LOG_DBG("lightness:%u transition_time:%u delay:%u", lightness, srv->transition.transition_time, srv->transition.delay);

    srv->state.last = srv->state.actual;
    set_transition_values(&srv->transition);
    srv->state.delta_lightness = ((float)(srv->state.target_actual - srv->state.actual) / srv->transition.counter);
    bt_mesh_server_start_transition(&srv->transition);
    return 0;
}

static int light_lightness_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint8_t tid;
    uint16_t linear, lightness;
    struct bt_mesh_light_lightness_srv *srv = model->rt->user_data;

    if ((buf->len != 3U) && (buf->len != 5U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    if (ctx->recv_op == OP_LIGHT_LIGHTNESS_SET || ctx->recv_op == OP_LIGHT_LIGHTNESS_SET_UNACK) {
        lightness = net_buf_simple_pull_le16(buf);
    } else {
        linear = net_buf_simple_pull_le16(buf);
        lightness = light_linear_to_actual(linear);
    }

    tid = net_buf_simple_pull_u8(buf);
    if (bt_mesh_tid_check_and_update(&srv->pre_tid, tid, ctx->addr, ctx->recv_dst))
        return 0;

    return light_lightness_set_ext(model, buf, lightness);
}

static int light_lightness_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    int ret = light_lightness_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return light_lightness_status_send(model, ctx);
}

static int light_lightness_last_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_lightness_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_LIGHT_LIGHTNESS_LAST_STATUS, 2);
    bt_mesh_model_msg_init(&msg, OP_LIGHT_LIGHTNESS_LAST_STATUS);
    net_buf_simple_add_le16(&msg, srv->state.last);

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int light_lightness_default_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    struct bt_mesh_light_lightness_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_LIGHT_LIGHTNESS_DEFAULT_STATUS, 2);
    bt_mesh_model_msg_init(&msg, OP_LIGHT_LIGHTNESS_DEFAULT_STATUS);
    net_buf_simple_add_le16(&msg, srv->state.def);

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int light_lightness_default_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    return light_lightness_default_status_send(model, ctx);
}

static int light_lightness_default_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint16_t lightness;
    struct bt_mesh_light_lightness_srv *srv = model->rt->user_data;

    lightness = net_buf_simple_pull_le16(buf);
    if (lightness == 0)
        lightness = srv->state.last;

    LOG_DBG("%u", srv->state.def);
    srv->state.def = lightness;
    return 0;
}

static int light_lightness_default_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    int ret = light_lightness_default_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return light_lightness_default_status_send(model, ctx);
}

static int light_lightness_range_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    uint8_t status)
{
    struct bt_mesh_light_lightness_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_LIGHT_LIGHTNESS_RANGE_STATUS, 5);
    bt_mesh_model_msg_init(&msg, OP_LIGHT_LIGHTNESS_RANGE_STATUS);
    net_buf_simple_add_u8(&msg, status);
    net_buf_simple_add_le16(&msg, srv->state.range_min);
    net_buf_simple_add_le16(&msg, srv->state.range_max);

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int light_lightness_range_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    return light_lightness_range_status_send(model, ctx, 0);
}

static int light_lightness_range_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint16_t range_min, range_max;
    struct bt_mesh_light_lightness_srv *srv = model->rt->user_data;

    range_min = net_buf_simple_pull_le16(buf);
    range_max = net_buf_simple_pull_le16(buf);

    LOG_DBG("%u %u", range_min, range_max);

    if (range_min == 0 || range_max == 0 || range_min > range_max)
        return -EINVAL;

    srv->state.range_min = range_min;
    srv->state.range_max = range_max;

    return 0;
}

static int light_lightness_range_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    int ret = light_lightness_range_set_unack(model, ctx, buf);
    if (ret < 0 || ret >= BT_MESH_STATUS_UNKNOWN)
        return ret;

    return light_lightness_range_status_send(model, ctx, ret);
}

const struct bt_mesh_model_op bt_mesh_light_lightness_srv_op[] = {
    { OP_LIGHT_LIGHTNESS_GET,                   BT_MESH_LEN_EXACT(0),       light_lightness_get },
    { OP_LIGHT_LIGHTNESS_SET,                   BT_MESH_LEN_MIN(3),         light_lightness_set },
    { OP_LIGHT_LIGHTNESS_SET_UNACK,             BT_MESH_LEN_MIN(3),         light_lightness_set_unack },
    { OP_LIGHT_LIGHTNESS_LINEAR_GET,            BT_MESH_LEN_EXACT(0),       light_lightness_get },
    { OP_LIGHT_LIGHTNESS_LINEAR_SET,            BT_MESH_LEN_MIN(3),         light_lightness_set },
    { OP_LIGHT_LIGHTNESS_LINEAR_SET_UNACK,      BT_MESH_LEN_MIN(3),         light_lightness_set_unack },
    { OP_LIGHT_LIGHTNESS_LAST_GET,              BT_MESH_LEN_EXACT(0),       light_lightness_last_get },
    { OP_LIGHT_LIGHTNESS_DEFAULT_GET,           BT_MESH_LEN_EXACT(0),       light_lightness_default_get },
    { OP_LIGHT_LIGHTNESS_RANGE_GET,             BT_MESH_LEN_EXACT(0),       light_lightness_range_get },
    BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op bt_mesh_light_lightness_setup_srv_op[] = {
    { OP_LIGHT_LIGHTNESS_DEFAULT_SET,           BT_MESH_LEN_EXACT(2),       light_lightness_default_set },
    { OP_LIGHT_LIGHTNESS_DEFAULT_SET_UNACK,     BT_MESH_LEN_EXACT(2),       light_lightness_default_set_unack },
    { OP_LIGHT_LIGHTNESS_RANGE_SET,             BT_MESH_LEN_EXACT(4),       light_lightness_range_set },
    { OP_LIGHT_LIGHTNESS_RANGE_SET_UNACK,       BT_MESH_LEN_EXACT(4),       light_lightness_range_set_unack },
    BT_MESH_MODEL_OP_END,
};

static int light_lightness_pub_update(const struct bt_mesh_model *model)
{
    struct bt_mesh_light_lightness_srv *srv = model->rt->user_data;

    bt_mesh_model_msg_init(model->pub->msg, OP_LIGHT_LIGHTNESS_STATUS);
    net_buf_simple_add_le16(model->pub->msg, srv->state.actual);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_le16(model->pub->msg, srv->state.target_actual);
        net_buf_simple_add_u8(model->pub->msg, srv->transition.remain_time);
    }

    return 0;
}

void light_lightness_config(struct bt_mesh_light_lightness_srv *srv, uint16_t lightness)
{
    LOG_INF("%u", lightness);
    srv->state.actual = lightness;
    gen_level_config(&srv->level, light_common_to_gen_level(lightness));
    gen_power_onoff_config(&srv->power_onoff, light_actual_to_gen_onoff(lightness));
}

void light_lightness_status_publish(const struct bt_mesh_model *model)
{
    LOG_INF("");
    light_lightness_pub_update(model);
    bt_mesh_model_publish(model);
}

static void light_lightness_work_handler(struct k_work *work)
{
    struct bt_mesh_light_lightness_srv *srv = CONTAINER_OF(work, struct bt_mesh_light_lightness_srv, transition.timer.work);

    LOG_DBG("just_started:%u counter:%u, quo_tt:%u  %d", srv->transition.just_started, srv->transition.counter,
        srv->transition.quo_tt, srv->state.delta_lightness);

    if (srv->transition.just_started) {
        srv->transition.just_started = false;

        if (srv->transition.counter == 0U) {
            light_lightness_config(srv, srv->state.target_actual);
            light_lightness_status_publish(srv->model);

            if (srv->cb != NULL && srv->cb->state_change != NULL)
                srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_LIGHT_LIGHTNESS_EVT, &srv->state);
        } else {
            srv->transition.start_timestamp = k_uptime_get();
            k_work_reschedule(&srv->transition.timer, K_MSEC(srv->transition.quo_tt));
        }

        return;
    }

    if (srv->transition.counter == 0 || (--srv->transition.counter) == 0) {
        light_lightness_config(srv, srv->state.target_actual);
    } else {
        light_lightness_config(srv, srv->state.actual + srv->state.delta_lightness);
        k_work_reschedule(&srv->transition.timer, K_MSEC(srv->transition.quo_tt));
    }

    light_lightness_status_publish(srv->model);
    if (srv->cb != NULL && srv->cb->state_change != NULL)
        srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_LIGHT_LIGHTNESS_EVT, &srv->state);
}

static void bt_mesh_light_lightness_cb_get(void *user_data, enum bt_mesh_srv_callback_evt evt, void *state)
{
    struct bt_mesh_light_lightness_srv *cur_srv = user_data;

    LOG_DBG("%u", evt);

    if (cur_srv->cb != NULL && cur_srv->cb->get != NULL)
        cur_srv->cb->get(cur_srv->cb->user_data, BT_MESH_SRV_LIGHT_LIGHTNESS_EVT, &cur_srv->state);

    switch (evt) {
    case BT_MESH_SRV_GEN_ONOFF_EVT: {
        struct bt_mesh_gen_onoff_state *onoff = state;
        onoff->onoff = light_actual_to_gen_onoff(cur_srv->state.actual);
    }
        break;
    case BT_MESH_SRV_GEN_LEVEL_EVT: {
        struct bt_mesh_gen_level_state *level = state;
        level->level = light_common_to_gen_level(cur_srv->state.actual);
    }
        break;
    default:
        break;
    }
}

static void bt_mesh_light_lightness_cb_set(void *user_data, enum bt_mesh_srv_callback_evt evt, void *srv)
{
    struct bt_mesh_light_lightness_srv *cur_srv = user_data;

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
        cur_srv->state.last = cur_srv->state.actual;
        break;
    }
    case BT_MESH_SRV_GEN_LEVEL_EVT: {
        struct bt_mesh_gen_level_srv *level = srv;
        cur_srv->state.target_actual = gen_level_to_light_common(level->state.target_level);
        if (cur_srv->state.target_actual != 0) {
            if (cur_srv->state.range_min && cur_srv->state.target_actual < cur_srv->state.range_min) {
                cur_srv->state.target_actual = cur_srv->state.range_min;
            } else if (cur_srv->state.range_max && cur_srv->state.target_actual > cur_srv->state.range_max) {
                cur_srv->state.target_actual = cur_srv->state.range_max;
            }
        }
        cur_srv->state.last = cur_srv->state.actual;
        break;
    }
    default:
        break;
    }

    if (cur_srv->cb != NULL && cur_srv->cb->set != NULL)
        cur_srv->cb->set(cur_srv->cb->user_data, BT_MESH_SRV_LIGHT_LIGHTNESS_EVT, cur_srv);

    switch (evt) {
    case BT_MESH_SRV_GEN_ONOFF_EVT: {
        struct bt_mesh_gen_onoff_srv *onoff = srv;
        onoff->state.target_onoff = light_actual_to_gen_onoff(cur_srv->state.target_actual);
        cur_srv->transition.child = &onoff->transition;
    }
        break;
    case BT_MESH_SRV_GEN_LEVEL_EVT: {
        struct bt_mesh_gen_level_srv *level = srv;
        level->state.target_level = light_common_to_gen_level(cur_srv->state.target_actual);
        cur_srv->transition.child = &level->transition;
    }
        break;
    default:
        break;
    }
}

static void bt_mesh_light_lightness_cb_state_change(void *user_data, enum bt_mesh_srv_callback_evt evt, void *state)
{
    struct bt_mesh_light_lightness_srv *srv = user_data;

    LOG_DBG("%u", evt);

    switch (evt) {
    case BT_MESH_SRV_GEN_ONOFF_EVT: {
        struct bt_mesh_gen_onoff_state *onoff = state;
        if (onoff->onoff == 1) {
            srv->state.actual = srv->state.def == 0 ? srv->state.last : srv->state.def;
        } else {
            srv->state.actual = 0;
        }
        gen_level_config(&srv->level, light_common_to_gen_level(srv->state.actual));
    }
        break;
    case BT_MESH_SRV_GEN_LEVEL_EVT: {
        struct bt_mesh_gen_level_state *level = state;
        uint16_t lightness = gen_level_to_light_common(level->level);
        if (lightness == 0) {
            srv->state.actual = 0;
        } else if (srv->state.range_min && lightness < srv->state.range_min) {
            srv->state.actual = srv->state.range_min;
        } else {
            srv->state.actual = lightness;
        }

        gen_power_onoff_config(&srv->power_onoff, light_actual_to_gen_onoff(srv->state.actual));
    }
        break;
    default:
        break;
    }

    if (srv->cb != NULL && srv->cb->state_change != NULL)
        srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_LIGHT_LIGHTNESS_EVT, &srv->state);
}

static struct bt_mesh_srv_callbacks bt_mesh_light_lightness_cb = {
    .get = bt_mesh_light_lightness_cb_get,
    .set = bt_mesh_light_lightness_cb_set,
    .state_change = bt_mesh_light_lightness_cb_state_change
};

static int bt_mesh_light_lightness_srv_init(const struct bt_mesh_model *model)
{
    struct bt_mesh_light_lightness_srv *srv = model->rt->user_data;

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
    model->pub->update = light_lightness_pub_update;

    bt_mesh_light_lightness_cb.user_data = srv;
    srv->level.cb = &bt_mesh_light_lightness_cb;
    srv->power_onoff.cb = &bt_mesh_light_lightness_cb;

    k_work_init_delayable(&srv->transition.timer, light_lightness_work_handler);

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_light_lightness_srv_cb = {
    .init = bt_mesh_light_lightness_srv_init,
};

static int bt_mesh_light_lightness_setup_srv_init(const struct bt_mesh_model *model)
{
    struct bt_mesh_light_lightness_srv *srv = model->rt->user_data;

    LOG_INF("");

    if (!srv) {
        LOG_ERR("No Server context provided");
        return -EINVAL;
    }

    srv->setup_model = model;

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_light_lightness_setup_srv_cb = {
    .init = bt_mesh_light_lightness_setup_srv_init,
};

static int light_ctl_temperature_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    struct bt_mesh_light_ctl_temperature_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_LIGHT_CTL_TEMPERATURE_STATUS, 9);
    bt_mesh_model_msg_init(&msg, OP_LIGHT_CTL_TEMPERATURE_STATUS);
    net_buf_simple_add_le16(&msg, srv->state.temp);
    net_buf_simple_add_le16(&msg, srv->state.deltauv);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_le16(&msg, srv->state.target_temp);
        net_buf_simple_add_le16(&msg, srv->state.target_deltauv);
        net_buf_simple_add_u8(&msg, srv->transition.remain_time);
    }

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int light_ctl_temperature_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_ctl_temperature_srv *srv = model->rt->user_data;

    if (srv->cb != NULL && srv->cb->get != NULL)
        srv->cb->get(srv->cb->user_data, BT_MESH_SRV_LIGHT_CTL_TEMP_EVT, &srv->state);

    return light_ctl_temperature_status_send(model, ctx);
}

static int light_ctl_temperature_set_ext(const struct bt_mesh_model *model, struct net_buf_simple *buf, uint16_t temp, int16_t deltauv)
{
    struct bt_mesh_light_ctl_temperature_srv *srv = model->rt->user_data;

    bt_mesh_server_stop_transition(&srv->transition);

    if (srv->state.temp_range_min != BT_MESH_TEMPERATURE_UNKNOWN && temp < srv->state.temp_range_min) {
        temp = srv->state.temp_range_min;
    } else if (srv->state.temp_range_max != BT_MESH_TEMPERATURE_UNKNOWN && temp > srv->state.temp_range_max) {
        temp = srv->state.temp_range_max;
    }

    srv->state.target_temp = temp;
    srv->state.target_deltauv = deltauv;

    if (srv->cb != NULL && srv->cb->set != NULL)
        srv->cb->set(srv->cb->user_data, BT_MESH_SRV_LIGHT_CTL_TEMP_EVT, srv);

    if (srv->state.target_temp == srv->state.temp && srv->state.target_deltauv == srv->state.deltauv)
        return 0;

    bt_mesh_srv_transition_get(model, &srv->transition, buf);
    LOG_DBG("temp:%u deltauv:%d transition_time:%u delay:%u", temp, deltauv, srv->transition.transition_time, srv->transition.delay);
    
    srv->state.last_temp = srv->state.temp;
    set_transition_values(&srv->transition);
    srv->state.delta_temp = ((float)(srv->state.target_temp - srv->state.temp) / srv->transition.counter);
    srv->state.delta_deltauv = ((float)(srv->state.target_deltauv - srv->state.deltauv) / srv->transition.counter);
    bt_mesh_server_start_transition(&srv->transition);

    return 0;
}

static int light_ctl_temperature_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint8_t tid;
    uint16_t temp;
    int16_t deltauv;
    struct bt_mesh_light_ctl_temperature_srv *srv = model->rt->user_data;

    if ((buf->len != 5U) && (buf->len != 7U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    temp = net_buf_simple_pull_le16(buf);
    deltauv = (int16_t)net_buf_simple_pull_le16(buf);
    if (temp < BT_MESH_TEMPERATURE_MIN || temp > BT_MESH_TEMPERATURE_MAX) {
        LOG_ERR("Invalid temperature 0x%04x", temp);
        return -EINVAL;
    }

    tid = net_buf_simple_pull_u8(buf);
    if (bt_mesh_tid_check_and_update(&srv->pre_tid, tid, ctx->addr, ctx->recv_dst))
        return 0;

    light_ctl_temperature_set_ext(model, buf, temp, deltauv);
    return 0;
}

static int light_ctl_temperature_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    int ret = light_ctl_temperature_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return light_ctl_temperature_status_send(model, ctx);
}

const struct bt_mesh_model_op bt_mesh_light_ctl_temperature_srv_op[] = {
    { OP_LIGHT_CTL_TEMPERATURE_GET,             BT_MESH_LEN_EXACT(0),       light_ctl_temperature_get },
    { OP_LIGHT_CTL_TEMPERATURE_SET,             BT_MESH_LEN_MIN(5),         light_ctl_temperature_set },
    { OP_LIGHT_CTL_TEMPERATURE_SET_UNACK,       BT_MESH_LEN_MIN(5),         light_ctl_temperature_set_unack },
    BT_MESH_MODEL_OP_END,
};

static int light_ctl_temperature_pub_update(const struct bt_mesh_model *model)
{
    struct bt_mesh_light_ctl_temperature_srv *srv = model->rt->user_data;

    bt_mesh_model_msg_init(model->pub->msg, OP_LIGHT_CTL_TEMPERATURE_STATUS);
    net_buf_simple_add_le16(model->pub->msg, srv->state.temp);
    net_buf_simple_add_le16(model->pub->msg, srv->state.deltauv);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_le16(model->pub->msg, srv->state.target_temp);
        net_buf_simple_add_le16(model->pub->msg, srv->state.target_deltauv);
        net_buf_simple_add_u8(model->pub->msg, srv->transition.remain_time);
    }

    return 0;
}

void light_ctl_temperature_config(struct bt_mesh_light_ctl_temperature_srv *srv, uint16_t temp, int16_t deltauv)
{
    LOG_INF("%u %d", temp, deltauv);
    srv->state.temp = temp;
    srv->state.deltauv = deltauv;
    gen_level_config(&srv->level, light_ctl_temp_to_gen_level(temp, srv->state.temp_range_min, srv->state.temp_range_max));
}

void light_ctl_temperature_status_publish(const struct bt_mesh_model *model)
{
    LOG_INF("");
    light_ctl_temperature_pub_update(model);
    bt_mesh_model_publish(model);
}

static void light_ctl_temperature_work_handler(struct k_work *work)
{
    struct bt_mesh_light_ctl_temperature_srv *srv = CONTAINER_OF(work, struct bt_mesh_light_ctl_temperature_srv, transition.timer.work);

    LOG_DBG("just_started:%u counter:%u, quo_tt:%u  %u %d", srv->transition.just_started, srv->transition.counter,
        srv->transition.quo_tt, srv->state.delta_temp, srv->state.delta_deltauv);

    if (srv->transition.just_started) {
        srv->transition.just_started = false;

        if (srv->transition.counter == 0U) {
            light_ctl_temperature_config(srv, srv->state.target_temp, srv->state.target_deltauv);
            light_ctl_temperature_status_publish(srv->model);

            if (srv->cb != NULL && srv->cb->state_change != NULL)
                srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_LIGHT_CTL_TEMP_EVT, &srv->state);
        } else {
            srv->transition.start_timestamp = k_uptime_get();
            k_work_reschedule(&srv->transition.timer, K_MSEC(srv->transition.quo_tt));
        }

        return;
    }

    if (srv->transition.counter == 0 || (--srv->transition.counter) == 0) {
        light_ctl_temperature_config(srv, srv->state.target_temp, srv->state.target_deltauv);
    } else {
        light_ctl_temperature_config(srv, srv->state.temp + srv->state.delta_temp,
            srv->state.deltauv + srv->state.delta_deltauv);
        k_work_reschedule(&srv->transition.timer, K_MSEC(srv->transition.quo_tt));
    }

    light_ctl_temperature_status_publish(srv->model);
    if (srv->cb != NULL && srv->cb->state_change != NULL)
        srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_LIGHT_CTL_TEMP_EVT, &srv->state);
}

static void bt_mesh_light_ctl_temperature_cb_get(void *user_data, enum bt_mesh_srv_callback_evt evt, void *state)
{
    struct bt_mesh_light_ctl_temperature_srv *cur_srv = user_data;

    LOG_DBG("%u", evt);

    if (cur_srv->cb != NULL && cur_srv->cb->get != NULL)
        cur_srv->cb->get(cur_srv->cb->user_data, BT_MESH_SRV_LIGHT_CTL_TEMP_EVT, &cur_srv->state);

    if (evt == BT_MESH_SRV_GEN_LEVEL_EVT) {
        struct bt_mesh_gen_level_state *level = state;
        level->level = light_ctl_temp_to_gen_level(cur_srv->state.temp, cur_srv->state.temp_range_min,
            cur_srv->state.temp_range_max);
    }
}

static void bt_mesh_light_ctl_temperature_cb_set(void *user_data, enum bt_mesh_srv_callback_evt evt, void *srv)
{
    struct bt_mesh_light_ctl_temperature_srv *cur_srv = user_data;

    bt_mesh_server_stop_transition(&cur_srv->transition);

    LOG_DBG("%u", evt);

    if (evt == BT_MESH_SRV_GEN_LEVEL_EVT) {
        struct bt_mesh_gen_level_srv *level = srv;
        cur_srv->state.target_temp = gen_level_to_light_ctl_temp(level->state.target_level,
            cur_srv->state.temp_range_min, cur_srv->state.temp_range_max);
        if (cur_srv->state.temp_range_min != BT_MESH_TEMPERATURE_UNKNOWN &&
            cur_srv->state.target_temp < cur_srv->state.temp_range_min) {
            cur_srv->state.target_temp = cur_srv->state.temp_range_min;
        } else if (cur_srv->state.temp_range_max != BT_MESH_TEMPERATURE_UNKNOWN &&
            cur_srv->state.target_temp > cur_srv->state.temp_range_max) {
            cur_srv->state.target_temp = cur_srv->state.temp_range_max;
        }
        cur_srv->state.last_temp = cur_srv->state.temp;
    }

    if (cur_srv->cb != NULL && cur_srv->cb->set != NULL)
        cur_srv->cb->set(cur_srv->cb->user_data, BT_MESH_SRV_LIGHT_CTL_TEMP_EVT, cur_srv);

    if (evt == BT_MESH_SRV_GEN_LEVEL_EVT) {
        struct bt_mesh_gen_level_srv *level = srv;
        level->state.target_level = light_ctl_temp_to_gen_level(cur_srv->state.target_temp,
            cur_srv->state.temp_range_min, cur_srv->state.temp_range_max);
        cur_srv->transition.child = &level->transition;
    }
}

static void bt_mesh_light_ctl_temperature_cb_state_change(void *user_data, enum bt_mesh_srv_callback_evt evt, void *state)
{
    struct bt_mesh_light_ctl_temperature_srv *srv = user_data;

    LOG_DBG("%u", evt);

    if (evt == BT_MESH_SRV_GEN_LEVEL_EVT) {
        struct bt_mesh_gen_level_state *level = state;
        srv->state.temp = gen_level_to_light_ctl_temp(level->level, srv->state.temp_range_min,
            srv->state.temp_range_max);
    }

    if (srv->cb != NULL && srv->cb->state_change != NULL)
        srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_LIGHT_CTL_TEMP_EVT, &srv->state);
}

static struct bt_mesh_srv_callbacks bt_mesh_light_ctl_temperature_cb = {
    .get = bt_mesh_light_ctl_temperature_cb_get,
    .set = bt_mesh_light_ctl_temperature_cb_set,
    .state_change = bt_mesh_light_ctl_temperature_cb_state_change
};

static int bt_mesh_light_ctl_temperature_srv_init(const struct bt_mesh_model *model)
{
    struct bt_mesh_light_ctl_temperature_srv *srv = model->rt->user_data;

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
    model->pub->update = light_ctl_temperature_pub_update;

    bt_mesh_light_ctl_temperature_cb.user_data = srv;
    srv->level.cb = &bt_mesh_light_ctl_temperature_cb;

    k_work_init_delayable(&srv->transition.timer, light_ctl_temperature_work_handler);

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_light_ctl_temperature_srv_cb = {
    .init = bt_mesh_light_ctl_temperature_srv_init,
};

static int light_ctl_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    struct bt_mesh_light_ctl_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_LIGHT_CTL_STATUS, 9);
    bt_mesh_model_msg_init(&msg, OP_LIGHT_CTL_STATUS);
    net_buf_simple_add_le16(&msg, srv->lightness->state.actual);
    net_buf_simple_add_le16(&msg, srv->temperature.state.temp);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_le16(&msg, srv->lightness->state.target_actual);
        net_buf_simple_add_le16(&msg, srv->temperature.state.target_temp);
        net_buf_simple_add_u8(&msg, srv->transition.remain_time);
    }

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int light_ctl_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    struct bt_mesh_light_ctl_srv *srv = model->rt->user_data;

    if (srv->lightness->cb != NULL && srv->lightness->cb->get != NULL)
        srv->lightness->cb->get(srv->lightness->cb->user_data, BT_MESH_SRV_LIGHT_LIGHTNESS_EVT, &srv->lightness->state);

    if (srv->temperature.cb != NULL && srv->temperature.cb->get != NULL)
        srv->temperature.cb->get(srv->temperature.cb->user_data, BT_MESH_SRV_LIGHT_CTL_TEMP_EVT, &srv->temperature.state);

    return light_ctl_status_send(model, ctx);
}

static int light_ctl_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint8_t tid;
    uint16_t lightness, temp;
    int16_t deltauv;
    struct bt_mesh_light_ctl_srv *srv = model->rt->user_data;

    if ((buf->len != 7U) && (buf->len != 9U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    lightness = net_buf_simple_pull_le16(buf);
    temp = net_buf_simple_pull_le16(buf);
    deltauv = (int16_t)net_buf_simple_pull_le16(buf);
    if (temp < BT_MESH_TEMPERATURE_MIN || temp > BT_MESH_TEMPERATURE_MAX) {
        LOG_ERR("Invalid temperature 0x%04x", temp);
        return -EINVAL;
    }

    tid = net_buf_simple_pull_u8(buf);
    if (bt_mesh_tid_check_and_update(&srv->pre_tid, tid, ctx->addr, ctx->recv_dst))
        return 0;

    light_ctl_temperature_set_ext(srv->temperature.model, buf, temp, deltauv);
    light_lightness_set_ext(srv->lightness->model, buf, lightness);

    bt_mesh_server_stop_transition(&srv->transition);
    bt_mesh_srv_transition_get(model, &srv->transition, buf);
    set_transition_values(&srv->transition);
    bt_mesh_server_start_transition(&srv->transition);
    return 0;
}

static int light_ctl_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    int ret = light_ctl_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return light_ctl_status_send(model, ctx);
}

static int light_ctl_temperature_range_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    uint8_t status)
{
    struct bt_mesh_light_ctl_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_LIGHT_CTL_TEMPERATURE_RANGE_STATUS, 5);
    bt_mesh_model_msg_init(&msg, OP_LIGHT_CTL_TEMPERATURE_RANGE_STATUS);
    net_buf_simple_add_u8(&msg, status);
    net_buf_simple_add_le16(&msg, srv->temperature.state.temp_range_min);
    net_buf_simple_add_le16(&msg, srv->temperature.state.temp_range_max);

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int light_ctl_temperature_range_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    return light_ctl_temperature_range_status_send(model, ctx, 0);
}

static int light_ctl_temperature_range_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint16_t range_min, range_max;
    struct bt_mesh_light_ctl_srv *srv = model->rt->user_data;

    range_min = net_buf_simple_pull_le16(buf);
    range_max = net_buf_simple_pull_le16(buf);

    LOG_DBG("%u %u", range_min, range_max);

    if (range_min > range_max || range_min == BT_MESH_TEMPERATURE_UNKNOWN || range_max == BT_MESH_TEMPERATURE_UNKNOWN)
        return -EINVAL;

    if (range_min < BT_MESH_TEMPERATURE_MIN)
        return BT_MESH_STATUS_CANNOT_SET_RANGE_MIN;

    if (range_max > BT_MESH_TEMPERATURE_MAX)
        return BT_MESH_STATUS_CANNOT_SET_RANGE_MAX;

    srv->temperature.state.temp_range_min = range_min;
    srv->temperature.state.temp_range_max = range_max;

    return 0;
}

static int light_ctl_temperature_range_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    int ret = light_ctl_temperature_range_set_unack(model, ctx, buf);
    if (ret < 0 || ret >= BT_MESH_STATUS_UNKNOWN)
        return ret;

    return light_ctl_temperature_range_status_send(model, ctx, ret);
}

static int light_ctl_default_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    struct bt_mesh_light_ctl_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_LIGHT_CTL_DEFAULT_STATUS, 6);
    bt_mesh_model_msg_init(&msg, OP_LIGHT_CTL_DEFAULT_STATUS);
    net_buf_simple_add_le16(&msg, srv->lightness->state.def);
    net_buf_simple_add_le16(&msg, srv->temperature.state.temp_def);
    net_buf_simple_add_le16(&msg, srv->temperature.state.deltauv_def);

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int light_ctl_default_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    return light_ctl_default_status_send(model, ctx);
}

static int light_ctl_default_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint16_t lightness, temp;
    int16_t deltauv;
    struct bt_mesh_light_ctl_srv *srv = model->rt->user_data;

    lightness = net_buf_simple_pull_le16(buf);
    temp = net_buf_simple_pull_le16(buf);
    deltauv = (int16_t)net_buf_simple_pull_le16(buf);
    if (temp < BT_MESH_TEMPERATURE_MIN || temp > BT_MESH_TEMPERATURE_MAX) {
        return -EINVAL;
    }

    if (srv->temperature.state.temp_range_min != BT_MESH_TEMPERATURE_UNKNOWN &&
        temp < srv->temperature.state.temp_range_min) {
        temp = srv->temperature.state.temp_range_min;
    } else if (srv->temperature.state.temp_range_max != BT_MESH_TEMPERATURE_UNKNOWN &&
               temp > srv->temperature.state.temp_range_max) {
        temp = srv->temperature.state.temp_range_max;
    }

    srv->lightness->state.def = lightness;
    srv->temperature.state.temp_def = temp;
    srv->temperature.state.deltauv_def = deltauv;

    return 0;
}

static int light_ctl_default_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    int ret = light_ctl_default_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return light_ctl_default_status_send(model, ctx);
}

const struct bt_mesh_model_op bt_mesh_light_ctl_srv_op[] = {
    { OP_LIGHT_CTL_GET,                         BT_MESH_LEN_EXACT(0),       light_ctl_get },
    { OP_LIGHT_CTL_SET,                         BT_MESH_LEN_MIN(7),         light_ctl_set },
    { OP_LIGHT_CTL_SET_UNACK,                   BT_MESH_LEN_MIN(7),         light_ctl_set_unack },
    { OP_LIGHT_CTL_TEMPERATURE_RANGE_GET,       BT_MESH_LEN_MIN(0),         light_ctl_temperature_range_get },
    { OP_LIGHT_CTL_DEFAULT_GET,                 BT_MESH_LEN_MIN(0),         light_ctl_default_get },
    BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op bt_mesh_light_ctl_setup_srv_op[] = {
    { OP_LIGHT_CTL_TEMPERATURE_RANGE_SET,       BT_MESH_LEN_EXACT(4),         light_ctl_default_set },
    { OP_LIGHT_CTL_TEMPERATURE_RANGE_SET_UNACK, BT_MESH_LEN_EXACT(4),         light_ctl_default_set_unack },
    { OP_LIGHT_CTL_DEFAULT_SET,                 BT_MESH_LEN_EXACT(6),         light_ctl_temperature_range_set },
    { OP_LIGHT_CTL_DEFAULT_SET_UNACK,           BT_MESH_LEN_EXACT(6),         light_ctl_temperature_range_set_unack },
    BT_MESH_MODEL_OP_END,
};

static int light_ctl_pub_update(const struct bt_mesh_model *model)
{
    struct bt_mesh_light_ctl_srv *srv = model->rt->user_data;

    bt_mesh_model_msg_init(model->pub->msg, OP_LIGHT_CTL_STATUS);
    net_buf_simple_add_le16(model->pub->msg, srv->lightness->state.actual);
    net_buf_simple_add_le16(model->pub->msg, srv->temperature.state.temp);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_le16(model->pub->msg, srv->lightness->state.target_actual);
        net_buf_simple_add_le16(model->pub->msg, srv->temperature.state.target_temp);
        net_buf_simple_add_u8(model->pub->msg, srv->transition.remain_time);
    }

    return 0;
}

void light_ctl_status_publish(const struct bt_mesh_model *model)
{
    LOG_INF("");
    light_ctl_pub_update(model);
    bt_mesh_model_publish(model);
}

static void light_ctl_work_handler(struct k_work *work)
{
    struct bt_mesh_light_ctl_srv *srv = CONTAINER_OF(work, struct bt_mesh_light_ctl_srv, transition.timer.work);

    LOG_DBG("just_started:%u counter:%u, quo_tt:%u", srv->transition.just_started, srv->transition.counter,
        srv->transition.quo_tt);

    if (srv->transition.just_started) {
        srv->transition.just_started = false;

        if (srv->transition.counter == 0U) {
            light_ctl_status_publish(srv->model);
        } else {
            srv->transition.start_timestamp = k_uptime_get();
            k_work_reschedule(&srv->transition.timer, K_MSEC(srv->transition.quo_tt));
        }

        return;
    }

    if (srv->transition.counter == 0 || (--srv->transition.counter) == 0) {
        light_ctl_status_publish(srv->model);
    } else {
        light_ctl_status_publish(srv->model);
        k_work_reschedule(&srv->transition.timer, K_MSEC(srv->transition.quo_tt));
    }
}

static void bt_mesh_light_ctl_cb_get(void *user_data, enum bt_mesh_srv_callback_evt evt, void *state)
{
    struct bt_mesh_light_ctl_srv *cur_srv = user_data;

    LOG_DBG("%u", evt);

    if (cur_srv->cb != NULL && cur_srv->cb->get != NULL)
        cur_srv->cb->get(cur_srv->cb->user_data, evt, state);
}

static void bt_mesh_light_ctl_cb_set(void *user_data, enum bt_mesh_srv_callback_evt evt, void *srv)
{
    struct bt_mesh_light_ctl_srv *cur_srv = user_data;

    LOG_DBG("%u", evt);

    if (cur_srv->cb != NULL && cur_srv->cb->set != NULL)
        cur_srv->cb->set(cur_srv->cb->user_data, evt, srv);
}

static void bt_mesh_light_ctl_cb_state_change(void *user_data, enum bt_mesh_srv_callback_evt evt, void *state)
{
    struct bt_mesh_light_ctl_srv *cur_srv = user_data;

    LOG_DBG("%u", evt);

    if (cur_srv->cb != NULL && cur_srv->cb->state_change != NULL)
        cur_srv->cb->state_change(cur_srv->cb->user_data, evt, state);
}

static struct bt_mesh_srv_callbacks bt_mesh_light_ctl_cb = {
    .get = bt_mesh_light_ctl_cb_get,
    .set = bt_mesh_light_ctl_cb_set,
    .state_change = bt_mesh_light_ctl_cb_state_change
};

static int bt_mesh_light_ctl_srv_init(const struct bt_mesh_model *model)
{
    struct bt_mesh_light_ctl_srv *srv = model->rt->user_data;

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
    model->pub->update = light_ctl_pub_update;

    bt_mesh_light_ctl_cb.user_data = srv;
    srv->lightness->cb = &bt_mesh_light_ctl_cb;
    srv->temperature.cb = &bt_mesh_light_ctl_cb;

    k_work_init_delayable(&srv->transition.timer, light_ctl_work_handler);

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_light_ctl_srv_cb = {
    .init = bt_mesh_light_ctl_srv_init,
};

static int bt_mesh_light_ctl_setup_srv_init(const struct bt_mesh_model *model)
{
    struct bt_mesh_light_ctl_srv *srv = model->rt->user_data;

    LOG_INF("");

    if (!srv) {
        LOG_ERR("No Server context provided");
        return -EINVAL;
    }

    srv->setup_model = model;

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_light_ctl_setup_srv_cb = {
    .init = bt_mesh_light_ctl_setup_srv_init,
};

static int light_hsl_hue_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    struct bt_mesh_light_hsl_hue_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_LIGHT_HSL_HUE_STATUS, 5);
    bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_HUE_STATUS);
    net_buf_simple_add_le16(&msg, srv->state.hue);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_le16(&msg, srv->state.target_hue);
        net_buf_simple_add_u8(&msg, srv->transition.remain_time);
    }

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int light_hsl_hue_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_hsl_hue_srv *srv = model->rt->user_data;

    if (srv->cb != NULL && srv->cb->get != NULL)
        srv->cb->get(srv->cb->user_data, BT_MESH_SRV_LIGHT_HSL_HUE_EVT, &srv->state);

    return light_hsl_hue_status_send(model, ctx);
}

static int light_hsl_hue_set_ext(const struct bt_mesh_model *model, struct net_buf_simple *buf, uint16_t hue)
{
    struct bt_mesh_light_hsl_hue_srv *srv = model->rt->user_data;

    bt_mesh_server_stop_transition(&srv->transition);

    hue = light_hsl_hue_update(hue, srv->state.hue_range_min, srv->state.hue_range_max);
    srv->state.target_hue = hue;

    if (srv->cb != NULL && srv->cb->set != NULL)
        srv->cb->set(srv->cb->user_data, BT_MESH_SRV_LIGHT_HSL_HUE_EVT, srv);

    if (srv->state.target_hue == srv->state.hue)
        return 0;

    bt_mesh_srv_transition_get(model, &srv->transition, buf);
    LOG_DBG("temp:%u transition_time:%u delay:%u", hue, srv->transition.transition_time, srv->transition.delay);

    srv->state.last_hue = srv->state.hue;
    set_transition_values(&srv->transition);
    srv->state.delta_hue = ((float)(srv->state.target_hue - srv->state.hue) / srv->transition.counter);
    bt_mesh_server_start_transition(&srv->transition);

    return 0;
}

static int light_hsl_hue_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint8_t tid;
    uint16_t hue;
    struct bt_mesh_light_hsl_hue_srv *srv = model->rt->user_data;

    if ((buf->len != 3U) && (buf->len != 5U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    hue = net_buf_simple_pull_le16(buf);
    tid = net_buf_simple_pull_u8(buf);
    if (bt_mesh_tid_check_and_update(&srv->pre_tid, tid, ctx->addr, ctx->recv_dst))
        return 0;

    light_hsl_hue_set_ext(model, buf, hue);
    return 0;
}

static int light_hsl_hue_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    int ret = light_hsl_hue_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return light_hsl_hue_status_send(model, ctx);
}

const struct bt_mesh_model_op bt_mesh_light_hsl_hue_srv_op[] = {
    { OP_LIGHT_HSL_HUE_GET,             BT_MESH_LEN_EXACT(0),       light_hsl_hue_get },
    { OP_LIGHT_HSL_HUE_SET,             BT_MESH_LEN_MIN(5),         light_hsl_hue_set },
    { OP_LIGHT_HSL_HUE_SET_UNACK,       BT_MESH_LEN_MIN(5),         light_hsl_hue_set_unack },
    BT_MESH_MODEL_OP_END,
};

static int light_hsl_hue_pub_update(const struct bt_mesh_model *model)
{
    struct bt_mesh_light_hsl_hue_srv *srv = model->rt->user_data;

    bt_mesh_model_msg_init(model->pub->msg, OP_LIGHT_HSL_HUE_STATUS);
    net_buf_simple_add_le16(model->pub->msg, srv->state.hue);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_le16(model->pub->msg, srv->state.target_hue);
        net_buf_simple_add_u8(model->pub->msg, srv->transition.remain_time);
    }

    return 0;
}

void light_hsl_hue_config(struct bt_mesh_light_hsl_hue_srv *srv, uint16_t hue)
{
    LOG_INF("%u", hue);
    srv->state.hue = hue;
    gen_level_config(&srv->level, light_common_to_gen_level(hue));
}

void light_hsl_hue_status_publish(const struct bt_mesh_model *model)
{
    LOG_INF("");
    light_hsl_hue_pub_update(model);
    bt_mesh_model_publish(model);
}

static void light_hsl_hue_work_handler(struct k_work *work)
{
    struct bt_mesh_light_hsl_hue_srv *srv = CONTAINER_OF(work, struct bt_mesh_light_hsl_hue_srv, transition.timer.work);

    LOG_DBG("just_started:%u counter:%u, quo_tt:%u  %d", srv->transition.just_started, srv->transition.counter,
        srv->transition.quo_tt, srv->state.delta_hue);

    if (srv->transition.just_started) {
        srv->transition.just_started = false;

        if (srv->transition.counter == 0U) {
            light_hsl_hue_config(srv, srv->state.target_hue);
            light_hsl_hue_status_publish(srv->model);

            if (srv->cb != NULL && srv->cb->state_change != NULL)
                srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_LIGHT_HSL_HUE_EVT, &srv->state);
        } else {
            srv->transition.start_timestamp = k_uptime_get();
            k_work_reschedule(&srv->transition.timer, K_MSEC(srv->transition.quo_tt));
        }

        return;
    }

    if (srv->transition.counter == 0 || (--srv->transition.counter) == 0) {
        light_hsl_hue_config(srv, srv->state.target_hue);
    } else {
        light_hsl_hue_config(srv, srv->state.hue + srv->state.delta_hue);
        k_work_reschedule(&srv->transition.timer, K_MSEC(srv->transition.quo_tt));
    }

    light_hsl_hue_status_publish(srv->model);
    if (srv->cb != NULL && srv->cb->state_change != NULL)
        srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_LIGHT_HSL_HUE_EVT, &srv->state);
}

static void bt_mesh_light_hsl_hue_cb_get(void *user_data, enum bt_mesh_srv_callback_evt evt, void *state)
{
    struct bt_mesh_light_hsl_hue_srv *cur_srv = user_data;

    LOG_DBG("%u", evt);

    if (cur_srv->cb != NULL && cur_srv->cb->get != NULL)
        cur_srv->cb->get(cur_srv->cb->user_data, BT_MESH_SRV_LIGHT_HSL_HUE_EVT, &cur_srv->state);

    if (evt == BT_MESH_SRV_GEN_LEVEL_EVT) {
        struct bt_mesh_gen_level_state *level = state;
        level->level = light_common_to_gen_level(cur_srv->state.hue);
    }
}

static void bt_mesh_light_hsl_hue_cb_set(void *user_data, enum bt_mesh_srv_callback_evt evt, void *srv)
{
    struct bt_mesh_light_hsl_hue_srv *cur_srv = user_data;

    bt_mesh_server_stop_transition(&cur_srv->transition);

    LOG_DBG("%u", evt);

    if (evt == BT_MESH_SRV_GEN_LEVEL_EVT) {
        struct bt_mesh_gen_level_srv *level = srv;
        cur_srv->state.target_hue = light_hsl_hue_update(gen_level_to_light_common(level->state.target_level),
            cur_srv->state.hue_range_min, cur_srv->state.hue_range_max);
        cur_srv->state.last_hue = cur_srv->state.hue;
    }

    if (cur_srv->cb != NULL && cur_srv->cb->set != NULL)
        cur_srv->cb->set(cur_srv->cb->user_data, BT_MESH_SRV_LIGHT_HSL_HUE_EVT, cur_srv);

    if (evt == BT_MESH_SRV_GEN_LEVEL_EVT) {
        struct bt_mesh_gen_level_srv *level = srv;
        level->state.target_level = light_common_to_gen_level(cur_srv->state.target_hue);
        cur_srv->transition.child = &level->transition;
    }
}

static void bt_mesh_light_hsl_hue_cb_state_change(void *user_data, enum bt_mesh_srv_callback_evt evt, void *state)
{
    struct bt_mesh_light_hsl_hue_srv *srv = user_data;

    LOG_DBG("%u", evt);

    if (evt == BT_MESH_SRV_GEN_LEVEL_EVT) {
        struct bt_mesh_gen_level_state *level = state;
        srv->state.hue = gen_level_to_light_common(level->level);
    }

    if (srv->cb != NULL && srv->cb->state_change != NULL)
        srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_LIGHT_HSL_HUE_EVT, &srv->state);
}

static struct bt_mesh_srv_callbacks bt_mesh_light_hsl_hue_cb = {
    .get = bt_mesh_light_hsl_hue_cb_get,
    .set = bt_mesh_light_hsl_hue_cb_set,
    .state_change = bt_mesh_light_hsl_hue_cb_state_change
};

static int bt_mesh_light_hsl_hue_srv_init(const struct bt_mesh_model *model)
{
    struct bt_mesh_light_hsl_hue_srv *srv = model->rt->user_data;

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
    model->pub->update = light_hsl_hue_pub_update;

    bt_mesh_light_hsl_hue_cb.user_data = srv;
    srv->level.cb = &bt_mesh_light_hsl_hue_cb;

    k_work_init_delayable(&srv->transition.timer, light_hsl_hue_work_handler);

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_light_hsl_hue_srv_cb = {
    .init = bt_mesh_light_hsl_hue_srv_init,
};

static int light_hsl_saturation_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    struct bt_mesh_light_hsl_saturation_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_LIGHT_HSL_SATURATION_STATUS, 5);
    bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_SATURATION_STATUS);
    net_buf_simple_add_le16(&msg, srv->state.saturation);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_le16(&msg, srv->state.target_saturation);
        net_buf_simple_add_u8(&msg, srv->transition.remain_time);
    }

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int light_hsl_saturation_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_hsl_saturation_srv *srv = model->rt->user_data;

    if (srv->cb != NULL && srv->cb->get != NULL)
        srv->cb->get(srv->cb->user_data, BT_MESH_SRV_LIGHT_HSL_SATURATION_EVT, &srv->state);

    return light_hsl_saturation_status_send(model, ctx);
}

static int light_hsl_saturation_set_ext(const struct bt_mesh_model *model, struct net_buf_simple *buf, uint16_t saturation)
{
    struct bt_mesh_light_hsl_saturation_srv *srv = model->rt->user_data;

    bt_mesh_server_stop_transition(&srv->transition);

    if (saturation < srv->state.saturation_range_min) {
        saturation = srv->state.saturation_range_min;
    } else if (saturation > srv->state.saturation_range_max) {
        saturation = srv->state.saturation_range_max;
    }
    srv->state.target_saturation = saturation;

    if (srv->cb != NULL && srv->cb->set != NULL)
        srv->cb->set(srv->cb->user_data, BT_MESH_SRV_LIGHT_HSL_SATURATION_EVT, srv);

    if (srv->state.target_saturation == srv->state.saturation)
        return 0;

    bt_mesh_srv_transition_get(model, &srv->transition, buf);
    LOG_DBG("saturation:%u transition_time:%u delay:%u", saturation, srv->transition.transition_time, srv->transition.delay);

    srv->state.last_saturation = srv->state.saturation;
    set_transition_values(&srv->transition);
    srv->state.delta_saturation = ((float)(srv->state.target_saturation - srv->state.saturation) / srv->transition.counter);
    bt_mesh_server_start_transition(&srv->transition);

    return 0;
}

static int light_hsl_saturation_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint8_t tid;
    uint16_t saturation;
    struct bt_mesh_light_hsl_saturation_srv *srv = model->rt->user_data;

    if ((buf->len != 3U) && (buf->len != 5U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    saturation = net_buf_simple_pull_le16(buf);
    tid = net_buf_simple_pull_u8(buf);
    if (bt_mesh_tid_check_and_update(&srv->pre_tid, tid, ctx->addr, ctx->recv_dst))
        return 0;

    light_hsl_saturation_set_ext(model, buf, saturation);
    return 0;
}

static int light_hsl_saturation_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    int ret = light_hsl_saturation_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return light_hsl_saturation_status_send(model, ctx);
}

const struct bt_mesh_model_op bt_mesh_light_hsl_saturation_srv_op[] = {
    { OP_LIGHT_HSL_SATURATION_GET,             BT_MESH_LEN_EXACT(0),       light_hsl_saturation_get },
    { OP_LIGHT_HSL_SATURATION_SET,             BT_MESH_LEN_MIN(5),         light_hsl_saturation_set },
    { OP_LIGHT_HSL_SATURATION_SET_UNACK,       BT_MESH_LEN_MIN(5),         light_hsl_saturation_set_unack },
    BT_MESH_MODEL_OP_END,
};

static int light_hsl_saturation_pub_update(const struct bt_mesh_model *model)
{
    struct bt_mesh_light_hsl_saturation_srv *srv = model->rt->user_data;

    bt_mesh_model_msg_init(model->pub->msg, OP_LIGHT_HSL_SATURATION_STATUS);
    net_buf_simple_add_le16(model->pub->msg, srv->state.saturation);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_le16(model->pub->msg, srv->state.target_saturation);
        net_buf_simple_add_u8(model->pub->msg, srv->transition.remain_time);
    }

    return 0;
}

void light_hsl_saturation_config(struct bt_mesh_light_hsl_saturation_srv *srv, uint16_t saturation)
{
    LOG_INF("%u", saturation);
    srv->state.saturation = saturation;
    gen_level_config(&srv->level, light_common_to_gen_level(saturation));
}

void light_hsl_saturation_status_publish(const struct bt_mesh_model *model)
{
    LOG_INF("");
    light_hsl_saturation_pub_update(model);
    bt_mesh_model_publish(model);
}

static void light_hsl_saturation_work_handler(struct k_work *work)
{
    struct bt_mesh_light_hsl_saturation_srv *srv = CONTAINER_OF(work, struct bt_mesh_light_hsl_saturation_srv, transition.timer.work);

    LOG_DBG("just_started:%u counter:%u, quo_tt:%u  %d", srv->transition.just_started, srv->transition.counter,
        srv->transition.quo_tt, srv->state.delta_saturation);

    if (srv->transition.just_started) {
        srv->transition.just_started = false;

        if (srv->transition.counter == 0U) {
            light_hsl_saturation_config(srv, srv->state.target_saturation);
            light_hsl_saturation_status_publish(srv->model);

            if (srv->cb != NULL && srv->cb->state_change != NULL)
                srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_LIGHT_HSL_SATURATION_EVT, &srv->state);
        } else {
            srv->transition.start_timestamp = k_uptime_get();
            k_work_reschedule(&srv->transition.timer, K_MSEC(srv->transition.quo_tt));
        }

        return;
    }

    if (srv->transition.counter == 0 || (--srv->transition.counter) == 0) {
        light_hsl_saturation_config(srv, srv->state.target_saturation);
    } else {
        light_hsl_saturation_config(srv, srv->state.saturation + srv->state.delta_saturation);
        k_work_reschedule(&srv->transition.timer, K_MSEC(srv->transition.quo_tt));
    }

    light_hsl_saturation_status_publish(srv->model);
    if (srv->cb != NULL && srv->cb->state_change != NULL)
        srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_LIGHT_HSL_SATURATION_EVT, &srv->state);
}

static void bt_mesh_light_hsl_saturation_cb_get(void *user_data, enum bt_mesh_srv_callback_evt evt, void *state)
{
    struct bt_mesh_light_hsl_saturation_srv *cur_srv = user_data;

    LOG_DBG("%u", evt);

    if (cur_srv->cb != NULL && cur_srv->cb->get != NULL)
        cur_srv->cb->get(cur_srv->cb->user_data, BT_MESH_SRV_LIGHT_HSL_SATURATION_EVT, &cur_srv->state);

    if (evt == BT_MESH_SRV_GEN_LEVEL_EVT) {
        struct bt_mesh_gen_level_state *level = state;
        level->level = light_common_to_gen_level(cur_srv->state.saturation);
    }
}

static void bt_mesh_light_hsl_saturation_cb_set(void *user_data, enum bt_mesh_srv_callback_evt evt, void *srv)
{
    struct bt_mesh_light_hsl_saturation_srv *cur_srv = user_data;

    bt_mesh_server_stop_transition(&cur_srv->transition);

    LOG_DBG("%u", evt);

    if (evt == BT_MESH_SRV_GEN_LEVEL_EVT) {
        struct bt_mesh_gen_level_srv *level = srv;
        cur_srv->state.target_saturation = gen_level_to_light_common(level->state.target_level);
        if (cur_srv->state.target_saturation < cur_srv->state.saturation_range_min) {
            cur_srv->state.target_saturation = cur_srv->state.saturation_range_min;
        } else if (cur_srv->state.target_saturation > cur_srv->state.saturation_range_max) {
            cur_srv->state.target_saturation = cur_srv->state.saturation_range_max;
        }
        cur_srv->state.last_saturation = cur_srv->state.saturation;
    }

    if (cur_srv->cb != NULL && cur_srv->cb->set != NULL)
        cur_srv->cb->set(cur_srv->cb->user_data, BT_MESH_SRV_LIGHT_HSL_SATURATION_EVT, cur_srv);

    if (evt == BT_MESH_SRV_GEN_LEVEL_EVT) {
        struct bt_mesh_gen_level_srv *level = srv;
        level->state.target_level = light_common_to_gen_level(cur_srv->state.target_saturation);
        cur_srv->transition.child = &level->transition;
    }
}

static void bt_mesh_light_hsl_saturation_cb_state_change(void *user_data, enum bt_mesh_srv_callback_evt evt, void *state)
{
    struct bt_mesh_light_hsl_saturation_srv *srv = user_data;

    LOG_DBG("%u", evt);

    if (evt == BT_MESH_SRV_GEN_LEVEL_EVT) {
        struct bt_mesh_gen_level_state *level = state;
        srv->state.saturation = gen_level_to_light_common(level->level);
    }

    if (srv->cb != NULL && srv->cb->state_change != NULL)
        srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_LIGHT_HSL_SATURATION_EVT, &srv->state);
}

static struct bt_mesh_srv_callbacks bt_mesh_light_hsl_saturation_cb = {
    .get = bt_mesh_light_hsl_saturation_cb_get,
    .set = bt_mesh_light_hsl_saturation_cb_set,
    .state_change = bt_mesh_light_hsl_saturation_cb_state_change
};

static int bt_mesh_light_hsl_saturation_srv_init(const struct bt_mesh_model *model)
{
    struct bt_mesh_light_hsl_saturation_srv *srv = model->rt->user_data;

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
    model->pub->update = light_hsl_saturation_pub_update;

    bt_mesh_light_hsl_saturation_cb.user_data = srv;
    srv->level.cb = &bt_mesh_light_hsl_saturation_cb;

    k_work_init_delayable(&srv->transition.timer, light_hsl_saturation_work_handler);

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_light_hsl_saturation_srv_cb = {
    .init = bt_mesh_light_hsl_saturation_srv_init,
};

static int light_hsl_target_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_LIGHT_HSL_TARGET_STATUS, 7);
    bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_TARGET_STATUS);
    net_buf_simple_add_le16(&msg, srv->lightness->state.target_actual);
    net_buf_simple_add_le16(&msg, srv->hue.state.target_hue);
    net_buf_simple_add_le16(&msg, srv->saturation.state.target_saturation);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_u8(&msg, srv->transition.remain_time);
    }

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int light_hsl_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_LIGHT_HSL_STATUS, 7);
    bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_STATUS);
    net_buf_simple_add_le16(&msg, srv->lightness->state.actual);
    net_buf_simple_add_le16(&msg, srv->hue.state.hue);
    net_buf_simple_add_le16(&msg, srv->saturation.state.saturation);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_u8(&msg, srv->transition.remain_time);
    }

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int light_hsl_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;

    if (srv->lightness->cb != NULL && srv->lightness->cb->get != NULL)
        srv->lightness->cb->get(srv->lightness->cb->user_data, BT_MESH_SRV_LIGHT_LIGHTNESS_EVT, &srv->lightness->state);

    if (srv->hue.cb != NULL && srv->hue.cb->get != NULL)
        srv->hue.cb->get(srv->hue.cb->user_data, BT_MESH_SRV_LIGHT_HSL_HUE_EVT, &srv->hue.state);

    if (srv->saturation.cb != NULL && srv->saturation.cb->get != NULL)
        srv->saturation.cb->get(srv->saturation.cb->user_data, BT_MESH_SRV_LIGHT_HSL_SATURATION_EVT, &srv->saturation.state);

    return light_hsl_status_send(model, ctx);
}

static int light_hsl_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint8_t tid;
    uint16_t lightness, hue, saturation;
    struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;

    if ((buf->len != 7U) && (buf->len != 9U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    lightness = net_buf_simple_pull_le16(buf);
    hue = net_buf_simple_pull_le16(buf);
    saturation = net_buf_simple_pull_le16(buf);

    tid = net_buf_simple_pull_u8(buf);
    if (bt_mesh_tid_check_and_update(&srv->pre_tid, tid, ctx->addr, ctx->recv_dst))
        return 0;

    light_lightness_set_ext(srv->lightness->model, buf, lightness);
    light_hsl_hue_set_ext(srv->hue.model, buf, hue);
    light_hsl_saturation_set_ext(srv->saturation.model, buf, saturation);

    bt_mesh_server_stop_transition(&srv->transition);
    bt_mesh_srv_transition_get(model, &srv->transition, buf);
    set_transition_values(&srv->transition);
    bt_mesh_server_start_transition(&srv->transition);
    return 0;
}

static int light_hsl_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    int ret = light_hsl_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return light_hsl_status_send(model, ctx);
}

static int light_hsl_range_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    uint8_t status)
{
    struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_LIGHT_HSL_DEFAULT_STATUS, 9);
    bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_DEFAULT_STATUS);
    net_buf_simple_add_u8(&msg, status);
    net_buf_simple_add_le16(&msg, srv->hue.state.hue_range_min);
    net_buf_simple_add_le16(&msg, srv->hue.state.hue_range_max);
    net_buf_simple_add_le16(&msg, srv->saturation.state.saturation_range_min);
    net_buf_simple_add_le16(&msg, srv->saturation.state.saturation_range_max);

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int light_hsl_range_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    return light_hsl_range_status_send(model, ctx, 0);
}

static int light_hsl_range_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint16_t hue_range_min, hue_range_max, saturation_range_min, saturation_range_max;
    struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;

    hue_range_min = net_buf_simple_pull_le16(buf);
    hue_range_max = net_buf_simple_pull_le16(buf);
    saturation_range_min = net_buf_simple_pull_le16(buf);
    saturation_range_max = net_buf_simple_pull_le16(buf);

    LOG_DBG("%u %u", hue_range_min, hue_range_max, saturation_range_min, saturation_range_max);
    if (saturation_range_min > saturation_range_max)
        return -EINVAL;

    srv->hue.state.hue_range_min = hue_range_min;
    srv->hue.state.hue_range_max = hue_range_max;
    srv->saturation.state.saturation_range_min = saturation_range_min;
    srv->saturation.state.saturation_range_max = saturation_range_max;

    return 0;
}

static int light_hsl_range_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    int ret = light_hsl_range_set_unack(model, ctx, buf);
    if (ret < 0 || ret >= BT_MESH_STATUS_UNKNOWN)
        return ret;

    return light_hsl_range_status_send(model, ctx, ret);
}

static int light_hsl_default_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_LIGHT_HSL_DEFAULT_STATUS, 6);
    bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_DEFAULT_STATUS);
    net_buf_simple_add_le16(&msg, srv->lightness->state.def);
    net_buf_simple_add_le16(&msg, srv->hue.state.hue_def);
    net_buf_simple_add_le16(&msg, srv->saturation.state.saturation_def);

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int light_hsl_default_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    return light_hsl_default_status_send(model, ctx);
}

static int light_hsl_default_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint16_t lightness, hue, saturation;
    struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;

    lightness = net_buf_simple_pull_le16(buf);
    hue = net_buf_simple_pull_le16(buf);
    saturation = net_buf_simple_pull_le16(buf);

    srv->lightness->state.def = lightness;
    srv->hue.state.hue_def = hue;
    srv->saturation.state.saturation_def = saturation;

    return 0;
}

static int light_hsl_default_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    int ret = light_hsl_default_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return light_hsl_default_status_send(model, ctx);
}

const struct bt_mesh_model_op bt_mesh_light_hsl_srv_op[] = {
    { OP_LIGHT_HSL_GET,                         BT_MESH_LEN_EXACT(0),       light_hsl_get },
    { OP_LIGHT_HSL_SET,                         BT_MESH_LEN_MIN(7),         light_hsl_set },
    { OP_LIGHT_HSL_SET_UNACK,                   BT_MESH_LEN_MIN(7),         light_hsl_set_unack },
    { OP_LIGHT_HSL_TARGET_GET,                  BT_MESH_LEN_MIN(0),         light_hsl_target_get },
    { OP_LIGHT_HSL_RANGE_GET,                   BT_MESH_LEN_MIN(0),         light_hsl_range_get },
    { OP_LIGHT_HSL_DEFAULT_GET,                 BT_MESH_LEN_MIN(0),         light_hsl_default_get },
    BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op bt_mesh_light_hsl_setup_srv_op[] = {
    { OP_LIGHT_HSL_RANGE_SET,                   BT_MESH_LEN_EXACT(8),         light_hsl_default_set },
    { OP_LIGHT_HSL_RANGE_SET_UNACK,             BT_MESH_LEN_EXACT(8),         light_hsl_default_set_unack },
    { OP_LIGHT_HSL_DEFAULT_SET,                 BT_MESH_LEN_EXACT(6),         light_hsl_range_set },
    { OP_LIGHT_HSL_DEFAULT_SET_UNACK,           BT_MESH_LEN_EXACT(6),         light_hsl_range_set_unack },
    BT_MESH_MODEL_OP_END,
};

static int light_hsl_pub_update(const struct bt_mesh_model *model)
{
    struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;

    bt_mesh_model_msg_init(model->pub->msg, OP_LIGHT_HSL_STATUS);
    net_buf_simple_add_le16(model->pub->msg, srv->lightness->state.actual);
    net_buf_simple_add_le16(model->pub->msg, srv->hue.state.hue);
    net_buf_simple_add_le16(model->pub->msg, srv->saturation.state.saturation);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_u8(model->pub->msg, srv->transition.remain_time);
    }

    return 0;
}

void light_hsl_status_publish(const struct bt_mesh_model *model)
{
    LOG_INF("");
    light_hsl_pub_update(model);
    bt_mesh_model_publish(model);
}

static void light_hsl_work_handler(struct k_work *work)
{
    struct bt_mesh_light_hsl_srv *srv = CONTAINER_OF(work, struct bt_mesh_light_hsl_srv, transition.timer.work);

    LOG_DBG("just_started:%u counter:%u, quo_tt:%u", srv->transition.just_started, srv->transition.counter,
        srv->transition.quo_tt);

    if (srv->transition.just_started) {
        srv->transition.just_started = false;

        if (srv->transition.counter == 0U) {
            light_hsl_status_publish(srv->model);
        } else {
            srv->transition.start_timestamp = k_uptime_get();
            k_work_reschedule(&srv->transition.timer, K_MSEC(srv->transition.quo_tt));
        }

        return;
    }

    if (srv->transition.counter == 0 || (--srv->transition.counter) == 0) {
        light_hsl_status_publish(srv->model);
    } else {
        light_hsl_status_publish(srv->model);
        k_work_reschedule(&srv->transition.timer, K_MSEC(srv->transition.quo_tt));
    }
}

static void bt_mesh_light_hsl_cb_get(void *user_data, enum bt_mesh_srv_callback_evt evt, void *state)
{
    struct bt_mesh_light_hsl_srv *cur_srv = user_data;

    LOG_DBG("%u", evt);

    if (cur_srv->cb != NULL && cur_srv->cb->get != NULL)
        cur_srv->cb->get(cur_srv->cb->user_data, evt, state);
}

static void bt_mesh_light_hsl_cb_set(void *user_data, enum bt_mesh_srv_callback_evt evt, void *srv)
{
    struct bt_mesh_light_hsl_srv *cur_srv = user_data;

    LOG_DBG("%u", evt);

    if (cur_srv->cb != NULL && cur_srv->cb->set != NULL)
        cur_srv->cb->set(cur_srv->cb->user_data, evt, srv);
}

static void bt_mesh_light_hsl_cb_state_change(void *user_data, enum bt_mesh_srv_callback_evt evt, void *state)
{
    struct bt_mesh_light_hsl_srv *cur_srv = user_data;

    LOG_DBG("%u", evt);

    if (cur_srv->cb != NULL && cur_srv->cb->state_change != NULL)
        cur_srv->cb->state_change(cur_srv->cb->user_data, evt, state);
}

static struct bt_mesh_srv_callbacks bt_mesh_light_hsl_cb = {
    .get = bt_mesh_light_hsl_cb_get,
    .set = bt_mesh_light_hsl_cb_set,
    .state_change = bt_mesh_light_hsl_cb_state_change
};

static int bt_mesh_light_hsl_srv_init(const struct bt_mesh_model *model)
{
    struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;

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
    model->pub->update = light_hsl_pub_update;

    bt_mesh_light_hsl_cb.user_data = srv;
    srv->hue.cb = &bt_mesh_light_hsl_cb;
    srv->saturation.cb = &bt_mesh_light_hsl_cb;

    k_work_init_delayable(&srv->transition.timer, light_hsl_work_handler);

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_light_hsl_srv_cb = {
    .init = bt_mesh_light_hsl_srv_init,
};

static int bt_mesh_light_hsl_setup_srv_init(const struct bt_mesh_model *model)
{
    struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;

    LOG_INF("");

    if (!srv) {
        LOG_ERR("No Server context provided");
        return -EINVAL;
    }

    srv->setup_model = model;

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_light_hsl_setup_srv_cb = {
    .init = bt_mesh_light_hsl_setup_srv_init,
};

static int light_xyl_target_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_xyl_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_LIGHT_XYL_TARGET_STATUS, 7);
    bt_mesh_model_msg_init(&msg, OP_LIGHT_XYL_TARGET_STATUS);
    net_buf_simple_add_le16(&msg, srv->lightness->state.target_actual);
    net_buf_simple_add_le16(&msg, srv->state.target_x);
    net_buf_simple_add_le16(&msg, srv->state.target_y);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_u8(&msg, srv->transition.remain_time);
    }

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int light_xyl_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    struct bt_mesh_light_xyl_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_LIGHT_XYL_STATUS, 7);
    bt_mesh_model_msg_init(&msg, OP_LIGHT_XYL_STATUS);
    net_buf_simple_add_le16(&msg, srv->lightness->state.actual);
    net_buf_simple_add_le16(&msg, srv->state.x);
    net_buf_simple_add_le16(&msg, srv->state.y);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_u8(&msg, srv->transition.remain_time);
    }

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int light_xyl_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    struct bt_mesh_light_xyl_srv *srv = model->rt->user_data;

    if (srv->lightness->cb != NULL && srv->lightness->cb->get != NULL)
        srv->lightness->cb->get(srv->lightness->cb->user_data, BT_MESH_SRV_LIGHT_LIGHTNESS_EVT, &srv->lightness->state);

    if (srv->cb != NULL && srv->cb->get != NULL)
        srv->cb->get(srv->cb->user_data, BT_MESH_SRV_LIGHT_XYL_EVT, &srv->state);

    return light_hsl_status_send(model, ctx);
}

static int light_xyl_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint8_t tid;
    uint16_t lightness, x, y;
    struct bt_mesh_light_xyl_srv *srv = model->rt->user_data;

    if ((buf->len != 7U) && (buf->len != 9U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    lightness = net_buf_simple_pull_le16(buf);
    x = net_buf_simple_pull_le16(buf);
    y = net_buf_simple_pull_le16(buf);

    tid = net_buf_simple_pull_u8(buf);
    if (bt_mesh_tid_check_and_update(&srv->pre_tid, tid, ctx->addr, ctx->recv_dst))
        return 0;

    light_lightness_set_ext(srv->lightness->model, buf, lightness);

    bt_mesh_server_stop_transition(&srv->transition);

    if (x < srv->state.x_range_min) {
        x = srv->state.x_range_min;
    } else if (x > srv->state.x_range_max) {
        x = srv->state.x_range_max;
    }

    if (y < srv->state.y_range_min) {
        y = srv->state.y_range_min;
    } else if (y > srv->state.y_range_max) {
        y = srv->state.y_range_max;
    }

    srv->state.target_x = x;
    srv->state.target_y = y;

    if (srv->cb != NULL && srv->cb->set != NULL)
        srv->cb->set(srv->cb->user_data, BT_MESH_SRV_LIGHT_XYL_EVT, srv);

    if (srv->state.target_x == srv->state.x && srv->state.target_y == srv->state.y)
        return 0;

    bt_mesh_srv_transition_get(model, &srv->transition, buf);
    LOG_DBG("lightness:%u transition_time:%u delay:%u", lightness, srv->transition.transition_time, srv->transition.delay);

    srv->state.last_x = srv->state.x;
    srv->state.last_y = srv->state.y;
    set_transition_values(&srv->transition);
    srv->state.delta_x = ((float)(srv->state.target_x - srv->state.x) / srv->transition.counter);
    srv->state.delta_y = ((float)(srv->state.target_y - srv->state.y) / srv->transition.counter);
    bt_mesh_server_start_transition(&srv->transition);

    return 0;
}

static int light_xyl_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    int ret = light_xyl_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return light_xyl_status_send(model, ctx);
}

static int light_xyl_range_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    uint8_t status)
{
    struct bt_mesh_light_xyl_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_LIGHT_XYL_DEFAULT_STATUS, 9);
    bt_mesh_model_msg_init(&msg, OP_LIGHT_XYL_DEFAULT_STATUS);
    net_buf_simple_add_u8(&msg, status);
    net_buf_simple_add_le16(&msg, srv->state.x_range_min);
    net_buf_simple_add_le16(&msg, srv->state.x_range_max);
    net_buf_simple_add_le16(&msg, srv->state.y_range_min);
    net_buf_simple_add_le16(&msg, srv->state.y_range_max);

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int light_xyl_range_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    return light_xyl_range_status_send(model, ctx, 0);
}

static int light_xyl_range_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint16_t x_range_min, x_range_max, y_range_min, y_range_max;
    struct bt_mesh_light_xyl_srv *srv = model->rt->user_data;

    x_range_min = net_buf_simple_pull_le16(buf);
    x_range_max = net_buf_simple_pull_le16(buf);
    y_range_min = net_buf_simple_pull_le16(buf);
    y_range_max = net_buf_simple_pull_le16(buf);

    LOG_DBG("%u %u", x_range_min, x_range_max, y_range_min, y_range_max);
    if (x_range_min > x_range_max || y_range_min > y_range_max)
        return -EINVAL;

    srv->state.x_range_min = x_range_min;
    srv->state.x_range_max = x_range_max;
    srv->state.y_range_min = y_range_min;
    srv->state.y_range_max = y_range_max;

    return 0;
}

static int light_xyl_range_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    int ret = light_xyl_range_set_unack(model, ctx, buf);
    if (ret < 0 || ret >= BT_MESH_STATUS_UNKNOWN)
        return ret;

    return light_hsl_range_status_send(model, ctx, ret);
}

static int light_xyl_default_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    struct bt_mesh_light_xyl_srv *srv = model->rt->user_data;

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_LIGHT_XYL_DEFAULT_STATUS, 6);
    bt_mesh_model_msg_init(&msg, OP_LIGHT_XYL_DEFAULT_STATUS);
    net_buf_simple_add_le16(&msg, srv->lightness->state.def);
    net_buf_simple_add_le16(&msg, srv->state.x_def);
    net_buf_simple_add_le16(&msg, srv->state.y_def);

    return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int light_xyl_default_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    return light_xyl_default_status_send(model, ctx);
}

static int light_xyl_default_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    uint16_t lightness, x, y;
    struct bt_mesh_light_xyl_srv *srv = model->rt->user_data;

    lightness = net_buf_simple_pull_le16(buf);
    x = net_buf_simple_pull_le16(buf);
    y = net_buf_simple_pull_le16(buf);

    srv->lightness->state.def = lightness;
    srv->state.x_def = x;
    srv->state.y_def = y;

    return 0;
}

static int light_xyl_default_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    int ret = light_xyl_default_set_unack(model, ctx, buf);
    if (ret != 0)
        return ret;

    return light_xyl_default_status_send(model, ctx);
}

const struct bt_mesh_model_op bt_mesh_light_xyl_srv_op[] = {
    { OP_LIGHT_XYL_GET,                         BT_MESH_LEN_EXACT(0),       light_xyl_get },
    { OP_LIGHT_XYL_SET,                         BT_MESH_LEN_MIN(7),         light_xyl_set },
    { OP_LIGHT_XYL_SET_UNACK,                   BT_MESH_LEN_MIN(7),         light_xyl_set_unack },
    { OP_LIGHT_XYL_TARGET_GET,                  BT_MESH_LEN_MIN(0),         light_xyl_target_get },
    { OP_LIGHT_XYL_RANGE_GET,                   BT_MESH_LEN_MIN(0),         light_xyl_range_get },
    { OP_LIGHT_XYL_DEFAULT_GET,                 BT_MESH_LEN_MIN(0),         light_xyl_default_get },
    BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op bt_mesh_light_xyl_setup_srv_op[] = {
    { OP_LIGHT_XYL_RANGE_SET,                   BT_MESH_LEN_EXACT(8),         light_xyl_default_set },
    { OP_LIGHT_XYL_RANGE_SET_UNACK,             BT_MESH_LEN_EXACT(8),         light_xyl_default_set_unack },
    { OP_LIGHT_XYL_DEFAULT_SET,                 BT_MESH_LEN_EXACT(6),         light_xyl_range_set },
    { OP_LIGHT_XYL_DEFAULT_SET_UNACK,           BT_MESH_LEN_EXACT(6),         light_xyl_range_set_unack },
    BT_MESH_MODEL_OP_END,
};

static int light_xyl_pub_update(const struct bt_mesh_model *model)
{
    struct bt_mesh_light_xyl_srv *srv = model->rt->user_data;

    bt_mesh_model_msg_init(model->pub->msg, OP_LIGHT_XYL_STATUS);
    net_buf_simple_add_le16(model->pub->msg, srv->lightness->state.actual);
    net_buf_simple_add_le16(model->pub->msg, srv->state.x);
    net_buf_simple_add_le16(model->pub->msg, srv->state.y);
    if (srv->transition.counter != 0) {
        calculate_rt(&srv->transition);
        net_buf_simple_add_u8(model->pub->msg, srv->transition.remain_time);
    }

    return 0;
}

void light_xyl_config(struct bt_mesh_light_xyl_srv *srv, uint16_t x, uint16_t y)
{
    LOG_INF("%x %x", x, y);
    srv->state.x = x;
    srv->state.y = y;
}

void light_xyl_status_publish(const struct bt_mesh_model *model)
{
    LOG_INF("");
    light_xyl_pub_update(model);
    bt_mesh_model_publish(model);
}

static void light_xyl_work_handler(struct k_work *work)
{
    struct bt_mesh_light_xyl_srv *srv = CONTAINER_OF(work, struct bt_mesh_light_xyl_srv, transition.timer.work);

    LOG_DBG("just_started:%u counter:%u, quo_tt:%u  %x %x", srv->transition.just_started, srv->transition.counter,
        srv->transition.quo_tt, srv->state.delta_x, srv->state.delta_y);

    if (srv->transition.just_started) {
        srv->transition.just_started = false;

        if (srv->transition.counter == 0U) {
            light_xyl_config(srv, srv->state.target_x, srv->state.target_y);
            light_xyl_status_publish(srv->model);

            if (srv->cb != NULL && srv->cb->state_change != NULL)
                srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_LIGHT_XYL_EVT, &srv->state);
        } else {
            srv->transition.start_timestamp = k_uptime_get();
            k_work_reschedule(&srv->transition.timer, K_MSEC(srv->transition.quo_tt));
        }

        return;
    }

    if (srv->transition.counter == 0 || (--srv->transition.counter) == 0) {
        light_xyl_config(srv, srv->state.target_x, srv->state.target_y);
    } else {
        light_xyl_config(srv, srv->state.x + srv->state.delta_x, srv->state.y + srv->state.delta_y);
        k_work_reschedule(&srv->transition.timer, K_MSEC(srv->transition.quo_tt));
    }

    light_xyl_status_publish(srv->model);
    if (srv->cb != NULL && srv->cb->state_change != NULL)
        srv->cb->state_change(srv->cb->user_data, BT_MESH_SRV_LIGHT_XYL_EVT, &srv->state);
}

static int bt_mesh_light_xyl_srv_init(const struct bt_mesh_model *model)
{
    struct bt_mesh_light_xyl_srv *srv = model->rt->user_data;

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
    model->pub->update = light_xyl_pub_update;

    k_work_init_delayable(&srv->transition.timer, light_xyl_work_handler);

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_light_xyl_srv_cb = {
    .init = bt_mesh_light_xyl_srv_init,
};

static int bt_mesh_light_xyl_setup_srv_init(const struct bt_mesh_model *model)
{
    struct bt_mesh_light_xyl_srv *srv = model->rt->user_data;

    LOG_INF("");

    if (!srv) {
        LOG_ERR("No Server context provided");
        return -EINVAL;
    }

    srv->setup_model = model;

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_light_xyl_setup_srv_cb = {
    .init = bt_mesh_light_xyl_setup_srv_init,
};

