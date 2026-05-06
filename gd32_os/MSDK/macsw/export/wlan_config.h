/*!
    \file    wifi_config.h
    \brief   Configs for WIFI.

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

#ifndef _WLAN_CONFIG_H_
#define _WLAN_CONFIG_H_

#include "platform_def.h"

// Feature
#define CFG_SOFTAP
#define CFG_HE
#define CFG_VIF_NUM                     1  //4
#define CFG_STA_NUM                     2

// RX
#define CFG_AMSDU_4K
#define CFG_RXBUF1_MPDU                 4  // RX_BUF1_MAX_MPDU_CNT
#ifndef CFG_AMSDU_4K
    #define CFG_AMSDU_12K
    #define CFG_RXBUF1_MPDU             12
#endif
#define CFG_BARX                        1
#define CFG_REORD_BUF                   12

// TX
#define CFG_TCPTX                       1
#define CFG_AGG
#define CFG_BATX                        5
#define CFG_TXDESC0                     4
#define CFG_TXDESC1                     12 //64
#define CFG_TXDESC2                     4  //64
#define CFG_TXDESC3                     4
#define CFG_TXDESC4                     1
#define CFG_MU_CNT                      1
#define CFG_SPC                         16

// PS
#define CFG_LPS
#define CFG_UAPSD
#define CFG_TWT                         8

// FTM
// #define CFG_FTM_INIT
// #define CFG_FTM_RSP

// Dbg
#define CFG_MAC_DBG
// #define CFG_PROF
// #define CFG_TRACE
#define CFG_REC
#define CFG_STATS
#define CFG_WIFI_RX_STATS

// Extra
#define CFG_KEYCFG
#define CFG_MFP
// #define CFG_SAE_PK
// #define CFG_DMA
// #define CFG_MM_MSG_ALL_IND
#if (CONFIG_PLATFORM == PLATFORM_ASIC_32103)
#define CFG_DM_SUPPORT
#define CFG_EFUSE
#endif

// Wi-Fi Protected Setup
// #define CFG_WPS

// Wi-Fi Enterprise with EAP-TLS Setup
// #define CFG_8021x_EAP_TLS

// 802.11R Fast BSS Transition
#define CFG_80211R

// check setting
#if (CONFIG_PLATFORM != PLATFORM_ASIC_32103)
    #ifdef CFG_DM_SUPPORT
        #error "DM support only in ASIC paltform"
    #endif
    #ifdef CFG_EFUSE
        #error "EFUSE support only in ASIC paltform"
    #endif
#endif

#if defined(CFG_UAPSD) && !defined(CFG_LPS)
    #error "the legacy PS mode shall be enabled when UAPSD is enabled"
#endif

#if defined(CFG_TWT) && !defined(CFG_LPS)
    #error "the legacy PS mode shall be enabled when TWT is enabled"
#endif // CFG_TWT

#if !defined(CFG_FTM_INIT) && defined(CFG_FTM_RSP)
    #error "FTM rsp shall not be enabled when intiator is not supported"
#endif

#ifdef CFG_HE
    #ifndef CFG_AGG
        #error "AGG shall be enabled when HE is supported"
    #endif
    #ifndef CFG_MFP
        #error "MFP shall be enabled when HE is supported"
    #endif
#else
    #ifdef CFG_TWT
        #error "TWT shall not be enabled when HE is not supported"
    #endif
    //#undef CFG_TWT
#endif // CFG_HE

// #define CFG_WIFI_HIGH_PERFORMANCE
#ifdef CFG_WIFI_HIGH_PERFORMANCE
    #undef CFG_REORD_BUF
    #define CFG_REORD_BUF                 20
    #undef CFG_TXDESC1
    #define CFG_TXDESC1                   20
    #undef CFG_AMSDU_4K
    #define CFG_AMSDU_8K
    #undef CFG_RXBUF1_MPDU
    #define CFG_RXBUF1_MPDU               8
#endif /* CFG_WIFI_HIGH_PERFORMANCE */

// #define CFG_WFA_HE
#ifdef CFG_WFA_HE
    #undef CFG_BLE_SUPPORT
    #undef CFG_COEX
    #undef CFG_BARX
    #define CFG_BARX                      2
    #undef CFG_TXDESC0
    #define CFG_TXDESC0                   12
    #undef CFG_TXDESC1
    #define CFG_TXDESC1                   12
    #undef CFG_TXDESC2
    #define CFG_TXDESC2                   12
    #undef CFG_TXDESC3
    #define CFG_TXDESC3                   12
    #undef CFG_REORD_BUF
    #define CFG_REORD_BUF                 20
    #undef CFG_AMSDU_4K
    #define CFG_AMSDU_8K
    #undef CFG_RXBUF1_MPDU
    #define CFG_RXBUF1_MPDU               8
#endif /* CFG_WFA_HE */

#define CFG_WIFI_CONCURRENT
#ifdef CFG_WIFI_CONCURRENT
    #undef CFG_VIF_NUM
    #define CFG_VIF_NUM                   2
    #undef CFG_STA_NUM
    #define CFG_STA_NUM                   3
    #undef CFG_SOFTAP
    #define CFG_SOFTAP
#endif /* CFG_WIFI_CONCURRENT */

// #define CFG_WIFI_MESH_SMART
#ifdef CFG_WIFI_MESH_SMART
    #undef CFG_SOFTAP
    #define CFG_SOFTAP
    #undef CFG_STA_NUM
    #define CFG_STA_NUM                   4
    #undef CFG_WIFI_CONCURRENT
    #define CFG_WIFI_CONCURRENT
#endif

// #define CFG_SOFTAP_MANY_CLIENTS
#ifdef CFG_SOFTAP_MANY_CLIENTS
    #undef CFG_STA_NUM
    #define CFG_STA_NUM                   16
#endif /* CFG_SOFTAP_MANY_CLIENTS */

// #define CFG_MULTI_STREAMS
#ifdef CFG_MULTI_STREAMS
    /* for two TCP RX streams */
    #undef CFG_BARX
    #define CFG_BARX                        2
    /* for two TCP TX streams */
    #undef CFG_TCPTX
    #define CFG_TCPTX                       2
#endif

// #define CFG_MIN_SRAM
#ifdef CFG_MIN_SRAM
    #undef CFG_TXDESC1
    #define CFG_TXDESC1                     4
    #undef CFG_REORD_BUF
    #define CFG_REORD_BUF                   5
    /* disabling AGG and HE can release another 12k sram */
    //#undef CFG_AGG  #undef CFG_HE
#endif

#if defined(CFG_WIFI_HIGH_PERFORMANCE) && defined(CFG_MIN_SRAM)
    #error "CFG_WIFI_HIGH_PERFORMANCE & CFG_MIN_SRAM cannot be defined simutaneously"
#endif
#if defined(CFG_WFA_HE) && defined(CFG_WIFI_HIGH_PERFORMANCE)
    #error "CFG_WFA_HE & CFG_WIFI_HIGH_PERFORMANCE cannot be defined simutaneously"
#endif
#if defined(CFG_WFA_HE) && defined(CFG_MIN_SRAM)
    #error "CFG_WFA_HE & CFG_MIN_SRAM cannot be defined simutaneously"
#endif

#endif /* _WLAN_CONFIG_H_ */
