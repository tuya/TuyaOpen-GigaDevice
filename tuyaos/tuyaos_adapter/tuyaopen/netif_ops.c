/**
 * @file netif_ops.c
 * @brief CORE_NETIF_OPS_T implementation for GD32 (Wi-Fi station).
 *
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 */

#include <stddef.h>
#include <string.h>

#include "core_netif_ops.h"
#include "tkl_system.h"
#include "tkl_thread.h"
#include "tkl_wifi.h"

/* Link state is polled: netmgr owns the single tkl_wifi_init() callback. */
#define NETIF_LINK_POLL_MS 1000

static CORE_NETIF_LINK_CHANGE_CB s_link_cb;
static TKL_THREAD_HANDLE         s_link_thread;
static int                       s_last_link = -1;

static void __copy_str(char *dst, size_t dst_size, const char *src)
{
    size_t len;

    if ((NULL == dst) || (0 == dst_size)) {
        return;
    }
    if (NULL == src) {
        dst[0] = '\0';
        return;
    }

    len = 0;
    while ((len < dst_size - 1) && (src[len] != '\0')) {
        len++;
    }
    memcpy(dst, src, len);
    dst[len] = '\0';
}

static OPERATE_RET __netif_get_ip(NW_IP_S *ip)
{
    if (NULL == ip) {
        return OPRT_INVALID_PARM;
    }

    memset(ip, 0, sizeof(*ip));
    return tkl_wifi_get_ip(WF_STATION, ip);
}

static void __netif_link_poll(void *arg)
{
    (void)arg;

    for (;;) {
        WF_STATION_STAT_E      stat = WSS_NO_AP_FOUND;
        CORE_NETIF_LINK_STAT_E link = CORE_NETIF_LINK_DOWN;

        if ((OPRT_OK == tkl_wifi_station_get_status(&stat)) && (WSS_GOT_IP == stat)) {
            link = CORE_NETIF_LINK_UP;
        }
        if ((NULL != s_link_cb) && ((int)link != s_last_link)) {
            s_last_link = (int)link;
            s_link_cb(link);
        }

        tkl_system_sleep(NETIF_LINK_POLL_MS);
    }
}

static OPERATE_RET __netif_set_link_cb(CORE_NETIF_LINK_CHANGE_CB cb)
{
    s_link_cb = cb;

    if ((NULL == s_link_thread) && (NULL != cb)) {
        return tkl_thread_create(&s_link_thread, "netif_link", 4096, 2, __netif_link_poll, NULL);
    }

    return OPRT_OK;
}

static OPERATE_RET __netif_foreach(CORE_NETIF_ADDR_CB cb, void *user_data)
{
    NW_IP_S           ip;
    CORE_NETIF_ADDR_T addr;
    const char       *ip_str;
    const char       *mask_str;
    const char       *gw_str;

    if (NULL == cb) {
        return OPRT_INVALID_PARM;
    }
    if (OPRT_OK != __netif_get_ip(&ip)) {
        return OPRT_COM_ERROR;
    }

#ifdef nwipstr
    ip_str   = ip.nwipstr;
    mask_str = ip.nwmaskstr;
    gw_str   = ip.nwgwstr;
#else
    ip_str   = ip.ip;
    mask_str = ip.mask;
    gw_str   = ip.gw;
#endif

    if ('\0' == ip_str[0]) {
        return OPRT_COM_ERROR;
    }

    memset(&addr, 0, sizeof(addr));
    __copy_str(addr.name, sizeof(addr.name), "sta");
    __copy_str(addr.ip, sizeof(addr.ip), ip_str);
    __copy_str(addr.mask, sizeof(addr.mask), mask_str);
    __copy_str(addr.gw, sizeof(addr.gw), gw_str);
    cb(&addr, user_data);

    return OPRT_OK;
}

CORE_NETIF_OPS_T core_netif_ops_platform = {
    .get_ip       = __netif_get_ip,
    .set_link_cb  = __netif_set_link_cb,
    .foreach_addr = __netif_foreach,
};
