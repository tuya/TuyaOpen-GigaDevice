/*!
    \file    drv_pm.c
    \brief   Tickless power management function for GD32VW55x SDK

    \version 2024-09-01, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2024, GigaDevice Semiconductor Inc.

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

/* Scheduler includes. */
#include <stdio.h>
#include "gd32vw55x.h"

#include <drivers/pm.h>
#include <stdlib.h>
#include "gd32vw55x_platform.h"
#include "wrapper_os.h"
#include "systime.h"

#ifdef RT_USING_PM

/* RISC-V TIMER is 64-bit long */
typedef uint64_t TickType_t;

#define DEEP_SLEEP_MIN_TIME_MS          2000
#define DEEP_SLEEP_MAX_TIME_MS          10000

#define configTICK_RATE_HZ              OS_TICK_RATE_HZ
#define portTICK_PERIOD_MS              ((TickType_t) 1000 / configTICK_RATE_HZ)
#define configCPU_CLOCK_HZ              OS_CPU_CLOCK_HZ

#define configSYSTICK_CLOCK_HZ          configCPU_CLOCK_HZ

#define SYSTICK_TICK_CONST              (configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ)

/* The systick is a 64-bit counter. */
#define portMAX_BIT_NUMBER              (SysTimer_MTIMER_Msk)

/* A fiddle factor to estimate the number of SysTick counts that would have
occurred while the SysTick counter is stopped during tickless idle
calculations. */
#define portMISSED_COUNTS_FACTOR        (45UL)

static TickType_t ulTimerCountsForOneTick = 0;

/*
 * The maximum number of tick periods that can be suppressed is limited by the
 * 24 bit resolution of the SysTick timer.
 */
TickType_t xMaximumPossibleSuppressedTicks = 0;

/*
 * Compensate for the CPU cycles that pass while the SysTick is stopped (low
 * power functionality only.
 */
static TickType_t ulStoppedTimerCompensation = 0;

uint16_t sleep_time;
uint16_t sleep_flag;
struct time_rtc time_before_sleep;
struct time_rtc time_after_sleep;

static uint32_t cpu_stats_start = 0;
static uint32_t cpu_sleep_ms = 0;

volatile TickType_t xExpectedIdleTime, xModifiableIdleTime, XLastLoadValue;

void rtthread_cpu_sleep_time_get(uint32_t *stats_ms, uint32_t *sleep_ms)
{
    if (sleep_ms)
        *sleep_ms = cpu_sleep_ms;
    if (stats_ms)
        *stats_ms = sys_current_time_get() - cpu_stats_start;

    cpu_sleep_ms = 0;
    cpu_stats_start = sys_current_time_get();
}
RTM_EXPORT(rtthread_cpu_sleep_time_get);

int rtthread_ready_to_sleep(void)
{
    return sys_wakelock_status_get() == 0 && wifi_hw_is_sleep();
}

static void pm_sleep(struct rt_pm *pm, uint8_t mode)
{
    if (!sleep_flag || (sys_ps_get() != SYS_PS_DEEP_SLEEP))
        return;

    rtc_32k_time_get(&time_before_sleep, 0);
//    dbg_print(INFO, "time_before_sleep sec %d msec %d\r\n\n", time_before_sleep.tv_sec, time_before_sleep.tv_msec);

    SysTimer_Stop();
    deep_sleep_enter(sleep_time);
    rtc_32k_time_get(&time_after_sleep, 1);

    /* set expected_idle_time to 0 to use our sleep function and bypass freertos wfi sleep */
    xModifiableIdleTime = 0;

    return;
}

static void pm_run(struct rt_pm *pm, rt_uint8_t mode)
{
    return;
}

static void pm_timer_start(struct rt_pm *pm, rt_uint32_t timeout_tick)
{
    uint32_t ulReloadValue, ulCompleteTickPeriods, ulCompletedSysTickDecrements;

    //printf("Enter TickLess %u\n", (uint32_t)xExpectedIdleTime);

    xExpectedIdleTime = timeout_tick;

    /* Make sure the SysTick reload value does not overflow the counter. */
    if (xExpectedIdleTime > xMaximumPossibleSuppressedTicks) {
        xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
    }

    /* Enter a critical section but don't use the taskENTER_CRITICAL()
    method as that will mask interrupts that should exit sleep mode. */
    __disable_irq();

    /* If a context switch is pending or a task is waiting for the scheduler
    to be unsuspended then abandon the low power entry. */
    if (!rtthread_ready_to_sleep()) {
        /* Restart from whatever is left in the count register to complete
        this tick period. */
        /* Restart SysTick. */
        //SysTimer_Start();

        // GD32VW55X, it will cause lost ticks too much as 1:4000 if reload tick here
        /* Reset the reload register to the value required for normal tick
           periods. */
        // SysTick_Reload(ulTimerCountsForOneTick);

        /* Re-enable interrupts - see comments above the cpsid instruction()
           above. */
        __enable_irq();
    } else {
        sleep_flag = 1;
        /* Stop the SysTick momentarily.  The time the SysTick is stopped for
        is accounted for as best it can be, but using the tickless mode will
        inevitably result in some tiny drift of the time maintained by the
        kernel with respect to calendar time. */
        SysTimer_Stop();

        /* Calculate the reload value required to wait xExpectedIdleTime
        tick periods.  -1 is used because this code will execute part way
        through one of the tick periods. */
        ulReloadValue = (ulTimerCountsForOneTick * (xExpectedIdleTime - 1UL));
        if (ulReloadValue > ulStoppedTimerCompensation) {
            ulReloadValue -= ulStoppedTimerCompensation;
        }

        // xTickCountBeforeSleep = xTaskGetTickCount();

        /* Set the new reload value. */
        SysTick_Reload(ulReloadValue);

        /* Get System timer load value before sleep */
        XLastLoadValue = SysTimer_GetLoadValue();

        /* Restart SysTick. */
        SysTimer_Start();
        ECLIC_EnableIRQ(CLIC_INT_TMR);
        __RWMB();

        /* Sleep until something happens.  configPRE_SLEEP_PROCESSING() can
        set its parameter to 0 to indicate that its implementation contains
        its own wait for interrupt or wait for event instruction, and so wfi
        should not be executed again.  However, the original expected idle
        time variable must remain unmodified, so a copy is taken. */
        xModifiableIdleTime = xExpectedIdleTime;
        if (sys_ps_get() == SYS_PS_DEEP_SLEEP) {
            if (xModifiableIdleTime < xMaximumPossibleSuppressedTicks) {
                sleep_time = DEEP_SLEEP_MIN_TIME_MS;
            } else {
                sleep_time = DEEP_SLEEP_MAX_TIME_MS;
            }
        }
    }

    return;
}

static void pm_timer_stop(struct rt_pm *pm)
{
    return;
}

static rt_tick_t pm_timer_get_tick(struct rt_pm *pm)
{
    uint32_t ulReloadValue, ulCompleteTickPeriods, ulCompletedSysTickDecrements;

        // The GCC compiler may optimize the follow variables
    volatile uint64_t passed_time;
    volatile uint64_t sys_timer_val, pass_timer_cnt;

    if (!sleep_flag)
        return 0;

    sleep_flag = 0;

    if (sys_ps_get() == SYS_PS_DEEP_SLEEP) {
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
//        dbg_print(NOTICE, "time_after_sleep sec %d msec %d\r\n", time_after_sleep.tv_sec, time_after_sleep.tv_msec);
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

        rt_tick_set(rt_tick_get() + passed_time * portTICK_PERIOD_MS);

        cpu_sleep_ms += passed_time;
    }

    if (xModifiableIdleTime > 0) {
        __WFI();
    } else {
        // GD32VW55X
        /* Get System timer load value if CPU enter deep sleep once */
        XLastLoadValue = SysTimer_GetLoadValue();
    }
    //configPOST_SLEEP_PROCESSING(xExpectedIdleTime);
    if (sys_ps_get() == SYS_PS_DEEP_SLEEP) {
        xExpectedIdleTime = 1;
    }

    // GD32VW55X, it do not need do follow step if CPU enter deep sleep once
    if (xModifiableIdleTime > 0) {
        /* Re-enable interrupts to allow the interrupt that brought the MCU
        out of sleep mode to execute immediately. */
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
    }

    /* Disable the SysTick clock.  Again,
       the time the SysTick is stopped for is accounted for as best it can
       be, but using the tickless mode will inevitably result in some tiny
       drift of the time maintained by the kernel with respect to calendar
       time*/
    ECLIC_DisableIRQ(CLIC_INT_TMR);

    /* Determine if SysTimer Interrupt is not yet happened,
    (in which case an interrupt other than the SysTick
    must have brought the system out of sleep mode). */
    if (SysTimer_GetLoadValue() >= (XLastLoadValue + ulReloadValue)) {
        /* As the pending tick will be processed as soon as this
        function exits, the tick value maintained by the tick is stepped
        forward by one less than the time spent waiting. */
        ulCompleteTickPeriods = xExpectedIdleTime - 1UL;
        //printf("TickLess - SysTimer Interrupt Entered!\n");
    } else {
        /* Something other than the tick interrupt ended the sleep.
        Work out how long the sleep lasted rounded to complete tick
        periods (not the ulReload value which accounted for part
        ticks). */
        xModifiableIdleTime = SysTimer_GetLoadValue();
        if (xModifiableIdleTime > XLastLoadValue) {
            ulCompletedSysTickDecrements = (xModifiableIdleTime - XLastLoadValue);
        } else {
            ulCompletedSysTickDecrements = (xModifiableIdleTime + portMAX_BIT_NUMBER - XLastLoadValue);
            //ulCompletedSysTickDecrements = (xModifiableIdleTime + 0xFFFFFFFF - XLastLoadValue);
        }

        /* How many complete tick periods passed while the processor
        was waiting? */
        ulCompleteTickPeriods = ulCompletedSysTickDecrements / ulTimerCountsForOneTick;

        /* The reload value is set to whatever fraction of a single tick
        period remains. */
        SysTick_Reload(ulTimerCountsForOneTick);
        //printf("TickLess - External Interrupt Happened!\n");
    }

    //printf("End TickLess %d\n", (uint32_t)ulCompleteTickPeriods);

    /* Restart SysTick */
    rt_tick_set(rt_tick_get() + ulCompleteTickPeriods);

    /* Exit with interrupts enabled. */
    ECLIC_EnableIRQ(CLIC_INT_TMR);
    systick_lock_acquire();
    __enable_irq();

    return ulCompleteTickPeriods;

}

void rt_system_lps_init(void)
{
    rt_uint8_t timer_mask = 0;
    static const struct rt_pm_ops pm_ops =
    {
        pm_sleep,
        pm_run,
        pm_timer_start,
        pm_timer_stop,
        pm_timer_get_tick,
    };

    ulTimerCountsForOneTick = (SYSTICK_TICK_CONST);
    xMaximumPossibleSuppressedTicks = 0xFFFFFFFFUL / ulTimerCountsForOneTick;
    ulStoppedTimerCompensation = portMISSED_COUNTS_FACTOR / (configCPU_CLOCK_HZ / configSYSTICK_CLOCK_HZ);
    timer_mask = (1UL << PM_SLEEP_MODE_DEEP) | (1UL << PM_SLEEP_MODE_NONE);
    rt_system_pm_init(&pm_ops, timer_mask, NULL);
}
RTM_EXPORT(rt_system_lps_init);

#endif /* RT_USING_PM */