/*!
    \file    init_rom.c
    \brief   Rom init function for GD32VW55x SDK.

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

#include "wrapper_os.h"
#include "init_rom.h"

#include "mbedtls/platform_time.h"
#include "mbedtls/platform.h"
#include "trng.h"

#include "mbedtls/version.h"

#define MBEDTLS_VER_2_17_0       0x02110000

#if (MBEDTLS_VERSION_NUMBER == MBEDTLS_VER_2_17_0) //mbedtls v2.17.0
#include "mbedtls/entropy_poll.h"
#endif

#include "init_rom_symbol.c"

struct rom_api_t *p_rom_api = (struct rom_api_t *)ROM_API_ARRAY_BASE;

static mbedtls_time_t my_time_get(mbedtls_time_t *p)
{
    return (sys_os_now(false) * OS_MS_PER_TICK);
}

static void mebdtls_platform_init(void)
{
#if (MBEDTLS_VERSION_NUMBER != MBEDTLS_VER_2_17_0)
    /* Redirect mbedtls_calloc/mbedtls_free in rom to sys_calloc/sys_mfree */
    typedef int (*mbedtls_platform_set_calloc_free_fn)(void * (*calloc_func)( size_t, size_t),
                                  void (*free_func)(void *));
    #define rom_mbedtls_platform_set_calloc_free ((mbedtls_platform_set_calloc_free_fn)0x0bf5ea58)

    rom_mbedtls_platform_set_calloc_free((void *(*)(size_t,  size_t))sys_calloc, sys_mfree);
#endif

    /* Reconfig function pointers for MbedTLS */
    mbedtls_platform_set_calloc_free((void *(*)(size_t,  size_t))sys_calloc, sys_mfree);
    mbedtls_platform_set_snprintf(snprintf);
    mbedtls_platform_set_printf(printf);
    mbedtls_platform_set_time(my_time_get);
#if (MBEDTLS_VERSION_NUMBER == MBEDTLS_VER_2_17_0) //mbedtls v2.17.0
    mbedtls_platform_set_hardware_poll(gd_hardware_poll);

    /* Others */
    mbedtls_ecp_curve_val_init();
#endif
}

/*!
    \brief      initialize rom
    \param[in]  none
    \param[out] none
    \retval     none
*/
void rom_init(void)
{
    trng_close(1);
    rom_symbol_init();
    mebdtls_platform_init();
}
