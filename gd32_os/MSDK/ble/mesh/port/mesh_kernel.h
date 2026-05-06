/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MESH_INCLUDE_BT_MESH_KERNEL_H__
#define MESH_INCLUDE_BT_MESH_KERNEL_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <sys/types.h>
#include "sys/slist.h"
#include "mesh_errno.h"


typedef uint32_t k_ticks_t;

typedef struct
{
    k_ticks_t ticks;
} k_timeout_t;

#define SYS_FOREVER_MS (-1)

#define K_NO_WAIT ((k_timeout_t) {0})
#define K_FOREVER ((k_timeout_t) {-1})


#define MS_PER_TICKS      1
/** number of milliseconds per second */
#define MSEC_PER_SEC 1000U

#define k_ticks_to_ms_floor32(t)        (t) * MS_PER_TICKS

#define K_MSEC(ms)    ((k_timeout_t) { ((ms) / MS_PER_TICKS) })

#define K_SECONDS(s)   K_MSEC((s) * MSEC_PER_SEC)

#define K_MINUTES(m)   K_SECONDS((m) * 60)

#define K_HOURS(h)     K_MINUTES((h) * 60)


uint32_t k_uptime_get_32(void);

/**
 * @brief Get system uptime.
 *
 * This routine returns the elapsed time since the system booted,
 * in milliseconds.
 *
 * @note
 *    While this function returns time in milliseconds, it does
 *    not mean it has millisecond resolution. The actual resolution depends on
 *    @kconfig{CONFIG_SYS_CLOCK_TICKS_PER_SEC} config option.
 *
 * @return Current uptime in milliseconds.
 */
int64_t k_uptime_get(void);

static inline int64_t k_uptime_delta(int64_t *reftime)
{
	int64_t uptime, delta;

	uptime = k_uptime_get();
	delta = uptime - *reftime;
	*reftime = uptime;

	return delta;
}

struct k_sem
{
    void         *sem;
};

int k_sem_init(struct k_sem *sem, unsigned int initial_count, unsigned int limit);

int k_sem_take(struct k_sem *sem, k_timeout_t timeout);

void k_sem_give(struct k_sem *sem);

void k_sem_free(struct k_sem *sem);

struct k_mem_slab
{
    void  *sema_count;
    uint32_t num_blocks;
    size_t   block_size;
};

int k_mem_slab_alloc(struct k_mem_slab *slab, void **mem, k_timeout_t timeout);

void k_mem_slab_free(struct k_mem_slab *slab, void *mem);

uint32_t k_mem_slab_num_free_get(struct k_mem_slab *slab);

enum
{
    /**
     * @cond INTERNAL_HIDDEN
     */

    /* The atomic API is used for all work and queue flags fields to
     * enforce sequential consistency in SMP environments.
     */

    /* Bits that represent the work item states.  At least nine of the
     * combinations are distinct valid stable states.
     */
    K_WORK_RUNNING_BIT = 0,
    K_WORK_CANCELING_BIT = 1,
    K_WORK_QUEUED_BIT = 2,
    K_WORK_DELAYED_BIT = 3,
    K_WORK_FLUSHING_BIT = 4,

    K_WORK_MASK = BIT(K_WORK_DELAYED_BIT) | BIT(K_WORK_QUEUED_BIT)
                  | BIT(K_WORK_RUNNING_BIT) | BIT(K_WORK_CANCELING_BIT) | BIT(K_WORK_FLUSHING_BIT),

    /* Static work flags */
    K_WORK_DELAYABLE_BIT = 8,
    K_WORK_DELAYABLE = BIT(K_WORK_DELAYABLE_BIT),

    /**
     * INTERNAL_HIDDEN @endcond
     */
    /* Transient work flags */

    /** @brief Flag indicating a work item that is running under a work
     * queue thread.
     *
     * Accessed via k_work_busy_get().  May co-occur with other flags.
     */
    K_WORK_RUNNING = BIT(K_WORK_RUNNING_BIT),

    /** @brief Flag indicating a work item that is being canceled.
     *
     * Accessed via k_work_busy_get().  May co-occur with other flags.
     */
    K_WORK_CANCELING = BIT(K_WORK_CANCELING_BIT),

    /** @brief Flag indicating a work item that has been submitted to a
     * queue but has not started running.
     *
     * Accessed via k_work_busy_get().  May co-occur with other flags.
     */
    K_WORK_QUEUED = BIT(K_WORK_QUEUED_BIT),

    /** @brief Flag indicating a delayed work item that is scheduled for
     * submission to a queue.
     *
     * Accessed via k_work_busy_get().  May co-occur with other flags.
     */
    K_WORK_DELAYED = BIT(K_WORK_DELAYED_BIT),

    /** @brief Flag indicating a synced work item that is being flushed.
     *
     * Accessed via k_work_busy_get().  May co-occur with other flags.
     */
    K_WORK_FLUSHING = BIT(K_WORK_FLUSHING_BIT),
};

struct k_work;

struct k_work_q;

typedef void (*k_work_handler_t)(struct k_work *work);

/** @brief A structure used to submit work. */
struct k_work
{
    /* All fields are protected by the work module spinlock.  No fields
     * are to be accessed except through kernel API.
     */

    /* Node to link into k_work_q pending list. */
    sys_snode_t node;

    /* The function to be invoked by the work queue thread. */
    k_work_handler_t handler;

    uint32_t flags;
};

struct k_work_q
{
    /* Flags describing queue state. */
    uint32_t flags;
};

#define Z_WORK_INITIALIZER(work_handler) { \
        .handler = work_handler, \
                   .flags = 0,\
    }

struct k_work_delayable
{
    /* The work item. */
    struct k_work work;

    void *timer;

    uint32_t start_time_ms;

    k_ticks_t timer_period;
};

#define Z_WORK_DELAYABLE_INITIALIZER(work_handler) { \
        .work = { \
                  .handler = work_handler, \
                  .flags = 0, \
                }, \
                .timer = NULL, \
    }

void k_work_init(struct k_work *work,
                 k_work_handler_t handler);

int k_work_submit(struct k_work *work);

int k_work_reschedule(struct k_work_delayable *dwork,
                      k_timeout_t delay);

void k_work_init_delayable(struct k_work_delayable *dwork,
                           k_work_handler_t handler);

int k_work_cancel_delayable(struct k_work_delayable *dwork);


k_ticks_t k_work_delayable_remaining_get(const struct k_work_delayable *dwork);

static inline struct k_work_delayable *k_work_delayable_from_work(struct k_work *work)
{
    return CONTAINER_OF(work, struct k_work_delayable, work);
}

int k_work_reschedule(struct k_work_delayable *dwork,
                      k_timeout_t delay);

int k_work_schedule(struct k_work_delayable *dwork,
                    k_timeout_t delay);

bool k_work_is_pending(const struct k_work *work);

bool k_work_delayable_is_pending(const struct k_work_delayable *dwork);

struct k_queue {
	void *data_q;
};

void k_queue_init(struct k_queue *queue);

void k_queue_append(struct k_queue *queue, void *data);

void *k_queue_get(struct k_queue *queue, k_timeout_t timeout);

struct k_fifo {
	struct k_queue _queue;
};

#define k_fifo_init(fifo)                                    \
	({                                                   \
	k_queue_init(&(fifo)->_queue);                       \
	})


#define k_fifo_put(fifo, data) \
	({ \
	k_queue_append(&(fifo)->_queue, data); \
	})

#define k_fifo_get(fifo, timeout) \
	({ \
	void *fg_ret = k_queue_get(&(fifo)->_queue, timeout); \
	fg_ret; \
	})

void mesh_kernel_init(void);


#endif /* MESH_INCLUDE_BT_MESH_KERNEL_H__ */
