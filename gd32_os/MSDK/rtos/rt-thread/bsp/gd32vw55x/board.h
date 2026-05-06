/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-04     BruceOu      first implementation
 */

#ifndef __BOARD__
#define __BOARD__
#include "gd32vw55x.h"
//#include "drv_gpio.h"
#include "rtconfig.h"

void rt_hw_board_init(void);

void vPortEnterCritical(void);
void vPortExitCritical(void);
uint32_t vPortInCritical(void);

void rt_hw_ticksetup(void);

#ifdef RT_USING_PM
void rt_system_lps_init(void);
#endif

#endif /* __BOARD__ */

/******************** end of file *******************/
