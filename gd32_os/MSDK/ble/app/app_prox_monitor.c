/*!
    \file    app_prox_monitor.c
    \brief   Proximity Monitor Application Module entry point

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

#include "ble_app_config.h"

#if (BLE_PROFILE_PROX_CLIENT)
/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "wrapper_os.h"
#include "dbg_print.h"
#include "app_prox_monitor.h"
#include "ble_conn.h"
#include "dlist.h"
#include "app_dev_mgr.h"

#define PATH_LOSS_SENSI_MS        500
#define HIGH_THRESHOLD            50
#define HYSTERESIS                5
#define LOW_THRESHOLD             40

static void app_proxm_tx_pwr_read_cb(uint8_t conn_id, uint8_t tx_pwr);
static void app_proxm_lls_alert_read_cb(uint8_t conn_id, proxm_alert_lvl_t level);
static void app_proxm_srv_found_cb(uint8_t conn_id, bool found);

static ble_proxm_callbacks_t proxm_callbacks = {
    .read_tx_pwr_cb = app_proxm_tx_pwr_read_cb,
    .read_lls_altert_cb = app_proxm_lls_alert_read_cb,
    .found_service_cb = app_proxm_srv_found_cb,
};

/* Proximity monitor application environment variable */
typedef struct
{
    dlist_t                 dev_list;
} app_proxm_env_t;

/* Proximity monitor application device information */
typedef struct
{
    dlist_t             list;
    uint8_t             conn_id;
    uint8_t             tx_power_lvl;
    uint16_t            min_time;
} app_proxm_dev_t;

/* Proximity monitor application environment data */
static app_proxm_env_t app_proxm_env;

/*!
    \brief      Allocate proximity monitor application device data by connection index
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     app_proxm_dev_t *: pointer to the proximity monitor device data allocated
*/
static app_proxm_dev_t *app_proxm_alloc_dev_by_conn_id(uint8_t conn_id)
{
    app_proxm_dev_t *p_device = NULL;

    p_device = (app_proxm_dev_t *)sys_malloc(sizeof(app_proxm_dev_t));

    if (p_device == NULL) {
        dbg_print(ERR, "app_proxr_alloc_dev_by_conn_id alloc device fail! \r\n");
        return NULL;
    }

    sys_memset(p_device, 0, sizeof(app_proxm_dev_t));

    INIT_DLIST_HEAD(&p_device->list);
    p_device->conn_id = conn_id;
    p_device->tx_power_lvl = 0xFF;

    list_add_tail(&p_device->list, &app_proxm_env.dev_list);
    return p_device;
}

/*!
    \brief      Find proximity monitor application device data by connection index
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     app_proxm_dev_t *: pointer to the proximity monitor device data found
*/
static app_proxm_dev_t *app_proxm_find_dev_by_conn_id(uint8_t conn_id)
{
    dlist_t *pos, *n;
    app_proxm_dev_t *p_device;

    if (list_empty(&app_proxm_env.dev_list)) {
        return NULL;
    }

    list_for_each_safe(pos, n, &app_proxm_env.dev_list) {
        p_device = list_entry(pos, app_proxm_dev_t, list);
        if (p_device->conn_id == conn_id) {
            return p_device;
        }
    }

    return NULL;
}

/*!
    \brief      Find proximity monitor application device data by connection index, if no such data, allocate one
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     app_proxm_dev_t *: pointer to the proximity monitor device data found pr allocated
*/
static app_proxm_dev_t *app_proxm_find_alloc_dev_by_conn_id(uint8_t conn_id)
{
    app_proxm_dev_t *p_device = app_proxm_find_dev_by_conn_id(conn_id);

    if (p_device == NULL) {
        p_device = app_proxm_alloc_dev_by_conn_id(conn_id);
    }

    return p_device;
}

/*!
    \brief      Remove proximity monitor application device data by connection index
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     none
*/
static void app_proxm_remove_dev_by_conn_id(uint8_t conn_id)
{
    dlist_t *pos, *n;
    app_proxm_dev_t *p_device = NULL;
    bool found = false;

    if (list_empty(&app_proxm_env.dev_list)) {
        return;
    }

    list_for_each_safe(pos, n, &app_proxm_env.dev_list) {
        p_device = list_entry(pos, app_proxm_dev_t, list);
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
    \brief      Proximity monitor application service found callback
    \param[in]  conn_id: connection index
    \param[in]  found: true if service is found, otherwise false
    \param[out] none
    \retval     none
*/
static void app_proxm_srv_found_cb(uint8_t conn_id, bool found)
{
    app_proxm_dev_t *p_dev = NULL;

    dbg_print(NOTICE, "app_proxm_srv_found_cb found: %u\r\n", found);
    if (found) {
        p_dev = app_proxm_find_alloc_dev_by_conn_id(conn_id);
        if (p_dev) {
            ble_device_t *p_sec_dev = dm_find_dev_by_conidx(conn_id);
            if (p_sec_dev) {
                p_dev->min_time = (PATH_LOSS_SENSI_MS * 4) / (p_sec_dev->conn_info.interval * 5);
                p_dev->min_time = p_dev->min_time == 0 ? 1 : p_dev->min_time;
            } else {
                p_dev->min_time = 5;
            }
            app_proxm_read_tx_pwr_val(conn_id);
        }
    }
}

/*!
    \brief      Proximity monitor application tx power read callback
    \param[in]  conn_id: connection index
    \param[in]  tx_pwr: tx power level
    \param[out] none
    \retval     none
*/
static void app_proxm_tx_pwr_read_cb(uint8_t conn_id, uint8_t tx_pwr)
{
    dbg_print(NOTICE, "app_proxm_tx_pwr_read_cb tx power: %u\r\n", tx_pwr);
    app_proxm_dev_t *p_dev = NULL;
    p_dev = app_proxm_find_dev_by_conn_id(conn_id);
    if (p_dev) {
        p_dev->tx_power_lvl = tx_pwr;
        ble_conn_tx_pwr_report_ctrl(conn_id, false, true);
        ble_conn_path_loss_ctrl(conn_id, true, HIGH_THRESHOLD, HYSTERESIS, LOW_THRESHOLD, HYSTERESIS,
                                p_dev->min_time);
    }
}

/*!
    \brief      Proximity monitor application lls alert value read callback
    \param[in]  conn_id: connection index
    \param[in]  level: alert level value
    \param[out] none
    \retval     none
*/
static void app_proxm_lls_alert_read_cb(uint8_t conn_id, proxm_alert_lvl_t level)
{
    dbg_print(NOTICE, "app_proxm_lls_alert_read_cb alert level: %u\r\n", level);
}

/*!
    \brief      Write lls alert value
    \param[in]  conn_id: connection index
    \param[in]  level: alert level value
    \param[out] none
    \retval     none
*/
void app_proxm_write_lls_alert_val(uint8_t conn_id, proxm_alert_lvl_t level)
{
    app_proxm_dev_t *p_dev = app_proxm_find_dev_by_conn_id(conn_id);

    if (p_dev) {
        ble_proxm_write_lls_char_value(conn_id, level);
    }
}

/*!
    \brief      Write ias alter value
    \param[in]  conn_id: connection index
    \param[in]  level: alert level value
    \param[out] none
    \retval     none
*/
void app_proxm_write_ias_alert_val(uint8_t conn_id, proxm_alert_lvl_t level)
{
    app_proxm_dev_t *p_dev = app_proxm_find_dev_by_conn_id(conn_id);

    if (p_dev) {
        ble_proxm_write_ias_char_value(conn_id, level);
    }
}

/*!
    \brief      Read ias alter value
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     none
*/
void app_proxm_read_lls_alert_val(uint8_t conn_id)
{
    app_proxm_dev_t *p_dev = app_proxm_find_dev_by_conn_id(conn_id);

    if (p_dev) {
        ble_proxm_read_lls_char_value(conn_id);
    }
}

/*!
    \brief      Read tx power value
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     none
*/
void app_proxm_read_tx_pwr_val(uint8_t conn_id)
{
    app_proxm_dev_t *p_dev = app_proxm_find_dev_by_conn_id(conn_id);

    if (p_dev) {
        ble_proxm_read_tx_pwr_char_value(conn_id);
    }
}

/*!
    \brief      Callback function to handle connection events
    \param[in]  event: connection event type
    \param[in]  p_data: pointer to connection event data
    \param[out] none
    \retval     none
*/
static void app_proxm_conn_evt_handler(ble_conn_evt_t event, ble_conn_data_u *p_data)
{
    app_proxm_dev_t *p_dev = NULL;
    if (event == BLE_CONN_EVT_STATE_CHG) {
        if (p_data->conn_state.state == BLE_CONN_STATE_DISCONNECTD) {
            app_proxm_remove_dev_by_conn_id(p_data->conn_state.info.discon_info.conn_idx);
        }
    } else if (event == BLE_CONN_EVT_PEER_TX_PWR_RPT_INFO) {
        p_dev = app_proxm_find_dev_by_conn_id(p_data->loc_tx_pwr.conn_idx);
        if (p_dev) {
            p_dev->tx_power_lvl = p_data->loc_tx_pwr_rpt.tx_pwr;
        }
    } else if (event == BLE_CONN_EVT_PATH_LOSS_CTRL_RSP) {
        if (p_data->path_ctrl.status != BLE_ERR_NO_ERROR) {
            dbg_print(ERR, "app_proxm_conn_evt_handler path loss control fail status 0x%x\r\n",
                      p_data->path_ctrl.status);
        }
    } else if (event == BLE_CONN_EVT_PATH_LOSS_THRESHOLD_INFO) {
        p_dev = app_proxm_find_dev_by_conn_id(p_data->path_loss_thr.conn_idx);
        dbg_print(NOTICE, "path loss report conn_idx %d, curr_path_loss %u, zone_entered %u\r\n",
                  p_data->path_loss_thr.conn_idx,
                  p_data->path_loss_thr.curr_path_loss, p_data->path_loss_thr.zone_entered);
        if (p_dev) {
            if (p_data->path_loss_thr.zone_entered == BLE_GAP_PATH_LOSS_LOW) {
                ble_proxm_write_ias_char_value(p_data->path_loss_thr.conn_idx, PROXM_ALERT_NONE);
            } else if (p_data->path_loss_thr.zone_entered == BLE_GAP_PATH_LOSS_MID) {
                ble_proxm_write_ias_char_value(p_data->path_loss_thr.conn_idx, PROXM_ALERT_MILD);
            } else {
                ble_proxm_write_ias_char_value(p_data->path_loss_thr.conn_idx, PROXM_ALERT_HIGH);
            }
        }
    }
}

/*!
    \brief      Init proximity monitor application module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_proxm_init(void)
{
    ble_status_t ret = BLE_ERR_NO_ERROR;

    ret = ble_conn_callback_register(app_proxm_conn_evt_handler);

    if (ret != BLE_ERR_NO_ERROR) {
        dbg_print(ERR, "app_proxm_init register conn fail\r\n");
        return;
    }

    ble_proxm_init(proxm_callbacks, PROXM_ALERT_MILD);
    INIT_DLIST_HEAD(&app_proxm_env.dev_list);
}

#endif // (BLE_PROFILE_PROX_CLIENT)
