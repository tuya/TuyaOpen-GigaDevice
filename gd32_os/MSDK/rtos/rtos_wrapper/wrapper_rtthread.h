/*!
    \file    wrapper_rtthread.h
    \brief   Header file for RT-Thread wrapper.

    \version 2024-08-19, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2024, GigaDevice Semiconductor Inc.

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

#ifndef __WRAPPER_RTTHREAD_H
#define __WRAPPER_RTTHREAD_H

/*============================ INCLUDES ======================================*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <rtconfig.h>
#include <rthw.h>
#include <rtthread.h>
#include <dbg_print.h>
#include "trng.h"
#include "systime.h"
#include "co_utils.h"
#include "dlist.h"

/*============================ MACROS ========================================*/
#define ADD_HEAP_NAME_LEN               (20)

/*============================ MACRO FUNCTIONS ========================================*/
/* Declare a variable to hold current interrupt status to restore it later */
#define SYS_SR_ALLOC()          register rt_base_t __srt_temp
/* Disable interrupts (nestted) */
#define SYS_CRITICAL_ENTER()        do {__srt_temp = rt_hw_interrupt_disable();}while(0)
/* Enable interrupts (nested) */
#define SYS_CRITICAL_EXIT()         do {rt_hw_interrupt_enable(__sr_temp);}while(0)

#define TASK_PRIO_HIGHER(n)             (-n)
#define TASK_PRIO_LOWER(n)              (n)
#define configMAX_TASK_NAME_LEN         (16)

typedef struct timer_wrapper {
    struct rt_timer os_timer;
    void *p_arg;
    timer_func_t timer_func;
} timer_wrapper_t;

typedef struct task_wrapper {
    rt_thread_t task_handle;
    rt_mq_t task_queue;
    rt_sem_t notification_sem;
    uint32_t id;            /*unique identifier for a task */
} task_wrapper_t;

typedef struct task_status
{
    rt_thread_t *thread_ptr;
    char *name;                         /* Pointer to thread's name */
    rt_uint8_t state;                   /* Thread's execution state */
    rt_uint32_t priority;               /* Priority of thread (0-1023) */
    rt_uint32_t stack_end;              /* Stack ending address */
    rt_uint32_t stack_min_free_size;    /* Stack minimum free size in words */
    rt_uint32_t id;                     /* thread->entry is used to identify a thread */
} task_status_t;

typedef struct add_heap_wrapper
{
    dlist_t               list;
    struct rt_memheap   _heap_added;
    uint8_t               name[ADD_HEAP_NAME_LEN];
} add_heap_wrapper_t;

/*============================ GLOBAL VARIABLES ==============================*/
#ifndef EXTERN
#define EXTERN extern
#endif

/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/

#endif //#ifndef __WRAPPER_RTTHREAD_H