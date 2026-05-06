/*!
    \file    ble_list.h
    \brief   Module for handling the BLE list operation.

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

#ifndef _BLE_LIST_H__
#define _BLE_LIST_H__

#include <stdint.h>

#include "ble_gap.h"
#include "ble_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Enumeration for list types */
typedef enum
{
    BLE_FAL_TYPE = 1,           /*!< Filter accept list */
    BLE_RAL_TYPE,               /*!< Resolving list */
    BLE_PAL_TYPE,               /*!< Periodic advertiser list */
} list_type_t;

/* Enumeration for operation types */
typedef enum
{
    RMV_DEVICE_FROM_LIST = 0,   /*!< Remove device from list */
    ADD_DEVICE_TO_LIST,         /*!< Add device to list */
    SET_DEVICES_TO_LIST,        /*!< Set list */
    CLEAR_DEVICE_LIST,          /*!< Clear list */
    GET_LOC_RPA,                /*!< Get local RPA */
    GET_PEER_RPA,               /*!< Get perr RPA */
} op_type_t;

/* Enumeration for list operation events */
typedef enum
{
    BLE_LIST_EVT_OP_RSP,                  /*!< Event notify for operate list response */
    BLE_LIST_EVT_LOC_RPA_GET_RSP,         /*!< Event notify for get local rpa response */
    BLE_LIST_EVT_PEER_RPA_GET_RSP,        /*!< Event notify for get peer rpa response */
} ble_list_evt_t;

/* List union data for list events */
typedef struct ble_list_data
{
    list_type_t   list_type;
    op_type_t     op_type;
    uint8_t       num;
    uint16_t      status;
    union op_data {
        const ble_gap_addr_t *p_fal_list;
        const ble_gap_ral_info_t *p_ral_list;
        const ble_gap_pal_info_t *p_pal_list;
        const ble_gap_addr_t     *p_rpa;
    } data;
} ble_list_data_t;

/* Prototype of BLE list event handler */
typedef void (*ble_list_evt_handler_t)(ble_list_evt_t event, ble_list_data_t *p_data);

/*!
    \brief      Register callback function to handle BLE list events
    \param[in]  callback: BLE list event handler function
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_list_callback_register(ble_list_evt_handler_t callback);

/*!
    \brief      Unregister callback function from handle BLE list module
    \param[in]  callback: BLE list event handler function
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_list_callback_unregister(ble_list_evt_handler_t callback);

/*!
    \brief      Add device to FAL or remove device from FAL
    \param[in]  p_addr_info: pointer to device address
    \param[in]  add: true to add to FAL, false to remove from FAL
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_fal_op(ble_gap_addr_t *p_addr_info, bool add);

/*!
    \brief      Set FAL
    \param[in]  num: number of provided address
    \param[in]  p_addr_info: pointer to array of address used for setting in FAL
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_fal_list_set(uint8_t num, ble_gap_addr_t *p_addr_info);

/*!
    \brief      Clear FAL
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_fal_clear(void);

/*!
    \brief      Get FAL size
    \param[in]  none
    \param[out] none
    \retval     uint8_t: FAL size
*/
uint8_t ble_fal_size_get(void);

/*!
    \brief      Add device to RAL or remove device from RAL
    \param[in]  p_ral_info: pointer to RAL information of the device
    \param[in]  add: true to add to RAL, false to remove from RAL
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_ral_op(ble_gap_ral_info_t *p_ral_info, bool add);

/*!
    \brief      Set RAL
    \param[in]  num: number of provided RAL information
    \param[in]  p_ral_info: pointer to array of RAL information used for setting in RAL
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_ral_list_set(uint8_t num, ble_gap_ral_info_t *p_ral_info);

/*!
    \brief      Clear RAL
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_ral_clear(void);

/*!
    \brief      Get RAL size
    \param[in]  none
    \param[out] none
    \retval     uint8_t: RAL size
*/
uint8_t ble_ral_size_get(void);

/*!
    \brief      Get local RPA
    \param[in]  p_peer_id: pointer to peer identity address
    \param[in]  peer_id_type: peer identity address type
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_loc_rpa_get(uint8_t *p_peer_id, uint8_t peer_id_type);

/*!
    \brief      Get peer RPA
    \param[in]  p_peer_id: pointer to peer identity address
    \param[in]  peer_id_type: peer identity address type
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_peer_rpa_get(uint8_t *p_peer_id, uint8_t peer_id_type);

/*!
    \brief      Add device to PAL or remove device from PAL
    \param[in]  p_pal_info: pointer to PAL information of the device
    \param[in]  add: true to add to PAL, false to remove from PAL
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_pal_op(ble_gap_pal_info_t *p_pal_info, bool add);

/*!
    \brief      Set PAL
    \param[in]  num: number of provided PAL information
    \param[in]  p_pal_info: pointer to array of PAL information used for setting in PAL
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_pal_list_set(uint8_t num, ble_gap_pal_info_t *p_pal_info);

/*!
    \brief      Clear PAL
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_pal_clear(void);

/*!
    \brief      Get PAL size
    \param[in]  none
    \param[out] none
    \retval     uint8_t: PAL size
*/
uint8_t ble_pal_size_get(void);

#ifdef __cplusplus
}
#endif

#endif // _BLE_LIST_H__
