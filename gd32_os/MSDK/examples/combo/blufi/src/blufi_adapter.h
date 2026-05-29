/*!
    \file    blufi_adapter.h
    \brief   Header file for Implemetions of blufi adapter.

    \version 2025-02-05, V1.0.0, firmware for GD32VW55x
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

#ifndef _BLUFI_ADAPTER_H_
#define _BLUFI_ADAPTER_H_

#include <stdint.h>
#include "ble_adv.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEV_NAME                                "BLUFI_DEVICE"
#define DEV_NAME_LEN                            strlen(dev_name)

#define BLUFI_VALUE_LEN                         512

#define BLUFI_GATT_SERVICE_UUID                 BLE_GATT_UUID_16_LSB(0xFFFF)
#define BLUFI_GATT_WRITE_UUID                   BLE_GATT_UUID_16_LSB(0xFF01)
#define BLUFI_GATT_NTF_UUID                     BLE_GATT_UUID_16_LSB(0xFF02)

/* Blue courier wifi attribute index */
enum blufi_att_idx
{
    // Service blue courier wifi
    BLUFI_IDX_PRIM_SVC,
    BLUFI_IDX_CHAR_WRITE,
    BLUFI_IDX_WRITE,
    BLUFI_IDX_CHAR_NTF,
    BLUFI_IDX_NTF,
    BLUFI_IDX_NTF_CFG,

    BLUFI_IDX_NUMBER,
};

/* Blue courier wifi link environment struct */
typedef struct
{
    uint8_t         conn_id;            /*!< Connection id. use to ble_srv_ntf_ind_send */
    uint8_t         adv_idx;            /*!< Advertising id. use to stop advertising */
    uint16_t        ntf_cfg;            /*!< NTF CCCD value */
    ble_adv_state_t adv_state;          /*!< Advertising state */
} blufi_adapter_env_t;

extern blufi_adapter_env_t blufi_adapter_env;

void blufi_ntf_event_send(uint8_t *p_val, uint16_t len);
ble_status_t blufi_adapter_init(void);

extern void btc_blufi_recv_handler(uint8_t *data, int len);
extern uint8_t btc_blufi_profile_init(void);
#ifdef __cplusplus
}
#endif

#endif // _BLUFI_ADAPTER_H_