/*!
    \file    ble_diss.h
    \brief   Header file of device information service server.

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

#ifndef _BLE_DISS_H_
#define _BLE_DISS_H_

#include <stdint.h>
#include "ble_types.h"
#include "ble_error.h"
#include "ble_gap.h"

/* Max length for Characteristic values */
#define BLE_DIS_VAL_MAX_LEN                     (31)

/* Vendor ID Source */
typedef enum
{
    BLE_DIS_VND_ID_SRC_BLUETOOTH_SIG  = 1,  /*!< Vendor ID assigned by Bluetooth SIG */
    BLE_DIS_VND_ID_SRC_USB_IMPL_FORUM = 2,  /*!< Vendor ID assigned by USB Implementer's Forum */
} ble_dis_vnd_id_src_t;

/* IEEE 11073 authoritative body value */
typedef enum
{
    BLE_DIS_IEEE_11073_BODY_EMPTY = 0,
    BLE_DIS_IEEE_11073_BODY_IEEE  = 1,
    BLE_DIS_IEEE_11073_BODY_CONT  = 2,
    BLE_DIS_IEEE_11073_BODY_EXP   = 254,
} ble_dis_ieee_body_t;

/* System ID */
typedef struct
{
    uint64_t manufact_id;               /*!< Manufacturer ID. Only 5 LSOs shall be used */
    uint32_t oui;                       /*!< Organizationally unique ID. Only 3 LSOs shall be used */
} ble_dis_sys_id_t;

/* PnP ID */
typedef struct
{
    uint8_t  vendor_id_source;          /*!< Vendor ID Source, @ref ble_dis_vnd_id_src_t */
    uint16_t vendor_id;                 /*!< Vendor ID */
    uint16_t product_id;                /*!< Product ID */
    uint16_t product_version;           /*!< Product Version */
} ble_dis_pnp_id_t;

/* Device Information Service init parameter structure */
typedef struct
{
    ble_data_t          manufact_name;      /*!< Manufacturer Name String */
    ble_data_t          model_num;          /*!< Model Number String */
    ble_data_t          serial_num;         /*!< Serial Number String */
    ble_data_t          hw_rev;             /*!< Hardware Revision String */
    ble_data_t          fw_rev;             /*!< Firmware Revision String */
    ble_data_t          sw_rev;             /*!< Software Revision String */
    ble_data_t          ieee_data;          /*!< IEEE Regulatory Certification Data List */
    ble_dis_sys_id_t   *p_sys_id;           /*!< System ID */
    ble_dis_pnp_id_t   *p_pnp_id;           /*!< PnP ID */
    ble_gap_sec_lvl_t   sec_lvl;            /*!< Security level required to access the service */
} ble_diss_init_param_t;

/*!
    \brief      Init Device Information Service Server
    \param[in]  p_param: pointer to Device Information Service init parameters
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_diss_init(ble_diss_init_param_t *p_param);

/*!
    \brief      Deinit Device Information Service Server
    \param[in]  nont
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_diss_deinit(void);

#endif // _BLE_DISS_H_
