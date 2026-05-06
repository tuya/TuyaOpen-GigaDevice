/*!
    \file    wpas_includes.h
    \brief   Header file for wpas includes.

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

#ifndef _WPAS_INCLUDES_H_
#define _WPAS_INCLUDES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "macif_api.h"
#include "ll.h"
#include "wrapper_os.h"
#include "wlan_config.h"
#include "build_config.h"

#include "gd32vw55x.h"

#include "ieee802_11_defs.h"
#include "wpas_comm.h"
#include "wpas_sae_crypto.h"
#include "wpas_aes.h"
#include "wpas_crypto.h"
#include "wpas_debug.h"
#include "wpas_sae.h"
#include "wpas_eapol.h"
#include "wpas_pmksa_cache.h"
#include "wpas_softap_cli.h"
#include "wpas_softap.h"
#include "wpas_intf.h"
#include "wpas_sa_query.h"
#include "wpas_buf.h"
#include "wpas_eap.h"
#include "wpas_eap_peer.h"
#include "wpas_eap_tls.h"
#include "wpas_wps.h"
#include "wpas_eap_wsc.h"
#include "wpas_ft.h"


/* wifi vif apis */
extern int wvif_to_vif_idx(void *wvif);
extern void *vif_idx_to_net_if(uint8_t vif_idx);
extern uint8_t * wifi_vif_mac_addr_get(int vif_idx);
extern int wifi_vif_is_sta_handshaked(int vif_idx);
extern int wifi_vif_is_sta_connected(int vif_idx);
extern int wifi_vif_is_cli_connected(struct wpas_ap *ap, uint8_t *sa);
extern int wifi_vif_sta_status_code_get(int vif_idx);

/* lwip apis */
extern int net_l2_send(void *net_if, const uint8_t *data, int data_len, uint16_t ethertype,
                const uint8_t *dst_addr, bool *ack);

/* wifi wpa apis */
extern void wifi_wpa_rx_mgmt_cb(struct wifi_frame_info *info, void *arg);
extern int wifi_wpa_eapol_to_vif_idx(struct wpas_eapol *eapol);
extern uint16_t wifi_wpa_get_connect_fail_status(void);
extern uint16_t wifi_wpa_get_disconnect_reason(void);
extern int wifi_wpa_sae_to_vif_idx(struct wpas_sae *sae);
extern int wifi_wpa_sa_query_to_vif_idx(struct sa_query_data *sa_query);
extern int wifi_wpa_ap_to_vif_idx(struct wpas_ap *w_ap);
extern void *wifi_wpa_w_eapol_get(int vif_idx);
extern void *wifi_wpa_w_sae_get(int vif_idx);
extern void *wifi_wpa_w_sa_query_get(int vif_idx);
extern void *wifi_wpa_w_ap_get(int vif_idx);
extern void *wifi_wpa_sta_eapol_cache_get(struct wpas_eapol *eapol);
extern void *wifi_wpa_sta_sae_cache_get(struct wpas_sae *w_sae);
extern int wifi_wpa_eapol_info_get(int vif_idx, struct eapol_info *info);
extern char * wifi_wpa_sta_cfg_ssid_get(int vif_idx, uint32_t *ssid_len);
extern char * wifi_wpa_sta_cfg_passphrase_get(int vif_idx, uint32_t *pwd_len);
extern uint8_t * wifi_wpa_sta_cfg_bssid_get(int vif_idx);
extern uint8_t wifi_wpa_sta_cfg_flush_cache_req_get(int vif_idx);
extern void *wifi_wpa_sta_ft_params_get(struct wpas_eapol *eapol);
extern void wifi_wpa_sta_ft_params_free(struct wpas_eapol *eapol);
extern void wifi_wpa_ft_ies_set(int vif_idx, uint8_t *ie, uint32_t ie_len);

extern char * wifi_wpa_ap_cfg_ssid_get(struct wpas_ap *w_ap, uint32_t *ssid_len);
extern char * wifi_wpa_ap_cfg_passphrase_get(struct wpas_ap *w_ap, uint32_t *pwd_len);
extern uint8_t * wifi_wpa_ap_cfg_bssid_get(struct wpas_ap *w_ap);
extern uint8_t wifi_wpa_ap_cfg_he_disabled_get(struct wpas_ap *w_ap);
extern uint8_t wifi_wpa_ap_cfg_mfp_get(struct wpas_ap *w_ap);
extern uint8_t wifi_wpa_ap_cfg_channel_get(struct wpas_ap *w_ap);
extern uint8_t wifi_wpa_ap_cfg_dtim_period_get(struct wpas_ap *w_ap);
extern uint8_t wifi_wpa_ap_cfg_bcn_interval_get(struct wpas_ap *w_ap);
extern uint8_t wifi_wpa_ap_cfg_hidden_get(struct wpas_ap *w_ap);
extern uint32_t wifi_wpa_ap_cfg_akm_get(struct wpas_ap *w_ap);
extern uint8_t wifi_wpa_ap_cfg_max_conn_get(struct wpas_ap *w_ap);
extern uint32_t wifi_wpa_sta_cfg_akm_get(int vif_idx);
extern int wifi_wpa_send_connect_fail_event(int vif_idx);
extern int wifi_wpa_send_rx_mgmt_done_event(int vif_idx, uint8_t *param, uint32_t param_len);
extern int wifi_wpa_send_disconnect_event(int vif_idx, uint8_t *param, uint32_t param_len);
extern int wifi_wpa_send_client_add_event(int vif_idx, uint8_t *param, uint32_t param_len);
extern int wifi_wpa_send_client_remove_event(int vif_idx, uint8_t *param, uint32_t param_len);
extern int wifi_wpa_send_eap_success_event(int vif_idx);
#ifdef CONFIG_EAP_TLS
extern void *wifi_wpa_sta_eap_ctx_get(int vif_idx);
#endif /* CONFIG_EAP_TLS */
#ifdef CONFIG_WPS
extern void *wifi_wpa_sta_wps_ctx_get(int vif_idx);
extern int wifi_wpa_send_wps_cred_event(int vif_idx, struct wps_credential *cred);
extern int wifi_wpa_send_wps_success_event(int vif_idx);
extern int wifi_wpa_send_wps_fail_event(int vif_idx);
#endif /* CONFIG_WPS */

/* wifi eloop apis */
typedef void (*eloop_timeout_handler)(void *eloop_data, void *user_ctx);
extern int eloop_timeout_register(unsigned int msecs,
               eloop_timeout_handler handler,
               void *eloop_data, void *user_data);
extern int eloop_timeout_cancel(eloop_timeout_handler handler,
             void *eloop_data, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* _WPAS_INCLUDES_H_*/
