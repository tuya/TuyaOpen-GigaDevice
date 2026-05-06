/*!
    \file    wifi_mesh_smart_config.c
    \brief   Config file of wifi mesh smart.

    \version 2025-07-20, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2025, GigaDevice Semiconductor Inc.

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

#include"app_cfg.h"
#include "wifi_management.h"
#include "gd32vw55x_gpio.h"
#include "wrapper_os.h"
#include "wifi_init.h"

#ifdef CONFIG_WIFI_MESH_SMART

#include "wifi_mesh_smart.h"

bool mesh_smart_led_enabled = false;

#define DEFAULT_MESH_ROLE                   MESH_SMART_NODE_TYPE_ROUTER

#define WIFI_MESH_SMART_INIT_TASK_STK_SIZE  512
#define WIFI_MESH_SMART_INIT_TASK_PRIO      OS_TASK_PRIORITY(1)

#define LED_GPIO_GROUP      GPIOB
#define LED_RED             GPIO_PIN_11
#define LED_GREEN           GPIO_PIN_12
#define LED_BLUE            GPIO_PIN_13
#define MESH_LED_ON(led)    gpio_bit_set(LED_GPIO_GROUP, led)
#define MESH_LED_OFF(led)   gpio_bit_reset(LED_GPIO_GROUP, led)

void led_level_show(uint8_t level)
{
    if (!mesh_smart_led_enabled) {
        return;
    }

    switch (level) {
    case 1:
        MESH_LED_ON(LED_RED);
        MESH_LED_ON(LED_GREEN);
        MESH_LED_ON(LED_BLUE);
        break;
    case 2:
        MESH_LED_ON(LED_RED);
        MESH_LED_OFF(LED_GREEN);
        MESH_LED_OFF(LED_BLUE);
        break;
    case 3:
        MESH_LED_OFF(LED_RED);
        MESH_LED_ON(LED_GREEN);
        MESH_LED_OFF(LED_BLUE);
        break;
    case 4:
        MESH_LED_OFF(LED_RED);
        MESH_LED_OFF(LED_GREEN);
        MESH_LED_ON(LED_BLUE);
        break;
    case 5:
        MESH_LED_OFF(LED_RED);
        MESH_LED_ON(LED_GREEN);
        MESH_LED_ON(LED_BLUE);
        break;
    default:
        MESH_LED_OFF(LED_RED);
        MESH_LED_OFF(LED_GREEN);
        MESH_LED_OFF(LED_BLUE);
        break;
    }
}

static void wifi_mesh_smart_led_config(bool enable)
{
    if (!enable) {
        mesh_smart_led_enabled = false;
        return;
    } else {
        mesh_smart_led_enabled = true;
        /* enable the LED GPIO clock */
        rcu_periph_clock_enable(RCU_GPIOB);
        /* configure LED GPIO pin */
        gpio_mode_set(LED_GPIO_GROUP, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_RED);
        gpio_output_options_set(LED_GPIO_GROUP, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, LED_RED);

        gpio_mode_set(LED_GPIO_GROUP, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_GREEN);
        gpio_output_options_set(LED_GPIO_GROUP, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, LED_GREEN);

        gpio_mode_set(LED_GPIO_GROUP, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_BLUE);
        gpio_output_options_set(LED_GPIO_GROUP, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, LED_BLUE);
    }
}

static void wifi_mesh_smart_init_task(void *arg)
{
    wifi_mesh_smart_cfg_t mesh_smart_info_cfg = {0};

    /* Wait for WiFi subsystem ready before any WiFi API calls */
    wifi_wait_ready();

    mesh_smart_info_cfg.vendor_id[0] = DEFAULT_VENDOR_IE_OUI_0;
    mesh_smart_info_cfg.vendor_id[1] = DEFAULT_VENDOR_IE_OUI_1;
    mesh_smart_info_cfg.mesh_smart_network_id = DEFAULT_MESH_SMART_NETWORK_ID;
    mesh_smart_info_cfg.mesh_smart_max_clients_number = WIFI_MESH_SMART_SOFTAP_MAX_CLIENTS_NUMBER;
    mesh_smart_info_cfg.mesh_smart_max_level = CONFIG_MESH_SMART_MAX_LEVEL;
    mesh_smart_info_cfg.mesh_role_type = DEFAULT_MESH_ROLE;
    sys_memcpy(mesh_smart_info_cfg.mesh_smart_softap_ssid, DEFAULT_MESH_SMART_SOFTAP_SSID, strlen(DEFAULT_MESH_SMART_SOFTAP_SSID));
    sys_memcpy(mesh_smart_info_cfg.mesh_smart_softap_password, DEFAULT_MESH_SMART_SOFTAP_PASSWORD, strlen(DEFAULT_MESH_SMART_SOFTAP_PASSWORD));
    mesh_smart_info_cfg.softap_ip_segment = 127;

    /* should start task first, because the network config will send event to task */
    if (wifi_mesh_smart_task_start() != 0) {
        app_print("MESH_SMART: mesh smart task start failed\r\n");
        sys_task_delete(NULL);
        return;
    }

    wifi_mesh_smart_led_config(true);

    if (wifi_mesh_smart_network_config(&mesh_smart_info_cfg, 1) != 0) {
        app_print("MESH_SMART: mesh smart network config failed\r\n");
        sys_task_delete(NULL);
        return;
    }

    wifi_management_roaming_set(false, -75);
    sys_task_delete(NULL);
}

void wifi_mesh_smart_network_init(void)
{
    if (sys_task_create(NULL, (const uint8_t *)"mesh_init", NULL,
                        WIFI_MESH_SMART_INIT_TASK_STK_SIZE, 0, 0,
                        WIFI_MESH_SMART_INIT_TASK_PRIO,
                        (task_func_t)wifi_mesh_smart_init_task, NULL) == NULL) {
        app_print("MESH_SMART: mesh smart init task start failed\r\n");
    }
}

#endif /* CONFIG_WIFI_MESH_SMART */
