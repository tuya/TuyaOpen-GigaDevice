/*!
    \file    raw_flash_api.h
    \brief   Flash RAW API for GD32VW55x SDK

    \version 2024-04-15, V1.0.0, firmware for GD32VW55x
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

#ifndef _FLASH_API_H_
#define _FLASH_API_H_

#define FLASH_SIZE_SIP              (0x400000)
#define FLASH_PAGE_SIZE             (0x1000)

#define FLASH_TOTAL_SIZE            FLASH_SIZE_SIP

typedef enum
{
    RAW_FLASH_ERASE_BLE_PRE_HANDLE,
    RAW_FLASH_ERASE_BLE_AFTER_HANDLE,
} raw_erase_type_t;

typedef void (*raw_flash_erase_handler_t)(raw_erase_type_t type);

void raw_flash_init(void);
uint32_t raw_flash_total_size(void);
int raw_flash_is_valid_offset(uint32_t offset);
int raw_flash_is_valid_addr(uint32_t addr);
void raw_flash_nodec_config(uint32_t nd_idx, uint32_t start_page, uint32_t end_page);
void raw_flash_offset_mapping(uint32_t of_spage, uint32_t of_epage, uint32_t of_value);
int raw_flash_read(uint32_t offset, void *data, int len);
int raw_flash_write(uint32_t offset, const void *data, int len);
int raw_flash_erase(uint32_t offset, int len);
int raw_flash_erase_handler_register(raw_flash_erase_handler_t callback);
void raw_flash_erase_handler_unregister(raw_flash_erase_handler_t callback);
int raw_flash_write_fast(uint32_t offset, const void *data, int len);

#endif
