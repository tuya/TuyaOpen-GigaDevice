/*!
    \file    app_scan_mgr.c
    \brief   Implementation of BLE application scan manager to record devices.

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

#if (BLE_APP_SUPPORT && (BLE_CFG_ROLE & (BLE_CFG_ROLE_OBSERVER | BLE_CFG_ROLE_CENTRAL)))
#include <string.h>
#include "app_scan_mgr.h"
#include "app_per_sync_mgr.h"

#include "ble_adv_data.h"
#include "ble_scan.h"

#include "wrapper_os.h"
#include "dbg_print.h"

/* Application scan manager module structure */
typedef struct scan_mgr_cb
{
    bool    update_with_rssi;       /*!< Updata scanned device list if RSSI changed */
    dlist_t devs_list;              /*!< Scanned device list */
} scan_mgr_cb_t;

/* Application scan manager module data */
static scan_mgr_cb_t ble_scan_mgr_cb;

/*!
    \brief      Find device information by address in the scanned device list
    \param[in]  p_peer_addr: pointer to peer device address
    \param[out] none
    \retval     dev_info_t *: pointer to the device information found
*/
dev_info_t *scan_mgr_find_device(ble_gap_addr_t *p_peer_addr)
{
    dlist_t *pos, *n;
    dev_info_t *p_dev_info = NULL;

    if (list_empty(&ble_scan_mgr_cb.devs_list)) {
        return NULL;
    }

    list_for_each_safe(pos, n, &ble_scan_mgr_cb.devs_list) {
        p_dev_info = list_entry(pos, dev_info_t, list);
        if (p_peer_addr->addr_type == p_dev_info->peer_addr.addr_type &&
            !memcmp(p_peer_addr->addr, p_dev_info->peer_addr.addr, BLE_GAP_ADDR_LEN)) {
            return p_dev_info;
        }
    }

    return NULL;
}

/*!
    \brief      Get scanned device list size
    \param[in]  none
    \param[out] none
    \retval     uint8_t: scanned device list size
*/
static uint8_t scan_mgr_list_size(void)
{
    uint8_t num = 0;
    dlist_t *pos, *n;

    if (list_empty(&ble_scan_mgr_cb.devs_list)) {
        return 0;
    }

    list_for_each_safe(pos, n, &ble_scan_mgr_cb.devs_list) {
        num++;
    }

    return num;
}

/*!
    \brief      Add device into scanned device list
    \param[in]  p_peer_addr: pointer to peer device address
    \param[out] none
    \retval     uint8_t: 0xFF if add device fail, otherwise index in the list
*/
uint8_t scan_mgr_add_device(ble_gap_addr_t *p_peer_addr)
{
    dev_info_t *p_dev_info = NULL;

    p_dev_info = (dev_info_t *)sys_malloc(sizeof(dev_info_t));
    if (p_dev_info) {
        memset(p_dev_info, 0, sizeof(dev_info_t));

        INIT_DLIST_HEAD(&p_dev_info->list);
        p_dev_info->peer_addr = *p_peer_addr;

        list_add_tail(&p_dev_info->list, &ble_scan_mgr_cb.devs_list);
        return scan_mgr_list_size() - 1;
    }
    return 0xFF;
}

/*!
    \brief      Function to handle @ref BLE_SCAN_EVT_ADV_RPT event
    \param[in]  p_info: pointer to advertising report information
    \param[out] none
    \retval     none
*/
static void scan_mgr_report_hdlr(ble_gap_adv_report_info_t *p_info)
{
    uint8_t *p_name = NULL;
    uint8_t name_len;
    uint8_t name[31] = {'\0'};
    dev_info_t *p_dev_info = scan_mgr_find_device(&p_info->peer_addr);

    if (p_info->period_adv_intv) {
        #if BLE_APP_PER_ADV_SUPPORT
        ble_per_sync_mgr_find_alloc_device(&p_info->peer_addr, p_info->adv_sid, p_info->period_adv_intv);
        #endif
    }

    if (p_dev_info == NULL || ble_scan_mgr_cb.update_with_rssi || p_dev_info->recv_name_flag == 0) {
        p_name = ble_adv_find(p_info->data.p_data, p_info->data.len, BLE_AD_TYPE_COMPLETE_LOCAL_NAME,
                              &name_len);
        if (p_name == NULL) {
            p_name = ble_adv_find(p_info->data.p_data, p_info->data.len, BLE_AD_TYPE_SHORT_LOCAL_NAME,
                                  &name_len);
        }

        if (p_name) {
            memcpy(name, p_name, name_len > 30 ? 30 : name_len);
        }

        if (p_dev_info == NULL) {
            uint8_t idx = scan_mgr_add_device(&p_info->peer_addr);
            p_dev_info = scan_mgr_find_dev_by_idx(idx);
            p_dev_info->adv_sid = p_info->adv_sid;
            p_dev_info->idx = idx;
            dbg_print(NOTICE, "new device addr %02X:%02X:%02X:%02X:%02X:%02X, addr type 0x%x, rssi %d, sid 0x%x, dev idx %u, peri_adv_int %u, name %s\r\n",
                   p_info->peer_addr.addr[5], p_info->peer_addr.addr[4], p_info->peer_addr.addr[3],
                   p_info->peer_addr.addr[2], p_info->peer_addr.addr[1], p_info->peer_addr.addr[0],
                   p_info->peer_addr.addr_type, p_info->rssi, p_info->adv_sid, idx, p_info->period_adv_intv, name);
        } else if ((p_dev_info->recv_name_flag == 0 && p_name != NULL) || ble_scan_mgr_cb.update_with_rssi) {
            dbg_print(NOTICE, "update device addr %02X:%02X:%02X:%02X:%02X:%02X, addr type 0x%x, rssi %d, sid 0x%x, dev idx %u name %s\r\n",
                   p_info->peer_addr.addr[5], p_info->peer_addr.addr[4], p_info->peer_addr.addr[3],
                   p_info->peer_addr.addr[2], p_info->peer_addr.addr[1], p_info->peer_addr.addr[0],
                   p_info->peer_addr.addr_type, p_info->rssi, p_info->adv_sid, p_dev_info->idx, name);
        }

        p_dev_info->recv_name_flag = (p_name == NULL ? 0 : 1);
    }
}

/*!
    \brief      Callback function to handle BLE scan events
    \param[in]  event: BLE scan event type
    \param[in]  p_data: pointer to the BLE scan event data
    \param[out] none
    \retval     none
*/
void ble_app_scan_mgr_evt_handler(ble_scan_evt_t event, ble_scan_data_u *p_data)
{
    switch (event) {
    case BLE_SCAN_EVT_ENABLE_RSP:
        if (p_data->enable_rsp.status) {
            dbg_print(NOTICE, "Ble scan enable fail, status 0x%x\r\n", p_data->enable_rsp.status);
        }
        break;

    case BLE_SCAN_EVT_DISABLE_RSP:
        if (p_data->disable_rsp.status) {
            dbg_print(NOTICE, "Ble scan disable fail status 0x%x\r\n", p_data->disable_rsp.status);
        }
        break;

    case BLE_SCAN_EVT_STATE_CHG:
        if (p_data->scan_state.scan_state == BLE_SCAN_STATE_ENABLED) {
            dbg_print(NOTICE, "Ble Scan enabled status 0x%x\r\n", p_data->scan_state.reason);
        } else if (p_data->scan_state.scan_state == BLE_SCAN_STATE_ENABLING) {
            scan_mgr_clear_dev_list();
        } else if (p_data->scan_state.scan_state == BLE_SCAN_STATE_DISABLED) {
            dbg_print(NOTICE, "Ble Scan disabled status 0x%x\r\n", p_data->scan_state.reason);
        }
        break;

    case BLE_SCAN_EVT_ADV_RPT:
        scan_mgr_report_hdlr(p_data->p_adv_rpt);
        break;

    default:
        break;
    }
}

/*!
    \brief      List all the scanned devices
    \param[in]  none
    \param[out] none
    \retval     none
*/
void scan_mgr_list_scanned_devices(void)
{
    dev_info_t *p_dev_info = NULL;
    dlist_t *pos, *n;
    uint8_t elt_idx = 0;

    if (list_empty(&ble_scan_mgr_cb.devs_list)) {
        dbg_print(NOTICE, "======= scan list empty =========\r\n");
        return;
    }

    list_for_each_safe(pos, n, &ble_scan_mgr_cb.devs_list) {
        p_dev_info = list_entry(pos, dev_info_t, list);
        dbg_print(NOTICE, "dev idx: %u, device addr: %02X:%02X:%02X:%02X:%02X:%02X\r\n", elt_idx,
               p_dev_info->peer_addr.addr[5], p_dev_info->peer_addr.addr[4], p_dev_info->peer_addr.addr[3],
               p_dev_info->peer_addr.addr[2], p_dev_info->peer_addr.addr[1], p_dev_info->peer_addr.addr[0]);
        elt_idx++;
    }
}

/*!
    \brief      Find scanned device information by index in scanned device list
    \param[in]  idx: scanned device index
    \param[out] none
    \retval     dev_info_t *: pointer to the scanned device information found
*/
dev_info_t *scan_mgr_find_dev_by_idx(uint8_t idx)
{
    dev_info_t *p_dev_info = NULL;
    dlist_t *pos, *n;
    uint8_t elt_idx = 0;

    if (list_empty(&ble_scan_mgr_cb.devs_list)) {
        return NULL;
    }

    list_for_each_safe(pos, n, &ble_scan_mgr_cb.devs_list) {
        p_dev_info = list_entry(pos, dev_info_t, list);
        if (elt_idx == idx) {
            return p_dev_info;
        }

        elt_idx++;
    }
    return NULL;
}

/*!
    \brief      Clear scanned device list
    \param[in]  none
    \param[out] none
    \retval     none
*/
void scan_mgr_clear_dev_list(void)
{
    dev_info_t *p_dev_info = NULL;
    dlist_t *pos, *n;

    if (list_empty(&ble_scan_mgr_cb.devs_list)) {
        return;
    }

    list_for_each_safe(pos, n, &ble_scan_mgr_cb.devs_list) {
        p_dev_info = list_entry(pos, dev_info_t, list);
        list_del(&p_dev_info->list);
        sys_mfree((void *)p_dev_info);
    }
}

/*!
    \brief      Enable scan
    \param[in]  update_rssi: true to update scanned device list when RSSI changes
    \param[out] none
    \retval     none
*/
void app_scan_enable(bool update_rssi)
{
    if (ble_scan_enable() != BLE_ERR_NO_ERROR) {
        dbg_print(NOTICE, "app_scan_enable fail!\r\n");
        return;
    }

    ble_scan_mgr_cb.update_with_rssi = update_rssi;
}

/*!
    \brief      Disable scan
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_scan_disable(void)
{
    if (ble_scan_disable() != BLE_ERR_NO_ERROR) {
        dbg_print(NOTICE, "app_scan_disable fail!\r\n");
        return;
    }

    ble_scan_mgr_cb.update_with_rssi = false;
}

/*!
    \brief      Reset application scan manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_scan_mgr_reset(void)
{
    scan_mgr_clear_dev_list();
    ble_scan_mgr_cb.update_with_rssi = false;
}

/*!
    \brief      Init application scan manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_scan_mgr_init(void)
{
    memset(&ble_scan_mgr_cb, 0, sizeof(ble_scan_mgr_cb));
    INIT_DLIST_HEAD(&ble_scan_mgr_cb.devs_list);
    ble_scan_callback_register(ble_app_scan_mgr_evt_handler);
}

/*!
    \brief      Deinit application scan manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_scan_mgr_deinit(void)
{
    scan_mgr_clear_dev_list();
    memset(&ble_scan_mgr_cb, 0, sizeof(ble_scan_mgr_cb));
    ble_scan_callback_unregister(ble_app_scan_mgr_evt_handler);
}

#endif // (BLE_APP_SUPPORT && (BLE_CFG_ROLE & (BLE_CFG_ROLE_OBSERVER | BLE_CFG_ROLE_CENTRAL)))
