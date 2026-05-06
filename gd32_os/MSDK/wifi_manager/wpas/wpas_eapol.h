/*
 * wpa_supplicant - WPA definitions
 * Copyright (c) 2003-2015, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

/*!
    \file    wpas_eapol.h
    \brief   Header file for wpas eapol.

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

#ifndef _WPAS_EAPOL_H_
#define _WPAS_EAPOL_H_

#define DEFAULT_EAPOL_VERSION   2

#define WPA_MAX_PSK_LEN         63
#define WPA_MIN_PSK_LEN         8

#define EAPOL_VERSION           2
#ifndef ETH_ALEN
#define ETH_ALEN                6
#endif

#define PMKID_LEN               16
#define PMK_LEN                 32
#define PMK_LEN_SUITE_B_192     48
#define PMK_LEN_MAX             64

#define WPA_REPLAY_COUNTER_LEN  8
#define WPA_NONCE_LEN           32
#define WPA_KEY_RSC_LEN         8
#define WPA_GMK_LEN             32
#define WPA_GTK_MAX_LEN         32
#define WPA_PASN_PMK_LEN        32
#define WPA_PASN_MAX_MIC_LEN    24
#define WPA_MAX_RSNXE_LEN       4
#define WPA_MAX_IE_LEN          128

#define WPA_IGTK_LEN            16
#define WPA_IGTK_MAX_LEN        32
#define WPA_BIGTK_LEN           16
#define WPA_BIGTK_MAX_LEN       32

#define WPA_EAPOL_KEY_MIC_MAX_LEN 32
#define WPA_KCK_MAX_LEN         32
#define WPA_KEK_MAX_LEN         64
#define WPA_TK_MAX_LEN          32
#define WPA_KDK_MAX_LEN         32
#define FILS_ICK_MAX_LEN        48
#define FILS_FT_MAX_LEN         48
#define WPA_PASN_KCK_LEN        32
#define WPA_PASN_MIC_MAX_LEN    24

/* IEEE 802.11, 8.5.2 EAPOL-Key frames */
#define WPA_KEY_INFO_TYPE_MASK          ((uint16_t) (BIT(0) | BIT(1) | BIT(2)))
#define WPA_KEY_INFO_TYPE_AKM_DEFINED   0
#define WPA_KEY_INFO_TYPE_HMAC_MD5_RC4  BIT(0)
#define WPA_KEY_INFO_TYPE_HMAC_SHA1_AES BIT(1)
#define WPA_KEY_INFO_TYPE_AES_128_CMAC  3
#define WPA_KEY_INFO_KEY_TYPE           BIT(3) /* 1 = Pairwise, 0 = Group key */
/* bit4..5 is used in WPA, but is reserved in IEEE 802.11i/RSN */
#define WPA_KEY_INFO_KEY_INDEX_MASK     (BIT(4) | BIT(5))
#define WPA_KEY_INFO_KEY_INDEX_SHIFT    4
#define WPA_KEY_INFO_INSTALL            BIT(6) /* pairwise */
#define WPA_KEY_INFO_TXRX               BIT(6) /* group */
#define WPA_KEY_INFO_ACK                BIT(7)
#define WPA_KEY_INFO_MIC                BIT(8)
#define WPA_KEY_INFO_SECURE             BIT(9)
#define WPA_KEY_INFO_ERROR              BIT(10)
#define WPA_KEY_INFO_REQUEST            BIT(11)
#define WPA_KEY_INFO_ENCR_KEY_DATA      BIT(12) /* IEEE 802.11i/RSN only */
#define WPA_KEY_INFO_SMK_MESSAGE        BIT(13)

#define RSN_KEY_DATA_GROUPKEY                RSN_SELECTOR(0x00, 0x0f, 0xac, 1)
#define RSN_KEY_DATA_MAC_ADDR                RSN_SELECTOR(0x00, 0x0f, 0xac, 3)
#define RSN_KEY_DATA_PMKID                   RSN_SELECTOR(0x00, 0x0f, 0xac, 4)
#define RSN_KEY_DATA_IGTK                    RSN_SELECTOR(0x00, 0x0f, 0xac, 9)
#define RSN_KEY_DATA_KEYID                   RSN_SELECTOR(0x00, 0x0f, 0xac, 10)
#define RSN_KEY_DATA_MULTIBAND_GTK           RSN_SELECTOR(0x00, 0x0f, 0xac, 11)
#define RSN_KEY_DATA_MULTIBAND_KEYID         RSN_SELECTOR(0x00, 0x0f, 0xac, 12)
#define RSN_KEY_DATA_OCI                     RSN_SELECTOR(0x00, 0x0f, 0xac, 13)
#define RSN_KEY_DATA_BIGTK                   RSN_SELECTOR(0x00, 0x0f, 0xac, 14)

#define RSN_AUTH_KEY_MGMT_UNSPEC_802_1X      RSN_SELECTOR(0x00, 0x0f, 0xac, 1)
#define RSN_AUTH_KEY_MGMT_PSK_OVER_802_1X    RSN_SELECTOR(0x00, 0x0f, 0xac, 2)
#define RSN_AUTH_KEY_MGMT_FT_802_1X          RSN_SELECTOR(0x00, 0x0f, 0xac, 3)
#define RSN_AUTH_KEY_MGMT_FT_PSK             RSN_SELECTOR(0x00, 0x0f, 0xac, 4)
#define RSN_AUTH_KEY_MGMT_802_1X_SHA256      RSN_SELECTOR(0x00, 0x0f, 0xac, 5)
#define RSN_AUTH_KEY_MGMT_PSK_SHA256         RSN_SELECTOR(0x00, 0x0f, 0xac, 6)
#define RSN_AUTH_KEY_MGMT_TPK_HANDSHAKE      RSN_SELECTOR(0x00, 0x0f, 0xac, 7)
#define RSN_AUTH_KEY_MGMT_SAE                RSN_SELECTOR(0x00, 0x0f, 0xac, 8)
#define RSN_AUTH_KEY_MGMT_FT_SAE             RSN_SELECTOR(0x00, 0x0f, 0xac, 9)
#define RSN_AUTH_KEY_MGMT_802_1X_SUITE_B     RSN_SELECTOR(0x00, 0x0f, 0xac, 11)
#define RSN_AUTH_KEY_MGMT_802_1X_SUITE_B_192 RSN_SELECTOR(0x00, 0x0f, 0xac, 12)
#define RSN_AUTH_KEY_MGMT_FT_802_1X_SHA384   RSN_SELECTOR(0x00, 0x0f, 0xac, 13)
#define RSN_AUTH_KEY_MGMT_FILS_SHA256        RSN_SELECTOR(0x00, 0x0f, 0xac, 14)
#define RSN_AUTH_KEY_MGMT_FILS_SHA384        RSN_SELECTOR(0x00, 0x0f, 0xac, 15)
#define RSN_AUTH_KEY_MGMT_FT_FILS_SHA256     RSN_SELECTOR(0x00, 0x0f, 0xac, 16)
#define RSN_AUTH_KEY_MGMT_FT_FILS_SHA384     RSN_SELECTOR(0x00, 0x0f, 0xac, 17)
#define RSN_AUTH_KEY_MGMT_OWE                RSN_SELECTOR(0x00, 0x0f, 0xac, 18)
#define RSN_AUTH_KEY_MGMT_PASN               RSN_SELECTOR(0x00, 0x0f, 0xac, 21)
#define RSN_AUTH_KEY_MGMT_CCKM               RSN_SELECTOR(0x00, 0x40, 0x96, 0x00)
#define RSN_AUTH_KEY_MGMT_OSEN               RSN_SELECTOR(0x50, 0x6f, 0x9a, 0x01)
#define RSN_AUTH_KEY_MGMT_DPP                RSN_SELECTOR(0x50, 0x6f, 0x9a, 0x02)

#define RSN_CIPHER_SUITE_NONE                RSN_SELECTOR(0x00, 0x0f, 0xac, 0)
#define RSN_CIPHER_SUITE_WEP40               RSN_SELECTOR(0x00, 0x0f, 0xac, 1)
#define RSN_CIPHER_SUITE_TKIP                RSN_SELECTOR(0x00, 0x0f, 0xac, 2)
#define RSN_CIPHER_SUITE_CCMP                RSN_SELECTOR(0x00, 0x0f, 0xac, 4)
#define RSN_CIPHER_SUITE_WEP104              RSN_SELECTOR(0x00, 0x0f, 0xac, 5)
#define RSN_CIPHER_SUITE_AES_128_CMAC        RSN_SELECTOR(0x00, 0x0f, 0xac, 6)
#define RSN_CIPHER_SUITE_NO_GROUP_ADDRESSED  RSN_SELECTOR(0x00, 0x0f, 0xac, 7)
#define RSN_CIPHER_SUITE_GCMP                RSN_SELECTOR(0x00, 0x0f, 0xac, 8)
#define RSN_CIPHER_SUITE_GCMP_256            RSN_SELECTOR(0x00, 0x0f, 0xac, 9)
#define RSN_CIPHER_SUITE_CCMP_256            RSN_SELECTOR(0x00, 0x0f, 0xac, 10)
#define RSN_CIPHER_SUITE_BIP_GMAC_128        RSN_SELECTOR(0x00, 0x0f, 0xac, 11)
#define RSN_CIPHER_SUITE_BIP_GMAC_256        RSN_SELECTOR(0x00, 0x0f, 0xac, 12)
#define RSN_CIPHER_SUITE_BIP_CMAC_256        RSN_SELECTOR(0x00, 0x0f, 0xac, 13)
#define RSN_CIPHER_SUITE_SMS4                RSN_SELECTOR(0x00, 0x14, 0x72, 1)
#define RSN_CIPHER_SUITE_CKIP                RSN_SELECTOR(0x00, 0x40, 0x96, 0)
#define RSN_CIPHER_SUITE_CKIP_CMIC           RSN_SELECTOR(0x00, 0x40, 0x96, 1)
#define RSN_CIPHER_SUITE_CMIC                RSN_SELECTOR(0x00, 0x40, 0x96, 2)
/* KRK is defined for nl80211 use only */
#define RSN_CIPHER_SUITE_KRK                 RSN_SELECTOR(0x00, 0x40, 0x96, 255)

/* IEEE 802.11, 7.3.2.25.3 RSN Capabilities */
#define WPA_CAPABILITY_PREAUTH                  BIT(0)
#define WPA_CAPABILITY_NO_PAIRWISE              BIT(1)
#define WPA_CAPABILITY_MFPR                     BIT(6)
#define WPA_CAPABILITY_MFPC                     BIT(7)
#define WPA_CAPABILITY_PEERKEY_ENABLED          BIT(9)
#define WPA_CAPABILITY_SPP_A_MSDU_CAPABLE       BIT(10)
#define WPA_CAPABILITY_SPP_A_MSDU_REQUIRED      BIT(11)
#define WPA_CAPABILITY_PBAC                     BIT(12)
#define WPA_CAPABILITY_EXT_KEY_ID_FOR_UNICAST   BIT(13)
#define WPA_CAPABILITY_OCVC                     BIT(14)

#define WPA_AUTH_KEY_MGMT_NONE               RSN_SELECTOR(0x00, 0x50, 0xf2, 0)
#define WPA_AUTH_KEY_MGMT_UNSPEC_802_1X      RSN_SELECTOR(0x00, 0x50, 0xf2, 1)
#define WPA_AUTH_KEY_MGMT_PSK_OVER_802_1X    RSN_SELECTOR(0x00, 0x50, 0xf2, 2)
#define WPA_AUTH_KEY_MGMT_CCKM               RSN_SELECTOR(0x00, 0x40, 0x96, 0)
#define WPA_CIPHER_SUITE_NONE                RSN_SELECTOR(0x00, 0x50, 0xf2, 0)
#define WPA_CIPHER_SUITE_TKIP                RSN_SELECTOR(0x00, 0x50, 0xf2, 2)
#define WPA_CIPHER_SUITE_CCMP                RSN_SELECTOR(0x00, 0x50, 0xf2, 4)

/* EAPOL-Key Key Data Encapsulation
 * GroupKey and PeerKey require encryption, otherwise, encryption is optional.
 */
#define RSN_KEY_DATA_GROUPKEY           RSN_SELECTOR(0x00, 0x0f, 0xac, 1)
#define RSN_KEY_DATA_MAC_ADDR           RSN_SELECTOR(0x00, 0x0f, 0xac, 3)
#define RSN_KEY_DATA_PMKID              RSN_SELECTOR(0x00, 0x0f, 0xac, 4)
#define RSN_KEY_DATA_IGTK               RSN_SELECTOR(0x00, 0x0f, 0xac, 9)
#define RSN_KEY_DATA_KEYID              RSN_SELECTOR(0x00, 0x0f, 0xac, 10)
#define RSN_KEY_DATA_MULTIBAND_GTK      RSN_SELECTOR(0x00, 0x0f, 0xac, 11)
#define RSN_KEY_DATA_MULTIBAND_KEYID    RSN_SELECTOR(0x00, 0x0f, 0xac, 12)
#define RSN_KEY_DATA_OCI                RSN_SELECTOR(0x00, 0x0f, 0xac, 13)
#define RSN_KEY_DATA_BIGTK              RSN_SELECTOR(0x00, 0x0f, 0xac, 14)

#define WFA_KEY_DATA_IP_ADDR_REQ        RSN_SELECTOR(0x50, 0x6f, 0x9a, 4)
#define WFA_KEY_DATA_IP_ADDR_ALLOC      RSN_SELECTOR(0x50, 0x6f, 0x9a, 5)
#define WFA_KEY_DATA_TRANSITION_DISABLE RSN_SELECTOR(0x50, 0x6f, 0x9a, 0x20)
#define WFA_KEY_DATA_DPP                RSN_SELECTOR(0x50, 0x6f, 0x9a, 0x21)

#define WPA_OUI_TYPE                    RSN_SELECTOR(0x00, 0x50, 0xf2, 1)

#define EAPOL_TIMEROUT                      2000  //ms (not larger than connect/roaming retry interval)

#ifdef CFG_SOFTAP
/* For softap mode */
#define RSNA_MAX_EAPOL_RETRIES              4

#define AP_EAPOL_KEY_FIRST_TIMEOUT          100  //ms
#define AP_EAPOL_KEY_SUBSEQ_TIMEOUT         1000  //ms
#define AP_EAPOL_KEY_FIRST_GROUP_TIMEOUT    500  //ms

#define WPA_AUTH_GROUP_REKEY_TIMEOUT        86400000 //600000  //ms

#define WPA_GROUP_UPDATE_COUNT              4
#define WPA_PAIRWISE_UPDATE_COUNT           4
#endif
/* For softap mode end */


#ifndef ETH_P_PAE
#define ETH_P_PAE   0x888E /* Port Access Entity (IEEE 802.1X) */
#endif /* ETH_P_PAE */

#ifndef ETH_P_EAPOL
#define ETH_P_EAPOL ETH_P_PAE
#endif /* ETH_P_EAPOL */
enum wpas_mode {
    WPAS_MODE_INFRA               = 0,
    WPAS_MODE_IBSS                = 1,
    WPAS_MODE_AP                  = 2,
    WPAS_MODE_P2P_GO              = 3,
    WPAS_MODE_P2P_GROUP_FORMATION = 4,
    WPAS_MODE_MESH                = 5,
};

#define MGMT_FRAME_PROTECTION_DEFAULT   3
/**
 * enum mfp_options - Management frame protection (IEEE 802.11w) options
 */
enum mfp_options {
    NO_MGMT_FRAME_PROTECTION        = 0,
    MGMT_FRAME_PROTECTION_OPTIONAL  = 1,
    MGMT_FRAME_PROTECTION_REQUIRED  = 2,
};

enum {
    IEEE802_1X_TYPE_EAP_PACKET                   = 0,
    IEEE802_1X_TYPE_EAPOL_START                  = 1,
    IEEE802_1X_TYPE_EAPOL_LOGOFF                 = 2,
    IEEE802_1X_TYPE_EAPOL_KEY                    = 3,
    IEEE802_1X_TYPE_EAPOL_ENCAPSULATED_ASF_ALERT = 4,
    IEEE802_1X_TYPE_EAPOL_MKA                    = 5,
};

enum {
    EAPOL_KEY_TYPE_RC4 = 1,
    EAPOL_KEY_TYPE_RSN = 2,
    EAPOL_KEY_TYPE_WPA = 254
};

enum key_flag {
    KEY_FLAG_MODIFY                 = (1U << (0)),//BIT(0),
    KEY_FLAG_DEFAULT                = (1U << (1)),//BIT(1),
    KEY_FLAG_RX                     = (1U << (2)),//BIT(2),
    KEY_FLAG_TX                     = (1U << (3)),//BIT(3),
    KEY_FLAG_GROUP                  = (1U << (4)),//BIT(4),
    KEY_FLAG_PAIRWISE               = (1U << (5)),//BIT(5),
    KEY_FLAG_PMK                    = (1U << (6)),//BIT(6),
    /* Used flag combinations */
    KEY_FLAG_RX_TX                  = KEY_FLAG_RX | KEY_FLAG_TX,
    KEY_FLAG_GROUP_RX_TX            = KEY_FLAG_GROUP | KEY_FLAG_RX_TX,
    KEY_FLAG_GROUP_RX_TX_DEFAULT    = KEY_FLAG_GROUP_RX_TX |
                                        KEY_FLAG_DEFAULT,
    KEY_FLAG_GROUP_RX               = KEY_FLAG_GROUP | KEY_FLAG_RX,
    KEY_FLAG_GROUP_TX_DEFAULT       = KEY_FLAG_GROUP | KEY_FLAG_TX |
                                        KEY_FLAG_DEFAULT,
    KEY_FLAG_PAIRWISE_RX_TX         = KEY_FLAG_PAIRWISE | KEY_FLAG_RX_TX,
    KEY_FLAG_PAIRWISE_RX            = KEY_FLAG_PAIRWISE | KEY_FLAG_RX,
    KEY_FLAG_PAIRWISE_RX_TX_MODIFY  = KEY_FLAG_PAIRWISE_RX_TX |
                                        KEY_FLAG_MODIFY,
    /* Max allowed flags for each key type */
    KEY_FLAG_PAIRWISE_MASK          = KEY_FLAG_PAIRWISE_RX_TX_MODIFY,
    KEY_FLAG_GROUP_MASK             = KEY_FLAG_GROUP_RX_TX_DEFAULT,
    KEY_FLAG_PMK_MASK               = KEY_FLAG_PMK,
};

struct wpas_key_desc {
    /**
     * alg - Encryption algorithm
     *
     * (%WPA_ALG_NONE, %WPA_ALG_WEP, %WPA_ALG_TKIP, %WPA_ALG_CCMP,
     * %WPA_ALG_BIP_AES_CMAC_128, %WPA_ALG_GCMP, %WPA_ALG_GCMP_256,
     * %WPA_ALG_CCMP_256, %WPA_ALG_BIP_GMAC_128, %WPA_ALG_BIP_GMAC_256,
     * %WPA_ALG_BIP_CMAC_256);
     * %WPA_ALG_NONE clears the key. */
    enum wpa_alg alg;

    /**
     * addr - Address of the peer STA
     *
     * (BSSID of the current AP when setting pairwise key in station mode),
     * ff:ff:ff:ff:ff:ff for broadcast keys, %NULL for default keys that
     * are used both for broadcast and unicast; when clearing keys, %NULL
     * is used to indicate that both the broadcast-only and default key of
     * the specified key index is to be cleared */
    const uint8_t *addr;

    /**
     * key_idx - Key index
     *
     * (0..3), usually 0 for unicast keys; 4..5 for IGTK; 6..7 for BIGTK */
    int key_idx;

    /**
     * set_tx - Configure this key as the default Tx key
     *
     * Only used when driver does not support separate unicast/individual
     * key */
    int set_tx;

    /**
     * seq - Sequence number/packet number
     *
     * seq_len octets, the next packet number to be used for in replay
     * protection; configured for Rx keys (in most cases, this is only used
     * with broadcast keys and set to zero for unicast keys); %NULL if not
     * set */
    const uint8_t *seq;

    /**
     * seq_len - Length of the seq, depends on the algorithm
     *
     * TKIP: 6 octets, CCMP/GCMP: 6 octets, IGTK: 6 octets */
    size_t seq_len;

    /**
     * key - Key buffer
     *
     * TKIP: 16-byte temporal key, 8-byte Tx Mic key, 8-byte Rx Mic Key */
    const uint8_t *key;

    /**
     * key_len - Length of the key buffer in octets
     *
     * WEP: 5 or 13, TKIP: 32, CCMP/GCMP: 16, IGTK: 16 */
    size_t key_len;

    /**
     * key_flag - Additional key flags
     *
     * %KEY_FLAG_MODIFY
     *  Set when an already installed key must be updated.
     *  So far the only use-case is changing RX/TX status for
     *  pairwise keys. Must not be set when deleting a key.
     * %KEY_FLAG_DEFAULT
     *  Set when the key is also a default key. Must not be set when
     *  deleting a key.
     * %KEY_FLAG_RX
     *  The key is valid for RX. Must not be set when deleting a key.
     * %KEY_FLAG_TX
     *  The key is valid for TX. Must not be set when deleting a key.
     * %KEY_FLAG_GROUP
     *  The key is a broadcast or group key.
     * %KEY_FLAG_PAIRWISE
     *  The key is a pairwise key.
     * %KEY_FLAG_PMK
     *  The key is a Pairwise Master Key (PMK).
     *
     * Valid and pre-defined combinations are:
     * %KEY_FLAG_GROUP_RX_TX
     *  WEP key not to be installed as default key.
     * %KEY_FLAG_GROUP_RX_TX_DEFAULT
     *  Default WEP or WPA-NONE key.
     * %KEY_FLAG_GROUP_RX
     *  GTK key valid for RX only.
     * %KEY_FLAG_GROUP_TX_DEFAULT
     *  GTK key valid for TX only, immediately taking over TX.
     * %KEY_FLAG_PAIRWISE_RX_TX
     *  Pairwise key immediately becoming the active pairwise key.
     * %KEY_FLAG_PAIRWISE_RX
     *  Pairwise key not yet valid for TX. (Only usable when Extended
     *  Key ID is supported by the driver.)
     * %KEY_FLAG_PAIRWISE_RX_TX_MODIFY
     *  Enable TX for a pairwise key installed with
     *  KEY_FLAG_PAIRWISE_RX.
     *
     * Not a valid standalone key type but pre-defined to be combined
     * with other key_flags:
     * %KEY_FLAG_RX_TX
     *  RX/TX key. */
    enum key_flag key_flag;
};

struct ieee802_1x_hdr {
    uint8_t version;
    uint8_t type;
    uint16_t length;
    /* followed by length octets of data */
} STRUCT_PACKED;

/**
 * struct wpa_ptk - WPA Pairwise Transient Key
 * IEEE Std 802.11i-2004 - 8.5.1.2 Pairwise key hierarchy
 */
struct wpa_ptk {
    uint8_t kck[WPA_KCK_MAX_LEN]; /* EAPOL-Key Key Confirmation Key (KCK) */
    uint8_t kek[WPA_KEK_MAX_LEN]; /* EAPOL-Key Key Encryption Key (KEK) */
    uint8_t tk[WPA_TK_MAX_LEN]; /* Temporal Key (TK) */
    uint8_t kck2[WPA_KCK_MAX_LEN]; /* FT reasoc Key Confirmation Key (KCK2) */
    uint8_t kek2[WPA_KEK_MAX_LEN]; /* FT reassoc Key Encryption Key (KEK2) */
    uint8_t kdk[WPA_KDK_MAX_LEN]; /* Key Derivation Key */
    size_t kck_len;
    size_t kek_len;
    size_t tk_len;
    size_t kck2_len;
    size_t kek2_len;
    size_t kdk_len;
    int installed; /* 1 if key has already been installed to driver */
};

struct wpa_eapol_key {
    uint8_t type;
    /* Note: key_info, key_length, and key_data_length are unaligned */
    uint8_t key_info[2]; /* big endian */
    uint8_t key_length[2]; /* big endian */
    uint8_t replay_counter[WPA_REPLAY_COUNTER_LEN];
    uint8_t key_nonce[WPA_NONCE_LEN];
    uint8_t key_iv[16];
    uint8_t key_rsc[WPA_KEY_RSC_LEN];
    uint8_t key_id[8]; /* Reserved in IEEE 802.11i/RSN */
    /* variable length Key MIC field */
    /* big endian 2-octet Key Data Length field */
    /* followed by Key Data Length bytes of Key Data */
} STRUCT_PACKED;

struct wpa_gtk_data {
    enum wpa_alg alg;
    int tx, key_rsc_len, keyidx;
    uint8_t gtk[WPA_GTK_MAX_LEN];
    int gtk_len;
};

struct wpa_gtk {
    uint8_t gtk[WPA_GTK_MAX_LEN];
    size_t gtk_len;
};

struct wpa_igtk {
    uint8_t igtk[WPA_IGTK_MAX_LEN];
    size_t igtk_len;
};

#define WPA_IGTK_KDE_PREFIX_LEN (2 + 6)
struct wpa_igtk_kde {
    uint8_t keyid[2];
    uint8_t pn[6];
    uint8_t igtk[WPA_IGTK_MAX_LEN];
} STRUCT_PACKED;

struct wpa_bigtk {
    uint8_t bigtk[WPA_BIGTK_MAX_LEN];
    size_t bigtk_len;
};

#define WPA_BIGTK_KDE_PREFIX_LEN (2 + 6)
struct wpa_bigtk_kde {
    uint8_t keyid[2];
    uint8_t pn[6];
    uint8_t bigtk[WPA_BIGTK_MAX_LEN];
} STRUCT_PACKED;

#define IEEE8021X_REPLAY_COUNTER_LEN 8
#define IEEE8021X_KEY_SIGN_LEN       16
#define IEEE8021X_KEY_IV_LEN         16

#define IEEE8021X_KEY_INDEX_FLAG 0x80
#define IEEE8021X_KEY_INDEX_MASK 0x03

struct ieee802_1x_eapol_key {
    uint8_t type;
    /* Note: key_length is unaligned */
    uint8_t key_length[2];
    /* does not repeat within the life of the keying material used to
     * encrypt the Key field; 64-bit NTP timestamp MAY be used here */
    uint8_t replay_counter[IEEE8021X_REPLAY_COUNTER_LEN];
    uint8_t key_iv[IEEE8021X_KEY_IV_LEN]; /* cryptographically random number */
    uint8_t key_index; /* key flag in the most significant bit:
               * 0 = broadcast (default key),
               * 1 = unicast (key mapping key); key index is in the
               * 7 least significant bits */
    /* HMAC-MD5 message integrity check computed with MS-MPPE-Send-Key as
     * the key */
    uint8_t key_signature[IEEE8021X_KEY_SIGN_LEN];

    /* followed by key: if packet body length = 44 + key length, then the
     * key field (of key_length bytes) contains the key in encrypted form;
     * if packet body length = 44, key field is absent and key_length
     * represents the number of least significant octets from
     * MS-MPPE-Send-Key attribute to be used as the keying material;
     * RC4 key used in encryption = Key-IV + MS-MPPE-Recv-Key */
} STRUCT_PACKED;

enum eapol_state_t {
    EAPOL_STATE_NOTHING = 0,
    EAPOL_STATE_PAIRWISE,
    EAPOL_STATE_GROUP,
    EAPOL_STATE_ESTABLISHED,
};

struct eapol_info {
    uint8_t own_addr[ETH_ALEN];
    uint8_t bssid[ETH_ALEN];
    uint32_t mac_akm;
    int mgmt_group_cipher;
    int key_mgmt;
    int group_cipher;
    int pairwise_cipher;
#ifdef CONFIG_OWE
    int owe_ptk_workaround;
    struct crypto_ecdh *owe_ecdh;
    uint16_t owe_group;
#endif /* CONFIG_OWE */
};

struct wpas_eapol
{
    enum eapol_state_t state;
    uint8_t eapol_version;
    struct eapol_info info;
    uint32_t keys_cleared;

    uint8_t pmk[PMK_LEN_MAX];
    size_t pmk_len;
    struct wpa_ptk ptk;
    struct wpa_ptk *tptk;
    uint8_t ptk_set:1;
    uint8_t tptk_set:1;
    uint8_t msg_3_of_4_ok:1;
    uint8_t renew_snonce:1;
    uint8_t rx_replay_counter_set:1;
    uint8_t snonce[WPA_NONCE_LEN];
    uint8_t anonce[WPA_NONCE_LEN]; /* ANonce from the last 1/4 msg */
    uint8_t rx_replay_counter[WPA_REPLAY_COUNTER_LEN];
    uint8_t request_counter[WPA_REPLAY_COUNTER_LEN];
    struct wpa_gtk gtk;
    struct wpa_igtk igtk;
    struct wpa_bigtk bigtk;

#ifdef WPA_REKEY
    int wpa_ptk_rekey;
    int wpa_deny_ptk0_rekey:1;
#endif /* WPA_REKEY */

    /* Selected configuration (based on Beacon/ProbeResp WPA IE) */
    int proto;
    uint8_t *assoc_wpa_ie;
    size_t assoc_wpa_ie_len;
#ifdef CONFIG_OWE
    size_t assoc_owe_ie_len;
#endif /* CONFIG_OWE */
    uint8_t *ap_wpa_ie;
    size_t ap_wpa_ie_len;
    uint8_t *ap_rsn_ie;
    size_t ap_rsn_ie_len;
};

#ifdef CFG_SOFTAP
/* For softap mode */
enum wpa_ptk_state_t{
    WPA_PTK_INITIALIZE,
    WPA_PTK_DISCONNECT,
    WPA_PTK_DISCONNECTED,
    WPA_PTK_AUTHENTICATION,
    WPA_PTK_AUTHENTICATION2,
    WPA_PTK_INITPMK,
    WPA_PTK_INITPSK,
    WPA_PTK_PTKSTART,
    WPA_PTK_PTKCALCNEGOTIATING,
    WPA_PTK_PTKCALCNEGOTIATING2,
    WPA_PTK_PTKINITNEGOTIATING,
    WPA_PTK_PTKINITDONE
};

enum wpa_ptk_group_state_t {
    WPA_PTK_GROUP_IDLE = 0,
    WPA_PTK_GROUP_REKEYNEGOTIATING,
    WPA_PTK_GROUP_REKEYESTABLISHED,
    WPA_PTK_GROUP_KEYERROR
};

enum wpa_group_state_t {
    WPA_GROUP_GTK_INIT = 0,
    WPA_GROUP_SETKEYS,
    WPA_GROUP_SETKEYSDONE,
    WPA_GROUP_FATAL_FAILURE
};

enum wpa_version_t {
    WPA_VERSION_NO_WPA = 0 /* WPA not used */,
    WPA_VERSION_WPA = 1 /* WPA / IEEE 802.11i/D3.0 */,
    WPA_VERSION_WPA2 = 2 /* WPA2 / IEEE 802.11i */
};

struct wpa_key_replay_t {
    uint8_t counter[WPA_REPLAY_COUNTER_LEN];
    bool valid;
};

enum wpa_event {
    WPA_AUTH, WPA_ASSOC, WPA_DISASSOC, WPA_DEAUTH, WPA_REAUTH,
    WPA_REAUTH_EAPOL, WPA_ASSOC_FT, WPA_ASSOC_FILS, WPA_DRV_STA_REMOVED
};

enum wpa_validate_result {
    WPA_IE_OK, WPA_INVALID_IE, WPA_INVALID_GROUP, WPA_INVALID_PAIRWISE,
    WPA_INVALID_AKMP, WPA_NOT_ENABLED, WPA_ALLOC_FAIL,
    WPA_MGMT_FRAME_PROTECTION_VIOLATION, WPA_INVALID_MGMT_GROUP_CIPHER,
    WPA_INVALID_MDIE, WPA_INVALID_PROTO, WPA_INVALID_PMKID,
    WPA_DENIED_OTHER_REASON
};

/* per group key state machine data */
struct wpa_group {
    struct wpa_group *next;
    int vlan_id;

    bool GInit;
    int GKeyDoneStations;
    bool GTKReKey;
    int GTK_len;
    int GN, GM;
    bool GTKAuthenticator;
    uint8_t Counter[WPA_NONCE_LEN];

    enum wpa_group_state_t wpa_group_state;

    uint8_t GMK[WPA_GMK_LEN];
    uint8_t GTK[2][WPA_GTK_MAX_LEN];
    uint8_t GNonce[WPA_NONCE_LEN];
    bool changed;
    bool first_sta_seen;
    bool reject_4way_hs_for_entropy;
    uint8_t IGTK[2][WPA_IGTK_MAX_LEN];
    uint8_t BIGTK[2][WPA_IGTK_MAX_LEN];
    int GN_igtk, GM_igtk;
    int GN_bigtk, GM_bigtk;

    uint32_t references;
};

/* For softap mode end */

struct wpa_cli_sm
{
    struct wpa_group *group;

    uint16_t auth_alg;
    enum wpa_ptk_state_t wpa_ptk_state;
    enum wpa_ptk_group_state_t wpa_ptk_group_state;

    uint32_t Init:1;
    uint32_t DeauthenticationRequest:1;
    uint32_t AuthenticationRequest:1;
    uint32_t ReAuthenticationRequest:1;
    uint32_t Disconnect:1;
    uint32_t TimeoutEvt:1;
    uint32_t EAPOLKeyReceived:1;
    uint32_t EAPOLKeyPairwise:1;
    uint32_t EAPOLKeyRequest:1;
    uint32_t MICVerified:1;
    uint32_t GUpdateStationKeys:1;

    uint16_t disconnect_reason; /* specific reason code to use with Disconnect */
    uint32_t TimeoutCtr;
    uint32_t GTimeoutCtr;
    uint8_t ANonce[WPA_NONCE_LEN];
    uint8_t SNonce[WPA_NONCE_LEN];
    uint8_t alt_SNonce[WPA_NONCE_LEN];
    uint8_t alt_replay_counter[WPA_REPLAY_COUNTER_LEN];
    uint8_t PMK[PMK_LEN_MAX];
    uint32_t pmk_len;
    uint8_t pmkid[PMKID_LEN]; /* valid if pmkid_set == 1 */
    struct wpa_ptk PTK;
    struct wpa_key_replay_t key_replay[RSNA_MAX_EAPOL_RETRIES];
    struct wpa_key_replay_t prev_key_replay[RSNA_MAX_EAPOL_RETRIES];

    uint8_t *last_rx_eapol_key; /* starting from IEEE 802.1X header */
    size_t last_rx_eapol_key_len;

    uint32_t keyidx_active:1;
    uint32_t use_ext_key_id:1;
    uint32_t PTK_valid:1;
    uint32_t pairwise_set:1;
    uint32_t tk_already_set:1;
    uint32_t Pair:1;
    uint32_t PInitAKeys:1; /* WPA only, not in IEEE 802.11i */
    uint32_t PTKRequest:1; /* not in IEEE 802.11i state machine */
    uint32_t has_GTK:1;
    uint32_t PtkGroupInit:1; /* init request for PTK Group state machine */

    uint32_t changed:1;
    uint32_t in_step_loop:1;
    uint32_t pending_deinit:1;
    uint32_t started:1;
    uint32_t mgmt_frame_prot:1;
    uint32_t rx_eapol_key_secure:1;
    uint32_t update_snonce:1;
    uint32_t alt_snonce_valid:1;
    uint32_t is_wnmsleep:1;
    uint32_t pmkid_set:1;
    uint32_t pending_1_of_4_timeout:1;
    uint32_t req_replay_counter_used:1;
    uint8_t req_replay_counter[WPA_REPLAY_COUNTER_LEN];

    uint8_t *wpa_ie;   //init TODO
    size_t wpa_ie_len;
    uint8_t *rsnxe;    //init TODO
    size_t rsnxe_len;

    enum wpa_version_t wpa;
    int pairwise; /* Pairwise cipher suite, WPA_CIPHER_* */
    int wpa_key_mgmt; /* the selected WPA_KEY_MGMT_* */
    struct rsn_pmksa_cache_entry *pmksa;

    uint32_t dot11RSNAStatsTKIPLocalMICFailures;
    uint32_t dot11RSNAStatsTKIPRemoteMICFailures;
};
#endif
int wpa_pmk_to_ptk(const uint8_t *pmk, size_t pmk_len, const char *label,
                    const uint8_t *addr1, const uint8_t *addr2,
                    const uint8_t *nonce1, const uint8_t *nonce2,
                    struct wpa_ptk *ptk, int akmp, int cipher,
                    const uint8_t *z, size_t z_len, size_t kdk_len);
int wpas_gen_wpa_or_rsn_ie(struct wpas_eapol *eapol);
int wpas_set_wpa_rsn_ie(struct wpas_eapol *eapol, const uint8_t *data, size_t len);
int wpas_rx_eapol(struct wpas_eapol *eapol, const uint8_t *buf, size_t len);
void wpas_eapol_reset(struct wpas_eapol *eapol);
int wpas_eapol_start(struct wpas_eapol *eapol, uint8_t *data, uint32_t len);
int wpas_eapol_stop(struct wpas_eapol *eapol);
uint8_t * wpa_alloc_eapol(struct wpas_eapol *eapol, uint8_t type,
                        const void *data, uint16_t data_len,
                        size_t *msg_len, void **data_pos);

const uint8_t * wpa_get_ie(uint8_t *data, uint32_t len, uint8_t eid);
uint16_t rsn_supp_capab(struct wpas_eapol *eapol);
void wpa_clear_keys(struct wpas_eapol *eapol, const uint8_t *addr);
void wpa_key_neg_complete(struct wpas_eapol *eapol,
                                const uint8_t *addr, int secure);

/* For softap mode */
#ifdef CFG_SOFTAP
struct wpas_ap;
struct ap_cli;
void wpa_ap_rx_eapol(struct wpas_ap *ap, uint8_t *buf, size_t len, uint8_t *sa);
void wpa_remove_ptk(struct wpas_ap *ap, struct ap_cli *cli);
int wpa_auth_sm_event(struct wpas_ap *ap,
                struct ap_cli *cli, enum wpa_event event);
int wpa_auth_sta_associated(struct wpas_ap *ap,          struct ap_cli *cli);
struct wpa_cli_sm * wpa_auth_sta_sm_init(struct wpas_ap *ap, struct ap_cli *cli);
void wpa_auth_sta_sm_free(struct wpas_ap *ap, struct ap_cli *cli);
void wpa_auth_sta_deinit(struct wpas_ap *ap, struct ap_cli *cli);
int wpa_auth_init(struct wpas_ap *ap);
void wpa_auth_deinit(struct wpas_ap *ap);
int wpa_group_init_keys(struct wpas_ap *ap);
enum wpa_validate_result
wpa_validate_wpa_ie(struct wpas_ap *ap,
            struct ap_cli *cli, int freq,
            const uint8_t *wpa_ie, size_t wpa_ie_len,
            const uint8_t *rsnxe, size_t rsnxe_len,
            const uint8_t *mdie, size_t mdie_len,
            const uint8_t *owe_dh, size_t owe_dh_len);
void wpa_auth_add_sae_pmkid(struct wpa_cli_sm *sm, const uint8_t *pmkid);
int wpa_auth_uses_sae(struct wpa_cli_sm *sm);
int wpa_auth_uses_ft_sae(struct wpa_cli_sm *sm);

/* For softap mode end */
#endif /* CFG_SOFTAP */

#endif /* _WPAS_EAPOL_H_ */
