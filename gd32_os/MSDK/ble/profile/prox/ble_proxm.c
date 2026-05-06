/*!
    \file    ble_proxm.c
    \brief   Proximity Monitor Profile Implementation.

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
#include "ble_proxm.h"
#include "wrapper_os.h"
#include "ble_conn.h"
#include "dbg_print.h"
#include "dlist.h"
#include "ble_sec.h"
#include "ble_gattc.h"
#include "ble_gatt.h"

/* Client environment variable */
typedef struct
{
    proxm_alert_lvl_t     dft_lvl;
    ble_proxm_callbacks_t callbacks;
} proxm_env_t;

static proxm_env_t proxm_env;

/*!
    \brief      Callback function to handle lls characters related GATT client operation
    \param[in]  p_msg_info: pointer to GATT client message
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_proxm_lls_callback(ble_gattc_msg_info_t *p_msg_info)
{
    if (p_msg_info->cli_msg_type == BLE_CLI_EVT_GATT_OPERATION) {
        switch (p_msg_info->msg_data.gattc_op_info.gattc_op_sub_evt) {
        case BLE_CLI_EVT_SVC_DISC_DONE_RSP: {
            if (p_msg_info->msg_data.gattc_op_info.gattc_op_data.svc_dis_done_ind.is_found) {
                ble_proxm_write_lls_char_value(p_msg_info->msg_data.gattc_op_info.conn_idx, proxm_env.dft_lvl);
                if (proxm_env.callbacks.found_service_cb) {
                    proxm_env.callbacks.found_service_cb(p_msg_info->msg_data.gattc_op_info.conn_idx, true);
                }
            }
        }
        break;

        case BLE_CLI_EVT_READ_RSP: {
            if (proxm_env.callbacks.read_lls_altert_cb) {
                proxm_env.callbacks.read_lls_altert_cb(p_msg_info->msg_data.gattc_op_info.conn_idx,
                                                       *(p_msg_info->msg_data.gattc_op_info.gattc_op_data.read_rsp.p_value));
            }
        }
        break;

        case BLE_CLI_EVT_WRITE_RSP: {
            if (p_msg_info->msg_data.gattc_op_info.gattc_op_data.write_rsp.status != BLE_ERR_NO_ERROR) {
                dbg_print(ERR, "ble_proxm_lls_callback write error 0x%x\r\n",
                          p_msg_info->msg_data.gattc_op_info.gattc_op_data.write_rsp.status);
            }
        }
        break;

        default:
            break;
        }
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle tps characters related GATT client operation
    \param[in]  p_msg_info: pointer to GATT client message
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_proxm_tx_pwr_callback(ble_gattc_msg_info_t *p_msg_info)
{
    if (p_msg_info->cli_msg_type == BLE_CLI_EVT_GATT_OPERATION) {
        switch (p_msg_info->msg_data.gattc_op_info.gattc_op_sub_evt) {
        case BLE_CLI_EVT_READ_RSP: {
            uint8_t tx_power = 0xFF;
            if (p_msg_info->msg_data.gattc_op_info.gattc_op_data.read_rsp.status != BLE_ERR_NO_ERROR) {
                dbg_print(ERR, "ble_proxm_tx_pwr_callback read tx power error 0x%x\r\n",
                          p_msg_info->msg_data.gattc_op_info.gattc_op_data.read_rsp.status);
            } else {
                tx_power = *(p_msg_info->msg_data.gattc_op_info.gattc_op_data.read_rsp.p_value);
            }

            if (proxm_env.callbacks.read_tx_pwr_cb) {
                proxm_env.callbacks.read_tx_pwr_cb(p_msg_info->msg_data.gattc_op_info.conn_idx, tx_power);
            }
        }
        break;

        case BLE_CLI_EVT_WRITE_RSP: {
            if (p_msg_info->msg_data.gattc_op_info.gattc_op_data.write_rsp.status != BLE_ERR_NO_ERROR) {
                dbg_print(ERR, "ble_proxm_tx_pwr_callback write error 0x%x\r\n",
                          p_msg_info->msg_data.gattc_op_info.gattc_op_data.write_rsp.status);
            }
        }
        break;

        default:
            break;
        }
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle ias character related GATT client operation
    \param[in]  p_msg_info: pointer to GATT client message
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_proxm_ias_callback(ble_gattc_msg_info_t *p_msg_info)
{
    if (p_msg_info->cli_msg_type == BLE_CLI_EVT_GATT_OPERATION) {
        switch (p_msg_info->msg_data.gattc_op_info.gattc_op_sub_evt) {
        case BLE_CLI_EVT_WRITE_RSP: {
            if (p_msg_info->msg_data.gattc_op_info.gattc_op_data.write_rsp.status != BLE_ERR_NO_ERROR) {
                dbg_print(ERR, "ble_proxm_ias_callback write error 0x%x\r\n",
                          p_msg_info->msg_data.gattc_op_info.gattc_op_data.write_rsp.status);
            }
        }
        break;

        default:
            break;

        }
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Write lls character
    \param[in]  conn_id: connection index
    \param[in]  alert_lvl: alert level value
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_proxm_write_lls_char_value(uint8_t conn_id, proxm_alert_lvl_t alert_lvl)
{
    ble_gattc_uuid_info_t svc_uuid;
    ble_gattc_uuid_info_t char_uuid;
    uint16_t handle;

    svc_uuid.instance_id = 0;
    svc_uuid.ble_uuid.type = BLE_UUID_TYPE_16;
    svc_uuid.ble_uuid.data.uuid_16 = BLE_GATT_SVC_LINK_LOSS;

    char_uuid.instance_id = 0;
    char_uuid.ble_uuid.type = BLE_UUID_TYPE_16;
    char_uuid.ble_uuid.data.uuid_16 = BLE_GATT_CHAR_ALERT_LEVEL;

    if (ble_gattc_find_char_handle(conn_id, &svc_uuid, &char_uuid, &handle) != BLE_ERR_NO_ERROR) {
        return BLE_ERR_PROCESSING;
    }

    return ble_gattc_write_req(conn_id, handle, 1, (uint8_t *)(&alert_lvl));
}

/*!
    \brief      Write ias character
    \param[in]  conn_id: connection index
    \param[in]  alert_lvl: alert level value
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_proxm_write_ias_char_value(uint8_t conn_id, proxm_alert_lvl_t alert_lvl)
{
    ble_gattc_uuid_info_t svc_uuid;
    ble_gattc_uuid_info_t char_uuid;
    uint16_t handle;

    svc_uuid.instance_id = 0;
    svc_uuid.ble_uuid.type = BLE_UUID_TYPE_16;
    svc_uuid.ble_uuid.data.uuid_16 = BLE_GATT_SVC_IMMEDIATE_ALERT;

    char_uuid.instance_id = 0;
    char_uuid.ble_uuid.type = BLE_UUID_TYPE_16;
    char_uuid.ble_uuid.data.uuid_16 = BLE_GATT_CHAR_ALERT_LEVEL;


    if (ble_gattc_find_char_handle(conn_id, &svc_uuid, &char_uuid, &handle) != BLE_ERR_NO_ERROR) {
        return BLE_ERR_PROCESSING;
    }

    return ble_gattc_write_cmd(conn_id, handle, 1, (uint8_t *)(&alert_lvl));
}

/*!
    \brief      Read lls character
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_proxm_read_lls_char_value(uint8_t conn_id)
{
    ble_gattc_uuid_info_t svc_uuid;
    ble_gattc_uuid_info_t char_uuid;
    uint16_t handle;

    svc_uuid.instance_id = 0;
    svc_uuid.ble_uuid.type = BLE_UUID_TYPE_16;
    svc_uuid.ble_uuid.data.uuid_16 = BLE_GATT_SVC_LINK_LOSS;

    char_uuid.instance_id = 0;
    char_uuid.ble_uuid.type = BLE_UUID_TYPE_16;
    char_uuid.ble_uuid.data.uuid_16 = BLE_GATT_CHAR_ALERT_LEVEL;


    if (ble_gattc_find_char_handle(conn_id, &svc_uuid, &char_uuid, &handle) != BLE_ERR_NO_ERROR) {
        dbg_print(NOTICE, "ble_proxm_read_lls_char_value can't find handle \r\n");
        return BLE_ERR_PROCESSING;
    }

    return ble_gattc_read(conn_id, handle, 0, 1);
}

/*!
    \brief      Read tx power character
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_proxm_read_tx_pwr_char_value(uint8_t conn_id)
{
    ble_gattc_uuid_info_t svc_uuid;
    ble_gattc_uuid_info_t char_uuid;
    uint16_t handle;

    svc_uuid.instance_id = 0;
    svc_uuid.ble_uuid.type = BLE_UUID_TYPE_16;
    svc_uuid.ble_uuid.data.uuid_16 = BLE_GATT_SVC_TX_POWER;

    char_uuid.instance_id = 0;
    char_uuid.ble_uuid.type = BLE_UUID_TYPE_16;
    char_uuid.ble_uuid.data.uuid_16 = BLE_GATT_CHAR_TX_POWER_LEVEL;

    if (ble_gattc_find_char_handle(conn_id, &svc_uuid, &char_uuid, &handle) != BLE_ERR_NO_ERROR) {
        dbg_print(NOTICE, "ble_proxm_read_tx_pwr_char_value can't find handle \r\n");
        return BLE_ERR_PROCESSING;
    }

    return ble_gattc_read(conn_id, handle, 0, 1);
}

/*!
    \brief      Init proximity monitor
    \param[in]  callbacks: proximity callback set
    \param[in]  dft_lvl: default alert level
    \param[out] none
    \retval     none
*/
void ble_proxm_init(ble_proxm_callbacks_t callbacks, proxm_alert_lvl_t dft_lvl)
{
    ble_uuid_t srv_uuid;

    srv_uuid.type = BLE_UUID_TYPE_16;
    srv_uuid.data.uuid_16 = BLE_GATT_SVC_LINK_LOSS;

    if (ble_gattc_svc_reg(&srv_uuid, ble_proxm_lls_callback) != BLE_ERR_NO_ERROR) {
        dbg_print(ERR, "ble_proxm_init register link loss service fail!");
        return;
    }

    srv_uuid.data.uuid_16 = BLE_GATT_SVC_IMMEDIATE_ALERT;
    ble_gattc_svc_reg(&srv_uuid, ble_proxm_ias_callback);

    srv_uuid.data.uuid_16 = BLE_GATT_SVC_TX_POWER;
    ble_gattc_svc_reg(&srv_uuid, ble_proxm_tx_pwr_callback);

    sys_memset(&proxm_env, 0, sizeof(proxm_env_t));
    proxm_env.callbacks = callbacks;
    proxm_env.dft_lvl = dft_lvl;
}

