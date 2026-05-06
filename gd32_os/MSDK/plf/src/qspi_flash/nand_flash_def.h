/*!
    \file    nand_flash_def.h
    \brief   Nand Flash Define for GD32VW55x SDK

    \version 2025-01-02, V1.0.0, firmware for GD32VW55x
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

#ifndef _NAND_FLASH_DEF_H_
#define _NAND_FLASH_DEF_H_

typedef enum
{
    FLASH_ERR_NO_ERROR                      = 0x00,   /*!< No error */

    FLASH_ERR_MALLOC_FAIL                   = 0x01,
    FLASH_ERR_OUT_OF_BOUND                  = 0x02,
    FLASH_ERR_OUT_OF_BBT_BOUND              = 0x03,
    FLASH_ERR_ECC_ERR                       = 0x04,
    FLASH_ERR_ERASE_FAIL                    = 0x05,
    FLASH_ERR_MV_READ_FAIL                  = 0x06,
    FLASH_ERR_MV_DATA_FAIL                  = 0x07,
    FLASH_ERR_NO_TAB_BLOCK                  = 0x08,
} flash_err_t;


#endif
