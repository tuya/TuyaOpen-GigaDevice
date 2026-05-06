/**
 * @file pthread.h
 * @brief POSIX Threads (pthread) API wrapper for FreeRTOS
 * @date 2026-01-28
 */

#ifndef __PTHREAD_H__
#define __PTHREAD_H__

#include <stdint.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Return values */
#define PTHREAD_SUCCESS              0
#define PTHREAD_EINVAL              22  /* Invalid argument */
#define PTHREAD_ENOMEM              12  /* Out of memory */
#define PTHREAD_EBUSY               16  /* Device or resource busy */
#define PTHREAD_ETIMEDOUT          110  /* Connection timed out */
#define PTHREAD_EDEADLK             35  /* Resource deadlock avoided */
#define PTHREAD_EAGAIN              11  /* Try again */
#define PTHREAD_EOVERFLOW           75  /* Value too large for defined data type */

/* Thread states */
#define PTHREAD_CREATE_JOINABLE      0
#define PTHREAD_CREATE_DETACHED      1

/* Mutex types */
#define PTHREAD_MUTEX_NORMAL         0
#define PTHREAD_MUTEX_RECURSIVE      1
#define PTHREAD_MUTEX_ERRORCHECK     2
#define PTHREAD_MUTEX_DEFAULT        PTHREAD_MUTEX_NORMAL

/* Pthread types */
typedef void* pthread_t;
typedef void* pthread_mutex_t;
typedef void* pthread_cond_t;
typedef void* pthread_rwlock_t;
typedef void* pthread_attr_t;
typedef void* pthread_mutexattr_t;
typedef void* pthread_condattr_t;
typedef void* pthread_rwlockattr_t;
typedef void* sem_t;

/* Thread attribute structure */
struct pthread_attr {
    uint32_t stack_size;
    uint32_t priority;
    uint8_t detach_state;
    void *stack_addr;
};

/* Mutex attribute structure */
struct pthread_mutexattr {
    uint8_t type;
};

/* Condition variable attribute structure */
struct pthread_condattr {
    uint8_t pshared;
};

/* Read-write lock attribute structure */
struct pthread_rwlockattr {
    uint8_t pshared;
};

/* ============================================
 * Thread Management Functions
 * ============================================ */

/**
 * @brief Create a new thread
 * @param thread Pointer to store thread handle
 * @param attr Thread attributes (NULL for default)
 * @param start_routine Thread function
 * @param arg Argument to pass to thread function
 * @return 0 on success, error code otherwise
 */
int pthread_create(char *name, pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);

/**
 * @brief Exit the calling thread
 * @param retval Return value
 */
void pthread_exit(void *retval);

/**
 * @brief Wait for thread to terminate
 * @param thread Thread to wait for
 * @param retval Pointer to store return value
 * @return 0 on success, error code otherwise
 */
int pthread_join(pthread_t thread, void **retval);

/**
 * @brief Detach a thread
 * @param thread Thread to detach
 * @return 0 on success, error code otherwise
 */
int pthread_detach(pthread_t thread);

/**
 * @brief Get current thread handle
 * @return Current thread handle
 */
pthread_t pthread_self(void);

/**
 * @brief Compare two thread IDs
 * @param t1 First thread
 * @param t2 Second thread
 * @return Non-zero if equal, 0 otherwise
 */
int pthread_equal(pthread_t t1, pthread_t t2);

/* ============================================
 * Thread Attribute Functions
 * ============================================ */

/**
 * @brief Initialize thread attributes
 * @param attr Attribute structure to initialize
 * @return 0 on success, error code otherwise
 */
int pthread_attr_init(pthread_attr_t *attr);

/**
 * @brief Destroy thread attributes
 * @param attr Attribute structure to destroy
 * @return 0 on success, error code otherwise
 */
int pthread_attr_destroy(pthread_attr_t *attr);

/**
 * @brief Set stack size in thread attributes
 * @param attr Attribute structure
 * @param stacksize Stack size in bytes
 * @return 0 on success, error code otherwise
 */
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);

/**
 * @brief Get stack size from thread attributes
 * @param attr Attribute structure
 * @param stacksize Pointer to store stack size
 * @return 0 on success, error code otherwise
 */
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize);

/**
 * @brief Set detach state in thread attributes
 * @param attr Attribute structure
 * @param detachstate PTHREAD_CREATE_JOINABLE or PTHREAD_CREATE_DETACHED
 * @return 0 on success, error code otherwise
 */
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);

/**
 * @brief Get detach state from thread attributes
 * @param attr Attribute structure
 * @param detachstate Pointer to store detach state
 * @return 0 on success, error code otherwise
 */
int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate);

/* ============================================
 * Mutex Functions
 * ============================================ */

/**
 * @brief Initialize a mutex
 * @param mutex Pointer to mutex
 * @param attr Mutex attributes (NULL for default)
 * @return 0 on success, error code otherwise
 */
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);

/**
 * @brief Destroy a mutex
 * @param mutex Pointer to mutex
 * @return 0 on success, error code otherwise
 */
int pthread_mutex_destroy(pthread_mutex_t *mutex);

/**
 * @brief Lock a mutex
 * @param mutex Pointer to mutex
 * @return 0 on success, error code otherwise
 */
int pthread_mutex_lock(pthread_mutex_t *mutex);

/**
 * @brief Try to lock a mutex without blocking
 * @param mutex Pointer to mutex
 * @return 0 on success, PTHREAD_EBUSY if already locked, error code otherwise
 */
int pthread_mutex_trylock(pthread_mutex_t *mutex);

/**
 * @brief Unlock a mutex
 * @param mutex Pointer to mutex
 * @return 0 on success, error code otherwise
 */
int pthread_mutex_unlock(pthread_mutex_t *mutex);

/**
 * @brief Initialize mutex attributes
 * @param attr Attribute structure to initialize
 * @return 0 on success, error code otherwise
 */
int pthread_mutexattr_init(pthread_mutexattr_t *attr);

/**
 * @brief Destroy mutex attributes
 * @param attr Attribute structure to destroy
 * @return 0 on success, error code otherwise
 */
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);

/**
 * @brief Set mutex type in attributes
 * @param attr Attribute structure
 * @param type Mutex type (PTHREAD_MUTEX_NORMAL, PTHREAD_MUTEX_RECURSIVE, etc.)
 * @return 0 on success, error code otherwise
 */
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);

/**
 * @brief Get mutex type from attributes
 * @param attr Attribute structure
 * @param type Pointer to store mutex type
 * @return 0 on success, error code otherwise
 */
int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type);

/* ============================================
 * Condition Variable Functions
 * ============================================ */

/**
 * @brief Initialize a condition variable
 * @param cond Pointer to condition variable
 * @param attr Condition variable attributes (NULL for default)
 * @return 0 on success, error code otherwise
 */
int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);

/**
 * @brief Destroy a condition variable
 * @param cond Pointer to condition variable
 * @return 0 on success, error code otherwise
 */
int pthread_cond_destroy(pthread_cond_t *cond);

/**
 * @brief Wait on a condition variable
 * @param cond Pointer to condition variable
 * @param mutex Pointer to associated mutex
 * @return 0 on success, error code otherwise
 */
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);

/**
 * @brief Wait on a condition variable with timeout
 * @param cond Pointer to condition variable
 * @param mutex Pointer to associated mutex
 * @param abstime Absolute timeout time
 * @return 0 on success, PTHREAD_ETIMEDOUT if timed out, error code otherwise
 */
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                           const struct timespec *abstime);

/**
 * @brief Signal a condition variable (wake one waiting thread)
 * @param cond Pointer to condition variable
 * @return 0 on success, error code otherwise
 */
int pthread_cond_signal(pthread_cond_t *cond);

/**
 * @brief Broadcast a condition variable (wake all waiting threads)
 * @param cond Pointer to condition variable
 * @return 0 on success, error code otherwise
 */
int pthread_cond_broadcast(pthread_cond_t *cond);

/**
 * @brief Initialize condition variable attributes
 * @param attr Attribute structure to initialize
 * @return 0 on success, error code otherwise
 */
int pthread_condattr_init(pthread_condattr_t *attr);

/**
 * @brief Destroy condition variable attributes
 * @param attr Attribute structure to destroy
 * @return 0 on success, error code otherwise
 */
int pthread_condattr_destroy(pthread_condattr_t *attr);

/* ============================================
 * Read-Write Lock Functions
 * ============================================ */

/**
 * @brief Initialize a read-write lock
 * @param rwlock Pointer to read-write lock
 * @param attr Read-write lock attributes (NULL for default)
 * @return 0 on success, error code otherwise
 */
int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr);

/**
 * @brief Destroy a read-write lock
 * @param rwlock Pointer to read-write lock
 * @return 0 on success, error code otherwise
 */
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);

/**
 * @brief Acquire a read lock
 * @param rwlock Pointer to read-write lock
 * @return 0 on success, error code otherwise
 */
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);

/**
 * @brief Try to acquire a read lock without blocking
 * @param rwlock Pointer to read-write lock
 * @return 0 on success, PTHREAD_EBUSY if lock is held by writer, error code otherwise
 */
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);

/**
 * @brief Acquire a write lock
 * @param rwlock Pointer to read-write lock
 * @return 0 on success, error code otherwise
 */
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);

/**
 * @brief Try to acquire a write lock without blocking
 * @param rwlock Pointer to read-write lock
 * @return 0 on success, PTHREAD_EBUSY if lock is held, error code otherwise
 */
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);

/**
 * @brief Unlock a read-write lock
 * @param rwlock Pointer to read-write lock
 * @return 0 on success, error code otherwise
 */
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);

/**
 * @brief Initialize read-write lock attributes
 * @param attr Attribute structure to initialize
 * @return 0 on success, error code otherwise
 */
int pthread_rwlockattr_init(pthread_rwlockattr_t *attr);

/**
 * @brief Destroy read-write lock attributes
 * @param attr Attribute structure to destroy
 * @return 0 on success, error code otherwise
 */
int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr);

/* ============================================
 * Semaphore Functions
 * ============================================ */

/**
 * @brief Initialize an unnamed semaphore
 * @param sem Pointer to semaphore
 * @param pshared Process sharing flag (0 for thread-local, non-zero for process-shared)
 * @param value Initial value of semaphore
 * @return 0 on success, error code otherwise
 */
int sem_init(sem_t *sem, int pshared, unsigned int value);

/**
 * @brief Destroy a semaphore
 * @param sem Pointer to semaphore
 * @return 0 on success, error code otherwise
 */
int sem_destroy(sem_t *sem);

/**
 * @brief Wait on a semaphore (blocking)
 * @param sem Pointer to semaphore
 * @return 0 on success, error code otherwise
 */
int sem_wait(sem_t *sem);

/**
 * @brief Try to wait on a semaphore without blocking
 * @param sem Pointer to semaphore
 * @return 0 on success, PTHREAD_EAGAIN if would block, error code otherwise
 */
int sem_trywait(sem_t *sem);

/**
 * @brief Wait on a semaphore with absolute timeout
 * @param sem Pointer to semaphore
 * @param abs_timeout Absolute timeout time
 * @return 0 on success, PTHREAD_ETIMEDOUT if timed out, error code otherwise
 */
int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);

/**
 * @brief Post (increment) a semaphore
 * @param sem Pointer to semaphore
 * @return 0 on success, error code otherwise
 */
int sem_post(sem_t *sem);

/**
 * @brief Get current value of a semaphore
 * @param sem Pointer to semaphore
 * @param sval Pointer to store current value
 * @return 0 on success, error code otherwise
 */
int sem_getvalue(sem_t *sem, int *sval);

#ifdef __cplusplus
}
#endif

#endif /* __PTHREAD_H__ */
