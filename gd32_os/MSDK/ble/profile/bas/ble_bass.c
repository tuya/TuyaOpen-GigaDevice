/*!
    \file    ble_bass.c
    \brief   Implementation of Battery Service Server.

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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "ble_utils.h"
#include "ble_gatt.h"
#include "ble_gatts.h"
#include "ble_bass.h"
#include "dbg_print.h"
#include "ble_config.h"

/* Max connection number for BASS */
#define BLE_BASS_MAX_CONN_NUM           BLE_MAX_CONN_NUM

/* Max battery level value */
#define BLE_BASS_BATT_LVL_MAX           100

/* Battery level value */
#define BLE_BASS_BATT_LVL_LEN           1

/* BAS attribute database handle list */
typedef enum
{
    BLE_BAS_HDL_SVC,                    /*!< Battery Service Declaration */

    BLE_BAS_HDL_BATT_LVL_CHAR,          /*!< Battery Level Characteristic Declaration */
    BLE_BAS_HDL_BATT_LVL_VAL,           /*!< Battery Level Characteristic Value */
    BLE_BAS_HDL_BATT_LVL_NTF_CFG,       /*!< Battery Level Characteristic Client Characteristic Configuration Descriptor */

    BLE_BAS_HDL_NB,
} ble_bass_attr_db_handle_t;

/* Structure of battery service information by connection */
typedef struct
{
    uint16_t        cccd;           /*!< Client Characteristic Configuration Descriptor value */
} ble_bass_conn_t;

/* Battery service server environment variable structure */
typedef struct
{
    uint8_t             svc_id;                             /*!< Service ID assigned by BLE server module */
    uint8_t             batt_lvl;                           /*!< Battery level value */
    ble_bass_conn_t     bass_conn[BLE_BASS_MAX_CONN_NUM];   /*!< Connection related information */
} ble_bass_env_t;

/* Battery service server environment value */
static ble_bass_env_t ble_bass_env;

/* BAS UUID 16bits array */
const uint8_t ble_bas_uuid[BLE_GATT_UUID_16_LEN] = UUID_16BIT_TO_ARRAY(BLE_GATT_SVC_BATTERY_SERVICE);

/* BAS Database Description */
const ble_gatt_attr_desc_t ble_bass_attr_db[BLE_BAS_HDL_NB] =
{
    [BLE_BAS_HDL_SVC]               = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_PRIMARY_SERVICE),  PROP(RD),           0             },

    [BLE_BAS_HDL_BATT_LVL_CHAR]     = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),   PROP(RD),           0             },
    [BLE_BAS_HDL_BATT_LVL_VAL]      = {UUID_16BIT_TO_ARRAY(BLE_GATT_CHAR_BATTERY_LEVEL),    PROP(RD)|PROP(NTF), OPT(NO_OFFSET) | BLE_BASS_BATT_LVL_LEN},
    [BLE_BAS_HDL_BATT_LVL_NTF_CFG]  = {UUID_16BIT_TO_ARRAY(BLE_GATT_DESC_CLIENT_CHAR_CFG),  PROP(RD)|PROP(WR),  OPT(NO_OFFSET)},
};

/*!
    \brief      Handle BLE connection connected event
    \param[in]  conn_idx: connection index
    \param[in]  p_addr: pointer to peer address information
    \param[out] none
    \retval     none
*/
static void ble_bass_on_connect(uint8_t conn_idx, ble_gap_addr_t *p_addr)
{
    if (conn_idx < BLE_BASS_MAX_CONN_NUM) {
        ///TODO: try to load CCD from flash if bonded before

        if (ble_bass_env.bass_conn[conn_idx].cccd & BLE_GATT_CCCD_NTF_BIT) {
            ble_gatts_ntf_ind_send(conn_idx, ble_bass_env.svc_id, BLE_BAS_HDL_BATT_LVL_VAL,
                                   &ble_bass_env.batt_lvl, BLE_BASS_BATT_LVL_LEN, BLE_GATT_NOTIFY);
        }
    }
}

/*!
    \brief      Handle BLE connection disconnected event
    \param[in]  conn_idx: connection index
    \param[out] none
    \retval     none
*/
static void ble_bass_on_disconnect(uint8_t conn_idx)
{
    if (conn_idx < BLE_BASS_MAX_CONN_NUM) {
        ble_bass_env.bass_conn[conn_idx].cccd = 0;
    }
}

/*!
    \brief      Callback function to handle gatts read event
    \param[in]  conn_idx: connection index
    \param[in]  p_req: pointer to gatts read request information data
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_bass_gatts_read_cb(uint8_t conn_idx, ble_gatts_read_req_t *p_req)
{
    uint8_t attr_idx = p_req->att_idx + BLE_BAS_HDL_SVC;
    uint8_t attr_val[2];
    uint16_t attr_len = 0;

    if (conn_idx >= BLE_BASS_MAX_CONN_NUM) {
        return BLE_ATT_ERR_APP_ERROR;
    }

    switch (attr_idx) {
    case BLE_BAS_HDL_BATT_LVL_VAL:
        attr_val[0] = ble_bass_env.batt_lvl;
        attr_len = BLE_BASS_BATT_LVL_LEN;
        break;

    case BLE_BAS_HDL_BATT_LVL_NTF_CFG:
        ble_write16p(attr_val, ble_bass_env.bass_conn[conn_idx].cccd);
        attr_len = BLE_GATT_CCCD_LEN;
        break;

    default:
        return BLE_ATT_ERR_INVALID_HANDLE;
    }

    p_req->val_len = ble_min(p_req->max_len, attr_len);
    memcpy(p_req->p_val, attr_val, p_req->val_len);

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle gatts write event
    \param[in]  conn_idx: connection index
    \param[in]  p_req: pointer to gatts write request information data
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_bass_gatts_write_cb(uint8_t conn_idx, ble_gatts_write_req_t *p_req)
{
    uint8_t attr_idx = p_req->att_idx + BLE_BAS_HDL_SVC;

    if (conn_idx >= BLE_BASS_MAX_CONN_NUM) {
        return BLE_ATT_ERR_APP_ERROR;
    }

    if (attr_idx != BLE_BAS_HDL_BATT_LVL_NTF_CFG) {
        return BLE_ATT_ERR_INVALID_HANDLE;
    }

    if (p_req->val_len != BLE_GATT_CCCD_LEN) {
        return BLE_ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
    }

    ble_bass_env.bass_conn[conn_idx].cccd = ble_read16p(p_req->p_val);
    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle BLE_SRV_EVT_GATT_OPERATION event
    \param[in]  p_info: pointer to gatts operation information data
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_bass_handle_gatts_op(ble_gatts_op_info_t *p_info)
{
    ble_status_t ret = BLE_ERR_NO_ERROR;

    switch (p_info->gatts_op_sub_evt) {
    case BLE_SRV_EVT_READ_REQ:
        ret = ble_bass_gatts_read_cb(p_info->conn_idx, &p_info->gatts_op_data.read_req);
        break;

    case BLE_SRV_EVT_WRITE_REQ:
        ret = ble_bass_gatts_write_cb(p_info->conn_idx, &p_info->gatts_op_data.write_req);
        break;

    case BLE_SRV_EVT_NTF_IND_SEND_RSP:
        if (p_info->gatts_op_data.ntf_ind_send_rsp.status != BLE_ERR_NO_ERROR) {
            dbg_print(WARNING, "bass ntf send fail, status 0x%x, conn_idx  0x%x, att_idx  %d\r\n",
                      p_info->gatts_op_data.ntf_ind_send_rsp.status, p_info->conn_idx,
                      p_info->gatts_op_data.ntf_ind_send_rsp.att_idx);
        }
        break;

    case BLE_SRV_EVT_NTF_IND_MTP_SEND_RSP:
        if (p_info->gatts_op_data.ntf_ind_mtp_send_rsp.status != BLE_ERR_NO_ERROR) {
            dbg_print(WARNING, "bass ntf send fail, status 0x%x, conn_idx  0x%x, att_idx  %d\r\n",
                      p_info->gatts_op_data.ntf_ind_mtp_send_rsp.status, p_info->conn_idx,
                      p_info->gatts_op_data.ntf_ind_mtp_send_rsp.att_idx);
        }
        break;

    default:
        break;
    }

    return ret;
}

/*!
    \brief      Callback function to handle GATT server message
    \param[in]  p_info: pointer to GATT server message information
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_bass_gatts_cb(ble_gatts_msg_info_t *p_info)
{
    ble_status_t ret = BLE_ERR_NO_ERROR;

    switch (p_info->srv_msg_type) {
    case BLE_SRV_EVT_SVC_ADD_RSP:
        if ((p_info->msg_data.svc_add_rsp.svc_id == ble_bass_env.svc_id) &&
            (p_info->msg_data.svc_add_rsp.status != BLE_ERR_NO_ERROR)) {
            dbg_print(WARNING, "bass svc add fail, status = 0x%x\r\n", p_info->msg_data.svc_add_rsp.status);
        }
        break;

    case BLE_SRV_EVT_SVC_RMV_RSP:
        if ((p_info->msg_data.svc_rmv_rsp.svc_id == ble_bass_env.svc_id) &&
            (p_info->msg_data.svc_rmv_rsp.status != BLE_ERR_NO_ERROR)) {
            dbg_print(WARNING, "bass svc remove fail, status = 0x%x\r\n", p_info->msg_data.svc_rmv_rsp.status);
        }
        break;

    case BLE_SRV_EVT_CONN_STATE_CHANGE_IND:
        if (p_info->msg_data.conn_state_change_ind.conn_state == BLE_CONN_STATE_DISCONNECTD) {
            ble_bass_on_disconnect(p_info->msg_data.conn_state_change_ind.info.disconn_info.conn_idx);
        } else if (p_info->msg_data.conn_state_change_ind.conn_state == BLE_CONN_STATE_CONNECTED) {
            ble_bass_on_connect(p_info->msg_data.conn_state_change_ind.info.conn_info.conn_idx,
                                &p_info->msg_data.conn_state_change_ind.info.conn_info.peer_addr);
        }
        break;

    case BLE_SRV_EVT_GATT_OPERATION:
        ret = ble_bass_handle_gatts_op(&p_info->msg_data.gatts_op_info);
        break;

    default:
        break;
    }

    return ret;
}

/*!
    \brief      Init Battery Service Server
    \param[in]  p_param: pointer to Battery Service init parameters
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_bass_init(ble_bass_init_param_t *p_param)
{
    uint8_t i;
    ble_status_t ret = BLE_ERR_NO_ERROR;

    ret = ble_gatts_svc_add(&ble_bass_env.svc_id, ble_bas_uuid, 0,
                            SVC_UUID(16) | SVC_SEC_LVL_VAL(p_param->sec_lvl),
                            ble_bass_attr_db, BLE_BAS_HDL_NB, ble_bass_gatts_cb);
    if (ret != BLE_ERR_NO_ERROR) {
        return ret;
    }

    ble_bass_env.batt_lvl = p_param->batt_lvl;
    for (i = 0; i< BLE_BASS_MAX_CONN_NUM; i++) {
        ble_bass_env.bass_conn[i].cccd = 0;
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Update battery level, notifications will be sent to remote devices if configured enable
    \param[in]  batt_lvl: battery level value, vaild from 0 to 100
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_bass_batt_lvl_update(uint8_t batt_lvl)
{
    uint8_t i;
    uint32_t conidx_bf = 0;

    if (batt_lvl > BLE_BASS_BATT_LVL_MAX) {
        batt_lvl = BLE_BASS_BATT_LVL_MAX;
    }

    ble_bass_env.batt_lvl = batt_lvl;

    for (i = 0; i < BLE_BASS_MAX_CONN_NUM; i++) {
        if (ble_bass_env.bass_conn[i].cccd & BLE_GATT_CCCD_NTF_BIT) {
            conidx_bf |= BIT(i);
        }
    }

    return ble_gatts_ntf_ind_mtp_send(conidx_bf, ble_bass_env.svc_id, BLE_BAS_HDL_BATT_LVL_VAL,
                                      &ble_bass_env.batt_lvl, 1, BLE_GATT_NOTIFY);
}

/*!
    \brief      Deinit Battery Service Server
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_bass_deinit(void)
{
    return ble_gatts_svc_rmv(ble_bass_env.svc_id);
}

