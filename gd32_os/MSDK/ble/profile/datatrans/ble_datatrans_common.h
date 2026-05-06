/*!
    \file    ble_datatrans_common.h
    \brief   Header file of datatrans common information.

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

#ifndef _BLE_DATATRANS_COMMON_H_
#define _BLE_DATATRANS_COMMON_H_

/* BLE datatrans related service/characteristic UUID value */
typedef enum
{
    BLE_GATT_SVC_DATATRANS_SERVICE = BLE_GATT_UUID_16_LSB(0x0101),  /* BLE datatrans service UUID */
    BLE_GATT_SVC_DATATRANS_RX_CHAR = BLE_GATT_UUID_16_LSB(0x0102),  /* BLE datatrans RX characteristic UUID */
    BLE_GATT_SVC_DATATRANS_TX_CHAR = BLE_GATT_UUID_16_LSB(0x0103),  /* BLE datatrans TX characteristic UUID */
} ble_datatrans_uuid_t;
#endif /* _BLE_DATATRANS_COMMON_H_ */
