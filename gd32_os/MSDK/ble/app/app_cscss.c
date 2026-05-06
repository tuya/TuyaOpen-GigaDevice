/*!
    \file    app_cscss.c
    \brief   Cycling Speed and Cadence Service Server Application Module entry point.

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

#if BLE_PROFILE_CSCS_SERVER
#include "app_cscss.h"
#include "dbg_print.h"

/*!
    \brief      Callback function to handle cumulative value set event
    \param[in]  conn_idx: connection index
    \param[in]  tx_pwr: transmit power
    \param[out] none
    \retval     none
*/
static void app_cumul_value_set_cb(uint8_t conn_idx, uint32_t tx_pwr)
{
    app_print("app_cumul_value_set_cb, conn idx 0x%x, tx pwr %d\r\n", conn_idx, tx_pwr);
}

/*!
    \brief      Callback function to handle location update event
    \param[in]  conn_idx: connection index
    \param[in]  location: location value to update
    \param[out] none
    \retval     none
*/
static void app_location_update_cb(uint8_t conn_idx, uint8_t location)
{
    app_print("app_location_update_cb, conn idx 0x%x, location %d\r\n", conn_idx, location);
}

/* Callback functions to cscss events */
static ble_cscss_callbacks_t cscss_cb = {
    app_cumul_value_set_cb,
    app_location_update_cb,
};

/*!
    \brief      Init APP cycling speed and cadence service server module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_app_cscss_init(void)
{
    ble_cscss_init_param_t param;
    ble_cscs_sensor_loc_t loc_supp[3] = {BLE_CSCS_SENSOR_LOC_FRONT_WHEEL,
                                         BLE_CSCS_SENSOR_LOC_LEFT_CRANK,
                                         BLE_CSCS_SENSOR_LOC_RIGHT_CRANK};

    param.csc_feature = BLE_CSCS_FEAT_WHEEL_REV_DATA_BIT |
                        BLE_CSCS_FEAT_CRANK_REV_DATA_BIT |
                        BLE_CSCS_FEAT_MULT_SENSOR_LOC_BIT;
    param.loc_supp_num = 3;
    param.p_loc_supp_list = loc_supp;
    param.sensor_loc = BLE_CSCS_SENSOR_LOC_FRONT_WHEEL;
    param.sec_lvl = BLE_GAP_SEC_UNAUTH;

    ble_cscss_init(&param, cscss_cb);
}

/*!
    \brief      Deinit APP cycling speed and cadence service server module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_app_cscss_deinit(void)
{
    ble_cscss_deinit();
}
#endif
