/*!
    \file    mbl_region.h
    \brief   MBL region definition for GD32VW55x SDK

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

#ifndef __MBL_REGION_H__
#define __MBL_REGION_H__

#include "config_gdm32.h"

#if (RE_MBL_OFFSET == 0)
    #define IMG_OVERHEAD_LEN    0
    #define MBL_COMPAT_PREFIX   0x1000
#else
    #define IMG_OVERHEAD_LEN    (RE_VTOR_ALIGNMENT)
    #define MBL_COMPAT_PREFIX   0
#endif

/* MBL: code and ro data */
#define MBL_CODE_START                      (RE_FLASH_BASE + RE_MBL_OFFSET + IMG_OVERHEAD_LEN + MBL_COMPAT_PREFIX)
#define MBL_CODE_SIZE                       (32 * 1024 - RE_MBL_OFFSET - IMG_OVERHEAD_LEN)
#define MBL_API_START                       (RE_FLASH_BASE + 31 * 1024)
#define MBL_API_SIZE                        (256)          /* unit: byte */

/* SRAM: Global varaiables, STACK, HEAP*/
#define MBL_DATA_START                      (RE_SRAM_BASE + RE_MBL_DATA_START)  /* skip rom variables */
#define MBL_BUF_SIZE                        (0x3000)
#define MBL_DATA_SIZE                       (0x1000 + MBL_MSP_STACK_SIZE + MBL_BUF_SIZE)
#define MBL_MSP_STACK_SIZE                  (0x1000)

#endif  // __MBL_REGION_H__
