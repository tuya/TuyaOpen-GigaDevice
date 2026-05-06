/*!
    \file    ble_cscss.h
    \brief   Header file for Cycling Speed and Cadence Service Server.

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

#ifndef _BLE_CSCSS_H_
#define _BLE_CSCSS_H_

#include "ble_cscs.h"
#include "ble_gap.h"

/* CSCSS init parameter structure */
typedef struct
{
    uint16_t                csc_feature;        /*!< CSC Feature Value, @ref ble_cscs_feat_bf_t */
    uint8_t                 sensor_loc;         /*!< Sensor location */
    ble_cscs_sensor_loc_t  *p_loc_supp_list;    /*!< List of supported sensor locations */
    uint8_t                 loc_supp_num;       /*!< Number of supported sensor locations in the list */
    ble_gap_sec_lvl_t       sec_lvl;            /*!< Security level required to access service */
} ble_cscss_init_param_t;

/* Prototype of CSCSS callback functions */
typedef struct
{
    void (*cumul_value_set_cb)(uint8_t conn_idx, uint32_t tx_pwr);
    void (*location_update_cb)(uint8_t conn_idx, uint8_t location);
} ble_cscss_callbacks_t;

/*!
    \brief      Init Cycling Speed and Cadence Service Server
    \param[in]  p_param: pointer to CSCSS init parameters
    \param[in]  callback: callback functions
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_cscss_init(ble_cscss_init_param_t *p_param, ble_cscss_callbacks_t callback);

/*!
    \brief      Deinit Cycling Speed and Cadence Service Server
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_cscss_deinit(void);

/*!
    \brief      Send CSC Measurement notification
    \param[in]  p_meas: pointer to CSC Measurement value to send
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_cscss_meas_send(ble_cscs_csc_meas_t *p_meas);

#endif // (_BLE_CSCSS_H_)
