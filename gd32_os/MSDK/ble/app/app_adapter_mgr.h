/*!
    \file    app_adapter_mgr.h
    \brief   BLE application entry point

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

#ifndef _APP_ADAPTER_MGR_H_
#define _APP_ADAPTER_MGR_H_

#include <stdint.h>

#include "ble_gap.h"

/*!
    \brief      Init adapter application module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_adapter_init(void);

/*!
    \brief      Deinit adapter application module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_adapter_deinit(void);

/*!
    \brief      Get BLE adapter name
    \param[in]  none
    \param[out] p_name: pointer to the address of adapter name
    \retval     uint8_t: name length
*/
uint8_t app_adp_get_name(uint8_t **p_name);

/*!
    \brief      Set BLE adapter name
    \param[in]  p_name: pointer to device name which need to change
    \param[in]  len: name length
    \param[out] none
    \retval     bool: true if set device name successfully, otherwise false
*/
bool app_adp_set_name(char *p_name, uint16_t len);

/*!
    \brief      Set BLE channel map
    \param[in]  map: BLE channel map
    \param[out] none
    \retval     none
*/
void app_le_set_chann_map(uint8_t map[5]);

/*!
    \brief      Control local adapter to enter BLE Test TX Mode
    \param[in]  chann: TX channel, range from 0x00 to 0x27
    \param[in]  tx_data_len: length in bytes of payload data in each packet, range from 0x00 to 0xFF
    \param[in]  tx_pkt_pl: payload type, @ref ble_gap_packet_payload_type
    \param[in]  phy: test PHY rate, @ref ble_gap_phy_pwr_value
    \param[in]  tx_pwr_lvl: transmit power level in dBm range from -127 to +20, 0x7E for minimum and 0x7F for maximum
    \param[out] none
    \retval     none
*/
void app_le_tx_test(uint8_t chann, uint8_t tx_data_len, uint8_t tx_pkt_pl, uint8_t phy,
                    int8_t tx_pwr_lvl);

/*!
    \brief      Control local adapter to enter BLE Test RX Mode
    \param[in]  chann: RX channel, range from 0x00 to 0x27
    \param[in]  phy: test PHY rate, @ref ble_gap_phy_pwr_value
    \param[in]  modulation_idx: modulation Index
    \param[out] none
    \retval     none
*/
void app_le_rx_test(uint8_t chann, uint8_t phy, uint8_t modulation_idx);

/*!
    \brief      Control local adapter to exit BLE Test Mode
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_le_test_end(void);

/*!
    \brief      Reset adapter application module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_ble_reset(void);

/*!
    \brief      Enable adapter application module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_ble_enable(void);

/*!
    \brief      Disable adapter application module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_ble_disable(void);

#endif // _APP_ADAPTER_MGR_H_
