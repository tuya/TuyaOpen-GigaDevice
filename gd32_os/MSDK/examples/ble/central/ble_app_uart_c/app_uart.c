/*!
    \file    app_uart.c
    \brief   APP UART for GD32VW55x SDK.

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
#include "app_uart.h"
#include "wrapper_os.h"
#include "dbg_print.h"
#include "wakelock.h"
#include "log_uart.h"
#include "ble_datatrans_cli.h"

/* Priority of the uart task */
#define UART_TASK_PRIORITY OS_TASK_PRIORITY(4)
/* Uart queque size */
#define UART_QUEUE_SIZE 3

/*  Message format */
struct uart_msg {
    uint16_t len;   /*!< Length, in bytes, of the message */
    void *data;     /*!< Pointer to the message */
};

/* Uart queque */
static os_queue_t uart_queue;
/* Uart cyclic buffer */
static cyclic_buf_t uart_cyc_buf;
/* Uart RX buffer */
char uart_rx_buf[UART_BUFFER_SIZE];
/* Uart RX buffer index*/
uint32_t uart_index = 0;

extern uint8_t conn_idx;

/*!
    \brief      app uart rx handle
    \param[in]  uart_cyc_buf: uart cyclic buffer
    \param[out] buf: data buf
    \param[in]  len: data length
    \retval     none
*/
static void app_uart_rx_handle_done(cyclic_buf_t *uart_cyc_buf, uint8_t *buf, uint16_t *len)
{
    if (*len > cyclic_buf_count(uart_cyc_buf)) {
        *len = cyclic_buf_count(uart_cyc_buf);
    }

    if (buf == NULL) {
        cyclic_buf_drop(uart_cyc_buf, *len);
    } else {
        cyclic_buf_read(uart_cyc_buf, buf, *len);
    }
}

/*!
    \brief      uart message precess
    \param[in]  msg: uart message
    \retval     none
*/
static void app_uart_msg_process(struct uart_msg *msg)
{
    cyclic_buf_t *p_cyclic_buf = (cyclic_buf_t *)msg->data;
    char *command;

    command = sys_malloc(msg->len + 1);
    if (command == NULL) {
        app_print("No buffer alloc for uart msg !\r\n");
        return;
    }

    app_uart_rx_handle_done((cyclic_buf_t *)msg->data, (uint8_t *)command, &msg->len);
    command[msg->len] = '\0';

    if (ble_datatrans_cli_write_char(conn_idx, (uint8_t *)command, msg->len) == BLE_ERR_NO_ERROR) {
        app_print("datatrans cli send data: \r\n");
        app_print("%s\r\n", command);
    }

    sys_mfree(command);
    return;
}

/*!
    \brief      uart task
    \param[in]  param:
    \retval     none
*/
static void app_uart_task(void *param)
{
    struct uart_msg msg;

    for (;;) {
        sys_queue_read(&uart_queue, &msg, -1, false);
        app_uart_msg_process(&msg);
    }
}

/*!
    \brief      uart message send
    \param[in]  msg_data: message data
    \param[in]  len: message length
    \retval     success 0
*/
int app_uart_send(void *msg_data, uint16_t len)
{
    struct uart_msg msg;

    msg.len  = len;
    msg.data = msg_data;
    return sys_queue_write(&uart_queue, &msg, 0, true);
}

/*!
    \brief      uart rx data handler
    \param[in]  none
    \retval     none
*/
static void rx_handler(void)
{
    // uart_index - 2 to remove 0x0d and 0x0a when using Husky
    if (uart_index > 2) {
        if (app_uart_send((void *)(&uart_cyc_buf), uart_index - 2) == 0) {
            if (cyclic_buf_write(&uart_cyc_buf, (uint8_t *)uart_rx_buf, uart_index - 2) == false) {
                dbg_print(ERR, "uart cyclic buffer full\r\n");
            }
        } else {
            /* queue was full */
            dbg_print(ERR, "queue full\r\n");
            /* TODO: report 'message ignored' status */
        }
    }

    uart_index = 0;
}

/*!
    \brief      app uart rx irq handler
    \param[in]  uart_port: uart port
    \retval     none
*/
static void app_uart_rx_irq_hdl(uint32_t uart_port)
{
    usart_interrupt_disable(uart_port, USART_INT_RBNE);

    while (1) {
        // We should have chance to check overflow error
        // Otherwise it may cause dead loop handle rx interrupt
        if (RESET != usart_flag_get(uart_port, USART_FLAG_ORERR)) {
            usart_flag_clear(uart_port, USART_FLAG_ORERR);
        }

        if ((RESET != usart_flag_get(uart_port, USART_FLAG_RBNE))) {
            uart_rx_buf[uart_index++] = (char)usart_data_receive(uart_port);
            if (uart_index >= UART_BUFFER_SIZE) {
                uart_index = 0;
            }
        } else {
            break;
        }
    }

    if(RESET != usart_flag_get(uart_port, USART_FLAG_IDLE)) {
        usart_flag_clear(uart_port, USART_FLAG_IDLE);
        if (uart_index > 0)
            rx_handler();
    }

    sys_wakelock_release(LOCK_ID_USART);
    usart_interrupt_enable(uart_port, USART_INT_RBNE);
}

/*!
    \brief      app uart config
    \param[in]  none
    \retval     none
*/

void app_uart_config(void)
{
    uart_config(LOG_UART, DEFAULT_LOG_BAUDRATE, false, false, false);

    /*wait IDLEF set and clear it*/
    while(RESET == usart_flag_get(LOG_UART, USART_FLAG_IDLE)) {

    }

    usart_flag_clear(LOG_UART, USART_FLAG_IDLE);
    usart_interrupt_enable(LOG_UART, USART_INT_IDLE);
}

/*!
    \brief      app uart initialize
    \retval     success 0
*/
int app_uart_init(void)
{
    if (sys_task_create_dynamic((const uint8_t *)"UART task", 512, UART_TASK_PRIORITY, app_uart_task, NULL) == NULL) {
        return -1;
    }

    if (sys_queue_init(&uart_queue, UART_QUEUE_SIZE, sizeof(struct uart_msg))) {
        return -2;
    }

    app_uart_config();
    memset(uart_rx_buf, 0, UART_BUFFER_SIZE);
    uart_index = 0;
    cyclic_buf_init(&uart_cyc_buf, 4 * UART_BUFFER_SIZE);
    uart_irq_callback_register(LOG_UART, app_uart_rx_irq_hdl);

    return 0;
}

