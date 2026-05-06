/*!
    \file    ble_per_sync.h
    \brief   Module for handling the BLE periodic sync.

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

#ifndef _BLE_PER_SYNC_H__
#define _BLE_PER_SYNC_H__

#include <stdint.h>
#include "ble_gap.h"
#include "ble_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Enumeration for periodic sync event */
typedef enum
{
    BLE_PER_SYNC_EVT_START_RSP,     /*!< Periodic sync start response */
    BLE_PER_SYNC_EVT_CANCEL_RSP,    /*!< Periodic sync cancel response */
    BLE_PER_SYNC_EVT_TERMINATE_RSP, /*!< Periodic sync terminate response */
    BLE_PER_SYNC_EVT_STATE_CHG,     /*!< Periodic sync change event */
    BLE_PER_SYNC_EVT_REPORT,        /*!< Periodic advertising received event.*/
    BLE_PER_SYNC_EVT_ESTABLISHED,   /*!< Periodic sync established event */
    BLE_PER_SYNC_EVT_RPT_CTRL_RSP,  /*!< Periodic sync report control response event */
} ble_per_sync_evt_t;

/* Enumeration for periodic sync state */
typedef enum
{
    BLE_PER_SYNC_STATE_TERMINATED,  /*!< Periodic sync state terminated */
    BLE_PER_SYNC_STATE_SYNCING,     /*!< Periodic sync state syncing */
    BLE_PER_SYNC_STATE_SYNCED,      /*!< Periodic sync state synced */
    BLE_PER_SYNC_STATE_CANCELING,   /*!< Periodic sync state canceling */
    BLE_PER_SYNC_STATE_TERMINATING, /*!< Periodic sync state terminating */
} ble_per_sync_state_t;

/* Enumeration for periodic sync report bit */
typedef enum
{
    BLE_PER_SYNC_RPT_ADV_EN_BIT         = 0x01,     /*!< Periodic advertising reports reception enabled */
    BLE_PER_SYNC_RPT_BIG_EN_BIT         = 0x02,     /*!< BIG Info advertising reports reception enabled */
    BLE_PER_SYNC_RPT_DUP_FILTER_EN_BIT  = 0x04,     /*!< Duplicate filtering enabled */
} ble_per_sync_rpt_ctrl_bit_t;

/* Periodic sync start response structure */
typedef struct
{
    uint16_t                status;     /*!< Periodic sync start response status, @ref ble_status_t */
} ble_per_sync_start_rsp_t;

/* Periodic sync cancel response structure */
typedef struct
{
    uint16_t                status;     /*!< Periodic sync cancel response status, @ref ble_status_t */
} ble_per_sync_cancel_rsp_t;

/* Periodic sync terminate response structure */
typedef struct
{
    uint16_t                status;     /*!< Periodic sync terminate response status, @ref ble_status_t */
} ble_per_sync_terminate_rsp_t;

/* Periodic sync state change structure */
typedef struct
{
    uint8_t                 sync_idx;   /*!< Periodic sync activity index */
    ble_per_sync_state_t    state;      /*!< Data structure for @ref ble_per_sync_state_t */
    uint16_t                reason;     /*!< Periodic sync change reason */
} ble_per_sync_state_chg_t;

/* Periodic sync advertising report structure */
typedef struct
{
    ble_gap_adv_report_info_t  *p_report;   /*!< Periodic sync advertising report information */
} ble_per_adv_rpt_t;

/* Periodic sync established structure */
typedef struct
{
    ble_gap_per_sync_estab_info_t    param;     /*!< Periodic sync established information */
} ble_per_sync_established_t;

/* Periodic sync report control structure */
typedef struct
{
    ble_gap_per_sync_rpt_ctrl_rsp_t param;  /*!< Periodic sync report control information */
} ble_per_sync_rpt_ctrl_rsp_t;

/* Periodic sync data */
typedef union ble_per_sync_data
{
    ble_per_sync_start_rsp_t        start_rsp;      /*!< Periodic sync start response */
    ble_per_sync_cancel_rsp_t       cancel_rsp;     /*!< Periodic sync cancel response */
    ble_per_sync_terminate_rsp_t    terminate_rsp;  /*!< Periodic sync terminate response */
    ble_per_sync_state_chg_t        sync_state;     /*!< Periodic sync state change */
    ble_per_adv_rpt_t               report;         /*!< Periodic sync advertising report */
    ble_per_sync_established_t      establish;      /*!< Periodic sync established */
    ble_per_sync_rpt_ctrl_rsp_t     rpt_ctrl_rsp;   /*!< Periodic sync report control */
} ble_per_sync_data_u;

/* Prototype of periodic sync meassage handler */
typedef void (*ble_per_sync_evt_handler_t)(ble_per_sync_evt_t event, ble_per_sync_data_u * p_data);

/*!
    \brief      Register callback function to handle BLE periodic sync events
    \param[in]  callback: BLE periodic sync event handler function
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_per_sync_callback_register(ble_per_sync_evt_handler_t callback);

/*!
    \brief      Unregister callback function from handle BLE periodic sync module
    \param[in]  callback: BLE periodic sync event handler function
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_per_sync_callback_unregister(ble_per_sync_evt_handler_t callback);

/*!
    \brief      Start periodic sync
    \param[in]  own_addr_type: local address type
    \param[in]  p_param: pointer to parameters used for periodic sync
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_per_sync_start(ble_gap_local_addr_type_t own_addr_type, ble_gap_per_sync_param_t *p_param);

/*!
    \brief      Cancel periodic sync start procedure
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_per_sync_cancel(void);

/*!
    \brief      Terminate periodic sync
    \param[in]  sync_idx: periodic sync index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_per_sync_terminate(uint8_t sync_idx);

/*!
    \brief      Control periodic sync report
    \param[in]  sync_idx: periodic sync index
    \param[in]  ctrl: periodic sync report control bit, @ref ble_per_sync_rpt_ctrl_bit_t
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_per_sync_report_ctrl(uint8_t sync_idx, uint8_t ctrl);

#ifdef __cplusplus
}
#endif

#endif // _BLE_PER_SYNC_H__
