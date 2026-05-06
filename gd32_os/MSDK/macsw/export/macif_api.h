/*!
    \file    macif_api.h
    \brief   Definition of MACIF API.

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

#ifndef _MACIF_API_H_
#define _MACIF_API_H_

#include "macif_types.h"
#include "mac_frame.h"
#include "mac_cfm.h"
#include "macif_priv.h"
#include "macif_vif.h"

// Structure containing parameters to establish link with cntrl
struct macif_cntrl_link
{
    // CFG queue to get response from CNTRL task
    _queue queue;
    // Socket, for CNTRL task, to send events
    int sock_send;
    // Socket to receive events from CNTRL task
    int sock_recv;
};

// macif cmd message
enum macif_cmd_index
{
    // Sent by wifi manager to retrieve HW capability (param: none)
    MACIF_HW_FEATURE_CMD = 1,
    // Sent by wifi manager to retrieve FW capability (param: none)
    MACIF_GET_CAPA_CMD,
    // Sent by wifi manager to install/remove Encryption key (param: @ref macif_cmd_set_key)
    MACIF_SET_KEY_CMD,
    // Sent by wifi manager to start a SCAN (param: @ref macif_cmd_scan)
    MACIF_SCAN_CMD,
    // Sent by wifi manager to initiate a connection (param: @ref macif_cmd_connect)
    MACIF_CONNECT_CMD,
    // Sent by wifi manager to end a connection (param: @ref macif_cmd_disconnect)
    MACIF_DISCONNECT_CMD,
    // Sent by wifi manager to open/close a control port (param: @ref macif_cmd_ctrl_port)
    MACIF_CTRL_PORT_CMD,
    // Sent by wifi manager to retrieve system statistics (param: none)
    MACIF_SYS_STATS_CMD,
    // Sent by wifi manager to obtain the scan result specified by bssid (param: none)
    MACIF_GET_SCAN_RESULT_CMD,
    // Sent by wifi manager to obtain scan results (param: none)
    MACIF_GET_SCAN_RESULTS_CMD,
    // Sent by wifi manager to retrieve FW/PHY supported features (param: none)
    MACIF_LIST_FEATURES_CMD,  //10
    // Sent to change the type of an vif at MAC level. MAC VIF is deleted (if it exists)
    // and re-created with the new type (unless type is VIF_UNKNOWN) (param: @ref
    // macif_cmd_set_vif_type)
    MACIF_SET_VIF_TYPE_CMD,
    // Sent by wifi manager to configure a monitor interface (param: @ref
    // macif_cmd_monitor_cfg)
    MACIF_MONITOR_CFG_CMD,
    // Sent by wifi manager to pass external authentication status (param: @ref
    // macif_cmd_external_auth_status)
    MACIF_SET_EX_AUTH_STATUS_CMD,
    // Sent by wifi manager to start an AP (param: @ref macif_cmd_start_ap)
    MACIF_START_AP_CMD,
    // Sent by wifi manager to stop an AP (param: @ref macif_cmd_stop_ap)
    MACIF_STOP_AP_CMD,
    // Sent by wifi manager to configure EDCA parameter for one AC (param: @ref
    // macif_cmd_set_edca)
    MACIF_SET_EDCA_CMD,
    // Sent by wifi manager to update the beacon (param: @ref macif_cmd_bcn_update)
    MACIF_BCN_UPDATE_CMD,
    // Send by wifi manager to register a new Station (param: @ref macif_cmd_sta_add)
    MACIF_STA_ADD_CMD,
    // Send by wifi manager to un-register a Station (param: @ref macif_cmd_sta_remove)
    MACIF_STA_REMOVE_CMD,
    // Send by wifi manager to retrieve Key current sequence number (param: @ref
    // macif_cmd_key_seqnum)
    MACIF_KEY_SEQNUM_CMD,
    // Enable VIF Power Save (param: @ref macif_cmd_enable_vif_ps)
    MACIF_ENABLE_VIF_PS_CMD,
    // Enable Power Save (param: @ref macif_cmd_set_ps_mode)
    MACIF_SET_PS_MODE_CMD,
    // Request statistics information for a station (param: @ref macif_cmd_get_sta_info)
    MACIF_GET_STA_INFO_CMD,
    // Request to probe if a client is still present (param: @ref macif_cmd_probe_client)
    MACIF_PROBE_CLIENT_CMD,
    // Request to remain on specific channel (param: @ref macif_cmd_remain_on_channel)
    MACIF_REMAIN_ON_CHANNEL_CMD,
    // Request to cancel remain on channel (param: @ref macif_cmd_cancel_remain_on_channel)
    MACIF_CANCEL_REMAIN_ON_CHANNEL_CMD,
    // Request RC statistics for a station (param: @ref macif_cmd_rc)
    MACIF_RC_CMD,
    // Request by wifi manager to setup NOA protocol (param: @ref macif_cmd_p2p_noa_cmd)
    MACIF_P2P_NOA_CMD,
    // Request to set RC rate (param: @ref macif_cmd_rc_set_rate)
    MACIF_RC_SET_RATE_CMD,
    // Request to join a mesh network (param: @ref macif_cmd_join_mesh_cmd)
    MACIF_JOIN_MESH_CMD,
    // Request to leave a mesh network (param: @ref macif_cmd_leave_mesh_cmd)
    MACIF_LEAVE_MESH_CMD,
    // Notification that a connection has been established or lost (param: @ref
    // macif_cmd_mesh_peer_update_ntf)
    MACIF_MESH_PEER_UPDATE_NTF_CMD,
    // Request by wifi manager to setup a FTM measurement (param: @ref
    // macif_cmd_ftm_start_cmd)
    MACIF_FTM_START_CMD,
    // Request update of RX filter set in MACHW (param: @ref macif_cmd_rx_filter)
    MACIF_RX_FILTER_SET_CMD,
    MACIF_SET_CHANNEL_CMD,

    MACIF_TWT_SETUP_REQ_CMD,
    MACIF_TWT_TEARDOWN_REQ_CMD,

    MACIF_DO_PRIV_REQ_CMD,
    MACIF_ROAMING_RSSI_CMD,

    // Sent by wifi manager to indicate dhcp done (param: @ref macif_cmd_dhcp_done)
    MACIF_DHCP_DONE_CMD,

    // Prepare key for WPA AP
    MACIF_PRE_SET_KEY_CMD,

    // Response to FT authentication with updated assoc elements
    MACIF_FT_AUTH_CMD,

    MACIF_MAX_CMD,
};

// Macif Event message
enum macif_event_index
{
    // Event sent when scan is done (param: @ref cfg_scan_completed)
    MACIF_SCAN_DONE_EVENT = 0,
    // Event sent when a new AP is found (param: @ref cfg_scan_result)
    MACIF_SCAN_RESULT_EVENT,
    // Event sent when the connection is finished (param: @ref cfg_connect_event)
    MACIF_CONNECT_EVENT,
    // Event sent if the connection is lost (param: @ref cfg_disconnect_event)
    MACIF_DISCONNECT_EVENT,
    // Event sent if rssi is too low (param: @ref cfg_roaming_event)
    MACIF_ROAMING_EVENT,
    // Event sent if a Michael MIC failure is detected (param: @ref
    // cfg_mic_failure_event)
    MACIF_MIC_FAILURE_EVENT,
    // Event sent by the RX task when management frame is forwarded by the wifi task
    // (param: @ref cfg_rx_mgmt_event)
    MACIF_RX_MGMT_EVENT,
    // Event to defer TX status processing (param: @ref cfg_tx_status_event)
    MACIF_TX_STATUS_EVENT,
    // Event sent by wifi task to request external authentication (i.e. Supplicant will
    // do the authentication procedure for the wifi task, used for SAE) (param: @ref
    // cfg_external_auth_event)
    MACIF_EXTERNAL_AUTH_EVENT,
    // Event sent after receiving PROBE_CLIENT_CMD, to indicate the actual client
    // status (param: @ref cfg_probe_client_event)
    MACIF_PROBE_CLIENT_EVENT,
    // Event sent after receiving REMAIN_ON_CHANNEL_CMD, to indicate that the
    // procedure is completed (param: @ref cfg_remain_on_channel_event)
    MACIF_REMAIN_ON_CHANNEL_EVENT,
    // Event sent after receiving CANCEL_REMAIN_ON_CHANNEL_CMD, to indicate that
    // the procedure is completed (param: @ref cfg_remain_on_channel_event)
    MACIF_REMAIN_ON_CHANNEL_EXP_EVENT,
    // Notification of a new peer candidate MESH (param: @ref
    // cfg_new_peer_candidate_event)
    MACIF_NEW_PEER_CANDIDATE_EVENT,
    // Event sent after receiving FTM_START_CMD, to indicate that the
    // procedure is completed (param: @ref cfg_ftm_done_event)
    MACIF_FTM_DONE_EVENT,
    MACIF_SET_CHANNEL_EVENT,
    MACIF_TWT_SETUP_EVENT,
    MACIF_DHCP_START_EVENT,
    MACIF_MBO_UPDATE_CHAN_REQ,

    // FT IEs received from FT authentication sequence from the AP
    // is forwarded (param: @ref macif_ft_auth_event)
    MACIF_FT_AUTH_EVENT,

    MACIF_MAX_EVENT_IDX
};

// macif return status
enum macif_status
{
    // Success status
    MACIF_STATUS_SUCCESS = 0,
    // Generic error status
    MACIF_STATUS_ERROR,
    // Error invalid VIF index parameter
    MACIF_STATUS_INVALID_VIF,
    // Error invalid STA index parameter
    MACIF_STATUS_INVALID_STA,
    // Error invalid parameter
    MACIF_STATUS_INVALID_PARAM,
};

// macif cmd header with queue
struct macif_msg_hdr
{
    // For CFG commands, queue handle to use to push the response
    _queue resp_queue;
    // Length, in bytes, of the message (including this header)
    uint16_t len;
    // ID of the message.
    uint16_t id;
};

// macif generic cmd structure
struct macif_cmd
{
    // header
    struct macif_msg_hdr hdr;
};

// macif cmd generic response structure
struct macif_cmd_resp
{
    // header
    struct macif_msg_hdr hdr;
    // Status
    uint32_t status;
};

// macif event generic structure
struct macif_event
{
    // header
    struct macif_msg_hdr hdr;
    // Status
    uint32_t status;
};


// structure for MACIF_GET_HW_FEATURE_CMD
struct macif_get_hw_feature_resp
{
    // header
    struct macif_msg_hdr hdr;
    // ME configuration
    struct me_config_req *me_config;
    // Channel configuration
    struct me_chan_config_req *chan;
};

// structure for MACIF_SET_KEY_CMD
struct macif_cmd_set_key
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    int vif_idx;
    // MAC addr (may be NULL for group key)
    const struct mac_addr *addr;
    // Cipher
    int cipher_suite;
    // Key index
    int key_idx;
    // Key
    const uint8_t *key;
    // Key length
    int key_len;
    // Initial Sequence number
    const uint8_t *seq;
    // Sequence number length
    int seq_len;
    // Whether this is a pairwise key
    bool pairwise;
};

// SSID representation used by MACIF_SCAN_CMD and MACIF_CONNECT_CMD
struct macif_scan_ssid
{
    // SSID string
    const char *ssid;
    // Length of the SSID string
    size_t len;
};

// structure for MACIF_RC_CMD
struct macif_cmd_rc
{
    // header
    struct macif_msg_hdr hdr;
    // Sta idx
    int sta_idx;
};

// structure for MACIF_RC_SET_RATE_CMD
struct macif_cmd_rc_set_rate
{
    // header
    struct macif_msg_hdr hdr;
    // Sta idx
    int sta_idx;
    // Fixed rate configuration
    uint16_t fixed_rate_cfg;
};

// structure for MACIF_SCAN_CMD
struct macif_cmd_scan
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    int vif_idx;
    // SSID to scan
    struct macif_scan_ssid *ssids;
    // Number of SSID in ssids
    int ssid_cnt;
    // Extra IE to add in the proce request
    const uint8_t *extra_ies;
    // Size of the extra IEs
    int extra_ies_len;
    // Array of frequencies to scan or %NULL for all frequencies.
    // The frequency is set in MHz. The array is zero-terminated.
    int *freqs;
    // Do not use CCK mode
    bool no_cck;
    // BSSID to scan, can be NULL for wildcard BSSID
    const uint8_t *bssid;
    // Scan duration, in TUs
    int duration;
    // Socket to use to send macif events
    int sock;
    // Passive scan request
    bool passive;
};

// structure for SCAN_DONE_EVENT
struct macif_scan_completed_event
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    // Status
    uint32_t status;
    // Nb result available with SCAN_RESULTS_CMD
    uint32_t result_cnt;
};

// structure for SCAN_RESULT_EVENT
struct macif_scan_result_event
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    // Frequency
    uint16_t freq;
    // RSSI of the received frame (dBm).
    int16_t rssi;
    // Length of the frame (beacon or probe response)
    uint16_t length;
    // Frame payload. Must be freed (using rtos_free) when event has been processed
    uint8_t *payload;
};

// structure for MACIF_CONNECT_CMD
struct macif_cmd_connect
{
    // header
    struct macif_msg_hdr hdr;
    // BSSID tO connect to
    const uint8_t *bssid;
    // SSID to connect to
    struct macif_scan_ssid ssid;
    // Channel of the AP
    struct mac_chan_def chan;
    // Vif idx
    uint16_t vif_idx;
    // Authentication Type
    uint16_t auth_alg;
    // Connection flags
    uint32_t flags;
    // Control port Ethertype
    uint16_t ctrl_port_ethertype;
    // UAPSD queues (bit0: VO, bit1: VI, bit2: BK, bit3: BE). Set to 0xFFFF to use
    // default config
    uint16_t uapsd;
    // Length, in bytes, of the extra IE
    uint32_t ie_len;
    // Extra IE to add to association request
    const uint8_t *ie;
    // Socket to use to send macif events
    int sock;
};

// structure for MACIF_CONNECT_EVENT
struct macif_connect_event
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    // BSSID
    struct mac_addr bssid;
    // Sta idx
    int sta_idx;
    // Status code of the connection procedure
    uint16_t status_code;
    // Frequency of the operational channel in MHz
    uint16_t freq;
    // Length of the AssocReq IEs (in bytes)
    uint16_t assoc_req_ie_len;
    // Length of the AssocRsp IEs (in bytes)
    uint16_t assoc_resp_ie_len;
    // AssocReq IEs (assoc_req_ie_len) followed by AssocRsp IEs (assoc_resp_ie_len)
    // Must be freed (using rtos_free) when event has been processed
    uint8_t *req_resp_ies;
};

// structure for MACIF_DISCONNECT_CMD
struct macif_cmd_disconnect
{
    // header
    struct macif_msg_hdr hdr;
    // Reason of the disconnection
    uint16_t reason_code;
    // Vif idx
    uint16_t vif_idx;
};

// structure for MACIF_DISCONNECT_EVENT
struct macif_disconnect_event
{
    // header
    struct macif_msg_hdr hdr;
    // Reason of the disconnection
    uint16_t reason_code;
    // Vif idx
    uint16_t vif_idx;
};

// structure for MACIF_ROAMING_EVENT
struct macif_roaming_event
{
    // header
    struct macif_msg_hdr hdr;
    // current rssi
    int8_t rssi_current;
    // Vif idx
    uint16_t vif_idx;
};

// structure for MACIF_CTRL_PORT_CMD
struct macif_cmd_ctrl_port
{
    // header
    struct macif_msg_hdr hdr;
    // VIF index
    uint16_t vif_idx;
    // Address of the STA managed by the port (Needed only for AP interface)
    struct mac_addr addr;
    // Port status
    uint16_t authorized;
};

// structure for MACIF_MIC_FAILURE_EVENT
struct macif_mic_failure_event
{
    // header
    struct macif_msg_hdr hdr;
    // Address of the sending STA
    struct mac_addr addr;
    // Boolean indicating if the packet was a group or unicast one (true if group)
    bool ga;
    // VIF index
    uint16_t vif_idx;
};

// Structure for @ref MACIF_SYS_STATS_RESP
struct macif_sys_stats_resp
{
    // header
    struct macif_msg_hdr hdr;
    // Status
    uint32_t status;
    // Response: sys stats
    struct dbg_get_sys_stat_cfm stats;
};

// Structure for MACIF_SCAN_RESULTS_CMD
struct macif_cmd_scan_results
{
    // header
    struct macif_msg_hdr hdr;
    // VIF index
    uint16_t vif_idx;
};

// Structure for MACIF_SCAN_RESULTS_RESP
struct macif_scan_results
{
    // scan results count
    uint32_t result_cnt;
    // scan results
    struct mac_scan_result result[SCANU_MAX_RESULTS];
};

// Structure for MACIF_SCAN_RESULTS_RESP
struct macif_scan_results_resp
{
    // header
    struct macif_msg_hdr hdr;
    // scan results
    struct macif_scan_results *results;
};

// Structure for MACIF_GET_SCAN_RESULT_CMD
struct macif_cmd_scan_result
{
    // header
    struct macif_msg_hdr hdr;
    // bssid
    uint8_t bssid[MAC_ADDR_LEN];
};

// Structure for SCANU_GET_SCAN_RESULT_CFM
struct macif_scan_result_resp
{
    // header
    struct macif_msg_hdr hdr;
    // bss info
    struct mac_scan_result result;
    // Status
    uint32_t status;
};

// Structure for @ref MACIF_LIST_FEATURES_RESP
struct macif_list_features_resp
{
    // header
    struct macif_msg_hdr hdr;
    // structure containing FW/PHY features
    struct mm_version_cfm version;
};

// Structure for @ref MACIF_SET_VIF_TYPE_CMD
struct macif_cmd_set_vif_type
{
    // header
    struct macif_msg_hdr hdr;
    // Index of the FHOST vif
    int vif_idx;
    // Type to set on the interface
    enum mac_vif_type type;
    // Is a P2P vif (only read if type is VIF_STA or VIF_AP)
    bool p2p;
};

// Structure for MACIF_MONITOR_CFG_CMD
struct macif_cmd_monitor_cfg
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    // monitor channel
    struct mac_chan_op chan;
    // unsupported frame
    bool uf;
    // rx frame callback
    cb_macif_rx cb;
    // arg for callback
    void *cb_arg;
};

// Structure for MACIF_RX_MGMT_EVENT
struct macif_rx_mgmt_event
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    // Frequency
    uint16_t freq;
    // RSSI of the received frame (dBm).
    int16_t rssi;
    // Length of the frame
    uint16_t length;
    // Frame payload.
    uint8_t *payload;
};


struct macif_mbo_update_non_pre_chan_event
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    char non_pref_chan[64];
};

// Structure for MACIF_TX_STATUS_EVENT
struct macif_tx_status_event
{
    // header
    struct macif_msg_hdr hdr;
    // Frame data
    const uint8_t *data;
    // Frame length
    uint32_t data_len;
    // TX status
    bool acknowledged;
};

// Structure for MACIF_EXTERNAL_AUTH_EVENT
struct macif_external_auth_event
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    // BSSID
    struct mac_addr bssid;
    // SSID
    struct mac_ssid ssid;
    // AKM
    uint32_t akm;
};

// Structure for MACIF_SET_EX_AUTH_STATUS_CMD
struct macif_cmd_external_auth_status
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    // Authentication status
    uint16_t status;
};

// Structure for MACIF_START_AP_CMD
struct macif_cmd_start_ap
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    // Basic rates
    struct mac_rateset basic_rates;
    // Operating Channel for the AP
    struct mac_chan_op chan;
    // Buffer containing the Beacon
    uint8_t *bcn;
    // Size, in bytes, of the Beacon buffer
    int bcn_len;
    // Offset within the beacon of the TIM element (in bytes)
    int tim_oft;
    // Size, in bytes, of the TIM element
    int tim_len;
    // Beacon interval in TU
    int bcn_int;
    // AP flags (@see mac_connection_flags)
    int flags;
    // Port number for ethernet authentication frame
    uint16_t ctrl_ethertype;
    // macif event socket (socket to upload macif event)
    int sock;
};

// Structure for MACIF_STOP_AP_CMD
struct macif_cmd_stop_ap
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
};

// Structure for MACIF_SET_EDCA_CMD
struct macif_cmd_set_edca
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    // HW queue
    uint8_t hw_queue;
    // Arbitration InterFrame Space Number
    uint8_t aifsn;
    // Contention Window minimum
    uint16_t cwmin;
    // Contention Window maximum
    uint16_t cwmax;
    // TXOP (in unit of 32us)
    uint16_t txop;
};

// Structure for MACIF_BCN_UPDATE_CMD
struct macif_cmd_bcn_update
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    // Buffer containing the Beacon
    uint8_t *bcn;
    // Size, in bytes, of the Beacon buffer
    int bcn_len;
    // Offset within the beacon of the TIM element (in bytes)
    int tim_oft;
    // Size, in bytes, of the TIM element
    int tim_len;
    // Offset of CSA (channel switch announcement) counters (0 means no counter)
    uint8_t csa_oft[BCN_MAX_CSA_CPT];
};

// Structure for MACIF_STA_ADD_CMD
struct macif_cmd_sta_add
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    // Association ID of the station
    uint16_t aid;
    // MAC address of the station to be added
    const struct mac_addr *addr;
    // Supported legacy rates
    struct mac_rateset rate_set;
    // HT Capabilities
    struct mac_htcapability ht_cap;
    // VHT Capabilities
    struct mac_vhtcapability vht_cap;
    // HE capabilities
    struct mac_hecapability he_cap;
    // STA flags (@ref mac_sta_flags)
    uint32_t flags;
    // Bit field indicating which queues have U-APSD enabled
    uint8_t uapsd_queues;
    // Maximum size, in frames, of a APSD service period
    uint8_t max_sp_len;
    // Operation mode information (valid if bit @ref STA_OPMOD_NOTIF is
    // set in the flags)
    uint8_t opmode;
    // Listen interval, in number of beacon period
    int listen_interval;
};

// Structure for MACIF_STA_REMOVE_CMD
struct macif_cmd_sta_remove
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    // MAC address of the station
    const struct mac_addr *addr;
};

// structure for MACIF_KEY_SEQNUM_CMD
struct macif_cmd_key_seqnum
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    // MAC address of the station (null for group key)
    const struct mac_addr *addr;
    // Key index
    uint16_t key_idx;
};

// structure for MACIF_KEY_SEQNUM_RESP
struct macif_key_seqnum_resp
{
    // header
    struct macif_msg_hdr hdr;
    // Status
    uint32_t status;
    // Seq num
    uint64_t seqnum;
};

// structure for MACIF_SET_PS_MODE_CMD
struct macif_cmd_set_ps_mode
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    // Power save on
    uint8_t ps_on;
    // Power Save mode
    uint8_t ps_mode;
};

// structure for MACIF_ENABLE_VIF_PS_CMD
struct macif_cmd_enable_vif_ps
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
};

// structure for MACIF_GET_STA_INFO_CMD
struct macif_cmd_get_sta_info
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    // MAC address of the station
    const struct mac_addr *addr;
};

// structure for MACIF_GET_STA_INFO_RESP
struct macif_get_sta_info_resp
{
    // header
    struct macif_msg_hdr hdr;
    // Station inactive time (msec)
    uint32_t inactive_msec;
};

// structure for MACIF_PROBE_CLIENT_CMD
struct macif_cmd_probe_client
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    // MAC address of the station
    const struct mac_addr *addr;
};

// structure for MACIF_PROBE_CLIENT_EVENT
struct macif_probe_client_event
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    // MAC address of the station
    const struct mac_addr *addr;
    // Whether client is still present
    bool client_present;
};

// structure for MACIF_REMAIN_ON_CHANNEL_CMD
struct macif_cmd_remain_on_channel
{
    // header
    struct macif_msg_hdr hdr;
    // Vif Index
    uint8_t vif_idx;
    // Channel frequency in MHz
    unsigned int freq;
    // Duration in ms
    unsigned int duration;
    // macif event socket (socket to upload macif event)
    int sock;
};

// structure for MACIF_CANCEL_REMAIN_ON_CHANNEL_CMD
struct macif_cmd_cancel_remain_on_channel
{
    // header
    struct macif_msg_hdr hdr;
    // Vif Index
    uint8_t vif_idx;
};

// Structure for MACIF_REMAIN_ON_CHANNEL_EVENT
struct macif_remain_on_channel_event
{
    // header
    struct macif_msg_hdr hdr;
    // Vif Index
    uint8_t vif_idx;
    // Channel frequency in MHz
    unsigned int freq;
    // Duration in ms
    unsigned int duration;
};

// Structure for MACIF_P2P_NOA_CMD
struct macif_cmd_p2p_noa
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    // Count
    uint8_t count;
    // Duration (in us)
    uint32_t duration_us;
    // Interval (in us)
    uint32_t interval_us;
    // Indicate if NoA can be paused for traffic reason
    bool dyn_noa;
};

// structure for MACIF_JOIN_MESH_CMD
struct macif_cmd_join_mesh
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    // DTIM period
    uint8_t dtim_period;
    // Beacon interval
    uint16_t bcn_int;
    // Length of the Mesh ID
    uint8_t mesh_id_len;
    // Mesh ID
    const uint8_t *mesh_id;
    // Length of the provided IEs (in bytes)
    uint8_t ie_len;
    // IEs to download
    const uint8_t *ie;
    // Legacy rate set
    struct mac_rateset rates;
    // Indicate if Mesh Peering Management (MPM) protocol is handled in userspace
    bool user_mpm;
    // Operating Channel for the MESH point
    struct mac_chan_op chan;
    // Indicate if MESH Point is using authentication
    bool is_auth;
    // Indicate which authentication method is used
    uint8_t auth_id;
    // macif event socket (socket to upload macif event)
    int sock;
};

// structure for MACIF_LEAVE_MESH_CMD
struct macif_cmd_leave_mesh
{
    // header
    struct macif_msg_hdr hdr;
    // Vif Index
    uint8_t vif_idx;
};

// structure for MACIF_MESH_PEER_UPDATE_NTF
struct macif_cmd_mesh_peer_update_ntf
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint16_t vif_idx;
    // MAC address of the station
    const struct mac_addr *addr;
    // Mesh Link State
    uint8_t state;
};

// Structure for MACIF_NEW_PEER_CANDIDATE_EVENT
struct macif_new_peer_candidate_event
{
    // header
    struct macif_msg_hdr hdr;
    // Vif Index
    uint8_t vif_idx;
    // Peer address. Must be freed (using rtos_free) when event has been processed
    uint8_t *peer;
    // Beacon IEs. Must be freed (using rtos_free) when event has been processed
    uint8_t *ies;
    // Length of ies (in bytes)
    uint16_t ie_len;
};

// structure for MACIF_FTM_CMD
struct macif_cmd_ftm_start
{
    // header
    struct macif_msg_hdr hdr;
    // Vif Index
    uint8_t vif_idx;
    // Socket to use to send macif events
    int sock;
};

// structure for MACIF_FTM_DONE_EVENT
struct macif_ftm_done_event
{
    // header
    struct macif_msg_hdr hdr;
    // parameters
    struct ftm_done_ind param;
};

// structure for MACIF_RX_FILTER_SET_CMD
struct macif_cmd_rx_filter
{
    // header
    struct macif_msg_hdr hdr;
    // Filter to set as defined by register rxCntrlReg of MACHW
    uint32_t filter;
};

// structure for MACIF_SET_CHANNEL_CMD
struct macif_cmd_set_channel {
    // header
    struct macif_msg_hdr hdr;
    // idx
    uint8_t chan_idx;
};

struct macif_set_channel_resp {
    // header
    struct macif_msg_hdr hdr;
    // status
    int status;
    // cfm
    struct mm_set_channel_cfm cfm;
};

struct macif_twt_setup_t {
    // Setup request type
    uint8_t setup_type;
    // Flow Type (0: Announced, 1: Unannounced)
    uint8_t flow_type;
    // Wake interval Exponent
    uint8_t wake_int_exp;
    // Unit of measurement of TWT Minimum Wake Duration (0:256us, 1:tu)
    bool wake_dur_unit;
    // Nominal Minimum TWT Wake Duration
    uint8_t min_twt_wake_dur;
    // TWT Wake Interval Mantissa
    uint16_t wake_int_mantissa;
};

// structure for MACIF_TWT_SETUP_REQ_CMD
struct macif_cmd_twt_setup_req {
    // header
    struct macif_msg_hdr hdr;
    // VIF Index
    uint8_t vif_idx;
    // Setup structure
    struct macif_twt_setup_t param;
};

// structure for MACIF_TWT_TEARDOWN_REQ_CMD
struct macif_cmd_twt_teardown_req {
    // header
    struct macif_msg_hdr hdr;
    // VIF Index
    uint8_t vif_idx;
    // TWT Negotiation type
    uint8_t neg_type;
    // All TWT
    uint8_t all_twt;
    // TWT flow ID
    uint8_t id;
};

// structure for MACIF_DO_PRIV_RESP
struct macif_do_priv_resp {
    // header
    struct macif_msg_hdr hdr;
    // status
    int status;
    // cfm
    struct do_priv_cfm cfm;
};

// structure for MACIF_DO_PRIV_REQ_CMD
struct macif_cmd_do_priv_req {
    // header
    struct macif_msg_hdr hdr;
    // req_type;
    WIFI_PRIV_REQ_E req_type;
    // param1
    uint32_t param1;
    // param2
    uint32_t param2;
    // param3
    uint32_t param3;
    // param4
    uint32_t param4;
    // result
    void* result;
};

// structure for MACIF_SET_ACTIVE_REQ_CMD
struct macif_cmd_set_active_req {
    // header
    struct macif_msg_hdr hdr;
    // req_type;
    uint8_t vif_idx;
    // active or idle
    bool active;
};

// FOR MACIF_CTL Only
// Structure containing the parameters of the @ref SM_CONNECT_IND message.
struct macif_connect_ind
{
    struct macif_msg_hdr hdr;
    // Status code of the connection procedure
    uint16_t status_code;
    // BSSID
    struct mac_addr bssid;
    // Flag indicating if the indication refers to an internal roaming or from a host request
    bool roamed;
    // Index of the VIF for which the association process is complete
    uint8_t vif_idx;
    // Index of the STA entry allocated for the AP
    uint8_t ap_idx;
    // Association Id allocated by the AP for this connection
    uint16_t aid;
    // Frequency of the operational channel in MHz
    uint16_t freq;
    // Length of the AssocReq IEs
    uint16_t assoc_req_ie_len;
    // Length of the AssocRsp IEs
    uint16_t assoc_rsp_ie_len;
    // IE buffer
    uint32_t assoc_ie_buf[0];
};

// FOR MACIF_CTL Only
// Structure containing the parameters of the @ref SM_FT_AUTH_IND message.
struct macif_ft_auth_ind
{
    // header
    struct macif_msg_hdr hdr;
    // Vif idx
    uint8_t vif_idx;
    // Length of the AuthRsp IEs
    uint16_t auth_ie_len;
    // IE buffer
    uint32_t auth_ie_buf[0];
};

// structure for MACIF_DHCP_DONE_CMD
struct macif_cmd_dhcp_done
{
    // header
    struct macif_msg_hdr hdr;
    // Vif Index
    uint8_t vif_idx;
};

// structure for MACIF_ROAMING_RSSI_CMD
struct macif_cmd_roaming_rssi
{
    // header
    struct macif_msg_hdr hdr;
    // Vif Index
    uint8_t vif_idx;
    // rssi threshold
    int8_t rssi_threshold;
    // rssi hysteresis
    uint8_t rssi_hysteresis;
};

// struture for MACIF_FT_AUTH_EVENT
struct macif_ft_auth_event
{
    // header
    struct macif_msg_hdr hdr;
    uint8_t *ies;
    uint16_t ies_len;
    uint8_t ft_action;
    // Vif idx
    uint8_t vif_idx;
};

/**
 ****************************************************************************************
 * @brief Send a CFG command to the Control TASK and get the response.
 *
 * @param[in]     cmd    Pointer to the command header (followed by the parameters)
 * @param[in,out] resp   Pointer to the response header (followed by the space for the
 *                       response parameters). This parameter can be set to NULL if no
 *                       response is expected.
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
int macif_cntrl_cmd_send(struct macif_msg_hdr *cmd,
                                 struct macif_msg_hdr *resp);

/**
 ****************************************************************************************
 * @brief CLI Send a CFG command to the Control TASK and get the response.
 *
 * @param[in]     cmd    Pointer to the command header (followed by the parameters)
 * @param[in,out] resp   Pointer to the response header (followed by the space for the
 *                       response parameters). This parameter can be set to NULL if no
 *                       response is expected.
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
int macif_cntrl_cmd_send_cli(struct macif_msg_hdr *cmd,
                                 struct macif_msg_hdr *resp);
/**
 ****************************************************************************************
 * @brief Send a CFG event to the specified socket
 *
 * Event will be entirely copied to the socket so memory pointed by msg can be re-used
 * after calling this function.
 *
 * @param[in] msg_hdr  Pointer on the header of the event to send.
 * @param[in] sock     Socket to send CFG events
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
int macif_cntrl_event_send(struct macif_msg_hdr *msg_hdr, int sock);

/**
 ****************************************************************************************
 * @brief Wait until new event is available.
 *
 * This function is blocking until a new event is sent by the CNTRL task on the specified
 * link. This function is used to get the event ID and length without consuming it and
 * then @ref macif_cntrl_event_get can be called with a buffer big enough to
 * retrieve the complete event or @ref macif_cntrl_event_discard to discard it.
 *
 * @param[in]  link     Link with CNTRL task to use
 * @param[out] msg_hdr  Event header
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
int macif_cntrl_event_peek_header(struct macif_cntrl_link *link,
                                          struct macif_msg_hdr *msg_hdr);

/**
 ****************************************************************************************
 * @brief Read oldest event available
 *
 * This function doesn't wait and it is meant to be called after @ref
 * macif_cntrl_event_peek_header once the event ID and length are known.
 *
 * @param[in]  link   Link with CNTRL task to use
 * @param[out] event  Event buffer
 * @param[in]  len    Length, in bytes, of the @p event buffer
 *
 * @return Number of bytes written on success and < 0 if error occurred.
 ****************************************************************************************
 */
int macif_cntrl_event_get(struct macif_cntrl_link *link, void *event, int len);

/**
 ****************************************************************************************
 * @brief Discard oldest event available.
 *
 * This function doesn't wait and it is meant to be called after @ref
 * macif_cntrl_event_peek_header once the event ID is known.
 *
 * @param[in] link     Link with CNTRL task to use
 * @param[in] msg_hdr  Event header from @ref macif_cntrl_event_peek_header
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
int macif_cntrl_event_discard(struct macif_cntrl_link *link,
                                      struct macif_msg_hdr *msg_hdr);

/**
 ****************************************************************************************
 * @brief config and start MM and ME
 *
 * @param[in] vif_idx vif index
 * @param[in] type MAC VIF type
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
int macif_cntrl_start(int vif_idx, enum mac_vif_type type);

/**
 ****************************************************************************************
 * @brief Open a CFG link with the wifi firmware.
 *
 * A CFG link is used for communication with the 'CONTROL' task of the wifi firmware
 * which centralizes all interaction with the 'WIFI' task.
 * When no longer required a link must be close using @ref macif_cntrl_link_close.
 *
 * @return pointer to macif_cntrl_link structure on success, NULL otherwise
 ****************************************************************************************
 */
struct macif_cntrl_link *macif_cntrl_link_open();

/**
 ****************************************************************************************
 * @brief Close a CFG link.
 *
 * Close a link opened with @ref macif_cntrl_link_open.
 *
 * @param[in] link  Pointer to previously opened link
 ****************************************************************************************
 */
void macif_cntrl_link_close(struct macif_cntrl_link *link);

/**
 ****************************************************************************************
 * @brief Process @ref MACIF_FTM_START_CMD message
 *
 * @param[in] msg FTM parameters (@ref macif_ftm_start_cmd)
 ****************************************************************************************
 */
int macif_cntrl_start_ftm(int vif_idx, struct mac_ftm_results *res);
/**
 ****************************************************************************************
 * @brief Return the channel associated to a given frequency
 *
 * @param[in] freq Channel frequency
 *
 * @return Channel definition whose primary frequency is the requested one and NULL if
 * there no such channel.
 ****************************************************************************************
 */
struct mac_chan_def *macif_wifi_chan_get(int freq);

/**
 ****************************************************************************************
 * @brief Push a buffer for transmission.
 *
 * The buffer is directly pushed with the TX mutex hold.
 *
 * @param[in] net_if      Pointer to the net interface for which the packet is pushed
 * @param[in] net_buf     Pointer to the net buffer to transmit.
 * @param[in] cfm_cb      Callback when transmission has been completed (may be NULL)
 * @param[in] cfm_cb_arg  Private argument for the callback.
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
int macif_tx_start(void *net_if, net_buf_tx_t *net_buf,
                   cb_macif_tx cfm_cb, void *cfm_cb_arg);

/**
 ****************************************************************************************
 * @brief Set the callback to call when receiving management frames (i.e. they have
 * not been processed by the wifi task).
 *
 * @attention The callback is called with a @ref wifi_frame_info parameter that is only
 * valid during the callback. If needed the callback is responsible to save the frame for
 * futher processing.
 *
 * @param[in] cb   Callback function pointer
 * @param[in] arg  Callback parameter (NULL if not needed)
 ****************************************************************************************
 */
void macif_rx_set_mgmt_cb(cb_macif_rx cb, void *arg);

/**
 ****************************************************************************************
 * @brief Set the callback to call when receiving packets in monitor mode.
 *
 * @attention The callback is called with a @ref wifi_frame_info parameter that is only
 * valid during the callback. If needed the callback is responsible to save the frame for
 * futher processing.
 *
 * @param[in] cb   Callback function pointer
 * @param[in] arg  Callback parameter (NULL if not needed)
 ****************************************************************************************
 */
void macif_rx_set_monitor_cb(cb_macif_rx cb, void *arg);

/**
 ****************************************************************************************
 * @brief Print rx statistics
 *
 * @param[in] vif_idx index if wifi VIF
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
int macif_rx_stats_print(int vif_idx);

/**
 ****************************************************************************************
 * @brief Transmit a 802.11 frame
 *
 * This function is used to send a pre-formatted 802.11 frame (most likely a Management
 * frame). The frame is sent as a singleton using  basic rates.
 * The memory passed as parameter is first copied before programming the transmission.
 * It can then be re-used immediately once the function returns.
 *
 * @attention If callback function is provided it will be executed in WIFI task context
 * and as such it should be fairly quick and cannot call any function of this API.
 *
 * @param[in] vif_idx    Index of the interface
 * @param[in] frame       Pointer to the data to send
 * @param[in] length      Size, in bytes, of the provided data
 * @param[in] no_cck      Whether CCK rates must not be used to transmit this frame
 * @param[in] cfm_cb      Callback when transmission has been completed (may be NULL)
 * @param[in] cfm_cb_arg  Private argument for the callback.
 *
 * @return A unique frame id (the one passed in the confirmation callback) and 0 if
 * an error happened
 ****************************************************************************************
 */
uint32_t wifi_send_80211_frame(int vif_idx, const uint8_t *frame, uint32_t length,
                                int no_cck, cb_macif_tx cfm_cb, void *cfm_cb_arg);
/**
 ****************************************************************************************
 * @brief Enable the TX queues for the given STA.
 *
 * Send message @ref MACIF_TX_STA_ADD to MACIF TX thread to enable the TX queues
 * associated to this STA.
 *
 * @param[in] sta_id       Station index to add
 * @param[in] buf_timeout  Maximum time, in us, that a buffer can remain queued in a TXQ
 *                         (only used when adding STA to an AP interface)
 ****************************************************************************************
 */
void macif_tx_sta_add(uint8_t sta_id, uint32_t buf_timeout);

/**
 ****************************************************************************************
 * @brief Disable the TX queues for the given STA.
 *
 * Send message @ref MACIF_TX_STA_DEL to MACIF TX thread to disable the TX queues
 * associated to this STA. Any pending packets in the queues will be freed.
 *
 * @param[in] sta_id Station index to delete
 ****************************************************************************************
 */
void macif_tx_sta_del(uint8_t sta_id);

/**
 ****************************************************************************************
 * @brief Send a CFG command to the Control TASK and get the response.
 *
 * @param[in]     cmd    Pointer to the command header (followed by the parameters)
 * @param[in,out] resp   Pointer to the response header (followed by the space for the
 *                       response parameters). This parameter can be set to NULL if no
 *                       response is expected.
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
int macif_ctl_cmd_execute(struct macif_msg_hdr *cmd, struct macif_msg_hdr *resp);

/**
 ****************************************************************************************
 * @brief config and start MM and ME
 *
 * @param[in] vif_idx vif index
 * @param[in] type MAC VIF type
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
int macif_ctl_start(int vif_idx, enum mac_vif_type type);

/**
 ****************************************************************************************
 * @brief Retrieve list of Stations connected to a certain VIF
 *
 * @param[in] wifi_vif         Virtual Interface Information
 * @param[in] table_max_size    Maximum size of the idx_table
 * @param[out] idx_table        Table to fill with the Station indexes
 *
 * @return Size of the idx_table
 ****************************************************************************************
 */
extern int macif_get_sta_idx_table(int vif_idx, uint8_t table_max_size,
                      uint8_t *idx_table);

/**
 ****************************************************************************************
 * @brief Format rate config
 *
 * @param[in] rate       WiFi rate
 * @param[in] tx_sgi    Short GI
 * @param[in] he_ltf     HE LTF
 *
 * @return Formatted value
 ****************************************************************************************
 */
extern uint32_t macif_format_rate_config(uint32_t rate, uint32_t tx_sgi, uint32_t he_ltf);

/**
 ****************************************************************************************
 * @brief get rate index
 *
 * @param[in] rate WiFi rate config
 *
 * @return rate index
 ****************************************************************************************
 */
extern int32_t macif_get_rate_idx(uint32_t rate);

/**
 ****************************************************************************************
 * @brief Get debug filter for WiFi
 *
 * @param[out] dbg_level        Debug level
 * @param[out] dbg_module    Debug module
 *
* @return none
 ****************************************************************************************
 */
extern void macif_dbg_filter_get(uint32_t *dbg_level, uint32_t *dbg_module);

/**
 ****************************************************************************************
 * @brief Set debug filter for WiFi
 *
 * @param[in] dbg_level        Debug level to set
 * @param[in] dbg_module    Debug module to set
 *
* @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
extern int macif_dbg_filter_set(uint32_t dbg_level, uint32_t dbg_module);

/**
 ****************************************************************************************
 * @brief get HW supported cipher
 *
 * To be called before wifi connect.
 *
 ****************************************************************************************
 */
extern uint32_t macif_setting_supp_cipher_get(void);

/**
 ****************************************************************************************
 * @brief get rate config value according to the index
 *
 * @param[in] idx rate index
 * @param[in] is_rx is rx rate
 * @param[out] rate_cfg rate config
 * @param[out] ru_size resource unit size
 ****************************************************************************************
 */
extern int macif_setting_rate_cfg_get(int idx, bool is_rx, uint32_t *rate_cfg, int *ru_size);

/**
 ****************************************************************************************
 * @brief Allocates an entry in the RX stats table
 *
 * @param[in] sta_idx   Station index and index of the RX stats table to allocate
 ****************************************************************************************
 */
extern void macif_alloc_rx_rates(int sta_idx);

/**
 ****************************************************************************************
 * @brief Free entry in the RX stats table
 *
 * @param[in] sta_idx   Station index and index of the RX stats table to free
 ****************************************************************************************
 */
extern void macif_free_rx_rates(int sta_idx);

/**
 ****************************************************************************************
 * @brief Get the final base mac address
 *
 * MAC address may be got from mac_addr_user_specified or macif_setting or efuse.
 *
 * @param[out] base_mac_addr the final base MAC address
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
extern int macif_ctl_base_addr_get(struct mac_addr *base_mac_addr);
extern int macif_cntrl_base_addr_get(struct mac_addr *base_mac_addr);

/**
 ****************************************************************************************
 * @brief Set user specified MAC address
 *
 * @param[in] user_mac_addr    MAC address to set
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
extern int macif_user_mac_addr_set(uint8_t *user_mac_addr);

#endif // _MACIF_API_H_
