/*!
    \file    systime.c
    \brief   Provide time related function specific for GD32VW55x SDK.

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

#include "systime.h"
#include "gd32vw55x.h"

// Number of seconds elapsed since EPOCH when firmware has been initialized
static uint32_t epoch_sec;
// Number of microseconds (modulo 1 sec) elapsed since EPOCH at firmware initialization
static uint32_t epoch_usec;

uint32_t clock_us_factor;

void systick_init(void)
{
    clock_us_factor = SystemCoreClock / 1000000;
}

void time_init(uint32_t sec, uint32_t usec)
{
    epoch_sec = sec;
    epoch_usec = usec;
}

uint64_t get_sys_local_time_ms()
{
    uint64_t tmp = get_sys_local_time_us();

    return tmp/1000;
}

/**
 ****************************************************************************************
 * @brief Read monotonic counter
 *
 * Read msb twice to detect if counter wrap while reading it.
 *
 * @msb Updated with msb part of the counter (16bits only)
 * @lsb Updated with lsb part of the counter (32bits)
 *
 * @return 64 bit us
 ****************************************************************************************
 */
uint64_t get_sys_local_time_us(void)
{
    // for GD32VW55X we use riscv mtime
    /* get current timer value */
    uint64_t tmp = SysTimer_GetLoadValue();

    return (uint64_t)(tmp / clock_us_factor);
}

int get_time(enum time_origin_t origin, uint32_t *sec, uint32_t *usec)
{
    uint32_t msb, lsb, _sec, _usec, tmp, fact;
    uint64_t ts;

    if (origin > SINCE_EPOCH)
        return -1;

    ts = get_sys_local_time_us();
    msb = (ts >> 32) & 0xffffffff;
    lsb = ts & 0xffffffff;

    /* Replace uint64_t / 1000000 (uint48_t in practice)
       by 3 ~fmul (mul + shift).
       Yes this is more complicated but also 20 times faster on cpu without div
       instruction.
       48bits time value is divided in 4 parts 0xAABBCCCCDDDD
       First two parts (A ,B) are only 8bits to allow more bits in the factor
       (merging this two parts with a 16 bits factor doesn't provide enough precision).
       For the third part (C) a factor on 16bits is enough so part is also 16bits.
       Fourth part (D) is always < 1000000 so no need to divide it.
       factor = 0x8637bd = 1 / 1000000 in Q43 = ((1<<43) / 1000000) (truncated on 24bits)
     */
    fact = 0x8637bd;
    tmp = (msb >> 8) * fact;
    _sec = (tmp >> 3);
    tmp = (msb & 0xff) * fact;
    _sec += (tmp >> 11);
    fact >>= 8;
    tmp = (lsb >> 16) * fact;
    _sec += (tmp >> 19);
    _usec = lsb - (_sec * 1000000);

    if (origin == SINCE_EPOCH)
    {
        _sec += epoch_sec;
        _usec += epoch_usec;

    }

    /* Previous computation ensure that loop won't run more than 2 times */
    while (_usec > 1000000)
    {
        _usec -= 1000000;
        _sec ++;
    }

    *sec = _sec;
    *usec = _usec;
    return 0;
}

int get_time_us(enum time_origin_t origin, uint64_t *usec)
{
    uint64_t val;

    if (origin > SINCE_EPOCH)
        return -1;

    val = get_sys_local_time_us();

    if (origin == SINCE_EPOCH)
    {
        val += (uint64_t)epoch_usec;
        val += (uint64_t)(epoch_sec * 1000000);
    }

    *usec = val;
    return 0;
}

/**
 ****************************************************************************************
 * @brief delay a time in microseconds
 *
 * @param[in]  nus: count in microseconds
 ****************************************************************************************
 */
void systick_udelay(uint32_t nus)
{
    uint64_t start_mtime, delta_mtime;
    uint64_t tmp = SysTimer_GetLoadValue(); /* get current timer value */

    do {
        start_mtime = SysTimer_GetLoadValue();
    } while (start_mtime == tmp);

    tmp = clock_us_factor * nus;

    /* continue counting until the delay time is reached */
    do {
        delta_mtime = SysTimer_GetLoadValue() - start_mtime;
    } while (delta_mtime < tmp);
}

