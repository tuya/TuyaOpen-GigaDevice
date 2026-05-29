/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "app_cfg.h"

#ifdef CONFIG_ALICLOUD_SUPPORT

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "iot_import.h"
#include "wrapper_os.h"

#define HAL_SEM_MAX_COUNT           (10)
#define HAL_SEM_INIT_COUNT          (0)

#define DEFAULT_THREAD_NAME         "linkkit_task"
#define DEFAULT_THREAD_SIZE         128//(1024)
#define DEFAULT_THREAD_PRORITY      (1)
#define TASK_STACK_ALIGN_SIZE       (4)

typedef void *Mutex_t;
typedef void *Semaphore_t;
/**
 * @brief Deallocate memory block
 *
 * @param[in] ptr @n Pointer to a memory block previously allocated with platform_malloc.
 * @return None.
 * @see None.
 * @note None.
 */
void HAL_Free(void *ptr)
{
    sys_mfree(ptr);
}

/**
 * @brief Allocates a block of size bytes of memory, returning a pointer to the beginning of the block.
 *
 * @param [in] size @n specify block size in bytes.
 * @return A pointer to the beginning of the block.
 * @see None.
 * @note Block value is indeterminate.
 */
void *HAL_Malloc(uint32_t size)
{
    return sys_malloc(size);
}

/**
 * @brief Create a mutex.
 *
 * @retval NULL : Initialize mutex failed.
 * @retval NOT_NULL : The mutex handle.
 * @see None.
 * @note None.
 */
void *HAL_MutexCreate(void)
{
    os_mutex_t mutex;

    sys_mutex_init(&mutex);

    return (void *)mutex;
}

/**
 * @brief Destroy the specified mutex object, it will release related resource.
 *
 * @param [in] mutex @n The specified mutex.
 * @return None.
 * @see None.
 * @note None.
 */
void HAL_MutexDestroy(void *mutex)
{
    os_mutex_t mtx = (os_mutex_t)mutex;

    if (NULL != mutex) {
        sys_mutex_free((os_mutex_t *)&mtx);
    }
}

/**
 * @brief Waits until the specified mutex is in the signaled state.
 *
 * @param [in] mutex @n the specified mutex.
 * @return None.
 * @see None.
 * @note None.
 */
void HAL_MutexLock(void *mutex)
{
    os_mutex_t mtx = (os_mutex_t)mutex;

    if (NULL != mutex) {
        sys_mutex_get((os_mutex_t *)&mtx);
    }
}

/**
 * @brief Releases ownership of the specified mutex object..
 *
 * @param [in] mutex @n the specified mutex.
 * @return None.
 * @see None.
 * @note None.
 */
void HAL_MutexUnlock(void *mutex)
{
    os_mutex_t mtx = (os_mutex_t)mutex;

    if (NULL != mutex) {
        sys_mutex_put((os_mutex_t *)&mtx);
    }
}

/**
 * @brief   create a semaphore
 *
 * @return semaphore handle.
 * @see None.
 * @note The recommended value of maximum count of the semaphore is 255.
 */
void *HAL_SemaphoreCreate(void)
{
    os_sema_t sem;

    if (0 != sys_sema_init(&sem, 0)) {
        return NULL;
    }

    return (void *)sem;
}

/**
 * @brief   destory a semaphore
 *
 * @param[in] sem @n the specified sem.
 * @return None.
 * @see None.
 * @note None.
 */
void HAL_SemaphoreDestroy(void *sem)
{
    os_sema_t sema = (os_sema_t)sem;

    if (NULL != sem) {
        sys_sema_free((os_sema_t *)&sema);
    }
}

/**
 * @brief   signal thread wait on a semaphore
 *
 * @param[in] sem @n the specified semaphore.
 * @return None.
 * @see None.
 * @note None.
 */
void HAL_SemaphorePost(void *sem)
{
    os_sema_t sema = (os_sema_t)sem;

    if (NULL != sem) {
        sys_sema_up((os_sema_t *)&sema);
    }
}

/**
 * @brief   wait on a semaphore
 *
 * @param[in] sem @n the specified semaphore.
 * @param[in] timeout_ms @n timeout interval in millisecond.
     If timeout_ms is PLATFORM_WAIT_INFINITE, the function will return only when the semaphore is signaled.
 * @return
   @verbatim
   =  0: The state of the specified object is signaled.
   =  -1: The time-out interval elapsed, and the object's state is nonsignaled.
   @endverbatim
 * @see None.
 * @note None.
 */
int HAL_SemaphoreWait(void *sem, uint32_t timeout_ms)
{
    os_sema_t sema = (os_sema_t)sem;

    if (NULL != sem) {
        if (PLATFORM_WAIT_INFINITE == timeout_ms) {
            return sys_sema_down((os_sema_t *)&sema, 0);
        } else {
            return sys_sema_down((os_sema_t *)&sema, timeout_ms);
        }
    }

    return -1;
}

/**
 * @brief Sleep thread itself.
 *
 * @param [in] ms @n the time interval for which execution is to be suspended, in milliseconds.
 * @return None.
 * @see None.
 * @note None.
 */
void HAL_SleepMs(uint32_t ms)
{
    sys_ms_sleep(ms);
}

/**
 * @brief  create a thread
 *
 * @param[out] thread_handle @n The new thread handle, memory allocated before thread created and return it, free it after thread joined or exit.
 * @param[in] start_routine @n A pointer to the application-defined function to be executed by the thread.
        This pointer represents the starting address of the thread.
 * @param[in] arg @n A pointer to a variable to be passed to the start_routine.
 * @param[in] hal_os_thread_param @n A pointer to stack params.
 * @param[out] stack_used @n if platform used stack buffer, set stack_used to 1, otherwise set it to 0.
 * @return
   @verbatim
     = 0: on success.
     = -1: error occur.
   @endverbatim
 * @see None.
 * @note None.
 */

typedef struct {
    void *arg;
    void *(*routine)(void *arg);
} task_context_t;

typedef void *(*task_routine_t)(void *argv);

static void task_function(void *arg)
{
    task_context_t *task_ctx = (task_context_t *)arg;
    void *task_arg = task_ctx->arg;
    task_routine_t task_routine = task_ctx->routine;

    sys_mfree(task_ctx);

    task_routine(task_arg);
}

int HAL_ThreadCreate(
    void **thread_handle,
    void *(*work_routine)(void *),
    void *arg,
    hal_os_thread_param_t *hal_os_thread_param,
    int *stack_used)
{
    int ret = 0;
    char *task_name;
    size_t stack_size;
    unsigned int task_pri;
    void *task_handle;
    task_context_t *task_ctx;

    if (stack_used != NULL) {
        *stack_used = 0;
    }

    if (!hal_os_thread_param) {
        task_pri = DEFAULT_THREAD_PRORITY;
    } else {
        task_pri = DEFAULT_THREAD_PRORITY + TASK_PRIO_HIGHER(hal_os_thread_param->priority);
    }
    if (!hal_os_thread_param || !hal_os_thread_param->name) {
        task_name = DEFAULT_THREAD_NAME;
    } else {
        task_name = hal_os_thread_param->name;
    }
    if (!hal_os_thread_param || hal_os_thread_param->stack_size == 0) {
        stack_size = DEFAULT_THREAD_SIZE;
    } else {
        stack_size = hal_os_thread_param->stack_size;
    }

    task_ctx = sys_malloc(sizeof(task_context_t));
    if (task_ctx == NULL) {
        return -1;
    }
    task_ctx->arg = arg;
    task_ctx->routine = work_routine;
    task_handle = sys_task_create(NULL, (const uint8_t *)task_name, NULL,
                    stack_size, 0, 0, OS_TASK_PRIORITY(task_pri), task_function, task_ctx);
    if (task_handle == NULL) {
        sys_mfree(task_ctx);
        return -1;
    }
    *thread_handle = task_handle;

    return ret;
}

static long long delta_time = 0;

void HAL_UTC_Set(long long ms)
{
    delta_time = ms - sys_time_get(NULL);
}

long long HAL_UTC_Get(void)
{
    return delta_time + sys_time_get(NULL);
}

void HAL_ThreadDetach(_IN_ void *thread_handle)
{
    printf("=>%s, TODO\r\n", __func__);
    (void)thread_handle;
}

void HAL_ThreadDelete(_IN_ void *thread_handle)
{
    sys_task_delete(thread_handle);
}

/**
 * @brief Retrieves the number of milliseconds that have elapsed since the system was boot.
 *
 * @return the number of milliseconds.
 * @see None.
 * @note None.
 */
uint64_t HAL_UptimeMs(void)
{
    return (uint64_t)sys_current_time_get();
}

#define ALI_MAX_DELAY (0xFFFFFFFFUL)
typedef void (*ali_timer_cb)(void *);

typedef struct ali_timer_context {
    os_timer_t tmr;
    void *usr_data;
    ali_timer_cb timer_func;
} ali_timer_context_t;

static void timer_callback_transfer(void *p_tmr, void *p_arg)
{
    ali_timer_cb timer_func;
    ali_timer_context_t *timer_ctx = (ali_timer_context_t *)p_arg;

    if (timer_ctx == NULL) {
        printf("timer_callback_transfer, p_arg is NULL, return\r\n");
        return;
    }

    timer_func = timer_ctx->timer_func;
    if (timer_func == NULL) {
        printf("timer_callback_transfer, timer_func is NULL, return\r\n");
        return;
    }

    timer_func(timer_ctx->usr_data);
}

void *HAL_Timer_Create_Ex(const char *name, void (*func)(void *), void *user_data, char repeat)
{
    ali_timer_context_t *ali_timer_wrapper = NULL;

    ali_timer_wrapper = (ali_timer_context_t *)sys_malloc(sizeof(ali_timer_context_t));
    if (ali_timer_wrapper == NULL) {
        printf("HAL_Timer_Create_Ex, malloc timer context failed\r\n");
        return NULL;
    }

    ali_timer_wrapper->usr_data = user_data;
    ali_timer_wrapper->timer_func = func;

    sys_timer_init(&ali_timer_wrapper->tmr, (const uint8_t *)name, ALI_MAX_DELAY, repeat, timer_callback_transfer, ali_timer_wrapper);
    return (void *)ali_timer_wrapper;
}

void *HAL_Timer_Create(const char *name, void (*func)(void *), void *user_data)
{
    ali_timer_context_t *ali_timer_wrapper = NULL;

    ali_timer_wrapper = (ali_timer_context_t *)sys_malloc(sizeof(ali_timer_context_t));
    if (ali_timer_wrapper == NULL) {
        printf("HAL_Timer_Create, malloc timer context failed\r\n");
        return NULL;
    }
    ali_timer_wrapper->usr_data = user_data;
    ali_timer_wrapper->timer_func = func;

    sys_timer_init(&ali_timer_wrapper->tmr, (const uint8_t *)name, ALI_MAX_DELAY, 0, timer_callback_transfer, ali_timer_wrapper);
    return (void *)ali_timer_wrapper;
}

int HAL_Timer_Delete(void *timer)
{
    ali_timer_context_t *timer_ctx = (ali_timer_context_t *)timer;

    if (!timer_ctx)
    {
        return FAIL_RETURN;
    }
    sys_timer_delete((os_timer_t *)&timer_ctx->tmr);

     if (timer_ctx != NULL) {
        sys_mfree(timer_ctx);
    }

    return SUCCESS_RETURN;
}

int HAL_Timer_Start(void *timer, int ms)
{
    ali_timer_context_t *timer_ctx = (ali_timer_context_t *)timer;

    if (!timer_ctx)
    {
        return FAIL_RETURN;
    }

    sys_timer_start_ext((os_timer_t *)&timer_ctx->tmr, ms, 0);

    return SUCCESS_RETURN;
}

int HAL_Timer_Stop(void *timer)
{
    ali_timer_context_t *timer_ctx = (ali_timer_context_t *)timer;

    if (!timer_ctx)
    {
        return FAIL_RETURN;
    }

    sys_timer_stop((os_timer_t *)&timer_ctx->tmr, 0);

    return SUCCESS_RETURN;
}

#ifdef REPORT_UUID_ENABLE
#define UUID_MAX_LEN (256)
int HAL_GetUUID(uint8_t *buf, int len)
{
    return -1;
}
#endif /* REPORT_UUID_ENABLE */
#endif /* CONFIG_ALICLOUD_SUPPORT */
