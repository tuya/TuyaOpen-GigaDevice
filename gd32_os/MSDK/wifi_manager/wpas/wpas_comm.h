/*
 * wpa_supplicant/hostapd / common helper functions, etc.
 * Copyright (c) 2002-2007, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

/*!
    \file    wpas_comm.h
    \brief   Header file for wpas common.

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

#ifndef _WPAS_COMM_H_
#define _WPAS_COMM_H_

#include <stddef.h>
#include "stdint.h"
#include "dlist.h"
#include "systime.h"

#ifndef __BYTE_ORDER
#define __BYTE_ORDER    __LITTLE_ENDIAN
#endif

#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#endif /* __LITTLE_ENDIAN */

#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN    4321
#endif /* __BIG_ENDIAN */

#ifndef __must_check
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define __must_check __attribute__((__warn_unused_result__))
#else
#define __must_check
#endif /* __GNUC__ */
#endif /* __must_check */

/*
 * Definitions for sparse validation
 * (http://kernel.org/pub/linux/kernel/people/josh/sparse/)
 */
#ifdef __CHECKER__
#define __force __attribute__((force))
#undef __bitwise
#define __bitwise __attribute__((bitwise))
#else
#define __force
#undef __bitwise
#define __bitwise
#endif
typedef uint16_t __bitwise be16;
typedef uint16_t __bitwise le16;
typedef uint32_t __bitwise be32;
typedef uint32_t __bitwise le32;

#ifndef bswap_16
#define bswap_16(a) ((((uint16_t) (a) << 8) & 0xff00) | (((uint16_t) (a) >> 8) & 0xff))
#endif

#ifndef bswap_32
#define bswap_32(a) ((((uint32_t) (a) << 24) & 0xff000000) | \
                     (((uint32_t) (a) << 8) & 0xff0000) | \
                     (((uint32_t) (a) >> 8) & 0xff00) | \
                     (((uint32_t) (a) >> 24) & 0xff))
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define le_to_host16(n) ((__force uint16_t) (le16) (n))
#define host_to_le16(n) ((__force le16) (uint16_t) (n))
#define be_to_host16(n) bswap_16((__force uint16_t) (be16) (n))
#define host_to_be16(n) ((__force be16) bswap_16((n)))
#define le_to_host32(n) ((__force uint32_t) (le32) (n))
#define host_to_le32(n) ((__force le32) (uint32_t) (n))
#define be_to_host32(n) bswap_32((__force uint32_t) (be32) (n))
#define host_to_be32(n) ((__force be32) bswap_32((n)))
#elif __BYTE_ORDER == __BIG_ENDIAN
#define le_to_host16(n) bswap_16(n)
#define host_to_le16(n) bswap_16(n)
#define be_to_host16(n) (n)
#define host_to_be16(n) (n)
#define le_to_host32(n) bswap_32(n)
#define host_to_le32(n) bswap_32(n)
#define be_to_host32(n) (n)
#define host_to_be32(n) (n)
#else
#error Could not determine CPU byte order
#endif

static inline int sys_snprintf_error(size_t size, int res)
{
    return res < 0 || (unsigned int) res >= size;
}

static inline int is_zero_ether_addr(const uint8_t *a)
{
    return !(a[0] | a[1] | a[2] | a[3] | a[4] | a[5]);
}

/* Macros for handling unaligned memory accesses */

static inline uint16_t WPA_GET_BE16(const uint8_t *a)
{
    return (a[0] << 8) | a[1];
}

static inline void WPA_PUT_BE16(uint8_t *a, uint16_t val)
{
    a[0] = val >> 8;
    a[1] = val & 0xff;
}

static inline uint16_t WPA_GET_LE16(const uint8_t *a)
{
    return (a[1] << 8) | a[0];
}

static inline void WPA_PUT_LE16(uint8_t *a, uint16_t val)
{
    a[1] = val >> 8;
    a[0] = val & 0xff;
}

static inline uint32_t WPA_GET_BE24(const uint8_t *a)
{
    return (a[0] << 16) | (a[1] << 8) | a[2];
}

static inline void WPA_PUT_BE24(uint8_t *a, uint32_t val)
{
    a[0] = (val >> 16) & 0xff;
    a[1] = (val >> 8) & 0xff;
    a[2] = val & 0xff;
}

static inline uint32_t WPA_GET_BE32(const uint8_t *a)
{
    return ((uint32_t) a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3];
}

static inline void WPA_PUT_BE32(uint8_t *a, uint32_t val)
{
    a[0] = (val >> 24) & 0xff;
    a[1] = (val >> 16) & 0xff;
    a[2] = (val >> 8) & 0xff;
    a[3] = val & 0xff;
}

static inline uint32_t WPA_GET_LE32(const uint8_t *a)
{
    return ((uint32_t) a[3] << 24) | (a[2] << 16) | (a[1] << 8) | a[0];
}

static inline void WPA_PUT_LE32(uint8_t *a, uint32_t val)
{
    a[3] = (val >> 24) & 0xff;
    a[2] = (val >> 16) & 0xff;
    a[1] = (val >> 8) & 0xff;
    a[0] = val & 0xff;
}

static inline uint64_t WPA_GET_BE64(const uint8_t *a)
{
    return (((uint64_t) a[0]) << 56) | (((uint64_t) a[1]) << 48) |
        (((uint64_t) a[2]) << 40) | (((uint64_t) a[3]) << 32) |
        (((uint64_t) a[4]) << 24) | (((uint64_t) a[5]) << 16) |
        (((uint64_t) a[6]) << 8) | ((uint64_t) a[7]);
}

static inline void WPA_PUT_BE64(uint8_t *a, uint64_t val)
{
    a[0] = val >> 56;
    a[1] = val >> 48;
    a[2] = val >> 40;
    a[3] = val >> 32;
    a[4] = val >> 24;
    a[5] = val >> 16;
    a[6] = val >> 8;
    a[7] = val & 0xff;
}

static inline uint64_t WPA_GET_LE64(const uint8_t *a)
{
    return (((uint64_t) a[7]) << 56) | (((uint64_t) a[6]) << 48) |
        (((uint64_t) a[5]) << 40) | (((uint64_t) a[4]) << 32) |
        (((uint64_t) a[3]) << 24) | (((uint64_t) a[2]) << 16) |
        (((uint64_t) a[1]) << 8) | ((uint64_t) a[0]);
}

static inline void WPA_PUT_LE64(uint8_t *a, uint64_t val)
{
    a[7] = val >> 56;
    a[6] = val >> 48;
    a[5] = val >> 40;
    a[4] = val >> 32;
    a[3] = val >> 24;
    a[2] = val >> 16;
    a[1] = val >> 8;
    a[0] = val & 0xff;
}
#define RSN_SELECTOR(a, b, c, d)                                          \
        ((((uint32_t) (a)) << 24) | (((uint32_t) (b)) << 16) | (((uint32_t) (c)) << 8) | \
        (uint32_t) (d))

#define RSN_SELECTOR_PUT(a, val)    WPA_PUT_BE32((uint8_t *) (a), (val))
#define RSN_SELECTOR_GET(a)         WPA_GET_BE32((const uint8_t *) (a))

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif
#ifndef ETH_HLEN
#define ETH_HLEN 14
#endif
#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif
#ifndef ETH_P_ALL
#define ETH_P_ALL 0x0003
#endif
#ifndef ETH_P_IP
#define ETH_P_IP 0x0800
#endif
#ifndef ETH_P_80211_ENCAP
#define ETH_P_80211_ENCAP 0x890d /* TDLS comes under this category */
#endif
#ifndef ETH_P_PAE
#define ETH_P_PAE 0x888E /* Port Access Entity (IEEE 802.1X) */
#endif /* ETH_P_PAE */
#ifndef ETH_P_EAPOL
#define ETH_P_EAPOL ETH_P_PAE
#endif /* ETH_P_EAPOL */
#ifndef ETH_P_RSN_PREAUTH
#define ETH_P_RSN_PREAUTH 0x88c7
#endif /* ETH_P_RSN_PREAUTH */
#ifndef ETH_P_RRB
#define ETH_P_RRB 0x890D
#endif /* ETH_P_RRB */
#ifndef ETH_P_OUI
#define ETH_P_OUI 0x88B7
#endif /* ETH_P_OUI */
#ifndef ETH_P_8021Q
#define ETH_P_8021Q 0x8100
#endif /* ETH_P_8021Q */

#ifndef BIT
#define BIT(x)                       ((uint32_t)((uint32_t)0x00000001U<<(x)))
#endif

#define broadcast_ether_addr (const uint8_t *) "\xff\xff\xff\xff\xff\xff"

#define WPA_PROTO_WPA                       BIT(0)
#define WPA_PROTO_RSN                       BIT(1)
#define WPA_PROTO_WAPI                      BIT(2)
#define WPA_PROTO_OSEN                      BIT(3)

#define WPA_AUTH_ALG_OPEN                   BIT(0)
#define WPA_AUTH_ALG_SHARED                 BIT(1)
#define WPA_AUTH_ALG_LEAP                   BIT(2)
#define WPA_AUTH_ALG_FT                     BIT(3)
#define WPA_AUTH_ALG_SAE                    BIT(4)
#define WPA_AUTH_ALG_FILS                   BIT(5)
#define WPA_AUTH_ALG_FILS_SK_PFS            BIT(6)

#define WPA_CIPHER_NONE                     BIT(0)
#define WPA_CIPHER_WEP40                    BIT(1)
#define WPA_CIPHER_WEP104                   BIT(2)
#define WPA_CIPHER_TKIP                     BIT(3)
#define WPA_CIPHER_CCMP                     BIT(4)
#define WPA_CIPHER_AES_128_CMAC             BIT(5)
#define WPA_CIPHER_GCMP                     BIT(6)
#define WPA_CIPHER_SMS4                     BIT(7)
#define WPA_CIPHER_GCMP_256                 BIT(8)
#define WPA_CIPHER_CCMP_256                 BIT(9)
#define WPA_CIPHER_BIP_GMAC_128             BIT(11)
#define WPA_CIPHER_BIP_GMAC_256             BIT(12)
#define WPA_CIPHER_BIP_CMAC_256             BIT(13)
#define WPA_CIPHER_GTK_NOT_USED             BIT(14)

#define WPA_KEY_MGMT_IEEE8021X              BIT(0)
#define WPA_KEY_MGMT_PSK                    BIT(1)
#define WPA_KEY_MGMT_NONE                   BIT(2)
#define WPA_KEY_MGMT_IEEE8021X_NO_WPA       BIT(3)
#define WPA_KEY_MGMT_WPA_NONE               BIT(4)
#define WPA_KEY_MGMT_FT_IEEE8021X           BIT(5)
#define WPA_KEY_MGMT_FT_PSK                 BIT(6)
#define WPA_KEY_MGMT_IEEE8021X_SHA256       BIT(7)
#define WPA_KEY_MGMT_PSK_SHA256             BIT(8)
#define WPA_KEY_MGMT_WPS                    BIT(9)
#define WPA_KEY_MGMT_SAE                    BIT(10)
#define WPA_KEY_MGMT_FT_SAE                 BIT(11)
#define WPA_KEY_MGMT_WAPI_PSK               BIT(12)
#define WPA_KEY_MGMT_WAPI_CERT              BIT(13)
#define WPA_KEY_MGMT_CCKM                   BIT(14)
#define WPA_KEY_MGMT_OSEN                   BIT(15)
#define WPA_KEY_MGMT_IEEE8021X_SUITE_B      BIT(16)
#define WPA_KEY_MGMT_IEEE8021X_SUITE_B_192  BIT(17)
#define WPA_KEY_MGMT_FILS_SHA256            BIT(18)
#define WPA_KEY_MGMT_FILS_SHA384            BIT(19)
#define WPA_KEY_MGMT_FT_FILS_SHA256         BIT(20)
#define WPA_KEY_MGMT_FT_FILS_SHA384         BIT(21)
#define WPA_KEY_MGMT_OWE                    BIT(22)
#define WPA_KEY_MGMT_DPP                    BIT(23)
#define WPA_KEY_MGMT_FT_IEEE8021X_SHA384    BIT(24)
#define WPA_KEY_MGMT_PASN                   BIT(25)
#define WPA_KEY_MGMT_SAE_EXT_KEY            BIT(26)
#define WPA_KEY_MGMT_FT_SAE_EXT_KEY         BIT(27)
#define WPA_KEY_MGMT_IEEE8021X_SHA384       BIT(28)

/* Timeout Interval Type */
#define WLAN_TIMEOUT_REASSOC_DEADLINE       1
#define WLAN_TIMEOUT_KEY_LIFETIME           2
#define WLAN_TIMEOUT_ASSOC_COMEBACK         3

#define WPA_SELECTOR_LEN                    4
#define WPA_VERSION                         1
#define RSN_SELECTOR_LEN                    4
#define RSN_VERSION                         1

#define WMM_OUI_TYPE                        2
#define WMM_OUI_SUBTYPE_INFORMATION_ELEMENT 0
#define WMM_OUI_SUBTYPE_PARAMETER_ELEMENT   1
#define WMM_OUI_SUBTYPE_TSPEC_ELEMENT       2
#define WMM_VERSION                         1

#define OSEN_IE_VENDOR_TYPE                 0x506f9a12

#define DEFAULT_FRAGMENT_SIZE               1398

enum wpa_alg {
    WPA_ALG_NONE,
    WPA_ALG_WEP,
    WPA_ALG_TKIP,
    WPA_ALG_CCMP,
    WPA_ALG_BIP_CMAC_128,
    WPA_ALG_GCMP,
    WPA_ALG_SMS4,
    WPA_ALG_KRK,
    WPA_ALG_GCMP_256,
    WPA_ALG_CCMP_256,
    WPA_ALG_BIP_GMAC_128,
    WPA_ALG_BIP_GMAC_256,
    WPA_ALG_BIP_CMAC_256
};

struct wpa_ie_hdr {
    uint8_t elem_id;
    uint8_t len;
    uint8_t oui[4];     /* 24-bit OUI followed by 8-bit OUI type */
    uint8_t version[2]; /* little endian */
} STRUCT_PACKED;

struct rsn_ie_hdr {
    uint8_t elem_id; /* WLAN_EID_RSN */
    uint8_t len;
    uint8_t version[2]; /* little endian */
} STRUCT_PACKED;

struct element {
    uint8_t id;
    uint8_t datalen;
    uint8_t data[];
} STRUCT_PACKED;

struct ieee80211_he_6ghz_band_cap {
     /* Minimum MPDU Start Spacing B0..B2
      * Maximum A-MPDU Length Exponent B3..B5
      * Maximum MPDU Length B6..B7 */
    le16 capab;
} STRUCT_PACKED;

struct ieee80211_ampe_ie {
    uint8_t selected_pairwise_suite[4];
    uint8_t local_nonce[32];
    uint8_t peer_nonce[32];
    /* Followed by
     * Key Replay Counter[8] (optional)
     *    (only in Mesh Group Key Inform/Acknowledge frames)
     * GTKdata[variable] (optional)
     *    (MGTK[variable] || Key RSC[8] || GTKExpirationTime[4])
     * IGTKdata[variable] (optional)
     *    (Key ID[2], IPN[6], IGTK[variable] in IGTK KDE format)
     */
} STRUCT_PACKED;

struct wpa_eapol_ie_parse {
    const uint8_t *wpa_ie;
    size_t wpa_ie_len;
    const uint8_t *rsn_ie;
    size_t rsn_ie_len;
    const uint8_t *pmkid;
    const uint8_t *key_id;
    const uint8_t *gtk;
    size_t gtk_len;
    const uint8_t *mac_addr;
    size_t mac_addr_len;
    const uint8_t *igtk;
    size_t igtk_len;
    const uint8_t *bigtk;
    size_t bigtk_len;
    const uint8_t *mdie;
    size_t mdie_len;
    const uint8_t *ftie;
    size_t ftie_len;
    const uint8_t *ip_addr_req;
    const uint8_t *ip_addr_alloc;
    const uint8_t *transition_disable;
    size_t transition_disable_len;
#ifdef CFG_DPP
    const uint8_t *dpp_kde;
    size_t dpp_kde_len;
#endif
    const uint8_t *oci;
    size_t oci_len;
    const uint8_t *osen;
    size_t osen_len;
    const uint8_t *rsnxe;
    size_t rsnxe_len;
    const uint8_t *reassoc_deadline;
    const uint8_t *key_lifetime;
    const uint8_t *lnkid;
    size_t lnkid_len;
    const uint8_t *ext_capab;
    size_t ext_capab_len;
    const uint8_t *supp_rates;
    size_t supp_rates_len;
    const uint8_t *ext_supp_rates;
    size_t ext_supp_rates_len;
    const uint8_t *ht_capabilities;
    const uint8_t *vht_capabilities;
    const uint8_t *he_capabilities;
    size_t he_capab_len;
    const uint8_t *he_6ghz_capabilities;
    const uint8_t *supp_channels;
    size_t supp_channels_len;
    const uint8_t *supp_oper_classes;
    size_t supp_oper_classes_len;
    uint8_t qosinfo;
    uint16_t aid;
    const uint8_t *wmm;
    size_t wmm_len;
};

struct wpa_ie_data {
    uint16_t proto;
    uint16_t has_pairwise;
    uint16_t pairwise_cipher;
    uint16_t has_group;
    uint16_t group_cipher;
    uint16_t mgmt_group_cipher;
    int key_mgmt;
    int capabilities;
    size_t num_pmkid;
    const uint8_t *pmkid;
};

#define WPA_KEY_MGMT_FT (WPA_KEY_MGMT_FT_PSK |  \
             WPA_KEY_MGMT_FT_IEEE8021X |        \
             WPA_KEY_MGMT_FT_IEEE8021X_SHA384 | \
             WPA_KEY_MGMT_FT_SAE |              \
             WPA_KEY_MGMT_FT_FILS_SHA256 |      \
             WPA_KEY_MGMT_FT_FILS_SHA384)

static inline int wpa_key_mgmt_sae(int akm)
{
    return !!(akm & (WPA_KEY_MGMT_SAE |
                WPA_KEY_MGMT_FT_SAE));
}

static inline int wpa_key_mgmt_wpa_psk(int akm)
{
    return !!(akm & (WPA_KEY_MGMT_PSK |
                WPA_KEY_MGMT_FT_PSK |
                WPA_KEY_MGMT_PSK_SHA256 |
                WPA_KEY_MGMT_SAE |
                WPA_KEY_MGMT_FT_SAE));
}

static inline int wpa_key_mgmt_sha256(int akm)
{
    return !!(akm & (WPA_KEY_MGMT_PSK_SHA256 |
                WPA_KEY_MGMT_IEEE8021X_SHA256 |
                WPA_KEY_MGMT_SAE |
                WPA_KEY_MGMT_FT_SAE |
                WPA_KEY_MGMT_OSEN |
                WPA_KEY_MGMT_IEEE8021X_SUITE_B |
                WPA_KEY_MGMT_FILS_SHA256 |
                WPA_KEY_MGMT_FT_FILS_SHA256));
}

static inline int wpa_key_mgmt_ft(int akm)
{
    return !!(akm & WPA_KEY_MGMT_FT);
}

static inline int wpa_key_mgmt_suite_b(int akm)
{
    return !!(akm & (WPA_KEY_MGMT_IEEE8021X_SUITE_B |
                WPA_KEY_MGMT_IEEE8021X_SUITE_B_192));
}

static inline int wpa_key_mgmt_wpa_ieee8021x(int akm)
{
    return !!(akm & (WPA_KEY_MGMT_IEEE8021X |
                WPA_KEY_MGMT_FT_IEEE8021X |
                WPA_KEY_MGMT_FT_IEEE8021X_SHA384 |
                WPA_KEY_MGMT_CCKM |
                WPA_KEY_MGMT_OSEN |
                WPA_KEY_MGMT_IEEE8021X_SHA256 |
                WPA_KEY_MGMT_IEEE8021X_SUITE_B |
                WPA_KEY_MGMT_IEEE8021X_SUITE_B_192 |
                WPA_KEY_MGMT_FILS_SHA256 |
                WPA_KEY_MGMT_FILS_SHA384 |
                WPA_KEY_MGMT_FT_FILS_SHA256 |
                WPA_KEY_MGMT_FT_FILS_SHA384));
}

static inline int wpa_key_mgmt_sha384(int akm)
{
    return !!(akm & (WPA_KEY_MGMT_IEEE8021X_SUITE_B_192 |
                WPA_KEY_MGMT_FT_IEEE8021X_SHA384 |
                WPA_KEY_MGMT_FILS_SHA384 |
                WPA_KEY_MGMT_FT_FILS_SHA384));
}

static inline int wpa_key_mgmt_sae_ext_key(int akm)
{
    return !!(akm & (WPA_KEY_MGMT_SAE_EXT_KEY |
                WPA_KEY_MGMT_FT_SAE_EXT_KEY));
}

static inline int wpa_key_mgmt_fils(int akm)
{
    return !!(akm & (WPA_KEY_MGMT_FILS_SHA256 |
                WPA_KEY_MGMT_FILS_SHA384 |
                WPA_KEY_MGMT_FT_FILS_SHA256 |
                WPA_KEY_MGMT_FT_FILS_SHA384));
}

static inline int is_multicast_ether_addr(const uint8_t *a)
{
    return a[0] & 0x01;
}

typedef long os_time_t;

struct os_time {
    os_time_t sec;
    os_time_t usec;
};

struct os_reltime {
    os_time_t sec;
    os_time_t usec;
};

#define for_each_element(_elem, _data, _datalen)                                    \
        for (_elem = (const struct element *) (_data);                              \
            (const uint8_t *) (_data) + (_datalen) - (const uint8_t *) _elem >=     \
            (int) sizeof(*_elem) &&                                                 \
            (const uint8_t *) (_data) + (_datalen) - (const uint8_t *) _elem >=     \
            (int) sizeof(*_elem) + _elem->datalen;                                  \
        _elem = (const struct element *) (_elem->data + _elem->datalen))

#define for_each_element_id(element, _id, data, datalen)    \
        for_each_element(element, data, datalen)            \
            if (element->id == (_id))

int sys_get_reltime(struct os_reltime *t);
void sys_reltime_sub(struct os_reltime *a, struct os_reltime *b,
                  struct os_reltime *res);
int sys_reltime_expired(struct os_reltime *now, struct os_reltime *t, os_time_t timeout_secs);
void * sys_realloc_array(void *ptr, size_t nmemb, size_t size);
int sys_memcmp_const(const void *a, const void *b, size_t len);
void *sys_memdup(const void *src, size_t len);
char *sys_strdup(const char *s);
size_t sys_strlcpy(char *dest, const char *src, size_t size);
int sys_snprintf(char *str, size_t size, const char *format, ...);
int sys_mktime(int year, int month, int day, int hour, int min, int sec, os_time_t *t);
int sys_gettime(struct os_time *t);

int hexstr2bin(const char *hex, uint8_t *buf, size_t len);
char * dup_binstr(const void *src, size_t len);

int has_ctrl_char(const uint8_t *data, size_t len);

uint8_t *wpas_set_fixed_field(uint8_t *pfrm, uint32_t field_len, uint8_t *field, uint32_t *tot_frm_len);
int wpa_cipher_rsc_len(int cipher);
unsigned int wpa_mic_len(int akmp, size_t pmk_len);
int wpa_eapol_key_mic(const uint8_t *key, size_t key_len, int akmp, int ver,
                        const uint8_t *buf, size_t len, uint8_t *mic);
void bin_clear_free(void *bin, size_t len);
void forced_memzero(void *ptr, size_t len);

int wpa_use_akm_defined(int akmp);
int wpas_mac_2_wpa_keymgmt(uint32_t akm);
int wpas_mac_2_wpa_cipher(uint32_t ciphers);
int wpas_mac_2_wpa_management_cipher(uint32_t ciphers);
int wpa_to_mac_cipher(enum wpa_alg alg, size_t key_len);
uint32_t wpa_cipher_to_suite(int proto, int cipher);
const char * wpa_cipher_txt(int cipher);

int wpa_pick_pairwise_cipher(int ciphers, int none_allowed);
int wpa_default_rsn_cipher(int freq);
int wpa_parse_wpa_ie_rsn(const uint8_t *rsn_ie, size_t rsn_ie_len,
                        struct wpa_ie_data *data);
int wpa_parse_kde_ies(const uint8_t *buf, size_t len, struct wpa_eapol_ie_parse *ie);
int wpa_cipher_key_len(int cipher);
int wpa_cipher_valid_pairwise(int cipher);
int wpa_cipher_valid_group(int cipher);
int wpa_cipher_valid_mgmt_group(int cipher);
enum wpa_alg wpa_cipher_to_alg(int cipher);
void inc_byte_array(uint8_t *counter, size_t len);
void wpa_get_ntp_timestamp(uint8_t *buf);
int wpa_use_aes_key_wrap(int akmp);
int wpa_compare_rsn_ie(int ft_initial_assoc,
                        const uint8_t *ie1, size_t ie1len,
                        const uint8_t *ie2, size_t ie2len);
int wpa_parse_wpa_ie_wpa(const uint8_t *wpa_ie, size_t wpa_ie_len,
                        struct wpa_ie_data *data);

int wpas_action_send(int vif_idx, const uint8_t *dst, const uint8_t *src,
                     const uint8_t *bssid, const uint8_t *data,
                     size_t data_len, int no_cck);
void wpas_action_receive(int vif_idx, uint8_t *pframe, uint32_t frm_len);
struct wpabuf * wpa_bss_get_vendor_ie_multi(uint8_t *bss_ie, uint32_t bss_ie_len,
                        uint32_t vendor_type);

int ieee802_11_parse_elems(const uint8_t *start, size_t len,
                struct ieee802_11_elems *elems,
                int show_errors);

struct wpabuf * ieee802_11_defrag(const uint8_t *data, size_t len, bool ext_elem);

int ieee802_11_ie_count(const uint8_t *ies, size_t ies_len);

#endif /* _WPAS_COMM_H_ */
