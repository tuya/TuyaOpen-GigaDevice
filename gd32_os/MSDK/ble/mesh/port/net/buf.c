/* buf.c - Buffer management */

/*
 * Copyright (c) 2015-2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mesh_cfg.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "sys/byteorder.h"
#include "mesh_errno.h"

#include "net/buf.h"
#include "wrapper_os.h"

#define LOG_LEVEL CONFIG_NET_BUF_LOG_LEVEL
#include "api/mesh_log.h"

#if defined(CONFIG_NET_BUF_LOG)
#define NET_BUF_DBG(fmt, ...) LOG_DBG("(%p) " fmt, k_current_get(), \
				      ##__VA_ARGS__)
#define NET_BUF_ERR(fmt, ...) LOG_ERR(fmt, ##__VA_ARGS__)
#define NET_BUF_WARN(fmt, ...) LOG_WRN(fmt, ##__VA_ARGS__)
#define NET_BUF_INFO(fmt, ...) LOG_INF(fmt, ##__VA_ARGS__)
#else

#define NET_BUF_DBG(fmt, ...)
#define NET_BUF_ERR(fmt, ...)
#define NET_BUF_WARN(fmt, ...)
#define NET_BUF_INFO(fmt, ...)
#endif /* CONFIG_NET_BUF_LOG */

static os_mutex_t net_buf_slist_mutex = NULL;
struct net_buf *net_buf_slist_get(sys_slist_t *list)
{
    struct net_buf *buf;

    if (net_buf_slist_mutex == NULL) {
        sys_mutex_init(&net_buf_slist_mutex);
    }

    sys_mutex_get(&net_buf_slist_mutex);
    buf = (void *)sys_slist_get(list);
    sys_mutex_put(&net_buf_slist_mutex);

    return buf;
}

void net_buf_slist_put(sys_slist_t *list, struct net_buf *buf)
{
    if (net_buf_slist_mutex == NULL) {
        sys_mutex_init(&net_buf_slist_mutex);
    }

    sys_mutex_get(&net_buf_slist_mutex);
    sys_slist_append(list, &buf->node);
    sys_mutex_put(&net_buf_slist_mutex);

}


#if defined(CONFIG_NET_BUF_LOG)
void net_buf_unref_debug(struct net_buf *buf, const char *func, int line)
#else
void net_buf_unref(struct net_buf *buf)
#endif
{
    __ASSERT_NO_MSG(buf);

    if (buf->pool_id->mutex == NULL) {
        return;
    }

    sys_mutex_get(&buf->pool_id->mutex);
#if defined(CONFIG_NET_BUF_LOG)
    if (!buf->ref) {
        NET_BUF_ERR("%s():%d: buf %p double free", func, line, buf);
        sys_mutex_put(&buf->pool_id->mutex);
        return;
    }
#endif

    if (--buf->ref > 0) {
        sys_mutex_put(&buf->pool_id->mutex);
        return;
    }

    buf->data = NULL;
    buf->len = 0;
    buf->size = 0;
    sys_mfree(buf->__buf);

    buf->__buf = NULL;

    buf->pool_id->uninit_count++;
    sys_mutex_put(&buf->pool_id->mutex);
}

void net_buf_reset(struct net_buf *buf)
{
	net_buf_simple_reset(&buf->b);
}

struct net_buf * net_buf_alloc(struct net_buf_pool *pool, k_timeout_t timeout)
{
    struct net_buf *buf_found = NULL;
    uint8_t i;

    if (pool->mutex == NULL) {
        sys_mutex_init(&pool->mutex);
    }

    if (sys_mutex_try_get(&pool->mutex, timeout.ticks * MS_PER_TICKS) != OS_OK) {
        return NULL;
    }

    if(pool->uninit_count == 0) {
        sys_mutex_put(&pool->mutex);
        return NULL;
    }

    for(i = 0; i < pool->buf_count; i++) {
        struct net_buf * buf = &(pool->__bufs[i]);
        if (buf->ref == 0) {
            buf->__buf = sys_malloc(pool->data_size);
            if (buf->__buf == NULL) {
                break;
            }
            buf_found = buf;
            buf_found->ref++;
            buf_found->pool_id = pool;
            buf->size = pool->data_size;
            pool->uninit_count--;
            net_buf_reset(buf);
            break;
        }
    }
    sys_mutex_put(&pool->mutex);

    return buf_found;
}

int net_buf_id(struct net_buf *buf)
{
    return buf - buf->pool_id->__bufs;
}

