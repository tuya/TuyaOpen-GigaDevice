/*!
    \file    blufi_adapter.c
    \brief   Implemetions of blufi adapter.

    \version 2025-02-05, V1.0.0, firmware for GD32VW55x
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

#include "blufi_adapter.h"
#include "ble_gatts.h"
#include "ble_adapter.h"
#include "ble_adv.h"
#include "ble_conn.h"
#include "wrapper_os.h"
#include "dbg_print.h"

uint8_t     prf_id;
blufi_adapter_env_t blufi_adapter_env = {0};
/* Device name array*/
static char dev_name[] = {DEV_NAME};

/* Blue courier wifi profile attribute database */
const ble_gatt_attr_desc_t blufi_att_db[BLUFI_IDX_NUMBER] = {
    [BLUFI_IDX_PRIM_SVC]   = { UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_PRIMARY_SERVICE), PROP(RD),             0                                 },
    [BLUFI_IDX_CHAR_WRITE] = { UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD),             0                                 },
    [BLUFI_IDX_WRITE]      = { UUID_16BIT_TO_ARRAY(BLUFI_GATT_WRITE_UUID),         PROP(WR),             BLUFI_VALUE_LEN                   },
    [BLUFI_IDX_CHAR_NTF]   = { UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD),             0                                 },
    [BLUFI_IDX_NTF]        = { UUID_16BIT_TO_ARRAY(BLUFI_GATT_NTF_UUID),           PROP(RD) | PROP(NTF), BLUFI_VALUE_LEN                   },
    [BLUFI_IDX_NTF_CFG]    = { UUID_16BIT_TO_ARRAY(BLE_GATT_DESC_CLIENT_CHAR_CFG), PROP(RD) | PROP(WR),  OPT(NO_OFFSET) | sizeof(uint16_t) },
};

/*!
    \brief      Send notify through GATT
    \param[in]  p_val: pointer to notification value to send
    \param[in]  len: notification value length
    \param[out] none
    \retval     none
*/
void blufi_ntf_event_send(uint8_t *p_val, uint16_t len)
{
    if (blufi_adapter_env.ntf_cfg == 0) {
        dbg_print(ERR, "%s fail\r\n", __func__);
        return;
    }

    ble_gatts_ntf_ind_send(blufi_adapter_env.conn_id, prf_id, BLUFI_IDX_NTF, p_val, len, BLE_GATT_NOTIFY);
}

/*!
    \brief      Callback function to handle GATT server messages
    \param[in]  p_srv_msg_info: pointer to GATT server message information
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t blufi_gatts_msg_cb(ble_gatts_msg_info_t *p_srv_msg_info)
{
    uint8_t  att_idx;
    uint8_t *data;
    uint16_t data_len, i;

    if (p_srv_msg_info->srv_msg_type == BLE_SRV_EVT_GATT_OPERATION) {
        /* Other connection operations are ignored */
        if (p_srv_msg_info->msg_data.gatts_op_info.conn_idx != blufi_adapter_env.conn_id) {
            return BLE_ERR_NO_ERROR;
        }

        if (p_srv_msg_info->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_WRITE_REQ) {
            att_idx = p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.write_req.att_idx;
            data = p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.write_req.p_val;
            data_len = p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.write_req.val_len;

            dbg_print(INFO, "%s att idx %d, value_len %d, value: ", __func__, att_idx, data_len);
            for (i = 0; i < data_len; i++)
                dbg_print(INFO, "%02x", data[i]);
            dbg_print(INFO, "\r\n");

            switch (att_idx) {
            case BLUFI_IDX_WRITE:
                btc_blufi_recv_handler(data, data_len);
                break;
            case BLUFI_IDX_NTF_CFG:
                if (data_len == sizeof(uint16_t))
                    blufi_adapter_env.ntf_cfg = *(uint16_t *)data;
                else
                    dbg_print(ERR, "%s ntf cfg invalid length\r\n", __func__);
                break;
            default:
                break;
            }
        }
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle BLE connection events
    \param[in]  event: BLE connection event type
    \param[in]  p_data: pointer to BLE connection event data
    \param[out] none
    \retval     none
*/
void blufi_conn_evt_handler(ble_conn_evt_t event, ble_conn_data_u *p_data)
{
    if (event == BLE_CONN_EVT_STATE_CHG &&
        p_data->conn_state.state == BLE_CONN_STATE_CONNECTED &&
        p_data->conn_state.info.conn_info.actv_idx == blufi_adapter_env.adv_idx) {
        blufi_adapter_env.conn_id = p_data->conn_state.info.conn_info.conn_idx;
        /* reinit parameter */
        blufi_adapter_env.ntf_cfg = 0;

        btc_blufi_profile_init();
    }
}

/*!
    \brief      Blue courier wifi link start advertising
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t blufi_adv_start(void)
{
    ble_data_t adv_data;
    ble_data_t adv_scanrsp_data;
    ble_adv_data_set_t adv;
    ble_adv_data_set_t scan_rsp;
    uint8_t data[BLE_GAP_LEGACY_ADV_MAX_LEN];
    uint8_t index = 0;

    sys_memset(data, 0, BLE_GAP_LEGACY_ADV_MAX_LEN);
    data[index++] = 2;                                       // length
    data[index++] = BLE_AD_TYPE_FLAGS;                       // AD type : flags
    data[index++] = 0x6;                                     // AD value
    data[index++] = DEV_NAME_LEN + 1;
    data[index++] = BLE_AD_TYPE_COMPLETE_LOCAL_NAME;
    memcpy(&data[index], dev_name, DEV_NAME_LEN);
    index += DEV_NAME_LEN;

    adv_data.len = index;
    adv_data.p_data = data;
    adv_scanrsp_data.len = index - 3;
    adv_scanrsp_data.p_data = &data[3]; // not include AD type flags
    adv.data_force = true;
    adv.data.p_data_force = &adv_data;
    scan_rsp.data_force = true;
    scan_rsp.data.p_data_force = &adv_scanrsp_data;

    return ble_adv_start(blufi_adapter_env.adv_idx, &adv, &scan_rsp, NULL);
}

/*!
    \brief      Callback function to handle BLE advertising events
    \param[in]  adv_evt: BLE advertising event type
    \param[in]  p_data: pointer to BLE advertising event data
    \param[in]  p_context: context used when create advertising
    \param[out] none
    \retval     none
*/
static void blufi_adv_mgr_evt_hdlr(ble_adv_evt_t adv_evt, void *p_data, void *p_context)
{
    if (adv_evt == BLE_ADV_EVT_STATE_CHG) {
        ble_adv_state_chg_t *p_chg = (ble_adv_state_chg_t *)p_data;
        ble_adv_state_t old_state = blufi_adapter_env.adv_state;

        dbg_print(NOTICE, "%s state change 0x%x ==> 0x%x, reason 0x%x\r\n", __func__,
            old_state, p_chg->state, p_chg->reason);

        blufi_adapter_env.adv_state = p_chg->state;

        if ((p_chg->state == BLE_ADV_STATE_CREATE) && (old_state == BLE_ADV_STATE_CREATING)) {
            blufi_adapter_env.adv_idx = p_chg->adv_idx;
            blufi_adv_start();
        } else if ((p_chg->reason != BLE_ERR_NO_ERROR) &&  (p_chg->state == BLE_ADV_STATE_IDLE) &&
                   (old_state == BLE_ADV_STATE_CREATING)) {
            ble_conn_callback_unregister(blufi_conn_evt_handler);
        }
    }
}

/*!
    \brief      Blue courier wifi link create advertising
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t blufi_adv_create(void)
{
    ble_adv_param_t adv_param = {0};

    adv_param.param.own_addr_type = BLE_GAP_LOCAL_ADDR_STATIC;
    adv_param.param.type = BLE_GAP_ADV_TYPE_LEGACY;
    adv_param.param.prop = BLE_GAP_ADV_PROP_UNDIR_CONN;
    adv_param.param.filter_pol = BLE_GAP_ADV_ALLOW_SCAN_ANY_CON_ANY;
    adv_param.param.disc_mode = BLE_GAP_ADV_MODE_GEN_DISC;
    adv_param.param.ch_map = 0x07; // Advertising channel map - 37, 38, 39
    adv_param.param.primary_phy = BLE_GAP_PHY_1MBPS;
    adv_param.param.adv_intv_min = 160; // Advertising minimum interval - 100ms (160*0.625ms)
    adv_param.param.adv_intv_max = 160; // Advertising maximum interval - 100ms (160*0.625ms)
    adv_param.restart_after_disconn = true;

    return ble_adv_create(&adv_param, blufi_adv_mgr_evt_hdlr, NULL);
}


/*!
    \brief      Enable blufi service
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t blufi_adapter_init(void)
{
    ble_status_t ret = BLE_ERR_NO_ERROR;
    uint8_t blufi_svc_uuid[16] = UUID_16BIT_TO_ARRAY(BLUFI_GATT_SERVICE_UUID);

    ble_gatts_svc_add(&prf_id, blufi_svc_uuid, 0, 0, blufi_att_db, BLUFI_IDX_NUMBER, blufi_gatts_msg_cb);

    sys_memset(&blufi_adapter_env, 0, sizeof(blufi_adapter_env));
    ble_conn_callback_register(blufi_conn_evt_handler);

    ret = blufi_adv_create();
    if (ret != BLE_ERR_NO_ERROR) {
        dbg_print(ERR, "%s create advertising fail. ret: %x\r\n", __func__, ret);
        ble_conn_callback_unregister(blufi_conn_evt_handler);
    }

    return ret;
}
