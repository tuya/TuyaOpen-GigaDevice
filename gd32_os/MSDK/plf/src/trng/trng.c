/*!
    \file    trng.c
    \brief   trng for GD32VW55x SDK

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

#include <stdlib.h>
#include "gd32vw55x_rcu.h"
#include "gd32vw55x_trng.h"
#include "dbg_print.h"
#include "wrapper_os.h"
#include "trng.h"

static uint32_t trng_initialized = 0;
/*!
    \brief       check whether TRNG is ready
    \param[in]   none
    \param[out]  none
    \retval      1: TRNG is ready 0: TRNG is not ready
*/
static int trng_ready_check(void)
{
    uint32_t timeout = 0;
    uint32_t trng_flag = STATUS_RESET;
    int reval = 1;

    /* check wherther the random data is valid */
    do {
        timeout++;
        trng_flag = trng_flag_get(TRNG_FLAG_DRDY);
    } while((STATUS_RESET == trng_flag) && (0xFFFF > timeout));

    if (STATUS_RESET == trng_flag) {
        /* ready check timeout */
        trng_flag = trng_flag_get(TRNG_FLAG_CECS);
        dbg_print(WARNING, "TRNG clock error(%d).\r\n", trng_flag);
        trng_flag = trng_flag_get(TRNG_FLAG_SECS);
        dbg_print(WARNING, "TRNG seed error(%d).\r\n", trng_flag);
        reval = 0;
    }

    /* return check status */
    return reval;
}

/*!
    \brief      configure TRNG module
    \param[in]  none
    \param[out] none
    \retval     ErrStatus: SUCCESS or ERROR
*/
static int trng_configuration(void)
{
    int reval = 0;
    uint32_t seed;

    if (!trng_initialized) {
        /* TRNG module clock enable */
        rcu_periph_clock_enable(RCU_TRNG);

        /* TRNG registers reset */
        trng_deinit();
        trng_enable();
        /* check TRNG work status */
        if (!trng_ready_check()) {
            reval = -1;
            return reval;
        }

        // use trng to generate seed, cost around 5400 cpu clk per 256 bytes
        seed = trng_get_true_random_data();
        srand(seed);
        trng_initialized = 1;
    }

    return reval;
}


/*!
    \brief      get random value
    \param[in]  len: length of random value want to get
    \param[out] output: pointer to the random value
    \retval     0 if succeed, non-zero otherwise
*/
int random_get(unsigned char *output, unsigned int len)
{
#if 0 // use trng only, cost around 6500 cpu clk per 256 bytes
    unsigned int n;
    unsigned char *p = output;
    unsigned int rand_data;
    int length = (int)len;

    if (trng_configuration() != 0) {
        return -1;
    }

    while (length > 0) {
        n = length;
        if (n > 4)
            n = 4;

        if (trng_ready_check()) {
            rand_data = trng_get_true_random_data();
        } else {
            return -1;
        }

        sys_memcpy(p, (uint8_t *)&rand_data, n);

        p += n;
        length -= n;
    }

    return 0;
#else
    uint8_t *p = output;
    uint32_t temp = 0;
    uint32_t n;
    int length = (int)len;

    if (trng_configuration() != 0) {
        return -1;
    }

    while (length > 0) {
        n = length;
        if (n > 4)
            n = 4;

        temp = rand();

        sys_memcpy(p, (uint8_t *)&temp, n);
        p += n;
        length -= n;
    }

    return 0;
#endif
}

/*!
    \brief      Entropy poll callback for gd hardware source
    \param[in]  data: pointer to the input data
    \param[in]  len: get random value length
    \param[out] output: pointer to the get random value
    \param[out] olen: pointer to the get random value length
    \retval     0: ok -1: not ok
*/
int gd_hardware_poll( void *data,
                           unsigned char *output, size_t len, size_t *olen )
{
    int ret;
    ((void) data);

    ret = random_get(output, len);
    if (ret < 0) {
        return -1;
    }
    if (olen)
        *olen = len;

    return( 0 );
}

/*!
    \brief      get TRNG value
    \param[in]  none
    \param[out] none
    \retval     getted TRNG value(0x00000000-0xffffffff)
*/
uint32_t trng_get(void)
{
    uint32_t rand;
    int ret;

    ret = trng_configuration();
    if (ret < 0) {
        return 0xFFFFFFFF;
    }
    rand = trng_get_true_random_data();

    return rand;
}

/*!
    \brief      close TRNG
    \param[in]  force, 1: force, 0: close if trng has been initialized
    \param[out] none
    \retval     none
*/
void trng_close(uint8_t force)
{
    if (trng_initialized || force) {
        trng_deinit();
        rcu_periph_clock_disable(RCU_TRNG);
        trng_initialized = 0;
    }
}
