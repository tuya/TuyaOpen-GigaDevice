/*!
    \file    ble_throughput_srv.c
    \brief   Implementations of ble throughput server

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

#include "ble_throughput_srv.h"
#include "ble_gatts.h"
#include "ble_error.h"
#include "dbg_print.h"
#include "systime.h"

#define BLE_THROUGHPUT_ATT_SERVICE_UUID     BLE_GATT_UUID_16_LSB(0xFFE0)
#define BLE_THROUGHPUT_ATT_WRITE_UUID       BLE_GATT_UUID_16_LSB(0xFFE1)
#define BLE_THROUGHPUT_ATT_MAX_LEN          244

/* BLE throughput server attribute database handle list */
enum ble_throughput_srv_att_idx
{
    BLE_THROUGHPUT_SRV_IDX_SVC,         /*!< BLE throughput Server Service Declaration */

    BLE_THROUGHPUT_SRV_IDX_CHAR,        /*!< BLE throughput Server Service Characteristic Declaration */
    BLE_THROUGHPUT_SRV_IDX_VAL,         /*!< BLE throughput Server Service Characteristic value */
    BLE_THROUGHPUT_SRV_IDX_CCCD,        /*!< BLE throughput Server Service Client Characteristic Configuration Descriptor */

    BLE_THROUGHPUT_SRV_IDX_NB,
};

/* BLE throughput server service ID assigned by GATT server module */
uint8_t throughput_svc_id;
static uint16_t ntf_idx = 0;
static uint16_t ntf_num = 200;
static uint8_t ntf_len = BLE_THROUGHPUT_ATT_MAX_LEN;
static uint64_t ntf_start_time = 0;
static uint8_t ntf_infinite = 0;

/* BLE throughput server service Database Description */
const ble_gatt_attr_desc_t ble_throughput_srv_att_db[BLE_THROUGHPUT_SRV_IDX_NB] = {
    [BLE_THROUGHPUT_SRV_IDX_SVC]  = { UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_PRIMARY_SERVICE) , PROP(RD)            , 0                                           },
    [BLE_THROUGHPUT_SRV_IDX_CHAR] = { UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC)  , PROP(RD)            , 0                                           },
    [BLE_THROUGHPUT_SRV_IDX_VAL]  = { UUID_16BIT_TO_ARRAY(BLE_THROUGHPUT_ATT_WRITE_UUID) , PROP(WC) | PROP(NTF), OPT(NO_OFFSET) | BLE_THROUGHPUT_ATT_MAX_LEN },
    [BLE_THROUGHPUT_SRV_IDX_CCCD] = { UUID_16BIT_TO_ARRAY(BLE_GATT_DESC_CLIENT_CHAR_CFG) , PROP(RD) | PROP(WR) , OPT(NO_OFFSET)                              },
};

ble_status_t ble_throughput_srv_ntf_send(uint8_t conn_idx)
{
    uint8_t data[BLE_THROUGHPUT_ATT_MAX_LEN];

    data[0] = ntf_idx;

    return ble_gatts_ntf_ind_send(conn_idx, throughput_svc_id, BLE_THROUGHPUT_SRV_IDX_VAL, data, ntf_len, BLE_GATT_NOTIFY);
}

/*!
    \brief      BLE throughput server to client.
    \param[in]  conn_idx: connection index
    \param[in]  len:      packet length.
    \param[in]  tx_num:   transmit packet number.
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_throughput_srv_to_cli(uint8_t conn_idx, uint8_t len, uint16_t tx_num, uint8_t infinite)
{
    ble_status_t ret;

    if (len > BLE_THROUGHPUT_ATT_MAX_LEN)
        return BLE_GAP_ERR_INVALID_PARAM;

    ntf_idx = 0;
    ntf_num = tx_num;
    ntf_len = len;
    ntf_start_time = get_sys_local_time_us();
    ntf_infinite = infinite;

    ret = ble_throughput_srv_ntf_send(conn_idx);
    ret |= ble_throughput_srv_ntf_send(conn_idx);
    ret |= ble_throughput_srv_ntf_send(conn_idx);
    ret |= ble_throughput_srv_ntf_send(conn_idx);

    return ret;
}

/*!
    \brief      Callback function to handle GATT server messages
    \param[in]  p_srv_msg_info: pointer to GATT server message information
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_throughput_srv_cb(ble_gatts_msg_info_t *p_srv_msg_info)
{
    uint32_t cost;
    float throughput;

    dbg_print(INFO, "[ble_throughput_srv_cb] srv_msg_type = %d\r\n", p_srv_msg_info->srv_msg_type);

    if (p_srv_msg_info->srv_msg_type == BLE_SRV_EVT_SVC_ADD_RSP) {
        dbg_print(INFO, "[ble_throughput_srv_cb], svc_add_rsp status = 0x%x\r\n", p_srv_msg_info->msg_data.svc_add_rsp.status);
    } else if (p_srv_msg_info->srv_msg_type == BLE_SRV_EVT_GATT_OPERATION){
        if (p_srv_msg_info->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_NTF_IND_SEND_RSP &&
            p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.ntf_ind_send_rsp.status == BLE_ERR_NO_ERROR) {
            ntf_idx++;
            if (ntf_idx == ntf_num) {
                cost = get_sys_local_time_us() - ntf_start_time;
                throughput = (ntf_num * ntf_len * 8) / ((float)cost / 1000);
                printf("ble throughput server to client. num:%u, len(byte):%u, time(us):%u, throughput: %f Kbps\r\n",
                    ntf_num, ntf_len, cost, throughput);
                if (ntf_infinite != 0) {
                    ntf_idx = 0;
                    ntf_start_time = get_sys_local_time_us();
                    ble_throughput_srv_ntf_send(p_srv_msg_info->msg_data.gatts_op_info.conn_idx);
                }
            } else if (ntf_idx < ntf_num) {
                ble_throughput_srv_ntf_send(p_srv_msg_info->msg_data.gatts_op_info.conn_idx);
            }
        } else if (p_srv_msg_info->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_WRITE_REQ) {
            if (p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.write_req.att_idx == BLE_THROUGHPUT_SRV_IDX_CCCD)
                dbg_print(INFO, "[ble_throughput_srv_cb], cccd value = %02x%02x\r\n",
                    p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.write_req.p_val[0],
                    p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.write_req.p_val[1]);
            else if (p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.write_req.att_idx == BLE_THROUGHPUT_SRV_IDX_VAL)
                dbg_print(INFO, "[ble_throughput_srv_cb], write len = %x\r\n", p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.write_req.val_len);
        }
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Init BLE throughput server
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_throughput_srv_init(void)
{
    uint8_t ble_throughput_svc_uuid[BLE_GATT_UUID_128_LEN] = UUID_16BIT_TO_ARRAY(BLE_THROUGHPUT_ATT_SERVICE_UUID);

    ble_gatts_svc_add(&throughput_svc_id, ble_throughput_svc_uuid, 0, 0, ble_throughput_srv_att_db, BLE_THROUGHPUT_SRV_IDX_NB, ble_throughput_srv_cb);
}

/*!
    \brief      Deinit BLE throughput server
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_throughput_srv_deinit(void)
{
    ble_gatts_svc_rmv(throughput_svc_id);
}

