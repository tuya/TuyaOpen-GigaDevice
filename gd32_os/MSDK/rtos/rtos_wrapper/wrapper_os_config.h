/*!
    \file    wrapper_os_config.h
    \brief   Config header file for OS wrapper.

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

#ifndef __WRAPPER_OS_CONFIG_H
#define __WRAPPER_OS_CONFIG_H

/*============================ INCLUDES ======================================*/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// defined in project file
// #define PLATFORM_OS_UCOS
// #define PLATFORM_OS_FREERTOS
// #define PLATFORM_OS_AOS_RHINO
// #define PLATFORM_OS_RTTHREAD
// #define PLATFORM_OS_LITEOS
// #define PLATFORM_OS_MBEDOS
// #define PLATFORM_OS_THREADX

/* Use a guard to ensure the following few definitions are'nt included in
assembly files that include this header file. */
#if defined ( __CC_ARM ) || defined ( __ARMCC_VERSION ) || defined ( __ICCARM__ ) || defined ( __GNUC__ )
extern uint32_t SystemCoreClock;
#endif

#define OS_CPU_CLOCK_HZ                 SystemCoreClock
#define OS_TICK_RATE_HZ                 1000
#define OS_MS_PER_TICK                  (1000 / OS_TICK_RATE_HZ)

/* Priority: 0 ~ 31, a larger value indicates a higher priority.
 * The priority of idle task is 0. */
#define OS_TASK_PRIO_IDLE               0
#define OS_TASK_PRIO_MAX                32
#define OS_TASK_PRIO_APP_BASE           16
// Macro building a task priority as an offset of the IDLE task priority
#define OS_TASK_PRIORITY(prio)          (OS_TASK_PRIO_IDLE + OS_TASK_PRIO_APP_BASE + (prio))

// #define CFG_HEAP_MEM_CHECK

#ifdef __cplusplus
}
#endif

#endif // __WRAPPER_OS_CONFIG_H
