/*!
    \file    generic_server.h
    \brief   Header file for BLE mesh generic server.

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

#ifndef GENERIC_SERVER_H_
#define GENERIC_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "device_properties.h"

extern const struct bt_mesh_model_op bt_mesh_gen_onoff_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_gen_onoff_srv_cb;

extern const struct bt_mesh_model_op bt_mesh_gen_level_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_gen_level_srv_cb;

extern const struct bt_mesh_model_op bt_mesh_gen_def_trans_time_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_gen_def_trans_time_srv_cb;

extern const struct bt_mesh_model_op bt_mesh_gen_power_onoff_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_gen_power_onoff_srv_cb;
extern const struct bt_mesh_model_op bt_mesh_gen_power_onoff_setup_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_gen_power_onoff_setup_srv_cb;

extern const struct bt_mesh_model_op bt_mesh_gen_power_level_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_gen_power_level_srv_cb;
extern const struct bt_mesh_model_op bt_mesh_gen_power_level_setup_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_gen_power_level_setup_srv_cb;

extern const struct bt_mesh_model_op bt_mesh_gen_battery_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_gen_battery_srv_cb;

extern const struct bt_mesh_model_op bt_mesh_gen_location_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_gen_location_srv_cb;
extern const struct bt_mesh_model_op bt_mesh_gen_location_setup_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_gen_location_setup_srv_cb;

extern const struct bt_mesh_model_op bt_mesh_gen_admin_property_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_gen_admin_property_srv_cb;
extern const struct bt_mesh_model_op bt_mesh_gen_mfr_property_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_gen_mfr_property_srv_cb;
extern const struct bt_mesh_model_op bt_mesh_gen_user_property_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_gen_user_property_srv_cb;
extern const struct bt_mesh_model_op bt_mesh_gen_client_property_srv_op[];
extern const struct bt_mesh_model_cb bt_mesh_gen_client_property_srv_cb;

#define BT_MESH_MODEL_GEN_ONOFF_SRV(srv, pub)                                                                   \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, bt_mesh_gen_onoff_srv_op, pub, srv, &bt_mesh_gen_onoff_srv_cb)


#define BT_MESH_MODEL_GEN_LEVEL_SRV(srv, pub)                                                                   \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_LEVEL_SRV, bt_mesh_gen_level_srv_op, pub, srv, &bt_mesh_gen_level_srv_cb)

#define BT_MESH_MODEL_GEN_DEF_TRANS_TIME_SRV(srv, pub)                                                          \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_DEF_TRANS_TIME_SRV, bt_mesh_gen_def_trans_time_srv_op, pub, srv,      \
                     &bt_mesh_gen_def_trans_time_srv_cb)

#define BT_MESH_MODEL_GEN_POWER_ONOFF_SRV(srv, pub, onoff_pub, def_trans_time_pub)                              \
    BT_MESH_MODEL_GEN_ONOFF_SRV(&(srv)->onoff, onoff_pub),                                                      \
    BT_MESH_MODEL_GEN_DEF_TRANS_TIME_SRV(&(srv)->def_trans_time, def_trans_time_pub),                           \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_POWER_ONOFF_SRV, bt_mesh_gen_power_onoff_srv_op, pub, srv,            \
                     &bt_mesh_gen_power_onoff_srv_cb),                                                          \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_POWER_ONOFF_SETUP_SRV, bt_mesh_gen_power_onoff_setup_srv_op, NULL,    \
                     srv, &bt_mesh_gen_power_onoff_setup_srv_cb)

#define BT_MESH_MODEL_GEN_POWER_LEVEL_SRV(srv, pub, level_pub, power_onoff_pub, onoff_pub, def_trans_time_pub)  \
    BT_MESH_MODEL_GEN_LEVEL_SRV(&(srv)->level, level_pub),                                                      \
    BT_MESH_MODEL_GEN_POWER_ONOFF_SRV(&(srv)->power_onoff, power_onoff_pub, onoff_pub, def_trans_time_pub),     \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_POWER_LEVEL_SRV, bt_mesh_gen_power_level_srv_op, pub, srv,            \
                     &bt_mesh_gen_power_level_srv_cb),                                                          \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_POWER_LEVEL_SETUP_SRV, bt_mesh_gen_power_level_setup_srv_op, NULL,    \
                     srv, &bt_mesh_gen_power_level_setup_srv_cb)

#define BT_MESH_MODEL_GEN_BATTERY_SRV(srv, pub)                                                                 \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_BATTERY_SRV, bt_mesh_gen_battery_srv_op, pub, srv,                    \
                     &bt_mesh_gen_battery_srv_cb)

#define BT_MESH_MODEL_GEN_LOCATION_SRV(srv, pub)                                                                \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_LOCATION_SRV, bt_mesh_gen_location_srv_op, pub, srv,                  \
                     &bt_mesh_gen_location_srv_cb),                                                             \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_LOCATION_SETUPSRV, bt_mesh_gen_location_setup_srv_op, NULL, srv,      \
                     &bt_mesh_gen_location_setup_srv_cb)

#define BT_MESH_MODEL_GEN_ADMIN_PROPERTY_SRV(srv, pub)                                                          \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_ADMIN_PROP_SRV, bt_mesh_gen_admin_property_srv_op, pub, srv,          \
                     &bt_mesh_gen_admin_property_srv_cb)

#define BT_MESH_MODEL_GEN_MFR_PROPERTY_SRV(srv, pub)                                                            \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_MANUFACTURER_PROP_SRV, bt_mesh_gen_mfr_property_srv_op, pub, srv,     \
                     &bt_mesh_gen_mfr_property_srv_cb)

#define BT_MESH_MODEL_GEN_USER_PROPERTY_SRV(srv, pub)                                                           \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_USER_PROP_SRV, bt_mesh_gen_user_property_srv_op, pub, srv,            \
                     &bt_mesh_gen_user_property_srv_cb)

#define BT_MESH_MODEL_GEN_CLIENT_PROPERTY_SRV(srv, pub)                                                         \
    BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_CLIENT_PROP_SRV, bt_mesh_gen_client_property_srv_op, pub, srv,        \
                     &bt_mesh_gen_client_property_srv_cb)

struct bt_mesh_gen_onoff_state {
    uint8_t onoff;
    uint8_t target_onoff;
};

struct bt_mesh_gen_onoff_srv {
    struct bt_mesh_srv_callbacks *cb;
    const struct bt_mesh_model *model;
    struct bt_mesh_pre_tid pre_tid;
    struct bt_mesh_state_transition transition;
    struct bt_mesh_gen_onoff_state state;
};

struct bt_mesh_gen_level_state {
    int16_t level;
    int16_t target_level;
    int16_t last_level;
    int16_t delta_level;
};

struct bt_mesh_gen_level_srv {
    struct bt_mesh_srv_callbacks *cb;
    const struct bt_mesh_model *model;
    struct bt_mesh_pre_tid pre_tid;
    struct bt_mesh_state_transition transition;
    struct bt_mesh_gen_level_state state;
};

struct bt_mesh_gen_def_trans_time_state {
    uint8_t transition_time;
};

struct bt_mesh_gen_def_trans_time_srv {
    struct bt_mesh_srv_callbacks *cb;
    const struct bt_mesh_model *model;
    struct bt_mesh_gen_def_trans_time_state state;
};

struct bt_mesh_gen_onpowerup_state {
    uint8_t onpowerup;
};

struct bt_mesh_gen_power_onoff_srv {
    struct bt_mesh_srv_callbacks *cb;
    const struct bt_mesh_model *model;
    const struct bt_mesh_model *setup_model;
    struct bt_mesh_gen_onpowerup_state state;

    struct bt_mesh_gen_onoff_srv onoff;
    struct bt_mesh_gen_def_trans_time_srv def_trans_time;
};

struct bt_mesh_gen_power_level_state {
    uint16_t actual;
    uint16_t target_actual;
    uint16_t last;
    uint16_t def;
    uint16_t range_min;
    uint16_t range_max;

    int32_t  delta_power;
};

struct bt_mesh_gen_power_level_srv {
    struct bt_mesh_srv_callbacks *cb;
    const struct bt_mesh_model *model;
    const struct bt_mesh_model *setup_model;
    struct bt_mesh_pre_tid pre_tid;
    struct bt_mesh_state_transition transition;
    struct bt_mesh_gen_power_level_state state;

    struct bt_mesh_gen_level_srv level;
    struct bt_mesh_gen_power_onoff_srv power_onoff;
};

enum bt_mesh_gen_battery_presence {
    /* The battery is not present. */
    BT_MESH_GEN_BATTERY_PRESENCE_NOT_PRESENT,
    /* The battery is present and is removable. */
    BT_MESH_GEN_BATTERY_PRESENCE_PRESENT_REMOVABLE,
    /* The battery is present and is non-removable. */
    BT_MESH_GEN_BATTERY_PRESENCE_PRESENT_NON_REMOVABLE,
    /* The battery presence is unknown. */
    BT_MESH_GEN_BATTERY_PRESENCE_UNKNOWN,
};

enum bt_mesh_gen_battery_indicator {
    /* The battery charge is Critically Low Level. */
    BT_MESH_GEN_BATTERY_INDICATOR_CRITICALLY_LOW,
    /* The battery charge is Low Level. */
    BT_MESH_GEN_BATTERY_INDICATOR_LOW,
    /* The battery charge is Good Level. */
    BT_MESH_GEN_BATTERY_INDICATOR_GOOD,
    /* The battery charge is unknown. */
    BT_MESH_GEN_BATTERY_INDICATOR_UNKNOWN,
};

enum bt_mesh_gen_battery_charging {
    /* The battery is not chargeable. */
    BT_MESH_GEN_BATTERY_NOT_CHARGEABLE,
    /* The battery is chargeable and is not charging. */
    BT_MESH_GEN_BATTERY_CHARGEABLE_NOT_CHARGING,
    /* The battery is chargeable and is charging. */
    BT_MESH_GEN_BATTERY_CHARGEABLE_CHARGING,
    /* The battery charging state is unknown. */
    BT_MESH_GEN_BATTERY_CHARGING_UNKNOWN,
};

enum bt_mesh_gen_battery_service {
    /* Reserved for future use. */
    BT_MESH_GEN_BATTERY_SERVICE_RCV,
    /** The battery does not require service. */
    BT_MESH_GEN_BATTERY_SERVICE_NOT_REQUIRED,
    /* The battery requires service. */
    BT_MESH_GEN_BATTERY_SERVICE_REQUIRED,
    /* The battery serviceability is unknown. */
    BT_MESH_GEN_BATTERY_SERVICE_UNKNOWN,
};

struct bt_mesh_gen_battery_state {
    uint32_t battery_level : 8,         /* The percentage of the charge level. value range: 0-100 */
             time_to_discharge : 24;    /* The remaining time (in minutes) of the discharging process */
    uint32_t time_to_charge : 24,       /* The remaining time (in minutes) of the charging process */
             battery_flags : 8;         /* 0–1 bits: Generic Battery Flags Presence, ref @bt_mesh_gen_battery_presence
                                         * 2–3 bits: Generic Battery Flags Indicator, ref @bt_mesh_gen_battery_indicator
                                         * 4–5 bits: Generic Battery Flags Charging, ref @bt_mesh_gen_battery_charging
                                         * 6–7 bits: Generic Battery Flags Serviceability, ref @bt_mesh_gen_battery_service */
};

struct bt_mesh_gen_battery_srv {
    struct bt_mesh_srv_callbacks *cb;
    const struct bt_mesh_model *model;
    struct bt_mesh_gen_battery_state state;
};

struct bt_mesh_gen_location_state {
    /* Global Coordinates (Latitude) */
    int32_t  global_latitude;
    /* Global Coordinates (Longitude) */
    int32_t  global_longitude;
    /* Global Altitude */
    int16_t  global_altitude;
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

struct bt_mesh_gen_location_srv {
    struct bt_mesh_srv_callbacks *cb;
    const struct bt_mesh_model *model;
    const struct bt_mesh_model *setup_model;
    struct bt_mesh_gen_location_state state;
    uint16_t pub_opcode;
};

struct bt_mesh_gen_property_state {
    uint16_t property_id;
    enum bt_mesh_generic_property_access access;
    struct net_buf_simple *val;
};

struct bt_mesh_gen_property_srv {
    struct bt_mesh_srv_callbacks *cb;
    const struct bt_mesh_model *model;
    uint16_t property_cnt;
    struct bt_mesh_gen_property_state *state;
};

struct bt_mesh_gen_user_property_srv {
    const struct bt_mesh_model *model;
    struct bt_mesh_gen_property_srv *admin_property;
    struct bt_mesh_gen_property_srv *mfr_property;
};

struct bt_mesh_gen_client_property_srv {
    const struct bt_mesh_model *model;
    uint16_t property_cnt;
    uint16_t *properties;
};

__INLINE uint16_t gen_level_to_gen_power_actual(int16_t level) {
    return level + 32768;
}

__INLINE int16_t gen_power_actual_to_gen_level(uint16_t actual) {
    return actual - 32768;
}

__INLINE int16_t gen_power_actual_to_gen_onoff(uint16_t actual) {
    return actual > 0 ? 1 : 0;
}

__INLINE struct bt_mesh_gen_property_state *gen_property_get(struct bt_mesh_gen_property_srv *srv, uint16_t property_id)
{
    if (srv == NULL || property_id == MESH_PROPERTY_ID_PROHIBITED)
        return NULL;

    for (int i = 0; i < srv->property_cnt; i++) {
        if (srv->state[i].property_id == property_id)
            return &srv->state[i];
    }

    return NULL;
}

void gen_onoff_config(struct bt_mesh_gen_onoff_srv *srv, uint8_t onoff);
void gen_onoff_status_publish(const struct bt_mesh_model *model);
void gen_level_config(struct bt_mesh_gen_level_srv *srv, int16_t level);
void gen_level_status_publish(const struct bt_mesh_model *model);
void gen_def_trans_time_config(struct bt_mesh_gen_def_trans_time_srv *srv, uint8_t transition_time);
void gen_def_trans_time_status_publish(const struct bt_mesh_model *model);
void gen_onpowerup_config(struct bt_mesh_gen_power_onoff_srv *srv, uint8_t onpowerup);
void gen_onpowerup_status_publish(const struct bt_mesh_model *model);
void gen_power_onoff_config(struct bt_mesh_gen_power_onoff_srv *srv, uint8_t onoff);
void gen_power_level_config(struct bt_mesh_gen_power_level_srv *srv, uint16_t power);
void gen_power_level_status_publish(const struct bt_mesh_model *model);
void gen_battery_status_publish(const struct bt_mesh_model *model);
void gen_location_config(struct bt_mesh_gen_location_srv *srv, struct bt_mesh_gen_location_state *state);
void gen_location_status_publish(const struct bt_mesh_model *model, uint16_t opcode);

#ifdef __cplusplus
}
#endif

#endif /* GENERIC_SERVER_H_ */
