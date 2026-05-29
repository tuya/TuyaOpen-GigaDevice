/*!
    \file    https_client_main.c
    \brief   the example of HTTPS client in station mode

    \version 2024-03-22, V1.0.0, firmware for GD32VW55x
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

#include <stdint.h>
#include <stdio.h>
#include "app_cfg.h"
#include "gd32vw55x_platform.h"
#include "wifi_management.h"
#include "wifi_init.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "trng.h"

#define SSID            "GL_6019"
#define PASSWORD        "12345678" // NULL // ""

#define SERVER_PORT     "443"
#define SERVER_NAME     "www.baidu.com"
#define HTTP_PROTOCOL   "HTTP/1.1\r\n"

/*
Getting the server CA certs can refer to chapter 3.8.1 in
document AN185 GD32VW553 Network Application Development Guide.docx.
*/
static const char baidu_ca_crt[]=
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

static int my_random(void *p_rng, unsigned char *output, size_t output_len)
{
    random_get(output, output_len);
    return 0;
}

static void my_debug( void *ctx, int level,
                      const char *file, int line,
                      const char *str )
{
    ((void) level);
    printf("%s:%04d: %s", file, line, str );
}

static int my_verify(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags)
{
    char buf[1024];
    ((void) data);

    printf("Verify requested for (Depth %d):\r\n", depth);
    mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "", crt);
    printf("%s", buf);

    if(((*flags) & MBEDTLS_X509_BADCERT_EXPIRED) != 0)
        printf("server certificate has expired\r\n");

    if(((*flags) & MBEDTLS_X509_BADCERT_REVOKED) != 0)
        printf("  ! server certificate has been revoked\r\n");

    if(((*flags) & MBEDTLS_X509_BADCERT_CN_MISMATCH) != 0)
        printf("  ! CN mismatch\r\n");

    if(((*flags) & MBEDTLS_X509_BADCERT_NOT_TRUSTED) != 0)
        printf("  ! self-signed or not signed by a trusted CA\r\n");

    if(((*flags) & MBEDTLS_X509_BADCRL_NOT_TRUSTED) != 0)
        printf("  ! CRL not trusted\r\n");

    if(((*flags) & MBEDTLS_X509_BADCRL_EXPIRED) != 0)
        printf("  ! CRL expired\r\n");

    if(((*flags) & MBEDTLS_X509_BADCERT_OTHER) != 0)
        printf("  ! other (unknown) flag\r\n");

    if(((*flags) & MBEDTLS_X509_BADCERT_BAD_KEY) != 0)
        printf("  ! The certificate is signed with an unacceptable key\r\n");

    if((*flags) == 0)
        printf("  Certificate verified without error flags\r\n");

    return(0);
}

static void https_client_test(void)
{
    int ret, len;
    mbedtls_net_context server_fd;
    unsigned int flags;
    unsigned char buf[1025];

    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt ca_cert;

    /*
    * 0. Initialize the configuration and the session data
    */
    mbedtls_debug_set_threshold( 0 );
    mbedtls_ecp_curve_val_init();
    mbedtls_net_init( &server_fd );
    mbedtls_ssl_init( &ssl );
    mbedtls_ssl_config_init( &conf );
    mbedtls_x509_crt_init( &ca_cert );
    printf( "  . Seeding the random number generator...\r\n" );

    /*
    * 1. Initialize certificates
    */
    printf( "  . Loading the CA root certificate ..." );
    ret = mbedtls_x509_crt_parse(&ca_cert, (const unsigned char *)baidu_ca_crt, strlen(baidu_ca_crt) + 1);
    if( ret < 0 ){
        printf( " failed\r\n  !  mbedtls_x509_crt_parse returned -0x%x\r\n", -ret );
        goto exit;
    }
    printf( " ok (%d skipped)\r\n", ret );

    /*
    * 2. Start the connection
    */
    printf( "  . Connecting to tcp/%s/%s...", SERVER_NAME, SERVER_PORT );
    if( ( ret = mbedtls_net_connect( &server_fd, SERVER_NAME,
        SERVER_PORT, MBEDTLS_NET_PROTO_TCP ) ) != 0 )
    {
        printf( " failed\r\n  ! mbedtls_net_connect returned %d\r\n", ret );
        goto exit;
    }
    printf( " ok\r\n" );

    /*
    * 3. Setup stuff
    */
    printf( "  . Setting up the SSL/TLS structure..." );
    if( ( ret = mbedtls_ssl_config_defaults( &conf,
        MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_STREAM,
        MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
    {
        printf( " failed\r\n  ! mbedtls_ssl_config_defaults returned %d\r\n", ret );
        goto exit;
    }
    mbedtls_ssl_conf_rng( &conf, my_random, NULL);
    mbedtls_ssl_conf_dbg( &conf, my_debug, NULL );
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&conf, &ca_cert, NULL);
    mbedtls_ssl_conf_verify(&conf, my_verify, NULL);

    if( ( ret = mbedtls_ssl_setup( &ssl, &conf ) ) != 0 )
    {
        printf( " failed\r\n  ! mbedtls_ssl_setup returned %d\r\n", ret );
        goto exit;
    }
    if( ( ret = mbedtls_ssl_set_hostname( &ssl, SERVER_NAME ) ) != 0 )
    {
        printf( " failed\r\n  ! mbedtls_ssl_set_hostname returned %d\r\n", ret );
        goto exit;
    }
    mbedtls_ssl_set_bio( &ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL );
    printf( " ok\r\n" );

    /*
    * 4. Handshake
    */
    printf( "  . Performing the SSL/TLS handshake..." );
    while( ( ret = mbedtls_ssl_handshake( &ssl ) ) != 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            printf( " failed\r\n  ! mbedtls_ssl_handshake returned -0x%x\r\n", -ret );
            goto exit;
        }
    }
    printf( " ok\r\n" );

    /*
    * 5. Verify the server certificate
    */
    printf( "  . Verifying peer X.509 certificate..." );

    /* In real life, we probably want to bail out when ret != 0 */
    if( ( flags = mbedtls_ssl_get_verify_result( &ssl ) ) != 0 )
    {
        char vrfy_buf[512];

        printf( " failed\r\n" );

        mbedtls_x509_crt_verify_info( vrfy_buf, sizeof( vrfy_buf ), "  ! ", flags );

        printf( "%s\r\n", vrfy_buf );
    }
    else
        printf( " ok\r\n");

    /*
    * 6. Write the HTTP request
    */
    printf("  > Write to server:");
    len = sprintf((char *)buf, "%s%s %s""Host: %s\r\n""%s", "HEAD https://",SERVER_NAME,HTTP_PROTOCOL,SERVER_NAME,"\r\n");
    while ((ret = mbedtls_ssl_write(&ssl, buf, len)) <= 0)
    {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            printf(" failed\r\n  ! mbedtls_ssl_write returned %d\r\n", ret);
            goto exit;
        }
    }
    len = ret;
    printf(" %d bytes written\r\n%s\r\n", len, (char *)buf);

    /*
    * 7. Read the HTTP response
    */
    printf("  < Read from server:\r\n");
    do
    {
        len = sizeof(buf) - 1;
        memset(buf, 0, sizeof(buf));
        ret = mbedtls_ssl_read(&ssl, buf, len);

        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
            continue;

        if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
            break;

        if (ret < 0)
        {
            printf("failed\r\n  ! mbedtls_ssl_read returned %d\r\n", ret);
            break;
        }

        if (ret == 0)
        {
            printf("\r\nEOF\r\n");
            break;
        }

        len = ret;
        printf(" %d bytes read\r\n%s", len, (char *)buf);
    } while (1);
    mbedtls_ssl_close_notify( &ssl );

exit:
    if( ret != 0 )
    {
        char error_buf[100];
        mbedtls_strerror( ret, error_buf, 100 );
        printf("Last error was: %d - %s\r\n", ret, error_buf );
    }
    mbedtls_net_free( &server_fd );
    mbedtls_x509_crt_free( &ca_cert );
    mbedtls_ssl_free( &ssl );
    mbedtls_ssl_config_free( &conf );
    printf("\r\nExit ssl client task: stack high water mark = %d\r\n", sys_stack_free_get(NULL));
}

static void https_client_task(void *param)
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
    * 3. Start HTTPS client
    */
    printf("Start HTTPS client.\r\n");
    https_client_test();

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

    sys_task_create_dynamic((const uint8_t *)"https client", 4096, OS_TASK_PRIORITY(0), https_client_task, NULL);

    sys_os_start();

    for ( ; ; );
}
