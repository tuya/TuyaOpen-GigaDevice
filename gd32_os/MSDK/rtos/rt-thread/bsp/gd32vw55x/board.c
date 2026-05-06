/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-04-02     hqfang       first version
 *
 */

#include <rtthread.h>
#if 0//def RT_USING_DEVICE
//#include <rtdevice.h>
#endif
#include "wrapper_os_config.h"
#include "board.h"

#include "boot.h"

#define SOC_TIMER_FREQ                          OS_CPU_CLOCK_HZ

#define SYSTICK_TICK_CONST                      (SOC_TIMER_FREQ / RT_TICK_PER_SECOND)
#define RT_KERNEL_INTERRUPT_LEVEL               0


#ifdef RT_USING_SERIAL
#include <drv_usart.h>
#endif

#if 0
// Pointer to the start of the HEAP
#define HEAP_BEGIN      __heap_bottom
#define HEAP_END        __heap_top

#define configTOTAL_HEAP_SIZE     ((uint32_t)__heap_top - (uint32_t)__heap_bottom

/*
 * - Implemented and defined in Nuclei SDK system_<Device>.c file
 * - Required macro NUCLEI_BANNER set to 0
 */
extern void _init(void);
#endif
#if 0//def RT_USING_DEVICE
/*
 * - Check MCU pin assignment here https://doc.nucleisys.com/nuclei_board_labs/hw/hw.html
 * - If you changed menuconfig to use different peripherals such as SPI, ADC, GPIO,
 *   HWTIMER, I2C, PWM, UART, WDT, RTC, please add or change related pinmux configuration
 *   code in functions(rt_hw_*_drvinit) below
 */

void rt_hw_spi_drvinit(void)
{

}

void rt_hw_adc_drvinit(void)
{

}

void rt_hw_gpio_drvinit(void)
{
    // Clock on all the GPIOs and AF
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_AF);
}

void rt_hw_hwtimer_drvinit(void)
{

}

void rt_hw_i2c_drvinit(void)
{

}

void rt_hw_pwm_drvinit(void)
{

}

void rt_hw_rtc_drvinit(void)
{

}

void rt_hw_uart_drvinit(void)
{
    /* Notice: Debug UART4 GPIO pins are already initialized in nuclei_sdk */

}

void rt_hw_wdt_drvinit(void)
{

}

void rt_hw_drivers_init(void)
{
#ifdef RT_USING_PIN
    rt_hw_gpio_drvinit();
#endif
#ifdef BSP_USING_UART
    rt_hw_uart_drvinit();
#endif
#ifdef BSP_USING_SPI
    rt_hw_spi_drvinit();
#endif
#ifdef BSP_USING_I2C
    rt_hw_i2c_drvinit();
#endif
#ifdef BSP_USING_ADC
    rt_hw_adc_drvinit();
#endif
#ifdef BSP_USING_WDT
    rt_hw_wdt_drvinit();
#endif
#ifdef BSP_USING_RTC
    rt_hw_rtc_drvinit();
#endif
#ifdef BSP_USING_HWTIMER
    rt_hw_hwtimer_drvinit();
#endif
#ifdef BSP_USING_PWM
    rt_hw_pwm_drvinit();
#endif
}
#endif /* RT_USING_DEVICE */

rt_weak void rt_hw_ticksetup(void)
{
    uint64_t ticks = SYSTICK_TICK_CONST;

    __disable_irq();

    /* Make SWI and SysTick the lowest priority interrupts. */
    /* Stop and clear the SysTimer. SysTimer as Non-Vector Interrupt */
    SysTick_Config(ticks);
    ECLIC_DisableIRQ(CLIC_INT_TMR);
    ECLIC_SetLevelIRQ(CLIC_INT_TMR, 0);
    ECLIC_SetShvIRQ(CLIC_INT_TMR, ECLIC_NON_VECTOR_INTERRUPT);
    ECLIC_EnableIRQ(CLIC_INT_TMR);

    /* Set SWI interrupt level to lowest level/priority, SysTimerSW as Vector Interrupt */
    ECLIC_SetShvIRQ(CLIC_INT_SFT, ECLIC_VECTOR_INTERRUPT);
    ECLIC_SetLevelIRQ(CLIC_INT_SFT, 0);
    ECLIC_EnableIRQ(CLIC_INT_SFT);
}

#define SysTick_Handler     eclic_mtip_handler

/**
 * @brief This is the timer interrupt service routine.
 *
 */
void SysTick_Handler(void)
{
#ifdef RT_USING_PM
    systick_lock_release();
#endif

    /* Reload systimer */
    SysTick_Reload(SYSTICK_TICK_CONST);

    /* enter interrupt */
    rt_interrupt_enter();

    /* tick increase */
    rt_tick_increase();

    /* leave interrupt */
    rt_interrupt_leave();
}

/**
 * @brief Setup hardware board for rt-thread
 *
 */
void rt_hw_board_init(void)
{
    /* OS Tick Configuration */
    //rt_hw_ticksetup();

#ifdef RT_USING_HEAP
    //rt_system_heap_init((void *) HEAP_BEGIN, (void *) HEAP_END);
#endif
}

/******************** end of file *******************/
