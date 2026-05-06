/*!
    \file    rom_uart.c
    \brief   UART APIs

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
#include "uart.h"
#include "systick.h"

uint32_t xmodem_uart = UART2;

/* Timeout for auto UART detection (in microseconds) */
#ifndef AUTO_DETECT_UART_TIMEOUT_US
#define AUTO_DETECT_UART_TIMEOUT_US 100000U  /* 100ms */
#endif

void uart_config(uint32_t usart_periph, uint32_t baud_rate)
{
    if (usart_periph == UART2) {
        rcu_periph_clock_enable(RCU_UART2);
        rcu_periph_clock_enable(RCU_GPIOA);
        gpio_af_set(GPIOA, GPIO_AF_10, GPIO_PIN_6);  // UART2 TX
        gpio_af_set(GPIOA, GPIO_AF_8, GPIO_PIN_7);   // UART2 RX
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_6);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_6);
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_7);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_7);
    } else if (usart_periph == UART1) {
        rcu_periph_clock_enable(RCU_GPIOA);
        rcu_periph_clock_enable(RCU_UART1);
        gpio_af_set(GPIOA, GPIO_AF_0, GPIO_PIN_4);  // UART1 TX
        gpio_af_set(GPIOA, GPIO_AF_0, GPIO_PIN_5);  // UART1 RX
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_4);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_4);
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_5);
    } else if (usart_periph == USART0) {
        rcu_periph_clock_enable(RCU_USART0);
        rcu_periph_clock_enable(RCU_GPIOA);
        rcu_periph_clock_enable(RCU_GPIOB);
        gpio_af_set(GPIOB, GPIO_AF_8, GPIO_PIN_15);  // USART0 TX
        gpio_af_set(GPIOA, GPIO_AF_2, GPIO_PIN_8);   // USART0 RX
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_8);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_8);
        gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_15);
        gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_15);
    } else {
        return;
    }
//    setvbuf(stdout, NULL, _IONBF, 0);

    usart_deinit(usart_periph);
    usart_baudrate_set(usart_periph, baud_rate);
    usart_receive_config(usart_periph, USART_RECEIVE_ENABLE);
    usart_transmit_config(usart_periph, USART_TRANSMIT_ENABLE);
    //usart_interrupt_enable(usart_periph, USART_INT_RBNE);
    usart_receive_fifo_enable(usart_periph);

    usart_enable(usart_periph);
}

void uart_config_all(uint32_t baud_rate)
{
    uart_config(USART0, baud_rate);
    uart_config(UART1, baud_rate);
    uart_config(UART2, baud_rate);
}

void uart_deinit(uint32_t uart)
{
    usart_deinit(uart);
}

char uart_readable(uint32_t uart)
{
    return (RESET != usart_flag_get(uart, USART_FLAG_RBNE));
}

char uart_writable(uint32_t uart)
{
    return (SET == usart_flag_get(uart, USART_FLAG_TBE));
}

char uart_getc(uint32_t uart)
{
    char ch = usart_data_receive(uart);
    if (RESET != usart_flag_get(uart, USART_FLAG_ORERR)) {
        printf("ORERR\r\n");
        usart_flag_clear(uart, USART_FLAG_ORERR);
    }

    return ch;
}

void uart_putc(uint32_t uart, char c)
{
    usart_data_transmit(uart, (uint8_t)c);
    while (SET != usart_flag_get(uart, USART_FLAG_TBE));
}

void uart_putdata(uint32_t uart, uint8_t* buf, uint32_t len)
{
    uint32_t cnt = 0;

    while (uart_writable(uart) == 0);
    for(cnt = 0; cnt < len; cnt++) {
        uart_putc(uart, *(buf + cnt));
    }
}

char uart_getc_with_timeout(uint32_t uart, char *pch, uint32_t timeout)
{
    do {
        if (uart_readable(uart)) {
            *pch = uart_getc(uart);
            return 0;
        }
        timeout--;
    } while (timeout != 0);
    return 1;
}

void uart_clean_rx(uint32_t uart)
{
    while (uart_readable(uart)) {
        uart_getc(uart);
    }
}

void uart_wait_txcomplete(uint32_t uart, uint32_t wait_times)
{
    uint32_t times = 0;

    /* Wait for Uart print out */
    while(1) {
        if (SET == usart_flag_get(uart, USART_FLAG_TC)){
            break;
        }

        sys_udelay(50);

        if (times++ > wait_times)
            break;
    }
}

int auto_detect_uart_internal(uint32_t timeout)
{
    int uart_periph = -1;
    char ch;
    uint32_t waited_us = 0;

    while (waited_us < timeout || timeout == 0xFFFFFFFF) {
        /* USART0 */
        if (REG32(USART0 + USART_STAT_REG_OFFSET) & BIT(5)) {  /* uart_readable(USART0) */
            ch = uart_getc(USART0);
            if (ch == 'u') {
                uart_periph = USART0;
                break;
            }
        }
        /* UART1 */
        if (REG32(UART1 + USART_STAT_REG_OFFSET) & BIT(5)) {  /* uart_readable(UART1) */
            ch = uart_getc(UART1);
            if (ch == 'u') {
                uart_periph = UART1;
                break;
            }
        }
        /* UART2 */
        if (REG32(UART2 + USART_STAT_REG_OFFSET) & BIT(5)) {  /* uart_readable(UART2) */
            ch = uart_getc(UART2);
            if (ch == 'u') {
                uart_periph = UART2;
                break;
            }
        }

        sys_udelay(5);      /* small delay to avoid tight spin */
        waited_us += 5;
    }
    return uart_periph;     /* -1 if timeout */
}
