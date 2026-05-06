/*!
    \file    ble_adv.h
    \brief   Implementation of BLE advertising module.

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

#ifndef _BLE_ADV_H_
#define _BLE_ADV_H_

#include <stdint.h>

#include "ble_gap.h"
#include "ble_error.h"
#include "ble_adv_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum number of advertising sets can be used simultaneously */
#define BLE_ADV_SET_NUM                     (2)     /*!< !Keep unchanged! */

/* Invalid advertising index */
#define BLE_ADV_INVALID_IDX                 (0xFF)

/* Advertising operation code */
typedef enum
{
    BLE_ADV_OP_CREATE   = 0x00, /*!< Advertising operation code create */
    BLE_ADV_OP_START    = 0x01, /*!< Advertising operation code start */
    BLE_ADV_OP_STOP     = 0x02, /*!< Advertising operation code stop */
    BLE_ADV_OP_RESTART  = 0x03, /*!< Advertising operation code restart */
    BLE_ADV_OP_REMOVE   = 0x04, /*!< Advertising operation code remove */
    BLE_ADV_OP_UPDATE   = 0x05, /*!< Advertising operation code update data */
} ble_adv_op_code_t;

/* Advertising state */
typedef enum
{
    BLE_ADV_STATE_IDLE              = 0x00, /*!< Idle state */
    BLE_ADV_STATE_CREATING          = 0x01, /*!< Advertising set is under creating */
    BLE_ADV_STATE_CREATE            = 0x02, /*!< Advertising set is created but not started */
    BLE_ADV_STATE_ADV_DATA_SET      = 0x03, /*!< Setting advertising data if needed */
    BLE_ADV_STATE_SCAN_RSP_DATA_SET = 0x04, /*!< Setting scan response data if needed */
    BLE_ADV_STATE_PER_ADV_DATA_SET  = 0x05, /*!< Setting periodic advertising data if needed */
    BLE_ADV_STATE_START             = 0x06, /*!< Advertising set is started.*/
} ble_adv_state_t;

/* BLE adv event */
typedef enum
{
    BLE_ADV_EVT_OP_RSP,             /*!< ADV operation response, associated event data is @ref ble_adv_op_rsp_t */
    BLE_ADV_EVT_STATE_CHG,          /*!< ADV state changed, associated event data is @ref ble_adv_state_chg_t */
    BLE_ADV_EVT_DATA_UPDATE_INFO,   /*!< ADV data update information, associated event data is @ref ble_adv_data_update_info_t */
    BLE_ADV_EVT_SCAN_REQ_RCV,       /*!< Scan request received, associated event data is @ref ble_adv_scan_req_rcv_t */
} ble_adv_evt_t;

/* BLE adv data type */
typedef enum
{
    BLE_ADV_DATA_TYPE_ADV,          /*!< Advertising data */
    BLE_ADV_DATA_TYPE_SCAN_RSP,     /*!< Scan response data */
    BLE_ADV_DATA_TYPE_PER_ADV       /*!< Periodic advertising data */
} ble_adv_data_type_t;

/* BLE adv operation response data structure */
typedef struct
{
    uint8_t             adv_idx;    /*!< Local adv index */
    ble_adv_op_code_t   op;         /*!< Operation code */
    uint16_t            status;     /*!< Operation response status, @ref ble_status_t */
} ble_adv_op_rsp_t;

/* BLE adv state change data structure */
typedef struct
{
    uint8_t         adv_idx;    /*!< Local adv index */
    ble_adv_state_t state;      /*!< Current state */
    uint16_t        reason;     /*!< State change reason, @ref ble_status_t */
} ble_adv_state_chg_t;

/* Advertising data updte information */
typedef struct
{
    uint8_t             adv_idx;    /*!< Local adv index */
    ble_adv_data_type_t type;       /*!< Adv data type */
    uint16_t            status;     /*!< Update status, @ref ble_status_t */
} ble_adv_data_update_info_t;

/* Indication of received scan request */
typedef struct
{
    uint8_t         adv_idx;    /*!< Local adv index */
    ble_gap_addr_t  peer_addr;  /*!< Transmitter device address */
} ble_adv_scan_req_rcv_t;

/* BLE adv parameters */
typedef struct
{
    ble_gap_adv_param_t     param;                  /*!< Advertising parameter used by GAP module */
    bool                    include_tx_pwr;         /*!< If transmit power should be included */
    bool                    scan_req_ntf;           /*!< If APP should be notified when receive scan request */
    bool                    restart_after_disconn;  /*!< If connectable advertising should restart when BLE link is disconnected */
} ble_adv_param_t;

/* BLE adv data to set */
typedef struct
{
    bool data_force;                    /*!< True to use data set by APP, otherwise BLE ADV module will encode advertising data using values in p_data_enc */

    union {
        ble_adv_data_t *p_data_enc;     /*!< Advertising data need to be encoded by BLE ADV module, used when data_force is false */
        ble_data_t     *p_data_force;   /*!< Advertising data forced by APP, used when data_force is true */
    } data;
} ble_adv_data_set_t;

/* Prototype of BLE advertising event handler */
typedef void (*ble_adv_evt_handler_t)(ble_adv_evt_t adv_evt, void *p_data, void *p_context);

/*!
    \brief      Create an advertising set
    \param[in]  p_param: pointer to advertising parameters
    \param[in]  hdlr: callback function to be registered, need to handle event defined in @ref ble_adv_evt_t
    \param[in]  p_context: context which will be passed back in the callback function
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adv_create(ble_adv_param_t *p_param, ble_adv_evt_handler_t hdlr, void *p_context);

/*!
    \brief      Start an advertising set which is created successfully
    \param[in]  adv_idx: local advertising set index
    \param[in]  p_adv_data: pointer to advertising data, NULL if no need to set
    \param[in]  p_scan_rsp_data: pointer to scan response data, NULL if no need to set
    \param[in]  p_per_adv_data: pointer to periodic advertising data, NULL if no need to set
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adv_start(uint8_t adv_idx, ble_adv_data_set_t *p_adv_data,
                ble_adv_data_set_t *p_scan_rsp_data, ble_adv_data_set_t *p_per_adv_data);

/*!
    \brief      Restart an advertising set which is stopped
    \param[in]  adv_idx: local advertising set index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adv_restart(uint8_t adv_idx);

/*!
    \brief      Stop an advertising set which is started
    \param[in]  adv_idx: local advertising set index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adv_stop(uint8_t adv_idx);

/*!
    \brief      Remove an advertising set which is not started
    \param[in]  adv_idx: local advertising set index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adv_remove(uint8_t adv_idx);

/*!
    \brief      Update advertising data/scan response data/periodic advertising data when the set is started
    \param[in]  adv_idx: local advertising set index
    \param[in]  p_adv_data: pointer to advertising data, NULL if no need to update
    \param[in]  p_scan_rsp_data: pointer to scan response data, NULL if no need to update
    \param[in]  p_per_adv_data: pointer to periodic advertising data, NULL if no need to update
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adv_data_update(uint8_t adv_idx, ble_adv_data_set_t *p_adv_data,
                ble_adv_data_set_t *p_scan_rsp_data, ble_adv_data_set_t *p_per_adv_data);

#ifdef __cplusplus
}
#endif

#endif // _BLE_ADV_H_
