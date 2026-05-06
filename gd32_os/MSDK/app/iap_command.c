/*!
    \file    iap_command.c
    \brief   the systick configuration file

    \version 2017-02-10, V1.0.0, firmware for GD32F30x
    \version 2018-10-10, V1.1.0, firmware for GD32F30x
    \version 2018-12-25, V2.0.0, firmware for GD32F30x
    \version 2020-09-30, V2.1.0, firmware for GD32F30x
*/

/*
    Copyright (c) 2020, GigaDevice Semiconductor Inc.

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

#include "uart_config.h"
#include "gd32vw55x.h"
#include "gd32vw55x_it.h"
#include "string.h"
#include "wrapper_os.h"
#include "iap_command.h"
#include "uart.h"

#define ACK 0x79
#define START_PAGE_NUM  32
#define F505_2K_PAGE_NUM (256 - START_PAGE_NUM) // 256 pages in bank0, 8 pages reserved
#define F505_4K_PAGE_NUM (128 - 1) // 128 pages in bank1, last page reserved

#define WAIT_ACK_MAX_TIME   5000
#define WAIT_INTERVAL_MS    2

#define WAIT_INTERVAL       sys_ms_sleep(WAIT_INTERVAL_MS);  // Avoid CPU busy-wait

/* USART receive buffer (used in interrupt) */
#define RX_BUFFER_SIZE 256
static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint16_t rx_write_index = 0;
static volatile uint16_t rx_read_index = 0;
static volatile uint8_t iap_mode_active = 0;  // IAP mode flag, 1=IAP command in progress

static uint8_t buffer_cmd[256] = {0};
static uint8_t ack_data;
static uint8_t print_ack = 0;
static os_mutex_t usart_iap_mutex = NULL;  /* Mutex for usart_send_ functions */

extern uint8_t play_mp3(uint16_t idx);

/*!
    \brief      get available bytes count in receive buffer
    \param[in]  none
    \param[out] none
    \retval     available bytes count
*/
static uint16_t rx_buffer_available(void)
{
    return (rx_write_index - rx_read_index) & (RX_BUFFER_SIZE - 1);
}

/*!
    \brief      read one byte from receive buffer
    \param[in]  none
    \param[out] none
    \retval     the byte read
*/
static uint8_t rx_buffer_read(void)
{
    uint8_t data = rx_buffer[rx_read_index];
    rx_read_index = (rx_read_index + 1) & (RX_BUFFER_SIZE - 1);
    return data;
}

/*!
    \brief      clear receive buffer
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void rx_buffer_clear(void)
{
    rx_read_index = rx_write_index;
}

/*!
    \brief      USART receive interrupt handler (called in ISR)
    \param[in]  none
    \param[out] none
    \retval     none
*/
void usart_iap_rx_handler(uint32_t usart_periph)
{
    /* Only receive data to IAP buffer when in IAP mode */
    if (!iap_mode_active) {
        return;
    }

    if (SET == usart_interrupt_flag_get(AT_UART, USART_INT_FLAG_RBNE)) {
        if (RESET != usart_flag_get(AT_UART, USART_FLAG_ORERR)) {
            usart_flag_clear(AT_UART, USART_FLAG_ORERR);
        }
        /* Read data and store in buffer */
        uint8_t data = usart_data_receive(AT_UART);
        uint16_t next_write = (rx_write_index + 1) & (RX_BUFFER_SIZE - 1);

        /* Check if buffer is full */
        if (next_write != rx_read_index) {
            rx_buffer[rx_write_index] = data;
            rx_write_index = next_write;
        }
        /* If buffer is full, discard data (optional: add error counter) */
    }
}

/*!
    \brief      calculate the checksum of the buffer
    \param[in]  data: the input buffer
    \param[in]  len: the length of the input buffer
    \param[out] none
    \retval     the checksum value
*/
static uint8_t data_xor(uint8_t* data , uint16_t len)
{
   uint16_t i ;
   uint8_t temp = data[0];

   for (i = 1; i < len; i++) {
       temp ^= data[i];
   }
   return temp;
}

/*!
    \brief      wait the ACK(0x79)
    \param[in]  none
    \param[out] none
    \retval     ErrStatus
*/
static ErrStatus wait_ack(void)
{
    uint32_t start_ms = sys_time_get(NULL);
    while (rx_buffer_available() == 0) {
        if (sys_time_get(NULL) - start_ms > WAIT_ACK_MAX_TIME) {
            printf("%s:%d wait ACK timeout\n", __func__, __LINE__);
            return ERROR;
        }
        WAIT_INTERVAL
    }

    ack_data = rx_buffer_read();
    if (0x79 != ack_data) {
        printf("%s:%d received 0x%02X, expected ACK(0x79)\n", __func__, __LINE__, ack_data);
        return ERROR;
    }
    if (print_ack) {
        printf("%s:%d recv ACK\n", __func__, __LINE__);
    }

    return SUCCESS;
}

static ErrStatus wait_byte_ack(uint8_t *ack_data)
{
    uint32_t start_ms = sys_time_get(NULL);
    uint8_t first_ack, data_byte, last_ack;

    /* Wait for first ACK */
    while (rx_buffer_available() == 0) {
        if (sys_time_get(NULL) - start_ms > WAIT_ACK_MAX_TIME) {
            printf("%s:%d wait first ACK timeout\n", __func__, __LINE__);
            return ERROR;
        }
        WAIT_INTERVAL
    }
    first_ack = rx_buffer_read();
    if (ACK != first_ack) {
        printf("%s:%d first byte is not ACK (0x%02X)\n", __func__, __LINE__, first_ack);
        return ERROR;
    }

    /* Wait for data byte */
    while (rx_buffer_available() == 0) {
        if (sys_time_get(NULL) - start_ms > WAIT_ACK_MAX_TIME) {
            printf("%s:%d wait data byte timeout\n", __func__, __LINE__);
            return ERROR;
        }
        WAIT_INTERVAL
    }
    data_byte = rx_buffer_read();

    /* Wait for last ACK */
    while (rx_buffer_available() == 0) {
        if (sys_time_get(NULL) - start_ms > WAIT_ACK_MAX_TIME) {
            printf("%s:%d wait last ACK timeout\n", __func__, __LINE__);
            return ERROR;
        }
        WAIT_INTERVAL
    }
    last_ack = rx_buffer_read();
    if (ACK != last_ack) {
        printf("%s:%d last byte is not ACK (0x%02X)\n", __func__, __LINE__, last_ack);
        return ERROR;
    }

    *ack_data = data_byte;
    if (print_ack) {
        printf("%s:%d recv ACK + data(0x%02X) + ACK\n", __func__, __LINE__, data_byte);
    }

    return SUCCESS;
}

static ErrStatus wait_word_ack(uint32_t *ack_data)
{
    uint32_t start_ms = sys_time_get(NULL);
    uint8_t first_ack, data_byte1, data_byte2, data_byte3, data_byte4, last_ack;

    /* Wait for first ACK */
    while (rx_buffer_available() == 0) {
        if (sys_time_get(NULL) - start_ms > WAIT_ACK_MAX_TIME) {
            printf("wait_word_ack: first ACK timeout\n");
            return ERROR;
        }
        WAIT_INTERVAL
    }
    first_ack = rx_buffer_read();
    if (ACK != first_ack) {
        printf("wait_word_ack: first byte not ACK, got 0x%02X\n", first_ack);
        return ERROR;
    }

    /* Wait for 4 data bytes */
    while (rx_buffer_available() == 0) {
        if (sys_time_get(NULL) - start_ms > WAIT_ACK_MAX_TIME) {
            printf("wait_word_ack: byte1 timeout\n");
            return ERROR;
        }
        WAIT_INTERVAL
    }
    data_byte1 = rx_buffer_read();

    while (rx_buffer_available() == 0) {
        if (sys_time_get(NULL) - start_ms > WAIT_ACK_MAX_TIME) {
            printf("wait_word_ack: byte2 timeout\n");
            return ERROR;
        }
        WAIT_INTERVAL
    }
    data_byte2 = rx_buffer_read();

    while (rx_buffer_available() == 0) {
        if (sys_time_get(NULL) - start_ms > WAIT_ACK_MAX_TIME) {
            printf("wait_word_ack: byte3 timeout\n");
            return ERROR;
        }
        WAIT_INTERVAL
    }
    data_byte3 = rx_buffer_read();

    while (rx_buffer_available() == 0) {
        if (sys_time_get(NULL) - start_ms > WAIT_ACK_MAX_TIME) {
            printf("wait_word_ack: byte4 timeout, got %02X %02X %02X\n", data_byte1, data_byte2, data_byte3);
            return ERROR;
        }
        WAIT_INTERVAL
    }
    data_byte4 = rx_buffer_read();

    /* 等待最后ACK */
    while (rx_buffer_available() == 0) {
        if (sys_time_get(NULL) - start_ms > WAIT_ACK_MAX_TIME) {
            printf("%s:%d wait last ACK timeout\n", __func__, __LINE__);
            return ERROR;
        }
        WAIT_INTERVAL
    }
    last_ack = rx_buffer_read();
    if (ACK != last_ack) {
        printf("%s:%d last byte is not ACK (0x%02X)\n", __func__, __LINE__, last_ack);
        return ERROR;
    }

    *ack_data = data_byte1<<24 | data_byte2<<16 | data_byte3<<8 | data_byte4;

    if (print_ack) {
        printf("%s:%d recv ACK + data(0x%08X) + ACK\n", __func__, __LINE__, *ack_data);
    }

    return SUCCESS;
}
/*!
    \brief      send the buff by USART
    \param[in]  buff: the buffer to be sent
    \param[out] len: the length of the buffer
    \retval     none
*/
static void usart_buffer_send(uint8_t* buff, uint32_t len)
{
    while (len--) {
        usart_data_transmit(AT_UART, *buff++);
        while (RESET == usart_flag_get(AT_UART, USART_FLAG_TBE));
    }
}

/*!
    \brief      get the erase sector number of the target
    \param[in]  bin_size: the size of bin to be load
    \param[out] none
    \retval     page number
*/
static uint32_t f505_get_page_number(uint32_t bin_size)
{
    uint32_t i, j;
    uint32_t bank0_page_size[248];
    uint32_t bank1_page_size[128];
    uint32_t page_number = 0;
    uint32_t page_address;
    uint32_t start_address;
    uint32_t end_address;

    for (i = 0; i < F505_2K_PAGE_NUM; i++) {
        bank0_page_size[i] = 2048; // 2KB for each page in bank0
    }

    for (i = 0; i < F505_4K_PAGE_NUM; i++) {
        bank1_page_size[i] = 4096; // 4KB for each page in bank1
    }

    page_address = F505_FW_START_ADDR + bin_size;

    for (i = 0; i < F505_2K_PAGE_NUM; i++) {
        start_address = F505_FW_START_ADDR;
        end_address = F505_FW_START_ADDR;
        for (j = 0; j <= i; j++) {
            start_address += bank0_page_size[j];
            end_address += bank0_page_size[j];
        }
        start_address -= bank0_page_size[i];
        end_address -= 1;
        if (page_address >= start_address && page_address <= end_address) {
            page_number = i;
            break;
        }
    }

    if (page_address > end_address) {
        for (i = 0; i < F505_4K_PAGE_NUM; i++) {
            start_address = F505_FW_START_ADDR + F505_2K_PAGE_NUM * 2048;
            end_address = F505_FW_START_ADDR + F505_2K_PAGE_NUM * 2048;
            for (j = 0; j <= i; j++) {
                start_address += bank1_page_size[j];
                end_address += bank1_page_size[j];
            }
            start_address -= bank1_page_size[i];
            end_address -= 1;
            if (page_address >= start_address && page_address <= end_address) {
                page_number = i + F505_2K_PAGE_NUM;
                break;
            }
        }
    }

    return page_number;
}

void usart_iap_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_USART0);

    // UART0 TX PA0
    gpio_af_set(GPIOA, GPIO_AF_0, GPIO_PIN_0);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_0);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_0);

    // UART0 RX PA15
    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_15);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_15);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_15);

    usart_deinit(AT_UART);
    usart_baudrate_set(AT_UART, 115200U);
    usart_receive_config(AT_UART, USART_RECEIVE_ENABLE);
    usart_transmit_config(AT_UART, USART_TRANSMIT_ENABLE);

    /* Enable receive interrupt */
    usart_interrupt_enable(AT_UART, USART_INT_RBNE);

    /* Disable FIFO, use interrupt mode */
    usart_receive_fifo_disable(AT_UART);

    usart_hardware_flow_rts_config(AT_UART, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(AT_UART, USART_CTS_DISABLE);

    /* Configure ECLIC interrupt */
    eclic_irq_enable(USART0_IRQn, 0xB, 0);

    usart_enable(AT_UART);

    uart_irq_callback_register(AT_UART, usart_iap_rx_handler);
    /* Clear receive buffer */
    rx_buffer_clear();

    /* Initialize mutex for usart send functions */
    if (sys_mutex_init((os_mutex_t *)&usart_iap_mutex) != OS_OK) {
        printf("Failed to initialize usart_iap_mutex\n");
    }
}

ErrStatus usart_send_fw_update_command(uint32_t erase_size)
{
    uint16_t idx = 6;
    ErrStatus result;

    sys_mutex_get((os_mutex_t *)&usart_iap_mutex);

    buffer_cmd[0] = USART_CMD_FW_UPDATE;
    buffer_cmd[1] = 0xFF - buffer_cmd[0];

    printf("Send firmware update command\n");
    usart_buffer_send(buffer_cmd, 2);

    if (SUCCESS != wait_ack()) {
        sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
        return ERROR;
    }

    printf("Play mp3 idx=%u\n", idx);
    play_mp3(idx);

    sys_ms_sleep(3000);

    sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
    return SUCCESS;
}

ErrStatus usart_send_fw_length_command(uint32_t erase_size)
{
    sys_mutex_get((os_mutex_t *)&usart_iap_mutex);

    buffer_cmd[0] = USART_CMD_FW_LENGTH;
    buffer_cmd[1] = 0xFF - buffer_cmd[0];

    printf("Send firmware length command\n");
    usart_buffer_send(buffer_cmd, 2);

    if (SUCCESS != wait_ack()) {
        sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
        return ERROR;
    }

    buffer_cmd[0] = (uint8_t)((erase_size >> 24) & 0xff);
    buffer_cmd[1] = (uint8_t)((erase_size >> 16) & 0xff);
    buffer_cmd[2] = (uint8_t)((erase_size >> 8) & 0xff);
    buffer_cmd[3] = (uint8_t)(erase_size & 0xff);
    buffer_cmd[4] = data_xor(buffer_cmd, 4);

    usart_buffer_send(buffer_cmd, 5);
    if (SUCCESS != wait_ack()) {
        sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
        return ERROR;
    }

    sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
    return SUCCESS;
}

/*!
    \brief      send the erase command
    \param[in]  none
    \param[out] none
    \retval     ErrStatus
*/
ErrStatus usart_send_erase_command(uint32_t bin_size)
{
    uint16_t page_number = 0;

    sys_mutex_get((os_mutex_t *)&usart_iap_mutex);

    buffer_cmd[0] = USART_CMD_Ex_ERASE;
    buffer_cmd[1] = 0xFF - buffer_cmd[0];

    printf("Send erase command\n");
    usart_buffer_send(buffer_cmd, 2);

    if (SUCCESS != wait_ack()) {
        sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
        return ERROR;
    }

    page_number = f505_get_page_number(bin_size);
    page_number = page_number - 1;

    buffer_cmd[0] = (page_number >> 8) & 0xff;
    buffer_cmd[1] = page_number & 0xff;

    buffer_cmd[2] = (START_PAGE_NUM >> 8) & 0xff;
    buffer_cmd[3] = START_PAGE_NUM & 0xff;

    buffer_cmd[4] = data_xor(buffer_cmd, 4);
    printf("Send erase page_number (%u) and start page (%u)\n", page_number, START_PAGE_NUM);
    usart_buffer_send(buffer_cmd, 5);

    if (SUCCESS != wait_ack()) {
        sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
        return ERROR;
    }

    sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
    return SUCCESS;
}

/*!
    \brief      send the program command
    \param[in]  pro_addr: the address of the target to be programmed
    \param[in]  bin_addr: the address where stored the bin file
    \param[in]  pro_size: the size to be programmed
    \param[out] none
    \retval     ErrStatus
*/
static ErrStatus usart_send_program_command(uint32_t pro_addr, uint32_t bin_addr, uint32_t pro_size)
{
    uint8_t i;

    buffer_cmd[0] = USART_CMD_PROGRAM;
    buffer_cmd[1] = 0xFF - buffer_cmd[0];

    usart_buffer_send(buffer_cmd, 2);

    if (SUCCESS != wait_ack()) {
        return ERROR;
    }
    buffer_cmd[0] = (uint8_t)((pro_addr >> 24) & 0xff);
    buffer_cmd[1] = (uint8_t)((pro_addr >> 16) & 0xff);
    buffer_cmd[2] = (uint8_t)((pro_addr >> 8) & 0xff);
    buffer_cmd[3] = (uint8_t)(pro_addr & 0xff);
    buffer_cmd[4] = data_xor(buffer_cmd, 4);

    usart_buffer_send(buffer_cmd, 5);
    if (SUCCESS != wait_ack()) {
        return ERROR;
    }
    memset(buffer_cmd, 0, sizeof(buffer_cmd));
    buffer_cmd[0] = pro_size - 1;
    for(i = 0; i < pro_size; i = i + 4) {
        *((uint32_t *)&buffer_cmd[1 + i]) = *(uint32_t *)(bin_addr + i);
    }
    buffer_cmd[pro_size + 1] = data_xor(buffer_cmd, pro_size + 1);

    usart_buffer_send(buffer_cmd, pro_size + 2);
    if (SUCCESS != wait_ack()) {
        return ERROR;
    }
    memset(buffer_cmd, 0, sizeof(buffer_cmd));
    return SUCCESS;
}

/*!
    \brief      send the program command
    \param[in]  pro_addr: the address of the target to be programmed
    \param[in]  bin_addr: the address where stored the bin file
    \param[out] none
    \retval     ErrStatus
*/
ErrStatus usart_program_bin(uint32_t pro_addr, uint32_t bin_addr, uint32_t bin_size)
{
    uint32_t i;
    uint32_t pack_num, pack_size, res_size;

    sys_mutex_get((os_mutex_t *)&usart_iap_mutex);

    pack_size = 252;   //4 bytes aligned

    pack_num = bin_size/pack_size;
    res_size = bin_size % pack_size;

    printf("%s:%d pack_num=%u res_size=%02X\n", __func__, __LINE__, pack_num, res_size);

    printf("Send program command\n");
    print_ack = 0;
    for (i = 0; i < pack_num; i++) {
        if (usart_send_program_command(pro_addr + i * pack_size, bin_addr + i * pack_size, pack_size) != SUCCESS) {
            printf("Send program command failed, i=%u\n", i);
            print_ack = 1;
            sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
            return ERROR;
        }
    }
    if (0 != res_size) {
        if (usart_send_program_command(pro_addr + pack_num * pack_size, bin_addr + pack_num * pack_size, res_size) != SUCCESS) {
            printf("Send program command failed, i=%u\n", pack_num);
            print_ack = 1;
            sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
            return ERROR;
        }
    }
    printf("recv ACK\n");

    print_ack = 1;
    sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
    return SUCCESS;
}

ErrStatus usart_send_reset_command(void)
{
    sys_mutex_get((os_mutex_t *)&usart_iap_mutex);

    buffer_cmd[0] = USART_CMD_GO;
    buffer_cmd[1]= 0xFF - buffer_cmd[0];

    printf("Send reset command\n");
    usart_buffer_send(buffer_cmd, 2);

    if (SUCCESS != wait_ack()) {
        sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
        return ERROR;
    }

    buffer_cmd[0] = 0x11;
    buffer_cmd[1] = 0x22;
    buffer_cmd[2] = 0x33;
    buffer_cmd[3] = 0x44;
    buffer_cmd[4] = data_xor(buffer_cmd, 4);

    printf("Send reset address (just for calculate checksum)\n");
    usart_buffer_send(buffer_cmd, 5);

    if (SUCCESS != wait_ack()) {
        sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
        return ERROR;
    }

    sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
    return SUCCESS;
}

ErrStatus usart_send_handshake_command(void)
{
    sys_mutex_get((os_mutex_t *)&usart_iap_mutex);

    buffer_cmd[0] = USART_CMD_HANDSHAKE;

    /* Clear receive buffer and enable IAP mode */
    rx_buffer_clear();
    iap_mode_active = 1;

    printf("Send handshake command\n");
    usart_buffer_send(buffer_cmd, 1);

    ErrStatus result = wait_ack();
    iap_mode_active = 0;  // Command done, disable IAP mode

    sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
    return result;
}

ErrStatus usart_send_run_stop_command(uint8_t flag)
{
    sys_mutex_get((os_mutex_t *)&usart_iap_mutex);

    buffer_cmd[0] = USART_CMD_RUN_STOP;
    buffer_cmd[1] = 0xFF - buffer_cmd[0];

    rx_buffer_clear();
    iap_mode_active = 1;

    usart_buffer_send(buffer_cmd, 2);

    if (SUCCESS != wait_ack()) {
        iap_mode_active = 0;
        sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
        return ERROR;
    }

    buffer_cmd[0] = flag;
    usart_buffer_send(buffer_cmd, 1);

    ErrStatus result = wait_ack();
    iap_mode_active = 0;  // Command done, disable IAP mode

    sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
    return result;
}


ErrStatus usart_send_motors_control(motor_id_t motor_id, bool control)
{
    if (motor_id >= MAX_ID) {
        printf("Invalid motor id (%u)\n", motor_id);
        return ERROR;
    }

    sys_mutex_get((os_mutex_t *)&usart_iap_mutex);

    buffer_cmd[0] = USART_CMD_MOTORS_CONTROL;
    buffer_cmd[1] = 0xFF - buffer_cmd[0];

    /* Clear receive buffer and enable IAP mode */
    rx_buffer_clear();
    iap_mode_active = 1;

    usart_buffer_send(buffer_cmd, 2);

    if (SUCCESS != wait_ack()) {
        iap_mode_active = 0;
        sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
        return ERROR;
    }

    buffer_cmd[0] = (uint8_t)motor_id;
    buffer_cmd[1] = (uint8_t)control;

    usart_buffer_send(buffer_cmd, 2);

    ErrStatus result = wait_ack();
    iap_mode_active = 0;  // Command done, disable IAP mode

    sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
    return result;
}

ErrStatus usart_motor_status_get(motor_id_t motor_id, uint8_t *status)
{
    if (motor_id >= MAX_ID) {
        printf("Invalid motor id (%u)\n", motor_id);
        return ERROR;
    }

    sys_mutex_get((os_mutex_t *)&usart_iap_mutex);

    buffer_cmd[0] = USART_CMD_MOTORS_STATUS_GET;
    buffer_cmd[1] = 0xFF - buffer_cmd[0];

    /* Clear receive buffer and enable IAP mode */
    rx_buffer_clear();
    iap_mode_active = 1;

    usart_buffer_send(buffer_cmd, 2);

    if (SUCCESS != wait_ack()) {
        iap_mode_active = 0;
        sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
        return ERROR;
    }

    buffer_cmd[0] = (uint8_t)motor_id;

    usart_buffer_send(buffer_cmd, 1);

    ErrStatus result = wait_byte_ack(status);
    iap_mode_active = 0;  // Command done, disable IAP mode

    sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
    return result;
}


ErrStatus usart_motor_current_get(motor_id_t motor_id, uint32_t *current)
{
    if (motor_id >= MAX_ID) {
        printf("Invalid motor id (%u)\n", motor_id);
        return ERROR;
    }

    sys_mutex_get((os_mutex_t *)&usart_iap_mutex);

    buffer_cmd[0] = USART_CMD_MOTORS_CURRENT_GET;
    buffer_cmd[1] = 0xFF - buffer_cmd[0];

    /* Clear receive buffer and enable IAP mode */
    rx_buffer_clear();
    iap_mode_active = 1;

    usart_buffer_send(buffer_cmd, 2);

    if (SUCCESS != wait_ack()) {
        iap_mode_active = 0;
        sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
        return ERROR;
    }

    buffer_cmd[0] = (uint8_t)motor_id;

    usart_buffer_send(buffer_cmd, 1);

    ErrStatus result = wait_word_ack(current);
    iap_mode_active = 0;  // Command done, disable IAP mode

    sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
    return result;
}

ErrStatus usart_servo_angle_get(uint32_t *angle)
{
    sys_mutex_get((os_mutex_t *)&usart_iap_mutex);

    buffer_cmd[0] = USART_CMD_SERVO_ANGLE_GET;
    buffer_cmd[1] = 0xFF - buffer_cmd[0];

    /* Clear receive buffer and enable IAP mode */
    rx_buffer_clear();
    iap_mode_active = 1;

    usart_buffer_send(buffer_cmd, 2);

    ErrStatus result = wait_word_ack(angle);
    iap_mode_active = 0;  // Command done, disable IAP mode

    sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
    return result;
}

ErrStatus usart_fan_speed_get(uint32_t *speed)
{
    sys_mutex_get((os_mutex_t *)&usart_iap_mutex);

    buffer_cmd[0] = USART_CMD_FAN_SPEED_GET;
    buffer_cmd[1] = 0xFF - buffer_cmd[0];

    /* Clear receive buffer and enable IAP mode */
    rx_buffer_clear();
    iap_mode_active = 1;

    usart_buffer_send(buffer_cmd, 2);

    ErrStatus result = wait_word_ack(speed);
    iap_mode_active = 0;  // Command done, disable IAP mode

    sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
    return result;
}

ErrStatus usart_work_mode_set(uint8_t mode)
{
    if (mode >= MODE_MAX) {
        printf("Invalid work mode (%u)\n", mode);
        return ERROR;
    }

    sys_mutex_get((os_mutex_t *)&usart_iap_mutex);

    buffer_cmd[0] = USART_CMD_WORK_MODE_SET;
    buffer_cmd[1] = 0xFF - buffer_cmd[0];

    /* Clear receive buffer and enable IAP mode */
    rx_buffer_clear();
    iap_mode_active = 1;

    usart_buffer_send(buffer_cmd, 2);

    if (SUCCESS != wait_ack()) {
        iap_mode_active = 0;
        sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
        return ERROR;
    }

    buffer_cmd[0] = mode;

    usart_buffer_send(buffer_cmd, 1);

    ErrStatus result = wait_ack();
    iap_mode_active = 0;  // Command done, disable IAP mode

    sys_mutex_put((os_mutex_t *)&usart_iap_mutex);
    return result;
}
