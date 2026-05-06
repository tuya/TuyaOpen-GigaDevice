/*!
    \file    ble_ota_srv.h
    \brief   Header file of ble ota server

    \version 2024-07-31, V1.0.0, firmware for GD32VW55x
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

#ifndef _BLE_OTA_SRV_H_
#define _BLE_OTA_SRV_H_

#include <stdint.h>
#include "ble_gatts.h"

#ifdef __cplusplus
extern "C" {
#endif

/* BLE ota related service/characteristic UUID value */
typedef enum
{
    BLE_GATT_SVC_OTA_SERVICE      = BLE_GATT_UUID_16_LSB(0xFF00),  /* BLE ota service UUID */
    BLE_GATT_SVC_OTA_DATA_CHAR    = BLE_GATT_UUID_16_LSB(0xFF11),  /* BLE ota data characteristic UUID */
    BLE_GATT_SVC_OTA_CONTROL_CHAR = BLE_GATT_UUID_16_LSB(0xFF22),  /* BLE ota control characteristic UUID */
} ble_ota_uuid_t;

/* Prototype of BLE ota server data receive callback function */
typedef void (*ble_ota_srv_rx_cb)(uint16_t data_len, uint8_t *p_data);
/* Prototype of BLE ota server data disconnct callback function */
typedef void (*ble_ota_disconn_cb)(uint8_t conn_idx);
/* Prototype of BLE ota server indication send callback function */
typedef void (*ble_ota_ind_send_cb)(uint8_t conn_idx);

typedef struct
{
    ble_ota_srv_rx_cb   ota_data_callback;    /* Rx data callback */
    ble_ota_srv_rx_cb   ota_control_callback; /* Rx cmd callback */
    ble_ota_disconn_cb  ota_disconn_callback; /* Rx disconnect callback */
    ble_ota_ind_send_cb ind_send_callback;    /* Rx ind send done callback */
} ble_ota_srv_callbacks_t;

/*!
    \brief      Init BLE ota server service
    \param[in]  p_ble_ota_callbacks: point to ota server callback function
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_ota_srv_init(ble_ota_srv_callbacks_t *p_callbacks);

/*!
    \brief      Deinit BLE ota server service
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_ota_srv_deinit(void);

/*!
    \brief      BLE ota server transmit data to client
    \param[in]  conn_idx: connection index
    \param[in]  p_buf: pointer to transmit data buffer
    \param[in]  len: transmit data length
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_ota_srv_tx(uint8_t conn_idx, uint8_t *p_buf, uint16_t len);

#ifdef __cplusplus
}
#endif
#endif // _BLE_OTA_SRV_H_
