/*!
    \file    nvds_flash.c
    \brief   Non Volatile Data Storage Flash memory driver for GD32VW55x SDK.

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
#include <stdio.h>
#include "nvds_flash.h"
#include "raw_flash_api.h"
#include "app_cfg.h"
#if NVDS_FLASH_SUPPORT
#include "nvds_type.h"
#include "rom_export.h"
#include "wrapper_os.h"
#include "dbg_print.h"
#include "crc.h"

/*
 * GLOBAL VARIABLE DECLARATION
 ****************************************************************************************
 */
// NVDS flash environment variable
struct nvds_flash_env_tag nvds_flash_env;

// NVDS flash environment variable list
static struct list nvds_flash_list;

static os_mutex_t nvds_mutex = NULL;
/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */
static int ns_del_used_cnt(struct nvds_flash_env_tag *flash_env, uint8_t ns_idx, uint8_t del_flag);

/**
 ****************************************************************************************
 * @brief Read a flash section
 *
 * This function is used to read a part of the flash memory.
 *
 * @param[in]  flash_env    Handle of the nvds flash operation
 * @param[in]  offset       Starting address from the beginning of the flash device
 * @param[in]  length       Size of the portion of flash to read
 * @param[out] buffer       Pointer on data to read

 * @return     status   0 if operation can start successfully
 ****************************************************************************************
 */
static int nvds_flash_read(struct nvds_flash_env_tag *flash_env, uint32_t offset, uint32_t length, uint8_t* buffer)
{
    if (!flash_env)
        return NVDS_ERR(NVDS_E_INVAL_PARAM);

    if ((offset + length) > flash_env->length)
        return NVDS_ERR(NVDS_E_INVAL_PARAM);

    if (rom_flash_read(flash_env->base_addr + offset, (void *)buffer, length))
        return NVDS_ERR(NVDS_E_FLASH_IO_FAIL);

    return NVDS_ERR(NVDS_OK);
}

/**
 ****************************************************************************************
 * @brief Write a flash section
 *
 * This function is used to write a part of the flash memory.
 *
 * @param[in]  flash_env    Handle of the nvds flash operation
 * @param[in]  offset       Starting address from the beginning of the flash device
 * @param[in]  length       Size of the portion of flash to write
 * @param[out] buffer       Pointer on data to write

 * @return     status   0 if operation can start successfully
 ****************************************************************************************
 */
static int nvds_flash_write(struct nvds_flash_env_tag *flash_env, uint32_t offset, uint32_t length, uint8_t* buffer)
{
    if (!flash_env)
        return NVDS_ERR(NVDS_E_INVAL_PARAM);

    if ((offset + length) > flash_env->length)
        return NVDS_ERR(NVDS_E_INVAL_PARAM);

    if (rom_flash_write(flash_env->base_addr + offset, (void *)buffer, length))
        return NVDS_ERR(NVDS_E_FLASH_IO_FAIL);

    return NVDS_ERR(NVDS_OK);
}


/**
 ****************************************************************************************
 * @brief Erase a flash section
 *
 * This function is used to erase a part of the flash memory.
 *
 * @param[in]  flash_env    Handle of the nvds flash operation
 * @param[in]  offset       Starting address from the beginning of the flash device
 * @param[in]  size         Size of the portion of flash to erase
 *
 * @return     status   0 if operation can start successfully
 ****************************************************************************************
 */
static int nvds_flash_erase(struct nvds_flash_env_tag *flash_env, uint32_t offset, uint32_t size)
{
    if (!flash_env)
        return NVDS_ERR(NVDS_E_INVAL_PARAM);

    if ((offset + size) > flash_env->length)
        return NVDS_ERR(NVDS_E_INVAL_PARAM);

    if (raw_flash_erase(flash_env->base_addr + offset, size))
        return NVDS_ERR(NVDS_E_FLASH_IO_FAIL);

    return NVDS_ERR(NVDS_OK);
}

/**
 ****************************************************************************************
 * @brief tag operation
 ****************************************************************************************
 */
static uint8_t tag_namespace_get(uint16_t tag)
{
    return (uint8_t)((tag & TAG_NAMESPACE_MSK) >> TAG_NAMESPACE_OFT);
}

#if 0
static void tag_namespace_set(uint8_t ns, uint16_t *tag)
{
    *tag = (*tag & ~TAG_NAMESPACE_MSK) | (ns << TAG_NAMESPACE_OFT);
}
#endif

static enum element_type tag_element_type_get(uint16_t tag)
{
    return (enum element_type)((tag & TAG_ELEMENT_TYPE_MSK) >> TAG_ELEMENT_TYPE_OFT);
}

#if 0
static void tag_element_type_set(enum element_type type, uint16_t *tag)
{
    *tag = (*tag & ~TAG_ELEMENT_TYPE_MSK) | (type << TAG_ELEMENT_TYPE_OFT);
}
#endif

static uint8_t tag_fragno_get(uint16_t tag)
{
    return (uint8_t)((tag & TAG_FRAG_NO_MSK) >> TAG_FRAG_NO_OFT);
}

#if 0
static void tag_fragno_set(uint8_t fragno, uint16_t *tag)
{
    *tag = (*tag & ~TAG_FRAG_NO_MSK) | (fragno << TAG_FRAG_NO_OFT);
}
#endif

static void tag_set(uint8_t ns, enum element_type type, uint8_t fragno, uint16_t *tag)
{
    *tag = (ns << TAG_NAMESPACE_OFT) | (type << TAG_ELEMENT_TYPE_OFT) | (fragno << TAG_FRAG_NO_OFT);
}

/**
 ****************************************************************************************
 * @brief crc32 calculate
 ****************************************************************************************
 */
static uint32_t element_header_crc32_calc(union entry_info *header)
{
    uint32_t addr = (uint32_t)header;
    uint32_t crc;

    crc = crc32(addr, offsetof(union entry_info, crc32), 0);
    crc = crc32(addr + offsetof(union entry_info, key), KEY_NAME_MAX_SIZE, crc);
    crc = crc32(addr + offsetof(union entry_info, value), sizeof(header->value), crc);

    return crc;
}

#if 0
static uint32_t element_hash_crc32_calc(union entry_info *header)
{
    uint32_t addr = (uint32_t)header;
    uint32_t crc32;
    uint8_t ns;

    ns = tag_namespace_get(header->tag);
    crc32 = co_crc32(&ns, sizeof(ns), 0);
    crc32 = co_crc32(addr + offsetof(union entry_info, key), KEY_NAME_MAX_SIZE, crc32);

    return crc32;
}
#endif

static uint32_t element_data_crc32_calc(void *data, uint32_t size)
{
    uint32_t addr = (uint32_t)data;

    return crc32(addr, size, 0);
}

static uint32_t page_header_crc32_calc(struct page_header *header)
{
    uint32_t addr = (uint32_t)header;
    uint32_t crc;

    crc = crc32(addr, offsetof(struct page_header, state), 0);
    crc = crc32(addr + offsetof(struct page_header, seqno),
                   offsetof(struct page_header, crc32) - offsetof(struct page_header, seqno), crc);

    return crc;
}

/**
 ****************************************************************************************
 * @brief entry states table
 ****************************************************************************************
 */
static int entry_states_table_read(struct nvds_flash_env_tag *flash_env, uint32_t page_addr,
                                    uint32_t *states)
{
    uint32_t address;

    if (!flash_env || !states)
        return NVDS_ERR(NVDS_E_FAIL);

    address = page_addr + PAGE_ENTRY_STATES_OFFSET;

    return nvds_flash_read(flash_env, address, ENTRY_SIZE, (uint8_t *)states);
}

static int entry_states_table_write(struct nvds_flash_env_tag *flash_env, uint32_t page_addr,
                                    uint32_t *states)
{
    uint32_t address;

    if (!flash_env || !states)
        return NVDS_ERR(NVDS_E_FAIL);

    address = page_addr + PAGE_ENTRY_STATES_OFFSET;

    return nvds_flash_write(flash_env, address, ENTRY_SIZE, (uint8_t *)states);
}

static int entry_state_get(uint32_t *states, uint8_t idx, enum entry_state *state)
{
    uint32_t table_idx = idx / 16;
    uint32_t bit_offset = (idx % 16) * 2;

    if (idx >= ENTRY_COUNT_PER_PAGE)
        return NVDS_ERR(NVDS_E_FAIL);

    *state = (enum entry_state)((states[table_idx] >> bit_offset) & 0x03);

    return NVDS_ERR(NVDS_OK);
}

static int entry_state_set(uint32_t *states, uint8_t idx, enum entry_state state)
{
    uint32_t table_idx = idx / 16;
    uint32_t bit_offset = (idx % 16) * 2;

    if (idx >= ENTRY_COUNT_PER_PAGE)
        return NVDS_ERR(NVDS_E_FAIL);

    if ((state == ENTRY_ILLEGAL) || (state >= ENTRY_ERROR))
        return NVDS_ERR(NVDS_E_FAIL);

    if (table_idx >= ENTRY_STATES_TABLE_SIZE)
        return NVDS_ERR(NVDS_E_FAIL);

    states[table_idx] = (states[table_idx] & ~(0x03 << bit_offset))
                         | ((uint32_t)state << bit_offset);

    return NVDS_ERR(NVDS_OK);
}

#if 0
static int entry_state_range_set(uint32_t *states, uint8_t start, uint8_t end, enum entry_state state)
{
    uint8_t idx;
    int ret;

    if (end < start || end >= ENTRY_COUNT_PER_PAGE)
        return NVDS_ERR(NVDS_E_FAIL);

    if ((state == ENTRY_ILLEGAL) || (state >= ENTRY_ERROR))
        return NVDS_ERR(NVDS_E_FAIL);

    for (idx = start; idx <= end; ++idx) {
        ret = entry_state_set(states, idx, state);
        NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);
    }

    return NVDS_ERR(NVDS_OK);
}
#endif

static int entry_state_alter(struct nvds_flash_env_tag *flash_env, struct page_env_tag *page,
                                uint8_t entry_idx, enum entry_state state)
{
    int ret;

    if (!flash_env || !page || (entry_idx >= ENTRY_COUNT_PER_PAGE))
        return NVDS_ERR(NVDS_E_FAIL);

    if ((state == ENTRY_ILLEGAL) || (state >= ENTRY_ERROR))
        return NVDS_ERR(NVDS_E_FAIL);

    /* modify in entry states table */
    ret = entry_state_set(page->entry_states, entry_idx, state);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

    /* change in falsh memory */
    ret = entry_states_table_write(flash_env, page->base_addr, page->entry_states);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

    return NVDS_ERR(NVDS_OK);
}

static int entry_state_range_alter(struct nvds_flash_env_tag *flash_env, struct page_env_tag *page,
                                uint8_t begin, uint8_t end, enum entry_state state)
{
    uint8_t idx;
    int ret;

    if ((end < begin) || (end >= ENTRY_COUNT_PER_PAGE))
        return NVDS_ERR(NVDS_E_FAIL);

    for (idx = begin; idx <= end; ++idx) {
        ret = entry_state_alter(flash_env, page, idx, state);
        NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);
    }

    return NVDS_ERR(NVDS_OK);
}

/**
 ****************************************************************************************
 * @brief namespace state, 1 is ns_idx used, 0 otherwise
 ****************************************************************************************
 */
static void ns_state_get(uint32_t *states, uint8_t idx, bool *ns_state)
{
    uint32_t table_idx = idx / 32;
    uint32_t bit_offset = idx % 32;

    *ns_state = (bool)((states[table_idx] >> bit_offset) & 0x01);
}

static void ns_state_set(uint32_t *states, uint8_t idx, bool ns_state)
{
    uint32_t table_idx = idx / 32;
    uint32_t bit_offset = idx % 32;

    states[table_idx] = (states[table_idx] & ~(0x01 << bit_offset))
                         | ((uint32_t)ns_state << bit_offset);
}

/**
 ****************************************************************************************
 * @brief Modify page state in page header
 ****************************************************************************************
 */
static int page_state_alter(struct nvds_flash_env_tag *flash_env, struct page_env_tag *page,
                                enum page_state c_state)
{
    uint16_t state_val;
    uint32_t address;
    int ret;

    if (!flash_env || !page)
        return NVDS_ERR(NVDS_E_FAIL);

    state_val = (uint16_t)c_state;
    address = page->base_addr + PAGE_HEADER_OFFSET + offsetof(struct page_header, state);
    ret = nvds_flash_write(flash_env, address, sizeof(state_val), (uint8_t *)&state_val);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

    page->header.state = state_val;
    return NVDS_ERR(NVDS_OK);
}

/**
 ****************************************************************************************
 * @brief Read / Write entry (32bytes)
 ****************************************************************************************
 */
#ifdef NVDS_FLASH_ENCRYPTED_SUPPORT
static int nvds_flash_security_init(struct nvds_flash_env_tag *flash_env)
{
    if (!flash_env->encrypted)
        return NVDS_ERR(NVDS_OK);

    /* Derive AES key from HUK */
    if (rom_do_symm_key_derive((uint8_t *)flash_env->label, strlen(flash_env->label), flash_env->crypt_env.key, AES_KEY_SZ))
        return NVDS_ERR(NVDS_E_SECUR_CFG_FAIL);

    mbedtls_aes_init(&flash_env->crypt_env.ctx);

    if (mbedtls_aes_setkey_enc(&flash_env->crypt_env.ctx, flash_env->crypt_env.key, AES_KEY_SZ * BITS_PER_BYTE))
        return NVDS_ERR(NVDS_E_SECUR_CFG_FAIL);

    if (mbedtls_aes_setkey_dec(&flash_env->crypt_env.ctx, flash_env->crypt_env.key, AES_KEY_SZ * BITS_PER_BYTE))
        return NVDS_ERR(NVDS_E_SECUR_CFG_FAIL);

    return NVDS_ERR(NVDS_OK);
}
#endif

static int entry_read(struct nvds_flash_env_tag *flash_env, struct page_env_tag *page,
                                uint8_t entry_idx, union entry_info* entry)
{
    uint32_t address;
    int ret, length;
    uint8_t *buf;

    if (!flash_env || !entry || !page)
        return NVDS_ERR(NVDS_E_FAIL);

    address = page->base_addr + PAGE_ENTRY_OFFSET + entry_idx * ENTRY_SIZE;

    /* read data from flash */
    ret = nvds_flash_read(flash_env, address, ENTRY_SIZE, (uint8_t *)entry);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

    /* decrypt data if encrypted enabled */
    if (flash_env->encrypted) {
        length = ENTRY_SIZE;
        if (length % AES_BLOCK_SZ != 0)
            return NVDS_ERR(NVDS_E_INVAL_PARAM);

        buf = (uint8_t *)entry;
        while (length > 0) {
            ret = mbedtls_aes_crypt_ecb(&flash_env->crypt_env.ctx, MBEDTLS_AES_DECRYPT, buf, buf);
            if (ret != 0)
                return NVDS_ERR(NVDS_E_DECR_FAIL);
            buf += AES_BLOCK_SZ;
            length -= AES_BLOCK_SZ;
        }
    }

    return NVDS_ERR(NVDS_OK);
}

static int entry_write(struct nvds_flash_env_tag *flash_env, struct page_env_tag *page,
                            union entry_info* entry)
{
    uint32_t address;
    int ret, length;
    uint8_t *buf;

    if (!flash_env || !entry || !page)
        return NVDS_ERR(NVDS_E_FAIL);

    if (page->next_free_idx >= ENTRY_COUNT_PER_PAGE)
        return NVDS_ERR(NVDS_E_FAIL);

    address = page->base_addr + PAGE_ENTRY_OFFSET + page->next_free_idx * ENTRY_SIZE;

    /* encrypt data before write to flash if encrypted enabled */
    if (flash_env->encrypted) {
        length = ENTRY_SIZE;
        if (length % AES_BLOCK_SZ != 0)
            return NVDS_ERR(NVDS_E_INVAL_PARAM);

        buf = (uint8_t *)entry;
        while (length > 0) {
            ret = mbedtls_aes_crypt_ecb(&flash_env->crypt_env.ctx, MBEDTLS_AES_ENCRYPT, buf, buf);
            if (ret != 0)
                return NVDS_ERR(NVDS_E_ENCR_FAIL);
            buf += AES_BLOCK_SZ;
            length -= AES_BLOCK_SZ;
        }
    }

    /* write data to flash */
    ret = nvds_flash_write(flash_env, address, ENTRY_SIZE, (uint8_t *)entry);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

    page->next_free_idx++;
    page->entry_cnt_used++;

    return NVDS_ERR(NVDS_OK);
}

static int entry_data_write(struct nvds_flash_env_tag *flash_env, uint32_t address,
                            uint32_t bufsize, uint8_t *buf)
{
    int ret, length, round_len, remain_len;
    uint8_t *input;
    uint8_t remain_buf[ENTRY_SIZE];

    if (!flash_env)
        return NVDS_ERR(NVDS_E_FAIL);

    /* write pure data to flash (i.e. not include entry header) */
    if (flash_env->encrypted) {
        /* make sure encrypted data size aligned to a multiple of ENTRY_SIZE */
        if (bufsize % ENTRY_SIZE) {
            round_len = ((bufsize >> 5) << 5);
            remain_len = ENTRY_SIZE;
        } else {
            round_len = bufsize;
            remain_len = 0;
        }

        /* encrypt data before write to flash if encrypted enabled */
        length = round_len;
        input = buf;
        while (length > 0) {
            ret = mbedtls_aes_crypt_ecb(&flash_env->crypt_env.ctx, MBEDTLS_AES_ENCRYPT, input, input);
            if (ret != 0)
                return NVDS_ERR(NVDS_E_ENCR_FAIL);
            input += AES_BLOCK_SZ;
            length -= AES_BLOCK_SZ;
        }

        if (round_len) {
            ret = nvds_flash_write(flash_env, address, round_len, buf);
            NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);
        }

        /* process remain data */
        if (remain_len) {
            sys_memset(remain_buf, 0xFF, ENTRY_SIZE);
            sys_memcpy(remain_buf, input, bufsize - round_len);
            length = remain_len;
            input = remain_buf;
            while (length > 0) {
                ret = mbedtls_aes_crypt_ecb(&flash_env->crypt_env.ctx, MBEDTLS_AES_ENCRYPT, input, input);
                if (ret != 0)
                    return NVDS_ERR(NVDS_E_ENCR_FAIL);
                input += AES_BLOCK_SZ;
                length -= AES_BLOCK_SZ;
            }

            /* write data to flash */
            ret = nvds_flash_write(flash_env, address + round_len, remain_len, remain_buf);
            NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);
        }
    } else {
        ret = nvds_flash_write(flash_env, address, bufsize, buf);
        NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);
    }

    return NVDS_ERR(NVDS_OK);
}

#ifdef NVDS_DEBUG
static int page_print(struct nvds_flash_env_tag *flash_env, struct page_env_tag *page)
{
    int entry_idx;
    enum entry_state state;
    union entry_info entry;
    int i;
    int ret;

    if (!page || !flash_env)
        return NVDS_ERR(NVDS_E_FAIL);

    printf("page addr: %u ---------------\n", page->base_addr);
    printf("page header:\n");
    nvds_flash_read(flash_env, page->base_addr, ENTRY_SIZE, (uint8_t *)&entry);
    for (i = 0; i < ENTRY_SIZE; i++)
        printf("%02x ", entry.data[i]);
    printf("\n");
    nvds_flash_read(flash_env, page->base_addr + ENTRY_SIZE, ENTRY_SIZE, (uint8_t *)&entry);
    for (i = 0; i < ENTRY_SIZE; i++)
        printf("%02x ", entry.data[i]);
    printf("\n");

    printf("used entry:\n");
    /* walk entry */
    for (entry_idx = 0; entry_idx < ENTRY_COUNT_PER_PAGE; entry_idx++) {
        ret = entry_state_get(page->entry_states, entry_idx, &state);
        NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

        if (state == ENTRY_FREE)
            break;
        ret = entry_read(flash_env, page, entry_idx, &entry);
        NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

        printf("%d: ", entry_idx);
        for (i = 0; i < ENTRY_SIZE; i++)
            printf("%02x ", entry.data[i]);
        printf("\n");
    }
    printf("page end --------------\n");

    return NVDS_ERR(NVDS_OK);
}
#endif

static int element_find(struct nvds_flash_env_tag *flash_env, uint8_t ns_idx,
                        const char* key, struct page_env_tag **page_find,
                        uint8_t *entry_find, struct page_env_tag *page_start, uint8_t entry_type)
{
    struct page_env_tag *page = NULL;
    union entry_info entry;
    enum entry_state state;
    uint8_t entry_idx;
    enum element_type type;
    uint8_t ns;
    int ret;
    /* fix: when one more bulk fragments exist in the same page,
    we can only return the first found one. It happens when the current
    page can only save pare of a bulk element ie the first fragment,
    and the current page has the most invalid entries, so the fist fragment
    will be moved to next active page which the second fragment
    is going to be written. */
    int entry_start = *entry_find;

    if (!flash_env || !key)
        return NVDS_ERR(NVDS_E_FAIL);

    /* from the specified used page start to find */
    if (page_start) {
        page = (struct page_env_tag *)list_pick(&flash_env->nvds_page_used);
        while (page) {
            if (page == page_start)
                break;

            page = (struct page_env_tag*)list_next(&page->list_hdr);
        }
    }

    /* from all used page to find */
    if (!page)
        page = (struct page_env_tag *)list_pick(&flash_env->nvds_page_used);

    while (page) {
        if ((page->header.state != PAGE_ACTIVE) && (page->header.state != PAGE_FULL)) {
            page = (struct page_env_tag*)list_next(&page->list_hdr);
            continue;
        }

        /* walk entry in the page */
        for (entry_idx = entry_start; entry_idx < ENTRY_COUNT_PER_PAGE;) {
            /* only find the valid entry */
            ret = entry_state_get(page->entry_states, entry_idx, &state);
            NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

            if (state == ENTRY_FREE)
                break;
            ret = entry_read(flash_env, page, entry_idx, &entry);
            NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

            /* compare namespace and key */
            ns = tag_namespace_get(entry.tag);
            type = tag_element_type_get(entry.tag);

            if ((state == ENTRY_USED) && (ns_idx == ns) && strlen(key) == strlen(entry.key) &&
                !sys_memcmp(key, entry.key, strlen(key))) {
                if ((entry_type == ELEMENT_ANY) || (type == entry_type)) {
                    if (entry.crc32 == element_header_crc32_calc(&entry)) {
                        *page_find = page;
                        *entry_find = entry_idx;
                        return NVDS_ERR(NVDS_OK);
                    }
                }
            }

            entry_idx++;
            if ((type == ELEMENT_MIDDLE) || (type == ELEMENT_BULK))
                entry_idx += ((entry.length + ENTRY_SIZE - 1) / ENTRY_SIZE);
        }

        /* go to next used page */
        page = (struct page_env_tag*)list_next(&page->list_hdr);

        /* when go to next new page, entry should be looking for from page start. */
        entry_start = 0;
    }

    return NVDS_ERR(NVDS_E_NOT_FOUND);
}

static int flash_env_check(uint32_t start_addr, uint32_t size)
{
    struct nvds_flash_env_tag *flash_env;

    /* check start range whether if overlap */
    flash_env = (struct nvds_flash_env_tag *)list_pick(&nvds_flash_list);
    while (flash_env) {
        if (((start_addr + size) < flash_env->base_addr)
            || (start_addr > (flash_env->base_addr + flash_env->length))) {
            flash_env = (struct nvds_flash_env_tag*)list_next(&flash_env->list_hdr);
            continue;
        } else {
            /* flash overlap */
            return NVDS_ERR(NVDS_E_INVAL_PARAM);
        }
    }

    return NVDS_ERR(NVDS_OK);
}

static int page_room_get(struct page_env_tag *page)
{
    /* return the empty area size */
    return ((ENTRY_COUNT_PER_PAGE - page->next_free_idx) * ENTRY_SIZE);
}

static int data_element_find(struct nvds_flash_env_tag *flash_env, uint8_t ns_idx, const char* key)
{
    uint8_t entry_idx = 0;
    struct page_env_tag *page;

    /* find namespace and key form used page */
    return element_find(flash_env, ns_idx, key, &page, &entry_idx, NULL, ELEMENT_ANY);
}

static int bulk_element_del(struct nvds_flash_env_tag *flash_env, uint8_t ns_idx, const char* key)
{
    struct page_env_tag *page;
    struct page_env_tag *page_start = NULL;
    union entry_info entry;
    uint8_t entry_idx = 0;
    int entry_cnt;
    int ret = NVDS_ERR(NVDS_OK);

    /* find bulkinfo entry by ns and key */
    ret = element_find(flash_env, ns_idx, key, &page, &entry_idx, page_start, ELEMENT_BULKINFO);
    if (ret == NVDS_ERR(NVDS_E_NOT_FOUND)) {
        return NVDS_ERR(NVDS_OK);
    } else if (ret) {
        return ret;
    }

    /* change bulkinfo entry state to ENTRY_UPDATED */
    ret = entry_state_range_alter(flash_env, page, entry_idx, entry_idx, ENTRY_UPDATED);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);
    page->entry_cnt_used--;

    entry_idx = 0;
    page_start = NULL;
    /* find all the remain bulk entry */
    do {
        ret = element_find(flash_env, ns_idx, key, &page, &entry_idx, page_start, ELEMENT_BULK);
        if (ret == NVDS_ERR(NVDS_E_NOT_FOUND)) {
            break;
        } else if (ret) {
            return ret;
        }

        /* modify all the remain bulk entry states to ENTRY_UPDATED */
        ret = entry_read(flash_env, page, entry_idx, &entry);
        NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

        entry_cnt = (entry.length + ENTRY_SIZE - 1) / ENTRY_SIZE;
        ret = entry_state_range_alter(flash_env, page, entry_idx, entry_idx + entry_cnt, ENTRY_UPDATED);
        NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);
        page->entry_cnt_used -= (entry_cnt + 1);

        entry_idx += (entry_cnt + 1);
        page_start = page;
    } while (page_start);

    return NVDS_ERR(NVDS_OK);
}

static int data_element_del(struct nvds_flash_env_tag *flash_env, uint8_t ns_idx, const char* key)
{
    struct page_env_tag *page;
    struct page_env_tag *page_start = NULL;
    union entry_info entry;
    uint8_t entry_idx = 0;
    int entry_cnt;
    int ret = NVDS_ERR(NVDS_OK);
    enum element_type type;

    /* find element by ns and key form used page */
    ret = element_find(flash_env, ns_idx, key, &page, &entry_idx, page_start, ELEMENT_ANY);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

    /* read entry */
    ret = entry_read(flash_env, page, entry_idx, &entry);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

    /* modify entry states to ENTRY_UPDATED */
    type = tag_element_type_get(entry.tag);
    if ((type == ELEMENT_BULKINFO) || (type == ELEMENT_BULK))
        return bulk_element_del(flash_env, ns_idx, key);

    if (type == ELEMENT_SMALL) {
        ret = entry_state_range_alter(flash_env, page, entry_idx, entry_idx, ENTRY_UPDATED);
        NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);
        page->entry_cnt_used--;
    } else if (type == ELEMENT_MIDDLE) {
        entry_cnt = (entry.length + ENTRY_SIZE - 1) / ENTRY_SIZE;
        ret = entry_state_range_alter(flash_env, page, entry_idx, entry_idx + entry_cnt, ENTRY_UPDATED);
        NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);
        page->entry_cnt_used -= (entry_cnt + 1);
    } else {
        return NVDS_ERR(NVDS_E_FAIL);
    }

#ifdef NVDS_DEBUG
    page_print(flash_env, page);
#endif

    return NVDS_ERR(NVDS_OK);
}

static int bulk_element_get(struct nvds_flash_env_tag *flash_env, uint8_t ns_idx, const char* key, uint8_t *buf, uint32_t *bufsize)
{
    struct page_env_tag *page;
    struct page_env_tag *page_start = NULL;
    union entry_info entry;
    uint8_t entry_idx = 0;
    uint32_t bulk_size;
    uint32_t frag_cnt;
    uint32_t frag_idx;
    uint8_t entry_start;
    uint8_t entry_end;
    uint32_t entry_len;
    uint8_t* dst;
    uint32_t dst_offset;
    uint32_t remain;
    uint32_t copy;
    //uint32_t crc32;
    uint8_t fragno;
    int ret;

    /* find bulkinfo entry by ns and key */
    ret = element_find(flash_env, ns_idx, key, &page, &entry_idx, page_start, ELEMENT_BULKINFO);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

    /* read entry */
    ret = entry_read(flash_env, page, entry_idx, &entry);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

    bulk_size = entry.bulk_info_t.bulksize;
    frag_cnt = entry.length;

    /* check if need to return real size */
    if (!buf) {
        *bufsize = bulk_size;
        return NVDS_ERR(NVDS_OK);
    } else if (*bufsize < bulk_size) {
        *bufsize = bulk_size;
        return NVDS_ERR(NVDS_E_INVALID_LENGTH);
    }

    *bufsize = bulk_size;
    dst_offset = 0;
    /* read bulk frag data */
    for (frag_idx = 0; frag_idx < frag_cnt; ++frag_idx) {
        page_start = NULL;
        entry_idx = 0;
        do {
            ret = element_find(flash_env, ns_idx, key, &page, &entry_idx, page_start, ELEMENT_BULK);
            if (ret == NVDS_ERR(NVDS_E_NOT_FOUND)) {
                break;
            } else if (ret) {
                return ret;
            }

            /* read entry */
            ret = entry_read(flash_env, page, entry_idx, &entry);
            NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

            fragno = tag_fragno_get(entry.tag);
            if (fragno == frag_idx) {
                entry_len = entry.length;

                /* read frag data */
                dst = buf + dst_offset;
                remain = entry.length;

                entry_start = entry_idx + 1;
                entry_end = entry_idx + ((entry.length + ENTRY_SIZE - 1) / ENTRY_SIZE);
                //crc32 = entry.varlen_info_t.datacrc32;

                for (entry_idx = entry_start; entry_idx <= entry_end; ++entry_idx) {
                    ret = entry_read(flash_env, page, entry_idx, &entry);
                    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

                    copy = (remain < ENTRY_SIZE) ? remain : ENTRY_SIZE;
                    sys_memcpy(dst, entry.data, copy);
                    remain -= copy;
                    dst += copy;
                }

// TODO
#if 0
                /* calculate data crc */
                if (crc32 != element_data_crc32_calc(dst, entry_len)) {
                    /* modify entry states to updated */
                    ret = entry_state_range_alter(flash_env, page, entry_start - 1, entry_end, ENTRY_UPDATED);
                    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

                    /* TODO Modify all frag data states to updated */
                    return NVDS_ERR(NVDS_E_NOT_FOUND);
                }
#endif
                dst_offset += entry_len;

                break;
            }

            /* record new entry index for element find */
            entry_idx++;
            entry_idx += ((entry.length + ENTRY_SIZE - 1) / ENTRY_SIZE);

            /* go to next page */
            page_start = page;
        } while (page_start);
    }

    return NVDS_ERR(NVDS_OK);
}

static int data_element_get(struct nvds_flash_env_tag *flash_env, uint8_t ns_idx, const char* key, uint8_t *buf, uint32_t *bufsize)
{
    struct page_env_tag *page;
    union entry_info entry;
    enum element_type type;
    uint8_t entry_start;
    uint8_t entry_end;
    uint32_t entry_len;
    uint8_t entry_idx = 0;
    uint8_t* dst;
    uint32_t remain;
    uint32_t copy;
    uint32_t crc32;
    int ret;

    /* find entry by ns and key form used page */
    ret = element_find(flash_env, ns_idx, key, &page, &entry_idx, NULL, ELEMENT_ANY);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

    /* read entry */
    ret = entry_read(flash_env, page, entry_idx, &entry);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

    type = tag_element_type_get(entry.tag);
    if ((type == ELEMENT_BULK) || (type == ELEMENT_BULKINFO))
        return bulk_element_get(flash_env, ns_idx, key, buf, bufsize);

    entry_len = entry.length;
    /* check if need to return real size */
    if (!buf) {
        *bufsize = entry_len;
        return NVDS_ERR(NVDS_OK);
    } else if (*bufsize < entry_len) {
        *bufsize = entry_len;
        return NVDS_ERR(NVDS_E_INVALID_LENGTH);
    }

    *bufsize = entry_len;
    if (type == ELEMENT_SMALL) {
        /* get data */
        sys_memcpy(buf, entry.value, entry_len);
    } else if (type == ELEMENT_MIDDLE) {
        dst = buf;
        remain = entry_len;

        entry_start = entry_idx + 1;
        entry_end = entry_idx + ((entry_len + ENTRY_SIZE - 1) / ENTRY_SIZE);
        crc32 = entry.varlen_info_t.datacrc32;

        for (entry_idx = entry_start; entry_idx <= entry_end; ++entry_idx) {
            ret = entry_read(flash_env, page, entry_idx, &entry);
            NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

            copy = (remain < ENTRY_SIZE) ? remain : ENTRY_SIZE;
            sys_memcpy(dst, entry.data, copy);
            remain -= copy;
            dst += copy;
        }

        /* calculate data crc */
        if (crc32 != element_data_crc32_calc(buf, entry_len))
            return NVDS_ERR(NVDS_E_NOT_FOUND);
    }

    return NVDS_ERR(NVDS_OK);
}

static bool bulk_element_compare(struct nvds_flash_env_tag *flash_env, uint8_t ns_idx, const char* key, uint8_t *buf, uint32_t bufsize)
{
    struct page_env_tag *page;
    struct page_env_tag *page_start = NULL;
    union entry_info entry;
    uint8_t entry_idx = 0;
    uint32_t bulk_size;
    uint32_t frag_cnt;
    uint32_t frag_idx;
    uint8_t entry_start;
    uint8_t entry_end;
    uint32_t entry_len;
    uint8_t* dst;
    uint32_t dst_offset;
    uint32_t remain;
    uint32_t copy;
    //uint32_t crc32;
    uint8_t fragno;
    int ret = NVDS_ERR(NVDS_OK);

    /* find bulk info entry */
    ret = element_find(flash_env, ns_idx, key, &page, &entry_idx, page_start, ELEMENT_BULKINFO);
    if (ret == NVDS_ERR(NVDS_E_NOT_FOUND)) {
        return true;
    } else if (ret) {
        return false;
    }

    /* read entry */
    ret = entry_read(flash_env, page, entry_idx, &entry);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), false);

    bulk_size = entry.bulk_info_t.bulksize;
    frag_cnt = entry.length;

    if (bulk_size != bufsize)
        return false;

    dst_offset = 0;
    /* compare bulk fragment data */
    for (frag_idx = 0; frag_idx < frag_cnt; ++frag_idx) {
        page_start = NULL;
        entry_idx = 0;
        do {
            ret = element_find(flash_env, ns_idx, key, &page, &entry_idx, page_start, ELEMENT_BULK);
            if (ret == NVDS_ERR(NVDS_E_NOT_FOUND)) {
                break;
            } else if (ret) {
                return false;
            }

            /* read entry */
            ret = entry_read(flash_env, page, entry_idx, &entry);
            NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), false);

            fragno = tag_fragno_get(entry.tag);
            if (fragno == frag_idx) {
                /* prepare data to be compared */
                dst = buf + dst_offset;
                remain = entry.length;

                /* extarct entry information */
                entry_len = entry.length;
                entry_start = entry_idx + 1;
                entry_end = entry_idx + ((entry.length + ENTRY_SIZE - 1) / ENTRY_SIZE);
                // crc32 = entry->varlen_info_t.datacrc32; // TODO

                /* compare fragment data directly */
                for (entry_idx = entry_start; entry_idx <= entry_end; ++entry_idx) {
                    ret = entry_read(flash_env, page, entry_idx, &entry);
                    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), false);

                    copy = (remain < ENTRY_SIZE) ? remain : ENTRY_SIZE;
                    if (sys_memcmp(dst, entry.data, copy))
                        return false;
                    remain -= copy;
                    dst += copy;
                }
                dst_offset += entry_len;
                break;
            }

            /* record new entry index for element find */
            entry_idx++;
            entry_idx += ((entry.length + ENTRY_SIZE - 1) / ENTRY_SIZE);

            /* go to next page */
            page_start = page;
        } while (page_start);
    }

    return true;
}

static bool element_compare(struct nvds_flash_env_tag *flash_env, uint8_t ns_idx, const char* key, uint8_t *buf, uint32_t bufsize)
{
    uint32_t data_size;
    struct page_env_tag *page;
    union entry_info entry;
    enum element_type type;
    uint8_t entry_start;
    uint8_t entry_end;
    uint8_t entry_idx = 0;
    uint8_t* dst;
    uint32_t remain;
    uint32_t copy;
    int ret;

    if (!flash_env || !key || !buf)
        return false;

    /* get element data size if exist */
    ret = data_element_get(flash_env, ns_idx, key, NULL, &data_size);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), false);

    if (data_size != bufsize)
        return false;

    /* compare data */
    ret = element_find(flash_env, ns_idx, key, &page, &entry_idx, NULL, ELEMENT_ANY);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), false);

    /* read entry */
    ret = entry_read(flash_env, page, entry_idx, &entry);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), false);

    type = tag_element_type_get(entry.tag);
    if (type == ELEMENT_SMALL) {
        if (entry.length != bufsize)
            return false;
        /* compare data */
        if (!sys_memcmp(buf, entry.value, entry.length)) {
            return true;
        } else {
            return false;
        }
    } else if (type == ELEMENT_MIDDLE) {
        if (entry.length != bufsize)
            return false;
        dst = buf;
        remain = entry.length;

        entry_start = entry_idx + 1;
        entry_end = entry_idx + ((entry.length + ENTRY_SIZE - 1) / ENTRY_SIZE);

        for (entry_idx = entry_start; entry_idx <= entry_end; ++entry_idx) {
            ret = entry_read(flash_env, page, entry_idx, &entry);
            NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), false);

            copy = (remain < ENTRY_SIZE) ? remain : ENTRY_SIZE;
            if (sys_memcmp(dst, entry.data, copy)) {
                return false;
            }
            remain -= copy;
            dst += copy;
        }

        return true;
    } else if ((type == ELEMENT_BULK) || (type == ELEMENT_BULKINFO)) {
        return bulk_element_compare(flash_env, ns_idx, key, buf, bufsize);
    }

    return false;
}

static int page_clear(struct nvds_flash_env_tag *flash_env, struct page_env_tag *page)
{
    if (!flash_env || !page)
        return NVDS_ERR(NVDS_E_FAIL);

    sys_memset(&page->header, 0xFF, sizeof(page->header));
    sys_memset(&page->entry_states, 0xFF, ENTRY_SIZE);
    page->entry_cnt_used = 0;
    page->next_free_idx = 0;

    return NVDS_ERR(NVDS_OK);
}

static struct page_env_tag * new_page_request(struct nvds_flash_env_tag *flash_env, uint32_t seq)
{
    struct page_env_tag *page;
    struct page_env_tag *p;
    struct page_env_tag *erase_page = NULL;
    uint16_t min_cnt_used = ENTRY_COUNT_PER_PAGE;
    union entry_info entry;
    uint8_t entry_idx;
    enum entry_state state;

    if (list_is_empty(&flash_env->nvds_page_free))
        return NULL;

    if (list_cnt(&flash_env->nvds_page_free) == 1) {
        /* candidate page */
        /* find max erase entry page */
        p = (struct page_env_tag *)list_pick(&flash_env->nvds_page_used);
        while (p) {
            if (p->entry_cnt_used < min_cnt_used) {
                min_cnt_used = p->entry_cnt_used;
                erase_page = p;
            }
            p = (struct page_env_tag *)list_next(&p->list_hdr);
        }

        /* all the entries are on-use state in all full page.
        then we can not find candidate page to erase. */
        if (!erase_page)
            return NULL;
    }

    page = (struct page_env_tag *)list_pick(&flash_env->nvds_page_free);
    if (page->header.state != PAGE_UNINITIALIZED) {
        if (nvds_flash_erase(flash_env, page->base_addr, SPI_FLASH_SEC_SIZE))
            return NULL;
    }

    /* initialize page header */
    page_clear(flash_env, page);
    page->header.magic = NVDS_FLASH_MAGIC;
    page->header.version = NVDS_FLASH_VERSION;
    page->header.state = PAGE_ACTIVE;
    page->header.seqno = seq;
    page->header.crc32 = page_header_crc32_calc(&page->header);

    if (nvds_flash_write(flash_env, page->base_addr + PAGE_HEADER_OFFSET,
        sizeof(page->header), (uint8_t *)&page->header))
        return NULL;

    /* mark current page to full */
    if (page_state_alter(flash_env, page, PAGE_ACTIVE))
        return NULL;

    /* select a page from free list, and move it to used list */
    list_extract(&flash_env->nvds_page_free, &page->list_hdr);
    list_push_back(&flash_env->nvds_page_used, &page->list_hdr);
    if (!list_is_empty(&flash_env->nvds_page_free))
        return page;

    /* move candidate page from used list to free list */
    list_extract(&flash_env->nvds_page_used, &erase_page->list_hdr);
    list_push_back(&flash_env->nvds_page_free, &erase_page->list_hdr);
    /* mark current page to full */
    if (page_state_alter(flash_env, erase_page, PAGE_CANDIDATE))
        return NULL;

    /* copy erase page */
    /* walk entry */
    for (entry_idx = 0; entry_idx < ENTRY_COUNT_PER_PAGE; entry_idx++) {
        entry_state_get(erase_page->entry_states, entry_idx, &state);
        if (state != ENTRY_USED)
            continue;

        if (entry_read(flash_env, erase_page, entry_idx, &entry))
            return NULL;

        if (entry_write(flash_env, page, &entry))
            return NULL;

        if (entry_state_alter(flash_env, page, page->next_free_idx - 1, ENTRY_USED))
            return NULL;
    }

    /* initializ erase page */
    if (nvds_flash_erase(flash_env, erase_page->base_addr, SPI_FLASH_SEC_SIZE))
        return NULL;

    page_clear(flash_env, erase_page);

    return page;
}

static int bulk_element_put(struct nvds_flash_env_tag *flash_env, uint8_t ns_idx, const char* key, uint8_t *buf, uint32_t bufsize)
{
    struct page_env_tag *cur_page;
    union entry_info entry;
    uint32_t address;
    uint32_t entry_cnt;
    uint32_t length;
    uint8_t entry_start;
    uint32_t remain;
    uint16_t frag_cnt = 0;
    int ret;
    uint16_t buf_offset = 0;
    uint8_t entry_idx = 0;
    struct page_env_tag *page_start = NULL;
    struct page_env_tag *page;

    cur_page = (struct page_env_tag *)list_pick_last(&flash_env->nvds_page_used);
    /* Sanity check : page state should only is PAGE_UNINITIALIZED / PAGE_ACTIVE / PAGE_FULL */
    if ((cur_page->header.state == PAGE_INVALID)
        || (cur_page->header.state == PAGE_ERROR)
        || (cur_page->header.state == PAGE_CANDIDATE)) {
        //printf("NVDS ERR : illegal page is being writing...\n");
        return NVDS_ERR(NVDS_E_FAIL);
    }

    buf_offset = 0;
    remain = bufsize;
    do {
        if (page_room_get(cur_page) < 2 * ENTRY_SIZE) {
            /* mark current page to full */
            ret = page_state_alter(flash_env, cur_page, PAGE_FULL);
            NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

            cur_page = new_page_request(flash_env, cur_page->header.seqno + 1);
            if (!cur_page)
                goto no_space_out;

            if (page_room_get(cur_page) < 2 * ENTRY_SIZE)
                goto no_space_out;
        }

        length = (page_room_get(cur_page) - ENTRY_SIZE) < remain ?
            (page_room_get(cur_page) - ENTRY_SIZE) : remain;
        entry_start = cur_page->next_free_idx;
        entry_cnt = (length + ENTRY_SIZE - 1) / ENTRY_SIZE;

        /* bulk frag entry */
        sys_memset((void*)&entry, 0xFF, sizeof(union entry_info));
        tag_set(ns_idx, ELEMENT_BULK, frag_cnt, &entry.tag);
        entry.length = length;
        sys_memcpy(entry.key, key, strlen(key));
        entry.key[strlen(key)] = 0;
        entry.varlen_info_t.datacrc32 = element_data_crc32_calc(buf, length);
        entry.crc32 = element_header_crc32_calc(&entry);
        ret = entry_write(flash_env, cur_page, &entry);
        NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

        /* write frag data */
        address = cur_page->base_addr + PAGE_ENTRY_OFFSET + cur_page->next_free_idx * ENTRY_SIZE;
        ret = entry_data_write(flash_env, address, length, buf + buf_offset);
        NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

        cur_page->next_free_idx += entry_cnt;
        cur_page->entry_cnt_used += entry_cnt;

        /* modify element states */
        ret = entry_state_range_alter(flash_env, cur_page, entry_start, entry_start + entry_cnt, ENTRY_USED);
        NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

        remain -= length;
        buf_offset += length;
        frag_cnt++;

#ifdef NVDS_DEBUG
        page_print(flash_env, cur_page);
#endif
    } while (remain > 0);

    /* write bulkinfo */
    cur_page = (struct page_env_tag *)list_pick_last(&flash_env->nvds_page_used);
    /* Sanity check : page state should only is PAGE_UNINITIALIZED / PAGE_ACTIVE / PAGE_FULL */
    if ((cur_page->header.state == PAGE_INVALID)
        || (cur_page->header.state == PAGE_ERROR)
        || (cur_page->header.state == PAGE_CANDIDATE)) {
        //printf("NVDS ERR : illegal page is being writing...\n");
        return NVDS_ERR(NVDS_E_FAIL);
    }

    if (page_room_get(cur_page) < ENTRY_SIZE) {
        /* mark current page to full */
        ret = page_state_alter(flash_env, cur_page, PAGE_FULL);
        NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

        cur_page = new_page_request(flash_env, cur_page->header.seqno + 1);
        if (!cur_page)
            goto no_space_out;

        if (page_room_get(cur_page) < ENTRY_SIZE)
            goto no_space_out;
    }

    /* bulkinfo entry */
    entry_start = cur_page->next_free_idx;
    sys_memset((void*)&entry, 0xFF, sizeof(union entry_info));
    tag_set(ns_idx, ELEMENT_BULKINFO, TAG_FRAG_NO_DEFAULT, &entry.tag);
    entry.length = frag_cnt;
    sys_memcpy(entry.key, key, strlen(key));
    entry.key[strlen(key)] = 0;
    entry.bulk_info_t.bulksize = bufsize;
    entry.crc32 = element_header_crc32_calc(&entry);
    ret = entry_write(flash_env, cur_page, &entry);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

    ret = entry_state_range_alter(flash_env, cur_page, entry_start, entry_start, ENTRY_USED);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

#ifdef NVDS_DEBUG
    page_print(flash_env, cur_page);
#endif

    return NVDS_ERR(NVDS_OK);
no_space_out:
    /* when we don't have any flash space to save the whole bulk element,
    mark the already saved fragment as invalid. */
    entry_idx = 0;
    page_start = NULL;
    /* find all the remain bulk entry */
    do {
        ret = element_find(flash_env, ns_idx, key, &page, &entry_idx, page_start, ELEMENT_BULK);
        if (ret == NVDS_ERR(NVDS_E_NOT_FOUND)) {
            break;
        } else if (ret) {
            return ret;
        }

        /* modify all the remain bulk entry states to ENTRY_UPDATED */
        ret = entry_read(flash_env, page, entry_idx, &entry);
        NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

        entry_cnt = (entry.length + ENTRY_SIZE - 1) / ENTRY_SIZE;
        ret = entry_state_range_alter(flash_env, page, entry_idx, entry_idx + entry_cnt, ENTRY_UPDATED);
        NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);
        page->entry_cnt_used -= (entry_cnt + 1);

        entry_idx += (entry_cnt + 1);
        page_start = page;
    } while (page_start);

    return NVDS_ERR(NVDS_E_NO_SPACE);
}

static int data_element_put(struct nvds_flash_env_tag *flash_env, uint8_t ns_idx, const char* key, uint8_t *buf, uint32_t bufsize)
{
    struct page_env_tag *cur_page;
    union entry_info entry;
    enum element_type type;
    int entry_start;
    int entry_cnt;
    uint32_t address;
    int ret;

    /* compare with current value if exist */
    if (element_compare(flash_env, ns_idx, key, buf, bufsize)) {
        /* find success need to subtract used_cnt one, data_element_put return ok will add one */
        ns_del_used_cnt(flash_env, ns_idx, 0);
        return NVDS_ERR(NVDS_OK);
    }

    /* del old element */
    ret = data_element_del(flash_env, ns_idx, key);
    if (ret == NVDS_ERR(NVDS_OK))
        /* delete success need to subtract used_cnt one */
        ns_del_used_cnt(flash_env, ns_idx, 0);
    else if (ret != NVDS_ERR(NVDS_E_NOT_FOUND))
        return NVDS_ERR(ret);

    if (bufsize > ELEMENT_MIDDLE_MAX_SIZE)
        return bulk_element_put(flash_env, ns_idx, key, buf, bufsize);

    cur_page = (struct page_env_tag *)list_pick_last(&flash_env->nvds_page_used);
    /* Sanity check : page state should only is PAGE_UNINITIALIZED / PAGE_ACTIVE / PAGE_FULL */
    if ((cur_page->header.state == PAGE_INVALID)
        || (cur_page->header.state == PAGE_ERROR)
        || (cur_page->header.state == PAGE_CANDIDATE)) {
        //printf("NVDS ERR : illegal page is being writing...\n");
        return NVDS_ERR(NVDS_E_FAIL);
    }

    if (((bufsize <= ELEMENT_SMALL_MAX_SIZE) && (page_room_get(cur_page) < ENTRY_SIZE)) ||
        ((bufsize > ELEMENT_SMALL_MAX_SIZE) && (page_room_get(cur_page) < (ENTRY_SIZE + bufsize)))) {
        /* mark current page to full */
        ret = page_state_alter(flash_env, cur_page, PAGE_FULL);
        NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

        /* request new page */
        cur_page = new_page_request(flash_env, cur_page->header.seqno + 1);
        if (!cur_page)
            return NVDS_ERR(NVDS_E_NO_SPACE);

        if (((bufsize <= ELEMENT_SMALL_MAX_SIZE) && (page_room_get(cur_page) < ENTRY_SIZE)) ||
            ((bufsize > ELEMENT_SMALL_MAX_SIZE) && (page_room_get(cur_page) < (ENTRY_SIZE + bufsize)))) {
            return NVDS_ERR(NVDS_E_NO_SPACE);
        }
    }

    /* write to current page */
    entry_start = cur_page->next_free_idx;
    entry_cnt = (bufsize + ENTRY_SIZE - 1) / ENTRY_SIZE;
    sys_memset((void*)&entry, 0xFF, sizeof(union entry_info));
    type = ELEMENT_SMALL;
    if (bufsize > ELEMENT_SMALL_MAX_SIZE)
        type = ELEMENT_MIDDLE;
    tag_set(ns_idx, type, TAG_FRAG_NO_DEFAULT, &entry.tag);
    entry.length = bufsize;
    sys_memcpy(entry.key, key, strlen(key));
    entry.key[strlen(key)] = 0;

    if (type == ELEMENT_SMALL) {
        sys_memcpy(entry.value, buf, bufsize);
    } else {
        entry.varlen_info_t.datacrc32 = element_data_crc32_calc(buf, bufsize);
    }

    entry.crc32 = element_header_crc32_calc(&entry);
    ret = entry_write(flash_env, cur_page, &entry);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

    if (type == ELEMENT_MIDDLE) {
        address = cur_page->base_addr + PAGE_ENTRY_OFFSET + cur_page->next_free_idx * ENTRY_SIZE;
        ret = entry_data_write(flash_env, address, bufsize, buf);
        NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

        cur_page->next_free_idx += entry_cnt;
        cur_page->entry_cnt_used += entry_cnt;

        /* modify element states */
        ret = entry_state_range_alter(flash_env, cur_page, entry_start, entry_start + entry_cnt, ENTRY_USED);
        NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);
    } else {
        ret = entry_state_range_alter(flash_env, cur_page, entry_start, entry_start, ENTRY_USED);
        NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);
    }

#ifdef NVDS_DEBUG
    page_print(flash_env, cur_page);
#endif

    return NVDS_ERR(NVDS_OK);
}

static int namespace_create(struct nvds_flash_env_tag *flash_env, const char* namespace,
                                    uint8_t *index)
{
    struct namespace_info* new_ns;
    uint8_t ns_idx;
    bool ns_state;
    int ret;

    /* find next free ns index */
    for (ns_idx = 1; ns_idx < NAMESPACE_NULL_IDX; ++ns_idx) {
        ns_state_get(flash_env->ns_states, ns_idx, &ns_state);
        if (!ns_state) {
            break;
        }
    }

    if (ns_idx == NAMESPACE_NULL_IDX) {
        /* all ns are used */
        return NVDS_ERR(NVDS_E_NO_SPACE);
    }

    new_ns = sys_malloc(sizeof(*new_ns));
    if (!new_ns)
        return NVDS_ERR(NVDS_E_NO_SPACE);
    sys_memset(new_ns, 0, sizeof(*new_ns));

    /* create new namespace */
    ret = data_element_put(flash_env, 0, namespace, &ns_idx, 1);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

    ns_state_set(flash_env->ns_states, ns_idx, true);

    sys_memcpy(new_ns->name, namespace, strlen(namespace));
    new_ns->name[strlen(namespace) + 1] = 0;
    new_ns->index = ns_idx;
    list_push_back(&flash_env->ns_list, &new_ns->list_hdr);
    *index = ns_idx;

    return NVDS_ERR(NVDS_OK);
}

static int ns_index_by_namespace(struct nvds_flash_env_tag *flash_env, const char* namespace,
                                    bool create, uint8_t *index)
{
    struct namespace_info *ns;
    int ret;
    uint32_t ns_len;

    if (!namespace) {
        *index = NAMESPACE_NULL_IDX;
    } else {
        ns_len = strlen(namespace);
        if (ns_len > (KEY_NAME_MAX_SIZE - 1))
            return NVDS_ERR(NVDS_E_INVAL_PARAM);

        ns = (struct namespace_info *)list_pick(&flash_env->ns_list);
        while (ns) {
            if(strlen(ns->name) == ns_len && !sys_memcmp(ns->name, namespace, ns_len)) {
                *index = ns->index;
                break;
            }

            ns = (struct namespace_info*)list_next(&ns->list_hdr);
        }

        if (!ns) {
            /* not find namespace */
            if (create) {
                ret = namespace_create(flash_env, namespace, index);
                NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);
            } else {
                return NVDS_ERR(NVDS_E_NOT_FOUND);
            }
        }
    }

    return NVDS_ERR(NVDS_OK);
}

static int ns_add_used_cnt(struct nvds_flash_env_tag *flash_env, uint8_t ns_idx)
{
    struct namespace_info *ns;

    if (!flash_env || ns_idx > NAMESPACE_MAX_CNT)
        return NVDS_ERR(NVDS_E_INVAL_PARAM);

    ns = (struct namespace_info *)list_pick(&flash_env->ns_list);
    while (ns) {
        if (ns->index == ns_idx) {
            ns->used_cnt++;
            break;
        }

        ns = (struct namespace_info*)list_next(&ns->list_hdr);
    }

    return NVDS_ERR(NVDS_OK);
}

static int ns_del_used_cnt(struct nvds_flash_env_tag *flash_env, uint8_t ns_idx, uint8_t del_flag)
{
    struct namespace_info *ns;
    int ret;

    if (!flash_env || ns_idx > NAMESPACE_MAX_CNT)
        return NVDS_ERR(NVDS_E_INVAL_PARAM);

    ns = (struct namespace_info *)list_pick(&flash_env->ns_list);
    while (ns) {
        if (ns->index == ns_idx) {
            ns->used_cnt--;
            if (ns->used_cnt == 0 && del_flag == 1) {
                list_extract(&flash_env->ns_list, &ns->list_hdr);
                ns_state_set(flash_env->ns_states, ns_idx, false);

                /* delete namespace */
                ret = data_element_del(flash_env, 0, ns->name);
                NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

                sys_mfree(ns);
            }
            break;
        }

        ns = (struct namespace_info*)list_next(&ns->list_hdr);
    }

    return NVDS_ERR(NVDS_OK);
}

static void page_header_read(struct nvds_flash_env_tag *flash_env, struct page_env_tag *page)
{
    int ret;
    struct page_header *header = &page->header;

    /* read page header */
    ret = nvds_flash_read(flash_env, page->base_addr + PAGE_HEADER_OFFSET,
                            sizeof(struct page_header), (uint8_t *)header);
    if (ret != 0) {
        header->state = PAGE_INVALID;
        return;
    }

    if (PAGE_UNINITIALIZED == header->state) {
        /* when page state indicate uninitialized, the whole page should be empty too.*/
        /* read whole page data to check if the page is empty */
        const int BLOCK_SIZE = 128;
        uint32_t buf_check[BLOCK_SIZE];
        uint32_t idx;
        for (uint32_t i = 0; i < SPI_FLASH_SEC_SIZE; i += 4 * BLOCK_SIZE) {
            ret = nvds_flash_read(flash_env, page->base_addr + PAGE_HEADER_OFFSET + i,
                                    4 * BLOCK_SIZE, (uint8_t *)buf_check);
            if (ret != 0) {
                header->state = PAGE_INVALID;
                return;
            }

            idx = 0;
            while (idx < BLOCK_SIZE) {
                if (buf_check[idx++] != 0xFFFFFFFF) {
                    header->state = PAGE_ERROR;
                    return;
                }
            }
        }
        header->state = PAGE_UNINITIALIZED;
    } else if (header->crc32 != page_header_crc32_calc(header)) {
        header->state = PAGE_ERROR;
        return;
    } else {
        /* check magic */
        if (header->magic != NVDS_FLASH_MAGIC) {
            header->state = PAGE_ERROR;
            return;
        }
    }

    return;
}

/**
 ****************************************************************************************
 * @brief Compare sequence number.
 *
 * @param[in] pageA Sequence to compare.
 * @param[in] pageB Sequence to compare.
 *
 * @return true if SequenceA small than SequenceB.
 ****************************************************************************************
 */
static bool cmp_sequence_no(struct list_hdr const * pageA, struct list_hdr const * pageB)
{
    uint32_t seqA = ((struct page_env_tag*)pageA)->header.seqno;
    uint32_t seqB = ((struct page_env_tag*)pageB)->header.seqno;

    return ((seqA & ~BIT(31)) < (seqB & ~BIT(31)));
}

static int pages_load(struct nvds_flash_env_tag *flash_env)
{
    uint32_t sector_cnt;
    uint32_t sector_idx;
    uint32_t entry_idx;
    uint32_t entry_cnt;
    struct page_env_tag *page;
    union entry_info entry;
    enum entry_state state;
    struct namespace_info *ns_info;
    enum element_type type;
    bool is_err = false;
    uint8_t ns;

    if (!flash_env)
        return NVDS_ERR(NVDS_E_INVAL_PARAM);

    if (flash_env->length % SPI_FLASH_SEC_SIZE)
        return NVDS_ERR(NVDS_E_INVAL_PARAM);

    sector_cnt = flash_env->length / SPI_FLASH_SEC_SIZE;

    /* walk page to create namespace list and element list */
    for (sector_idx = 0; sector_idx < sector_cnt; ++sector_idx) {
        page = sys_malloc(sizeof(struct page_env_tag));
        if (!page)
            return NVDS_ERR(NVDS_E_NO_SPACE);
        sys_memset(page, 0, sizeof(struct page_env_tag));

        page->base_addr = sector_idx * SPI_FLASH_SEC_SIZE;

        /* detect page header, page data would be ignored when error occured */
        page_header_read(flash_env, page);
        if ((page->header.state == PAGE_UNINITIALIZED)
            || (page->header.state == PAGE_ERROR)
            || (page->header.state == PAGE_INVALID)) {
            list_push_back(&flash_env->nvds_page_free, &page->list_hdr);
            continue;
        }

        /* load entry states table where we actually care about in the page */
        if (entry_states_table_read(flash_env, page->base_addr, page->entry_states)) {
            list_push_back(&flash_env->nvds_page_free, &page->list_hdr);
            continue;
        }

        /* next_free_idx is the first unused entry, need initialize to ENTRY_COUNT_PER_PAGE,
        otherwise there is problem when the last page is full of load */
        page->next_free_idx = ENTRY_COUNT_PER_PAGE;
        page->entry_cnt_used = 0;

        /* walk entry */
        for (entry_idx = 0; entry_idx < ENTRY_COUNT_PER_PAGE;) {
            /* get the entry state from states table */
            entry_state_get(page->entry_states, entry_idx, &state);

            if (state == ENTRY_FREE) {
                /* check following whether if read empty */
                /* record next free entry idx */
                page->next_free_idx = entry_idx;
                break;
            } else if (state == ENTRY_ILLEGAL) {
                /* mark entry state to updated */
                entry_state_alter(flash_env, page, entry_idx, ENTRY_UPDATED);
                entry_idx++;
                continue;
            } else if (state == ENTRY_UPDATED) {
                entry_idx++;
                continue;
            } else {
                /* read entry data */
                if (entry_read(flash_env, page, entry_idx, &entry)) {
                    is_err = true;
                    break;
                }

                /* check entry crc */
                if (entry.crc32 != element_header_crc32_calc(&entry)) {
                    entry_state_alter(flash_env, page, entry_idx, ENTRY_UPDATED);
                }

                ns = tag_namespace_get(entry.tag);
                if (ns == 0) {
                    /* save namespace to list when find ns index = 0 */
                    ns_info = sys_malloc(sizeof(struct namespace_info));
                    if (!ns_info)
                        return NVDS_ERR(NVDS_E_NO_SPACE);
                    sys_memset(ns_info, 0, sizeof(struct namespace_info));
                    ns_info->index = entry.value[0];
                    sys_memcpy(ns_info->name, entry.key, strlen(entry.key));
                    list_push_back(&flash_env->ns_list, &ns_info->list_hdr);

                    /* set the ns_idx has already used */
                    ns_state_set(flash_env->ns_states, ns_info->index, true);
                } else {
                    /* save element hash list */
                }

                /* entry index should skip element data if exist */
                entry_idx++;
                page->entry_cnt_used++;

                type = tag_element_type_get(entry.tag);
                if ((ELEMENT_MIDDLE == type) || (ELEMENT_BULK == type)) {
                    entry_cnt = (entry.length + ENTRY_SIZE - 1) / ENTRY_SIZE;
                    entry_idx += entry_cnt;
                    /* record used entry idx */
                    page->entry_cnt_used += entry_cnt;
                }
            }
        }

        if (is_err) {
            list_push_back(&flash_env->nvds_page_free, &page->list_hdr);
        } else {
            list_insert(&flash_env->nvds_page_used, &page->list_hdr, cmp_sequence_no);
            //co_list_push_back(&flash_env->nvds_page_used, &page->list_hdr);
        }
    }

    /* walk used page list to record namespace used count */
    page = (struct page_env_tag *)list_pick(&flash_env->nvds_page_used);
    while (page) {
        /* walk entry */
        for (entry_idx = 0; entry_idx < ENTRY_COUNT_PER_PAGE;) {
            if (entry_read(flash_env, page, entry_idx, &entry)) {
                break;
            }

            entry_state_get(page->entry_states, entry_idx, &state);

            if (state == ENTRY_USED) {
                /* find */
                ns_info = (struct namespace_info *)list_pick(&flash_env->ns_list);
                ns = tag_namespace_get(entry.tag);
                while (ns_info) {
                    if (ns == ns_info->index) {
                        ns_info->used_cnt++;
                        break;
                    }

                    ns_info = (struct namespace_info *)list_next(&ns_info->list_hdr);
                }
            }

            entry_idx++;
            type = tag_element_type_get(entry.tag);
            if ((ELEMENT_MIDDLE == type) || (ELEMENT_BULK == type)) {
                entry_cnt = (entry.length + ENTRY_SIZE - 1) / ENTRY_SIZE;
                entry_idx += entry_cnt;
            }
        }

        page = (struct page_env_tag*)list_next(&page->list_hdr);
    }

    /* push one page to used if used page list is empty */
    if (list_is_empty(&flash_env->nvds_page_used))
        new_page_request(flash_env, 0);

    return NVDS_ERR(NVDS_OK);
}

static const char* page_state_string_get(uint16_t state)
{
    if (state == PAGE_UNINITIALIZED)
        return "empty";
    else if (state == PAGE_ACTIVE)
        return "active";
    else if (state == PAGE_FULL)
        return "full";
    else if (state == PAGE_CANDIDATE)
        return "";
    else if (state == PAGE_ERROR)
        return "error";
    else if (state == PAGE_INVALID)
        return "invalid";
    else
        return "unknown";
}

static const char* entry_state_string_get(enum entry_state state)
{
    switch(state) {
    case ENTRY_FREE:
        return "empty";
    case ENTRY_USED:
        return "on-use";
    case ENTRY_UPDATED:
        return "deleted";
    case ENTRY_ILLEGAL:
        return "illegal";
    default:
        return "error";
    }
}

static const char* element_type_string_get(enum element_type type)
{
    switch (type) {
    case ELEMENT_SMALL:
        return "small";
    case ELEMENT_MIDDLE:
        return "middle";
    case ELEMENT_BULK:
        return "bulk";
    case ELEMENT_BULKINFO:
        return "bulk info";
    default:
        return "unknown";
    }
}

static void nvds_dump_from_flash(struct nvds_flash_env_tag *flash_env)
{
    uint32_t sector_cnt;
    uint32_t sector_idx;
    struct page_env_tag page;
    uint8_t entry_idx;
    enum entry_state state;
    union entry_info entry;
    uint8_t entry_cnt;
    enum element_type type;
    uint8_t ns;
    uint32_t remain, size;
    int i;

    /* dump nvds flash basic inforamtion */
    dbg_print(NOTICE, "======basic information======\r\n");
    dbg_print(NOTICE, "label\t:%s\r\n", flash_env->label);
    dbg_print(NOTICE, "address\t:0x%08X ~ 0x%08X\r\n", flash_env->base_addr, flash_env->base_addr + flash_env->length - 1);

    /* read all pages */
    sector_cnt = flash_env->length / SPI_FLASH_SEC_SIZE;
    for (sector_idx = 0; sector_idx < sector_cnt; ++sector_idx) {
        /* print page information */
        dbg_print(NOTICE, "======page======\r\n");
        page.base_addr = sector_idx * SPI_FLASH_SEC_SIZE;
        /* read page header, uninitialized / error / invalid page will skip */
        page_header_read(flash_env, &page);
        dbg_print(NOTICE, "sector:%d, magic:0x%x, version:0x%x, state:%s, seq:%d\r\n",
            sector_idx,
            page.header.magic,
            page.header.version,
            page_state_string_get(page.header.state),
            page.header.seqno);

        if ((page.header.state == PAGE_UNINITIALIZED)
            || (page.header.state == PAGE_ERROR)
            || (page.header.state == PAGE_INVALID)) {
            continue;
        }

        /* read entry state table */
        entry_states_table_read(flash_env, page.base_addr, page.entry_states);

        /* walk entry */
        for (entry_idx = 0; entry_idx < ENTRY_COUNT_PER_PAGE;) {
            entry_state_get(page.entry_states, entry_idx, &state);
            dbg_print(NOTICE, "entry[%d] offset:0x%03X, state:%s, ",
                entry_idx,
                PAGE_ENTRY_OFFSET + entry_idx * ENTRY_SIZE,
                entry_state_string_get(state));

            if (state == ENTRY_FREE) {
                entry_idx++;
                dbg_print(NOTICE, "\r\n");
                continue;
            }

            entry_read(flash_env, &page, entry_idx, &entry);
            ns = tag_namespace_get(entry.tag);
            type = tag_element_type_get(entry.tag);

            /* show bulk info entry */
            if(ELEMENT_BULKINFO == type) {
                dbg_print(NOTICE, "type:%s, ns:%d, key:%s, frag cnt:%d, total size:%d\r\n",
                    element_type_string_get(type),
                    ns,
                    entry.key,
                    entry.length,
                    entry.bulk_info_t.bulksize);

                entry_idx++;
                continue;
            }

            dbg_print(NOTICE, "type:%s, ns:%d, ", element_type_string_get(type), ns);
            if (ns == 0) {
                dbg_print(NOTICE, "key:%s, length:%d\r\n", entry.key, entry.length);
            } else {
                dbg_print(NOTICE, "key_str:%s, key_hex:", entry.key);
                size = strlen(entry.key);
                for(i = 0; i < size; i++) {
                    dbg_print(NOTICE, "%x ", (uint8_t)entry.key[i]);
                }
                dbg_print(NOTICE, ", length:%d\r\n", entry.length);
            }

            if (ELEMENT_BULK == type) {
                dbg_print(NOTICE, "\tfrag[%d] value:", tag_fragno_get(entry.tag));
            } else {
                dbg_print(NOTICE, "\tvalue:");
            }

            if (ELEMENT_SMALL == type) {
                for(i = 0; i < entry.length; i++) {
                    dbg_print(NOTICE, "%02x ", entry.value[i]);
                }
                dbg_print(NOTICE, "\r\n");
                entry_idx++;
            } else {
                entry_idx++;
                entry_cnt = (entry.length + ENTRY_SIZE - 1) / ENTRY_SIZE;
                remain = entry.length;
                for(int idx = 0; idx < entry_cnt; idx++) {
                    entry_read(flash_env, &page, entry_idx + idx, &entry);
                    size = (remain > ENTRY_SIZE) ? ENTRY_SIZE : remain;
                    for(i = 0; i < size; i++) {
                        dbg_print(NOTICE, "%02x ", entry.data[i]);
                        if ((i + 1) % 16 == 0)
                            dbg_print(NOTICE, "\r\n\t\t");
                    }
                    remain -= size;
                }
                dbg_print(NOTICE, "\r\n");
                entry_idx += entry_cnt;
            }
        }
    }
}

static void nvds_dump_from_list(struct nvds_flash_env_tag *flash_env)
{
    struct page_env_tag *page;
    enum entry_state state;
    union entry_info entry;
    uint8_t entry_cnt;
    uint8_t entry_idx;
    enum element_type type;
    struct namespace_info *ns_info;
    uint8_t ns;
    uint32_t remain, size;
    int i;

    /* dump nvds flash basic inforamtion */
    dbg_print(NOTICE, "======basic information======\r\n");
    dbg_print(NOTICE, "label\t:%s\r\n", flash_env->label);
    dbg_print(NOTICE, "address\t:0x%08X ~ 0x%08X\r\n", flash_env->base_addr, flash_env->base_addr + flash_env->length - 1);
    dbg_print(NOTICE, "used page\t:%d\r\n", list_cnt(&flash_env->nvds_page_used));
    dbg_print(NOTICE, "free page\t:%d\r\n", list_cnt(&flash_env->nvds_page_free));

    /* dump namespace list information */
    dbg_print(NOTICE, "======namespace======\r\n");
    ns_info = (struct namespace_info *)list_pick(&flash_env->ns_list);
    while (ns_info) {
        dbg_print(NOTICE, "[%d]:%s, storaged key:%d\r\n", ns_info->index, ns_info->name, ns_info->used_cnt);
        ns_info = (struct namespace_info*)list_next(&ns_info->list_hdr);
    }

    /* parsing used page list */
    page = (struct page_env_tag *)list_pick(&flash_env->nvds_page_used);
    while (page) {
        /* print page information */
        dbg_print(NOTICE, "======page======\r\n");
        dbg_print(NOTICE, "sector:%d, state:%s, seq:%d, entry:%d\r\n",
            page->base_addr / SPI_FLASH_SEC_SIZE,
            page_state_string_get(page->header.state),
            page->header.seqno, page->entry_cnt_used);

        /* walk entry */
        for (entry_idx = 0; entry_idx < ENTRY_COUNT_PER_PAGE;) {
            /* only dump entry state is ENTRY_USED */
            entry_state_get(page->entry_states, entry_idx, &state);
            if (state != ENTRY_USED) {
                entry_idx++;
                continue;
            }

            entry_read(flash_env, page, entry_idx, &entry);
            /* namespace define will skip */
            ns = tag_namespace_get(entry.tag);
            if (!ns){
                entry_idx++;
                continue;
            }
            type = tag_element_type_get(entry.tag);

            /* show bulk info entry */
            if(ELEMENT_BULKINFO == type) {
                dbg_print(NOTICE, "entry[%d] offset:0x%03X, state:%s, type:%s, ns:%d, key:%s, frag cnt:%d, total size:%d\r\n",
                        entry_idx,
                        PAGE_ENTRY_OFFSET + entry_idx * ENTRY_SIZE,
                        entry_state_string_get(state),
                        element_type_string_get(type),
                        ns,
                        entry.key,
                        entry.length,
                        entry.bulk_info_t.bulksize);

                entry_idx++;
                continue;
            }

            dbg_print(NOTICE, "entry[%d] offset:0x%03X, state:%s, type:%s, ns:%d, ",
                entry_idx,
                PAGE_ENTRY_OFFSET + entry_idx * ENTRY_SIZE,
                entry_state_string_get(state),
                element_type_string_get(type),
                ns);
            if (ns == 0) {
                dbg_print(NOTICE, "key:%s, length:%d\r\n", entry.key, entry.length);
            } else {
                dbg_print(NOTICE, "key_str:%s, key_hex:", entry.key);
                size = strlen(entry.key);
                for(i = 0; i < size; i++) {
                    dbg_print(NOTICE, "%x ", (uint8_t)entry.key[i]);
                }
                dbg_print(NOTICE, ", length:%d\r\n", entry.length);
            }

            if (ELEMENT_BULK == type) {
                dbg_print(NOTICE, "\tfrag[%d] value:", tag_fragno_get(entry.tag));
            } else {
                dbg_print(NOTICE, "\tvalue:");
            }

            if (ELEMENT_SMALL == type) {
                for(i = 0; i < entry.length; i++) {
                    dbg_print(NOTICE, "%02x ", entry.value[i]);
                }
                dbg_print(NOTICE, "\r\n");
                entry_idx++;
            } else {
                entry_idx++;
                entry_cnt = (entry.length + ENTRY_SIZE - 1) / ENTRY_SIZE;
                remain = entry.length;
                for(int idx = 0; idx < entry_cnt; idx++) {
                    entry_read(flash_env, page, entry_idx + idx, &entry);
                    size = (remain > ENTRY_SIZE) ? ENTRY_SIZE : remain;
                    for(i = 0; i < size; i++) {
                        dbg_print(NOTICE, "%02x ", entry.data[i]);
                        if ((i + 1) % 16 == 0)
                            dbg_print(NOTICE, "\r\n\t\t");
                    }
                    remain -= size;
                }
                dbg_print(NOTICE, "\r\n");
                entry_idx += entry_cnt;
            }
        }
        page = (struct page_env_tag*)list_next(&page->list_hdr);
    }
}

static void nvds_dump_namespace(struct nvds_flash_env_tag *flash_env, const char *namespace)
{
    struct page_env_tag *page;
    enum entry_state state;
    union entry_info entry;
    uint8_t entry_cnt;
    uint8_t entry_idx;
    enum element_type type;
    uint32_t remain, size;
    int i;
    uint8_t ns_idx;
    int ret;

    /* find ns idx by namespace */
    ret = ns_index_by_namespace(flash_env, namespace, false, &ns_idx);
    if (ret != NVDS_ERR(NVDS_OK))
        return;

    /* parsing used page list */
    page = (struct page_env_tag *)list_pick(&flash_env->nvds_page_used);
    while (page) {
        /* walk entry */
        for (entry_idx = 0; entry_idx < ENTRY_COUNT_PER_PAGE;) {
            /* only dump entry state is ENTRY_USED */
            entry_state_get(page->entry_states, entry_idx, &state);
            if (state != ENTRY_USED) {
                entry_idx++;
                continue;
            }

            entry_read(flash_env, page, entry_idx, &entry);

            if (ns_idx != tag_namespace_get(entry.tag)) {
                entry_idx++;
                continue;
            }

            type = tag_element_type_get(entry.tag);

            /* show bulk info entry */
            if(ELEMENT_BULKINFO == type) {
                dbg_print(NOTICE, "entry[%d] offset:0x%03X, state:%s, type:%s, ns:%d, key:%s, frag cnt:%d, total size:%d\r\n",
                entry_idx,
                PAGE_ENTRY_OFFSET + entry_idx * ENTRY_SIZE,
                entry_state_string_get(state),
                element_type_string_get(type),
                ns_idx,
                entry.key,
                entry.length,
                entry.bulk_info_t.bulksize);

                entry_idx++;
                continue;
            }

            dbg_print(NOTICE, "entry[%d] offset:0x%03X, state:%s, type:%s, ns:%d, ",
                entry_idx,
                PAGE_ENTRY_OFFSET + entry_idx * ENTRY_SIZE,
                entry_state_string_get(state),
                element_type_string_get(type),
                ns_idx);
            if (ns_idx == 0) {
                dbg_print(NOTICE, "key:%s, length:%d\r\n", entry.key, entry.length);
            } else {
                dbg_print(NOTICE, "key_str:%s, key_hex:", entry.key);
                size = strlen(entry.key);
                for(i = 0; i < size; i++) {
                    dbg_print(NOTICE, "%x ", (uint8_t)entry.key[i]);
                }
                dbg_print(NOTICE, ", length:%d\r\n", entry.length);
            }

            if (ELEMENT_BULK == type) {
                dbg_print(NOTICE, "\tfrag[%d] value:", tag_fragno_get(entry.tag));
                entry_idx++;
            } else {
                dbg_print(NOTICE, "\tvalue:");
            }

            if (ELEMENT_SMALL == type) {
                for(i = 0; i < entry.length; i++) {
                    dbg_print(NOTICE, "%02x ", entry.value[i]);
                }
                dbg_print(NOTICE, "\r\n");
                entry_idx++;
            } else {
                entry_idx++;
                entry_cnt = (entry.length + ENTRY_SIZE - 1) / ENTRY_SIZE;
                remain = entry.length;
                for(int idx = 0; idx < entry_cnt; idx++) {
                    entry_read(flash_env, page, entry_idx + idx, &entry);
                    size = (remain > ENTRY_SIZE) ? ENTRY_SIZE : remain;
                    for(i = 0; i < size; i++) {
                        dbg_print(NOTICE, "%02x ", entry.data[i]);
                        if ((i + 1) % 16 == 0)
                            dbg_print(NOTICE, "\r\n\t\t");
                    }
                    remain -= size;
                }
                dbg_print(NOTICE, "\r\n");
                entry_idx += entry_cnt;
            }
        }
        page = (struct page_env_tag*)list_next(&page->list_hdr);
    }
}

static int nvds_flash_env_init(struct nvds_flash_env_tag *flash_env, uint32_t start_addr, uint32_t size, const char *label)
{
    int ret;

    /* Init nvds flash environment */
    flash_env->base_addr = start_addr;
    flash_env->length = size;
    strncpy(flash_env->label, label, LABEL_NAME_MAX_SIZE);
#ifdef NVDS_FLASH_ENCRYPTED_SUPPORT
    flash_env->encrypted = 1;
    /* Initial security key */
    ret = nvds_flash_security_init(flash_env);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);
#else
    flash_env->encrypted = 0;
#endif

    /* Load nvds flash data */
    ret = pages_load(flash_env);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

    list_push_back(&nvds_flash_list, &flash_env->list_hdr);

    return NVDS_ERR(NVDS_OK);
}

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */
int nvds_del_keys_by_namespace(void *handle, const char *namespace)
{
    struct nvds_flash_env_tag *flash_env;
    struct page_env_tag *page;
    enum entry_state state;
    union entry_info entry;
    uint8_t entry_idx;
    int entry_cnt;
    enum element_type type;
    struct namespace_info *ns;
    uint8_t ns_idx;
    uint32_t ns_len;
    int ret;

    if (handle)
        flash_env = (struct nvds_flash_env_tag *)handle;
    else
        flash_env = &nvds_flash_env;

    /* find ns idx by namespace */
    ns_len = strlen(namespace);
    if (ns_len > (KEY_NAME_MAX_SIZE - 1))
        return NVDS_ERR(NVDS_E_INVAL_PARAM);

    ns = (struct namespace_info *)list_pick(&flash_env->ns_list);
    while (ns) {
        if(strlen(ns->name) == ns_len && !sys_memcmp(ns->name, namespace, ns_len)){
            ns_idx = ns->index;
            break;
        }

        ns = (struct namespace_info*)list_next(&ns->list_hdr);
    }
    if (!ns)
        return NVDS_ERR(NVDS_E_NOT_FOUND);

    /* parsing used page list */
    page = (struct page_env_tag *)list_pick(&flash_env->nvds_page_used);
    while (page) {
        if ((page->header.state != PAGE_ACTIVE) && (page->header.state != PAGE_FULL)) {
            page = (struct page_env_tag*)list_next(&page->list_hdr);
            continue;
        }

        /* walk entry in the page */
        for (entry_idx = 0; entry_idx < ENTRY_COUNT_PER_PAGE;) {
            /* only find the valid entry */
            ret = entry_state_get(page->entry_states, entry_idx, &state);
            NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

            if (state == ENTRY_FREE)
                break;
            ret = entry_read(flash_env, page, entry_idx, &entry);
            NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

            /* compare namespace*/
            type = tag_element_type_get(entry.tag);
            if ((state == ENTRY_USED) && (ns_idx == tag_namespace_get(entry.tag))) {
                if ((type == ELEMENT_SMALL) || (type == ELEMENT_BULKINFO)) {
                    ret = entry_state_range_alter(flash_env, page, entry_idx, entry_idx, ENTRY_UPDATED);
                    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);
                    page->entry_cnt_used--;
                } else if ((type == ELEMENT_MIDDLE) || (type == ELEMENT_BULK)) {
                    entry_cnt = (entry.length + ENTRY_SIZE - 1) / ENTRY_SIZE;
                    ret = entry_state_range_alter(flash_env, page, entry_idx, entry_idx + entry_cnt, ENTRY_UPDATED);
                    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);
                    page->entry_cnt_used -= (entry_cnt + 1);
                }

                /* when there is no key in namespace, we can delete namespace */
                ns->used_cnt--;
                if (ns->used_cnt == 0) {
                    list_extract(&flash_env->ns_list, &ns->list_hdr);
                    ns_state_set(flash_env->ns_states, ns_idx, false);

                    /* delete namespace */
                    ret = data_element_del(flash_env, 0, ns->name);
                    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

                    sys_mfree(ns);
                }
            }

            entry_idx++;
            if ((type == ELEMENT_MIDDLE) || (type == ELEMENT_BULK))
                entry_idx += ((entry.length + ENTRY_SIZE - 1) / ENTRY_SIZE);
        }

        /* go to next used page */
        page = (struct page_env_tag*)list_next(&page->list_hdr);
    }

    return NVDS_ERR(NVDS_OK);
}

int nvds_find_keys_by_namespace(void *handle, const char *namespace, found_keys_cb cb)
{
    struct nvds_flash_env_tag *flash_env;
    struct page_env_tag *page;
    enum entry_state state;
    union entry_info entry;
    uint8_t entry_idx;
    enum element_type type;
    uint8_t ns_idx;
    int ret;

    if (handle)
        flash_env = (struct nvds_flash_env_tag *)handle;
    else
        flash_env = &nvds_flash_env;

    /* find ns idx by namespace */
    ret = ns_index_by_namespace(flash_env, namespace, false, &ns_idx);
    NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

    /* parsing used page list */
    page = (struct page_env_tag *)list_pick(&flash_env->nvds_page_used);
    while (page) {
        if ((page->header.state != PAGE_ACTIVE) && (page->header.state != PAGE_FULL)) {
            page = (struct page_env_tag*)list_next(&page->list_hdr);
            continue;
        }

        /* walk entry */
        for (entry_idx = 0; entry_idx < ENTRY_COUNT_PER_PAGE;) {
            ret = entry_state_get(page->entry_states, entry_idx, &state);
            NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

            if (state == ENTRY_FREE)
                break;

            ret = entry_read(flash_env, page, entry_idx, &entry);
            NVDS_ERR_RET(ret == NVDS_ERR(NVDS_OK), ret);

            type = tag_element_type_get(entry.tag);
            if ((state == ENTRY_USED) && (tag_namespace_get(entry.tag) == ns_idx)) {
                if (type != ELEMENT_BULK) {
                    if (cb) {
                        cb(namespace, entry.key, entry.length);
                    }
                }
            }

            entry_idx++;
            if ((type == ELEMENT_MIDDLE) || (type == ELEMENT_BULK))
                entry_idx += ((entry.length + ENTRY_SIZE - 1) / ENTRY_SIZE);

        }
        page = (struct page_env_tag*)list_next(&page->list_hdr);
    }

    return NVDS_OK;
}

void nvds_flash_deinit(void *handle)
{
    struct nvds_flash_env_tag *flash_env = (struct nvds_flash_env_tag *)handle;
    struct namespace_info* ns;
    struct page_env_tag *p;

    list_extract(&nvds_flash_list, &flash_env->list_hdr);

    if (flash_env) {
        while (!list_is_empty(&flash_env->ns_list)) {
            ns = (struct namespace_info*)list_pop_front(&flash_env->ns_list);
            if (!ns)
                break;
            sys_mfree(ns);
        }

        while (!list_is_empty(&flash_env->nvds_page_used)) {
            p = (struct page_env_tag*)list_pop_front(&flash_env->nvds_page_used);
            if (!p)
                break;
            sys_mfree(p);
        }

        while (!list_is_empty(&flash_env->nvds_page_free)) {
            p = (struct page_env_tag*)list_pop_front(&flash_env->nvds_page_free);
            if (!p)
                break;
            sys_mfree(p);
        }

        sys_mfree(flash_env);
    }
}

void *nvds_flash_init(uint32_t start_addr, uint32_t size, const char *label)
{
    struct nvds_flash_env_tag *flash_env;

    if(flash_env_check(start_addr, size))
        return NULL;

    if (!nvds_mutex) {
        sys_mutex_init(&nvds_mutex);
        if (!nvds_mutex)
            return NULL;
    }

    flash_env = sys_malloc(sizeof(struct nvds_flash_env_tag));
    if (!flash_env)
        goto exit;
    sys_memset(flash_env, 0, sizeof(struct nvds_flash_env_tag));

    if (NVDS_ERR(NVDS_OK) != nvds_flash_env_init(flash_env, start_addr, size, label))
        goto exit;

    return (void *)flash_env;
exit:
    if (nvds_mutex) {
        sys_mutex_free(&nvds_mutex);
        nvds_mutex = NULL;
    }

    if (flash_env)
        sys_mfree(flash_env);

    return NULL;
}

int nvds_flash_internal_init(void)
{
    int ret = NVDS_ERR(NVDS_E_FAIL);

    if (!nvds_mutex) {
        sys_mutex_init(&nvds_mutex);
        if (!nvds_mutex)
            return NVDS_ERR(NVDS_E_FAIL);
    }

    list_init(&nvds_flash_list);

    ret = flash_env_check(NVDS_FLASH_INTERNAL_ADDR, NVDS_FLASH_INTERNAL_SIZE);
    if (NVDS_ERR(NVDS_OK) != ret)
        goto exit;

    //user data partition config NO-RTDEC if flash use aes encryption
    raw_flash_nodec_config(1, RE_IMG_1_END >> 12, (RE_END_OFFSET >> 12) - 1);

    ret = nvds_flash_env_init(&nvds_flash_env, NVDS_FLASH_INTERNAL_ADDR, NVDS_FLASH_INTERNAL_SIZE, LABEL_INNER_NVDS_FLASH);
    if (NVDS_ERR(NVDS_OK) != ret)
        goto exit;

    return ret;
exit:
    if (nvds_mutex) {
        sys_mutex_free(&nvds_mutex);
        nvds_mutex = NULL;
    }
    return ret;
}

int nvds_data_put(void *handle, const char *namespace, const char *key, uint8_t *data, uint32_t length)
{
    struct nvds_flash_env_tag *flash_env;
    uint8_t ns_idx;
    int ret;

    if (!key || !data || (length == 0) ||
        (length > ELEMENT_BULK_MAX_SIZE) || (strlen(key) > (KEY_NAME_MAX_SIZE - 1)))
        return NVDS_ERR(NVDS_E_INVAL_PARAM);

    if (OS_OK != sys_mutex_get(&nvds_mutex))
        return NVDS_ERR(NVDS_E_FAIL);

    if (handle)
        flash_env = (struct nvds_flash_env_tag *)handle;
    else
        flash_env = &nvds_flash_env;

    /* find ns idx by namespace */
    ret = ns_index_by_namespace(flash_env, namespace, true, &ns_idx);
    if (ret != NVDS_ERR(NVDS_OK))
        goto exit;

    ret = data_element_put(flash_env, ns_idx, key, data, length);
    if (ret == NVDS_ERR(NVDS_OK))
        ns_add_used_cnt(flash_env, ns_idx);

exit:
    sys_mutex_put(&nvds_mutex);
    return ret;
}

int nvds_data_get(void *handle, const char *namespace, const char *key, uint8_t *data, uint32_t *length)
{
    struct nvds_flash_env_tag *flash_env;
    uint8_t ns_idx;
    int ret;

    if (!key || !length || (strlen(key) > (KEY_NAME_MAX_SIZE - 1)))
        return NVDS_ERR(NVDS_E_INVAL_PARAM);

    if (OS_OK != sys_mutex_get(&nvds_mutex))
        return NVDS_ERR(NVDS_E_FAIL);

    if (handle)
        flash_env = (struct nvds_flash_env_tag *)handle;
    else
        flash_env = &nvds_flash_env;

    /* find ns idx by namespace */
    ret = ns_index_by_namespace(flash_env, namespace, false, &ns_idx);
    if (ret != NVDS_ERR(NVDS_OK))
        goto exit;

    ret = data_element_get(flash_env, ns_idx, key, data, length);

exit:
    sys_mutex_put(&nvds_mutex);
    return ret;
}

int nvds_data_del(void *handle, const char *namespace, const char *key)
{
    struct nvds_flash_env_tag *flash_env;
    uint8_t ns_idx;
    int ret;

    if (!key || (strlen(key) > (KEY_NAME_MAX_SIZE - 1)))
        return NVDS_ERR(NVDS_E_INVAL_PARAM);

    if (OS_OK != sys_mutex_get(&nvds_mutex))
        return NVDS_ERR(NVDS_E_FAIL);

    if (handle)
        flash_env = (struct nvds_flash_env_tag *)handle;
    else
        flash_env = &nvds_flash_env;

    /* find ns idx by namespace */
    ret = ns_index_by_namespace(flash_env, namespace, false, &ns_idx);
    if (ret != NVDS_ERR(NVDS_OK))
        goto exit;

    ret = data_element_del(flash_env, ns_idx, key);
    if (ret == NVDS_ERR(NVDS_OK))
        ns_del_used_cnt(flash_env, ns_idx, 1);

exit:
    sys_mutex_put(&nvds_mutex);
    return ret;
}

int nvds_data_find(void *handle, const char *namespace, const char *key)
{
    struct nvds_flash_env_tag *flash_env;
    uint8_t ns_idx;
    int ret;

    if (!key || (strlen(key) > (KEY_NAME_MAX_SIZE - 1)))
        return NVDS_ERR(NVDS_E_INVAL_PARAM);

    if (OS_OK != sys_mutex_get(&nvds_mutex))
        return NVDS_ERR(NVDS_E_FAIL);

    if (handle)
        flash_env = (struct nvds_flash_env_tag *)handle;
    else
        flash_env = &nvds_flash_env;

    /* find ns idx by namespace */
    ret = ns_index_by_namespace(flash_env, namespace, false, &ns_idx);
    if (ret != NVDS_ERR(NVDS_OK))
        goto exit;

    ret = data_element_find(flash_env, ns_idx, key);

exit:
    sys_mutex_put(&nvds_mutex);
    return ret;
}

int nvds_clean(const char *nvds_label)
{
    struct nvds_flash_env_tag *flash_env;
    const char *label = nvds_label;
    uint32_t len;
    int internal = 0;
    int ret;

    if (OS_OK != sys_mutex_get(&nvds_mutex))
        return NVDS_ERR(NVDS_E_FAIL);

    if(!label) {
        internal = 1;
        label = LABEL_INNER_NVDS_FLASH;
    }

    len = strlen(label);

    /* find the one from nvds_flash_list by label */
    flash_env = (struct nvds_flash_env_tag *)list_pick(&nvds_flash_list);
    while (flash_env) {
        if(strlen(flash_env->label) == len && !sys_memcmp(flash_env->label, label, len))
            break;

        flash_env = (struct nvds_flash_env_tag*)list_next(&flash_env->list_hdr);
    }

    if (!flash_env) {
        ret = NVDS_ERR(NVDS_E_NOT_FOUND);
        goto exit;
    }

    ret = nvds_flash_erase(flash_env, 0, flash_env->length);
    if (ret != NVDS_ERR(NVDS_OK))
        goto exit;

    if (internal) {
        memset(&nvds_flash_env, 0, sizeof(nvds_flash_env));
        nvds_flash_env_init(&nvds_flash_env, NVDS_FLASH_INTERNAL_ADDR, NVDS_FLASH_INTERNAL_SIZE, LABEL_INNER_NVDS_FLASH);
    } else {
        nvds_flash_deinit(flash_env);
    }

exit:
    sys_mutex_put(&nvds_mutex);
    return ret;
}

void nvds_dump(void *handle, uint8_t verbose, const char *namespace)
{
    struct nvds_flash_env_tag *flash_env;

    if (OS_OK != sys_mutex_get(&nvds_mutex))
        return;

    if (handle)
        flash_env = (struct nvds_flash_env_tag *)handle;
    else
        flash_env = &nvds_flash_env;

    if (namespace) {
        nvds_dump_namespace(flash_env, namespace);
    } else {
        if (verbose)
            nvds_dump_from_flash(flash_env);
        else
            nvds_dump_from_list(flash_env);
    }

    sys_mutex_put(&nvds_mutex);
}

int nvds_find_all_namespace(void *handle, found_namespace_cb cb)
{
    struct nvds_flash_env_tag *flash_env;
    struct namespace_info *ns;

    if (handle)
        flash_env = (struct nvds_flash_env_tag *)handle;
    else
        flash_env = &nvds_flash_env;


    ns = (struct namespace_info *)list_pick(&flash_env->ns_list);
    while (ns) {
        if (cb)
            cb(ns->name);

        ns = (struct namespace_info*)list_next(&ns->list_hdr);
    }

    return NVDS_OK;
}

#else /* NVDS_FLASH_SUPPORT */
int nvds_flash_internal_init()
{
    return NVDS_OK;
}

void *nvds_flash_init(uint32_t start_addr, uint32_t size, const char *label)
{
    return NULL;
}

void nvds_flash_deinit(void *handle)
{
}

int nvds_data_put(void *handle, const char *namespace, const char *key, uint8_t *data, uint32_t length)
{
    return NVDS_E_NOT_USE_FLASH;
}

int nvds_data_get(void *handle, const char *namespace, const char *key, uint8_t *data, uint32_t *length)
{
    return NVDS_E_NOT_USE_FLASH;
}

int nvds_data_del(void *handle, const char *namespace, const char *key)
{
    return NVDS_E_NOT_USE_FLASH;
}

int nvds_data_find(void *handle, const char *namespace, const char *key)
{
    return NVDS_E_NOT_USE_FLASH;
}

int nvds_clean(const char *nvds_label)
{
    return NVDS_E_NOT_USE_FLASH;
}

void nvds_dump(void *handle, uint8_t verbose, const char *namespace)
{
    return;
}

int nvds_find_all_namespace(void *handle, found_namespace_cb cb)
{
    return NVDS_E_NOT_USE_FLASH;
}

#endif /* NVDS_FLASH_SUPPORT */
