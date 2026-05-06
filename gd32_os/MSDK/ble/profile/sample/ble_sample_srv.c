/*!
    \file    ble_sample_srv.c
    \brief   Implementations of ble sample server

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

#include <string.h>

#include "ble_sample_srv.h"
#include "ble_gap.h"
#include "ble_gatt.h"
#include "ble_gatts.h"
#include "ble_error.h"
#include "dbg_print.h"
#include "ble_utils.h"
#include "ble_storage.h"

/* BLE sample server service UUID */
#define UUID_BLE_SAMPLE_SRV_SERVICE_128         {0xEF,0xCD,0xAB,0x89,0x67,0x45,0x23,0x01,0x00,0x00,0x00,0x00,0x11,0x11,0x00,0x00}

/* BLE sample server read Characteristic UUID */
#define UUID_BLE_SAMPLE_SRV_READ_HANDLE_128     {0xEF,0xCD,0xAB,0x89,0x67,0x45,0x23,0x01,0x00,0x00,0x00,0x00,0x22,0x22,0x00,0x00}

/* BLE sample server write Characteristic UUID */
#define UUID_BLE_SAMPLE_SRV_WRITE_HANDLE_128    {0xEF,0xCD,0xAB,0x89,0x67,0x45,0x23,0x01,0x00,0x00,0x00,0x00,0x33,0x33,0x00,0x00}

/* BLE sample server notify Characteristic UUID */
#define UUID_BLE_SAMPLE_SRV_NOTIFY_HANDLE_128   {0xEF,0xCD,0xAB,0x89,0x67,0x45,0x23,0x01,0x00,0x00,0x00,0x00,0x44,0x44,0x00,0x00}

#if STORAGE_FEAT_SUPPORT
/* BLE sample server storage Characteristic UUID */
#define UUID_BLE_SAMPLE_SRV_STORAGE_HANDLE_128  {0xEF,0xCD,0xAB,0x89,0x67,0x45,0x23,0x01,0x00,0x00,0x00,0x00,0x55,0x55,0x00,0x00}
#endif

/* Max length that BLE sample server Characteristic value can be written */
#define BLE_SAMPLE_SRV_WRITE_MAX_LEN         512

/* Sample data length */
#define BLE_SAMPLE_DATA_LENGTH               2

/* Storage sample data struct*/
typedef struct
{
    uint16_t cccd_value;
    uint8_t  char_val[BLE_SAMPLE_DATA_LENGTH];
} ble_sample_srv_data_t;

/* BLE sample server attribute database handle list */
enum ble_sample_srv_att_idx
{
    BLE_SAMPLE_SRV_IDX_SVC,                     /*!< BLE Sample Server Service Declaration */

    BLE_SAMPLE_SRV_IDX_READ_HANDLE_CHAR,       /*!< BLE Sample Server Service Read Characteristic Declaration */
    BLE_SAMPLE_SRV_IDX_READ_HANDLE_VAL,        /*!< BLE Sample Server Service Read Characteristic value */
    BLE_SAMPLE_SRV_IDX_WRITE_HANDLE_CHAR,      /*!< BLE Sample Server Service Write Characteristic Declaration */
    BLE_SAMPLE_SRV_IDX_WRITE_HANDLE_VAL,       /*!< BLE Sample Server Service Write Characteristic value */
    BLE_SAMPLE_SRV_IDX_NOTIFY_HANDLE_CHAR,     /*!< BLE Sample Server Service Notify Characteristic Declaration */
    BLE_SAMPLE_SRV_IDX_NOTIFY_HANDLE_VAL,      /*!< BLE Sample Server Service Notify Characteristic value */
    BLE_SAMPLE_SRV_IDX_NOTIFY_HANDLE_CCCD_CFG, /*!< BLE Sample Server Service Notify Client Characteristic Configuration Descriptor */
#if STORAGE_FEAT_SUPPORT
    BLE_SAMPLE_SRV_IDX_STOREGE_HANDLE_CHAR,    /*!< BLE Sample Server Service Storage Characteristic Declaration */
    BLE_SAMPLE_SRV_IDX_STOREGE_HANDLE_VAL,     /*!< BLE Sample Server Service Storage Characteristic value */
    BLE_SAMPLE_SRV_IDX_STOREGE_HANDLE_CCCD_CFG,/*!< BLE Sample Server Service Storage Client Characteristic Configuration Descriptor */
#endif
    BLE_SAMPLE_SRV_IDX_NB,
};

/* BLE sample server service ID assigned by GATT server module */
static uint8_t svc_id;

/* Sample server data*/
static ble_sample_srv_data_t ble_sample_srv_data[BLE_PEER_NUM_MAX];

#if STORAGE_FEAT_SUPPORT
/* Id for storage */
static uint16_t data_id = 0x1111;
/* Storage sample data*/
static ble_sample_srv_data_t ble_sample_srv_storage_data[BLE_PEER_NUM_MAX];
/* Storage sample data default value */
uint8_t storage_buf[BLE_SAMPLE_DATA_LENGTH] = {0x11, 0x11};
#endif

/* BLE sample server service UUID array */
const uint8_t ble_sample_srv_svc_uuid[BLE_GATT_UUID_128_LEN] = UUID_BLE_SAMPLE_SRV_SERVICE_128;

/* BLE sample server service Database Description */
const ble_gatt_attr_desc_t ble_sample_srv_att_db[BLE_SAMPLE_SRV_IDX_NB] = {

    [BLE_SAMPLE_SRV_IDX_SVC]                     = { UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_PRIMARY_SERVICE), PROP(RD)                                        , 0                                             },

    [BLE_SAMPLE_SRV_IDX_READ_HANDLE_CHAR]        = { UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC) , PROP(RD)                                        , 0                                             },

    [BLE_SAMPLE_SRV_IDX_READ_HANDLE_VAL]         = { UUID_BLE_SAMPLE_SRV_READ_HANDLE_128               , PROP(RD) | ATT_UUID(128)                        , OPT(NO_OFFSET) | BLE_SAMPLE_DATA_LENGTH       },

    [BLE_SAMPLE_SRV_IDX_WRITE_HANDLE_CHAR]       = { UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC) , PROP(RD)                                        , 0                                             },

    [BLE_SAMPLE_SRV_IDX_WRITE_HANDLE_VAL]        = { UUID_BLE_SAMPLE_SRV_WRITE_HANDLE_128              , PROP(WR) | PROP(WC) | ATT_UUID(128)             , OPT(NO_OFFSET) | BLE_SAMPLE_SRV_WRITE_MAX_LEN },

    [BLE_SAMPLE_SRV_IDX_NOTIFY_HANDLE_CHAR]      = { UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC) , PROP(RD)                                        , 0                                             },

    [BLE_SAMPLE_SRV_IDX_NOTIFY_HANDLE_VAL]       = { UUID_BLE_SAMPLE_SRV_NOTIFY_HANDLE_128             , PROP(NTF) | PROP(IND) | ATT_UUID(128)           , OPT(NO_OFFSET) | BLE_SAMPLE_SRV_WRITE_MAX_LEN },

    [BLE_SAMPLE_SRV_IDX_NOTIFY_HANDLE_CCCD_CFG]  = { UUID_16BIT_TO_ARRAY(BLE_GATT_DESC_CLIENT_CHAR_CFG), PROP(RD) | PROP(WR)                             , OPT(NO_OFFSET)                                },

#if STORAGE_FEAT_SUPPORT
    [BLE_SAMPLE_SRV_IDX_STOREGE_HANDLE_CHAR]     = { UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC) , PROP(RD)                                        , 0                                             },

    [BLE_SAMPLE_SRV_IDX_STOREGE_HANDLE_VAL]      = { UUID_BLE_SAMPLE_SRV_STORAGE_HANDLE_128            , PROP(NTF) | PROP(RD) | PROP(WR) | ATT_UUID(128) , OPT(NO_OFFSET) | BLE_SAMPLE_DATA_LENGTH       },

    [BLE_SAMPLE_SRV_IDX_STOREGE_HANDLE_CCCD_CFG] = { UUID_16BIT_TO_ARRAY(BLE_GATT_DESC_CLIENT_CHAR_CFG), PROP(RD) | PROP(WR)                             , OPT(NO_OFFSET)                                },
#endif
};

/*!
    \brief      Handle BLE connection disconnected event
    \param[in]  conn_idx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_sample_srv_on_disconnect(uint8_t conn_idx, uint16_t reason)
{
    dbg_print(INFO, "ble sample srv disconnect, conn_idx %d, reason 0x%x\r\n", conn_idx, reason);

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Handle BLE connection connected event
    \param[in]  conn_idx: connection index
    \param[in]  p_addr: pointer to peer address information
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_sample_srv_on_connect(uint8_t conn_idx, ble_gap_addr_t *p_addr)
{
#if STORAGE_FEAT_SUPPORT
    void *p_data = NULL;
    uint32_t data_length = 0;
#endif

    dbg_print(INFO, "ble sample srv connect, conn_idx %d\r\n", conn_idx);

    ble_sample_srv_data[conn_idx].cccd_value = 0;
    memset(ble_sample_srv_data[conn_idx].char_val, 0, BLE_SAMPLE_DATA_LENGTH);

#if STORAGE_FEAT_SUPPORT
    if (!ble_svc_data_load(conn_idx, data_id, (void **)&p_data, &data_length)) {
        if (data_length == sizeof(ble_sample_srv_data_t)) {
            memcpy(&ble_sample_srv_storage_data[conn_idx], p_data, data_length);
        }
    } else {
        memset(&ble_sample_srv_storage_data[conn_idx], 0, sizeof(ble_sample_srv_data_t));
        memcpy(ble_sample_srv_storage_data[conn_idx].char_val, storage_buf, BLE_SAMPLE_DATA_LENGTH);
    }
#endif

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle gatts read event
    \param[in]  conn_idx: connection index
    \param[in]  p_req: pointer to gatts read request information data
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_sample_srv_read_cb(uint8_t conn_idx, ble_gatts_read_req_t *p_req)
{
    switch (p_req->att_idx) {
    case BLE_SAMPLE_SRV_IDX_READ_HANDLE_VAL: {
        uint8_t read_buf[BLE_SAMPLE_DATA_LENGTH] = {0x41, 0x41};

        p_req->val_len = BLE_SAMPLE_DATA_LENGTH;
        p_req->att_len = BLE_SAMPLE_DATA_LENGTH;
        memcpy(p_req->p_val, read_buf, p_req->val_len);
    } break;

    case BLE_SAMPLE_SRV_IDX_NOTIFY_HANDLE_CCCD_CFG:
        p_req->val_len = BLE_GATT_CCCD_LEN;
        p_req->att_len = BLE_GATT_CCCD_LEN;
        memcpy(p_req->p_val, &ble_sample_srv_data[conn_idx].cccd_value, p_req->val_len);
        break;

#if STORAGE_FEAT_SUPPORT
    case BLE_SAMPLE_SRV_IDX_STOREGE_HANDLE_VAL:
        p_req->val_len = BLE_SAMPLE_DATA_LENGTH;
        p_req->att_len = BLE_SAMPLE_DATA_LENGTH;
        memcpy(p_req->p_val, &ble_sample_srv_storage_data[conn_idx].char_val, p_req->val_len);
        break;

    case BLE_SAMPLE_SRV_IDX_STOREGE_HANDLE_CCCD_CFG:
        p_req->val_len = BLE_GATT_CCCD_LEN;
        p_req->att_len = BLE_GATT_CCCD_LEN;
        memcpy(p_req->p_val, &ble_sample_srv_storage_data[conn_idx].cccd_value, p_req->val_len);
        break;
#endif

    default:
        break;
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle gatts write event
    \param[in]  conn_idx: connection index
    \param[in]  p_req: pointer to gatts write request information data
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_sample_srv_write_cb(uint8_t conn_idx, ble_gatts_write_req_t *p_req)
{
    switch (p_req->att_idx) {
    case BLE_SAMPLE_SRV_IDX_WRITE_HANDLE_VAL: {
        uint16_t i = 0;

        dbg_print(NOTICE, "ble sample srv write value len: %d, data: ",p_req->val_len);
        for (i = 0; i < p_req->val_len; i++) {
            dbg_print(NOTICE, "0x%02x ", p_req->p_val[i]);
        }
        dbg_print(NOTICE, "\r\n");
    } break;

    case BLE_SAMPLE_SRV_IDX_NOTIFY_HANDLE_CCCD_CFG:
        if (p_req->val_len == BLE_GATT_CCCD_LEN) {
            dbg_print(NOTICE, "ble sample srv write cccd value: 0x%x\r\n", *p_req->p_val);
            memcpy(&ble_sample_srv_data[conn_idx].cccd_value, p_req->p_val, p_req->val_len);
        }
        break;

#if STORAGE_FEAT_SUPPORT
    case BLE_SAMPLE_SRV_IDX_STOREGE_HANDLE_VAL:
        if (p_req->val_len == BLE_SAMPLE_DATA_LENGTH) {
            memcpy(ble_sample_srv_storage_data[conn_idx].char_val, p_req->p_val,p_req->val_len);
            ble_svc_data_save(conn_idx, data_id, sizeof(ble_sample_srv_data_t),
                              (uint8_t *)&ble_sample_srv_storage_data[conn_idx]);
        }
        break;

    case BLE_SAMPLE_SRV_IDX_STOREGE_HANDLE_CCCD_CFG:
        if (p_req->val_len == BLE_GATT_CCCD_LEN) {
            memcpy(&ble_sample_srv_storage_data[conn_idx].cccd_value, p_req->p_val,p_req->val_len);
            ble_svc_data_save(conn_idx, data_id, sizeof(ble_sample_srv_data_t),
                              (uint8_t *)&ble_sample_srv_storage_data[conn_idx]);
        }
        break;
#endif

    default:
        break;
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle server notification/indication send event
    \param[in]  conn_idx: connection index
    \param[in]  p_rsp: pointer to gattc notification/indication information data
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_sample_srv_ntf_ind_send_cb(uint8_t conn_idx, ble_gatts_ntf_ind_send_rsp_t *p_rsp)
{
    switch (p_rsp->att_idx) {
    case BLE_SAMPLE_SRV_IDX_NOTIFY_HANDLE_VAL:
        dbg_print(NOTICE, "ble sample srv ntf send rsp status 0x%x, conn idx %d, att idx %d\r\n",
                  p_rsp->status, conn_idx, p_rsp->att_idx);
        break;

#if STORAGE_FEAT_SUPPORT
    case BLE_SAMPLE_SRV_IDX_STOREGE_HANDLE_VAL:
        dbg_print(NOTICE, "ble sample srv storage ntf send rsp status 0x%x, conn idx %d, att idx %d\r\n",
                  p_rsp->status, conn_idx, p_rsp->att_idx);
        break;
#endif

    default:
        break;
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle mtu information event
    \param[in]  conn_idx: connection index
    \param[in]  p_info: pointer to mtu information data
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_sample_srv_mtu_info_cb(uint8_t conn_idx, ble_gatts_mtu_info_t *p_info)
{
    dbg_print(NOTICE, "ble sample srv mtu info, conn_idx %d, mtu size %d\r\n", conn_idx, p_info->mtu);

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle BLE_SRV_EVT_GATT_OPERATION event
    \param[in]  p_info: pointer to gatts operation information data
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_sample_srv_handle_gatts_op(ble_gatts_op_info_t *p_info)
{
    uint8_t conn_idx = p_info->conn_idx;

    switch (p_info->gatts_op_sub_evt) {
    case BLE_SRV_EVT_READ_REQ:
        ble_sample_srv_read_cb(conn_idx, &p_info->gatts_op_data.read_req);
        break;

    case BLE_SRV_EVT_WRITE_REQ:
        ble_sample_srv_write_cb(conn_idx, &p_info->gatts_op_data.write_req);
        break;

    case BLE_SRV_EVT_NTF_IND_SEND_RSP:
        ble_sample_srv_ntf_ind_send_cb(conn_idx, &p_info->gatts_op_data.ntf_ind_send_rsp);
        break;

    case BLE_SRV_EVT_MTU_INFO:
        ble_sample_srv_mtu_info_cb(conn_idx, &p_info->gatts_op_data.mtu_info);
        break;

    default:
        break;
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle GATT server messages
    \param[in]  p_srv_msg_info: pointer to GATT server message information
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_sample_srv_cb(ble_gatts_msg_info_t *p_srv_msg_info)
{
    switch (p_srv_msg_info->srv_msg_type) {
    case BLE_SRV_EVT_SVC_ADD_RSP:
        dbg_print(INFO, "ble sample srv svc add rsp status 0x%x\r\n",
                  p_srv_msg_info->msg_data.svc_add_rsp.status);
        break;

    case BLE_SRV_EVT_SVC_RMV_RSP:
        dbg_print(INFO, "ble sample srv svc rmv rsp status 0x%x\r\n",
                  p_srv_msg_info->msg_data.svc_rmv_rsp.status);
        break;

    case BLE_SRV_EVT_CONN_STATE_CHANGE_IND: {
        ble_gatts_conn_state_change_ind_t * p_ind = &p_srv_msg_info->msg_data.conn_state_change_ind;

        if (p_ind->conn_state == BLE_CONN_STATE_DISCONNECTD) {
            ble_sample_srv_on_disconnect(p_ind->info.disconn_info.conn_idx, p_ind->info.disconn_info.reason);
        } else if (p_ind->conn_state == BLE_CONN_STATE_CONNECTED) {
            ble_sample_srv_on_connect(p_ind->info.conn_info.conn_idx, &p_ind->info.conn_info.peer_addr);
        }
    } break;

    case BLE_SRV_EVT_GATT_OPERATION:
        ble_sample_srv_handle_gatts_op(&p_srv_msg_info->msg_data.gatts_op_info);
        break;

    default:
        break;
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Add BLE sample server service to GATT server module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_sample_srv_add_prf(void)
{
    ble_gatts_svc_add(&svc_id, ble_sample_srv_svc_uuid, 0, SVC_UUID(128),
                      ble_sample_srv_att_db, BLE_SAMPLE_SRV_IDX_NB, ble_sample_srv_cb);
}

/*!
    \brief      Remove BLE sample server service from GATT server module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_sample_srv_rmv_prf(void)
{
    ble_gatts_svc_rmv(svc_id);
}

/*!
    \brief      Send notify/indicate
    \param[in]  conn_idx: connection index
    \param[in]  len: data length
    \param[in]  p_data: pointer to data to send
    \param[out] none
    \retval     none
*/
void ble_sample_srv_ntf_send(uint8_t conn_idx, uint16_t len, uint8_t *p_data)
{
    ble_status_t status = BLE_ERR_NO_ERROR;

    if (p_data == NULL)
        return;

    if ((ble_sample_srv_data[conn_idx].cccd_value & BLE_GATT_CCCD_NTF_BIT) == 0) {
        dbg_print(NOTICE, "ble sample srv ntf not enabled!!!\r\n");
        return;
    }

    status = ble_gatts_ntf_ind_send(conn_idx, svc_id, BLE_SAMPLE_SRV_IDX_NOTIFY_HANDLE_VAL, p_data, len, BLE_GATT_NOTIFY);
    if (BLE_ERR_NO_ERROR != status)
        dbg_print(NOTICE, "ble sample srv ntf send fail, status 0x%02x\r\n", status);
}

/*!
    \brief      Storage characteristic send notify/indicate
    \param[in]  conn_idx: connection index
    \param[out] none
    \retval     none
*/
void ble_sample_srv_storage_char_ntf_send(uint8_t conn_idx)
{
#if STORAGE_FEAT_SUPPORT
    if (ble_sample_srv_storage_data[conn_idx].cccd_value & BLE_GATT_CCCD_NTF_BIT) {
        ble_gatts_ntf_ind_send(conn_idx, svc_id, BLE_SAMPLE_SRV_IDX_STOREGE_HANDLE_VAL,
                               ble_sample_srv_storage_data[conn_idx].char_val, BLE_SAMPLE_DATA_LENGTH, BLE_GATT_NOTIFY);
    } else {
        dbg_print(NOTICE, "ble sample srv storage ntf not enabled!!!\r\n");
    }
#endif
}

/*!
    \brief      Init BLE sample server
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_sample_srv_init(void)
{
    ble_sample_srv_add_prf();
}

/*!
    \brief      Deinit BLE sample server
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_sample_srv_deinit(void)
{
    ble_sample_srv_rmv_prf();
}
