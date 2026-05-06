/*
 * client-coap.c -- LwIP example
 *
 * Copyright (C) 2013-2016 Christian Amsüss <chrysn@fsfe.org>
 * Copyright (C) 2018-2024 Jon Shallow <supjps-libcoap@jpshallow.com>
 * Copyright (c) 2024, GigaDevice Semiconductor Inc.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * This file is part of the CoAP library libcoap. Please see README for terms
 * of use.
 */
#include "app_cfg.h"
#include <string.h>
#include <stdint.h>
#include "client-coap.h"
#include "coap3/coap_internal.h"
#include "lwip/sockets.h"

#ifdef CONFIG_COAP

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

static coap_context_t *main_coap_context = NULL;
static coap_optlist_t *optlist = NULL;
static coap_session_t *session = NULL;

static int quit = 1;

static coap_response_t message_handler(coap_session_t *session,
                                    const coap_pdu_t *sent,
                                    const coap_pdu_t *received,
                                    const coap_mid_t id)
{
    const uint8_t *data;
    size_t len;
    size_t offset;
    size_t total;

    (void)session;
    (void)sent;
    (void)id;

    coap_pdu_code_t rcv_code = coap_pdu_get_code(received);

    if (COAP_RESPONSE_CLASS(rcv_code) == 2) {
        switch (rcv_code) {
        case COAP_RESPONSE_CODE_CREATED:                    // = COAP_RESPONSE_CODE(201),
            coap_log_info("CREATED\r\n");
            quit = 1;
            break;
        case COAP_RESPONSE_CODE_DELETED:                    // = COAP_RESPONSE_CODE(202),
            coap_log_info("DELETED\r\n");
            quit = 1;
            break;
        case COAP_RESPONSE_CODE_VALID:                      // = COAP_RESPONSE_CODE(203),
            coap_log_info("VALID\r\n");
            quit = 1;
            break;
        case COAP_RESPONSE_CODE_CHANGED:                    // = COAP_RESPONSE_CODE(204),
            coap_log_info("CHANGED\r\n");
            quit = 1;
            break;
        case COAP_RESPONSE_CODE_CONTENT:                    // = COAP_RESPONSE_CODE(205),
            if (coap_get_data_large(received, &len, &data, &offset, &total)) {
                coap_log_info("%*.*s", (int)len, (int)len, (const char *)data);
                if (len + offset == total) {
                    coap_log_info("\n");
                    quit = 1;
                }
            } else if (total == 0) {
                coap_log_info("get 0 byte data\r\n");
                quit = 1;
            }
            break;
        case COAP_RESPONSE_CODE_CONTINUE:                   // = COAP_RESPONSE_CODE(231),
            coap_log_info("CONTINUE\r\n");
            break;
        default:
            coap_log_info("unknown code:%x");
            quit = 1;
            break;
        }
    } else {
            coap_log_info("recv code:%x\r\n", rcv_code);
            quit = 1;
    }

    return COAP_RESPONSE_OK;
}

static int event_handler(coap_session_t *session COAP_UNUSED,
                        const coap_event_t event)
{
    switch (event) {
    case COAP_EVENT_DTLS_CLOSED:
    case COAP_EVENT_TCP_CLOSED:
    case COAP_EVENT_SESSION_CLOSED:
    case COAP_EVENT_OSCORE_DECRYPTION_FAILURE:
    case COAP_EVENT_OSCORE_NOT_ENABLED:
    case COAP_EVENT_OSCORE_NO_PROTECTED_PAYLOAD:
    case COAP_EVENT_OSCORE_NO_SECURITY:
    case COAP_EVENT_OSCORE_INTERNAL_ERROR:
    case COAP_EVENT_OSCORE_DECODE_ERROR:
    case COAP_EVENT_WS_PACKET_SIZE:
    case COAP_EVENT_WS_CLOSED:
        quit = 1;
        break;
    case COAP_EVENT_DTLS_CONNECTED:
    case COAP_EVENT_DTLS_RENEGOTIATE:
    case COAP_EVENT_DTLS_ERROR:
    case COAP_EVENT_TCP_CONNECTED:
    case COAP_EVENT_TCP_FAILED:
    case COAP_EVENT_SESSION_CONNECTED:
    case COAP_EVENT_SESSION_FAILED:
    case COAP_EVENT_PARTIAL_BLOCK:
    case COAP_EVENT_XMIT_BLOCK_FAIL:
    case COAP_EVENT_SERVER_SESSION_NEW:
    case COAP_EVENT_SERVER_SESSION_DEL:
    case COAP_EVENT_BAD_PACKET:
    case COAP_EVENT_MSG_RETRANSMITTED:
    case COAP_EVENT_WS_CONNECTED:
    case COAP_EVENT_KEEPALIVE_FAILURE:
    default:
        break;
    }
    return 0;
}

static void nack_handler(coap_session_t *session COAP_UNUSED,
                        const coap_pdu_t *sent COAP_UNUSED,
                        const coap_nack_reason_t reason,
                        const coap_mid_t id COAP_UNUSED)
{

    switch (reason) {
    case COAP_NACK_TOO_MANY_RETRIES:
    case COAP_NACK_NOT_DELIVERABLE:
    case COAP_NACK_RST:
    case COAP_NACK_TLS_FAILED:
        coap_log_err("cannot send CoAP pdu\n");
        quit = 1;
        break;
    case COAP_NACK_ICMP_ISSUE:
    default:
        break;
    }
    return;
}

static int
resolve_address(const char *host, const char *service, coap_address_t *dst,
                coap_proto_t *proto, int scheme_hint_bits) {

    coap_addr_info_t *addr_info;
    coap_str_const_t str_host;
    uint16_t port = service ? atoi(service) : 0;
    int ret = 0;

    str_host.s = (const uint8_t *)host;
    str_host.length = strlen(host);

    addr_info = coap_resolve_address_info(&str_host, port, port, port, port,
                                            AF_UNSPEC, scheme_hint_bits,
                                            COAP_RESOLVE_TYPE_REMOTE);
    if (addr_info) {
        ret = 1;
        *dst = addr_info->addr;
        *proto = addr_info->proto;
    }

    coap_free_address_info(addr_info);
    return ret;
}

void client_coap_init(struct coap_client_config* client_cfg)
{
    coap_pdu_t *pdu;
    coap_address_t dst;
    int len;
    coap_uri_t uri;
    char portbuf[8];
    unsigned char buf[100];
    int res;
    coap_proto_t proto;

    quit = 0;

    /* Initialize libcoap library */
    coap_startup();

    coap_set_log_level(client_cfg->log_level);

    /* Parse the URI */
    len = coap_split_uri((const unsigned char *)client_cfg->use_uri, strlen(client_cfg->use_uri), &uri);
    if (len != 0) {
        coap_log_warn("Failed to parse uri\r\n");
        goto exit;
    }

    if (uri.scheme != COAP_URI_SCHEME_COAP && uri.scheme != COAP_URI_SCHEME_COAPS) {
        coap_log_warn("Unsupported URI type\r\n");
        goto exit;
    }

    if (uri.scheme == COAP_URI_SCHEME_COAPS && !coap_dtls_is_supported()) {
        coap_log_warn("DTLS not supported\r\n");
        goto exit;
    }

    snprintf(portbuf, sizeof(portbuf), "%d", uri.port);
    snprintf((char *)buf, sizeof(buf), "%*.*s", (int)uri.host.length,
                     (int)uri.host.length, (const char *)uri.host.s);
    /* resolve destination address where server should be sent */
    len = resolve_address((const char *)buf, portbuf, &dst, &proto, 1 << uri.scheme);
    if (len == 0) {
        coap_log_warn("Failed to resolve address\r\n");
        goto exit;
    }

    main_coap_context = coap_new_context(NULL);
    if (main_coap_context == NULL) {
        coap_log_warn("Failed to initialize context\r\n");
        goto exit;
    }

    coap_context_set_block_mode(main_coap_context, COAP_BLOCK_USE_LIBCOAP);

  if (proto == COAP_PROTO_DTLS || proto == COAP_PROTO_TLS ||
      proto == COAP_PROTO_WSS) {
    static coap_dtls_cpsk_t dtls_psk;
    static char client_sni[256];

    memset(&dtls_psk, 0, sizeof(dtls_psk));
    dtls_psk.version = COAP_DTLS_CPSK_SETUP_VERSION;
    snprintf(client_sni, sizeof(client_sni), "%*.*s", (int)uri.host.length, (int)uri.host.length,
             uri.host.s);
    dtls_psk.client_sni = client_sni;
    dtls_psk.psk_info.identity.s = (const uint8_t *)client_cfg->use_id;
    dtls_psk.psk_info.identity.length = strlen(client_cfg->use_id);
    dtls_psk.psk_info.key.s = (const uint8_t *)client_cfg->use_psk;
    dtls_psk.psk_info.key.length = strlen(client_cfg->use_psk);

    session = coap_new_client_session_psk2(main_coap_context, NULL, &dst,
                                           COAP_PROTO_DTLS, &dtls_psk);
  } else {
    session = coap_new_client_session(main_coap_context, NULL, &dst,
                                      proto);
  }

    if(session == NULL) {
        coap_log_warn("Failed to create session\r\n");
        goto exit;
    }

    coap_register_response_handler(main_coap_context, message_handler);
    coap_register_event_handler(main_coap_context, event_handler);

    coap_register_nack_handler(main_coap_context, nack_handler);

    /* construct CoAP message */
    pdu = coap_pdu_init(client_cfg->pdu_type,
                        client_cfg->pdu_code,
                        coap_new_message_id(session),
                        coap_session_max_pdu_size(session));
    if (pdu == NULL) {
        coap_log_warn("Failed to create PDU\r\n");
        goto exit;
    }

    len = coap_uri_into_options(&uri, &dst, &optlist, 1, buf, sizeof(buf));
    if (len != 0) {
        coap_log_warn("Failed to create options\r\n");
        goto exit;
    }

    /* Add option list (which will be sorted) to the PDU */
    if (optlist) {
        res = coap_add_optlist_pdu(pdu, &optlist);
        if (res != 1) {
            coap_log_warn("Failed to add options to PDU\r\n");
            goto exit;
        }
    }

    if ((client_cfg->pdu_code == COAP_REQUEST_CODE_PUT) && (client_cfg->put_data != NULL)) {
        uint8_t buf[4];

        if (!coap_insert_option(pdu, COAP_OPTION_CONTENT_FORMAT,
                                coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_TEXT_PLAIN),
                                buf)) {
            goto exit;
        }
        res = coap_add_data(pdu, strlen((const char *)client_cfg->put_data), client_cfg->put_data);
        if (res != 1) {
            coap_log_warn("Failed to add data to PDU\r\n");
            goto exit;
        }
    }

    /* and send the PDU */
    if (coap_send(session, pdu) == COAP_INVALID_MID) {
        coap_log_warn("Failed to send PDU\r\n");
        goto exit;
    }

    return;

exit:
    quit = 1;
    return;
}

void client_coap_finished(void)
{
    if (optlist) {
        coap_delete_optlist(optlist);
        optlist = NULL;
    }
    if (session) {
        coap_session_release(session);
        session = NULL;
    }
    if (main_coap_context) {
        coap_free_context(main_coap_context);
        main_coap_context = NULL;
    }
    coap_cleanup();
}

int client_coap_poll(void)
{
    if (quit != 1) {
        coap_io_process(main_coap_context, 1000);
    }
    return quit;
}

#endif /* CONFIG_COAP */
