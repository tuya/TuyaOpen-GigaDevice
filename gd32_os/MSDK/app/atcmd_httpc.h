/*!
    \file    atcmd_httpc.h
    \brief   AT command HTTPC part for GD32VW55x SDK

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

#ifndef __ATCMD_HTTPC_H__
#define __ATCMD_HTTPC_H__

#define MAX_HTTP_REQ_HEADER_NUM      5
#define MAX_HTTP_REQ_HEADER_LEN      128

#define HTTPC_BUF_SIZE               2048  // default
#define HTTPC_MAX_BUF_SIZE           10240

#define HTTPC_CONN_TIMEOUT           10000  // ms
#define HTTPC_MAX_CONN_TIMEOUT       180000  // ms

#define HTTPC_URL_CFG_MIN_LEN        8
#define HTTPC_URL_CFG_MAX_LEN        2048

typedef enum
{
    RSVF = 0,
    HEAD,
    GET,
    POST,
    PUT,
    DELETE
} http_method_t;

typedef enum
{
    XWFU = 0,   // application/x-www-form-urlencoded
    JSON,       // application/json
    FDATA,      // multipart/form-data
    XML         // text/xml
} http_content_type_t;

typedef enum
{
    OVER_RSVF = 0,
    OVER_TCP,
    OVER_SSL
} http_transport_type_t;


typedef struct
{
    http_method_t method;
    http_content_type_t content_type;
    http_transport_type_t transport_type;

    char *url;
    uint16_t url_len;

    char *host;
    uint16_t host_len;

    char *path;
    uint16_t path_len;

    char *data;
    uint16_t data_len;

    uint8_t atcmd_idx;

    uint8_t header_cnt;
    char *headers[MAX_HTTP_REQ_HEADER_NUM];
    uint16_t headers_len[MAX_HTTP_REQ_HEADER_NUM];

    // option args for AT+HTTPGETSIZE
    uint16_t tx_size;
    uint16_t rx_size;
    uint32_t timeout;
} httpc_req_info_t;

typedef struct {
    char *httpc_url;
    uint16_t url_len;
} http_url_info_t;

typedef struct {
    uint8_t header_cnt;
    char *headers[MAX_HTTP_REQ_HEADER_NUM];
    uint16_t headers_len[MAX_HTTP_REQ_HEADER_NUM];
} http_header_info_t;

void at_httpc_req_send(int argc, char **argv);
void at_httpc_getsize(int argc, char **argv);
void at_httpc_get(int argc, char **argv);
void at_httpc_post(int argc, char **argv);
void at_httpc_put(int argc, char **argv);
void at_httpc_url_cfg(int argc, char **argv);
void at_httpc_head_cfg(int argc, char **argv);

#endif  /* __ATCMD_TCPIP_H__ */
