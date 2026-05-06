/*!
    \file    wifi_softap_provisioning.c
    \brief   provsioning for WiFi softap mode

    \version 2021-10-30, V1.0.0, firmware for GD32VW55x
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

#include "app_cfg.h"
#include "wrapper_os.h"
#include "lwip/sockets.h"
#include "lwip/apps/httpd.h"
#include "lwip/tcpip.h"
#include "lwip/err.h"
#include "wifi_netif.h"
#include "wifi_netlink.h"
#include "wifi_management.h"
#include "wifi_softap_provisioning.h"
#include "dnsd.h"

#ifdef CONFIG_WIFI_MESH_SMART
#include "wifi_mesh_smart.h"
#endif

#ifdef CONFIG_SOFTAP_PROVISIONING

static os_task_t* provisioning_task_tcb = NULL;

static uint8_t config_ssid[WIFI_SSID_MAX_LEN + 1];
static uint8_t config_password[WPA_MAX_PSK_LEN + 1];

static uint8_t ap_ssid[WIFI_SSID_MAX_LEN + 1];
const uint8_t ap_password[] = "12345678";

#define PROVISIONING_TASK_STK_SIZE                  512
#define PROVISIONING_TASK_QUEUE_SIZE                4
#define PROVISIONING_TASK_PRIO                      (16 + 2)

#define MAX_RETRY_COUNT                             5

typedef enum {
    PROVISIONING_MSG_CONFIGURED = 1,
    PROVISIONING_MSG_STA_CONNECT_OK,
    PROVISIONING_MSG_STA_CONNECT_FAILED,
    PROVISIONING_MSG_STOP,
} SOFTAP_PROVISIONING_MESSAGE_TYPE_E;

typedef enum {
    PROVISIONING_STATE_IDLE = 0,
    PROVISIONING_STATE_WAIT_CONFIGURED,
    PROVISIONING_STATE_STA_CONNECTING,
    PROVISIONING_STATE_SUCCESSFUL,
} SOFTAP_PROVISIONING_STATE_E;

// callback function
static void sta_cb_conn_ok(void *eloop_data, void *user_ctx);
static void sta_cb_conn_fail(void *eloop_data, void *user_ctx);

static void wifi_softap_provisonging_send_msg(SOFTAP_PROVISIONING_MESSAGE_TYPE_E msg_type)
{

    SOFTAP_PROVISIONING_MESSAGE_TYPE_E msg = msg_type;

    if (provisioning_task_tcb == NULL)
        return;

    sys_task_post(provisioning_task_tcb, &msg, 0);
}


static void sta_cb_conn_ok(void *eloop_data, void *user_ctx)
{
    eloop_event_unregister(WIFI_MGMT_EVENT_CONNECT_SUCCESS, sta_cb_conn_ok);
    eloop_event_unregister(WIFI_MGMT_EVENT_CONNECT_FAIL, sta_cb_conn_fail);
    wifi_softap_provisonging_send_msg(PROVISIONING_MSG_STA_CONNECT_OK);
}


static void sta_cb_conn_fail(void *eloop_data, void *user_ctx)
{
    eloop_event_unregister(WIFI_MGMT_EVENT_CONNECT_SUCCESS, sta_cb_conn_ok);
    eloop_event_unregister(WIFI_MGMT_EVENT_CONNECT_FAIL, sta_cb_conn_fail);
    wifi_softap_provisonging_send_msg(PROVISIONING_MSG_STA_CONNECT_FAILED);
}

static void wifi_softap_provisioning(void *arg)
{
    wifi_ap_auth_mode_t auth_mode = AUTH_MODE_WPA2;
    SOFTAP_PROVISIONING_MESSAGE_TYPE_E msg_type;
    SOFTAP_PROVISIONING_STATE_E state = PROVISIONING_STATE_IDLE;
    int max_cnt_count = MAX_RETRY_COUNT;

    LOCK_TCPIP_CORE();
    httpd_init();
    UNLOCK_TCPIP_CORE();
    dns_server_start();

    uint8_t *addr = wifi_vif_mac_addr_get(WIFI_VIF_INDEX_DEFAULT);
    snprintf((char *)ap_ssid, sizeof(ap_ssid), "wifi_provisioning_%02x_%02x_%02x", addr[3], addr[4], addr[5]);

    wifi_management_ap_start((char *)ap_ssid, (char *)ap_password, 1, auth_mode, 0);
    state = PROVISIONING_STATE_WAIT_CONFIGURED;

    while (1) {
        if (sys_task_wait(0, &msg_type) == OS_OK) {
            if (msg_type == PROVISIONING_MSG_CONFIGURED) {
                if (state == PROVISIONING_STATE_WAIT_CONFIGURED) {
                #ifdef CONFIG_WIFI_MESH_SMART
                    wifi_management_ap_stop();
                    state = PROVISIONING_STATE_SUCCESSFUL;
                    netlink_printf("softap provisioning get ssid and pwd ok, config mesh smart and exit provision\r\n");
                    wifi_mesh_smart_config_rootap_info((char *)config_ssid, (char *)config_password);
                    break;
                #else
                    netlink_printf("softap provisioning got configure, start connecting\r\n");
                    state = PROVISIONING_STATE_STA_CONNECTING;
                    wifi_management_sta_start();

                    if (wifi_management_connect((char *)config_ssid, (char *)config_password, 0) < 0) {
                        netlink_printf("softap provisioning start connecting failed\r\n");
                        wifi_management_ap_start((char *)ap_ssid, (char *)ap_password, 1, auth_mode, 0);
                        state = PROVISIONING_STATE_WAIT_CONFIGURED;
                    } else {
                        eloop_event_register(WIFI_MGMT_EVENT_CONNECT_SUCCESS, sta_cb_conn_ok, NULL, NULL);
                        eloop_event_register(WIFI_MGMT_EVENT_CONNECT_FAIL, sta_cb_conn_fail, NULL, NULL);
                    }
                #endif
                } else {
                    netlink_printf("softap provisioning got dulicate configure, state %d\r\n", state);
                }
            } else if (msg_type == PROVISIONING_MSG_STA_CONNECT_OK) {
                state = PROVISIONING_STATE_SUCCESSFUL;
                netlink_printf("softap provisioning connnect ok, exit provision\r\n");
                break;
            }
            else if (msg_type == PROVISIONING_MSG_STA_CONNECT_FAILED) {
                if (max_cnt_count == 0) {
                    max_cnt_count = MAX_RETRY_COUNT;
                    netlink_printf("softap provisioning connnect failed, restart softap\r\n");
                    wifi_management_ap_start((char *)ap_ssid, (char *)ap_password, 1, auth_mode, 0);
                    state = PROVISIONING_STATE_WAIT_CONFIGURED;
                } else {
                #ifdef CONFIG_WIFI_MESH_SMART
                    wifi_management_ap_stop();
                    state = PROVISIONING_STATE_SUCCESSFUL;
                    wifi_mesh_smart_config_rootap_info((char *)config_ssid, (char *)config_password);
                    netlink_printf("softap provisioning get ssid and pwd ok, config mesh smart and exit provision\r\n");
                    break;
                #else
                    max_cnt_count--;
                    state = PROVISIONING_STATE_STA_CONNECTING;
                    wifi_management_sta_start();
                    netlink_printf("retry (%d) to connect config_ssid=%s config_password=%s\r\n", max_cnt_count, config_ssid, config_password);
                    wifi_management_connect((char *)config_ssid, (char *)config_password, 0);
                    eloop_event_register(WIFI_MGMT_EVENT_CONNECT_SUCCESS, sta_cb_conn_ok, NULL, NULL);
                    eloop_event_register(WIFI_MGMT_EVENT_CONNECT_FAIL, sta_cb_conn_fail, NULL, NULL);
                #endif
                }
            }
            else if (msg_type == PROVISIONING_MSG_STOP) {
                netlink_printf("softap provisioning stop\r\n");
                wifi_management_ap_stop();
                state = PROVISIONING_STATE_IDLE;
                break;
            }
        }
    }

    // Other task should lock tcpip core if call raw api
    LOCK_TCPIP_CORE();
    httpd_stop();
    UNLOCK_TCPIP_CORE();

    dns_server_stop();
    provisioning_task_tcb = NULL;
    netlink_printf("softap provisioning exit, state %d\r\n", state);
    sys_task_delete(NULL);
}

void wifi_softap_provisioning_start(void)
{
    if (provisioning_task_tcb)
        return;

    provisioning_task_tcb = sys_task_create(NULL, (const uint8_t *)"ap_prov", NULL, PROVISIONING_TASK_STK_SIZE,
        PROVISIONING_TASK_QUEUE_SIZE, sizeof(SOFTAP_PROVISIONING_MESSAGE_TYPE_E),
        PROVISIONING_TASK_PRIO, (task_func_t)wifi_softap_provisioning, NULL);

    if (provisioning_task_tcb == NULL)
        netlink_printf("softap provisioning start failed\r\n");
}

void wifi_softap_provisioning_stop(void)
{
    wifi_softap_provisonging_send_msg(PROVISIONING_MSG_STOP);
}

int32_t wifi_softap_provisioning_config(uint8_t *ssid, uint8_t *pass)
{
    uint32_t len;
    int32_t ret = 0;

    do {
        len = strlen((const char *)ssid);
        if (len > WIFI_SSID_MAX_LEN) {
            ret = -1;
            break;
        }

        // Spaces in the ssid entered on the page will be URL-encoded as +. Here, the + needs to be reverted back to spaces.
        for (int i = 0; i < len; i++) {
            if (ssid[i] == '+') {
                ssid[i] = ' ';
            }
        }

        sys_memcpy(config_ssid, ssid, len);
        config_ssid[len] = 0;

        len = strlen((const char *)pass);
        if (len > WPA_MAX_PSK_LEN || len < 8) {
            ret = -1;
            break;
        }
        sys_memcpy(config_password, pass, len);
        wifi_softap_provisonging_send_msg(PROVISIONING_MSG_CONFIGURED);
    } while(0);

    return ret;
}
#endif
