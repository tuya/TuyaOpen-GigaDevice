/*!
    \file    generic_client.c
    \brief   Implementation of BLE mesh generic client.

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
#include "generic_client.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include "api/mesh_log.h"

static int bt_mesh_gen_onoff_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_onoff_status status = {0};
    struct bt_mesh_gen_onoff_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    if ((buf->len != 1U) && (buf->len != 3U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    status.present_onoff = net_buf_simple_pull_u8(buf);
    if (buf->len == 2) {
        status.op_en = true;
        status.target_onoff = net_buf_simple_pull_u8(buf);
        status.remain_time = net_buf_simple_pull_u8(buf);
    }

    if ((status.present_onoff > 1) || (status.target_onoff > 1)) {
        return -EINVAL;
    }

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_GEN_ONOFF_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_GEN_ONOFF_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_gen_onoff_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_onoff_set *req, struct bt_mesh_gen_onoff_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_GEN_ONOFF_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 4);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_GEN_ONOFF_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_GEN_ONOFF_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_GEN_ONOFF_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_u8(&msg, req->onoff);
        net_buf_simple_add_u8(&msg, req->tid);
        if (req->op_en) {
            net_buf_simple_add_u8(&msg, req->transition_time);
            net_buf_simple_add_u8(&msg, req->delay);
        }
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_gen_level_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_level_status status = {0};
    struct bt_mesh_gen_level_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    if ((buf->len != 2U) && (buf->len != 5U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    status.present_level = net_buf_simple_pull_le16(buf);
    if (buf->len == 3) {
        status.op_en = true;
        status.target_level = net_buf_simple_pull_le16(buf);
        status.remain_time = net_buf_simple_pull_u8(buf);
    }

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_GEN_LEVEL_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_GEN_LEVEL_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_gen_level_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_level_set *req, struct bt_mesh_gen_level_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_GEN_LEVEL_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 5);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_GEN_LEVEL_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_GEN_LEVEL_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_GEN_LEVEL_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_le16(&msg, req->level);
        net_buf_simple_add_u8(&msg, req->tid);
        if (req->op_en) {
            net_buf_simple_add_u8(&msg, req->transition_time);
            net_buf_simple_add_u8(&msg, req->delay);
        }
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_gen_delta_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_delta_set *req, struct bt_mesh_gen_level_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_GEN_LEVEL_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 7);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_SET) {
        bt_mesh_model_msg_init(&msg, OP_GEN_LEVEL_DELTA_SET);
    } else {
        bt_mesh_model_msg_init(&msg, OP_GEN_LEVEL_DELTA_SET_UNACK);
        rsp = NULL;
    }

    net_buf_simple_add_le32(&msg, req->delta_level);
    net_buf_simple_add_u8(&msg, req->tid);
    if (req->op_en) {
        net_buf_simple_add_u8(&msg, req->transition_time);
        net_buf_simple_add_u8(&msg, req->delay);
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_gen_move_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_move_set *req, struct bt_mesh_gen_level_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_GEN_LEVEL_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 5);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_SET) {
        bt_mesh_model_msg_init(&msg, OP_GEN_LEVEL_MOVE_SET);
    } else {
        bt_mesh_model_msg_init(&msg, OP_GEN_LEVEL_MOVE_SET_UNACK);
        rsp = NULL;
    }

    net_buf_simple_add_le16(&msg, req->delta_level);
    net_buf_simple_add_u8(&msg, req->tid);
    if (req->op_en) {
        net_buf_simple_add_u8(&msg, req->transition_time);
        net_buf_simple_add_u8(&msg, req->delay);
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_gen_def_trans_time_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_def_trans_time_status status = {0};
    struct bt_mesh_gen_def_trans_time_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    status.transition_time = net_buf_simple_pull_u8(buf);

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_GEN_DEF_TRANS_TIME_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_GEN_DEF_TRANS_TIME_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_gen_def_trans_time_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_def_trans_time_set *req, struct bt_mesh_gen_def_trans_time_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_GEN_DEF_TRANS_TIME_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 1);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_GEN_DEF_TRANS_TIME_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_GEN_DEF_TRANS_TIME_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_GEN_DEF_TRANS_TIME_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_u8(&msg, req->transition_time);
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_gen_onpowerup_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_onpowerup_status status = {0};
    struct bt_mesh_gen_onpowerup_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    status.onpowerup = net_buf_simple_pull_u8(buf);

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_GEN_ONPOWERUP_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_GEN_POWER_ONOFF_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_gen_onpowerup_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_onpowerup_set *req, struct bt_mesh_gen_onpowerup_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_GEN_ONPOWERUP_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 1);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_GEN_ONPOWERUP_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_GEN_ONPOWERUP_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_GEN_ONPOWERUP_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_u8(&msg, req->onpowerup);
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_gen_power_level_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_power_level_status status = {0};
    struct bt_mesh_gen_power_level_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    if ((buf->len != 2U) && (buf->len != 5U)) {
        LOG_ERR("The message size for the application opcode is incorrect.");
        return -EMSGSIZE;
    }

    status.present_power = net_buf_simple_pull_le16(buf);
    if (buf->len == 3) {
        status.op_en = true;
        status.target_power = net_buf_simple_pull_le16(buf);
        status.remain_time = net_buf_simple_pull_u8(buf);
    }

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_GEN_POWER_LEVEL_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_GEN_POWER_LEVEL_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_gen_power_level_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_power_level_set *req, struct bt_mesh_gen_power_level_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_GEN_POWER_LEVEL_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 5);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_GEN_POWER_LEVEL_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_GEN_POWER_LEVEL_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_GEN_POWER_LEVEL_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_le16(&msg, req->power);
        net_buf_simple_add_u8(&msg, req->tid);
        if (req->op_en) {
            net_buf_simple_add_u8(&msg, req->transition_time);
            net_buf_simple_add_u8(&msg, req->delay);
        }
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_gen_power_last_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_power_last_status status = {0};
    struct bt_mesh_gen_power_last_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    status.last_power = net_buf_simple_pull_le16(buf);

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_GEN_POWER_LAST_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_GEN_POWER_LAST_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_gen_power_last_cli_get(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    struct bt_mesh_gen_power_last_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_GEN_POWER_LAST_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_POWER_LAST_GET, 0);
    bt_mesh_model_msg_init(&msg, OP_GEN_POWER_LAST_GET);

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_gen_power_def_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_power_def_status status = {0};
    struct bt_mesh_gen_power_def_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    status.def_power = net_buf_simple_pull_le16(buf);

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_GEN_POWER_DEFAULT_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_GEN_POWER_DEF_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_gen_power_def_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_power_def_set *req, struct bt_mesh_gen_power_def_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_GEN_POWER_DEFAULT_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 2);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_GEN_POWER_DEFAULT_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_GEN_POWER_DEFAULT_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_GEN_POWER_DEFAULT_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_le16(&msg, req->def_power);
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_gen_power_range_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_power_range_status status = {0};
    struct bt_mesh_gen_power_range_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    status.status_code = net_buf_simple_pull_u8(buf);
    status.range_min = net_buf_simple_pull_le16(buf);
    status.range_max = net_buf_simple_pull_le16(buf);

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_GEN_POWER_RANGE_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_GEN_POWER_RANGE_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_gen_power_range_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_power_range_set *req, struct bt_mesh_gen_power_range_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_GEN_POWER_RANGE_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 4);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_GEN_POWER_RANGE_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_GEN_POWER_RANGE_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_GEN_POWER_RANGE_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_le16(&msg, req->range_min);
        net_buf_simple_add_le16(&msg, req->range_max);
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_gen_battery_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_battery_status status = {0};
    struct bt_mesh_gen_battery_status *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;
    uint32_t tmp;

    tmp = net_buf_simple_pull_le32(buf);
    status.battery_level = tmp & 0xFF;
    status.time_to_discharge = tmp >> 8;
    tmp = net_buf_simple_pull_le32(buf);
    status.time_to_charge = tmp & 0xFFFFFF;
    status.battery_flags = tmp >> 24;

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_GEN_BATTERY_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_GEN_BATTERY_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_gen_battery_cli_get(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    struct bt_mesh_gen_battery_status *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_GEN_BATTERY_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_BATTERY_GET, 0);
    bt_mesh_model_msg_init(&msg, OP_GEN_BATTERY_GET);

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_gen_location_global_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_location_global status = {0};
    struct bt_mesh_gen_location_global *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    status.global_latitude = net_buf_simple_pull_le32(buf);
    status.global_longitude = net_buf_simple_pull_le32(buf);
    status.global_altitude = net_buf_simple_pull_le16(buf);

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_GEN_LOCATION_GLOBAL_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_GEN_LOCATION_GLOBAL_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_gen_location_global_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_location_global *req, struct bt_mesh_gen_location_global *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_GEN_LOCATION_GLOBAL_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 10);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_GEN_LOCATION_GLOBAL_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_GEN_LOCATION_GLOBAL_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_GEN_LOCATION_GLOBAL_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_le32(&msg, req->global_latitude);
        net_buf_simple_add_le32(&msg, req->global_longitude);
        net_buf_simple_add_le16(&msg, req->global_altitude);
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_gen_location_local_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_location_local status = {0};
    struct bt_mesh_gen_location_local *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    status.local_north = net_buf_simple_pull_le16(buf);
    status.local_east = net_buf_simple_pull_le16(buf);
    status.local_altitude = net_buf_simple_pull_le16(buf);
    status.floor_number = net_buf_simple_pull_u8(buf);
    status.uncertainty = net_buf_simple_pull_le16(buf);

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, OP_GEN_LOCATION_LOCAL_STATUS, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            *rsp = status;
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, BT_MESH_CLI_GEN_LOCATION_LOCAL_EVT, ctx, &status);
    }

    return 0;
}

int bt_mesh_gen_location_local_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_location_local *req, struct bt_mesh_gen_location_local *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_GEN_LOCATION_LOCAL_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };
    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 9);

    BT_MESH_CLI_OPERATION_CHECK(operation, req);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_GEN_LOCATION_LOCAL_GET);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_GEN_LOCATION_LOCAL_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_GEN_LOCATION_LOCAL_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_le16(&msg, req->local_north);
        net_buf_simple_add_le16(&msg, req->local_east);
        net_buf_simple_add_le16(&msg, req->local_altitude);
        net_buf_simple_add_u8(&msg, req->floor_number);
        net_buf_simple_add_le16(&msg, req->uncertainty);
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

__INLINE enum bt_mesh_cli_callback_evt gen_property_op_to_evt(uint32_t op) {
    switch(op) {
    case OP_GEN_ADMIN_PROPS_STATUS:
        return BT_MESH_CLI_GEN_ADMIN_PROPERTIES_EVT;
    case OP_GEN_ADMIN_PROP_STATUS:
        return BT_MESH_CLI_GEN_ADMIN_PROPERTY_EVT;
    case OP_GEN_MFR_PROPS_STATUS:
        return BT_MESH_CLI_GEN_MFR_PROPERTIES_EVT;
    case OP_GEN_MFR_PROP_STATUS:
        return BT_MESH_CLI_GEN_MFR_PROPERTY_EVT;
    case OP_GEN_USER_PROPS_STATUS:
        return BT_MESH_CLI_GEN_USER_PROPERTIES_EVT;
    case OP_GEN_USER_PROP_STATUS:
        return BT_MESH_CLI_GEN_USER_PROPERTY_EVT;
    case OP_GEN_CLIENT_PROPS_STATUS:
        return BT_MESH_CLI_GEN_CLIENT_PROPERTIES_EVT;
    default:
        return BT_MESH_CLI_GEN_USER_PROPERTY_EVT;
    };
}

static int bt_mesh_gen_properties_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_properties status = {0};
    struct bt_mesh_gen_properties *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    status.properties_cnt = buf->len / 2;
    status.properties_id = (uint16_t *)buf->data;

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, ctx->recv_op, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            rsp->properties_cnt = MIN(rsp->properties_cnt, status.properties_cnt);
            memcpy(rsp->properties_id, status.properties_id, rsp->properties_cnt * 2);
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, gen_property_op_to_evt(ctx->recv_op), ctx, &status);
    }

    return 0;
}

static int bt_mesh_gen_property_cli_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
    struct net_buf_simple *buf)
{
    struct bt_mesh_gen_property status = {0};
    struct bt_mesh_gen_property *rsp;
    struct bt_mesh_model_cli_common *cli = model->rt->user_data;

    status.property_id = net_buf_simple_pull_le16(buf);
    if (buf->len > 0) {
        status.access = net_buf_simple_pull_u8(buf);
        status.data_len = buf->len;
        status.data = (uint8_t *)buf->data;
    } else {
        status.access = GENERIC_USER_PROPERTY_UNKNOWN;
    }

    if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, ctx->recv_op, ctx->addr, (void **)&rsp)) {
        if (rsp) {
            rsp->property_id = status.property_id;
            rsp->access = status.access;
            rsp->data_len = MIN(rsp->data_len, status.data_len);
            memcpy(rsp->data, status.data, rsp->data_len);
        }
        bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
    }

    if (cli->cb != NULL && cli->cb->status != NULL) {
        cli->cb->status(cli, gen_property_op_to_evt(ctx->recv_op), ctx, &status);
    }

    return 0;
}

int bt_mesh_gen_admin_properties_cli_get(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    struct bt_mesh_gen_properties *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_GEN_ADMIN_PROPS_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };

    if (rsp == NULL || rsp->properties_cnt == 0 || rsp->properties_id == NULL) {
        return -EINVAL;
    }

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_ADMIN_PROPS_GET, 0);
    bt_mesh_model_msg_init(&msg, OP_GEN_ADMIN_PROPS_GET);

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_gen_admin_property_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_property *req, struct bt_mesh_gen_property *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_GEN_ADMIN_PROP_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };

    if (req == NULL ||
        (operation != BT_MESH_CLI_OPERATION_GET && req->data_len > 0 && req->data == NULL)) {
        return -EINVAL;
    }

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, req->data_len + 3);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_GEN_ADMIN_PROP_GET);
        net_buf_simple_add_le16(&msg, req->property_id);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_GEN_ADMIN_PROP_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_GEN_ADMIN_PROP_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_le16(&msg, req->property_id);
        net_buf_simple_add_u8(&msg, req->access);
        net_buf_simple_add_mem(&msg, req->data, req->data_len);
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_gen_mfr_properties_cli_get(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    struct bt_mesh_gen_properties *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_GEN_MFR_PROPS_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };

    if (rsp == NULL || rsp->properties_cnt == 0 || rsp->properties_id == NULL) {
        return -EINVAL;
    }

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_MFR_PROPS_GET, 0);
    bt_mesh_model_msg_init(&msg, OP_GEN_MFR_PROPS_GET);

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_gen_mfr_property_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_mfr_property_req *req, struct bt_mesh_gen_property *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_GEN_MFR_PROP_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };

    if (req == NULL) {
        return -EINVAL;
    }

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, 3);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_GEN_MFR_PROP_GET);
        net_buf_simple_add_le16(&msg, req->property_id);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_GEN_MFR_PROP_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_GEN_MFR_PROP_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_le16(&msg, req->property_id);
        net_buf_simple_add_u8(&msg, req->access);
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_gen_user_properties_cli_get(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    struct bt_mesh_gen_properties *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_GEN_USER_PROPS_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };

    if (rsp == NULL || rsp->properties_cnt == 0 || rsp->properties_id == NULL) {
        return -EINVAL;
    }

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_USER_PROPS_GET, 0);
    bt_mesh_model_msg_init(&msg, OP_GEN_USER_PROPS_GET);

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_gen_user_property_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_user_property_req *req, struct bt_mesh_gen_property *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_GEN_USER_PROP_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };

    if (req == NULL) {
        return -EINVAL;
    }

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_DUMMY_2_BYTE, req->data_len + 2);

    if (operation == BT_MESH_CLI_OPERATION_GET) {
        bt_mesh_model_msg_init(&msg, OP_GEN_USER_PROP_GET);
        net_buf_simple_add_le16(&msg, req->property_id);
    } else {
        if (operation == BT_MESH_CLI_OPERATION_SET) {
            bt_mesh_model_msg_init(&msg, OP_GEN_USER_PROP_SET);
        } else {
            bt_mesh_model_msg_init(&msg, OP_GEN_USER_PROP_SET_UNACK);
            rsp = NULL;
        }

        net_buf_simple_add_le16(&msg, req->property_id);
        net_buf_simple_add_mem(&msg, req->data, req->data_len);
    }

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_gen_client_properties_cli_get(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    uint16_t start_property_id, struct bt_mesh_gen_properties *rsp)
{
    const struct bt_mesh_msg_rsp_ctx rsp_ctx = {
        .ack = &cli->ack_ctx,
        .op = OP_GEN_CLIENT_PROPS_STATUS,
        .user_data = rsp,
        .timeout = cli->msg_timeout,
    };

    if (rsp == NULL || rsp->properties_cnt == 0 || rsp->properties_id == NULL) {
        return -EINVAL;
    }

    BT_MESH_MODEL_BUF_DEFINE(msg, OP_GEN_USER_PROPS_GET, 2);
    bt_mesh_model_msg_init(&msg, OP_GEN_USER_PROPS_GET);
    net_buf_simple_add_le16(&msg, start_property_id);

    return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

static int bt_mesh_gen_cli_init(const struct bt_mesh_model *model)
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
    cli->msg_timeout = CONFIG_BT_MESH_GEN_CLI_TIMEOUT;
    bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);

    return 0;
}

const struct bt_mesh_model_cb bt_mesh_gen_cli_cb = {
    .init = bt_mesh_gen_cli_init,
};

const struct bt_mesh_model_op bt_mesh_gen_onoff_cli_op[] = {
    { OP_GEN_ONOFF_STATUS,              BT_MESH_LEN_MIN(1),     bt_mesh_gen_onoff_cli_status },
    BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op bt_mesh_gen_level_cli_op[] = {
    { OP_GEN_LEVEL_STATUS,              BT_MESH_LEN_MIN(2),     bt_mesh_gen_level_cli_status },
    BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op bt_mesh_gen_def_trans_time_cli_op[] = {
    { OP_GEN_DEF_TRANS_TIME_STATUS,     BT_MESH_LEN_EXACT(1),   bt_mesh_gen_def_trans_time_cli_status },
    BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op bt_mesh_gen_power_onoff_cli_op[] = {
    { OP_GEN_ONPOWERUP_STATUS,          BT_MESH_LEN_EXACT(1),   bt_mesh_gen_onpowerup_cli_status },
    BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op bt_mesh_gen_power_level_cli_op[] = {
    { OP_GEN_POWER_LEVEL_STATUS,        BT_MESH_LEN_MIN(2),     bt_mesh_gen_power_level_cli_status },
    { OP_GEN_POWER_LAST_STATUS,         BT_MESH_LEN_EXACT(2),   bt_mesh_gen_power_last_cli_status },
    { OP_GEN_POWER_DEFAULT_STATUS,      BT_MESH_LEN_EXACT(2),   bt_mesh_gen_power_def_cli_status },
    { OP_GEN_POWER_RANGE_STATUS,        BT_MESH_LEN_EXACT(5),   bt_mesh_gen_power_range_cli_status },
    BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op bt_mesh_gen_battery_cli_op[] = {
    { OP_GEN_BATTERY_STATUS,            BT_MESH_LEN_EXACT(8),   bt_mesh_gen_battery_cli_status },
    BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op bt_mesh_gen_location_cli_op[] = {
    { OP_GEN_LOCATION_GLOBAL_STATUS,    BT_MESH_LEN_EXACT(10),  bt_mesh_gen_location_global_cli_status },
    { OP_GEN_LOCATION_LOCAL_STATUS,     BT_MESH_LEN_EXACT(9),   bt_mesh_gen_location_local_cli_status },
    BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op bt_mesh_gen_property_cli_op[] = {
    { OP_GEN_ADMIN_PROPS_STATUS,        BT_MESH_LEN_MIN(0),     bt_mesh_gen_properties_cli_status },
    { OP_GEN_ADMIN_PROP_STATUS,         BT_MESH_LEN_MIN(2),     bt_mesh_gen_property_cli_status },
    { OP_GEN_MFR_PROPS_STATUS,          BT_MESH_LEN_MIN(0),     bt_mesh_gen_properties_cli_status },
    { OP_GEN_MFR_PROP_STATUS,           BT_MESH_LEN_MIN(2),     bt_mesh_gen_property_cli_status },
    { OP_GEN_USER_PROPS_STATUS,         BT_MESH_LEN_MIN(0),     bt_mesh_gen_properties_cli_status },
    { OP_GEN_USER_PROP_STATUS,          BT_MESH_LEN_MIN(2),     bt_mesh_gen_property_cli_status },
    { OP_GEN_CLIENT_PROPS_STATUS,       BT_MESH_LEN_MIN(0),     bt_mesh_gen_properties_cli_status },
    BT_MESH_MODEL_OP_END,
};

