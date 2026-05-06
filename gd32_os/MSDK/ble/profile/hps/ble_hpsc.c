/*!
    \file    ble_hpsc.c
    \brief   Implementation of http proxy service client .

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
#include "ble_hpsc.h"
#include "wrapper_os.h"
#include "ble_conn.h"
#include "dbg_print.h"
#include "dlist.h"
#include "ble_sec.h"
#include "ble_gattc.h"
#include "ble_gatt.h"

/* Http Proxy Service Client environment variable */
typedef struct
{
    dlist_t             dev_list;
    ble_hpsc_callbacks_t callbacks;
} hpsc_env_t;

/* Http Proxy Service device information */
typedef struct
{
    dlist_t             list;
    uint8_t             conn_id;
    bool                cccd_reg;
} hpsc_dev_t;

static hpsc_env_t hps_env;

/*!
    \brief      Allocate Http Proxy Service device structor
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     hpsc_dev_t *: pointer to the Http Proxy Service device structor
*/
static hpsc_dev_t *hpsc_alloc_dev_by_conn_id(uint8_t conn_id)
{
    hpsc_dev_t *p_device = NULL;

    p_device = (hpsc_dev_t *)sys_malloc(sizeof(hpsc_dev_t));

    if (p_device == NULL) {
        dbg_print(ERR, "hpsc_alloc_dev_by_conn_id alloc device fail! \r\n");
        return NULL;
    }

    sys_memset(p_device, 0, sizeof(hpsc_dev_t));
    //FIX TODO load storage

    INIT_DLIST_HEAD(&p_device->list);
    p_device->conn_id = conn_id;

    list_add_tail(&p_device->list, &hps_env.dev_list);
    return p_device;
}

/*!
    \brief      Find Http Proxy Service device structor
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     hpsc_dev_t *: pointer to the Http Proxy Service device structor found
*/
static hpsc_dev_t *hpsc_find_dev_by_conn_id(uint8_t conn_id)
{
    dlist_t *pos, *n;
    hpsc_dev_t *p_device;

    if (list_empty(&hps_env.dev_list)) {
        return NULL;
    }

    list_for_each_safe(pos, n, &hps_env.dev_list) {
        p_device = list_entry(pos, hpsc_dev_t, list);
        if (p_device->conn_id == conn_id) {
            return p_device;
        }
    }

    return NULL;
}

/*!
    \brief      Find Http Proxy Service device structor, if no such device, allocate one
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     hpsc_dev_t *: pointer to the Http Proxy Service device structor found ot allocated
*/
static hpsc_dev_t *hpsc_find_alloc_dev_by_conn_id(uint8_t conn_id)
{
    hpsc_dev_t *p_device = hpsc_find_dev_by_conn_id(conn_id);

    if (p_device == NULL) {
        p_device = hpsc_alloc_dev_by_conn_id(conn_id);
    }

    return p_device;
}

/*!
    \brief      Remove Http Proxy Service device structor
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     none
*/
static void hpsc_remove_dev_by_conn_id(uint8_t conn_id)
{
    dlist_t *pos, *n;
    hpsc_dev_t *p_device = NULL;
    bool found = false;

    if (list_empty(&hps_env.dev_list)) {
        return;
    }

    list_for_each_safe(pos, n, &hps_env.dev_list) {
        p_device = list_entry(pos, hpsc_dev_t, list);
        if (p_device->conn_id == conn_id) {
            found = true;
            break;
        }
    }

    if (found) {
        list_del(&p_device->list);
        sys_mfree(p_device);
    }
}

/*!
    \brief      Register status code character CCCD
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     none
*/
static void ble_hpsc_reg_status_code_cccd(uint8_t conn_id)
{
    uint8_t value = 1;
    ble_gattc_uuid_info_t svc_uuid;
    ble_gattc_uuid_info_t char_uuid;
    ble_gattc_uuid_info_t desc_uuid;
    uint16_t handle;

    svc_uuid.instance_id = 0;
    svc_uuid.ble_uuid.type = BLE_UUID_TYPE_16;
    svc_uuid.ble_uuid.data.uuid_16 = BLE_GATT_SVC_HTTP_PROXY;

    char_uuid.instance_id = 0;
    char_uuid.ble_uuid.type = BLE_UUID_TYPE_16;
    char_uuid.ble_uuid.data.uuid_16 = BLE_GATT_CHAR_HPS_STATUS_CODE;

    desc_uuid.instance_id = 0;
    desc_uuid.ble_uuid.type = BLE_UUID_TYPE_16;
    desc_uuid.ble_uuid.data.uuid_16 = BLE_GATT_DESC_CLIENT_CHAR_CFG;

    if (ble_gattc_find_desc_handle(conn_id, &svc_uuid, &char_uuid, &desc_uuid,
                                   &handle) != BLE_ERR_NO_ERROR) {
        dbg_print(ERR, "ble_hpsc_reg_status_code_cccd find error \r\n");
        return;
    }

    ble_gattc_write_req(conn_id, handle, 1, &value);
}

/*!
    \brief      Callback function to handle GATT client messages
    \param[in]  p_msg_info: pointer to the GATT client message information
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_hpsc_client_callback(ble_gattc_msg_info_t *p_msg_info)
{
    hpsc_dev_t *p_device = NULL;

    if (p_msg_info->cli_msg_type == BLE_CLI_EVT_GATT_OPERATION) {
        p_device = hpsc_find_dev_by_conn_id(p_msg_info->msg_data.gattc_op_info.conn_idx);

        switch (p_msg_info->msg_data.gattc_op_info.gattc_op_sub_evt) {
        case BLE_CLI_EVT_SVC_DISC_DONE_RSP: {
            if (p_msg_info->msg_data.gattc_op_info.gattc_op_data.svc_dis_done_ind.is_found &&
                p_device == NULL) {
                p_device = hpsc_alloc_dev_by_conn_id(p_msg_info->msg_data.gattc_op_info.conn_idx);

                if (p_device && !p_device->cccd_reg) {
                    ble_hpsc_reg_status_code_cccd(p_device->conn_id);
                }
            }
        }
        break;

        case BLE_CLI_EVT_READ_RSP: {
            if (p_device && hps_env.callbacks.read_cb) {
                hps_read_result_t read_result;
                if (p_msg_info->msg_data.gattc_op_info.gattc_op_data.read_rsp.char_uuid.data.uuid_16 ==
                    BLE_GATT_CHAR_HPS_URI) {
                    read_result.type = HTTP_URI;
                } else if (p_msg_info->msg_data.gattc_op_info.gattc_op_data.read_rsp.char_uuid.data.uuid_16 ==
                           BLE_GATT_CHAR_HPS_HEADERS) {
                    read_result.type = HTTP_HEADERS;
                } else {
                    read_result.type = HTTP_ENTITY_BODY;
                }
                read_result.p_value = p_msg_info->msg_data.gattc_op_info.gattc_op_data.read_rsp.p_value;
                read_result.value_len = p_msg_info->msg_data.gattc_op_info.gattc_op_data.read_rsp.length;

                hps_env.callbacks.read_cb(p_device->conn_id, read_result);
            }
        }
        break;

        case BLE_CLI_EVT_WRITE_RSP: {
            if (p_device && hps_env.callbacks.write_cb) {
                hps_write_result_t write_result;
                if (p_msg_info->msg_data.gattc_op_info.gattc_op_data.write_rsp.char_uuid.data.uuid_16 ==
                    BLE_GATT_CHAR_HPS_URI) {
                    write_result.type = HTTP_URI;
                } else if (p_msg_info->msg_data.gattc_op_info.gattc_op_data.write_rsp.char_uuid.data.uuid_16 ==
                           BLE_GATT_CHAR_HPS_HEADERS) {
                    write_result.type = HTTP_HEADERS;
                } else if (p_msg_info->msg_data.gattc_op_info.gattc_op_data.write_rsp.char_uuid.data.uuid_16 ==
                           BLE_GATT_CHAR_HPS_ENTITY_BODY) {
                    write_result.type = HTTP_ENTITY_BODY;
                } else if (p_msg_info->msg_data.gattc_op_info.gattc_op_data.write_rsp.char_uuid.data.uuid_16 ==
                           BLE_GATT_CHAR_HPS_STATUS_CODE) {
                    p_device->cccd_reg = true;
                    return BLE_ERR_NO_ERROR;
                }

                write_result.status = p_msg_info->msg_data.gattc_op_info.gattc_op_data.write_rsp.status;

                hps_env.callbacks.write_cb(p_device->conn_id, write_result);
            }
        }
        break;

        case BLE_CLI_EVT_NTF_IND_RCV: {
            if (p_device && hps_env.callbacks.ntf_ind_cb) {
                hps_status_code_ind_t ind_result;
                sys_memcpy(ind_result.status_code, p_msg_info->msg_data.gattc_op_info.gattc_op_data.ntf_ind.p_value,
                           HPS_STATUS_CODE_LEN);

                hps_env.callbacks.ntf_ind_cb(p_device->conn_id, ind_result);
            }
        }
        break;

        default:
            break;

        }

    } else if (p_msg_info->cli_msg_type == BLE_CLI_EVT_CONN_STATE_CHANGE_IND) {
        if (p_msg_info->msg_data.conn_state_change_ind.conn_state == BLE_CONN_STATE_DISCONNECTD) {
            hpsc_remove_dev_by_conn_id(p_msg_info->msg_data.conn_state_change_ind.info.disconn_info.conn_idx);
        }
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Write character value
    \param[in]  conn_id: connection index
    \param[in]  p_value: pointer to the value
    \param[in]  value_len: value length
    \param[in]  type: character type
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_hpsc_write_char_value(uint8_t conn_id, uint8_t *p_value, uint16_t value_len,
                                       ble_hps_char_type type)
{
    hpsc_dev_t *p_device = hpsc_find_dev_by_conn_id(conn_id);
    ble_gattc_uuid_info_t svc_uuid;
    ble_gattc_uuid_info_t char_uuid;
    uint16_t handle;

    if (p_device == NULL) {
        return BLE_ERR_NO_RESOURCES;
    }

    svc_uuid.instance_id = 0;
    svc_uuid.ble_uuid.type = BLE_UUID_TYPE_16;
    svc_uuid.ble_uuid.data.uuid_16 = BLE_GATT_SVC_HTTP_PROXY;

    char_uuid.instance_id = 0;
    char_uuid.ble_uuid.type = BLE_UUID_TYPE_16;

    if (type == HTTP_URI) {
        char_uuid.ble_uuid.data.uuid_16 = BLE_GATT_CHAR_HPS_URI;
    } else if (type == HTTP_HEADERS) {
        char_uuid.ble_uuid.data.uuid_16 = BLE_GATT_CHAR_HPS_HEADERS;
    } else {
        char_uuid.ble_uuid.data.uuid_16 = BLE_GATT_CHAR_HPS_ENTITY_BODY;
    }

    if (ble_gattc_find_char_handle(conn_id, &svc_uuid, &char_uuid, &handle) != BLE_ERR_NO_ERROR) {
        return BLE_ERR_PROCESSING;
    }

    value_len = value_len > BLE_HPS_VAL_MAX_LEN ? BLE_HPS_VAL_MAX_LEN : value_len;

    return ble_gattc_write_req(conn_id, handle, value_len, p_value);
}

/*!
    \brief      Write Http Proxy Service control point character
    \param[in]  conn_id: connection index
    \param[in]  value: HPS control point operation code
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_hpsc_write_ctrl_point(uint8_t conn_id, ble_hps_op_code_t value)
{
    hpsc_dev_t *p_device = hpsc_find_dev_by_conn_id(conn_id);
    ble_gattc_uuid_info_t svc_uuid;
    ble_gattc_uuid_info_t char_uuid;
    uint16_t handle;

    if (p_device == NULL) {
        return BLE_ERR_NO_RESOURCES;
    }

    svc_uuid.instance_id = 0;
    svc_uuid.ble_uuid.type = BLE_UUID_TYPE_16;
    svc_uuid.ble_uuid.data.uuid_16 = BLE_GATT_SVC_HTTP_PROXY;

    char_uuid.instance_id = 0;
    char_uuid.ble_uuid.type = BLE_UUID_TYPE_16;
    char_uuid.ble_uuid.data.uuid_16 = BLE_GATT_CHAR_HPS_CTRL_POINT;

    if (ble_gattc_find_char_handle(conn_id, &svc_uuid, &char_uuid, &handle) != BLE_ERR_NO_ERROR) {
        return BLE_ERR_PROCESSING;
    }

    return ble_gattc_write_req(conn_id, handle, 1, (uint8_t *)&value);
}

/*!
    \brief      Read Http Proxy Service character
    \param[in]  conn_id: onnection index
    \param[in]  type: character type
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_hpsc_read_char_value(uint8_t conn_id, ble_hps_char_type type)
{
    hpsc_dev_t *p_device = hpsc_find_dev_by_conn_id(conn_id);
    ble_gattc_uuid_info_t svc_uuid;
    ble_gattc_uuid_info_t char_uuid;
    uint16_t handle;

    if (p_device == NULL) {
        return BLE_ERR_NO_RESOURCES;
    }

    svc_uuid.instance_id = 0;
    svc_uuid.ble_uuid.type = BLE_UUID_TYPE_16;
    svc_uuid.ble_uuid.data.uuid_16 = BLE_GATT_SVC_HTTP_PROXY;

    char_uuid.instance_id = 0;
    char_uuid.ble_uuid.type = BLE_UUID_TYPE_16;

    if (type == HTTP_URI) {
        char_uuid.ble_uuid.data.uuid_16 = BLE_GATT_CHAR_HPS_URI;
    } else if (type == HTTP_HEADERS) {
        char_uuid.ble_uuid.data.uuid_16 = BLE_GATT_CHAR_HPS_HEADERS;
    } else if (type == HTTP_ENTITY_BODY) {
        char_uuid.ble_uuid.data.uuid_16 = BLE_GATT_CHAR_HPS_ENTITY_BODY;
    } else if (type == HTTP_SECURITY) {
        char_uuid.ble_uuid.data.uuid_16 = BLE_GATT_CHAR_HPS_SECURITY;
    } else {
        return BLE_ERR_PROCESSING;
    }

    if (ble_gattc_find_char_handle(conn_id, &svc_uuid, &char_uuid, &handle) != BLE_ERR_NO_ERROR) {
        dbg_print(NOTICE, "ble_hpsc_read_char_value can't find handle \r\n");
        return BLE_ERR_PROCESSING;
    }

    return ble_gattc_read(conn_id, handle, 0, BLE_HPS_VAL_MAX_LEN);
}

 /*!
    \brief      Init Http Proxy Service client
    \param[in]  callbacks: HPS client callback function set
    \param[out] none
    \retval     none
*/
void ble_hpsc_init(ble_hpsc_callbacks_t callbacks)
{
    ble_uuid_t srv_uuid;

    srv_uuid.type = BLE_UUID_TYPE_16;
    srv_uuid.data.uuid_16 = BLE_GATT_SVC_HTTP_PROXY;

    if (ble_gattc_svc_reg(&srv_uuid, ble_hpsc_client_callback) != BLE_ERR_NO_ERROR) {
        return;
    }

    sys_memset(&hps_env, 0, sizeof(hpsc_env_t));
    INIT_DLIST_HEAD(&hps_env.dev_list);
    hps_env.callbacks = callbacks;
}

