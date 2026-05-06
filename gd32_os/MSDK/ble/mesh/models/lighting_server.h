/*!
    \file    lighting_server.h
    \brief   Header file for BLE mesh lighting server.

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

#ifndef LIGHTING_SERVER_H_
#define LIGHTING_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "generic_server.h"

extern const struct bt_mesh_model_op bt_mesh_light_lightness_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_light_lightness_srv_cb;
extern const struct bt_mesh_model_op bt_mesh_light_lightness_setup_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_light_lightness_setup_srv_cb;
extern const struct bt_mesh_model_op bt_mesh_light_ctl_temperature_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_light_ctl_temperature_srv_cb;
extern const struct bt_mesh_model_op bt_mesh_light_ctl_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_light_ctl_srv_cb;
extern const struct bt_mesh_model_op bt_mesh_light_ctl_setup_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_light_ctl_setup_srv_cb;
extern const struct bt_mesh_model_op bt_mesh_light_hsl_hue_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_light_hsl_hue_srv_cb;
extern const struct bt_mesh_model_op bt_mesh_light_hsl_saturation_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_light_hsl_saturation_srv_cb;
extern const struct bt_mesh_model_op bt_mesh_light_hsl_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_light_hsl_srv_cb;
extern const struct bt_mesh_model_op bt_mesh_light_hsl_setup_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_light_hsl_setup_srv_cb;
extern const struct bt_mesh_model_op bt_mesh_light_xyl_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_light_xyl_srv_cb;
extern const struct bt_mesh_model_op bt_mesh_light_xyl_setup_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_light_xyl_setup_srv_cb;

#define BT_MESH_MODEL_LIGHT_LIGHTNESS_SRV(srv, pub, level_pub, power_onoff_pub, onoff_pub, def_trans_time_pub)  \
    BT_MESH_MODEL_GEN_LEVEL_SRV(&(srv)->level, level_pub),                                                      \
    BT_MESH_MODEL_GEN_POWER_ONOFF_SRV(&(srv)->power_onoff, power_onoff_pub, onoff_pub, def_trans_time_pub),     \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SRV, bt_mesh_light_lightness_srv_op, pub, srv,            \
                     &bt_mesh_light_lightness_srv_cb),                                                          \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SETUP_SRV, bt_mesh_light_lightness_setup_srv_op, NULL,    \
                     srv, &bt_mesh_light_lightness_setup_srv_cb)

#define BT_MESH_MODEL_LIGHT_CTL_TEMPERATURE_SRV(srv, pub, level_pub)                                            \
    BT_MESH_MODEL_GEN_LEVEL_SRV(&(srv)->level, level_pub),                                                      \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_CTL_TEMP_SRV, bt_mesh_light_ctl_temperature_srv_op, pub, srv,       \
                     &bt_mesh_light_ctl_temperature_srv_cb)

#define BT_MESH_MODEL_LIGHT_CTL_SRV(srv, pub)                                                                   \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_CTL_TEMP_SRV, bt_mesh_light_ctl_srv_op, pub, srv,                   \
                     &bt_mesh_light_ctl_srv_cb)                                                                 \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_CTL_SETUP_SRV, bt_mesh_light_ctl_setup_srv_op, NULL,                \
                     srv, &bt_mesh_light_ctl_setup_srv_cb)

#define BT_MESH_MODEL_LIGHT_HSL_HUE_SRV(srv, pub, level_pub)                                                    \
    BT_MESH_MODEL_GEN_LEVEL_SRV(&(srv)->level, level_pub),                                                      \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_HSL_HUE_SRV, bt_mesh_light_hsl_hue_srv_op, pub, srv,                \
                     &bt_mesh_light_hsl_hue_srv_cb)

#define BT_MESH_MODEL_LIGHT_HSL_SAT_SRV(srv, pub, level_pub)                                                    \
    BT_MESH_MODEL_GEN_LEVEL_SRV(&(srv)->level, level_pub),                                                      \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_HSL_SAT_SRV, bt_mesh_light_hsl_saturation_srv_op, pub, srv,         \
                     &bt_mesh_light_hsl_saturation_srv_cb)

#define BT_MESH_MODEL_LIGHT_HSL_SRV(srv, pub)                                                                   \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_HSL_SRV, bt_mesh_light_hsl_srv_op, pub, srv,                        \
                     &bt_mesh_light_hsl_srv_cb)                                                                 \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_HSL_SETUP_SRV, bt_mesh_light_hsl_setup_srv_op, NULL,                \
                     srv, &bt_mesh_light_hsl_setup_srv_cb)

#define BT_MESH_MODEL_LIGHT_XYL_SRV(srv, pub)                                                                   \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_XYL_SRV, bt_mesh_light_xyl_srv_op, pub, srv,                        \
                     &bt_mesh_light_xyl_srv_cb)                                                                 \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_XYL_SETUP_SRV, bt_mesh_light_xyl_setup_srv_op, NULL,                \
                     srv, &bt_mesh_light_xyl_setup_srv_cb)

struct bt_mesh_light_lightness_state {
    uint16_t actual;
    uint16_t target_actual;
    uint16_t last;
    uint16_t def;
    uint16_t range_min;
    uint16_t range_max;

    int32_t delta_lightness;
};

struct bt_mesh_light_lightness_srv {
    struct bt_mesh_srv_callbacks *cb;
    const struct bt_mesh_model *model;
    const struct bt_mesh_model *setup_model;
    struct bt_mesh_pre_tid pre_tid;
    struct bt_mesh_state_transition transition;
    struct bt_mesh_light_lightness_state state;

    struct bt_mesh_gen_level_srv level;
    struct bt_mesh_gen_power_onoff_srv power_onoff;
};

struct bt_mesh_light_temperature_state {
    uint16_t temp;
    uint16_t target_temp;
    uint16_t last_temp;
    uint16_t temp_def;
    uint16_t temp_range_min; // default 0xffff
    uint16_t temp_range_max; // default 0xffff

    int16_t deltauv;
    int16_t target_deltauv;
    int16_t deltauv_def;

    int32_t delta_temp;
    int32_t delta_deltauv;
};

struct bt_mesh_light_ctl_temperature_srv {
    struct bt_mesh_srv_callbacks *cb;
    const struct bt_mesh_model *model;
    struct bt_mesh_pre_tid pre_tid;
    struct bt_mesh_state_transition transition;
    struct bt_mesh_light_temperature_state state;

    struct bt_mesh_gen_level_srv level;
};

struct bt_mesh_light_ctl_srv {
    struct bt_mesh_srv_callbacks *cb;
    const struct bt_mesh_model *model;
    const struct bt_mesh_model *setup_model;
    struct bt_mesh_pre_tid pre_tid;
    struct bt_mesh_state_transition transition;

    struct bt_mesh_light_lightness_srv *lightness;
    struct bt_mesh_light_ctl_temperature_srv temperature;
};

struct bt_mesh_light_hsl_hue_state {
    uint16_t hue;
    uint16_t target_hue;
    uint16_t last_hue;
    uint16_t hue_def;
    uint16_t hue_range_min;
    uint16_t hue_range_max;

    int32_t delta_hue;
};

struct bt_mesh_light_hsl_hue_srv {
    struct bt_mesh_srv_callbacks *cb;
    const struct bt_mesh_model *model;
    struct bt_mesh_pre_tid pre_tid;
    struct bt_mesh_state_transition transition;
    struct bt_mesh_light_hsl_hue_state state;

    struct bt_mesh_gen_level_srv level;
};

struct bt_mesh_light_hsl_saturation_state {
    uint16_t saturation;
    uint16_t target_saturation;
    uint16_t last_saturation;
    uint16_t saturation_def;
    uint16_t saturation_range_min;
    uint16_t saturation_range_max;

    int32_t delta_saturation;
};

struct bt_mesh_light_hsl_saturation_srv {
    struct bt_mesh_srv_callbacks *cb;
    const struct bt_mesh_model *model;
    struct bt_mesh_pre_tid pre_tid;
    struct bt_mesh_state_transition transition;
    struct bt_mesh_light_hsl_saturation_state state;

    struct bt_mesh_gen_level_srv level;
};

struct bt_mesh_light_hsl_srv {
    struct bt_mesh_srv_callbacks *cb;
    const struct bt_mesh_model *model;
    const struct bt_mesh_model *setup_model;
    struct bt_mesh_pre_tid pre_tid;
    struct bt_mesh_state_transition transition;

    struct bt_mesh_light_lightness_srv *lightness;
    struct bt_mesh_light_hsl_hue_srv hue;
    struct bt_mesh_light_hsl_saturation_srv saturation;
};


struct bt_mesh_light_xyl_state {
    uint16_t x;
    uint16_t y;
    uint16_t target_x;
    uint16_t target_y;
    uint16_t last_x;
    uint16_t last_y;
    uint16_t x_def;
    uint16_t y_def;
    uint16_t x_range_min;
    uint16_t y_range_min;
    uint16_t x_range_max;
    uint16_t y_range_max;

    int32_t delta_x;
    int32_t delta_y;
};

struct bt_mesh_light_xyl_srv {
    struct bt_mesh_srv_callbacks *cb;
    const struct bt_mesh_model *model;
    const struct bt_mesh_model *setup_model;
    struct bt_mesh_pre_tid pre_tid;
    struct bt_mesh_state_transition transition;
    struct bt_mesh_light_xyl_state state;

    struct bt_mesh_light_lightness_srv *lightness;
};

__INLINE uint16_t gen_level_to_light_common(int16_t level)
{
    return level + 32768;
}

__INLINE int16_t light_common_to_gen_level(uint16_t actual)
{
    return actual - 32768;
}

__INLINE int16_t light_actual_to_gen_onoff(uint16_t actual)
{
    return actual > 0 ? 1 : 0;
}

__INLINE int16_t light_ctl_temp_to_gen_level(uint16_t actual, uint16_t range_min, uint16_t range_max)
{
    if (range_max == range_min)
        return 0;

    return DIV_ROUND_UP((CLAMP(actual, range_min, range_max) - range_min) * 65535, (range_max - range_min)) - 32768;
}

__INLINE uint16_t gen_level_to_light_ctl_temp(int16_t level, uint16_t range_min, uint16_t range_max)
{
    return range_min + DIV_ROUND_UP((int32_t)(level + 32768) * (range_max - range_min), 65535);
}

__INLINE uint16_t light_hsl_hue_update(uint16_t hue, uint16_t hue_range_min, uint16_t hue_range_max)
{
    if (hue_range_min < hue_range_max) {
        if (hue < hue_range_min) {
            hue = hue_range_min;
        } else if (hue > hue_range_max) {
            hue = hue_range_max;
        }
    } else if (hue_range_min > hue_range_max) {
        if (hue < hue_range_min && hue >= (hue_range_min + hue_range_max) / 2) {
            hue = hue_range_min;
        } else if (hue > hue_range_max && hue < (hue_range_min + hue_range_max) / 2) {
            hue = hue_range_max;
        }
    } else {
        hue = hue_range_min;
    }

    return hue;
}

void light_lightness_config(struct bt_mesh_light_lightness_srv *srv, uint16_t lightness);
void light_lightness_status_publish(const struct bt_mesh_model *model);
void light_ctl_temperature_config(struct bt_mesh_light_ctl_temperature_srv *srv, uint16_t temp, int16_t deltauv);
void light_ctl_temperature_status_publish(const struct bt_mesh_model *model);
void light_ctl_status_publish(const struct bt_mesh_model *model);
void light_hsl_hue_config(struct bt_mesh_light_hsl_hue_srv *srv, uint16_t hue);
void light_hsl_hue_status_publish(const struct bt_mesh_model *model);
void light_hsl_saturation_config(struct bt_mesh_light_hsl_saturation_srv *srv, uint16_t saturation);
void light_hsl_saturation_status_publish(const struct bt_mesh_model *model);
void light_hsl_status_publish(const struct bt_mesh_model *model);
void light_xyl_config(struct bt_mesh_light_xyl_srv *srv, uint16_t x, uint16_t y);
void light_xyl_status_publish(const struct bt_mesh_model *model);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_SERVER_H_ */
