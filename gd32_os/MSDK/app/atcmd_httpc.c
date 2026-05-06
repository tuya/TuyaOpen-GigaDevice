/*!
    \file    atcmd_httpc.c
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

#include "lwip/apps/http_client.h"
#include "lwip/altcp_tls.h"

#ifdef CONFIG_ATCMD_HTTP_CLIENT

static char *content_type_str[5] = {
    "application/x-www-form-urlencoded",
    "application/json",
    "multipart/form-data",
    "text/xml",
    NULL
};

static char *method_str[7] = {
    "RSVF",
    "HEAD",
    "GET",
    "POST",
    "PUT",
    "DELETE",
    NULL
};

static char *httpc_atcmd[8] = {
    "HTTPCLIENT",
    "HTTPGETSIZE",
    "HTTPCGET",
    "HTTPCPOST",
    "HTTPCPUT",
    "HTTPURLCFG",
    "HTTPCHEAD",
    NULL
};

static http_url_info_t url_info = {NULL, 0};
static http_header_info_t req_header;

#if LWIP_ALTCP
static altcp_allocator_t httpc_tls_allocator;
static struct altcp_tls_config *httpc_tls_config = NULL;
#endif

static httpc_connection_t httpc_settings;
static httpc_state_t *httpc_connection = NULL;
static uint32_t http_content_len, http_received_len;

static uint8_t httpc_cmd_done = 1;

/**
 * parse HTTP URL to get host and path
 */
int parse_http_url(const char *url, char **host, char **path) {
    const char *p = url;
    const char *host_start, *host_end, *path_start;
    size_t host_len, path_len;

    // skip (http:// or https://)
    if (strncmp(p, "http://", 7) == 0) {
        p += 7;
    } else if (strncmp(p, "https://", 8) == 0) {
        p += 8;
    } else {
        return -1; // not HTTP protocol
    }

    host_start = p;

    while (*p && *p != '/' && *p != ':') {
        p++;
    }
    host_end = p;

    if (*p == ':') {
        while (*p && *p != '/') {
            p++;
        }
        host_end = p;
    }

    host_len = host_end - host_start;
    *host = (char*)sys_malloc(host_len + 1);
    if (!*host) return -1;
    memcpy(*host, host_start, host_len);
    (*host)[host_len] = '\0';

    if (*p == '\0' || *p != '/') {
        *path = (char*)sys_malloc(2);
        if (!*path) {
            sys_mfree(*host);
            return -1;
        }
        strcpy(*path, "/");
    } else {
        path_start = p;
        path_len = strlen(path_start);
        *path = (char*)sys_malloc(path_len + 1);
        if (!*path) {
            sys_mfree(*host);
            return -1;
        }
        memcpy(*path, path_start, path_len);
        (*path)[path_len] = '\0';
    }

    return 0;
}

static void httpc_free_req_info(httpc_req_info_t *req_info)
{
    if (req_info->url != NULL)
        sys_mfree(req_info->url);

    if (req_info->host != NULL)
        sys_mfree(req_info->host);

    if (req_info->path != NULL)
        sys_mfree(req_info->path);

    if (req_info->data != NULL)
        sys_mfree(req_info->data);

    for (int i = 0; i < MAX_HTTP_REQ_HEADER_NUM; i++) {
        if (req_info->headers[i] != NULL)
            sys_mfree(req_info->headers[i]);
    }

    sys_memset(req_info, 0, sizeof(httpc_req_info_t));
}

static int compare_before_colon(const char *str1, const char *str2)
{
    if (!str1 || !str2) {
        return 0;
    }

    while (*str1 && *str2 && *str1 != ':' && *str2 != ':') {
        if (*str1 != *str2) {
            return 0;
        }
        str1++;
        str2++;
    }

    return ((*str1 == ':') && (*str2 == ':'));
}


/**
 * parse http client request args
 * return 0 success，-1 fail
 */
static int httpc_parse_req_args(int argc, char **argv, httpc_req_info_t *req_info)
{
    char *url = NULL;
    char *host = NULL;
    char *path = NULL;
    char *data = NULL;
    char *endptr = NULL;
    char *req_hdr = NULL;
    uint16_t req_hdr_len;
    uint8_t idx, i;
    uint8_t req_hdr_start_idx;

    AT_RSP_START(256);
    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            goto Error;
        }
    } else if (argc >= 7) {
        req_info->method = (http_method_t)strtoul((const char *)argv[1], &endptr, 10);
        if (req_info->method > DELETE) {
            AT_TRACE("method error\r\n");
            goto Error;
        }
        req_info->content_type = (http_content_type_t)strtoul((const char *)argv[2], &endptr, 10);
        if (req_info->content_type > XML) {
            AT_TRACE("content_type error\r\n");
            goto Error;
        }

        url = at_string_parse(argv[3]);
        req_info->url_len = (url != NULL ? strlen(url) : 0);

        if (url == NULL) {
            if (url_info.httpc_url != NULL) {
                url = url_info.httpc_url;
                req_info->url_len = url_info.url_len;
            }
        }

        host = at_string_parse(argv[4]);
        req_info->host_len = (host != NULL ? strlen(host) : 0);

        path = at_string_parse(argv[5]);
        req_info->path_len = (path != NULL ? strlen(path) : 0);

        if (req_info->url_len > 0) {
            req_info->url = (char *)sys_malloc(req_info->url_len + 1);
            if (req_info->url == NULL) {
                AT_TRACE("sys_malloc for url fail\r\n");
                goto Error;
            }
            strcpy(req_info->url, url);

            // if url is not empty, parse the url and get host and path
            if (parse_http_url(url, &req_info->host, &req_info->path) != 0) {
                AT_TRACE("invalid url\r\n");
                goto Error;
            }
            req_info->host_len = strlen(req_info->host);
            req_info->path_len = strlen(req_info->path);
        } else if (req_info->host_len > 0) {
            req_info->host = (char *)sys_malloc(req_info->host_len + 1);
            if (req_info->host == NULL) {
                AT_TRACE("sys_malloc for host fail\r\n");
                goto Error;
            }
            strcpy(req_info->host, host);

            if (req_info->path_len > 0) {
                req_info->path = (char *)sys_malloc(req_info->path_len + 1);
                if (req_info->path == NULL) {
                    AT_TRACE("sys_malloc for path fail\r\n");
                    goto Error;
                }
                strcpy(req_info->path, path);
            }
        } else {
            AT_TRACE("both url and host are empty\r\n");
            goto Error;
        }

        req_info->transport_type = (http_transport_type_t)strtoul((const char *)argv[6], &endptr, 10);
        if (req_info->transport_type > OVER_SSL) {
            AT_TRACE("transport_type error\r\n");
            goto Error;
        }

        // url is empty, generate url acoording host and path
        if (req_info->url_len == 0) {
            uint16_t max_url_len = req_info->host_len + req_info->path_len + 8 + 1;
            req_info->url = (char *)sys_malloc(max_url_len);
            if (req_info->url == NULL) {
                AT_TRACE("sys_malloc for url fail\r\n");
                goto Error;
            }
            if (req_info->transport_type == OVER_SSL)
                req_info->url_len = (uint16_t)co_snprintf(req_info->url, max_url_len, "https://%s%s", req_info->host, req_info->path);
            else
                req_info->url_len = (uint16_t)co_snprintf(req_info->url, max_url_len, "http://%s%s", req_info->host, req_info->path);
        }

        // only POST method exist data arg
        if (req_info->method == POST) {
            data = at_string_parse(argv[7]);
            req_info->data_len = strlen(data);
            if (req_info->data_len == 0) {
                AT_TRACE("POST method but data is empty\r\n");
                goto Error;
            }

            req_info->data = (char *)sys_malloc(req_info->data_len + 1);
            if (req_info->data == NULL) {
                AT_TRACE("sys_malloc for data fail\r\n");
                goto Error;
            }
            strcpy(req_info->data, data);
        }

        req_hdr_start_idx = (req_info->method == POST) ? 8 : 7;

        // parse http_req_header
        if (argc > req_hdr_start_idx + 1) {
            for (int idx = req_hdr_start_idx, i = 0; idx < argc && i < MAX_HTTP_REQ_HEADER_NUM; idx++, i++) {
                req_hdr = at_string_parse(argv[idx]);
                req_hdr_len = (req_hdr != NULL ? strlen(req_hdr) : 0);
                if (req_hdr_len > MAX_HTTP_REQ_HEADER_LEN) {
                    AT_TRACE("the input request header is too long %u\r\n", req_hdr_len);
                    goto Error;
                }

                if (req_hdr_len == 0) {
                    AT_TRACE("contains http request header, but it is empty\r\n");
                    goto Error;
                }

                if (strchr(req_hdr, ':') == NULL) {
                    AT_TRACE("invalid header format\r\n");
                    goto Error;
                }

                req_info->headers[i] = (char *)sys_malloc(req_hdr_len + 1);
                if (req_info->headers[i] == NULL) {
                    AT_TRACE("sys_malloc for http_req_header fail\r\n");
                    goto Error;
                }
                strcpy(req_info->headers[i], req_hdr);
                req_info->headers_len[i] = req_hdr_len;
                req_info->header_cnt++;
            }
        }
    } else {
        goto Error;
    }

    AT_RSP_FREE();
    return 0;

Error:
    httpc_free_req_info(req_info);
    AT_RSP_ERR();
    return -1;

Usage:
    AT_RSP("+HTTPCLIENT=<method>,<content-type>,<\"url\">,[<\"host\">],[<\"path\">],<transport_type>[,<\"data\">][,<\"http_req_header\">][,<\"http_req_header\">][...]\r\n");
    AT_RSP_OK();
    return 0;
}

// Transmission Result Callback
static void httpc_result_cb(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err)
{
    httpc_connection_t *conn_settings = (httpc_connection_t *)arg;

    if (httpc_result == HTTPC_RESULT_OK) {

        if ((http_content_len == 0) && ((conn_settings->atcmd_idx == 0) || (conn_settings->atcmd_idx == 2))) { // HTTPCLIENT or HTTPCGET
            AT_RSP_START(16);
            AT_RSP("\r\n");
            AT_RSP_OK();
        }

        AT_TRACE("Transfer completed!\n");
    } else {
        AT_TRACE("Transfer failed: %d\n", httpc_result);

        if ((conn_settings->atcmd_idx == 3) || (conn_settings->atcmd_idx == 4)) {   // HTTPCPOST or HTTPCPUT
            AT_RSP_START(64);
            AT_RSP("SEND FAIL\r\n");
            AT_RSP_DIRECT(rsp_buf, rsp_buf_idx);
            sys_mfree(rsp_buf);
        } else {
            AT_RSP_START(64);
            AT_RSP("+%s:\r\n", httpc_atcmd[conn_settings->atcmd_idx]);
            AT_RSP_FAIL();
        }
    }

    http_received_len = 0;
    http_content_len = 0;

    if (conn_settings->req_buffer != NULL) {
        sys_mfree(conn_settings->req_buffer);
        conn_settings->req_buffer = NULL;
    }
#if LWIP_ALTCP
    if (httpc_tls_config != NULL) {
        altcp_tls_free_config(httpc_tls_config);
        httpc_tls_config = NULL;
        altcp_tls_free_entropy();
    }
#endif

    httpc_cmd_done = 1;
}

// callback function when http header receiced complete
static err_t httpc_headers_done_cb(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len)
{
    err_t ret;
    httpc_connection_t *conn_settings = (httpc_connection_t *)arg;

    // 处理 Content-Length 未知的情况（chunked transfer 或无 Content-Length）
    if (content_len == 0xFFFFFFFF) {
        http_content_len = 0;  // 表示长度未知
        AT_TRACE("Content-Length unknown (chunked or not specified)\n");
    } else {
        http_content_len = content_len;
    }

    char at_rsp_hdr[64] = {0};
    int at_rsp_hdr_len;

    // check status code
    if (hdr != NULL && hdr->payload != NULL && strstr((char*)hdr->payload, "200 OK") != NULL) {
        AT_TRACE("recv header success!\n\n");
        ret = ERR_OK;
    } else {
        AT_TRACE("recv header fail\n\n");
        ret = ERR_ABRT;
    }

    if ((conn_settings->atcmd_idx == 1) || (conn_settings->atcmd_idx == 0 && conn_settings->method == HEAD)) { // HTTPGETSIZE or HTTPCLIENT HEAD
        AT_RSP_START(64);
        if (ret == ERR_OK) {
            AT_RSP("+%s:%u\r\n", httpc_atcmd[conn_settings->atcmd_idx], http_content_len);
            AT_RSP_OK();
        } else {
            AT_RSP("+%s:%u\r\n", httpc_atcmd[conn_settings->atcmd_idx], http_content_len);
            AT_RSP_FAIL();
        }
    } else if ((conn_settings->atcmd_idx == 0) || (conn_settings->atcmd_idx == 2)) { // HTTPCLIENT or HTTPCGET
        if (ret == ERR_OK) {
            at_rsp_hdr_len = co_snprintf(at_rsp_hdr, sizeof(at_rsp_hdr), "+%s:%u,", httpc_atcmd[conn_settings->atcmd_idx], http_content_len);
            at_hw_send(at_rsp_hdr, at_rsp_hdr_len);
        } else {
            AT_RSP_START(64);
            AT_RSP("+%s:%u\r\n", httpc_atcmd[conn_settings->atcmd_idx], http_content_len);
            AT_RSP_FAIL();
        }
    }

    if ((conn_settings->atcmd_idx == 3) || (conn_settings->atcmd_idx == 4)) {   // HTTPCPOST or HTTPCPUT
        char *rsp = (ret == ERR_OK ? "SEND OK" : "SEND FAIL");
        AT_RSP_START(32);
        AT_RSP("%s\r\n", rsp);
        AT_RSP_DIRECT(rsp_buf, rsp_buf_idx);
        sys_mfree(rsp_buf);
    }

    return ret;
}

// callback function for data receive
static err_t httpc_altcp_recv_cb(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err)
{
    struct pbuf *q = NULL;
    httpc_connection_t *conn_settings = (httpc_connection_t *)arg;

    AT_TRACE("Enter %s =========================p->tot_len=%u\n", __func__, p->tot_len);

    if (p != NULL) {
        http_received_len += p->tot_len;

        for (q = p; q != NULL; q = q->next) {
            if ((conn_settings->atcmd_idx == 0) || (conn_settings->atcmd_idx == 2))
                at_hw_send((char *)q->payload, q->len);
        }

        // update recv window
        altcp_recved(conn, p->tot_len);
        pbuf_free(p);

        // printing progress
        if (http_content_len > 0) {
            AT_TRACE("Progress: %.1f%%\r", (http_received_len * 100.0) / http_content_len);
        } else {
            // Content-Length 未知，只显示已接收字节数
            AT_TRACE("Received: %u bytes\r", http_received_len);
        }

        if (http_content_len > 0 && http_received_len >= http_content_len) {
            AT_TRACE("\nreceived completed.\n");
            http_received_len = 0;

            if ((conn_settings->atcmd_idx == 0) || (conn_settings->atcmd_idx == 2)) {
                AT_RSP_START(16);
                AT_RSP("\r\n");
                AT_RSP_OK();
            }
        }
    }

    return ERR_OK;
}

static int httpc_create_request_content(httpc_req_info_t *req_info, char *buffer, size_t buffer_size)
{
    int i, j;
    int offset = 0;

    offset = co_snprintf(buffer, buffer_size, "%s %s HTTP/1.1\r\n" "Host: %s\r\n",
        method_str[req_info->method], req_info->path, req_info->host);

    for (i = 0; i < req_info->header_cnt; i++) {
        if (req_info->headers[i] == NULL)
            continue;

        for (j = 0; j < req_header.header_cnt; j++) {
            if (req_header.headers[j] == NULL)
                continue;

            if (compare_before_colon((const char *)req_info->headers[i], (const char *)req_header.headers[j]))
                break;
        }
        if (j >= req_header.header_cnt)
            offset += co_snprintf(buffer + offset, buffer_size - offset, "%s\r\n", req_info->headers[i]);
    }

    for (int j = 0; j < req_header.header_cnt; j++) {
        if (req_header.headers[j] == NULL)
            continue;

        offset += co_snprintf(buffer + offset, buffer_size - offset, "%s\r\n", req_header.headers[j]);
    }

    if (req_info->method == POST) {
        if (strstr(buffer, "Content-Type:") == NULL)
            offset += co_snprintf(buffer + offset, buffer_size - offset, "Content-Type: %s\r\n" "Content-Length: %d\r\n",
                content_type_str[req_info->content_type], req_info->data_len);
    }

    if (strstr(buffer, "Connection:") == NULL)
        offset += co_snprintf(buffer + offset, buffer_size - offset, "Connection: Close\r\n" "\r\n");

    if (req_info->method == POST)
        offset += co_snprintf(buffer + offset, buffer_size - offset, "%s", req_info->data);

    return offset;
}

static int httpc_connection_init(httpc_connection_t *settings, httpc_req_info_t *req_info)
{
    int req_str_len;
    char *buffer = NULL;

#if LWIP_ALTCP
    if (req_info->transport_type == OVER_SSL) {
        // If http server requires cert authenticate, http client should get ca cert
        httpc_tls_config = altcp_tls_create_config_client(NULL, 0);
        if (httpc_tls_config == NULL) {
            return -1;
        }
        httpc_tls_allocator.alloc = (altcp_new_fn)altcp_tls_alloc;
        httpc_tls_allocator.arg = httpc_tls_config;

        settings->altcp_allocator = &httpc_tls_allocator;
    } else {
        // if alloc is NULL, will call altcp_tcp_new_ip_type to create simple TCP connection
        settings->altcp_allocator = NULL;
    }
#endif

    settings->atcmd_idx = req_info->atcmd_idx;
    settings->method = req_info->method;
    settings->timeout = req_info->timeout;
    settings->result_fn = httpc_result_cb;
    settings->headers_done_fn = httpc_headers_done_cb;

    // create httpc request
    buffer = (char *)sys_malloc(req_info->tx_size);
    if (buffer == NULL) {
        AT_TRACE("sys_malloc buffer fail!\r\n");
        return -1;
    }

    req_str_len = httpc_create_request_content(req_info, buffer, req_info->tx_size);

    settings->req_buffer = (uint8_t *)sys_malloc(req_str_len + 1);
    strcpy((char *)settings->req_buffer, buffer);

    sys_mfree(buffer);
    return 0;
}

void at_httpc_req_send(int argc, char **argv)
{
    int ret;
    err_t err;
    uint16_t server_port;
    httpc_req_info_t req_info;


    if (!httpc_cmd_done) {
        AT_TRACE("Previous HTTP command is still in progress.\r\n");
        return;
    }
    httpc_cmd_done = 0;

    sys_memset(&req_info, 0, sizeof(httpc_req_info_t));

    // init default value
    req_info.tx_size = HTTPC_BUF_SIZE;
    req_info.rx_size = HTTPC_BUF_SIZE;
    req_info.timeout = HTTPC_CONN_TIMEOUT;
    req_info.atcmd_idx = 0;

    ret = httpc_parse_req_args(argc, argv, &req_info);
    if (ret < 0) {
        AT_TRACE("parse http client request args fail! ret = %d\r\n", ret);
        goto END;
    }

    if ((ret == 0) && (argc == 2)) {
        goto END;
    }

    // init http client connection
    sys_memset(&httpc_settings, 0, sizeof(httpc_connection_t));
    ret = httpc_connection_init(&httpc_settings, &req_info);
    if (ret < 0) {
        AT_TRACE("init http client connection fail! ret = %d\r\n", ret);
        goto END;
    }

    server_port = (req_info.transport_type == OVER_SSL) ? 443 : 80;

    AT_TRACE("%s:: req_info.url: %s\n\n", __func__, req_info.url);

    err = httpc_send_request(req_info.host, server_port, req_info.url, &httpc_settings,
                             httpc_altcp_recv_cb, &httpc_settings, &httpc_connection);

    if (err != ERR_OK) {
        AT_TRACE("httpc_send_request failed: %d\n", err);
        goto END;
    }

    httpc_free_req_info(&req_info);
    return;

END:
    httpc_free_req_info(&req_info);
    httpc_cmd_done = 1;
}

static int httpc_parse_getsize_args(int argc, char **argv, httpc_req_info_t *req_info)
{
    char *url = NULL;
    char *host = NULL;
    char *path = NULL;
    char *endptr = NULL;

    AT_RSP_START(256);

    if ((argc < 2) || (argc > 5))
        goto Error;

    if ((argc == 2) && (argv[1][0] == AT_QUESTION))
        goto Usage;

    url = at_string_parse(argv[1]);
    req_info->url_len = (url != NULL ? strlen(url) : 0);

    if (req_info->url_len == 0) {
        if (url_info.httpc_url != NULL) {
            url = url_info.httpc_url;
            req_info->url_len = url_info.url_len;
        }
    }

    if (req_info->url_len > 0) {
        req_info->url = (char *)sys_malloc(req_info->url_len + 1);
        if (req_info->url == NULL) {
            AT_TRACE("sys_malloc for url fail\r\n");
            goto Error;
        }
        strcpy(req_info->url, url);
    } else {
        goto Error;
    }

    if (parse_http_url(url, &req_info->host, &req_info->path) != 0) {
        AT_TRACE("invalid url\r\n");
        goto Error;
    }
    req_info->host_len = strlen(req_info->host);
    req_info->path_len = strlen(req_info->path);

    if (strstr(req_info->url, "https://") != NULL)
        req_info->transport_type = OVER_SSL;
    else if (strstr(req_info->url, "http://") != NULL)
        req_info->transport_type = OVER_TCP;
    else
        goto Error;

    if (argc >= 3) {
        req_info->tx_size = (uint16_t)strtoul((const char *)argv[2], &endptr, 10);
        AT_TRACE("%s: req_info->tx_size=%u\n", __func__, req_info->tx_size);
    }
    if ((req_info->tx_size == 0) || (req_info->tx_size > HTTPC_MAX_BUF_SIZE)) {
        AT_TRACE("invalid tx size %u\r\n", req_info->tx_size);
        req_info->tx_size = HTTPC_BUF_SIZE;
    }

    if (argc >= 4) {
        req_info->rx_size = (uint16_t)strtoul((const char *)argv[3], &endptr, 10);
        AT_TRACE("%s: req_info->rx_size=%u\n", __func__, req_info->rx_size);
    }
    if ((req_info->rx_size == 0) || (req_info->rx_size > HTTPC_MAX_BUF_SIZE)) {
        AT_TRACE("invalid rx size %u\r\n", req_info->rx_size);
        req_info->rx_size = HTTPC_BUF_SIZE;
    }

    if (argc == 5) {
        req_info->timeout = (uint32_t)strtoul((const char *)argv[4], &endptr, 10);
        AT_TRACE("%s: req_info->timeout=%u\n", __func__, req_info->timeout);
    }
    if ((req_info->timeout == 0) || (req_info->timeout > HTTPC_MAX_CONN_TIMEOUT)) {
        AT_TRACE("invalid timeout %u\r\n", req_info->timeout);
        req_info->timeout = HTTPC_CONN_TIMEOUT;
    }

    AT_RSP_FREE();
    return 0;

Error:
    httpc_free_req_info(req_info);
    AT_RSP_ERR();
    return -1;

Usage:
    if (strcmp(argv[0], "AT+HTTPGETSIZE") == 0) {
        AT_RSP("+HTTPGETSIZE=<\"url\">[,<tx size>][,<rx size>][,<timeout>]\r\n");
    } else if (strcmp(argv[0], "AT+HTTPCGET") == 0) {
        AT_RSP("+HTTPCGET=<\"url\">[,<tx size>][,<rx size>][,<timeout>]\r\n");
    }
    AT_RSP_OK();
    return 0;
}

void at_httpc_getsize(int argc, char **argv)
{
    int ret;
    err_t err;
    uint16_t server_port;
    httpc_req_info_t req_info;

    if (!httpc_cmd_done) {
        AT_TRACE("Previous HTTP command is still in progress.\r\n");
        return;
    }
    httpc_cmd_done = 0;

    sys_memset(&req_info, 0, sizeof(httpc_req_info_t));

    req_info.method = HEAD;
    req_info.atcmd_idx = 1;

    ret = httpc_parse_getsize_args(argc, argv, &req_info);
    if (ret < 0) {
        AT_TRACE("parse http client getsize args fail! ret = %d\r\n", ret);
        goto END;
    }

    if ((ret == 0) && (argc == 2) && (argv[1][0] == AT_QUESTION)) {
        goto END;
    }

    // init http client connection
    sys_memset(&httpc_settings, 0, sizeof(httpc_connection_t));
    ret = httpc_connection_init(&httpc_settings, &req_info);
    if (ret < 0) {
        AT_TRACE("init http client connection fail! ret = %d\r\n", ret);
        goto END;
    }

    server_port = (req_info.transport_type == OVER_SSL) ? 443 : 80;

    AT_TRACE("%s:: req_info.url: %s\n\n", __func__, req_info.url);

    err = httpc_send_request(req_info.host, server_port, req_info.url, &httpc_settings,
                             httpc_altcp_recv_cb, &httpc_settings, &httpc_connection);

    if (err != ERR_OK) {
        AT_TRACE("httpc_send_request failed: %d\n", err);
        goto END;
    }

    httpc_free_req_info(&req_info);
    return;

END:
    httpc_free_req_info(&req_info);
    httpc_cmd_done = 1;
}

void at_httpc_get(int argc, char **argv)
{
    int ret;
    err_t err;
    uint16_t server_port;
    httpc_req_info_t req_info;

    if (!httpc_cmd_done) {
        AT_TRACE("Previous HTTP command is still in progress.\r\n");
        return;
    }
    httpc_cmd_done = 0;

    sys_memset(&req_info, 0, sizeof(httpc_req_info_t));

    req_info.method = GET;
    req_info.atcmd_idx = 2;

    ret = httpc_parse_getsize_args(argc, argv, &req_info);
    if (ret < 0) {
        AT_TRACE("parse http client get args fail! ret = %d\r\n", ret);
        goto END;
    }

    if ((ret == 0) && (argc == 2) && (argv[1][0] == AT_QUESTION)) {
        goto END;
    }

    // init http client connection
    sys_memset(&httpc_settings, 0, sizeof(httpc_connection_t));
    ret = httpc_connection_init(&httpc_settings, &req_info);
    if (ret < 0) {
        AT_TRACE("init http client connection fail! ret = %d\r\n", ret);
        goto END;
    }

    server_port = (req_info.transport_type == OVER_SSL) ? 443 : 80;

    AT_TRACE("%s:: req_info.url: %s\n\n", __func__, req_info.url);

    err = httpc_send_request(req_info.host, server_port, req_info.url, &httpc_settings,
                             httpc_altcp_recv_cb, &httpc_settings, &httpc_connection);

    if (err != ERR_OK) {
        AT_TRACE("httpc_send_request failed: %d\n", err);
        goto END;
    }

    httpc_free_req_info(&req_info);
    return;

END:
    httpc_free_req_info(&req_info);
    httpc_cmd_done = 1;
}

static int httpc_parse_post_args(int argc, char **argv, httpc_req_info_t *req_info)
{
    char *url = NULL;
    char *endptr = NULL;
    char *req_hdr = NULL;
    uint16_t req_hdr_len;
    uint8_t idx, i;

    AT_RSP_START(256);
    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            goto Error;
        }
    } else if (argc >= 3) {
        url = at_string_parse(argv[1]);
        req_info->url_len = (url != NULL ? strlen(url) : 0);
        if (req_info->url_len == 0) {
            if (url_info.httpc_url != NULL) {
                url = url_info.httpc_url;
                req_info->url_len = url_info.url_len;
            }
        }

        if (req_info->url_len > 0) {
            req_info->url = (char *)sys_malloc(req_info->url_len + 1);
            if (req_info->url == NULL) {
                AT_TRACE("sys_malloc for url fail\r\n");
                goto Error;
            }
            strcpy(req_info->url, url);
        } else {
            AT_TRACE("url is empty\r\n");
            goto Error;
        }

        if (parse_http_url(url, &req_info->host, &req_info->path) != 0) {
            AT_TRACE("invalid url\r\n");
            goto Error;
        }
        req_info->host_len = strlen(req_info->host);
        req_info->path_len = strlen(req_info->path);

        if (strstr(req_info->url, "https://") != NULL)
            req_info->transport_type = OVER_SSL;
        else if (strstr(req_info->url, "http://") != NULL)
            req_info->transport_type = OVER_TCP;
        else
            goto Error;

        req_info->data_len = (uint16_t)strtoul((const char *)argv[2], &endptr, 10);
        if (req_info->data_len == 0) {
            AT_TRACE("post date length is zero\r\n");
            goto Error;
        }

        req_info->data = (char *)sys_malloc(req_info->data_len + 1);
        if (req_info->data == NULL) {
            AT_TRACE("sys_malloc for http post data fail\r\n");
            goto Error;
        }

        if (argc == 3) {
            goto Input;
        }

        if (argc >= 4) {
            req_info->header_cnt = (uint8_t)strtoul((const char *)argv[3], &endptr, 10);
            if (req_info->header_cnt == 0) {
                AT_TRACE("post date length is zero\r\n");
                goto Input;
            }

            for (int idx = 4, i = 0; idx < argc && i < MAX_HTTP_REQ_HEADER_NUM && i < req_info->header_cnt; idx++, i++) {
                req_hdr = at_string_parse(argv[idx]);
                req_hdr_len = (req_hdr != NULL ? strlen(req_hdr) : 0);

                if (req_hdr_len > MAX_HTTP_REQ_HEADER_LEN) {
                    AT_TRACE("the input request header is too long %u\r\n", req_hdr_len);
                    goto Error;
                }

                if (req_hdr_len == 0) {
                    AT_TRACE("contains http request header, but it is empty\r\n");
                    goto Error;
                }

                if (strchr(req_hdr, ':') == NULL) {
                    AT_TRACE("invalid header format\r\n");
                    goto Error;
                }

                req_info->headers[i] = (char *)sys_malloc(req_hdr_len + 1);
                if (req_info->headers[i] == NULL) {
                    AT_TRACE("sys_malloc for http post header fail\r\n");
                    goto Error;
                }
                strcpy(req_info->headers[i], req_hdr);
                req_info->headers_len[i] = req_hdr_len;
            }
        }
    } else {
        goto Error;
    }

Input:
    AT_RSP("OK\r\n");
    AT_RSP(">\r\n");
    AT_RSP_DIRECT(rsp_buf, rsp_buf_idx);
    sys_mfree(rsp_buf);
    at_hw_dma_receive((uint32_t)req_info->data, req_info->data_len);

    return 0;

Error:
    httpc_free_req_info(req_info);
    AT_RSP_ERR();
    return -1;

Usage:
    if (strcmp(argv[0], "AT+HTTPCPOST") == 0) {
        AT_RSP("+HTTPCPOST=<\"url\">,<length>[,<http_req_header_cnt>][,<http_req_header>..<http_req_header>]\r\n");
    } else if (strcmp(argv[0], "AT+HTTPCPUT") == 0) {
        AT_RSP("+HTTPCPUT=<\"url\">,<length>[,<http_req_header_cnt>][,<http_req_header>..<http_req_header>]\r\n");
    }
    AT_RSP_OK();
    return 0;
}

void at_httpc_post(int argc, char **argv)
{
    int ret;
    err_t err;
    uint16_t server_port;
    httpc_req_info_t req_info;

    if (!httpc_cmd_done) {
        AT_TRACE("Previous HTTP command is still in progress.\r\n");
        return;
    }
    httpc_cmd_done = 0;

    sys_memset(&req_info, 0, sizeof(httpc_req_info_t));

    req_info.method = POST;
    req_info.atcmd_idx = 3;
    req_info.tx_size = HTTPC_BUF_SIZE;
    req_info.rx_size = HTTPC_BUF_SIZE;
    req_info.timeout = HTTPC_CONN_TIMEOUT;

    ret = httpc_parse_post_args(argc, argv, &req_info);
    if (ret < 0) {
        AT_TRACE("parse http client post args fail! ret = %d\r\n", ret);
        goto END;
    }

    if ((ret == 0) && (argc == 2) && (argv[1][0] == AT_QUESTION)) {
        goto END;
    }

    // init http client connection
    sys_memset(&httpc_settings, 0, sizeof(httpc_connection_t));
    ret = httpc_connection_init(&httpc_settings, &req_info);
    if (ret < 0) {
        AT_TRACE("init http client connection fail! ret = %d\r\n", ret);
        goto END;
    }

    server_port = (req_info.transport_type == OVER_SSL) ? 443 : 80;

    AT_TRACE("%s:: req_info.url: %s\n\n", __func__, req_info.url);

    err = httpc_send_request(req_info.host, server_port, req_info.url, &httpc_settings,
                             httpc_altcp_recv_cb, &httpc_settings, &httpc_connection);

    if (err != ERR_OK) {
        AT_TRACE("httpc_send_request failed: %d\n", err);
        goto END;
    }

    httpc_free_req_info(&req_info);
    return;

END:
    httpc_free_req_info(&req_info);
    httpc_cmd_done = 1;
}

void at_httpc_put(int argc, char **argv)
{
    int ret;
    err_t err;
    uint16_t server_port;
    httpc_req_info_t req_info;

    if (!httpc_cmd_done) {
        AT_TRACE("Previous HTTP command is still in progress.\r\n");
        return;
    }
    httpc_cmd_done = 0;

    sys_memset(&req_info, 0, sizeof(httpc_req_info_t));

    req_info.method = PUT;
    req_info.atcmd_idx = 4;
    req_info.tx_size = HTTPC_BUF_SIZE;
    req_info.rx_size = HTTPC_BUF_SIZE;
    req_info.timeout = HTTPC_CONN_TIMEOUT;

    ret = httpc_parse_post_args(argc, argv, &req_info);
    if (ret < 0) {
        AT_TRACE("parse http client post args fail! ret = %d\r\n", ret);
        goto END;
    }

    if ((ret == 0) && (argc == 2) && (argv[1][0] == AT_QUESTION)) {
        goto END;
    }

    // init http client connection
    sys_memset(&httpc_settings, 0, sizeof(httpc_connection_t));
    ret = httpc_connection_init(&httpc_settings, &req_info);
    if (ret < 0) {
        AT_TRACE("init http client connection fail! ret = %d\r\n", ret);
        goto END;
    }

    server_port = (req_info.transport_type == OVER_SSL) ? 443 : 80;

    AT_TRACE("%s:: req_info.url: %s\n\n", __func__, req_info.url);

    err = httpc_send_request(req_info.host, server_port, req_info.url, &httpc_settings,
                             httpc_altcp_recv_cb, &httpc_settings, &httpc_connection);

    if (err != ERR_OK) {
        AT_TRACE("httpc_send_request failed: %d\n", err);
        goto END;
    }

    httpc_free_req_info(&req_info);
    return;

END:
    httpc_free_req_info(&req_info);
    httpc_cmd_done = 1;
}

void at_httpc_url_cfg(int argc, char **argv)
{
    char *endptr = NULL;
    uint16_t url_len;

    AT_RSP_START(128);
    if ((argc < 1) || (argc > 2))
        goto Error;

    if ((argc == 2) && (argv[1][0] == AT_QUESTION))
        goto Usage;

    if ((argc == 1) && (strcmp(argv[0], "AT+HTTPURLCFG?") == 0)) {

        if (url_info.url_len == 0) {
            AT_RSP("+HTTPURLCFG:%u,\r\n", url_info.url_len);
            AT_RSP_OK();
        } else {
            sys_mfree(rsp_buf);
            AT_RSP_START((url_info.url_len + 64));
            AT_RSP("+HTTPURLCFG:%u,%s\r\n", url_info.url_len, url_info.httpc_url);
            AT_RSP_OK();
        }

        return;
    }

    if (argc == 2) {
        url_len = (uint16_t)strtoul((const char *)argv[1], &endptr, 10);
        if (url_len == 0) {
            if (url_info.httpc_url != NULL) {
                sys_mfree(url_info.httpc_url);
                url_info.httpc_url = NULL;
                url_info.url_len = 0;
            }
            AT_RSP("SET OK\r\n");
            AT_RSP_DIRECT(rsp_buf, rsp_buf_idx);
            sys_mfree(rsp_buf);
            return;
        }

        if ((url_len < HTTPC_URL_CFG_MIN_LEN) || (url_len > HTTPC_URL_CFG_MAX_LEN)) {
            AT_TRACE("invalid url length:%u\r\n", url_len);
            goto Error;
        }

        if (url_info.httpc_url != NULL) {
            sys_mfree(url_info.httpc_url);
            url_info.url_len = 0;
        }

        url_info.httpc_url = (char *)sys_malloc(url_len + 1);
        if (url_info.httpc_url == NULL) {
            AT_TRACE("sys_malloc for cfg url fail\r\n");
            goto Error;
        }
        sys_memset(url_info.httpc_url, 0, (url_len + 1));
        url_info.url_len = url_len;

        AT_RSP("OK\r\n");
        AT_RSP(">\r\n");
        AT_RSP_DIRECT(rsp_buf, rsp_buf_idx);

        at_hw_dma_receive((uint32_t)url_info.httpc_url, url_info.url_len);
        url_info.httpc_url[url_len] = '\0';

        rsp_buf_idx = 0;
        AT_RSP("SET OK\r\n");
        AT_RSP_DIRECT(rsp_buf, rsp_buf_idx);
        sys_mfree(rsp_buf);

        return;
    }

Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+HTTPURLCFG=<url length>\r\n");
    AT_RSP_OK();
    return;
}

void at_httpc_head_cfg(int argc, char **argv)
{
    char *endptr = NULL;
    uint16_t header_len;
    char *tmp_header = NULL;

    AT_RSP_START(768);
    if ((argc < 1) || (argc > 2))
        goto Error;

    if ((argc == 2) && (argv[1][0] == AT_QUESTION))
        goto Usage;

    if ((argc == 1) && (strcmp(argv[0], "AT+HTTPCHEAD?") == 0)) {
        for (int i = 0; i < req_header.header_cnt; i++)
            AT_RSP("+HTTPCHEAD:%d,\"%s\"\r\n", i, req_header.headers[i]);

        AT_RSP_OK();
        return;
    }

    if (argc == 2) {
        header_len = (uint16_t)strtoul((const char *)argv[1], &endptr, 10);
        if (header_len == 0) {
            for (int i = 0; i < MAX_HTTP_REQ_HEADER_NUM; i++) {
                if (req_header.headers[i] != NULL)
                    sys_mfree(req_header.headers[i]);
            }
            sys_memset(&req_header, 0, sizeof(req_header));

            AT_RSP_OK();
            return;
        }

        if (header_len > MAX_HTTP_REQ_HEADER_LEN) {
            AT_TRACE("invalid http header length:%u\r\n", header_len);
            goto Error;
        }

        AT_RSP("OK\r\n");
        AT_RSP(">\r\n");
        AT_RSP_DIRECT(rsp_buf, rsp_buf_idx);

        tmp_header = (char *)sys_malloc(header_len + 1);
        if (tmp_header == NULL) {
            AT_TRACE("sys_malloc for tmp_header fail\r\n");
            goto Error;
        }

        at_hw_dma_receive((uint32_t)tmp_header, header_len);
        tmp_header[header_len] = '\0';

        if (strchr(tmp_header, ':') == NULL) {
            AT_TRACE("invalid header format\r\n");
            goto Error;
        }

        int updated = 0;
        for (int i = 0; i < req_header.header_cnt; i++) {
            if (compare_before_colon((const char *)tmp_header, (const char *)req_header.headers[i])) {
                if (strlen(req_header.headers[i]) < header_len) {
                    sys_mfree(req_header.headers[i]);

                    req_header.headers[i] = (char *)sys_malloc(header_len + 1);
                    if (req_header.headers[i] == NULL) {
                        AT_TRACE("sys_malloc for new request header fail\r\n");
                        goto Error;
                    }
                }
                strcpy(req_header.headers[i], tmp_header);
                req_header.headers_len[i] = header_len;
                updated = 1;
                break;
            }
        }

        if (updated == 0) {
            if (req_header.header_cnt < MAX_HTTP_REQ_HEADER_NUM) {
                req_header.headers[req_header.header_cnt] = (char *)sys_malloc(header_len + 1);
                if (req_header.headers[req_header.header_cnt] == NULL) {
                    AT_TRACE("sys_malloc for new request header fail\r\n");
                    goto Error;
                }
                strcpy(req_header.headers[req_header.header_cnt], tmp_header);
                req_header.headers_len[req_header.header_cnt] = header_len;
                req_header.header_cnt++;
            } else {
                AT_TRACE("Header storage full\r\n");
                goto Error;
            }
        }

        if (tmp_header != NULL)
            sys_mfree(tmp_header);

        rsp_buf_idx = 0;
        AT_RSP_OK();

        return;
    }

Error:
    if (tmp_header != NULL)
        sys_mfree(tmp_header);

    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+HTTPCHEAD=<req_header_len>\r\n");
    AT_RSP_OK();
    return;
}
#endif
