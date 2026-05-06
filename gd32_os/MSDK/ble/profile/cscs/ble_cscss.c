/*!
    \file    ble_cscss.c
    \brief   Implementation of Cycling Speed and Cadence Service Server.

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

#include "ble_config.h"
#include "ble_utils.h"
#include "ble_gap.h"
#include "ble_gatt.h"
#include "ble_gatts.h"
#include "ble_cscss.h"
#include "dbg_print.h"

/* Max connection number for CSCSS */
#define BLE_CSCSS_MAX_CONN_NUM              BLE_MAX_CONN_NUM

/* CSC feature value length */
#define BLE_CSCSS_CSC_FEAT_VAL_LEN          2

/* Sensor location value length */
#define BLE_CSCSS_SENSOR_LOC_VAL_LEN        1

/* Cycling Speed and Cadence Service attribute database handle list */
typedef enum
{
    BLE_CSCS_HDL_SVC,                /*!< Cycling Speed and Cadence Service Declaration */

    BLE_CSCS_HDL_CSC_MEAS_CHAR,      /*!< CSC Measurement Characteristic Declaration */
    BLE_CSCS_HDL_CSC_MEAS_VAL,       /*!< CSC Measurement Characteristic Value */
    BLE_CSCS_HDL_CSC_MEAS_NTF_CFG,   /*!< CSC Measurement Characteristic Client Characteristic Configuration Descriptor */

    BLE_CSCS_HDL_CSC_FEAT_CHAR,      /*!< CSC Feature Characteristic Declaration */
    BLE_CSCS_HDL_CSC_FEAT_VAL,       /*!< CSC Feature Characteristic Value */

    BLE_CSCS_HDL_SENSOR_LOC_CHAR,    /*!< Sensor Location Characteristic Declaration */
    BLE_CSCS_HDL_SENSOR_LOC_VAL,     /*!< Sensor Location Characteristic Value */

    BLE_CSCS_HDL_SC_CTRL_PT_CHAR,    /*!< SC Control Point Characteristic Declaration */
    BLE_CSCS_HDL_SC_CTRL_PT_VAL,     /*!< SC Control Point Characteristic Value */
    BLE_CSCS_HDL_SC_CTRL_PT_NTF_CFG, /*!< SC Control Point Characteristic Client Characteristic Configuration Descriptor */

    BLE_CSCS_HDL_NB,                 /*!< Number of attribute */
} ble_cscss_attr_db_handle_t;

/* Structure of CSCS information by connection */
typedef struct
{
    uint8_t         meas_cccd;          /*!< CSC Measurement Client Characteristic Configuration Descriptor value */
    uint8_t         sc_ctrl_pt_cccd;    /*!< SC Control Point Client Characteristic Configuration Descriptor value */
} ble_cscss_conn_t;

/* CSCSS environment variable structure */
typedef struct
{
    uint8_t                  svc_id;                                 /*!< Service ID assigned by BLE server module */
    uint16_t                 features;                               /*!< Services features */
    uint8_t                  sensor_loc;                             /*!< Sensor location */
    uint8_t                  loc_supp_num;                           /*!< Number of supported sensor locations in the list */
    ble_cscs_sensor_loc_t    loc_supp_list[BLE_CSCS_SENSOR_LOC_MAX]; /*!< List of supported sensor locations */
    ble_cscss_conn_t         cscss_conn[BLE_CSCSS_MAX_CONN_NUM];     /*!< Connection related information */
    ble_cscs_ctrl_pt_op_code ctrl_pt_op;                             /*!< Current control point op code */
    ble_cscss_callbacks_t    callbacks;                              /*!< Callback functions APP registered */
} ble_cscss_env_t;

/* CSCSS environment value */
static ble_cscss_env_t ble_cscss_env;

/* CSCS UUID 16bits array */
const uint8_t ble_cscs_uuid[BLE_GATT_UUID_16_LEN] = UUID_16BIT_TO_ARRAY(BLE_GATT_SVC_CYCLING_SPEED_CADENCE);

/* CSCSS Database Description */
const ble_gatt_attr_desc_t ble_cscss_attr_db[BLE_CSCS_HDL_NB] =
{
    [BLE_CSCS_HDL_SVC]                = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_PRIMARY_SERVICE), PROP(RD),             0                                               },

    [BLE_CSCS_HDL_CSC_MEAS_CHAR]      = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD),             0                                               },
    [BLE_CSCS_HDL_CSC_MEAS_VAL]       = {UUID_16BIT_TO_ARRAY(BLE_GATT_CHAR_CSC_MEAS),        PROP(NTF),            BLE_CSCS_CSC_MEAS_MAX_LEN                       },
    [BLE_CSCS_HDL_CSC_MEAS_NTF_CFG]   = {UUID_16BIT_TO_ARRAY(BLE_GATT_DESC_CLIENT_CHAR_CFG), PROP(RD) | PROP(WR),  OPT(NO_OFFSET)                                  },

    [BLE_CSCS_HDL_CSC_FEAT_CHAR]      = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD),             0                                               },
    [BLE_CSCS_HDL_CSC_FEAT_VAL]       = {UUID_16BIT_TO_ARRAY(BLE_GATT_CHAR_CSC_FEAT),        PROP(RD),             OPT(NO_OFFSET) | BLE_CSCSS_CSC_FEAT_VAL_LEN     },

    [BLE_CSCS_HDL_SENSOR_LOC_CHAR]    = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD),             0                                               },
    [BLE_CSCS_HDL_SENSOR_LOC_VAL]     = {UUID_16BIT_TO_ARRAY(BLE_GATT_CHAR_SENSOR_LOC),      PROP(RD),             OPT(NO_OFFSET) | BLE_CSCSS_SENSOR_LOC_VAL_LEN   },

    [BLE_CSCS_HDL_SC_CTRL_PT_CHAR]    = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD),             0                                               },
    [BLE_CSCS_HDL_SC_CTRL_PT_VAL]     = {UUID_16BIT_TO_ARRAY(BLE_GATT_CHAR_SC_CTRL_PT),      PROP(IND) | PROP(WR), OPT(NO_OFFSET) | BLE_CSCS_SC_CTRL_PT_RSP_MAX_LEN},
    [BLE_CSCS_HDL_SC_CTRL_PT_NTF_CFG] = {UUID_16BIT_TO_ARRAY(BLE_GATT_DESC_CLIENT_CHAR_CFG), PROP(RD) | PROP(WR),  OPT(NO_OFFSET)                                  },
};

/*!
    \brief      Function to handle control point request
    \param[in]  conn_idx: connection index
    \param[in]  p_req: pointer to control point request data
    \param[out] p_rsp: pointer to control point response data
    \param[out] rsp_len: pointer to response data length
    \retval     none
*/
static void ble_cscss_handle_ctrl_pt_req(uint8_t conn_idx, uint8_t *p_req, uint8_t *p_rsp, uint16_t *rsp_len)
{
    uint8_t op_code;
    uint8_t *p_start = p_rsp;
    uint8_t rsp_status = BLE_CSCS_CTRL_PT_RSP_NOT_SUPP;

    op_code = *p_req++;

    switch (op_code) {
    case BLE_CSCS_CTRL_PT_OP_SET_CUMUL_VAL:
        // Check if the Wheel Revolution Data feature is supported
        if (ble_cscss_env.features & BLE_CSCS_FEAT_WHEEL_REV_DATA_BIT) {
            rsp_status = BLE_CSCS_CTRL_PT_RSP_SUCCESS;
            ble_cscss_env.ctrl_pt_op = op_code;

            if (ble_cscss_env.callbacks.cumul_value_set_cb) {
                ble_cscss_env.callbacks.cumul_value_set_cb(conn_idx, ble_read32p(p_req));
            }
        }
        break;

    case BLE_CSCS_CTRL_PT_OP_UPDATE_LOC:
        // Check if the Multiple Sensor Location feature is supported
        if (ble_cscss_env.features & BLE_CSCS_FEAT_MULT_SENSOR_LOC_BIT) {
            uint8_t sensor_loc = *p_req;

            // Check the sensor location value
            if (sensor_loc < BLE_CSCS_SENSOR_LOC_MAX) {
                rsp_status = BLE_CSCS_CTRL_PT_RSP_SUCCESS;
                ble_cscss_env.ctrl_pt_op = op_code;
                ble_cscss_env.sensor_loc = sensor_loc;

                if (ble_cscss_env.callbacks.location_update_cb) {
                    ble_cscss_env.callbacks.location_update_cb(conn_idx, sensor_loc);
                }
            } else {
                rsp_status = BLE_CSCS_CTRL_PT_RSP_INVALID_PARAM;
            }
        }
        break;

    case BLE_CSCS_CTRL_PT_OP_REQ_SUPP_LOC:
        // Check if the Multiple Sensor Location feature is supported
        if (ble_cscss_env.features & BLE_CSCS_FEAT_MULT_SENSOR_LOC_BIT) {
            rsp_status = BLE_CSCS_CTRL_PT_RSP_SUCCESS;
            ble_cscss_env.ctrl_pt_op = op_code;
        }
        break;

    default:
        break;
    }

    *p_rsp++ = BLE_CSCS_CTRL_PT_RSP_CODE;
    *p_rsp++ = op_code;
    *p_rsp++ = rsp_status;

    if ((rsp_status == BLE_CSCS_CTRL_PT_RSP_SUCCESS) && (op_code == BLE_CSCS_CTRL_PT_OP_REQ_SUPP_LOC)) {
        uint8_t i;

        for (i = 0; i < ble_cscss_env.loc_supp_num; i++) {
            *p_rsp++ = ble_cscss_env.loc_supp_list[i];
        }
    }

    *rsp_len = p_rsp - p_start;
}

/*!
    \brief      Handle BLE connection connected event
    \param[in]  conn_idx: connection index
    \param[in]  p_addr: pointer to peer address information
    \param[out] none
    \retval     none
*/
static void ble_cscss_on_connect(uint8_t conn_idx, ble_gap_addr_t *p_addr)
{
    if (conn_idx < BLE_CSCSS_MAX_CONN_NUM) {
        ///TODO: try to load CCCD from flash if bonded before
    }
}

/*!
    \brief      Handle BLE connection disconnected event
    \param[in]  conn_idx: connection index
    \param[out] none
    \retval     none
*/
static void ble_cscss_on_disconnect(uint8_t conn_idx)
{
    if (conn_idx < BLE_CSCSS_MAX_CONN_NUM) {
        ble_cscss_env.cscss_conn[conn_idx].meas_cccd = 0;
        ble_cscss_env.cscss_conn[conn_idx].sc_ctrl_pt_cccd = 0;
    }
}

/*!
    \brief      Callback function to handle gatts read event
    \param[in]  conn_idx: connection index
    \param[in]  p_req: pointer to gatts read request information data
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_cscss_gatts_read_cb(uint8_t conn_idx, ble_gatts_read_req_t *p_req)
{
    uint8_t attr_idx = p_req->att_idx + BLE_CSCS_HDL_SVC;
    uint16_t status = BLE_ERR_NO_ERROR;
    uint16_t attr_len = 0;
    uint8_t attr_val[2];

    if (conn_idx >= BLE_CSCSS_MAX_CONN_NUM) {
        return BLE_ATT_ERR_APP_ERROR;
    }

    switch (attr_idx) {
    case BLE_CSCS_HDL_CSC_MEAS_NTF_CFG:
        ble_write16p(attr_val, ble_cscss_env.cscss_conn[conn_idx].meas_cccd);
        attr_len = 2;
        break;

    case BLE_CSCS_HDL_CSC_FEAT_VAL:
        ble_write16p(attr_val, ble_cscss_env.features);
        attr_len = BLE_CSCSS_CSC_FEAT_VAL_LEN;
        break;

    case BLE_CSCS_HDL_SENSOR_LOC_VAL:
        attr_val[0] = ble_cscss_env.sensor_loc;
        attr_len = BLE_CSCSS_SENSOR_LOC_VAL_LEN;
        break;

    case BLE_CSCS_HDL_SC_CTRL_PT_NTF_CFG:
        ble_write16p(attr_val, ble_cscss_env.cscss_conn[conn_idx].sc_ctrl_pt_cccd);
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
static ble_status_t ble_cscss_gatts_write_cb(uint8_t conn_idx, ble_gatts_write_req_t *p_req)
{
    uint8_t attr_idx = p_req->att_idx + BLE_CSCS_HDL_SVC;
    uint16_t status = BLE_ERR_NO_ERROR;
    uint8_t rsp_data[BLE_CSCS_SC_CTRL_PT_RSP_MAX_LEN] = {0};
    uint16_t rsp_len = 0;

    if (conn_idx >= BLE_CSCSS_MAX_CONN_NUM) {
        return BLE_ATT_ERR_APP_ERROR;
    }

    switch (attr_idx) {
    case BLE_CSCS_HDL_CSC_MEAS_NTF_CFG:
        if (p_req->val_len == BLE_GATT_CCCD_LEN) {
            ble_cscss_env.cscss_conn[conn_idx].meas_cccd = ble_read16p(p_req->p_val);
            ///TODO: save to flash for bonded device
        } else {
            status = BLE_ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
        }
        break;

    case BLE_CSCS_HDL_SC_CTRL_PT_VAL:
        if (!ble_cscss_env.cscss_conn[conn_idx].sc_ctrl_pt_cccd) {
            // Check if sending of indications has been enabled
            status = BLE_CSCS_ERROR_CCCD_IMPROPER_CFG;
        } else if (ble_cscss_env.ctrl_pt_op != BLE_CSCS_CTRL_PT_OP_RESERVED) {
            // A procedure is already in progress
            status = BLE_CSCS_ERROR_PROC_IN_PROGRESS;
        } else {
            ble_cscss_handle_ctrl_pt_req(conn_idx, p_req->p_val, rsp_data, &rsp_len);
        }
        break;

    case BLE_CSCS_HDL_SC_CTRL_PT_NTF_CFG:
        if (p_req->val_len == BLE_GATT_CCCD_LEN) {
            ble_cscss_env.cscss_conn[conn_idx].sc_ctrl_pt_cccd = ble_read16p(p_req->p_val);
            ///TODO: save to flash for bonded device
        } else {
            status = BLE_ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
        }
        break;

    default:
        status = BLE_ATT_ERR_INVALID_HANDLE;
    }

    if (p_req->local_req == false) {
        p_req->pending_cfm = true;
        ble_gatts_svc_attr_write_cfm(conn_idx, p_req->token, status);
    }

    if ((attr_idx == BLE_CSCS_HDL_SC_CTRL_PT_VAL) && (status == BLE_ERR_NO_ERROR)) {
        ble_gatts_ntf_ind_send(conn_idx, ble_cscss_env.svc_id, BLE_CSCS_HDL_SC_CTRL_PT_VAL,
                               rsp_data, rsp_len, BLE_GATT_INDICATE);
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle BLE_SRV_EVT_GATT_OPERATION event
    \param[in]  p_info: pointer to gatts operation information data
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_cscss_handle_gatts_op(ble_gatts_op_info_t *p_info)
{
    ble_status_t ret = BLE_ERR_NO_ERROR;

    switch (p_info->gatts_op_sub_evt) {
    case BLE_SRV_EVT_READ_REQ:
        ret = ble_cscss_gatts_read_cb(p_info->conn_idx, &p_info->gatts_op_data.read_req);
        break;

    case BLE_SRV_EVT_WRITE_REQ:
        ret = ble_cscss_gatts_write_cb(p_info->conn_idx, &p_info->gatts_op_data.write_req);
        break;

    case BLE_SRV_EVT_NTF_IND_SEND_RSP:
        if (p_info->gatts_op_data.ntf_ind_send_rsp.status != BLE_ERR_NO_ERROR) {
            dbg_print(WARNING, "cscss ntf/ind send fail, status 0x%x, conn_idx %d, att_idx %d\r\n",
                      p_info->gatts_op_data.ntf_ind_send_rsp.status, p_info->conn_idx,
                      p_info->gatts_op_data.ntf_ind_send_rsp.att_idx);
        }

        if (p_info->gatts_op_data.ntf_ind_send_rsp.att_idx == BLE_CSCS_HDL_SC_CTRL_PT_VAL) {
            ble_cscss_env.ctrl_pt_op = BLE_CSCS_CTRL_PT_OP_RESERVED;
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
static ble_status_t ble_cscss_gatts_cb(ble_gatts_msg_info_t *p_info)
{
    ble_status_t ret = BLE_ERR_NO_ERROR;

    switch (p_info->srv_msg_type) {
    case BLE_SRV_EVT_SVC_ADD_RSP:
        if ((p_info->msg_data.svc_add_rsp.svc_id == ble_cscss_env.svc_id) &&
            (p_info->msg_data.svc_add_rsp.status != BLE_ERR_NO_ERROR)) {
            dbg_print(WARNING, "cscss svc add fail, status 0x%x\r\n", p_info->msg_data.svc_add_rsp.status);
        }
        break;

    case BLE_SRV_EVT_SVC_RMV_RSP:
        if ((p_info->msg_data.svc_rmv_rsp.svc_id == ble_cscss_env.svc_id) &&
            (p_info->msg_data.svc_rmv_rsp.status != BLE_ERR_NO_ERROR)) {
            dbg_print(WARNING, "cscss svc add fail, status 0x%x\r\n", p_info->msg_data.svc_rmv_rsp.status);
        }
        break;

    case BLE_SRV_EVT_CONN_STATE_CHANGE_IND:
        if (p_info->msg_data.conn_state_change_ind.conn_state == BLE_CONN_STATE_DISCONNECTD) {
            ble_cscss_on_disconnect(p_info->msg_data.conn_state_change_ind.info.disconn_info.conn_idx);
        } else if (p_info->msg_data.conn_state_change_ind.conn_state == BLE_CONN_STATE_CONNECTED) {
            ble_cscss_on_connect(p_info->msg_data.conn_state_change_ind.info.conn_info.conn_idx,
                                &p_info->msg_data.conn_state_change_ind.info.conn_info.peer_addr);
        }
        break;

    case BLE_SRV_EVT_GATT_OPERATION:
        ret = ble_cscss_handle_gatts_op(&p_info->msg_data.gatts_op_info);
        break;

    default:
        break;
    }

    return ret;
}

/*!
    \brief      Init Cycling Speed and Cadence Service Server
    \param[in]  p_param: pointer to CSCSS init parameters
    \param[in]  callback: callback functions
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_cscss_init(ble_cscss_init_param_t *p_param, ble_cscss_callbacks_t callback)
{
    ble_status_t ret = BLE_ERR_NO_ERROR;

    if (p_param->loc_supp_num > BLE_CSCS_SENSOR_LOC_MAX) {
        return BLE_PRF_ERR_INVALID_PARAM;
    }

    ret = ble_gatts_svc_add(&ble_cscss_env.svc_id, ble_cscs_uuid, 0,
                            SVC_UUID(16) | SVC_SEC_LVL_VAL(p_param->sec_lvl),
                            ble_cscss_attr_db, BLE_CSCS_HDL_NB, ble_cscss_gatts_cb);
    if (ret != BLE_ERR_NO_ERROR) {
        return ret;
    }

    ble_cscss_env.features = p_param->csc_feature;
    ble_cscss_env.loc_supp_num = p_param->loc_supp_num;
    ble_cscss_env.sensor_loc = p_param->sensor_loc;
    ble_cscss_env.ctrl_pt_op = BLE_CSCS_CTRL_PT_OP_RESERVED;
    memcpy(ble_cscss_env.loc_supp_list, p_param->p_loc_supp_list,
           sizeof(ble_cscs_sensor_loc_t) * ble_cscss_env.loc_supp_num);

    ble_cscss_env.callbacks = callback;

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Deinit Cycling Speed and Cadence Service Server
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_cscss_deinit(void)
{
    return ble_gatts_svc_rmv(ble_cscss_env.svc_id);
}

/*!
    \brief      Send CSC Measurement notification
    \param[in]  p_meas: pointer to CSC Measurement value to send
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_cscss_meas_send(ble_cscs_csc_meas_t *p_meas)
{
    uint8_t meas_data[BLE_CSCS_CSC_MEAS_MAX_LEN] = {0};
    uint8_t meas_flags = 0;
    uint8_t *p_data = meas_data;
    uint8_t i, len;
    uint32_t conidx_bf = 0;

    if (p_meas == NULL) {
        return BLE_GAP_ERR_INVALID_PARAM;
    }

    if ((ble_cscss_env.features & BLE_CSCS_FEAT_WHEEL_REV_DATA_BIT) &&
        (p_meas->flags & BLE_CSCS_MEAS_WHEEL_REV_DATA_PRESENT)) {
        meas_flags |= BLE_CSCS_MEAS_WHEEL_REV_DATA_PRESENT;
    }

    if ((ble_cscss_env.features & BLE_CSCS_FEAT_CRANK_REV_DATA_BIT) &&
        (p_meas->flags & BLE_CSCS_MEAS_CRANK_REV_DATA_PRESENT)) {
        meas_flags |= BLE_CSCS_MEAS_CRANK_REV_DATA_PRESENT;
    }

    *p_data++ = meas_flags;
    len = 1;

    if (meas_flags & BLE_CSCS_MEAS_WHEEL_REV_DATA_PRESENT) {
        ble_write32p(p_data, p_meas->cumul_wheel_rev);
        p_data += 4;
        ble_write16p(p_data, p_meas->last_wheel_evt_time);
        p_data += 2;
        len += 6;
    }

    if (meas_flags & BLE_CSCS_MEAS_CRANK_REV_DATA_PRESENT) {
        ble_write16p(p_data, p_meas->cumul_crank_rev);
        p_data += 2;
        ble_write16p(p_data, p_meas->last_crank_evt_time);
        p_data += 2;
        len += 4;
    }

    for (i = 0; i < BLE_CSCSS_MAX_CONN_NUM; i++) {
        if (ble_cscss_env.cscss_conn[i].meas_cccd & BLE_GATT_CCCD_NTF_BIT) {
            conidx_bf |= BIT(i);
        }
    }

    return ble_gatts_ntf_ind_mtp_send(conidx_bf, ble_cscss_env.svc_id, BLE_CSCS_HDL_CSC_MEAS_VAL,
                                      meas_data, len, BLE_GATT_NOTIFY);
}

