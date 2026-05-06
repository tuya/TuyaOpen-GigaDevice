/*!
    \file    ble_datatrans_srv.c
    \brief   Implementations of ble datatrans server

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
#include "ble_datatrans_srv.h"
#include "ble_datatrans_common.h"
#include "ble_gap.h"
#include "ble_gatt.h"
#include "ble_gatts.h"
#include "ble_error.h"
#include "dbg_print.h"
#include "ble_utils.h"
#include "wrapper_os.h"

/* Max length that BLE datatrans server Characteristic value can be written */
#define BLE_DATATRANS_SRV_WRITE_MAX_LEN         512

/* BLE datatrans server data receive callback function */
static ble_datatrans_srv_rx_cb datatrans_srv_rx_cb = NULL;

/* BLE datatrans server attribute database handle list */
enum ble_datatrans_srv_att_idx
{
    BLE_DATATRANS_SRV_IDX_SVC,                  /*!< BLE Datatrans Server Service Declaration */
    BLE_DATATRANS_SRV_IDX_RX_HANDLE_CHAR,       /*!< RX Characteristic Declaration */
    BLE_DATATRANS_SRV_IDX_RX_HANDLE_VAL,        /*!< RX Characteristic value */
    BLE_DATATRANS_SRV_IDX_TX_HANDLE_CHAR,       /*!< TX Characteristic Declaration */
    BLE_DATATRANS_SRV_IDX_TX_HANDLE_VAL,        /*!< TX Characteristic value */
    BLE_DATATRANS_SRV_IDX_TX_HANDLE_CCCD_CFG,   /*!< TX Characteristic cccd */
    BLE_DATATRANS_SRV_IDX_NB,
};

/* BLE datatrans server service ID assigned by GATT server module */
static uint8_t svc_id;

/* BLE datatrans server service UUID array */
const uint8_t ble_datatrans_srv_svc_uuid[BLE_GATT_UUID_16_LEN] = UUID_16BIT_TO_ARRAY(BLE_GATT_SVC_DATATRANS_SERVICE);

/* BLE datatrans server service Database Description */
const ble_gatt_attr_desc_t ble_datatrans_srv_att_db[BLE_DATATRANS_SRV_IDX_NB] = {

    [BLE_DATATRANS_SRV_IDX_SVC]                 = { UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_PRIMARY_SERVICE)  , PROP(RD)            , 0                                                },

    [BLE_DATATRANS_SRV_IDX_RX_HANDLE_CHAR]      = { UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC)   , PROP(RD)            , 0                                                },

    [BLE_DATATRANS_SRV_IDX_RX_HANDLE_VAL]       = { UUID_16BIT_TO_ARRAY(BLE_GATT_SVC_DATATRANS_RX_CHAR) , PROP(WR) | PROP(WC) , OPT(NO_OFFSET) | BLE_DATATRANS_SRV_WRITE_MAX_LEN },

    [BLE_DATATRANS_SRV_IDX_TX_HANDLE_CHAR]      = { UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC)   , PROP(RD)            , 0                                                },

    [BLE_DATATRANS_SRV_IDX_TX_HANDLE_VAL]       = { UUID_16BIT_TO_ARRAY(BLE_GATT_SVC_DATATRANS_TX_CHAR) , PROP(NTF)           , OPT(NO_OFFSET)                                   },

    [BLE_DATATRANS_SRV_IDX_TX_HANDLE_CCCD_CFG]  = { UUID_16BIT_TO_ARRAY(BLE_GATT_DESC_CLIENT_CHAR_CFG)  , PROP(RD) | PROP(WR) , OPT(NO_OFFSET)                                   },

};

/*!
    \brief      Callback function to handle GATT server messages
    \param[in]  p_srv_msg_info: pointer to GATT server message information
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_srv_cb(ble_gatts_msg_info_t *p_srv_msg_info)
{
    uint8_t status = BLE_ERR_NO_ERROR;

    switch (p_srv_msg_info->srv_msg_type) {
    case BLE_SRV_EVT_SVC_ADD_RSP:
        dbg_print(NOTICE, "[ble_datatrans_srv_cb], svc_add_rsp status = 0x%x\r\n",
                  p_srv_msg_info->msg_data.svc_add_rsp.status);
        break;

    case BLE_SRV_EVT_SVC_RMV_RSP:
        dbg_print(NOTICE, "[ble_datatrans_srv_cb], svc_rmv_rsp status = 0x%x\r\n",
                  p_srv_msg_info->msg_data.svc_rmv_rsp.status);
        break;

    case BLE_SRV_EVT_GATT_OPERATION: {
        uint8_t conn_idx = p_srv_msg_info->msg_data.gatts_op_info.conn_idx;

        if (p_srv_msg_info->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_NTF_IND_SEND_RSP) {
            ble_gatts_ntf_ind_send_rsp_t *p_rsp = &p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.ntf_ind_send_rsp;

            if (p_rsp->att_idx == BLE_DATATRANS_SRV_IDX_TX_HANDLE_VAL) {
                //add user code here
            }
        } else if (p_srv_msg_info->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_WRITE_REQ) {
            ble_gatts_write_req_t *p_req = &p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.write_req;

            if (p_req->att_idx == BLE_DATATRANS_SRV_IDX_RX_HANDLE_VAL) {
                if (datatrans_srv_rx_cb)
                    datatrans_srv_rx_cb(p_srv_msg_info->msg_data.gatts_op_info.conn_idx, p_req->val_len, p_req->p_val);
            } else if (p_req->att_idx == BLE_DATATRANS_SRV_IDX_TX_HANDLE_CCCD_CFG) {
                dbg_print(NOTICE, "[ble_datatrans_srv_cb], write CCCD to 0x%x\r\n",
                          *((uint16_t *)p_req->p_val));
            }
        } else if (p_srv_msg_info->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_READ_REQ) {
            ble_gatts_read_req_t * p_req = &p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.read_req;

            if (p_req->att_idx == BLE_DATATRANS_SRV_IDX_TX_HANDLE_CCCD_CFG) {
                p_req->val_len = BLE_GATT_CCCD_LEN;
                p_req->att_len = BLE_GATT_CCCD_LEN;
                memset(p_req->p_val, 0, p_req->val_len);
            }
        }
    } break;

    default:
        break;
    }

    return status;
}

/*!
    \brief      BLE datatrans server service rx callback register
    \param[in]  callback: datatrans server callback function
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_srv_rx_cb_reg(ble_datatrans_srv_rx_cb callback)
{
    datatrans_srv_rx_cb = callback;

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      BLE datatrans server service rx callback unregister
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_srv_rx_cb_unreg(void)
{
    datatrans_srv_rx_cb = NULL;

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Init BLE datatrans server service
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_srv_init(void)
{
    return ble_gatts_svc_add(&svc_id, ble_datatrans_srv_svc_uuid, 0, SVC_UUID(16),
                             ble_datatrans_srv_att_db, BLE_DATATRANS_SRV_IDX_NB,
                             ble_datatrans_srv_cb);
}

/*!
    \brief      Deinit BLE datatrans server service
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_srv_deinit(void)
{
    return ble_gatts_svc_rmv(svc_id);
}

/*!
    \brief      BLE datatrans server transmit data to client
    \param[in]  conn_idx: connection index
    \param[in]  p_buf: pointer to transmit data buffer
    \param[in]  len: transmit data length
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_srv_tx(uint8_t conn_idx, uint8_t *p_buf, uint16_t len)
{
    return ble_gatts_ntf_ind_send(conn_idx, svc_id, BLE_DATATRANS_SRV_IDX_TX_HANDLE_VAL,
                                  p_buf, len, BLE_GATT_NOTIFY);
}

/*!
    \brief      BLE datatrans server multiple transmit data to client
    \param[in]  conidx_bf: connection index bit field
    \param[in]  p_buf: pointer to transmit data buffer
    \param[in]  len: transmit data length
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_srv_tx_mtp(uint32_t conidx_bf, uint8_t *p_buf, uint16_t len)
{
    return ble_gatts_ntf_ind_mtp_send(conidx_bf, svc_id, BLE_DATATRANS_SRV_IDX_TX_HANDLE_VAL,
                                  p_buf, len, BLE_GATT_NOTIFY);
}


