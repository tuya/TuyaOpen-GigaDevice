/*!
    \file    websocket_client_main.c
    \brief   the example of websocket client in station mode

    \version 2025-07-17, V1.0.0, firmware for GD32VW55x
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

#include <stdint.h>
#include <stdio.h>
#include "app_cfg.h"
#include "gd32vw55x_platform.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "wifi_management.h"
#include "wifi_init.h"
#include "tinyws.h"

#define SSID            "GL_6019"
#define PASSWORD        "12345678" // NULL // ""

#define WS_URI              "wss://echo.websocket.events"
#define WS_HEADER           "Cache-Control: max-age=3600\r\n"
#define PING_INTERVAL       10
#define PINGPONG_TIMEOUT    120
#define BUF_SIZE            1024
#define TIMEOUT_MS          15000
#define WS_DATA_TEXT        "Websocket test data"
#define WS_DATA_LEN         (sizeof(WS_DATA_TEXT) - 1)
#define SEND_TIMEOUT_MS     10000
#define SEND_DATA_COUNT     5
#define SEND_INTV_MS        (PING_INTERVAL * 1000)

static struct ws_session_info_t ws_session_info = {
    PING_INTERVAL,
    PINGPONG_TIMEOUT,
    BUF_SIZE
};

static void at_ws_session_event_ind(struct ws_session *ws, ws_session_event_t event, uint8_t *data, size_t len)
{
    uint32_t i;
    switch (event) {
    case WS_EVENT_CONNECTED:
        printf("websocket connected\r\n");
        break;
    case WS_EVENT_RX_TXT_DATA:
        printf("websocket RX text data:\r\n");
        for (i = 0; i < len; i++) {
            printf("%c", data[i]);
        }
        printf("\r\n");
        break;
    case WS_EVENT_RX_BIN_DATA:
        printf("websocket RX binary data:\r\n");
        for (i = 0; i < len; i++) {
            printf("0x%02x", data[i]);
        }
        printf("\r\n");
        break;
    case WS_EVENT_DISCONNECT:
        printf("websocket disconnect:\r\n");
        break;
    default:
        break;
    }
}

static void websocket_client_test(void)
{
    int ret = 0, readable = 0, count = 0;
    char *ws_data_text = WS_DATA_TEXT;
    uint8_t ws_data_len = WS_DATA_LEN;
    uint64_t send_tick_ms;

    struct ws_session *ws = (struct ws_session *)sys_malloc(sizeof(struct ws_session));
    if (ws == NULL) {
        printf("ws malloc fail.\r\n");
        return;
    }
    sys_memset(ws, 0, sizeof(struct ws_session));

    ret = at_ws_session_init(&ws, WS_URI, NULL, NULL, NULL, WS_HEADER, &ws_session_info, TIMEOUT_MS, (ws_event_indicate_fun_t)at_ws_session_event_ind);
    if (ret != 0) {
        printf("ws init fail.\r\n");
        return;
    }

    ws->run = true;

    ws->state = WS_STATE_INIT;

    send_tick_ms = sys_current_time_get();

    while (ws->run) {
        switch (ws->state) {
            case WS_STATE_INIT:
                if (ws_session_connect(ws) < 0) {
                    printf("net connect failed\r\n");
                    ws_net_error_abort(ws);
                    break;
                }
                printf("Connected to %s://%s:%d\r\n", ws->conf.scheme, ws->conf.host, ws->conf.port);

                ws->state = WS_STATE_CONNECTED;
                ws->wait_for_pong_resp = false;
                ws->ind(ws, WS_EVENT_CONNECTED, NULL, 0);

                break;
            case WS_STATE_CONNECTED:
                if (readable < 0) {
                    ws_net_error_abort(ws);
                    break;
                }

                if (sys_current_time_get() - ws->ping_tick_ms > ws->conf.ping_interval_sec*1000) {
                    ws->ping_tick_ms = sys_current_time_get();
                    printf("Sending PING...\r\n");
                    ws_write(ws, WS_OPCODE_PING | WS_FIN, WS_MASK, NULL, 0);

                    if (!ws->wait_for_pong_resp && ws->conf.pingpong_timeout_sec) {
                        ws->pingpong_tick_ms = sys_current_time_get();
                        ws->wait_for_pong_resp = true;
                    }
                }

                if (sys_current_time_get() - ws->pingpong_tick_ms > ws->conf.pingpong_timeout_sec * 1000 ) {
                    if (ws->wait_for_pong_resp) {
                        printf("Error, no PONG received for more than %d seconds after PING\r\n", ws->conf.pingpong_timeout_sec);
                        ws_net_error_abort(ws);
                        break;
                    }
                }

                if (readable == 0) {
                    printf("session no data\r\n");
                    break;
                }
                ws->ping_tick_ms = sys_current_time_get();

                if (ws_read(ws, ws->rx_buf, ws->rx_buf_size) < 0) {
                    printf("read data failed\r\n");
                    ws_net_error_abort(ws);
                    break;
                }
                break;

            case WS_STATE_NET_ERROR:
                ws->ind(ws, WS_EVENT_DISCONNECT, NULL, 0);
                if (!ws->auto_reconnect) {
                    ws->run = false;
                    // ws->ind(ws, WS_EVENT_DISCONNECT, NULL, 0);
                    break;
                }

                if (sys_current_time_get() - ws->reconnect_tick_ms > ws->wait_timeout_ms) {
                    ws->state = WS_STATE_INIT;
                    ws->reconnect_tick_ms = sys_current_time_get();
                    printf("Reconnecting...\r\n");
                }
                break;

            case WS_STATE_CLOSING:
                if (!ws->close_sended) {
                    if (ws_write(ws, WS_OPCODE_CLOSE | WS_FIN, WS_MASK, NULL, 0) < 0) {
                        printf("send close failed, close it anyway\r\n");
                    }
                    ws->close_sended = true;
                }
                ws->ind(ws, WS_EVENT_DISCONNECT, NULL, 0);
                break;

            default:
                printf("default state: %d\r\n", ws->state);
                break;
        }

        if (WS_STATE_CONNECTED == ws->state) {
            readable = ws_poll_read(ws->fd, 1000); //Poll every 1000ms
            if (readable < 0) {
                printf("poll read returned %d, errno=%d", readable, errno);
            }
            if (sys_current_time_get() - send_tick_ms > SEND_INTV_MS) {
                send_tick_ms = sys_current_time_get();
                if (count < SEND_DATA_COUNT) {
                    ws_session_write_op(ws, WS_OPCODE_TEXT | WS_FIN, (uint8_t *)ws_data_text, ws_data_len, SEND_TIMEOUT_MS);
                    count++;
                } else {
                    ws_session_write_op(ws, WS_OPCODE_CLOSE | WS_FIN, (uint8_t *)ws_data_text, ws_data_len, SEND_TIMEOUT_MS);
                }
            }
        } else if (WS_STATE_NET_ERROR == ws->state) {
            sys_ms_sleep(ws->wait_timeout_ms);
        }
        else if (WS_STATE_CLOSING == ws->state) {
            if (ws->close_sended) {
                printf("websocket is closed\r\n");
                ws->run = false;
                ws->state = WS_STATE_UNKNOW;
            }
            break;
        }
    }

    ws_net_error_abort(ws);
    ws->state = WS_STATE_UNKNOW;
    sys_sema_up(&ws->exit_sem);
    ws_session_close(ws);
}

static void ws_session_task(void *param)
{
    int status = 0;
    char *ssid = SSID;
    char *password = PASSWORD;
    struct mac_scan_result candidate;

    if (ssid == NULL) {
        printf("ssid can not be NULL!\r\n");
        goto exit;
    }

    /*
    * 1. Start Wi-Fi scan
    */
    printf("Start Wi-Fi scan.\r\n");
    status = wifi_management_scan(1, ssid);
    if (status != 0) {
        printf("Wi-Fi scan failed.\r\n");
        goto exit;
    }
    sys_memset(&candidate, 0, sizeof(struct mac_scan_result));
    status = wifi_netlink_candidate_ap_find(WIFI_VIF_INDEX_DEFAULT, NULL, ssid, &candidate);
    if (status != 0) {
        goto exit;
    }

    /*
    * 2. Start Wi-Fi connection
    */
    printf("Start Wi-Fi connection.\r\n");
    if (wifi_management_connect(ssid, password, 1) != 0) {
        printf("Wi-Fi connection failed\r\n");
        goto exit;
    }

    /*
    * 3. Start WebSocket client
    */
    printf("Start WebSocket client.\r\n");
    websocket_client_test();

    /*
    * 4. Stop Wi-Fi connection
    */
    printf("Stop Wi-Fi connection.\r\n");
    wifi_management_disconnect();

exit:
    printf("the test has ended.\r\n");
    sys_task_delete(NULL);
}

int main(void)
{
    platform_init();

    if (wifi_init()) {
        printf("wifi init failed.\r\n");
    }

    sys_task_create_dynamic((const uint8_t *)"ws_client", 1536, OS_TASK_PRIORITY(0), ws_session_task, NULL);

    sys_os_start();

    for ( ; ; );
}