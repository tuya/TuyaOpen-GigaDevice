/*!
    \file    ble_gatts.h
    \brief   Definitions of gatt server.

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

#ifndef _BLE_GATTS_H_
#define _BLE_GATTS_H_

#include <stdint.h>
#include "ble_gatt.h"
#include "ble_conn.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GATT server event type */
typedef enum
{
    BLE_SRV_EVT_SVC_ADD_RSP,           /*!< Add service response event */
    BLE_SRV_EVT_SVC_RMV_RSP,           /*!< Remove service response event */
    BLE_SRV_EVT_CONN_STATE_CHANGE_IND, /*!< Connection state change indication event */
    BLE_SRV_EVT_GATT_OPERATION,        /*!< Server gatt operation event */
} ble_gatts_evt_t;

/* GATT server operation subevent type */
typedef enum
{
    BLE_SRV_EVT_READ_REQ,              /*!< Read request event */
    BLE_SRV_EVT_WRITE_REQ,             /*!< Write request event */
    BLE_SRV_EVT_NTF_IND_SEND_RSP,      /*!< Notify/indicate response event */
    BLE_SRV_EVT_NTF_IND_MTP_SEND_RSP,  /*!< Notify/indicate multiple response event */
    BLE_SRV_EVT_MTU_INFO,              /*!< MTU information event */
} ble_gatts_op_sub_evt_t;

/* GATT server service add response structure */
typedef struct
{
    uint16_t        status;     /*!< Status of the operation (see enum #ble_err_t) */
    uint16_t        start_hdl;  /*!< Service start handle */
    uint8_t         svc_id;     /*!< Service identifier */
} ble_gatts_svc_add_rsp_t;

/* GATT server service remove response structure */
typedef struct
{
    uint16_t        status;     /*!< Status of the operation (see enum #ble_err_t) */
    uint8_t         svc_id;     /*!< Service identifier */
} ble_gatts_svc_rmv_rsp_t;

/* GATT server read request indication structure */
typedef struct
{
    uint8_t        svc_id;      /*!< service id */
    uint16_t       token;       /*!< Token provided by GATT module that must be used in the confirm */
    uint16_t       att_idx;     /*!< Attribute index */
    uint16_t       offset;      /*!< Value offset */
    bool           pending_cfm; /*!< pending confirm */
    uint16_t       max_len;     /*!< Readmax length */
    uint16_t       val_len;     /*!< Value length */
    uint16_t       att_len;     /*!< Attribute length */
    uint8_t       *p_val;       /*!< Value pointer */
} ble_gatts_read_req_t;

/* GATT server write request indication structure */
typedef struct
{
    uint8_t        svc_id;      /*!< service id */
    uint16_t       token;       /*!< Token provided by GATT module that must be used in the confirm */
    uint16_t       att_idx;     /*!< Attribute index */
    uint16_t       offset;      /*!< Value offset */
    bool           pending_cfm; /*!< Pending confirm */
    bool           local_req;   /*!< Write requested by local */
    uint16_t       val_len;     /*!< Value length */
    uint8_t       *p_val;       /*!< Value pointer */
} ble_gatts_write_req_t;

/* GATT server notification/indication send response structure */
typedef struct
{
    uint16_t    status;     /*!< Status of the operation (see enum #ble_err_t) */
    uint8_t     svc_id;     /*!< Service identifier */
    uint16_t    att_idx;    /*!< Attribute index */
    uint8_t     type;       /*!< Send type, @ref ble_gatt_evt_type_t */
} ble_gatts_ntf_ind_send_rsp_t;

/* GATT server notification/indication multiple send response structure */
typedef struct
{
    uint16_t    status;     /*!< Status of the operation (see enum #ble_err_t) */
    uint8_t     svc_id;     /*!< Service identifier */
    uint16_t    att_idx;    /*!< Attribute index */
} ble_gatts_ntf_ind_mtp_send_rsp_t;

/* GATT server MTU information structure */
typedef struct
{
    uint16_t    mtu;        /*!< MTU size */
} ble_gatts_mtu_info_t;

/* GATT server connection information structure */
typedef struct
{
    uint8_t         conn_idx;   /*!< Connection index */
    ble_gap_addr_t  peer_addr;  /*!< Bluetooth address of peer device */
} ble_gatts_conn_info_t;

/* GATT server disconnection information structure */
typedef struct
{
    uint8_t         conn_idx;   /*!< Connection index */
    uint16_t        reason;     /*!< Disconnect reason */
} ble_gatts_disconn_info_t;

/* GATT server connection state change information structure */
typedef struct
{
    ble_conn_state_t conn_state;  /*!< Connection state.  */
    union {
        ble_gatts_conn_info_t     conn_info;     /*!< Connect information */
        ble_gatts_disconn_info_t  disconn_info;  /*!< Disconnect information */
    } info;
} ble_gatts_conn_state_change_ind_t;

/* GATT server operation information structure */
typedef struct
{
    ble_gatts_op_sub_evt_t    gatts_op_sub_evt;                         /*!< Gatts operation sub event */
    uint8_t conn_idx;                                                   /*!< Connection index */
    union {
        ble_gatts_read_req_t                    read_req;               /*!< Read request */
        ble_gatts_write_req_t                   write_req;              /*!< Write request */
        ble_gatts_ntf_ind_send_rsp_t            ntf_ind_send_rsp;       /*!< service notify/indicate send */
        ble_gatts_ntf_ind_mtp_send_rsp_t        ntf_ind_mtp_send_rsp;   /*!< service notify/indicate multiple send */
        ble_gatts_mtu_info_t                    mtu_info;               /*!< MTU information */
    } gatts_op_data;
} ble_gatts_op_info_t;

/* GATT server message information structure */
typedef struct
{
    ble_gatts_evt_t srv_msg_type;               /*!< Gatts message type */
    union ble_gatts_data
    {
        ble_gatts_svc_add_rsp_t                 svc_add_rsp;            /*!< Service add response */
        ble_gatts_svc_rmv_rsp_t                 svc_rmv_rsp;            /*!< Service remove response */
        ble_gatts_conn_state_change_ind_t       conn_state_change_ind;  /*!< Connection state change indication */
        ble_gatts_op_info_t                     gatts_op_info;          /*!< Gatts opration information */
    } msg_data;
} ble_gatts_msg_info_t;


/* Prototype of BLE GATT server message handler */
typedef ble_status_t (*p_fun_srv_cb)(ble_gatts_msg_info_t *p_srv_msg_info);

/* Prototype of BLE GATT server service list handler */
typedef void (*p_fun_svc_list_cb)(uint8_t svc_id, const uint8_t *p_svc_uuid, uint8_t svc_type, uint8_t svc_info);

/* Prototype of BLE GATT characteristic service list handler */
typedef void (*p_fun_char_list_cb)(uint8_t svc_id, const uint8_t *p_char_uuid, uint16_t char_val_idx, uint16_t char_info);

/* Prototype of BLE GATT descriptor service list handler */
typedef void (*p_fun_desc_list_cb)(uint8_t svc_id, const uint8_t *p_desc_uuid, uint16_t desc_idx, uint16_t char_info);

/*!
    \brief      Add service to GATT server module
    \param[in]  uuid: pointer to service UUID
    \param[in]  start_hdl: attribute start handle, 0 means randomly assign by GATT server module
    \param[in]  info: service information, @ref ble_gatt_svc_info_bf
    \param[in]  p_table: pointer to attribute table, should be an array of @ref ble_gatt_attr_desc_t
    \param[in]  table_length: attribute table size
    \param[in]  srv_cb: callback function to handle GATT server messages
    \param[out] p_svc_id: pointer to service ID assigned by GATT server module
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gatts_svc_add(uint8_t *p_svc_id, const uint8_t *uuid, uint16_t start_hdl, uint8_t info,
                        const void *p_table, uint16_t table_length, p_fun_srv_cb srv_cb);

/*!
    \brief      Remove service from GATT server module
    \param[in]  svc_id: service ID
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gatts_svc_rmv(uint8_t svc_id);

/*!
    \brief      Send notification/indication to remote device
    \param[in]  conn_idx: connection index
    \param[in]  svc_id: service ID
    \param[in]  att_idx: attribute index in service attribute table
    \param[in]  p_val: pointer to notification/indication value to send
    \param[in]  len: notification/indication value length
    \param[in]  evt_type: event type(notification or indication)
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gatts_ntf_ind_send(uint8_t conn_idx, uint8_t svc_id, uint16_t att_idx, uint8_t *p_val,
                             uint16_t len, ble_gatt_evt_type_t evt_type);

/*!
    \brief      Send notification/indication by handle
    \param[in]  conn_idx: connection index
    \param[in]  handle: attribute handle
    \param[in]  p_val: pointer to notification/indication value to send
    \param[in]  len: notification/indication value length
    \param[in]  evt_type: event type(notification or indication)
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gatts_ntf_ind_send_by_handle(uint8_t conn_idx, uint16_t handle, uint8_t *p_val,
                                              uint16_t len, ble_gatt_evt_type_t evt_type);

/*!
    \brief      Send notification/indication to multiple remote devices
    \param[in]  conidx_bf: connection index bit field
    \param[in]  svc_id: service ID
    \param[in]  att_idx: attribute index in service attribute table
    \param[in]  p_val: pointer to notification/indication value to send
    \param[in]  len: notification/indication value length
    \param[in]  evt_type: event type(notification or indication)
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gatts_ntf_ind_mtp_send(uint32_t conidx_bf, uint8_t svc_id, uint16_t att_idx,
                             uint8_t *p_val, uint16_t len, ble_gatt_evt_type_t evt_type);

/*!
    \brief      Get GATT mtu
    \param[in]  conidx: connection index
    \param[out] p_mtu: pointer to GATT mtu
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gatts_mtu_get(uint8_t conidx, uint16_t *p_mtu);

/*!
    \brief      Confirm attribute write request from peer client
    \param[in]  conn_idx: connection index
    \param[in]  token: token value get from the write request indication
    \param[in]  status: confirm status
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gatts_svc_attr_write_cfm(uint8_t conn_idx, uint16_t token, uint16_t status);

/*!
    \brief      Confirm attribute read request from peer client
    \param[in]  conn_idx: connection index
    \param[in]  token: token value get from the read request indication
    \param[in]  status: confirm status
    \param[in]  total_len: attribute total length
    \param[in]  value_len: attribute length to confirm
    \param[in]  p_value: pointer to attribute value to confirm
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gatts_svc_attr_read_cfm(uint8_t conn_idx, uint16_t token, uint16_t status,
                            uint16_t total_len, uint16_t value_len, uint8_t *p_value);

/*!
    \brief      Get service start handle
    \param[in]  svc_id: service ID
    \param[out] p_handle: pointer to start handle of the service
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gatts_get_start_hdl(uint8_t svc_id, uint16_t *p_handle);

/*!
    \brief      Set attribute value, BLE_SRV_EVT_GATT_OPERATION event with subevent BLE_SRV_EVT_WRITE_REQ
                will be sent to the corresponding service callback function
    \param[in]  conn_idx: connection index
    \param[in]  svc_id: service ID
    \param[in]  char_idx: characteristic index
    \param[in]  len: value length
    \param[in]  p_value: pointer to value
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gatts_set_attr_val(uint8_t conn_idx, uint8_t svc_id, uint8_t char_idx, uint16_t len, uint8_t *p_value);

/*!
    \brief      List service
    \param[in]  cb: service list callback
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gatts_list_svc(p_fun_svc_list_cb cb);

/*!
    \brief      List characteristic
    \param[in]  svc_id: service ID
    \param[in]  cb: characteristic list callback
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gatts_list_char(uint8_t svc_id, p_fun_char_list_cb cb);

/*!
    \brief      List characteristic
    \param[in]  svc_id: service ID
    \param[in]  char_val_idx: characteristic value index
    \param[in]  cb: descripter list callback
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_gatts_list_desc(uint8_t svc_id, uint16_t char_val_idx, p_fun_desc_list_cb cb);

#ifdef __cplusplus
}
#endif

#endif // _BLE_GATTS_H_
