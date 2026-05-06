/*!
    \file    ble_app_config.h
    \brief   BLE application config file.

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

#ifndef _BLE_APP_CONFIG_H_
#define _BLE_APP_CONFIG_H_

#include "platform_def.h"
#include "ble_config.h"


#if defined(CFG_BLE_SUPPORT) && defined(BLE_HOST_SUPPORT)
#define BLE_APP_SUPPORT                 1
#else
#define BLE_APP_SUPPORT                 0
#endif

#if BLE_APP_SUPPORT
#define BLE_APP_CMD_SUPPORT             1
#else
#define BLE_APP_CMD_SUPPORT             0
#endif

#if BLE_APP_SUPPORT && defined(BLE_PER_ADV_SUPPORT)
#define BLE_APP_PER_ADV_SUPPORT         1
#else
#define BLE_APP_PER_ADV_SUPPORT         0
#endif

#if BLE_APP_SUPPORT && defined(BLE_PAST_SUPPORT)
#define BLE_APP_PAST_SUPPORT            1
#else
#define BLE_APP_PAST_SUPPORT            0
#endif

#if BLE_APP_SUPPORT && defined(BLE_DATA_LEN_EXTEN_SUPPORT)
#define BLE_APP_DATA_LEN_EXTEN_SUPPORT  1
#else
#define BLE_APP_DATA_LEN_EXTEN_SUPPORT  0
#endif

#if BLE_APP_SUPPORT && defined(BLE_PHY_UPDATE_SUPPORT)
#define BLE_APP_PHY_UPDATE_SUPPORT      1
#else
#define BLE_APP_PHY_UPDATE_SUPPORT      0
#endif

#if BLE_APP_SUPPORT && defined(BLE_PWR_CTRL_SUPPORT)
#define BLE_APP_PWR_CTRL_SUPPORT        1
#else
#define BLE_APP_PWR_CTRL_SUPPORT        0
#endif

#if BLE_APP_SUPPORT && defined(BLE_PING_SUPPORT)
#define BLE_APP_PING_SUPPORT            1
#else
#define BLE_APP_PING_SUPPORT            0
#endif

#if BLE_APP_SUPPORT && defined(BLE_GATT_CLIENT_SUPPORT)
#define BLE_APP_GATT_CLIENT_SUPPORT     1
#else
#define BLE_APP_GATT_CLIENT_SUPPORT     0
#endif

#if BLE_APP_SUPPORT && defined(BLE_EATT_SUPPORT)
#define BLE_APP_EATT_SUPPORT            1
#else
#define BLE_APP_EATT_SUPPORT            0
#endif

#if BLE_APP_SUPPORT && defined(BLE_BIS_SUPPORT)
#define BLE_APP_BIS_SUPPORT             1
#else
#define BLE_APP_BIS_SUPPORT             0
#endif

#if BLE_APP_SUPPORT && defined(BLE_CIS_SUPPORT)
#define BLE_APP_CIS_SUPPORT             1
#else
#define BLE_APP_CIS_SUPPORT             0
#endif

// profile related config
#if defined(CFG_BLE_SUPPORT) && defined(BLE_HOST_SUPPORT)
#define BLE_PROFILE_DIS_SERVER              1
#define BLE_PROFILE_BLUE_COURIER_SERVER     1
#define BLE_PROFILE_SAMPLE_SERVER           1
#define BLE_PROFILE_THROUGHPUT_SERVER       0
#define BLE_PROFILE_BAS_SERVER              0
#else
#define BLE_PROFILE_DIS_SERVER              0
#define BLE_PROFILE_BLUE_COURIER_SERVER     0
#define BLE_PROFILE_SAMPLE_SERVER           0
#define BLE_PROFILE_THROUGHPUT_SERVER       0
#define BLE_PROFILE_BAS_SERVER              0
#endif

#if defined(CFG_BLE_SUPPORT) && defined (BLE_HOST_SUPPORT) && defined (BLE_GATT_CLIENT_SUPPORT)
#define BLE_PROFILE_SAMPLE_CLIENT           0
#define BLE_PROFILE_THROUGHPUT_CLIENT       0
#else
#define BLE_PROFILE_SAMPLE_CLIENT           0
#define BLE_PROFILE_THROUGHPUT_CLIENT       0
#endif

#endif  //_BLE_APP_CONFIG_H_
