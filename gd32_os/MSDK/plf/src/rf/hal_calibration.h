/*!
    \file    hal_calibration.h
    \brief   Header file for WLAN HAL RF calibration.

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

#ifndef __HAL_CALIBRATION_H
#define __HAL_CALIBRATION_H
/*============================ INCLUDES ======================================*/
#include "platform_def.h"
#include <stdint.h>
#include <stdbool.h>
/*============================ MACROS ========================================*/
#define RF_TX_DCK_DELAY                         10

/*============================ MACRO FUNCTIONS ===============================*/

/*============================ TYPES =========================================*/

/*============================ GLOBAL VARIABLES ==============================*/

/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/
int32_t hal_rf_tx_dck(uint32_t mode);
void hal_rf_rx_dck(void);
int32_t hal_rf_lnak(void);
int32_t hal_rf_rck(void);

#ifdef CFG_BLE_SUPPORT
void ble_rf_tx_iqk(uint32_t param1, uint32_t param2, int32_t iq_pass, int32_t lo_pass);
void ble_rf_rx_iqk(uint32_t param1, uint32_t param2, int32_t iq_pass);
uint32_t ble_rx_psd(uint32_t points, uint32_t option, uint32_t result_mem_addr);
void ble_loopback(uint8_t enable);
void ble_calibration_all(void);
#endif

#ifdef CFG_WLAN_SUPPORT
void wifi_rf_tx_iqk(uint32_t param1, uint32_t param2, int32_t iq_pass, int32_t lo_pass);
void wifi_rf_rx_iqk(uint32_t param1, uint32_t param2, int32_t iq_pass);
uint32_t wifi_rx_psd(uint32_t points, uint32_t option, uint32_t result_mem_addr);
void wifi_calibration_all(void);
#endif

bool is_rfk_once(void);
void rfk_enter(uint32_t is_wifi);
void rfk_exit(uint32_t is_wifi);
void ble_load_rfk(void);

/*============================ IMPLEMENTATION ================================*/

#endif /* __HAL_CALIBRATION_H */
