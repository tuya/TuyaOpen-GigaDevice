/*!
    \file    plf_assert.c
    \brief   Debug Assert function for GD32VW55x SDK.

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

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "plf_assert.h"
#include "ll.h"
#include "debug_print.h"
#include <stdint.h>
#include <stdbool.h>

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/*
 * LOCAL FUNCTION DEFINITION
 ****************************************************************************************
 */

/*
 * EXPORTED FUNCTION DEFINITION
 ****************************************************************************************
 */
// Variable to enable infinite loop on assert
static volatile int plf_asrt_block = 1;

void plf_assert_err(const char *condition, const char * file, int line)
{
    int i;

    co_printf("ASSERT ERROR: in %s at line %d\r\n", file, line);

    // Let time for the message transfer
    for (i = 0; i<2000;i++){plf_asrt_block = 1;};

    GLOBAL_INT_STOP();

    while(plf_asrt_block);
}

void plf_assert_param(int param0, int param1, const char * file, int line)
{
    int i;

    co_printf("ASSERT ERROR: param0 0x%08x param1 0x%08x, in %s at line %d\r\n", param0, param1, file, line);

    // Let time for the message transfer
    for (i = 0; i<2000;i++){plf_asrt_block = 1;};

    GLOBAL_INT_STOP();

    while(plf_asrt_block);
}

void plf_assert_warn(int param0, int param1, const char * file, int line)
{
    co_printf("ASSERT WARNING: param0 0x%08x param1 0x%08x, in %s at line %d\r\n", param0, param1, file, line);
}
