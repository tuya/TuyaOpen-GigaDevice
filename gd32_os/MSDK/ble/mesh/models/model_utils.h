/*!
    \file    model_utils.h
    \brief   Header file for BLE mesh model utils.

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

#ifndef MODEL_UTILS_H_
#define MODEL_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "compiler.h"

#define BT_MESH_TEMPERATURE_MIN                0x0320
#define BT_MESH_TEMPERATURE_MAX                0x4E20
#define BT_MESH_TEMPERATURE_UNKNOWN            0xFFFF

#define BT_MESH_CLI_OPERATION_CHECK(operation, req)                                                                 \
    if ((operation == BT_MESH_CLI_OPERATION_SET || operation == BT_MESH_CLI_OPERATION_SET_UNACK) && req == NULL) {  \
        return -EINVAL;                                                                                             \
    }

enum bt_mesh_cli_operation {
    BT_MESH_CLI_OPERATION_GET,
    BT_MESH_CLI_OPERATION_SET,
    BT_MESH_CLI_OPERATION_SET_UNACK,
};

enum bt_mesh_generic_property_access {
    NOT_GENERIC_USER_PROPERTY               = 0,
    GENERIC_USER_PROPERTY_READ              = 1,
    GENERIC_USER_PROPERTY_WRITTEN           = 2,
    GENERIC_USER_PROPERTY_READ_AND_WRITTEN  = 3,
    GENERIC_USER_PROPERTY_UNKNOWN           = 4,
};

/* Refer 7.2 of Mesh Model Specification */
enum bt_mesh_status_codes {
    BT_MESH_STATUS_RANGE_UPDATE_SUCCESS,
    BT_MESH_STATUS_CANNOT_SET_RANGE_MIN,
    BT_MESH_STATUS_CANNOT_SET_RANGE_MAX,
    BT_MESH_STATUS_UNKNOWN
};

enum bt_mesh_srv_callback_evt {
    BT_MESH_SRV_GEN_ONOFF_EVT,
    BT_MESH_SRV_GEN_LEVEL_EVT,
    BT_MESH_SRV_GEN_DEF_TRANS_TIME_EVT,
    BT_MESH_SRV_GEN_POWER_ONOFF_EVT,
    BT_MESH_SRV_GEN_POWER_LEVEL_EVT,
    BT_MESH_SRV_GEN_BATTERY_EVT,
    BT_MESH_SRV_GEN_LOCATION_GLOBAL_EVT,
    BT_MESH_SRV_GEN_LOCATION_LOCAL_EVT,
    BT_MESH_SRV_GEN_ADMIN_PROPERTY_EVT,
    BT_MESH_SRV_GEN_MFR_PROPERTY_EVT,

    BT_MESH_SRV_LIGHT_LIGHTNESS_EVT,
    BT_MESH_SRV_LIGHT_CTL_TEMP_EVT,
    BT_MESH_SRV_LIGHT_HSL_HUE_EVT,
    BT_MESH_SRV_LIGHT_HSL_SATURATION_EVT,
    BT_MESH_SRV_LIGHT_XYL_EVT,
};

enum bt_mesh_cli_callback_evt {
    BT_MESH_CLI_GEN_ONOFF_EVT,
    BT_MESH_CLI_GEN_LEVEL_EVT,
    BT_MESH_CLI_GEN_DEF_TRANS_TIME_EVT,
    BT_MESH_CLI_GEN_POWER_ONOFF_EVT,
    BT_MESH_CLI_GEN_POWER_LEVEL_EVT,
    BT_MESH_CLI_GEN_POWER_LAST_EVT,
    BT_MESH_CLI_GEN_POWER_DEF_EVT,
    BT_MESH_CLI_GEN_POWER_RANGE_EVT,
    BT_MESH_CLI_GEN_BATTERY_EVT,
    BT_MESH_CLI_GEN_LOCATION_GLOBAL_EVT,
    BT_MESH_CLI_GEN_LOCATION_LOCAL_EVT,
    BT_MESH_CLI_GEN_ADMIN_PROPERTIES_EVT,
    BT_MESH_CLI_GEN_ADMIN_PROPERTY_EVT,
    BT_MESH_CLI_GEN_MFR_PROPERTIES_EVT,
    BT_MESH_CLI_GEN_MFR_PROPERTY_EVT,
    BT_MESH_CLI_GEN_USER_PROPERTIES_EVT,
    BT_MESH_CLI_GEN_USER_PROPERTY_EVT,
    BT_MESH_CLI_GEN_CLIENT_PROPERTIES_EVT,

    BT_MESH_CLI_LIGHT_LIGHTNESS_EVT,
    BT_MESH_CLI_LIGHT_LIGHTNESS_LAST_EVT,
    BT_MESH_CLI_LIGHT_LIGHTNESS_DEFAULT_EVT,
    BT_MESH_CLI_LIGHT_LIGHTNESS_RANGE_EVT,
    BT_MESH_CLI_LIGHT_CTL_EVT,
    BT_MESH_CLI_LIGHT_CTL_TEMPERATURE_EVT,
    BT_MESH_CLI_LIGHT_CTL_TEMPERATURE_RANGE_EVT,
    BT_MESH_CLI_LIGHT_CTL_DEFAULT_EVT,
    BT_MESH_CLI_LIGHT_HSL_EVT,
    BT_MESH_CLI_LIGHT_HSL_TARGET_EVT,
    BT_MESH_CLI_LIGHT_HSL_RANGE_EVT,
    BT_MESH_CLI_LIGHT_HSL_DEFAULT_EVT,
    BT_MESH_CLI_LIGHT_HSL_HUE_EVT,
    BT_MESH_CLI_LIGHT_HSL_SAT_EVT,
    BT_MESH_CLI_LIGHT_XYL_EVT,
    BT_MESH_CLI_LIGHT_XYL_TARGET_EVT,
    BT_MESH_CLI_LIGHT_XYL_RANGE_EVT,
    BT_MESH_CLI_LIGHT_XYL_DEFAULT_EVT,
};


struct bt_mesh_pre_tid {
    uint8_t  tid;
    uint16_t src;
    uint16_t dst;
    int64_t  timestamp;
};

struct bt_mesh_srv_callbacks {
    void *user_data;
    void (*const get)(void *user_data, enum bt_mesh_srv_callback_evt evt, void *state);
    void (*const set)(void *user_data, enum bt_mesh_srv_callback_evt evt, void *srv);
    void (*const state_change)(void *user_data, enum bt_mesh_srv_callback_evt evt, void *state);
};

struct bt_mesh_cli_callbacks {
    void (*const status)(void *cli, enum bt_mesh_cli_callback_evt evt, struct bt_mesh_msg_ctx *ctx, void *status);
};

struct bt_mesh_model_cli_common {
    struct bt_mesh_cli_callbacks *cb;
    const struct bt_mesh_model *model;
    struct bt_mesh_msg_ack_ctx ack_ctx;
    int32_t msg_timeout;
};

int bt_mesh_tid_check_and_update(struct bt_mesh_pre_tid *pre, uint8_t tid, uint16_t src, uint16_t dst);

uint16_t bt_mesh_sqrt32(uint32_t val);

__INLINE uint16_t light_actual_to_linear(uint16_t actual)
{
    /* Conversion: linear = CEIL(65535 * (actual * actual) / (65535 * 65535))) */
    return DIV_ROUND_UP(actual * actual, 65535UL);
}

__INLINE uint16_t light_linear_to_actual(uint16_t linear)
{
    /* Conversion: actual = 65535 * sqrt(linear / 65535) */
    return bt_mesh_sqrt32(linear * 65535UL);
}

#ifdef __cplusplus
}
#endif

#endif /* MODEL_UTILS_H_ */

