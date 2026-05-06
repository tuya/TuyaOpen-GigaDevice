/*!
    \file    ble_storage.h
    \brief   Implementation of the BLE storage.

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

#ifndef _BLE_STORAGE_H_
#define _BLE_STORAGE_H_

#include <stdint.h>
#include <string.h>
#include "ble_gap.h"
#include "ble_error.h"

/* Max number of peer devices can be stored */
#define BLE_PEER_NUM_MAX            8           /*!< !Keep unchanged! */

/*!
    \brief      Store peer bond infomation
    \param[in]  addr: pointer to device address
    \param[in]  bond_data: pointer to bond data to store
    \param[out] none
    \retval     ble_status_t:  BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_peer_data_bond_store(ble_gap_addr_t *addr, ble_gap_sec_bond_data_t *bond_data);

/*!
    \brief      Load peer bond infomation
    \param[in]  addr: pointer to device address
    \param[out] bond_data: pointer to bond data loaded
    \retval     ble_status_t:  BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_peer_data_bond_load(ble_gap_addr_t *addr, ble_gap_sec_bond_data_t *bond_data);

/*!
    \brief      Delete bond data from flash
    \param[in]  addr: pointer to device address
    \param[out] none
    \retval     ble_status_t:  BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_peer_data_delete(ble_gap_addr_t *addr);

/*!
    \brief      Load bond device address from flash
    \param[in]  num: pointer to max number of device address to load
    \param[out] num: pointer to number of device address that successfully loaded
    \param[out] id_addrs: pointer to array of device address loaded
    \retval     ble_status_t:  BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_peer_all_addr_get(uint8_t *num, ble_gap_addr_t *id_addrs);

/*!
    \brief      Service load date
    \param[in]  conn_idx: connection index
    \param[in]  data_id: specified id for service date save
    \param[out] pp_data: pointer of data pointer which point to service data according to data_id
    \param[out] p_len: point to date length
    \retval     ble_status_t:  BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_svc_data_load(uint8_t conn_idx, uint16_t data_id, void **pp_data, uint32_t *p_len);

/*!
    \brief      Service save date
    \param[in]  conn_idx: connection index
    \param[in]  data_id: specified id for service date save
    \param[in]  len: date length
    \param[in]  p_data: point to date
    \retval     ble_status_t:  BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_svc_data_save(uint8_t conn_idx, uint16_t data_id, uint32_t len, uint8_t *p_data);
#endif // _BLE_STORAGE_H_
