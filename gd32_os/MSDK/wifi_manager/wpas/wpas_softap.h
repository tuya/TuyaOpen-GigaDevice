/*!
    \file    wpas_softap.h
    \brief   Header file for wpas softap.

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

#ifndef _WPAS_SOFTAP_H_
#define _WPAS_SOFTAP_H_

/*============================ MACROS ========================================*/
#define AP_RATE_BASIC          0x00000001

#define AP_CHAN_WIDTH_20       0x00000001

#define AP_CHAN_DISABLED       0x00000001
#define AP_CHAN_NO_IR          0x00000002
#define AP_CHAN_RADAR          0x00000008

#define HE_MAX_MAC_CAPAB_SIZE       6
#define HE_MAX_PHY_CAPAB_SIZE       11
#define HE_MAX_MCS_CAPAB_SIZE       12
#define HE_MAX_PPET_CAPAB_SIZE      25

#define NUM_TX_QUEUES_AP            4

#ifndef ETH_P_PAE
#define ETH_P_PAE       0x888E /* Port Access Entity (IEEE 802.1X) */
#endif /* ETH_P_PAE */

#define MAX_STA_NUM                 CFG_STA_NUM

#define WPA_KEY_RSC_LEN 8
/*============================ TYPES =========================================*/
struct ap_wmm_ac_params {
    uint8_t cwmin;
    uint8_t cwmax;
    uint8_t aifs;
    uint8_t admission_control_mandatory;
    uint16_t txop_limit; /* in units of 32us */
};

struct ap_rate_data {
    int rate; /* rate in 100 kbps */
    int flags; /* AP_RATE_ flags */
};

struct ap_freq_params
{
    // freq - Primary channel center frequency in MHz
    int freq;
    // sec_channel_offset - Secondary channel offset for HT40
    // 0 = HT40 disabled, -1 = secondary channel below, 1 = secondary channel above
    int sec_channel_offset;
    // Segment 0 center frequency in MHz
    int center_freq1;
    // Segment 1 center frequency in MHz, Non-zero only for bandwidth 80 and an 80+80 channel
    int center_freq2;
    // Band (2.4GHz, 5GHz)
    uint8_t band;
    // Channel bandwidth in MHz (20, 40, 80, 160)
    uint8_t bandwidth;
};

struct ap_channel_data {
    // chan - Channel number (IEEE 802.11)
    uint8_t chan;
    // freq - Frequency in MHz
    uint16_t freq;
    // max_tx_power - Regulatory transmit power limit in dBm
    uint8_t max_tx_power;
    // flag - Channel flags (AP_CHAN_*)
    int flag;
    // allowed_bw - Allowed channel width bitmask
    uint32_t allowed_bw;
};

struct he_capabilities {
    uint8_t he_supported;
    uint8_t phy_cap[HE_MAX_PHY_CAPAB_SIZE];
    uint8_t mac_cap[HE_MAX_MAC_CAPAB_SIZE];
    uint8_t mcs[HE_MAX_MCS_CAPAB_SIZE];
    uint8_t ppet[HE_MAX_PPET_CAPAB_SIZE];
};

// struct he_phy_capabilities_info - HE PHY capabilities
struct he_phy_capabilities_info {
    uint8_t he_su_beamformer;
    uint8_t he_su_beamformee;
    uint8_t he_mu_beamformer;
};

// struct he_operation - HE operation
struct he_operation {
    uint8_t he_bss_color;
    uint8_t he_bss_color_disabled;
    uint8_t he_bss_color_partial;
    uint8_t he_default_pe_duration;
    uint8_t he_twt_required;
    uint8_t he_twt_responder;
    uint16_t he_rts_threshold;
    uint8_t he_er_su_disable;
    uint16_t he_basic_mcs_nss_set;
};

struct ap_tx_queue_params {
    int aifs;
    int cwmin;
    int cwmax;
    int burst; /* maximum burst time in 0.1 ms, i.e., 10 = 1 ms */
};

struct ap_capa_info
{
    uint32_t beacon_intv;
    uint8_t dtim_period;
    uint8_t ignore_broadcast_ssid;
    uint16_t num_rates;
    int *basic_rates;
    struct ap_rate_data *current_rates;

    uint32_t short_ssid;

    // preamble - Whether short preamble is enabled
    bool preamble;

    uint16_t max_listen_interval;

    // num_channels - Number of entries in the channels array
    uint8_t num_channels;
    // channels - Array of supported channels
    struct ap_channel_data *channels;

    // ht_capab - HT (IEEE 802.11n) capabilities
    uint16_t ht_capab;
    // mcs_set - MCS (IEEE 802.11n) rate parameters
    uint8_t mcs_set[16];
    // a_mpdu_params - A-MPDU (IEEE 802.11n) parameters
    uint8_t a_mpdu_params;
    // HT (IEEE 802.11n) operation
    uint16_t ht_op_mode;
    // he_capab - HE (IEEE 802.11ax) capabilities
    struct he_capabilities he_capab;
    struct he_phy_capabilities_info he_phy_capab;
    struct he_operation he_op;

    // Extended capabilities
    uint8_t extended_capab[10];
    // Extended capabilities mask
    uint8_t extended_capab_mask[10];

    bool wmm_enabled;
    bool wmm_uapsd;
    struct ap_wmm_ac_params wmm_ac_params[WMM_AC_NUM];

    struct ap_tx_queue_params tx_queue[NUM_TX_QUEUES_AP];
};

struct ap_bss_info {
    // current frequency info
    struct ap_freq_params freq;
    /*
     * Bitfield for indicating which AIDs are allocated. Only AID values
     * 1-2007 are used and as such, the bit at index 0 corresponds to AID
     * 1.
     */
//#define AID_WORDS ((2008 + 31) / 32)
#define AID_WORDS (1)
    uint32_t cli_aid[AID_WORDS];

    uint8_t cli_num_max;
    // Number of associated stations that do not support Short Slot Time
    uint8_t num_cli_no_short_slot_time;
    // Number of associated stations that do not support Short Preamble
    uint8_t num_cli_no_short_preamble;
    // Number of associated Non-ERP stations (i.e., stations using 802.11b in 802.11g BSS)
    uint8_t num_cli_non_erp;
    /* Number of HT associated stations that do not support greenfield */
    uint8_t num_cli_ht_no_gf;
    /* Number of associated non-HT stations */
    uint8_t num_cli_no_ht;
    /* Number of HT associated stations 20 MHz */
    uint8_t num_cli_ht_20mhz;
    /* WMM parameters*/
    // Previous WMM element information
    struct ap_wmm_ac_params prev_wmm[WMM_AC_NUM];
    int parameter_set_count;
};

struct wpa_psk {
    uint8_t psk[PMK_LEN];
};

struct ap_security
{
    uint32_t key_mgmt;
    uint16_t pairwise_cipher;
    uint16_t group_cipher;
    uint16_t mgmt_group_cipher;

    uint8_t auth_algs;
    uint8_t wpa_proto;

    uint8_t disable_gtk;
    uint8_t beacon_prot;
    uint8_t transition_disable;

    uint8_t ieee80211w;
    uint8_t sae_require_mfp;

    uint8_t *wpa_ie;
    size_t wpa_ie_len;
    struct wpa_psk *wpa_psk;
};

struct ap_sae_info {
    uint32_t sae_sync;
    dlist_t sae_commit_queue;  //Init TODO
};

struct ap_wpa_info {
    struct wpa_group *group;

    uint32_t dot11RSNAStatsTKIPRemoteMICFailures;
    uint32_t dot11RSNAAuthenticationSuiteSelected;
    uint32_t dot11RSNAPairwiseCipherSelected;
    uint32_t dot11RSNAGroupCipherSelected;
    //uint8_t dot11RSNAPMKIDUsed[PMKID_LEN];
    uint32_t dot11RSNAAuthenticationSuiteRequested; /* FIX: update */
    uint32_t dot11RSNAPairwiseCipherRequested; /* FIX: update */
    uint32_t dot11RSNAGroupCipherRequested; /* FIX: update */
    uint32_t dot11RSNATKIPCounterMeasuresInvoked;
    uint32_t dot11RSNA4WayHandshakeFailures;
};

struct wpas_ap {
    uint32_t cli_num;
    struct ap_cli *cli;

    struct ap_security ap_sec;
    struct ap_sae_info ap_sae;
    struct ap_capa_info *ap_capa;
    struct ap_bss_info *ap_bss;
    struct ap_wpa_info *ap_eapol;

    struct rsn_pmksa_cache ap_cache;
};

struct handle_mgmt_cb_params {
    int vif_idx;
    // frame data length
    size_t data_len;
    // frame data, actual buffer allocated is data_len bytes long
    uint8_t data[0];
};

enum ssid_match_result {
    NO_SSID_MATCH,
    EXACT_SSID_MATCH,
    WILDCARD_SSID_MATCH,
    CO_LOCATED_SSID_MATCH,
};

/*============================ PROTOTYPES ====================================*/

int wpas_ap_start(int vif_idx);

int wpas_ap_stop(int vif_idx, uint16_t deauth_reason);

void handle_ieee802_11_mgmt(struct wpas_ap * ap, struct wifi_frame_info *info, struct ieee80211_mgmt *mgmt);

void ap_mgmt_tx_cb_handler(struct wpas_ap *ap, uint8_t *data, size_t len, uint8_t *ack);

int ap_update_bcn(struct wpas_ap *ap, uint8_t *vendor_ie, int vendor_ie_len);

int ap_send_deauth(struct wpas_ap * ap, const uint8_t *addr, int reason);

int ap_send_disassoc(struct wpas_ap * ap, const uint8_t *addr, int reason);

int ap_ht_operation_update(struct wpas_ap *ap);

void handle_auth(struct wpas_ap * ap, const struct ieee80211_mgmt *mgmt, size_t len, int rssi, int from_queue);

void auth_timeout_func(void *eloop_data, void *user_ctx);

void assoc_timeout_func(void *eloop_data, void *user_ctx);

#endif /* _WPAS_SOFTAP_H_ */
