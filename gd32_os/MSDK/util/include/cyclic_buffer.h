/*!
    \file    cyclic_buffer.h
    \brief   Header file for cyclic buffer.

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

#ifndef __CYCLIC_BUFFER_H__
#define __CYCLIC_BUFFER_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cyclic_buf
{
    uint16_t read_idx;
    uint16_t write_idx;
    uint16_t len;
    uint8_t  *buf;
} cyclic_buf_t;

bool cyclic_buf_init(cyclic_buf_t *cyclic, uint16_t len);
uint16_t cyclic_buf_room(cyclic_buf_t *cyclic);
uint16_t cyclic_buf_count(cyclic_buf_t *cyclic);
bool cyclic_buf_peek(cyclic_buf_t *cyclic, uint8_t *buf, uint16_t len);
bool cyclic_buf_read(cyclic_buf_t *cyclic, uint8_t *buf, uint16_t len);
bool cyclic_buf_drop(cyclic_buf_t *cyclic, uint16_t len);
bool cyclic_buf_clear(cyclic_buf_t *cyclic);
bool cyclic_buf_write(cyclic_buf_t *cyclic, uint8_t *buf, uint16_t len);
void cyclic_buf_free(cyclic_buf_t *cyclic);

#ifdef __cplusplus
}
#endif

#endif /* __CYCLIC_BUFFER_H__ */
