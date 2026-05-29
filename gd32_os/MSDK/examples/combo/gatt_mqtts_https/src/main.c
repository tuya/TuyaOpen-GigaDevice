/*!
    \file    main.c
    \brief   Main function of (ble gatt + wifi mqtts + wifi https) example.

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
#include <stdint.h>
#include <stdio.h>
#include "app_cfg.h"
#include "gd32vw55x_platform.h"
#include "wifi_management.h"
#include "wifi_init.h"
#include "dbg_print.h"
#include "wrapper_os.h"

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/*==== Please check the following setting before build project =======*/
/*========================== USER CONFIGURATION ======================*/
/* Please choose BLE worked as Central or Periheral */
#define BLE_ROLE            BLE_CENTRAL
#define BLE_CENTRAL         1
#define BLE_PERIPHERAL      2

/* Please set the correct AP information */
#define SSID            "Testing-WIFI"
#define PASSWORD        "Testwifi@2020" // NULL // ""

/* Please set the correct MQTT server information */
ip_addr_t MQTT_SERVER_IP = IPADDR4_INIT_BYTES(192, 168, 1, 12);
#define MQTT_SERVER_PORT     8883

/* Please set the correct HTTPS server information */
#define HTTPS_SERVER_NAME     "www.baidu.com"
#define HTTPS_SERVER_PORT     "443"

/* Please set round number */
#define TEST_ROUND            10000
/*========================= USER CONFIGURATION END ====================*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

#if (BLE_ROLE == BLE_CENTRAL)
char mqtt_client_id[] = {'G', 'i', 'g', 'a', 'D', 'e', 'v', 'i', 'c', 'e', '1', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#else
char mqtt_client_id[] = {'G', 'i', 'g', 'a', 'D', 'e', 'v', 'i', 'c', 'e', '2', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif

/* Priority of the ble datatrans task */
#define TRANS_TASK_PRIORITY         OS_TASK_PRIORITY(-1)
#define MQTTS_HTTPS_TASK_PRIORITY   OS_TASK_PRIORITY(0)
#define MQTTS_PUB_TASK_PRIORITY     OS_TASK_PRIORITY(-1)
#define HTTPS_GET_TASK_PRIORITY     OS_TASK_PRIORITY(-1)

#define BLE_MTU                 512
static uint8_t ble_trans_data[BLE_MTU];

extern uint8_t central_connected;
extern uint8_t peripheral_connected;

/*
Getting the server CA certs can refer to chapter 3.8.1 in
document AN185 GD32VW553 Network Application Development Guide.docx.
*/
static const char baidu_ca_crt[]=
"-----BEGIN CERTIFICATE-----\r\n" \
"MIIETjCCAzagAwIBAgINAe5fFp3/lzUrZGXWajANBgkqhkiG9w0BAQsFADBXMQsw\r\n" \
"CQYDVQQGEwJCRTEZMBcGA1UEChMQR2xvYmFsU2lnbiBudi1zYTEQMA4GA1UECxMH\r\n" \
"Um9vdCBDQTEbMBkGA1UEAxMSR2xvYmFsU2lnbiBSb290IENBMB4XDTE4MDkxOTAw\r\n" \
"MDAwMFoXDTI4MDEyODEyMDAwMFowTDEgMB4GA1UECxMXR2xvYmFsU2lnbiBSb290\r\n" \
"IENBIC0gUjMxEzARBgNVBAoTCkdsb2JhbFNpZ24xEzARBgNVBAMTCkdsb2JhbFNp\r\n" \
"Z24wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDMJXaQeQZ4Ihb1wIO2\r\n" \
"hMoonv0FdhHFrYhy/EYCQ8eyip0EXyTLLkvhYIJG4VKrDIFHcGzdZNHr9SyjD4I9\r\n" \
"DCuul9e2FIYQebs7E4B3jAjhSdJqYi8fXvqWaN+JJ5U4nwbXPsnLJlkNc96wyOkm\r\n" \
"DoMVxu9bi9IEYMpJpij2aTv2y8gokeWdimFXN6x0FNx04Druci8unPvQu7/1PQDh\r\n" \
"BjPogiuuU6Y6FnOM3UEOIDrAtKeh6bJPkC4yYOlXy7kEkmho5TgmYHWyn3f/kRTv\r\n" \
"riBJ/K1AFUjRAjFhGV64l++td7dkmnq/X8ET75ti+w1s4FRpFqkD2m7pg5NxdsZp\r\n" \
"hYIXAgMBAAGjggEiMIIBHjAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB\r\n" \
"/zAdBgNVHQ4EFgQUj/BLf6guRSSuTVD6Y5qL3uLdG7wwHwYDVR0jBBgwFoAUYHtm\r\n" \
"GkUNl8qJUC99BM00qP/8/UswPQYIKwYBBQUHAQEEMTAvMC0GCCsGAQUFBzABhiFo\r\n" \
"dHRwOi8vb2NzcC5nbG9iYWxzaWduLmNvbS9yb290cjEwMwYDVR0fBCwwKjAooCag\r\n" \
"JIYiaHR0cDovL2NybC5nbG9iYWxzaWduLmNvbS9yb290LmNybDBHBgNVHSAEQDA+\r\n" \
"MDwGBFUdIAAwNDAyBggrBgEFBQcCARYmaHR0cHM6Ly93d3cuZ2xvYmFsc2lnbi5j\r\n" \
"b20vcmVwb3NpdG9yeS8wDQYJKoZIhvcNAQELBQADggEBACNw6c/ivvVZrpRCb8RD\r\n" \
"M6rNPzq5ZBfyYgZLSPFAiAYXof6r0V88xjPy847dHx0+zBpgmYILrMf8fpqHKqV9\r\n" \
"D6ZX7qw7aoXW3r1AY/itpsiIsBL89kHfDwmXHjjqU5++BfQ+6tOfUBJ2vgmLwgtI\r\n" \
"fR4uUfaNU9OrH0Abio7tfftPeVZwXwzTjhuzp3ANNyuXlava4BJrHEDOxcd+7cJi\r\n" \
"WOx37XMiwor1hkOIreoTbv3Y/kIvuX1erRjvlJDKPSerJpSZdcfL03v3ykzTr1Eh\r\n" \
"kluEfSufFT90y1HonoMOFm8b50bOI7355KKL0jlrqnkckSziYSQtjipIcJDEHsXo\r\n" \
"4HA=\r\n" \
"-----END CERTIFICATE-----";

/*==========================================================================================*/
extern int ble_central_tx(uint8_t *p_buf, uint16_t len);
extern int ble_peripheral_tx(uint8_t *p_buf, uint16_t len);
extern int https_client_start(char *host, char *port, const uint8_t *cert, uint32_t cert_len);
extern int https_client_get(char *host);
extern void https_client_stop(void);
extern int mqtt_client_start(ip_addr_t *server_ip, uint16_t port);
extern int mqtt_client_publish(void);
extern int mqtt_client_subscribe(void);
extern void mqtt_client_stop(void);

static void print_status(void)
{
    int total, used, free, max_used;

    sys_heap_info(&total, &free, &max_used);
    used = total - free;
    max_used = total - max_used;

    app_print("\r\n=================================================\r\n");
    app_print("RTOS HEAP: free=%d used=%d max_used=%d/%d\n\n",
                free, used, max_used, total);

    app_print("TaskName\t\tState\tPri\tStack\tID\tStackBase\r\n");
    app_print("--------------------------------------------------\r\n");
    sys_task_list(NULL);
}

static void random_delay(void)
{
    uint32_t rand;

    sys_random_bytes_get(&rand, sizeof(rand));

    rand = rand % 9000 + 1000;

    sys_msleep(rand);
}

static void ble_data_transfer_task(void *param)
{
    uint32_t i, value = 0;

    for (i = 0; i < TEST_ROUND; i++) {
        random_delay();

#if (BLE_ROLE == BLE_CENTRAL)
        if (!central_connected) {
            continue;
        }
#else
        if (!peripheral_connected) {
            continue;
        }
#endif

        value = i % 10;
        sys_memset(ble_trans_data, value, BLE_MTU);

#if (BLE_ROLE == BLE_CENTRAL)
        if (ble_central_tx(ble_trans_data, BLE_MTU)) {
            app_print("[BLE] TX[%08u]: failed\r\n", i);
        } else {
            app_print("[BLE] TX[%08u]: %u\r\n", i, BLE_MTU);
        }
#else
        if (ble_peripheral_tx(ble_trans_data, BLE_MTU)) {
            app_print("[BLE] TX[%08u]: failed\r\n", i);
        } else {
            app_print("[BLE] TX[%08u]: %u\r\n", i, BLE_MTU);
        }
#endif
    }
    sys_task_delete(NULL);
}

static void mqtts_pub_task(void *param)
{
    int i, ret;
    /*
    * MQTTS client periodically publish
    */
    for (i = 0; i < TEST_ROUND; i++) {
        random_delay();

        app_print("[WIFI MQTTS]: ");
        ret = mqtt_client_publish();
        if (ret != 0) {
            app_print("MQTT publish failed (ret = %d).\r\n", ret);
        }
    }

    mqtt_client_stop();

    sys_task_delete(NULL);
}

static void https_get_task(void *param)
{
    int i, ret;
    /*
    * HTTPS client periodically get
    */
    for (i = 0; i < TEST_ROUND; i++) {
        random_delay();

        ret = https_client_get(HTTPS_SERVER_NAME);
        if (ret != 0) {
            app_print("Https get failed (ret = %d).\r\n", ret);
        }
        if (i % 10 == 0)
            print_status();
    }

    https_client_stop();

    sys_task_delete(NULL);
}

static void mqtts_https_task(void *param)
{
    int ret, i;

    wifi_wait_ready();

    print_status();

    /*
    * 1. Start Wi-Fi connection
    */
Retry_wifi_connect:
    app_print("Wi-Fi connect with %s (%s)...\r\n", SSID, PASSWORD);
    ret = wifi_management_connect(SSID, PASSWORD, 1);
    if (ret != 0) {
        app_print("Wi-Fi connect failed (ret %d).\r\n", ret);
        sys_msleep(2000);
        goto Retry_wifi_connect;
    }
    print_status();

    /*
    * 2. Start MQTTS client
    */
Retry_mqtts_connect:
    app_print("Start MQTTS client.\r\n");
    ret = mqtt_client_start(&MQTT_SERVER_IP, MQTT_SERVER_PORT);
    if (ret != 0) {
        app_print("Mqtts connect failed (ret %d).\r\n", ret);
        sys_msleep(3000);
        goto Retry_mqtts_connect;
    }
    print_status();

    /*
    * 3. MQTTS client subscribe
    */
    app_print("MQTTS client subscribe.\r\n");
    mqtt_client_subscribe();
    print_status();

    /*
    * 4. Start HTTPS client
    */
Retry_https_connect:
    app_print("Start HTTPS client.\r\n");
    ret = https_client_start(HTTPS_SERVER_NAME, HTTPS_SERVER_PORT,
                (const uint8_t *)baidu_ca_crt, strlen(baidu_ca_crt) + 1);
    if (ret != 0) {
        app_print("Https connect failed (ret %d).\r\n", ret);
        sys_msleep(3000);
        goto Retry_https_connect;
    }
    print_status();

    /*
    * 5. Create MQTTS client periodically publish task
    */
    app_print("Create mqtts pub task.\r\n");
    sys_task_create_dynamic((const uint8_t *)"mqtts pub", 1088, MQTTS_PUB_TASK_PRIORITY,
            mqtts_pub_task, NULL);

    /*
    * 6. Create HTTPS client periodically get task
    */
    app_print("Create https get task.\r\n");
    sys_task_create_dynamic((const uint8_t *)"https get", 1088, HTTPS_GET_TASK_PRIORITY,
            https_get_task, NULL);
    print_status();

    sys_task_delete(NULL);
}

int main(void)
{
    sys_os_init();
    platform_init();

#if (BLE_ROLE == BLE_CENTRAL)
    ble_central_init();
#else
    ble_peripheral_init();
#endif

    if (wifi_init()) {
        app_print("wifi init failed.\r\n");
        return -1;
    }

    if (sys_task_create_dynamic((const uint8_t *)"ble trans", 512, TRANS_TASK_PRIORITY,
            ble_data_transfer_task, NULL) == NULL) {
        return -2;
    }

    if (sys_task_create_dynamic((const uint8_t *)"mqtts https", 4096, MQTTS_HTTPS_TASK_PRIORITY,
            mqtts_https_task, NULL) == NULL) {
        return -3;
    }

    sys_os_start();

    for ( ; ; );
}

