/*!
    \file    util.c
    \brief   Util function for GD32VW55x SDK.

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

#include "trace_ext.h"
#include "util.h"
#include "dbg_print.h"
#include "app_cfg.h"
#include <stdio.h>

uint8_t global_debug_level = NOTICE;

void util_init(void)
{
    trace_ext_init(true, false);
}

uint32_t byte_atoi(const char *inString)
{
    uint32_t theNum;
    char suffix = '\0';

    /* scan the number and any suffices */
    sscanf(inString, "%u%c", &theNum, &suffix);

    /* convert according to [Mm Kk] */
    switch ( suffix ) {
        case 'M':  theNum *= 1024 * 1024;  break;
        case 'K':  theNum *= 1024;  break;
        case 'm':  theNum *= 1000 * 1000;  break;
        case 'k':  theNum *= 1000;  break;
        default: break;
    }
    return theNum;
}

static int util_char2num(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

int util_hex2byte(char *hex)
{
    int a, b;
    a = util_char2num(*hex++);
    if (a < 0)
        return -1;
    b = util_char2num(*hex++);
    if (b < 0)
        return -1;
    return (a << 4) | b;
}

int util_hexstr2bin(char *hex, uint8_t *buf, size_t len)
{
    size_t i;
    int a;
    char *ipos = hex;
    uint8_t *opos = buf;

    for (i = 0; i < len; i++) {
        a = util_hex2byte(ipos);
        if (a < 0)
            return -1;
        *opos++ = a;
        ipos += 2;
    }
    return 0;
}

