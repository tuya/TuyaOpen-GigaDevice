/*!
    \file    ble_datatrans_srv.h
    \brief   Header file of ble datatrans server

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

#ifndef _BLE_DTATRANS_SRV_H_
#define _BLE_DTATRANS_SRV_H_

#include <stdint.h>
#include "ble_gatts.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Prototype of BLE datatrans server data receive callback function */
typedef void (*ble_datatrans_srv_rx_cb)(uint8_t conn_idx, uint16_t data_len, uint8_t *p_data);

/*!
    \brief      BLE datatrans server service rx callback register
    \param[in]  callback: datatrans server callback function
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_srv_rx_cb_reg(ble_datatrans_srv_rx_cb callback);

/*!
    \brief      BLE datatrans server service rx callback unregister
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_srv_rx_cb_unreg(void);

/*!
    \brief      Init BLE datatrans server service
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_srv_init(void);

/*!
    \brief      Deinit BLE datatrans server service
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_srv_deinit(void);

/*!
    \brief      BLE datatrans server transmit data to client
    \param[in]  conn_idx: connection index
    \param[in]  p_buf: pointer to transmit data buffer
    \param[in]  len: transmit data length
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_srv_tx(uint8_t conn_idx, uint8_t *p_buf, uint16_t len);

/*!
    \brief      BLE datatrans server multiple transmit data to client
    \param[in]  conidx_bf: connection index bit field
    \param[in]  p_buf: pointer to transmit data buffer
    \param[in]  len: transmit data length
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_datatrans_srv_tx_mtp(uint32_t conidx_bf, uint8_t *p_buf, uint16_t len);

#ifdef __cplusplus
}
#endif
#endif // _BLE_DTATRANS_SRV_H_
