/*!
    \file    wrapper_threadx.h
    \brief   Header file for Threadx wrapper.

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

#ifndef __WRAPPER_THREADX_H
#define __WRAPPER_THREADX_H

/*============================ INCLUDES ======================================*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "dlist.h"
#include "tx_api.h"
#include "tx_initialize.h"
#include "tx_thread.h"
#include "tx_semaphore.h"
#include "tx_queue.h"
#include "threadxConfig.h"
#include "threadx_port.h"
#include "tx_timer.h"
#include "co_list.h"
#include "wrapper_os.h"

#define ADD_BYTE_POOL_NAME_LEN          20

/*============================ TYPES =========================================*/
typedef struct task_wrapper {
    struct co_list_hdr hdr;
    TX_THREAD    tx_thread;
    void         *p_stack;
    os_queue_t   task_queue;
    task_func_t  task_func;
    void        *func_argv;

    uint32_t task_notify_val;
    uint32_t task_notify_val_pend;
    uint32_t *p_notify_val_ret;
    TX_SEMAPHORE notification_sem;
    uint8_t notification_pending;
    uint8_t clear_on_pend;
    uint32_t clear_mask;
    char      name[configMAX_TASK_NAME_LEN];
} task_wrapper_t;

typedef enum
{
    eWrapNoAction = 0,
    eWrapSetBits,
    eWrapIncrement,
    eWrapSetValueWithOverwrite,
    eWrapSetValueWithoutOverwrite,
} eWrapperNotifyAction;

#define TXFR_NOTIFYACTION_VALID(x) (((int)x >= (int)eWrapNoAction) && ((int)x <= (int)eWrapSetValueWithoutOverwrite))

/*============================ GLOBAL VARIABLES ==============================*/
#ifndef EXTERN
#define EXTERN extern
#endif

// EXTERN uint32_t os_idle_task_stk[configMINIMAL_STACK_SIZE] __ALIGNED(8);
// EXTERN uint32_t os_timer_task_stk[configTIMER_TASK_STACK_DEPTH] __ALIGNED(8);

/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/

// Queue set related structures and type definitions.
typedef struct tx_queueset {
    TX_QUEUE queue;
} tx_queueset_t;

// Queue related structures and type definitions.
typedef struct wapper_tx_queue {
    ULONG id;
    uint8_t allocated;          // 1: Use wrapper queue, 0: use tx api queue
    tx_queueset_t *p_set;
    uint8_t *p_mem;
    TX_SEMAPHORE read_sem;
    TX_SEMAPHORE write_sem;
    uint8_t *p_write;
    uint8_t *p_read;
    int32_t queue_length;
    uint32_t msg_size;
} wapper_tx_queue_t;

typedef struct add_byte_pool
{
    dlist_t               list;
    TX_BYTE_POOL          byte_pool;
    uint32_t              high_heap_mark;
    uint32_t              cur_heap_mark;
    uint8_t               name[ADD_BYTE_POOL_NAME_LEN];
} add_byte_pool_t;

void sys_task_change_timeslice(void *task, uint32_t timeslice);

void create_threadx_idle_task(void);

#endif //#ifndef __WRAPPER_THREADX_H