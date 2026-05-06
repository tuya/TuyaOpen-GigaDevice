/*
 * EAP server/peer: Shared EAP definitions
 * Copyright (c) 2004-2014, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

/*!
    \file    wpas_eap.h
    \brief   Header file for wpas eap.

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

#ifndef WPAS_EAP_H
#define WPAS_EAP_H

#include "wpas_buf.h"
#include "wpas_eap_supp_sm.h"

/*==============================================================*/
/* eap_common/eap_def.h */
/*==============================================================*/
/* RFC 3748 - Extensible Authentication Protocol (EAP) */

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif /* _MSC_VER */

struct eap_hdr {
    uint8_t code;
    uint8_t identifier;
    uint16_t length; /* including code and identifier; network byte order */
    /* followed by length-4 octets of data */
} STRUCT_PACKED;

#ifdef _MSC_VER
#pragma pack(pop)
#endif /* _MSC_VER */

enum { EAP_CODE_REQUEST = 1, EAP_CODE_RESPONSE = 2, EAP_CODE_SUCCESS = 3,
       EAP_CODE_FAILURE = 4, EAP_CODE_INITIATE = 5, EAP_CODE_FINISH = 6 };

/* EAP Request and Response data begins with one octet Type. Success and
 * Failure do not have additional data. */

/* Type field in EAP-Initiate and EAP-Finish messages */
enum eap_erp_type {
    EAP_ERP_TYPE_REAUTH_START = 1,
    EAP_ERP_TYPE_REAUTH = 2,
};

/* ERP TV/TLV types */
enum eap_erp_tlv_type {
    EAP_ERP_TLV_KEYNAME_NAI = 1,
    EAP_ERP_TV_RRK_LIFETIME = 2,
    EAP_ERP_TV_RMSK_LIFETIME = 3,
    EAP_ERP_TLV_DOMAIN_NAME = 4,
    EAP_ERP_TLV_CRYPTOSUITES = 5,
    EAP_ERP_TLV_AUTHORIZATION_INDICATION = 6,
    EAP_ERP_TLV_CALLED_STATION_ID = 128,
    EAP_ERP_TLV_CALLING_STATION_ID = 129,
    EAP_ERP_TLV_NAS_IDENTIFIER = 130,
    EAP_ERP_TLV_NAS_IP_ADDRESS = 131,
    EAP_ERP_TLV_NAS_IPV6_ADDRESS = 132,
};

/* ERP Cryptosuite */
enum eap_erp_cryptosuite {
    EAP_ERP_CS_HMAC_SHA256_64 = 1,
    EAP_ERP_CS_HMAC_SHA256_128 = 2,
    EAP_ERP_CS_HMAC_SHA256_256 = 3,
};

/*
 * EAP Method Types as allocated by IANA:
 * http://www.iana.org/assignments/eap-numbers
 */
enum eap_type {
    EAP_TYPE_NONE = 0,
    EAP_TYPE_IDENTITY = 1 /* RFC 3748 */,
    EAP_TYPE_NOTIFICATION = 2 /* RFC 3748 */,
    EAP_TYPE_NAK = 3 /* Response only, RFC 3748 */,
    EAP_TYPE_MD5 = 4, /* RFC 3748 */
    EAP_TYPE_OTP = 5 /* RFC 3748 */,
    EAP_TYPE_GTC = 6, /* RFC 3748 */
    EAP_TYPE_TLS = 13 /* RFC 2716 */,
    EAP_TYPE_LEAP = 17 /* Cisco proprietary */,
    EAP_TYPE_SIM = 18 /* RFC 4186 */,
    EAP_TYPE_TTLS = 21 /* RFC 5281 */,
    EAP_TYPE_AKA = 23 /* RFC 4187 */,
    EAP_TYPE_PEAP = 25 /* draft-josefsson-pppext-eap-tls-eap-06.txt */,
    EAP_TYPE_MSCHAPV2 = 26 /* draft-kamath-pppext-eap-mschapv2-00.txt */,
    EAP_TYPE_TLV = 33 /* draft-josefsson-pppext-eap-tls-eap-07.txt */,
    EAP_TYPE_TNC = 38 /* TNC IF-T v1.0-r3; note: tentative assignment;
               * type 38 has previously been allocated for
               * EAP-HTTP Digest, (funk.com) */,
    EAP_TYPE_FAST = 43 /* RFC 4851 */,
    EAP_TYPE_PAX = 46 /* RFC 4746 */,
    EAP_TYPE_PSK = 47 /* RFC 4764 */,
    EAP_TYPE_SAKE = 48 /* RFC 4763 */,
    EAP_TYPE_IKEV2 = 49 /* RFC 5106 */,
    EAP_TYPE_AKA_PRIME = 50 /* RFC 5448 */,
    EAP_TYPE_GPSK = 51 /* RFC 5433 */,
    EAP_TYPE_PWD = 52 /* RFC 5931 */,
    EAP_TYPE_EKE = 53 /* RFC 6124 */,
    EAP_TYPE_TEAP = 55 /* RFC 7170 */,
    EAP_TYPE_EXPANDED = 254 /* RFC 3748 */
};


/* SMI Network Management Private Enterprise Code for vendor specific types */
enum {
    EAP_VENDOR_IETF = 0,
    EAP_VENDOR_MICROSOFT = 0x000137 /* Microsoft */,
    EAP_VENDOR_WFA = 0x00372A /* Wi-Fi Alliance (moved to WBA) */,
    EAP_VENDOR_HOSTAP = 39068 /* hostapd/wpa_supplicant project */,
    EAP_VENDOR_WFA_NEW = 40808 /* Wi-Fi Alliance */
};

#define EAP_VENDOR_UNAUTH_TLS EAP_VENDOR_HOSTAP
#define EAP_VENDOR_TYPE_UNAUTH_TLS 1

#define EAP_VENDOR_WFA_UNAUTH_TLS 13

#define EAP_MSK_LEN 64
#define EAP_EMSK_LEN 64
#define EAP_EMSK_NAME_LEN 8
#define ERP_MAX_KEY_LEN 64

/*==============================================================*/
/* eap_common/eap_common.h */
/*==============================================================*/
struct erp_tlvs {
    const uint8_t *keyname;
    const uint8_t *domain;

    uint8_t keyname_len;
    uint8_t domain_len;
};

struct eap_context {
    const char *ca_cert;
    const char *client_key;
    const char *client_key_password;
    const char *identity;
    uint8_t identity_len;
    const char *client_cert;
    const char *phase1;
};

int eap_hdr_len_valid(const struct wpabuf *msg, size_t min_payload);
const uint8_t * eap_hdr_validate(int vendor, enum eap_type eap_type,
                            const struct wpabuf *msg, size_t *plen);
struct wpabuf * eap_msg_alloc(int vendor, enum eap_type type,
                  size_t payload_len, uint8_t code, uint8_t identifier);
void eap_update_len(struct wpabuf *msg);
uint8_t eap_get_id(const struct wpabuf *msg);
enum eap_type eap_get_type(const struct wpabuf *msg);

int eap_register_methods(void);
void eap_unregister_methods(void);

void wpas_eap_result_cb(struct eapol_sm *esm, enum eapol_supp_result result);
void wpas_eap_notify_eapol_done(struct eapol_sm *esm);
int wpas_eap_rx_eapol(struct eapol_sm *esm, uint8_t *data, uint32_t data_len);
int wpas_eap_send(struct eapol_sm *esm, int type, const uint8_t *buf, size_t len);
void wpas_eap_start(struct eapol_sm *sm);

#endif /* WPAS_EAP_H */
