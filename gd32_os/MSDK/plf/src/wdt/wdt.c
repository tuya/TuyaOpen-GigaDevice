/*!
    \file    wdt.c
    \brief   watchdog for GD32VW55x SDK

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
#include "wdt.h"
#include "gd32vw55x.h"

uint8_t fwdgt_init(uint32_t timeout_ms)
{
    uint8_t prescaler_div = FWDGT_PSC_DIV256;
    uint16_t reload_value = 0;

    if (timeout_ms >= 0 && timeout_ms <= 511) {
        prescaler_div = FWDGT_PSC_DIV4;
        reload_value = timeout_ms << 3;
    } else if (timeout_ms >= 512 && timeout_ms <= 1023) {
        prescaler_div = FWDGT_PSC_DIV8;
        reload_value = timeout_ms << 2;
    } else if (timeout_ms >= 1024 && timeout_ms <= 2047) {
        prescaler_div = FWDGT_PSC_DIV16;
        reload_value = timeout_ms << 1;
    } else if (timeout_ms >= 2048 && timeout_ms <= 4095) {
        prescaler_div = FWDGT_PSC_DIV32;
        reload_value = timeout_ms;
    } else if (timeout_ms >= 4096 && timeout_ms <= 8190) {
        prescaler_div = FWDGT_PSC_DIV64;
        reload_value = timeout_ms >> 1;
    } else if (timeout_ms >= 8191 && timeout_ms <= 16380) {
        prescaler_div = FWDGT_PSC_DIV128;
        reload_value = timeout_ms >> 2;
    } else if (timeout_ms >= 16381 && timeout_ms <= 32760) {
        prescaler_div = FWDGT_PSC_DIV256;
        reload_value = timeout_ms >> 3;
    } else {
        prescaler_div = FWDGT_PSC_DIV256;
        reload_value = 0xFFF;
    }

    return fwdgt_config(reload_value, prescaler_div);
}

void fwdgt_start(void)
{
    fwdgt_enable();
}

void fwdgt_refresh(void)
{
    fwdgt_counter_reload();
}
