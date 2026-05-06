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

#ifndef _QSPI_FLASH_API_H_
#define _QSPI_FLASH_API_H_
#include "app_cfg.h"

#define    NOR_FLASH_PAGE_SIZE          (256) /*256 bytes per page */
#define    NOR_FLASH_SECTOR_SIZE        ((uint16_t)0x1000) /* 4096 bytes */

#if (QSPI_FLASH_MEM == 16)
#define    NOR_FLASH_SECTOR_NUM         ((uint16_t)0x1000) /* 4096 */
#elif (QSPI_FLASH_MEM == 32)
#define    NOR_FLASH_SECTOR_NUM         ((uint16_t)0x2000) /* 8192 */
#else
#define    NOR_FLASH_SECTOR_NUM         (512) /* 512 */
#endif

bool qspi_flash_api_init(void);
int qspi_flash_erase(uint32_t offset, uint32_t len);
int qspi_flash_write(uint32_t offset, uint8_t *data, uint32_t len);
int qspi_flash_read(uint32_t offset, uint8_t *data, uint32_t len);
void qspi_flash_chip_erase(void);
void qspi_flash_dma_enable(void);
#endif
