/*!
    \file    gd32vw55x_it.c
    \brief   interrupt service routines

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

#include "gd32vw55x_it.h"
#include "gd32vw55x.h"
#include "uart.h"
#include "wakelock.h"
#ifdef CFG_BLE_SUPPORT
#include "ble_export.h"
#if FEAT_SUPPORT_BLE_DATATRANS
#include "app_datatrans_srv.h"
#endif
#endif
#include "dbg_print.h"
#include "trace_uart.h"
#include "gd32vw55x_platform.h"
#include "wrapper_os.h"

#ifdef CFG_WLAN_SUPPORT
#include "wlan_config.h"
#include "wifi_export.h"
#endif

#ifdef CFG_BLE_HCI_MODE
#include "ble_uart.h"
#endif

#ifdef TUYAOS_SUPPORT
#include "tkl_i2c.h"
#include "tkl_pwm.h"
#include "tkl_gpio.h"
#include "tkl_timer.h"
#endif

#if defined CONFIG_ATCMD
#include "atcmd.h"
#endif

/*!
    \brief      this function handles USART0 exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void USART0_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    uart_irq_hdl(USART0);

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}

/*!
    \brief      this function handles USART1 exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void UART1_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    uart_irq_hdl(UART1);

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}

/*!
    \brief      this function handles USART1 exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void UART2_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    uart_irq_hdl(UART2);

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}

#ifdef HCI_UART_RX_DMA
void DMA_Channel5_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    hci_uart_dma_channel5_irq_hdl();

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}
#endif

#if defined(CONFIG_ATCMD) && defined(CONFIG_ATCMD_SPI)
void SPI_IRQHandler(void)
{
    sys_int_enter();
    sys_wakelock_acquire(LOCK_ID_SPI);
    at_spi_rx_irq_hdl();
    sys_int_exit();
}
void DMA_Channel3_IRQHandler(void)
{
    sys_int_enter();
    at_spi_tx_dma_irq_hdl(DMA_CH3);
    sys_int_exit();
}
#endif

#ifdef TRACE_UART_DMA
#ifdef CONFIG_PLATFORM_ASIC
#ifdef CFG_BLE_HCI_MODE
void DMA_Channel6_IRQHandler(void)
{
    trace_uart_dma_channel_irq_hdl();
}
#else
void DMA_Channel7_IRQHandler(void)
{
    trace_uart_dma_channel_irq_hdl();
}
#endif      // CFG_BLE_HCI_MODE end
#else
void DMA_Channel1_IRQHandler(void)
{
    trace_uart_dma_channel_irq_hdl();
}
#endif     // CONFIG_PLATFORM_ASIC end
#endif     // TRACE_UART_DMA end

#if defined CONFIG_ATCMD && defined HCI_UART_RX_DMA
#error "THE ATCMD AND HCI_UART_RX_DMA SHOULD NOT USE SAME UART PORT AT THE SAME TIME"
#elif defined CONFIG_ATCMD && !defined HCI_UART_RX_DMA

#ifdef CONFIG_ATCMD_SPI
void DMA_Channel2_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    at_spi_rx_dma_irq_hdl(DMA_CH2);

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}

#elif (AT_UART_INDEX == UART0_INDEX)
void DMA_Channel2_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    at_uart_rx_dma_irq_hdl(DMA_CH2);

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}
#elif (AT_UART_INDEX == UART1_INDEX)
void DMA_Channel0_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    at_uart_rx_dma_irq_hdl(DMA_CH0);

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}
#elif (AT_UART_INDEX == UART2_INDEX)
void DMA_Channel5_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    at_uart_rx_dma_irq_hdl(DMA_CH5);

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}
#endif //(CONFIG_ATCMD_SPI)
#endif //defined CONFIG_ATCMD && defined HCI_UART_RX_DMA

#if FEAT_SUPPORT_BLE_DATATRANS && (BLE_DATATRANS_MODE == PURE_DATA_TRANSMIT_MODE)
#if (LOG_UART_INDEX == UART0_INDEX)
void DMA_Channel2_IRQHandler(void)
{
    app_datatrans_uart_rx_dma_irq_hdl(DMA_CH2);
}
#elif (LOG_UART_INDEX == UART1_INDEX)
void DMA_Channel0_IRQHandler(void)
{
    app_datatrans_uart_rx_dma_irq_hdl(DMA_CH0);
}
#elif (LOG_UART_INDEX == UART2_INDEX)
void DMA_Channel5_IRQHandler(void)
{
    app_datatrans_uart_rx_dma_irq_hdl(DMA_CH5);
}
#endif //(LOG_UART_INDEX == UART0_INDEX)
#endif //FEAT_SUPPORT_BLE_DATATRANS && (BLE_DATATRANS_MODE == PURE_DATA_TRANSMIT_MODE)

void RTC_WKUP_IRQHandler(void)
{
    deep_sleep_exit();
}

/*!
    \brief      this function handles EXTI5_9 exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void EXTI5_9_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    deep_sleep_exit();

#ifdef LOG_UART
    dbg_print(NOTICE, "WAKEUP For Console, Input Any Command or Press 'Enter' Key to Deep Sleep\r\n#\r\n");
    usart_command_enable(LOG_UART,USART_CMD_RXFCMD);
    sys_wakelock_acquire(LOCK_ID_USART);
#endif
#ifdef TUYAOS_SUPPORT
    gpio_irq_hdl(5);
#endif
    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */

}

#ifdef CFG_WLAN_SUPPORT
#ifdef CFG_LPS
void WIFI_WKUP_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    wlan_exti_exit();
    deep_sleep_exit();
    wifi_wakeup_isr();
    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}

#elif defined (CFG_PS_HW_WAKE)
void WIFI_WKUP_IRQHandler(void)
{
    wlan_exti_exit();
    // HW is idle wake up from sleep,
    // We must set HW to active early to receive bcn
    wifi_wakeup(1);
    dbg_print(DEBUG, "ex\n");
}
#endif

void WIFI_INT_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    intc_irq();

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}

void WIFI_INTGEN_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    //wake up wifi moudle if sleep
    wifi_wakeup(1);
#ifdef CFG_LPS
    if (!wifi_in_doze())
#endif
    {
        hal_machw_gen_handler();
#ifdef CFG_RTOS
        wifi_core_task_resume(true);
#endif
    }

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}

void WIFI_PROT_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    txl_prot_trigger();
#ifdef CFG_RTOS
    wifi_core_task_resume(true);
#endif

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}

void LA_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    hal_la_isr();
#ifdef CFG_RTOS
    wifi_core_task_resume(true);
#endif

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}

void WIFI_RX_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    rxl_mpdu_isr();
#ifdef CFG_RTOS
    wifi_core_task_resume(true);
#endif

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}

void WIFI_TX_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    txl_transmit_trigger();
#ifdef CFG_RTOS
    wifi_core_task_resume(true);
#endif

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}
#endif // CFG_WLAN_SUPPORT

#ifdef CFG_BLE_SUPPORT
void BLE_POWER_STATUS_IRQHandler(void)
{
    /* ble core goes from active to sleep and ble_ps_fall_en will generate power status interrupt */
    if (ble_power_status_fall_status() != 0) {
        ble_power_status_fall_clear();

        /* power off */
        ble_pmu_config(0);

        /* configuring ble exti protection a power status rise interrupt was generated when configuring CPU deepsleep but no ble exti interrupt */
        ble_exti_enter();

        /* release the ble lock of cpu deepsleep */
        ble_wakelock_release();
    }

    /* ble core goes from sleep to active and ble_ps_rise_en will generate power status interrupt */
    if (ble_power_status_rise_status() != 0) {
        ble_power_status_rise_clear();

        /* power on */
        ble_pmu_config(1);

        /* ble pmu off, the modem is not saved and needs to be reconfigured */
        ble_modem_config();

        /* configuring ble exti exit */
        ble_exti_exit();

        /* acquire the ble lock of cpu deepsleep */
        ble_wakelock_acquire();
    }
}

void BLE_WKUP_IRQHandler(void)
{
    /* ble module clear exti by self */
    ble_exti_exit();
    deep_sleep_exit();
}

void BLE_HALF_SLOT_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    ble_hslot_isr();
#ifdef CFG_RTOS
    ble_stack_task_resume(true);
#endif

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}

void BLE_SLEEP_MODE_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    ble_slp_isr();
#ifdef CFG_RTOS
    ble_stack_task_resume(true);
#endif

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}

void BLE_ENCRYPTION_ENGINE_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    ble_crypt_isr();
#ifdef CFG_RTOS
    ble_stack_task_resume(true);
#endif

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}

void BLE_SW_TRIG_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    ble_sw_isr();
#ifdef CFG_RTOS
    ble_stack_task_resume(true);
#endif

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}

void BLE_FINE_TIMER_TARGET_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    ble_fine_tgt_isr();

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}

void BLE_STAMP_TARGET1_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    ble_ts_tgt1_isr();
#ifdef CFG_RTOS
    ble_stack_task_resume(true);
#endif

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}

void BLE_STAMP_TARGET2_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    ble_ts_tgt2_isr();
#ifdef CFG_RTOS
    ble_stack_task_resume(true);
#endif

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}

void BLE_STAMP_TARGET3_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    ble_ts_tgt3_isr();
#ifdef CFG_RTOS
    ble_stack_task_resume(true);
#endif

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}

void BLE_FREQ_SELECT_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    ble_hop_isr();
#ifdef CFG_RTOS
    ble_stack_task_resume(true);
#endif

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}

void BLE_ERROR_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    ble_error_isr();
#ifdef CFG_RTOS
    ble_stack_task_resume(true);
#endif

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}

void BLE_FIFO_ACTIVITY_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */

    ble_fifo_isr();
#ifdef CFG_RTOS
    ble_stack_task_resume(true);
#endif

    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}
#endif // CFG_BLE_SUPPORT

#ifdef TUYAOS_SUPPORT
void I2C0_EV_IRQHandler(void)
{
    i2c_irq_hdl(I2C0);
}

void I2C0_ER_IRQHandler(void)
{
    i2c_irq_hdl(I2C0);
}

void I2C1_EV_IRQHandler(void)
{
    i2c_irq_hdl(I2C1);
}

void I2C1_ER_IRQHandler(void)
{
    i2c_irq_hdl(I2C1);
}

void TIMER0_Channel_IRQHandler(void)
{
    pwm_cap_irq_hdl(TIMER0);
}

void TIMER15_IRQHandler(void)
{
    pwm_cap_irq_hdl(TIMER15);
}

void TIMER16_IRQHandler(void)
{
    pwm_cap_irq_hdl(TIMER16);
}

void TIMER1_IRQHandler(void)
{
    timer_irq_hdl(TIMER1);
}

void TIMER2_IRQHandler(void)
{
    timer_irq_hdl(TIMER2);
}

void EXTI0_IRQHandler(void)
{
    gpio_irq_hdl(0);
}

void EXTI1_IRQHandler(void)
{
    gpio_irq_hdl(1);
}

void EXTI2_IRQHandler(void)
{
    gpio_irq_hdl(2);
}

void EXTI3_IRQHandler(void)
{
    gpio_irq_hdl(3);
}

void EXTI4_IRQHandler(void)
{
    gpio_irq_hdl(4);
}

void EXTI10_15_IRQHandler(void)
{
    gpio_irq_hdl(6);
}
#endif

#if (SPI_ROLE == SPI_ROLE_MASTER)
#define HANDSHAKE_GPIO                          GPIOA
#define HANDSHAKE_PIN                           GPIO_PIN_12

#define NSS_GPIO                                GPIOA
#define NSS_PIN                                 GPIO_PIN_4

#define STABLE_COUNT                             4

extern os_sema_t spi_slave_ready_sema;
extern os_sema_t spi_slave_notify_data_sema;

volatile uint32_t false_notify_cnt = 0;
void EXTI10_15_IRQHandler(void)
{
    volatile uint8_t detected_cnt = 0;

    if (RESET == exti_interrupt_flag_get(EXTI_12))
        return;

    for (int i = 0; i < STABLE_COUNT; i++) {
        if (gpio_input_bit_get(HANDSHAKE_GPIO, HANDSHAKE_PIN) == SET) {
            detected_cnt++;
        } else {
            break;
        }
    }
    if (detected_cnt >= STABLE_COUNT) {
        if (spi_hw_is_idle() &&
                gpio_input_bit_get(NSS_GPIO, NSS_PIN) == SET) {
            sys_sema_up_from_isr(&spi_slave_notify_data_sema);
        } else if (spi_hw_is_in_atack() && (spi_flag_get(SPI_FLAG_TRANS) == RESET)) {
//        } else if (spi_hw_is_in_atack() && (gpio_input_bit_get(NSS_GPIO, NSS_PIN) == SET)) {
            sys_sema_up_from_isr(&spi_slave_ready_sema);
        } else {
            false_notify_cnt++;
        }
    } else {
        //printf("false trigger, s:%d, d:%d, n:%d\r\n",
        //        spi_hw_is_idle(), detected_cnt, false_notify_cnt);
    }

    exti_interrupt_flag_clear(EXTI_12);

}

#endif
