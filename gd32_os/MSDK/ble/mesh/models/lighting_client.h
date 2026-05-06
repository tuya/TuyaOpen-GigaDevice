/*!
    \file    lighting_client.h
    \brief   Header file for BLE mesh lighting client.

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

#ifndef LIGHTING_CLIENT_H_
#define LIGHTING_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_BT_MESH_LIGHT_CLI_TIMEOUT                    5000

extern const struct bt_mesh_model_cb bt_mesh_light_cli_cb;
const struct bt_mesh_model_op bt_mesh_light_lightness_cli_op[];
const struct bt_mesh_model_op bt_mesh_light_ctl_cli_op[];
const struct bt_mesh_model_op bt_mesh_light_hsl_cli_op[];
const struct bt_mesh_model_op bt_mesh_light_xyl_cli_op[];

#define BT_MESH_MODEL_LIGHT_LIGHTNESS_CLI(cli, pub)                                                         \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI, bt_mesh_light_lightness_cli_op, pub, cli, &bt_mesh_light_cli_cb)
#define BT_MESH_MODEL_LIGHT_CTL_CLI(cli, pub)                                                               \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_CTL_CLI, bt_mesh_light_ctl_cli_op, pub, cli, &bt_mesh_light_cli_cb)
#define BT_MESH_MODEL_LIGHT_HSL_CLI(cli, pub)                                                               \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_HSL_CLI, bt_mesh_light_hsl_cli_op, pub, cli, &bt_mesh_light_cli_cb)
#define BT_MESH_MODEL_LIGHT_XYL_CLI(cli, pub)                                                               \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_XYL_CLI, bt_mesh_light_xyl_cli_op, pub, cli, &bt_mesh_light_cli_cb)

struct bt_mesh_light_lightness_set {
    uint16_t actual;
    uint8_t tid;
    bool    op_en;
    uint8_t transition_time;
    uint8_t delay;
};

struct bt_mesh_light_lightness_status {
    uint16_t present_actual;
    bool     op_en;
    uint16_t target_actual;
    uint8_t  remain_time;
};

struct bt_mesh_light_lightness_linear_set {
    uint16_t linear;
    uint8_t  tid;
    bool     op_en;
    uint8_t  transition_time;
    uint8_t  delay;
};

struct bt_mesh_light_lightness_last_status {
    uint16_t last_actual;
};

struct bt_mesh_light_lightness_default {
    uint16_t lightness;
};

struct bt_mesh_light_lightness_range_set {
    uint16_t range_min;
    uint16_t range_max;
};

struct bt_mesh_light_lightness_range_status {
    uint8_t  status_code;
    uint16_t range_min;
    uint16_t range_max;
};

struct bt_mesh_light_ctl_set {
    uint16_t lightness;
    uint16_t temp;
    int16_t  delta_uv;
    uint8_t  tid;
    bool     op_en;
    uint8_t  transition_time;
    uint8_t  delay;
};

struct bt_mesh_light_ctl_status {
    uint16_t present_lightness;
    uint16_t present_temp;
    bool     op_en;
    uint16_t target_lightness;
    uint16_t target_temp;
    uint8_t  remain_time;
};

struct bt_mesh_light_ctl_temp_set {
    uint16_t temp;
    int16_t  delta_uv;
    uint8_t  tid;
    bool     op_en;
    uint8_t  transition_time;
    uint8_t  delay;
};

struct bt_mesh_light_ctl_temp_status {
    uint16_t present_temp;
    int16_t  present_delta_uv;
    bool     op_en;
    uint16_t target_temp;
    int16_t  target_delta_uv;
    uint8_t  remain_time;
};

struct bt_mesh_light_ctl_temp_range_set {
    uint16_t range_min;
    uint16_t range_max;
};

struct bt_mesh_light_ctl_temp_range_status {
    uint8_t  status_code;
    uint16_t range_min;
    uint16_t range_max;
};

struct bt_mesh_light_ctl_default {
    uint16_t lightness;
    uint16_t temp;
    int16_t  delta_uv;
};

struct bt_mesh_light_hsl_set {
    uint16_t lightness;
    uint16_t hue;
    uint16_t sat;
    uint8_t  tid;
    bool     op_en;
    uint8_t  transition_time;
    uint8_t  delay;
};

struct bt_mesh_light_hsl_status {
    uint16_t lightness;
    uint16_t hue;
    uint16_t sat;
    bool     op_en;
    uint8_t  remain_time;
};

struct bt_mesh_light_hsl_range_set {
    uint16_t hue_range_min;
    uint16_t hue_range_max;
    uint16_t sat_range_min;
    uint16_t sat_range_max;
};

struct bt_mesh_light_hsl_range_status {
    uint8_t  status_code;
    uint16_t hue_range_min;
    uint16_t hue_range_max;
    uint16_t sat_range_min;
    uint16_t sat_range_max;
};

struct bt_mesh_light_hsl_default {
    uint16_t lightness;
    uint16_t hue;
    uint16_t sat;
};

struct bt_mesh_light_hsl_hue_set {
    uint16_t hue;
    uint8_t  tid;
    bool     op_en;
    uint8_t  transition_time;
    uint8_t  delay;
};

struct bt_mesh_light_hsl_hue_status {
    uint16_t hue;
    bool     op_en;
    uint16_t target_hue;
    uint8_t  remain_time;
};

struct bt_mesh_light_hsl_sat_set {
    uint16_t sat;
    uint8_t  tid;
    bool     op_en;
    uint8_t  transition_time;
    uint8_t  delay;
};

struct bt_mesh_light_hsl_sat_status {
    uint16_t sat;
    bool     op_en;
    uint16_t target_sat;
    uint8_t  remain_time;
};

struct bt_mesh_light_xyl_set {
    uint16_t lightness;
    uint16_t x;
    uint16_t y;
    uint8_t  tid;
    bool     op_en;
    uint8_t  transition_time;
    uint8_t  delay;
};

struct bt_mesh_light_xyl_status {
    uint16_t lightness;
    uint16_t x;
    uint16_t y;
    bool     op_en;
    uint8_t  remain_time;
};

struct bt_mesh_light_xyl_range_set {
    uint16_t x_range_min;
    uint16_t x_range_max;
    uint16_t y_range_min;
    uint16_t y_range_max;
};

struct bt_mesh_light_xyl_range_status {
    uint8_t  status_code;
    uint16_t x_range_min;
    uint16_t x_range_max;
    uint16_t y_range_min;
    uint16_t y_range_max;
};

struct bt_mesh_light_xyl_default {
    uint16_t lightness;
    uint16_t x;
    uint16_t y;
};

int bt_mesh_light_lightness_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_lightness_set *req,
    struct bt_mesh_light_lightness_status *rsp);
int bt_mesh_light_lightness_linear_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_lightness_linear_set *req,
    struct bt_mesh_light_lightness_status *rsp);
int bt_mesh_light_lightness_last_cli_get(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    struct bt_mesh_light_lightness_last_status *rsp);
int bt_mesh_light_lightness_default_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_lightness_default *req,
    struct bt_mesh_light_lightness_default *rsp);
int bt_mesh_light_lightness_range_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_lightness_range_set *req,
    struct bt_mesh_light_lightness_range_status *rsp);
int bt_mesh_light_ctl_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_ctl_set *req, struct bt_mesh_light_ctl_status *rsp);
int bt_mesh_light_ctl_temp_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_ctl_temp_set *req,
    struct bt_mesh_light_ctl_temp_status *rsp);
int bt_mesh_light_ctl_temp_range_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_ctl_temp_range_set *req,
    struct bt_mesh_light_ctl_temp_range_status *rsp);
int bt_mesh_light_ctl_default_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_ctl_default *req,
    struct bt_mesh_light_ctl_default *rsp);
int bt_mesh_light_hsl_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_hsl_set *req, struct bt_mesh_light_hsl_status *rsp);
int bt_mesh_light_hsl_target_cli_get(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    struct bt_mesh_light_hsl_status *rsp);
int bt_mesh_light_hsl_range_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_hsl_range_set *req,
    struct bt_mesh_light_hsl_range_status *rsp);
int bt_mesh_light_hsl_default_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_hsl_default *req,
    struct bt_mesh_light_hsl_default *rsp);
int bt_mesh_light_hsl_hue_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_hsl_hue_set *req,
    struct bt_mesh_light_hsl_hue_status *rsp);
int bt_mesh_light_hsl_sat_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_hsl_sat_set *req,
    struct bt_mesh_light_hsl_sat_status *rsp);
int bt_mesh_light_xyl_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_xyl_set *req, struct bt_mesh_light_xyl_status *rsp);
int bt_mesh_light_xyl_target_cli_get(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    struct bt_mesh_light_xyl_status *rsp);
int bt_mesh_light_xyl_range_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_xyl_range_set *req,
    struct bt_mesh_light_xyl_range_status *rsp);
int bt_mesh_light_xyl_default_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_light_xyl_default *req,
    struct bt_mesh_light_xyl_default *rsp);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_CLIENT_H_ */
