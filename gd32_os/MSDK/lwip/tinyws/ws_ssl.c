/*!
    \file    ws_ssl.c
    \brief   websockets ssl wrapper for GD32VW55x SDK

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

#include "mbedtls/version.h"
#define MBEDTLS_VER_2_17_0       0x02110000
#if (!defined(MBEDTLS_VERSION_NUMBER) || MBEDTLS_VERSION_NUMBER != MBEDTLS_VER_2_17_0)
#include "mbedtls/mbedtls_config.h"
#else
#include "mbedtls/ssl_internal.h"
#include "rom_export_mbedtls.h"
#include "rom_export.h"
#endif

#include "mbedtls/ssl.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/psa_util.h"
#include "ws_ssl.h"
#include "trng.h"
#include <string.h>

static const char ws_ca_pem[]=
"-----BEGIN CERTIFICATE-----\r\n" \
"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\r\n" \
"-----END CERTIFICATE-----\r\n";

struct wss_tls{
    mbedtls_ssl_context ctx;
    mbedtls_ssl_config conf;
    mbedtls_net_context net_ctx;
#if TLS_VERIFY_SERVER_REQUIRED
    mbedtls_x509_crt ca_cert;
#endif
};


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

void *wss_tls_connect(int *sock , char *host, int port)
{

    int ret;
    struct wss_tls *tls =NULL;
    char sport[5];
    tls = (struct wss_tls *) sys_malloc(sizeof(struct wss_tls));

    if (tls) {
        mbedtls_ssl_context *ssl = &tls->ctx;
        mbedtls_ssl_config *conf = &tls->conf;
        mbedtls_net_context *net_ctx = &tls->net_ctx;
#if TLS_VERIFY_SERVER_REQUIRED
        mbedtls_x509_crt *ca_cert_ptr = &tls->ca_cert;
#endif
        sys_memset(tls, 0, sizeof(struct wss_tls));

        net_ctx->fd = *sock;
        sys_memset(sport, 0, 5);
        sprintf(sport, "%d", port);

        if((ret = mbedtls_net_connect(net_ctx, host, sport, MBEDTLS_NET_PROTO_TCP)) != 0){
            printf("mbedtls_net_connect error %d\r\n", ret);
            goto exit;
        }

        *sock = net_ctx->fd;
        mbedtls_ssl_init(ssl);
        mbedtls_ssl_config_init(conf);
        mbedtls_ssl_set_bio(ssl, net_ctx, mbedtls_net_send, mbedtls_net_recv, NULL);

#if TLS_VERIFY_SERVER_REQUIRED
        mbedtls_x509_crt_init(ca_cert_ptr);
        if ((ret = mbedtls_x509_crt_parse(ca_cert_ptr, (const unsigned char *)ws_ca_pem, strlen(ws_ca_pem) + 1)) != 0) {
            printf("mbedtls_x509_crt_parse error %d\r\n", ret);
            goto exit;
        }
#endif


         if ((ret = mbedtls_ssl_config_defaults(conf,
                MBEDTLS_SSL_IS_CLIENT,
                MBEDTLS_SSL_TRANSPORT_STREAM,
                MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
            printf("mbedtls_ssl_config_defaults error %d\n", ret);
            goto exit;
        }

#if TLS_VERIFY_SERVER_REQUIRED
        mbedtls_ssl_conf_authmode(conf, MBEDTLS_SSL_VERIFY_REQUIRED);
        mbedtls_ssl_conf_ca_chain(conf, ca_cert_ptr, NULL);
#else
        mbedtls_ssl_conf_authmode(conf, MBEDTLS_SSL_VERIFY_NONE);
#endif
        mbedtls_ssl_conf_rng(conf, my_random, NULL);
        mbedtls_ssl_conf_dbg(conf, my_debug, NULL );

        if((ret = mbedtls_ssl_setup(ssl, conf)) != 0) {
            printf("mbedtls_ssl_setup error %d\r\n", ret);
            goto exit;
        }

        if((ret = mbedtls_ssl_set_hostname(ssl, host)) != 0) {
            printf("mbedtls_ssl_set_hostname error %d\r\n", ret);
            goto exit;
        }
    } else {
        printf("ssl mem alloc failed\r\n");
        ret = -1;
        goto exit;
    }
exit:
    if(ret && tls){
        mbedtls_net_free(&tls->net_ctx);
        mbedtls_ssl_free(&tls->ctx);
        mbedtls_ssl_config_free(&tls->conf);
#if TLS_VERIFY_SERVER_REQUIRED
        mbedtls_x509_crt_free(&tls->ca_cert);
#endif
        sys_mfree(tls);
        tls = NULL;
    }
    return (void *) tls;
}

int wss_tls_handshake(void *t)
{
    struct wss_tls *tls = (struct wss_tls *) t;

    int ret = 0;

    if((ret = mbedtls_ssl_handshake(&tls->ctx)) != 0) {
        printf("mbedtls_ssl_handshake error %d\r\n", ret);
        ret = -1;
    } else {
        printf("mbedtls_sslUse ciphersuite %s\r\n", mbedtls_ssl_get_ciphersuite(&tls->ctx));
        uint32_t flags = mbedtls_ssl_get_verify_result(&tls->ctx);
        if (flags == 0) {
            printf("mbedtls_ssl_get_verify_result successfully\r\n");
            ret = 0;
        } else {
            char vrfy_buf[512];
            mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "", flags);
            printf("Certificate verification failed: %s\r\n", vrfy_buf);
            ret = -1;
        }
    }

    return ret;
}

void wss_tls_close(void *t, int *s)
{
    struct wss_tls *tls = (struct wss_tls *) t;
    if (tls == NULL) {
        return;
    }

    if(tls)
        mbedtls_ssl_close_notify(&tls->ctx);

    if(*s != -1){
        mbedtls_net_free(&tls->net_ctx);
        *s = -1;
    }
    mbedtls_ssl_free(&tls->ctx);
#if TLS_VERIFY_SERVER_REQUIRED
    mbedtls_x509_crt_free(&tls->ca_cert);
#endif
    if(tls)
        mbedtls_ssl_config_free(&tls->conf);
#if defined(MBEDTLS_PSA_CRYPTO_C) && (MBEDTLS_VERSION_NUMBER != 0x02110000)
    mbedtls_psa_crypto_free();
#endif
    sys_mfree(tls);
    tls = NULL;
}

int wss_tls_write(void *t, uint8_t *buf, int len)
{
    int ret;
    struct wss_tls *tls = (struct wss_tls *) t;

    ret = mbedtls_ssl_write(&tls->ctx, (unsigned char const*)buf, len);
    if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
        ret = 0;
    return ret;
}

int wss_tls_read(void *t, uint8_t *buf, int buf_len)
{
    int ret;
    struct wss_tls *tls = (struct wss_tls *) t;

    ret = mbedtls_ssl_read(&tls->ctx, (unsigned char*)buf, buf_len);
    if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE
            || ret == MBEDTLS_ERR_NET_RECV_FAILED)
        ret =0;

    return ret;
}
