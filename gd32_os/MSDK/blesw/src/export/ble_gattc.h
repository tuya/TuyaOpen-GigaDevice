/*!
    \file    ble_gattc.h
    \brief   Definitions of gatt client.

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

#ifndef _BLE_GATTC_H_
#define _BLE_GATTC_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include "ble_gatt.h"
#include "ble_conn.h"
#include "ble_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GATT client UUID information structure */
typedef struct
{
    uint8_t instance_id;            /*!< Instance ID */
    ble_uuid_t ble_uuid;            /*!< UUID */
} ble_gattc_uuid_info_t;

/* GATT client service discover done information structure */
typedef struct
{
    bool is_found;                  /*!< If the service is found */
    uint8_t svc_instance_num;       /*!< Instance number */
} ble_gattc_svc_dis_done_t;

/* GATT client read response structure */
typedef struct
{
    ble_status_t status;            /*!< Read result */
    ble_uuid_t   svc_uuid;          /*!< Service UUID */
    ble_uuid_t   char_uuid;         /*!< Characteristic UUID */
    uint8_t      svc_instance_id;   /*!< Service instance ID */
    uint16_t     handle;            /*!< Attribute handle */
    uint16_t     length;            /*!< Value length */
    uint8_t     *p_value;           /*!< Value data pointer */
} ble_gattc_read_rsp_t;

/* GATT client write response structure */
typedef struct
{
    ble_status_t status;            /*!< Write result */
    ble_uuid_t   svc_uuid;          /*!< Service UUID */
    ble_uuid_t   char_uuid;         /*!< Characteristic UUID */
    uint16_t     handle;            /*!< Attribute handle */
} ble_gattc_write_rsp_t;

/* GATT client notification indication structure */
typedef struct
{
    ble_uuid_t svc_uuid;            /*!< Service UUID */
    ble_uuid_t char_uuid;           /*!< Characteristic UUID */
    uint16_t   handle;              /*!< Attribute handle */
    uint16_t   length;              /*!< Value length */
    uint8_t   *p_value;             /*!< Value pointer */
    bool       is_ntf;              /*!< Is notification or indication */
} ble_gattc_ntf_ind_t;

/* GATT client MTU update response structure */
typedef struct
{
    ble_status_t status;            /*!< MTU update result */
} ble_gattc_mtu_update_rsp_t;

/* GATT client MTU information structure */
typedef struct
{
    uint16_t    mtu;                /*!< MTU size */
} ble_gattc_mtu_info_t;

/* GATT client event type */
typedef enum
{
    BLE_CLI_EVT_CONN_STATE_CHANGE_IND,  /*!< Connection state change event */
    BLE_CLI_EVT_GATT_OPERATION,         /*!< Gatt operation event */
} ble_gattc_evt_t;

/* GATT client operation subevent type */
typedef enum
{
    BLE_CLI_EVT_SVC_DISC_DONE_RSP,      /*!< Discovery all service done event */
    BLE_CLI_EVT_READ_RSP,               /*!< Read response event */
    BLE_CLI_EVT_WRITE_RSP,              /*!< Write response event */
    BLE_CLI_EVT_NTF_IND_RCV,            /*!< Notification/indication received event */
    BLE_CLI_EVT_MTU_UPDATE_RSP,         /*!< MTU update response event */
    BLE_CLI_EVT_MTU_INFO,               /*!< MTU information event */
} ble_gattc_op_sub_evt_t;

/* GATT client connection information structure */
typedef struct
{
    uint8_t         conn_idx;       /*!< Connection index */
    ble_gap_addr_t  peer_addr;      /*!< Bluetooth address of peer device */
} ble_gattc_conn_info_t;

/* GATT client disconnection information structure */
typedef struct
{
    uint8_t         conn_idx;       /*!< Connection index.  */
    uint16_t        reason;         /*!< Disconnect reason */
} ble_gattc_disconn_info_t;

/* GATT client connection state change information structure */
typedef struct
{
    ble_conn_state_t conn_state;                /*!< Connection state.  */
    union {
        ble_gattc_conn_info_t     conn_info;    /*!< Connect information */
        ble_gattc_disconn_info_t  disconn_info; /*!< Disconnect information */
    } info;
} ble_gattc_conn_state_change_ind_t;

/* GATT client operation information structure */
typedef struct
{
    ble_gattc_op_sub_evt_t    gattc_op_sub_evt;         /*!< Gatts operation sub event */
    uint8_t conn_idx;                                   /*!< Connection index.  */
    union {
        ble_gattc_svc_dis_done_t     svc_dis_done_ind;  /*!< Service discovery done indication */
        ble_gattc_read_rsp_t         read_rsp;          /*!< Read response */
        ble_gattc_write_rsp_t        write_rsp;         /*!< Write response */
        ble_gattc_ntf_ind_t          ntf_ind;           /*!< Notification/indication message */
        ble_gattc_mtu_update_rsp_t   mtu_update_rsp;    /*!< MTU update response */
        ble_gattc_mtu_info_t         mtu_info;          /*!< MTU information */
    } gattc_op_data;
} ble_gattc_op_info_t;

/* GATT client message structure */
typedef struct
{
    ble_gattc_evt_t cli_msg_type;           /*!< Gattc message type */
    union
    {
        ble_gattc_conn_state_change_ind_t    conn_state_change_ind;     /*!< Connection state change indication */
        ble_gattc_op_info_t                  gattc_op_info;             /*!< Gattc opration information */
    } msg_data;
} ble_gattc_msg_info_t;

/* Prototype of BLE GATT client message handler */
typedef ble_status_t (*p_fun_cli_cb)(ble_gattc_msg_info_t *p_cli_msg_info);

/* Prototype of BLE GATT discovery done handler */
typedef void (*p_discovery_done_cb)(uint8_t conn_idx, uint16_t status);

/*!
    \brief      Start GATT discovery
    \param[in]  conn_idx: connection index
    \param[in]  callback: callback function to handle when GATT discovery is done
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gattc_start_discovery(uint8_t conn_idx, p_discovery_done_cb callback);

/*!
    \brief      Register callback function to handle GATT client message
    \param[in]  p_svc_uuid: pointer to service UUID
    \param[in]  p_cb: GATT client message handler
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gattc_svc_reg(ble_uuid_t *p_svc_uuid, p_fun_cli_cb p_cb);

/*!
    \brief      Unregister callback function from GATT client module
    \param[in]  p_svc_uuid: pointer to service UUID
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gattc_svc_unreg(ble_uuid_t *p_svc_uuid);

/*!
    \brief      Find characteristic value handle
    \param[in]  conn_idx: connection index
    \param[in]  svc_uuid: pointer to service UUID information
    \param[in]  char_uuid: pointer to characteristic UUID information
    \param[out] handle: pointer to attribute value handle
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gattc_find_char_handle(uint8_t conn_idx, ble_gattc_uuid_info_t *svc_uuid, ble_gattc_uuid_info_t *char_uuid, uint16_t *handle);

/*!
    \brief      Find characteristic description handle
    \param[in]  conn_idx: connection index
    \param[in]  svc_uuid: pointer to service UUID information
    \param[in]  char_uuid: pointer to characteristic UUID information
    \param[in]  desc_uuid: pointer to description UUID information
    \param[out] handle: pointer to attribute value handle
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gattc_find_desc_handle(uint8_t conn_idx, ble_gattc_uuid_info_t *svc_uuid,
                                      ble_gattc_uuid_info_t *char_uuid, ble_gattc_uuid_info_t *desc_uuid, uint16_t *handle);

/*!
    \brief      Send GATT read request
    \param[in]  conidx: connection index
    \param[in]  hdl: attribute handle
    \param[in]  offset: read offset
    \param[in]  length: read length
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gattc_read(uint8_t conidx, uint16_t hdl, uint16_t offset, uint16_t length);

/*!
    \brief      Send GATT write request
    \param[in]  conidx: connection index
    \param[in]  hdl: attribute handle
    \param[in]  length: write length
    \param[in]  p_value: pointer to write value
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gattc_write_req(uint8_t conidx, uint16_t hdl, uint16_t length, uint8_t *p_value);

/*!
    \brief      Send GATT write command
    \param[in]  conidx: connection index
    \param[in]  hdl: attribute handle
    \param[in]  length: write length
    \param[in]  p_value: pointer to write value
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gattc_write_cmd(uint8_t conidx, uint16_t hdl, uint16_t length, uint8_t *p_value);

/*!
    \brief      Send GATT signed write command
    \param[in]  conidx: connection index
    \param[in]  hdl: attribute handle
    \param[in]  length: write length
    \param[in]  p_value: pointer to write value
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gattc_write_signed(uint8_t conidx, uint16_t hdl, uint16_t length, uint8_t *p_value);

/*!
    \brief      Update GATT mtu
    \param[in]  conidx: connection index
    \param[in]  mtu_size: prefered mtu size to update, 0 means no preference and stack will use max available size
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gattc_mtu_update(uint8_t conidx, uint16_t mtu_size);

/*!
    \brief      Get GATT mtu size
    \param[in]  conidx: connection index
    \param[out] p_mtu: pointer to mtu size
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gattc_mtu_get(uint8_t conidx, uint16_t *p_mtu);

#ifdef __cplusplus
}
#endif

#endif // _BLE_GATTC_H_
