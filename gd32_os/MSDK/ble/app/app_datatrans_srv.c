/*!
    \file    app_datatrans_srv.c
    \brief   Datatrans Service Server Application Module entry point.

    \version 2024-07-2, V1.0.0, firmware for GD32VW55x
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

#include <string.h>
#include "app_datatrans_srv.h"
#include "ble_datatrans_srv.h"
#include "wrapper_os.h"
#include "dbg_print.h"
#include "app_dev_mgr.h"
#include "cmd_shell.h"
#include "uart.h"
#include "gd32vw55x.h"
#include "uart_config.h"
#include "log_uart.h"

#define PASSTH_TERMINATE_STR            "+++"

static bool disconn_flag;

void app_datatrans_uart_rx_dma_irq_hdl(uint32_t dma_channel)
{
    if(RESET != dma_interrupt_flag_get(dma_channel, DMA_INT_FLAG_FTF)){
        dma_interrupt_flag_clear(dma_channel, DMA_INT_FLAG_FTF);
    }
}

bool app_datatrans_terminate_string_check(uint8_t *str)
{
    if (strlen(PASSTH_TERMINATE_STR) == strlen((char *)str) && strncmp((char *)str, PASSTH_TERMINATE_STR, strlen(PASSTH_TERMINATE_STR)) == 0)
        return true;
    return false;
}

static void uart_dma_receive_config(uint32_t usart_periph, uint32_t baudrate)
{
    uart_tx_idle_wait(usart_periph);

    uart_config(usart_periph, baudrate, false, true, false);
    switch (usart_periph) {
    case USART0:
        eclic_irq_enable(DMA_Channel2_IRQn, 8, 0);
        break;
    case UART1:
        eclic_irq_enable(DMA_Channel0_IRQn, 8, 0);
        break;
    case UART2:
    default:
        eclic_irq_enable(DMA_Channel5_IRQn, 8, 0);
        break;
    }
}

static void uart_irq_receive_config(uint32_t usart_periph, uint32_t baudrate)
{
    uint32_t dma_channel;

    switch (usart_periph) {
    case USART0:
        eclic_irq_disable(DMA_Channel2_IRQn);
        dma_channel = DMA_CH2;
        break;
    case UART1:
        eclic_irq_disable(DMA_Channel0_IRQn);
        dma_channel = DMA_CH0;
        break;
    case UART2:
    default:
        eclic_irq_disable(DMA_Channel5_IRQn);
        dma_channel = DMA_CH5;
        break;
    }

    usart_dma_receive_config(usart_periph, USART_RECEIVE_DMA_DISABLE);
    uart_config(usart_periph, baudrate, false, false, false);
}

static void uart_dma_receive_start(uint32_t usart_periph, uint32_t address, uint32_t num)
{
    uint32_t dma_channel;

    uart_dma_single_mode_config(usart_periph, DMA_PERIPH_TO_MEMORY);

    switch (usart_periph) {
    case USART0:
        dma_channel = DMA_CH2;
        break;
    case UART1:
        dma_channel = DMA_CH0;
        break;
    case UART2:
    default:
        dma_channel = DMA_CH5;
        break;
    }

    dma_memory_address_config(dma_channel, DMA_MEMORY_0, address);
    dma_transfer_number_config(dma_channel, num);
    dma_channel_enable(dma_channel);
}

static void uart_dma_receive_stop(uint32_t usart_periph)
{
    uint32_t dma_channel;

    switch (usart_periph) {
    case USART0:
        dma_channel = DMA_CH2;
        break;
    case UART1:
        dma_channel = DMA_CH0;
        break;
    case UART2:
    default:
        dma_channel = DMA_CH5;
        break;
    }

    dma_interrupt_flag_clear(dma_channel, DMA_INT_FLAG_FTF);
    dma_interrupt_disable(dma_channel, DMA_INT_FTF);
    dma_channel_disable(dma_channel);
}

static uint32_t dma_get_cur_received_num(uint32_t usart_periph, uint32_t size)
{
    uint32_t dma_channel;

    switch (usart_periph) {
    case USART0:
        dma_channel = DMA_CH2;
        break;
    case UART1:
        dma_channel = DMA_CH0;
        break;
    case UART2:
    default:
        dma_channel = DMA_CH5;
        break;
    }

    return (size - dma_transfer_number_get(dma_channel));
}

/*!
    \brief      APP datatrans service server tx callback
    \param[in]  data_len: data_length
    \param[in]  p_data: pointer to data to handle
    \param[out] none
    \retval     none
*/
static void app_datatrans_srv_tx_callback(uint16_t data_len, uint8_t *p_data)
{
    uint32_t conidx_bf = dm_get_conidx_bf();

    if (conidx_bf && (ble_datatrans_srv_tx_mtp(conidx_bf, p_data, data_len) == BLE_ERR_NO_ERROR)) {
        // add user code here
    }
}

/*!
    \brief      APP datatrans service server rx callback
    \param[in]  conn_idx: connection index
    \param[in]  data_len: data_length
    \param[in]  p_data: pointer to data to handle
    \param[out] none
    \retval     none
*/
static void app_datatrans_srv_rx_callback(uint8_t conn_idx, uint16_t data_len, uint8_t *p_data)
{
    log_uart_put_data(p_data, data_len);
}

void app_datatrans_conn_evt_handler(ble_conn_evt_t event, ble_conn_data_u *p_data)
{
    if (event == BLE_CONN_EVT_STATE_CHG) {
        if (p_data->conn_state.state == BLE_CONN_STATE_DISCONNECTD)
            disconn_flag = true;
        else if (p_data->conn_state.state == BLE_CONN_STATE_CONNECTED)
            disconn_flag = false;
    }
}

/*!
    \brief      Start datatrans
    \param[in]  conidx: connection index
    \param[in]  baudrate: uart baudrate
    \param[out] none
    \retval     none
*/
void app_datatrans_start(uint8_t conidx, uint32_t baudrate)
{
    uint32_t cur_cnt = 0;
    uint16_t att_mtu_size = 0;
    uint16_t data_max_size = 0;
    uint8_t *tx_buf = NULL;
    bool reset = true;

    if (!dm_check_connection_valid(conidx)) {
        dbg_print(NOTICE, "link has not been established\r\n");
        return;
    }

    ble_gatts_mtu_get(0 , &att_mtu_size);
    data_max_size = att_mtu_size - 3;
    tx_buf = sys_malloc(data_max_size);
    if (!tx_buf) {
        dbg_print(NOTICE, "buffer alloc fail\r\n");
        return;
    }

    while (1) {
        if(reset) {
            uart_dma_receive_config(LOG_UART, baudrate);   //have to reconfig uart here, or one byte left data will be transfered by dma
            while(RESET == usart_flag_get(LOG_UART, USART_FLAG_IDLE));
            usart_flag_clear(LOG_UART, USART_FLAG_IDLE);
            reset = false;
            sys_memset(tx_buf, 0, data_max_size);
            uart_dma_receive_start(LOG_UART, (uint32_t)tx_buf, data_max_size);
        }

        sys_ms_sleep(1);

        if (disconn_flag == true) {
            disconn_flag = false;
            break;
        }

        if (RESET != usart_flag_get(LOG_UART, USART_FLAG_IDLE)) {
            usart_flag_clear(LOG_UART, USART_FLAG_IDLE);
            cur_cnt = dma_get_cur_received_num(LOG_UART, data_max_size);
            reset = true;
            uart_dma_receive_stop(LOG_UART);

            if (app_datatrans_terminate_string_check(tx_buf))
                break;

            if (ble_datatrans_srv_tx(conidx, tx_buf, cur_cnt) != BLE_ERR_NO_ERROR) {
                dbg_print(NOTICE, "data send fail\r\n");
                if (tx_buf)
                    sys_mfree(tx_buf);
                return;
            }
        }
    }

    uart_dma_receive_stop(LOG_UART);
    uart_irq_receive_config(LOG_UART, baudrate);
    sys_mfree(tx_buf);
}

/*!
    \brief      Init APP datatrans service server module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_datatrans_srv_init(void)
{
#if (BLE_DATATRANS_MODE == MIXED_TRANSMIT_MODE)
    cmd_unkwn_cmd_handler_reg(app_datatrans_srv_tx_callback);
#endif
    ble_datatrans_srv_init();
    ble_datatrans_srv_rx_cb_reg(app_datatrans_srv_rx_callback);

#if (BLE_DATATRANS_MODE == PURE_DATA_TRANSMIT_MODE)
    ble_conn_callback_register(app_datatrans_conn_evt_handler);
#endif
}

/*!
    \brief      Deinit APP datatrans service server module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_datatrans_srv_deinit(void)
{
#if (BLE_DATATRANS_MODE == MIXED_TRANSMIT_MODE)
    cmd_unkwn_cmd_handler_unreg();
#endif
    ble_datatrans_srv_deinit();
#if (BLE_DATATRANS_MODE == PURE_DATA_TRANSMIT_MODE)
    ble_conn_callback_unregister(app_datatrans_conn_evt_handler);
#endif
}

