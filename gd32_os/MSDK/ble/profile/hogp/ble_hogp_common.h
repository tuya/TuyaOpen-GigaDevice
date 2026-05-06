/*!
    \file    ble_hogp_common.h
    \brief   Header file of hogp common information.

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

#ifndef _BLE_HOGP_COMMON_H_
#define _BLE_HOGP_COMMON_H_

#include "ble_types.h"
#include <stdint.h>
#include "ble_gatt.h"

/* GATT 16-bit Universal Hogp Identifier */
enum
{
    BLE_HOGP_SVC_HID               = BLE_GATT_UUID_16_LSB(0x1812),   /**< Hid Over Gatt Protocal Service. */
    BLE_HOGP_SVC_HID_INFO          = BLE_GATT_UUID_16_LSB(0x2A4A),   /**< HID Information. */
    BLE_HOGP_SVC_REPORT_MAP        = BLE_GATT_UUID_16_LSB(0x2A4B),   /**< Report Map. */
    BLE_HOGP_SVC_HID_CTNL_PT       = BLE_GATT_UUID_16_LSB(0x2A4C),   /**< HID Control Point. */
    BLE_HOGP_SVC_REPORT            = BLE_GATT_UUID_16_LSB(0x2A4D),   /**< Report. */
    BLE_HOGP_SVC_PROTOCOL_MODE     = BLE_GATT_UUID_16_LSB(0x2A4E),   /**< Protocol Mode. */
    BLE_HOGP_SVC_BOOT_KB_IN_REPORT = BLE_GATT_UUID_16_LSB(0x2A22),   /**< Boot Keyboard Input Report. */
};

/**< HID Information bit values. */
enum hogp_info_bit
{
    HIDS_REMOTE_WAKE_CAPABLE           = 0x01,   /**< Device capable of providing wake-up signal to a HID host. */
    HIDS_NORM_CONNECTABLE              = 0x02,   /**< Normally connectable support bit. */
};

/**< HID Control Point Characteristic value keys. */
enum hogp_ctnl_pt
{
    HOGP_CTNL_PT_SUSPEND            = 0x00,   /**< Suspend. */
    HOGP_CTNL_PT_EXIT_SUSPEND,                /**< Exit suspend. */
};

/**< Protocol Mode Char. value Keys. */
enum hogp_boot_prot_mode
{
    HOGP_BOOT_PROTOCOL_MODE         = 0x00,    /**< Boot Protocol Mode. */
    HOGP_REPORT_PROTOCOL_MODE,                 /**< Report Protocol Mode. */
};

/**< HID Information structure. */
typedef struct
{
    uint16_t bcdHID;         /**< bcdHID. */
    uint8_t  bCountryCode;   /**< bCountryCode. */
    uint8_t  flags;          /**< Flags. */
} hids_hid_info_t;

#endif /* _BLE_HOGP_COMMON_H_ */
