/*!
    \file    app_per_sync_mgr.h
    \brief   Definitions of BLE application periodic sync manager to record devices.

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

#ifndef APP_PER_SYNC_MGR_H_
#define APP_PER_SYNC_MGR_H_

#include "dlist.h"
#include "ble_gap.h"
#include "ble_per_sync.h"

/* Periodic sync device information */
typedef struct per_dev_info
{
    dlist_t              list;              /*!< List Head */
    uint8_t              sync_idx;          /*!< Period sync index */
    bool                 in_pal;            /*!< Device exist in pal(periodic advertiser list) flag */
    uint8_t              phy;               /*!< PHY on which synchronization has been established (#ble_gap_phy) */
    uint8_t              clk_acc;           /*!< Advertiser clock accuracy (see enum #ble_gap_clock_accuracy) */
    uint16_t             period_adv_intv;   /*!< Periodic advertising interval (in unit of 1.25ms, min is 7.5ms) */
    uint16_t             serv_data;         /*!< Only valid for a Periodic Advertising Sync Transfer, otherwise ignore */
    ble_gap_pal_info_t   sync_info;         /*!< Periodic sync information */
    ble_per_sync_state_t state;             /*!< Periodic sync state */
} per_dev_info_t;

/*!
    \brief      Clear PAL flag of all devices in the list
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_per_sync_clear_all_dev_list_flag(void);

/*!
    \brief      Cancel ongoing periodic sync procedure
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_per_sync_cancel(void);

/*!
    \brief      Terminate periodic sync
    \param[in]  sync_idx: periodic sync index
    \param[out] none
    \retval     none
*/
void app_per_sync_terminate(uint8_t sync_idx);

/*!
    \brief      Find device in periodic sync manager list
    \param[in]  p_peer_addr: pointer to peer address
    \param[in]  adv_sid: advertising sid
    \param[out] none
    \retval     per_dev_info_t *: pointer to periodic sync device information found
*/
per_dev_info_t *ble_per_sync_mgr_find_device(ble_gap_addr_t *p_peer_addr, uint8_t adv_sid);

/*!
    \brief      Find device in periodic sync manager list, if no such device, allocate one
    \param[in]  p_peer_addr: pointer to peer address
    \param[in]  adv_sid: advertising sid
    \param[in]  period_adv_intv: periodic advertising interval
    \param[out] none
    \retval     per_dev_info_t *: pointer to periodic sync device information found or allocated
*/
per_dev_info_t *ble_per_sync_mgr_find_alloc_device(ble_gap_addr_t *p_peer_addr, uint8_t adv_sid,
                                                   uint16_t period_adv_intv);

/*!
    \brief      Reset application periodic sync manager
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_per_sync_mgr_reset(void);

/*!
    \brief      Find periodic sync device information by index
    \param[in]  sync_idx: periodic sync index
    \param[out] none
    \retval     per_dev_info_t *: pointer to periodic sync device information found
*/
per_dev_info_t *sync_mgr_find_device_by_idx(uint8_t sync_idx);
/*!
    \brief      Init application periodic sync manager
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_per_sync_mgr_init(void);

/*!
    \brief      Deinit application periodic sync manager
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_per_sync_mgr_deinit(void);

#endif // APP_PER_SYNC_MGR_H_
