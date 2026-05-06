/*!
    \file    trace_uart.c
    \brief   TRACE UART for GD32VW55x SDK.

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
#include <stdio.h>
#include "gd32vw55x.h"
#include "wrapper_os.h"
#include "dbg_print.h"
#include "wakelock.h"
#include "trace_uart.h"
#include "trace_ext.h"
#include "cyclic_buffer.h"

#ifdef CFG_GD_TRACE_EXT
#if (!defined(LOG_UART) && defined(TRACE_UART))
static char trace_uart_buf[UART_BUFFER_SIZE];
static uint16_t trace_uart_index = 0;
static cyclic_buf_t trace_uart_cyc_buf;
trace_uart_rx_callback rx_callback = NULL;

static void trace_cmd_rx_indicate(void)
{
    if (cyclic_buf_write(&trace_uart_cyc_buf, (uint8_t *)trace_uart_buf, trace_uart_index + 1)) {
        if (rx_callback) {
            if (rx_callback(0x23, (void *)(&trace_uart_cyc_buf), trace_uart_index + 1)) {
                /* queue was full */
                dbg_print(ERR, "queue full\r\n");
                /* TODO: report 'message ignored' status */
            }
        } else {
            dbg_print(ERR, "rx_callback is not found\r\n");
        }
    } else {
        dbg_print(ERR, "trace uart cyclic buffer full\r\n");
    }

    trace_uart_index = 0;
}

static void trace_uart_rx_irq_hdl(uint32_t uart_port)
{
    char ch;
    usart_interrupt_disable(uart_port, USART_INT_RBNE);
    while (1) {
        // We should have chance to check overflow error
        // Otherwise it may cause dead loop handle rx interrupt
        if (RESET != usart_flag_get(uart_port, USART_FLAG_ORERR)) {
            usart_flag_clear(uart_port, USART_FLAG_ORERR);
        }

        if ((RESET != usart_flag_get(uart_port, USART_FLAG_RBNE))) {
            ch = (char)usart_data_receive(uart_port);
        } else {
            break;
        }

        if (ch == '\0') {
            break;
        }

        if (isprint(ch)) {
            trace_uart_buf[trace_uart_index++] = ch;
            if (trace_uart_index >= UART_BUFFER_SIZE) {
                trace_uart_index = 0;
            }
        } else if (ch == '\r') { /* putty doesn't transmit '\n' */
            trace_console(trace_uart_index, (uint8_t *)trace_uart_buf);
            trace_uart_buf[trace_uart_index] = '\0';

            if (trace_uart_index > 0) {
                trace_cmd_rx_indicate();
            }

            trace_console(4, (uint8_t *)"\r\n# ");
            sys_wakelock_release(LOCK_ID_USART);
        } else if (ch == '\b') { /* non-destructive backspace */
            if (trace_uart_index > 0) {
                trace_uart_buf[--trace_uart_index] = '\0';
            }
        }
    }

    usart_interrupt_enable(uart_port, USART_INT_RBNE);
}

void trace_uart_rx_cb_register(trace_uart_rx_callback callback)
{
    rx_callback = callback;
}

#endif

void uart_transfer_trace_data(const uint8_t *d, int size)
{
#ifdef TRACE_UART
    uart_put_data(TRACE_UART, d, size);
#endif
}

#ifdef TRACE_UART_DMA
void trace_uart_dma_channel_irq_hdl(void)
{
    // FIX TODO need to check more error
    if (RESET != dma_interrupt_flag_get(TRACE_DMA_CHNL, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(TRACE_DMA_CHNL, DMA_INT_FLAG_FTF);
        dma_interrupt_flag_clear(TRACE_DMA_CHNL, DMA_INT_FLAG_HTF);

        trace_dma_transfer_cmplt();
    }
}

void trace_uart_dma_transfer(uint32_t address, uint32_t num)
{
    dma_memory_address_config(TRACE_DMA_CHNL, DMA_MEMORY_0, address);
    dma_transfer_number_config(TRACE_DMA_CHNL, num);
    dma_channel_enable(TRACE_DMA_CHNL);
}
#endif

void trace_uart_init(void)
{
#ifdef TRACE_UART
    bool dma_tx = false;
    bool flow_cntl = false;
#ifdef TRACE_UART_DMA
    dma_tx = true;
#endif

#ifndef LOG_UART
    flow_cntl = true;
    memset(trace_uart_buf, 0, UART_BUFFER_SIZE);
    trace_uart_index = 0;
    cyclic_buf_init(&trace_uart_cyc_buf, 4 * UART_BUFFER_SIZE);
    uart_irq_callback_register(TRACE_UART, trace_uart_rx_irq_hdl);
#endif

    uart_config(TRACE_UART, BAUDRATE_2000000, flow_cntl, false, dma_tx);
#endif

}
#endif /* CFG_GD_TRACE_EXT */
