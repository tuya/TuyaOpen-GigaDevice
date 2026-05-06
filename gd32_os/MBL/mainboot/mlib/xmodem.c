/*!
    \file    xmodem.c
    \brief   UART xmodem protocol

    \version 2022-06-06, V1.0.0
*/

/*
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

#include <stdio.h>
#include <string.h>
#include "platform_def.h"
#include "uart_config.h"
#include "gd32vw55x_usart.h"
#include "rom_export_mbedtls.h"
#include "rom_flash.h"
#include "rom_trace.h"
#include "xmodem.h"
#include "uart.h"
#include "systick.h"

// #define XMODEM_DEBUG_EN
#ifdef XMODEM_DEBUG_EN
#define dbg(...)        printf(__VA_ARGS__);
#else
#define dbg(...)    do {} while (0)
#endif

static xmodem_ctrl_t xmodem_ctrl;
extern uint32_t xmodem_uart;

void xmodem_led_init(void)
{
    /* enable clock */
    rcu_periph_clock_enable(RCU_GPIOB);

    /* configure red led */
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_11);
    //gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_11);

    /* configure green led */
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_12);
    //gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_12);

    /* configure blue led */
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_13);
    //gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_13);
}

void xmodem_led_on(int led)
{
    if (led == LED_RED) {
        gpio_bit_set(GPIOB, GPIO_PIN_11);
    } else if (led == LED_GREEN) {
        gpio_bit_set(GPIOB, GPIO_PIN_12);
    } else if (led == LED_BLUE) {
        gpio_bit_set(GPIOB, GPIO_PIN_13);
    }
}

void xmodem_led_off(int led)
{
    if (led == LED_RED) {
        gpio_bit_reset(GPIOB, GPIO_PIN_11);
    } else if (led == LED_GREEN) {
        gpio_bit_reset(GPIOB, GPIO_PIN_12);
    } else if (led == LED_BLUE) {
        gpio_bit_reset(GPIOB, GPIO_PIN_13);
    }
}

void xmodem_gpio_deinit(void)
{
    gpio_deinit(GPIOA);
    gpio_deinit(GPIOB);
    gpio_deinit(GPIOC);

}

static int auto_detect_uart(uint32_t timeout)
{
    uart_config_all(115200);

    return auto_detect_uart_internal(timeout);
}

static void xmodem_write(uint8_t *ptr, uint32_t wr_offset, uint32_t frame_size)
{
    uint32_t index = 0;
    uint8_t location = wr_offset >> 24;
    uint32_t offset = (wr_offset & 0xFFFFFF);

    if (location == XMODEM_WRITE_FLASH) {
        flash_write_fast(offset, (const void *)ptr, frame_size);
    } else if(location == XMODEM_WRITE_RAM) {
        for(index = 0; index < 256; index++) {
            *((uint32_t *)(wr_offset + index * 4)) = (*((uint32_t *)(ptr + index * 4)));
        }
    }
}

static uint32_t xmodem_inquiry(uint8_t code)
{
    if (code != ACK) {
        sys_udelay(1000);
    }

    uart_putc(xmodem_uart, code);

    return _TRUE;
}

static uint32_t xmodem_get_opcode(xmodem_ctrl_t *p_xmctrl)
{
    char ch = 0;
    uint32_t status = XMODEM_OK;

    if (uart_getc_with_timeout(xmodem_uart, &ch, WAIT_FRAME_TIME)) {
        p_xmctrl->b_finished = _TRUE;
        status =  XMODEM_TIMEOUT;
        dbg("Xmodem: wait opcode timeout\r\n");
        return status;
    }

    ch = ch & 0xff;

    switch(ch)
    {
        case CAN :
            p_xmctrl->b_finished = _TRUE;
            status = XMODEM_CANCEL;
            break;
        case EOT :
            p_xmctrl->b_finished = _TRUE;
            xmodem_inquiry(ACK);
            status = XMODEM_COMPLETE;
            break;
        case SOH :
            p_xmctrl->expected_sz =  FRAME_SIZE;
            break;
        case STX :
            p_xmctrl->expected_sz =  p_xmctrl->frame_size;
            break;
        case ESC :
            p_xmctrl->b_finished = _TRUE;
            status = XMODEM_ABORT;
            break;
        default :
            //while (uart_getc_with_timeout(xmodem_uart, &tmp, WAIT_CHAR_TIME) == 0);
            p_xmctrl->b_finished = _TRUE;
            status = XMODEM_TIMEOUT;
        break;
    }

    return status;
}

static uint32_t xmodem_get_payload(xmodem_ctrl_t *p_xmctrl)
{
    uint8_t frame_no;
    uint8_t frame_no_bar;
    uint8_t is_duplicate;
    char uch = 0;
    char uchk;
    uint16_t summation;
    uint32_t j;
    uint32_t wr_offset = 0;
    uint32_t status = XMODEM_OK;
    uint32_t frm_err = 0;

    if (uart_getc_with_timeout(xmodem_uart, (char *)(&frame_no), WAIT_CHAR_TIME)) {
        p_xmctrl->b_finished = _TRUE;
        status = XMODEM_TIMEOUT;
        dbg("Xmodem: get frame seq timeout\r\n");
        return status;
    }

    /* check invert number */
    if (uart_getc_with_timeout(xmodem_uart, (char *)(&frame_no_bar), WAIT_CHAR_TIME)) {
        p_xmctrl->b_finished = _TRUE;
        status = XMODEM_TIMEOUT;
        dbg("Xmodem: get frame seq invert timeout\r\n");
        return status;
    }

    status = XMODEM_OK;

    if ((((~frame_no)&0xff)  ^ frame_no_bar) != 0) {
        status = XMODEM_NAK;
        frm_err = 1;
    }

    is_duplicate = 0;
    if (frame_no < p_xmctrl->cur_frm_seq) {
        /* re-transmit packets, just send ACK and ignore it */
        is_duplicate = 1;
    } else if (frame_no != p_xmctrl->cur_frm_seq) {
        status = XMODEM_NAK;
        frm_err = 2;
    }

    /* get data */
    summation = 0;
    for (j = 0; j < p_xmctrl->expected_sz; j++) {
        if (uart_getc_with_timeout(xmodem_uart, (char*)(xmodem_ctrl.frm_buf + j), WAIT_CHAR_TIME)) {
            p_xmctrl->b_finished = _TRUE;
            status = XMODEM_TIMEOUT;
            dbg("Xmodem: get data timeout(expected %d, actual %d)\r\n", p_xmctrl->expected_sz, j);
            return status;
        }
        summation += *(xmodem_ctrl.frm_buf + j);
    }

    /* CRC check */
    if (uart_getc_with_timeout(xmodem_uart, &uch, WAIT_CHAR_TIME)) {
        p_xmctrl->b_finished = _TRUE;
        status = XMODEM_TIMEOUT;
        return status;
    }

    uchk = summation & 0xff;
    if (uchk != uch) {
        status = XMODEM_NAK;
        frm_err = 3;
    } else {
        if (!is_duplicate) {
            dbg("RX %d \t", p_xmctrl->expected_sz);
            wr_offset = *((uint32_t*)xmodem_ctrl.frm_buf); /* 4byte load address */
            xmodem_write(xmodem_ctrl.frm_buf + 4, wr_offset, p_xmctrl->expected_sz - 4);
        }

        /* somehow the xmodem sender "TeraTerm" will send extra junk bytes if we start the xmodem */
        /* receiver before the xmodem sender start. So, we need to drop those extra data */
        uart_clean_rx(xmodem_uart);
    }

    if (status == XMODEM_OK) {
        if (is_duplicate) {
            /* duplicated frame */
            status = XMODEM_NAK;
            dbg("Xmodem: duplicate\r\n");
        }
        p_xmctrl->b_finished = _TRUE;
        xmodem_inquiry(ACK);
    } else if(status == XMODEM_NAK){
        dbg("Xmodem: frame error(%d)\r\n", frm_err);
        xmodem_inquiry(NAK);
        p_xmctrl->b_finished = _TRUE;
    }

    return status;
}

static uint32_t xmodem_rx_frame(xmodem_ctrl_t *p_xmctrl)
{
    uint32_t status = 0;

    p_xmctrl->expected_sz = 1;
    p_xmctrl->b_finished = _FALSE;


    /* get xmodem opcode */
    status = xmodem_get_opcode(p_xmctrl);

    /* get following bytes */
    while (p_xmctrl->b_finished == _FALSE) {
        status = xmodem_get_payload(p_xmctrl);
    }

    /* update rx index */
    if (status == XMODEM_OK) {
        p_xmctrl->cur_frm_seq = (p_xmctrl->cur_frm_seq + 1) & 0xff;
    }

    return status;
}

static void xmodem_handshake(void)
{
    char c;
    volatile int timeout = WAIT_HANDSHAKE_TIME;

    uart_clean_rx(xmodem_uart);
Retry:
    timeout = WAIT_HANDSHAKE_TIME;
    while (timeout-- > 0) {
        if (uart_readable(xmodem_uart)) {
            c = uart_getc(xmodem_uart);
            // dbg("%x\n", c);
            switch(c){
                case BAUDSET:
                {
                    uint8_t temp[4];
                    uint32_t baud_rate;
                    int i;
                    memset((void *)temp, 0, sizeof(temp));
                    for(i = 0; i < 4; i++){
                        if(uart_getc_with_timeout(xmodem_uart, (char *)(&temp[i]), WAIT_HANDSHAKE_TIME)){
                            dbg("Xmodem: BAUDSET timeout, goto retry.\r\n");
                            goto End;
                        }
                    }
#ifdef XMODEM_DUMP_EN
                    dbg("RX: ");
                    for (i = 0; i < 4; i++)
                        dbg("%02X ", temp[i]);
                    dbg("\r\n");
#endif
                    baud_rate = temp[0] + (temp[1] << 8) + (temp[2] << 16) + (temp[3] << 16);
                    dbg("Xmodem: set baudrate %d\r\n", baud_rate);
                    uart_putc(xmodem_uart, ACK);
                    uart_wait_txcomplete(xmodem_uart, 100000);
                    uart_config(xmodem_uart, baud_rate);
                    timeout = WAIT_HANDSHAKE_TIME;
                }
                break;

                case FRMSIZE:
                {
                    uint8_t frm_sz_page;
                    if(uart_getc_with_timeout(xmodem_uart, (char *)(&frm_sz_page), WAIT_HANDSHAKE_TIME)){
                        dbg("Xmodem: FRMSIZE timeout, goto retry.\r\n");
                        goto End;
                    }
                    xmodem_ctrl.frame_size = (frm_sz_page << 8) + 4;
                    dbg("Xmodem: set frame size %d(%d).\r\n", xmodem_ctrl.frame_size, frm_sz_page);
                    uart_putc(xmodem_uart, ACK);
                    return;
                }
                break;

                case FLERASE:
                {
                    uint8_t temp[8];
                    __IO uint32_t addr;
                    uint32_t size;
                    uint8_t bchip_erase;
                    int i;

                    memset(temp, 0xAA, sizeof(temp));
                    for(i = 0; i < 6; i++){
                        if(uart_getc_with_timeout(xmodem_uart, (char *)(&temp[i]), WAIT_HANDSHAKE_TIME)){
                            dbg("Xmodem: FLERASE timeout, goto retry.\r\n");
                            goto End;
                        }
                    }
#ifdef XMODEM_DUMP_EN
                    dbg("RX: ");
                    for (i = 0; i < 6; i++)
                        dbg("%02X ", temp[i]);
                    dbg("\r\n");
#endif
                    bchip_erase = temp[0];
                    addr = temp[1] + (temp[2] << 8) + (temp[3] << 16);
                    size = temp[4] + (temp[5] << 8);
                    dbg("Xmodem: erase 0x%x, 0x%x, chip erase %d\r\n", addr, size, bchip_erase);

                    if (bchip_erase) {
                        flash_erase_chip();
                    } else {
                        flash_erase(addr, (size << 12));
                    }
                    uart_putc(xmodem_uart, ACK);
                    timeout = WAIT_HANDSHAKE_TIME;
                }
                break;

                case CPIDGET:
                {
                    uint32_t m_pid = 0x0;

                    m_pid =  *(__IO uint32_t *)0x40022100;
                    dbg("Xmodem: get chip id %x\r\n", m_pid);
                    uart_putdata(xmodem_uart, (uint8_t*) &m_pid, 4);
                    timeout = WAIT_HANDSHAKE_TIME;
                }
                break;

                case SYNCREQ:
                {
                    uart_putc(xmodem_uart, SYNCACK);
                    timeout = WAIT_HANDSHAKE_TIME;
                }
                break;
                default:
                    break;
            }
        }
    }
End:
    dbg("Xmodem: timeout, goto retry.\r\n");
    uart_config(xmodem_uart, DEFAULT_BAUDRATE);
    goto Retry;
}

static int xmodem_img_download(void)
{
    uint32_t status;
    uint32_t rxlen = 0;
    uint32_t finished;

    xmodem_ctrl.cur_frm_seq = 1;

    finished = _FALSE;
    while (finished == _FALSE) {
        status = xmodem_rx_frame(&xmodem_ctrl);

        switch (status) {
            case XMODEM_OK :
                rxlen += xmodem_ctrl.expected_sz;
                break;

            case XMODEM_TIMEOUT :
                /* Clear Rx FIFO and NAK, Xmodem will Try again */
                //dbg("timeout, goto retry\r\n");
                uart_clean_rx(xmodem_uart);
                xmodem_inquiry(NAK);
                finished = _TRUE;
                rxlen = 0;
                break;

            case XMODEM_COMPLETE :
                status = XMODEM_OK;
                finished = _TRUE;
                break;

            case XMODEM_NAK:
                break;

            case XMODEM_ABORT :
                status = XMODEM_ABORT;
                rxlen = 0;
                break;
            default :
                xmodem_inquiry(CAN);
                finished = _TRUE;
                rxlen = 0;
                break;
        }
    }
    return status;
}

static int xmodem_done_check(void)
{
    int ret = -1;
    volatile uint32_t timeout = 2000000;

    while (timeout-- > 0) {
        if (uart_readable(xmodem_uart)) {
            char c = uart_getc(xmodem_uart);
            // dbg("Xmodem: c = 0x%2x\r\n", c);
            switch(c){
                case IMGCHCK:
                {
                    __IO uint32_t addr;
                    uint32_t size;
                    uint8_t type; //0: sum, 1: crc, 2: sha256
                    __IO uint32_t read_data;
                    uint32_t sz_align, remain;
                    uint32_t checksum;
                    uint8_t hash[32];
                    uint8_t temp[8];
                    uint32_t i;

                    memset(temp, 0, sizeof(temp));
                    for(i = 0; i < 7; i++){
                        if(uart_getc_with_timeout(xmodem_uart,(char *)(&temp[i]), WAIT_HANDSHAKE_TIME)){
                            ret = -2;
                            goto Error;
                        }
                    }
                    addr = temp[0] + (temp[1] << 8) + (temp[2] << 16);
                    size = temp[3] + (temp[4] << 8) + (temp[5] << 16);
                    type = temp[6];
                    dbg("Xmodem: check 0x%x, 0x%x, %d\r\n", addr, size, type);
                    if ((addr + size) > flash_total_size()) {
                        ret = -3;
                        goto Error;
                    }

                    /* disable hw RTDEC */
                    flash_nodec_config(0, 0, 0x3FF);

                    remain = (size & 0x03);
                    sz_align = size - remain;
                    if (type == 0) {
                        checksum = 0;
                        for(i = 0; i < sz_align; i += 4){
                            read_data = *(uint32_t *)(FLASH_BASE + addr + i);
                            checksum += read_data;
                        }
                        if (remain) {
                            read_data = *(uint32_t *)(FLASH_BASE + addr + i);
                            checksum += ((read_data  << (8 * (4 - remain))) >> (8 * (4 - remain)));
                        }
                        uart_putdata(xmodem_uart, (uint8_t*) &checksum, 4);
                    } else if (type == 1) {
                        crc_data_register_reset();
                        checksum = crc_block_data_calculate((uint32_t *)(FLASH_BASE + addr), size / 4);
                        if (remain) {
                            read_data = *(uint32_t *)(FLASH_BASE + addr + sz_align);
                            read_data = ((read_data  << (8 * (4 - remain))) >> (8 * (4 - remain)));
                            checksum = crc_single_data_calculate(read_data);
                        }
                        uart_putdata(xmodem_uart, (uint8_t*) &checksum, 4);
                    } else if (type == 2) {
                        hau_hash_sha_256((unsigned char *)(FLASH_BASE + addr), size, hash);
                        uart_putdata(xmodem_uart, hash, 32);
                    } else {
                        uart_putc(xmodem_uart, NAK);
                        ret = -4;
                        goto Error;
                    }

                    /* backup hw RTDEC */
                    flash_nodec_config(0, 0x3FF, 0);

                    timeout = 2000000;
                    break;
                }

                case IMGDONE:
                {
                    uint8_t result;
                    if(uart_getc_with_timeout(xmodem_uart,(char *)&result, WAIT_HANDSHAKE_TIME)){
                        ret = -5;
                        goto Error;
                    }
                    if (1 == result) {
                        dbg("Xmodem: verified OK.\r\n");
                        uart_putc(xmodem_uart, ACK);
                        return 0;
                    } else {
                        dbg("Xmodem: verified Fail.\r\n");
                        ret = -6;
                        goto Error;
                    }
                }
                default:
                    break;
            }
        }
    }
Error:
    dbg("Xmodem: Check failed(%d).\r\n", ret);
    return ret;
}

void xmodem_start(uint32_t timeout)
{
    int status;
    int usart_periph = -1;

    systick_init();
    xmodem_led_init();
Retry:
    xmodem_led_off(LED_BLUE);
    xmodem_led_off(LED_RED);
    xmodem_led_on(LED_GREEN);

    /* 1. Auto Detect UART */
    usart_periph = auto_detect_uart(timeout);
    if(usart_periph < 0) {
        xmodem_gpio_deinit();
        return;
    }
    xmodem_uart = usart_periph;

    dbg("Xmodem: xmodem uart %x.\r\n", xmodem_uart);

    xmodem_led_off(LED_GREEN);
    xmodem_led_on(LED_BLUE);
    /* 2. inform APP to start XMODEM */
    uart_putc(xmodem_uart, SYNCACK);

    /* 3. Baudrate handshake and check OK */
    xmodem_handshake();

    /* 4. Receive frames and write to FLASH */
    status = xmodem_img_download();
    if (status != XMODEM_OK) {
        xmodem_led_on(LED_RED);
        sys_udelay(1000000);
        xmodem_led_off(LED_RED);
        goto Retry;
    }

    /* 5. Check the firmware in FLASH and reset */
    if (xmodem_done_check() < 0) {
        xmodem_led_on(LED_RED);
        sys_udelay(1000000);
        xmodem_led_off(LED_RED);
        goto Retry;
    }

    xmodem_led_off(LED_BLUE);
    xmodem_gpio_deinit();
    SysTimer_SoftwareReset();

    return;
}
