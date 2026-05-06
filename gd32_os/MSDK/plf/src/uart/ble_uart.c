/*!
    \file    ble_uart.c
    \brief   BLE UART for GD32VW55x SDK.

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
#include "wrapper_os.h"
#include "dbg_print.h"
#include "wakelock.h"
#include "ble_export.h"
#include "ll.h"
#include "ble_uart.h"

#ifdef CFG_BLE_HCI_MODE
void (*read_callback)(void *, uint8_t) = NULL;
void *read_dummy = NULL;
uint8_t *read_buf_ptr = NULL;
uint32_t read_buf_size = 0;
uint32_t current_index = 0;
char hci_uart_buf[HCI_UART_BUFFER_SIZE];
uint32_t hci_uart_index = 0;

void hci_uart_read_handler(void)
{
    void (*callback)(void *, uint8_t) = NULL;
    void *data = NULL;

    if (hci_uart_index - current_index >= read_buf_size && read_buf_size != 0) {
        memcpy(read_buf_ptr, hci_uart_buf + current_index, read_buf_size);
        current_index += read_buf_size;
        read_buf_size = 0;

        // Retrieve callback pointer
        callback = read_callback;
        data     = read_dummy;
        if (callback != NULL) {
            // Clear callback pointer
            read_callback = NULL;
            read_dummy    = NULL;

            // Call handler
            callback(data, 0);

            if (__get_CONTROL() == 1) {
                ble_stack_task_resume(true);
            } else {
                ble_stack_task_resume(false);
            }
        }

        if (hci_uart_index == current_index) {
            current_index = 0;
            hci_uart_index = 0;
        }
    }
}

static void hci_uart_irq_hdl(uint32_t uart_port)
{
    if (hci_uart_index < HCI_UART_BUFFER_SIZE) {
        hci_uart_buf[hci_uart_index++] = uart_getc(uart_port);
    }

    hci_uart_read_handler();
}

#ifdef HCI_UART_RX_DMA
void hci_uart_dma_channel5_irq_hdl(void)
{
    void (*callback)(void *, uint8_t) = NULL;
    void *data = NULL;

    if (RESET != dma_interrupt_flag_get(HCI_DMA_CHNL, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(HCI_DMA_CHNL, DMA_INT_FLAG_FTF);
        dma_interrupt_flag_clear(HCI_DMA_CHNL, DMA_INT_FLAG_HTF);

        // Retrieve callback pointer
        callback = read_callback;
        data     = read_dummy;
        if (callback != NULL) {
            // Clear callback pointer
            read_callback = NULL;
            read_dummy    = NULL;

            // Call handler
            callback(data, 0);

            ble_stack_task_resume(true);
        }
    }
}
#endif

void uart_flow_on(void)
{
}

bool uart_flow_off(void)
{
    return true;
}

void uart_read(uint8_t *bufptr, uint32_t size, void (*callback)(void *, uint8_t), void *dummy)
{
    if (bufptr == NULL || size == 0 || callback == NULL) {
        dbg_print(ERR, "uart_read, input param error\r\n");
        return;
    }

#ifdef HCI_UART
    read_callback = callback;
    read_dummy    = dummy;
    read_buf_ptr  = bufptr;
    read_buf_size = size;

#ifdef HCI_UART_RX_DMA
    dma_memory_address_config(HCI_DMA_CHNL, DMA_MEMORY_0, (uint32_t)bufptr);
    dma_transfer_number_config(HCI_DMA_CHNL, size);
    dma_channel_enable(HCI_DMA_CHNL);
#else
    GLOBAL_INT_DISABLE();
    hci_uart_read_handler();
    GLOBAL_INT_RESTORE();
#endif
#endif
}

void uart_write(uint8_t *bufptr, uint32_t size, void (*callback)(void *, uint8_t), void *dummy)
{
    if (bufptr == NULL || size == 0 || callback == NULL) {
        dbg_print(ERR, "uart_write, input param error\r\n");
        return;
    }

#ifdef HCI_UART
    uart_put_data(HCI_UART, bufptr, size);
#endif
    callback(dummy, 0);
}

// Creation of uart external interface api
ble_uart_func_t uart_api = {
    uart_read,
    uart_write,
    uart_flow_on,
    uart_flow_off,
};

void ble_uart_init(void)
{
#ifdef HCI_UART
    read_dummy = NULL;
    read_buf_ptr = NULL;
    read_buf_size = 0;
    current_index = 0;
    memset(hci_uart_buf, 0, HCI_UART_BUFFER_SIZE);
    hci_uart_index = 0;
#ifdef HCI_UART_RX_DMA
    uart_config(HCI_UART, DEFAULT_LOG_BAUDRATE, true, true, false);
#else
    uart_config(HCI_UART, DEFAULT_LOG_BAUDRATE, true, false, false);
#endif

    uart_irq_callback_register(HCI_UART, hci_uart_irq_hdl);
#endif
}

ble_uart_func_t *ble_uart_func_get(void)
{
    return &uart_api;
}
#endif /* CFG_BLE_HCI_MODE */
