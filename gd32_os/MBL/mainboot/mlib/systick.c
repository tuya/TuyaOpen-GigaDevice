/*!
    \file  systick.c
    \brief the systick configuration file
*/

/*
    Copyright (C) 2016 GigaDevice

    2016-08-15, V1.0.0, firmware for GD32F4xx
*/

#include "gd32vw55x.h"
#include "systick.h"

#define TICK_RATE_HZ        ( ( uint32_t ) 1000 )

static uint32_t fac_us = 0;

uint32_t clock_us_factor;

void systick_init(void)
{
    clock_us_factor = SystemCoreClock / 1000000;
}

void sys_udelay(uint32_t nus)
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
