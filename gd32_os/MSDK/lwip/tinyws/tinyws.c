/*!
    \file    tinyws.c
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
#include "wrapper_os.h"
#include "mbedtls/base64.h"
#include "mbedtls/sha1.h"
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "trng.h"
#include "tinyws.h"


static char *trimwhitespace(const char *str)
{
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) {
        return (char *)str;
    }

    // Trim trailing space
    end = (char *)(str + strlen(str) - 1);
    while (end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end + 1) = 0;

    return (char *)str;
}

char *my_strcasestr(const char *haystack, const char *needle) {
    if (!*needle) {
        return (char *)haystack;
    }

    const char *p1 = haystack;
    while (*p1) {
        const char *p1_advance = p1;
        const char *p2 = needle;

        while (*p2 && *p1_advance && tolower((unsigned char)*p1_advance) == tolower((unsigned char)*p2)) {
            p1_advance++;
            p2++;
        }

        if (!*p2) {
            return (char *)p1;
        }

        p1++;
    }

    return NULL;
}

static char *find_http_header(const char *buffer, const char *key)
{
    char *found = my_strcasestr(buffer, key);
    if (found) {
        found += strlen(key);
        char *found_end = strstr(found, "\r\n");
        if (found_end) {
            found_end[0] = 0;//terminal string
            return trimwhitespace(found);
        }
    }
    return NULL;
}


int ws_session_free(struct ws_session *ws)
{
    if (ws->tx_buf) {
        sys_mfree(ws->tx_buf);
        ws->tx_buf = NULL;
    }

    if (ws->rx_buf) {
        sys_mfree(ws->rx_buf);
        ws->rx_buf = NULL;
    }

    if (ws->conf.auth) {
        sys_mfree(ws->conf.auth);
        ws->conf.auth = NULL;
    }

    if (ws->conf.headers) {
        sys_mfree(ws->conf.headers);
        ws->conf.headers = NULL;
    }

    if (ws->lock) {
        sys_mutex_free(&ws->lock);
        ws->lock = NULL;
    }

    if (ws->exit_sem) {
        sys_sema_free(&ws->exit_sem);
        ws->exit_sem = NULL;
    }

    sys_mfree(ws);
    return 0;
}

int ws_net_connect(struct ws_session *ws)
{
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *p;
    int ret;
    int opt = 1, option = 1;
    char sport[5];
    memset(sport, 0, 5);
    sprintf(sport, "%d", ws->conf.port);
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((ret = getaddrinfo(ws->conf.host, sport, &hints, &res)) != 0) {
        WS_ERROR("getaddrinfo failed return %d\r\n", ret);
        ws->fd = -1;
        return -1;
    }
    ret = -1;
    for(p = res; p != NULL; p = p->ai_next){
        ws->fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (ws->fd < 0) {
            continue;
        }

        if((setsockopt(ws->fd, SOL_SOCKET, SO_KEEPALIVE, (const char *)&opt, sizeof(opt))) < 0){
            WS_ERROR("set keepalive failed!\n");
            close(ws->fd);
            ws->fd = -1;
            break;
        }

        if((setsockopt(ws->fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&option, sizeof(option))) < 0){
            WS_ERROR("set SO_REUSEADDR failed!\n");
            close(ws->fd);
            ws->fd = -1;
            break;
        }

        if (connect(ws->fd, p->ai_addr, p->ai_addrlen) == 0) {
            ret = 0;
            break;
        }
    }
    freeaddrinfo(res);
    return ret;
}

int ws_net_close(struct ws_session *ws)
{
    int ret;

    if (ws->fd < 0)
        return 0;

    ret = close(ws->fd);
    ws->fd = -1;
    return ret;
}

int ws_net_write(struct ws_session *ws, uint8_t *data, size_t data_len)
{
    int ret;
    int err = 0;
    uint32_t err_len = sizeof(err);

    ret = send(ws->fd, data, data_len, 0);

    if (ret < 0)
        getsockopt(ws->fd, SOL_SOCKET, SO_ERROR, &err, &err_len);

    if (ret < 0 && (err == EWOULDBLOCK || err == EAGAIN || err == ENOMEM))
        return 0;
    else if(ret < 0)
        return -1;
    else
        return ret;
}

int ws_net_read(struct ws_session *ws, uint8_t *data, size_t data_len)
{
    int ret;
    int err = 0;
    uint32_t err_len = sizeof(err);

    ret = recv(ws->fd, data, data_len, 0);

    if (ret < 0) {
        getsockopt(ws->fd, SOL_SOCKET, SO_ERROR, &err, &err_len);
    }

    if (ret < 0 && (err == EWOULDBLOCK || err == EAGAIN || err == ENOMEM))
        return 0;
    else if (ret < 0)
        return -1;
    else
        return ret;
}

int wss_net_connect(struct ws_session *ws)
{
    int ret;
    static int opt = 1, option = 1;

    ws->fd = -1;
    ws->tls = (void *)wss_tls_connect(&ws->fd, ws->conf.host, ws->conf.port);

    if(ws->tls == NULL){
        WS_ERROR("ssl connect failed\n");
        goto exit;
    }

    if((ret = setsockopt(ws->fd, SOL_SOCKET, SO_KEEPALIVE, (const char *)&opt, sizeof(opt))) < 0){
        WS_ERROR("set SO_KEEPALIVE failed %d\n", ret);
        goto exit;
    }
    if((ret = setsockopt(ws->fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&option, sizeof(option))) < 0){
        WS_ERROR("set SO_REUSEADDR failed %d\n", ret);
        goto exit;
    }

    if((ret = wss_tls_handshake(ws->tls)) !=0 ){
        WS_ERROR("wss_tls_handshake failed %d\n", ret);
        goto exit;
    }
    return 0;
exit:
    wss_tls_close(ws->tls, &ws->fd);
    return -1;
}

int wss_net_close(struct ws_session *ws)
{
    if (ws->fd < 0)
        return -1;
    wss_tls_close(ws->tls, &ws->fd);

    ws->fd = -1;
    return 0;
}

int wss_net_read(struct ws_session *ws, uint8_t *data, size_t data_len)
{
    int ret = 0;

    ret = wss_tls_read(ws->tls, data, data_len);
    int err = 0;
    uint32_t err_len = sizeof(err);

    if (ret < 0) {
        getsockopt(ws->fd, SOL_SOCKET, SO_ERROR, &err, &err_len);
    }
    if(ret < 0 && (err == EWOULDBLOCK || err == EAGAIN || err == ENOMEM)) {
        return 0;
    }
    else if(ret < 0){
        WS_DEBUG("wss_tls_read failed %d", ret);
        return -1;
    }
    else
        return ret;
}

int wss_net_write(struct ws_session *ws, uint8_t *data, size_t data_len)
{
    int ret = 0;

    ret = wss_tls_write(ws->tls, data, data_len);
    int err = 0;
    uint32_t err_len = sizeof(err);

    if (ret < 0) {
        getsockopt(ws->fd, SOL_SOCKET, SO_ERROR, &err, &err_len);
    }
    if(ret < 0 && (err == EWOULDBLOCK || err == EAGAIN || err == ENOMEM)) {
        return 0;
    }
    else if(ret < 0){
        WS_DEBUG("ssl_write failed, return: %d", ret);
        return -1;
    }
    else
        return ret;
}

int ws_set_net_ops(struct ws_session *ws)
{
    if (ws->conf.ssl) {
        ws->net_close = &wss_net_close;
        ws->net_connect = &wss_net_connect;
        ws->net_write = &wss_net_write;
        ws->net_read = &wss_net_read;
    } else {
        ws->net_close = &ws_net_close;
        ws->net_connect = &ws_net_connect;
        ws->net_write = &ws_net_write;
        ws->net_read = &ws_net_read;
    }
    return 0;
}

int ws_net_error_abort(struct ws_session *ws)
{
    ws->state = WS_STATE_NET_ERROR;
    ws->reconnect_tick_ms = sys_current_time_get();
    return ws->net_close(ws);
}

int ws_session_connect(struct ws_session *ws)
{
    uint8_t random_key[16];
    uint8_t client_key[28] = {0};
    struct ws_session_conf *ws_conf = &ws->conf;

    const char *user_agent_ptr = (ws_conf->origin[0] == 0) ? "GD32 Websocket Client" : (const char *)ws_conf->origin;
    size_t out_len = sizeof(client_key);

    if (ws->net_connect(ws) < 0) {
        WS_ERROR("Error connecting to host");
        return -1;
    }

    random_get(random_key, sizeof(random_key));

    mbedtls_base64_encode(client_key, out_len, &out_len, (const uint8_t *)random_key, sizeof(random_key));

    int len = snprintf((char *)ws->tx_buf, ws->tx_buf_size,
                         "GET %s HTTP/1.1\r\n"
                         "Connection: Upgrade\r\n"
                         "Host: %s:%d\r\n"
                         "User-Agent: %s\r\n"
                         "Upgrade: websocket\r\n"
                         "Sec-WebSocket-Version: 13\r\n"
                         "Sec-WebSocket-Key: %s\r\n",
                         (ws_conf->path[0] != 0) ? ws_conf->path: "/",
                         ws_conf->host,
                         ws_conf->port,
                         user_agent_ptr,
                         client_key);
    if (len <= 0 || len >= ws->tx_buf_size) {
        WS_ERROR("websocket request, tx buf size exceed, len: %d, buffer size: %d", len, ws->tx_buf_size);
        return -1;
    }

    if (ws->conf.subprotocol[0] != '\0') {
        int r = snprintf((char *)ws->tx_buf + len, ws->tx_buf_size - len, "Sec-WebSocket-Protocol: %s\r\n", ws->conf.subprotocol);
        len += r;
        if (r <= 0 || len >= ws->tx_buf_size) {
            WS_ERROR("add subprotocol, tx buf size exceed, len: %d, buffer size: %d", len, ws->tx_buf_size);
            return -1;
        }
    }

    if (ws->conf.auth) {
        int r = snprintf((char *)ws->tx_buf + len, ws->tx_buf_size - len, "Authorization: %s\r\n", ws->conf.auth);
        len += r;
        if (r <= 0 || len >= ws->tx_buf_size) {
            WS_ERROR("add auth, tx buf size exceed, len: %d, buffer size: %d", len, ws->tx_buf_size);
            return -1;
        }
    }

    if (ws->conf.headers) {
        int r = snprintf((char *)ws->tx_buf + len, ws->tx_buf_size - len, "%s", ws->conf.headers);
        len += r;
        if (r <= 0 || len >= ws->tx_buf_size) {
            WS_ERROR("add headers, tx buf size exceed, len: %d, buffer size: %d", len, ws->tx_buf_size);
            return -1;
        }
    }

    int r = snprintf((char *)ws->tx_buf + len, ws->tx_buf_size - len, "\r\n");
    len += r;

    if (r <= 0 || len >= ws->tx_buf_size) {
        WS_ERROR("add tx buf size exceed : %d, buffer size: %d", len, ws->tx_buf_size);
        return -1;
    }

    if (ws->net_write(ws, ws->tx_buf, len) <= 0) {
        WS_ERROR("write websocket header %s", ws->tx_buf);
        return -1;
    }

    int hdr_len = 0;
    do {
        if ((len = ws->net_read(ws, ws->rx_buf + hdr_len, ws->rx_buf_size - hdr_len)) <= 0) {
            WS_ERROR("read response for websocket header %s", ws->rx_buf);
            return -1;
        }
        hdr_len += len;
        ws->rx_buf[hdr_len] = '\0';
        WS_DEBUG("Read websocket header frag %d, current header size: %d, buf:\r\n %s\r\n", len, hdr_len, ws->rx_buf);
    } while (NULL == strstr((char *)ws->rx_buf, "\r\n\r\n") && hdr_len < ws->rx_buf_size);

    char *server_key = find_http_header((const char *)ws->rx_buf, "Sec-WebSocket-Accept:");
    if (server_key == NULL) {
        WS_ERROR("Sec-WebSocket-Accept not found");
        return -1;
    } else {
         WS_DEBUG("Sec-WebSocket-Accept KEY %s", server_key);
    }

    uint8_t local_sha1[20];
    uint8_t local_key[33] = {0};
    const char *local_magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    mbedtls_sha1_context sha1_ctx;
    mbedtls_sha1_init(&sha1_ctx);
    mbedtls_sha1_starts(&sha1_ctx);
    mbedtls_sha1_update(&sha1_ctx, client_key, strlen((const char *)client_key));
    mbedtls_sha1_update(&sha1_ctx, (const uint8_t *)local_magic, strlen(local_magic));
    mbedtls_sha1_finish(&sha1_ctx, local_sha1);
    mbedtls_sha1_free(&sha1_ctx);
    mbedtls_base64_encode(local_key, sizeof(local_key), &out_len, local_sha1, sizeof(local_sha1));
    out_len = out_len < sizeof(local_key) ? out_len : sizeof(local_key) - 1;
    local_key[out_len] = 0;
    WS_DEBUG("local_key %s\r\n", local_key);
    if (strcmp((char *)local_key, (char *)server_key) != 0) {
        WS_ERROR("websocket key verify failed\r\n");
        return -1;
    }

    return 0;
}

int ws_write(struct ws_session *ws, int opcode, int mask_flag, const uint8_t *buffer, int len)
{
    uint8_t *buf = (uint8_t *)buffer;
    uint8_t hdr[WEBSOCKET_HDR_SIZE];
    uint8_t *mask;
    int hdr_len = 0, i;

    hdr[hdr_len++] = opcode;

    if (len <= 125) {
        hdr[hdr_len++] = (uint8_t)(len | mask_flag);
    } else if (len < 65536) {
        hdr[hdr_len++] = WS_SIZE_2B | mask_flag;
        hdr[hdr_len++] = (uint8_t)(len >> 8);
        hdr[hdr_len++] = (uint8_t)(len & 0xFF);
    } else {
        hdr[hdr_len++] = WS_SIZE_4B | mask_flag;
        hdr[hdr_len++] = 0;
        hdr[hdr_len++] = 0;
        hdr[hdr_len++] = 0;
        hdr[hdr_len++] = 0;
        hdr[hdr_len++] = (uint8_t)((len >> 24) & 0xFF);
        hdr[hdr_len++] = (uint8_t)((len >> 16) & 0xFF);
        hdr[hdr_len++] = (uint8_t)((len >> 8) & 0xFF);
        hdr[hdr_len++] = (uint8_t)((len >> 0) & 0xFF);
    }

    if (mask_flag) {
        mask = &hdr[hdr_len];
        random_get(mask, 4);
        hdr_len += 4;

        for (i = 0; i < len; ++i) {
            buf[i] = (buf[i] ^ mask[i & 0x03]);
        }
    }

    if (ws->net_write(ws, hdr, hdr_len) != hdr_len) {
        WS_ERROR("Error write header");
        return -1;
    }

    if (len == 0) {
        return 0;
    }

    int ret = ws->net_write(ws, buf, len);


    return ret;
}


static int ws_read_data(struct ws_session *ws, uint8_t *buf, int len)
{
    struct ws_rx_frame *rx_frame = &ws->rx_frame;
    int to_len;
    int read_len = 0;

    if (rx_frame->remaining > len) {
        WS_DEBUG("to read %d are longer than ws buffer %d", rx_frame->remaining, len);
        to_len = len;
    } else {
        to_len = rx_frame->remaining;
    }

    if (to_len != 0 && (read_len = ws->net_read(ws, buf, to_len)) <= 0) {
        WS_ERROR("Error read data");
        return read_len;
    }
    rx_frame->remaining -= read_len;

    if ((*((uint32_t *)&rx_frame->mask_key[0])) & 0xffffffff) {
        for (int i = 0; i < read_len; i++) {
            buf[i] = (buf[i] ^ rx_frame->mask_key[i % 4]);
        }
    }
    return read_len;
}

static int ws_read_hdr(struct ws_session *ws, uint8_t *buf, int len)
{
    int payload_len;
    struct ws_rx_frame *rx_frame = &ws->rx_frame;
    uint8_t *p = buf;
    uint8_t mask;
    int read_len;
    int header = 2;
    int mask_len = 4;

    rx_frame->hdr_recved = false;

    if ((read_len = ws->net_read(ws, p, header)) <= 0) {
        WS_ERROR("Error read data");
        return read_len;
    }

    rx_frame->hdr_recved = true;

    // remeber the last opcode if it is a continue frame
    if ((*p & 0x0F) != 0)
        rx_frame->op = (*p & 0x0F);

    if (*p & WS_FIN)
        rx_frame->fin_frame = true;
    else
        rx_frame->fin_frame = false;

    p++;
    mask = ((*p >> 7) & 0x01);
    payload_len = (*p & 0x7F);
    p++;

    WS_DEBUG("Opcode: %d, mask: %d, len: %d\r\n", rx_frame->op, mask, payload_len);

    if (payload_len == WS_SIZE_2B) {
        header = 2;
        if ((read_len = ws->net_read(ws, p, header)) <= 0) {
            WS_ERROR("Error read data");
            return read_len;
        }
        payload_len = p[0] << 8 | p[1];
    } else if (payload_len == WS_SIZE_4B) {
        header = 8;
        if ((read_len = ws->net_read(ws, p, header)) <= 0) {
            WS_ERROR("Error read data");
            return read_len;
        }

        if (p[0] != 0 || p[1] != 0 || p[2] != 0 || p[3] != 0) {
            payload_len = 0xFFFFFFFF;
        } else {
            payload_len = p[4] << 24 | p[5] << 16 | p[6] << 8 | p[7];
        }
    }

    if (mask) {
        // Read and store mask
        if (payload_len != 0 && (read_len = ws->net_read(ws, p, mask_len)) <= 0) {
            WS_ERROR("Error read data");
            return read_len;
        }
        memcpy(rx_frame->mask_key, p, mask_len);
    } else {
        memset(rx_frame->mask_key, 0, mask_len);
    }

    rx_frame->payload_len = payload_len;
    rx_frame->remaining = payload_len;
    rx_frame->total_len += payload_len;
    // rx_frame->pos = 0;

    return payload_len;
}


static int ws_handle_ctrl(struct ws_session *ws)
{
    struct ws_rx_frame *rx_frame = &ws->rx_frame;
    uint8_t *to_send = NULL;
    int to_send_len = 0;
    int payload_len = rx_frame->payload_len;
    int ctrl_len;
    int ret = 0;

    if (rx_frame->hdr_recved == false ||
        !(rx_frame->op & WS_OPCODE_CONTROL_FRAME)) {
        return 0;
    }

    to_send_len = payload_len;
    if (to_send_len > 0) {
        to_send = sys_malloc(to_send_len);
        if (to_send == NULL) {
            WS_ERROR("Cannot allocate buffer for control frames, need-%d", to_send_len);
            return -1;
        }
    } else {
        to_send_len = 0;
    }

    ret = 0;

    do {
        ctrl_len = ws_read_data(ws, to_send, to_send_len);
        if (ctrl_len != payload_len) {
            WS_ERROR("Control frame (opcode=%d) payload read failed (payload_len=%d, read_len=%d)",
                    rx_frame->op, payload_len, ctrl_len);
            ret = -1;
            break;
        }

        if (rx_frame->op == WS_OPCODE_PING) {
            ctrl_len = ws_write(ws, WS_OPCODE_PONG | WS_FIN, WS_MASK, to_send,
                                payload_len);
            if (ctrl_len != payload_len) {
                WS_ERROR("PONG send failed (payload_len=%d, written_len=%d)", payload_len, ctrl_len);
                ret = -1;
                break;
            }
            WS_DEBUG("PONG sent correctly (payload_len=%d)", payload_len);

            rx_frame->hdr_recved = false;

        } else if (rx_frame->op == WS_OPCODE_CLOSE) {
            if (ws_write(ws, WS_OPCODE_CLOSE | WS_FIN, WS_MASK, NULL, 0) < 0) {
                WS_ERROR("Sending CLOSE frame with 0 payload failed");
                ret = -1;
                break;
            }
            WS_DEBUG("CLOSE frame with no payload sent correctly");

            rx_frame->hdr_recved = false;

            ws->state = WS_STATE_CLOSING;
            ws->close_sended = true;
            WS_DEBUG("Connection terminated gracefully");
            ret = 1;
        } else if (rx_frame->op == WS_OPCODE_PONG) {
            WS_DEBUG("Received PONG frame with payload=%d", payload_len);
            rx_frame->hdr_recved = false;
            ws->wait_for_pong_resp = false;
            ret = 2;
        }
    } while(0);

    sys_mfree(to_send);
    return ret > 0 ? 0 : ret;
}


int ws_read(struct ws_session *ws, uint8_t *buf, int len)
{
    int read_len = 0;
    ws_session_event_t event = WS_EVENT_RX_TXT_DATA;
    struct ws_rx_frame *rx_frame = &ws->rx_frame;
    uint8_t ws_hdr[WEBSOCKET_HDR_SIZE];

    if (!rx_frame->hdr_recved) {
        if ( (read_len = ws_read_hdr(ws, &ws_hdr[0], WEBSOCKET_HDR_SIZE)) < 0) {
            rx_frame->remaining = 0;
            // rx_frame->pos = 0;
            return read_len;
        }

        if (rx_frame->hdr_recved && (rx_frame->op & WS_OPCODE_CONTROL_FRAME)) {
            return ws_handle_ctrl(ws);
        }

        // Indicate data to app if it is fin frame
        if (read_len == 0 && rx_frame->fin_frame == false) {
            rx_frame->hdr_recved = false;
            // rx_frame->remaining = 0;
            // rx_frame->pos = 0;
            return 0;
        }
    }

    if (rx_frame->op & WS_OPCODE_TEXT)
        event = WS_EVENT_RX_TXT_DATA;
    else if (rx_frame->op & WS_OPCODE_BINARY)
        event = WS_EVENT_RX_BIN_DATA;

    while (rx_frame->remaining) {
        if ( (read_len = ws_read_data(ws, buf + rx_frame->pos, len - rx_frame->pos)) <= 0) {
            WS_ERROR("Error reading payload data");
            rx_frame->hdr_recved = false;
            rx_frame->remaining = 0;
            return read_len;
        }
        rx_frame->pos = rx_frame->pos + read_len;

        WS_DEBUG("read data %d,  reamin %d\r\n", read_len, rx_frame->remaining);
        // indicate data to APP
        if ((rx_frame->pos >= len && rx_frame->remaining > 0)) {
            WS_DEBUG("Indicate a incompelted rx frame becasue buffer is not enough, payload len %d buf len %d reamin %d, total len %d\r\n",
                rx_frame->payload_len, len, rx_frame->remaining, rx_frame->total_len);
            buf[rx_frame->pos] = '\0';
            ws->ind(ws, event, buf, rx_frame->pos);
            rx_frame->pos = 0;
        }
    }

    if (rx_frame->fin_frame && rx_frame->remaining == 0) {
        buf[rx_frame->pos] = '\0';
        ws->ind(ws, event, buf, rx_frame->pos);
        rx_frame->pos = 0;
        rx_frame->total_len = 0;
        rx_frame->fin_frame = false;
    }

    if (rx_frame->remaining == 0) {
        rx_frame->hdr_recved = false;
    }

    return read_len;
}

int ws_poll_read(int fd, int timeout_ms)
{
    int ret = -1;
    struct timeval timeout;
    fd_set readset;
    fd_set errset;

    FD_ZERO(&readset);
    FD_ZERO(&errset);
    FD_SET(fd, &readset);
    FD_SET(fd, &errset);

    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    ret = select(fd + 1, &readset, NULL, &errset, &timeout);

    if (ret > 0 && FD_ISSET(fd, &errset)) {
        int sock_errno = 0;
        uint32_t optlen = sizeof(sock_errno);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &sock_errno, &optlen);
        WS_ERROR("poll_read select error %d, fd = %d", sock_errno, fd);
        ret = -1;
    }

    return ret;
}

static void ws_session_task(void *arg)
{
    struct ws_session *ws = (struct ws_session *)arg;
    int readable = 0;
    ws->run = true;

    ws->state = WS_STATE_INIT;

    while (ws->run) {
        sys_mutex_get(&ws->lock);
        switch (ws->state) {
            case WS_STATE_INIT:
                if (ws_session_connect(ws) < 0) {
                    WS_ERROR(" net connect failed");
                    ws_net_error_abort(ws);
                    break;
                }
                WS_DEBUG("Connected to %s://%s:%d", ws->conf.scheme, ws->conf.host, ws->conf.port);

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
                    WS_DEBUG("Sending PING...");
                    ws_write(ws, WS_OPCODE_PING | WS_FIN, WS_MASK, NULL, 0);

                    if (!ws->wait_for_pong_resp && ws->conf.pingpong_timeout_sec) {
                        ws->pingpong_tick_ms = sys_current_time_get();
                        ws->wait_for_pong_resp = true;
                    }
                }

                if (sys_current_time_get() - ws->pingpong_tick_ms > ws->conf.pingpong_timeout_sec * 1000 ) {
                    if (ws->wait_for_pong_resp) {
                        WS_ERROR("Error, no PONG received for more than %d seconds after PING", ws->conf.pingpong_timeout_sec);
                        ws_net_error_abort(ws);
                        break;
                    }
                }

                if (readable == 0) {
                    WS_DEBUG("session no data");
                    break;
                }
                ws->ping_tick_ms = sys_current_time_get();

                if (ws_read(ws, ws->rx_buf, ws->rx_buf_size) < 0) {
                    WS_ERROR("read data failed");
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
                    WS_DEBUG("Reconnecting...");
                }
                break;

            case WS_STATE_CLOSING:
                if (!ws->close_sended) {
                    if (ws_write(ws, WS_OPCODE_CLOSE | WS_FIN, WS_MASK, NULL, 0) < 0) {
                        WS_ERROR("send close failed, close it anyway\r\n");
                    }
                    ws->close_sended = true;
                }
                ws->ind(ws, WS_EVENT_DISCONNECT, NULL, 0);
                break;

            default:
                WS_DEBUG("default state: %d", ws->state);
                break;
        }

        sys_mutex_put(&ws->lock);

        if (WS_STATE_CONNECTED == ws->state) {
            readable = ws_poll_read(ws->fd, 1000); //Poll every 1000ms
            if (readable < 0) {
                WS_ERROR("poll read returned %d, errno=%d", readable, errno);
            }
        } else if (WS_STATE_NET_ERROR == ws->state) {
            sys_ms_sleep(ws->wait_timeout_ms);
        }
        else if (WS_STATE_CLOSING == ws->state) {
            if (ws->close_sended) {
                WS_DEBUG("websocket is closed");
                ws->run = false;
                ws->state = WS_STATE_UNKNOW;
            }
            break;
        }
    }

    ws_net_error_abort(ws);
    ws->state = WS_STATE_UNKNOW;
    sys_sema_up(&ws->exit_sem);
    sys_task_delete(NULL);
}

int ws_session_write(struct ws_session *ws, int op, uint8_t *buf, int len, uint32_t timeout_ms)
{
    int ret = 0;
    int send_op;
    int to_len;
    int remain_len = len;
    uint8_t *pos;
    struct timeval timeout, old_timeout;
    socklen_t optlen = sizeof(old_timeout);

    sys_mutex_get(&ws->lock);

    if (!ws->run || ws->state != WS_STATE_CONNECTED) {
        ret = -1;
        goto send_finish;
    }

    if (op == WS_OPCODE_PING && ws->wait_for_pong_resp) {
        printf("Previous PING not responded by PONG, refuse to send another PING\r\n");
        ret = -1;
        goto send_finish;
    }

    if (timeout_ms) {
        getsockopt(ws->fd, SOL_SOCKET, SO_SNDTIMEO, &old_timeout, &optlen);
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(ws->fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    }

    send_op = op;
    pos = buf;

    do {
        if (remain_len > ws->tx_buf_size)
            to_len = ws->tx_buf_size;
        else {
            to_len = remain_len;
            send_op |= WS_FIN;
        }

        if (op == WS_OPCODE_PING || op == WS_OPCODE_PONG || op == WS_OPCODE_CLOSE) {
            if (len > ws->tx_buf_size)
                printf("for control frames, data length exceeds tx_buf_size, and the data has been truncated\r\n");
            send_op |= WS_FIN;
            remain_len = to_len;
        }

        sys_memcpy(ws->tx_buf, pos, to_len);

        if ((ret = ws_write(ws, send_op, WS_MASK, ws->tx_buf, to_len)) < 0) {
            WS_ERROR("ws sesstion send failed");
            ws_net_error_abort(ws);
            break;
        }

        if ((op == WS_OPCODE_PING) && !ws->wait_for_pong_resp && ws->conf.pingpong_timeout_sec) {
            ws->pingpong_tick_ms = sys_current_time_get();
            ws->wait_for_pong_resp = true;
        }

        if (op == WS_OPCODE_CLOSE) {
            ws->close_sended = true;
            ws->state = WS_STATE_CLOSING;
        }

        send_op = 0;
        remain_len = remain_len - ret;
        pos = pos + ret;
    } while (remain_len > 0);

    if (timeout_ms) {
        setsockopt(ws->fd, SOL_SOCKET, SO_SNDTIMEO, &old_timeout, sizeof(old_timeout));
    }

send_finish:
    sys_mutex_put(&ws->lock);
    return  ret < 0 ? ret : len - remain_len;
}

int ws_session_write_txt(struct ws_session *ws, uint8_t *buf, int len)
{
    return ws_session_write(ws, WS_OPCODE_TEXT, buf, len, 0);
}

int ws_session_write_bin(struct ws_session *ws, uint8_t *buf, int len)
{
    return ws_session_write(ws, WS_OPCODE_BINARY, buf, len, 0);
}

int ws_session_write_op(struct ws_session *ws, uint32_t op, uint8_t *buf, int len, uint32_t timeout_ms)
{
    return ws_session_write(ws, op, buf, len, timeout_ms);
}

int ws_session_close(struct ws_session *ws)
{
    uint32_t wait;
    if (ws == NULL)
        return -1;

    wait = ws->wait_timeout_ms;

    sys_mutex_get(&ws->lock);

    do {
        if (!ws->run)
            break;

        if (ws->close_sended)
            break;

        if (ws->state == WS_STATE_CONNECTED) {
            ws->state = WS_STATE_CLOSING;
        } else {
            ws->run = false;
        }
    }while (0);

    sys_mutex_put(&ws->lock);

    while (wait--) {
        if (!ws->run && ws->state == WS_STATE_UNKNOW) {
            break;
        }
    }

    if (ws->exit_sem) {
        sys_sema_down(&ws->exit_sem, 0);
    }

    ws_session_free(ws);
    return 0;
}

struct ws_session *ws_session_init(char *url, int port, char *path, char* origin, int tx_buf_len, int rx_buf_len, ws_event_indicate_fun_t ind)
{

    struct ws_session *ws = (struct ws_session *)sys_malloc(sizeof(struct ws_session));

    if (url == NULL)
        return NULL;

    if(ws == NULL){
        WS_ERROR("malloc session failed");
        return NULL;
    }
    sys_memset(ws, 0, sizeof(struct ws_session));
    ws->task_handle = NULL;
    ws->lock        = NULL;
    ws->exit_sem    = NULL;
    ws->conf.port = port;

    if (origin && strlen(origin) >= sizeof(ws->conf.origin)) {
        WS_ERROR("origin size exceeded");
        sys_mfree(ws);
        return NULL;
    }

    if(origin) {
        sys_memcpy(ws->conf.origin, origin, strlen(origin));
    }

    if (url == NULL) {
        WS_ERROR("url is NULL");
        sys_mfree(ws);
        return NULL;
    }

    if(path)
        sys_memcpy(ws->conf.path, path, strlen(path));

    if (!strncmp(url, "wss://", strlen("wss://"))){
        sys_memcpy(ws->conf.host, (url + strlen("wss://")), (strlen(url) - strlen("wss://")));
        ws->conf.ssl = 1;
        ws->conf.scheme = "wss";
        if(ws->conf.port <= 0)
            ws->conf.port = 443;
    }
    else if(!strncmp(url, "ws://", strlen("ws://"))){
        sys_memcpy(ws->conf.host, (url + strlen("ws://")), (strlen(url) - strlen("ws://")));
        ws->conf.ssl = 0;
        ws->conf.scheme = "ws";
        if(ws->conf.port <= 0)
            ws->conf.port = 80;
    }
    else {
        WS_ERROR("ERROR: Url format is wrong: %s", url);
        sys_mfree(ws);
        return NULL;
    }

    ws->state = WS_STATE_INIT;
    ws->fd = -1;
    // ws->conf.ssl = 0;

    ws->tx_buf = (uint8_t *)sys_malloc(tx_buf_len);
    ws->rx_buf = (uint8_t *)sys_malloc(rx_buf_len);



    if (!ws->tx_buf || !ws->rx_buf) {
        WS_ERROR("ERROR: Malloc tx rx buffer memory fail");
        ws_session_free(ws);
        return NULL;
    }
    ws->tx_buf_size = tx_buf_len;
    ws->rx_buf_size = rx_buf_len;
    sys_memset(ws->tx_buf, 0, tx_buf_len);
    sys_memset(ws->rx_buf, 0, rx_buf_len);

    if(ws_set_net_ops(ws) < 0){
        WS_ERROR("ERROR: Init function failed");
        ws_session_free(ws);
        return NULL;
    }
    ws->wait_timeout_ms = 1000;
    ws->conf.ping_interval_sec = 60;
    ws->conf.pingpong_timeout_sec = 60;
    ws->ping_tick_ms = sys_current_time_get();
    sys_mutex_init(&ws->lock);
    sys_sema_init_ext(&ws->exit_sem, 1, 0);
    ws->ind = ind;
    return ws;
}

int ws_parse_uri(struct ws_session *ws, const char *uri)
{
    if (!ws || !uri)
        return -1;

    char host[128] = {0};
    char path[128] = {0};
    int port = 0;
    bool port_default;

    if (!strncmp(uri, "wss://", strlen("wss://"))) {
        ws->conf.ssl = 1;
        ws->conf.scheme = "wss";
    } else if (!strncmp(uri, "ws://", strlen("ws://"))) {
        ws->conf.ssl = 0;
        ws->conf.scheme = "ws";
    } else {
        WS_ERROR("ERROR: Uri format is wrong: %s\r\n", uri);
        return -1;
    }

    const char *p = strstr(uri, "://");
    p += 3;
    if (*p == '\0') {
        WS_ERROR("ERROR: Uri host is NULL\r\n");
        return -1;
    }
    const char *host_start = p;
    const char *port_start = strchr(host_start, ':');
    const char *path_start = strchr(host_start, '/');

    if (port_start && (!path_start || port_start < path_start)) {
        size_t host_len = port_start - host_start;
        if (host_len >= sizeof(host))
            host_len = sizeof(host) - 1;
        strncpy(host, host_start, host_len);
        host[host_len] = '\0';

        port_start++;
        if (path_start) {
            port = atoi(port_start);
            port_default = 0;
            strncpy(path, path_start, sizeof(path) - 1);
            path[sizeof(path) - 1] = '\0';
        } else {
            port = atoi(port_start);
            port_default = 0;
            path[0] = '\0';
        }
    } else {
        if (path_start) {
            size_t host_len = path_start - host_start;
            if (host_len >= sizeof(host))
                host_len = sizeof(host) - 1;
            strncpy(host, host_start, host_len);
            host[host_len] = '\0';
            strncpy(path, path_start, sizeof(path) - 1);
            path[sizeof(path) - 1] = '\0';
            if (strcmp(ws->conf.scheme, "wss") == 0) {
                port = 443;
                port_default = 1;
            } else {
                port = 80;
                port_default = 1;
            }
        } else {
            strncpy(host, host_start, sizeof(host) - 1);
            host[sizeof(host) - 1] = '\0';
            path[0] = '\0';
            if (strcmp(ws->conf.scheme, "wss") == 0) {
                port = 443;
                port_default = 1;
            } else {
                port = 80;
                port_default = 1;
            }
        }
    }

    strncpy(ws->conf.host, host, sizeof(ws->conf.host) - 1);
    ws->conf.host[sizeof(ws->conf.host) - 1] = '\0';
    strncpy(ws->conf.path, path, sizeof(ws->conf.path) - 1);
    ws->conf.path[sizeof(ws->conf.path) - 1] = '\0';
    ws->conf.port = port;
    ws->conf.port_default = port_default;

    return 0;
}

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
)
{
    if (ws == NULL || *ws == NULL) {
        return -1;
    }

    if (uri == NULL) {
        WS_ERROR("uri is NULL\r\n");
        goto Error;
    }

    if (ws_parse_uri(*ws, uri) < 0) {
        WS_ERROR("parse uri fail\r\n");
        goto Error;
    }

    (*ws)->task_handle = NULL;
    if ((*ws)->lock) {
        sys_mutex_free(&(*ws)->lock);
    }
    (*ws)->lock = NULL;

    if ((*ws)->exit_sem) {
        sys_sema_free(&(*ws)->exit_sem);
    }
    (*ws)->exit_sem = NULL;

    if (origin && strlen(origin) >= sizeof((*ws)->conf.origin)) {
        WS_ERROR("origin size exceeded\r\n");
        goto Error;
    }
    sys_memset((*ws)->conf.origin, 0, sizeof((*ws)->conf.origin));
    if (origin) {
        sys_memcpy((*ws)->conf.origin, origin, strlen(origin));
    }

    if (sub_protocol && strlen(sub_protocol) >= sizeof((*ws)->conf.subprotocol)) {
        WS_ERROR("subprotocol size exceeded\r\n");
        goto Error;
    }
    sys_memset((*ws)->conf.subprotocol, 0, sizeof((*ws)->conf.subprotocol));
    if (sub_protocol) {
        sys_memcpy((*ws)->conf.subprotocol, sub_protocol, strlen(sub_protocol));
    }

    (*ws)->fd = -1;

    if ((*ws)->tx_buf) {
        sys_mfree((*ws)->tx_buf);
    }
    (*ws)->tx_buf = (uint8_t *)sys_malloc(ws_info->tx_buf_size);

    if ((*ws)->rx_buf) {
        sys_mfree((*ws)->rx_buf);
    }
    (*ws)->rx_buf = (uint8_t *)sys_malloc(ws_info->tx_buf_size);

    if (!(*ws)->tx_buf || !(*ws)->rx_buf) {
        WS_ERROR("ERROR: Malloc tx rx buffer memory fail\r\n");
        goto Error;
    }

    if (ws_set_net_ops(*ws) < 0) {
        WS_ERROR("ERROR: Init function failed\r\n");
        goto Error;
    }

    if ((*ws)->conf.headers) {
        sys_mfree((*ws)->conf.headers);
        (*ws)->conf.headers = NULL;
    }
    if (all_headers && ws_session_set_header(*ws, all_headers) < 0) {
        WS_ERROR("set header failed\r\n");
        goto Error;
    }

    if ((*ws)->conf.auth) {
        sys_mfree((*ws)->conf.auth);
        (*ws)->conf.auth = NULL;
    }
    if (auth && ws_session_set_auth(*ws, auth) < 0) {
        WS_ERROR("set auth failed\r\n");
        goto Error;
    }

    (*ws)->tx_buf_size = ws_info->tx_buf_size;
    (*ws)->rx_buf_size = ws_info->tx_buf_size;
    sys_memset((*ws)->tx_buf, 0, (*ws)->tx_buf_size);
    sys_memset((*ws)->rx_buf, 0, (*ws)->rx_buf_size);

    (*ws)->wait_timeout_ms = timeout_ms;
    (*ws)->conf.ping_interval_sec = ws_info->ping_interval_sec;
    (*ws)->conf.pingpong_timeout_sec = ws_info->pingpong_timeout_sec;
    (*ws)->ping_tick_ms = sys_current_time_get();
    (*ws)->reconnect_tick_ms = 0;
    (*ws)->pingpong_tick_ms = 0;
    sys_memset(&(*ws)->rx_frame, 0, sizeof((*ws)->rx_frame));
    (*ws)->close_sended = false;
    (*ws)->run = false;
    (*ws)->wait_for_pong_resp = false;
    (*ws)->tls = NULL;
    sys_mutex_init(&(*ws)->lock);
    sys_sema_init_ext(&(*ws)->exit_sem, 1, 0);
    (*ws)->ind = ind;
    ws_session_set_autoreconnect(*ws, true);
    return 0;

Error:
    if (*ws && (*ws)->tx_buf_size == 0) {
        ws_session_free(*ws);
        *ws = NULL;
    }
    return -1;
}

int ws_session_start(struct ws_session* ws)
{
    if (ws == NULL) {
        return -1;
    }
    if (ws->state > WS_STATE_INIT) {
        WS_ERROR("The client has started");
        return -1;
    }

    ws->task_handle = sys_task_create(NULL, (const uint8_t *)"ws_client", NULL, WS_TASK_STK_SIZE, WS_TASK_QUEUE_SIZE, 0, WS_TASK_PRIO,
                    (task_func_t)ws_session_task, ws);

    if (ws->task_handle == NULL)
        return -1;

    return 0;
}

int ws_session_set_header(struct ws_session *ws, const char * header)
{
    if (ws == NULL || header == NULL)
        return -1;

    if (ws->conf.headers) {
        sys_mfree(ws->conf.headers);
    }
    ws->conf.headers = (char *)sys_malloc(strlen(header) + 1);
    if (ws->conf.headers == NULL) {
        WS_ERROR("malloc header failed\r\n");
        return -1;
    }
    sys_memcpy(ws->conf.headers, header, strlen(header));
    ws->conf.headers[strlen(header)] = '\0';
    return 0;
}

int ws_session_set_autoreconnect(struct ws_session *ws, bool auto_reconnect)
{
    if (ws == NULL)
        return -1;

    ws->auto_reconnect = auto_reconnect;

    return 0;
}

int ws_session_set_auth(struct ws_session *ws, const char *auth)
{
    if (ws == NULL || auth == NULL) {
        return -1;
    }

    if (ws->conf.auth) {
        sys_mfree(ws->conf.auth);
    }
    ws->conf.auth = (char *)sys_malloc(strlen(auth) + 1);
    if (ws->conf.auth == NULL) {
        WS_ERROR("malloc auth failed\r\n");
        return -1;
    }
    sys_memcpy(ws->conf.auth, auth, strlen(auth));
    ws->conf.auth[strlen(auth)] = '\0';
    return 0;
}