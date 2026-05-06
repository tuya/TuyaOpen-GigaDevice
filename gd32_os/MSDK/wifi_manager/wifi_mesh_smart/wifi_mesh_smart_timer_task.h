/*!
    \file    #include "wifi_mesh_smart_timer_task.h"
    \brief   Header file of generic timer task for wifi_mesh_smart.

    \version 2025-08-22, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2025, GigaDevice Semiconductor Inc.

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

#ifndef __TIMER_TASK_H__
#define __TIMER_TASK_H__

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "wrapper_os.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
    \brief Timer callback function type
    \param[in] timer_id: timer identifier
    \param[in] user_data: user data passed during timer registration
*/
typedef void (*timer_callback_t)(uint32_t timer_id, void *user_data);

/*!
    \brief Timer registration result codes
*/
typedef enum {
    TIMER_TASK_OK = 0,              /*!< operation successful */
    TIMER_TASK_ERR_INVALID_PARAM,   /*!< invalid parameter */
    TIMER_TASK_ERR_NO_MEMORY,       /*!< no memory available */
    TIMER_TASK_ERR_NOT_FOUND,       /*!< timer not found */
    TIMER_TASK_ERR_ALREADY_EXISTS,  /*!< timer already exists */
    TIMER_TASK_ERR_TASK_NOT_RUNNING /*!< timer task not running */
} timer_task_result_t;

/*!
    \brief Timer modes
*/
typedef enum {
    TIMER_MODE_ONE_SHOT = 0,    /*!< timer fires once then automatically unregisters */
    TIMER_MODE_PERIODIC         /*!< timer fires periodically until manually unregistered */
} timer_mode_t;


/*!
    \brief Initialize the timer task
    \retval timer_task_result_t: operation result
*/
timer_task_result_t timer_task_init();

/*!
    \brief Deinitialize the timer task
    \retval timer_task_result_t: operation result
*/
timer_task_result_t timer_task_deinit(void);

/*!
    \brief Register a timer
    \param[in] timer_id: unique timer identifier
    \param[in] timeout_ms: timeout in milliseconds
    \param[in] mode: timer mode (one-shot or periodic)
    \param[in] callback: callback function to be called when timer expires
    \param[in] user_data: user data to be passed to callback
    \retval timer_task_result_t: operation result
*/
timer_task_result_t timer_task_register(uint32_t timer_id,
                                       uint32_t timeout_ms,
                                       timer_mode_t mode,
                                       timer_callback_t callback,
                                       void *user_data);

/*!
    \brief Unregister a timer
    \param[in] timer_id: timer identifier
    \retval timer_task_result_t: operation result
*/
timer_task_result_t timer_task_unregister(uint32_t timer_id);

/*!
    \brief Reset a timer (restart with original timeout)
    \param[in] timer_id: timer identifier
    \retval timer_task_result_t: operation result
*/
timer_task_result_t timer_task_reset(uint32_t timer_id);

/*!
    \brief Update timer timeout
    \param[in] timer_id: timer identifier
    \param[in] new_timeout_ms: new timeout in milliseconds
    \retval timer_task_result_t: operation result
*/
timer_task_result_t timer_task_update_timeout(uint32_t timer_id, uint32_t new_timeout_ms);

/*!
    \brief Check if timer is active
    \param[in] timer_id: timer identifier
    \retval bool: true if timer is active, false otherwise
*/
bool timer_task_is_active(uint32_t timer_id);

/*!
    \brief Get remaining time for a timer
    \param[in] timer_id: timer identifier
    \param[out] remaining_ms: pointer to store remaining time in milliseconds
    \retval timer_task_result_t: operation result
*/
timer_task_result_t timer_task_get_remaining_time(uint32_t timer_id, uint32_t *remaining_ms);

/*!
    \brief Get timer task statistics
    \param[out] active_timers: number of currently active timers
    \retval timer_task_result_t: operation result
*/
timer_task_result_t timer_task_get_stats(uint32_t *active_timers);

#ifdef __cplusplus
}
#endif

#endif /* __TIMER_TASK_H__ */
