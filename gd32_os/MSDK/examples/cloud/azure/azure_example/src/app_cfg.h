/*!
    \file    app_cfg.h
    \brief   configuration for the example of azure

    \version 2024-03-22, V1.0.0, firmware for GD32VW55x
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

#ifndef _APP_CFG_H_
#define _APP_CFG_H_

#include "platform_def.h"

#define CONFIG_DEBUG_PRINT_ENABLE

#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
#ifndef CONFIG_AZURE_IOT_SUPPORT
#error "CONFIG_AZURE_IOT_SUPPORT must be defined"
#endif

#if (CONFIG_BOARD != PLATFORM_BOARD_32VW55X_F527)
#error "PLATFORM_BOARD_32VW55X_F527 must be selected"
#endif

#undef CONFIG_ATCMD
#define CONFIG_ATCMD
#endif /* CONFIG_AZURE_F527_DEMO_SUPPORT */

#ifdef CONFIG_AZURE_IOT_SUPPORT
#define CONFIG_NEED_RSA_4096
#endif

#ifdef CFG_BLE_SUPPORT
#undef CFG_BLE_SUPPORT
#endif

#ifdef CFG_COEX
#undef CFG_COEX
#endif

#endif  /* _APP_CFG_H_ */
