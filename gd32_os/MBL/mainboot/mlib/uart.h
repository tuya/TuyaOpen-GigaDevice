#ifndef __UART_H__
#define __UART_H__

#include "gd32vw55x.h"
#include "gd32vw55x_usart.h"

#define DEFAULT_BAUDRATE                 115200U

extern uint32_t log_uart;
extern uint32_t xmodem_uart;

void log_uart_init(void);

void uart_config(uint32_t usart_periph, uint32_t baud_rate);
void uart_config_all(uint32_t baud_rate);

void uart_deinit(uint32_t uart);
char uart_readable(uint32_t uart);
char uart_writable(uint32_t uart);
char uart_getc(uint32_t uart);
void uart_putc(uint32_t uart, char c);
void uart_putdata(uint32_t uart, uint8_t* buf, uint32_t len);
char uart_getc_with_timeout(uint32_t uart, char *pch, uint32_t timeout);
void uart_clean_rx(uint32_t uart);
void uart_wait_txcomplete(uint32_t uart, uint32_t wait_times);
int auto_detect_uart_internal(uint32_t timeout);

#endif  // __UART_H__
