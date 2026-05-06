/*!
    \file    ble_proxm.h
    \brief   Header file of Proximity Monitor Profile.

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

#ifndef _BLE_PROXM_H_
#define _BLE_PROXM_H_

#include <stdint.h>
#include "ble_error.h"
#include "ble_prox_comm.h"

/* Callbacks of the Proximity monitor */
typedef struct ble_proxm_callbacks
{
    void (*read_tx_pwr_cb)(uint8_t conn_id, uint8_t tx_pwr);
    void (*read_lls_altert_cb)(uint8_t conn_id, proxm_alert_lvl_t lvl);
    void (*found_service_cb)(uint8_t conn_id, bool found);
} ble_proxm_callbacks_t;

/*!
    \brief      Write lls character
    \param[in]  conn_id: connection index
    \param[in]  alert_lvl: alert level value
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_proxm_write_lls_char_value(uint8_t conn_id, proxm_alert_lvl_t alert_lvl);

/*!
    \brief      Write ias character
    \param[in]  conn_id: connection index
    \param[in]  alert_lvl: alert level value
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_proxm_write_ias_char_value(uint8_t conn_id, proxm_alert_lvl_t alert_lvl);

/*!
    \brief      Read lls character
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_proxm_read_lls_char_value(uint8_t conn_id);

/*!
    \brief      Read tx power character
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_proxm_read_tx_pwr_char_value(uint8_t conn_id);

/*!
    \brief      Init proximity monitor
    \param[in]  callbacks: proximity callback set
    \param[in]  dft_lvl: default alert level
    \param[out] none
    \retval     none
*/
void ble_proxm_init(ble_proxm_callbacks_t callbacks, proxm_alert_lvl_t dft_lvl);

#endif // _BLE_PROXM_H_
