/*!
    \file    wrapper_threadx.c
    \brief   Threadx wrapper for GD32VW55x SDK

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

#include <dbg_print.h>
#include "wrapper_threadx.h"
#include "compiler.h"
#include "trng.h"
#include "systime.h"
#ifdef CFG_PLATFORM_FPGA_V7
#include "co_math.h"
#endif
#include "co_utils.h"
#include "boot.h"

#define configTOTAL_HEAP_SIZE     ((uint32_t)__heap_top - (uint32_t)__heap_bottom)
#define TX_IDLE_TASK_STACK_SZIE   1024

// Semaphore related structures and type definitions.
typedef struct wrapper_tx_sem {
    TX_SEMAPHORE sem;
    uint32_t max_count;
    uint8_t allocated;
} wrapper_tx_sem_t;

typedef struct wrapper_tx_mutex {
    TX_MUTEX mutex;
    uint8_t allocated;
} wrapper_tx_mutex_t;

#ifndef CFG_HEAP_MEM_CHECK
typedef struct wrapper_mem {
    uint32_t  size;
    uint8_t   memory[0];
} wrapper_mem_t;
#endif

typedef struct wrapper_tx_timer {
    TX_TIMER timer;
    uint32_t period;
    uint8_t one_shot;
    timer_func_t callback;
    void    *args;
} wrapper_tx_timer_t;

typedef struct task_status
{
    TX_THREAD *thread_ptr;
    CHAR  *tx_thread_name;                /* Pointer to thread's name     */
    UINT  tx_thread_state;                /* Thread's execution state     */
    UINT  tx_thread_priority;             /* Priority of thread (0-1023)  */
    VOID  *tx_thread_stack_end;           /* Stack ending address     */
    VOID  *tx_thread_stack_highest_ptr;   /* Stack highest usage pointer  */
    ULONG  tx_thread_time_slice;           /* Current time-slice       */
} task_status_t;

typedef struct idle_task
{
    TX_THREAD       idle_thread;
    void            *p_stack;
    struct co_list  rmv_task_list;
} idle_task_t;


#define TX_RUNNING_CHAR     ( 'X' )
#define TX_BLOCKED_CHAR     ( 'B' )
#define TX_READY_CHAR       ( 'R' )
#define TX_DELETED_CHAR     ( 'D' )
#define TX_SUSPENDED_CHAR   ( 'S' )


// Pointer to the start of the HEAP
uint8_t *ucHeap = (uint8_t *)__heap_bottom;

TX_BYTE_POOL byte_pool;
uint32_t high_heap_mark = 0;
uint32_t cur_heap_mark = 0;
dlist_t added_byte_pools;
static idle_task_t threadx_idle_task;

uint8_t sys_ps_mode = 0;

#ifdef TX_NOT_INTERRUPTABLE
#define CRITAL_QUEUE_SIZE         100
typedef struct critical_queue {
    TX_QUEUE queue;
    void    *p_mem;
} critical_queue_t;

static critical_queue_t critic_queue;
#endif

static UINT sys_timeout_2_tickcount(int timeout_ms)
{
    if (timeout_ms < 0) {
        return TX_WAIT_FOREVER;
    } else if (timeout_ms != 0) {
        return (timeout_ms * TX_TIMER_TICKS_PER_SECOND + 999) / 1000;
    }

    return TX_NO_WAIT;
}

static void tx_thread_func_wrapper(ULONG arg)
{
    task_wrapper_t *task_wrapper = (task_wrapper_t *)arg;

    if (task_wrapper != NULL) {
        task_wrapper->task_func(task_wrapper->func_argv);
    }
}

static int32_t xWrapperQueueSend(wapper_tx_queue_t *wrapper_queue, void *msg, int32_t timeout_ms)
{
    TX_INTERRUPT_SAVE_AREA;
    UINT timeout;
    UINT ret;

    configASSERT(wrapper_queue != NULL);
    configASSERT(msg != NULL);

    timeout = (UINT)sys_timeout_2_tickcount(timeout_ms);

    // Use thread Queue API
    if(wrapper_queue->p_set != NULL) {
        // To prevent deadlocks don't wait when posting on a queue set.
        ret = tx_queue_send(&wrapper_queue->p_set->queue, msg, timeout);
        if(ret != TX_SUCCESS) {
            // Fatal error, queue full errors are ignored on purpose to match the original behaviour.
            if(ret != TX_QUEUE_FULL) {
                configASSERT(0);
            }
            return OS_ERROR;
        }
        return OS_OK;
    }

    // Use wrapper semaphore queue
    // Wait for space to be available on the queue.
    ret = tx_semaphore_get(&wrapper_queue->write_sem, timeout);
    if(ret != TX_SUCCESS) {
        return OS_ERROR;
    }

    // Enqueue the message.
    TX_DISABLE;
    sys_memcpy(wrapper_queue->p_write, msg, wrapper_queue->msg_size);
    if(wrapper_queue->p_write >= (wrapper_queue->p_mem + (wrapper_queue->msg_size * (wrapper_queue->queue_length - 1u)))) {
        wrapper_queue->p_write = wrapper_queue->p_mem;
    } else {
        wrapper_queue->p_write += wrapper_queue->msg_size;
    }
    TX_RESTORE;

    // Signal that there is an additional message available on the queue.
    ret = tx_semaphore_put(&wrapper_queue->read_sem);
    if(ret != TX_SUCCESS) {
        configASSERT(0);
        return OS_ERROR;
    }

    return OS_OK;
}

static bool xWrapperQueueIsQueueFull(wapper_tx_queue_t *wrapper_queue)
{
    ULONG count = 0;
    UINT ret;

    configASSERT(wrapper_queue != NULL);

    // Use thread Queue API
    if(wrapper_queue->p_set != NULL) {
        ret = tx_queue_info_get(&wrapper_queue->p_set->queue, NULL, NULL, &count,
                                NULL, NULL, NULL);
        if (count == 0) {
            return true;
        }

        return false;
    }

    // Enqueue the message.
    ret = tx_semaphore_info_get(&wrapper_queue->write_sem, NULL, &count, NULL, NULL, NULL);
    if(ret != TX_SUCCESS) {
        configASSERT(0);
        return 0;
    }

    if(count == 0u) {
        return true;
    } else {
        return false;
    }
}

static int32_t xWrapperQueueReceive(wapper_tx_queue_t *wrapper_queue, void *pvBuffer, uint32_t timeout)
{
    TX_INTERRUPT_SAVE_AREA;
    UINT ret;

    configASSERT(wrapper_queue != NULL);
    configASSERT(pvBuffer != NULL);

    // Use thread Queue API
    if(wrapper_queue->p_set != NULL) {
        // To prevent deadlocks don't wait when posting on a queue set.
        ret = tx_queue_receive(&wrapper_queue->p_set->queue, pvBuffer, timeout);
        if(ret != TX_SUCCESS) {
            // Fatal error, queue full errors are ignored on purpose to match the original behaviour.
            configASSERT(0);
            return OS_TIMEOUT;
        }
        return OS_OK;
    }

    // Use wrapper semaphore queue
    // Wait for a message to be available on the queue.
    ret = tx_semaphore_get(&wrapper_queue->read_sem, timeout);
    if(ret != TX_SUCCESS) {
        return OS_TIMEOUT;
    }

    // Retrieve the message.
    TX_DISABLE;
    sys_memcpy(pvBuffer, wrapper_queue->p_read, wrapper_queue->msg_size);
    if(wrapper_queue->p_read >= (wrapper_queue->p_mem + (wrapper_queue->msg_size * (wrapper_queue->queue_length - 1u)))) {
        wrapper_queue->p_read = wrapper_queue->p_mem;
    } else {
        wrapper_queue->p_read += wrapper_queue->msg_size;
    }
    TX_RESTORE;

    // Signal that there's additional space available on the queue.
    ret = tx_semaphore_put(&wrapper_queue->write_sem);
    if(ret != TX_SUCCESS) {
        configASSERT(0);
        return OS_TIMEOUT;
    }

    return OS_OK;

}


static int32_t xWrapperQueueReset(wapper_tx_queue_t *wrapper_queue)
{
    TX_INTERRUPT_SAVE_AREA;
    UINT ret;
    UINT write_post;

    configASSERT(wrapper_queue != NULL);


    // Use thread Queue API
    if(wrapper_queue->p_set != NULL) {
        tx_queue_flush(&wrapper_queue->p_set->queue);
        return OS_OK;
    }

    // Use wrapper semaphore queue
    write_post = 0u;
    TX_DISABLE;
    _tx_thread_preempt_disable++;

    // Reset pointers.
    wrapper_queue->p_write = wrapper_queue->p_mem;
    wrapper_queue->p_read = wrapper_queue->p_mem;

    // Reset read semaphore.
    wrapper_queue->read_sem.tx_semaphore_count = 0u;

    // Reset write semaphore.
    if(wrapper_queue->write_sem.tx_semaphore_count != wrapper_queue->queue_length) {
        write_post = 1u;
        wrapper_queue->write_sem.tx_semaphore_count = wrapper_queue->queue_length - 1u;
    }

    _tx_thread_preempt_disable--;
    TX_RESTORE;

    if(write_post == 1u) {
        // Signal that there's space available on the queue in case a writer was waiting before the reset.
        ret = tx_semaphore_put(&wrapper_queue->write_sem);
        if(ret != TX_SUCCESS) {
            configASSERT(0);
            return OS_ERROR;
        }
    } else {
        _tx_thread_system_preempt_check();
    }

    return OS_OK;
}

static uint32_t ulWrapperTaskNotifyTake(bool xClearCountOnExit, ULONG xTicksToWait)
{
    TX_INTERRUPT_SAVE_AREA;

    task_wrapper_t *task_wrapper;
    TX_THREAD *p_thread;
    uint32_t val;
    UINT ret;
    UCHAR pend;

    pend = TX_FALSE;
    p_thread = tx_thread_identify();

    if (p_thread == NULL) {
        return 0;
    }

    task_wrapper = (task_wrapper_t *)(p_thread->tx_thread_entry_parameter);

    TX_DISABLE;

    ret = tx_semaphore_get(&task_wrapper->notification_sem, TX_NO_WAIT);

    if(ret == TX_SUCCESS) {
        val = task_wrapper->task_notify_val;
        task_wrapper->p_notify_val_ret = NULL;
        if(xClearCountOnExit != false) {
            task_wrapper->task_notify_val = 0u;
        } else {
            task_wrapper->task_notify_val--;
        }
    } else {
        pend = TX_TRUE;
        task_wrapper->p_notify_val_ret = &val;
        task_wrapper->clear_on_pend = xClearCountOnExit;
        task_wrapper->clear_mask = (uint32_t)-1;
    }

    TX_RESTORE;

    if(pend == TX_TRUE) {
        ret = tx_semaphore_get(&task_wrapper->notification_sem, xTicksToWait);
        task_wrapper->p_notify_val_ret = NULL;
        if(ret != TX_SUCCESS) {
            return 0u;
        }
    }

    return val;
}

static uint32_t xWrapperTaskNotifyAndQuery(task_wrapper_t * xTaskToNotify,
                                                      uint32_t ulValue,
                                                      eWrapperNotifyAction eAction,
                                                      uint32_t *pulPreviousNotifyValue)
{
    TX_INTERRUPT_SAVE_AREA;

    UINT ret;
    UCHAR notified;
    uint32_t ret_val;
    UCHAR waiting;

    configASSERT(xTaskToNotify != NULL);
    configASSERT(TXFR_NOTIFYACTION_VALID(eAction));

    TX_DISABLE;

    if(pulPreviousNotifyValue != NULL) {
        *pulPreviousNotifyValue = xTaskToNotify->task_notify_val;
    }

    waiting = TX_FALSE;
    notified = TX_FALSE;
    ret_val = 1;

    if(xTaskToNotify->notification_sem.tx_semaphore_suspended_count != 0u) {
        waiting = TX_TRUE;
    }

    if(xTaskToNotify->notification_sem.tx_semaphore_count == 0u) {
        _tx_thread_preempt_disable++;

        ret = tx_semaphore_put(&xTaskToNotify->notification_sem);

        _tx_thread_preempt_disable--;

        if(ret != TX_SUCCESS) {
            TX_RESTORE;
            configASSERT(0);
            return 0;
        }
        xTaskToNotify->task_notify_val_pend = xTaskToNotify->task_notify_val;

        notified = TX_TRUE;
    }

    switch (eAction) {
        case eWrapNoAction:
            break;

        case eWrapSetBits:
            xTaskToNotify->task_notify_val |= ulValue;
            break;

        case eWrapIncrement:
            xTaskToNotify->task_notify_val++;
            break;

        case eWrapSetValueWithOverwrite:
            xTaskToNotify->task_notify_val = ulValue;
            break;

        case eWrapSetValueWithoutOverwrite:
            if(notified == TX_TRUE) {
                xTaskToNotify->task_notify_val = ulValue;
            } else {
                ret_val = 0;
            }
            break;

        default:
            TX_RESTORE;
            return 0;
            break;
    }

    if(waiting == TX_TRUE) {
        *xTaskToNotify->p_notify_val_ret = xTaskToNotify->task_notify_val;

        if(xTaskToNotify->clear_on_pend == TX_TRUE) {
            xTaskToNotify->task_notify_val &= ~xTaskToNotify->clear_mask;
        } else {
            xTaskToNotify->task_notify_val--;
        }
    }

    TX_RESTORE;

    _tx_thread_system_preempt_check();

    return ret_val;
}

static void vWrapperTaskSuspendAll(void)
{
    TX_INTERRUPT_SAVE_AREA;

    TX_DISABLE;
    _tx_thread_preempt_disable++;
    TX_RESTORE;
}

static void xWrapperTaskResumeAll(void)
{
    TX_INTERRUPT_SAVE_AREA;

    TX_DISABLE;
    _tx_thread_preempt_disable--;
    TX_RESTORE;

    _tx_thread_system_preempt_check();
}

static void tx_timer_callback_wrapper(ULONG id)
{
    wrapper_tx_timer_t *p_timer;

    p_timer = (wrapper_tx_timer_t *)id;

    if(p_timer == NULL) {
        configASSERT(0);
    }

    p_timer->callback((void *)p_timer, p_timer->args);
}


#ifndef CFG_HEAP_MEM_CHECK
/***************** heap management implementation *****************/
/*!
    \brief      allocate a block of memory with a minimum of 'size' bytes.
    \param[in]  size: the minimum size of the requested block in bytes
    \param[out] none
    \retval     address to allocated memory, NULL pointer if there is an error
*/
void *sys_malloc(size_t size)
{
    wrapper_mem_t *pointer = NULL;
    dlist_t *pos, *n;
    add_byte_pool_t *p_byte_pool;


    if (tx_byte_allocate(&byte_pool, (VOID **) &pointer, size + sizeof(wrapper_mem_t), TX_NO_WAIT) == TX_SUCCESS) {
        goto done;
    }

    if (list_empty(&added_byte_pools)) {
        return NULL;
    }

    sys_enter_critical();
    list_for_each_safe(pos, n, &added_byte_pools) {
        p_byte_pool = list_entry(pos, add_byte_pool_t, list);

        if (tx_byte_allocate(&p_byte_pool->byte_pool, (VOID **) &pointer, size + sizeof(wrapper_mem_t), TX_NO_WAIT) == TX_SUCCESS) {
            break;
        }
    }
    sys_exit_critical();

done:
    sys_enter_critical();
    if (pointer != NULL) {
        pointer->size = size;
        cur_heap_mark += (size + sizeof(wrapper_mem_t) + (sizeof(UCHAR *)) + (sizeof(ALIGN_TYPE)));
        if (cur_heap_mark > high_heap_mark) {
            high_heap_mark = cur_heap_mark;
        }
    }
    sys_exit_critical();
    return pointer != NULL ? (void *)pointer->memory : NULL;
}

/*!
    \brief      allocate a certian chunks of memory with specified size
                Note: The allocated memory is filled with bytes of value zero.
                All chunks in the allocated memory are contiguous.
    \param[in]  count: multiple number of size want to malloc
    \param[in]  size:  number of size want to malloc
    \param[out] none
    \retval     address to allocated memory, NULL pointer if there is an error
*/
void *sys_calloc(size_t count, size_t size)
{
    void *mem_ptr;

    mem_ptr = sys_malloc(count * size);
    if (mem_ptr)
        sys_memset(mem_ptr, 0, (count * size));

    return mem_ptr;
}

/*!
    \brief      change the size of a previously allocated memory block.
    \param[in]  mem: address to the old buffer
    \param[in]  size: number of the new buffer size
    \param[out] none
    \retval     address to allocated memory, NULL pointer if there is an error
*/
void *sys_realloc(void *mem, size_t size)
{
    void *mem_ptr;
    uint32_t copy_size = 0;
    wrapper_mem_t *pointer = NULL;

    if (mem != NULL) {
        pointer = CONTAINER_OF((uint8_t *)mem, wrapper_mem_t, memory);
        copy_size = pointer->size > size ? size : pointer->size;
    }

    mem_ptr = sys_malloc(size);

    if (mem_ptr) {
        sys_memset(mem_ptr, 0, size);
        sys_memcpy(mem_ptr, mem, copy_size);
        sys_mfree(mem);
        return mem_ptr;
    }

    return NULL;
}

/*!
    \brief      free a memory to the heap
    \param[in]  ptr: pointer to the address want to free
    \param[out] none
    \retval     none
*/
void sys_mfree(void *ptr)
{
    wrapper_mem_t *pointer = NULL;

    if (ptr != NULL) {
        pointer = CONTAINER_OF((uint8_t *)ptr, wrapper_mem_t, memory);

        sys_enter_critical();
        cur_heap_mark -= (pointer->size + sizeof(wrapper_mem_t) + (sizeof(UCHAR *)) + (sizeof(ALIGN_TYPE)));
        sys_exit_critical();
    }

    tx_byte_release(pointer);
}
#endif

/*!
    \brief      get system free heap size
    \param[in]  none
    \param[out] none
    \retval     system free heap size value(0x00000000-0xffffffff)
*/
int32_t sys_free_heap_size(void)
{
    ULONG available_bytes = 0;
    ULONG available_bytes_temp = 0;
    dlist_t *pos, *n;
    add_byte_pool_t *p_byte_pool;

    tx_byte_pool_info_get(&byte_pool, NULL, &available_bytes, NULL, NULL, NULL, NULL);

    if (list_empty(&added_byte_pools)) {
        return available_bytes;
    }

    sys_enter_critical();
    list_for_each_safe(pos, n, &added_byte_pools) {
        p_byte_pool = list_entry(pos, add_byte_pool_t, list);
        tx_byte_pool_info_get(&p_byte_pool->byte_pool, NULL, &available_bytes_temp, NULL, NULL, NULL, NULL);
        available_bytes += available_bytes_temp;
    }
    sys_exit_critical();


    return available_bytes;
}

/*!
    \brief      get minimum free heap size that has been reached
    \param[in]  none
    \param[out] none
    \retval     system minimum free heap size value(0x00000000-0xffffffff)
*/
int32_t sys_min_free_heap_size(void)
{
    int32_t min_heap_size = 0;
    dlist_t *pos, *n;
    add_byte_pool_t *p_byte_pool;
    uint32_t total_size = byte_pool.tx_byte_pool_size;

    sys_enter_critical();
    list_for_each_safe(pos, n, &added_byte_pools) {
        p_byte_pool = list_entry(pos, add_byte_pool_t, list);
        total_size += p_byte_pool->byte_pool.tx_byte_pool_size;
    }
    sys_exit_critical();


    return (int32_t)(total_size - high_heap_mark);
}

/*!
    \brief      get system least heap block size
                Note: it's used to count all memory used by heap and heap management structures.
                    A least block is usually consist of a heap block header and a minimum block.
                    However heap allocation may not align the allocated heap area with this minimum block,
                    the counting results may be inaccurate.
    \param[in]  none
    \param[out] none
    \retval     system heap block size value in bytes(0x0000-0xffff)
*/
uint16_t sys_heap_block_size(void)
{
    return TX_BYTE_BLOCK_MIN;
}

/*!
    \brief      get heap info
    \param[in]  total_size: get total heap size
    \param[in]  free_size: get free heap size
    \param[in]  min_free_size: get minimum free heap size
    \param[out] none
    \retval     none
*/
void sys_heap_info(int *total_size, int *free_size, int *min_free_size)
{
    ULONG available_bytes = 0;
    ULONG available_bytes_temp = 0;

    dlist_t *pos, *n;
    add_byte_pool_t *p_byte_pool;
    uint32_t byte_size = byte_pool.tx_byte_pool_size;

    tx_byte_pool_info_get(&byte_pool, NULL, &available_bytes, NULL, NULL, NULL, NULL);

    sys_enter_critical();
    list_for_each_safe(pos, n, &added_byte_pools) {
        p_byte_pool = list_entry(pos, add_byte_pool_t, list);
        byte_size += p_byte_pool->byte_pool.tx_byte_pool_size;
        if(tx_byte_pool_info_get(&p_byte_pool->byte_pool, NULL, &available_bytes_temp, NULL, NULL, NULL, NULL) == TX_SUCCESS) {
            available_bytes += available_bytes_temp;
        }
    }
    sys_exit_critical();

    if(total_size) {
        *total_size = byte_size;
    }

    if(free_size) {
        *free_size = available_bytes;
    }

    if(min_free_size) {
        *min_free_size = (int)(byte_size - high_heap_mark);
    }

}

/***************** memory manipulation *****************/
extern void Mem_Copy(void *des, const void *src, uint32_t n);

/*!
    \brief      copy buffer content from source address to destination address.
    \param[in]  src: the address of source buffer
    \param[in]  n: the length to copy
    \param[out] des: the address of destination buffer
    \retval     none
*/
void sys_memcpy(void *des, const void *src, uint32_t n)
{
    memcpy(des, src, n);
}

/*!
    \brief      move buffer content from source address to destination address
                Note: It could work between two overlapped buffers.
    \param[in]  src: the address of source buffer
    \param[in]  n: the length to move
    \param[out] des: the address of destination buffer
    \retval     none
*/
void sys_memmove(void *des, const void *src, uint32_t n)
{
    char *tmp = (char *)des;
    char *s = (char *)src;

    if (s < tmp && tmp < s + n) {
        tmp += n;
        s += n;

        while (n--)
            *(--tmp) = *(--s);
    } else {
        while (n--)
            *tmp++ = *s++;
    }
}

/*!
    \brief      set the content of the buffer to specified value
    \param[in]  s: The address of a buffer
    \param[in]  c: the value want to memset
    \param[in]  count: count value want to memset
    \param[out] none
    \retval     none
*/
void sys_memset(void *s, uint8_t c, uint32_t count)
{
    uint32_t dword_cnt;
    uint32_t dword_value;
    uint8_t *p_dst = (uint8_t *)s;

    while (((uint32_t)p_dst & 0x03) != 0) {
        if (count == 0) {
            return;
        }
        *p_dst++ = c;
        count--;
    }

    dword_cnt = (count >> 2);
    count &= 0x03;
    dword_value = (((uint32_t)(c) << 0) & 0x000000FF) | (((uint32_t)(c) << 8) & 0x0000FF00) |
                    (((uint32_t)(c) << 16) & 0x00FF0000) | (((uint32_t)(c) << 24) & 0xFF000000);

    while (dword_cnt > 0u) {
        *(uint32_t *)p_dst = dword_value;
        p_dst += 4;
        dword_cnt--;
    }

    while (count > 0u) {
        *p_dst++ = c;
        count--;
    }
}

/*!
    \brief      compare two buffers
    \param[in]  buf1: address to the source buffer 1
    \param[in]  buf2: address to the source buffer 2
    \param[in]  count: the compared buffer size in bytes
    \param[out] none
    \retval      0 if buf1 equals buf2, non-zero otherwise.
*/
int32_t sys_memcmp(const void *buf1, const void *buf2, uint32_t count)
{
    if(!count)
        return 0;

    while (--count && *((char *)buf1) == *((char *)buf2)) {
        buf1 = (char *)buf1 + 1;
        buf2 = (char *)buf2 + 1;
    }

    return (*((uint8_t *)buf1) - *((uint8_t *)buf2));
}

/***************** OS API wrappers *****************/
/*!
    \brief      create a task wrapping a task handle and a message queue
                Note:  Message queue wrapped in the task_wrapper_t is created to only hold pointers,
                  therefore the objects pointed by these pointers are not thread safe. You need to protect
                  them for some scenarios.
    \param[in]  static_tcb: pointer to the task buffer
    \param[in]  name: the task's name
    \param[in]  stack_base: the task's stack base address
    \param[in]  stack_size: the task's stack size, in words (4 bytes)
    \param[in]  queue_size: the task's message queue size
    \param[in]  queue_item_size: the task's message queue item size
    \param[in]  priority: the task's priority
    \param[in]  func: the task's entry function
    \param[in]  ctx: the task's parameter
    \param[out] none
    \retval     the task handle if succeeded, NULL otherwise.
*/
void *sys_task_create(void *static_tcb, const uint8_t *name, uint32_t *stack_base, uint32_t stack_size,
                    uint32_t queue_size, uint32_t queue_item_size, uint32_t priority, task_func_t func, void *ctx)
{
    task_wrapper_t *task_wrapper = NULL;

    configASSERT(static_tcb == NULL);
    configASSERT(stack_base == NULL);
    configASSERT(func != NULL);
    configASSERT(priority < OS_TASK_PRIO_MAX);

    priority = OS_TASK_PRIO_MAX - 1 - priority;
    task_wrapper = (task_wrapper_t *)sys_malloc(sizeof(task_wrapper_t));

    if (task_wrapper == NULL) {
        return NULL;
    }

    sys_memset(task_wrapper, 0, sizeof(task_wrapper_t));

    stack_size = stack_size * sizeof(portSTACK_TYPE);
    task_wrapper->p_stack = sys_malloc(stack_size);
    if (task_wrapper->p_stack == NULL) {
        goto free_wapper;
    }


    task_wrapper->task_func = func;
    task_wrapper->func_argv = ctx;

    if (queue_size > 0) {
        if (sys_queue_init(&(task_wrapper->task_queue), queue_size, queue_item_size) != OS_OK) {
            goto free_stack;
        }
    }
    else {
        task_wrapper->task_queue = NULL;
    }

    if (name) {
        if ((strlen(name) + 1) < configMAX_TASK_NAME_LEN) {
            strcpy(task_wrapper->name, name);
        }
        else {
            strncpy(task_wrapper->name, name, (configMAX_TASK_NAME_LEN - 1));
        }
    }

    if(tx_semaphore_create(&task_wrapper->notification_sem, "", 0u) != TX_SUCCESS) {
        goto free_queue;
    }

    if (tx_thread_create(&task_wrapper->tx_thread, task_wrapper->name, tx_thread_func_wrapper, (ULONG)task_wrapper, task_wrapper->p_stack, stack_size,
                      priority, priority, 1, TX_AUTO_START) != TX_SUCCESS) {
        goto free_sema;
    }


    return (void *)task_wrapper;
free_sema:
    tx_semaphore_delete(&task_wrapper->notification_sem);
free_queue:
    if (task_wrapper->task_queue)
        sys_queue_free(&(task_wrapper->task_queue));
free_stack:
    sys_mfree(task_wrapper->p_stack);
free_wapper:
    sys_mfree(task_wrapper);
    return NULL;
}

void sys_task_change_timeslice(void *task, uint32_t timeslice)
{
    task_wrapper_t *task_wrapper = NULL;
    ULONG old_time_slice;

    configASSERT(task != NULL);

    task_wrapper = (task_wrapper_t *)task;
    tx_thread_time_slice_change(&task_wrapper->tx_thread, timeslice, &old_time_slice);
}


/*!
    \brief      return RTOS task name
    \param[in]  task: pointer to the task handle
    \param[out] none
    \retval     task name
*/
char* sys_task_name_get(void *task)
{
    if (task == NULL) {
        TX_THREAD       *thread_ptr;
        /* Pickup thread pointer.  */
        thread_ptr = tx_thread_identify();
        if (thread_ptr != NULL)
            return thread_ptr->tx_thread_name;
    }
    else {
        task_wrapper_t *task_wrapper = (task_wrapper_t *)task;

        if (task_wrapper != NULL) {
            return task_wrapper->tx_thread.tx_thread_name;
        }
    }

    return "";
}

/*!
    \brief      delete a task and free all associated resources
                Note: Message queue wrapped in the task_wrapper_t is created to only hold pointers,
                  therefore the objects pointed by these pointers are not thread safe. You need to protect
                  them for some scenarios.
    \param[in]  task: pointer to the task handle, delete the current task if it's NULL
    \param[out] none
    \retval     none
*/
void sys_task_delete(void *task)
{
    task_wrapper_t *task_wrapper = NULL;
    TX_THREAD *p_thread;
    UINT ret;
    TX_INTERRUPT_SAVE_AREA;

    if (task == NULL) {
        p_thread = tx_thread_identify();
        if (p_thread == NULL) {
            return;
        }
        task_wrapper = (task_wrapper_t *)(p_thread->tx_thread_entry_parameter);
    }
    else {
        task_wrapper = (task_wrapper_t *)task;
    }

    if (task_wrapper != NULL) {
        sys_enter_critical();
        co_list_push_back(&threadx_idle_task.rmv_task_list, &(task_wrapper->hdr));
        sys_exit_critical();
        // Make sure the task is terminated, which may return an error if that's already the case so the return value is ignored.
        ret = tx_thread_terminate(&task_wrapper->tx_thread);
        if(ret != TX_SUCCESS) {
            dbg_print(ERR, "task terminate fail\r\n");
            configASSERT(0);
        }
    }
}

/*!
    \brief      wait for a message from the message queue wrapped in the current task
    \param[in]  timeout_ms: millisecond timeout
                            0 means forever,
                            if the time is less than a tick, will roll up to 1 tick
    \param[out] msg_ptr: pointer to the message
    \retval     function run status
      \arg        OS_ERROR: sys_task_wait, task wrapper is NULL
      \arg        OS_TIMEOUT: get message timeout
      \arg        OS_OK: run success
*/
int32_t sys_task_wait(uint32_t timeout_ms, void *msg_ptr)
{
    task_wrapper_t *task_wrapper = NULL;
    TX_THREAD *p_thread;
    int32_t result;

    p_thread = tx_thread_identify();
    if (p_thread == NULL) {
        dbg_print(ERR, "sys_task_wait, current task is NULL\r\n");
        configASSERT(0);
        return OS_ERROR;
    }
    task_wrapper = (task_wrapper_t *)(p_thread->tx_thread_entry_parameter);
    result = sys_queue_fetch(&task_wrapper->task_queue, msg_ptr, timeout_ms, 1);
    if (result != OS_OK) {
        return OS_TIMEOUT;
    }

    return OS_OK;
}

/*!
    \brief      post a message to the message queue wrapped in the target task
    \param[in]  receiver_task: pointer to the message receiver task handle
    \param[in]  msg_ptr: pointer to the message
    \param[in]  from_isr: whether it's called from an ISR, it's only meaningful to some OS
                    with implementations seperated for different execution context, like FreeRTOS
    \param[out] none
    \retval     function run status
      \arg        OS_ERROR: have error happen
      \arg        OS_OK: run success
*/
int32_t sys_task_post(void *receiver_task, void *msg_ptr, uint8_t from_isr)
{
    task_wrapper_t *task_wrapper = (task_wrapper_t *)receiver_task;
    if (task_wrapper == NULL) {
        dbg_print(ERR, "sys_task_post, task wrapper is NULL\r\n");
        return OS_ERROR;
    }

    if (xWrapperQueueIsQueueFull((wapper_tx_queue_t *)task_wrapper->task_queue)) {
        dbg_print(ERR, "sys_task_post: queue full, task is %s\r\n", sys_task_name_get(task_wrapper));
    }

    if (xWrapperQueueSend((wapper_tx_queue_t *)task_wrapper->task_queue, msg_ptr, TX_NO_WAIT) != OS_OK) {
        dbg_print(ERR, "sys_task_post: send fail, return error\r\n");
        return OS_ERROR;
    }

    return OS_OK;

}

/*!
    \brief      flush all messages of the message queue wrapped in the target task
                Note: All messages will be recycled to the free queue and the task will be resumed to
                  run after the operation, since we are queuing pointers in the task wrapped message queue,
                  do not to forget to handle all these pointers well to avoid memory leak.
    \param[in]  task: pointer to the task handle
    \param[out] none
    \retval     none
*/
void sys_task_msg_flush(void *task)
{
    task_wrapper_t *task_wrapper = (task_wrapper_t *)task;
    TX_THREAD *p_thread;

    if (task == NULL) {
        p_thread = tx_thread_identify();
        task_wrapper = (task_wrapper_t *)(p_thread->tx_thread_entry_parameter);
    }

    if (task_wrapper != NULL) {
        xWrapperQueueReset((wapper_tx_queue_t *)task_wrapper->task_queue);
    }
}

/*!
    \brief      get the number of waiting messages in the task queue
    \param[in]  task: pointer to the task handle
    \param[in]  from_isr: whether is from ISR
    \param[out] none
    \retval     the number of waiting messages if succeeded, OS_ERROR otherwise.
*/
int32_t sys_task_msg_num(void *task, uint8_t from_isr)
{
    task_wrapper_t *task_wrapper = (task_wrapper_t *)task;
    TX_THREAD *p_thread;

    if (task == NULL) {
        p_thread = tx_thread_identify();
        task_wrapper = (task_wrapper_t *)(p_thread->tx_thread_entry_parameter);
    }

    if (task_wrapper == NULL) {
        dbg_print(ERR, "sys_task_msg_num, task wrapper is NULL\r\n");
        return OS_ERROR;
    }

    return sys_queue_cnt(&task_wrapper->task_queue);;

}

/*!
    \brief      initialize notification for a WIFI task.
    \param[in]  task: pointer to the task handle
    \param[out] none
    \retval     return 0 on success and != 0 if error occurred.
*/
int sys_task_init_notification(void *task)
{
    return 0;
}

/*!
    \brief      task suspend itself until it is notified (or timeout expires)
    \param[in]  timeout: Maximum duration to wait, in ms, if no notification is pending.
                         0 means do not wait and -1 means wait indefinitely.
    \param[out] none
    \retval     the number of pending notification (0 if timeout was reached)
*/
int sys_task_wait_notification(int timeout)
{
    return ulWrapperTaskNotifyTake(true, sys_timeout_2_tickcount(timeout));
}

/*!
    \brief      send notification to a task
    \param[in]  task: Task to notify.
    \param[in]  isr: Indicate if this is called from ISR.
    \param[out] none
    \retval     none
*/
void sys_task_notify(void *task, bool isr)
{
    task_wrapper_t *task_wrapper = (task_wrapper_t *)task;

    return xWrapperTaskNotifyAndQuery(task_wrapper, 0u, eWrapIncrement, NULL);
}

/*!
    \brief      return RTOS current task stack depth from special sp index
    \param[in]  cur_sp sp index
    \param[out] none
    \retval     stack depth
*/
int32_t sys_current_task_stack_depth(unsigned long cur_sp)
{
    TX_THREAD *p_thread;

    p_thread = tx_thread_identify();

    if (p_thread != NULL) {
        return ((uint8_t *)p_thread->tx_thread_stack_end - cur_sp);
    }

    return 0;
}


/*!
    \brief      get the free stack size of the target task
    \param[in]  task: the pointer to the task handle
    \param[out] none
    \retval     the free stack size in words (4 bytes)(0x00000000-0xffffffff)
*/
uint32_t sys_stack_free_get(void *task)
{
#ifdef TX_ENABLE_STACK_CHECKING
    task_wrapper_t *task_wrapper = (task_wrapper_t *)task;
    TX_THREAD *p_thread;

    if (task == NULL) {
        p_thread = tx_thread_identify();
        task_wrapper = (task_wrapper_t *)(p_thread->tx_thread_entry_parameter);
    }

    if (task_wrapper == NULL) {
        dbg_print(ERR, "sys_stack_free_get, task wrapper is NULL\r\n");
        return 0;
    }

    return task_wrapper->tx_thread.tx_thread_stack_highest_ptr - task_wrapper->tx_thread.tx_thread_stack_start;
#else
    dbg_print(ERR, "sys_stack_free_get, TX_ENABLE_STACK_CHECKING should defined\r\n");
    return 0;
#endif
}

/*!
    \brief      list statistics for all tasks
    \param[in]  none
    \param[out] pwrite_buf: pointer to the result buffer, expect a statistics string
    \retval     none
*/
void sys_task_list(char *pwrite_buf)
{
    char *buf = NULL;
    task_status_t *p_task_array = NULL;
    uint32_t threads_num;
    threads_num = _tx_thread_created_count;
    uint16_t i = 0;
    uint16_t j = 0;
    TX_THREAD *thread_ptr;
    uint8_t *pp;
    char cStatus;

    p_task_array = sys_zalloc(threads_num * sizeof(task_status_t));
    if (p_task_array == NULL) {
        dbg_print(ERR, "list statistics for all tasks failed, p_task_array == NULL.\r\n");
        return;
    }

    // 26: the length of string "\t%c\t%u\t%u\t%u\t0x%08x\r\n" in func vTaskList().
    if (NULL == pwrite_buf) {
        buf = sys_zalloc(threads_num * (configMAX_TASK_NAME_LEN + 26));
        if (buf == NULL) {
            dbg_print(ERR, "list statistics for all tasks failed, buf == NULL.\r\n");
            sys_mfree(p_task_array);
            return;
        }
    } else {
        buf = pwrite_buf;
    }

    vWrapperTaskSuspendAll();
    if (threads_num > _tx_thread_created_count) {
        threads_num = _tx_thread_created_count;
    }

    thread_ptr = _tx_thread_created_ptr;
    for (i = 0; i < threads_num; i++) {
        p_task_array[i].thread_ptr = thread_ptr;
        p_task_array[i].tx_thread_name = thread_ptr->tx_thread_name;
        p_task_array[i].tx_thread_state = thread_ptr->tx_thread_state;
        p_task_array[i].tx_thread_priority = thread_ptr->tx_thread_priority;
        p_task_array[i].tx_thread_stack_end = thread_ptr->tx_thread_stack_end;
        p_task_array[i].tx_thread_stack_highest_ptr = thread_ptr->tx_thread_stack_highest_ptr;
        p_task_array[i].tx_thread_time_slice = thread_ptr->tx_thread_time_slice;
        thread_ptr = thread_ptr->tx_thread_created_next;
    }

    xWrapperTaskResumeAll();

    pp = buf;
    for (i = 0; i < threads_num; i++) {
        bool padding = false;
        for( j = 0; j < (uint16_t) configMAX_TASK_NAME_LEN; j++ ) {
            if (padding) {
                pp[j] = ' ';
                continue;
            }

            if (p_task_array[i].tx_thread_name[j] == '\0') {
                padding = true;
                pp[j] = ' ';
            } else {
                pp[j] = p_task_array[i].tx_thread_name[j];
            }
        }

        pp += j;

        switch(p_task_array[i].tx_thread_state) {
            case TX_READY:
                if (p_task_array[i].thread_ptr == _tx_thread_current_ptr) {
                    cStatus = TX_RUNNING_CHAR;
                } else {
                    cStatus = TX_READY_CHAR;
                }
                break;

            case TX_QUEUE_SUSP:
            case TX_SEMAPHORE_SUSP:
            case TX_EVENT_FLAG:
            case TX_BLOCK_MEMORY:
            case TX_BYTE_MEMORY:
            case TX_IO_DRIVER:
            case TX_FILE:
            case TX_TCP_IP:
            case TX_MUTEX_SUSP:
                cStatus = TX_BLOCKED_CHAR;
                break;

            case TX_SUSPENDED:
            case TX_SLEEP:
                cStatus = TX_SUSPENDED_CHAR;
                break;

            case TX_COMPLETED:
            case TX_TERMINATED:
                cStatus = TX_DELETED_CHAR;
                break;

            default:            /* Should not get here, but it is included
                                to prevent static checking errors. */
                cStatus = ( char ) 0x00;
                break;
        }

        /* Write the rest of the string. */
        sprintf(pp, "\t%c\t%u\t%u\t%u\t0x%08x\r\n", cStatus, ( unsigned int ) p_task_array[i].tx_thread_priority,
                ( unsigned int ) p_task_array[i].tx_thread_stack_highest_ptr, ( unsigned int ) p_task_array[i].tx_thread_time_slice,
                ( unsigned int ) p_task_array[i].tx_thread_stack_end );

        pp += strlen(pp);
    }

    printf("%s\r\n", buf);
    sys_mfree(p_task_array);
    if (NULL == pwrite_buf)
        sys_mfree(buf);
}

/*!
    \brief      create and initialize a semaphore extend
    \param[in]  sema: the pointer to the semaphore handle
    \param[in]  max_count: max count of semaphore
    \param[in]  init_count: initialize count of semaphore
    \param[out] sema: pointer to the semaphore
    \retval     OS_OK if succeeded, OS_ERROR otherwise.
*/
int32_t sys_sema_init_ext(os_sema_t *sema, int max_count, int init_count)
{
    wrapper_tx_sem_t *p_sem = NULL;

    p_sem = sys_malloc(sizeof(wrapper_tx_sem_t));

    if (p_sem == NULL) {
        return OS_ERROR;
    }

    sys_memset(p_sem, 0, sizeof(wrapper_tx_sem_t));

    p_sem->max_count = max_count;
    p_sem->allocated = 1u;

    if(tx_semaphore_create(&p_sem->sem, "sys_sema", init_count) != TX_SUCCESS) {
        sys_mfree(p_sem);
        return OS_ERROR;
    }

    *sema = p_sem;
    return OS_OK;
}

/*!
    \brief      create and initialize a semaphore
    \param[in]  sema: the pointer to the semaphore handle
    \param[in]  init_val: initialize vlaue of semaphore
    \param[out] sema: pointer to the semaphore
    \retval     OS_OK if succeeded, OS_ERROR otherwise.
*/
int32_t sys_sema_init(os_sema_t *sema, int32_t init_val)
{
    return sys_sema_init_ext(sema, 0xffffffff, init_val);
}

/*!
    \brief      free a semaphore
    \param[in]  sema: pointer to the semaphore handle
    \param[out] none
    \retval     none
*/
void sys_sema_free(os_sema_t *sema)
{
    wrapper_tx_sem_t *p_sem = NULL;

    configASSERT(sema != NULL);

    p_sem = (wrapper_tx_sem_t *)(*sema);
    if (*sema == NULL) {
        return;
    }

    tx_semaphore_delete(&p_sem->sem);

    if(p_sem->allocated == 1u) {
        sys_mfree(p_sem);
    }

    *sema = NULL;
}

/*!
    \brief      release a semaphore
                Note:  It could be called either in task context or interrupt context except for some OS
                  that has seperated implementations for the different context, like FreeRTOS
    \param[in]  sema: pointer to the semaphore handle
    \param[out] none
    \retval     none
*/
void sys_sema_up(os_sema_t *sema)
{
    TX_INTERRUPT_SAVE_AREA;
    UINT ret;
    wrapper_tx_sem_t *p_sem = NULL;

    configASSERT(sema != NULL);

    p_sem= (wrapper_tx_sem_t *)(*sema);

    TX_DISABLE;
    _tx_thread_preempt_disable++;

    /* Maximum semaphore count reached return failure. */
    if(p_sem->sem.tx_semaphore_count >= p_sem->max_count) {
        dbg_print(ERR, "sys_sema_up, max_count limmited\r\n");
        goto sema_up_done;
    }

    ret = tx_semaphore_put(&p_sem->sem);
    if(ret != TX_SUCCESS) {
        dbg_print(ERR, "sys_sema_up failed\r\n");
    }

sema_up_done:
    _tx_thread_preempt_disable--;
    TX_RESTORE;

    _tx_thread_system_preempt_check();
}

/*!
    \brief      release a semaphore in a interrupt context, it's only meaningful to some OS
                with implementations seperated for different execution context, like FreeRTOS
    \param[in]  sema: pointer to the semaphore handle
    \param[out] none
    \retval     none
*/
void sys_sema_up_from_isr(os_sema_t *sema)
{
    sys_sema_up(sema);
}

/*!
    \brief      Require the semaphore within a given time constraint
    \param[in]  sema: pointer to the semaphore handle
    \param[in]  timeout_ms: millisecond value of timeout,
                    0 means forever,
                    if the time is less than a tick, will roll up to 1 tick.
    \param[out] none
    \retval     function run status
      \arg        OS_TIMEOUT: timeout
      \arg        OS_OK: run success
*/
int32_t sys_sema_down(os_sema_t *sema, uint32_t timeout_ms)
{
    uint32_t timeout_ticks = 0;
    wrapper_tx_sem_t *p_sema = NULL;
    UINT ret;

    configASSERT(sema != NULL);

    p_sema = (wrapper_tx_sem_t *)(*sema);

    if (timeout_ms == 0) {
        timeout_ticks = TX_WAIT_FOREVER;
    } else {
        timeout_ticks = (timeout_ms * TX_TIMER_TICKS_PER_SECOND + 999) / 1000;
    }

    ret = tx_semaphore_get(&p_sema->sem, timeout_ticks);
    if(ret != TX_SUCCESS) {
        return OS_TIMEOUT;
    }

    return OS_OK;
}

/*!
    \brief      return a semaphore count.
    \param[in]  sema: pointer to the semaphore handle
    \param[out] none
    \retval     semaphore count.
*/
int sys_sema_get_count(os_sema_t *sema)
{
    UINT ret;
    ULONG count;
    wrapper_tx_sem_t *p_sema = NULL;

    configASSERT(sema != NULL);

    p_sema = (wrapper_tx_sem_t *)(*sema);

    ret = tx_semaphore_info_get(&p_sema->sem, NULL, &count, NULL, NULL, NULL);
    if(ret != TX_SUCCESS) {
        return 0;
    }

    return (int)count;

}

/*!
    \brief      create and initialize a mutex
    \param[in]  mutex: pointer to the mutext handle
    \param[out] none
    \retval     function run status
      \arg        OS_ERROR: return error
      \arg        OS_OK: run success
*/
int sys_mutex_init(os_mutex_t *mutex)
{
    wrapper_tx_mutex_t *p_mutex = NULL;
    UINT ret;

    p_mutex = sys_malloc(sizeof(wrapper_tx_mutex_t));

    configASSERT(p_mutex != NULL);

    if(p_mutex == NULL) {
        *mutex = NULL;
        return OS_ERROR;
    }

    sys_memset(p_mutex, 0, sizeof(wrapper_tx_mutex_t));
    p_mutex->allocated = 1u;

    ret = tx_mutex_create(&p_mutex->mutex, "sys mutex", TX_INHERIT);
    if(ret != TX_SUCCESS) {
        sys_mfree(p_mutex);
        *mutex = NULL;
        return OS_ERROR;
    }

    *mutex = p_mutex;
    return OS_OK;
}

/*!
    \brief      free a mutex
    \param[in]  mutex: pointer to the mutex handle
    \param[out] none
    \retval     none
*/
void sys_mutex_free(os_mutex_t *mutex)
{
    UINT ret;
    wrapper_tx_mutex_t *p_mutex = NULL;

    if (*mutex == NULL) {
        dbg_print(ERR, "sys_mutex_free, mutex = NULL\r\n");
        return;
    }

    p_mutex = (wrapper_tx_mutex_t *)(*mutex);

    ret = tx_mutex_delete(&p_mutex->mutex);

    if(ret != TX_SUCCESS) {
        dbg_print(ERR, "sys_mutex_free, delete mutex fail\r\n");
        return;
    }

    if(p_mutex->allocated == 1u) {
        sys_mfree(p_mutex);
    }

    *mutex = NULL;
}

/*!
    \brief      require the mutex
    \param[in]  mutex: pointer to the mutex handle
    \param[out] none
    \retval     function run status
      \arg        OS_ERROR: return error
      \arg        OS_OK: run success
*/
int32_t sys_mutex_get(os_mutex_t *mutex)
{
    wrapper_tx_mutex_t *p_mutex = NULL;
    ULONG timeout = 60 * 1000 / OS_MS_PER_TICK;

    p_mutex = (wrapper_tx_mutex_t *)(*mutex);

    while (tx_mutex_get(&p_mutex->mutex, timeout) != TX_SUCCESS) {
        dbg_print(ERR, "[%s] get mutex 0x%08x failed, retry\r\n", sys_task_name_get(NULL), *mutex);
    }

    return OS_OK;
}

/*!
    \brief      try to require the mutex
    \param[in]  mutex: pointer to the mutex handle
    \param[in]  timeout: Maximum duration to wait, in ms.
                         0 means do not wait and -1 means wait indefinitely.
    \param[out] none
    \retval     function run status
      \arg        OS_ERROR: return error
      \arg        OS_OK: run success
*/
int32_t sys_mutex_try_get(os_mutex_t *mutex, int timeout)
{
    wrapper_tx_mutex_t *p_mutex = NULL;

    p_mutex = (wrapper_tx_mutex_t *)(*mutex);

    if (tx_mutex_get(&p_mutex->mutex, sys_timeout_2_tickcount(timeout)) != TX_SUCCESS) {
        return OS_ERROR;
    }

    return OS_OK;
}

/*!
    \brief      release a mutex
                Note: It could be only called in task context for some OS
                  that has seperated implementations for the different context, like FreeRTOS
    \param[in]  mutex: pointer to the mutext
    \param[out] none
    \retval     none
*/
void sys_mutex_put(os_mutex_t *mutex)
{
    UINT ret;
    wrapper_tx_mutex_t *p_mutex = NULL;
    configASSERT(mutex);
    configASSERT(*mutex);

    p_mutex = (wrapper_tx_mutex_t *)(*mutex);
    ret = tx_mutex_put(&p_mutex->mutex);
    if(ret != TX_SUCCESS) {
        dbg_print(ERR, "sys_mutex_put failed\r\n");
        return;
    }
}

/*!
    \brief      create and initialize a message queue
    \param[in]  queue_size: queue size
    \param[in]  item_size: queue item size
    \param[out] queue: pointer to the queue
    \retval     function run status
      \arg        OS_ERROR: return error
      \arg        OS_OK: run success
*/
int32_t sys_queue_init(os_queue_t *queue, int32_t queue_size, uint32_t item_size)
{
    wapper_tx_queue_t *p_queue = NULL;
    void *p_mem = NULL;
    size_t mem_size;
    uint32_t ret;

    configASSERT(queue_size > 0);

    p_queue = sys_malloc(sizeof(wapper_tx_queue_t));
    if(p_queue == NULL) {
        return OS_ERROR;
    }

    sys_memset(p_queue, 0, sizeof(wapper_tx_queue_t));

    mem_size = item_size * queue_size;
    p_mem = sys_malloc(mem_size);
    if(p_mem == NULL) {
        goto free_queue;
    }

    sys_memset(p_mem, 0, mem_size);
    p_queue->p_mem = p_mem;

    // Use thread queue api
    if (item_size >= sizeof(ULONG) && item_size <= (16 * sizeof(ULONG)) &&
          (item_size % sizeof(ULONG)) == 0) {

        p_queue->p_set = sys_malloc(sizeof(tx_queueset_t));
        if(p_queue->p_set == NULL) {
            goto free_mem;
        }

        ret = tx_queue_create(&p_queue->p_set->queue, "sys_queue", item_size / sizeof(ULONG), p_mem, mem_size);
        if(ret != TX_SUCCESS) {
            configASSERT(0);
            goto free_set;
        }
        *queue = p_queue;
        return OS_OK;
    }

    // Use semaphore to create queue
    p_queue->allocated = 1u;
    p_queue->id = TX_QUEUE_ID;
    p_queue->p_write = (uint8_t *)p_mem;
    p_queue->p_read = (uint8_t *)p_mem;
    p_queue->msg_size = item_size;
    p_queue->queue_length = queue_size;

    ret = tx_semaphore_create(&p_queue->read_sem, "sys_queue", 0u);
    if(ret != TX_SUCCESS) {
        goto free_mem;
    }

    ret = tx_semaphore_create(&p_queue->write_sem, "sys_queue", queue_size);
    if(ret != TX_SUCCESS) {
        tx_semaphore_delete(&p_queue->read_sem);
        goto free_mem;
    }

    *queue = p_queue;
    return OS_OK;

free_set:
    sys_mfree(p_queue->p_set);
free_mem:
    sys_mfree(p_mem);
free_queue:
    sys_mfree(p_queue);
    dbg_print(ERR, "sys_queue_init fail\r\n");
    return OS_ERROR;
}

/*!
    \brief      free a message queue
    \param[in]  queue: pointer to the queue
    \param[out] none
    \retval     none
*/
void sys_queue_free(os_queue_t *queue)
{
    INT ret;
    wapper_tx_queue_t *wrapper_queue = (wapper_tx_queue_t *)(*queue);

    if (wrapper_queue != NULL) {
        // Use wrapper queue
        if (wrapper_queue->allocated == 1u) {
            ret = tx_semaphore_delete(&wrapper_queue->read_sem);
            if(ret != TX_SUCCESS) {
                configASSERT(0);
            }

            ret = tx_semaphore_delete(&wrapper_queue->write_sem);
            if(ret != TX_SUCCESS) {
                configASSERT(0);
            }
        }
        else {
            ret = tx_queue_delete(&wrapper_queue->p_set->queue);
            if(ret != TX_SUCCESS) {
                configASSERT(0);
            }
            sys_mfree(wrapper_queue->p_set);
        }

        sys_mfree(wrapper_queue->p_mem);
        sys_mfree(wrapper_queue);
    }

    *queue = NULL;
}

/*!
    \brief      post a message to the target message queue
    \param[in]  queue: pointer to the queue handle
    \param[in]  msg: pointer to the message
    \param[out] none
    \retval     function run status
      \arg        OS_ERROR: return error
      \arg        OS_OK: run success
*/
int32_t sys_queue_post(os_queue_t *queue, void *msg)
{
    configASSERT(queue != NULL);
    configASSERT(*queue != NULL);
    configASSERT(msg != NULL);

    return xWrapperQueueSend((wapper_tx_queue_t *)(*queue), msg, TX_NO_WAIT);
}

/*!
    \brief      post a message to the target message queue
    \param[in]  queue: pointer to the queue handle
    \param[in]  msg: pointer to the message
    \param[out] none
    \retval     function run status
      \arg        OS_ERROR: return error
      \arg        OS_OK: run success
*/
int32_t sys_queue_post_with_timeout(os_queue_t *queue, void *msg, int32_t timeout_ms)
{
    configASSERT(queue != NULL);
    configASSERT(*queue != NULL);
    configASSERT(msg != NULL);

    return xWrapperQueueSend(*queue, msg, timeout_ms);
}

/*!
    \brief      fetch a message from the message queue within a given time constraint
    \param[in]  queue: pointer to the queue handle
    \param[in]  timeout_ms: timeout in ms. For blocking operation, 0 means forever, if the time is
                    less than a tick, will roll up to 1 tick; for non-blocking operation, it's unused.
    \param[in]  is_blocking: whether it's a blocking operation
    \param[out] msg: pointer to the message
    \retval     function run status
      \arg        OS_TIMEOUT: timeout
      \arg        OS_OK: run success
*/
int32_t sys_queue_fetch(os_queue_t *queue, void *msg, uint32_t timeout_ms, uint8_t is_blocking)
{
    uint32_t timeout_ticks = 0;

    configASSERT(queue != NULL);
    configASSERT(*queue != NULL);
    configASSERT(msg != NULL);

    if (!is_blocking) {
        timeout_ticks = 0;
    } else if (is_blocking && (timeout_ms == 0)) {
        timeout_ticks = TX_WAIT_FOREVER;
    }
    else {
        timeout_ticks = (timeout_ms * TX_TIMER_TICKS_PER_SECOND + 999) / 1000;
    }

    return xWrapperQueueReceive((wapper_tx_queue_t *)(*queue), msg, timeout_ticks);
}

/*!
    \brief      check if a RTOS message queue is empty or not.
    \param[in]  queue: pointer to the queue handle
    \param[out] none
    \retval     true if queue is empty, false otherwise.
*/
bool sys_queue_is_empty(os_queue_t *queue)
{
    ULONG count;
    UINT ret;
    wapper_tx_queue_t *wrapper_queue;

    configASSERT(queue != NULL);
    configASSERT(*queue != NULL);

    wrapper_queue = (wapper_tx_queue_t *)(*queue);

    // Use thread Queue API
    if(wrapper_queue->p_set != NULL) {
        ret = tx_queue_info_get(&wrapper_queue->p_set->queue, TX_NULL, &count, TX_NULL, TX_NULL, TX_NULL, TX_NULL);
        if(ret != TX_SUCCESS) {
            // Fatal error, queue full errors are ignored on purpose to match the original behaviour.
            configASSERT(0);
            return false;
        }
        goto done;
    }

    ret = tx_semaphore_info_get(&wrapper_queue->read_sem, NULL, &count, NULL, NULL, NULL);
    if(ret != TX_SUCCESS) {
        configASSERT(0);
        return false;
    }

done:
    if(count == 0u) {
        return true;
    } else {
        return false;
    }
}

/*!
    \brief      get the number of messages pending a queue.
    \param[in]  queue: pointer to the queue handle
    \param[out] none
    \retval     The number of messages pending in the queue.
*/
int sys_queue_cnt(os_queue_t *queue)
{
    ULONG count = 0;
    UINT ret;
    wapper_tx_queue_t *p_wrapper_queue = NULL;

    configASSERT(queue != NULL);
    configASSERT(*queue != NULL);

    p_wrapper_queue = (wapper_tx_queue_t *)(*queue);

    // Use thread Queue API
    if(p_wrapper_queue->p_set != NULL) {
        tx_queue_info_get(&p_wrapper_queue->p_set->queue, NULL, &count, NULL,
                    NULL, NULL, NULL);
        return count;
    }

    // use wrapper queue
    ret = tx_semaphore_info_get(&p_wrapper_queue->read_sem, NULL, &count, NULL, NULL, NULL);
    if(ret != TX_SUCCESS) {
        return 0;
    }

    return count;

}

/*!
    \brief      write a message at the end of a RTOS message queue.
    \param[in]  queue: pointer to the queue handle
    \param[in]  msg: Message to copy in the queue. (It is assume that buffer is of the size specified in @ref os_queue_t_create)
    \param[in]  timeout: Maximum duration to wait, in ms, if queue is full. 0 means do not wait and -1 means wait indefinitely.
    \param[in]  isr: Indicate if this is called from ISR. If set, @p timeout parameter is ignored.
    \param[out] none
    \retval     0 on success and != 0 if error occurred (i.e queue was full and maximum duration has been reached).
*/
int sys_queue_write(os_queue_t *queue, void *msg, int timeout, bool isr)
{
    configASSERT(queue != NULL);
    configASSERT(*queue != NULL);

    if (isr) {
        timeout = 0;
    }

    return xWrapperQueueSend((wapper_tx_queue_t *) (*queue), msg, timeout);
}

/*!
    \brief      read a message from a RTOS message queue.
    \param[in]  queue: pointer to the queue handle
    \param[in]  msg: Buffer to copy into. (It is assume that buffer is of the size specified in @ref os_queue_t_create)
    \param[in]  timeout: Maximum duration to wait, in ms, if queue is empty. 0 means do not wait and -1 means wait indefinitely.
    \param[in]  isr: Indicate if this is called from ISR. If set, @p timeout parameter is ignored.
    \param[out] none
    \retval     0 on success and != 0 if error occurred (i.e queue was empty and maximum duration has been reached).
*/
int sys_queue_read(os_queue_t *queue, void *msg, int timeout, bool isr)
{
    uint32_t timeout_ticks = 0;
    configASSERT(queue != NULL);
    configASSERT(*queue != NULL);

    if (isr) {
        timeout = 0;
    }

    timeout_ticks = sys_timeout_2_tickcount(timeout);
    return xWrapperQueueReceive((wapper_tx_queue_t *)(*queue), msg, timeout_ticks);
}

/*!
    \brief      get the current system up time
    \param[in]  none
    \param[out] none
    \retval     milliseconds since the system boots up(0x00000000-0xffffffff)
*/
uint32_t sys_current_time_get(void)
{
    uint32_t ticks = tx_time_get();
    return ticks * OS_MS_PER_TICK;
}

/*!
    \brief      get the system time
    \param[in]  p: not used
    \param[out] none
    \retval     milliseconds since the system boots up(0x00000000-0xffffffff)
*/
uint32_t sys_time_get(void *p)
{
    return sys_current_time_get();
}

/*!
    \brief      put current task to sleep for a specified period of time
    \param[in]  ms: millisecond value, if the time is less than a tick, will roll up to 1 tick.
    \param[out] none
    \retval     none
*/
void sys_ms_sleep(int ms)
{
    uint32_t tick;

    if (ms <= 0)
        return;

    tick = ms / OS_MS_PER_TICK;
    if (tick == 0) {
        tick = 1;
    }

    tx_thread_sleep(tick);
}

/*!
    \brief      delay the current task for several microseconds
                Note: The task will being blocked during the delayed time
    \param[in]  nus: microsecond value
    \param[out] none
    \retval     none
*/
void sys_us_delay(uint32_t nus)
{
    vWrapperTaskSuspendAll();
    systick_udelay(nus);
    xWrapperTaskResumeAll();
}

/*!
    \brief      give up the execution of the current task
    \param[in]  none
    \param[out] none
    \retval     none
*/
void sys_yield(void)
{
    tx_thread_relinquish();
}

/*!
    \brief      pend the task scheduling
    \param[in]  none
    \param[out] none
    \retval     none
*/
void sys_sched_lock(void)
{
    vWrapperTaskSuspendAll();
}

/*!
    \brief      resume the task scheduling
    \param[in]  none
    \param[out] none
    \retval     none
*/
void sys_sched_unlock(void)
{
    xWrapperTaskResumeAll();
}

/*!
    \brief      Initialize the given buffer with the random data
    \param[in]  size: size of random data
    \param[out] dst: pointer to the get random bytes
    \retval     function run status
      \arg        OS_OK: run success
*/
int32_t sys_random_bytes_get(void *dst, uint32_t size)
{
#ifndef CFG_PLATFORM_FPGA_V7
    return random_get(dst, size);
#else
    size_t i;
    unsigned char *dstc = (unsigned char *)dst;

    for (i = 0; i < size ; i++) {
        dstc[i] = co_rand_byte();
    }
    return 0;
#endif
}

/*!
    \brief      initialize a timer
    \param[in]  timer: pointer to the timer handle
    \param[in]  name: pointer to the timer name
    \param[in]  delay: the timeout in milliseconds
    \param[in]  periodic: whether it's periodic timer
    \param[in]  func: the timer call back function
    \param[in]  arg:the argument that will pass to the call back
    \param[out] none
    \retval     none
*/
void sys_timer_init(os_timer_t *timer, const uint8_t *name, uint32_t delay, uint8_t periodic, timer_func_t func, void *arg)
{
    wrapper_tx_timer_t  *p_timer;
    UINT ret;
    ULONG resch_ticks;

    p_timer = (wrapper_tx_timer_t *)sys_zalloc(sizeof(wrapper_tx_timer_t));
    if (p_timer == NULL) {
        dbg_print(ERR, "sys_timer_init, malloc timer context failed\r\n");
        return;
    }

    p_timer->callback = func;
    p_timer->args = arg;
    p_timer->period = (delay / OS_MS_PER_TICK);

    if(periodic != false) {
        resch_ticks = p_timer->period;
        p_timer->one_shot = 1u;
    } else {
        p_timer->one_shot = 0u;
        resch_ticks = 0u;
    }

    // NOTE: If define TX_TIMER_PROCESS_IN_ISR, callback will be done in isr
    ret = tx_timer_create(&p_timer->timer, (char *)name, tx_timer_callback_wrapper, (ULONG)p_timer, p_timer->period, resch_ticks, TX_NO_ACTIVATE);
    if(ret != TX_SUCCESS) {
        sys_mfree(p_timer);
        dbg_print(ERR, "sys_timer_init, return error\r\n");
        return;
    }

    *timer = p_timer;
}

/*!
    \brief      delete a timer
    \param[in]  timer: pointer to the timer handle
    \param[out] none
    \retval     none
*/
void sys_timer_delete(os_timer_t *timer)
{
    wrapper_tx_timer_t  *p_timer;
    UINT ret;

    configASSERT(timer != NULL);

    p_timer = (wrapper_tx_timer_t *)(*timer);

    if (p_timer == NULL) {
        dbg_print(ERR, "sys_timer_delete, timer = NULL\r\n");
        return;
    }

    ret = tx_timer_delete(&p_timer->timer);
    if(ret != TX_SUCCESS) {
        configASSERT(0);
        dbg_print(ERR, "sys_timer_delete fail\r\n");
        return;
    }

    sys_mfree(p_timer);
}

/*!
    \brief       start a timer
    \param[in]  timer: pointer to the timer handle
    \param[in]  from_isr: whether it's called from an ISR, it's only meaningful to some OS
                  with implementations seperated for different execution context, like FreeRTOS
    \param[out] none
    \retval     none
*/
void sys_timer_start(os_timer_t *timer, uint8_t from_isr)
{
    wrapper_tx_timer_t  *p_timer;
    UINT ret;

    configASSERT(timer != NULL);

    p_timer = (wrapper_tx_timer_t *)(*timer);

    if (p_timer == NULL) {
        dbg_print(ERR, "sys_timer_start, timer = NULL\r\n");
        return;
    }

    ret = tx_timer_activate(&p_timer->timer);
    if(ret != TX_SUCCESS) {
        dbg_print(ERR, "sys_timer_start (0x%08x) return fail, from_isr is %d\r\n", *timer, from_isr);
        return;
    }
}

/*!
    \brief      start the timer extension with a specified timeout
    \param[in]  timer: pointer to the timer handle
    \param[in]  delay: time want to delay
    \param[in]  from_isr: whether it's called from an ISR, it's only meaningful to some OS
                  with implementations seperated for different execution context, like FreeRTOS
    \param[out] none
    \retval     none
*/
void sys_timer_start_ext(os_timer_t *timer, uint32_t delay, uint8_t from_isr)
{
    wrapper_tx_timer_t  *p_timer;
    UINT ret;
    uint32_t timer_ticks;
    TX_INTERRUPT_SAVE_AREA;

    configASSERT(timer != NULL);

    p_timer = (wrapper_tx_timer_t *)(*timer);

    if (p_timer == NULL) {
        dbg_print(ERR, "sys_timer_start_ext, timer = NULL\r\n");
        return;
    }

    if (delay <= OS_MS_PER_TICK) {
        timer_ticks = 1;
    } else {
        timer_ticks = delay / OS_MS_PER_TICK;
    }

    TX_DISABLE;

    ret = tx_timer_deactivate(&p_timer->timer);

    if(ret != TX_SUCCESS) {
        TX_RESTORE;
        dbg_print(ERR, "sys_timer_start_ext, stop timer fail\r\n");
        return;
    }

    if(p_timer->one_shot != 0u) {
        ret = tx_timer_change(&p_timer->timer, timer_ticks, timer_ticks);
    } else {
        ret = tx_timer_change(&p_timer->timer, timer_ticks, 0u);
    }

    if(ret != TX_SUCCESS) {
        TX_RESTORE;
        dbg_print(ERR, "sys_timer_start_ext, change time fail\r\n");
        return;
    }

    ret = tx_timer_activate(&p_timer->timer);
    if(ret != TX_SUCCESS) {
        TX_RESTORE;
        dbg_print(ERR, "sys_timer_start_ext, restart timer fail\r\n");
        return;
    }

    TX_RESTORE;
}

/*!
    \brief      stop the timer
    \param[in]  timer: pointer to the timer handle
    \param[in]  from_isr: whether it's called from an ISR, it's only meaningful to some OS
                      with implementations seperated for different execution context, like FreeRTOS
    \param[out] none
    \retval     function run state 1: run ok, 0: have error happen
*/
uint8_t sys_timer_stop(os_timer_t *timer, uint8_t from_isr)
{
    wrapper_tx_timer_t  *p_timer;
    UINT ret;

    configASSERT(timer != NULL);

    p_timer = (wrapper_tx_timer_t *)(*timer);

    if (p_timer == NULL) {
        dbg_print(ERR, "sys_timer_stop, timer = NULL\r\n");
        return;
    }

    ret = tx_timer_deactivate(&p_timer->timer);
    if(ret != TX_SUCCESS) {
        dbg_print(ERR, "sys_timer_stop fail\r\n");
        return 0;
    }

    return 1;
}

/*!
    \brief      check if the timer is active and pending for expiration
    \param[in]  timer: pointer to the timer handle
    \param[out] none
    \retval     1 if it's pending, 0 otherwise.
*/
uint8_t sys_timer_pending(os_timer_t *timer)
{
    wrapper_tx_timer_t  *p_timer;
    UINT ret;
    UINT is_active;

    configASSERT(timer != NULL);

    p_timer = (wrapper_tx_timer_t *)(*timer);

    if (p_timer == NULL) {
        dbg_print(ERR, "sys_timer_pending, timer = NULL\r\n");
        return;
    }

    ret = tx_timer_info_get(&p_timer->timer, NULL, &is_active, NULL, NULL, NULL);
    if(ret !=  TX_SUCCESS) {
        dbg_print(ERR, "sys_timer_pending get info fail\r\n");
        return 0;
    }

    if(is_active == TX_TRUE) {
        return 0;
    } else {
        return 1;
    }

}

/*!
    \brief      Miscellaneous initialization work after OS initialized and
                task scheduler started
                Note: It runs in the standalone start task
    \param[in]  none
    \param[out] none
    \retval     none
*/
void sys_os_misc_init(void)
{

}

/*!
    \brief      initialize the OS
    \param[in]  none
    \param[out] none
    \retval     none
*/
void sys_os_init(void)
{
    _tx_initialize_kernel_setup();

    /* Create a byte memory pool from which to allocate the thread stacks.  */
    tx_byte_pool_create(&byte_pool, "byte pool", ucHeap, configTOTAL_HEAP_SIZE);
    INIT_DLIST_HEAD(&added_byte_pools);
    cur_heap_mark = 0;
    high_heap_mark = 0;

#ifdef TX_NOT_INTERRUPTABLE
    tx_byte_allocate(&byte_pool, (VOID **) &critic_queue.p_mem,  CRITAL_QUEUE_SIZE * sizeof(ULONG), TX_NO_WAIT);
    if(tx_queue_create(&critic_queue.queue, "crital_queue", TX_1_ULONG, critic_queue.p_mem, CRITAL_QUEUE_SIZE * sizeof(ULONG)) != TX_SUCCESS) {
        configASSERT(0);
    }

    cur_heap_mark = (CRITAL_QUEUE_SIZE * sizeof(ULONG) + (sizeof(UCHAR *)) + (sizeof(ALIGN_TYPE)));
    high_heap_mark = cur_heap_mark;
#endif

    create_threadx_idle_task();
}

/*!
    \brief      start the OS
    \param[in]  none
    \param[out] none
    \retval     none
*/
void sys_os_start(void)
{
    /* start scheduler */
    /* Enter the ThreadX kernel.  */
    tx_kernel_enter();
}

/*!
    \brief      get the current RTOS time, in tick.
    \param[in]  isr: Indicate if this is called from ISR.
    \param[out] none
    \retval     The current RTOS time (in tick)
*/
uint32_t sys_os_now(bool isr)
{
    return tx_time_get();
}

/*!
    \brief      add heap region
    \param[in]  ucStartAddress: start address.
    \param[in]  xSizeInBytes: size in bytes.
    \param[out] none
    \retval     none
*/
void sys_add_heap_region(uint32_t ucStartAddress, uint32_t xSizeInBytes)
{
    add_byte_pool_t *p_byte_pool = sys_calloc(1, sizeof(add_byte_pool_t));

    if (p_byte_pool) {
        INIT_DLIST_HEAD(&p_byte_pool->list);
        co_snprintf(p_byte_pool->name, ADD_BYTE_POOL_NAME_LEN, "pool_%08x:", ucStartAddress);
        if (tx_byte_pool_create(&p_byte_pool->byte_pool, p_byte_pool->name, (void *)ucStartAddress, xSizeInBytes) == TX_SUCCESS)
        {
            sys_enter_critical();
            list_add_tail(&p_byte_pool->list, &added_byte_pools);
            sys_exit_critical();
        }
        else {
            sys_mfree(p_byte_pool);
        }
    }
}

/*!
    \brief      remove heap region
    \param[in]  ucStartAddress: start address.
    \param[in]  xSizeInBytes: size in bytes.
    \param[out] none
    \retval     none
*/
void sys_remove_heap_region(uint32_t ucStartAddress, uint32_t xSizeInBytes)
{
    dlist_t *pos, *n;
    add_byte_pool_t *p_byte_pool;

    if (list_empty(&added_byte_pools)) {
        return;
    }

    sys_enter_critical();
    list_for_each_safe(pos, n, &added_byte_pools) {
        p_byte_pool = list_entry(pos, add_byte_pool_t, list);
        if ((uint32_t)p_byte_pool->byte_pool.tx_byte_pool_start == ucStartAddress) {
            if (tx_byte_pool_delete(&p_byte_pool->byte_pool) == TX_SUCCESS) {
                list_del(&p_byte_pool->list);
                sys_mfree(p_byte_pool);
            }
            break;
        }
    }
    sys_exit_critical();
}

/*!
    \brief      return RTOS current task handle
    \param[in]  none
    \param[out] none
    \retval     current task handle
*/
os_task_t sys_current_task_handle_get(void)
{
    TX_THREAD *p_thread;

    p_thread = tx_thread_identify();

    return (os_task_t)(p_thread->tx_thread_entry_parameter);
}

/*!
    \brief      change the priority of a task
    \param[in]  task: Task handle.
    \param[in]  priority: priority to set to the task.
    \param[out] none
    \retval     none
*/
void sys_priority_set(void *task, os_prio_t priority)
{
    TX_THREAD *p_thread;
    UINT old_priority;
    UINT ret;
    task_wrapper_t *task_wrapper = (task_wrapper_t *)task;

    configASSERT(priority < OS_TASK_PRIO_MAX);

    priority = OS_TASK_PRIO_MAX - 1 - priority;

    if(task == NULL) {
        p_thread = tx_thread_identify();
    } else {
        p_thread = &task_wrapper->tx_thread;
    }

    if (p_thread == NULL) {
        dbg_print(ERR, "current thread is NULL\r\n");
        return;
    }

    ret = tx_thread_priority_change(p_thread, priority, &old_priority);
    if(ret != TX_SUCCESS) {
        configASSERT(0);
        dbg_print(ERR, "sys_priority_set fail\r\n");
        return;
    }
}

/*!
    \brief      get the priority of a task
    \param[in]  task: Task handle.
    \param[out] priority to the task
    \retval     none
*/
os_prio_t sys_priority_get(void *task)
{
    TX_THREAD *p_thread;
    UINT priority;
    UINT ret;
    task_wrapper_t *task_wrapper = (task_wrapper_t *)task;

    if(task == NULL) {
        p_thread = tx_thread_identify();
    } else {
        p_thread = &task_wrapper->tx_thread;
    }

    ret = tx_thread_info_get(p_thread, NULL, NULL, NULL, &priority, NULL, NULL, NULL, NULL);
    if(ret != TX_SUCCESS) {
        configASSERT(0);
        dbg_print(ERR, "sys_priority_get fail\r\n");
        return 0;
    }

    return (TX_MAX_PRIORITIES - 1u - priority);

}

/*!
    \brief      rtos in critical
    \param[in]  none
    \param[out] none
    \retval     interrupt status in the critical nesting
*/
uint32_t sys_in_critical(void)
{
#ifdef TX_NOT_INTERRUPTABLE
    ULONG count = 0;
    tx_queue_info_get(&critic_queue.queue, NULL, &count, NULL,
                        NULL, NULL, NULL);
    return count;
#else

    return vPortInCritical();
#endif
}

/*!
    \brief      rtos enter critical
    \param[in]  none
    \param[out] none
    \retval     none
*/
void sys_enter_critical(void)
{
#ifdef TX_NOT_INTERRUPTABLE
    ULONG count = 0;
    register int interrupt_save;
    int interrupt_value;
    TX_DISABLE;

    // Use thread Queue API
    tx_queue_info_get(&critic_queue.queue, NULL, &count, NULL,
                    NULL, NULL, NULL);

    if (count >= CRITAL_QUEUE_SIZE) {
        dbg_print(ERR, "sys_enter_critical nest too much\r\n");
        configASSERT(0);
        TX_RESTORE;
        return;
    }

    _tx_thread_preempt_disable++;
    interrupt_value = interrupt_save;
    tx_queue_send(&critic_queue.queue, &interrupt_value, TX_NO_WAIT);
#else
    vPortEnterCritical();
    _tx_thread_preempt_disable++;
#endif
}

/*!
    \brief      rtos exit critical
    \param[in]  none
    \param[out] none
    \retval     none
*/

void sys_exit_critical(void)
{
#ifdef TX_NOT_INTERRUPTABLE
    ULONG count = 0;
    register int interrupt_save;
    int interrupt_value;
    TX_DISABLE;

    // Use thread Queue API
    tx_queue_info_get(&critic_queue.queue, NULL, &count, NULL,
                    NULL, NULL, NULL);

    if (count == 0) {
        dbg_print(ERR, "sys_exit_critical not nested\r\n");
        configASSERT(0);
        TX_RESTORE;
        return;
    }

    _tx_thread_preempt_disable--;
    tx_queue_receive(&critic_queue.queue, &interrupt_value, TX_NO_WAIT);
    interrupt_save = interrupt_value;
    TX_RESTORE;
#else
    _tx_thread_preempt_disable--;
    vPortExitCritical();
    _tx_thread_system_preempt_check();
#endif
}

/*!
    \brief      OS IRQ service hook called just after the ISR starts
    \param[in]  none
    \param[out] none
    \retval     none
*/
void sys_int_enter(void)
{
    // Threadx no longer need record this
    return;
}

/*!
    \brief      OS IRQ service hook called before the ISR exits
    \param[in]  none
    \param[out] none
    \retval     none
*/
void sys_int_exit(void)
{
    // Threadx no longer need record this
    return;
}

/*!
    \brief      set rtos power save mode
    \param[in]  mode
    \param[out] none
    \retval     none
*/
void sys_ps_set(uint8_t mode)
{
    sys_ps_mode = mode;
}

/*!
    \brief      get rtos power save mode
    \param[in]  none
    \param[out] none
    \retval     current rtos power mode
*/
uint8_t sys_ps_get(void)
{
    return sys_ps_mode;
}

/*!
    \brief      get cpu sleep time and stats time
    \param[out] stats_ms
    \param[out] sleep_ms
    \retval     none
*/
void sys_cpu_sleep_time_get(uint32_t *stats_ms, uint32_t *sleep_ms)
{
    *stats_ms = 0;
    *sleep_ms = 0;
    return;
}

/*!
    \brief      show cpu usage percentage per task
    \retval     none
*/
void sys_cpu_stats(void)
{
}

/*!
    \brief      check task exist or not
    \param[in]  name: task name
    \param[out] none
    \retval     1: task exist, 0: task not exist
*/
uint8_t sys_task_exist(const uint8_t *name)
{
    uint16_t i = 0;
    TX_THREAD *thread_ptr;
    uint8_t found = 0;

    vWrapperTaskSuspendAll();
    thread_ptr = _tx_thread_created_ptr;
    for (i = 0; i < _tx_thread_created_count; i++) {
        if (name) {
            if ((strlen(name) + 1) < configMAX_TASK_NAME_LEN) {
                if (strcmp(name, thread_ptr->tx_thread_name) == 0) {
                    found = 1;
                    break;
                }
            }
            else {
                if (strncmp(name, thread_ptr->tx_thread_name, (configMAX_TASK_NAME_LEN - 1)) == 0) {
                    found = 1;
                    break;
                }
            }
        }

        thread_ptr = thread_ptr->tx_thread_created_next;
    }

    xWrapperTaskResumeAll();

    return found;
}


static void dump_byte_pool_block_list(TX_BYTE_POOL *pool_ptr)
{
    UCHAR       *current_ptr;
    UCHAR       *next_ptr;
    UCHAR       **this_block_link_ptr;
    UINT        examine_blocks;
    ALIGN_TYPE  *free_ptr;
    UCHAR       *work_ptr;
    int         count = 0;

    current_ptr = pool_ptr->tx_byte_pool_search;
    examine_blocks = pool_ptr->tx_byte_pool_fragments + ((UINT) 1);
    do {
        work_ptr =  TX_UCHAR_POINTER_ADD(current_ptr, (sizeof(UCHAR *)));
        free_ptr =  TX_UCHAR_TO_ALIGN_TYPE_POINTER_CONVERT(work_ptr);

        /* Pickup the next block's pointer.  */
        this_block_link_ptr =  TX_UCHAR_TO_INDIRECT_UCHAR_POINTER_CONVERT(current_ptr);
        next_ptr =             *this_block_link_ptr;

        if ((*free_ptr) == TX_BYTE_BLOCK_FREE) {
            printf("%s [%d]=%p, %d\r\n", pool_ptr->tx_byte_pool_name, count++, work_ptr, TX_UCHAR_POINTER_DIF(next_ptr, current_ptr));
        }

        current_ptr =  next_ptr;

        if (examine_blocks != ((UINT) 0))
        {

            examine_blocks--;
        }

    } while(examine_blocks != ((UINT) 0));
}

void dump_mem_block_list(void)
{
    dlist_t *pos, *n;
    add_byte_pool_t *p_byte_pool;

    sys_enter_critical();
    dump_byte_pool_block_list(&byte_pool);
    sys_exit_critical();

    sys_enter_critical();
    list_for_each_safe(pos, n, &added_byte_pools) {
        p_byte_pool = list_entry(pos, add_byte_pool_t, list);
        dump_byte_pool_block_list(&p_byte_pool->byte_pool);
    }
    sys_exit_critical();
}

static void tx_idle_task_entry(ULONG id)
{
    task_wrapper_t *p_task = NULL;
    UINT ret;

    for(;;) {
        sys_enter_critical();
        p_task = (task_wrapper_t *)co_list_pop_front(&threadx_idle_task.rmv_task_list);
        sys_exit_critical();


        if (p_task != NULL) {
             ret = tx_thread_delete(&p_task->tx_thread);
            if(ret != TX_SUCCESS) {
                configASSERT(0);
                continue;
            }

            if (p_task->p_stack) {
                sys_mfree(p_task->p_stack);
            }

            if (p_task->task_queue) {
                sys_queue_free(&p_task->task_queue);
            }

            tx_semaphore_delete(&p_task->notification_sem);

            sys_mfree(p_task);
        }
    }
}

void create_threadx_idle_task(void)
{
    co_list_init(&threadx_idle_task.rmv_task_list);

    if (tx_byte_allocate(&byte_pool, (VOID **) &threadx_idle_task.p_stack, TX_IDLE_TASK_STACK_SZIE, TX_NO_WAIT) != TX_SUCCESS) {
        configASSERT(0);
        return;
    }


    if (tx_thread_create(&threadx_idle_task.idle_thread, "idle task", tx_idle_task_entry, NULL, threadx_idle_task.p_stack, TX_IDLE_TASK_STACK_SZIE,
                    TX_MAX_PRIORITIES - 1u, TX_MAX_PRIORITIES - 1u, 0u, TX_AUTO_START) != TX_SUCCESS) {
        configASSERT(0);
        tx_byte_release(threadx_idle_task.p_stack);
        return;
    }

    cur_heap_mark += (TX_IDLE_TASK_STACK_SZIE + (sizeof(UCHAR *)) + (sizeof(ALIGN_TYPE)));
    high_heap_mark = cur_heap_mark;

}

