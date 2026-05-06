/*!
    \file    tinyws.h
    \brief   websockets implement for GD32VW55x SDK

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
#ifndef __TINYWS_H
#define __TINYWS_H

#include <stdbool.h>
#include "ws_ssl.h"

#define DEBUG_WS    2

#define WS_LOG(level, fmt, ...) printf("[WS %s] %s: " fmt "\r\n", level, __FUNCTION__, ##__VA_ARGS__)

#if DEBUG_WS >= 2
#define WS_DEBUG(fmt, ...) WS_LOG("DEBUG", fmt, ##__VA_ARGS__)
#else
#define WS_DEBUG(fmt, ...)
#endif

#if DEBUG_WS >= 1
#define WS_ERROR(fmt, ...) WS_LOG("ERROR", fmt, ##__VA_ARGS__)
#define WS_WARN(fmt, ...) WS_LOG("WARN", fmt, ##__VA_ARGS__)
#else
#define WS_ERROR(fmt, ...)
#define WS_WARN(fmt, ...)
#endif

#if TLS_VERIFY_SERVER_REQUIRED == 1
#define WS_TASK_STK_SIZE                  1536
#else
#define WS_TASK_STK_SIZE                  768
#endif
#define WS_TASK_QUEUE_SIZE                0
#define WS_TASK_PRIO                      16

#define WS_MASK                         0x80
#define WS_SIZE_2B                      126
#define WS_SIZE_4B                      127
#define WEBSOCKET_HDR_SIZE              16


#define WS_FIN                      0x80
#define WS_OPCODE_CONT              0x00
#define WS_OPCODE_TEXT              0x01
#define WS_OPCODE_BINARY            0x02
#define WS_OPCODE_CLOSE             0x08
#define WS_OPCODE_PING              0x09
#define WS_OPCODE_PONG              0x0a
#define WS_OPCODE_CONTROL_FRAME     0x08

#define WS_MAX_LINK_NUM             3

#define WS_MAX_REQ_HEADER_NUM       5
#define WS_MAX_REQ_HEADER_LEN       256

typedef enum {
    WS_STATE_UNKNOW = -1,
    WS_STATE_INIT = 0,
    WS_STATE_CONNECTED,
    WS_STATE_NET_ERROR,
    WS_STATE_CLOSING,
    WS_STATE_MAX
} ws_session_state_t;


typedef enum {
    WS_EVENT_UNKNOW = -1,
    WS_EVENT_CONNECTED,
    WS_EVENT_RX_TXT_DATA,
    WS_EVENT_RX_BIN_DATA,
    WS_EVENT_DISCONNECT,
    WS_EVENT_MAX
} ws_session_event_t;

struct ws_rx_frame {
    uint8_t op;
    char mask_key[4];
    int payload_len;
    int remaining;
    int pos;
    int total_len;
    bool hdr_recved;
    bool fin_frame;
};

struct ws_session_info_t {
    uint32_t ping_interval_sec;
    uint32_t pingpong_timeout_sec;
    uint32_t tx_buf_size;
};

struct ws_session_conf{
    char                        host[128];
    char                        path[128];
    char                        subprotocol[65];
    char                        origin[200];
    char                        *scheme;
    int                         port;
    bool                        port_default;
    char                        *headers;
    char                        *auth;
    int                         pingpong_timeout_sec;
    size_t                      ping_interval_sec;
    bool                        ssl;
} ;



struct ws_session
{
    os_task_t                task_handle;
    os_mutex_t                  lock;
    os_sema_t                   exit_sem;
    int fd;
    int (*net_connect)(struct ws_session *ws);
    int (*net_close)(struct ws_session *ws);
    int (*net_write)(struct ws_session *ws, uint8_t *data, size_t data_len);
    int (*net_read)(struct ws_session *ws, uint8_t *data, size_t data_len);
    void (*ind)(struct ws_session *ws, ws_session_event_t event, uint8_t *data, size_t len);
    struct ws_session_conf      conf;
    ws_session_state_t          state;
    uint64_t                    keepalive_tick_ms;
    uint64_t                    reconnect_tick_ms;
    uint64_t                    ping_tick_ms;
    uint64_t                    pingpong_tick_ms;
    uint32_t                    wait_timeout_ms;
    uint8_t                     *rx_buf;
    uint8_t                     *tx_buf;
    int                         rx_buf_size;
    int                         tx_buf_size;
    struct ws_rx_frame          rx_frame;
    bool                        close_sended;
    bool                        auto_reconnect;
    bool                        run;
    bool                        wait_for_pong_resp;
    void                        *tls;
    void                        *priv;
};

typedef void (*ws_event_indicate_fun_t)(struct ws_session *ws, ws_session_event_t event, uint8_t *data, size_t len);

struct ws_session *ws_session_init(char *url, int port, char *path, char* origin, int tx_buf_len, int rx_buf_len, ws_event_indicate_fun_t ind);
int at_ws_session_init(
    struct ws_session **ws,
    const char *uri,
    const char *origin,
    const char *sub_protocol,
    const char *auth,
    const char *all_headers,
    struct ws_session_info_t *ws_info,
    uint32_t timeout_ms,
    ws_event_indicate_fun_t ind
);

int ws_session_connect(struct ws_session *ws);
int ws_net_error_abort(struct ws_session *ws);
int ws_session_start(struct ws_session* ws);
int ws_write(struct ws_session *ws, int opcode, int mask_flag, const uint8_t *buffer, int len);
int ws_session_write_txt(struct ws_session *ws, uint8_t *buf, int len);
int ws_session_write_bin(struct ws_session *ws, uint8_t *buf, int len);
int ws_session_write_op(struct ws_session *ws, uint32_t op, uint8_t *buf, int len, uint32_t timeout_ms);
int ws_read(struct ws_session *ws, uint8_t *buf, int len);
int ws_poll_read(int fd, int timeout_ms);
int ws_session_free(struct ws_session *ws);
int ws_session_close(struct ws_session *ws);
int ws_session_set_header(struct ws_session *ws, const char * header);
int ws_session_set_autoreconnect(struct ws_session *ws, bool auto_reconnect);
int ws_session_set_auth(struct ws_session *ws, const char *auth);
#endif