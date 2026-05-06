/*!
    \file    nand_flash_cfg.h
    \brief   Nand Flash Config for GD32VW55x SDK

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

#ifndef _NAND_FLASH_CFG_H_
#define _NAND_FLASH_CFG_H_


#define    NAND_PAGE_SIZE           ((uint16_t)0x0800) /* 2 * 1024 bytes per page w/o Spare Area */
#define    NAND_BLOCK_SIZE          ((uint16_t)0x0040) /* 64 pages per block */
#define    NAND_SPARE_AREA_SIZE     ((uint16_t)0x0040) /* last 64 bytes as spare area */
#define    NAND_BLOCK_COUNT         2048               /* the count of block */
#define    NAND_PAGE_TOTAL_SIZE     (NAND_PAGE_SIZE + NAND_SPARE_AREA_SIZE)   /* page size + spare area size */

#define    BLOCK_NUM_FOR_USER           1900
#define    BLOCK_NUM_FOR_ReplaceBlock   98
#define    BLOCK_NUM_FOR_Table          /*50*/   (NAND_BLOCK_COUNT - BLOCK_NUM_FOR_USER - BLOCK_NUM_FOR_ReplaceBlock)

#endif
