/*!
    \file    ble_l2cap_coc.h
    \brief   Module for handling the BLE l2cap.

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

#ifndef _BLE_L2CAP_COC_H__
#define _BLE_L2CAP_COC_H__

#include <stdint.h>
#include <stdbool.h>

#include "ble_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Invalid L2CAP channel local index */
#define BLE_L2CAP_INVALID_CHANN_LID         0xFF

/* Parameter to set in nb_chan of @ref ble_l2cap_coc_conn_cfm to reject connection due to not enough authorization */
#define BLE_L2CAP_COC_NOT_AUTORIZED         0xFF

/* Size of L2CAP Length field */
#define BLE_L2CAP_LENGTH_LEN       (2)
/* Size of L2CAP CID field */
#define BLE_L2CAP_CID_LEN          (2)
/* Size of L2CAP header */
#define BLE_L2CAP_HEADER_LEN       (BLE_L2CAP_LENGTH_LEN + BLE_L2CAP_CID_LEN)

/* SPSM value */
enum ble_l2cap_spsm
{
    BLE_L2CAP_SPSM_IPSP = 0x0023,           /*!< Internet Protocol Support Profile */
    BLE_L2CAP_SPSM_OTS  = 0x0025,           /*!< Object Transfer Service */
    BLE_L2CAP_SPSM_ATT  = 0x0027,           /*!< Attribute */
};

/* L2CAP security level bit field */
enum ble_l2cap_sec_lvl_bf
{
    BLE_L2CAP_COC_EKS_BIT       = 0x01,     /*!< With encrypted security requirement also requires a 128-bit encryption key size */
    BLE_L2CAP_COC_EKS_POS       = 0,

    BLE_L2CAP_COC_SEC_LVL_MASK  = 0x06,     /*!< Channel minimum required security level, @ref ble_gap_sec_lvl */
    BLE_L2CAP_COC_SEC_LVL_LSB   = 1,
};

/* BLE L2CAP event type */
typedef enum
{
    BLE_L2CAP_COC_EVT_REG_RSP,              /*!< Receive add spsm register response. @ref ble_l2cap_spsm_reg_rsp_t */
    BLE_L2CAP_COC_EVT_UNREG_RSP,            /*!< Receive remove spsm unregister response. @ref ble_l2cap_spsm_reg_rsp_t */
    BLE_L2CAP_COC_EVT_CONN_IND,             /*!< Indication of receive a L2CAP credit oriented connection is initiated by peer device. @ref ble_l2cap_coc_conn_ind_t */
    BLE_L2CAP_COC_EVT_CONN_FAIL,            /*!< Receive a L2CAP credit oriented connect response fail. @ref ble_l2cap_coc_conn_fail_t */
    BLE_L2CAP_COC_EVT_CONN_INFO,            /*!< Indication of a L2CAP credit oriented connected. @ref ble_l2cap_coc_conn_info_t */
    BLE_L2CAP_COC_EVT_RECFG_RSP,            /*!< Receive reconfig response. @ref ble_l2cap_coc_recfg_rsp_t */
    BLE_L2CAP_COC_EVT_DISCONN_INFO,         /*!< Indication of a L2CAP connection oriented channel disconnected. @ref ble_l2cap_coc_disconn_info_t */
    BLE_L2CAP_COC_EVT_TX_RSP,               /*!< Receive send reseponse. @ref ble_l2cap_coc_sdu_tx_rsp_t */
    BLE_L2CAP_COC_EVT_RX_IND,               /*!< Indication of receive peer data. @ref ble_l2cap_coc_sdu_rx_ind_t */
} ble_l2cap_coc_evt_t;

/* L2CAP parameters */
typedef struct
{
    uint8_t     nb_chan;                    /*!< Number of L2CAP channel created */
    uint16_t    local_rx_mtu;               /*!< Local device reception Maximum Transmit Unit size */
} ble_l2cap_coc_param_t;

/* L2CAP SPSM register response structure */
typedef struct
{
    uint16_t    status;                     /*!< Status of the operation @ref ble_err_t */
    uint16_t    spsm;                       /*!< Simplified Protocol/Service Multiplexer */
} ble_l2cap_spsm_reg_rsp_t;

/* L2CAP connection confirm structure */
typedef struct
{
    uint8_t     chann_num;                  /*!< Number of L2CAP channel requested to be created in parallel */
    uint16_t    token;                      /*!< Token provided by L2CAP module */
    uint16_t    local_rx_mtu;               /*!< Local device Maximum Transmit Unit reception size */
} ble_l2cap_coc_conn_cfm_t;

/* L2CAP connection indication structure */
typedef struct
{
    uint8_t     conn_idx;                   /*!< Connection Index */
    uint8_t     chann_num;                  /*!< Number of L2CAP channel requested to be created in parallel */
    uint16_t    token;                      /*!< Token provided by L2CAP module */
    uint16_t    spsm;                       /*!< Simplified Protocol/Service Multiplexer */
    uint16_t    peer_rx_mtu;                /*!< Peer device Maximum Transmit Unit reception size */
} ble_l2cap_coc_conn_ind_t;

/* L2CAP connect fail information structure */
typedef struct
{
    uint16_t    status;                     /*!< Status of the operation @def enum ble_err_t */
    uint8_t     conidx;                     /*!< Connection Index */
    uint8_t     channel_num;                /*!< Already created channel, created channel will be reported in @ref BLE_L2CAP_COC_EVT_CONN_INFO */
    uint16_t    spsm;                       /*!< Simplified Protocol/Service Multiplexer */
} ble_l2cap_coc_conn_fail_t;

/* L2CAP connection information structure */
typedef struct
{
    uint16_t    spsm;                       /*!< Simplified Protocol/Service Multiplexer */
    uint8_t     conn_idx;                   /*!< Connection Index */
    uint8_t     chann_lid;                  /*!< Created L2CAP channel local index */
    uint16_t    local_rx_mtu;               /*!< Local device reception Maximum Transmit Unit size */
    uint16_t    peer_rx_mtu;                /*!< Peer device reception Maximum Transmit Unit size */
} ble_l2cap_coc_conn_info_t;

/* L2CAP reconfigure response structure */
typedef struct
{
    uint16_t    status;                     /*!< Status of the operation @ref ble_err_t */
    uint8_t     conn_idx;                   /*!< Connection Index */
} ble_l2cap_coc_recfg_rsp_t;

/* L2CAP disconnection information structure */
typedef struct
{
    uint16_t    spsm;                       /*!< Simplified Protocol/Service Multiplexer */
    uint8_t     conn_idx;                   /*!< Connection Index */
    uint8_t     chann_lid;                  /*!< L2CAP channel local index */
    uint16_t    reason;                     /*!< Termination Reason @ref ble_err_t */
} ble_l2cap_coc_disconn_info_t;

/* L2CAP sdu tx response structure */
typedef struct
{
    uint16_t    spsm;                       /*!< Simplified Protocol/Service Multiplexer */
    uint16_t    status;                     /*!< Status of the operation @ref ble_err_t */
    uint8_t     conn_idx;                   /*!< Connection Index */
    uint8_t     chann_lid;                  /*!< L2CAP channel local index */
} ble_l2cap_coc_sdu_tx_rsp_t;

/* L2CAP sdu rx indication structure */
typedef struct
{
    uint16_t    spsm;                       /*!< Simplified Protocol/Service Multiplexer */
    uint8_t     conn_idx;                   /*!< Connection Index */
    uint8_t     chann_lid;                  /*!< L2CAP channel local index */
    uint16_t    status;                     /*!< Status of the operation @ref ble_err_t */
    uint16_t    token;                      /*!< Token req_ind_code by L2CAP module */
    uint16_t    len;                        /*!< SDU Length */
    uint8_t    *p_data;                     /*!< SDU Data */
} ble_l2cap_coc_sdu_rx_ind_t;

/* L2CAP event data */
typedef union ble_l2cap_coc_data {
    ble_l2cap_spsm_reg_rsp_t      reg_rsp;              /*!< Receive add spsm register response */
    ble_l2cap_coc_conn_ind_t      conn_ind;             /*!< Indication of receive a L2CAP credit oriented connection is initiated by peer device */
    ble_l2cap_coc_conn_fail_t     conn_fail;            /*!< Receive a L2CAP credit oriented connect response fail */
    ble_l2cap_coc_conn_info_t     conn_info;            /*!< Indication of a L2CAP credit oriented connected */
    ble_l2cap_coc_recfg_rsp_t     recfg_rsp;            /*!< Receive reconfig response */
    ble_l2cap_coc_disconn_info_t  disconn_info;         /*!< Indication of a L2CAP connection oriented channel disconnected */
    ble_l2cap_coc_sdu_tx_rsp_t    tx_rsp;               /*!< Receive send reseponse */
    ble_l2cap_coc_sdu_rx_ind_t    rx_ind;               /*!< Indication of receive peer data */
} ble_l2cap_coc_data_u;

/* Prototype of L2CAP event handler */
typedef void (*ble_l2cap_coc_evt_handler_t)(ble_l2cap_coc_evt_t event, ble_l2cap_coc_data_u *p_data);

/*!
    \brief      Register callback function to handle L2CAP events
    \param[in]  callback: L2CAP event handler
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_l2cap_coc_callback_register(ble_l2cap_coc_evt_handler_t callback);

/*!
    \brief      Unregister callback function from L2CAP module
    \param[in]  callback: L2CAP event handler
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_l2cap_coc_callback_unregister(ble_l2cap_coc_evt_handler_t callback);

/*!
    \brief      Register Simplified Protocol/Service Multiplexer
    \param[in]  spsm: Simplified Protocol/Service Multiplexer
    \param[in]  sec_lvl_bf: security level bit field @def ble_l2cap_sec_lvl_bf
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_l2cap_spsm_register(uint16_t spsm, uint8_t sec_lvl_bf);

/*!
    \brief      Unregister Simplified Protocol/Service Multiplexer
    \param[in]  spsm: Simplified Protocol/Service Multiplexer
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_l2cap_spsm_unregister(uint16_t spsm);

/*!
    \brief      Confirm a L2CAP credit oriented connection is initiated by peer device
    \param[in]  conidx: connection index
    \param[in]  spsm: Simplified Protocol/Service Multiplexer
    \param[in]  cfm: confirm information
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_l2cap_coc_connection_cfm(uint8_t conidx, uint16_t spsm, ble_l2cap_coc_conn_cfm_t cfm);

/*!
    \brief      Create a L2CAP credit oriented connection request
    \param[in]  conidx: connection index
    \param[in]  spsm: Simplified Protocol/Service Multiplexer
    \param[in]  param: L2CAP connection parameters
    \param[in]  enhanced: true to use enhanced L2CAP COC negotiation, otherwise false
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_l2cap_coc_connection_req(uint8_t conidx, uint16_t spsm,
                                ble_l2cap_coc_param_t param, bool enhanced);

/*!
    \brief      Reconfig a L2CAP credit oriented connection paramter
    \param[in]  conidx: connection index
    \param[in]  nb_chan: number of L2CAP channels to reconfigure
    \param[in]  local_rx_mtu: rx mtu to reconfigure
    \param[in]  local_rx_mps: rx mps to reconfigure
    \param[in]  p_chann_lid: pointer to array that contains list of L2CAP channel identifier
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_l2cap_coc_connection_recfg(uint8_t conidx, uint8_t nb_chan,
                                uint16_t local_rx_mtu, uint16_t local_rx_mps, uint8_t *p_chann_lid);

/*!
    \brief      Terminate a L2CAP credit oriented connection
    \param[in]  conidx: connection index
    \param[in]  chan_lid: L2CAP channel identifier
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_l2cap_coc_terminate(uint8_t conidx, uint8_t chan_lid);

/*!
    \brief      Transmit L2CAP segment packet which can be start segment or continuation segment
    \param[in]  conidx: connection index
    \param[in]  chan_lid: L2CAP channel identifier
    \param[in]  length: transmit data length
    \param[in]  p_data: pointer to transmit data value
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_l2cap_coc_sdu_send(uint8_t conidx, uint8_t chan_lid, uint16_t length,
                                    uint8_t *p_data);

/*!
    \brief      Control usage of Enhanced L2CAP COC negotiation
    \param[in]  conidx: connection index
    \param[in]  enable: true to enable Enhanced L2CAP COC negotiation, otherwise false
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_l2cap_coc_enhanced_enable(uint8_t conidx, bool enable);

#ifdef __cplusplus
}
#endif

#endif // _BLE_L2CAP_COC_H__
