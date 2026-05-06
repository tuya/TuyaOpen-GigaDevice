/*!
    \file    uart.c
    \brief   UART BSP for GD32VW55x SDK.

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

#include <ctype.h>
#include "gd32vw55x.h"
#include <stdio.h>
#include "app_cfg.h"
#include "uart.h"
#include "wrapper_os.h"
#include "wakelock.h"

struct uart_driver
{
    uart_cb_item_t uart_cbs[MAX_UART_NUM];
};

static struct uart_driver uart_mgr;

void uart_driver_init(void)
{
    memset(&uart_mgr, 0, sizeof(struct uart_driver));
}

bool uart_irq_callback_register(uint32_t uart_port, uart_rx_irq_hdl_t callback)
{
    uint8_t i;

    for (i = 0; i < MAX_UART_NUM; i++) {
        // uart port already register callback
        if (uart_mgr.uart_cbs[i].uart_port == uart_port) {
            return false;
        } else if (uart_mgr.uart_cbs[i].uart_port == 0) {
            uart_mgr.uart_cbs[i].callback = callback;
            uart_mgr.uart_cbs[i].uart_port = uart_port;
            return true;
        }
    }

    return false;
}

bool uart_irq_callback_unregister(uint32_t uart_port)
{
    uint8_t i;

    for (i = 0; i < MAX_UART_NUM; i++) {
        if (uart_mgr.uart_cbs[i].uart_port == uart_port) {
            uart_mgr.uart_cbs[i].callback = NULL;
            uart_mgr.uart_cbs[i].uart_port = 0;
            return true;
        }
    }

    return false;
}

void uart_dma_single_mode_config(uint32_t uart, uint32_t direction)
{
    dma_single_data_parameter_struct dma_init_struct;
    dma_channel_enum dma_chnlx = DMA_CH0;

    dma_single_data_para_struct_init(&dma_init_struct);
    dma_init_struct.direction = direction;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;

    if (direction == DMA_MEMORY_TO_PERIPH) {
        dma_init_struct.periph_addr = (uint32_t)&USART_TDATA(uart);
    } else if (direction == DMA_PERIPH_TO_MEMORY) {
        dma_init_struct.periph_addr = (uint32_t)&USART_RDATA(uart);
    } else {
        return;
    }

    switch (uart) {
    case USART0:
        if (direction == DMA_MEMORY_TO_PERIPH) {
            dma_chnlx = DMA_CH7;
        } else if (direction == DMA_PERIPH_TO_MEMORY) {
            dma_chnlx = DMA_CH2;
        }
        break;

    case UART1:
        if (direction == DMA_MEMORY_TO_PERIPH) {
            dma_chnlx = DMA_CH1;
        } else if (direction == DMA_PERIPH_TO_MEMORY) {
            dma_chnlx = DMA_CH0;
        }
        break;

    case UART2:
        if (direction == DMA_MEMORY_TO_PERIPH) {
            dma_chnlx = DMA_CH6;
        } else if (direction == DMA_PERIPH_TO_MEMORY) {
            dma_chnlx = DMA_CH5;
        }
        break;

    default:
        break;
    }

    dma_deinit(dma_chnlx);
    dma_single_data_mode_init(dma_chnlx, &dma_init_struct);

    dma_circulation_disable(dma_chnlx);
    dma_channel_subperipheral_select(dma_chnlx, DMA_SUBPERI4);
    dma_flow_controller_config(dma_chnlx, DMA_FLOW_CONTROLLER_DMA);

    dma_interrupt_enable(dma_chnlx, DMA_INT_FTF);
}

/*!
    \brief      configure usart
    \param[in]  usart_periph: USARTx(x=0,1)
    \param[out] none
    \retval     none
*/
void uart_config(uint32_t usart_periph, uint32_t baudrate, bool flow_cntl, bool dma_rx, bool dma_tx)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);

    if (usart_periph == USART0) {
        rcu_periph_clock_enable(RCU_USART0);

        gpio_af_set(USART0_TX_GPIO, USART0_TX_AF_NUM, USART0_TX_PIN);
        gpio_af_set(USART0_RX_GPIO, USART0_RX_AF_NUM, USART0_RX_PIN);
        gpio_mode_set(USART0_TX_GPIO, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART0_TX_PIN);
        gpio_output_options_set(USART0_TX_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, USART0_TX_PIN);
        gpio_mode_set(USART0_RX_GPIO, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART0_RX_PIN);
        gpio_output_options_set(USART0_RX_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, USART0_RX_PIN);

#ifdef CONFIG_PLATFORM_ASIC
        /* configure cts/rts */
        if (flow_cntl) {
            gpio_af_set(USART0_CTS_GPIO, USART0_CTS_AF_NUM, USART0_CTS_PIN);
            gpio_af_set(USART0_RTS_GPIO, USART0_RTS_AF_NUM, USART0_RTS_PIN);
            gpio_mode_set(USART0_CTS_GPIO, GPIO_MODE_AF, GPIO_PUPD_NONE, USART0_CTS_PIN);
            gpio_output_options_set(USART0_CTS_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, USART0_CTS_PIN);
            gpio_mode_set(USART0_RTS_GPIO, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART0_RTS_PIN);
            gpio_output_options_set(USART0_RTS_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, USART0_RTS_PIN);
        }
#endif
    }
    else if (usart_periph == UART1) {
        rcu_periph_clock_enable(RCU_UART1);

        gpio_af_set(UART1_TX_GPIO, UART1_TX_AF_NUM, UART1_TX_PIN);
        gpio_af_set(UART1_RX_GPIO, UART1_RX_AF_NUM, UART1_RX_PIN);
        gpio_mode_set(UART1_TX_GPIO, GPIO_MODE_AF, GPIO_PUPD_PULLUP, UART1_TX_PIN);
        gpio_output_options_set(UART1_TX_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, UART1_TX_PIN);
        gpio_mode_set(UART1_RX_GPIO, GPIO_MODE_AF, GPIO_PUPD_PULLUP, UART1_RX_PIN);
        gpio_output_options_set(UART1_RX_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, UART1_RX_PIN);

#ifdef CONFIG_PLATFORM_ASIC
        if (flow_cntl) {
            /* configure cts/rts */
            gpio_af_set(UART1_CTS_GPIO, UART1_CTS_AF_NUM, UART1_CTS_PIN);
            gpio_af_set(UART1_RTS_GPIO, UART1_RTS_AF_NUM, UART1_RTS_PIN);
            gpio_mode_set(UART1_CTS_GPIO, GPIO_MODE_AF, GPIO_PUPD_NONE, UART1_CTS_PIN);
            gpio_output_options_set(UART1_CTS_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, UART1_CTS_PIN);
            gpio_mode_set(UART1_RTS_GPIO, GPIO_MODE_AF, GPIO_PUPD_NONE, UART1_RTS_PIN);
            gpio_output_options_set(UART1_RTS_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, UART1_RTS_PIN);
        }
#endif
    } else if (usart_periph == UART2) {
        rcu_periph_clock_enable(RCU_UART2);

        gpio_af_set(UART2_TX_GPIO, UART2_TX_AF_NUM, UART2_TX_PIN);
        gpio_af_set(UART2_RX_GPIO, UART2_RX_AF_NUM, UART2_RX_PIN);
        gpio_mode_set(UART2_TX_GPIO, GPIO_MODE_AF, GPIO_PUPD_PULLUP, UART2_TX_PIN);
        gpio_output_options_set(UART2_TX_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, UART2_TX_PIN);
        gpio_mode_set(UART2_RX_GPIO, GPIO_MODE_AF, GPIO_PUPD_PULLUP, UART2_RX_PIN);
        gpio_output_options_set(UART2_RX_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, UART2_RX_PIN);

        if (flow_cntl) {
            /* configure cts/rts */
            gpio_af_set(UART2_CTS_GPIO, UART2_CTS_AF_NUM, UART2_CTS_PIN);
            gpio_af_set(UART2_RTS_GPIO, UART2_RTS_AF_NUM, UART2_RTS_PIN);
            gpio_mode_set(UART2_CTS_GPIO, GPIO_MODE_AF, GPIO_PUPD_NONE, UART2_CTS_PIN);
            gpio_output_options_set(UART2_CTS_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, UART2_CTS_PIN);
            gpio_mode_set(UART2_RTS_GPIO, GPIO_MODE_AF, GPIO_PUPD_NONE, UART2_RTS_PIN);
            gpio_output_options_set(UART2_RTS_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, UART2_RTS_PIN);
        }
    }

    /* close printf buffer */
    setvbuf(stdout, NULL, _IONBF, 0);

    usart_deinit(usart_periph);
    usart_baudrate_set(usart_periph, baudrate);
    usart_receive_config(usart_periph, USART_RECEIVE_ENABLE);
    usart_transmit_config(usart_periph, USART_TRANSMIT_ENABLE);
    usart_interrupt_enable(usart_periph, USART_INT_RBNE);
    usart_receive_fifo_enable(usart_periph);

    if (flow_cntl) {
        usart_hardware_flow_rts_config(usart_periph, USART_RTS_ENABLE);
        usart_hardware_flow_cts_config(usart_periph, USART_CTS_ENABLE);
    } else {
        usart_hardware_flow_rts_config(usart_periph, USART_RTS_DISABLE);
        usart_hardware_flow_cts_config(usart_periph, USART_CTS_DISABLE);
    }

    if (dma_rx) {
        usart_interrupt_disable(usart_periph, USART_INT_RBNE);
        rcu_periph_clock_enable(RCU_DMA);
        uart_dma_single_mode_config(usart_periph, DMA_PERIPH_TO_MEMORY);
        usart_dma_receive_config(usart_periph, USART_RECEIVE_DMA_ENABLE);
    }

    if (dma_tx) {
        rcu_periph_clock_enable(RCU_DMA);
        uart_dma_single_mode_config(usart_periph, DMA_MEMORY_TO_PERIPH);
        usart_dma_transmit_config(usart_periph, USART_TRANSMIT_DMA_ENABLE);
    }

    usart_enable(usart_periph);
}

void uart_put_data(uint32_t usart_periph, const uint8_t *d, int size)
{
    if (size == 0) {
        return;
    }

    while (1) {
        while (RESET == usart_flag_get(usart_periph, USART_FLAG_TBE));
        usart_data_transmit(usart_periph, *d++);
        size--;
        if (size == 0) {
            return;
        }
    }
}

void uart_putc_noint(uint32_t usart_periph, uint8_t c)
{
    while (RESET == usart_flag_get(usart_periph, USART_FLAG_TBE));
    usart_data_transmit(usart_periph, (uint8_t)c);
}

char uart_getc(uint32_t uart_id)
{
    uint8_t rx_char = '\0';

    if ((RESET != usart_interrupt_flag_get(uart_id, USART_INT_FLAG_RBNE)) &&
        (RESET != usart_flag_get(uart_id, USART_FLAG_RBNE))) {
        rx_char = (char)usart_data_receive(uart_id);
        if (RESET != usart_flag_get(uart_id, USART_FLAG_ORERR)) {
            usart_flag_clear(uart_id, USART_FLAG_ORERR);
        }
    }
    return (char)rx_char;
}

extern void tuya_uart_irq_hdl(uint32_t uart);
void uart_irq_hdl(uint32_t uart)
{
    tuya_uart_irq_hdl(uart);
#ifdef CONFIG_BASECMD
    uint8_t i;

    for (i = 0; i < MAX_UART_NUM; i++) {
        if (uart_mgr.uart_cbs[i].uart_port == uart) {
            uart_mgr.uart_cbs[i].callback(uart);
            return;
        }
    }

    usart_interrupt_disable(uart, USART_INT_RBNE);
    while (1) {
        // We should have chance to check overflow error
        // Otherwise it may cause dead loop handle rx interrupt
        if (RESET != usart_flag_get(uart, USART_FLAG_ORERR)) {
            usart_flag_clear(uart, USART_FLAG_ORERR);
        }

        if ((RESET != usart_flag_get(uart, USART_FLAG_RBNE))) {
            usart_data_receive(uart);
        } else {
            break;
        }
    }

    usart_interrupt_enable(uart, USART_INT_RBNE);
#endif
}

void uart_tx_idle_wait(uint32_t usart_periph)
{
    while (RESET == usart_flag_get(usart_periph, USART_FLAG_TC));
}

int uart_getc_with_timeout(uint32_t usart_periph, char *ch, int timeout)
{
    while (timeout -- > 0) {
        if (RESET != usart_flag_get(usart_periph, USART_FLAG_RBNE)) {
            *ch = (char)usart_data_receive(usart_periph);
            return 0;
        }
    }
    return 1;
}

void uart_rx_flush(uint32_t usart_periph)
{
    while (RESET != usart_flag_get(usart_periph, USART_FLAG_RBNE)) {
        usart_data_receive(usart_periph);
    }
}
