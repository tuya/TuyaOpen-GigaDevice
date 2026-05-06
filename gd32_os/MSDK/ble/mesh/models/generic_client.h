/*!
    \file    generic_client.h
    \brief   Header file for BLE mesh generic client.

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

#ifndef GENERIC_CLIENT_H_
#define GENERIC_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_BT_MESH_GEN_CLI_TIMEOUT                    5000

extern const struct bt_mesh_model_cb bt_mesh_gen_cli_cb;
extern const struct bt_mesh_model_op bt_mesh_gen_onoff_cli_op[];
extern const struct bt_mesh_model_op bt_mesh_gen_level_cli_op[];
extern const struct bt_mesh_model_op bt_mesh_gen_def_trans_time_cli_op[];
extern const struct bt_mesh_model_op bt_mesh_gen_power_onoff_cli_op[];
extern const struct bt_mesh_model_op bt_mesh_gen_power_level_cli_op[];
extern const struct bt_mesh_model_op bt_mesh_gen_battery_cli_op[];
extern const struct bt_mesh_model_op bt_mesh_gen_location_cli_op[];
extern const struct bt_mesh_model_op bt_mesh_gen_property_cli_op[];

#define BT_MESH_MODEL_GEN_ONOFF_CLI(cli, pub)                                                               \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_ONOFF_CLI, bt_mesh_gen_onoff_cli_op, pub, cli, &bt_mesh_gen_cli_cb)
#define BT_MESH_MODEL_GEN_LEVEL_CLI(cli, pub)                                                               \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_LEVEL_CLI, bt_mesh_gen_level_cli_op, pub, cli, &bt_mesh_gen_cli_cb)
#define BT_MESH_MODEL_GEN_DEF_TRANS_TIME_CLI(cli, pub)                                                      \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_DEF_TRANS_TIME_CLI, bt_mesh_gen_def_trans_time_cli_op, pub, cli,  \
                     &bt_mesh_gen_cli_cb)
#define BT_MESH_MODEL_GEN_POWER_ONOFF_CLI(cli, pub)                                                         \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_POWER_ONOFF_CLI, bt_mesh_gen_power_onoff_cli_op, pub, cli,        \
                     &bt_mesh_gen_cli_cb)
#define BT_MESH_MODEL_GEN_POWER_LEVEL_CLI(cli, pub)                                                         \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_POWER_LEVEL_CLI, bt_mesh_gen_power_level_cli_op, pub, cli,        \
                     &bt_mesh_gen_cli_cb)
#define BT_MESH_MODEL_GEN_BATTERY_CLI(cli, pub)                                                             \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_BATTERY_CLI, bt_mesh_gen_battery_cli_op, pub, cli, &bt_mesh_gen_cli_cb)
#define BT_MESH_MODEL_GEN_LOCATION_CLI(cli, pub)                                                            \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_LOCATION_CLI, bt_mesh_gen_location_cli_op, pub, cli, &bt_mesh_gen_cli_cb)
#define BT_MESH_MODEL_GEN_PROPERTY_CLI(cli, pub)                                                            \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_PROP_CLI, bt_mesh_gen_property_cli_op, pub, cli, &bt_mesh_gen_cli_cb)

struct bt_mesh_gen_onoff_set {
    uint8_t onoff;
    uint8_t tid;
    bool    op_en;
    uint8_t transition_time;
    uint8_t delay;
};

struct bt_mesh_gen_onoff_status {
    uint8_t present_onoff;
    bool    op_en;
    uint8_t target_onoff;
    uint8_t remain_time;
};

struct bt_mesh_gen_level_set {
    int16_t level;
    uint8_t tid;
    bool    op_en;
    uint8_t transition_time;
    uint8_t delay;
};

struct bt_mesh_gen_delta_set {
    int32_t delta_level;
    uint8_t tid;
    bool    op_en;
    uint8_t transition_time;
    uint8_t delay;
};

struct bt_mesh_gen_move_set {
    int16_t delta_level;
    uint8_t tid;
    bool    op_en;
    uint8_t transition_time;
    uint8_t delay;
};

struct bt_mesh_gen_level_status {
    int16_t present_level;
    bool    op_en;
    int16_t target_level;
    uint8_t remain_time;
};

struct bt_mesh_gen_def_trans_time_set {
    uint8_t transition_time;
};

struct bt_mesh_gen_def_trans_time_status {
    uint8_t transition_time;
};

struct bt_mesh_gen_onpowerup_set {
    uint8_t onpowerup;
};

struct bt_mesh_gen_onpowerup_status {
    uint8_t onpowerup;
};

struct bt_mesh_gen_power_level_set {
    uint16_t power;
    uint8_t  tid;
    bool     op_en;
    uint8_t  transition_time;
    uint8_t  delay;
};

struct bt_mesh_gen_power_level_status {
    uint16_t present_power;
    bool     op_en;
    uint16_t target_power;
    uint8_t  remain_time;
};

struct bt_mesh_gen_power_last_status {
    uint16_t last_power;
};

struct bt_mesh_gen_power_def_set {
    uint16_t def_power;
};

struct bt_mesh_gen_power_def_status {
    uint16_t def_power;
};

struct bt_mesh_gen_power_range_set {
    uint16_t range_min;
    uint16_t range_max;
};

struct bt_mesh_gen_power_range_status {
    uint8_t  status_code;
    uint16_t range_min;
    uint16_t range_max;
};

struct bt_mesh_gen_battery_status {
    uint32_t battery_level : 8,         /* The percentage of the charge level. value range: 0-100 */
             time_to_discharge : 24;    /* The remaining time (in minutes) of the discharging process */
    uint32_t time_to_charge : 24,       /* The remaining time (in minutes) of the charging process */
             battery_flags : 8;         /* 0–1 bits: Generic Battery Flags Presence, ref @bt_mesh_gen_battery_presence
                                         * 2–3 bits: Generic Battery Flags Indicator, ref @bt_mesh_gen_battery_indicator
                                         * 4–5 bits: Generic Battery Flags Charging, ref @bt_mesh_gen_battery_charging
                                         * 6–7 bits: Generic Battery Flags Serviceability, ref @bt_mesh_gen_battery_service */
};

struct bt_mesh_gen_location_global {
    /* Global Coordinates (Latitude) */
    int32_t  global_latitude;
    /* Global Coordinates (Longitude) */
    int32_t  global_longitude;
    /* Global Altitude */
    int16_t  global_altitude;
};

struct bt_mesh_gen_location_local {
    /* Local Coordinates (North) */
    int16_t  local_north;
    /* Local Coordinates (East) */
    int16_t  local_east;
    /* Local Altitude */
    int16_t  local_altitude;
    /* Floor Number */
    uint8_t  floor_number;
    /* Uncertainty */
    uint16_t uncertainty;
};

struct bt_mesh_gen_properties {
    uint16_t properties_cnt;
    uint16_t *properties_id;
};

struct bt_mesh_gen_property {
    uint16_t property_id;
    enum bt_mesh_generic_property_access access;
    uint8_t *data;
    uint16_t data_len;
};

struct bt_mesh_gen_mfr_property_req {
    uint16_t property_id;
    enum bt_mesh_generic_property_access access;
};

struct bt_mesh_gen_user_property_req {
    uint16_t property_id;
    uint8_t *data;
    uint16_t data_len;
};

int bt_mesh_gen_onoff_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_onoff_set *req, struct bt_mesh_gen_onoff_status *rsp);

int bt_mesh_gen_level_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_level_set *req, struct bt_mesh_gen_level_status *rsp);

int bt_mesh_gen_delta_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_delta_set *req, struct bt_mesh_gen_level_status *rsp);

int bt_mesh_gen_move_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_move_set *req, struct bt_mesh_gen_level_status *rsp);

int bt_mesh_gen_def_trans_time_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_def_trans_time_set *req, struct bt_mesh_gen_def_trans_time_status *rsp);

int bt_mesh_gen_onpowerup_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_onpowerup_set *req, struct bt_mesh_gen_onpowerup_status *rsp);

int bt_mesh_gen_power_level_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_power_level_set *req, struct bt_mesh_gen_power_level_status *rsp);

int bt_mesh_gen_power_last_cli_get(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    struct bt_mesh_gen_power_last_status *rsp);

int bt_mesh_gen_power_def_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_power_def_set *req, struct bt_mesh_gen_power_def_status *rsp);

int bt_mesh_gen_power_range_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_power_range_set *req, struct bt_mesh_gen_power_range_status *rsp);

int bt_mesh_gen_battery_cli_get(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    struct bt_mesh_gen_battery_status *rsp);

int bt_mesh_gen_location_global_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_location_global *req, struct bt_mesh_gen_location_global *rsp);

int bt_mesh_gen_location_local_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_location_local *req, struct bt_mesh_gen_location_local *rsp);

int bt_mesh_gen_admin_properties_cli_get(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    struct bt_mesh_gen_properties *rsp);

int bt_mesh_gen_admin_property_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_property *req, struct bt_mesh_gen_property *rsp);

int bt_mesh_gen_mfr_properties_cli_get(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    struct bt_mesh_gen_properties *rsp);

int bt_mesh_gen_mfr_property_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_mfr_property_req *req, struct bt_mesh_gen_property *rsp);

int bt_mesh_gen_user_properties_cli_get(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    struct bt_mesh_gen_properties *rsp);

int bt_mesh_gen_user_property_cli_handle(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    enum bt_mesh_cli_operation operation, struct bt_mesh_gen_user_property_req *req, struct bt_mesh_gen_property *rsp);

int bt_mesh_gen_client_properties_cli_get(struct bt_mesh_model_cli_common *cli, struct bt_mesh_msg_ctx *ctx,
    uint16_t start_property_id, struct bt_mesh_gen_properties *rsp);

#ifdef __cplusplus
}
#endif

#endif /* GENERIC_CLIENT_H_ */
