/*!
    \file    ble_iso.h
    \brief   BLE BIG/CIG/ISO Manager.

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

#ifndef _BLE_ISO_H_
#define _BLE_ISO_H_

#include "ble_error.h"
#include "ble_gap.h"

/* Max supported ISO steam(BIS/CIS) number */
#define BLE_ISO_MAX_STREAM_NUM      2           /*!< !Keep unchanged! */

/* BLE ISO events */
typedef enum
{
    BLE_ISO_EVT_BIG_INFO_RX,            /*!< BIG information received, associated event data is @ref ble_gap_big_info_t */
    BLE_ISO_EVT_BIG_CREATE_FAIL,        /*!< BIG create fail, associated event data is @ref ble_iso_create_fail_info_t */
    BLE_ISO_EVT_BIG_STREAM_INFO,        /*!< BIG stream local index information, associated event data is @ref ble_iso_big_stream_info_t */
    BLE_ISO_EVT_BIG_CREATE_INFO,        /*!< BIG create success, associated event data is @ref ble_gap_big_create_info_t */
    BLE_ISO_EVT_BIG_SYNC_STATUS,        /*!< BIG sync status changed, associated event data is @ref ble_gap_big_sync_status_info_t */
    BLE_ISO_EVT_BIG_SYNC_INFO,          /*!< BIG sync established, associated event data is @ref ble_gap_big_sync_added_info_t */
    BLE_ISO_EVT_CIG_CREATE_FAIL,        /*!< CIG create fail, associated event data is @ref ble_iso_create_fail_info_t */
    BLE_ISO_EVT_CIG_CREATE_INFO,        /*!< CIG create success, associated event data is @ref ble_iso_cig_create_info_t */
    BLE_ISO_EVT_CIS_CONN_INFO,          /*!< CIS connected, associated event data is @ref ble_gap_cis_conn_info_t */
    BLE_ISO_EVT_CIS_DISCONN_INFO,       /*!< CIS disconnected, associated event data is @ref ble_gap_cis_disconn_info_t */
    BLE_ISO_EVT_ISO_TEST_CNT,           /*!< ISO stream receive count in test mode, associated event data is @ref ble_gap_iso_test_cnt_info_t */
} ble_iso_evt_t;

/* BIG state */
typedef enum
{
    BLE_ISO_STATE_BIG_IDLE,             /*!< IDLE state */
    BLE_ISO_STATE_BIG_CREATING,         /*!< BIG is under creating */
    BLE_ISO_STATE_BIG_CREATED           /*!< BIG is created */
} ble_iso_big_state_t;

/* BLE ISO test payload type */
typedef enum
{
    BLE_ISO_TEST_PL_TYPE_ZERO,          /*!< Zero length payload */
    BLE_ISO_TEST_PL_TYPE_VARIABLE,      /*!< Variable length payload */
    BLE_ISO_TEST_PL_TYPE_MAX,           /*!< Maximum length payload */
} ble_iso_test_payload_type_t;

/* BIG parameters */
typedef union
{
    ble_gap_big_param_t         param;          /*!< BIG parameters used for Non-Test command */
    ble_gap_big_test_param_t    test_param;     /*!< BIG parameters used for Test command */
} ble_iso_big_param_t;

/* CIG parameters */
typedef union
{
    ble_gap_cig_param_t         param;          /*!< CIG parameters used for Non-Test command */
    ble_gap_cig_test_param_t    test_param;     /*!< CIG parameters used for Test command */
} ble_iso_cig_param_t;

/* CIS parameters */
typedef union
{
    ble_gap_cis_param_t         param;          /*!< CIS parameters used for Non-Test command */
    ble_gap_cis_test_param_t    test_param;     /*!< CIS parameters used for Test command */
} ble_iso_cis_param_t;

/* ISO create fail information */
typedef struct
{
    uint16_t    status;                         /*!< ISO create fail status, @ref ble_status_t */
} ble_iso_create_fail_info_t;

/* BIG stream information */
typedef struct
{
    uint8_t     big_handle;     /*!< BIG Handle */
    uint8_t     stream_num;     /*!< Number of streams in the group */
    uint8_t     group_lid;      /*!< Allocated group local index */
    uint8_t    *p_stream_lid;   /*!< List of allocated stream local index */
} ble_iso_big_stream_info_t;

/* CIG create success information */
typedef struct
{
    uint8_t     group_lid;      /*!< Group local index */
} ble_iso_cig_create_info_t;

/* Prototype of BLE ISO event handler */
typedef void (*ble_iso_evt_handler_t)(ble_iso_evt_t event, void *p_data);

/*!
    \brief      Register callback function to handle ISO event
    \param[in]  callback: ISO event handle function
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_iso_callback_register(ble_iso_evt_handler_t callback);

/*!
    \brief      Unregister callback function from BLE ISO module
    \param[in]  callback: ISO event handle function
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_iso_callback_unregister(ble_iso_evt_handler_t callback);

/*!
    \brief      Create a BIG
    \param[in]  adv_idx: associated periodic advertising index
    \param[in]  test_cmd: true to use Test command, otherwise use Non-Test command
    \param[in]  big_hdl: BIG handle value
    \param[in]  bis_num: BIS number in the BIG
    \param[in]  p_param: pointer to BIG parameters
    \param[in]  encrypt: true to create an encrypted BIG, otherwise create an unencrypted one
    \param[in]  p_bc: pointer to broadcast code for encypted BIG. Use NULL for unencrypted BIG
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_iso_big_create(uint8_t adv_idx, bool test_cmd, uint8_t big_hdl, uint8_t bis_num,
                    ble_iso_big_param_t *p_param, bool encrypt, uint8_t *p_bc);

/*!
    \brief      Terminate a BIG
    \param[in]  group_lid: BIG local index, can be get in @ref BLE_ISO_EVT_BIG_CREATE_INFO event
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_iso_big_terminate(uint8_t group_lid);

/*!
    \brief      Create a BIG Sync
    \param[in]  sync_idx: periodic advertising synchronization index
    \param[in]  big_hdl: BIG handle
    \param[in]  bis_num: BIS number to sync
    \param[in]  sync_tout_ms: maximum permitted time between successful receptions of BIS PDUs
    \param[in]  mse: maximum number of subevents the controller should use to receive data payloads in each interval
    \param[in]  encrypt: true to sync an encrypted BIG, otherwise to sync an unencrypted one
    \param[in]  p_bc: pointer to broadcast code for encypted BIG. Use NULL for unencrypted BIG
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_iso_big_sync_create(uint8_t sync_idx, uint8_t big_hdl, uint8_t bis_num,
                uint16_t sync_tout_ms, uint8_t mse, bool encrypt, uint8_t *p_bc);

/*!
    \brief      Terminate a BIG Sync
    \param[in]  group_lid: BIG local index, can be get in @ref BLE_ISO_EVT_BIG_SYNC_INFO event
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_iso_big_sync_terminate(uint8_t group_lid);

/*!
    \breif      Create a CIG
    \param[in]  conn_idx: associated LE connection index
    \param[in]  cig_id: CIG ID
    \param[in]  cis_num: CIS number in the CIG
    \param[in]  test_cmd: true to use Test command, otherwise use Non-Test command
    \param[in]  p_cig_param: pointer to CIG parameters
    \param[in]  p_cis_param: pointer to CIS parameters
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_iso_cig_create(uint8_t conn_idx, uint8_t cig_id, uint8_t cis_num, bool test_cmd,
                ble_iso_cig_param_t *p_cig_param, ble_iso_cis_param_t *p_cis_param);

/*!
    \breif      Prepare a CIS stream so it can be accepted when remote device create the CIS
    \param[in]  conn_idx: associated LE connection index.
    \param[in]  cig_id: CIG ID.
    \param[in]  cis_id: CIS ID
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_iso_cis_prepare(uint8_t conn_idx, uint8_t cig_id, uint8_t cis_id);

/*!
    \breif      Disconnect a CIS
    \param[in]  stream_lid: stream local index, can be get in @ref BLE_ISO_EVT_CIS_CONN_INFO event
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_iso_cis_disconn(uint8_t stream_lid);

/*!
    \breif      Terminate a CIG
    \param[in]  group_lid: CIG local index, can be get in @ref BLE_ISO_EVT_CIG_CREATE_INFO event
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_iso_cig_terminate(uint8_t group_lid);

/*!
    \breif      Start ISO tx test
    \param[in]  stream_lid: stream local index, can be get in @ref BLE_ISO_EVT_BIG_STREAM_INFO or @ref BLE_ISO_EVT_CIS_CONN_INFO event
    \param[in]  type: payload type, @ref ble_iso_test_payload_type_t
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_iso_test_tx(uint8_t stream_lid, uint8_t type);

/*!
    \breif      Start ISO rx test
    \param[in]  stream_lid: stream local index, can be get in @ref BLE_ISO_EVT_BIG_SYNC_INFO or @ref BLE_ISO_EVT_CIS_CONN_INFO event
    \param[in]  type: payload type, @ref ble_iso_test_payload_type_t
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_iso_test_rx(uint8_t stream_lid, uint8_t type);

/*!
    \breif      Stop ISO tx/rx test
    \param[in]  stream_lid: Stream local index, can be get in @ref BLE_ISO_EVT_BIG_STREAM_INFO,
                            @ref BLE_ISO_EVT_BIG_SYNC_INFO or @ref BLE_ISO_EVT_CIS_CONN_INFO event
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_iso_test_end(uint8_t stream_lid);

/*!
    \breif      Read the test counters in ISO test rx mode
    \param[in]  stream_lid: Stream local index, can be get in @ref BLE_ISO_EVT_BIG_SYNC_INFO or @ref BLE_ISO_EVT_CIS_CONN_INFO event
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_iso_test_cnt_read(uint8_t stream_lid);

#endif // _BLE_ISO_H_
