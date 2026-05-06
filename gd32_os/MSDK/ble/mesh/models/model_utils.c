/*!
    \file    model_utils.c
    \brief   Implementation of BLE mesh model utils.

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

#include "model_utils.h"

int bt_mesh_tid_check_and_update(struct bt_mesh_pre_tid *pre, uint8_t tid, uint16_t src, uint16_t dst)
{
    if (pre->src == src && pre->dst == dst && pre->tid == tid && k_uptime_delta(&pre->timestamp) < 6000) {
        return -EALREADY;
    }

    pre->src = src;
    pre->dst = dst;
    pre->tid = tid;

    return 0;
}

uint16_t bt_mesh_sqrt32(uint32_t val)
{
    uint32_t l = 0, r = val, mid;
    uint64_t tmp;

    if (val < 2)
        return val;

    while (l <= r) {
        mid = (l + r) >> 1;
        tmp = mid * mid;
        if (tmp == val)
            return mid;
        else if (tmp < val)
            l = mid + 1;
        else
            r = mid - 1;
    }

    return (uint16_t)r;
}

