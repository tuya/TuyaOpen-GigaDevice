/*!
    \file    ble_datatrans_cli.h
    \brief   Header file of ble datatrans client.

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

#ifndef _BLE_DATATRANS_CLI_H_
#define _BLE_DATATRANS_CLI_H_

#include <stdint.h>
#include "ble_error.h"
#include "ble_gatt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Prototype of BLE datatrans client data receive callback function */
typedef void (*ble_datatrans_cli_rx_cb)(uint8_t conn_idx, uint16_t data_len, uint8_t *p_data);

/*!
    \brief      BLE datatrans client service rx callback register
    \param[in]  callback: datatrans client callback function
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_cli_rx_cb_reg(ble_datatrans_cli_rx_cb callback);

/*!
    \brief      BLE datatrans client service rx callback unregister
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_cli_rx_cb_unreg(void);

/*!
    \brief      Init BLE datatrans client
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_cli_init(void);

/*!
    \brief      Deinit BLE datatrans client
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_cli_deinit(void);

/*!
    \brief      BLE datatrans client write characteristic
    \param[in]  conn_idx: connection index
    \param[in]  p_buf: buffer pointer
    \param[in]  len: buffer length
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_cli_write_char(uint8_t conn_idx, uint8_t *p_buf, uint16_t len);

/*!
    \brief      BLE datatrans client write cccd
    \param[in]  conn_idx: connection index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_cli_write_cccd(uint8_t conn_idx);

#ifdef __cplusplus
}
#endif

#endif // _BLE_DATATRANS_CLI_H_
