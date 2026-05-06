/*!
    \file    app_dev_mgr.c
    \brief   Implementation of BLE application dev manager to record paired or connected devices.

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

#if (BLE_APP_SUPPORT)
#include <string.h>
#include "app_dev_mgr.h"
#include "app_adapter_mgr.h"

#include "wrapper_os.h"
#include "ble_storage.h"
#include "dbg_print.h"

/* Application device manager environment structure */
struct ble_dev_mgr
{
    dlist_t     sec_dev_list;
};

/* Application device manager environment data */
static struct ble_dev_mgr device_mgr;

/*!
    \brief      Check if the device address can be resolved by given IRK
    \param[in]  p_addr: pointer to device address
    \param[in]  p_device: pointer to device information which contains IRK information
    \param[out] none
    \retval     bool: true if the address can be resolved by the given IRK in the device information
*/
static bool ble_addr_resolvable(ble_gap_addr_t *p_addr, ble_device_t *p_device)
{
    if (p_addr->addr_type != BLE_GAP_ADDR_TYPE_RANDOM ||
        BLE_IS_RESOLVE_BDA(p_addr->addr) != true) {
        return false;
    }

    if (p_device->bond_info.key_msk & BLE_GAP_KDIST_IDKEY) {
        if (ble_gap_rpa_matches_irk(p_addr->addr, p_device->bond_info.peer_irk.irk)) {
            dbg_print(NOTICE, "ble_addr_resolvable match irk %02X:%02X:%02X:%02X:%02X:%02X \r\n",
                   p_device->cur_addr.addr[5], p_device->cur_addr.addr[4], p_device->cur_addr.addr[3],
                   p_device->cur_addr.addr[2], p_device->cur_addr.addr[1], p_device->cur_addr.addr[0]);

            return true;
        }
    }

    return false;
}

/*!
    \brief      Clear list flag of all the devices
    \param[in]  wl: true to clear FAL list, otherwise clear RAL list
    \param[out] none
    \retval     none
*/
void dm_clear_all_dev_list_flag(bool wl)
{
    dlist_t *pos, *n;
    ble_device_t *p_device;

    if (list_empty(&device_mgr.sec_dev_list)) {
        return;
    }

    list_for_each_safe(pos, n, &device_mgr.sec_dev_list) {
        p_device = list_entry(pos, ble_device_t, list);
        if (wl) {
            p_device->in_wl = false;
        } else {
            p_device->in_ral = false;
        }
    }
}

/*!
    \brief      Function to handle connection disconnected event
    \param[in]  conidx: connection index
    \param[out] none
    \retval     none
*/
void dm_handle_dev_disconnected(uint8_t conidx)
{
    ble_device_t *p_device = dm_find_dev_by_conidx(conidx);

    if (p_device == NULL) {
        return;
    }

    if (p_device->pending_remove && p_device->bonded) {
        ble_peer_data_delete(&p_device->cur_addr);
        memset(&p_device->bond_info, 0, sizeof(ble_gap_sec_bond_data_t));
        p_device->bonded = false;
    }

    if (!p_device->bonded && !p_device->in_wl && !p_device->in_ral) {
        list_del(&p_device->list);
        sys_mfree(p_device);
    } else {
        p_device->conn_hdl = BLE_CONN_HANDLE_INVALID;
        p_device->conn_idx = BLE_CONN_CONIDX_INVALID;
        p_device->state = BLE_CONN_STATE_DISCONNECTD;
        p_device->encry_cmplt = false;
        p_device->update_state = IDLE_STATE;
        p_device->enable_fast_param = false;
    }
}

/*!
    \brief      Remove device with specific address from the device list
    \param[in]  address: device address
    \param[out] none
    \retval     none
*/
void dm_remove_dev_by_addr(ble_gap_addr_t address)
{
    dlist_t *pos, *n;
    ble_device_t *p_device = NULL;
    bool found = false;

    if (list_empty(&device_mgr.sec_dev_list)) {
        return;
    }

    list_for_each_safe(pos, n, &device_mgr.sec_dev_list) {
        p_device = list_entry(pos, ble_device_t, list);
        if (!memcmp(&p_device->cur_addr, &address, sizeof(ble_gap_addr_t))) {
            found = true;
            break;
        }

        if (!memcmp(&p_device->bond_info.peer_irk.identity, &address, sizeof(ble_gap_addr_t))) {
            found = true;
            break;
        }

        if (ble_addr_resolvable(&address, p_device)) {
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
    \brief      Allocate device by address and put it in the device list
    \param[in]  address: device address
    \param[out] none
    \retval     ble_device_t *: pointer to the allocated device information
*/
ble_device_t *dm_alloc_dev_by_addr(ble_gap_addr_t address)
{
    ble_status_t ret;
    ble_device_t *p_device = NULL;

    p_device = (ble_device_t *)sys_malloc(sizeof(ble_device_t));

    if (p_device == NULL) {
        return NULL;
    }
    memset(p_device, 0, sizeof(ble_device_t));

    INIT_DLIST_HEAD(&p_device->list);
    memcpy(&p_device->cur_addr, &address, sizeof(ble_gap_addr_t));

    if (address.addr_type == BLE_GAP_ADDR_TYPE_PUBLIC ||
        address.addr_type == BLE_GAP_ADDR_TYPE_RANDOM) {
        memcpy(&p_device->bond_info.peer_irk.identity, &address, sizeof(ble_gap_addr_t));
    }

    /* loading bond info */
    ret = ble_peer_data_bond_load(&p_device->cur_addr, &p_device->bond_info);
    if (ret == BLE_ERR_NO_ERROR) {
        p_device->bonded = true;
    }

    p_device->conn_idx = BLE_CONN_CONIDX_INVALID;
    p_device->conn_hdl = BLE_CONN_HANDLE_INVALID;

    list_add_tail(&p_device->list, &device_mgr.sec_dev_list);
    return p_device;
}

/*!
    \brief      Find device information in the device list by address
    \param[in]  address: device address
    \param[out] none
    \retval     ble_device_t *: pointer to the device information found, NULL if no such device
*/
ble_device_t *dm_find_dev_by_addr(ble_gap_addr_t address)
{
    dlist_t *pos, *n;
    ble_device_t *p_device;

    if (list_empty(&device_mgr.sec_dev_list)) {
        return NULL;
    }

    list_for_each_safe(pos, n, &device_mgr.sec_dev_list) {
        p_device = list_entry(pos, ble_device_t, list);
        if (!memcmp(&p_device->cur_addr, &address, sizeof(ble_gap_addr_t))) {
            return p_device;
        }

        if (!memcmp(&p_device->bond_info.peer_irk.identity, &address, sizeof(ble_gap_addr_t))) {
            return p_device;
        }

        if (ble_addr_resolvable(&address, p_device)) {
            return p_device;
        }
    }

    return NULL;
}

/*!
    \brief      Find device information in the device list by address, if not in the list, allocate one
    \param[in]  address: device address
    \param[out] none
    \retval     ble_device_t *: pointer to the device information found or allocated
*/
ble_device_t *dm_find_alloc_dev_by_addr(ble_gap_addr_t address)
{
    ble_device_t *p_device = dm_find_dev_by_addr(address);
    if (p_device != NULL) {
        return p_device;
    }

    p_device = dm_alloc_dev_by_addr(address);
    return p_device;
}

/*!
    \brief      Find device information in the device list by connection index
    \param[in]  conidx: connection index
    \param[out] none
    \retval     ble_device_t *: pointer to the device information found, NULL if no such device
*/
ble_device_t *dm_find_dev_by_conidx(uint8_t conidx)
{
    dlist_t *pos, *n;
    ble_device_t *p_device;

    if (list_empty(&device_mgr.sec_dev_list)) {
        return NULL;
    }

    list_for_each_safe(pos, n, &device_mgr.sec_dev_list) {
        p_device = list_entry(pos, ble_device_t, list);
        if (p_device->conn_idx == conidx) {
            return p_device;
        }
    }

    return NULL;
}

/*!
    \brief      Find device information in the device list by device index
    \param[in]  idx: device index
    \param[out] none
    \retval     ble_device_t *: pointer to the device information found, NULL if no such device
*/
ble_device_t *dm_find_dev_by_idx(uint8_t idx)
{
    dlist_t *pos, *n;
    ble_device_t *p_device;
    uint8_t cnt = 0;

    if (list_empty(&device_mgr.sec_dev_list)) {
        return NULL;
    }

    list_for_each_safe(pos, n, &device_mgr.sec_dev_list) {
        p_device = list_entry(pos, ble_device_t, list);
        if (cnt == idx) {
            return p_device;
        }
        cnt++;
    }

    return NULL;
}

/*!
    \brief      List securty devices callback
    \param[in]  dev_idx: device index
    \param[in]  p_device: pointer to device information
    \param[out] none
    \retval     none
*/
void dm_list_sec_devices_cb(uint8_t dev_idx, ble_device_t *p_device)
{
    uint8_t i;

    dbg_print(NOTICE, "======= dev idx %u =========\r\n", dev_idx);
    dbg_print(NOTICE, "-->   sec device cur_addr %02X:%02X:%02X:%02X:%02X:%02X \r\n",
           p_device->cur_addr.addr[5], p_device->cur_addr.addr[4], p_device->cur_addr.addr[3],
           p_device->cur_addr.addr[2], p_device->cur_addr.addr[1], p_device->cur_addr.addr[0]);
    dbg_print(NOTICE, "-->   sec device id_addr %02X:%02X:%02X:%02X:%02X:%02X \r\n",
           p_device->bond_info.peer_irk.identity.addr[5], p_device->bond_info.peer_irk.identity.addr[4],
           p_device->bond_info.peer_irk.identity.addr[3], p_device->bond_info.peer_irk.identity.addr[2],
           p_device->bond_info.peer_irk.identity.addr[1], p_device->bond_info.peer_irk.identity.addr[0]);
    if (p_device->bonded) {
        if (p_device->bond_info.key_msk & BLE_LOC_LTK_ENCKEY) {
            dbg_print(NOTICE, "local key size %d, ltk(hex): ", p_device->bond_info.local_ltk.key_size);

            for (i = 0; i < BLE_GAP_KEY_LEN; i++) {
                dbg_print(NOTICE, "%02x", p_device->bond_info.local_ltk.ltk[i]);
            }
            dbg_print(NOTICE, "\r\n");
        }

        if (p_device->bond_info.key_msk & BLE_PEER_LTK_ENCKEY) {
            dbg_print(NOTICE, "peer key size %d, ltk(hex): ", p_device->bond_info.peer_ltk.key_size);

            for (i = 0; i < BLE_GAP_KEY_LEN; i++) {
                dbg_print(NOTICE, "%02x", p_device->bond_info.peer_ltk.ltk[i]);
            }
            dbg_print(NOTICE, "\r\n");
        }

        if (p_device->bond_info.key_msk & BLE_PEER_IDKEY) {
            dbg_print(NOTICE, "peer irk(hex): ");

            for (i = 0; i < BLE_GAP_KEY_LEN; i++) {
                dbg_print(NOTICE, "%02x", p_device->bond_info.peer_irk.irk[i]);
            }
            dbg_print(NOTICE, "\r\n");
        }

        if (p_device->bond_info.key_msk & BLE_LOC_CSRK) {
            dbg_print(NOTICE, "local csrk(hex): ");

            for (i = 0; i < BLE_GAP_KEY_LEN; i++) {
                dbg_print(NOTICE, "%02x", p_device->bond_info.local_csrk.csrk[i]);
            }
            dbg_print(NOTICE, "\r\n");
        }

        if (p_device->bond_info.key_msk & BLE_PEER_CSRK) {
            dbg_print(NOTICE, "peer csrk(hex): ");

            for (i = 0; i < BLE_GAP_KEY_LEN; i++) {
                dbg_print(NOTICE, "%02x", p_device->bond_info.peer_csrk.csrk[i]);
            }
            dbg_print(NOTICE, "\r\n");
        }
    }
}

/*!
    \brief      List all the device information in the list
    \param[in]  none
    \param[out] none
    \retval     none
*/
void dm_list_sec_devices(p_dm_list_sec_devices_cb cb)
{
    dlist_t *pos, *n;
    ble_device_t *p_device;
    uint8_t elt_idx = 0;

    if (cb == NULL)
        return;

    if (list_empty(&device_mgr.sec_dev_list)) {
        dbg_print(NOTICE, "======= list empty =========\r\n");
        return;
    }

    list_for_each_safe(pos, n, &device_mgr.sec_dev_list) {
        p_device = list_entry(pos, ble_device_t, list);
        cb(elt_idx, p_device);
        elt_idx++;
    }
}

/*!
    \brief      dm get connection index bit field
    \param[in]  none
    \param[out] none
    \retval     uint32_t: connection index bit field
*/
uint32_t dm_get_conidx_bf(void)
{
    uint32_t conidx_bf = 0;
    dlist_t *pos, *n;
    ble_device_t *p_device;

    if (list_empty(&device_mgr.sec_dev_list)) {
        return conidx_bf;
    }

    list_for_each_safe(pos, n, &device_mgr.sec_dev_list) {
        p_device = list_entry(pos, ble_device_t, list);
        if (p_device->conn_idx < BLE_PEER_NUM_MAX) {
            conidx_bf |= 1 << p_device->conn_idx;
        }
    }

    return conidx_bf;
}

/*!
    \brief      dm check connection valid
    \param[in]  conn_idx: connection index
    \param[out] none
    \retval     bool: true if connection is valid, otherwise false
*/
bool dm_check_connection_valid(uint8_t conn_idx)
{
    dlist_t *pos, *n;
    ble_device_t *p_device;

    if (list_empty(&device_mgr.sec_dev_list)) {
        return false;
    }

    if (conn_idx == BLE_CONN_CONIDX_INVALID)
        return false;

    list_for_each_safe(pos, n, &device_mgr.sec_dev_list) {
        p_device = list_entry(pos, ble_device_t, list);
        if (p_device->conn_idx == conn_idx) {
            return true;
        }
    }

    return false;
}

/*!
    \brief      Reset application device manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_dm_reset(void)
{
    dlist_t *pos, *n;
    ble_device_t *p_device;
    uint8_t i, num = BLE_PEER_NUM_MAX;
    ble_gap_addr_t addr_array[BLE_PEER_NUM_MAX];

    if (list_empty(&device_mgr.sec_dev_list)) {
        return;
    }

    list_for_each_safe(pos, n, &device_mgr.sec_dev_list) {
        p_device = list_entry(pos, ble_device_t, list);
        list_del(&p_device->list);
        sys_mfree(p_device);
    }

    //load bonded information
    if (ble_peer_all_addr_get(&num, addr_array) == BLE_ERR_NO_ERROR) {
        for (i = 0; i < num; i++) {
            dm_alloc_dev_by_addr(addr_array[i]);
        }
    }
}

/*!
    \brief      Init application device manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_dm_init(void)
{
    uint8_t i, num = BLE_PEER_NUM_MAX;
    ble_gap_addr_t addr_array[BLE_PEER_NUM_MAX];
    memset(&device_mgr, 0, sizeof(struct ble_dev_mgr));
    INIT_DLIST_HEAD(&device_mgr.sec_dev_list);

    //load bonded information
    if (ble_peer_all_addr_get(&num, addr_array) == BLE_ERR_NO_ERROR) {
        for (i = 0; i < num; i++) {
            dm_alloc_dev_by_addr(addr_array[i]);
        }
    }
}

/*!
    \brief      Deinit application device manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_dm_deinit(void)
{
    dlist_t *pos, *n;
    ble_device_t *p_device = NULL;

    if (list_empty(&device_mgr.sec_dev_list)) {
        return;
    }

    list_for_each_safe(pos, n, &device_mgr.sec_dev_list) {
        p_device = list_entry(pos, ble_device_t, list);
        list_del(&p_device->list);
        sys_mfree(p_device);
    }
}
#endif // (BLE_APP_SUPPORT)
