/*!
    \file    macif_vif.h
    \brief   Definition of MACIF VIF API.

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

#ifndef _MACIF_VIF_H_
#define _MACIF_VIF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "macif_types.h"

// For STA
#define MAC_STA_MGMT_RX_FILTER ~(CO_BIT(WLAN_FC_STYPE_ACTION) \
                            | CO_BIT(WLAN_FC_STYPE_AUTH) \
                            | CO_BIT(WLAN_FC_STYPE_DEAUTH) \
                            | CO_BIT(WLAN_FC_STYPE_DISASSOC))

// For AP accept everyting except beacon
#define MAC_AP_MGMT_RX_FILTER CO_BIT(WLAN_FC_STYPE_BEACON)

/**
 ****************************************************************************************
 * @brief Get VIF type
 *
 * @param[in] vif_idx index of VIF
 *
 * @return VIF type.
 ****************************************************************************************
 */
enum mac_vif_type macif_vif_type_get(uint32_t vif_idx);

/**
 ****************************************************************************************
 * @brief Get VIF RSSI for STA mode
 *
 * @param[in] vif_idx index of VIF
 *
 * @return RSSI.
 ****************************************************************************************
 */
int8_t macif_vif_sta_rssi_get(uint32_t vif_idx);

/**
 ****************************************************************************************
 * @brief Get VIF AP state for AP mode
 *
 * @param[in] vif_idx index of VIF
 *
 * @return AP state.
 ****************************************************************************************
 */
enum wifi_ap_state macif_vif_ap_state_get(uint32_t vif_idx);

/**
 ****************************************************************************************
 * @brief Set VIF AP state for AP mode
 *
 * @param[in] vif_idx index of VIF
 * @param[in] ap_state AP state to set
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
int32_t macif_vif_ap_state_set(uint32_t vif_idx, enum wifi_ap_state ap_state);

/**
 ****************************************************************************************
 * @brief Get VIF channel context channel information
 *
 * @param[in] vif_idx index of VIF
 *
 * @return context channel.
 ****************************************************************************************
 */
struct mac_chan_op *macif_vif_chan_ctxt_chan_get(uint32_t vif_idx);

/**
 ****************************************************************************************
 * @brief Set wpa rx filter
 *
 * @param[in] vif_idx index of VIF
 * @param[in] rx_filter wpa rx filter
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
int32_t macif_vif_wpa_rx_filter_set(uint32_t vif_idx, uint32_t rx_filter);

/**
 ****************************************************************************************
 * @brief Set AP isolation mode
 *
 * @param[in] vif_idx index of VIF
 * @param[in] isolation_mode
 *                - true: don't route unciast traffic between AP clients at MAC level
 *                - false (default): route unicast traffic between AP client at MAC level
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
int32_t macif_vif_ap_isolation_set(uint32_t vif_idx, bool isolation_mode);

/**
 ****************************************************************************************
 * @brief Check if input channel is the same as the current channel
 *
 * @param[in] vif_idx index of VIF
 * @param[in] channel for check
 *
 * @return ture if it is the current channel.
 ****************************************************************************************
 */
 int macif_vif_current_chan_get(uint32_t vif_idx, uint8_t *channel);

/**
 ****************************************************************************************
 * @brief Get mac vif status
 *
 * @param[in] vif_idx index of VIF
 * @param[out] status the mac vif status
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
int macif_vif_status_get(int vif_idx, struct mac_vif_status *status);

/**
 ****************************************************************************************
 * @brief Get the associated client MAC addresses of the SoftAP
 *
 * @param[in] vif_idx index of VIF
 * @param[out] mac_info the associated client MAC addresses of the SoftAP
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
int macif_vif_ap_assoc_info_get(uint32_t vif_idx, uint16_t *mac_info);

/**
 ****************************************************************************************
 * @brief Get the wireless mode for STA and Softap
 *
 * @param[in] vif_idx index of VIF
 *
 * @return wifi_wireless_mode
 ****************************************************************************************
 */
enum wifi_wireless_mode macif_vif_wireless_mode_get(uint32_t vif_idx);

/**
 ****************************************************************************************
 * @brief set wireless mode for the station
 *
 * @param[in] wirelss mode
 ****************************************************************************************
 */
void macif_vif_wireless_mode_set(uint32_t wireless_mode);

/**
 ****************************************************************************************
 * @brief get roaming rssi threshold
 *
 * @param[in] wirelss mode
 ****************************************************************************************
 */
int8_t macif_vif_roaming_rssi_get(uint32_t vif_idx);

/**
 ****************************************************************************************
 * @brief get ap country code
 *
 * @param[in] vif_idx index of VIF
 *
 * @return wifi_wireless_mode
 ****************************************************************************************
 */
uint8_t macif_vif_country_code_get(uint32_t vif_idx);

#ifdef CFG_WIFI_MESH_SMART
/**
 ****************************************************************************************
 * @brief get vendor ie
 *
 * @param[in] vif_idx index of VIF
 * @param[out] vendor_ie the vendor ie buffer
 * @param[in] max_len the max length of vendor ie buffer
 *
 * @return length of vendor ie, -1 if error occurred.
 ****************************************************************************************
 */
int8_t macif_vif_vendor_ie_get(uint32_t vif_idx, uint8_t *vendor_ie, uint8_t max_len);
#endif

#ifdef __cplusplus
 }
#endif

#endif // _MACIF_VIF_H_
