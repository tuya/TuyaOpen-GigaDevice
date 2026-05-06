/*!
    \file    ble_hogpd.c
    \brief   Implementation of hogp Service Server.

    \version 2024-05-17, V1.0.0, firmware for GD32VW55x
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
#include "ble_profile_config.h"
#include <string.h>
#include "dbg_print.h"
#include "ble_utils.h"
#include "ble_gatt.h"
#include "ble_gatts.h"
#include "ble_hogpd.h"

/* ble hogpd environment */
typedef struct
{
    uint8_t         proto_mode;                        /*!< proto_mode */
    hids_hid_info_t hid_info;                          /*!< hid informatiopn */
    uint16_t        boot_kb_ntf_cfg[BLE_MAX_CONN_NUM]; /*!< boot keyboard notification configration */
    uint16_t        report_ntf_cfg[BLE_MAX_CONN_NUM];  /*!< report notification configration */
    uint16_t        report_id;                         /*!< report id */
}ble_hogpd_env_t;

/* BLE hogp server attribute database handle list */
enum
{
    BLE_HOGPD_IDX_SVC,                             /*!< Hogp service Declaration */

//    HOGPD_IDX_INCL_SVC,

    BLE_HOGPD_IDX_HID_INFO_CHAR,                   /*!< HID Information*/
    BLE_HOGPD_IDX_HID_INFO_VAL,

    BLE_HOGPD_IDX_HID_CTNL_PT_CHAR,                /*!< HID Control Point*/
    BLE_HOGPD_IDX_HID_CTNL_PT_VAL,

    BLE_HOGPD_IDX_REPORT_MAP_CHAR,                 /*!< Report Map*/
    BLE_HOGPD_IDX_REPORT_MAP_VAL,
    BLE_HOGPD_IDX_REPORT_MAP_EXT_REP_REF,

    BLE_HOGPD_IDX_PROTO_MODE_CHAR,                 /*!< Protocol Mode*/
    BLE_HOGPD_IDX_PROTO_MODE_VAL,

    BLE_HOGPD_IDX_BOOT_KB_IN_REPORT_CHAR,          /*!< Boot Keyboard Input Report*/
    BLE_HOGPD_IDX_BOOT_KB_IN_REPORT_VAL,
    BLE_HOGPD_IDX_BOOT_KB_IN_REPORT_NTF_CFG,

    BLE_HOGPD_IDX_REPORT_CHAR,                     /*!<Input Report*/
    BLE_HOGPD_IDX_REPORT_VAL,
    BLE_HOGPD_IDX_REPORT_REP_REF,
    BLE_HOGPD_IDX_REPORT_NTF_CFG,

    BLE_HOGPD_IDX_NB,
};

/* hid report map */
static const uint8_t app_hid_report_map[] =
{
    0x05, 0x01,     /// USAGE PAGE (Generic Desktop)
    0x09, 0x06,     /// USAGE (Mouse)
    0xA1, 0x01,     /// COLLECTION (Application)

    0x05, 0x07,     /// USAGE PAGE (Generic Desktop)
    0x09, 0x06,     /// USAGE (Mouse)
    0xA1, 0x01,     /// COLLECTION (Application)
    0x85, 0x01,     /// REPORT ID (1) - MANDATORY
    0x95, 0x08,
    0x75, 0x08,     /// REPORT SIZE (1)
    0x15, 0x00,     /// LOGICAL MINIMUM (0)
    0x25, 0xFF,     /// LOGICAL MAXIMUM (1)
    0x19, 0x00,     /// USAGE MINIMUM (1)
    0x29, 0xFF,     /// USAGE MAXIMUM (8)
    0x81, 0x00,     /// INPUT (Data, Variable, Absolute)
    0xc0,
    0xc0,
};

/* HOGPD Database Description */
const ble_gatt_attr_desc_t ble_hogpd_attr_db[BLE_HOGPD_IDX_NB] =
{
    [BLE_HOGPD_IDX_SVC]                             = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_PRIMARY_SERVICE)  , PROP(RD)            ,0                                             },

  //    [HOGPD_IDX_INCL_SVC]                        = {GATT_DECL_INCLUDE,PROP(RD) , 0},

    [BLE_HOGPD_IDX_HID_INFO_CHAR]                   = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC)   , PROP(RD)            , 0                                            },
    [BLE_HOGPD_IDX_HID_INFO_VAL]                    = {UUID_16BIT_TO_ARRAY(BLE_HOGP_SVC_HID_INFO)          , PROP(RD)            , OPT(NO_OFFSET) | sizeof(hids_hid_info_t)},
    [BLE_HOGPD_IDX_HID_CTNL_PT_CHAR]                = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC)   , PROP(RD)            , 0                                            },
    [BLE_HOGPD_IDX_HID_CTNL_PT_VAL]                 = {UUID_16BIT_TO_ARRAY(BLE_HOGP_SVC_HID_CTNL_PT)       , PROP(WC)            , OPT(NO_OFFSET) | sizeof(uint8_t)             },
    [BLE_HOGPD_IDX_REPORT_MAP_CHAR]                 = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC)   , PROP(RD)            , 0                                            },
    [BLE_HOGPD_IDX_REPORT_MAP_VAL]                  = {UUID_16BIT_TO_ARRAY(BLE_HOGP_SVC_REPORT_MAP)        , PROP(RD)            , HOGPD_REPORT_MAP_MAX_LEN                     },
    [BLE_HOGPD_IDX_REPORT_MAP_EXT_REP_REF]          = {UUID_16BIT_TO_ARRAY(BLE_GATT_DESC_EXT_REPORT_REF)   , PROP(RD)            , sizeof(uint16_t)                             },
    [BLE_HOGPD_IDX_PROTO_MODE_CHAR]                 = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC)   , PROP(RD)            , 0                                            },
    [BLE_HOGPD_IDX_PROTO_MODE_VAL]                  = {UUID_16BIT_TO_ARRAY(BLE_HOGP_SVC_PROTOCOL_MODE)     , PROP(RD) | PROP(WC) , OPT(NO_OFFSET) | sizeof(uint8_t)             },
    [BLE_HOGPD_IDX_BOOT_KB_IN_REPORT_CHAR]          = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC)   , PROP(RD)            , 0                                            },
    [BLE_HOGPD_IDX_BOOT_KB_IN_REPORT_VAL]           = {UUID_16BIT_TO_ARRAY(BLE_HOGP_SVC_BOOT_KB_IN_REPORT) , PROP(RD) | PROP(NTF), OPT(NO_OFFSET) | HOGPD_BOOT_REPORT_MAX_LEN   },
    [BLE_HOGPD_IDX_BOOT_KB_IN_REPORT_NTF_CFG]       = {UUID_16BIT_TO_ARRAY(BLE_GATT_DESC_CLIENT_CHAR_CFG)  , PROP(RD) | PROP(WR) , OPT(NO_OFFSET)                               },
    [BLE_HOGPD_IDX_REPORT_CHAR]                     = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC)   , PROP(RD)            , 0                                            },
    [BLE_HOGPD_IDX_REPORT_VAL]                      = {UUID_16BIT_TO_ARRAY(BLE_HOGP_SVC_REPORT)            , PROP(RD) | PROP(NTF), OPT(NO_OFFSET) | HOGPD_REPORT_MAX_LEN        },
    [BLE_HOGPD_IDX_REPORT_REP_REF]                  = {UUID_16BIT_TO_ARRAY(BLE_GATT_DESC_REPORT_REF)       , PROP(RD)            ,sizeof(uint16_t)                              },
    [BLE_HOGPD_IDX_REPORT_NTF_CFG]                  = {UUID_16BIT_TO_ARRAY(BLE_GATT_DESC_CLIENT_CHAR_CFG)  , PROP(RD) | PROP(WR) , OPT(NO_OFFSET)                               },
};

/*!< Service ID assigned by BLE server module */
uint8_t hogpd_svc_id = 0;
/* Hogpd service server environment variable structure */
ble_hogpd_env_t ble_hogp_env = {0};
/* Hogpd UUID 16bits array */
const uint8_t ble_hogpd_svc_uuid[BLE_GATT_UUID_16_LEN] = UUID_16BIT_TO_ARRAY(BLE_HOGP_SVC_HID);

/*!
    \brief      Callback function to handle BLE_SRV_EVT_GATT_OPERATION event
    \param[in]  p_info: pointer to gatts operation information data
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_hogpd_handle_gatts_op(ble_gatts_op_info_t *p_info)
{
    ble_status_t ret = BLE_ERR_NO_ERROR;
    uint8_t attr_idx = 0;
    uint16_t len = 0;
    uint8_t attr_len = 0;
    uint8_t *p_attr = NULL;
    uint16_t report_map_cfg = 0;
    uint8_t conn_idx = p_info->conn_idx;

    if (p_info->gatts_op_sub_evt == BLE_SRV_EVT_READ_REQ) {
        ble_gatts_read_req_t * p_read_req = &p_info->gatts_op_data.read_req;

        attr_idx = p_read_req->att_idx;
        switch (attr_idx) {
        case BLE_HOGPD_IDX_HID_INFO_VAL:
            p_attr = (uint8_t *)&ble_hogp_env.hid_info;
            attr_len = sizeof(hids_hid_info_t);
            break;

        case BLE_HOGPD_IDX_PROTO_MODE_VAL:
            p_attr = (uint8_t *)&ble_hogp_env.proto_mode;
            attr_len = sizeof(uint8_t);
            break;

        case BLE_HOGPD_IDX_BOOT_KB_IN_REPORT_NTF_CFG:
            p_attr = (uint8_t *)&ble_hogp_env.boot_kb_ntf_cfg[conn_idx];
            attr_len = sizeof(uint16_t);
            break;

        case BLE_HOGPD_IDX_REPORT_NTF_CFG:
            p_attr = (uint8_t *)&ble_hogp_env.report_ntf_cfg[conn_idx];
            attr_len = sizeof(uint16_t);
            break;

        case BLE_HOGPD_IDX_REPORT_REP_REF:
            p_attr = (uint8_t *)&ble_hogp_env.report_id;
            attr_len = sizeof(uint16_t);
            break;

        case BLE_HOGPD_IDX_REPORT_MAP_VAL:
            p_attr = (uint8_t *)(app_hid_report_map + p_read_req->offset);
            attr_len = sizeof(app_hid_report_map);
            break;

        case BLE_HOGPD_IDX_REPORT_MAP_EXT_REP_REF:
            p_attr = (uint8_t *)&report_map_cfg;
            attr_len = sizeof(uint16_t);
            break;

        default:
            break;
        }

        if (p_read_req->offset > attr_len) {
            return BLE_ATT_ERR_INVALID_OFFSET;
        }

        len = ble_min(p_read_req->max_len, attr_len - p_read_req->offset);
        p_read_req->val_len = len;
        memcpy(p_read_req->p_val, p_attr, len);
    } else if (p_info->gatts_op_sub_evt == BLE_SRV_EVT_WRITE_REQ) {
        ble_gatts_write_req_t * p_write_req = &p_info->gatts_op_data.write_req;

        attr_idx = p_write_req->att_idx;

        switch (attr_idx) {
        case BLE_HOGPD_IDX_PROTO_MODE_VAL:
            if (p_write_req->val_len == 1)
                ble_hogp_env.proto_mode = *p_write_req->p_val;
            break;

        case BLE_HOGPD_IDX_BOOT_KB_IN_REPORT_NTF_CFG:
            if (p_write_req->val_len == BLE_GATT_CCCD_LEN) {
                memcpy(&ble_hogp_env.boot_kb_ntf_cfg[conn_idx], p_write_req->p_val,p_write_req->val_len);
            }
            break;

        case BLE_HOGPD_IDX_REPORT_NTF_CFG:
            if (p_write_req->val_len == BLE_GATT_CCCD_LEN) {
                memcpy(&ble_hogp_env.report_ntf_cfg[conn_idx], p_write_req->p_val,p_write_req->val_len);
            }
            break;

        default:
            break;
        }
    }

    return ret;
}


/*!
    \brief      Callback function to handle GATT server messages
    \param[in]  p_srv_msg_info: pointer to GATT server message information
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_hogpd_srv_cb(ble_gatts_msg_info_t *p_srv_msg_info)
{
    switch (p_srv_msg_info->srv_msg_type) {
    case BLE_SRV_EVT_SVC_ADD_RSP:
        dbg_print(INFO, "[ble_hogpd_srv_cb], svc_add_rsp status = 0x%x\r\n", p_srv_msg_info->msg_data.svc_add_rsp.status);
        break;

    case BLE_SRV_EVT_SVC_RMV_RSP:
        dbg_print(INFO, "[ble_hogpd_srv_cb], svc_rmv_rsp status = 0x%x\r\n", p_srv_msg_info->msg_data.svc_rmv_rsp.status);
        break;

    case BLE_SRV_EVT_CONN_STATE_CHANGE_IND: {
        ble_gatts_conn_state_change_ind_t * p_ind = &p_srv_msg_info->msg_data.conn_state_change_ind;
        uint8_t conn_idx = p_ind->info.conn_info.conn_idx;

        if (p_ind->conn_state == BLE_CONN_STATE_DISCONNECTD) {
            dbg_print(INFO, "[ble_hogpd_srv_cb] conn_state_change_ind disconnected event, conn_idx = %d, disconn reason = 0x%x\r\n", conn_idx, p_ind->info.disconn_info.reason);
        } else if (p_ind->conn_state == BLE_CONN_STATE_CONNECTED) {
            dbg_print(INFO, "[ble_hogpd_srv_cb] conn_state_change_ind connected event, conn_idx = %d\r\n", conn_idx);
        }
    } break;

    case BLE_SRV_EVT_GATT_OPERATION:
        ble_hogpd_handle_gatts_op(&p_srv_msg_info->msg_data.gatts_op_info);
        break;
    default:
        break;
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Hogpd send kb value
    \param[in]  conn_idx: connection index
    \param[out] none
    \retval     none
*/
void ble_hogpd_send_kb_value(uint8_t conn_idx, uint8_t * p_value)
{
    uint8_t notify_buf[KB_REPORT_LENGTH] = {0};

    if (ble_hogp_env.report_ntf_cfg[conn_idx] & BLE_GATT_CCCD_NTF_BIT) {
        memcpy(notify_buf, p_value, KB_REPORT_LENGTH);
        ble_gatts_ntf_ind_send(conn_idx, hogpd_svc_id, BLE_HOGPD_IDX_REPORT_VAL,
                               notify_buf, KB_REPORT_LENGTH, BLE_GATT_NOTIFY);
    }
}

/*!
    \brief      Init BLE hogp server
    \param[in]  none
    \param[out] none
    \retval     none
*/
ble_status_t ble_hogpd_init(ble_hogpd_param_t *p_param)
{
    ble_status_t ret = BLE_ERR_NO_ERROR;

    if (p_param == NULL) {
        return BLE_PRF_ERR_INVALID_PARAM;
    }

    ret = ble_gatts_svc_add(&hogpd_svc_id, ble_hogpd_svc_uuid, 0, SVC_UUID(16), ble_hogpd_attr_db,
                          BLE_HOGPD_IDX_NB, ble_hogpd_srv_cb);

    if (ret != BLE_ERR_NO_ERROR) {
        return ret;
    }

    ble_hogp_env.hid_info.bcdHID = p_param->hid_info.bcdHID;
    ble_hogp_env.hid_info.bCountryCode = p_param->hid_info.bCountryCode;
    ble_hogp_env.hid_info.flags = p_param->hid_info.flags;
    ble_hogp_env.report_id = 0x0101;     //id = 1, typr = 1(input)
    return BLE_ERR_NO_ERROR;
}


