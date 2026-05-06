/*!
    \file    ble_proxr.c
    \brief   Proximity Reporter Profile Implementation.

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
#include "ble_proxr.h"
#include "dbg_print.h"
#include "ble_conn.h"
#include "dlist.h"
#include "wrapper_os.h"
#include "tps/ble_tpss.h"
#include "lls/ble_llss.h"
#include "ias/ble_iass.h"

/* Proximity reporter server environment variable */
typedef struct
{
    uint8_t                 lls_id;
    uint8_t                 tps_id;
    uint8_t                 ias_id;
    dlist_t                 dev_list;
    ble_proxr_callbacks_t   callbacks;
} proxr_env_t;

/* Proximity reporter device information */
typedef struct
{
    dlist_t             list;
    uint8_t             conn_id;
    ble_gap_addr_t      addr;
    uint8_t             tx_power_lvl;
    uint8_t             lls_alert_val;
    bool                reading_tx_pwr;
    uint16_t            pending_token;
} proxr_dev_t;

static proxr_env_t *p_proxr_env = NULL;

/*!
    \brief      Allocate proximity reporter device structor
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     proxr_dev_t *: pointer to proximity reporter device allocated
*/
static proxr_dev_t *proxr_alloc_dev_by_conn_id(uint8_t conn_id)
{
    proxr_dev_t *p_device = NULL;

    p_device = (proxr_dev_t *)sys_malloc(sizeof(proxr_dev_t));

    if (p_device == NULL) {
        dbg_print(ERR, "proxr_alloc_dev_by_conn_id alloc device fail! \r\n");
        return NULL;
    }

    sys_memset(p_device, 0, sizeof(proxr_dev_t));

    INIT_DLIST_HEAD(&p_device->list);
    p_device->conn_id = conn_id;
    p_device->tx_power_lvl = 0xFF;
    p_device->pending_token = 0xFFFF;

    list_add_tail(&p_device->list, &p_proxr_env->dev_list);
    return p_device;
}

/*!
    \brief      Find proximity reporter device structor
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     proxr_dev_t *: pointer to proximity reporter device found
*/
static proxr_dev_t *proxr_find_dev_by_conn_id(uint8_t conn_id)
{
    dlist_t *pos, *n;
    proxr_dev_t *p_device;

    if (list_empty(&p_proxr_env->dev_list)) {
        return NULL;
    }

    list_for_each_safe(pos, n, &p_proxr_env->dev_list) {
        p_device = list_entry(pos, proxr_dev_t, list);
        if (p_device->conn_id == conn_id) {
            return p_device;
        }
    }

    return NULL;
}

/*!
    \brief      Find proximity reporter device structor, if no such device, allocate one
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     proxr_dev_t *: pointer to proximity reporter device found or allocated
*/
static proxr_dev_t *proxr_find_alloc_dev_by_conn_id(uint8_t conn_id)
{
    proxr_dev_t *p_device = proxr_find_dev_by_conn_id(conn_id);

    if (p_device == NULL) {
        p_device = proxr_alloc_dev_by_conn_id(conn_id);
    }

    return p_device;
}

/*!
    \brief      Remove proximity reporter device structor from records
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     none
*/
static void proxr_remove_dev_by_conn_id(uint8_t conn_id)
{
    dlist_t *pos, *n;
    proxr_dev_t *p_device = NULL;
    bool found = false;

    if (list_empty(&p_proxr_env->dev_list)) {
        return;
    }

    list_for_each_safe(pos, n, &p_proxr_env->dev_list) {
        p_device = list_entry(pos, proxr_dev_t, list);
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
    \brief      Handle pending read operation
    \param[in]  conn_id: connection index
    \param[in]  status: read confirm status
    \param[out] none
    \retval     none
*/
static void proxr_handle_pending_read(uint8_t conn_id, ble_status_t status)
{
    dlist_t *pos, *n;
    proxr_dev_t *p_device = NULL;

    if (list_empty(&p_proxr_env->dev_list)) {
        return;
    }

    list_for_each_safe(pos, n, &p_proxr_env->dev_list) {
        p_device = list_entry(pos, proxr_dev_t, list);
        if (p_device->conn_id == conn_id && p_device->pending_token != 0xFFFF) {
            ble_gatts_svc_attr_read_cfm(p_device->conn_id, p_device->pending_token, status, 1, 1,
                                        &(p_device->tx_power_lvl));

            p_device->pending_token = 0xFFFF;
        }
    }
}

/*!
    \brief      Callback function to handle lls related GATT server messages
    \param[in]  p_cb_data: pointer to GATT server message
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_lls_rw_cb(ble_gatts_msg_info_t *p_cb_data)
{
    proxr_dev_t *p_dev = NULL;

    if (p_cb_data->srv_msg_type == BLE_SRV_EVT_GATT_OPERATION) {
        p_dev = proxr_find_dev_by_conn_id(p_cb_data->msg_data.gatts_op_info.conn_idx);
        if (p_dev == NULL) {
            return BLE_ATT_ERR_VALUE_NOT_ALLOWED;
        }

        if (p_cb_data->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_READ_REQ) {
            *(p_cb_data->msg_data.gatts_op_info.gatts_op_data.read_req.p_val) = p_dev->lls_alert_val;
            p_cb_data->msg_data.gatts_op_info.gatts_op_data.read_req.val_len = 1;
            p_cb_data->msg_data.gatts_op_info.gatts_op_data.read_req.att_len = 1;
        } else if (p_cb_data->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_WRITE_REQ) {
            if (*(p_cb_data->msg_data.gatts_op_info.gatts_op_data.write_req.p_val) > PROXM_ALERT_HIGH) {
                return BLE_ATT_ERR_APP_ERROR;
            }

            p_dev->lls_alert_val = *(p_cb_data->msg_data.gatts_op_info.gatts_op_data.write_req.p_val);
        }
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle tps related GATT server messages
    \param[in]  p_cb_data: pointer to GATT server message
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_tps_rw_cb(ble_gatts_msg_info_t *p_cb_data)
{
    proxr_dev_t *p_dev = NULL;

    if (p_cb_data->srv_msg_type == BLE_SRV_EVT_GATT_OPERATION) {
        p_dev = proxr_find_dev_by_conn_id(p_cb_data->msg_data.gatts_op_info.conn_idx);
        if (p_dev == NULL) {
            return BLE_ATT_ERR_VALUE_NOT_ALLOWED;
        }

        if (p_cb_data->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_READ_REQ) {
            if (p_dev->reading_tx_pwr) {
                dbg_print(WARNING, "ble_tps_rw_cb is reading local tx power\r\n");
                p_dev->pending_token = p_cb_data->msg_data.gatts_op_info.gatts_op_data.read_req.token;
                p_cb_data->msg_data.gatts_op_info.gatts_op_data.read_req.pending_cfm = true;
            } else {
                *(p_cb_data->msg_data.gatts_op_info.gatts_op_data.read_req.p_val) = p_dev->tx_power_lvl;
                p_cb_data->msg_data.gatts_op_info.gatts_op_data.read_req.val_len = 1;
                p_cb_data->msg_data.gatts_op_info.gatts_op_data.read_req.att_len = 1;
            }

        }
    }
    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle ias related GATT server messages
    \param[in]  p_cb_data: pointer to GATT server message
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_ias_rw_cb(ble_gatts_msg_info_t *p_cb_data)
{
    proxr_dev_t *p_dev = NULL;

    if (p_cb_data->srv_msg_type == BLE_SRV_EVT_GATT_OPERATION) {
        p_dev = proxr_find_dev_by_conn_id(p_cb_data->msg_data.gatts_op_info.conn_idx);

        if (p_dev == NULL) {
            dbg_print(ERR, "ble_ias_rw_cbcan't find or alloc device \r\n");
            return BLE_ATT_ERR_VALUE_NOT_ALLOWED;
        }

        if (p_cb_data->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_WRITE_REQ) {
            if (*(p_cb_data->msg_data.gatts_op_info.gatts_op_data.write_req.p_val) > PROXM_ALERT_HIGH) {
                return BLE_ATT_ERR_APP_ERROR;
            } else {
                if (p_proxr_env->callbacks.path_loss_alert_update) {
                    p_proxr_env->callbacks.path_loss_alert_update(p_dev->addr,
                                                                  *(p_cb_data->msg_data.gatts_op_info.gatts_op_data.write_req.p_val));
                }
            }
        }
    } else if (p_cb_data->srv_msg_type == BLE_SRV_EVT_CONN_STATE_CHANGE_IND) {
        if (p_cb_data->msg_data.conn_state_change_ind.conn_state == BLE_CONN_STATE_DISCONNECTD) {
            p_dev = proxr_find_dev_by_conn_id(
                        p_cb_data->msg_data.conn_state_change_ind.info.disconn_info.conn_idx);

            if (p_dev != NULL) {
                if (p_proxr_env->callbacks.path_loss_alert_update) {
                    p_proxr_env->callbacks.path_loss_alert_update(p_dev->addr, PROXM_ALERT_NONE);
                }

                // FIX TODO Now we only concern connection timeout reason
                if (p_cb_data->msg_data.conn_state_change_ind.info.disconn_info.reason == BLE_LL_ERR_CON_TIMEOUT) {
                    if (p_proxr_env->callbacks.lls_alert_update) {
                        p_proxr_env->callbacks.lls_alert_update(p_dev->addr, p_dev->lls_alert_val);
                    }
                }
                proxr_remove_dev_by_conn_id(p_dev->conn_id);
            }
        } else if (p_cb_data->msg_data.conn_state_change_ind.conn_state == BLE_CONN_STATE_CONNECTED) {
            p_dev = proxr_find_alloc_dev_by_conn_id(
                        p_cb_data->msg_data.conn_state_change_ind.info.conn_info.conn_idx);
            if (p_dev != NULL) {
                p_dev->addr = p_cb_data->msg_data.conn_state_change_ind.info.conn_info.peer_addr;
                if (p_proxr_env->tps_id != 0xFF && !p_dev->reading_tx_pwr) {
                    p_dev->reading_tx_pwr = true;
                    ble_conn_phy_get(p_dev->conn_id);
                    ble_conn_tx_pwr_report_ctrl(p_dev->conn_id, true, false);
                }
            }
            if (p_proxr_env->callbacks.lls_alert_update) {
                p_proxr_env->callbacks.lls_alert_update(
                    p_cb_data->msg_data.conn_state_change_ind.info.conn_info.peer_addr, PROXM_ALERT_NONE);
            }
        }
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle connection events
    \param[in]  event: connection event type
    \param[in]  p_data: pointer to connection event data
    \param[out] none
    \retval     none
*/
static void ble_proxr_conn_evt_handler(ble_conn_evt_t event, ble_conn_data_u *p_data)
{
    proxr_dev_t *p_dev = NULL;
    if (event == BLE_CONN_EVT_PHY_GET_RSP) {
        if (p_data->phy_get.status != BLE_ERR_NO_ERROR) {
            p_dev = proxr_find_dev_by_conn_id(p_data->phy_get.conn_idx);
            if (p_dev) {
                p_dev->reading_tx_pwr = false;
                proxr_handle_pending_read(p_dev->conn_id, BLE_ATT_ERR_APP_ERROR);
            }
        }
    } else if (event == BLE_CONN_EVT_PHY_INFO) {
        p_dev = proxr_find_dev_by_conn_id(p_data->phy_val.conn_idx);

        if (p_dev && p_dev->reading_tx_pwr) {
            ble_conn_local_tx_pwr_get(p_data->phy_val.conn_idx, p_data->phy_val.tx_phy);
        }
    } else if (event == BLE_CONN_EVT_LOC_TX_PWR_GET_RSP) {
        p_dev = proxr_find_dev_by_conn_id(p_data->loc_tx_pwr.conn_idx);

        if (p_dev && p_dev->reading_tx_pwr) {
            if (p_data->loc_tx_pwr.status == BLE_ERR_NO_ERROR) {
                p_dev->tx_power_lvl = p_data->loc_tx_pwr.tx_pwr;
                proxr_handle_pending_read(p_dev->conn_id, p_data->loc_tx_pwr.status);
            } else {
                proxr_handle_pending_read(p_dev->conn_id, BLE_ATT_ERR_APP_ERROR);
            }
            p_dev->reading_tx_pwr = false;

        }
        dbg_print(NOTICE, "local tx power %d\r\n", p_data->loc_tx_pwr.tx_pwr);
    } else if (event == BLE_CONN_EVT_LOC_TX_PWR_RPT_INFO) {
        p_dev = proxr_find_dev_by_conn_id(p_data->loc_tx_pwr.conn_idx);
        if (p_dev) {
            p_dev->tx_power_lvl = p_data->loc_tx_pwr_rpt.tx_pwr;
        }

        dbg_print(NOTICE, "local tx power report info %d\r\n", p_data->loc_tx_pwr.tx_pwr);
    }
}

/*!
    \brief      Init proximity reporter
    \param[in]  callbacks: proximity reporter callback set
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_proxr_init(ble_proxr_callbacks_t callbacks)
{
    ble_status_t ret = BLE_ERR_NO_ERROR;
    p_proxr_env = sys_malloc(sizeof(proxr_env_t));
    if (p_proxr_env == NULL) {
        return BLE_ERR_NO_MEM_AVAIL;
    }

    sys_memset(p_proxr_env, 0, sizeof(proxr_env_t));
    p_proxr_env->callbacks = callbacks;

    ret = ble_conn_callback_register(ble_proxr_conn_evt_handler);

    if (ret != BLE_ERR_NO_ERROR) {
        sys_mfree(p_proxr_env);
    } else {
        INIT_DLIST_HEAD(&p_proxr_env->dev_list);
        p_proxr_env->lls_id = ble_llss_init(ble_lls_rw_cb);

        if (p_proxr_env->lls_id == 0xFF) {
            sys_mfree(p_proxr_env);
            ble_conn_callback_unregister(ble_proxr_conn_evt_handler);
            return BLE_ATT_ERR_APP_ERROR;
        }

        p_proxr_env->tps_id = ble_tpss_init(ble_tps_rw_cb);
        p_proxr_env->ias_id = ble_iass_init(ble_ias_rw_cb);
    }

    return ret;
}

