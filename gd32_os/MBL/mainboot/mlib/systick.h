/*!
    \file  systick.h
    \brief the header file of systick
*/

/*
    Copyright (C) 2016 GigaDevice

    2016-08-15, V1.0.0, firmware for GD32F4xx
*/

#ifndef SYS_TICK_H
#define SYS_TICK_H

#include "stdint.h"

void systick_init(void);
void sys_udelay(uint32_t nus);

#endif /* SYS_TICK_H */
