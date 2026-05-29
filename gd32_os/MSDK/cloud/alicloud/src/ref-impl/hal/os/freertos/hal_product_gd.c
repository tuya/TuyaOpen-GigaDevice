/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "app_cfg.h"

#ifdef CONFIG_ALICLOUD_SUPPORT
#include <stdio.h>
#include <string.h>

#include "iot_import.h"

#include "config_gdm32.h"
#include "wrapper_os.h"

#define __DEMO__

#ifdef __DEMO__
    char _product_key[PRODUCT_KEY_LEN + 1];
    char _product_secret[PRODUCT_SECRET_LEN + 1];
    char _device_name[DEVICE_NAME_LEN + 1];
    char _device_secret[DEVICE_SECRET_LEN + 1];
#endif

/**
 * @brief Get device name from user's system persistent storage
 *
 * @param [ou] device_name: array to store device name, max length is IOTX_DEVICE_NAME_LEN
 * @return the actual length of device name
 */
int HAL_GetDeviceName(char device_name[DEVICE_NAME_MAXLEN])
{
    int len = strlen(_device_name);
    sys_memset(device_name, 0x0, DEVICE_NAME_MAXLEN);

#ifdef __DEMO__
    strncpy(device_name, _device_name, len);
#endif
    return strlen(device_name);
}

/**
 * @brief Get device secret from user's system persistent storage
 *
 * @param [ou] device_secret: array to store device secret, max length is IOTX_DEVICE_SECRET_LEN
 * @return the actual length of device secret
 */
int HAL_GetDeviceSecret(char device_secret[DEVICE_SECRET_MAXLEN])
{
    int len = strlen(_device_secret);
    sys_memset(device_secret, 0x0, DEVICE_SECRET_MAXLEN);

#ifdef __DEMO__
    strncpy(device_secret, _device_secret, len);
#endif
    return len;
}

/**
 * @brief Get product key from user's system persistent storage
 *
 * @param [ou] product_key: array to store product key, max length is IOTX_PRODUCT_KEY_LEN
 * @return  the actual length of product key
 */
int HAL_GetProductKey(char product_key[PRODUCT_KEY_MAXLEN])
{
    int len = strlen(_product_key);
    sys_memset(product_key, 0x0, PRODUCT_KEY_MAXLEN);

#ifdef __DEMO__
    strncpy(product_key, _product_key, len);
#endif
    return len;
}

int HAL_GetProductSecret(char product_secret[PRODUCT_SECRET_MAXLEN])
{
    int len = strlen(_product_secret);
    sys_memset(product_secret, 0x0, PRODUCT_SECRET_MAXLEN);

#ifdef __DEMO__
    strncpy(product_secret, _product_secret, len);
#endif
    return len;
}

/**
 * @brief Get firmware version
 *
 * @param [ou] version: array to store firmware version, max length is IOTX_FIRMWARE_VER_LEN
 * @return the actual length of firmware version
 */
int HAL_GetFirmwareVersion(char *version)
{
    uint32_t ver = RE_IMG_VERSION;

    return HAL_Snprintf(version, 64, "%d.%d.%d", (ver >> 24), ((ver << 8) >> 24), (ver & 0xFFFF));
}

int HAL_SetDeviceName(char *device_name)
{
    int len = strlen(device_name);

#ifdef __DEMO__
    if (len > DEVICE_NAME_LEN) {
        return -1;
    }
    sys_memset(_device_name, 0x0, DEVICE_NAME_LEN + 1);
    strncpy(_device_name, device_name, len);
#endif
    return len;
}

int HAL_SetDeviceSecret(char *device_secret)
{
    int len = strlen(device_secret);

#ifdef __DEMO__
    if (len > DEVICE_SECRET_LEN) {
        return -1;
    }
    sys_memset(_device_secret, 0x0, DEVICE_SECRET_LEN + 1);
    strncpy(_device_secret, device_secret, len);
#endif
    return len;
}

int HAL_SetProductKey(char *product_key)
{
    int len = strlen(product_key);

#ifdef __DEMO__
    if (len > PRODUCT_KEY_LEN) {
        return -1;
    }
    sys_memset(_product_key, 0x0, PRODUCT_KEY_LEN + 1);
    strncpy(_product_key, product_key, len);
#endif
    return len;
}

int HAL_SetProductSecret(char *product_secret)
{
    int len = strlen(product_secret);

#ifdef __DEMO__
    if (len > PRODUCT_SECRET_LEN) {
        return -1;
    }
    sys_memset(_product_secret, 0x0, PRODUCT_SECRET_LEN + 1);
    strncpy(_product_secret, product_secret, len);
#endif
    return len;

}

int HAL_GetPartnerID(char *pid_str)
{
    sys_memset(pid_str, 0x0, PID_STRLEN_MAX);
#ifdef __DEMO__
    strcpy(pid_str, "Giga Device");
#endif
    return strlen(pid_str);
}

int HAL_GetModuleID(char *mid_str)
{
    sys_memset(mid_str, 0x0, MID_STRLEN_MAX);
#ifdef __DEMO__
    strcpy(mid_str, "GD32W553");
#endif
    return strlen(mid_str);
}

char *HAL_GetChipID(_OU_ char *cid_str)
{
    sys_memset(cid_str, 0x0, HAL_CID_LEN);
#ifdef __DEMO__
    strncpy(cid_str, "76:ba:ed:20:00:58", HAL_CID_LEN);
    cid_str[HAL_CID_LEN - 1] = '\0';
#endif
    return cid_str;
}

int HAL_GetDeviceID(_OU_ char *device_id)
{
    char pk[PRODUCT_KEY_MAXLEN];
    char dn[DEVICE_NAME_MAXLEN];

    sys_memset(device_id, 0x0, DEVICE_ID_LEN);
    sys_memset(pk, 0x0, PRODUCT_KEY_MAXLEN);
    sys_memset(dn, 0x0, DEVICE_NAME_MAXLEN);

    HAL_GetProductKey(pk);
    HAL_GetDeviceName(dn);

    HAL_Snprintf(device_id, DEVICE_ID_LEN, "%s.%s", pk, dn);

    return strlen(device_id);
}

#endif /* CONFIG_ALICLOUD_SUPPORT */
