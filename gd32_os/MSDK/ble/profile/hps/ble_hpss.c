/*!
    \file    ble_hpss.c
    \brief   Implementation of http proxy service server .

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
#include "ble_hpss.h"
#include "wrapper_os.h"
#include "ble_conn.h"
#include "dbg_print.h"
#include "dlist.h"
#include "ble_sec.h"

#define HPS_URI_UPDATE_MASK           0x01
#define HPS_HEADERS_UPDATE_MASK       0x02
#define HPS_BODY_UPDATE_MASK          0x04
#define HPS_UPDATE_ALL                0x07

/* HPS Attribute database handle list */
typedef enum
{
    BLE_HPS_HDL_SVC,

    BLE_HPS_HDL_URI_CHAR,
    BLE_HPS_HDL_URI_VAL,

    BLE_HPS_HDL_HEADER_CHAR,
    BLE_HPS_HDL_HEADER_VAL,

    BLE_HPS_HDL_ENTITY_BODY_CHAR,
    BLE_HPS_HDL_ENTITY_BODY_VAL,

    BLE_HPS_HDL_CTRL_POINT_CHAR,
    BLE_HPS_HDL_CTRL_POINT_VAL,

    BLE_HPS_HDL_SECURITY_CHAR,
    BLE_HPS_HDL_SECURITY_VAL,

    BLE_HPS_HDL_STATUS_CODE_CHAR,
    BLE_HPS_HDL_STATUS_CODE_VAL,
    BLE_HPS_HDL_STATUS_CODE_CHAR_DESC,

    BLE_HPS_HDL_NB,
} ble_hps_attr_db_handle_t;

/* HPS value structure */
typedef struct
{
    uint8_t uri[BLE_HPS_VAL_MAX_LEN];
    uint16_t uri_len;

    uint8_t headers[BLE_HPS_VAL_MAX_LEN];
    uint16_t headers_len;

    uint8_t body[BLE_HPS_VAL_MAX_LEN];
    uint16_t body_len;

    uint8_t update_msk;
    uint8_t ctrl_op_code;
    uint8_t security;
    uint8_t status_code[HPS_STATUS_CODE_LEN];
} ble_hps_value_t;

/* Http Proxy Service Server environment variable */
typedef struct
{
    uint8_t             hps_id;
    dlist_t             dev_list;
    ble_hpss_callbacks_t callback;
} hpss_env_t;

/* Http Proxy Service device information */
typedef struct
{
    dlist_t             list;
    uint8_t             conn_id;
    ble_hps_value_t     hps_att_val;
    bool                op_ongoing;
    bool                cccd_reg;
} hpss_dev_t;

/* HPS Database Description */
const ble_gatt_attr_desc_t ble_hps_attr_db[BLE_HPS_HDL_NB] = {
    [BLE_HPS_HDL_SVC]                 = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_PRIMARY_SERVICE), PROP(RD),            0                  },

    [BLE_HPS_HDL_URI_CHAR]            = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD),            0                  },
    [BLE_HPS_HDL_URI_VAL]             = {UUID_16BIT_TO_ARRAY(BLE_GATT_CHAR_HPS_URI),         PROP(WR),            BLE_HPS_VAL_MAX_LEN},

    [BLE_HPS_HDL_HEADER_CHAR]         = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD),            0                  },
    [BLE_HPS_HDL_HEADER_VAL]          = {UUID_16BIT_TO_ARRAY(BLE_GATT_CHAR_HPS_HEADERS),     PROP(RD) | PROP(WR), BLE_HPS_VAL_MAX_LEN},

    [BLE_HPS_HDL_ENTITY_BODY_CHAR]    = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD),            0                  },
    [BLE_HPS_HDL_ENTITY_BODY_VAL]     = {UUID_16BIT_TO_ARRAY(BLE_GATT_CHAR_HPS_ENTITY_BODY), PROP(RD) | PROP(WR),  BLE_HPS_VAL_MAX_LEN},

    [BLE_HPS_HDL_CTRL_POINT_CHAR]     = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD),            0                  },
    [BLE_HPS_HDL_CTRL_POINT_VAL]      = {UUID_16BIT_TO_ARRAY(BLE_GATT_CHAR_HPS_CTRL_POINT),  PROP(WR),            1                  },

    [BLE_HPS_HDL_SECURITY_CHAR]       = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD),            0                  },
    [BLE_HPS_HDL_SECURITY_VAL]        = {UUID_16BIT_TO_ARRAY(BLE_GATT_CHAR_HPS_SECURITY),    PROP(RD),            1                  },

    [BLE_HPS_HDL_STATUS_CODE_CHAR]    = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD),            0                  },
    [BLE_HPS_HDL_STATUS_CODE_VAL]     = {UUID_16BIT_TO_ARRAY(BLE_GATT_CHAR_HPS_STATUS_CODE), PROP(NTF),           HPS_STATUS_CODE_LEN},
    [BLE_HPS_HDL_STATUS_CODE_CHAR_DESC] = {UUID_16BIT_TO_ARRAY(BLE_GATT_DESC_CLIENT_CHAR_CFG),  PROP(WR),         1                  },
};

static hpss_env_t *p_hpss_env = NULL;
static const uint8_t ble_hps_uuid[2] = UUID_16BIT_TO_ARRAY(BLE_GATT_SVC_HTTP_PROXY);

/*!
    \brief      Allocate Http Proxy Service device structor
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     hpss_dev_t *: pointer to the Http Proxy Service device structor allocated
*/
static hpss_dev_t *hpss_alloc_dev_by_conn_id(uint8_t conn_id)
{
    hpss_dev_t *p_device = NULL;

    p_device = (hpss_dev_t *)sys_malloc(sizeof(hpss_dev_t));

    if (p_device == NULL) {
        dbg_print(ERR, "hpss_alloc_dev_by_conn_id alloc device fail! \r\n");
        return NULL;
    }

    sys_memset(p_device, 0, sizeof(hpss_dev_t));

    INIT_DLIST_HEAD(&p_device->list);
    p_device->conn_id = conn_id;

    list_add_tail(&p_device->list, &p_hpss_env->dev_list);
    return p_device;
}

/*!
    \brief      Find Http Proxy Service device structor
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     hpss_dev_t *: pointer to the Http Proxy Service device structor found
*/
static hpss_dev_t *hpss_find_dev_by_conn_id(uint8_t conn_id)
{
    dlist_t *pos, *n;
    hpss_dev_t *p_device;

    if (list_empty(&p_hpss_env->dev_list)) {
        return NULL;
    }

    list_for_each_safe(pos, n, &p_hpss_env->dev_list) {
        p_device = list_entry(pos, hpss_dev_t, list);
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
    \retval     hpss_dev_t *: pointer to the Http Proxy Service device structor found or allocated
*/
static hpss_dev_t *hpss_find_alloc_dev_by_conn_id(uint8_t conn_id)
{
    hpss_dev_t *p_device = hpss_find_dev_by_conn_id(conn_id);

    if (p_device == NULL) {
        p_device = hpss_alloc_dev_by_conn_id(conn_id);
    }

    return p_device;
}

/*!
    \brief      Remove Http Proxy Service device structor from records
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     none
*/
static void hpss_remove_dev_by_conn_id(uint8_t conn_id)
{
    dlist_t *pos, *n;
    hpss_dev_t *p_device = NULL;
    bool found = false;

    if (list_empty(&p_hpss_env->dev_list)) {
        return;
    }

    list_for_each_safe(pos, n, &p_hpss_env->dev_list) {
        p_device = list_entry(pos, hpss_dev_t, list);
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
    \brief      Http Proxy Service handle control point operation
    \param[in]  p_device: pointer to the Http Proxy Service device structor
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t handle_ctrl_point_op(hpss_dev_t *p_device)
{
    ble_status_t ret = BLE_ERR_NO_ERROR;

    if (p_device->hps_att_val.ctrl_op_code == HTTP_RSVF ||
        p_device->hps_att_val.ctrl_op_code > HTTP_REQUEST_CANCEL) {
        ret = BLE_ATT_ERR_REQUEST_NOT_SUPPORTED;
    } else {
        if (p_device->hps_att_val.ctrl_op_code == HTTP_REQUEST_CANCEL) {
            p_device->op_ongoing = false;
        } else {
            if (!p_device->cccd_reg) {
                ret = BLE_PRF_CCCD_IMPR_CONFIGURED;
            } else if (p_device->hps_att_val.update_msk != HPS_UPDATE_ALL) {
                ret = HPS_INV_REQ_ERR;
            } else {
                if (p_hpss_env->callback.check_network_cb && !p_hpss_env->callback.check_network_cb()) {
                    ret = HPS_NETWORK_UNAVL_ERR;
                } else if (p_device->hps_att_val.ctrl_op_code >= HTTPS_GET_REQUEST &&
                           p_device->hps_att_val.ctrl_op_code < HTTP_REQUEST_CANCEL) {
                    if (p_hpss_env->callback.check_certs_cb &&
                        p_hpss_env->callback.check_certs_cb(p_device->conn_id, p_device->hps_att_val.uri,
                                                            p_device->hps_att_val.uri_len)) {
                        p_device->hps_att_val.security = HTTP_CERT_URI;
                    } else {
                        p_device->hps_att_val.security = HTTP_UNCERT_URI;
                    }
                }
            }
        }
    }

    if (ret == BLE_ERR_NO_ERROR) {
        if (p_hpss_env->callback.http_request_cb) {
            ble_hps_req_info_t info = {
                .conn_id = p_device->conn_id,
                .p_uri = p_device->hps_att_val.uri,
                .uri_len = p_device->hps_att_val.uri_len,
                .p_headers = p_device->hps_att_val.headers,
                .headers_len = p_device->hps_att_val.headers_len,
                .p_body = p_device->hps_att_val.body,
                .body_len = p_device->hps_att_val.body_len,
                .ctrl_op_code = p_device->hps_att_val.ctrl_op_code,
            };

            if (p_hpss_env->callback.http_request_cb(info)) {
                p_device->op_ongoing = true;
                p_device->hps_att_val.update_msk = 0;
            }
        } else {
            ret = BLE_ATT_ERR_INSUFF_RESOURCE;
        }
    }

    return ret;
}

/*!
    \brief      Callback function to handle GATT server messages
    \param[in]  p_cb_data: pointer to GATT server message
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_hpss_rw_cb(ble_gatts_msg_info_t *p_cb_data)
{
    uint16_t attr_idx;
    uint8_t *p_attr = NULL;
    uint16_t attr_len = 0;
    uint8_t status = BLE_ERR_NO_ERROR;
    hpss_dev_t *p_dev = NULL;

    if (p_cb_data->srv_msg_type == BLE_SRV_EVT_GATT_OPERATION) {
        p_dev = hpss_find_alloc_dev_by_conn_id(p_cb_data->msg_data.gatts_op_info.conn_idx);
        if (p_dev == NULL) {
            return BLE_ATT_ERR_VALUE_NOT_ALLOWED;
        }

        if (p_cb_data->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_READ_REQ) {
            ble_gatts_read_req_t *p_read_req = &(p_cb_data->msg_data.gatts_op_info.gatts_op_data.read_req);

            attr_idx = p_read_req->att_idx + BLE_HPS_HDL_SVC;

            if (p_dev->op_ongoing) {
                return BLE_ATT_ERR_VALUE_NOT_ALLOWED;
            }

            switch (attr_idx) {
            case BLE_HPS_HDL_HEADER_VAL: {
                p_attr = p_dev->hps_att_val.headers;
                p_read_req->att_len = p_dev->hps_att_val.headers_len;
            }
            break;

            case BLE_HPS_HDL_ENTITY_BODY_VAL: {
                p_attr = p_dev->hps_att_val.body;
                p_read_req->att_len = p_dev->hps_att_val.body_len;
            }
            break;

            case BLE_HPS_HDL_SECURITY_VAL: {
                p_attr = p_dev->hps_att_val.status_code;
                p_read_req->att_len = HPS_STATUS_CODE_LEN;
            }
            break;

            default:
                return BLE_ATT_ERR_INVALID_HANDLE;
            }

            if (p_read_req->offset > attr_len) {
                return BLE_ATT_ERR_INVALID_OFFSET;
            }

            p_read_req->val_len = ble_min(p_read_req->max_len, attr_len - p_read_req->offset);
            sys_memcpy(p_read_req->p_val, &p_attr[p_read_req->offset], p_read_req->val_len);
        } else if (p_cb_data->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_WRITE_REQ) {
            ble_gatts_write_req_t *p_write_req = &(p_cb_data->msg_data.gatts_op_info.gatts_op_data.write_req);

            attr_idx = p_write_req->att_idx + BLE_HPS_HDL_SVC;

            if ((p_write_req->offset + p_write_req->val_len) > BLE_HPS_VAL_MAX_LEN) {
                return BLE_ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
            } else if (p_dev->op_ongoing) {
                if (attr_idx == BLE_HPS_HDL_CTRL_POINT_VAL &&
                    *(p_write_req->p_val) != HTTP_REQUEST_CANCEL) {
                    return BLE_PRF_PROC_IN_PROGRESS;
                } else {
                    return BLE_ATT_ERR_VALUE_NOT_ALLOWED;
                }
            }

            switch (attr_idx) {
            case BLE_HPS_HDL_URI_VAL: {
                sys_memcpy(&p_dev->hps_att_val.uri[p_write_req->offset], p_write_req->p_val, p_write_req->val_len);
                p_dev->hps_att_val.uri_len = p_write_req->offset + p_write_req->val_len;
                p_dev->hps_att_val.update_msk |= HPS_URI_UPDATE_MASK;
            }
            break;

            case BLE_HPS_HDL_HEADER_VAL: {
                sys_memcpy(&p_dev->hps_att_val.headers[p_write_req->offset], p_write_req->p_val,
                           p_write_req->val_len);
                p_dev->hps_att_val.headers_len = p_write_req->offset + p_write_req->val_len;
                p_dev->hps_att_val.update_msk |= HPS_HEADERS_UPDATE_MASK;
            }
            break;

            case BLE_HPS_HDL_ENTITY_BODY_VAL: {
                sys_memcpy(&p_dev->hps_att_val.body[p_write_req->offset], p_write_req->p_val, p_write_req->val_len);
                p_dev->hps_att_val.body_len = p_write_req->offset + p_write_req->val_len;
                p_dev->hps_att_val.update_msk |= HPS_BODY_UPDATE_MASK;
            }
            break;

            case BLE_HPS_HDL_CTRL_POINT_VAL: {
                p_dev->hps_att_val.ctrl_op_code = *(p_write_req->p_val);
                status = handle_ctrl_point_op(p_dev);
            }
            break;

            case BLE_HPS_HDL_STATUS_CODE_CHAR_DESC: {
                if (*(p_write_req->p_val) == 0) {
                    p_dev->cccd_reg = false;
                } else {
                    p_dev->cccd_reg = true;
                }
            }
            break;

            default:
                return BLE_ATT_ERR_INVALID_HANDLE;
            }
        }
    } else if (p_cb_data->srv_msg_type == BLE_SRV_EVT_CONN_STATE_CHANGE_IND) {
        if (p_cb_data->msg_data.conn_state_change_ind.conn_state == BLE_CONN_STATE_DISCONNECTD) {
            hpss_remove_dev_by_conn_id(p_cb_data->msg_data.conn_state_change_ind.info.disconn_info.conn_idx);
        }
    }

    return status;
}

/*!
    \brief      Set Http Proxy Server response
    \param[in]  response: response information
    \param[out] none
    \retval     none
*/
void ble_hpss_response_set(ble_hps_resp_info_t response)
{
    hpss_dev_t *p_dev = hpss_find_dev_by_conn_id(response.conn_id);

    if (p_dev == NULL) {
        dbg_print(ERR, "ble_hpss_response_set can't find device \r\n");
        return;
    }

    *((uint16_t *)(&p_dev->hps_att_val.status_code[0])) = response.status_code;
    p_dev->hps_att_val.status_code[2] = 0;

    if (response.headers_len > BLE_HPS_VAL_MAX_LEN) {
        sys_memcpy(p_dev->hps_att_val.headers, response.p_headers, BLE_HPS_VAL_MAX_LEN);
        p_dev->hps_att_val.status_code[2] |= HPS_HEADERS_TRUNC_BIT;
        p_dev->hps_att_val.headers_len = BLE_HPS_VAL_MAX_LEN;
    } else if (response.headers_len != 0) {
        sys_memcpy(p_dev->hps_att_val.headers, response.p_headers, response.headers_len);
        p_dev->hps_att_val.status_code[2] |= HPS_HEADERS_RECVD_BIT;
        p_dev->hps_att_val.headers_len = response.headers_len;
    }

    if (response.body_len > BLE_HPS_VAL_MAX_LEN) {
        sys_memcpy(p_dev->hps_att_val.body, response.p_body, BLE_HPS_VAL_MAX_LEN);
        p_dev->hps_att_val.status_code[2] |= HPS_BODY_TRUNC_BIT;
        p_dev->hps_att_val.body_len = BLE_HPS_VAL_MAX_LEN;
    } else if (response.body_len != 0) {
        sys_memcpy(p_dev->hps_att_val.body, response.p_body, response.body_len);
        p_dev->hps_att_val.status_code[2] |= HPS_BODY_RECVD_BIT;
        p_dev->hps_att_val.body_len = response.body_len;
    }

    p_dev->op_ongoing = false;
    ble_gatts_ntf_ind_send(response.conn_id, p_hpss_env->hps_id, BLE_HPS_HDL_STATUS_CODE_VAL,
                           p_dev->hps_att_val.status_code, HPS_STATUS_CODE_LEN, BLE_GATT_NOTIFY);
}

/*!
    \brief      Init Http Proxy Service server
    \param[in]  callbacks: HPS server callback set
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_hpss_init(ble_hpss_callbacks_t callbacks)
{
    ble_status_t ret = BLE_ERR_NO_ERROR;

    p_hpss_env = sys_malloc(sizeof(hpss_env_t));
    if (p_hpss_env == NULL) {
        return BLE_ERR_NO_MEM_AVAIL;
    }

    memset(p_hpss_env, 0, sizeof(hpss_env_t));
    p_hpss_env->callback = callbacks;

    ret = ble_gatts_svc_add(&p_hpss_env->hps_id, ble_hps_uuid, 0, SVC_UUID(16), ble_hps_attr_db,
                            BLE_HPS_HDL_NB, ble_hpss_rw_cb);

    if (ret != BLE_ERR_NO_ERROR) {
        sys_mfree(p_hpss_env);
        return ret;
    }

    INIT_DLIST_HEAD(&p_hpss_env->dev_list);

    return ret;
}

