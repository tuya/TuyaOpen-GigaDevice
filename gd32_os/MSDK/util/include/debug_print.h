/*!
    \file    debug_print.h
    \brief   Header file for debug print.

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

#ifndef __DEBUG_PRINT_H
#define __DEBUG_PRINT_H

#include <stdarg.h>
#include <stdint.h>

#define ERROR_LEVEL         1
#define WARNING_LEVEL       2
#define INFO_LEVEL          3

#define ETHIF_LEVEL         INFO_LEVEL
#define MAIN_LEVEL          INFO_LEVEL
#define HTTPD_LEVEL         INFO_LEVEL
#define TCP_OUT_LEVEL       INFO_LEVEL
#define SYS_ARCH_LEVEL      INFO_LEVEL

#define MAC_ARG(a)          ((a)[0] & 0xFF), ((a)[0] >> 8), ((a)[1] & 0xFF), ((a)[1] >> 8), ((a)[2] & 0xFF), ((a)[2] >> 8)
#define MAC_ARG_UINT8(a)    (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MAC_FMT             "%02x:%02x:%02x:%02x:%02x:%02x"
#define IP_FMT              "%d.%d.%d.%d"
#define IP_ARG(a)           ((a) & 0xFF), (((a) >> 8) & 0xFF), (((a) >> 16) & 0xFF), ((a) >> 24)

int co_printf(const char *format, ...);
int co_snprintf(char *out, int space, const char *format, ...);
int print_buffer(unsigned long addr, void *data, unsigned long width, unsigned long count, unsigned long linelen);
int print(char **out, const char *format, va_list args, int space);

/**
 ****************************************************************************************
 * @brief Execute a pseudo snprintf function
 *
 * @param[out] buffer  Output buffer
 * @param[in]  size    Size of the output buffer
 * @param[in]  fmt     Format string
 *
 * @return Upon successful return, returns the number of characters printed (excluding the
 * null byte used to end output to strings). If the output was truncated due to the size
 * limit, then the return value is the number of characters (excluding the terminating
 * null byte) which would have been written to the final string if enough space had been
 * available. Thus, a return value of size or more means that the output was truncated.
 *
 ****************************************************************************************
 */
uint32_t dbg_snprintf(char *buffer, uint32_t size, const char *fmt, ...);

/**
 ****************************************************************************************
 * @brief Execute a pseudo vsnprintf function, with extra offset.
 *
 * @param[out] buffer  Output buffer
 * @param[in]  size    Size of the output buffer
 * @param[in]  offset  Offset of final string at which the writing in buffer should start
 * @param[in]  fmt     Format string
 * @param[in]  args    Variable list of arguments
 *
 * @return Upon successful return, returns the number of characters printed (excluding the
 * null byte used to end output to strings). If the output was truncated due to the size
 * limit, then the return value is the number of characters (excluding the terminating
 * null byte) which would have been written to the final string if enough space had been
 * available. Thus, a return value of size or more means that the output was truncated.
 *
 ****************************************************************************************
 */
uint32_t dbg_vsnprintf_offset(char *buffer, uint32_t size, uint32_t offset,
                              const char *fmt, va_list args);

/**
 ****************************************************************************************
 * @brief Execute a pseudo vsnprintf function
 *
 * @param[out] buf     Output buffer
 * @param[in]  size    Size of the output buffer
 * @param[in]  fmt     Format string
 * @param[in]  args    Variable list of arguments
 *
 * @return Upon successful return, returns the number of characters printed (excluding the
 * null byte used to end output to strings). If the output was truncated due to the size
 * limit, then the return value is the number of characters (excluding the terminating
 * null byte) which would have been written to the final string if enough space had been
 * available. Thus, a return value of size or more means that the output was truncated.
 *
 ****************************************************************************************
 */
#define dbg_vsnprintf(buf, size, fmt, args) dbg_vsnprintf_offset(buf, size, 0, fmt, args)

void debug_print_dump_data(char *title, char *mem, int mem_size);

int str2hex(char *input, int input_len, unsigned char *output, int output_len);

#define DEBUG_ASSERT(expr)  do {\
                             if (!(expr))\
                                 {co_printf("%s:%u ASSERT: "#expr"\n", __func__, __LINE__);}\
                            } while(0)

#endif /* __DEBUG_PRINT_H */
