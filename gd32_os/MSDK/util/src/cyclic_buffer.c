/*!
    \file    cyclic_buffer.c
    \brief   cyclic buffer for GD32VW55x SDK.

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

#include <stdint.h>
#include <string.h>
#include "cyclic_buffer.h"
#include "wrapper_os.h"

bool cyclic_buf_init(cyclic_buf_t *cyclic, uint16_t len)
{
    if (!cyclic)
    {
        return false;
    }

    cyclic->read_idx = 0;
    cyclic->write_idx = 0;
    cyclic->len  = len;
    cyclic->buf  = sys_zalloc(cyclic->len);

    if (!cyclic->buf)
    {
        return false;
    }

    return true;
}

void cyclic_buf_free(cyclic_buf_t *cyclic)
{
    if (!cyclic)
    {
        return;
    }

    if (cyclic->buf)
    {
        sys_mfree(cyclic->buf);
    }

    cyclic->read_idx = 0;
    cyclic->write_idx = 0;
    cyclic->len  = 0;
    cyclic->buf  = NULL;
}

uint16_t cyclic_buf_count(cyclic_buf_t *cyclic)
{
    if (!cyclic || !cyclic->buf)
    {
        return 0;
    }

    return ((cyclic->write_idx + cyclic->len - cyclic->read_idx) % cyclic->len);
}

uint16_t cyclic_buf_room(cyclic_buf_t *cyclic)
{
    if (!cyclic || !cyclic->buf)
    {
        return 0;
    }

    return ((cyclic->read_idx + cyclic->len - (cyclic->write_idx + 1)) % cyclic->len);
}

bool cyclic_buf_peek(cyclic_buf_t *cyclic, uint8_t *buf, uint16_t len)
{
    if (!cyclic || !cyclic->buf)
    {
        return false;
    }

    if (len > cyclic_buf_count(cyclic))
    {
        return false;
    }

    if (cyclic->read_idx + len <= cyclic->len)
    {
        sys_memcpy(buf, cyclic->buf + cyclic->read_idx, len);
    }
    else
    {
        uint16_t tlen = cyclic->len - cyclic->read_idx;

        sys_memcpy(buf, cyclic->buf + cyclic->read_idx, tlen);
        sys_memcpy(buf + tlen, cyclic->buf, len - tlen);
    }

    return true;
}

bool cyclic_buf_read(cyclic_buf_t *cyclic, uint8_t *buf, uint16_t len)
{
    if (!cyclic || !cyclic->buf)
    {
        return false;
    }

    if (len > cyclic_buf_count(cyclic))
    {
        return false;
    }

    if (cyclic->read_idx + len <= cyclic->len)
    {
        sys_memcpy(buf, cyclic->buf + cyclic->read_idx, len);
        cyclic->read_idx += len;
    }
    else
    {
        uint16_t tlen = cyclic->len - cyclic->read_idx;

        sys_memcpy(buf, cyclic->buf + cyclic->read_idx, tlen);
        sys_memcpy(buf + tlen, cyclic->buf, len - tlen);
        cyclic->read_idx = len - tlen;
    }

    cyclic->read_idx %= cyclic->len;

    return true;
}

bool cyclic_buf_drop(cyclic_buf_t *cyclic, uint16_t len)
{
    if (!cyclic || !cyclic->buf)
    {
        return false;
    }

    if (len > cyclic_buf_count(cyclic))
    {
        return false;
    }

    if (cyclic->read_idx + len <= cyclic->len)
    {
        cyclic->read_idx += len;
    }
    else
    {
        uint16_t tlen = cyclic->len - cyclic->read_idx;

        cyclic->read_idx = len - tlen;
    }

    cyclic->read_idx %= cyclic->len;

    return true;
}

bool cyclic_buf_clear(cyclic_buf_t *cyclic)
{
    if (!cyclic || !cyclic->buf)
    {
        return false;
    }

    memset(cyclic->buf, 0, cyclic->len);

    cyclic->read_idx = 0;
    cyclic->write_idx = 0;

    return true;
}


bool cyclic_buf_write(cyclic_buf_t *cyclic, uint8_t *buf, uint16_t len)
{
    if (!cyclic || !cyclic->buf)
    {
        return false;
    }

    if (cyclic_buf_room(cyclic) < len)
    {
        return false;
    }

    if (cyclic->write_idx + len <= cyclic->len)
    {
        sys_memcpy(cyclic->buf + cyclic->write_idx, buf, len);
        cyclic->write_idx += len;
    }
    else
    {
        uint16_t tlen = cyclic->len - cyclic->write_idx;

        sys_memcpy(cyclic->buf + cyclic->write_idx, buf, tlen);
        sys_memcpy(cyclic->buf, buf + tlen, len - tlen);
        cyclic->write_idx = len - tlen;
    }

    cyclic->write_idx %= cyclic->len;

    return true;
}
