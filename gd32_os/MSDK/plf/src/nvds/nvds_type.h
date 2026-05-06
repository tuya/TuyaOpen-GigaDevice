/*!
    \file    nvds_type.h
    \brief   Header file contains the declaration of Non Volatile
             Data Storage Flash's elements.

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

#ifndef _NVDS_TYPE_H_
#define _NVDS_TYPE_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "slist.h"
#include "mbedtls/aes.h"
#include "mbedtls/platform.h"
#include "config_gdm32.h"

/*
 * DEFINES
 ****************************************************************************************
 */
// Flash basic information
// NVDS location offset base 0x08000000 in FLASH :
#define NVDS_FLASH_INTERNAL_ADDR        RE_NVDS_DATA_OFFSET//(0x3FB000)
/* Last page(4KB) was write protected by flash option bytes, so keep last page is unused! */
#if ((RE_END_OFFSET - RE_NVDS_DATA_OFFSET - 0x1000) < 0x4000)
    #error "At least 16K for internal nvds!"
#endif

// NVDS size in FLASH (4*4KB = 16KBytes)
#define NVDS_FLASH_INTERNAL_SIZE        (0x4000)

// Support encryped nvds
// #define NVDS_FLASH_ENCRYPTED_SUPPORT
// NVDS magic number keyword
#define NVDS_FLASH_MAGIC                0x4E564453 /* "NVDS"*/
#define NVDS_FLASH_VERSION              0xFFFF
// SPI Flash sector size
#define SPI_FLASH_SEC_SIZE              4096

// Entry
// One entry size
#define ENTRY_SIZE                      32
// Max entry count of one page
// Page header and entry states table will take 32*2 bytes
#define ENTRY_COUNT_PER_PAGE            ((SPI_FLASH_SEC_SIZE / ENTRY_SIZE) - 2)
// Entry states table
#define ENTRY_STATES_TABLE_SIZE         (ENTRY_SIZE / sizeof(uint32_t))

// Page offset definition
// Page header offset of page
#define PAGE_HEADER_OFFSET              0
// Entry states table offset of page
#define PAGE_ENTRY_STATES_OFFSET        (PAGE_HEADER_OFFSET + sizeof(struct page_header))
// First entry offset of page
#define PAGE_ENTRY_OFFSET               (PAGE_ENTRY_STATES_OFFSET + ENTRY_SIZE)

// Max key size
#define KEY_NAME_MAX_SIZE               16

// namespace states table, use one bit to indicate namespace using
#define NAMESPACE_STATES_TABLE_SIZE     8

// Element size
#define ELEMENT_SMALL_MAX_SIZE          8
#define ELEMENT_MIDDLE_MAX_SIZE         256
// Bulk element maximum length is
// max length = (page size - page header bytes - entry states table byts
//                - bulk info element bytes) * max frag cnt
#define ELEMENT_BULK_MAX_SIZE           (400 * 32)

// TAG definition
// Offset of the namespace index field
#define TAG_NAMESPACE_OFT               0
// Mask of the namespace index field
#define TAG_NAMESPACE_MSK               (0xFF << TAG_NAMESPACE_OFT)
// Offset of the element type field
#define TAG_ELEMENT_TYPE_OFT            13
// Mask of the element type field
#define TAG_ELEMENT_TYPE_MSK            (0x7 << TAG_ELEMENT_TYPE_OFT)
// Offset of the frag no field
#define TAG_FRAG_NO_OFT                 8
// Mask of the frag no field
#define TAG_FRAG_NO_MSK                 (0x1F << TAG_FRAG_NO_OFT)
#define TAG_FRAG_NO_DEFAULT             0x1F

// namespace definition
#define NAMESPACE_DEFINE_IDX            0
#define NAMESPACE_NULL_IDX              0xFE
#define NAMESPACE_ANY_IDX               0xFF
// valid count exclude 0, 254, 255
#define NAMESPACE_MAX_CNT               253

// cryption definition
#define LABEL_NAME_MAX_SIZE             32
#define LABEL_INNER_NVDS_FLASH          "inner_nvds"
/* Assumes 8bit bytes! */
#define BITS_PER_BYTE                   ((size_t)8)
#define AES_KEY_SZ                      16
#define AES_BLOCK_SZ                    AES_KEY_SZ

// #define NVDS_DEBUG

#define NVDS_ERR_RET(cond, ret)         \
    ({                                  \
        if (!(cond)) {                  \
            return ret;                 \
        }                               \
    })

// element type
enum element_type {
    // small element
    ELEMENT_SMALL = 0,
    // middle element
    ELEMENT_MIDDLE,
    // bulk element
    ELEMENT_BULK,
    // bulkinfo element
    ELEMENT_BULKINFO,
    // any element
    ELEMENT_ANY = 7,
};

// entry state
enum entry_state {
    ENTRY_FREE      = 3,        /* 'b11, default value, indicate this entry is free to use. */
    ENTRY_USED      = 2,        /* 'b10, once this entry has been used. */
    ENTRY_UPDATED   = 0,        /* 'b00, when value in this entry has been updated. */
    ENTRY_ILLEGAL   = 1,        /* 'b01, this value should not be found. */
    ENTRY_ERROR     = 4
};

// page state
enum page_state {
    // flash default value, page has not been initialized.
    PAGE_UNINITIALIZED = 0xffff,

    // current operated page
    PAGE_ACTIVE = 0xfffe,

    // page has not enough room to accept new entry
    PAGE_FULL = 0xfffc,

    // the page has been stored with highest number of unused entrys
    // so it can be erased for further usage
    PAGE_CANDIDATE = 0xfff8,

    // page has been detected some unrecoverable errors
    PAGE_ERROR = 0xfff0,

    PAGE_INVALID = 0,
};

union entry_info
{
    struct
    {
        uint16_t tag; /* ns(7:0) + type(7:5) + fragno(4:0) */
        uint16_t length;
        uint32_t crc32;
        char key[KEY_NAME_MAX_SIZE];
        union {
            struct varlen_info {
                uint32_t  reserved;
                uint32_t  datacrc32;
            } varlen_info_t;
            struct bulk_info {
                uint32_t reserved;
                uint32_t bulksize;
            } bulk_info_t;
            uint8_t value[8];
        };
    };

    uint8_t data[ENTRY_SIZE];
};

struct namespace_info
{
    struct list_hdr list_hdr;
    char name[KEY_NAME_MAX_SIZE];
    uint8_t index;
    uint32_t used_cnt;
};

struct page_header
{
    // magic code
    uint32_t magic;
    // nvds page format version
    uint16_t version;
    // page state
    uint16_t state;
    // sequence number of this page
    uint32_t seqno;
    // unused, must be 0xff
    uint8_t rsv[16];
    // crc of everything except mState
    uint32_t crc32;
};

struct entry_hash
{
    struct list_hdr list_hdr;
    // entry index
    uint8_t index;
    // entry crc
    uint32_t crc32;
};

struct page_env_tag
{
    struct list_hdr list_hdr;
    // page header
    struct page_header header;
    // entry state table
    uint32_t entry_states[ENTRY_STATES_TABLE_SIZE];
    // element hash list
    struct list elt_list;
    // base address
    uint32_t base_addr;
    // used entry count
    uint16_t entry_cnt_used;
    // updated entry count
    uint16_t entry_cnt_updated;
    // record next free entry
    uint32_t next_free_idx;
    // record first used entry
    uint32_t first_used_idx;
    // record page need erase flag
    bool need_erase;
};

struct nvds_crypt_env {
    mbedtls_aes_context ctx;
    uint8_t key[AES_KEY_SZ];
};

struct nvds_flash_env_tag {
    struct list_hdr list_hdr;
    // label for one nvda flash storage, zero-ter
    char label[LABEL_NAME_MAX_SIZE + 1];
    // starting address of the nvds storage in flash
    uint32_t base_addr;
    // size of the storage, in bytes, should be aligned to 4K
    uint32_t length;
    // flag is set to true if nvds storage is encrypted
    uint8_t encrypted;
    // aes key for encrypt and decrypt
    struct nvds_crypt_env crypt_env;

    // namespace list
    struct list ns_list;
    uint32_t ns_states[NAMESPACE_STATES_TABLE_SIZE];

    // free page list
    struct list nvds_page_free;
    // used page list
    struct list nvds_page_used;
};

#endif /* _NVDS_TYPE_H_ */
