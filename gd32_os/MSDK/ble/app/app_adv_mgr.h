/*!
    \file    app_adv_mgr.h
    \brief   Header file for application Advertising Manager.

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

#ifndef APP_ADV_MGR_H_
#define APP_ADV_MGR_H_

#include "ble_gap.h"

/* BLE advertising type */
#define BLE_ADV_TYPE_LEGACY     0       /*!< Legacy advertising */
#define BLE_ADV_TYPE_EXTENDED   1       /*!< Extended advertising */
#define BLE_ADV_TYPE_PERIODIC   2       /*!< Periodic advertising */

/* Default adv type */
#define DEFAULT_ADV_TYPE        BLE_ADV_TYPE_LEGACY

/* Minimum advertising interval - 100ms */
#define APP_ADV_INT_MIN         (160)
/* Maximum advertising interval - 100ms */
#define APP_ADV_INT_MAX         (160)

/* Advertising parameters */
typedef struct
{
    uint8_t type;               /*!< advertising type, @ref ble_gap_adv_type_t */
    uint16_t prop;              /*!< Advertising properties. @ref ble_gap_legacy_adv_prop_t for legacy advertising,
                                     @ref ble_gap_extended_adv_prop_t for extended advertising,
                                     @ref ble_gap_periodic_adv_prop_t for periodic advertising */
    uint8_t pri_phy;            /*!< Indicate on which PHY primary advertising has to be performed, @ref ble_gap_phy_t */
    uint8_t sec_phy;            /*!< Indicate on which PHY secondary advertising has to be performed, @ref ble_gap_phy_t */
    bool wl_enable;             /*!< True to use whitelist, otherwise do not use */
    uint8_t own_addr_type;      /*!< Own address type used in advertising, @ref ble_gap_local_addr_type_t */
    uint8_t disc_mode;          /*!< Discovery mode, @ref ble_gap_adv_discovery_mode_t */
    uint16_t max_data_len;      /*!< Max advertising data length */
    uint8_t ch_map;             /*!< Channel map */
    uint32_t adv_intv;          /*!< Adv interval */
    ble_gap_addr_t peer_addr;   /*!< Peer address, used for directed advertising */
} app_adv_param_t;

/*!
    \brief      Set advertising data
    \param[in]  p_data: pointer to data to set to adv data
    \param[in]  len: data length
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t app_adv_set_adv_data(uint8_t *p_data, uint16_t len);

/*!
    \brief      Set scan response data
    \param[in]  p_data: pointer to data to set to scan response data
    \param[in]  len: data length
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t app_adv_set_scan_rsp_data(uint8_t *p_data, uint16_t len);

/*!
    \brief      Create an advertising
    \param[in]  p_param: pointer to advertising parameters
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t app_adv_create(app_adv_param_t *p_param);

/*!
    \brief      Stop an advertising if it is started
    \param[in]  idx: local advertising set index
    \param[in]  rmv_adv: true to remove advertising set after it is stopped, otherwise false
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t app_adv_stop(uint8_t idx, bool rmv_adv);

/*!
    \brief      Restart an advertising if it is stopped
    \param[in]  idx: local advertising set index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t app_adv_restart(uint8_t idx);

/*!
    \brief      Update advertising data
    \param[in]  idx: local advertising set index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t app_adv_data_update(uint8_t idx);

/*!
    \brief      Update advertising data for all advertising set
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_adv_data_update_all(void);

/*!
    \brief      Choose advertising data to be used
    \param[in]  adv_data_type: advertising data type index
    \param[out] none
    \retval     none
*/
void app_set_adv_data_type(uint8_t adv_data_type);

/*!
    \brief      Reset APP advertising manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_adv_mgr_reset(void);

/*!
    \brief      Init APP advertising manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_adv_mgr_init(void);

/*!
    \brief      Deinit APP advertising manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_adv_mgr_deinit(void);

#endif // APP_ADV_MGR_H_
