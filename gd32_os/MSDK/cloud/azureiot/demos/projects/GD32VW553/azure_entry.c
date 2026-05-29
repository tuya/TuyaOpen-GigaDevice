/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure_entry.h"

#ifdef CONFIG_AZURE_IOT_SUPPORT
#include "demo_config.h"
#include "sntp.h"
#include "trng.h"
#include "wifi_management.h"
#include "wifi_netif.h"
#include "wifi_init.h"
#include "FreeRTOS.h"
#include "task.h"
#include <time.h>
#include "tcpip.h"

#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
#include "atcmd_azure.h"
#include "sample_azure_iot_f527_api.h"
#endif

#include "wifi_vif.h"
#if NVDS_FLASH_SUPPORT
#include "nvds_flash.h"
#endif


static const char *pTimeServers[] = {"pool.ntp.org", "time.nist.gov", "time.ustc.edu.cn"};
const size_t numTimeServers = sizeof(pTimeServers) / sizeof(char *);

volatile uint32_t unix_time_base_g = 0;

uint64_t ullGetUnixTime(void);

/* Log function */
void vLoggingPrintf(const char * pcFormat,
                     ...)
{
    va_list vargs;

    va_start(vargs, pcFormat);
    vprintf(pcFormat, vargs);
    va_end(vargs);
}

/*
 * Prototypes for the demos that can be started from this project.
 */
extern void vStartDemoTask(void);


void prvStopSNTP(void)
{
    LOCK_TCPIP_CORE();

    sntp_stop();

    UNLOCK_TCPIP_CORE();
    unix_time_base_g = 0;
}

/* SNTP initialize */
int prvInitializeSNTP(void)
{
    uint32_t unixTime = 0;
    uint8_t retry_count = 0;

    configPRINTF(("Initializing SNTP.\r\n"));

    LOCK_TCPIP_CORE();
    sntp_setoperatingmode(SNTP_OPMODE_POLL);

    for (uint8_t i = 0; i < numTimeServers; i++) {
        sntp_setservername(i, pTimeServers[i]);
    }
    sntp_init();
    UNLOCK_TCPIP_CORE();

    do {
        unixTime = ( uint32_t )ullGetUnixTime();

        if (unixTime < democonfigSNTP_INIT_WAIT) {
            retry_count++;
            // configPRINTF(("SNTP not queried yet. Retrying.\r\n"));
            vTaskDelay(democonfigSNTP_INIT_RETRY_DELAY / portTICK_PERIOD_MS);
        }
    } while ((unixTime < democonfigSNTP_INIT_WAIT) && (retry_count < democonfigSNTP_INIT_RETRY_COUNT));

    if (retry_count >= democonfigSNTP_INIT_RETRY_COUNT) {
        configPRINTF(("SNTP failed. Please check the WiFi is connected to Internet.\r\n"));
        prvStopSNTP();
        return 1;
    }

#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
    atcmd_wifi_conn_rsp(WIFI_CONN_OK);
#endif
    configPRINTF(("> SNTP Initialized: %lu\r\n", unixTime));
    return 0;
}

/* configure SNTP, and start azure demo */
void cmd_azure_iot(int argc, char **argv)
{

#ifndef CONFIG_AZURE_F527_DEMO_SUPPORT
    prvInitializeSNTP();
#endif
    /* Start azure connect to iot hub */
    vStartDemoTask();
}

#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
void cmd_azure_cli(int argc, char **argv)
{
    int action;
    int res;

    if (argc != 2) {
        printf("Usage: azure_cli <action>\r\n");
        printf("<mode>: 0 exit, 1 - connect, 2 - disconnect\r\n");
        return;
    }

    action = atoi(argv[1]);
    if (action == 0) { // Terminate azure tasks
        res = azure_iot_hub_local_message_send(AZURE_IOT_AT_EXIT, NULL, 0);
        if (res)
            printf("send AT_EXIT message fail, res=%d\r\n", res);
    } else if (action == 1) {
        res = azure_iot_hub_local_message_send(AZURE_IOT_AT_CONNECT, NULL, 0);
        if (res)
            printf("send AT_CONNECT message fail, res=%d\r\n", res);
    } else if (action == 2) {
        res = azure_iot_hub_local_message_send(AZURE_IOT_AT_DISCONNECT, NULL, 0);
        if (res)
            printf("send AT_DISCONNECT message fail, res=%d\r\n", res);
    }
}
#endif
/* get the current system up time (second) */
uint32_t os_system_time_get(void);

uint64_t ullGetUnixTime(void)
{
    uint32_t timenow;

    timenow = unix_time_base_g + os_system_time_get();

    return (uint64_t)(timenow);
}

/* get the wifi connected state */
bool xAzureSample_IsConnectedToInternet(void)
{
    return wifi_vif_is_sta_connected(0);
}

#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
int azure_package_version_set(uint16_t package_ver)
{
    return nvds_data_put(NULL, NVDS_NS_WIFI_INFO, AZURE_PACKAGE_VER,
                        (uint8_t *)(&package_ver), sizeof(uint16_t));
}

uint16_t azure_package_version_get(void)
{
    uint16_t package_ver;
    uint16_t data_len = sizeof(uint16_t);

    int ret = nvds_data_get(NULL, NVDS_NS_WIFI_INFO, AZURE_PACKAGE_VER,
                            (uint8_t *)(&package_ver), (uint32_t *)(&data_len));
    if (ret != 0) {
        package_ver = 0x1000; //default ver
    }

    return package_ver;
}
#endif /* CONFIG_AZURE_F527_DEMO_SUPPORT */
/*-----------------------------------------------------------*/

int mbedtls_platform_entropy_poll(void * data,
                                   unsigned char * output,
                                   size_t len,
                                   size_t *olen)
{
    return gd_hardware_poll(data, output, len, olen);
}

void vApplicationDaemonTaskStartupHook(void)
{
    wifi_wait_ready();
}

#if configSUPPORT_STATIC_ALLOCATION
/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
 * implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
 * used by the Idle task. */
void vApplicationGetIdleTaskMemory(StaticTask_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    uint32_t * pulIdleTaskStackSize)
{
/* If the buffers to be provided to the Idle task are declared inside this
 * function then they must be declared static - otherwise they will be allocated on
 * the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle
     * task's state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
 * implementation of vApplicationGetTimerTaskMemory() to provide the memory that is
 * used by the RTOS daemon/time task. */
void vApplicationGetTimerTaskMemory(StaticTask_t ** ppxTimerTaskTCBBuffer,
                                     StackType_t ** ppxTimerTaskStackBuffer,
                                     uint32_t * pulTimerTaskStackSize)
{
/* If the buffers to be provided to the Timer task are declared inside this
 * function then they must be declared static - otherwise they will be allocated on
 * the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle
     * task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

#endif

#endif /* CONFIG_AZURE_IOT_SUPPORT */
