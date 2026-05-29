/*!
    \file    coap_server_main.c
    \brief   the example of coap server in station mode

    \version 2024-06-06, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2024, GigaDevice Semiconductor Inc.

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
#include "server-coap.h"
#include "gd32vw55x_platform.h"
#include "wifi_management.h"
#include "wifi_init.h"

#define SSID            "tplink_5010"
#define PASSWORD         NULL //"12345678"

uint8_t coap_server_terminate;

static void coap_server_task_func(void *param)
{
    int running = 1;
    int status = 0;
    char *ssid = SSID;
    char *password = PASSWORD;

    if (ssid == NULL) {
        printf("ssid can not be NULL!\r\n");
        goto exit;
    }

    /*
    * 1. Start Wi-Fi connection
    */
    printf("Start Wi-Fi connection.\r\n");
    if (wifi_management_connect(ssid, password, 1) != 0) {
        printf("Wi-Fi connection failed\r\n");
        goto exit;
    }

    /*
    * 2. Start COAP server
    */
    printf("Server Application started.\n");

    server_coap_init();

    while (running && !coap_server_terminate) {
        running = server_coap_poll();
    }

    server_coap_finished();

    /*
    * 3. Stop Wi-Fi connection
    */
    printf("Stop Wi-Fi connection.\r\n");
    wifi_management_disconnect();
exit:
    printf("Server Application finished.\r\n");
    sys_task_delete(NULL);
}

static void coap_server_task_start(void)
{
    coap_server_terminate = 0;

    if (sys_task_create_dynamic((const uint8_t *)"coap_server",
                                368 , OS_TASK_PRIORITY(1),
                                coap_server_task_func, NULL) == NULL) {
        app_print("ERROR: Create coap server task failed\r\n");
    }

    return;
}

int main(void)
{
    platform_init();

    if (wifi_init()) {
        printf("wifi init failed.\r\n");
    }

    coap_server_task_start();

    sys_os_start();

    for ( ; ; );
}
