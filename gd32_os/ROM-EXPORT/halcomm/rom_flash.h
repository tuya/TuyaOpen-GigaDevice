/*!
    \file    rom_flash.h
    \brief   Rom flash interface for GD32VW55x SDK

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

#ifndef __ROM_FLASH_H__
#define __ROM_FLASH_H__

#define FLASH_TOTAL_SIZE            (0x400000)
#define FLASH_PAGE_SIZE             (0x1000)

extern uint32_t flash_tot_sz;

#define flash_erase_size()          FLASH_PAGE_SIZE

int is_valid_flash_offset(uint32_t offset);
int is_valid_flash_addr(uint32_t addr);
uint32_t flash_total_size(void);
int flash_init(void);
int flash_read_indirect(uint32_t offset, void *data, int len);
int flash_read(uint32_t offset, void *data, int len);
int flash_write(uint32_t offset, const void *data, int len);
int flash_write_fast(uint32_t offset, const void *data, int len);
int flash_erase(uint32_t offset, int len);
int flash_erase_chip(void);
int flash_get_obstat(void *obstat);
void flash_set_ob(uint32_t ob);
void flash_set_obusr(uint32_t obusr);
void flash_set_wrp(uint32_t idx, uint32_t spage, uint32_t epage);
void flash_nodec_config(uint32_t nd_idx, uint32_t spage, uint32_t epage);
void flash_offset_config(uint32_t offpage, uint32_t spage, uint32_t epage);
void flash_wtrim_config(uint32_t offset, uint32_t value);

#ifdef ROM_SELF_TEST
void flash_self_test(void);
#endif

#endif  /* __ROM_FLASH_H__ */
