/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mesh_cfg.h"
#include "mesh_kernel.h"
#include "wrapper_os.h"

#if defined(PLATFORM_OS_FREERTOS)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "wrapper_freertos.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "timers.h"
#include "list.h"
#include "compiler.h"
#include "trng.h"
#include "systime.h"
#endif

#include "ble_export.h"

#define LOG_LEVEL CONFIG_BT_MESH_KERNEL_LOG_LEVEL
#include "api/mesh_log.h"

#define MESH_KERNEL_TASK_USED         0

#define MESH_FIFI_QUEUE_SIZE      50

typedef struct mesh_kernel_msg
{
    uint16_t      id;

    union
    {

    } data;
} mesh_kernel_msg_t;

enum mesh_kernel_evt_id
{
    K_MESH_EVT_RSV               = 0x00,
    K_MESH_MSG_EVT,
};

#define K_MESH_EVT_ID_MSG         ((K_MESH_MSG_EVT << 8))

#define K_MESH_EVT_ID_TYPE_GET(id)         ((id & 0xFF00) >> 8)
#define K_MESH_EVT_ID_SUBTYPE_GET(id)      (id & 0xFF)

struct mesh_kernel_cb
{
#if (MESH_KERNEL_TASK_USED)
    os_task_t task_handle;
    os_sema_t list_sema;
#endif
    os_mutex_t mutex;
    sys_slist_t work_list;
};

static struct mesh_kernel_cb mesh_kernel = {
#if (MESH_KERNEL_TASK_USED)
    .task_handle = NULL,
    .list_sema = NULL,
#endif
    .mutex = NULL,
};

static inline void flag_clear(uint32_t *flagp,
                              uint32_t bit)
{
    *flagp &= ~(BIT(bit));
}

static inline void flag_set(uint32_t *flagp,
                            uint32_t bit)
{
    *flagp |= BIT(bit);
}

static inline bool flag_test(const uint32_t *flagp,
                             uint32_t bit)
{
    return (*flagp & BIT(bit)) != 0U;
}

static inline bool flag_test_and_clear(uint32_t *flagp,
                                       int bit)
{
    bool ret = flag_test(flagp, bit);

    flag_clear(flagp, bit);

    return ret;
}

static inline void flags_set(uint32_t *flagp,
                             uint32_t flags)
{
    *flagp = flags;
}

static inline uint32_t flags_get(const uint32_t *flagp)
{
    return *flagp;
}

uint32_t k_uptime_get_32(void)
{
    return sys_current_time_get();
}

int64_t k_uptime_get(void)
{
#if defined(PLATFORM_OS_FREERTOS)
    return (int64_t)(xTaskGetTickCount() * (TickType_t)OS_MS_PER_TICK);
#else
    return (int64_t)sys_current_time_get();
#endif
}

int k_sem_init(struct k_sem *sem, unsigned int initial_count, unsigned int limit)
{
    int32_t status = sys_sema_init_ext(&sem->sem, limit, initial_count);

    return status == OS_OK ? 0 : -EINVAL;
}

int k_sem_take(struct k_sem *sem, k_timeout_t timeout)
{
    int32_t status;

    if (timeout.ticks == 0) {
        timeout.ticks = MS_PER_TICKS;
    }

    if (timeout.ticks == -1) {
        status = sys_sema_down(&sem->sem, 0);
    } else {
        status = sys_sema_down(&sem->sem, timeout.ticks / MS_PER_TICKS);
    }
    if (status == OS_TIMEOUT) {
        return -EAGAIN;
    }

    return 0;
}

void k_sem_give(struct k_sem *sem)
{
    sys_sema_up(&sem->sem);
}

void k_sem_free(struct k_sem *sem)
{
    sys_sema_free(&sem->sem);
}

int k_mem_slab_alloc(struct k_mem_slab *slab, void **mem, k_timeout_t timeout)
{
    int32_t status;
    if (slab->sema_count == NULL) {
        if (sys_sema_init_ext(&slab->sema_count, slab->num_blocks, slab->num_blocks) != OS_OK) {
            return -EAGAIN;
        }
    }

    if (sys_sema_get_count(&slab->sema_count) == 0) {
        return -ENOMEM;
    }

    if (timeout.ticks == 0) {
        timeout.ticks = MS_PER_TICKS;
    }

    if (timeout.ticks == -1) {
        status = sys_sema_down(&slab->sema_count, 0);
    } else {
        status = sys_sema_down(&slab->sema_count, timeout.ticks / MS_PER_TICKS);
    }

    if (status == OS_TIMEOUT || status != OS_OK) {
        return -EAGAIN;
    }

    *mem = sys_malloc(slab->block_size);
    return 0;
}

void k_mem_slab_free(struct k_mem_slab *slab, void *mem)
{
    if (slab->sema_count == NULL) {
        sys_sema_init_ext(&slab->sema_count, slab->num_blocks, slab->num_blocks);
        return;
    }

    sys_mfree(mem);
    sys_sema_up(slab->sema_count);
    return;
}

uint32_t k_mem_slab_num_free_get(struct k_mem_slab *slab)
{
    if (slab->sema_count == NULL) {
        if (sys_sema_init_ext(&slab->sema_count, slab->num_blocks, slab->num_blocks) != OS_OK) {
            return 0;
        } else {
            return slab->num_blocks;
        }
    }

    return sys_sema_get_count(&slab->sema_count);
}


static void k_work_notify_task(void)
{
#if (MESH_KERNEL_TASK_USED)
    sys_sema_up(&mesh_kernel.list_sema);
#else
    mesh_kernel_msg_t msg;

    msg.id = K_MESH_EVT_ID_MSG;

    if (!ble_local_app_msg_send(&msg, sizeof(mesh_kernel_msg_t))) {
        LOG_ERR("mesh kernel notify task fail!");
        return;
    }
#endif
}

void k_work_init(struct k_work *work, k_work_handler_t handler)
{
    __ASSERT_NO_MSG(work != NULL);
    __ASSERT_NO_MSG(handler != NULL);

    work->handler = handler;
    work->flags = 0;
}

int k_work_submit(struct k_work *work)
{
    int ret = 0;

#if (MESH_KERNEL_TASK_USED)
    if (mesh_kernel.task_handle == NULL) {
        return -ENODEV;
    }
#endif

    sys_mutex_get(&mesh_kernel.mutex);
    if (flag_test(&work->flags, K_WORK_CANCELING_BIT)) {
        /* Disallowed */
        ret = -EBUSY;
    } else if (!flag_test(&work->flags, K_WORK_QUEUED_BIT)) {
        ret = 1;
        sys_slist_append(&mesh_kernel.work_list, &work->node);
        flag_set(&work->flags, K_WORK_QUEUED_BIT);

        if (flag_test(&work->flags, K_WORK_RUNNING_BIT)) {
            ret = 2;
        }
    }
    sys_mutex_put(&mesh_kernel.mutex);

    if (ret > 0) {
        k_work_notify_task();
    }

    return ret;
}

bool k_work_is_pending(const struct k_work *work)
{
    bool busy = false;
    sys_mutex_get(&mesh_kernel.mutex);
    busy = (flags_get(&work->flags) & K_WORK_MASK) ? true : false;
    sys_mutex_put(&mesh_kernel.mutex);

    return busy;
}

static void work_timeout(void *p_tmr, void *p_arg)
{
    struct k_work_delayable *dwork = p_arg;
    bool added = false;

    sys_mutex_get(&mesh_kernel.mutex);
    if (sys_timer_pending(&dwork->timer)) {
        // timer is active, it must be rescheduled before timeout function is completed
        LOG_ERR("work_timeout timer is still pending\r\n");
    } else if (flag_test_and_clear(&dwork->work.flags, K_WORK_DELAYED_BIT)) {
        dwork->timer_period = 0;
        if (!flag_test(&dwork->work.flags, K_WORK_QUEUED_BIT)) {
            sys_slist_append(&mesh_kernel.work_list, &dwork->work.node);
            flag_set(&dwork->work.flags, K_WORK_QUEUED_BIT);
            added = true;
        }
    }
    sys_mutex_put(&mesh_kernel.mutex);

    if (added) {
        k_work_notify_task();
    }
}

void k_work_init_delayable(struct k_work_delayable *dwork, k_work_handler_t handler)
{
    dwork->work.handler = handler;
    dwork->work.flags = 0;

    sys_timer_init(&dwork->timer, (const uint8_t *)"", 1, 0, work_timeout, dwork);
}

int k_work_cancel_delayable(struct k_work_delayable *dwork)
{
    sys_mutex_get(&mesh_kernel.mutex);
    if (dwork->timer == NULL) {
        sys_timer_init(&dwork->timer, (const uint8_t *)"", 1, 0, work_timeout, dwork);
        sys_mutex_put(&mesh_kernel.mutex);
        return 0;
    }

    if (flag_test_and_clear(&dwork->work.flags, K_WORK_DELAYED_BIT)) {
        sys_timer_stop(&dwork->timer, false);
    } else if (flag_test_and_clear(&dwork->work.flags, K_WORK_QUEUED_BIT)) {
        sys_slist_find_and_remove(&mesh_kernel.work_list, &dwork->work.node);
    }
    sys_mutex_put(&mesh_kernel.mutex);

    return 0;
}

int k_work_schedule(struct k_work_delayable *dwork, k_timeout_t delay)
{
    bool added = false;
    sys_mutex_get(&mesh_kernel.mutex);
    if (dwork->timer == NULL) {
        sys_timer_init(&dwork->timer, (const uint8_t *)"", 1, 0, work_timeout, dwork);
    }

    if (((flags_get(&dwork->work.flags) & K_WORK_MASK) & ~K_WORK_RUNNING) == 0U) {
        dwork->start_time_ms = sys_current_time_get();
        dwork->timer_period = delay.ticks;
        if (delay.ticks == 0) {
            if (!flag_test(&dwork->work.flags, K_WORK_QUEUED_BIT)) {
                sys_slist_append(&mesh_kernel.work_list, &dwork->work.node);
                flag_set(&dwork->work.flags, K_WORK_QUEUED_BIT);
                added = true;
            }
        } else {
            flag_set(&dwork->work.flags, K_WORK_DELAYED_BIT);
            sys_timer_start_ext(&dwork->timer, delay.ticks * MS_PER_TICKS, false);
        }

    }

    sys_mutex_put(&mesh_kernel.mutex);

    if (added) {
        k_work_notify_task();
    }
    return 0;
}

int k_work_reschedule(struct k_work_delayable *dwork, k_timeout_t delay)
{
    bool added = false;
    sys_mutex_get(&mesh_kernel.mutex);
    if (dwork->timer == NULL) {
        sys_timer_init(&dwork->timer, (const uint8_t *)"", 1, 0, work_timeout, dwork);
    }

    if (flag_test_and_clear(&dwork->work.flags, K_WORK_DELAYED_BIT)) {
        sys_timer_stop(&dwork->timer, false);
    }

    dwork->start_time_ms = sys_current_time_get();
    dwork->timer_period = delay.ticks;
    if (delay.ticks == 0) {
        if (!flag_test(&dwork->work.flags, K_WORK_QUEUED_BIT)) {
            sys_slist_append(&mesh_kernel.work_list, &dwork->work.node);
            flag_set(&dwork->work.flags, K_WORK_QUEUED_BIT);
            added = true;
        }
    } else {
        flag_set(&dwork->work.flags, K_WORK_DELAYED_BIT);
        sys_timer_start_ext(&dwork->timer, delay.ticks * MS_PER_TICKS, false);
    }

    sys_mutex_put(&mesh_kernel.mutex);

    if (added) {
        k_work_notify_task();
    }
    return 0;
}

bool k_work_delayable_is_pending(const struct k_work_delayable *dwork)
{
    bool busy = false;
    sys_mutex_get(&mesh_kernel.mutex);
    busy = flags_get(&dwork->work.flags) & K_WORK_MASK ? true : false;
    sys_mutex_put(&mesh_kernel.mutex);
    return busy;
}

k_ticks_t k_work_delayable_remaining_get(const struct k_work_delayable *dwork)
{
    uint32_t cur_time = sys_current_time_get();

    uint32_t delta = (cur_time - dwork->start_time_ms) / MS_PER_TICKS;

    return delta > dwork->timer_period ? 0 : (dwork->timer_period - delta);
}

static void mesh_kernel_handle_task(void)
{
    sys_snode_t *cur = NULL;
    struct k_work *cur_work;

    sys_mutex_get(&mesh_kernel.mutex);
    cur = sys_slist_get(&mesh_kernel.work_list);
    if (cur == NULL) {
        sys_mutex_put(&mesh_kernel.mutex);
        return;
    }

    cur_work = CONTAINER_OF(cur, struct k_work, node);

    flag_clear(&cur_work->flags, K_WORK_QUEUED_BIT);
    flag_set(&cur_work->flags, K_WORK_RUNNING_BIT);

    sys_mutex_put(&mesh_kernel.mutex);

    if (cur_work->handler) {
        cur_work->handler(cur_work);
    }

    sys_mutex_get(&mesh_kernel.mutex);
    flag_clear(&cur_work->flags, K_WORK_RUNNING_BIT);
    sys_mutex_put(&mesh_kernel.mutex);

}

static bool mesh_kernel_msg_hdl(void *p_buf)
{
    uint8_t subtype;
    mesh_kernel_msg_t *p_msg = (mesh_kernel_msg_t *)p_buf;
    subtype = K_MESH_EVT_ID_SUBTYPE_GET(p_msg->id);

    switch (K_MESH_EVT_ID_TYPE_GET(p_msg->id)) {
    case K_MESH_MSG_EVT: {
        mesh_kernel_handle_task();
    } break;

    default:
      break;
    }

    return true;
}

#if (MESH_KERNEL_TASK_USED)
static void mesh_kernel_task(void *param)
{
    sys_snode_t *cur = NULL;
    struct k_work *cur_work;
    for (;;) {
        sys_sema_down(&mesh_kernel.list_sema, 0);
        mesh_kernel_handle_task();
    }
}
#endif

void k_queue_init(struct k_queue *queue)
{
    if (queue) {
        // FIX TODO queue size may be too small or too large
        int32_t status = sys_queue_init(&queue->data_q, MESH_FIFI_QUEUE_SIZE, sizeof(void *));
        if (status != OS_OK) {
            LOG_ERR("k_queue_init init fail");
        }
    }
}

void k_queue_append(struct k_queue *queue, void *data)
{
    int32_t status = sys_queue_post(&queue->data_q, &data);
    if (status != OS_OK) {
        LOG_ERR("k_queue_append fail");
    }
}

void *k_queue_get(struct k_queue *queue, k_timeout_t timeout)
{
    void *msg = NULL;

    if (timeout.ticks == 0) {
        timeout.ticks = MS_PER_TICKS;
    }

    if (timeout.ticks == -1) {
        sys_queue_read(&queue->data_q, &msg, 0, false);
    } else {
        sys_queue_read(&queue->data_q, &msg, timeout.ticks / MS_PER_TICKS, false);
    }

    return msg;
}

void mesh_kernel_init(void)
{
    mesh_log_init();

    sys_slist_init(&mesh_kernel.work_list);

    if (sys_mutex_init(&mesh_kernel.mutex) != OS_OK) {
        LOG_ERR("mesh_kernel_init mutex init fail");
        return;
    }

#if (MESH_KERNEL_TASK_USED)
    if (sys_sema_init(&mesh_kernel.list_sema, 0) != OS_OK) {
        LOG_ERR("mesh_kernel_init sema init fail");
        sys_mutex_free(&mesh_kernel.mutex);
        return;
    }

    mesh_kernel.task_handle = sys_task_create_dynamic((const uint8_t *)"mesh kernel task", 768,
                                                      OS_TASK_PRIORITY(2), mesh_kernel_task, NULL);

    if (mesh_kernel.task_handle == NULL) {
        LOG_ERR("mesh_kernel_init mesh task create fail");
    }
#else
    ble_app_msg_hdl_reg(mesh_kernel_msg_hdl);
#endif
}

