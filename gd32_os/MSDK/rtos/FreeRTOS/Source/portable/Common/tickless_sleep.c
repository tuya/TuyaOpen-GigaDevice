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

/* Shortest idle window worth an hxtal restart and a pll relock on the way back; anything
 * below falls through to the port's plain __WFI(). */
#define DEEP_SLEEP_FLOOR_MS     20

/* How early to set the alarm so the restart and relock land on the deadline rather than after
 * it. Measured against the 32k rtc: asking for 50 ms forty times ran 4.5 ms per iteration long,
 * 100 ms ran 3.2 ms long, and the offset stays flat as the period grows - a fixed exit cost,
 * not a rate error. One tick of margin did not come close to covering it. Wakeup count and
 * current are unchanged by this; only whether the task is a little early or a little late. */
#define DEEP_SLEEP_EXIT_MS      5

/* Wakelock id 6, taken by the tuyaos adapter for peripherals that need their clock to keep
 * running - a dma transfer still in flight, a pwm still driving its pin, a hardware timer still
 * counting. Unlike ids 0..4 it does not bar sleep, only the deep kind: shallow sleep stops the
 * core and leaves every peripheral clock alive, which is exactly what those want. */
#define PERI_CLOCK_LOCK_MSK     (0x01UL << 6)

/* Milliseconds slept past what vTaskStepTick() may be told about, replayed as pending ticks
 * so the tasks that came due meanwhile are released in order. Called from the idle hook
 * because xTaskCatchUpTicks() refuses to run with the scheduler suspended. */
extern BaseType_t xTaskCatchUpTicks(TickType_t xTicksToCatchUp);
static volatile uint32_t sleep_catchup_ticks = 0;

/* Whether the last pre-sleep call actually took the deep path. The post-sleep hook rewrites the
 * port's idle time to say "the tick has been stepped already, credit nothing", which is only
 * true when this file did the stepping. Now that pre-sleep can decline - window under the floor,
 * or a peripheral holding its clock - the two have to agree, or the shallow sleep that happens
 * instead goes entirely uncounted and the tick runs slow. */
static volatile int deep_sleep_taken = 0;

void freertos_sleep_catchup(void)
{
    uint32_t n;

    portENTER_CRITICAL();
    n = sleep_catchup_ticks;
    sleep_catchup_ticks = 0;
    portEXIT_CRITICAL();

    if (n) {
        xTaskCatchUpTicks((TickType_t)n);
    }
}
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
    uint64_t idle_ms;
    struct time_rtc time_before_sleep;
    struct time_rtc time_after_sleep;
    // The GCC compiler may optimize the follow variables
    volatile uint64_t passed_time;
    volatile uint64_t sys_timer_val, pass_timer_cnt;

    deep_sleep_taken = 0;

    if (sys_ps_get() == SYS_PS_DEEP_SLEEP
        && (sys_wakelock_status_get() & PERI_CLOCK_LOCK_MSK) == 0) {
        idle_ms = (*expected_idle_time) * portTICK_PERIOD_MS;

        if (idle_ms < DEEP_SLEEP_FLOOR_MS) {
            /* leave *expected_idle_time alone so the port falls back to its own __WFI() */
            return;
        }

        deep_sleep_taken = 1;

        /* Wake on the deadline the scheduler handed over, DEEP_SLEEP_EXIT_MS early so the
         * restart lands on it rather than after it. Sleeping past it would make every software
         * timer late by whatever was overslept, which no amount of tick accounting can give
         * back. An associated station normally takes a dtim beacon well before this and the rtc
         * alarm never fires. */
        if (idle_ms > DEEP_SLEEP_MAX_TIME_MS) {
            sleep_time = DEEP_SLEEP_MAX_TIME_MS;
        } else {
            sleep_time = (uint16_t)(idle_ms - DEEP_SLEEP_EXIT_MS);
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

        /* vTaskStepTick() asserts rather than step past xNextTaskUnblockTime, and sleeping
         * longer than granted is exactly how that happens. Step as far as it allows and bank
         * the rest; dropping it leaves the tick running slow by whatever was overslept. */
        if (passed_time > idle_ms) {
            sleep_catchup_ticks += (uint32_t)((passed_time - idle_ms) / portTICK_PERIOD_MS);
            passed_time = idle_ms;
        }

        vTaskStepTick(passed_time * portTICK_PERIOD_MS);
#if configGENERATE_RUN_TIME_STATS
        cpu_sleep_ms += passed_time;
#endif
    }
}

void freertos_post_sleep_processing(unsigned long long *expected_idle_time)
{
    /* Was sys_ps_get() == SYS_PS_DEEP_SLEEP, which is only a statement of intent. Asking whether
     * the deep path ran instead leaves the port free to count the shallow sleep it did on its
     * own - it steps xExpectedIdleTime - 1 ticks, and being told 1 here means it steps none. */
    if (deep_sleep_taken) {
        *expected_idle_time = 1;
    }
}

int freertos_ready_to_sleep(void)
{
    /* PERI_CLOCK_LOCK_MSK deliberately masked out - see its comment. Everything else still
     * means no sleep of any depth. */
    return (sys_wakelock_status_get() & ~PERI_CLOCK_LOCK_MSK) == 0 && wifi_hw_is_sleep();
}

#endif  /* ( configUSE_TICKLESS_IDLE != 0 ) */
