/*!
    \file    dsp.c
    \brief   DSP for GD32VW55x SDK.

    \version 2023-07-20, V1.0.0, firmware for GD32VW55x
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

#include "dsp.h"
#include "gd32vw55x.h"
#include "riscv_math.h"
#include "riscv_const_structs.h"
#include "riscv_common_tables.h"
#include "../util/include/util.h"

int32_t calc_iqk_comp(float phi, float gain)
{
    float sin, cos, temp;
    int32_t phase_comp, gain_comp;
    if (gain < 0.0001f)
        gain = 0.0001f;

    riscv_sin_cos_f32(phi, &sin, &cos);

    if (cos < 0.0001f)
        cos = 0.0001f;
    temp = (sin / cos) * BIT8;
    if (temp < 0)
        phase_comp = (int32_t)(temp - 0.5f);
    else
        phase_comp = (int32_t)(temp + 0.5f);

    temp = (gain) * BIT10;

    if (temp < 0)
        gain_comp = (int32_t)(temp - 0.5f);
    else
        gain_comp = (int32_t)(temp + 0.5f);

    return (phase_comp << 16) + (gain_comp);
}

int cfft_f32(uint32_t points, float * fp)
{
    const riscv_cfft_instance_f32 *S = NULL;

    // Check points and find FFT const struct.
    switch (points) {
        case 256:
            S = &riscv_cfft_sR_f32_len256;
            break;
        case 512:
            S = &riscv_cfft_sR_f32_len512;
            break;
        case 1024:
            S = &riscv_cfft_sR_f32_len1024;
            break;
        default:
            return -1;
    }

    // Square of ABS
    riscv_cfft_f32(S, fp, 1, 1);

    return 0;
}
