/*!
    \file    lighting_client.c
    \brief   Implementation of BLE mesh lighting client.

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

#include "src/msg.h"
#include "models.h"
#include "model_utils.h"
#include "lighting_client.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include "api/mesh_log.h"

static int bt_mesh_light_lightness_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_lightness_status status = {0};
    struct bt_mesh_light_lightness_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    if ((buf->len != 2U) && (buf->len != 5U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    status.present_actual = net_buf_simple_pull_le16(buf);
    if (buf->len == 3) {
        status.op_en = true;
        status.target_actual = net_buf_simple_pull_le16(buf);
        status.remain_time = net_buf_simple_pull_u8(buf);
    }

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_LIGHT_LIGHTNESS_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_LIGHT_LIGHTNESS_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_light_lightness_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_lightness_set *req,
    struct bt_mesh_light_lightness_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_LIGHT_LIGHTNESS_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 5);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_LIGHT_LIGHTNESS_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_LIGHTNESS_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_LIGHTNESS_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_le16(&msg, req->actual);
        net_buf_simple_add_u8(&msg, req->tid);
        if (req->op_en) {
            net_buf_simple_add_u8(&msg, req->transition_time);
            net_buf_simple_add_u8(&msg, req->delay);
        }
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_light_lightness_linear_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_lightness_status status = {0};
    struct bt_mesh_light_lightness_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    if ((buf->len != 2U) && (buf->len != 5U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    status.present_actual = light_linear_to_actual(net_buf_simple_pull_le16(buf));
    if (buf->len == 3) {
        status.op_en = true;
        status.target_actual = light_linear_to_actual(net_buf_simple_pull_le16(buf));
        status.remain_time = net_buf_simple_pull_u8(buf);
    }

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_LIGHT_LIGHTNESS_LINEAR_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_LIGHT_LIGHTNESS_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_light_lightness_linear_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_lightness_linear_set *req,
    struct bt_mesh_light_lightness_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_LIGHT_LIGHTNESS_LINEAR_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 5);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_LIGHT_LIGHTNESS_LINEAR_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_LIGHTNESS_LINEAR_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_LIGHTNESS_LINEAR_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_le16(&msg, req->linear);
        net_buf_simple_add_u8(&msg, req->tid);
        if (req->op_en) {
            net_buf_simple_add_u8(&msg, req->transition_time);
            net_buf_simple_add_u8(&msg, req->delay);
        }
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_light_lightness_last_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_lightness_last_status status = {0};
    struct bt_mesh_light_lightness_last_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    status.last_actual = net_buf_simple_pull_le16(buf);

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_LIGHT_LIGHTNESS_LAST_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_LIGHT_LIGHTNESS_LAST_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_light_lightness_last_cli_get(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    struct bt_mesh_light_lightness_last_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_LIGHT_LIGHTNESS_LAST_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_LIGHT_LIGHTNESS_LAST_GET, 0);
    bt_mesh_model_msg_init(&msg, OP_LIGHT_LIGHTNESS_LAST_GET);

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_light_lightness_default_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_lightness_default status = {0};
    struct bt_mesh_light_lightness_default *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    status.lightness = net_buf_simple_pull_le16(buf);

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_LIGHT_LIGHTNESS_DEFAULT_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_LIGHT_LIGHTNESS_DEFAULT_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_light_lightness_default_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_lightness_default *req,
    struct bt_mesh_light_lightness_default *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_LIGHT_LIGHTNESS_DEFAULT_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 2);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_LIGHT_LIGHTNESS_DEFAULT_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_LIGHTNESS_DEFAULT_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_LIGHTNESS_DEFAULT_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_le16(&msg, req->lightness);
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_light_lightness_range_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_lightness_range_status status = {0};
    struct bt_mesh_light_lightness_range_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    status.status_code = net_buf_simple_pull_u8(buf);
    status.range_min = net_buf_simple_pull_le16(buf);
    status.range_max = net_buf_simple_pull_le16(buf);

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_LIGHT_LIGHTNESS_RANGE_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_LIGHT_LIGHTNESS_RANGE_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_light_lightness_range_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_lightness_range_set *req,
    struct bt_mesh_light_lightness_range_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_LIGHT_LIGHTNESS_RANGE_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 4);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_LIGHT_LIGHTNESS_RANGE_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_LIGHTNESS_RANGE_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_LIGHTNESS_RANGE_SET_UNACK);
            rsp = NULL;
        }

        if (req->range_min == 0 || req->range_max == 0 || req->range_min > req->range_max) {
            return -EINVAL;
        }

        net_buf_simple_add_le16(&msg, req->range_min);
        net_buf_simple_add_le16(&msg, req->range_max);
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_light_ctl_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_ctl_status status = {0};
    struct bt_mesh_light_ctl_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    if ((buf->len != 4U) && (buf->len != 9U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    status.present_lightness = net_buf_simple_pull_le16(buf);
    status.present_temp = net_buf_simple_pull_le16(buf);
    if (buf->len == 5) {
        status.op_en = true;
        status.target_lightness = net_buf_simple_pull_le16(buf);
        status.target_temp = net_buf_simple_pull_le16(buf);
        status.remain_time = net_buf_simple_pull_u8(buf);
    }

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_LIGHT_CTL_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_LIGHT_CTL_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_light_ctl_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_ctl_set *req, struct bt_mesh_light_ctl_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_LIGHT_CTL_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 9);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_LIGHT_CTL_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_CTL_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_CTL_SET_UNACK);
            rsp = NULL;
        }

        if (req->temp < BT_MESH_TEMPERATURE_MIN || req->temp > BT_MESH_TEMPERATURE_MAX) {
            return -EINVAL;
        }

        net_buf_simple_add_le16(&msg, req->lightness);
        net_buf_simple_add_le16(&msg, req->temp);
        net_buf_simple_add_le16(&msg, req->delta_uv);
        net_buf_simple_add_u8(&msg, req->tid);
        if (req->op_en) {
            net_buf_simple_add_u8(&msg, req->transition_time);
            net_buf_simple_add_u8(&msg, req->delay);
        }
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_light_ctl_temp_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_ctl_temp_status status = {0};
    struct bt_mesh_light_ctl_temp_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    if ((buf->len != 4U) && (buf->len != 9U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    status.present_temp = net_buf_simple_pull_le16(buf);
    status.present_delta_uv = (int16_t)net_buf_simple_pull_le16(buf);
    if (buf->len == 5) {
        status.op_en = true;
        status.target_temp = net_buf_simple_pull_le16(buf);
        status.target_delta_uv = (int16_t)net_buf_simple_pull_le16(buf);
        status.remain_time = net_buf_simple_pull_u8(buf);
    }

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_LIGHT_CTL_TEMPERATURE_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_LIGHT_CTL_TEMPERATURE_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_light_ctl_temp_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_ctl_temp_set *req,
    struct bt_mesh_light_ctl_temp_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_LIGHT_CTL_TEMPERATURE_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 7);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_LIGHT_CTL_TEMPERATURE_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_CTL_TEMPERATURE_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_CTL_TEMPERATURE_SET_UNACK);
            rsp = NULL;
        }

        if (req->temp < BT_MESH_TEMPERATURE_MIN || req->temp > BT_MESH_TEMPERATURE_MAX) {
            return -EINVAL;
        }

        net_buf_simple_add_le16(&msg, req->temp);
        net_buf_simple_add_le16(&msg, req->delta_uv);
        net_buf_simple_add_u8(&msg, req->tid);
        if (req->op_en) {
            net_buf_simple_add_u8(&msg, req->transition_time);
            net_buf_simple_add_u8(&msg, req->delay);
        }
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_light_ctl_temp_range_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_ctl_temp_range_status status = {0};
    struct bt_mesh_light_ctl_temp_range_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    status.status_code = net_buf_simple_pull_u8(buf);
    status.range_min = net_buf_simple_pull_le16(buf);
    status.range_max = net_buf_simple_pull_le16(buf);

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_LIGHT_CTL_TEMPERATURE_RANGE_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_LIGHT_CTL_TEMPERATURE_RANGE_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_light_ctl_temp_range_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_ctl_temp_range_set *req,
    struct bt_mesh_light_ctl_temp_range_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_LIGHT_CTL_TEMPERATURE_RANGE_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 4);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_LIGHT_CTL_TEMPERATURE_RANGE_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_CTL_TEMPERATURE_RANGE_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_CTL_TEMPERATURE_RANGE_SET_UNACK);
            rsp = NULL;
        }

        if (req->range_min < BT_MESH_TEMPERATURE_MIN || req->range_min > BT_MESH_TEMPERATURE_MAX
            || req->range_max < BT_MESH_TEMPERATURE_MIN || req->range_max > BT_MESH_TEMPERATURE_MAX
            || req->range_min > req->range_max) {
            return -EINVAL;
        }

        net_buf_simple_add_le16(&msg, req->range_min);
        net_buf_simple_add_le16(&msg, req->range_max);
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_light_ctl_default_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_ctl_default status = {0};
    struct bt_mesh_light_ctl_default *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    status.lightness = net_buf_simple_pull_le16(buf);
    status.temp = net_buf_simple_pull_le16(buf);
    status.delta_uv = (int16_t)net_buf_simple_pull_le16(buf);

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_LIGHT_CTL_DEFAULT_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_LIGHT_CTL_DEFAULT_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_light_ctl_default_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_ctl_default *req,
    struct bt_mesh_light_ctl_default *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_LIGHT_CTL_DEFAULT_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 6);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_LIGHT_CTL_DEFAULT_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_CTL_DEFAULT_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_CTL_DEFAULT_SET_UNACK);
            rsp = NULL;
        }

        if (req->temp < BT_MESH_TEMPERATURE_MIN || req->temp > BT_MESH_TEMPERATURE_MAX) {
            return -EINVAL;
        }

        net_buf_simple_add_le16(&msg, req->lightness);
        net_buf_simple_add_le16(&msg, req->temp);
        net_buf_simple_add_le16(&msg, req->delta_uv);
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_light_hsl_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_hsl_status status = {0};
    struct bt_mesh_light_hsl_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    if ((buf->len != 6U) && (buf->len != 7U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    status.lightness = net_buf_simple_pull_le16(buf);
    status.hue = net_buf_simple_pull_le16(buf);
    status.sat = net_buf_simple_pull_le16(buf);
    if (buf->len == 1) {
        status.op_en = true;
        status.remain_time = net_buf_simple_pull_u8(buf);
    }

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_LIGHT_HSL_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_LIGHT_HSL_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_light_hsl_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_hsl_set *req, struct bt_mesh_light_hsl_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_LIGHT_HSL_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 9);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_le16(&msg, req->lightness);
        net_buf_simple_add_le16(&msg, req->hue);
        net_buf_simple_add_le16(&msg, req->sat);
        net_buf_simple_add_u8(&msg, req->tid);
        if (req->op_en) {
            net_buf_simple_add_u8(&msg, req->transition_time);
            net_buf_simple_add_u8(&msg, req->delay);
        }
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_light_hsl_target_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_hsl_status status = {0};
    struct bt_mesh_light_hsl_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    if ((buf->len != 6U) && (buf->len != 7U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    status.lightness = net_buf_simple_pull_le16(buf);
    status.hue = net_buf_simple_pull_le16(buf);
    status.sat = net_buf_simple_pull_le16(buf);
    if (buf->len == 1) {
        status.op_en = true;
        status.remain_time = net_buf_simple_pull_u8(buf);
    }

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_LIGHT_HSL_TARGET_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_LIGHT_HSL_TARGET_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_light_hsl_target_cli_get(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    struct bt_mesh_light_hsl_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_LIGHT_HSL_TARGET_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_LIGHT_HSL_TARGET_GET, 0);
    bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_TARGET_GET);

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_light_hsl_range_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_hsl_range_status status = {0};
    struct bt_mesh_light_hsl_range_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    status.status_code = net_buf_simple_pull_u8(buf);
    status.hue_range_min = net_buf_simple_pull_le16(buf);
    status.hue_range_max = net_buf_simple_pull_le16(buf);
    status.sat_range_min = net_buf_simple_pull_le16(buf);
    status.sat_range_max = net_buf_simple_pull_le16(buf);

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_LIGHT_HSL_RANGE_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_LIGHT_HSL_RANGE_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_light_hsl_range_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_hsl_range_set *req,
    struct bt_mesh_light_hsl_range_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_LIGHT_HSL_RANGE_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 8);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_RANGE_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_RANGE_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_RANGE_SET_UNACK);
            rsp = NULL;
        }

        if (req->sat_range_min > req->sat_range_max) {
            return -EINVAL;
        }

        net_buf_simple_add_le16(&msg, req->hue_range_min);
        net_buf_simple_add_le16(&msg, req->hue_range_max);
        net_buf_simple_add_le16(&msg, req->sat_range_min);
        net_buf_simple_add_le16(&msg, req->sat_range_max);
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_light_hsl_default_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_hsl_default status = {0};
    struct bt_mesh_light_hsl_default *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    status.lightness = net_buf_simple_pull_le16(buf);
    status.hue = net_buf_simple_pull_le16(buf);
    status.sat = net_buf_simple_pull_le16(buf);

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_LIGHT_HSL_DEFAULT_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_LIGHT_HSL_DEFAULT_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_light_hsl_default_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_hsl_default *req,
    struct bt_mesh_light_hsl_default *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_LIGHT_HSL_DEFAULT_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 6);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_DEFAULT_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_DEFAULT_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_DEFAULT_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_le16(&msg, req->lightness);
        net_buf_simple_add_le16(&msg, req->hue);
        net_buf_simple_add_le16(&msg, req->sat);
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_light_hsl_hue_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_hsl_hue_status status = {0};
    struct bt_mesh_light_hsl_hue_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    if ((buf->len != 2U) && (buf->len != 5U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    status.hue = net_buf_simple_pull_le16(buf);
    if (buf->len == 3) {
        status.op_en = true;
        status.target_hue = net_buf_simple_pull_le16(buf);
        status.remain_time = net_buf_simple_pull_u8(buf);
    }

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_LIGHT_HSL_HUE_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_LIGHT_HSL_HUE_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_light_hsl_hue_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_hsl_hue_set *req,
    struct bt_mesh_light_hsl_hue_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_LIGHT_HSL_HUE_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 5);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_HUE_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_HUE_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_HUE_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_le16(&msg, req->hue);
        net_buf_simple_add_u8(&msg, req->tid);
        if (req->op_en) {
            net_buf_simple_add_u8(&msg, req->transition_time);
            net_buf_simple_add_u8(&msg, req->delay);
        }
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}


static int bt_mesh_light_hsl_sat_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_hsl_sat_status status = {0};
    struct bt_mesh_light_hsl_sat_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    if ((buf->len != 2U) && (buf->len != 5U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    status.sat = net_buf_simple_pull_le16(buf);
    if (buf->len == 3) {
        status.op_en = true;
        status.target_sat = net_buf_simple_pull_le16(buf);
        status.remain_time = net_buf_simple_pull_u8(buf);
    }

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_LIGHT_HSL_SATURATION_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_LIGHT_HSL_SAT_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_light_hsl_sat_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_hsl_sat_set *req,
    struct bt_mesh_light_hsl_sat_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_LIGHT_HSL_SATURATION_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 5);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_SATURATION_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_SATURATION_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_HSL_SATURATION_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_le16(&msg, req->sat);
        net_buf_simple_add_u8(&msg, req->tid);
        if (req->op_en) {
            net_buf_simple_add_u8(&msg, req->transition_time);
            net_buf_simple_add_u8(&msg, req->delay);
        }
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_light_xyl_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_xyl_status status = {0};
    struct bt_mesh_light_xyl_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    if ((buf->len != 6U) && (buf->len != 7U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    status.lightness = net_buf_simple_pull_le16(buf);
    status.x = net_buf_simple_pull_le16(buf);
    status.y = net_buf_simple_pull_le16(buf);
    if (buf->len == 1) {
        status.op_en = true;
        status.remain_time = net_buf_simple_pull_u8(buf);
    }

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_LIGHT_XYL_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_LIGHT_HSL_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_light_xyl_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_xyl_set *req, struct bt_mesh_light_xyl_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_LIGHT_XYL_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 9);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_LIGHT_XYL_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_XYL_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_XYL_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_le16(&msg, req->lightness);
        net_buf_simple_add_le16(&msg, req->x);
        net_buf_simple_add_le16(&msg, req->y);
        net_buf_simple_add_u8(&msg, req->tid);
        if (req->op_en) {
            net_buf_simple_add_u8(&msg, req->transition_time);
            net_buf_simple_add_u8(&msg, req->delay);
        }
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_light_xyl_target_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_xyl_status status = {0};
    struct bt_mesh_light_xyl_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    if ((buf->len != 6U) && (buf->len != 7U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    status.lightness = net_buf_simple_pull_le16(buf);
    status.x = net_buf_simple_pull_le16(buf);
    status.y = net_buf_simple_pull_le16(buf);
    if (buf->len == 1) {
        status.op_en = true;
        status.remain_time = net_buf_simple_pull_u8(buf);
    }

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_LIGHT_XYL_TARGET_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_LIGHT_XYL_TARGET_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_light_xyl_target_cli_get(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    struct bt_mesh_light_xyl_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_LIGHT_XYL_TARGET_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_LIGHT_XYL_TARGET_GET, 0);
    bt_mesh_model_msg_init(&msg, OP_LIGHT_XYL_TARGET_GET);

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_light_xyl_range_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_xyl_range_status status = {0};
    struct bt_mesh_light_xyl_range_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    status.status_code = net_buf_simple_pull_u8(buf);
    status.x_range_min = net_buf_simple_pull_le16(buf);
    status.x_range_max = net_buf_simple_pull_le16(buf);
    status.y_range_min = net_buf_simple_pull_le16(buf);
    status.y_range_max = net_buf_simple_pull_le16(buf);

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_LIGHT_XYL_RANGE_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_LIGHT_XYL_RANGE_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_light_xyl_range_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_xyl_range_set *req,
    struct bt_mesh_light_xyl_range_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_LIGHT_XYL_RANGE_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 8);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_LIGHT_XYL_RANGE_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_XYL_RANGE_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_XYL_RANGE_SET_UNACK);
            rsp = NULL;
        }

        if (req->x_range_min > req->x_range_max || req->y_range_min > req->y_range_max) {
            return -EINVAL;
        }

        net_buf_simple_add_le16(&msg, req->x_range_min);
        net_buf_simple_add_le16(&msg, req->x_range_max);
        net_buf_simple_add_le16(&msg, req->y_range_min);
        net_buf_simple_add_le16(&msg, req->y_range_max);
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_light_xyl_default_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_light_xyl_default status = {0};
    struct bt_mesh_light_xyl_default *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    status.lightness = net_buf_simple_pull_le16(buf);
    status.x = net_buf_simple_pull_le16(buf);
    status.y = net_buf_simple_pull_le16(buf);

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_LIGHT_XYL_DEFAULT_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_LIGHT_XYL_DEFAULT_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_light_xyl_default_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_xyl_default *req,
    struct bt_mesh_light_xyl_default *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_LIGHT_XYL_DEFAULT_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 6);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_LIGHT_XYL_DEFAULT_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_XYL_DEFAULT_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_LIGHT_XYL_DEFAULT_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_le16(&msg, req->lightness);
        net_buf_simple_add_le16(&msg, req->x);
        net_buf_simple_add_le16(&msg, req->y);
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_light_cli_init(const struct bt_mesh_model *model)
{
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;
    if (!cli) {
        LOG_ERR("No Client context provided");
        return -EINVAL;
    }

    if (!model->pub) {
        LOG_ERR("No publication support");
        return -EINVAL;
    }

    cli->model = model;
    cli->msg_timeout = CONFIG_BT_MESH_LIGHT_CLI_TIMEOUT;
    bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_light_cli_cb = {
    .init = bt_mesh_light_cli_init,
};

const struct bt_mesh_model_op bt_mesh_light_lightness_cli_op[] = {
    { OP_LIGHT_LIGHTNESS_STATUS,                BT_MESH_LEN_MIN(2),     bt_mesh_light_lightness_cli_status },
    { OP_LIGHT_LIGHTNESS_LINEAR_STATUS,         BT_MESH_LEN_MIN(2),     bt_mesh_light_lightness_linear_cli_status },
    { OP_LIGHT_LIGHTNESS_LAST_STATUS,           BT_MESH_LEN_EXACT(2),   bt_mesh_light_lightness_last_cli_status },
    { OP_LIGHT_LIGHTNESS_DEFAULT_STATUS,        BT_MESH_LEN_EXACT(2),   bt_mesh_light_lightness_default_cli_status },
    { OP_LIGHT_LIGHTNESS_RANGE_STATUS,          BT_MESH_LEN_EXACT(5),   bt_mesh_light_lightness_range_cli_status },
    BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op bt_mesh_light_ctl_cli_op[] = {
    { OP_LIGHT_CTL_STATUS,                      BT_MESH_LEN_MIN(4),     bt_mesh_light_ctl_cli_status },
    { OP_LIGHT_CTL_TEMPERATURE_STATUS,          BT_MESH_LEN_MIN(4),     bt_mesh_light_ctl_temp_cli_status },
    { OP_LIGHT_CTL_TEMPERATURE_RANGE_STATUS,    BT_MESH_LEN_EXACT(5),   bt_mesh_light_ctl_temp_range_cli_status },
    { OP_LIGHT_CTL_DEFAULT_STATUS,              BT_MESH_LEN_EXACT(6),   bt_mesh_light_ctl_default_cli_status },
    BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op bt_mesh_light_hsl_cli_op[] = {
    { OP_LIGHT_HSL_STATUS,                      BT_MESH_LEN_MIN(6),     bt_mesh_light_hsl_cli_status },
    { OP_LIGHT_HSL_TARGET_STATUS,               BT_MESH_LEN_MIN(6),     bt_mesh_light_hsl_target_cli_status },
    { OP_LIGHT_HSL_RANGE_STATUS,                BT_MESH_LEN_EXACT(9),   bt_mesh_light_hsl_range_cli_status },
    { OP_LIGHT_HSL_DEFAULT_STATUS,              BT_MESH_LEN_EXACT(6),   bt_mesh_light_hsl_default_cli_status },
    { OP_LIGHT_HSL_HUE_STATUS,                  BT_MESH_LEN_MIN(2),     bt_mesh_light_hsl_hue_cli_status },
    { OP_LIGHT_HSL_SATURATION_STATUS,           BT_MESH_LEN_MIN(2),     bt_mesh_light_hsl_sat_cli_status },
    BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op bt_mesh_light_xyl_cli_op[] = {
    { OP_LIGHT_XYL_STATUS,                      BT_MESH_LEN_MIN(6),     bt_mesh_light_xyl_cli_status },
    { OP_LIGHT_XYL_TARGET_STATUS,               BT_MESH_LEN_MIN(6),     bt_mesh_light_xyl_target_cli_status },
    { OP_LIGHT_XYL_RANGE_STATUS,                BT_MESH_LEN_EXACT(9),   bt_mesh_light_xyl_range_cli_status },
    { OP_LIGHT_XYL_DEFAULT_STATUS,              BT_MESH_LEN_EXACT(6),   bt_mesh_light_xyl_default_cli_status },
};
