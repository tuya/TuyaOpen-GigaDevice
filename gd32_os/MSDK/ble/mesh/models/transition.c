/*!
    \file    transition.c
    \brief   Implementation of BLE mesh transition.

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
#include "src/access.h"

#include "models.h"
#include "model_utils.h"
#include "transition.h"
#include "generic_server.h"

/** Unknown encoded transition time value */
#define TRANSITION_UNKNOWN_VALUE            (0x3F)
#define TRANSITION_DELAY_TIME_STEP_MS       (5)
#define DEVICE_SPECIFIC_RESOLUTION          (10)

static const uint32_t transition_step_resolution[] = {
    100,
    1 * MSEC_PER_SEC,
    10 * MSEC_PER_SEC,
    60 * 10 * MSEC_PER_SEC,
};

static const uint32_t transition_limits[] = {
    6200,
    62 * MSEC_PER_SEC,
    620 * MSEC_PER_SEC,
    60 * 620 * MSEC_PER_SEC,
};

int32_t transition_time_decode(uint8_t transition)
{
    uint8_t steps = (transition & 0x3f);
    uint8_t step_resolution = (transition >> 6);

    return (steps == TRANSITION_UNKNOWN_VALUE) ? SYS_FOREVER_MS : (transition_step_resolution[step_resolution] * steps);
}

uint8_t transition_time_encode(int32_t transition_time)
{
    uint8_t i, steps;

    if (transition_time == SYS_FOREVER_MS) {
        return TRANSITION_UNKNOWN_VALUE;
    }

    if (transition_time == 0) {
        return 0;
    }

    for (i = 0; i < ARRAY_SIZE(transition_limits); ++i) {
        if (transition_time > transition_limits[i]) {
            continue;
        }

        steps = (transition_time + transition_step_resolution[i] / 2) / transition_step_resolution[i];
        return (i << 6) | steps;
    }

    return TRANSITION_UNKNOWN_VALUE;
}

uint8_t transition_delay_encode(uint32_t delay_time)
{
    return delay_time / TRANSITION_DELAY_TIME_STEP_MS;
}

uint32_t transition_delay_decode(uint8_t delay)
{
    return delay * TRANSITION_DELAY_TIME_STEP_MS;
}

void calculate_rt(struct bt_mesh_state_transition *transition)
{
    uint8_t steps, resolution;
    int32_t duration_remainder;
    int64_t now;

    if (transition->type == TRANSITION_TYPE_MOVE) {
        transition->remain_time = TRANSITION_UNKNOWN_VALUE;
        return;
    }

    if (transition->just_started) {
        transition->remain_time = transition->transition_time;
    } else {
        now = k_uptime_get();
        duration_remainder = transition->total_duration - (now - transition->start_timestamp);

        if (duration_remainder > 620000) {
            /* > 620 seconds -> resolution = 0b11 [10 minutes] */
            resolution = 0x03;
            steps = duration_remainder / 600000;
        } else if (duration_remainder > 62000) {
            /* > 62 seconds -> resolution = 0b10 [10 seconds] */
            resolution = 0x02;
            steps = duration_remainder / 10000;
        } else if (duration_remainder > 6200) {
            /* > 6.2 seconds -> resolution = 0b01 [1 seconds] */
            resolution = 0x01;
            steps = duration_remainder / 1000;
        } else if (duration_remainder > 0) {
            /* <= 6.2 seconds -> resolution = 0b00 [100 ms] */
            resolution = 0x00;
            steps = duration_remainder / 100;
        } else {
            resolution = 0x00;
            steps = 0x00;
        }

        transition->remain_time = (resolution << 6) | steps;
    }
}

bool set_transition_counter(struct bt_mesh_state_transition *transition)
{
    uint8_t steps_multiplier, resolution;

    resolution = (transition->transition_time >> 6);
    steps_multiplier = (transition->transition_time & 0x3F);
    if (steps_multiplier == 0U) {
        return false;
    }

    switch (resolution) {
    case 0: /* 100ms */
        transition->total_duration = steps_multiplier * 100U;
        break;
    case 1: /* 1 second */
        transition->total_duration = steps_multiplier * 1000U;
        break;
    case 2: /* 10 seconds */
        transition->total_duration = steps_multiplier * 10000U;
        break;
    case 3: /* 10 minutes */
        transition->total_duration = steps_multiplier * 600000U;
        break;
    }

    transition->counter = ((float) transition->total_duration / 100);

    if (transition->counter > DEVICE_SPECIFIC_RESOLUTION) {
        transition->counter = DEVICE_SPECIFIC_RESOLUTION;
    }

    return true;
}


void set_transition_values(struct bt_mesh_state_transition *transition)
{
    if (!set_transition_counter(transition)) {
        return;
    }

    if (transition->type == TRANSITION_TYPE_MOVE) {
        transition->quo_tt = transition->total_duration;
        return;
    }

    transition->quo_tt = transition->total_duration / transition->counter;
}

static uint8_t bt_mesh_get_def_trans_time(const struct bt_mesh_model *model)
{
    /* Refer 3.3.3.1 of Mesh Model Specification */
    uint16_t addr;
    const struct bt_mesh_elem *element = bt_mesh_model_elem(model);
    struct bt_mesh_gen_def_trans_time_srv *srv = NULL;
    const struct bt_mesh_model *def_trans_time_model = NULL;

    for (addr = element->rt->addr; addr >= bt_mesh_primary_addr(); addr--) {
        element = bt_mesh_elem_find(addr);
        if (element) {
            def_trans_time_model = bt_mesh_model_find(element, BT_MESH_MODEL_ID_GEN_DEF_TRANS_TIME_SRV);
            if (def_trans_time_model) {
                srv = (struct bt_mesh_gen_def_trans_time_srv *)(def_trans_time_model->rt->user_data);
                return srv->state.transition_time;
            }
        }
    }

    return 0;
}

void bt_mesh_srv_transition_get(const struct bt_mesh_model *model, struct bt_mesh_state_transition *transition,
    struct net_buf_simple *buf)
{
    struct net_buf_simple_state state;

    if (buf->len == 2) {
        net_buf_simple_save(buf, &state);
        transition->transition_time = net_buf_simple_pull_u8(buf);
        transition->delay = net_buf_simple_pull_u8(buf);
        net_buf_simple_restore(buf, &state);
        return;
    }

    transition->transition_time = bt_mesh_get_def_trans_time(model);
    transition->delay = 0;
}

void bt_mesh_server_stop_transition(struct bt_mesh_state_transition *transition)
{
    if (transition->child != NULL) {
        bt_mesh_server_stop_transition(transition->child);
        transition->child = NULL;
    }

    if (transition->counter != 0 || transition->delay != 0) {
        k_work_cancel_delayable(&transition->timer);
        memset(transition, 0x0, offsetof(struct bt_mesh_state_transition, start_timestamp));
    }
}

void bt_mesh_server_start_transition(struct bt_mesh_state_transition *transition)
{
    if (transition->counter == 0U && transition->delay == 0) {
        k_work_schedule(&transition->timer, K_NO_WAIT);
    } else if (transition->delay == 0) {
        transition->start_timestamp = k_uptime_get();
        k_work_schedule(&transition->timer, K_MSEC(transition->quo_tt));
    } else {
        transition->just_started = true;
        k_work_schedule(&transition->timer, K_MSEC(TRANSITION_DELAY_TIME_STEP_MS * transition->delay));
    }
}
