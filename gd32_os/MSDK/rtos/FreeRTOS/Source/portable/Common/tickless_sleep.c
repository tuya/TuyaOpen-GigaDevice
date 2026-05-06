/*!
    \file    tickless_sleep.c
    \brief   Tickless sleep function for GD32VW55x SDK

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
#include "app_cfg.h"

#include <stdint.h>
#include "dbg_print.h"
#include "wakelock.h"
#include "tickless_sleep.h"
#include "gd32vw55x_platform.h"
#include "systime.h"
#include "gd32vw55x.h"
#include "wrapper_os.h"
#include "FreeRTOS.h"

#ifdef CFG_WLAN_SUPPORT
#include "wlan_config.h"
#include "wifi_export.h"
#endif

#if ( configUSE_TICKLESS_IDLE != 0 )
#if configGENERATE_RUN_TIME_STATS
static uint32_t cpu_stats_start = 0;
static uint32_t cpu_sleep_ms = 0;
#endif
extern uint64_t xMaximumPossibleSuppressedTicks;
/* maybe need wrap it as sys_task_step_tick later */
extern void vTaskStepTick(const uint64_t xTicksToJump);

void freertos_cpu_sleep_time_get(uint32_t *stats_ms, uint32_t *sleep_ms)
{
#if configGENERATE_RUN_TIME_STATS
    if (sleep_ms)
        *sleep_ms = cpu_sleep_ms;
    if (stats_ms)
        *stats_ms = sys_current_time_get() - cpu_stats_start;

    cpu_sleep_ms = 0;
    cpu_stats_start = sys_current_time_get();
#else
    *stats_ms = 0;
    *sleep_ms = 0;

#endif
}

void freertos_pre_sleep_processing(unsigned long long *expected_idle_time)
{
    uint16_t sleep_time;
    struct time_rtc time_before_sleep;
    struct time_rtc time_after_sleep;
    // The GCC compiler may optimize the follow variables
    volatile uint64_t passed_time;
    volatile uint64_t sys_timer_val, pass_timer_cnt;

    if (sys_ps_get() == SYS_PS_DEEP_SLEEP) {
        if (*expected_idle_time < xMaximumPossibleSuppressedTicks) {
            sleep_time = DEEP_SLEEP_MIN_TIME_MS;
        } else {
            sleep_time = DEEP_SLEEP_MAX_TIME_MS;
        }

        rtc_32k_time_get(&time_before_sleep, 0);
        //dbg_print(INFO, "time_before_sleep sec %d msec %d\r\n\n", time_before_sleep.tv_sec, time_before_sleep.tv_msec);

        SysTimer_Stop();
        deep_sleep_enter(sleep_time);

        /* set expected_idle_time to 0 to use our sleep function and bypass freertos wfi sleep */
        *expected_idle_time = 0;

        /* CPU wake up by interrupt and run in irc16M clock, so we need enable interrupt to change clock
        by func system_clock_config_nspe() in NSPE\WIFI_IOT\bsp\bsp_wlan.c */
        __enable_irq();

        /* Make sure interrupt enable is executed */
        __RWMB();
        __FENCE_I();
        __NOP();

        /* Disable interrupts again because the clock is about to be stopped
           and interrupts that execute while the clock is stopped will increase
           any slippage between the time maintained by the RTOS and calendar
           time. */
        __disable_irq();

        SysTimer_Start();
        rtc_32k_time_get(&time_after_sleep, 1);
        //dbg_print(INFO, "time_after_sleep sec %d msec %d\r\n", time_after_sleep.tv_sec, time_after_sleep.tv_msec);
        if (time_after_sleep.tv_sec >= time_before_sleep.tv_sec) {
            passed_time = (time_after_sleep.tv_sec * 1000 + time_after_sleep.tv_msec) -
                    (time_before_sleep.tv_sec * 1000 + time_before_sleep.tv_msec);
        } else {
            passed_time = ((time_after_sleep.tv_sec + 60) * 1000 + time_after_sleep.tv_msec) -
                    (time_before_sleep.tv_sec * 1000 + time_before_sleep.tv_msec);
        }
        pass_timer_cnt = passed_time * clock_us_factor * 1000;

        /* compensate the systimer clock */
        SysTimer_Stop();
        sys_timer_val = SysTimer_GetLoadValue();
        sys_timer_val += pass_timer_cnt;
        SysTimer_SetLoadValue(sys_timer_val);
        SysTimer_Start();

        //dbg_print(INFO, "wakeup: sleep time = %d passed_time = %d\r\n", sleep_time, passed_time);

        vTaskStepTick(passed_time * portTICK_PERIOD_MS);
#if configGENERATE_RUN_TIME_STATS
        cpu_sleep_ms += passed_time;
#endif
    }
}

void freertos_post_sleep_processing(unsigned long long *expected_idle_time)
{
    if (sys_ps_get() == SYS_PS_DEEP_SLEEP) {
        *expected_idle_time = 1;
    }
}

int freertos_ready_to_sleep(void)
{
    return sys_wakelock_status_get() == 0 && wifi_hw_is_sleep();
}

#endif  /* ( configUSE_TICKLESS_IDLE != 0 ) */
