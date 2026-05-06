/*!
    \file    app_conn_mgr.h
    \brief   Definitions of BLE application scan manager to record devices.

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

#ifndef APP_CONN_MGR_H_
#define APP_CONN_MGR_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "ble_gap.h"

/* Connection parameter update state */
typedef enum
{
    IDLE_STATE = 0,
    UPDATING_STATE ,
    FAST_PARAM_UPDATING_STATE,
    FAST_PARAM_UPDATED_STATE,
} ble_conn_param_upd_state_t;

/* Connection parameters structure */
typedef struct
{
    uint16_t    interval;       /*!< Connection interval */
    uint16_t    latency;        /*!< Connection latency */
    uint16_t    supv_tout;      /*!< Supervision timeout */
    uint16_t    ce_len_min;     /*!< Min CE length */
    uint16_t    ce_len_max;     /*!< Max CE length */
} ble_conn_params_t;

/* Phy parameters structure */
typedef struct
{
    uint8_t    tx_phy;
    uint8_t    rx_phy;
} ble_phy_params_t;

/*!
    \brief      Connection phy get
    \param[in]  tx_phy: pointer to tx_phy
    \param[in]  rx_phy: pointer to rx_phy
    \param[out] none
    \retval     bool: true if get phy value successfully started, otherwise false
*/
bool app_conn_phy_get(uint8_t conn_idx, uint8_t *tx_phy, uint8_t *rx_phy);

/*!
    \brief      Init APP connection manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_conn_mgr_init(void);

/*!
    \brief      Deinit APP connection manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_conn_mgr_deinit(void);

/*!
    \brief      Reset APP connection manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_conn_mgr_reset(void);

#endif // APP_CONN_MGR_H_
