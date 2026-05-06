/*
 * Event loop
 * Copyright (c) 2002-2006, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 *
 * This file defines an event loop interface that supports processing events
 * from registered timeouts (i.e., do something after N seconds), sockets
 * (e.g., a new packet available for reading), and signals. eloop.c is an
 * implementation of this interface using select() and sockets. This is
 * suitable for most UNIX/POSIX systems. When porting to other operating
 * systems, it may be necessary to replace that implementation with OS specific
 * mechanisms.
 */

/*!
    \file    wifi_eloop.h
    \brief   Header file for wifi eloop.

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

#ifndef _WIFI_ELOOP_H_
#define _WIFI_ELOOP_H_

#include "stdint.h"
/*============================ INCLUDES ======================================*/
/*============================ MACROS ========================================*/
/**
 * ELOOP_ALL_CTX - eloop_timeout_cancel() magic number to match all timeouts
 */
#define ELOOP_ALL_CTX                ((void *) -1)

/* event with value 0 will be ignored by the event loop */
#define ELOOP_EVENT_WAKEUP             1
#define ELOOP_EVENT_TERMINATE          2
#define ELOOP_EVENT_ALL                3
#define ELOOP_EVENT_MAX                4

#define ELOOP_EVENT_ID(vif_idx, event) ((vif_idx << 12) + event)
#define ELOOP_EVENT_GET_VIF(event_id)  ((event_id >> 12) & 0xF)
#define ELOOP_EVENT_GET_EV(event_id)   (event_id & 0xFFF)

/*============================ MACRO FUNCTIONS ===============================*/
/* #define ELOOP_INSTALL_STATIC_EVENT(event, callback, eloop_data, user_ctx) \
                                 {eloop_data, user_ctx, callback, event}
*/

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr)     (sizeof(arr)/sizeof(arr[0]))
#endif

/*============================ TYPES =========================================*/
/* BIT[15:12]: vif index, BIT[11:0]: event index */
typedef uint16_t eloop_event_id_t;

/*
 * A compound type that has the same size as uint32_t, we use it to carry more
 * information along with the event type while still deliver the message the
 * same way as a normal event type by copying it to the event/message queue.
*/
typedef struct eloop_message {
    uint16_t event_id;
    uint16_t reason;
    uint8_t *param;
    uint32_t param_len;
} eloop_message_t;

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/
/**
 * eloop_event_handler - eloop generic event callback type
 * @eloop_ctx: Registered callback context data (eloop_data)
 * @sock_ctx: Registered callback context data (user_data)
 */
typedef void (*eloop_event_handler)(void *eloop_data, void *user_ctx);

/**
 * eloop_timeout_handler - eloop timeout event callback type
 * @eloop_ctx: Registered callback context data (eloop_data)
 * @sock_ctx: Registered callback context data (user_data)
 */
typedef void (*eloop_timeout_handler)(void *eloop_data, void *user_ctx);

/**
 * eloop_init() - Initialize global event loop data
 * Returns: 0 on success
 *
 * This function must be called before any other eloop_* function.
 */
int wifi_eloop_init(void);

/**
 * eloop_event_register - Register handler for generic events
 * @event: Event to wait (eloop implementation specific)
 * @handler: Callback function to be called when event is triggered
 * @eloop_data: Callback context data (eloop_data)
 * @user_data: Callback context data (user_data)
 * Returns: 0 on success, -1 on failure
 *
 * Register an event handler for the given event. This function is used to
 * register eloop implementation specific events which are mainly targeted for
 * operating system specific code (driver interface and l2_packet) since the
 * portable code will not be able to use such an OS-specific call. The handler
 * function will be called whenever the event is triggered. The handler
 * function is responsible for clearing the event after having processed it in
 * order to avoid eloop from calling the handler again for the same event.
 */
int eloop_event_register(eloop_event_id_t event_id,
             eloop_event_handler handler,
             void *eloop_data, void *user_data);

/**
 * eloop_event_unregister - Unregister handler for a generic event
 * @event_id: Event ID to cancel (eloop implementation specific)
 * @handler: Callback function to be called when event is triggered
 *
 * Unregister a generic event notifier that was previously registered with
 * eloop_event_register().
 */
void eloop_event_unregister(eloop_event_id_t event_id,
                eloop_event_handler handler);

/**
 * eloop_event_send - Send an event to the event loop
 * @vif_idx: index of wifi vif
 * @event: Event to signal
 * Returns: 0 on success, -1 on failure or event queue full
 *
 * Send an event to the event queue of event loop task which will process
 * events in FIFO order as soon as possible.
 */
int eloop_event_send(uint8_t vif_idx, uint16_t event);

/**
 * eloop_message_send - Send a message to the event loop
 * @vif_idx: index of wifi vif
 * @event: Event to signal
 * @reason: reason to signal
 * @param: parameter pointer
 * @len: the length of the input parameter
 * Returns: 0 on success, -1 on failure or event queue full
 *
 * Send a message to the event queue of event loop task which will process
 * them in FIFO order as soon as possible.
 */
int eloop_message_send(uint8_t vif_idx, uint16_t event, int reason, uint8_t *param, uint32_t len);

/**
 * eloop_timeout_register - Register timeout
 * @secs: Number of seconds to the timeout
 * @usecs: Number of microseconds to the timeout
 * @handler: Callback function to be called when timeout occurs
 * @eloop_data: Callback context data (eloop_ctx)
 * @user_data: Callback context data (sock_ctx)
 * Returns: 0 on success, -1 on failure
 *
 * Register a timeout that will cause the handler function to be called after
 * given time.
 */
int eloop_timeout_register(unsigned int msecs,
               eloop_timeout_handler handler,
               void *eloop_data, void *user_data);

/**
 * eloop_timeout_cancel - Cancel timeouts
 * @handler: Matching callback function
 * @eloop_data: Matching eloop_data or %ELOOP_ALL_CTX to match all
 * @user_data: Matching user_data or %ELOOP_ALL_CTX to match all
 * Returns: Number of cancelled timeouts
 *
 * Cancel matching <handler,eloop_data,user_data> timeouts registered with
 * eloop_timeout_register(). ELOOP_ALL_CTX can be used as a wildcard for
 * cancelling all timeouts regardless of eloop_data/user_data.
 */
int eloop_timeout_cancel(eloop_timeout_handler handler,
             void *eloop_data, void *user_data);

int eloop_timeout_all_cancel(void);

/**
 * eloop_is_timeout_registered - Check if a timeout is already registered
 * @handler: Matching callback function
 * @eloop_data: Matching eloop_data
 * @user_data: Matching user_data
 * Returns: 1 if the timeout is registered, 0 if the timeout is not registered
 *
 * Determine if a matching <handler,eloop_data,user_data> timeout is registered
 * with eloop_timeout_register().
 */
int eloop_timeout_is_registered(eloop_timeout_handler handler,
                void *eloop_data, void *user_data);

/**
 * eloop_run - Start the event loop
 *
 * Start the event loop and continue running as long as there are any
 * registered event handlers. This function is run after event loop has been
 * initialized with event_init() and one or more events have been registered.
 */
void wifi_eloop_run(void);

/**
 * eloop_terminate - Terminate event loop
 *
 * Terminate event loop even if there are registered events. This can be used
 * to request the program to be terminated cleanly.
 */
void wifi_eloop_terminate(void);

/**
 * eloop_destroy - Free any resources allocated for the event loop
 *
 * After calling eloop_destroy(), other eloop_* functions must not be called
 * before re-running eloop_init().
 */
void wifi_eloop_destroy(void);

/**
 * eloop_terminated - Check whether event loop has been terminated
 * Returns: 1 = event loop terminate, 0 = event loop still running
 *
 * This function can be used to check whether eloop_terminate() has been called
 * to request termination of the event loop. This is normally used to abort
 * operations that may still be queued to be run when eloop_terminate() was
 * called.
 */
int wifi_eloop_terminated(void);

/*============================ IMPLEMENTATION ================================*/

#endif /* _WIFI_ELOOP_H_ */
