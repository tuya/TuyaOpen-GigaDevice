/*!
    \file    wifi_vif.h
    \brief   Header file of wifi virtual interface.

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

#ifndef _WIFI_VIF_H_
#define _WIFI_VIF_H_

#include "app_cfg.h"
#include "wlan_config.h"
#include "build_config.h"

#include "wrapper_os.h"
#include "wifi_netif.h"
#include "lwip/netif.h"
#include "macif_api.h"

#ifndef CONFIG_WPA_SUPPLICANT
#include "wpas_includes.h"
#endif

/*============================ MACROS ========================================*/
#define WIFI_SSID_MAX_LEN       32
#define WIFI_ALEN               6
#define WIFI_HOSTNAME_MAX_LEN   32

#define WPAS_MIN_PASSPHRASE_LEN 8
#define WPAS_MAX_PASSPHRASE_LEN 63
#define WPAS_WEP40_ASCII_LEN        5
#define WPAS_WEP40_HEX_LEN          10
#define WPAS_WEP104_ASCII_LEN       13
#define WPAS_WEP104_HEX_LEN         26

/*============================ TYPES =========================================*/
enum wifi_vif_type {
    WVIF_UNKNOWN,
    WVIF_STA,
    WVIF_AP,
    WVIF_MONITOR,
};

enum wvif_ap_state
{
    WIFI_AP_STATE_INIT = 0,
    WIFI_AP_STATE_STARTED,
    // WIFI_AP_STATE_BEFORE_CHANNEL_SWITCH,  //TODO, now just stop and restart ap
    WIFI_AP_STATE_UNKNOWN,
};

enum wvif_sta_state
{
    WIFI_STA_STATE_IDLE = 0,
    WIFI_STA_STATE_WPS,
    WIFI_STA_STATE_SCAN,
    WIFI_STA_STATE_CONNECT,
    WIFI_STA_STATE_HANDSHAKE,
    WIFI_STA_STATE_IP_GETTING,
    WIFI_STA_STATE_CONNECTED,
    WIFI_STA_STATE_UNKNOWN,
};

enum wvif_sta_ps_mode
{
    WIFI_STA_PS_MODE_OFF = 0,
    WIFI_STA_PS_MODE_ALW_ON,
    WIFI_STA_PS_MODE_BASED_ON_TD,
};

#ifdef CFG_WPS
struct wps_config_t {
    /**
     * registrar - Whether this end is a Registrar
     */
    uint8_t registrar;

    /**
     * pin - Enrollee Device Password (%NULL for Registrar or PBC)
     */
    uint8_t pin[9];

    /**
     * pbc - Whether this is protocol run uses PBC
     */
    uint8_t pbc;
};

struct wps_cred_t {
    char ssid[WIFI_SSID_MAX_LEN];
    uint8_t ssid_len;
    char passphrase[WPA_MAX_PSK_LEN];
    uint8_t passphrase_len;
    uint8_t channel;
};
#endif /* CFG_WPS */

#ifdef CFG_8021x_EAP_TLS
struct eap_config_t {
    uint8_t conn_with_enterprise;
    const char *ca_cert;
    const char *client_key;
    const char *client_key_password;
    const char *identity;
    uint8_t identity_len;
    const char *client_cert;
    const char *phase1;
};
#endif /* CFG_8021x_EAP_TLS */

/// structure of mdie parameters
struct md_ie
{
    uint8_t mobility_domain[MAC_INFOELT_MDE_MDID_LEN];
    uint8_t ft_capab;
};

struct sta_cfg {
    char ssid[WIFI_SSID_MAX_LEN + 1];
    uint8_t ssid_len;
    uint8_t channel;
    uint8_t bw;  /* enum mac_chan_bandwidth */
    uint8_t bssid[WIFI_ALEN];
    uint8_t conn_with_bssid;
    uint32_t akm;       /* bit-field of @ref mac_akm_suite */
    uint32_t g_cipher;  /* bit-field of @ref mac_cipher_suite */
    uint32_t p_cipher;  /* bit-field of @ref mac_cipher_suite */
    char passphrase[WPA_MAX_PSK_LEN + 1];
    uint8_t passphrase_len;
#ifdef CFG_8021x_EAP_TLS
    struct eap_config_t eap_cfg;
#endif
#ifdef CFG_WPS
    struct wps_config_t wps_cfg;
#endif
    uint8_t mfpr; /* 1: MFP required */
    uint8_t mfpr_user_setted; /* whether mfpr is set by user */
    uint8_t conn_blocked;
    uint8_t flush_cache_req;
    uint8_t scan_mode; /* 0: fast scan (connect to first found AP), 1: all channel scan (connect to strongest AP) */
    uint8_t scan_mode_user_setted; /* whether scan_mode is set by user */
    uint8_t pci_en; /* PCI certification: 0=allow all encryption (including OPEN/WEP), 1=exclude OPEN/WEP */
    struct md_ie md;
};  //24 dwords

struct wifi_sta
{
    struct sta_cfg cfg;

    enum wvif_sta_state state;
    uint8_t ap_id; // Index of the STA being the AP peer of the device
    // UAPSD queue config for STA interface (bitfield, same format as QoS info)
    uint8_t uapsd_queues;
    uint16_t aid;
    uint32_t last_reason;
    uint16_t reason_code;
    uint16_t status_code;
    uint32_t history_ip;  // shorten dhcp time
    uint8_t psmode;      // enum wvif_sta_ps_mode

#ifndef CONFIG_WPA_SUPPLICANT
    struct wpas_eapol w_eapol;
    struct wpas_sae w_sae;
    struct rsn_pmksa_cache cache;
    struct sa_query_data sa_query;
    // 802.11r fast transition
    struct ft_params *ft;
#ifdef CFG_WPS
    struct wps_context *wps_ctx;
#endif
#ifdef CFG_8021x_EAP_TLS
    struct eap_context *eap_ctx;
#endif
#if defined (CONFIG_WPS) || defined(CFG_8021x_EAP_TLS)
    struct eapol_sm *esm;
#endif
#endif /* not defined CONFIG_WPA_SUPPLICANT */
};

/**
 * SAE Public Key configuration.
 * SAE PK is an extension to SAE authentication that allow to authenticate
 * an AP based on a static public/private key pair.
 * The AP password acts as a fingerprint of the Public Key, the modifier value
 * and the SSID. It can then no longer be randomly selected and must be
 * recomputed each time the key or SSID is updated.
 *
 * @note Generating a new password takes a lot of times (several minutes)
 */
struct ap_sae_pk
{
    /**
     * Whether SAE Public Key should be enabled.
     * If false other fields in this structure are ignored.
     */
    bool enable;
    /**
     * Pointer to the Private key to use for SAE PK (DER encoding of ASN.1
     * ECPrivateKey without public key).
     * If NULL, a new key is generated using p-256 curve (i.e. sae group 19)
     * and this pointer will be updated to point to the new key. It is user
     * responsibility to save (if wanted) and free this new key.
     * If a new key is generated, then a new password is always computed and
     * saved in @p key field (of @ref wifi_vif_ap_cfg).
     *
     * If a key is provided, a new password is computed only if no password
     * is provided in the @p key field (of @ref wifi_vif_ap_cfg).
     * If a password is provided it is assumed that it has been computed for
     * the key,modifier and SSID configured (otherwise clients using SAE-PK
     * won't trust this AP).
     */
    uint8_t *private_key;
    /**
     * If @p private_key is not NULL, size if byte of the buffer.
     * If a new key is generated, then this value is updated after calling
     * @ref wifi_configure_vif.
     */
    int private_key_len;
    /**
     * Modifier value for the SSID/private key.
     * If a new password is computed, then the modifier value is used as
     * starting point, unless it is only 0 in which case a new random value
     * is used as starting point.
     * In all case, this buffer is updated after each password computation.
     */
    uint8_t modifier[16];
    /**
     * If a new password is generated, indicate the 'Sec' value to use.
     * Possible values are 3 or 5 and invalid value are treated a '3'
     * Setting 5 will computed a more secure password but will also significantly
     * increase computation time
     */
    int sec;
    /**
     * If a new password is generated, indicate the number of groups of 4 characters
     * to include. Minimum value is 3 and maximum value depends of the private key
     * curve (12 for p-256). Value lower than 3 as treated as 3 and too big are capped
     * to the maximum value.
     */
    int nb_part;
};

struct ap_cfg {
    char ssid[WIFI_SSID_MAX_LEN + 1];
    uint8_t ssid_len;
    uint8_t channel;  //20M only
    uint8_t hidden;
    char passphrase[WPA_MAX_PSK_LEN + 1];
    uint8_t passphrase_len;
    uint8_t bssid[WIFI_ALEN];
    uint8_t mfp;
    uint32_t akm;
    uint8_t he_disabled;
    uint8_t bcn_interval;
    uint8_t dtim_period;
    uint8_t max_conn;  // Maximum number of stations allowed to connect
#ifdef CONFIG_WPA_SUPPLICANT
    struct ap_sae_pk sae_pk;
#endif
};  //24 dwords

struct wifi_ap
{
    struct ap_cfg cfg;
    enum wvif_ap_state ap_state;

#ifndef CONFIG_WPA_SUPPLICANT
    struct wpas_ap w_ap;
#endif /* not defined CONFIG_WPA_SUPPLICANT */
};

/**
 * Function prototype for RX callbacks
 */
struct wifi_monitor
{
    uint8_t channel;  //20M only
    bool uf;
    cb_macif_rx cb;
    void *cb_arg;
};

// Structure containing the information about the virtual interfaces
struct wifi_vif_tag
{
    // TCPIP network interface structure
    struct netif net_if;

    // MAC address of the VIF
    struct mac_addr mac_addr;

    // Pointer to the MAC VIF structure
    void *mac_vif;

    // Parameter to indicate if admission control is mandatory for any access category -
    // TODO rework
    uint8_t acm;

    // WVIF type
    enum wifi_vif_type wvif_type;

    // User-defined hostname (for STA mode)
    char user_hostname[WIFI_HOSTNAME_MAX_LEN + 1];

    // WiFi Network information
    union {
        struct wifi_sta sta;
        struct wifi_ap ap;
        struct wifi_monitor monitor;
    };
#ifdef CONFIG_WPA_SUPPLICANT
    struct wifi_wpa_vif_tag *wpa_vif;
#endif
};

#include "wifi_wpa.h"

/*============================ GLOBAL VARIABLES ==============================*/
extern struct wifi_vif_tag wifi_vif_tab[CFG_VIF_NUM];

/*============================ PROTOTYPES ====================================*/
void wifi_vif_init(int vif_idx, struct mac_addr *base_mac_addr);
int wifi_vifs_init(struct mac_addr *base_mac_addr);
void wifi_vifs_deinit(void);
int wifi_vif_type_set(int vif_idx, enum wifi_vif_type wvif_type);
int wifi_vif_name(int vif_idx, char *name, int len);
void wifi_vif_reset(int vif_idx, enum wifi_vif_type type);
void *vif_idx_to_mac_vif(uint8_t vif_idx);
void *wvif_to_mac_vif(void *wvif);
void *vif_idx_to_net_if(uint8_t vif_idx);
void *vif_idx_to_wvif(uint8_t vif_idx);
int wvif_to_vif_idx(void *wvif);
uint8_t wifi_vif_sta_uapsd_get(int vif_idx);
int wifi_vif_sta_status_code_get(int vif_idx);
int wifi_vif_uapsd_queues_set(int vif_idx, uint8_t uapsd_queues);
uint8_t * wifi_vif_mac_addr_get(int vif_idx);
void wifi_vif_mac_vif_set(int vif_idx, void *mac_vif);
int wifi_vif_is_softap(int vif_idx);
int wifi_vif_is_sta_connecting(int vif_idx);
int wifi_vif_is_sta_handshaked(int vif_idx);
int wifi_vif_is_sta_connected(int vif_idx);
int wifi_vif_idx_from_name(const char *name);
uint32_t wifi_vif_history_ip_get(void);
void wifi_vif_user_addr_set(uint8_t *user_addr);
int wifi_vif_hostname_set(int vif_idx, const char *hostname);
const char *wifi_vif_hostname_get(int vif_idx);
int wifi_vif_ap_set_max_conn(int vif_idx, uint8_t max_conn);

#endif /* _WIFI_VIF_H_ */
