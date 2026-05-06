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

#ifndef TRANSITION_H_
#define TRANSITION_H_

#ifdef __cplusplus
extern "C" {
#endif

enum transition_types {
    TRANSITION_TYPE_MOVE = 0x01,
    TRANSITION_TYPE_NON_MOVE
};

struct bt_mesh_state_transition {
    struct bt_mesh_state_transition *child;
    bool     just_started;
    uint8_t  type;
    uint8_t  transition_time;
    uint8_t  remain_time;
    uint8_t  delay;
    uint32_t quo_tt;
    uint32_t counter;
    uint32_t total_duration;
    int64_t  start_timestamp;

    struct k_work_delayable timer;
};

uint8_t transition_time_encode(int32_t transition_time);
int32_t transition_time_decode(uint8_t transition);
uint8_t transition_delay_encode(uint32_t delay_time);
uint32_t transition_delay_decode(uint8_t delay);

void calculate_rt(struct bt_mesh_state_transition *transition);

bool set_transition_counter(struct bt_mesh_state_transition *transition);

void set_transition_values(struct bt_mesh_state_transition *transition);

void bt_mesh_srv_transition_get(const struct bt_mesh_model *model, struct bt_mesh_state_transition *transition,
    struct net_buf_simple *buf);

void bt_mesh_server_stop_transition(struct bt_mesh_state_transition *transition);

void bt_mesh_server_start_transition(struct bt_mesh_state_transition *transition);

#ifdef __cplusplus
}
#endif

#endif /* TRANSITION_H_ */
