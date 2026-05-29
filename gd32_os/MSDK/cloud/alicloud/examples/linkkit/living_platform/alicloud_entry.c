/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */
#include "app_cfg.h"
#ifdef CONFIG_ALICLOUD_SUPPORT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "iot_export.h"
#include "alicloud_entry.h"
#include "living_platform_main.h"
#include "living_platform_ut.h"
#include "wrapper_os.h"

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

int linkkit_main(int argc, char **argv)
{
    living_platform_main_params_t paras;
    paras.argc = argc;
    paras.argv = argv;

    aiot_kv_init();

#ifdef LOG_LEVEL_DEBUG
    IOT_SetLogLevel(IOT_LOG_DEBUG);
#else
    IOT_SetLogLevel(IOT_LOG_ERROR);
#endif
    load_living_platform_meta_info();

    living_platform_main((void *)&paras);

    aiot_kv_deinit();

    return 0;
}

static void living_task(void *argv)
{
    living_platform_main_params_t *paras = (living_platform_main_params_t *)argv;
    linkkit_main(paras->argc, paras->argv);
    printf("living task exit.\r\n");
    sys_task_delete(NULL);
}

void cmd_alicloud_linkkit(int argc, char **argv)
{
    void *handle;
    living_platform_main_params_t paras;

    if (argc != 2) {
        printf("Usage: ali_cloud <mode>\r\n");
        printf("<mode>: 1 - smart config, 2 - softap config, 0 - stop alicloud\r\n");
        return;
    }
    if (atoi(argv[1]) == 0) { // Terminate alicloud tasks
        g_linkkit_terminate = 1;
        return;
    }

    paras.argc = argc;
    paras.argv = argv;

    handle = sys_task_create_dynamic((const uint8_t *)"alicloud_task",
                    LIVING_STACK_SIZE, OS_TASK_PRIORITY(LIVING_TASK_PRIO),
                    (task_func_t)living_task, (void *)&paras);
    if (handle == NULL) {
        printf("ERROR: create alicloud task failed.\r\n");
        return;
    }

    sys_ms_sleep(1000);
    awss_config_press();
    return;
}

#endif /* CONFIG_ALICLOUD_SUPPORT */
