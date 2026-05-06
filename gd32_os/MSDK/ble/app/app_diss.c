/*!
    \file    app_diss.c
    \brief   Device Information Service Server Application Module entry point.

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

#if BLE_PROFILE_DIS_SERVER
#include "ble_diss.h"
#include "app_diss.h"

/*!
    \brief      Init APP device information service server module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_app_diss_init(void)
{
    ble_diss_init_param_t param;

    uint8_t manufat_name[] = "GigaDevice";
    uint8_t model_name[] = "GD32VW55x";
    uint8_t serial_num[] = "HDM1";
    uint8_t hw_rev[] = "HW ver 1.0";
    uint8_t fw_rev[] = "FW ver 1.0";
    uint8_t sw_rev[] = "SW ver 1.0";
    uint8_t ieee_data[] = {BLE_DIS_IEEE_11073_BODY_EXP, 0, 'e', 'x', 'p', 'e', 'r', 'i', 'm', 'e', 'n', 't', 'a', 'l'};
    ble_dis_sys_id_t sys_id;
    ble_dis_pnp_id_t pnp_id;

    sys_id.manufact_id = 0x000C2B0C2B;
    sys_id.oui = 0x00010203;

    pnp_id.vendor_id_source = BLE_DIS_VND_ID_SRC_BLUETOOTH_SIG;
    pnp_id.vendor_id = 0x0C2B;
    pnp_id.product_id = 0x01;
    pnp_id.product_version = 0x01;

    param.manufact_name.p_data = manufat_name;
    param.manufact_name.len = sizeof(manufat_name);
    param.model_num.p_data = model_name;
    param.model_num.len = sizeof(model_name);
    param.serial_num.p_data = serial_num;
    param.serial_num.len = sizeof(serial_num);
    param.hw_rev.p_data = hw_rev;
    param.hw_rev.len = sizeof(hw_rev);
    param.fw_rev.p_data = fw_rev;
    param.fw_rev.len = sizeof(fw_rev);
    param.sw_rev.p_data = sw_rev;
    param.sw_rev.len = sizeof(sw_rev);
    param.ieee_data.p_data = ieee_data;
    param.ieee_data.len = sizeof(ieee_data);
    param.p_sys_id = &sys_id;
    param.p_pnp_id = &pnp_id;

    param.sec_lvl = BLE_GAP_SEC_UNAUTH;

    ble_diss_init(&param);
}

/*!
    \brief      Deinit APP device information service server module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_app_diss_deinit(void)
{
    ble_diss_deinit();
}
#endif

