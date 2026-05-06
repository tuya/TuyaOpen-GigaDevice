/*!
    \file    macif_types.h
    \brief   Types definition for MACIF.

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

#ifndef _MACIF_TYPES_H_
#define _MACIF_TYPES_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdio.h>
#include "mac_types.h"
#include "lwip_import.h"
#include "rtos_import.h"

#define WIFI_VIF_INDEX_DEFAULT      0

/* FOR WIFI CONCURRENT */
#define WIFI_VIF_INDEX_STA_MODE     0
#define WIFI_VIF_INDEX_SOFTAP_MODE  1

/**
 * wifi task identifier
 */
enum wifi_task_id
{
    WIFI_CORE_TASK = 0,
    MACIF_RX_TASK,
    MACIF_TX_TASK,
    SUPPLICANT_TASK,
    IP_TASK,
    WIFI_MGMT_TASK,
    MACIF_CONTROL_TASK,
    MAX_TASK,
};

/**
 * Fully hosted interface status
 */
struct mac_vif_status
{
    /**
     * ID on the interface
     */
    int index;
    /**
     * Type of the interface (@ref mac_vif_type)
     */
    int type;
    /**
     * MAC address of the interface
     */
    const uint8_t *mac_addr;
    /**
     * Current operating channel.
     * Reset to 0 if there is no operating channel associated the interface
     * (e.g. non connected STA interface)
     */
    struct mac_chan_op chan;
    union
    {
        /**
         * Fields specific to a STA interface
         */
        struct
        {
            /**
             * BSSID of the AP. (Set to 0 if interface is not connected)
             */
            struct mac_addr bssid;
            struct mac_ssid ssid;
            /**
             * RSSI (in dBm) of the last received beacon. (valid only if connected)
             */
            int8_t rssi;
            uint8_t active;
        } sta;
        /**
         * Fields specific to a AP interface
         */
        struct
        {
            /**
             * state of the softap. (@ref wifi_ap_state)
             */
            uint8_t state;
        } ap;
    };
};

enum wifi_ap_state
{
    /**
     * softap is close.
     */
    AP_CLOSE = 0,
    /**
     * softap is open.
     */
    AP_OPEN,
    /**
     * softap stop because another vif in sta mode will run in the different channel. softap need switch it's channel.
     */
    AP_STOP_BEFORE_CHANNEL_SWITCH,
};

/**
 * Fully hosted frame information
 */
struct wifi_frame_info
{
    /**
     * Interface index that received the frame. (-1 if unknown)
     */
    int vif_idx;
    /**
     * Length (in bytes) of the frame.
     */
    uint16_t length;
    /**
     * Primary channel frequency (in MHz) on which the frame has been received.
     */
    uint16_t freq;
    /**
     * Received signal strength (in dBm)
     */
    int8_t rssi;
    /**
     * Frame payload. Can be NULL if monitor mode is started with @p uf parameter set to
     * true. In this case all other fields are still valid.
     */
    uint8_t *payload;
};

/**
 * DPP bootstrapping methods
 */
enum wifi_dpp_bootstrap_type
{
    /**
     * Use QR code as bootstrap method
     */
    WIFI_DPP_BOOTSTRAP_QRCODE,
    /**
     * Use PublicKey Exchange as bootstrap method
     */
    WIFI_DPP_BOOTSTRAP_PKEX,
};

/**
 * DPP curves (for bootstrapping key).
 * @note: Using high complexity curves may result in timeout during DPP protocol
 */
enum wifi_dpp_curve
{
    /**
     * 256-bit random ECP group (ike group 19)
     */
    DPP_CURVE_PRIME256v1,
    /**
     * 384-bit random ECP group (ike group 20)
     */
    DPP_CURVE_SECP384r1,
    /**
     * 521-bit random ECP group (ike group 21)
     */
    DPP_CURVE_SECP521r1,
    /**
     * 256-bit Brainpool ECP group (ike group 28)
     */
    DPP_CURVE_BRAINPOOLP256r1,
    /**
     * 384-bit Brainpool ECP group (ike group 29)
     */
    DPP_CURVE_BRAINPOOLP384r1,
    /**
     * 512-bit Brainpool ECP group (ike group 30)
     */
    DPP_CURVE_BRAINPOOLP512r1,
};

/**
 * Wireless mode.
 * @note: 11b,11g,11bg,11n,11gn,11bgn,11bgn/ax
 */
enum wifi_wireless_mode
{
    WIRELESS_MODE_UNKNOWN = 0,
    WIRELESS_MODE_11B,
    WIRELESS_MODE_11G,
    WIRELESS_MODE_11BG,
    WIRELESS_MODE_11N,
    WIRELESS_MODE_11GN,
    WIRELESS_MODE_11BGN,
    WIRELESS_MODE_11GN_AX,
    WIRELESS_MODE_11BGN_AX,
};

/**
 * Function prototype for RX callbacks
 */
typedef void (*cb_macif_rx)(struct wifi_frame_info *info, void *arg);

/**
 * Function prototype for frame transmission callbacks
 * - frame_id is the one returned by @ref wifi_send_80211_frame
 * - acknowledged indicates if the frame has been acknowledged by peer (or successfully
 *   sent for multicast frame)
 * - arg is the private argument passed when frame has been pushed for transmission
 */
typedef void (*cb_macif_tx)(uint32_t frame_id, bool acknowledged, void *arg);

#ifdef __cplusplus
 }
#endif

#endif // _MACIF_TYPES_H_
