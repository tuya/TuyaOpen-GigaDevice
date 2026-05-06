/*
 * hostapd / Station table
 * Copyright (c) 2002-2017, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

/*!
    \file    wpas_softap_cli.h
    \brief   Header file for wpas softap client.

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

#ifndef _WPAS_SOFTAP_CLI_H_
#define _WPAS_SOFTAP_CLI_H_

#define WLAN_CLI_WMM                BIT(0)
#define WLAN_CLI_HT                 BIT(1)
#define WLAN_CLI_HE                 BIT(2)
#define WLAN_CLI_NONERP             BIT(3)
#define WLAN_CLI_SHORT_PREAMBLE     BIT(4)
#define WLAN_CLI_AUTHORIZED         BIT(5)
#define WLAN_CLI_MFP                BIT(6)
#define WLAN_CLI_PENDING_POLL       BIT(7)

#define WIFI_CLI_NULL_STATE         BIT(0)
#define WIFI_CLI_AUTH_NONE          BIT(1)
#define WIFI_CLI_AUTH_SUCCESS       BIT(2)
#define WIFI_CLI_ASSOC_STATE        BIT(3)
#define WIFI_CLI_ASSOC_SUCCESS      BIT(4)

/* Default value for maximum station inactivity. After AP_MAX_INACTIVITY has
 * passed since last received frame from the station, a nullfunc data frame is
 * sent to the station. If this frame is not acknowledged and no other frames
 * have been received, the station will be disassociated after
 * AP_DISASSOC_DELAY seconds. Similarly, the station will be deauthenticated
 * after AP_DEAUTH_DELAY seconds has passed after disassociation. */
#define AP_MAX_INACTIVITY (2 * 60 * 1000) // (5 * 60 * 1000)
#define AP_DISASSOC_DELAY (3 * 1000)
#define AP_INACTIVE_CHECK_DELAY (2 * 1000)
#define AP_DEAUTH_DELAY (1 * 1000)
/* Number of mseconds to keep STA entry with Authenticated flag after it has
 * been disassociated. */
#define AP_MAX_INACTIVITY_AFTER_DISASSOC (1 * 30 * 1000)
/* Number of mseconds to keep STA entry after it has been deauthenticated. */
#define AP_MAX_INACTIVITY_AFTER_DEAUTH (1 * 5 * 1000)

enum sta_timeout{
    STA_NULLFUNC = 0, STA_DISASSOC, STA_DEAUTH, STA_REMOVE,
    STA_DISASSOC_FROM_CLI
} ;

struct ap_cli
{
    struct ap_cli *next;
    struct mac_addr addr;

    /* Last Authentication/(Re)Association Request/Action frame sequence
     * control */
    uint16_t last_seq_ctrl;
    /* Last Authentication/(Re)Association Request/Action frame subtype */
    uint8_t last_subtype;
    uint8_t cli_state;
    uint8_t supported_rates[32];
    uint8_t supported_rates_len;
    uint8_t auth_alg;
    uint16_t aid; /* STA's unique AID (1 .. 2007) or 0 if not yet assigned */
    uint16_t disconnect_reason_code; /* RADIUS server override */
    uint8_t qosinfo; /* Valid when WLAN_CLI_WMM is set */
    uint16_t capability;
    uint16_t listen_interval; /* or beacon_intv for APs */
    uint32_t flags; /* Bitfield of WLAN_CLI_* */
    struct ieee80211_ht_capabilities *ht_capabilities;
    struct ieee80211_he_capabilities *he_capab;
    size_t he_capab_len;

    uint8_t nonerp_set:1;
    uint8_t no_short_slot_time_set:1;
    uint8_t no_short_preamble_set:1;
    uint8_t no_ht_gf_set:1;
    uint8_t no_ht_set:1;
    uint8_t ht_20mhz_set:1;

    /* the count of NULL frame we send to check whether the client is inactive or not */
    uint8_t inactive_check_cnt;
    enum sta_timeout timeout_next;
    uint16_t deauth_reason;
    uint16_t disassoc_reason;

    struct sae_data *sae;
    struct wpa_cli_sm *sm;
};

/* Maximum number of supported rates (from both Supported Rates and Extended
 * Supported Rates IEs). */
#define WLAN_SUPP_RATES_MAX 32

struct ap_cli * ap_cli_get(struct wpas_ap *ap, const uint8_t *addr);

struct ap_cli * ap_cli_add(struct wpas_ap *ap, const uint8_t *addr);

void ap_cli_free(struct wpas_ap *ap, struct ap_cli *cli);

void ap_free_cli_all(struct wpas_ap *ap);

int associated_cli_add(struct wpas_ap *ap, struct ap_cli *cli);

int associated_cli_remove(struct wpas_ap *ap, struct mac_addr *addr);

void ap_cli_set_authorized(struct wpas_ap *ap, struct ap_cli *cli, int authorized);

void ap_cli_disconnect(struct wpas_ap *ap, struct ap_cli *cli, uint16_t reason);

void ap_handle_timer(void *eloop_ctx, void *timeout_ctx);

#endif /* _WPAS_SOFTAP_CLI_H_ */
