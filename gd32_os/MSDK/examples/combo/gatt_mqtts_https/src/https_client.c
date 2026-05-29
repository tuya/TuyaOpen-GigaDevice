/*!
    \file    https_client.c
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
#include "dbg_print.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "trng.h"

#define HTTP_PROTOCOL   "HTTP/1.1\r\n"

#define HTTPS_BUF_LEN   1025

static uint8_t https_buf[HTTPS_BUF_LEN];
static mbedtls_ssl_context https_ssl;
static mbedtls_net_context https_server_fd;
static mbedtls_ssl_config conf;
static mbedtls_x509_crt ca_cert;

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
    app_print("%s:%04d: %s", file, line, str );
}

static int my_verify(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags)
{
    char buf[1024];
    ((void) data);

    app_print("Verify requested for (Depth %d):\r\n", depth);
    mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "", crt);
    app_print("%s", buf);

    if(((*flags) & MBEDTLS_X509_BADCERT_EXPIRED) != 0)
        app_print("server certificate has expired\r\n");

    if(((*flags) & MBEDTLS_X509_BADCERT_REVOKED) != 0)
        app_print("  ! server certificate has been revoked\r\n");

    if(((*flags) & MBEDTLS_X509_BADCERT_CN_MISMATCH) != 0)
        app_print("  ! CN mismatch\r\n");

    if(((*flags) & MBEDTLS_X509_BADCERT_NOT_TRUSTED) != 0)
        app_print("  ! self-signed or not signed by a trusted CA\r\n");

    if(((*flags) & MBEDTLS_X509_BADCRL_NOT_TRUSTED) != 0)
        app_print("  ! CRL not trusted\r\n");

    if(((*flags) & MBEDTLS_X509_BADCRL_EXPIRED) != 0)
        app_print("  ! CRL expired\r\n");

    if(((*flags) & MBEDTLS_X509_BADCERT_OTHER) != 0)
        app_print("  ! other (unknown) flag\r\n");

    if(((*flags) & MBEDTLS_X509_BADCERT_BAD_KEY) != 0)
        app_print("  ! The certificate is signed with an unacceptable key\r\n");

    if((*flags) == 0)
        app_print("  Certificate verified without error flags\r\n");

    return(0);
}

int https_client_start(char *host, char *port, const uint8_t *cert, uint32_t cert_len)
{
    int ret, len;
    unsigned int flags;

    /*
    * 0. Initialize the configuration and the session data
    */
    mbedtls_debug_set_threshold( 0 );
    mbedtls_ecp_curve_val_init();
    mbedtls_net_init( &https_server_fd );
    mbedtls_ssl_init( &https_ssl );
    mbedtls_ssl_config_init( &conf );
    mbedtls_x509_crt_init( &ca_cert );
    app_print( "  . Seeding the random number generator...\r\n" );

    /*
    * 1. Initialize certificates
    */
    app_print( "  . Loading the CA root certificate ..." );
    ret = mbedtls_x509_crt_parse(&ca_cert, cert, cert_len);
    if( ret < 0 ){
        app_print( " failed\r\n  !  mbedtls_x509_crt_parse returned -0x%x\r\n", -ret );
        goto error;
    }
    app_print( " ok (%d skipped)\r\n", ret );

    /*
    * 2. Start the connection
    */
    app_print( "  . Connecting to tcp/%s/%s...", host, port );
    if( ( ret = mbedtls_net_connect( &https_server_fd, host,
                    port, MBEDTLS_NET_PROTO_TCP ) ) != 0 )
    {
        app_print( " failed\r\n  ! mbedtls_net_connect returned %d\r\n", ret );
        goto error;
    }
    app_print( " ok\r\n" );

    /*
    * 3. Setup stuff
    */
    app_print( "  . Setting up the SSL/TLS structure..." );
    if( ( ret = mbedtls_ssl_config_defaults( &conf,
        MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_STREAM,
        MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
    {
        app_print( " failed\r\n  ! mbedtls_ssl_config_defaults returned %d\r\n", ret );
        goto error;
    }
    mbedtls_ssl_conf_rng( &conf, my_random, NULL);
    mbedtls_ssl_conf_dbg( &conf, my_debug, NULL );
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&conf, &ca_cert, NULL);
    mbedtls_ssl_conf_verify(&conf, my_verify, NULL);

    if( ( ret = mbedtls_ssl_setup( &https_ssl, &conf ) ) != 0 )
    {
        app_print( " failed\r\n  ! mbedtls_ssl_setup returned %d\r\n", ret );
        goto error;
    }
    if( ( ret = mbedtls_ssl_set_hostname( &https_ssl, host ) ) != 0 )
    {
        app_print( " failed\r\n  ! mbedtls_ssl_set_hostname returned %d\r\n", ret );
        goto error;
    }
    mbedtls_ssl_set_bio( &https_ssl, &https_server_fd, mbedtls_net_send, mbedtls_net_recv, NULL );
    app_print( " ok\r\n" );

    /*
    * 4. Handshake
    */
    app_print( "  . Performing the SSL/TLS handshake..." );
    while( ( ret = mbedtls_ssl_handshake( &https_ssl ) ) != 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            app_print( " failed\r\n  ! mbedtls_ssl_handshake returned -0x%x\r\n", -ret );
            goto error;
        }
    }
    app_print( " ok\r\n" );

    /*
    * 5. Verify the server certificate
    */
    app_print( "  . Verifying peer X.509 certificate..." );

    /* In real life, we probably want to bail out when ret != 0 */
    if( ( flags = mbedtls_ssl_get_verify_result( &https_ssl ) ) != 0 )
    {
        char vrfy_buf[512];

        app_print( " failed\r\n" );

        mbedtls_x509_crt_verify_info( vrfy_buf, sizeof( vrfy_buf ), "  ! ", flags );

        app_print( "%s\r\n", vrfy_buf );
    }
    else
        app_print( " ok\r\n");

    return ret;

error:
    if( ret != 0 )
    {
        char error_buf[100];
        mbedtls_strerror( ret, error_buf, 100 );
        app_print("Last error was: %d - %s\r\n", ret, error_buf );
    }
    mbedtls_net_free( &https_server_fd );
    mbedtls_x509_crt_free( &ca_cert );
    mbedtls_ssl_free( &https_ssl );
    mbedtls_ssl_config_free( &conf );
    return ret;
}

int https_client_get(char *host)
{
    uint32_t len;
    int ret = 0;

    /*
    * 1. Write the HTTP request
    */
    sys_memset(https_buf, 0, sizeof(https_buf));

    len = co_snprintf((char *)https_buf, HTTPS_BUF_LEN, "%s%s %s""Host: %s\r\n""%s",
                    "HEAD https://", host, HTTP_PROTOCOL, host, "\r\n");

    app_print("[WIFI HTTPS] << Write to server:");
    while ((ret = mbedtls_ssl_write(&https_ssl, https_buf, len)) <= 0)
    {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            app_print(" failed\r\n  ! mbedtls_ssl_write returned %d\r\n", ret);
            goto exit;
        }
    }
    len = ret;
    app_print(" %d bytes written\r\n%s\r\n", len, (char *)https_buf);

    /*
    * 2. Read the HTTP response
    */
    do
    {
        len = HTTPS_BUF_LEN - 1;
        sys_memset(https_buf, 0, HTTPS_BUF_LEN);
        app_print("[WIFI HTTPS] >> Read from server:");
        ret = mbedtls_ssl_read(&https_ssl, https_buf, len);

        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
            continue;

        if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
            break;

        if (ret < 0)
        {
            app_print("failed\r\n  ! mbedtls_ssl_read returned %d\r\n", ret);
            break;
        }

        if (ret == 0)
        {
            app_print("\r\nEOF\r\n");
            break;
        }

        len = ret;
        //app_print(" %d bytes read\r\n%s", len, (char *)https_buf);
        app_print(" %d bytes read\r\n", len);
        break;
    } while (1);
    // app_print(">> Read End\r\n");

exit:
    if( ret < 0 )
    {
        char error_buf[100];
        mbedtls_strerror( ret, error_buf, 100 );
        app_print("Last error was: %d (%s)\r\n", ret, error_buf );
        return ret;
    }
    return 0;
}

void https_client_stop(void)
{
    mbedtls_ssl_close_notify( &https_ssl );
    mbedtls_x509_crt_free( &ca_cert );
    mbedtls_net_free( &https_server_fd );
    mbedtls_ssl_config_free( &conf );
    mbedtls_ssl_free( &https_ssl );
}
