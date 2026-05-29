/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "app_cfg.h"

#ifdef CONFIG_ALICLOUD_SUPPORT

#include <stdio.h>
#include <string.h>

#include "iot_import.h"

#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"


#include "mbedtls/config.h"

#include "mbedtls/error.h"
#include "mbedtls/ssl.h"
#include "mbedtls/net.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/pk.h"
#include "mbedtls/debug.h"
#include "mbedtls/platform.h"

#include "wrapper_os.h"

/**
 * @brief Set malloc/free function.
 *
 * @param [in] hooks: @n Specify malloc/free function you want to use
 *
 * @retval DTLS_SUCCESS : Success.
   @retval        other : Fail.
 * @see None.
 * @note If connect cloud by CoAP,you need realize it.
 */
DLL_HAL_API int HAL_DTLSHooks_set(dtls_hooks_t *hooks)
{
    hal_info("=>%s, TODO\r\n", __func__);
    return (int)1;
}

/**
 * @brief Establish a DSSL connection.
 *
 * @param [in] p_options: @n Specify paramter of DTLS
   @verbatim
           p_host : @n Specify the hostname(IP) of the DSSL server
             port : @n Specify the DSSL port of DSSL server
    p_ca_cert_pem : @n Specify the root certificate which is PEM format.
   @endverbatim
 * @return DSSL handle.
 * @see None.
 * @note If connect cloud by CoAP,you need realize it..
 */
DLL_HAL_API DTLSContext *HAL_DTLSSession_create(coap_dtls_options_t *p_options)
{
    hal_info("=>%s, TODO\r\n", __func__);
    return (DTLSContext *)1;
}

/**
 * @brief Destroy the specific DSSL connection.
 *
 * @param[in] context: @n Handle of the specific connection.
 *
 * @return The result of free dtls session
 * @retval DTLS_SUCCESS : Read success.
 * @retval DTLS_INVALID_PARAM : Invalid parameter.
 * @retval DTLS_INVALID_CA_CERTIFICATE : Invalid CA Certificate.
 * @retval DTLS_HANDSHAKE_IN_PROGRESS : Handshake in progress.
 * @retval DTLS_HANDSHAKE_FAILED : Handshake failed.
 * @retval DTLS_FATAL_ALERT_MESSAGE : Recv peer fatal alert message.
 * @retval DTLS_PEER_CLOSE_NOTIFY : The DTLS session was closed by peer.
 * @retval DTLS_SESSION_CREATE_FAILED : Create session fail.
 * @retval DTLS_READ_DATA_FAILED : Read data fail.
 * @note If connect cloud by CoAP,you need realize it.
 */
DLL_HAL_API unsigned int HAL_DTLSSession_free(DTLSContext *context)
{
    hal_info("=>%s, TODO\r\n", __func__);
    return (unsigned)1;
}

/**
 * @brief Read data from the specific DSSL connection with timeout parameter.
 *        The API will return immediately if len be received from the specific DSSL connection.
 *
 * @param [in] context @n A descriptor identifying a DSSL connection.
 * @param [in] p_data @n A pointer to a buffer to receive incoming data.
 * @param [in] p_datalen @n The length, in bytes, of the data pointed to by the 'p_data' parameter.
 * @param [in] timeout_ms @n Specify the timeout value in millisecond. In other words, the API block 'timeout_ms' millisecond maximumly.
 * @return The result of read data from DSSL connection
 * @retval DTLS_SUCCESS : Read success.
 * @retval DTLS_FATAL_ALERT_MESSAGE : Recv peer fatal alert message.
 * @retval DTLS_PEER_CLOSE_NOTIFY : The DTLS session was closed by peer.
 * @retval DTLS_READ_DATA_FAILED : Read data fail.
 * @note If connect cloud by CoAP,you need realize it..
 */
DLL_HAL_API unsigned int HAL_DTLSSession_read(DTLSContext *context,
                                              unsigned char *p_data,
                                              unsigned int *p_datalen,
                                              unsigned int timeout_ms)
{
    hal_info("=>%s, TODO\r\n", __func__);
    return (unsigned)1;
}

/**
 * @brief Write data into the specific DSSL connection.
 *
 * @param [in] context @n A descriptor identifying a connection.
 * @param [in] p_data @n A pointer to a buffer containing the data to be transmitted.
 * @param [in] p_datalen @n The length, in bytes, of the data pointed to by the 'p_data' parameter.
 * @retval DTLS_SUCCESS : Success.
   @retval        other : Fail.
 * @note If connect cloud by CoAP,you need realize it..
 */
DLL_HAL_API unsigned int HAL_DTLSSession_write(DTLSContext *context,
                                               const unsigned char *p_data,
                                               unsigned int *p_datalen)
{
    hal_info("=>%s, TODO\r\n", __func__);
    return (unsigned)1;
}

extern void *HAL_Malloc(uint32_t size);
extern void HAL_Free(void *ptr);

/**
 * Mbedtls related Start
 * */

#define SEND_TIMEOUT_SECONDS (10)
#define DEBUG_LEVEL (0)

typedef struct _TLSDataParams {
    mbedtls_ssl_context ssl;     /**< mbed TLS control context. */
    mbedtls_net_context fd;      /**< mbed TLS network context. */
    mbedtls_ssl_config conf;     /**< mbed TLS configuration context. */
    mbedtls_x509_crt cacertl;    /**< mbed TLS CA certification. */
    mbedtls_x509_crt clicert;    /**< mbed TLS Client certification. */
    mbedtls_pk_context pkey;     /**< mbed TLS Client key. */
} TLSDataParams_t, *TLSDataParams_pt;

static unsigned int _avRandom()
{
    uint32_t rand_num;

    sys_random_bytes_get((void *)&rand_num, 4);
    return (((unsigned int)rand_num << 16) + rand_num);
}

static int _ssl_random(void *p_rng, unsigned char *output, size_t output_len)
{
    uint32_t rnglen = output_len;
    uint8_t rngoffset = 0;

    while (rnglen > 0) {
        *(output + rngoffset) = (unsigned char)_avRandom();
        rngoffset++;
        rnglen--;
    }
    return 0;
}

static void _ssl_debug(void *ctx, int level, const char *file, int line,
                       const char *str)
{
    ((void)level);
    if (NULL != str)
        printf("%s:%04d: %s", file, line, str);
}

static int _real_confirm(int verify_result)
{
    hal_info("certificate verification result: 0x%02x\r\n", verify_result);

#if defined(FORCE_SSL_VERIFY)
    if ((verify_result & MBEDTLS_X509_BADCERT_EXPIRED) != 0)
    {
        hal_err("! fail ! ERROR_CERTIFICATE_EXPIRED\r\n");
        return -1;
    }

    if ((verify_result & MBEDTLS_X509_BADCERT_REVOKED) != 0)
    {
        hal_err("! fail ! server certificate has been revoked\r\n");
        return -1;
    }

    if ((verify_result & MBEDTLS_X509_BADCERT_CN_MISMATCH) != 0)
    {
        hal_err("! fail ! CN mismatch\r\n");
        return -1;
    }

    if ((verify_result & MBEDTLS_X509_BADCERT_NOT_TRUSTED) != 0)
    {
        hal_err("! fail ! self-signed or not signed by a trusted CA\r\n");
        return -1;
    }
#endif

    return 0;
}

static ssl_hooks_t g_ssl_hooks = {HAL_Malloc, HAL_Free};

static int _ssl_client_init(mbedtls_ssl_context *ssl,
                            mbedtls_net_context *tcp_fd,
                            mbedtls_ssl_config * conf,
                            mbedtls_x509_crt *crt509_ca, const char *ca_crt,
                            size_t ca_len, mbedtls_x509_crt *crt509_cli,
                            const char *cli_crt, size_t cli_len,
                            mbedtls_pk_context *pk_cli, const char *cli_key,
                            size_t key_len, const char *cli_pwd, size_t pwd_len)
{
    int ret = -1;
#ifdef CONFIG_SSL_TEST
    char *str = "0";
    //parse_cipher_suite_set(str);
#endif

    /*
     * 0. Initialize the RNG and the session data
     */
#if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold((int)DEBUG_LEVEL);
#endif

#ifndef CONFIG_TZ_ENABLED  //  MBL has called it yet
    //mbedtls_ecp_curve_val_init();
#endif
    mbedtls_net_init(tcp_fd);
    mbedtls_ssl_init(ssl);
    mbedtls_ssl_config_init(conf);
    mbedtls_x509_crt_init(crt509_ca);

    /*verify_source->trusted_ca_crt==NULL
     * 0. Initialize certificates
     */

    hal_info("Loading the CA root certificate ...\r\n");
    if (NULL != ca_crt)
    {
        if (0 != (ret = mbedtls_x509_crt_parse(
                      crt509_ca, (const unsigned char *)ca_crt, ca_len)))
        {
            hal_err(" failed ! x509parse_crt returned -0x%04x\r\n", -ret);
            return ret;
        }
    }
    hal_info(" Loading ok (%d skipped)\r\n", ret);

    /* Setup Client Cert/Key */
#if defined(MBEDTLS_X509_CRT_PARSE_C)
#if defined(MBEDTLS_CERTS_C)
    mbedtls_x509_crt_init(crt509_cli);
    mbedtls_pk_init(pk_cli);
#endif
    if (cli_crt != NULL && cli_key != NULL)
    {
#if defined(MBEDTLS_CERTS_C)
        hal_info("start prepare client cert .\r\n");
        ret = mbedtls_x509_crt_parse(crt509_cli, (const unsigned char *)cli_crt, cli_len);
#else
        {
            ret = 1;
            hal_err("MBEDTLS_CERTS_C not defined.\r\n");
        }
#endif
        if (ret != 0)
        {
            hal_err(" failed!  mbedtls_x509_crt_parse returned -0x%x\r\n",
                         -ret);
            return ret;
        }

#if defined(MBEDTLS_CERTS_C)
        hal_info("start mbedtls_pk_parse_key[%s]\r\n", cli_pwd);
        ret =
            mbedtls_pk_parse_key(pk_cli, (const unsigned char *)cli_key, key_len,
                                 (const unsigned char *)cli_pwd, pwd_len);
#else
        {
            ret = 1;
            hal_err("MBEDTLS_CERTS_C not defined.\r\n");
        }
#endif

        if (ret != 0)
        {
            hal_err(" failed\n  !  mbedtls_pk_parse_key returned -0x%x\r\n",
                         -ret);
            return ret;
        }
    }
#endif /* MBEDTLS_X509_CRT_PARSE_C */

    return 0;
}

static int _TLSConnectNetwork(TLSDataParams_t *pTlsData, const char *addr,
                              const char *port, const char *ca_crt,
                              size_t ca_crt_len, const char *client_crt,
                              size_t client_crt_len, const char *client_key,
                              size_t client_key_len, const char *client_pwd,
                              size_t client_pwd_len)
{
    int ret = -1;
    /*
     * 0. Init
     */
    if (0 != (ret = _ssl_client_init(
                  &(pTlsData->ssl), &(pTlsData->fd), &(pTlsData->conf),
                  &(pTlsData->cacertl), ca_crt, ca_crt_len, &(pTlsData->clicert),
                  client_crt, client_crt_len, &(pTlsData->pkey), client_key,
                  client_key_len, client_pwd, client_pwd_len))) {
        hal_err(" failed ! ssl_client_init returned -0x%04x\r\n", -ret);
        return ret;
    }

    /*
     * 1. Start the connection
     */
    hal_info("Connecting to /%s/%s...\r\n", addr, port);
    if (0 != (ret = mbedtls_net_connect(&(pTlsData->fd), addr, port,
                                        MBEDTLS_NET_PROTO_TCP))) {
        hal_err(" failed ! net_connect returned -0x%04x\r\n", -ret);
        return ret;
    }
    hal_info(" ok\r\n");

    /*
     * 2. Setup stuff
     */
    hal_info("  . Setting up the SSL/TLS structure\r\n");
    if ((ret = mbedtls_ssl_config_defaults(
             &(pTlsData->conf), MBEDTLS_SSL_IS_CLIENT,
             MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        hal_err(" failed! mbedtls_ssl_config_defaults returned %d\r\n", ret);
        return ret;
    }

    mbedtls_ssl_conf_max_version(&pTlsData->conf, MBEDTLS_SSL_MAJOR_VERSION_3,
                                 MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_min_version(&pTlsData->conf, MBEDTLS_SSL_MAJOR_VERSION_3,
                                 MBEDTLS_SSL_MINOR_VERSION_3);

    hal_info(" ok\r\n");

    /* OPTIONAL is not optimal for security, but makes interop easier in this
     * simplified example */
    if (ca_crt != NULL) {
#if defined(FORCE_SSL_VERIFY)
        mbedtls_ssl_conf_authmode(&(pTlsData->conf),
                                  MBEDTLS_SSL_VERIFY_REQUIRED);
#else
        mbedtls_ssl_conf_authmode(&(pTlsData->conf),
                                  MBEDTLS_SSL_VERIFY_OPTIONAL);
#endif
    }
    else {
        mbedtls_ssl_conf_authmode(&(pTlsData->conf), MBEDTLS_SSL_VERIFY_NONE);
    }

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    mbedtls_ssl_conf_ca_chain(&(pTlsData->conf), &(pTlsData->cacertl), NULL);

    if ((ret = mbedtls_ssl_conf_own_cert(
             &(pTlsData->conf), &(pTlsData->clicert), &(pTlsData->pkey))) != 0) {
        hal_err(" failed\n  ! mbedtls_ssl_conf_own_cert returned %d\r\n",
                     ret);
        return ret;
    }
#endif

#if defined(CONFIG_TZ_ENABLED)
    mbedtls_ssl_conf_rng( &(pTlsData->conf), mbedtls_random_nsc, NULL);
#else
    mbedtls_ssl_conf_rng(&(pTlsData->conf), _ssl_random, NULL);
#endif
    mbedtls_ssl_conf_dbg(&(pTlsData->conf), _ssl_debug, NULL);
    /* mbedtls_ssl_conf_dbg( &(pTlsData->conf), _ssl_debug, stdout ); */

    if ((ret = mbedtls_ssl_setup(&(pTlsData->ssl), &(pTlsData->conf))) != 0) {
        hal_err("failed! mbedtls_ssl_setup returned %d\r\n", ret);
        return ret;
    }
    mbedtls_ssl_set_hostname(&(pTlsData->ssl), addr);
    mbedtls_ssl_set_bio(&(pTlsData->ssl), &(pTlsData->fd), mbedtls_net_send,
                        mbedtls_net_recv, mbedtls_net_recv_timeout);

    /*
     * 4. Handshake
     */
    hal_info("Performing the SSL/TLS handshake...\r\n");
    while ((ret = mbedtls_ssl_handshake(&(pTlsData->ssl))) != 0) {
        if ((ret != MBEDTLS_ERR_SSL_WANT_READ) &&
            (ret != MBEDTLS_ERR_SSL_WANT_WRITE)) {
            hal_err("failed  ! mbedtls_ssl_handshake returned -0x%04x\r\n",
                         -ret);
            return ret;
        }
    }
    hal_info(" ok\r\n");
    /*
     * 5. Verify the server certificate
     */
    hal_info("  . Verifying peer X.509 certificate..\r\n");
    if (0 != (ret = _real_confirm(
                  mbedtls_ssl_get_verify_result(&(pTlsData->ssl))))) {
        hal_err(" failed  ! verify result not confirmed.\r\n");
        return ret;
    }
    hal_info(" Verifying OK\r\n");
    /* n->my_socket = (int)((n->tlsdataparams.fd).fd); */
    /* WRITE_IOT_DEBUG_LOG("my_socket=%d", n->my_socket); */

    return 0;
}

static int _network_ssl_read(TLSDataParams_t *pTlsData, char *buffer, int len,
                             int timeout_ms)
{
    uint32_t readLen = 0;
    static int net_status = 0;
    int ret = -1;

    mbedtls_ssl_conf_read_timeout(&(pTlsData->conf), timeout_ms);
    while (readLen < len) {
        ret = mbedtls_ssl_read(&(pTlsData->ssl),
                               (unsigned char *)(buffer + readLen),
                               (len - readLen));
        if (ret > 0) {
            readLen += ret;
            net_status = 0;
        }
        else if (ret == 0) {
        /* if ret is 0 and net_status is -2, indicate the connection is closed during last call */
            return (net_status == -2) ? net_status : readLen;
        }
        else {
            if (MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY == ret) {
                hal_err("ssl recv error, ssl closed: code = %d\r\n", ret);
                net_status = -2; /* connection is closed */
                break;
            }
            else if ((MBEDTLS_ERR_SSL_TIMEOUT == ret) ||
                     (MBEDTLS_ERR_SSL_CONN_EOF == ret) ||
                     (MBEDTLS_ERR_SSL_SESSION_TICKET_EXPIRED == ret) ||
                     (MBEDTLS_ERR_SSL_WANT_READ == ret) ||
                     (MBEDTLS_ERR_SSL_NON_FATAL == ret)) {
                /* read already complete */
                /* if call mbedtls_ssl_read again, it will return 0 (means EOF) */
                return readLen;
            }
            else {
                hal_err("ssl recv error, ssl error: code = %d\r\n", ret);
                net_status = -1;
                return -1; /* Connection error */
            }
        }
    }

    return (readLen > 0) ? readLen : net_status;
}

static int _network_ssl_write(TLSDataParams_t *pTlsData, const char *buffer,
                              int len, int timeout_ms)
{
    uint32_t writtenLen = 0;
    int ret = -1;

    while (writtenLen < len) {
        ret = mbedtls_ssl_write(&(pTlsData->ssl),
                                (unsigned char *)(buffer + writtenLen),
                                (len - writtenLen));
        if (ret > 0) {
            writtenLen += ret;
            continue;
        }
        else if (ret == 0) {
            hal_err("ssl write timeout\r\n");
            return 0;
        }
        else {
            hal_err("ssl write fail, code=%d\r\n", ret);
            return -1; /* Connnection error */
        }
    }

    return writtenLen;
}

static void _network_ssl_disconnect(TLSDataParams_t *pTlsData)
{
    mbedtls_ssl_close_notify(&(pTlsData->ssl));
    mbedtls_net_free(&(pTlsData->fd));
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    mbedtls_x509_crt_free(&(pTlsData->cacertl));
    if ((pTlsData->pkey).pk_info != NULL)
    {
        hal_info("need release client crt&key\r\n");
#if defined(MBEDTLS_CERTS_C)
        mbedtls_x509_crt_free(&(pTlsData->clicert));
        mbedtls_pk_free(&(pTlsData->pkey));
#endif
    }
#endif
    mbedtls_ssl_free(&(pTlsData->ssl));
    mbedtls_ssl_config_free(&(pTlsData->conf));
    hal_info("ssl_disconnect\r\n");
}

int32_t HAL_SSL_Destroy(uintptr_t handle)
{
    if ((uintptr_t)NULL == handle) {
        hal_err("handle is NULL\r\n");
        return 0;
    }

    _network_ssl_disconnect((TLSDataParams_t *)handle);
    HAL_Free((void *)handle);
    return 0;
}

DLL_HAL_API uintptr_t HAL_SSL_Establish(
            _IN_ const char *host,
            _IN_ uint16_t port,
            _IN_ const char *ca_crt,
            _IN_ size_t ca_crt_len)
{
    char port_str[6];
    TLSDataParams_pt pTlsData;

    pTlsData = HAL_Malloc(sizeof(TLSDataParams_t));
    if (NULL == pTlsData)
        return (uintptr_t)NULL;

    sys_memset(pTlsData, 0x0, sizeof(TLSDataParams_t));

    snprintf(port_str, sizeof(port_str), "%u", port);

    if (0 != _TLSConnectNetwork(pTlsData, host, port_str, ca_crt, ca_crt_len,
                                NULL, 0, NULL, 0, NULL, 0)) {
        _network_ssl_disconnect(pTlsData);
        HAL_Free((void *)pTlsData);
        return (uintptr_t)NULL;
    }

    return (uintptr_t)pTlsData;
}

int HAL_SSLHooks_set(ssl_hooks_t *hooks)
{
    if (hooks == NULL || hooks->malloc == NULL || hooks->free == NULL)
    {
        return -1;
    }

    g_ssl_hooks.malloc = hooks->malloc;
    g_ssl_hooks.free = hooks->free;

    return 0;
}

static void HAL_utils_ms_to_timeval(int timeout_ms, struct timeval *tv)
{
    tv->tv_sec = timeout_ms / 1000;
    tv->tv_usec = (timeout_ms - (tv->tv_sec * 1000)) * 1000;
}

DLL_HAL_API int32_t HAL_SSL_Read(_IN_ uintptr_t handle, _OU_ char *buf, _OU_ int len, _IN_ int timeout_ms)
{
    return _network_ssl_read((TLSDataParams_t *)handle, buf, len, timeout_ms);

}

DLL_HAL_API int32_t HAL_SSL_Write(_IN_ uintptr_t handle, _IN_ const char *buf, _IN_ int len, _IN_ int timeout_ms)
{
    return _network_ssl_write((TLSDataParams_t *)handle, buf, len, timeout_ms);
}

#endif /* CONFIG_ALICLOUD_SUPPORT */
