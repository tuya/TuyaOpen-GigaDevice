/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#include "app_cfg.h"

#ifdef CONFIG_ALICLOUD_SUPPORT
#include <stdio.h>
#include <stdint.h>
#include "iot_import.h"
#include "config_gdm32.h"
#include "rom_export.h"

#if NVDS_FLASH_SUPPORT
#include "nvds_type.h"
#include "nvds_flash.h"
#endif
#include "aiot_kv_api.h"
#include "raw_flash_api.h"
#include "aiot_al_kv.h"


/* Redefine the start address and size according to user's flash layout */
#define ALICLOUD_FLASH_BLOCK_START      (RE_IMG_1_END) // 0x3CB000
#define ALICLOUD_FLASH_BLOCK_SIZE       0x2000 // 8K

/**
 * Erase an area on a Flash logical partition
 *
 * @note  Erase on an address will erase all data on a sector that the
 *        address is belonged to, this function does not save data that
 *        beyond the address area but in the affected sector, the data
 *        will be lost.
 *
 * @param[in]  in_partition  The target flash logical partition which should be erased
 * @param[in]  off_set       Start address of the erased flash area
 * @param[in]  size          Size of the erased flash area
 *
 * @return  0 : On success, EIO : If an error occurred with any step
 */
int32_t aiot_al_kv_flash_erase(uint32_t off_set, uint32_t size)
{
    uint32_t flash_off = 0;

    flash_off = off_set + ALICLOUD_FLASH_BLOCK_START;
    if (flash_off <= RE_END_OFFSET) {
        if (!raw_flash_erase(flash_off, size))
            return 0;
    }

    return -1;
}

/**
 * Write data to an area on a flash logical partition without erase
 *
 * @param[in]  in_partition    The target flash logical partition which should be read which should be written
 * @param[in]  off_set         Point to the start address that the data is written to, and
 *                             point to the last unwritten address after this function is
 *                             returned, so you can call this function serval times without
 *                             update this start address.
 * @param[in]  inBuffer        point to the data buffer that will be written to flash
 * @param[in]  inBufferLength  The length of the buffer
 *
 * @return  0 : On success, EIO : If an error occurred with any step
 */
int32_t aiot_al_kv_flash_write(uint32_t *off_set, const void *in_buf, uint32_t in_buf_len)
{
    uint32_t flash_off = 0;

    if (off_set == NULL)
        return -1;

    flash_off = *off_set + ALICLOUD_FLASH_BLOCK_START;
    if (flash_off <= RE_END_OFFSET) {
        if (!raw_flash_write(flash_off, in_buf, in_buf_len)) {
            *off_set = *off_set + in_buf_len;
            return 0;
        }
    }

    return -1;
}

/**
 * Read data from an area on a Flash to data buffer in RAM
 *
 * @param[in]  in_partition    The target flash logical partition which should be read
 * @param[in]  off_set         Point to the start address that the data is read, and
 *                             point to the last unread address after this function is
 *                             returned, so you can call this function serval times without
 *                             update this start address.
 * @param[in]  outBuffer       Point to the data buffer that stores the data read from flash
 * @param[in]  inBufferLength  The length of the buffer
 *
 * @return  0 : On success, EIO : If an error occurred with any step
 */
int32_t aiot_al_kv_flash_read(uint32_t *off_set, void *out_buf, uint32_t out_buf_len)
{
    uint32_t flash_off = 0;

    if (off_set == NULL)
        return -1;

    flash_off = *off_set + ALICLOUD_FLASH_BLOCK_START;
    if (flash_off <= RE_END_OFFSET) {
        if (!raw_flash_read(flash_off, out_buf, out_buf_len)) {
            *off_set = *off_set + out_buf_len;
            return 0;
        }
    }

    return -1;
}

int aiot_al_is_lk_kv(void)
{
#if NVDS_FLASH_SUPPORT
    return 1;
#else
    return 0;
#endif
}

int aiot_al_lk_kv_get_value_len(void)
{
    printf("->%s TODO\r\n", __func__);
#if NVDS_FLASH_SUPPORT
    return ELEMENT_BULK_MAX_SIZE;
#else
    return -1;
#endif
}

int aiot_al_lk_kv_set(const char *key, const void *val, int len)
{
#if NVDS_FLASH_SUPPORT
    return nvds_data_put(NULL, NVDS_NS_ALICLOUD_INFO, key, (uint8_t *)val, (uint32_t)len);
#else
    return -1;
#endif
}

int aiot_al_lk_kv_get(const char *key, void *val, int *buffer_len)
{
#if NVDS_FLASH_SUPPORT
    return nvds_data_get(NULL, NVDS_NS_ALICLOUD_INFO, key, (uint8_t *)val, (uint32_t *)buffer_len);
#else
    return -1;
#endif
}

int aiot_al_lk_kv_del(const char *key)
{
#if NVDS_FLASH_SUPPORT
    return nvds_data_del(NULL, NVDS_NS_ALICLOUD_INFO, key);
#else
    return -1;
#endif
}

#endif /* CONFIG_ALICLOUD_SUPPORT */
