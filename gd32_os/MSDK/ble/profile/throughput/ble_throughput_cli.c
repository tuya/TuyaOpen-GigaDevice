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
#include "ble_error.h"
#include "ble_throughput_cli.h"
#include "ble_gattc.h"
#include "wrapper_os.h"
#include "dbg_print.h"
#include "systime.h"

#define BLE_THROUGHPUT_ATT_SERVICE_UUID     BLE_GATT_UUID_16_LSB(0xFFE0)
#define BLE_THROUGHPUT_ATT_WRITE_UUID       BLE_GATT_UUID_16_LSB(0xFFE1)
#define BLE_THROUGHPUT_ATT_MAX_LEN          244

static uint16_t char_handle  = 0;
static uint16_t write_idx = 0;
static uint16_t write_num = 200;
static uint8_t write_len = BLE_THROUGHPUT_ATT_MAX_LEN;
static uint64_t write_start_time = 0;
static uint8_t write_infinite = 0;

/*!
    \brief      BLE throughput client write characteristic
    \param[in]  conn_idx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_throughput_cli_write_char(uint8_t conn_idx)
{
    uint8_t write_buf[BLE_THROUGHPUT_ATT_MAX_LEN];

    write_buf[0] = write_idx;

    return ble_gattc_write_cmd(conn_idx, char_handle, write_len, write_buf);
}

/*!
    \brief      BLE throughput client to server.
    \param[in]  conn_idx: connection index
    \param[in]  len:      packet length.
    \param[in]  tx_num:   transmit packet number.
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_throughput_cli_to_srv(uint8_t conn_idx, uint8_t len, uint16_t tx_num, uint8_t infinite)
{
    ble_gattc_uuid_info_t srv_uuid_info = {0};
    ble_gattc_uuid_info_t char_uuid_info = {0};
    ble_status_t status;

    if (len > BLE_THROUGHPUT_ATT_MAX_LEN)
        return BLE_GAP_ERR_INVALID_PARAM;

    if (char_handle == 0) {
        srv_uuid_info.instance_id = 0;
        srv_uuid_info.ble_uuid.type = BLE_UUID_TYPE_16;
        srv_uuid_info.ble_uuid.data.uuid_16 = BLE_THROUGHPUT_ATT_SERVICE_UUID;
        char_uuid_info.instance_id = 0;
        char_uuid_info.ble_uuid.type = BLE_UUID_TYPE_16;
        char_uuid_info.ble_uuid.data.uuid_16 = BLE_THROUGHPUT_ATT_WRITE_UUID;

        status = ble_gattc_find_char_handle(conn_idx, &srv_uuid_info, &char_uuid_info, &char_handle);
        if(status != BLE_ERR_NO_ERROR)
            return status;
    }

    write_idx = 0;
    write_len = len;
    write_num = tx_num;
    write_start_time = get_sys_local_time_us();
    write_infinite = infinite;

    status = ble_throughput_cli_write_char(conn_idx);
    status |= ble_throughput_cli_write_char(conn_idx);
    status |= ble_throughput_cli_write_char(conn_idx);
    status |= ble_throughput_cli_write_char(conn_idx);

    return status;
}

/*!
    \brief      BLE throughput client write CCCD
    \param[in]  conn_idx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_throughput_cli_write_cccd(uint8_t conn_idx)
{
    ble_gattc_uuid_info_t srv_uuid_info = {0};
    ble_gattc_uuid_info_t char_uuid_info = {0};
    ble_gattc_uuid_info_t desc_uuid_info = {0};
    ble_status_t status = BLE_ERR_NO_ERROR;
    uint16_t handle  = 0;
    uint8_t cccd_buf[BLE_GATT_UUID_16_LEN] = {1, 0};

    srv_uuid_info.instance_id = 0;
    srv_uuid_info.ble_uuid.type = BLE_UUID_TYPE_16;
    srv_uuid_info.ble_uuid.data.uuid_16 = BLE_THROUGHPUT_ATT_SERVICE_UUID;
    char_uuid_info.instance_id = 0;
    char_uuid_info.ble_uuid.type = BLE_UUID_TYPE_16;
    char_uuid_info.ble_uuid.data.uuid_16 = BLE_THROUGHPUT_ATT_WRITE_UUID;

    desc_uuid_info.instance_id = 0;
    desc_uuid_info.ble_uuid.type = BLE_UUID_TYPE_16;
    desc_uuid_info.ble_uuid.data.uuid_16 = BLE_GATT_DESC_CLIENT_CHAR_CFG;

    status = ble_gattc_find_desc_handle(conn_idx, &srv_uuid_info, &char_uuid_info, &desc_uuid_info, &handle);
    if( status != BLE_ERR_NO_ERROR)
        return status;

    status = ble_gattc_write_req(conn_idx, handle, BLE_GATT_UUID_16_LEN, cccd_buf);

    return status;
}

/*!
    \brief      Callback function to handle GATT client messages
    \param[in]  p_cli_msg_info: pointer to GATT client message information
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_throughput_cli_cb(ble_gattc_msg_info_t *p_cli_msg_info)
{
    uint32_t cost;

    dbg_print(INFO, "[ble_throughput_cli_cb]cli_msg_type = %d\r\n", p_cli_msg_info->cli_msg_type);

    if (p_cli_msg_info->cli_msg_type == BLE_CLI_EVT_CONN_STATE_CHANGE_IND){
        if (p_cli_msg_info->msg_data.conn_state_change_ind.conn_state == BLE_CONN_STATE_CONNECTED) {
            ble_gattc_mtu_update(p_cli_msg_info->msg_data.conn_state_change_ind.info.conn_info.conn_idx, 0);
        }
    } else if (p_cli_msg_info->cli_msg_type == BLE_CLI_EVT_GATT_OPERATION) {
        if (p_cli_msg_info->msg_data.gattc_op_info.gattc_op_sub_evt == BLE_CLI_EVT_SVC_DISC_DONE_RSP) {
            dbg_print(INFO, "[ble_throughput_cli_cb]svc_dis_done_ind = %d %d\r\n",
                p_cli_msg_info->msg_data.gattc_op_info.gattc_op_data.svc_dis_done_ind.is_found,
                p_cli_msg_info->msg_data.gattc_op_info.gattc_op_data.svc_dis_done_ind.svc_instance_num);

            if (p_cli_msg_info->msg_data.gattc_op_info.gattc_op_data.svc_dis_done_ind.is_found)
                ble_throughput_cli_write_cccd(p_cli_msg_info->msg_data.gattc_op_info.conn_idx);
        } else if (p_cli_msg_info->msg_data.gattc_op_info.gattc_op_sub_evt == BLE_CLI_EVT_WRITE_RSP &&
                   p_cli_msg_info->msg_data.gattc_op_info.gattc_op_data.write_rsp.handle == char_handle &&
                   p_cli_msg_info->msg_data.gattc_op_info.gattc_op_data.write_rsp.status == BLE_ERR_NO_ERROR) {
            write_idx++;
            if (write_idx == write_num) {
                cost = get_sys_local_time_us() - write_start_time;
                float throughput = (write_num * write_len * 8) / ((float)cost / 1000);
                printf("ble throughput client to server. num:%u, len(byte):%u, time(us):%u, throughput: %f Kbps\r\n",
                    write_num, write_len, cost, throughput);
                if (write_infinite != 0) {
                    write_idx = 0;
                    write_start_time = get_sys_local_time_us();
                    ble_throughput_cli_write_char(p_cli_msg_info->msg_data.gattc_op_info.conn_idx);
                }
            } else if (write_idx < write_num) {
                ble_throughput_cli_write_char(p_cli_msg_info->msg_data.gattc_op_info.conn_idx);
            }
        } else if (p_cli_msg_info->msg_data.gattc_op_info.gattc_op_sub_evt == BLE_CLI_EVT_NTF_IND_RCV) {
            dbg_print(INFO, "[ble_throughput_cli_cb] notify receive len=%d\r\n", p_cli_msg_info->msg_data.gattc_op_info.gattc_op_data.ntf_ind.length);
        }
    }

    return BLE_ERR_NO_ERROR;
}


/*!
    \brief      Init BLE throughput client
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_throughput_cli_init(void)
{
    ble_uuid_t srv_uuid;

    srv_uuid.type = BLE_UUID_TYPE_16;
    srv_uuid.data.uuid_16 = BLE_THROUGHPUT_ATT_SERVICE_UUID;

    ble_gattc_svc_reg(&srv_uuid, ble_throughput_cli_cb);

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Deinit BLE throughput client
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_throughput_cli_deinit(void)
{
    ble_uuid_t srv_uuid;

    srv_uuid.type = BLE_UUID_TYPE_16;
    srv_uuid.data.uuid_16 = BLE_THROUGHPUT_ATT_SERVICE_UUID;

    return ble_gattc_svc_unreg(&srv_uuid);
}

