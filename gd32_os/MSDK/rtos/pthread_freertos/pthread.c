/**
 * @file pthread.c
 * @brief POSIX Threads (pthread) API wrapper implementation for FreeRTOS
 * @date 2026-01-28
 */

#include "pthread.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include <stdlib.h>
#include <string.h>

/* Default configuration */
#ifndef PTHREAD_DEFAULT_STACK_SIZE
#define PTHREAD_DEFAULT_STACK_SIZE    512//2048
#endif

#ifndef PTHREAD_DEFAULT_PRIORITY
#define PTHREAD_DEFAULT_PRIORITY      (tskIDLE_PRIORITY + 1)
#endif

/* Internal structures */
typedef struct {
    TaskHandle_t task_handle;
    void *(*start_routine)(void *);
    void *arg;
    void *retval;
    uint8_t detached;
    uint8_t joined;
    SemaphoreHandle_t join_sem;
} pthread_control_block_t;

typedef struct {
    SemaphoreHandle_t mutex;
    uint8_t type;
} pthread_mutex_control_block_t;

typedef struct {
    SemaphoreHandle_t sem;
    uint32_t waiting_count;
} pthread_cond_control_block_t;

typedef struct {
    SemaphoreHandle_t read_sem;   /* Semaphore for readers */
    SemaphoreHandle_t write_mutex; /* Mutex for writers */
    SemaphoreHandle_t resource_mutex; /* Mutex for updating reader count */
    uint32_t reader_count;
    TaskHandle_t write_owner;     /* Task that owns write lock (NULL if none) */
} pthread_rwlock_control_block_t;

typedef struct {
    SemaphoreHandle_t sem;
    uint32_t max_value;
} sem_control_block_t;

/* ============================================
 * Internal Helper Functions
 * ============================================ */

/**
 * @brief Thread wrapper function
 */
static void pthread_task_wrapper(void *arg)
{
    pthread_control_block_t *pcb = (pthread_control_block_t *)arg;

    if (pcb && pcb->start_routine) {
        pcb->retval = pcb->start_routine(pcb->arg);
    }

    /* Signal joining thread if any */
    if (pcb && pcb->join_sem) {
        xSemaphoreGive(pcb->join_sem);
    }

#if 0
    /* If detached, clean up immediately */
    if (pcb && pcb->detached) {
        if (pcb->join_sem) {
            vSemaphoreDelete(pcb->join_sem);
        }
        vPortFree(pcb);
    }
#else
    while (pcb && pcb->detached == 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    //printf("$pcb %p detached %d join_sema %p\r\n", pcb, pcb->detached, pcb->join_sem);
    if (pcb && pcb->detached) {
        if (pcb->join_sem) {
            vSemaphoreDelete(pcb->join_sem);
        }
        //printf("@free pcb %p\r\n", pcb);
        vPortFree(pcb);
    }
#endif
    vTaskDelete(NULL);
}

/* ============================================
 * Thread Management Functions
 * ============================================ */

int pthread_create(char *name, pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg)
{
    /* Parse attributes */
    uint32_t stack_size = PTHREAD_DEFAULT_STACK_SIZE;
    uint32_t priority = PTHREAD_DEFAULT_PRIORITY;
    uint8_t detach_state = PTHREAD_CREATE_JOINABLE;

    if (!thread || !start_routine) {
        return PTHREAD_EINVAL;
    }

    /* Allocate control block */
    pthread_control_block_t *pcb = pvPortMalloc(sizeof(pthread_control_block_t));
    if (!pcb) {
        return PTHREAD_ENOMEM;
    }

    memset(pcb, 0, sizeof(pthread_control_block_t));
    pcb->start_routine = start_routine;
    pcb->arg = arg;
    pcb->retval = NULL;

    if (attr && *attr) {
        struct pthread_attr *pattr = (struct pthread_attr *)(*attr);
        if (pattr->stack_size > 0) {
            stack_size = pattr->stack_size / sizeof(StackType_t);
        }
        priority = pattr->priority;
        detach_state = pattr->detach_state;
    }

    pcb->detached = (detach_state == PTHREAD_CREATE_DETACHED) ? 1 : 0;

    /* Create join semaphore for joinable threads */
    if (!pcb->detached) {
        pcb->join_sem = xSemaphoreCreateBinary();
        if (!pcb->join_sem) {
            vPortFree(pcb);
            return PTHREAD_ENOMEM;
        }
    }

    /* Create FreeRTOS task */
    BaseType_t ret = xTaskCreate(pthread_task_wrapper, name,
                                  stack_size, pcb, priority, &pcb->task_handle);
    //BaseType_t ret = xTaskCreate(pthread_task_wrapper, "pthread",
    //                              stack_size, pcb, priority, &pcb->task_handle);
    if (ret != pdPASS) {
        if (pcb->join_sem) {
            vSemaphoreDelete(pcb->join_sem);
        }
        vPortFree(pcb);
        return PTHREAD_ENOMEM;
    }

    *thread = (pthread_t)pcb;
    return PTHREAD_SUCCESS;
}

void pthread_exit(void *retval)
{
    TaskHandle_t task = xTaskGetCurrentTaskHandle();
    pthread_control_block_t *pcb = NULL;

    /* Find PCB for current task */
    /* Note: In a real implementation, you might maintain a list of PCBs */
    /* For simplicity, we store retval in task local storage or similar */

    vTaskDelete(task);

    /* This should never return */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int pthread_join(pthread_t thread, void **retval)
{
    if (!thread) {
        return PTHREAD_EINVAL;
    }

    pthread_control_block_t *pcb = (pthread_control_block_t *)thread;

    if (pcb->detached) {
        return PTHREAD_EINVAL;
    }

    if (pcb->joined) {
        return PTHREAD_EINVAL;
    }

    pcb->joined = 1;

    /* Wait for thread to complete */
    if (pcb->join_sem) {
        xSemaphoreTake(pcb->join_sem, portMAX_DELAY);

        if (retval) {
            *retval = pcb->retval;
        }
        /* Clean up */
#if 0
        vSemaphoreDelete(pcb->join_sem);
        vPortFree(pcb);
#else
        //printf("^free join_sem %p\r\n", pcb->join_sem);
        vSemaphoreDelete(pcb->join_sem);
        pcb->join_sem = NULL;
        pcb->detached = 1;
#endif
    }

    return PTHREAD_SUCCESS;
}

int pthread_detach(pthread_t thread)
{
    if (!thread) {
        return PTHREAD_EINVAL;
    }

    pthread_control_block_t *pcb = (pthread_control_block_t *)thread;

    if (pcb->detached) {
        return PTHREAD_EINVAL;
    }

    pcb->detached = 1;

    /* If thread already finished, clean up now */
    if (pcb->join_sem) {
        if (uxSemaphoreGetCount(pcb->join_sem) > 0) {
            vSemaphoreDelete(pcb->join_sem);
            vPortFree(pcb);
        }
    }

    return PTHREAD_SUCCESS;
}

pthread_t pthread_self(void)
{
    return (pthread_t)xTaskGetCurrentTaskHandle();
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
    return (t1 == t2) ? 1 : 0;
}

/* ============================================
 * Thread Attribute Functions
 * ============================================ */

int pthread_attr_init(pthread_attr_t *attr)
{
    if (!attr) {
        return PTHREAD_EINVAL;
    }

    struct pthread_attr *pattr = pvPortMalloc(sizeof(struct pthread_attr));
    if (!pattr) {
        return PTHREAD_ENOMEM;
    }

    pattr->stack_size = PTHREAD_DEFAULT_STACK_SIZE;
    pattr->priority = PTHREAD_DEFAULT_PRIORITY;
    pattr->detach_state = PTHREAD_CREATE_JOINABLE;
    pattr->stack_addr = NULL;

    *attr = (pthread_attr_t)pattr;
    return PTHREAD_SUCCESS;
}

int pthread_attr_destroy(pthread_attr_t *attr)
{
    if (!attr || !(*attr)) {
        return PTHREAD_EINVAL;
    }

    vPortFree(*attr);
    *attr = NULL;
    return PTHREAD_SUCCESS;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
    if (!attr || !(*attr)) {
        return PTHREAD_EINVAL;
    }

    struct pthread_attr *pattr = (struct pthread_attr *)(*attr);
    pattr->stack_size = stacksize;
    return PTHREAD_SUCCESS;
}

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize)
{
    if (!attr || !(*attr) || !stacksize) {
        return PTHREAD_EINVAL;
    }

    struct pthread_attr *pattr = (struct pthread_attr *)(*attr);
    *stacksize = pattr->stack_size;
    return PTHREAD_SUCCESS;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{
    if (!attr || !(*attr)) {
        return PTHREAD_EINVAL;
    }

    if (detachstate != PTHREAD_CREATE_JOINABLE &&
        detachstate != PTHREAD_CREATE_DETACHED) {
        return PTHREAD_EINVAL;
    }

    struct pthread_attr *pattr = (struct pthread_attr *)(*attr);
    pattr->detach_state = detachstate;
    return PTHREAD_SUCCESS;
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate)
{
    if (!attr || !(*attr) || !detachstate) {
        return PTHREAD_EINVAL;
    }

    struct pthread_attr *pattr = (struct pthread_attr *)(*attr);
    *detachstate = pattr->detach_state;
    return PTHREAD_SUCCESS;
}

/* ============================================
 * Mutex Functions
 * ============================================ */

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    if (!mutex) {
        return PTHREAD_EINVAL;
    }

    pthread_mutex_control_block_t *mcb = pvPortMalloc(sizeof(pthread_mutex_control_block_t));
    if (!mcb) {
        return PTHREAD_ENOMEM;
    }

    /* Parse attributes */
    uint8_t type = PTHREAD_MUTEX_NORMAL;
    if (attr && *attr) {
        struct pthread_mutexattr *mattr = (struct pthread_mutexattr *)(*attr);
        type = mattr->type;
    }

    mcb->type = type;

    /* Create FreeRTOS mutex */
    if (type == PTHREAD_MUTEX_RECURSIVE) {
        mcb->mutex = xSemaphoreCreateRecursiveMutex();
    } else {
        mcb->mutex = xSemaphoreCreateMutex();
    }

    if (!mcb->mutex) {
        vPortFree(mcb);
        return PTHREAD_ENOMEM;
    }

    *mutex = (pthread_mutex_t)mcb;
    return PTHREAD_SUCCESS;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    if (!mutex || !(*mutex)) {
        return PTHREAD_EINVAL;
    }

    pthread_mutex_control_block_t *mcb = (pthread_mutex_control_block_t *)(*mutex);

    if (mcb->mutex) {
        vSemaphoreDelete(mcb->mutex);
    }

    vPortFree(mcb);
    *mutex = NULL;
    return PTHREAD_SUCCESS;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    if (!mutex || !(*mutex)) {
        return PTHREAD_EINVAL;
    }

    pthread_mutex_control_block_t *mcb = (pthread_mutex_control_block_t *)(*mutex);

    BaseType_t ret;
    if (mcb->type == PTHREAD_MUTEX_RECURSIVE) {
        ret = xSemaphoreTakeRecursive(mcb->mutex, portMAX_DELAY);
    } else {
        ret = xSemaphoreTake(mcb->mutex, portMAX_DELAY);
    }

    return (ret == pdTRUE) ? PTHREAD_SUCCESS : PTHREAD_EINVAL;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    if (!mutex || !(*mutex)) {
        return PTHREAD_EINVAL;
    }

    pthread_mutex_control_block_t *mcb = (pthread_mutex_control_block_t *)(*mutex);

    BaseType_t ret;
    if (mcb->type == PTHREAD_MUTEX_RECURSIVE) {
        ret = xSemaphoreTakeRecursive(mcb->mutex, 0);
    } else {
        ret = xSemaphoreTake(mcb->mutex, 0);
    }

    return (ret == pdTRUE) ? PTHREAD_SUCCESS : PTHREAD_EBUSY;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    if (!mutex || !(*mutex)) {
        return PTHREAD_EINVAL;
    }

    pthread_mutex_control_block_t *mcb = (pthread_mutex_control_block_t *)(*mutex);

    BaseType_t ret;
    if (mcb->type == PTHREAD_MUTEX_RECURSIVE) {
        ret = xSemaphoreGiveRecursive(mcb->mutex);
    } else {
        ret = xSemaphoreGive(mcb->mutex);
    }

    return (ret == pdTRUE) ? PTHREAD_SUCCESS : PTHREAD_EINVAL;
}

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
    if (!attr) {
        return PTHREAD_EINVAL;
    }

    struct pthread_mutexattr *mattr = pvPortMalloc(sizeof(struct pthread_mutexattr));
    if (!mattr) {
        return PTHREAD_ENOMEM;
    }

    mattr->type = PTHREAD_MUTEX_DEFAULT;
    *attr = (pthread_mutexattr_t)mattr;
    return PTHREAD_SUCCESS;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
    if (!attr || !(*attr)) {
        return PTHREAD_EINVAL;
    }

    vPortFree(*attr);
    *attr = NULL;
    return PTHREAD_SUCCESS;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
    if (!attr || !(*attr)) {
        return PTHREAD_EINVAL;
    }

    if (type != PTHREAD_MUTEX_NORMAL &&
        type != PTHREAD_MUTEX_RECURSIVE &&
        type != PTHREAD_MUTEX_ERRORCHECK) {
        return PTHREAD_EINVAL;
    }

    struct pthread_mutexattr *mattr = (struct pthread_mutexattr *)(*attr);
    mattr->type = type;
    return PTHREAD_SUCCESS;
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type)
{
    if (!attr || !(*attr) || !type) {
        return PTHREAD_EINVAL;
    }

    struct pthread_mutexattr *mattr = (struct pthread_mutexattr *)(*attr);
    *type = mattr->type;
    return PTHREAD_SUCCESS;
}

/* ============================================
 * Condition Variable Functions
 * ============================================ */

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    if (!cond) {
        return PTHREAD_EINVAL;
    }

    pthread_cond_control_block_t *ccb = pvPortMalloc(sizeof(pthread_cond_control_block_t));
    if (!ccb) {
        return PTHREAD_ENOMEM;
    }

    ccb->sem = xSemaphoreCreateCounting(0xFFFFFFFF, 0);
    if (!ccb->sem) {
        vPortFree(ccb);
        return PTHREAD_ENOMEM;
    }

    ccb->waiting_count = 0;
    *cond = (pthread_cond_t)ccb;
    return PTHREAD_SUCCESS;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
    if (!cond || !(*cond)) {
        return PTHREAD_EINVAL;
    }

    pthread_cond_control_block_t *ccb = (pthread_cond_control_block_t *)(*cond);

    if (ccb->sem) {
        vSemaphoreDelete(ccb->sem);
    }

    vPortFree(ccb);
    *cond = NULL;
    return PTHREAD_SUCCESS;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    if (!cond || !(*cond) || !mutex || !(*mutex)) {
        return PTHREAD_EINVAL;
    }

    pthread_cond_control_block_t *ccb = (pthread_cond_control_block_t *)(*cond);
    pthread_mutex_control_block_t *mcb = (pthread_mutex_control_block_t *)(*mutex);

    ccb->waiting_count++;

    /* Unlock mutex */
    pthread_mutex_unlock(mutex);

    /* Wait on semaphore */
    xSemaphoreTake(ccb->sem, portMAX_DELAY);

    ccb->waiting_count--;

    /* Re-lock mutex */
    pthread_mutex_lock(mutex);

    return PTHREAD_SUCCESS;
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                           const struct timespec *abstime)
{
    if (!cond || !(*cond) || !mutex || !(*mutex) || !abstime) {
        return PTHREAD_EINVAL;
    }

    pthread_cond_control_block_t *ccb = (pthread_cond_control_block_t *)(*cond);
    pthread_mutex_control_block_t *mcb = (pthread_mutex_control_block_t *)(*mutex);

    /* Calculate timeout in ticks */
    /* Note: This is a simplified implementation */
    TickType_t timeout_ticks = pdMS_TO_TICKS(abstime->tv_sec * 1000 + abstime->tv_nsec / 1000000);

    ccb->waiting_count++;

    /* Unlock mutex */
    pthread_mutex_unlock(mutex);

    /* Wait on semaphore with timeout */
    BaseType_t ret = xSemaphoreTake(ccb->sem, timeout_ticks);

    ccb->waiting_count--;

    /* Re-lock mutex */
    pthread_mutex_lock(mutex);

    return (ret == pdTRUE) ? PTHREAD_SUCCESS : PTHREAD_ETIMEDOUT;
}

int pthread_cond_signal(pthread_cond_t *cond)
{
    if (!cond || !(*cond)) {
        return PTHREAD_EINVAL;
    }

    pthread_cond_control_block_t *ccb = (pthread_cond_control_block_t *)(*cond);

    if (ccb->waiting_count > 0) {
        xSemaphoreGive(ccb->sem);
    }

    return PTHREAD_SUCCESS;
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
    if (!cond || !(*cond)) {
        return PTHREAD_EINVAL;
    }

    pthread_cond_control_block_t *ccb = (pthread_cond_control_block_t *)(*cond);

    uint32_t count = ccb->waiting_count;
    for (uint32_t i = 0; i < count; i++) {
        xSemaphoreGive(ccb->sem);
    }

    return PTHREAD_SUCCESS;
}

int pthread_condattr_init(pthread_condattr_t *attr)
{
    if (!attr) {
        return PTHREAD_EINVAL;
    }

    struct pthread_condattr *cattr = pvPortMalloc(sizeof(struct pthread_condattr));
    if (!cattr) {
        return PTHREAD_ENOMEM;
    }

    cattr->pshared = 0;
    *attr = (pthread_condattr_t)cattr;
    return PTHREAD_SUCCESS;
}

int pthread_condattr_destroy(pthread_condattr_t *attr)
{
    if (!attr || !(*attr)) {
        return PTHREAD_EINVAL;
    }

    vPortFree(*attr);
    *attr = NULL;
    return PTHREAD_SUCCESS;
}

/* ============================================
 * Read-Write Lock Functions
 * ============================================ */

int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr)
{
    if (!rwlock) {
        return PTHREAD_EINVAL;
    }

    pthread_rwlock_control_block_t *rcb = pvPortMalloc(sizeof(pthread_rwlock_control_block_t));
    if (!rcb) {
        return PTHREAD_ENOMEM;
    }

    memset(rcb, 0, sizeof(pthread_rwlock_control_block_t));

    /* Create semaphore for read access (initially available) */
    rcb->read_sem = xSemaphoreCreateBinary();
    if (!rcb->read_sem) {
        vPortFree(rcb);
        return PTHREAD_ENOMEM;
    }
    xSemaphoreGive(rcb->read_sem);

    /* Create mutex for write access */
    rcb->write_mutex = xSemaphoreCreateMutex();
    if (!rcb->write_mutex) {
        vSemaphoreDelete(rcb->read_sem);
        vPortFree(rcb);
        return PTHREAD_ENOMEM;
    }

    /* Create mutex for protecting reader count */
    rcb->resource_mutex = xSemaphoreCreateMutex();
    if (!rcb->resource_mutex) {
        vSemaphoreDelete(rcb->read_sem);
        vSemaphoreDelete(rcb->write_mutex);
        vPortFree(rcb);
        return PTHREAD_ENOMEM;
    }

    rcb->reader_count = 0;
    rcb->write_owner = NULL;
    *rwlock = (pthread_rwlock_t)rcb;
    return PTHREAD_SUCCESS;
}

int pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
    if (!rwlock || !(*rwlock)) {
        return PTHREAD_EINVAL;
    }

    pthread_rwlock_control_block_t *rcb = (pthread_rwlock_control_block_t *)(*rwlock);

    if (rcb->read_sem) {
        vSemaphoreDelete(rcb->read_sem);
    }
    if (rcb->write_mutex) {
        vSemaphoreDelete(rcb->write_mutex);
    }
    if (rcb->resource_mutex) {
        vSemaphoreDelete(rcb->resource_mutex);
    }

    vPortFree(rcb);
    *rwlock = NULL;
    return PTHREAD_SUCCESS;
}

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
    if (!rwlock || !(*rwlock)) {
        return PTHREAD_EINVAL;
    }

    pthread_rwlock_control_block_t *rcb = (pthread_rwlock_control_block_t *)(*rwlock);

    /* Protect reader count update */
    xSemaphoreTake(rcb->resource_mutex, portMAX_DELAY);

    /* First reader blocks writers */
    if (rcb->reader_count == 0) {
        xSemaphoreTake(rcb->read_sem, portMAX_DELAY);
    }

    rcb->reader_count++;
    xSemaphoreGive(rcb->resource_mutex);

    return PTHREAD_SUCCESS;
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
    if (!rwlock || !(*rwlock)) {
        return PTHREAD_EINVAL;
    }

    pthread_rwlock_control_block_t *rcb = (pthread_rwlock_control_block_t *)(*rwlock);

    /* Try to protect reader count update */
    if (xSemaphoreTake(rcb->resource_mutex, 0) != pdTRUE) {
        return PTHREAD_EBUSY;
    }

    /* First reader tries to block writers */
    if (rcb->reader_count == 0) {
        if (xSemaphoreTake(rcb->read_sem, 0) != pdTRUE) {
            xSemaphoreGive(rcb->resource_mutex);
            return PTHREAD_EBUSY;
        }
    }

    rcb->reader_count++;
    xSemaphoreGive(rcb->resource_mutex);

    return PTHREAD_SUCCESS;
}

int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
    if (!rwlock || !(*rwlock)) {
        return PTHREAD_EINVAL;
    }

    pthread_rwlock_control_block_t *rcb = (pthread_rwlock_control_block_t *)(*rwlock);

    /* Acquire write mutex to ensure exclusive write access */
    xSemaphoreTake(rcb->write_mutex, portMAX_DELAY);

    /* Wait for all readers to finish */
    xSemaphoreTake(rcb->read_sem, portMAX_DELAY);

    /* Record the write lock owner */
    rcb->write_owner = xTaskGetCurrentTaskHandle();

    return PTHREAD_SUCCESS;
}

int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock)
{
    if (!rwlock || !(*rwlock)) {
        return PTHREAD_EINVAL;
    }

    pthread_rwlock_control_block_t *rcb = (pthread_rwlock_control_block_t *)(*rwlock);

    /* Try to acquire write mutex */
    if (xSemaphoreTake(rcb->write_mutex, 0) != pdTRUE) {
        return PTHREAD_EBUSY;
    }

    /* Try to wait for all readers to finish */
    if (xSemaphoreTake(rcb->read_sem, 0) != pdTRUE) {
        xSemaphoreGive(rcb->write_mutex);
        return PTHREAD_EBUSY;
    }

    /* Record the write lock owner */
    rcb->write_owner = xTaskGetCurrentTaskHandle();

    return PTHREAD_SUCCESS;
}

int pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
    if (!rwlock || !(*rwlock)) {
        return PTHREAD_EINVAL;
    }

    pthread_rwlock_control_block_t *rcb = (pthread_rwlock_control_block_t *)(*rwlock);
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();

    /* Check if this is a write lock by comparing task handle */
    if (rcb->write_owner == current_task) {
        /* This was a write lock */
        rcb->write_owner = NULL;
        xSemaphoreGive(rcb->read_sem);
        xSemaphoreGive(rcb->write_mutex);
        return PTHREAD_SUCCESS;
    }

    /* This was a read lock */
    xSemaphoreTake(rcb->resource_mutex, portMAX_DELAY);

    if (rcb->reader_count > 0) {
        rcb->reader_count--;

        /* Last reader unblocks writers */
        if (rcb->reader_count == 0) {
            xSemaphoreGive(rcb->read_sem);
        }
    }

    xSemaphoreGive(rcb->resource_mutex);

    return PTHREAD_SUCCESS;
}

int pthread_rwlockattr_init(pthread_rwlockattr_t *attr)
{
    if (!attr) {
        return PTHREAD_EINVAL;
    }

    struct pthread_rwlockattr *rattr = pvPortMalloc(sizeof(struct pthread_rwlockattr));
    if (!rattr) {
        return PTHREAD_ENOMEM;
    }

    rattr->pshared = 0;
    *attr = (pthread_rwlockattr_t)rattr;
    return PTHREAD_SUCCESS;
}

int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr)
{
    if (!attr || !(*attr)) {
        return PTHREAD_EINVAL;
    }

    vPortFree(*attr);
    *attr = NULL;
    return PTHREAD_SUCCESS;
}

/* ============================================
 * Semaphore Functions
 * ============================================ */

int sem_init(sem_t *sem, int pshared, unsigned int value)
{
    if (!sem) {
        return PTHREAD_EINVAL;
    }

    /* FreeRTOS doesn't support process-shared semaphores */
    if (pshared != 0) {
        return PTHREAD_EINVAL;
    }

    sem_control_block_t *scb = pvPortMalloc(sizeof(sem_control_block_t));
    if (!scb) {
        return PTHREAD_ENOMEM;
    }

    /* Create a counting semaphore */
    scb->max_value = 0xFFFFFFFF;
    scb->sem = xSemaphoreCreateCounting(scb->max_value, value);

    if (!scb->sem) {
        vPortFree(scb);
        return PTHREAD_ENOMEM;
    }

    *sem = (sem_t)scb;
    return PTHREAD_SUCCESS;
}

int sem_destroy(sem_t *sem)
{
    if (!sem || !(*sem)) {
        return PTHREAD_EINVAL;
    }

    sem_control_block_t *scb = (sem_control_block_t *)(*sem);

    if (scb->sem) {
        vSemaphoreDelete(scb->sem);
    }

    vPortFree(scb);
    *sem = NULL;
    return PTHREAD_SUCCESS;
}

int sem_wait(sem_t *sem)
{
    if (!sem || !(*sem)) {
        return PTHREAD_EINVAL;
    }

    sem_control_block_t *scb = (sem_control_block_t *)(*sem);

    BaseType_t ret = xSemaphoreTake(scb->sem, portMAX_DELAY);
    return (ret == pdTRUE) ? PTHREAD_SUCCESS : PTHREAD_EINVAL;
}

int sem_trywait(sem_t *sem)
{
    if (!sem || !(*sem)) {
        return PTHREAD_EINVAL;
    }

    sem_control_block_t *scb = (sem_control_block_t *)(*sem);

    BaseType_t ret = xSemaphoreTake(scb->sem, 0);
    return (ret == pdTRUE) ? PTHREAD_SUCCESS : PTHREAD_EAGAIN;
}

int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout)
{
    if (!sem || !(*sem) || !abs_timeout) {
        return PTHREAD_EINVAL;
    }

    sem_control_block_t *scb = (sem_control_block_t *)(*sem);

    /* Calculate timeout in ticks */
    /* Note: This is a simplified implementation using absolute timeout */
    TickType_t timeout_ticks = pdMS_TO_TICKS(abs_timeout->tv_sec * 1000 + abs_timeout->tv_nsec / 1000000);

    BaseType_t ret = xSemaphoreTake(scb->sem, timeout_ticks);
    return (ret == pdTRUE) ? PTHREAD_SUCCESS : PTHREAD_ETIMEDOUT;
}

int sem_post(sem_t *sem)
{
    if (!sem || !(*sem)) {
        return PTHREAD_EINVAL;
    }

    sem_control_block_t *scb = (sem_control_block_t *)(*sem);

    BaseType_t ret = xSemaphoreGive(scb->sem);
    return (ret == pdTRUE) ? PTHREAD_SUCCESS : PTHREAD_EOVERFLOW;
}

int sem_getvalue(sem_t *sem, int *sval)
{
    if (!sem || !(*sem) || !sval) {
        return PTHREAD_EINVAL;
    }

    sem_control_block_t *scb = (sem_control_block_t *)(*sem);

    *sval = (int)uxSemaphoreGetCount(scb->sem);
    return PTHREAD_SUCCESS;
}
