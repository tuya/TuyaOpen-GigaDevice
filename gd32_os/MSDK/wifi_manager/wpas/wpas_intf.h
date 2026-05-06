/*!
    \file    wpas_intf.h
    \brief   Header file for wpas interface.

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

#ifndef _WPAS_INTF_H_
#define _WPAS_INTF_H_

#include "macif_types.h"

int wpas_eapol_send(int vif_idx, const uint8_t *dst_addr,
                    uint16_t proto, const uint8_t *data, size_t data_len);
int wpas_set_mac_ctrl_port(int vif_idx, uint8_t *addr, int authorized);
int wpas_set_mac_key(int vif_idx, enum wpa_alg alg,
                            const uint8_t *addr, int key_idx,
                            const uint8_t *seq, size_t seq_len,
                            const uint8_t *key, size_t key_len,
                            enum key_flag key_flag, uint32_t *keys_cleared);

int wpas_pre_set_mac_key(int vif_idx, enum wpa_alg alg,
                            const uint8_t *addr, int key_idx,
                            const uint8_t *seq, size_t seq_len,
                            const uint8_t *key, size_t key_len,
                            enum key_flag key_flag, uint32_t *keys_cleared);

int wpas_get_mac_key_seqnum(int vif_idx, const uint8_t *addr, int idx, uint8_t *seq);
int wpas_get_mac_inact_msec(int vif_idx, uint8_t *addr, uint32_t *inactive_time);
int wpas_set_mac_probe_client(int vif_idx, uint8_t *addr);
int wpas_get_mac_hw_feature(void *me_config, void *me_chan);
int wpas_set_mac_edca(int vif_idx, uint8_t queue, uint8_t aifs,
                        uint16_t cwmin, uint16_t cwmax, uint16_t txop);
int wpas_start_mac_ap(int vif_idx, uint8_t *bcn, int bcn_len, int tim_oft, int tim_len);
int wpas_update_mac_bcn(int vif_idx, uint8_t *bcn, int bcn_len, int tim_oft, int tim_len);
int wpas_stop_mac_ap(int vif_idx);
int wpas_remove_mac_sta(int vif_idx, struct mac_addr *addr);
int wpas_add_mac_sta(int vif_idx, struct ap_cli *cli);
int wpas_set_mac_ext_auth_resp(int vif_idx, int status);
int wpas_get_mac_scan_result(int vif_idx, uint8_t *bssid, struct mac_scan_result *candidate);
int wpas_set_mac_connect(int vif_idx, struct mac_scan_result *candidate,
                                uint8_t *extra_ie, uint32_t extra_ie_len, bool is_wps);
int wpas_set_mac_disconnect(int vif_idx, uint16_t reason);
int wpas_set_wep_key(int vif_idx, uint8_t key_idx, uint8_t *wep_key, uint8_t wep_key_len);

#ifdef CONFIG_IEEE80211R
int wpas_set_ft_ies(int vif_idx, uint8_t *ie, uint32_t ie_len);
#endif /* CONFIG_IEEE80211R */
#endif /* _WPAS_INTF_H_ */
