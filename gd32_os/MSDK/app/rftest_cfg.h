/*!
    \file    rftest_cfg.h
    \brief   RF Test configuration for GD32VW55x SDK

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

#ifndef _RFTEST_CFG_H_
#define _RFTEST_CFG_H_

// #define CONFIG_RF_TEST_SUPPORT
#ifdef CONFIG_RF_TEST_SUPPORT
    #undef CONFIG_BASECMD
    #undef CONFIG_ATCMD
    #undef CONFIG_INTERNAL_DEBUG
#endif

// #define CONFIG_SIGNALING_TEST_SUPPORT
#ifdef CONFIG_SIGNALING_TEST_SUPPORT
    #ifndef CONFIG_BASECMD
        #error "CONFIG_BASECMD must be defined in signaling test!"
    #endif
    #undef CONFIG_ATCMD
    #undef CONFIG_INTERNAL_DEBUG
#endif

#if defined(CONFIG_RF_TEST_SUPPORT) && defined(CONFIG_SIGNALING_TEST_SUPPORT)
  #error "CONFIG_RF_TEST_SUPPORT and CONFIG_SIGNALING_TEST_SUPPORT cannot be defined simultaneously"
#endif

// #define CONFIG_BLE_DTM_SUPPORT
#ifdef CONFIG_BLE_DTM_SUPPORT
    #undef CONFIG_BASECMD
    #undef CONFIG_ATCMD
    #undef CONFIG_INTERNAL_DEBUG
    #undef CONFIG_RF_TEST_SUPPORT
    #undef CONFIG_SIGNALING_TEST_SUPPORT
#endif

#endif  /* _RFTEST_CFG_H_ */
