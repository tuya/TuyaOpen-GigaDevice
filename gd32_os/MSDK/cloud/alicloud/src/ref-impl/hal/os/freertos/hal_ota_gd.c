/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "app_cfg.h"

#if defined(CONFIG_ALICLOUD_SUPPORT)

#include "iot_import.h"
#include "config_gdm32.h"
#include "rom_export.h"
#include "raw_flash_api.h"
#include "wrapper_os.h"

#define platform_warn(fmt, ...) printf("Livingkit HAL warn:" fmt, ##__VA_ARGS__)
#define platform_info(fmt, ...) printf("Livingkit HAL info:" fmt, ##__VA_ARGS__)
#define platform_err(fmt, ...) printf("Livingkit HAL error:" fmt, ##__VA_ARGS__)

struct hal_ota_info {
    uint8_t running_idx;            // cur running image index
    uint32_t img_start_addr;      // flash start address to be written

    uint32_t max_img_len;
    uint32_t g_firmware_offset;     // flash offset to be written
};

struct hal_ota_info hal_ota_info_inst;
void HAL_Firmware_Persistence_Start(void)
{
    uint8_t running_idx;
    uint32_t res;
    struct hal_ota_info *hal_ota_info_ptr = &hal_ota_info_inst;

    platform_info("OTA start...\r\n");

    memset(hal_ota_info_ptr, 0, sizeof(struct hal_ota_info));

    res = rom_sys_status_get(SYS_RUNNING_IMG, LEN_SYS_RUNNING_IMG, &running_idx);
    if (res < 0) {
        platform_err("OTA get running idx failed! (res = %d)\r\n", res);
        return;
    }
    hal_ota_info_ptr->running_idx = running_idx;
    if (running_idx == IMAGE_0) {
        hal_ota_info_ptr->img_start_addr = RE_IMG_1_OFFSET;
        hal_ota_info_ptr->max_img_len = RE_IMG_1_END - RE_IMG_1_OFFSET;
    } else {
        hal_ota_info_ptr->img_start_addr = RE_IMG_0_OFFSET;
        hal_ota_info_ptr->max_img_len = RE_IMG_1_OFFSET - RE_IMG_0_OFFSET;
    }

    res = raw_flash_erase(hal_ota_info_ptr->img_start_addr, hal_ota_info_ptr->max_img_len);
    if (res < 0) {
        platform_err("OTA flash erase failed (res = %d)\r\n", res);
    }

}

extern int dm_ota_get_ota_handle(void **handle);
int HAL_Firmware_Persistence_Stop(void)
{
    int32_t res;
    struct hal_ota_info *hal_ota_info_ptr = &hal_ota_info_inst;
    void *ota_handle = NULL;
    int err;

    /* check ota result */
    err = dm_ota_get_ota_handle(&ota_handle);
    if (err != SUCCESS_RETURN) {
        platform_err("OTA failed, get ota_handle failed! (err = %d)\r\n", err);
        return FAIL_RETURN;
    }
    err = IOT_OTA_GetLastError(ota_handle);
    if (err) {
        platform_err("OTA failed! (err = %d)\r\n", err);
        return FAIL_RETURN;
    }

    /* Set image status */
    res = rom_sys_set_img_flag(hal_ota_info_ptr->running_idx, (IMG_FLAG_IA_MASK | IMG_FLAG_NEWER_MASK), (IMG_FLAG_IA_OK | IMG_FLAG_OLDER));
    res |= rom_sys_set_img_flag(!(hal_ota_info_ptr->running_idx), (IMG_FLAG_IA_MASK | IMG_FLAG_VERIFY_MASK | IMG_FLAG_NEWER_MASK), 0);
    res |= rom_sys_set_img_flag(!(hal_ota_info_ptr->running_idx), IMG_FLAG_NEWER_MASK, IMG_FLAG_NEWER);

    if (res != 0) {
        platform_err("OTA set image status failed! (res = %d)\r\n", res);
        goto exit;
    }

    hal_ota_info_ptr->g_firmware_offset = 0;
    platform_info("OTA finish... Please reboot now.\r\n");

exit:
    return res;
}

int HAL_Firmware_Persistence_Write(char *buffer, uint32_t length)
{
    int32_t res;
    struct hal_ota_info *hal_ota_info_ptr = &hal_ota_info_inst;

    if (hal_ota_info_ptr->img_start_addr == 0) {
        platform_err("OTA is not started yet!\r\n");
       return -1;
    }

    if ((hal_ota_info_ptr->g_firmware_offset + length) > hal_ota_info_ptr->max_img_len) {
        platform_err("OTA firmware size overflow!\r\n");
        res = -2;
        goto exit;
    }

    res = raw_flash_write((hal_ota_info_ptr->img_start_addr + hal_ota_info_ptr->g_firmware_offset), (uint8_t *)buffer, length);
    if (res < 0) {
        platform_err("OTA flash write failed!\r\n");
        res = -3;
        goto exit;
    }

    hal_ota_info_ptr->g_firmware_offset += length;

    //platform_err("OTA flash write at g_firmware_offset=0x%08x!\r\n", hal_ota_info_ptr->g_firmware_offset);
    return 0;

exit:
    return res;
}

#endif /* CONFIG_ALICLOUD_SUPPORT */
