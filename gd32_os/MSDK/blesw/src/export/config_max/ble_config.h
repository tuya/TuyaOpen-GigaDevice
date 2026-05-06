/*!
    \file    ble_config.h
    \brief   Ble MAX config definitions.

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

#ifndef _BLE_CONFIG_H_
#define _BLE_CONFIG_H_

#define BLE_CFG_ROLE_BROADCASTER    0x01
#define BLE_CFG_ROLE_PERIPHERAL     0x02
#define BLE_CFG_ROLE_OBSERVER       0x04
#define BLE_CFG_ROLE_CENTRAL        0x08
#define BLE_CFG_ROLE_ALL            (BLE_CFG_ROLE_BROADCASTER | BLE_CFG_ROLE_PERIPHERAL  |         \
                                     BLE_CFG_ROLE_OBSERVER    | BLE_CFG_ROLE_CENTRAL)

#define BLE_CFG_ROLE                BLE_CFG_ROLE_ALL

#define BLE_MAX_CONN_NUM            4

#define BLE_HOST_SUPPORT

#define BLE_PER_ADV_SUPPORT

#ifdef BLE_PER_ADV_SUPPORT
#define BLE_PAST_SUPPORT
#endif

#define BLE_DATA_LEN_EXTEN_SUPPORT

#define BLE_PHY_UPDATE_SUPPORT

#define BLE_PWR_CTRL_SUPPORT

#define BLE_PING_SUPPORT

#define BLE_SCA_SUPPORT

#define BLE_GATT_CLIENT_SUPPORT

#define BLE_EATT_SUPPORT

#define BLE_GATT_CACHING_SUPPORT

#define BLE_SMP_SC_SUPPORT

#ifdef BLE_PER_ADV_SUPPORT
#define BLE_BIS_SUPPORT
#endif

#define BLE_CIS_SUPPORT
#endif  //_BLE_CONFIG_H_
