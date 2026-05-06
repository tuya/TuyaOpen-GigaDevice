/*!
    \file    app_peri_sync_mgr.c
    \brief   Implementation of BLE application periodic sync manager to record devices.

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

#if (BLE_APP_PER_ADV_SUPPORT)
#include <string.h>
#include "ble_per_sync.h"
#include "app_per_sync_mgr.h"

#include "wrapper_os.h"
#include "dbg_print.h"

/* Application periodic sync manager environment structure */
typedef struct sync_mgr_cb
{
    dlist_t devs_list;
} sync_mgr_cb_t;

/* Application periodic sync manager environment data */
static sync_mgr_cb_t ble_sync_mgr_cb;

/*!
    \brief      Find periodic sync device information by index
    \param[in]  sync_idx: periodic sync index
    \param[out] none
    \retval     per_dev_info_t *: pointer to periodic sync device information found
*/
per_dev_info_t *sync_mgr_find_device_by_idx(uint8_t sync_idx)
{
    per_dev_info_t *p_sync_dev = NULL;
    dlist_t *pos, *n;

    if (list_empty(&ble_sync_mgr_cb.devs_list)) {
        return NULL;
    }

    list_for_each_safe(pos, n, &ble_sync_mgr_cb.devs_list) {
        p_sync_dev = list_entry(pos, per_dev_info_t, list);
        if (p_sync_dev->sync_idx == sync_idx) {
            return p_sync_dev;
        }
    }

    return NULL;
}

/*!
    \brief      Callback function to handle BLE periodic sync events
    \param[in]  event: BLE periodic sync event type
    \param[in]  p_data: pointer to BLE periodic sync event data
    \param[out] none
    \retval     none
*/
static void ble_per_sync_evt_handler(ble_per_sync_evt_t event, ble_per_sync_data_u *p_data)
{
    per_dev_info_t *p_sync_dev = NULL;
    switch (event) {
    case BLE_PER_SYNC_EVT_STATE_CHG:
        dbg_print(NOTICE, "periodic sync idx %u, state %u \r\n", p_data->sync_state.sync_idx,
               p_data->sync_state.state);
        break;

    case BLE_PER_SYNC_EVT_ESTABLISHED:
        dbg_print(NOTICE, "periodic device synced. sync idx %u, addr %02X:%02X:%02X:%02X:%02X:%02X \r\n",
               p_data->establish.param.actv_idx, p_data->establish.param.addr.addr[5],
               p_data->establish.param.addr.addr[4], p_data->establish.param.addr.addr[3],
               p_data->establish.param.addr.addr[2], p_data->establish.param.addr.addr[1],
               p_data->establish.param.addr.addr[0]);
        p_sync_dev = ble_per_sync_mgr_find_alloc_device(&p_data->establish.param.addr,
                                                        p_data->establish.param.adv_sid,
                                                        p_data->establish.param.intv);
        if (p_sync_dev) {
            p_sync_dev->sync_idx = p_data->establish.param.actv_idx;
            p_sync_dev->phy = p_data->establish.param.phy;
            p_sync_dev->period_adv_intv = p_data->establish.param.intv;
            p_sync_dev->serv_data = p_data->establish.param.serv_data;
        }
        break;

    case BLE_PER_SYNC_EVT_REPORT: {
        ble_gap_adv_report_info_t  *p_report = p_data->report.p_report;
        p_sync_dev = sync_mgr_find_device_by_idx(p_report->actv_idx);

        if (p_sync_dev) {
            dbg_print(NOTICE, "periodic device reported, addr %02X:%02X:%02X:%02X:%02X:%02X \r\n",
                   p_sync_dev->sync_info.addr[5], p_sync_dev->sync_info.addr[4], p_sync_dev->sync_info.addr[3],
                   p_sync_dev->sync_info.addr[2], p_sync_dev->sync_info.addr[1], p_sync_dev->sync_info.addr[0]);
        }
    }
    break;

    case BLE_PER_SYNC_EVT_RPT_CTRL_RSP: {
        ble_per_sync_rpt_ctrl_rsp_t  *p_res = &p_data->rpt_ctrl_rsp;
        p_sync_dev = sync_mgr_find_device_by_idx(p_res->param.actv_idx);

        if (p_sync_dev) {
            dbg_print(NOTICE, "periodic device report ctrl status 0x%x \r\n", p_res->param.status);
        }
    }
    break;

    default:
        break;
    }
}

/*!
    \brief      Clear PAL flag of all devices in the list
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_per_sync_clear_all_dev_list_flag(void)
{
    per_dev_info_t *p_sync_dev = NULL;
    dlist_t *pos, *n;

    if (list_empty(&ble_sync_mgr_cb.devs_list)) {
        return;
    }

    list_for_each_safe(pos, n, &ble_sync_mgr_cb.devs_list) {
        p_sync_dev = list_entry(pos, per_dev_info_t, list);
        p_sync_dev->in_pal = false;
    }
}

/*!
    \brief      Find device in periodic sync manager list
    \param[in]  p_peer_addr: pointer to peer address
    \param[in]  adv_sid: advertising sid
    \param[out] none
    \retval     per_dev_info_t *: pointer to periodic sync device information found
*/
per_dev_info_t *ble_per_sync_mgr_find_device(ble_gap_addr_t *p_peer_addr, uint8_t adv_sid)
{
    per_dev_info_t *p_sync_dev = NULL;
    dlist_t *pos, *n;

    if (list_empty(&ble_sync_mgr_cb.devs_list)) {
        return NULL;
    }

    list_for_each_safe(pos, n, &ble_sync_mgr_cb.devs_list) {
        p_sync_dev = list_entry(pos, per_dev_info_t, list);
        if (p_peer_addr->addr_type == p_sync_dev->sync_info.addr_type &&
            adv_sid == p_sync_dev->sync_info.adv_sid &&
            !memcmp(p_peer_addr->addr, p_sync_dev->sync_info.addr, BLE_GAP_ADDR_LEN)) {
            return p_sync_dev;
        }
    }

    return NULL;
}

/*!
    \brief      Find device in periodic sync manager list, if no such device, allocate one
    \param[in]  p_peer_addr: pointer to peer address
    \param[in]  adv_sid: advertising sid
    \param[in]  period_adv_intv: periodic advertising interval
    \param[out] none
    \retval     per_dev_info_t *: pointer to periodic sync device information found or allocated
*/
per_dev_info_t *ble_per_sync_mgr_find_alloc_device(ble_gap_addr_t *p_peer_addr, uint8_t adv_sid,
                                                   uint16_t period_adv_intv)
{
    per_dev_info_t *p_sync_dev = ble_per_sync_mgr_find_device(p_peer_addr, adv_sid);

    if (p_sync_dev != NULL) {
        return p_sync_dev;
    }

    p_sync_dev = (per_dev_info_t *)sys_malloc(sizeof(per_dev_info_t));
    if (p_sync_dev) {
        memset(p_sync_dev, 0, sizeof(per_dev_info_t));
        INIT_DLIST_HEAD(&p_sync_dev->list);
        p_sync_dev->sync_info.addr_type = p_peer_addr->addr_type;
        memcpy(p_sync_dev->sync_info.addr, p_peer_addr->addr, BLE_GAP_ADDR_LEN);
        p_sync_dev->sync_info.adv_sid = adv_sid;
        p_sync_dev->period_adv_intv = period_adv_intv;
        list_add_tail(&p_sync_dev->list, &ble_sync_mgr_cb.devs_list);
    }
    return p_sync_dev;
}

/*!
    \brief      Clear periodic sync manager list
    \param[in]  none
    \param[out] none
    \retval     none
*/
void per_sync_mgr_clear_dev_list(void)
{
    per_dev_info_t *p_sync_dev = NULL;
    dlist_t *pos, *n;

    if (list_empty(&ble_sync_mgr_cb.devs_list)) {
        return;
    }

    list_for_each_safe(pos, n, &ble_sync_mgr_cb.devs_list) {
        p_sync_dev = list_entry(pos, per_dev_info_t, list);
        list_del(&p_sync_dev->list);
        sys_mfree((void *)p_sync_dev);
    }
}

/*!
    \brief      Reset application periodic sync manager
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_per_sync_mgr_reset(void)
{
    per_sync_mgr_clear_dev_list();
}

/*!
    \brief      Cancel ongoing periodic sync procedure
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_per_sync_cancel(void)
{
    ble_status_t status = ble_per_sync_cancel();
    if (status != BLE_ERR_NO_ERROR) {
        dbg_print(ERR, "per sync cancel fail! status: %x\r\n", status);
        return;
    }

    dbg_print(NOTICE, "per sync cancel success\r\n");
}

/*!
    \brief      Terminate periodic sync
    \param[in]  sync_idx: periodic sync index
    \param[out] none
    \retval     none
*/
void app_per_sync_terminate(uint8_t sync_idx)
{
    per_dev_info_t *p_sync_dev = sync_mgr_find_device_by_idx(sync_idx);
    if (p_sync_dev == NULL) {
        dbg_print(ERR, "app_per_sync_terminate fail! not found sync device\r\n");
        return;
    }

    if (ble_per_sync_terminate(sync_idx) != BLE_ERR_NO_ERROR) {
        dbg_print(NOTICE, "app_per_sync_terminate fail! \r\n");
    }
}

/*!
    \brief      Init application periodic sync manager
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_per_sync_mgr_init(void)
{
    INIT_DLIST_HEAD(&ble_sync_mgr_cb.devs_list);
    ble_per_sync_callback_register(ble_per_sync_evt_handler);
}

/*!
    \brief      Deinit application periodic sync manager
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_per_sync_mgr_deinit(void)
{
    ble_per_sync_callback_unregister(ble_per_sync_evt_handler);
}

#endif // (BLE_APP_SUPPORT && BLE_APP_PER_ADV_SUPPORT)
