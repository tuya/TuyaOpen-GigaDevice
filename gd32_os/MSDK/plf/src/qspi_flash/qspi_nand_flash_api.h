/*!
    \file    qspi_flash_api.h
    \brief   Flash QSPI API for GD32VW55x SDK

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

#ifndef _QSPI_NAND_FLASH_API_H_
#define _QSPI_NAND_FLASH_API_H_

#include "nand_flash_def.h"


bool qspi_nandflash_api_init(void);

flash_err_t qspi_nandflash_write(uint8_t *buffer, uint16_t page_no, uint16_t offset_in_page, uint32_t byte_cnt);

flash_err_t qspi_nandflash_read(uint8_t *buffer, uint16_t page_no, uint16_t offset_in_page, uint32_t byte_cnt);

flash_err_t qspi_nandflash_block_erase(uint16_t block_no);

bool qspi_nandflash_is_badblock(uint16_t block_no);

bool qspi_nandflash_check_is_badpage_by_read(uint16_t page_no);

#endif
