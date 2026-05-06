/*!
    \file    ble_sample_cli.c
    \brief   Implementations of ble sample client.

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

#include "ble_sample_cli.h"
#include "ble_gattc.h"
#include "wrapper_os.h"
#include "dbg_print.h"

/* BLE sample client service UUID */
#define UUID_SAMPLE_SERVICE_128                {0xEF,0xCD,0xAB,0x89,0x67,0x45,0x23,0x01,0x00,0x00,0x00,0x00,0x11,0x11,0x00,0x00}

/* BLE sample client read characteristic UUID */
#define UUID_SAMPLE_READ_CHARACTERISTIC_128    {0xEF,0xCD,0xAB,0x89,0x67,0x45,0x23,0x01,0x00,0x00,0x00,0x00,0x22,0x22,0x00,0x00}

/* BLE sample client write characteristic UUID */
#define UUID_SAMPLE_WRITE_CHARACTERISTIC_128   {0xEF,0xCD,0xAB,0x89,0x67,0x45,0x23,0x01,0x00,0x00,0x00,0x00,0x33,0x33,0x00,0x00}

/* BLE sample client notify characteristic UUID */
#define UUID_SAMPLE_NOTIFY_CHARACTERISTIC_128  {0xEF,0xCD,0xAB,0x89,0x67,0x45,0x23,0x01,0x00,0x00,0x00,0x00,0x44,0x44,0x00,0x00}



/* BLE sample client service UUID array */
const uint8_t ble_sample_cli_uuid[BLE_GATT_UUID_128_LEN] = UUID_SAMPLE_SERVICE_128;

/* BLE sample client read characteristic UUID array */
const uint8_t ble_sample_cli_read_char_uuid[BLE_GATT_UUID_128_LEN] = UUID_SAMPLE_READ_CHARACTERISTIC_128;

/* BLE sample client write characteristic UUID array */
const uint8_t ble_sample_cli_write_char_uuid[BLE_GATT_UUID_128_LEN] = UUID_SAMPLE_WRITE_CHARACTERISTIC_128;

/* BLE sample client notify characteristic UUID array */
const uint8_t ble_sample_cli_notify_char_uuid[BLE_GATT_UUID_128_LEN] = UUID_SAMPLE_NOTIFY_CHARACTERISTIC_128;



/*!
    \brief      BLE sample client read characteristic
    \param[in]  conn_idx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_sample_cli_read_char(uint8_t conn_idx)
{
    ble_gattc_uuid_info_t srv_uuid_info = {0};
    ble_gattc_uuid_info_t char_uuid_info = {0};
    ble_status_t status = BLE_ERR_NO_ERROR;
    uint16_t char_handle  = 0;

    srv_uuid_info.instance_id = 0;
    srv_uuid_info.ble_uuid.type = BLE_UUID_TYPE_128;
    memcpy(srv_uuid_info.ble_uuid.data.uuid_128, ble_sample_cli_uuid, BLE_GATT_UUID_128_LEN);
    char_uuid_info.instance_id = 0;
    char_uuid_info.ble_uuid.type = BLE_UUID_TYPE_128;
    memcpy(char_uuid_info.ble_uuid.data.uuid_128, ble_sample_cli_read_char_uuid, BLE_GATT_UUID_128_LEN);

    status = ble_gattc_find_char_handle(conn_idx, &srv_uuid_info, &char_uuid_info, &char_handle);

    if( status != BLE_ERR_NO_ERROR) {
        return status;
    }

    status = ble_gattc_read(conn_idx, char_handle, 0, 0);

    return status;
}

/*!
    \brief      BLE sample client write characteristic
    \param[in]  conn_idx: connection index
    \param[in]  len: data length
    \param[in]  p_data: pointer to data to send
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_sample_cli_write_char(uint8_t conn_idx, uint16_t len, uint8_t *p_data)
{
    ble_gattc_uuid_info_t srv_uuid_info = {0};
    ble_gattc_uuid_info_t char_uuid_info = {0};
    ble_status_t status = BLE_ERR_NO_ERROR;
    uint16_t char_handle  = 0;

    srv_uuid_info.instance_id = 0;
    srv_uuid_info.ble_uuid.type = BLE_UUID_TYPE_128;
    memcpy(srv_uuid_info.ble_uuid.data.uuid_128, ble_sample_cli_uuid, BLE_GATT_UUID_128_LEN);
    char_uuid_info.instance_id = 0;
    char_uuid_info.ble_uuid.type = BLE_UUID_TYPE_128;
    memcpy(char_uuid_info.ble_uuid.data.uuid_128, ble_sample_cli_write_char_uuid, BLE_GATT_UUID_128_LEN);

    status = ble_gattc_find_char_handle(conn_idx, &srv_uuid_info, &char_uuid_info, &char_handle);

    if( status != BLE_ERR_NO_ERROR) {
        return status;
    }

    status = ble_gattc_write_req(conn_idx, char_handle, len, p_data);

    if (BLE_ERR_NO_ERROR != status)
        dbg_print(NOTICE, "[ble_sample_cli_write_char] write fail, status: 0x%02x\r\n", status);

    return status;
}

/*!
    \brief      BLE sample client write CCCD
    \param[in]  conn_idx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_sample_cli_write_cccd(uint8_t conn_idx)
{
    ble_gattc_uuid_info_t srv_uuid_info = {0};
    ble_gattc_uuid_info_t char_uuid_info = {0};
    ble_gattc_uuid_info_t desc_uuid_info = {0};
    ble_status_t status = BLE_ERR_NO_ERROR;
    uint16_t handle  = 0;
    uint8_t cccd_buf[BLE_GATT_UUID_16_LEN] = {1, 0};

    srv_uuid_info.instance_id = 0;
    srv_uuid_info.ble_uuid.type = BLE_UUID_TYPE_128;
    memcpy(srv_uuid_info.ble_uuid.data.uuid_128, ble_sample_cli_uuid, BLE_GATT_UUID_128_LEN);
    char_uuid_info.instance_id = 0;
    char_uuid_info.ble_uuid.type = BLE_UUID_TYPE_128;
    memcpy(char_uuid_info.ble_uuid.data.uuid_128, ble_sample_cli_notify_char_uuid, BLE_GATT_UUID_128_LEN);

    desc_uuid_info.instance_id = 0;
    desc_uuid_info.ble_uuid.type = BLE_UUID_TYPE_16;
    desc_uuid_info.ble_uuid.data.uuid_16 = BLE_GATT_DESC_CLIENT_CHAR_CFG;

    status = ble_gattc_find_desc_handle(conn_idx, &srv_uuid_info, &char_uuid_info, &desc_uuid_info, &handle);

    if( status != BLE_ERR_NO_ERROR) {
        return status;
    }

    status = ble_gattc_write_req(conn_idx, handle, 2, cccd_buf);

    return status;
}

/*!
    \brief      Handle BLE connection disconnected event
    \param[in]  conn_idx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_sample_cli_on_disconnect(uint8_t conn_idx, uint16_t reason)
{
    dbg_print(NOTICE, "[ble_sample_cli_on_disconnect] conn_state_change_ind disconnected event, conn_idx = %d, disconn reason = 0x%x\r\n", conn_idx, reason);

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Handle BLE connection connected event
    \param[in]  conn_idx: connection index
    \param[in]  p_addr: pointer to peer address information
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_sample_cli_on_connect(uint8_t conn_idx, ble_gap_addr_t *p_addr)
{
    dbg_print(NOTICE, "[ble_sample_cli_on_connect] conn_state_change_ind connected event, conn_idx = %d\r\n", conn_idx);

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle gattc read event
    \param[in]  conn_idx: connection index
    \param[in]  p_rsp: pointer to gattc read respond information data
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_sample_cli_read_cb(uint8_t conn_idx, ble_gattc_read_rsp_t *p_rsp)
{
    if (!memcmp(p_rsp->char_uuid.data.uuid_128, ble_sample_cli_read_char_uuid, BLE_GATT_UUID_128_LEN)) {
        uint16_t i;

        dbg_print(NOTICE,"[ble_sample_cli_read_cb] status: 0x%x, read hdl: %04x, value_length: %d, value: ",
            p_rsp->status, p_rsp->handle, p_rsp->length);
        for (i = 0; i < p_rsp->length; i++) {
            dbg_print(NOTICE,"0x%02x ", p_rsp->p_value[i]);
        }
        dbg_print(NOTICE,"\r\n");
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle gattc write event
    \param[in]  conn_idx: connection index
    \param[in]  p_req: pointer to gattc write respond information data
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_sample_cli_write_cb(uint8_t conn_idx, ble_gattc_write_rsp_t *p_rsp)
{
    if (!memcmp(p_rsp->char_uuid.data.uuid_128, ble_sample_cli_write_char_uuid, BLE_GATT_UUID_128_LEN)) {
        dbg_print(NOTICE,"[ble_sample_cli_write_cb] status: 0x%x; write hdl: %04x\r\n,", p_rsp->status, p_rsp->handle);
    }
    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle gattc notification/indication receive event
    \param[in]  conn_idx: connection index
    \param[in]  p_ntf_ind: pointer to gattc notification/indication receive information data
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_sample_cli_ntf_ind_rcv(uint8_t conn_idx, ble_gattc_ntf_ind_t *p_ntf_ind)
{
    if (!memcmp(p_ntf_ind->char_uuid.data.uuid_128, ble_sample_cli_notify_char_uuid, BLE_GATT_UUID_128_LEN)) {
        uint16_t i;

        dbg_print(NOTICE,"[ble_sample_cli_ntf_ind_rcv] ntf_ind hdl: %04x, value_length: %d, value: ", p_ntf_ind->handle, p_ntf_ind->length);

        for (i = 0; i < p_ntf_ind->length; i++) {
            dbg_print(NOTICE,"0x%02x ", p_ntf_ind->p_value[i]);
        }

        dbg_print(NOTICE,"\r\n");
    }
    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle BLE_CLI_EVT_GATT_OPERATION event
    \param[in]  p_info: pointer to gatts operation information data
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_sample_cli_handle_gattc_op(ble_gattc_op_info_t *p_info)
{
    uint8_t conn_idx = p_info->conn_idx;

    switch (p_info->gattc_op_sub_evt) {
    case BLE_CLI_EVT_SVC_DISC_DONE_RSP:
        dbg_print(NOTICE, "[ble_sample_cli_handle_gattc_op] discovery result = %d, svc_instance_num = %d\r\n", p_info->gattc_op_data.svc_dis_done_ind.is_found, p_info->gattc_op_data.svc_dis_done_ind.svc_instance_num);
        break;

    case BLE_CLI_EVT_READ_RSP:
        ble_sample_cli_read_cb(conn_idx, &p_info->gattc_op_data.read_rsp);
        break;

    case BLE_CLI_EVT_WRITE_RSP:
        ble_sample_cli_write_cb(conn_idx, &p_info->gattc_op_data.write_rsp);
        break;

    case BLE_CLI_EVT_NTF_IND_RCV:
        ble_sample_cli_ntf_ind_rcv(conn_idx, &p_info->gattc_op_data.ntf_ind);
        break;

    default:
        break;
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle GATT client messages
    \param[in]  p_cli_msg_info: pointer to GATT client message information
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_sample_cli_cb(ble_gattc_msg_info_t *p_cli_msg_info)
{
    dbg_print(INFO, "[ble_sample_cli_cb]cli_msg_type = %d\r\n", p_cli_msg_info->cli_msg_type);

    switch (p_cli_msg_info->cli_msg_type) {
    case BLE_CLI_EVT_CONN_STATE_CHANGE_IND:
        if (p_cli_msg_info->msg_data.conn_state_change_ind.conn_state == BLE_CONN_STATE_DISCONNECTD) {
            ble_sample_cli_on_disconnect(p_cli_msg_info->msg_data.conn_state_change_ind.info.disconn_info.conn_idx,
                p_cli_msg_info->msg_data.conn_state_change_ind.info.disconn_info.reason);
        } else if (p_cli_msg_info->msg_data.conn_state_change_ind.conn_state == BLE_CONN_STATE_CONNECTED) {
            ble_sample_cli_on_connect(p_cli_msg_info->msg_data.conn_state_change_ind.info.conn_info.conn_idx,
                &p_cli_msg_info->msg_data.conn_state_change_ind.info.conn_info.peer_addr);
        }
        break;

    case BLE_CLI_EVT_GATT_OPERATION:
        ble_sample_cli_handle_gattc_op(&p_cli_msg_info->msg_data.gattc_op_info);
        break;

    default:
        break;
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Function to print UUID information
    \param[in]  p_ble_uuid: UUID value to print
    \param[out] none
    \retval     none
*/
void ble_sample_cli_uuid_print(ble_uuid_t *p_ble_uuid)
{
    switch (p_ble_uuid->type) {
    case  BLE_UUID_TYPE_16:
        dbg_print(INFO,"uuid_type = %d, uuid = 0x%x\r\n", p_ble_uuid->type, p_ble_uuid->data.uuid_16);
        break;

    case  BLE_UUID_TYPE_32:
        dbg_print(INFO,"uuid_type = %d, uuid = 0x%x\r\n", p_ble_uuid->type, p_ble_uuid->data.uuid_32);
        break;

    case  BLE_UUID_TYPE_128:{
        uint8_t i;
        dbg_print(INFO,"uuid_type = %d, uuid = ", p_ble_uuid->type);
        for(i = 0; i < BLE_GATT_UUID_128_LEN; i++) {
            dbg_print(INFO,"%x ", p_ble_uuid->data.uuid_128[i]);
        }
        dbg_print(INFO,"\r\n");
    } break;

    default:
        break;
    }
}

/*!
    \brief      Init BLE sample client
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_sample_cli_init(void)
{
    ble_uuid_t srv_uuid;

    srv_uuid.type = BLE_UUID_TYPE_128;
    memcpy(srv_uuid.data.uuid_128, ble_sample_cli_uuid, BLE_GATT_UUID_128_LEN);

    return ble_gattc_svc_reg(&srv_uuid, ble_sample_cli_cb);
}

/*!
    \brief      Deinit BLE sample client
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_sample_cli_deinit(void)
{
    ble_uuid_t srv_uuid;

    srv_uuid.type = BLE_UUID_TYPE_128;
    memcpy(srv_uuid.data.uuid_128, ble_sample_cli_uuid, BLE_GATT_UUID_128_LEN);

    return ble_gattc_svc_unreg(&srv_uuid);
}
