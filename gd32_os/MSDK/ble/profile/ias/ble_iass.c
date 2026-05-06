/*!
    \file    ble_iass.c
    \brief   Implementation of immediate alert service server .

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

#include "ble_app_config.h"

#if (BLE_PROFILE_PROX_SERVER)
#include <string.h>

#include "ble_utils.h"
#include "ble_gatt.h"
#include "ble_iass.h"
#include "dbg_print.h"

/* IAS Database Description */
const ble_gatt_attr_desc_t ble_ias_attr_db[BLE_IAS_HDL_NB] = {
    [BLE_IAS_HDL_SVC]                 = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_PRIMARY_SERVICE), PROP(RD),            0                  },

    [BLE_IAS_HDL_ALERT_LVL_CHAR]      = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD),            0                  },
    [BLE_IAS_HDL_ALERT_LVL_VAL]       = {UUID_16BIT_TO_ARRAY(BLE_GATT_CHAR_ALERT_LEVEL),     PROP(WC),            OPT(NO_OFFSET) | sizeof(uint8_t)},
};

static const uint8_t ble_ias_uuid[2] = UUID_16BIT_TO_ARRAY(BLE_GATT_SVC_IMMEDIATE_ALERT);

/*!
    \brief      Init Immediate Alert Service Server
    \param[in]  p_rw_cb: GATT server message callback fucntion
    \param[out] none
    \retval     uint8_t: Service ID
*/
uint8_t ble_iass_init(p_fun_srv_cb p_rw_cb)
{
    ble_status_t ret = BLE_ERR_NO_ERROR;
    uint8_t srv_id = 0xFF;

    ret = ble_gatts_svc_add(&srv_id, ble_ias_uuid, 0, SVC_UUID(16), ble_ias_attr_db,
                            BLE_IAS_HDL_NB, p_rw_cb);

    if (ret != BLE_ERR_NO_ERROR) {
        srv_id = 0xFF;
    }

    return srv_id;
}

#endif //BLE_PROFILE_PROX_SERVER
