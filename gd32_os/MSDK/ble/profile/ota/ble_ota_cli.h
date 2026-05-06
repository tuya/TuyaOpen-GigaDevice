/*!
    \file    ble_ota_cli.h
    \brief   Header file of ble ota client.

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

#ifndef _BLE_OTA_CLI_H_
#define _BLE_OTA_CLI_H_

#include <stdint.h>
#include "ble_error.h"
#include "ble_gatt.h"

/* BLE ota related service/characteristic UUID value */
typedef enum
{
    BLE_GATT_SVC_OTA_SERVICE      = BLE_GATT_UUID_16_LSB(0xFF00),  /* BLE ota service UUID */
    BLE_GATT_SVC_OTA_DATA_CHAR    = BLE_GATT_UUID_16_LSB(0xFF11),  /* BLE ota data characteristic UUID */
    BLE_GATT_SVC_OTA_CONTROL_CHAR = BLE_GATT_UUID_16_LSB(0xFF22),  /* BLE ota control characteristic UUID */
} ble_ota_uuid_t;

/* Prototype of BLE ota client data receive callback function */
typedef void (*ble_ota_cli_rx_cb)(uint16_t data_len, uint8_t *p_data);

/* Prototype of BLE ota client data tx done callback function */
typedef void (*ble_ota_cli_tx_cb)(ble_status_t status);

/* Prototype of BLE ota client data disconnection done callback function */
typedef void (*ble_ota_cli_disconn_cb)(uint8_t conn_idx);

/* Prototype of BLE ota client data tx done callback function */
typedef struct
{
    ble_ota_cli_rx_cb      ota_cli_rx_callback;      /*BLE ota client data receive callback function */
    ble_ota_cli_tx_cb      ota_cli_tx_callback;      /*BLE ota client data tx done callback function */
    ble_ota_cli_disconn_cb ota_cli_disconn_callback; /*BLE ota client disconnection function */
} ble_ota_cli_callbacks_t;

/*!
    \brief      BLE ota client write cmd
    \param[in]  conn_idx: connection index
    \param[in]  p_buf: buffer pointer
    \param[in]  len: buffer length
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_ota_cli_write_cmd(uint8_t conn_idx, uint8_t *p_buf, uint16_t len);

/*!
    \brief      BLE ota client write cccd
    \param[in]  conn_idx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_ota_cli_write_cmd_cccd(uint8_t conn_idx);

/*!
    \brief      BLE ota client write cmd
    \param[in]  conn_idx: connection index
    \param[in]  p_buf: buffer pointer
    \param[in]  len: buffer length
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_ota_cli_write_data(uint8_t conn_idx, uint8_t *p_buf, uint16_t len);

/*!
    \brief      Init BLE ota client
    \param[in]  p_callbacks: ota client callback function
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_ota_cli_init(ble_ota_cli_callbacks_t *p_callbacks);

/*!
    \brief      Deinit BLE ota client
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_ota_cli_deinit(void);
#endif // _BLE_OTA_CLI_H_
