/*!
    \file    atcmd_ota_demo.h
    \brief   AT command to demo OTA run on alibaba ecs server for GD32VW55x SDK

    \version 2025-08-21, V1.0.0, firmware for GD32VW55x
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
#include "app_cfg.h"

#ifdef CONFIG_ATCMD_OTA_DEMO

#include "mbedtls/version.h"
#include "config_gdm32.h"

#define MBEDTLS_VER_2_17_0       0x02110000
#if (!defined(MBEDTLS_VERSION_NUMBER) || MBEDTLS_VERSION_NUMBER != MBEDTLS_VER_2_17_0)
#include "mbedtls/mbedtls_config.h"
#else
#include "mbedtls/ssl_internal.h"
#include "rom_export_mbedtls.h"
#include "rom_export.h"
#endif

#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/ssl_ciphersuites.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/x509.h"
#include "mbedtls/md.h"
#include "mbedtls/sha256.h"


#include "mqtt_cmd.h"
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"
#include "mqtt_client_config.h"
#include "mqtt5_client_config.h"
#include "co_utils.h"

#include "atcmd_ota_demo.h"
#include "atcmd_dfu.h"

#include "config_gdm32.h"
#include "rom_export.h"
#include "raw_flash_api.h"
#include "wrapper_os.h"
#include "gd32vw55x.h"
#include "ble_export.h"


#include "trng.h"
#include "stdlib.h"
#include <string.h>

#include "wifi_management.h"
#include "cJSON.h"
#include "atcmd_mqtt.h"
#include "mqtt_cmd.h"

/*==========Please set the correct AP information ========*/
//#define SSID                                    "Testing-WiFi"
//#define PASSWORD                                "12345678"


/*========== Alibaba ECS Server Information ========*/
#define ALI_ECS_SERVER_HOST                     "120.26.121.22"

/*========== MQTT Broker Information run on Alibaba ECS Server ========*/
#define ALI_ECS_MQTT_PORT                       8883
#define ALI_ECS_MQTT_CLIENT_ID                  "ecs_demo_mqtt_e5d4866e"
#define ALI_ECS_MQTT_CLIENT_USERNAME            "user"
#define ALI_ECS_MQTT_CLIENT_PASSWORD            "123456"

/*========== Alibaba ECS Server Subscribe Topic =========*/
//#define ALI_ECS_SUB_TOPIC_PREFIX                "ali/ecs/demo/sub/"

#define ALI_ECS_SUB_TOPIC_OTA_VW553             "ota/vw553"
#define ALI_ECS_SUB_TOPIC_OTA_MUSIC             "ota/music"

#define ALI_ECS_SUB_TOPIC_SYSTEM_RESET          "system/reset"
#define ALI_ECS_SUB_TOPIC_SYSTEM_LED            "system/led"

#define ALI_ECS_SUB_TOPIC_WIFI_CONN             "wifi/conn"
#define ALI_ECS_SUB_TOPIC_WIFI_DISCONN          "wifi/disconn"

/* Subscribe all the topics, '#' is an wildcard character */
#define ALI_ECS_SUB_TOPIC_ALL                   "+"

/*========== Alibaba ECS Server Publish Topic =========*/
//#define ALI_ECS_PUB_TOPIC_PREFIX                "ali/ecs/demo/pub/"

#define ALI_ECS_PUB_TOPIC_OTA_VW553_STATUS      "ota/vw553/status"

#define ALI_ECS_PUB_TOPIC_OTA_MUSIC_STATUS      "ota/music/status"

#define ALI_ECS_PUB_TOPIC_SYSTEM_LED_STATUS     "system/led/status"

#define ALI_ECS_PUB_TOPIC_WIFI_STATUS           "wifi/status"

#define ALI_ECS_PUB_TOPIC_SYSTEM_VERSION        "system/version"

/*========== HTTPS Server Information run on Alibaba ECS Server ========*/
#define ALI_ECS_HTTPS_PORT                      "443"

#define TLS_VERIFY_SRV_CERT                     1
#define TLS_VERIFY_CLI_CERT                     1


#if (TLS_VERIFY_SRV_CERT || TLS_VERIFY_CLI_CERT)
#define TLS_CRT_USED
#include "atcmd_ota_certs.c"
#endif
/*========== OTA Definitions ========*/


typedef struct {
    mbedtls_net_context net_ctx;
    mbedtls_ssl_context ssl_ctx;
    mbedtls_ssl_config ssl_conf;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_x509_crt ca_cert;
    char server_host[OTA_MAX_SERVER_HOST_LEN];
    char server_port[5];
    char path[OTA_MAX_URL_LEN];
#if TLS_VERIFY_CLI_CERT
    mbedtls_x509_crt cli_cert;
    mbedtls_pk_context cli_key;
#endif
} ota_ssl_wrapper_t;

/*========== HTTP Protocol Definitions ========*/
#define HTTP_PROTOCOL "HTTP/1.1\r\n"
#define HTTPS_GET                               1
#define HTTPS_HEAD                              2
#define HTTPS_OPTIONS                           3
#define HTTPS_TRACE                             4
#define HTTPS_POST                              5

#define HEADER_MAX_LINES                        20
#define HEADER_LINE_MAX_LEN                     256
#define HEADER_STATUS_MSG_MAX_LEN               64

enum http_reply_code {
    HTTP_OK = 200,
    HTTP_PARTIAL_CONTENT = 206,
    HTTP_BAD_REQUEST = 400,
    UPNP_INVALID_ACTION = 401,
    UPNP_INVALID_ARGS = 402,
    HTTP_NOT_FOUND = 404,
    HTTP_PRECONDITION_FAILED = 412,
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_UNIMPLEMENTED = 501,
};

enum mqtt_scheme {
    MQTT_OVER_TCP = 1,
    MQTT_OVER_TLS = 2,
};

typedef struct {
    char version[16];              // HTTP version
    int status_code;               // Status code
    char status_message[HEADER_STATUS_MSG_MAX_LEN];       // Status Message
    char headers[HEADER_MAX_LINES][HEADER_LINE_MAX_LEN]; // Header lines
    int header_count;              // Lines of Header
    //unsigned char *body;
    uint32_t body_offset;
    size_t body_length;
} httpResponse_t;

/*===========LED Configuration===============*/
#if CONFIG_BOARD == PLATFORM_BOARD_32VW55X_START
#define AT_OTA_DEMO_LED_GPIO_PORT         GPIOB
#define AT_OTA_DEMO_LED_GPIO_PIN          GPIO_PIN_13
#elif (CONFIG_BOARD == PLATFORM_BOARD_32VW55X_EVAL || CONFIG_BOARD == PLATFORM_BOARD_32VW55X_SONIC)
#define AT_OTA_DEMO_LED_GPIO_PORT         GPIOC
#define AT_OTA_DEMO_LED_GPIO_PIN          GPIO_PIN_6
#endif

static int ssl_debug_level;

ota_ctx_t *g_ota_ctx_ptr = NULL;


/*!
    \brief      owner debug function
    \param[in]  file: pointer to the file
    \param[in]  line: line value
    \param[in]  str: pointer to the string
    \param[out] none
    \retval     none
*/
static void my_debug(void *ctx, int level,
                      const char *file, int line,
                      const char *str)
{
    ((void) level);
    AT_TRACE("%s:%04d: %s", file, line, str);
}


/*!
    \brief      owner get random function
    \param[in]  p_rng:
    \param[in]  output_len: get random calue length
    \param[out] output: pointer to the output value
    \retval     function run state(0: no error, other: have error)
*/
static int my_random(void *p_rng, unsigned char *output, size_t output_len)
{
    random_get(output, output_len);
    return 0;
}


static void print_hex_array(const uint8_t* arr, size_t len)
{
    AT_TRACE("{");
    for (size_t i = 0; i < len; ++i) {
        AT_TRACE("0x%02x", arr[i]);
        if (i != len - 1) AT_TRACE(", ");
    }
    AT_TRACE("}\n");
}


static void bytes_to_hex_string(const unsigned char *bytes, size_t length, char *hex_string)
{
    for (size_t i = 0; i < length; i++) {
        sprintf(hex_string + i * 2, "%02x", bytes[i]);
    }
    hex_string[length * 2] = '\0'; // end with '\0'
}


static int hex_char_to_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}


static int hex_string_to_bytes(const char* hex_str,
                       uint8_t* output,
                       size_t output_size)
{
    if (!hex_str || !output)
        return -1;

    size_t hex_len = strlen(hex_str);
    if (hex_len == 0 || hex_len % 2 != 0)
        return -2;

    size_t required_size = hex_len / 2;
    if (required_size > output_size)
        return -3;

    for (size_t i = 0; i < required_size; ++i) {
        const char high_char = hex_str[2*i];
        const char low_char = hex_str[2*i + 1];

        const int high = hex_char_to_value(high_char);
        const int low = hex_char_to_value(low_char);

        if (high < 0 || low < 0)
            return -4;

        output[i] = (high << 4) | low;
    }

    return 0;
}


/* Extract hostname path and port from URL */
static void extract_hostname_path_port(const char *url, char *hostname, char *path, char *port)
{
    const char *start = NULL;
    const char *end = NULL;
    const char *colon = NULL;
    const char *path_start = NULL;
    size_t hostname_len = 0;
    size_t path_len = 0;

    /* Skip "http://" or "https://" */
    start = strstr(url, "://");
    if (start != NULL) {
        start += 3; // skip "://"
    } else {
        start = url;
    }

    /* Search the end of hostname (':' or '/') */
    end = strpbrk(start, ":/");
    if (end == NULL) {
        hostname_len = strlen(start);
    } else {
        hostname_len = end - start;
    }
    strncpy(hostname, start, hostname_len);
    hostname[hostname_len] = '\0';

    /* Check the port is exist or not */
    colon = strchr(start, ':');
    if (colon != NULL && (end == NULL || colon < end)) {
        strncpy(port, colon + 1, (end ? end - colon - 1 : strlen(colon + 1)));
        port[(end ? end - colon - 1 : strlen(colon + 1))] = '\0';
    } else {
        /* default port */
        if (strncmp(url, "https://", 8) == 0) {
            strcpy(port, "443"); // default port for https
        } else if (strncmp(url, "http://", 7) == 0) {
            strcpy(port, "80"); // default port for http
        } else {
            strcpy(port, "443");
        }
    }

    /* Search the start offset of path */
    path_start = strchr(start, '/');
    if (path_start == NULL) {
        strcpy(path, "/"); // Return root dir if not exist
    } else {
        path_len = strlen(path_start);
        strncpy(path, path_start, path_len);
        path[path_len] = '\0';
    }
}


static int extract_from_query_content(ota_ctx_t *ota_ctx, const char *json_string)
{
    cJSON *root = NULL, *version_item = NULL, *checksum_item = NULL, *url_item = NULL;
    int ret = -1;

    if (ota_ctx == NULL || json_string == NULL) {
        AT_TRACE("Invalid parameters\n");
        return -1;
    }

    // Parse JSON string
    root = cJSON_Parse(json_string);
    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            AT_TRACE("JSON parse error before: %s\n", error_ptr);
        }
        goto cleanup;
    }

    // Extract version from root json
    version_item = cJSON_GetObjectItem(root, "version");
    if (cJSON_IsString(version_item) && (version_item->valuestring != NULL)) {
        strncpy(ota_ctx->version, version_item->valuestring, strlen(version_item->valuestring));
        ota_ctx->version[sizeof(ota_ctx->version) - 1] = '\0';
    } else {
        AT_TRACE("FW version not found or invalid\n");
        goto cleanup;
    }

    // Extract checkSum from root json
    checksum_item = cJSON_GetObjectItem(root, "checkSum");
    if (cJSON_IsString(checksum_item) && (checksum_item->valuestring != NULL)) {
        ret = hex_string_to_bytes(checksum_item->valuestring, ota_ctx->checksum, 16);
        if (ret)
            goto cleanup;
    } else {
        AT_TRACE("FW checkSum not found or invalid\n");
        goto cleanup;
    }

    // Extract FW URL from root json
    url_item = cJSON_GetObjectItem(root, "url");
    if (cJSON_IsString(url_item) && (url_item->valuestring != NULL)) {
        if (ota_ctx->update_url != NULL) {
            sys_mfree(ota_ctx->update_url);
        }
        ota_ctx->update_url = sys_malloc(strlen(url_item->valuestring) + 1);
        if (ota_ctx->update_url == NULL) {
            ret = OTA_INTERNAL_MEM_ERR;
            AT_TRACE("Malloc for FW Update url failed\n");
            goto cleanup;
        }
        strncpy(ota_ctx->update_url, url_item->valuestring, strlen(url_item->valuestring));
        ota_ctx->update_url[strlen(url_item->valuestring)] = '\0';
    } else {
        AT_TRACE("FW Update URL not found or invalid\n");
        goto cleanup;
    }

    ret = 0;
cleanup:
    if (root != NULL) {
        cJSON_Delete(root);
    }
    return ret;
}


/* Parse HTTP Response status line */
static int parse_status_line(const char *response, httpResponse_t *http_response)
{
    char line[512];
    const char *ptr = strstr(response, "\r\n"); // Find the end of the first line
    if (ptr == NULL) {
        return -1; // Not found
    }

    size_t line_length = ptr - response;
    if (line_length > sizeof(line)) {
        AT_TRACE("HTTP Response status line too long\r\n");
        return -2;
    }
    strncpy(line, response, line_length);
    line[line_length] = '\0';

    sscanf(line, "%s %d %[^\r\n]", http_response->version, &http_response->status_code, http_response->status_message);

    if (http_response->status_code != HTTP_OK && http_response->status_code != HTTP_PARTIAL_CONTENT)
        return -3;
    return 0;
}


/* Parse HTTP Response Header */
static int parse_headers(const char *response, httpResponse_t *http_response, const char **body_start_ptr)
{
    const char *ptr = strstr(response, "\r\n\r\n"); // Find the end of the Header
    char *header_line_start = strstr(response, "\r\n");
    char header_line[HEADER_LINE_MAX_LEN];
    int header_index = 0;

    if (ptr == NULL) {
        return -1; // Not found
    }
    *body_start_ptr = ptr + 4; // The start of http response body

    while (header_line_start < ptr && header_index < HEADER_MAX_LINES) {
        char *header_line_end = strstr(header_line_start, "\r\n");
        if (!header_line_end || header_line_end > ptr) {
            return -2;
            break;
        }
        size_t length = header_line_end - header_line_start;
        if (length > sizeof(header_line)) {
            AT_TRACE("header_line overflow\r\n");
            return -3;
        }
        strncpy(header_line, header_line_start, length);
        header_line[length] = '\0';

        // Save each Header line
        strncpy(http_response->headers[header_index], header_line, sizeof(http_response->headers[header_index]));
        header_index++;

        // Move to next line
        header_line_start = header_line_end + 2; // skip "\r\n"
    }

    http_response->header_count = header_index;
    return 0;
}


/* Extract Content-Length from Header */
static size_t get_content_length(httpResponse_t *http_response)
{
    if (http_response == NULL)
        return 0;

    for (int i = 0; i < http_response->header_count; i++) {
        if (strncmp(http_response->headers[i], "Content-Length", 14) == 0) {
            char *colon = strchr(http_response->headers[i], ':');
            if (colon) {
                return (size_t)atoi(colon + 1);
            }
        }
    }
    return 0; // return 0 if not found
}


/* Parse HTTP Response */
static int32_t parse_http_response(const char *response, httpResponse_t *http_response)
{
    int ret = 0;

    /* Parse status line */
    if (parse_status_line(response, http_response) != 0) {
        AT_TRACE("Failed to parse status line\n");
        ret = OTA_SERVER_ACCESS_ERR;
        return ret;
    }

    /* Parse response header */
    const char *body_start = NULL;
    if (parse_headers(response, http_response, &body_start) != 0) {
        AT_TRACE("Failed to parse headers\n");
        ret = OTA_SERVER_ACCESS_ERR;
        return ret;
    }

    /* Extract and parse response body */
    size_t content_length = get_content_length(http_response);
    http_response->body_length = content_length;
    http_response->body_offset = body_start - response;

    return 0;
}


static void at_indicate_ota_progress(int state, int percent)
{
    int msg_len = 128;
    AT_RSP_START(64);
    int err = 0;

    AT_RSP("+IND_W=CIUPDATESTATE,%d", state);
    if (state >= OTA_ST_IN_PROGRESS)
        AT_RSP(",%d", percent);

    AT_RSP("\r\n");
    AT_RSP_IMMEDIATE();
    AT_RSP_FREE();

    mqtt_client_t *at_mqtt_client = mqtt_client_get();
    if (at_mqtt_client != NULL &&  mqtt_client_is_connected(at_mqtt_client)) {
        // publish OTA Progress to MQTT Broker
        char *pub_ota_msg = sys_zalloc(msg_len);
        if (pub_ota_msg == NULL) {
            app_print("Malloc for pub_ota_msg failed\r\n");
            return;
        }
        if (state < OTA_ST_IN_PROGRESS)
            snprintf(pub_ota_msg, msg_len, "{\"status\":\"terminated\"}");
        else if (percent != 100)
            snprintf(pub_ota_msg, msg_len, "{\"status\":\"ongoing\",\"progress\":%d}", percent);
        else
            snprintf(pub_ota_msg, msg_len, "{\"status\":\"completed\"}");

        if (strncmp(g_ota_ctx_ptr->fw_name, "VW553", 5) == 0)
            at_mqtt_msg_pub(ALI_ECS_PUB_TOPIC_OTA_VW553_STATUS, pub_ota_msg, strlen(pub_ota_msg), 0, 0);
        else if (strncmp(g_ota_ctx_ptr->fw_name, "MUSIC", 5) == 0)
            at_mqtt_msg_pub(ALI_ECS_PUB_TOPIC_OTA_MUSIC_STATUS, pub_ota_msg, strlen(pub_ota_msg), 0, 0);
        sys_mfree(pub_ota_msg);
    } else {
        if (at_mqtt_client == NULL)
            app_print("at_mqtt_client is null\r\n");
    }
}


static void ota_timer_callback(void *p_tmr, void *p_arg)
{
    ota_ctx_t *ota_ctx = (ota_ctx_t *)p_arg;
    if (ota_ctx == NULL)
        return;
    AT_TRACE("===AT ota timeout===\r\n");
    ota_ctx->reason = OTA_TIMEOUT;
}


static int ota_ctx_init(ota_ctx_t *ota_ctx_ptr)
{
    if (ota_ctx_ptr == NULL)
        return -1;

    if (ota_ctx_ptr->ota_tmr == NULL) {
        sys_timer_init(&ota_ctx_ptr->ota_tmr, (const uint8_t *)"ota_tmr",
                        OTA_TIMEOUT_LIMIT, 0,
                        ota_timer_callback, (void *)ota_ctx_ptr);
    }

    ota_ctx_ptr->file_length = 0;
    ota_ctx_ptr->current_offset = 0;
    ota_ctx_ptr->real_length = 0;
    ota_ctx_ptr->segment_length = OTA_FW_SEGMENT_LEN;
    ota_ctx_ptr->reason = 0;

    wifi_netlink_ps_mode_set(0, 0); // turn off wifi ps before ota
    // wait_mqtt_disconnect();     // disconnect MQTTS connection
    return 0;
}


void ota_ctx_reset(ota_ctx_t *ota_ctx_ptr)
{
    if (ota_ctx_ptr == NULL) {
        g_ota_ctx_ptr = NULL;
        return;
    }

    if (ota_ctx_ptr->update_url) {
        sys_mfree(ota_ctx_ptr->update_url);
    }

    if (ota_ctx_ptr->query_url) {
        sys_mfree(ota_ctx_ptr->query_url);
    }

    if (ota_ctx_ptr->ota_tmr) {
        sys_timer_stop(&ota_ctx_ptr->ota_tmr, 0);
        sys_timer_delete(&ota_ctx_ptr->ota_tmr);
    }

    if (ota_ctx_ptr->buf)
        sys_mfree(ota_ctx_ptr->buf);

    sys_memset(ota_ctx_ptr, 0, sizeof(*ota_ctx_ptr));

    sys_mfree(ota_ctx_ptr);
    g_ota_ctx_ptr = NULL;

    wifi_netlink_ps_mode_set(0, 2); // recover wifi ps after ota
}

void at_ota_demo_ssl_client_disconnect(ota_ssl_wrapper_t *ota_ssl)
{
    if (!ota_ssl)
        return;

    mbedtls_net_free(&ota_ssl->net_ctx);
    mbedtls_entropy_free(&ota_ssl->entropy);
    mbedtls_ctr_drbg_free(&ota_ssl->ctr_drbg);
    mbedtls_x509_crt_free(&ota_ssl->ca_cert);
#if TLS_VERIFY_CLI_CERT
    mbedtls_x509_crt_free(&ota_ssl->cli_cert);
    mbedtls_pk_free(&ota_ssl->cli_key);
#endif
    mbedtls_ssl_free(&ota_ssl->ssl_ctx);
    mbedtls_ssl_config_free(&ota_ssl->ssl_conf);
#if (MBEDTLS_VERSION_NUMBER != MBEDTLS_VER_2_17_0)
    AT_TRACE("mbedtls PSA memory free\r\n");
    mbedtls_psa_crypto_free();
#endif
    sys_mfree(ota_ssl);
}

int at_ota_demo_ssl_client(ota_ssl_wrapper_t *ota_ssl, int verify_server)
{
    int ret = 0, len;
    unsigned char buf[128] = {0};
    const char *pers = "ssl_client";

    if (ota_ssl == NULL) {
        ret = OTA_PARAM_ERR;
        goto exit;
    }
    /* Initialize */
    mbedtls_net_init(&ota_ssl->net_ctx);
    mbedtls_ssl_init(&ota_ssl->ssl_ctx);
    mbedtls_ssl_config_init(&ota_ssl->ssl_conf);
    mbedtls_entropy_init(&ota_ssl->entropy);
    mbedtls_ctr_drbg_init(&ota_ssl->ctr_drbg);

    if (verify_server)
        mbedtls_x509_crt_init(&ota_ssl->ca_cert);

#if TLS_VERIFY_CLI_CERT
    mbedtls_x509_crt_init(&ota_ssl->cli_cert);
    mbedtls_pk_init(&ota_ssl->cli_key);
#endif

    mbedtls_debug_set_threshold(ssl_debug_level);

    const char *alpn[] = {"http/1.1", NULL};
    mbedtls_ssl_conf_alpn_protocols(&ota_ssl->ssl_conf, alpn);
    mbedtls_ssl_conf_session_tickets(&ota_ssl->ssl_conf, MBEDTLS_SSL_SESSION_TICKETS_ENABLED);

    mbedtls_ssl_set_hostname(&ota_ssl->ssl_ctx, NULL);

#if (MBEDTLS_VERSION_NUMBER != MBEDTLS_VER_2_17_0)
    /* Setup SSL version */
    mbedtls_ssl_conf_min_version(&ota_ssl->ssl_conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_4); // TLSv1.3
    mbedtls_ssl_conf_max_version(&ota_ssl->ssl_conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_4); // TLSv1.3
#endif

    /* Setup random number generator */
    ret = mbedtls_ctr_drbg_seed(&ota_ssl->ctr_drbg, mbedtls_entropy_func,
                    &ota_ssl->entropy,
                    (const unsigned char *)pers,
                    strlen(pers));
    if (ret != 0) {
        AT_TRACE("AT+CIUPDATE: failed to initialize CTR_DRBG: -0x%x\n", -ret);
        ret = OTA_PARAM_ERR;
        goto exit;
    }

#if TLS_VERIFY_SRV_CERT
    if (verify_server) {
        /* Setup server's CA cert chain */
        ret = mbedtls_x509_crt_parse(&ota_ssl->ca_cert, (const unsigned char *)ecs_ca_crt, strlen(ecs_ca_crt) + 1);
        if (ret < 0) {
            AT_TRACE("AT+CIUPDATE: Failed to parse CA certificate chain: -0x%x\n", -ret);
            ret = OTA_SERVER_ACCESS_ERR;
            goto exit;
        }
        AT_TRACE("CA certificate chain loaded successfully\n");
    }
#endif

#if TLS_VERIFY_CLI_CERT
    AT_TRACE("  . Loading the Client certificate ...\r\n");

    ret = mbedtls_x509_crt_parse(&ota_ssl->cli_cert, (const unsigned char *)ecs_cli_crt, strlen(ecs_cli_crt) + 1);
    if (ret < 0) {
        AT_TRACE(" AT+CIUPDATE: failed\r\n  ! mbedtls_x509_crt_parse returned -0x%x\r\n", -ret);
        goto exit;
    }

    AT_TRACE("ok (%d skipped)\r\n", ret);

    AT_TRACE("  . Loading the Client key ...\r\n");
#if  (MBEDTLS_VERSION_NUMBER != MBEDTLS_VER_2_17_0)
    ret = mbedtls_pk_parse_key(&ota_ssl->cli_key, (const unsigned char *)ecs_cli_key, strlen(ecs_cli_key) + 1, NULL, 0, my_random, 0);
#else
    ret = mbedtls_pk_parse_key(&ota_ssl->cli_key, (const unsigned char *)ecs_cli_key, strlen(ecs_cli_key) + 1, NULL, 0);
#endif
    if (ret < 0) {
        AT_TRACE("AT+CIUPDATE: failed\r\n  !  mbedtls_pk_parse_key returned -0x%x\r\n", -ret);
        goto exit;
    }

    AT_TRACE(" ok (%d skipped)\r\n", ret);
#endif

    /* Connecting to server */
    AT_TRACE("Connecting to %s:%s...\n", ota_ssl->server_host, ota_ssl->server_port);
    ret = mbedtls_net_connect(&ota_ssl->net_ctx, ota_ssl->server_host, ota_ssl->server_port, MBEDTLS_NET_PROTO_TCP);
    if (ret != 0) {
        AT_TRACE("AT+CIUPDATE: Failed to connect to the server: -0x%x\n", -ret);
        ret = OTA_SERVER_ACCESS_ERR;
        goto exit;
    }
    AT_TRACE("Connected to the server\n");

    /* Setup SSL */
    mbedtls_ssl_config_defaults(
                &ota_ssl->ssl_conf,
                MBEDTLS_SSL_IS_CLIENT,
                MBEDTLS_SSL_TRANSPORT_STREAM,
                MBEDTLS_SSL_PRESET_DEFAULT);
    mbedtls_ssl_conf_read_timeout(&ota_ssl->ssl_conf, 10000); //10s

    /* Setup random number generator */
    mbedtls_ssl_conf_rng(&ota_ssl->ssl_conf, my_random, NULL);
    mbedtls_ssl_conf_dbg(&ota_ssl->ssl_conf, my_debug, NULL);
#if TLS_VERIFY_SRV_CERT
    if (verify_server) {
        /* Setup CA cert chain to verify server */
        mbedtls_ssl_conf_ca_chain(&ota_ssl->ssl_conf, &ota_ssl->ca_cert, NULL);

        /* Verify server's cert, ignore client's cert */
        mbedtls_ssl_conf_authmode(&ota_ssl->ssl_conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    } else {
        mbedtls_ssl_conf_authmode(&ota_ssl->ssl_conf, MBEDTLS_SSL_VERIFY_NONE);
    }
#else
    mbedtls_ssl_conf_authmode(&ota_ssl->ssl_conf, MBEDTLS_SSL_VERIFY_NONE);
#endif

#if TLS_VERIFY_CLI_CERT
    mbedtls_ssl_conf_own_cert(&ota_ssl->ssl_conf, &ota_ssl->cli_cert, &ota_ssl->cli_key);
#endif

    mbedtls_ssl_conf_read_timeout(&ota_ssl->ssl_conf, OTA_TIMEOUT_LIMIT);

#if (MBEDTLS_VERSION_NUMBER != MBEDTLS_VER_2_17_0) && defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
    mbedtls_ssl_conf_max_frag_len(&ota_ssl->ssl_conf, MBEDTLS_SSL_MAX_FRAG_LEN_1024);
#endif
    /* Setup SSL context */
    ret = mbedtls_ssl_setup(&ota_ssl->ssl_ctx, &ota_ssl->ssl_conf);
    if (ret != 0) {
        AT_TRACE("AT+CIUPDATE: Failed to setup SSL context: -0x%x\n", -ret);
        ret = OTA_SERVER_ACCESS_ERR;
        goto exit;
    }

#if (MBEDTLS_VERSION_NUMBER != MBEDTLS_VER_2_17_0) && defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
    int frag_len = mbedtls_ssl_get_max_in_record_payload(&ota_ssl->ssl_ctx);
    frag_len = mbedtls_ssl_get_max_out_record_payload(&ota_ssl->ssl_ctx);
#endif

    mbedtls_ssl_set_bio(&ota_ssl->ssl_ctx, &ota_ssl->net_ctx, mbedtls_net_send, NULL, mbedtls_net_recv_timeout);

    /* SSL/TLS handshake */
    AT_TRACE("Performing the SSL/TLS handshake...\n");
    ret = mbedtls_ssl_handshake(&ota_ssl->ssl_ctx);
    if (ret != 0) {
        AT_TRACE("AT+CIUPDATE: SSL handshake failed: -0x%x\n", -ret);
        ret = OTA_SERVER_ACCESS_ERR;
        goto exit;
    }
    AT_TRACE("SSL/TLS handshake succeeded\n");

#if TLS_VERIFY_SRV_CERT
    /* Verify server's cert chain */
    uint32_t flags = mbedtls_ssl_get_verify_result(&ota_ssl->ssl_ctx);
    if (flags != 0) {
        mbedtls_x509_crt_verify_info((char *)buf, sizeof(buf), "VR:", flags);
        AT_TRACE("AT+CIUPDATE: Server certificate verification failed: %s\n", buf);
        ret = OTA_SERVER_ACCESS_ERR;
        goto exit;
    }
    AT_TRACE("Server certificate verification succeeded\n");
#endif

exit:
    AT_TRACE("ssl_client exit with ret=%d\r\n", ret);
    return ret;
}


int at_fw_url_query(ota_ctx_t *ota_ctx, ota_ssl_wrapper_t *ota_ssl, char *http_request, httpResponse_t *http_response)
{
    int err = OTA_OK;
    uint16_t url_len = 0;
    uint32_t len = 0;
    int verify_cert = 1;

    if (ota_ctx == NULL || ota_ctx->state != OTA_ST_QUERY || ota_ctx->query_url == NULL) {
        return -1;
    }

    url_len = strlen(ota_ctx->query_url);
    if (url_len == 0 || url_len > OTA_MAX_URL_LEN) {
        return -2;
    }

    ota_ctx->state = OTA_ST_QUERY;
    sys_timer_stop(&ota_ctx->ota_tmr, 0);
    sys_timer_start(&ota_ctx->ota_tmr, 0);

    extract_hostname_path_port(ota_ctx->query_url, ota_ssl->server_host, ota_ssl->path, ota_ssl->server_port);
    if (strlen(ota_ssl->server_host) == 0 || strlen(ota_ssl->path) == 0 || strlen(ota_ssl->server_port) == 0)
        goto exit;

    err = at_ota_demo_ssl_client(ota_ssl, verify_cert);
    if (err) {
        err = OTA_QUERY_URL_ERR;
        goto exit;
    }

    /* Get the json file */
    len = snprintf(http_request, OTA_MAX_URL_LEN,
                   "GET %s HTTP/1.1\r\n"
                   "Host: %s\r\n"
                   "Connection: keep-alive\r\n"
                   "\r\n",
                   ota_ssl->path, ota_ssl->server_host);

    err = mbedtls_ssl_write(&ota_ssl->ssl_ctx, (const unsigned char *)http_request, strlen(http_request));
    if (err <= 0) {
        AT_TRACE("Failed to send data: -0x%x\n", -err);
        err = OTA_QUERY_URL_ERR;
        goto exit;
    }
    AT_TRACE("Receiving server response...\n");

    if ((len = mbedtls_ssl_read(&ota_ssl->ssl_ctx, ota_ctx->buf, ota_ctx->buf_len)) > 0) {
        err = parse_http_response((const char *)ota_ctx->buf, http_response);
        if (err) {
            AT_TRACE("AT+CIUPDATE: failed to parse http response\r\n");
            err = OTA_QUERY_CONTENT_ERR;
            goto exit;
        }

        if (http_response->status_code != HTTP_OK || http_response->body_length == 0 || \
                    http_response->body_length > OTA_MAX_URL_JSON_LEN) {
            AT_TRACE("status:%d, len:%d\r\n", http_response->status_code, http_response->body_length);
            err = OTA_QUERY_CONTENT_ERR;
            goto exit;
        }
    } else {
        AT_TRACE("Failed to read server response: -0x%x\n", -err);
        err = OTA_QUERY_CONTENT_ERR;
        goto exit;
    }

    AT_TRACE("Receiving server response OK....%s\n", (char *)(ota_ctx->buf + http_response->body_offset));

    err = extract_from_query_content(ota_ctx, (char *)(ota_ctx->buf + http_response->body_offset));
    if (err) {
        AT_TRACE("Failed to extract content: %s\r\n", (char *)(ota_ctx->buf + http_response->body_offset));
        err = OTA_QUERY_CONTENT_ERR;
    }

exit:
    if (err || ota_ctx->reason) {
        AT_TRACE("AT+CIUPDATE: query ota url fail\r\n", err);
        err = (err ? err : ota_ctx->reason);
    } else {
        // Update OTA State
        ota_ctx->state = OTA_ST_READY;
        AT_TRACE("AT+CIUPDATE: query ota url success\r\n");
    }
    AT_TRACE("at_fw_url_query exit with err=%d\r\n", err);
    return err;
}


static void at_ota_demo_task(void *param)
{
    uint8_t verify_cert = 1;
    char *http_request = NULL;
    int len = 0;
    ota_ssl_wrapper_t *ota_ssl = NULL;
    httpResponse_t *http_response = NULL;
    uint32_t ota_percent = 0, step = 0;
    int http_rsp_header_parsed = 0;
    ota_ctx_t *ctx = (ota_ctx_t *)param;
    int *err_ptr = &ctx->reason;

    if (ctx == NULL) {
        AT_TRACE("AT+CIUPDATE: invalid param\r\n");
        goto Error1;
    }

    ota_ctx_init(ctx);

    http_response = (httpResponse_t *)sys_zalloc(sizeof(httpResponse_t));
    if (http_response == NULL) {
        *err_ptr = OTA_INTERNAL_MEM_ERR;
        goto Error1;
    }

    ota_ssl = sys_zalloc(sizeof(ota_ssl_wrapper_t));
    if (!ota_ssl) {
        AT_TRACE("AT+CIUPDATE: malloc ssl_ctx failed\r\n");
        *err_ptr = OTA_INTERNAL_MEM_ERR;
        goto Error1;
    }

    http_request = sys_zalloc(OTA_MAX_URL_LEN);
    if (!http_request) {
        AT_TRACE("AT+CIUPDATE: malloc http request failed\r\n");
        *err_ptr = OTA_INTERNAL_MEM_ERR;
        goto Error1;
    }

    if (ctx->buf) {
        sys_mfree(ctx->buf);
        ctx->buf = NULL;
    }

    ctx->buf = sys_zalloc(OTA_FW_SEGMENT_LEN);
    if (ctx->buf == NULL) {
        *err_ptr = OTA_INTERNAL_MEM_ERR;
        goto Error1;
    }
    ctx->buf_len = OTA_FW_SEGMENT_LEN;

    *err_ptr = at_fw_url_query(ctx, ota_ssl, http_request, http_response);
    if (*err_ptr) {
        AT_TRACE("AT+CIUPDATE: query OTA url failed\r\n");
        goto Error;
    }

    ctx->state = OTA_ST_IN_PROGRESS;

    extract_hostname_path_port(ctx->update_url, ota_ssl->server_host, ota_ssl->path, ota_ssl->server_port);
    AT_TRACE("server_host:%s, path:%s, port:%s\r\n", ota_ssl->server_host, ota_ssl->path, ota_ssl->server_port);

    if (strlen(ota_ssl->server_host) == 0 || strlen(ota_ssl->path) == 0 || strlen(ota_ssl->server_port) == 0) {
        AT_TRACE("AT+CIUPDATE: invalid update url\r\n");
        *err_ptr = OTA_PARAM_ERR;
        goto Error;
    }

    len = snprintf(http_request, OTA_MAX_URL_LEN,
                    "GET %s HTTP/1.1\r\n"
                    "Host: %s:%s\r\n"
                    "User-Agent: curl/7.75.0\r\n"
                    "Accept: */*\r\n"
                    "Connection: keep-alive\r\n"
                    "\r\n\r\n",
                    ota_ssl->path, ota_ssl->server_host, ota_ssl->server_port);

    len = mbedtls_ssl_write(&ota_ssl->ssl_ctx, (const unsigned char *)http_request, strlen(http_request));
    if (len <= 0) {
        AT_TRACE("AT+CIUPDATE: failed to send data: -0x%x\n", -len);
        *err_ptr = OTA_SERVER_ACCESS_ERR;
        goto Error;
    }

    int http_rsp_parsed = 0;

    sys_memset(ctx->buf, 0, OTA_FW_SEGMENT_LEN);
    sys_memset(http_response, 0, sizeof(*http_response));

    while ((ctx->current_offset < ctx->file_length - 1) &&
                (ctx->reason == 0) &&
                (ctx->state != OTA_ST_COMPLETED)) {
        int real_len = 0;

        sys_timer_stop(&ctx->ota_tmr, 0);
        sys_timer_start(&ctx->ota_tmr, 0);

        if ((real_len = mbedtls_ssl_read(&ota_ssl->ssl_ctx, ctx->buf, ctx->segment_length)) > 0) {
            if (http_rsp_header_parsed == 0) {
                http_rsp_header_parsed = 1;
                int ret = parse_http_response((const char *)ctx->buf, http_response);
                if (ret == 0 && (http_response->status_code == HTTP_OK || http_response->status_code == HTTP_PARTIAL_CONTENT)) {
                    AT_TRACE("status_code=%d, %s\r\n", http_response->status_code, ctx->buf);


                    ctx->file_length = http_response->body_length;
                    ctx->segment_length = OTA_FW_SEGMENT_LEN;
                    ctx->current_offset = 0;
                    ctx->buf_offset = http_response->body_offset;
                    ctx->real_length = real_len - http_response->body_offset;

                    if (at_dfu_get_ready(DFU_MODE_WIFI, http_response->body_length) < 0) {
                        AT_TRACE("dfu get ready fail, body_length=%d\r\n", http_response->body_length);
                        *err_ptr = OTA_DOWNLOAD_ERR;
                        goto Error;
                    }

                    step = ctx->file_length / 100;
                } else {
                    AT_TRACE("AT+CIUPDATE: status_code=%d, %s\r\n", http_response->status_code, ctx->buf);
                    *err_ptr = OTA_DOWNLOAD_ERR;
                    goto Error;
                }
            } else {
                ctx->buf_offset = 0;
                ctx->real_length = real_len;
            }

            /* Write to Flash */
            len = at_dfu_write_image((uint8_t *)(ctx->buf + ctx->buf_offset), ctx->real_length);
            if (len < 0) {
                AT_TRACE("OTA flash write failed!\r\n");
                *err_ptr = OTA_INTERNAL_FLASH_ERR;
                break;
            }

            ctx->current_offset += ctx->real_length;
            if (ctx->current_offset >= (ota_percent + 10) * step) {
                ota_percent += 10;
                if (ota_percent != 100)
                    at_indicate_ota_progress(ctx->state, ota_percent);
            }

        }            else {
            AT_TRACE("Failed to read server response: -0x%x\n", -real_len);
            *err_ptr = OTA_DOWNLOAD_ERR;
            break;
        }
    }

    /* Verify and report */
    if (ctx->reason == 0) {
        unsigned char image_checksum[16] = {0};
        at_dfu_verify_image(image_checksum, MBEDTLS_MD_MD5);
        if (memcmp(image_checksum, ctx->checksum, 16) == 0) {
            AT_TRACE("vw553 fw checksum matches\r\n");
            *err_ptr = OTA_OK;
        } else {
            AT_TRACE("vw553 fw checksum mismatches\r\n");
            print_hex_array(image_checksum, 16);
            AT_TRACE("vs\r\n");
            print_hex_array(ctx->checksum, 16);
            *err_ptr = OTA_VERIFY_ERR;
            at_dfu_finish(false);
            goto Error;
        }

        if (at_dfu_finish(true)) {
            *err_ptr = OTA_VERIFY_ERR;
            goto Error;
        }

        ota_percent = 100;
        *err_ptr = OTA_OK;
        ctx->state = OTA_ST_COMPLETED;
        at_indicate_ota_progress(ctx->state, ota_percent);
        AT_TRACE("file transfer ok---------\r\n");
    } else {
        AT_TRACE("file transfer fail--------, %d\r\n", ctx->reason);
    }

Error:
    at_ota_demo_ssl_client_disconnect(ota_ssl);
    ota_ssl = NULL;

Error1:
    if (*err_ptr != 0) {
        at_indicate_ota_progress(*err_ptr, 0);
        at_dfu_finish(false);
        ctx->state = OTA_ST_PENDING;
    }

    if (http_request)
        sys_mfree(http_request);

    if (http_response)
        sys_mfree(http_response);

    ota_ctx_reset(ctx);

    if (ota_ssl)
        sys_mfree(ota_ssl);

    if (http_response)
        sys_mfree(http_response);

    sys_task_delete(NULL);
}


static void at_fw_update(int argc, char **argv)
{
    char *url = NULL;
    uint32_t url_len = 0;

    AT_RSP_START(64);

    if (argc != 2)
        goto Error;

    if (argv[1][0] == AT_QUESTION)
        goto Usage;

    url = at_string_parse(argv[1]);
    //url = argv[1];

    url_len = (url != NULL) ? strlen(url) : 0;
    if (url_len == 0) {
        AT_TRACE("AT+CIUPDATE: Firmware url is empty\r\n");
        goto Error;
    }

    if (g_ota_ctx_ptr != NULL && g_ota_ctx_ptr->state != OTA_ST_IDLE) {
        AT_TRACE("AT+CIUPDATE: OTA is in progress, please wait\r\n");
        goto Error;
    }

    if (g_ota_ctx_ptr == NULL) {
        g_ota_ctx_ptr = sys_zalloc(sizeof(ota_ctx_t));
        if (g_ota_ctx_ptr == NULL) {
            AT_TRACE("AT+CIUPDATE: sys_calloc for ota_demo_ctx_t fail\r\n");
            goto Error;
        }
    }

    g_ota_ctx_ptr->query_url = sys_calloc(1, url_len + 1);
    if (g_ota_ctx_ptr->query_url == NULL) {
        AT_TRACE("AT+CIUPDATE: sys_calloc for url fail\r\n");
        goto Error;
    }

    strncpy(g_ota_ctx_ptr->query_url, url, url_len);
    g_ota_ctx_ptr->query_url[url_len] = '\0';

    g_ota_ctx_ptr->state = OTA_ST_QUERY;

    if (sys_task_create_dynamic((const uint8_t *)"OTA_TASK",
                    OTA_TASK_STK_SIZE,
                    OTA_TASK_PRIO,
                    (task_func_t)at_ota_demo_task, (void *)g_ota_ctx_ptr) == NULL) {
        AT_TRACE("AT+CIUPDATE: Create ota demo task failed\r\n");
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    if (g_ota_ctx_ptr != NULL) {
        if (g_ota_ctx_ptr->query_url != NULL) {
            sys_mfree(g_ota_ctx_ptr->query_url);
            g_ota_ctx_ptr->query_url = NULL;
        }
        sys_mfree(g_ota_ctx_ptr);
        g_ota_ctx_ptr = NULL;
    }

    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+CIUPDATE=<\"fw_url\">\r\n");
    AT_RSP_OK();
    return;
}

static void at_ota_demo_led_config(void)
{
    gpio_mode_set(AT_OTA_DEMO_LED_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, AT_OTA_DEMO_LED_GPIO_PIN);
    gpio_output_options_set(AT_OTA_DEMO_LED_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, AT_OTA_DEMO_LED_GPIO_PIN);
}

static void at_ota_demo_led_on(void)
{
    gpio_bit_set(AT_OTA_DEMO_LED_GPIO_PORT, AT_OTA_DEMO_LED_GPIO_PIN);
}

static void at_ota_demo_led_off(void)
{
    gpio_bit_reset(AT_OTA_DEMO_LED_GPIO_PORT, AT_OTA_DEMO_LED_GPIO_PIN);
}

static uint32_t at_ota_demo_led_status(void)
{
    return gpio_output_bit_get(AT_OTA_DEMO_LED_GPIO_PORT, AT_OTA_DEMO_LED_GPIO_PIN);
}

void at_ota_demo_recv_sub_topic_cb(void *inpub_arg, const char *data, uint16_t payload_length)
{
    const char *topic = data;

    if (strncmp(topic, ALI_ECS_SUB_TOPIC_OTA_VW553, strlen(ALI_ECS_SUB_TOPIC_OTA_VW553)) == 0) {
        if (g_ota_ctx_ptr == NULL || g_ota_ctx_ptr->state == OTA_ST_IDLE) { // OTA is not started
            g_ota_ctx_ptr = sys_zalloc(sizeof(ota_ctx_t));
            if (g_ota_ctx_ptr == NULL) {
                AT_TRACE("AT+CIUPDATE: sys_calloc for ota_demo_ctx_t fail\r\n");
                return;
            }
            snprintf(g_ota_ctx_ptr->fw_name, 8, "VW553");
        } else {
            AT_TRACE("OTA is in progress, please wait\r\n");
        }
    } else if (strncmp(topic, ALI_ECS_SUB_TOPIC_OTA_MUSIC, strlen(ALI_ECS_SUB_TOPIC_OTA_MUSIC)) == 0) {
        if (g_ota_ctx_ptr == NULL) { // OTA is not started
            g_ota_ctx_ptr = sys_zalloc(sizeof(ota_ctx_t));
            if (g_ota_ctx_ptr == NULL) {
                AT_TRACE("AT+CIUPDATE: sys_calloc for ota_demo_ctx_t fail\r\n");
            }
        }
        if (g_ota_ctx_ptr && g_ota_ctx_ptr->state == OTA_ST_IDLE) {
            snprintf(g_ota_ctx_ptr->fw_name, 8, "MUSIC");
        } else {
            AT_TRACE("OTA is in progress, please wait\r\n");
        }
    } else if (strncmp(topic, ALI_ECS_SUB_TOPIC_SYSTEM_RESET, strlen(ALI_ECS_SUB_TOPIC_SYSTEM_RESET)) == 0) {
        AT_RSP_START(16);
        AT_RSP("+IND_W=RESET\r\n");
        AT_RSP_IMMEDIATE();
        AT_RSP_FREE();
    } else if (strncmp(topic, ALI_ECS_SUB_TOPIC_SYSTEM_LED, strlen(ALI_ECS_SUB_TOPIC_SYSTEM_LED)) == 0) {
        AT_TRACE("subscribed LED topic recved\r\n");
    } else {
        AT_TRACE("unknown topic:%s\r\n", topic);
    }

    return;
}

void at_ota_demo_recv_sub_msg_cb(void *inpub_arg, const uint8_t *data, uint16_t payload_length, uint8_t flags, uint8_t retain)
{
    const char *msg = (const char *)data;
    char *json = NULL, *cmd = NULL;

    /* Extract cmd from msg */
    cmd = strstr(msg, "{\"cmd\":") + 8;
    if (cmd == NULL) {
        AT_TRACE("Invalid subscribed msg:%s\r\n", (const char *)data);
        return;
    }

    if (strncmp(cmd, "ota", 3) == 0) {     /* {"cmd": "ota", "url":"https://xxxxxx"} */
        if (g_ota_ctx_ptr != NULL && g_ota_ctx_ptr->state == OTA_ST_IDLE) {
            if (strstr(msg, "\"url\":") != NULL) {
                json = strstr(msg, "\"url\":\"") + 7;
                json[strcspn(json, "\"}]")] = 0;  // remove trailing "}]"
                at_fw_update(2, (char *[]){"AT+OTADEMO", json});
            } else {
                AT_TRACE("invalid ota msg format\r\n");
            }
        }
    } else if (strncmp(cmd, "reset", 5) == 0) { /* {"cmd":"reset"} */
        sys_ms_sleep(1000);
        SysTimer_SoftwareReset();
    } else if (strncmp(cmd, "led", 3) == 0) { /* {"cmd": "led", "on":true/false} */
        if (strstr(msg, "\"on\":") != NULL) {
            json = strstr(msg, "\"on\":") + 5;
            json[strcspn(json, "}")] = 0;  // remove trailing "}]"
            strncmp(json, "true", 4) == 0 ? at_ota_demo_led_on() : at_ota_demo_led_off();
        } else {
            AT_TRACE("invalid led msg format\r\n");
        }
    } else {
        AT_TRACE("unknown msg:%s\r\n", msg);
    }

    return;
}

static int at_mqtt_connect(int argc, char **argv)
{
    uint32_t port;
    uint8_t state, reconnect = 1;
    char *endptr = NULL, *host = NULL, *client_id = NULL, *username = NULL, *password = NULL;
    int ret;

    host = ALI_ECS_SERVER_HOST;
    if (strlen(host) > MQTT_HOST_MAX_LEN) {
        ret = AT_MQTT_HOST_IS_OVERLENGTH;
        AT_TRACE("invalid MQTT host, ERR CODE:0x%08x\r\n", ret);
        goto Error;
    }

    port = ALI_ECS_MQTT_PORT;
    if (port > MQTT_MAX_PORT) {
        ret = AT_MQTT_PORT_VALUE_IS_WRONG;
        AT_TRACE("invalid MQTT port, ERR CODE:0x%08x\r\n", ret);
        goto Error;
    }

    client_id = ALI_ECS_MQTT_CLIENT_ID;
    if (strlen(client_id) > MQTT_CLIENT_ID_LEN) {
        ret = AT_MQTT_CLIENT_ID_IS_OVERLENGTH;
        AT_TRACE("invalid MQTT client_id, ERR CODE:0x%08x\r\n", ret);
        goto Error;
    }
    username = ALI_ECS_MQTT_CLIENT_USERNAME;
    if (strlen(username) > MQTT_USERNAME_LEN) {
        ret = AT_MQTT_USERNAME_IS_OVERLENGTH;
        AT_TRACE("invalid MQTT username, ERR CODE:0x%08x\r\n", ret);
        goto Error;
    }
    password = ALI_ECS_MQTT_CLIENT_PASSWORD;
    if (strlen(password) > MQTT_PASSWORD_LEN) {
        ret = AT_MQTT_PASSWORD_IS_OVERLENGTH;
        AT_TRACE("invalid MQTT password, ERR CODE:0x%08x\r\n", ret);
        goto Error;
    }

    if (mqtt_client_id_set(client_id, strlen(client_id)) != 0) {
        ret = AT_MQTT_CLIENT_ID_READ_FAILED;
        AT_TRACE("MQTT: client id set failed\r\n");
        goto Error;
    }
    if (mqtt_client_user_set(username, strlen(username)) != 0) {
        ret = AT_MQTT_USERNAME_READ_FAILED;
        AT_TRACE("MQTT: user name set failed\r\n");
        goto Error;
    }
    if (mqtt_client_pass_set(password, strlen(password)) != 0) {
        ret = AT_MQTT_PASSWORD_READ_FAILED;
        AT_TRACE("MQTT: user password set failed\r\n");
        goto Error;
    }

    bool at_ota_enabled = true;
    if ((ret = at_mqtt_connect_server(host, port, reconnect, at_ota_enabled)) == 0) {
        uint32_t connect_time = sys_current_time_get();
        while (sys_current_time_get() - connect_time <= MQTT_LINK_TIME_LIMIT * 2) {
            mqtt_client_t *at_mqtt_client = mqtt_client_get();
            if (at_mqtt_client != NULL &&  mqtt_client_is_connected(at_mqtt_client)) {
                mqtt_set_inpub_callback(at_mqtt_client, at_ota_demo_recv_sub_topic_cb, at_ota_demo_recv_sub_msg_cb, client_user_info_get());
                AT_RSP_START(256);
                AT_RSP("+MQTTCONNECTED:0,%d,\"%s\",%d,%d\r\n", mqtt_scheme_get(), host, port, reconnect);
                AT_RSP_OK();
                AT_TRACE("MQTT: connect success\r\n");
                ret = 0;
                break;
            } else if (at_mqtt_client == NULL) {
                AT_TRACE("MQTT: connect failed\r\n");
                goto Error;
            }
            AT_TRACE("MQTT: connecting...\r\n");
            sys_ms_sleep(1000);
        }
    } else {
        AT_TRACE("MQTT: connect failed, ret:%d\r\n", ret);
        goto Error;
    }

    return 0;

Error:
    return ret;
}

void at_ota_demo_start(int argc, char **argv)
{
    int err = 0;
    AT_RSP_START(64);
    char *pub_msg = NULL, *endptr = NULL;
    int vif_idx = WIFI_VIF_INDEX_DEFAULT, msg_len = 128;
    struct wifi_vif_tag *wvif = vif_idx_to_wvif(vif_idx);
    int using_tls;

    if (argc != 2) {
        goto Error;
    }

    if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
        goto Usage;
    }

    using_tls = (uint32_t)strtoul((const char *)argv[1], &endptr, 0);

    if (using_tls > 0)
        mqtt_scheme_set(5);
    else
        mqtt_scheme_set(1);
/*
    while (wvif->sta.state != WIFI_STA_STATE_CONNECTED) {
        app_print("Wi-Fi connect with %s ...\r\n", SSID);
        err = wifi_management_connect(SSID, PASSWORD, 1);
        if (err != 0) {
            app_print("Wi-Fi connect failed (ret %d).\r\n", err);
            sys_ms_sleep(10000);
        }
    }
*/
    /* 1. connect to mqtt server, */
    err = at_mqtt_connect(0, NULL);
    if (err) {
        AT_TRACE("AT+OTADEMO: mqtt connect failed, err=%d\r\n", err);
        goto Error;
    }

    at_ota_demo_led_config();
    /* send OK RSP*/
    AT_RSP_OK();

    /* 2. subscribe OTA topic， wildcast char is '+' */
    at_mqtt_msg_sub(ALI_ECS_SUB_TOPIC_OTA_VW553, 0, 1);

    /* 3. subscribe RESET topic */
    at_mqtt_msg_sub(ALI_ECS_SUB_TOPIC_SYSTEM_RESET, 0, 1);

    /* 4. subscribe LED turn on/off topic */
    at_mqtt_msg_sub(ALI_ECS_SUB_TOPIC_SYSTEM_LED, 0, 1);

    /* 5. Publish WiFi status*/
    pub_msg = sys_zalloc(msg_len);
    if (pub_msg == NULL) {
        AT_TRACE("AT+OTADEMO: malloc pub_wifi_msg failed\r\n");
        return;
    }
    if (wvif->sta.state == WIFI_STA_STATE_CONNECTED) {
        snprintf(pub_msg, msg_len, "{\"connected\":1,\"ssid\":\"%s\", \"rssi\":%d}",
            wvif->sta.cfg.ssid,
            macif_vif_sta_rssi_get(WIFI_VIF_INDEX_DEFAULT));
        at_mqtt_msg_pub(ALI_ECS_PUB_TOPIC_WIFI_STATUS, pub_msg, strlen(pub_msg), 0, 0);
    } else {
        snprintf(pub_msg, msg_len, "{\"connected\":0}");
    }
    sys_ms_sleep(1000);

    sys_memset(pub_msg, 0, msg_len);
    if (at_ota_demo_led_status()) {
        snprintf(pub_msg, msg_len, "{\"on\":true}");
    } else {
        snprintf(pub_msg, msg_len, "{\"on\":false}");
    }
    at_mqtt_msg_pub(ALI_ECS_PUB_TOPIC_SYSTEM_LED_STATUS, pub_msg, strlen(pub_msg), 0, 0);


    sys_memset(pub_msg, 0, msg_len);

    snprintf(pub_msg, msg_len, "Image Version: %s%x.%x.%x.%03x\n",
                RE_CUSTOMER_NAME,
                (RE_IMG_VERSION >> 28),
                (RE_IMG_VERSION >> 20) & 0xFF,
                (RE_IMG_VERSION >> 12) & 0xFF,
                RE_IMG_VERSION & 0xFFF);

    at_mqtt_msg_pub(ALI_ECS_PUB_TOPIC_SYSTEM_VERSION, pub_msg, strlen(pub_msg), 0, 0);

    if (pub_msg)
        sys_mfree(pub_msg);

    return;
Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+OTADEMO\r\n");
    AT_RSP_OK();
    return;
}
#endif /* CONFIG_ATCMD_OTA_DEMO */