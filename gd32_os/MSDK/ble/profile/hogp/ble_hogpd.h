/*!
    \file    ble_hogpd.h
    \brief   Header file of hogp service server.

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
#ifndef _BLE_HOGPD_H_
#define _BLE_HOGPD_H_

#include <stdint.h>
#include "ble_types.h"
#include "ble_hogp_common.h"

#define HOGPD_INSTANCES_MAX                    (0x01)       /* Maximum number of HID Over GATT Device task instances */

#define HOGPD_REPORT_MAX_LEN                   (45)         /* Maximal length of Report Char. Value */

#define HOGPD_REPORT_MAP_MAX_LEN               (512)        /* Maximal length of Report Map Char. Value */

#define HOGPD_BOOT_REPORT_MAX_LEN              (8)          /* Length of Boot Report Char. Value Maximal Length */

#define HOGPD_BOOT_KB_IN_NTF_CFG_MASK          (0x40)       /* Boot KB Input Report Notification Configuration Bit Mask */

#define HOGPD_BOOT_MOUSE_IN_NTF_CFG_MASK       (0x80)       /* Boot KB Input Report Notification Configuration Bit Mask */

#define HOGPD_REPORT_NTF_CFG_MASK              (0x20)       /* Boot Report Notification Configuration Bit Mask */

#define KB_REPORT_LENGTH                       (8)          /* Keyboard report length */

#define MM_KB_REPORT_LENGTH                    (3)          /* MM keyboard report length */

#define KB_REPORT_IDX                          (1)          /* Keyboard report index */

/* Database Creation Service Instance Configuration structure */
typedef struct
{
    hids_hid_info_t hid_info;              /*!< HID Information Char. Values */
    uint8_t         proto_mode;            /*!< proto_mode */
} ble_hogpd_param_t;

/*!
    \brief      Init BLE hogp server
    \param[in]  none
    \param[out] none
    \retval     none
*/
ble_status_t ble_hogpd_init(ble_hogpd_param_t *p_param);

/*!
    \brief      Hogpd send kb value
    \param[in]  conn_idx: connection index
    \param[out] none
    \retval     none
*/
void ble_hogpd_send_kb_value(uint8_t conn_idx, uint8_t * p_value);

#endif // _BLE_HOGPD_H_