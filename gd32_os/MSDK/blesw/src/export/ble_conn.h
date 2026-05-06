/*!
    \file    ble_conn.h
    \brief   Module for handling the BLE connection.

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

#ifndef _BLE_CONN_H__
#define _BLE_CONN_H__

#include <stdint.h>
#include <stdbool.h>

#include "ble_gap.h"
#include "ble_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Resolvable private address mask */
#define BLE_RESOLVE_ADDR_MASK   0xC0

/* MSB of resolvable private address */
#define BLE_RESOLVE_ADDR_MSB    0x40

/*!
    \brief      Check if an address is resolvable private address
    \param[in]  x: address to check
    \param[out] none
    \retval     bool: true if the address is resolvable private address, otherwise false
*/
static inline bool BLE_IS_RESOLVE_BDA(uint8_t x[6])
{
    return (x[5] & BLE_RESOLVE_ADDR_MASK) == BLE_RESOLVE_ADDR_MSB;
}

/* BLE connection role */
typedef enum ble_role
{
    BLE_MASTER,     /*!< Master role */
    BLE_SLAVE,      /*!< Slave role */
} ble_role_t;

/* Connection events. These events are propagated if a handler is provided during the operation of the Connection Module */
typedef enum
{
    BLE_CONN_EVT_CONN_RSP,                    /*!< Event notify for create connection response */
    BLE_CONN_EVT_DISCONN_RSP,                 /*!< Event notify for disconnect connection response */
    BLE_CONN_EVT_CONN_CANCEL_RSP,             /*!< Event notify for create connection cancel response */
    BLE_CONN_EVT_SEC_INFO_SET_RSP,            /*!< Event notify for security information set response */
    BLE_CONN_EVT_INIT_STATE_CHG,              /*!< Event notify for connection init state changed */
    BLE_CONN_EVT_STATE_CHG,                   /*!< Event notify for connection state changed */
    BLE_CONN_EVT_PEER_NAME_GET_RSP,           /*!< Event notify for get remote name response */
    BLE_CONN_EVT_PEER_VERSION_GET_RSP,        /*!< Event notify for get remote version response */
    BLE_CONN_EVT_PEER_FEATS_GET_RSP,          /*!< Event notify for get remote features response */
    BLE_CONN_EVT_PEER_APPEARANCE_GET_RSP,     /*!< Event notify for get remote appearance response */
    BLE_CONN_EVT_PEER_SLV_PRF_PARAM_GET_RSP,  /*!< Event notify for get peripheral slave perfer parameters response */
    BLE_CONN_EVT_PEER_ADDR_RESLV_GET_RSP,     /*!< Event notify for get remote address resolution feature response */
    BLE_CONN_EVT_PEER_RPA_ONLY_GET_RSP,       /*!< Event notify for get remote rpa only feature response */
    BLE_CONN_EVT_PEER_DB_HASH_GET_RSP,        /*!< Event notify for get remote database hash feature response */
    BLE_CONN_EVT_PING_TO_VAL_GET_RSP,         /*!< Event notify for get ping timeout value operation response */
    BLE_CONN_EVT_PING_TO_INFO,                /*!< Event notify for ping timeout value */
    BLE_CONN_EVT_PING_TO_SET_RSP,             /*!< Event notify for set ping timeout value response */
    BLE_CONN_EVT_RSSI_GET_RSP,                /*!< Event notify for get rssi response */
    BLE_CONN_EVT_CHANN_MAP_GET_RSP,           /*!< Event notify for get channel map response */
    BLE_CONN_EVT_NAME_GET_IND,                /*!< Event notify for local name getting operation */
    BLE_CONN_EVT_APPEARANCE_GET_IND,          /*!< Event notify for local apperance getting operation */
    BLE_CONN_EVT_SLAVE_PREFER_PARAM_GET_IND,  /*!< Event notify for local perfer parameters getting operation */
    BLE_CONN_EVT_NAME_SET_IND,                /*!< Event notify for local name setting operation */
    BLE_CONN_EVT_APPEARANCE_SET_IND,          /*!< Event notify for local appearance setting operation */
    BLE_CONN_EVT_PARAM_UPDATE_IND,            /*!< Event notify for connection parameter updating operation */
    BLE_CONN_EVT_PARAM_UPDATE_RSP,            /*!< Event notify for update connection parameters */
    BLE_CONN_EVT_PARAM_UPDATE_INFO,           /*!< Event notify for connection parameter update information */
    BLE_CONN_EVT_PKT_SIZE_SET_RSP,            /*!< Event notify for set packet size response */
    BLE_CONN_EVT_PKT_SIZE_INFO,               /*!< Event notify for packet size setting information */
    BLE_CONN_EVT_PHY_GET_RSP,                 /*!< Event notify for get phy response */
    BLE_CONN_EVT_PHY_SET_RSP,                 /*!< Event notify for set phy response */
    BLE_CONN_EVT_PHY_INFO,                    /*!< Event notify for phy information */
    BLE_CONN_EVT_LOC_TX_PWR_GET_RSP,          /*!< Event notify for get local tx power value response */
    BLE_CONN_EVT_PEER_TX_PWR_GET_RSP,         /*!< Event notify for get peer tx power value response */
    BLE_CONN_EVT_TX_PWR_RPT_CTRL_RSP,         /*!< Event notify for control tx power report response */
    BLE_CONN_EVT_LOC_TX_PWR_RPT_INFO,         /*!< Event notify for local tx power report information */
    BLE_CONN_EVT_PEER_TX_PWR_RPT_INFO,        /*!< Event notify for peer tx power report information */
    BLE_CONN_EVT_PATH_LOSS_CTRL_RSP,          /*!< Event notify for control path loss response */
    BLE_CONN_EVT_PATH_LOSS_THRESHOLD_INFO,    /*!< Event notify for path loss threshold report information */
    BLE_CONN_EVT_PER_SYNC_TRANS_RSP,          /*!< Event notify for start periodic advertising sync transfer response */

    BLE_CONN_EVT_MAX
} ble_conn_evt_t;

/* Connection init state. These states are propagated with connection event @ref BLE_CONN_EVT_INIT_STATE_CHG */
typedef enum
{
    BLE_INIT_STATE_IDLE,              /*!< Init state for idle */
    BLE_INIT_STATE_STARTING,          /*!< Init state for starting */
    BLE_INIT_STATE_STARTED,           /*!< Init state for started */
    BLE_INIT_STATE_DISABLING,         /*!< Init state for disabling */
} ble_init_state_t;

/* Connection state. These states are propagated with connection event @ref BLE_CONN_EVT_STATE_CHG */
typedef enum
{
    BLE_CONN_STATE_DISCONNECTD,         /*!< Connection state for disconnected */
    BLE_CONN_STATE_CONNECTED,           /*!< Connection state for connected */
    BLE_CONN_STATE_DISCONNECTING,       /*!< Connection state for disconnecting */
} ble_conn_state_t;

/* Create connection response structure for @ref BLE_CONN_EVT_CONN_RSP */
typedef struct
{
    uint16_t        status;         /*!< Create connection response status, @ref ble_status_t */
} ble_conn_conn_rsp_t;

/* Disconnect connection response structure for @ref BLE_CONN_EVT_DISCONN_RSP */
typedef struct
{
    uint8_t         conn_idx;       /*!< Connection index */
    uint16_t        status;         /*!< Disconnect connection response status, @ref ble_status_t */
} ble_conn_disconn_rsp_t;

/* Create connection cancel response structure for @ref BLE_CONN_EVT_CONN_CANCEL_RSP */
typedef struct
{
    uint16_t        status;         /*!< Create connection cancel response status, @ref ble_status_t */
} ble_conn_conn_cancel_rsp_t;

/* Set security information response structure for @ref BLE_CONN_EVT_SEC_INFO_SET_RSP */
typedef struct
{
    uint16_t        status;         /*!< Set security information response status, @ref ble_status_t */
} ble_conn_sec_info_set_rsp_t;

/* Initial state structure for @ref BLE_CONN_EVT_INIT_STATE_CHG */
typedef struct
{
    uint8_t          init_idx;   /*!< Init index, has no meanings for @ref BLE_INIT_STATE_IDLE/@ref BLE_INIT_STATE_STARTING.*/
    bool             wl_used;    /*!< Filter accept list used.*/
    ble_init_state_t state;      /*!< Data structure for @ref BLE_CONN_EVT_INIT_STATE_CHG */
    uint16_t         reason;
} ble_init_state_chg_t;

/* Connection state structure for @ref BLE_CONN_EVT_STATE_CHG */
typedef struct
{
    ble_conn_state_t state;      /*!< Data structure for @ref BLE_CONN_EVT_STATE_CHG */
    union conn_info {
        ble_gap_conn_info_t     conn_info;
        ble_gap_disconn_info_t  discon_info;
    } info;
} ble_conn_state_chg_t;

/* Connection union data for connection events */
typedef union ble_conn_data
{
    ble_conn_conn_rsp_t                     conn_rsp;
    ble_conn_disconn_rsp_t                  disconn_rsp;
    ble_conn_conn_cancel_rsp_t              conn_cancel_rsp;
    ble_conn_sec_info_set_rsp_t             sec_info_set_rsp;
    ble_init_state_chg_t                    init_state;
    ble_conn_state_chg_t                    conn_state;
    ble_gap_peer_name_get_rsp_t             peer_name;
    ble_gap_peer_ver_get_rsp_t              peer_version;
    ble_gap_peer_feats_get_rsp_t            peer_features;
    ble_gap_peer_appearance_get_rsp_t       peer_appearance;
    ble_gap_slave_prefer_param_get_rsp_t    peer_slv_prf_param;
    ble_gap_peer_addr_resol_get_rsp_t       peer_addr_reslv_sup;
    ble_gap_peer_rpa_only_get_rsp_t         rpa_only;
    ble_gap_peer_db_hash_get_rsp_t          db_hash;
    ble_gap_ping_tout_get_rsp_t             ping_to_val;
    ble_gap_ping_tout_info_t                ping_timeout;
    ble_gap_ping_tout_set_rsp_t             ping_to_set;
    ble_gap_rssi_get_rsp_t                  rssi_ind;
    ble_gap_chann_map_get_rsp_t             chnl_map_ind;
    ble_gap_name_get_ind_t                  name_get_ind;
    ble_gap_appearance_get_ind_t            appearance_get_ind;
    ble_gap_slave_prefer_param_get_ind_t    slave_prefer_param_get_ind;
    ble_gap_name_set_ind_t                  name_set_ind;
    ble_gap_appearance_set_ind_t            appearance_set_ind;
    ble_gap_conn_param_update_ind_t         conn_param_req_ind;
    ble_gap_conn_param_update_rsp_t         conn_param_rsp;
    ble_gap_conn_param_info_t               conn_params;
    ble_gap_pkt_size_set_rsp_t              pkt_size_set_rsp;
    ble_gap_pkt_size_info_t                 pkt_size_info;
    ble_gap_phy_get_rsp_t                   phy_get;
    ble_gap_phy_set_rsp_t                   phy_set;
    ble_gap_phy_info_t                      phy_val;
    ble_gap_local_tx_pwr_get_rsp_t          loc_tx_pwr;
    ble_gap_peer_tx_pwr_get_rsp_t           peer_tx_pwr;
    ble_gap_tx_pwr_report_ctrl_rsp_t        tx_pwr_rpt_ctrl_rsp;
    ble_gap_tx_pwr_report_info_t            loc_tx_pwr_rpt;
    ble_gap_tx_pwr_report_info_t            peer_tx_pwr_rpt;
    ble_gap_path_loss_ctrl_rsp_t            path_ctrl;
    ble_gap_path_loss_threshold_info_t      path_loss_thr;
    ble_gap_per_adv_sync_trans_rsp_t        sync_trans_rsp;
} ble_conn_data_u;

/* Prototype of BLE connection event handler */
typedef void (*ble_conn_evt_handler_t)(ble_conn_evt_t event, ble_conn_data_u *p_data);

/*!
    \brief      Register callback function to connection module
    \param[in]  callback: callback function to handle BLE connection events
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_callback_register(ble_conn_evt_handler_t callback);

/*!
    \brief      Unregister callback function to connection module
    \param[in]  callback: callback function that registered before
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_callback_unregister(ble_conn_evt_handler_t callback);

/*!
    \brief      Create connection with remote device
    \param[in]  p_param: pointer to init parameters, default parametes will be used if it is NULL
    \param[in]  own_addr_type: local address type
    \param[in]  p_peer_addr_info: peer address info, only valid if use_wl is false
    \param[in]  use_wl: true to use filter accept list
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_connect(ble_gap_init_param_t *p_param, ble_gap_local_addr_type_t own_addr_type,
                             ble_gap_addr_t *p_peer_addr_info, bool use_wl);

/*!
    \brief      Disconnect connection with remote device
    \param[in]  conidx: connection index
    \param[in]  reason: disconnect reason
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_disconnect(uint8_t conidx, uint16_t reason);

/*!
    \brief      Cancel connect procedure
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_connect_cancel(void);

/*!
    \brief      Set security information when connection established
    \details    This function should only be used when security keys are managed by APP and
                must be called when state changed to @ref BLE_CONN_STATE_CONNECTED
    \param[in]  conidx: connection index
    \param[in]  p_local_csrk: pointer to local CSRK
    \param[in]  p_peer_csrk: pointer to peer CSRK, value can be get in @ref BLE_SEC_EVT_PAIRING_SUCCESS_INFO or NULL if not bonded
    \param[in]  pairing_lvl: pairing level, value can be get in @ref BLE_SEC_EVT_PAIRING_SUCCESS_INFO or 0 if not bonded
    \param[in]  enc_key_present: encryption key present, value can be get in @ref BLE_SEC_EVT_PAIRING_SUCCESS_INFO or 0 if not bonded
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_sec_info_set(uint8_t conidx, uint8_t *p_local_csrk, uint8_t *p_peer_csrk,
                        uint8_t pairing_lvl, uint8_t enc_key_present);

/*!
    \brief      Get remote device's name
    \param[in]  conidx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_peer_name_get(uint8_t conidx);

/*!
    \brief      Get remote device's supported features
    \param[in]  conidx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_peer_feats_get(uint8_t conidx);

/*!
    \brief      Get remote device's appearance
    \param[in]  conidx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_peer_appearance_get(uint8_t conidx);

/*!
    \brief      Get remote device's version
    \param[in]  conidx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_peer_version_get(uint8_t conidx);

/*!
    \brief      Get remote device's peripheral perfer parameters
    \param[in]  conidx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_peer_slave_prefer_param_get(uint8_t conidx);

/*!
    \brief      Get remote device's address resolution support feature
    \param[in]  conidx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_peer_addr_resolution_support_get(uint8_t conidx);

/*!
    \brief      Get remote device's rpa only feature
    \param[in]  conidx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_peer_rpa_only_get(uint8_t conidx);

/*!
    \brief      Get remote device's database hash value
    \param[in]  conidx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_peer_db_hash_get(uint8_t conidx);

/*!
    \brief      Get current PHY used for the connection
    \param[in]  conidx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_phy_get(uint8_t conidx);

/*!
    \brief      Set PHY used for the connection
    \param[in]  conidx: connection index
    \param[in]  tx_phy: supported PHY for data transmission, @ref ble_gap_phy_bf
    \param[in]  rx_phy: supported PHY for data reception, @ref ble_gap_phy_bf
    \param[in]  phy_opt: phy options for coded phy, @ref ble_gap_phy_option
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_phy_set(uint8_t conidx, uint8_t tx_phy, uint8_t rx_phy, uint8_t phy_opt);

/*!
    \brief      Set maximum TX octets and TX time for the connection
    \param[in]  conidx: connection index
    \param[in]  tx_octets: preferred maximum number of payload octets that should include in a single Data Channel PDU
    \param[in]  tx_time: preferred maximum number of microseconds that should use to transmit a single Data Channel PDU
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_pkt_size_set(uint8_t conidx, uint16_t tx_octets, uint16_t tx_time);

/*!
    \brief      Get channel map for the connection
    \param[in]  conidx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_chann_map_get(uint8_t conidx);

/*!
    \brief      Get ping timeout value for the connection
    \param[in]  conidx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_ping_to_get(uint8_t conidx);

/*!
    \brief      Set ping timeout value for the connection
    \param[in]  conidx: connection index
    \param[in]  tout: authenticated payload timeout value
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_ping_to_set(uint8_t conidx, uint16_t tout);

/*!
    \brief      Get RSSI value of the last packet received
    \param[in]  conidx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_rssi_get(uint8_t conidx);

/*!
    \brief      Confirm getting name operation from peer device
    \param[in]  conidx: connection index
    \param[in]  status: status of the confirmation
    \param[in]  token: token value provided in the indication, @ref ble_gap_name_get_ind_t
    \param[in]  cmpl_len: complete name length
    \param[in]  p_name: pointer to name value starting from the offset in the indication
    \param[in]  name_len: length of provided name value
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_name_get_cfm(uint8_t conidx, uint16_t status, uint16_t token,
                uint16_t cmpl_len, uint8_t *p_name, uint16_t name_len);

/*!
    \brief      Confirm getting appearance operation from peer device
    \param[in]  conidx: connection index
    \param[in]  status: status of the confirmation
    \param[in]  token: token value provided in the indication, @ref ble_gap_appearance_get_ind_t
    \param[in]  appearance: device appearance
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_appearance_get_cfm(uint8_t conidx, uint16_t status, uint16_t token, uint16_t appearance);

/*!
    \brief      Confirm getting slave perfer parameters operation from peer device
    \param[in]  conidx: connection index
    \param[in]  status: status of the confirmation
    \param[in]  token: token value provided in the indication, @ref ble_gap_slave_prefer_param_get_ind_t
    \param[in]  p_param: pointer to peripheral preferred connection parameters
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_slave_prefer_param_get_cfm(uint8_t conidx, uint16_t status, uint16_t token,
                        ble_gap_slave_prefer_param_t *p_param);

/*!
    \brief      Confirm setting name operation from peer device
    \param[in]  conidx: connection index
    \param[in]  status: status of the confirmation
    \param[in]  token: token value provided in the indication, @ref ble_gap_name_set_ind_t
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_name_set_cfm(uint8_t conidx, uint16_t status, uint16_t token);

/*!
    \brief      Confirm setting appearance operation from peer device
    \param[in]  conidx: connection index
    \param[in]  status: status of the confirmation
    \param[in]  token: token value provided in the indication, @ref ble_gap_appearance_set_ind_t
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_appearance_set_cfm(uint8_t conidx, uint16_t status, uint16_t token);

/*!
    \brief      Confirm connection parameter update operation from peer device
    \param[in]  conidx: connection index
    \param[in]  accept: true to accept the new connection parameters, otherwise false
    \param[in]  ce_len_min: minimum connection event duration
    \param[in]  ce_len_max: maximum connection event duration
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_param_update_cfm(uint8_t conidx, bool accept, uint16_t ce_len_min, uint16_t ce_len_max);

/*!
    \brief      Start connection parameter update operation
    \param[in]  conidx: connection index
    \param[in]  int_min: minimum connection interval
    \param[in]  int_max: maximum connection interval
    \param[in]  latency: connection latency
    \param[in]  supv_to: supervison timeout
    \param[in]  ce_len_min: minimum connection event duration
    \param[in]  ce_len_max: maximum connection event duration
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_param_update_req(uint8_t conidx, uint16_t int_min, uint16_t int_max, uint16_t latency,
                                   uint16_t supv_to, uint16_t ce_len_min, uint16_t ce_len_max);

/*!
    \brief      Get local transmit power of the given PHY
    \param[in]  conidx: connection index
    \param[in]  phy: connection PHY
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_local_tx_pwr_get(uint8_t conidx, ble_gap_phy_pwr_value_t phy);

/*!
    \brief      Get peer transmit power of the given PHY
    \param[in]  conidx: connection index
    \param[in]  phy: connection PHY
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_peer_tx_pwr_get(uint8_t conidx, ble_gap_phy_pwr_value_t phy);

/*!
    \brief      Control power change report of local and remote transmit power
    \param[in]  conidx: connection index
    \param[in]  local_en: true to enable local power change report, false to disable
    \param[in]  remote_en: true to enable remote power change report, false to disable
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_tx_pwr_report_ctrl(uint8_t conidx, bool local_en, bool remote_en);

/*!
    \brief      Control connection path loss report
    \param[in]  conidx: connection index
    \param[in]  enable: 1 to enable path loss report, 0 to disable
    \param[in]  high_threshold: high threshold value in dB
    \param[in]  high_hysteresis: high hysteresis value in dB
    \param[in]  low_threshold: low threshold value in dB
    \param[in]  low_hysteresis: low hysteresis value in dB
    \param[in]  min_time: minimum time spent in connection events
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_path_loss_ctrl(uint8_t conidx, uint8_t enable, uint8_t high_threshold,
                uint8_t high_hysteresis, uint8_t low_threshold, uint8_t low_hysteresis, uint16_t min_time);

/*!
    \brief      Start periodic adverting sync transfer procedure
    \param[in]  conidx: connection index
    \param[in]  trans_idx: periodic advertising or periodic sync index
    \param[in]  srv_data: service data provided by application
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_per_adv_sync_trans(uint8_t conidx, uint8_t trans_idx, uint16_t srv_data);

/*!
    \brief      Enable central feature
    \param[in]  conidx: connection index
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_conn_enable_central_feat(uint8_t conidx);

#ifdef __cplusplus
}
#endif

#endif // _BLE_CONN_H__
