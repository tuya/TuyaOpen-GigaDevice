/*!
    \file    systime.h
    \brief   Provide time related function specific for GD32VW55x SDK.

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

#ifndef _SYSTIME_H_
#define _SYSTIME_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

/**
 * Time origin
 */
enum time_origin_t {
    /** Since boot time */
    SINCE_BOOT,
    /** Since Epoch : 1970-01-01 00:00:00 +0000 (UTC) */
    SINCE_EPOCH,
};

extern uint32_t clock_us_factor;;

/**
 ****************************************************************************************
 * @brief Initialize time.
 *
 * @param[in] sec  Number of seconds since Epoch when the system started.
 * @param[in] usec Number of microseconds since Epoch when the system started
 *                 (excluding the seconds in sec).
 ****************************************************************************************
 */
void time_init(uint32_t sec, uint32_t usec);

/**
 ****************************************************************************************
 * @brief Get current time.
 *
 * return the current time, from the selected origin, in a sec/usec split.
 *
 * @param[in]  origin Select the time origin (Since boot or since Epoch)
 * @param[out] sec    Udapted with the number of seconds since the selected origin.
 * @param[out] usec   Updated with the number of microseconds since the selected origin.
 *                   (excluding the seconds in sec)
 *
 * @return 0 on success and != 0 otherwise.
 ****************************************************************************************
 */
int get_time(enum time_origin_t origin, uint32_t *sec, uint32_t *usec);

 /*!
    \brief      get_local_time
    \param[in]  none
    \param[out] none
    \retval     64 bit timestamp(ms)
*/
/**
 ****************************************************************************************
 * @brief Get current time.
 *
 * return the current time, from the selected origin, in msec.
 *
 * @param[in]  origin Select the time origin (Since boot or since Epoch)
 * @param[out] usec   Updated with the number of milliseconds since the selected origin.
 *
 * @return 0 on success and != 0 otherwise.
 ****************************************************************************************
 */
uint64_t get_sys_local_time_ms();

/**
 ****************************************************************************************
 * @brief Get current time.
 *
 * return the current time, from the selected origin, in usec.
 *
 * @param[in]  origin Select the time origin (Since boot or since Epoch)
 * @param[out] usec   Updated with the number of microseconds since the selected origin.
 *
 * @return 0 on success and != 0 otherwise.
 ****************************************************************************************
 */
int get_time_us(enum time_origin_t origin, uint64_t *usec);

/**
 ****************************************************************************************
 * @brief Get current time.
 *
 * return the current time,  in usec.
 *
 * @return time in usec
 ****************************************************************************************
 */
uint64_t get_sys_local_time_us(void);

/**
 ****************************************************************************************
 * @brief initialize systick parameter
 ****************************************************************************************
 */
void systick_init(void);

/**
 ****************************************************************************************
 * @brief delay a time in microseconds
 *
 * @param[in]  nus: count in microseconds
 ****************************************************************************************
 */
void systick_udelay(uint32_t nus);

#ifdef __cplusplus
 }
#endif

#endif /* _SYSTIME_H_ */
