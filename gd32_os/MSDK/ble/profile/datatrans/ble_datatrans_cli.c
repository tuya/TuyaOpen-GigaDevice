/*!
    \file    ble_datatrans_cli.c
    \brief   Implementations of ble datatrans client.

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
#include "ble_error.h"
#include "ble_datatrans_cli.h"
#include "ble_datatrans_common.h"
#include "ble_gattc.h"
#include "dbg_print.h"

/* BLE datatrans client data receive callback function */
static ble_datatrans_cli_rx_cb datatrans_cli_rx_cb = NULL;

/*!
    \brief      BLE datatrans client write characteristic
    \param[in]  conn_idx: connection index
    \param[in]  p_buf: buffer pointer
    \param[in]  len: buffer length
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_cli_write_char(uint8_t conn_idx, uint8_t *p_buf, uint16_t len)
{
    ble_gattc_uuid_info_t svc_uuid_info = {0};
    ble_gattc_uuid_info_t char_uuid_info = {0};
    ble_status_t status = BLE_ERR_NO_ERROR;
    uint16_t char_handle  = 0;

    svc_uuid_info.instance_id = 0;
    svc_uuid_info.ble_uuid.type = BLE_UUID_TYPE_16;
    svc_uuid_info.ble_uuid.data.uuid_16 = BLE_GATT_SVC_DATATRANS_SERVICE;
    char_uuid_info.instance_id = 0;
    char_uuid_info.ble_uuid.type = BLE_UUID_TYPE_16;
    char_uuid_info.ble_uuid.data.uuid_16 = BLE_GATT_SVC_DATATRANS_RX_CHAR;

    status = ble_gattc_find_char_handle(conn_idx, &svc_uuid_info, &char_uuid_info, &char_handle);

    if (status == BLE_ERR_NO_ERROR) {
        status = ble_gattc_write_req(conn_idx, char_handle, len, p_buf);
    }

    return status;
}

/*!
    \brief      BLE datatrans client write cccd
    \param[in]  conn_idx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_cli_write_cccd(uint8_t conn_idx)
{
    ble_gattc_uuid_info_t svc_uuid_info = {0};
    ble_gattc_uuid_info_t char_uuid_info = {0};
    ble_gattc_uuid_info_t desc_uuid_info = {0};
    ble_status_t status = BLE_ERR_NO_ERROR;
    uint16_t handle  = 0;
    uint8_t cccd_buf[BLE_GATT_UUID_16_LEN] = {1, 0};

    svc_uuid_info.instance_id = 0;
    svc_uuid_info.ble_uuid.type = BLE_UUID_TYPE_16;
    svc_uuid_info.ble_uuid.data.uuid_16 = BLE_GATT_SVC_DATATRANS_SERVICE;
    char_uuid_info.instance_id = 0;
    char_uuid_info.ble_uuid.type = BLE_UUID_TYPE_16;
    char_uuid_info.ble_uuid.data.uuid_16 = BLE_GATT_SVC_DATATRANS_TX_CHAR;

    desc_uuid_info.instance_id = 0;
    desc_uuid_info.ble_uuid.type = BLE_UUID_TYPE_16;
    desc_uuid_info.ble_uuid.data.uuid_16 = BLE_GATT_DESC_CLIENT_CHAR_CFG;

    status = ble_gattc_find_desc_handle(conn_idx, &svc_uuid_info, &char_uuid_info, &desc_uuid_info, &handle);

    if( status != BLE_ERR_NO_ERROR) {
        return status;
    }

    status = ble_gattc_write_req(conn_idx, handle, 2, cccd_buf);

    return status;
}

/*!
    \brief      Callback function to handle GATT client messages
    \param[in]  p_cli_msg_info: pointer to GATT client message information
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_cli_cb(ble_gattc_msg_info_t *p_cli_msg_info)
{
    switch (p_cli_msg_info->cli_msg_type) {
    case BLE_CLI_EVT_CONN_STATE_CHANGE_IND:
        if (p_cli_msg_info->msg_data.conn_state_change_ind.conn_state == BLE_CONN_STATE_DISCONNECTD) {
            dbg_print(NOTICE, "[ble_datatrans_cli_cb] conn_state_change_ind disconnected event, conn_idx = %d, disconn reason = 0x%x\r\n",
                      p_cli_msg_info->msg_data.conn_state_change_ind.info.disconn_info.conn_idx,
                      p_cli_msg_info->msg_data.conn_state_change_ind.info.disconn_info.reason);
        } else if (p_cli_msg_info->msg_data.conn_state_change_ind.conn_state == BLE_CONN_STATE_CONNECTED) {
            dbg_print(NOTICE, "[ble_datatrans_cli_cb] conn_state_change_ind connected event, conn_idx = %d\r\n",
                      p_cli_msg_info->msg_data.conn_state_change_ind.info.conn_info.conn_idx);
        }
        break;

    case BLE_CLI_EVT_GATT_OPERATION:
        switch (p_cli_msg_info->msg_data.gattc_op_info.gattc_op_sub_evt) {
        case BLE_CLI_EVT_SVC_DISC_DONE_RSP:
            ble_datatrans_cli_write_cccd(p_cli_msg_info->msg_data.gattc_op_info.conn_idx);
            dbg_print(NOTICE, "[ble_datatrans_cli_cb] discovery result = %d, svc_instance_num = %d\r\n",
                      p_cli_msg_info->msg_data.gattc_op_info.gattc_op_data.svc_dis_done_ind.is_found,
                      p_cli_msg_info->msg_data.gattc_op_info.gattc_op_data.svc_dis_done_ind.svc_instance_num);
            break;

        case BLE_CLI_EVT_WRITE_RSP: {
            ble_gattc_write_rsp_t *p_rsp = &p_cli_msg_info->msg_data.gattc_op_info.gattc_op_data.write_rsp;
            if (p_rsp->char_uuid.data.uuid_16 == BLE_GATT_SVC_DATATRANS_RX_CHAR) {
                //add user code
            }
        } break;

        case BLE_CLI_EVT_NTF_IND_RCV: {
            ble_gattc_ntf_ind_t *p_ntf_ind = &p_cli_msg_info->msg_data.gattc_op_info.gattc_op_data.ntf_ind;

            if (p_ntf_ind->char_uuid.data.uuid_16 == BLE_GATT_SVC_DATATRANS_TX_CHAR) {
                if (datatrans_cli_rx_cb)
                    datatrans_cli_rx_cb(p_cli_msg_info->msg_data.gattc_op_info.conn_idx, p_ntf_ind->length, p_ntf_ind->p_value);
            }
        } break;

        default:
            break;
        }
        break;
    default:
        break;
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      BLE datatrans client service rx callback register
    \param[in]  callback: datatrans client callback function
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_cli_rx_cb_reg(ble_datatrans_cli_rx_cb callback)
{
    datatrans_cli_rx_cb = callback;

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      BLE datatrans client service rx callback unregister
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_cli_rx_cb_unreg(void)
{
    datatrans_cli_rx_cb = NULL;

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Init BLE datatrans client
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_cli_init(void)
{
    ble_uuid_t svc_uuid;

    svc_uuid.type = BLE_UUID_TYPE_16;
    svc_uuid.data.uuid_16 = BLE_GATT_SVC_DATATRANS_SERVICE;

    return ble_gattc_svc_reg(&svc_uuid, ble_datatrans_cli_cb);
}

/*!
    \brief      Deinit BLE datatrans client
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_cli_deinit(void)
{
    ble_uuid_t svc_uuid;

    svc_uuid.type = BLE_UUID_TYPE_16;
    svc_uuid.data.uuid_16 = BLE_GATT_SVC_DATATRANS_SERVICE;

    return ble_gattc_svc_unreg(&svc_uuid);
}


