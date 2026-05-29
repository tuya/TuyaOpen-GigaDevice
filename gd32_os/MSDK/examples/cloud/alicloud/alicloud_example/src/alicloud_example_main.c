/*!
    \file    alicloud_example_main.c
    \brief   the example of running as an alicloud device

    \version 2024-03-22, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2021, GigaDevice Semiconductor Inc.

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

#include <stdint.h>
#include <stdio.h>
#include "app_cfg.h"
#include "gd32vw55x_platform.h"
#include "wifi_management.h"
#include "wifi_init.h"

#include "dbg_print.h"
#include "wrapper_os.h"


#include "iot_export.h"
#include "alicloud_entry.h"
#include "living_platform_main.h"
#include "living_platform_ut.h"
#include "config_gdm32.h"


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*==== Please check the following setting before build project =======*/
/*========================== USER CONFIGURATION ======================*/
#define ALICLOUD_CONFIG_NETWORK_METHOD      USING_DEVAP
#define TEST_CONTENT                        TEST_ALICLOUD

/*========================= USER CONFIGURATION END ====================*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

static void load_living_platform_meta_info(void)
{
    int len = 0;
    char key_buf[MAX_KEY_LEN];
    char product_key[PRODUCT_KEY_LEN + 1] = {0};
    char product_secret[PRODUCT_SECRET_LEN + 1] = {0};
    char device_name[DEVICE_NAME_LEN + 1] = {0};
    char device_secret[DEVICE_SECRET_LEN + 1] = {0};

    len = PRODUCT_KEY_LEN + 1;
    sys_memset(key_buf, 0, MAX_KEY_LEN);
    sys_memset(product_key, 0, sizeof(product_key));
    HAL_Snprintf(key_buf, MAX_KEY_LEN, "%s_%d", KV_KEY_PK, 0);
    HAL_Kv_Get(key_buf, product_key, &len);

    len = PRODUCT_SECRET_LEN + 1;
    sys_memset(key_buf, 0, MAX_KEY_LEN);
    sys_memset(product_secret, 0, sizeof(product_secret));
    HAL_Snprintf(key_buf, MAX_KEY_LEN, "%s_%d", KV_KEY_PS, 0);
    HAL_Kv_Get(key_buf, product_secret, &len);

    len = DEVICE_NAME_LEN + 1;
    sys_memset(key_buf, 0, MAX_KEY_LEN);
    sys_memset(device_name, 0, sizeof(device_name));
    HAL_Snprintf(key_buf, MAX_KEY_LEN, "%s_%d", KV_KEY_DN, 0);
    HAL_Kv_Get(key_buf, device_name, &len);

    len = DEVICE_SECRET_LEN + 1;
    sys_memset(key_buf, 0, MAX_KEY_LEN);
    sys_memset(device_secret, 0, sizeof(device_secret));
    HAL_Snprintf(key_buf, MAX_KEY_LEN, "%s_%d", KV_KEY_DS, 0);
    HAL_Kv_Get(key_buf, device_secret, &len);

    if ((strlen(product_key) > 0) && (strlen(product_secret) > 0) && (strlen(device_name) > 0))
    {
        HAL_SetProductKey(product_key);
        HAL_SetProductSecret(product_secret);
        HAL_SetDeviceName(device_name);
        HAL_SetDeviceSecret(device_secret);
        printf("pk[%s]\r\n", product_key);
        printf("dn[%s]\r\n", device_name);
    }
    else
    {
        HAL_SetProductKey(PRODUCT_KEY);
        HAL_SetProductSecret(PRODUCT_SECRET);
        HAL_SetDeviceName(DEVICE_NAME);
        HAL_SetDeviceSecret(DEVICE_SECRET);
        printf("pk[%s]\r\n", PRODUCT_KEY);
        printf("dn[%s]\r\n", DEVICE_NAME);
    }
}

int linkkit_main(int method)
{
    aiot_kv_init();

#ifdef LOG_LEVEL_DEBUG
    IOT_SetLogLevel(IOT_LOG_DEBUG);
#else
    IOT_SetLogLevel(IOT_LOG_ERROR);
#endif

    load_living_platform_meta_info();

    living_platform_main((void *)method);

    aiot_kv_deinit();

    return 0;
}

static void living_task(void *param)
{
    int method = ALICLOUD_CONFIG_NETWORK_METHOD;

    wifi_netlink_auto_conn_set(1);

    awss_config_press();
    sys_ms_sleep(1000);
    linkkit_main(method);

    sys_task_delete(NULL);
}

int main(void)
{
    platform_init();

    app_print("SDK Version: %u.%u.%u\n", RE_IMG_VERSION >> 24,
                (RE_IMG_VERSION & 0xFF0000) >> 16, RE_IMG_VERSION & 0xFFFF);

    if (wifi_init()) {
        printf("wifi init failed.\r\n");
    }

    sys_task_create_dynamic((const uint8_t *)"alicloud_task", LIVING_STACK_SIZE,
                            OS_TASK_PRIORITY(LIVING_TASK_PRIO), (task_func_t)living_task, NULL);

    sys_os_start();

    for ( ; ; );
}
