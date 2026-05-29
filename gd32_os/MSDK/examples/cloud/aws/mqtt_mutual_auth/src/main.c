/*!
    \file    main.c
    \brief   Main loop of GD32VW55x SDK.

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

#include <stdint.h>
#include "wifi_export.h"
#include "gd32vw55x_platform.h"
#include "uart.h"
#include "gd32vw55x.h"
#include "wrapper_os.h"
#include "cmd_shell.h"
#include "util.h"
#include "wlan_config.h"
#include "wifi_init.h"
#include "user_setting.h"
#include "version.h"

/*!
    \brief      Init applications.
                This function is called to initialize all the applications.
    \param[in]  none.
    \param[out] none.
    \retval     none.
*/
static void application_init(void)
{
    if (cmd_shell_init()) {
        dbg_print(ERR, "cmd shell init failed\r\n");
    }

    util_init();

    user_setting_init();

    if (wifi_init()) {
        dbg_print(ERR, "wifi init failed\r\n");
    }
}

/*!
    \brief      Main entry point.
                This function is called right after the booting process has completed.
    \param[in]  none
    \param[out] none
    \retval     none.
*/
int main(void)
{
    sys_os_init();
    platform_init();

    application_init();

    sys_os_start();
}

