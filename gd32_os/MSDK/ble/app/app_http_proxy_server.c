/*!
    \file    app_http_proxy_server.c
    \brief   Http Proxy Service Server Application Module entry point

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

#include "ble_app_config.h"

#if (BLE_PROFILE_HPS_SERVER)
/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "wrapper_os.h"
#include "dbg_print.h"
#include "app_http_proxy_server.h"

#include "wifi_netif.h"
#include "wifi_export.h"
#include "macif_vif.h"

#include "wlan_config.h"
#include "mbedtls/config.h"
#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/error.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/ssl_ciphersuites.h"
#include "rom_export_mbedtls.h"
#include "rom_export.h"
#include "trng.h"

static const char app_rsa1_cli_key[] =
    "-----BEGIN RSA PRIVATE KEY-----\r\n" \
    "MIICXQIBAAKBgQDF1y6cWqlmASkxTUue2obcck3burDnDvBn5AplEZAdfAFzMndt\r\n" \
    "Au/zLyI05ujjYw1N0W1TNrdJY5XxkWpvDomacPtEnK+274OC58Q7HiEAh1SxeNgf\r\n" \
    "q4pvQ3esUwVu6Ls/vSFpfjpFeyKsk1ucXEfENdCEh+b+K/qkk7zF9AEBfwIDAQAB\r\n" \
    "AoGBAJMwUpc0xE8FkhYCAb6/qhIcYFyXesGM1cMVX75t4KBu/80qwLszsj1k1bgy\r\n" \
    "CxYRPXal1wZP8PECzC2bGGpjkG8tma19vFbIXOinJdiNj0HpqyR7uWJORZC26fYM\r\n" \
    "tX8MNEzqkV3SLaBRiQ8nElQy/IkSwpHzrBsO9TgN3GetjIuxAkEA5575sh2c3TQ5\r\n" \
    "hF/0xxw1HW4p+cZaiaBgLFypkk1mXyTUaFX9d2frz8Oe/pac4sR9lBnYmTyTRg/v\r\n" \
    "TfCGjYVNkwJBANqp/j4C4362JceT3bvROkw1hrxaX2mivhhBzmnA2Ebz4aEPjKUH\r\n" \
    "vpOPBGx4UxthIHmvrJ/DFzjJuuqbK01ND+UCQEzSrM0IB2RTExS14vE7iN53EJMY\r\n" \
    "2CS3vc5Y+aFd7Kt4Ar+MbeJx5IPnxU950xVfyKsbm3zP26UsWdoHAgnkgeMCQEM6\r\n" \
    "/Ran4LZ23orMZeJ3ZAtGcdS7nJZoGTZwFTzitByso3TXyRB8nxXTZTLMlBDY/hkr\r\n" \
    "8FF2tE8bh0LWzquHxBkCQQCHH37ie0ErnR9+71JkY4hM8qbo7plOkSDBoyZ/xsxg\r\n" \
    "1BylXma0s48nwJeYAOSvxVVw5oupoZgxzvL4oPHRzZgJ\r\n" \
    "-----END RSA PRIVATE KEY-----";

static const char app_rsa1_cli_crt[] =
    "-----BEGIN CERTIFICATE-----\r\n" \
    "MIICHTCCAYYCCQDcr9nMMGEhyzANBgkqhkiG9w0BAQsFADBTMQswCQYDVQQGEwJj\r\n" \
    "bjEQMA4GA1UECAwHamlhbmdzdTEPMA0GA1UEBwwGc3V6aG91MSEwHwYDVQQKDBhJ\r\n" \
    "bnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQwHhcNMTcxMjI3MDg0NTAwWhcNMjcxMjI1\r\n" \
    "MDg0NTAwWjBTMQswCQYDVQQGEwJjbjEQMA4GA1UECAwHamlhbmdzdTEPMA0GA1UE\r\n" \
    "BwwGc3V6aG91MSEwHwYDVQQKDBhJbnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQwgZ8w\r\n" \
    "DQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBAMXXLpxaqWYBKTFNS57ahtxyTdu6sOcO\r\n" \
    "8GfkCmURkB18AXMyd20C7/MvIjTm6ONjDU3RbVM2t0ljlfGRam8OiZpw+0Scr7bv\r\n" \
    "g4LnxDseIQCHVLF42B+rim9Dd6xTBW7ouz+9IWl+OkV7IqyTW5xcR8Q10ISH5v4r\r\n" \
    "+qSTvMX0AQF/AgMBAAEwDQYJKoZIhvcNAQELBQADgYEAZ/vXyB7vmQodNWKMDIfq\r\n" \
    "ZBpAyOnlWoh66eSVVp0CKH8+XwCI2KNbMnztAuvwOFxfjjvmXkcEIgR425hTq0n2\r\n" \
    "bAudp8yTi7bx7pNQpnUveoQqf2gPjvWttkBsmdmUDF40q0OLA9meYGD8ZrMxwaV4\r\n" \
    "2Tc+Zfb2TdIxgunYpj5F5E8=\r\n" \
    "-----END CERTIFICATE-----";


static const char app_baidu_ca_crt[] =
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

static const char *app_ca_cert_str = NULL;

static bool app_check_certs_cb(uint8_t conn_id, uint8_t *p_uri, uint16_t uri_len);
static bool app_check_network_cb(void);
static bool app_http_request_cb(ble_hps_req_info_t info);

#define HTTPS_SERVER_PORT "443"
#define HTTP_SERVER_PORT "80"
#define HTTP_PROTOCOL "HTTP/1.0\r\n"

/* HPS Application request information */
typedef struct
{
    uint8_t conn_id;
    uint8_t p_uri[513];
    uint16_t uri_len;

    uint8_t  p_headers[513];
    uint16_t headers_len;

    uint8_t  p_body[513];
    uint16_t body_len;

    ble_hps_op_code_t ctrl_op_code;
} app_hps_req_info_t;


/* HPS Application get operation state */
typedef enum
{
    HTTP_IDLE = 0,
    HTTP_STATUS_LINE = 1,
    HTTP_HEADER = 2,
    HTTP_BODY = 3,
} app_get_state_t;

static ble_hpss_callbacks_t callbacks = {
    app_check_certs_cb,
    app_check_network_cb,
    app_http_request_cb,
};

/*!
    \brief      Generate random data
    \param[in]  p_rng: The CTR_DRBG context
    \param[in]  output_len: length of random value want to get
    \param[out] output: pointer to the random value
    \retval     int: 0 if succeed, non-zero otherwise
*/
static int app_random(void *p_rng, unsigned char *output, size_t output_len)
{
    return random_get(output, output_len);
}

/*!
    \brief      Application debug function
    \param[in]  ctx: opaque context for the callback
    \param[in]  level: debug level
    \param[in]  file: file name
    \param[in]  line: line number
    \param[in]  str: message
    \param[out] none
    \retval     none
*/
static void app_debug(void *ctx, int level,
                      const char *file, int line,
                      const char *str)
{
    ((void) level);
    dbg_print(INFO, "%s:%04d: %s", file, line, str);
}

/*!
    \brief      Application verify function
    \param[in]  data: pointer to the input value
    \param[in]  crt: pointer to the X.509 certificates structures
    \param[in]  depth: depth vlaue
    \param[in]  flags: pointer to the flag value
    \param[out] none
    \retval     int: 0 if succeed, non-zero otherwise
*/
static int app_verify(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags)
{
    char buf[1024];
    ((void) data);

    dbg_print(INFO, "Verify requested for (Depth %d):\r\n", depth);
    mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "", crt);
    dbg_print(INFO, "%s", buf);

    if (((*flags) & MBEDTLS_X509_BADCERT_EXPIRED) != 0) {
        dbg_print(INFO, "server certificate has expired\r\n");
    }

    if (((*flags) & MBEDTLS_X509_BADCERT_REVOKED) != 0) {
        dbg_print(INFO, "  ! server certificate has been revoked\r\n");
    }

    if (((*flags) & MBEDTLS_X509_BADCERT_CN_MISMATCH) != 0) {
        dbg_print(INFO, "  ! CN mismatch\r\n");
    }

    if (((*flags) & MBEDTLS_X509_BADCERT_NOT_TRUSTED) != 0) {
        dbg_print(INFO, "  ! self-signed or not signed by a trusted CA\r\n");
    }

    if (((*flags) & MBEDTLS_X509_BADCRL_NOT_TRUSTED) != 0) {
        dbg_print(INFO, "  ! CRL not trusted\r\n");
    }

    if (((*flags) & MBEDTLS_X509_BADCRL_EXPIRED) != 0) {
        dbg_print(INFO, "  ! CRL expired\r\n");
    }

    if (((*flags) & MBEDTLS_X509_BADCERT_OTHER) != 0) {
        dbg_print(INFO, "  ! other (unknown) flag\r\n");
    }

    if (((*flags) & MBEDTLS_X509_BADCERT_BAD_KEY) != 0) {
        dbg_print(INFO, "  ! The certificate is signed with an unacceptable key\r\n");
    }

    if ((*flags) == 0) {
        dbg_print(INFO, "  Certificate verified without error flags\r\n");
    }

    return (0);
}

/*!
    \brief      Get http response code
    \param[in]  httpbuf: pointer to the input data buffer
    \param[out] none
    \retval     int32_t: status code
*/
static int32_t get_http_rsp_code(uint8_t *httpbuf)
{
    int32_t response_code;
    char *p_start = NULL;
    char *p_end = NULL;
    char re_code[10] = {0};

    sys_memset(re_code, 0, sizeof(re_code));

    p_start = strstr((char *)httpbuf, "HTTP/1.");
    if (NULL == p_start) {
        return -1;
    }
    p_start += strlen("HTTP/1.") + 1;
    while (*p_start == ' ') {
        p_start++;
    }

    p_end = strstr(p_start, " ");
    if (p_end) {
        if (p_end - p_start > sizeof(re_code)) {
            return -2;
        }

        sys_memcpy(re_code, p_start, (p_end - p_start));
    }

    response_code = atoi(re_code);
    return response_code;
}

/*!
    \brief      Get http header length
    \param[in]  httpbuf: pointer to the input data buffer
    \param[out] none
    \retval     int32_t: header length
*/
static int32_t get_http_hdr_len(uint8_t *httpbuf)
{
    char *p_start = NULL;
    char *p_end = NULL;
    int32_t headlen = 0;

    p_start = (char *)httpbuf;
    p_end = strstr((char *)httpbuf, "\r\n\r\n");
    if (p_end == NULL) {
        dbg_print(ERR, "Can't not find the http head!\r\n");
        return -1;
    }
    p_end = p_end + strlen("\r\n\r\n");
    headlen = (p_end - p_start);

    return headlen;
}

/*!
    \brief      Application check cert
    \param[in]  conn_id: connection index
    \param[in]  p_uri: pointer to the uri value
    \param[in]  uri_len: uri length
    \param[out] none
    \retval     bool: true if check successfully, otherwise false
*/
static bool app_check_certs_cb(uint8_t conn_id, uint8_t *p_uri, uint16_t uri_len)
{
    char *p_buf = sys_malloc(uri_len + 1);

    if (p_buf == NULL) {
        return false;
    }

    sys_memcpy(p_buf, p_uri, uri_len);
    p_buf[uri_len] = '\0';

    if (strstr(p_buf, "www.baidu.com") != NULL) {
        sys_mfree(p_buf);
        return true;
    }

    sys_mfree(p_buf);
    return false;
}

/*!
    \brief      Application check network
    \param[in]  none
    \param[out] none
    \retval     bool: true if check successfully, otherwise false
*/
static bool app_check_network_cb(void)
{
    struct mac_vif_status status;
    int i;

    for (i = 0; i < CFG_VIF_NUM; i++) {
        if (!macif_vif_status_get(i, &status)) {
            if (status.type == VIF_STA) {
                if (status.sta.active) {
                    return true;
                }
            }
        }
    }

    return false;
}

/*!
    \brief      Application get cert
    \param[in]  p_uri: pointer to the uri value
    \param[in]  uri_len: uri length
    \param[out] none
    \retval     bool: true if get successfully, otherwise false
*/
static bool get_cert(const uint8_t *p_uri, uint16_t uri_len)
{
    char *p_buf = sys_malloc(uri_len + 1);

    if (p_buf == NULL) {
        return false;
    }

    sys_memcpy(p_buf, p_uri, uri_len);
    p_buf[uri_len] = '\0';

    if (strstr(p_buf, "www.baidu.com") != NULL) {
        app_ca_cert_str = app_baidu_ca_crt;
        sys_mfree(p_buf);
        return true;
    }

    sys_mfree(p_buf);
    return false;
}

/*!
    \brief      http request operation task
    \param[in]  arg: argues
    \param[out] none
    \retval     none
*/
static void http_request_task(void *arg)
{
    app_hps_req_info_t *p_info = (app_hps_req_info_t *)arg;

    int ret, len;
    mbedtls_net_context server_fd;
    unsigned int flags;
    char buf[1540];
    bool need_cert = false;
    const char *p_host;
    const char *p_port = HTTPS_SERVER_PORT;
    uint8_t state = HTTP_IDLE;
    ble_hps_resp_info_t hps_resp;
    char *p_buf = buf;
    bool drop_data = false;
    uint16_t read_len = 0;

    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;

    mbedtls_x509_crt ca_cert;

    mbedtls_x509_crt cli_cert;
    mbedtls_pk_context cli_key;

    memset(&hps_resp, 0, sizeof(ble_hps_resp_info_t));
    hps_resp.conn_id = p_info->conn_id;

    /*
    * 0. Initialize the RNG and the session data
    */
    mbedtls_ecp_curve_val_init();
    mbedtls_net_init(&server_fd);
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_x509_crt_init(&ca_cert);
    mbedtls_x509_crt_init(&cli_cert);
    mbedtls_pk_init(&cli_key);


    /*
    * 1. Initialize certificates
    */
    if (p_info->ctrl_op_code >= HTTPS_GET_REQUEST && p_info->ctrl_op_code < HTTP_REQUEST_CANCEL) {
        need_cert = true;
        p_host = strstr((char *)p_info->p_uri, "https://") + strlen("https://");
        p_port = HTTPS_SERVER_PORT;
    } else if (p_info->ctrl_op_code > HTTP_RSVF && p_info->ctrl_op_code <= HTTP_DELETE_REQUEST) {
        need_cert = false;
        p_host = strstr((char *)p_info->p_uri, "http://") + strlen("http://");
        p_port = HTTP_SERVER_PORT;
    } else {
        goto exit;
    }

    if (need_cert) {
        if (get_cert(p_info->p_uri, p_info->uri_len)) {
            dbg_print(INFO, "  . Loading the CA root certificate ...");

            ret = mbedtls_x509_crt_parse(&ca_cert, (const unsigned char *)app_ca_cert_str,
                                         strlen(app_ca_cert_str) + 1);
            if (ret < 0) {
                dbg_print(ERR, " failed\r\n  !  mbedtls_x509_crt_parse returned -0x%x\r\n", -ret);
                goto exit;
            }
            dbg_print(INFO, " ok (%d skipped)\r\n", ret);
        } else {
            need_cert = false;
        }
    }

    if (need_cert) {
        dbg_print(INFO, "  . Loading the Client certificate ...");

        ret = mbedtls_x509_crt_parse(&cli_cert, (const unsigned char *)app_rsa1_cli_crt,
                                     strlen(app_rsa1_cli_crt) + 1);
        if (ret < 0) {
            printf(" failed\r\n  !  mbedtls_x509_crt_parse returned -0x%x\r\n", -ret);
            goto exit;
        }

        dbg_print(INFO, " ok (%d skipped)\r\n", ret);

        dbg_print(INFO, "  . Loading the Client key ...");

        ret = mbedtls_pk_parse_key(&cli_key, (const unsigned char *)app_rsa1_cli_key,
                                   strlen(app_rsa1_cli_key) + 1, NULL, 0);
        if (ret < 0) {
            dbg_print(ERR, " failed\r\n  !  mbedtls_pk_parse_key returned -0x%x\r\n", -ret);
            goto exit;
        }

        dbg_print(INFO, " ok (%d skipped)\r\n", ret);
    }

    /*
    * 2. Start the connection
    */
    dbg_print(NOTICE, "  . Connecting to tcp/%s/%s...", p_host, p_port);

    if ((ret = mbedtls_net_connect(&server_fd, p_host,
                                   p_port, MBEDTLS_NET_PROTO_TCP)) != 0) {
        dbg_print(ERR, " failed\r\n  ! mbedtls_net_connect returned %d\r\n", ret);
        goto exit;
    }

    dbg_print(NOTICE, " ok\r\n");

    /*
    * 3. Setup stuff
    */
    dbg_print(INFO, "  . Setting up the SSL/TLS structure...");

    if ((ret = mbedtls_ssl_config_defaults(&conf,
                                           MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        dbg_print(ERR, " failed\r\n  ! mbedtls_ssl_config_defaults returned %d\r\n", ret);
        goto exit;
    }

    dbg_print(INFO, " ok\r\n");

    /* OPTIONAL is not optimal for security,
      * but makes interop easier in this simplified example */

    mbedtls_ssl_conf_rng(&conf, app_random, NULL);
    mbedtls_ssl_conf_dbg(&conf, app_debug, NULL);

    if (need_cert) {
        mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
        mbedtls_ssl_conf_ca_chain(&conf, &ca_cert, NULL);
        mbedtls_ssl_conf_verify(&conf, app_verify, NULL);
        mbedtls_ssl_conf_own_cert(&conf, &cli_cert, &cli_key);
    } else {
        mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);
    }

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
        dbg_print(ERR, " failed\r\n  ! mbedtls_ssl_setup returned %d\r\n", ret);
        goto exit;
    }

    if ((ret = mbedtls_ssl_set_hostname(&ssl, p_host)) != 0) {
        dbg_print(ERR, " failed\r\n  ! mbedtls_ssl_set_hostname returned %d\r\n", ret);
        goto exit;
    }

    mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    /*
    * 4. Handshake
    */
    dbg_print(NOTICE, "  . Performing the SSL/TLS handshake...");

    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            dbg_print(ERR, " failed\r\n  ! mbedtls_ssl_handshake returned -0x%x\r\n", -ret);
            goto exit;
        }
    }

    dbg_print(NOTICE, " ok\r\n");

    /*
    * 5. Verify the server certificate
    */
    dbg_print(NOTICE, "  . Verifying peer X.509 certificate...");

    /* In real life, we probably want to bail out when ret != 0 */
    if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0) {
        char vrfy_buf[512];

        dbg_print(ERR, " failed\r\n");

        mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);

        dbg_print(NOTICE, "%s\r\n", vrfy_buf);
    } else {
        dbg_print(NOTICE, " ok %d\r\n", p_info->ctrl_op_code);
    }


    /*
    * 6. Write the HTTP request
    */
    dbg_print(INFO, "  > Write to server:");

    // FIX TODO headers and body should be in request
    switch (p_info->ctrl_op_code) {
    case HTTP_GET_REQUEST:
    case HTTPS_GET_REQUEST:
        len = sprintf((char *)buf, "%s%s %s%s", "GET ", p_info->p_uri, HTTP_PROTOCOL, "\r\n");
        break;
    case HTTP_HEAD_REQUEST:
    case HTTPS_HEAD_REQUEST:
        len = sprintf((char *)buf, "%s%s %s%s", "HEAD ", p_info->p_uri, HTTP_PROTOCOL, "\r\n");
        break;
    case HTTP_POST_REQUEST:
    case HTTPS_POST_REQUEST:
        len = sprintf((char *)buf, "%s%s %s%s%u%s%s", "POST ",  p_info->p_uri, HTTP_PROTOCOL,
                      "Content-Length:", p_info->body_len, "\r\nContent-Type:application/x-www-form-urlencoded\r\n\r\n",
                      p_info->p_body);
        break;
    default:
        len = sprintf((char *)buf, "%s%s %s%s", "HEAD ", p_info->p_uri, HTTP_PROTOCOL, "\r\n");
        break;
    }

    while ((ret = mbedtls_ssl_write(&ssl, (unsigned char *)buf, len)) <= 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            dbg_print(ERR, " failed\r\n  ! mbedtls_ssl_write returned %d\r\n", ret);
            goto exit;
        }
    }

    len = ret;
    dbg_print(INFO, " %d bytes written\r\n%s\r\n", len, (char *)buf);

    memset(buf, 0, sizeof(buf));
    state = HTTP_STATUS_LINE;
    do {
        len = BLE_HPS_VAL_MAX_LEN;

        ret = mbedtls_ssl_read(&ssl, (unsigned char *)p_buf, len);

        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
            continue;
        }

        if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
            break;
        }

        if (ret < 0) {
            dbg_print(ERR, "failed\r\n ! mbedtls_ssl_read returned %d\r\n", ret);
            break;
        }

        read_len += ret;
        if (state == HTTP_STATUS_LINE) {
            hps_resp.status_code = get_http_rsp_code((uint8_t *)p_buf);
            if (hps_resp.status_code < 0) {
                break;
            }

            state = HTTP_HEADER;
            p_buf = strstr((char *)buf, "\r\n");
            p_buf += strlen("\r\n");
            memcpy(buf, p_buf, read_len - (p_buf - buf));

            read_len -= (p_buf - buf);
            p_buf = buf + read_len;
        }

        if (state == HTTP_HEADER) {
            ret = get_http_hdr_len((uint8_t *)buf);

            if (ret == -1) {
                if (drop_data) {
                    if (read_len > 10) {
                        sys_memcpy(buf, buf + read_len - 10, 10);
                        p_buf = buf + 10;
                        read_len = 10;
                    }
                } else if (read_len > BLE_HPS_VAL_MAX_LEN) {
                    hps_resp.headers_len = BLE_HPS_VAL_MAX_LEN + 1;
                    drop_data = true;
                    hps_resp.p_headers = sys_malloc(hps_resp.headers_len + 1);
                    if (hps_resp.p_headers == NULL) {
                        hps_resp.headers_len = 0;
                        break;
                    }
                    sys_memcpy(hps_resp.p_headers, buf, hps_resp.headers_len);
                    hps_resp.p_headers[hps_resp.headers_len] = '\0';

                    sys_memcpy(buf, buf + read_len - 10, 10);
                    p_buf = buf + 10;
                    read_len = 10;
                }
            } else {
                if (drop_data) {
                    drop_data = false;
                } else {
                    hps_resp.headers_len = ret;
                    hps_resp.p_headers = sys_malloc(hps_resp.headers_len + 1);
                    if (hps_resp.p_headers == NULL) {
                        hps_resp.headers_len = 0;
                        break;
                    }
                    sys_memcpy(hps_resp.p_headers, buf, hps_resp.headers_len);
                    hps_resp.p_headers[hps_resp.headers_len] = '\0';
                }
                state = HTTP_BODY;
                p_buf = buf + ret;
                sys_memcpy(buf, p_buf, read_len - (p_buf - buf));
                read_len -= (p_buf - buf);
                p_buf = buf + read_len;
            }
        }

        if (state == HTTP_BODY) {
            if (read_len > BLE_HPS_VAL_MAX_LEN) {
                p_buf = buf + BLE_HPS_VAL_MAX_LEN + 1;
                read_len = BLE_HPS_VAL_MAX_LEN + 1;
            }
        }

        if (ret == 0) {
            break;
        }

    } while (1);

    mbedtls_ssl_close_notify(&ssl);

    if (state == HTTP_BODY) {
        if (read_len > BLE_HPS_VAL_MAX_LEN) {
            hps_resp.body_len = BLE_HPS_VAL_MAX_LEN + 1;
        } else {
            hps_resp.body_len = read_len;
        }

        hps_resp.p_body = sys_malloc(hps_resp.body_len + 1);
        if (hps_resp.p_body != NULL) {
            sys_memcpy(hps_resp.p_body, buf, hps_resp.body_len);
            hps_resp.p_body[hps_resp.body_len] = '\0';
        }
    }

exit:
    mbedtls_net_free(&server_fd);
    mbedtls_x509_crt_free(&ca_cert);
    mbedtls_x509_crt_free(&cli_cert);
    mbedtls_pk_free(&cli_key);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);

    ble_hpss_response_set(hps_resp);

    if (hps_resp.p_headers != NULL) {
        sys_mfree(hps_resp.p_headers);
    }

    if (hps_resp.p_body != NULL) {
        sys_mfree(hps_resp.p_body);
    }

    sys_mfree(p_info);
    sys_task_delete(NULL);
}

/*!
    \brief      Http application request callback
    \param[in]  info: request information
    \param[out] none
    \retval     bool: true if http request successfully, otherwise false
*/
static bool app_http_request_cb(ble_hps_req_info_t info)
{
    app_hps_req_info_t *p_info = sys_malloc(sizeof(app_hps_req_info_t));

    if (p_info == NULL) {
        dbg_print(ERR, "app_http_request_cb alloc info fail!\r\n");
        return false;
    }

    sys_memset(p_info, 0, sizeof(ble_hps_req_info_t));

    p_info->conn_id = info.conn_id;
    p_info->ctrl_op_code = info.ctrl_op_code;

    if (info.uri_len && info.p_uri) {
        sys_memcpy(p_info->p_uri, info.p_uri, info.uri_len);
        p_info->p_uri[info.uri_len] = '\0';
        p_info->uri_len = info.uri_len;
    }

    if (info.headers_len && info.p_headers) {
        sys_memcpy(p_info->p_headers, info.p_headers, info.headers_len);
        p_info->p_headers[info.headers_len] = '\0';
        p_info->headers_len = info.headers_len;
    }

    if (info.body_len && info.p_body) {
        sys_memcpy(p_info->p_body, info.p_body, info.body_len);
        p_info->p_body[info.body_len] = '\0';
        p_info->body_len = info.body_len;
    }

    if (sys_task_create_dynamic((const uint8_t *)"http request", 3584, OS_TASK_PRIORITY(2),
                                (task_func_t)http_request_task, p_info) == NULL) {
        dbg_print(ERR, "Create http request task failed\r\n");
        sys_mfree(p_info);
        return false;
    }

    return true;
}

/*!
    \brief      Init Http Proxy Service application module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_hpss_init(void)
{
    ble_hpss_init(callbacks);
}

#endif // (BLE_PROFILE_HPS_SERVER)
