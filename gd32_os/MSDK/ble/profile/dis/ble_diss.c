/*!
    \file    ble_diss.c
    \brief   Implementation of device information service server.

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

#include "ble_utils.h"
#include "ble_gatt.h"
#include "ble_gatts.h"
#include "ble_diss.h"
#include "wrapper_os.h"

/* System ID string length */
#define BLE_DIS_SYS_ID_LEN                          (0x08)

/* PnP ID length */
#define BLE_DIS_PNP_ID_LEN                          (0x07)

/* DIS related service/characteristic UUID value */
typedef enum
{
    BLE_DIS_SVC_DEVICE_INFO  = BLE_GATT_UUID_16_LSB(0x180A),    /*!< Device Information Service */

    BLE_DIS_CHAR_SYS_ID      = BLE_GATT_UUID_16_LSB(0x2A23),    /*!< System ID characteristic */
    BLE_DIS_CHAR_MODEL_NB    = BLE_GATT_UUID_16_LSB(0x2A24),    /*!< Model Number characteristic */
    BLE_DIS_CHAR_SERIAL_NB   = BLE_GATT_UUID_16_LSB(0x2A25),    /*!< Serial Number characteristic */
    BLE_DIS_CHAR_FW_REV      = BLE_GATT_UUID_16_LSB(0x2A26),    /*!< Firmware Revision characteristic */
    BLE_DIS_CHAR_HW_REV      = BLE_GATT_UUID_16_LSB(0x2A27),    /*!< Hardware Rsssevision characteristic */
    BLE_DIS_CHAR_SW_REV      = BLE_GATT_UUID_16_LSB(0x2A28),    /*!< Software Revision characteristic */
    BLE_DIS_CHAR_MANUF_NAME  = BLE_GATT_UUID_16_LSB(0x2A29),    /*!< Manufacturer Name characteristic */
    BLE_DIS_CHAR_IEEE_CERTIF = BLE_GATT_UUID_16_LSB(0x2A2A),    /*!< IEEE Regulatory Certification Data List characteristic */
    BLE_DIS_CHAR_PNP_ID      = BLE_GATT_UUID_16_LSB(0x2A50),    /*!< PnP ID characteristic */
} ble_dis_char_uuid_t;

/* DIS Attribute database handle list */
typedef enum
{
    BLE_DIS_HDL_SVC,                /*!< Device Information Service Declaration */

    BLE_DIS_HDL_MANUFACT_NAME_CHAR, /*!< Manufacturer Name Characteristic Declaration */
    BLE_DIS_HDL_MANUFACT_NAME_VAL,  /*!< Manufacturer Name Characteristic Value */

    BLE_DIS_HDL_MODEL_NB_CHAR,      /*!< Model Number String Characteristic Declaration */
    BLE_DIS_HDL_MODEL_NB_VAL,       /*!< Model Number String Characteristic Value */

    BLE_DIS_HDL_SERIAL_NB_CHAR,     /*!< Serial Number String Characteristic Declaration */
    BLE_DIS_HDL_SERIAL_NB_VAL,      /*!< Serial Number String Characteristic Value */

    BLE_DIS_HDL_HARD_REV_CHAR,      /*!< Hardware Revision String Characteristic Declaration */
    BLE_DIS_HDL_HARD_REV_VAL,       /*!< Hardware Revision String Characteristic Value */

    BLE_DIS_HDL_FIRM_REV_CHAR,      /*!< Firmware Revision String Characteristic Declaration */
    BLE_DIS_HDL_FIRM_REV_VAL,       /*!< Firmware Revision String Characteristic Value */

    BLE_DIS_HDL_SW_REV_CHAR,        /*!< Software Revision String Characteristic Declaration */
    BLE_DIS_HDL_SW_REV_VAL,         /*!< Software Revision String Characteristic Value */

    BLE_DIS_HDL_SYSTEM_ID_CHAR,     /*!< System ID Characteristic Declaration */
    BLE_DIS_HDL_SYSTEM_ID_VAL,      /*!< System ID Characteristic Value */

    BLE_DIS_HDL_IEEE_CHAR,          /*!< IEEE Regulatory Certification Data List Characteristic Declaration */
    BLE_DIS_HDL_IEEE_VAL,           /*!< IEEE Regulatory Certification Data List Characteristic Value */

    BLE_DIS_HDL_PNP_ID_CHAR,        /*!< PnP ID Characteristic Declaration */
    BLE_DIS_HDL_PNP_ID_VAL,         /*!< PnP ID Characteristic Value */

    BLE_DIS_HDL_NB,
} ble_diss_attr_db_handle_t;

/* DIS value structure */
typedef struct
{
    uint8_t manufact_name[BLE_DIS_VAL_MAX_LEN];     /*!< Manufacturer Name String */
    uint8_t manufact_name_len;
    uint8_t model_num[BLE_DIS_VAL_MAX_LEN];         /*!< Model Number String */
    uint8_t model_num_len;
    uint8_t serial_num[BLE_DIS_VAL_MAX_LEN];        /*!< Serial Number String */
    uint8_t serial_num_len;
    uint8_t hw_rev[BLE_DIS_VAL_MAX_LEN];            /*!< Hardware Revision String */
    uint8_t hw_rev_len;
    uint8_t fw_rev[BLE_DIS_VAL_MAX_LEN];            /*!< Firmware Revision String */
    uint8_t fw_rev_len;
    uint8_t sw_rev[BLE_DIS_VAL_MAX_LEN];            /*!< Software Revision String */
    uint8_t sw_rev_len;
    uint8_t sys_id[BLE_DIS_SYS_ID_LEN];             /*!< System ID */
    uint8_t ieee_data[BLE_DIS_VAL_MAX_LEN];         /*!< IEEE Regulatory Certification Data List */
    uint8_t ieee_data_len;
    uint8_t pnp_id[BLE_DIS_PNP_ID_LEN];             /*!< PnP ID */
} ble_diss_value_t;

/* DIS Database Description */
const ble_gatt_attr_desc_t ble_diss_attr_db[BLE_DIS_HDL_NB] =
{
    [BLE_DIS_HDL_SVC]                = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_PRIMARY_SERVICE), PROP(RD), 0                  },

    [BLE_DIS_HDL_MANUFACT_NAME_CHAR] = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD), 0                  },
    [BLE_DIS_HDL_MANUFACT_NAME_VAL]  = {UUID_16BIT_TO_ARRAY(BLE_DIS_CHAR_MANUF_NAME),       PROP(RD), BLE_DIS_VAL_MAX_LEN},

    [BLE_DIS_HDL_MODEL_NB_CHAR]      = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD), 0                  },
    [BLE_DIS_HDL_MODEL_NB_VAL]       = {UUID_16BIT_TO_ARRAY(BLE_DIS_CHAR_MODEL_NB),         PROP(RD), BLE_DIS_VAL_MAX_LEN},

    [BLE_DIS_HDL_SERIAL_NB_CHAR]     = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD), 0                  },
    [BLE_DIS_HDL_SERIAL_NB_VAL]      = {UUID_16BIT_TO_ARRAY(BLE_DIS_CHAR_SERIAL_NB),        PROP(RD), BLE_DIS_VAL_MAX_LEN},

    [BLE_DIS_HDL_HARD_REV_CHAR]      = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD), 0                  },
    [BLE_DIS_HDL_HARD_REV_VAL]       = {UUID_16BIT_TO_ARRAY(BLE_DIS_CHAR_HW_REV),           PROP(RD), BLE_DIS_VAL_MAX_LEN},

    [BLE_DIS_HDL_FIRM_REV_CHAR]      = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD), 0                  },
    [BLE_DIS_HDL_FIRM_REV_VAL]       = {UUID_16BIT_TO_ARRAY(BLE_DIS_CHAR_FW_REV),           PROP(RD), BLE_DIS_VAL_MAX_LEN},

    [BLE_DIS_HDL_SW_REV_CHAR]        = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD), 0                  },
    [BLE_DIS_HDL_SW_REV_VAL]         = {UUID_16BIT_TO_ARRAY(BLE_DIS_CHAR_SW_REV),           PROP(RD), BLE_DIS_VAL_MAX_LEN},

    [BLE_DIS_HDL_SYSTEM_ID_CHAR]     = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD), 0                  },
    [BLE_DIS_HDL_SYSTEM_ID_VAL]      = {UUID_16BIT_TO_ARRAY(BLE_DIS_CHAR_SYS_ID),           PROP(RD), BLE_DIS_SYS_ID_LEN },

    [BLE_DIS_HDL_IEEE_CHAR]          = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD), 0                  },
    [BLE_DIS_HDL_IEEE_VAL]           = {UUID_16BIT_TO_ARRAY(BLE_DIS_CHAR_IEEE_CERTIF),      PROP(RD), BLE_DIS_VAL_MAX_LEN},

    [BLE_DIS_HDL_PNP_ID_CHAR]        = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD), 0                  },
    [BLE_DIS_HDL_PNP_ID_VAL]         = {UUID_16BIT_TO_ARRAY(BLE_DIS_CHAR_PNP_ID),           PROP(RD), BLE_DIS_PNP_ID_LEN },
};

/* Device information value */
static ble_diss_value_t ble_diss_val = {0};

/* DIS service ID assigned by GATT server module */
static uint8_t ble_diss_svc_id = 0xFF;

/* DIS UUID 16bits array */
const uint8_t ble_dis_uuid[2] = UUID_16BIT_TO_ARRAY(BLE_DIS_SVC_DEVICE_INFO);

/*!
    \brief      Callback function to handle GATT server messages
    \param[in]  p_srv_msg_info: pointer to GATT server message information
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_diss_srv_cb(ble_gatts_msg_info_t *p_srv_msg_info)
{
    uint8_t attr_idx = 0;
    uint16_t len = 0;
    uint8_t attr_len = 0;
    uint8_t *p_attr = NULL;

    if (p_srv_msg_info->srv_msg_type == BLE_SRV_EVT_GATT_OPERATION) {
        if (p_srv_msg_info->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_READ_REQ) {
            ble_gatts_read_req_t * p_read_req = &p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.read_req;

            attr_idx = p_read_req->att_idx;
            switch (attr_idx) {
            case BLE_DIS_HDL_MANUFACT_NAME_VAL: {
                p_attr   = ble_diss_val.manufact_name;
                attr_len = ble_diss_val.manufact_name_len;
            } break;

            case BLE_DIS_HDL_MODEL_NB_VAL: {
                p_attr   = ble_diss_val.model_num;
                attr_len = ble_diss_val.model_num_len;
            } break;

            case BLE_DIS_HDL_SERIAL_NB_VAL: {
                p_attr   = ble_diss_val.serial_num;
                attr_len = ble_diss_val.serial_num_len;
            } break;

            case BLE_DIS_HDL_HARD_REV_VAL: {
                p_attr   = ble_diss_val.hw_rev;
                attr_len = ble_diss_val.hw_rev_len;
            } break;

            case BLE_DIS_HDL_FIRM_REV_VAL: {
                p_attr   = ble_diss_val.fw_rev;
                attr_len = ble_diss_val.fw_rev_len;
            } break;

            case BLE_DIS_HDL_SW_REV_VAL: {
                p_attr   = ble_diss_val.sw_rev;
                attr_len = ble_diss_val.sw_rev_len;
            } break;

            case BLE_DIS_HDL_SYSTEM_ID_VAL: {
                p_attr   = ble_diss_val.sys_id;
                attr_len = BLE_DIS_SYS_ID_LEN;
            } break;

            case BLE_DIS_HDL_IEEE_VAL: {
                p_attr   = ble_diss_val.ieee_data;
                attr_len = ble_diss_val.ieee_data_len;
            } break;

            case BLE_DIS_HDL_PNP_ID_VAL: {
                p_attr   = ble_diss_val.pnp_id;
                attr_len = BLE_DIS_PNP_ID_LEN;
            } break;

            default:
                return BLE_ATT_ERR_INVALID_HANDLE;
            }

            if (p_read_req->offset > attr_len) {
                return BLE_ATT_ERR_INVALID_OFFSET;
            }

            len = ble_min(p_read_req->max_len, attr_len - p_read_req->offset);
            p_read_req->val_len = len;
            sys_memcpy(p_read_req->p_val, p_attr, len);
        }
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Init Device Information Service Server
    \param[in]  p_param: pointer to Device Information Service init parameters
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_diss_init(ble_diss_init_param_t *p_param)
{
    ble_status_t ret = BLE_ERR_NO_ERROR;

    if (p_param == NULL) {
        return BLE_PRF_ERR_INVALID_PARAM;
    }

    ret = ble_gatts_svc_add(&ble_diss_svc_id, ble_dis_uuid, 0, SVC_UUID(16) | SVC_SEC_LVL_VAL(p_param->sec_lvl),
                            ble_diss_attr_db, BLE_DIS_HDL_NB, ble_diss_srv_cb);

    if (ret != BLE_ERR_NO_ERROR) {
        return ret;
    }

    if (p_param->manufact_name.len) {
        ble_diss_val.manufact_name_len = ble_min(p_param->manufact_name.len, BLE_DIS_VAL_MAX_LEN);
        sys_memcpy(ble_diss_val.manufact_name, p_param->manufact_name.p_data, ble_diss_val.manufact_name_len);
    }

    if (p_param->model_num.len) {
        ble_diss_val.model_num_len = ble_min(p_param->model_num.len, BLE_DIS_VAL_MAX_LEN);
        sys_memcpy(ble_diss_val.model_num, p_param->model_num.p_data, ble_diss_val.model_num_len);
    }

    if (p_param->serial_num.len) {
        ble_diss_val.serial_num_len = ble_min(p_param->serial_num.len, BLE_DIS_VAL_MAX_LEN);
        sys_memcpy(ble_diss_val.serial_num, p_param->serial_num.p_data, ble_diss_val.serial_num_len);
    }

    if (p_param->hw_rev.len) {
        ble_diss_val.hw_rev_len = ble_min(p_param->hw_rev.len, BLE_DIS_VAL_MAX_LEN);
        sys_memcpy(ble_diss_val.hw_rev, p_param->hw_rev.p_data, ble_diss_val.hw_rev_len);
    }

    if (p_param->fw_rev.len) {
        ble_diss_val.fw_rev_len = ble_min(p_param->fw_rev.len, BLE_DIS_VAL_MAX_LEN);
        sys_memcpy(ble_diss_val.fw_rev, p_param->fw_rev.p_data, ble_diss_val.fw_rev_len);
    }

    if (p_param->sw_rev.len) {
        ble_diss_val.sw_rev_len = ble_min(p_param->sw_rev.len, BLE_DIS_VAL_MAX_LEN);
        sys_memcpy(ble_diss_val.sw_rev, p_param->sw_rev.p_data, ble_diss_val.sw_rev_len);
    }

    if (p_param->ieee_data.len) {
        ble_diss_val.ieee_data_len = ble_min(p_param->ieee_data.len, BLE_DIS_VAL_MAX_LEN);
        sys_memcpy(ble_diss_val.ieee_data, p_param->ieee_data.p_data, ble_diss_val.ieee_data_len);
    }

    if (p_param->p_sys_id) {
        ble_diss_val.sys_id[0] = (p_param->p_sys_id->manufact_id & 0x00000000FF);
        ble_diss_val.sys_id[1] = (p_param->p_sys_id->manufact_id & 0x000000FF00) >> 8;
        ble_diss_val.sys_id[2] = (p_param->p_sys_id->manufact_id & 0x0000FF0000) >> 16;
        ble_diss_val.sys_id[3] = (p_param->p_sys_id->manufact_id & 0x00FF000000) >> 24;
        ble_diss_val.sys_id[4] = (p_param->p_sys_id->manufact_id & 0xFF00000000) >> 32;

        ble_diss_val.sys_id[5] = (p_param->p_sys_id->oui & 0x0000FF);
        ble_diss_val.sys_id[6] = (p_param->p_sys_id->oui & 0x00FF00) >> 8;
        ble_diss_val.sys_id[7] = (p_param->p_sys_id->oui & 0xFF0000) >> 16;
    }

    if (p_param->p_pnp_id) {
        ble_diss_val.pnp_id[0] = p_param->p_pnp_id->vendor_id_source;
        ble_diss_val.pnp_id[1] = p_param->p_pnp_id->vendor_id & 0x00FF;
        ble_diss_val.pnp_id[2] = (p_param->p_pnp_id->vendor_id & 0xFF00) >> 8;
        ble_diss_val.pnp_id[3] = p_param->p_pnp_id->product_id & 0x00FF;
        ble_diss_val.pnp_id[4] = (p_param->p_pnp_id->product_id & 0xFF00) >> 8;
        ble_diss_val.pnp_id[5] = p_param->p_pnp_id->product_version & 0x00FF;
        ble_diss_val.pnp_id[6] = (p_param->p_pnp_id->product_version & 0xFF00) >> 8;
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Deinit Device Information Service Server
    \param[in]  nont
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_diss_deinit(void)
{
    return ble_gatts_svc_rmv(ble_diss_svc_id);
}
