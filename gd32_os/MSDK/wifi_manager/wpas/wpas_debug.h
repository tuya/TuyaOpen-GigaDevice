/*!
    \file    wpas_debug.h
    \brief   Header file for wpas debug.

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

#ifndef _WPAS_DEBUG_H_
#define _WPAS_DEBUG_H_

#include "stdio.h"

#define wpa_info(fmt, ...)          \
    do {                                        \
        if (wpas_info_print) {      \
            printf(fmt, ## __VA_ARGS__);     \
        }                                       \
    } while (0)

#ifdef CONFIG_WPA_DEBUG
#define wpa_printf  printf
#else /* CONFIG_WPA_DEBUG */
#define wpa_printf(...)
#endif  /* CONFIG_WPA_DEBUG */

#ifdef CONFIG_WPA_DATA_DUMP
#define wpa_hex_dump(title, buf, len) \
    do { \
        int ii; \
        printf("\r\n=== %s (len=%d)===", title, (int)(len)); \
        for (ii = 0; ii < (len); ii++) { \
            if (ii % 16 == 0) \
                printf("\r\n"); \
            printf("%02x ", *((uint8_t*)buf + ii)); \
        } \
        printf("\r\n");\
    } while(0);
#else /* CONFIG_WPA_DATA_DUMP */
#define wpa_hex_dump(...)
#endif /* CONFIG_WPA_DATA_DUMP */

#define WPA_ASSERT(expr) \
    do {\
        if (!(expr)) {\
            printf("%s:%d ASSERT: "#expr"\n", __func__, __LINE__);\
            while (1);\
        }\
    } while(0)

extern uint8_t wpas_info_print;

void wpas_info_print_close(void);
void wpas_info_print_open(void);

#endif  /* _WPAS_DEBUG_H_ */
