/*!
    \file    build_config.h
    \brief   build config for wpas

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

#ifndef _BUILD_CONFIG_H_
#define _BUILD_CONFIG_H_

#include "wlan_config.h"

// #define CONFIG_WPA_DEBUG
// #define CONFIG_WPA_DATA_DUMP

#define CONFIG_WPA3_SAE
#define CONFIG_OWE
#define CONFIG_WPA3_PMK_CACHE_ENABLE

#if (CONFIG_PLATFORM != PLATFORM_FPGA_32103_V7)
#define WPA_HW_SECURITY_ENGINE_ENABLE
#define HW_ACC_ENGINE_LOCK() GLOBAL_INT_DISABLE()
#define HW_ACC_ENGINE_UNLOCK() GLOBAL_INT_RESTORE()
#endif

#ifdef CFG_WPS
    #define CONFIG_WPS
    #undef IEEE8021X_EAPOL
    #define IEEE8021X_EAPOL
    #undef CONFIG_EAP_WSC
    #define CONFIG_EAP_WSC
#endif

#ifdef CFG_8021x_EAP_TLS
    #define CONFIG_EAP_TLS
    #undef IEEE8021X_EAPOL
    #define CONFIG_TLS_INTERNAL_CLIENT
    #define CONFIG_INTERNAL_LIBTOMMATH
    #define IEEE8021X_EAPOL
    #define CONFIG_SUITEB
    #define CONFIG_SUITEB192
    #define CONFIG_SHA256
    #define CONFIG_SHA384
    #define CONFIG_INTERNAL_SHA384
    #define CONFIG_SHA512
    #define CONFIG_INTERNAL_SHA512
    #define CONFIG_TLSV12
    #define CONFIG_GDWIFI
    // #define CONFIG_EAP_USE_MD5
#endif

// 802.11r fast transition
#define CONFIG_IEEE80211R

#endif /* _BUILD_CONFIG_H_ */
