/*
 * Event loop based on select() loop
 * Copyright (c) 2002-2009, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

/*!
    \file    wifi_eloop.c
    \brief   Wifi eloop for GD32VW55x SDK.

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

/*============================ INCLUDES ======================================*/
#include "wrapper_os.h"
#include "dlist.h"
#include "wifi_eloop.h"
#include "wifi_management.h"

/*============================ MACROS ========================================*/

/*============================ MACRO FUNCTIONS ===============================*/
/*============================ TYPES =========================================*/
struct eloop_event {
    void *eloop_data;
    void *user_data;
    eloop_event_handler handler;
    eloop_event_id_t event_id;
};

struct eloop_timeout {
    struct list_head list;
    uint32_t time;
    void *eloop_data;
    void *user_data;
    eloop_timeout_handler handler;
};

struct eloop_data {
    size_t event_count;
    struct eloop_event *events;
    struct list_head timeout;
    int terminate;
};

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
static struct eloop_data eloop;
static struct eloop_event eloop_predefined_events[] = {
    {NULL, NULL, wifi_mgmt_cb_run_state_machine, ELOOP_EVENT_ALL},
};

/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/
/*!
    \brief      initialize global event loop data(This function must be called before any other eloop_* function.)
    \param[in]  none
    \param[out] none
    \retval     0 on success
*/
int wifi_eloop_init(void)
{
    sys_memset(&eloop, 0, sizeof(eloop));

    INIT_DLIST_HEAD(&eloop.timeout);

    return 0;
}

/*!
    \brief      register handler for generic events
    \param[in]  event: event to wait (eloop implementation specific)
    \param[in]  handler: callback function to be called when event is triggered
    \param[in]  eloop_data: callback context data (eloop_data)
    \param[in]  user_data: callback context data (user_data)
    \param[out] none
    \retval     0 on success, -1 on failure
*/
int eloop_event_register(eloop_event_id_t event_id,
             eloop_event_handler handler,
             void *eloop_data, void *user_data)
{
    struct eloop_event *tmp;

    if (eloop.terminate)
        return -1;

    tmp = sys_realloc(eloop.events,
             (eloop.event_count + 1) * sizeof(struct eloop_event));
    if (tmp == NULL)
        return -1;

    tmp[eloop.event_count].eloop_data = eloop_data;
    tmp[eloop.event_count].user_data = user_data;
    tmp[eloop.event_count].handler = handler;
    tmp[eloop.event_count].event_id = event_id;
    eloop.event_count++;
    eloop.events = tmp;

    return 0;
}

/*!
    \brief      unregister handler for a generic event, Unregister a generic event notifier that was previously registered with eloop_event_register().
    \param[in]  event_id: event ID to cancel (eloop implementation specific)
    \param[in]  handler: callback function to be called when event is triggered
    \param[out] none
    \retval     none
*/
void eloop_event_unregister(eloop_event_id_t event_id,
                eloop_event_handler handler)
{
    size_t i;

    if (eloop.terminate)
        return;

    if (eloop.events == NULL || eloop.event_count == 0)
        return;

    for (i = 0; i < eloop.event_count; i++) {
        if ((eloop.events[i].event_id == event_id) &&
            (eloop.events[i].handler == handler))
            break;
    }
    if (i == eloop.event_count)
        return;

    if (i != eloop.event_count - 1) {
        sys_memmove(&eloop.events[i], &eloop.events[i + 1],
               (eloop.event_count - i - 1) *
               sizeof(struct eloop_event));
    }
    eloop.event_count--;
}

/*!
    \brief      send an event to the event loop
    \param[in]  event: event to signal
    \param[out] none
    \retval     0 on success, -1 on failure or event queue full
*/
int eloop_event_send(uint8_t vif_idx, uint16_t event)
{
    eloop_message_t message = {0};
    if (eloop.terminate)
        return -1;

    message.event_id = ELOOP_EVENT_ID(vif_idx, event);
    message.param = NULL;
    message.param_len = 0;
    return sys_task_post(wifi_mgmt_task_tcb, &message, 0);
}

/*!
    \brief      send a message to the event loop
    \param[in]  message: message to signal
    \param[out] none
    \retval     0 on success, -1 on failure or event queue full
*/
int eloop_message_send(uint8_t vif_idx, uint16_t event, int reason, uint8_t *param, uint32_t len)
{
    eloop_message_t message = {0};
    if (eloop.terminate)
        return -1;

    message.event_id = ELOOP_EVENT_ID(vif_idx, event);
    message.reason = reason;
    message.param = param;
    message.param_len = len;
    return sys_task_post(wifi_mgmt_task_tcb, &message, 0);
}

/*!
    \brief      dispatch event
    \param[in]  message: message to signal
    \param[out] none
    \retval     none
*/
static void eloop_event_dispatch(eloop_message_t message)
{
    int i, j;
    eloop_event_id_t event_id = message.event_id;
    uint16_t event = ELOOP_EVENT_GET_EV(event_id);

    if (event == ELOOP_EVENT_WAKEUP)
        return;

    if (event == ELOOP_EVENT_TERMINATE) {
        eloop.terminate = 1;
        return;
    }

    for (j = 0; j < ARRAY_SIZE(eloop_predefined_events); j++) {
        if (eloop_predefined_events[j].event_id == ELOOP_EVENT_ALL ||
                eloop_predefined_events[j].event_id == event_id) {
            eloop_predefined_events[j].handler(
                        eloop_predefined_events[j].eloop_data,
                        &message);
        }
    }

    for (i = 0; i < eloop.event_count; i++) {
        if (eloop.events[i].event_id == event_id) {
            eloop.events[i].handler(
                        eloop.events[i].eloop_data,
                        eloop.events[i].user_data);
        }
    }
}

/*!
    \brief      register timeout
    \param[in]  msecs: number of milliseconds to the timeout
    \param[in]  handler: callback function to be called when timeout occurs
    \param[in]  eloop_data: callback context data (eloop_ctx)
    \param[in]  user_data: callback context data (sock_ctx)
    \param[out] none
    \retval     0 on success, -1 on failure
*/
int eloop_timeout_register(unsigned int msecs,
               eloop_timeout_handler handler,
               void *eloop_data, void *user_data)
{
    struct eloop_timeout *timeout, *tmp;
    SYS_SR_ALLOC();

    if (eloop.terminate)
        return -1;

    timeout = sys_zalloc(sizeof(*timeout));
    if (timeout == NULL)
        return -1;

    timeout->time = sys_current_time_get();
    timeout->time += msecs;
    timeout->eloop_data = eloop_data;
    timeout->user_data = user_data;
    timeout->handler = handler;

    sys_enter_critical();
    /* Maintain timeouts in order of increasing time */
    list_for_each_entry(tmp, &eloop.timeout, struct eloop_timeout, list) {
        if (timeout->time < tmp->time) {
            list_add(&timeout->list, tmp->list.prev);
            goto exit;
        }
    }
    list_add_tail(&timeout->list, &eloop.timeout);

exit:
    sys_exit_critical();

    return 0;
}

/*!
    \brief      remove eloop timeout
    \param[in]  timeout: pointer to the eloop_timeout struction need to remove
    \param[out] none
    \retval     none
*/
static void eloop_timeout_remove(struct eloop_timeout *timeout)
{
    list_del(&timeout->list);
    sys_mfree(timeout);
}

/*!
    \brief      cancel timeouts
    \param[in]  handler: matching callback function
    \param[in]  eloop_data: matching eloop_data or %ELOOP_ALL_CTX to match all
    \param[in]  user_data: matching user_data or %ELOOP_ALL_CTX to match all
    \param[out] none
    \retval     number of cancelled timeouts(0x00000000-0xffffffff)
*/
int eloop_timeout_cancel(eloop_timeout_handler handler,
             void *eloop_data, void *user_data)
{
    struct eloop_timeout *timeout, *prev;
    int removed = 0;
    SYS_SR_ALLOC();

    if (eloop.terminate)
        return -1;

    sys_enter_critical();
    list_for_each_entry_safe(timeout, prev, &eloop.timeout,
                struct eloop_timeout, list) {
        if (timeout->handler == handler &&
                (timeout->eloop_data == eloop_data ||
                    eloop_data == ELOOP_ALL_CTX) &&
                (timeout->user_data == user_data ||
                    user_data == ELOOP_ALL_CTX)) {
            eloop_timeout_remove(timeout);
            removed++;
        }
    }
    sys_exit_critical();

    return removed;
}

//FOR test
int eloop_timeout_all_cancel(void)
{
    struct eloop_timeout *timeout, *prev;
    SYS_SR_ALLOC();
    sys_enter_critical();
    list_for_each_entry_safe(timeout, prev, &eloop.timeout,
                struct eloop_timeout, list) {
        printf("===============>remove timeout: "
               "eloop_data=%p user_data=%p handler=%p\r\n",
               timeout->eloop_data, timeout->user_data, timeout->handler);
        eloop_timeout_remove(timeout);
    }
    sys_exit_critical();
    return 0;
}

/*!
    \brief      check if a timeout is already registered
    \param[in]  handler: matching callback function
    \param[in]  eloop_data: matching eloop_data
    \param[in]  user_data: matching user_data
    \param[out] none
    \retval     1 if the timeout is registered, 0 if the timeout is not registered(0x00000000-0xffffffff)
*/
int eloop_timeout_is_registered(eloop_timeout_handler handler,
                void *eloop_data, void *user_data)
{
    struct eloop_timeout *tmp;
    SYS_SR_ALLOC();

    sys_enter_critical();
    list_for_each_entry(tmp, &eloop.timeout, struct eloop_timeout, list) {
        if (tmp->handler == handler &&
                tmp->eloop_data == eloop_data &&
                tmp->user_data == user_data) {
            sys_exit_critical();
            return 1;
        }
    }
    sys_exit_critical();

    return 0;
}

/*!
    \brief      handle eloop timeout
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void eloop_timeout_handle()
{
    struct eloop_timeout *timeout;
    uint32_t now;
    SYS_SR_ALLOC();

    sys_enter_critical();
    timeout = list_first_entry(&eloop.timeout, struct eloop_timeout, list);
    if (timeout) {
        now = sys_current_time_get();
        if (SYS_TIME_AFTER_EQ(now, timeout->time)) {
            void *eloop_data = timeout->eloop_data;
            void *user_data = timeout->user_data;
            eloop_timeout_handler handler =
                timeout->handler;
            eloop_timeout_remove(timeout);
            sys_exit_critical();
            handler(eloop_data, user_data);
            return;
        }
    }
    sys_exit_critical();
}

/*!
    \brief      start the event loop
    \param[in]  none
    \param[out] none
    \retval     none
*/
void wifi_eloop_run(void)
{
    uint32_t now;
    uint32_t remain;
    eloop_message_t message;
    struct eloop_timeout *timeout;
    SYS_SR_ALLOC();

    while (!eloop.terminate) {
        remain = 0;

        sys_enter_critical();
        timeout = list_first_entry(&eloop.timeout, struct eloop_timeout, list);
        if (timeout) {
            now = sys_current_time_get();
            if (SYS_TIME_BEFORE(now, timeout->time)) {
                remain = timeout->time - now;
            }
        }
        sys_exit_critical();

        if (timeout && remain == 0) {
            sys_yield();
        } else {
            if (sys_task_wait(timeout ? remain : 0, &message) == OS_OK) {
                eloop_event_dispatch(message);
            }
        }

        eloop_timeout_handle();
    }
}

/*!
    \brief      terminate event loop
    \param[in]  none
    \param[out] none
    \retval     none
*/
void wifi_eloop_terminate(void)
{
    eloop_event_send(0, ELOOP_EVENT_TERMINATE);
}

/*!
    \brief      free any resources allocated for the event loop
    \param[in]  none
    \param[out] none
    \retval     none
*/
void wifi_eloop_destroy(void)
{
    struct eloop_timeout *timeout, *prev;
    eloop_message_t message;
    uint32_t now __MAYBE_UNUSED;

    SYS_SR_ALLOC();

    /* terminated by eloop_terminate, all events have been rejected since then */
    //sys_task_msg_flush(&wifi_mgmt_task_tcb);
    while (sys_task_msg_num(wifi_mgmt_task_tcb, 0)) {
        sys_task_wait(1, &message);
        if (message.param) {
            sys_mfree(message.param);
        }
    }

    now = sys_current_time_get();
    sys_enter_critical();
    list_for_each_entry_safe(timeout, prev, &eloop.timeout,
                struct eloop_timeout, list) {
        wifi_sm_printf(WIFI_SM_INFO, "ELOOP: remaining timeout: %u "
               "eloop_data=%p user_data=%p handler=%p",
               timeout->time - now, timeout->eloop_data, timeout->user_data,
               timeout->handler);
        eloop_timeout_remove(timeout);
    }
    sys_exit_critical();

    if (eloop.events)
        sys_mfree(eloop.events);
}

/*!
    \brief      check whether event loop has been terminated
    \param[in]  none
    \param[out] none
    \retval     1: event loop terminate, 0: event loop still running
*/
int wifi_eloop_terminated(void)
{
    return eloop.terminate;
}
