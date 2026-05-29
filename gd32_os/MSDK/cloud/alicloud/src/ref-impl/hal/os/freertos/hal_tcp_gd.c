/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "app_cfg.h"

#ifdef CONFIG_ALICLOUD_SUPPORT

#include <stdio.h>
#include <string.h>

#include "iot_import.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"

#include "wrapper_os.h"
uintptr_t HAL_TCP_Establish(const char *host, uint16_t port)
{
    struct addrinfo hints;
    struct addrinfo *addrInfoList = NULL;
    struct addrinfo *cur = NULL;
    int fd = 0;
    int rc = 0;
    char service[6];
    uint8_t dns_retry = 0;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; /* only IPv4 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    sprintf(service, "%u", port);

     while(dns_retry++ < 8) {
        rc = getaddrinfo(host, service, &hints, &addrInfoList);
        if (rc != 0) {
            hal_err("--------------getaddrinfo error[%d], rc: %d, host: %s, port: %s\n", dns_retry, rc, host, service);
            sys_ms_sleep(1000);
            continue;
        }else{
            break;
        }
    }

    if (rc != 0) {
        return (uintptr_t)(-1);
    }

    for (cur = addrInfoList; cur != NULL; cur = cur->ai_next) {
        if (cur->ai_family != AF_INET) {
            hal_err("socket type error");
            rc = -1;
            continue;
        }

        fd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
        if (fd < 0) {
            hal_err("create socket error");
            rc = -1;
            continue;
        }

        if (connect(fd, cur->ai_addr, cur->ai_addrlen) == 0) {
            rc = fd;
            break;
        }

        close(fd);
        hal_err("connect error");
        rc = -1;
    }

    if (-1 == rc) {
        hal_err("fail to establish tcp");
    } else {
        hal_info("success to establish tcp, fd=%d", rc);
    }
    freeaddrinfo(addrInfoList);

    return (uintptr_t)rc;
}

DLL_HAL_API int32_t HAL_TCP_Destroy(_IN_ uintptr_t fd)
{
    int rc;

    /* Shutdown both send and receive operations. */
    rc = shutdown((int) fd, 2);
    if (0 != rc) {
        hal_err("shutdown error");
        return -1;
    }

    rc = close((int) fd);
    if (0 != rc) {
        hal_err("closesocket error");
        return -1;
    }

    return 0;
}

int32_t HAL_TCP_Write(uintptr_t fd, const char *buf, uint32_t len, uint32_t timeout_ms)
{
    int ret;
    uint32_t len_sent;
    uint64_t t_end, t_left;
    fd_set sets;

    t_end = sys_time_get(NULL) + timeout_ms;
    len_sent = 0;
    ret = 1; /* send one time if timeout_ms is value 0 */
    int net_err = 0;

    do {
        t_left = t_end - sys_time_get(NULL);
        if (0 <= t_left) {
            struct timeval timeout;

            FD_ZERO(&sets);
            FD_SET(fd, &sets);

            timeout.tv_sec = t_left / 1000;
            timeout.tv_usec = (t_left % 1000) * 1000;

            ret = select(fd + 1, NULL, &sets, NULL, &timeout);
            if (ret > 0) {
                if (0 == FD_ISSET(fd, &sets)) {
                    hal_err("Should NOT arrive");
                    /* If timeout in next loop, it will not sent any data */
                    ret = 0;
                    continue;
                }
            } else if (0 == ret) {
                hal_err("select-write timeout %d", (int)fd);
                break;
            } else {
                if (EINTR == errno) {
                    hal_err("EINTR be caught");
                    continue;
                }

                hal_err("select-write fail, ret = select() = %d", ret);
                net_err = 1;
                break;
            }
        }

        if (ret > 0) {
            ret = send(fd, buf + len_sent, len - len_sent, 0);
            if (ret > 0) {
                len_sent += ret;
            } else if (0 == ret) {
                hal_err("No data be sent");
            } else {
                if (EINTR == errno) {
                    hal_err("EINTR be caught");
                    continue;
                }

                hal_err("send fail, ret = send() = %d", ret);
                net_err = 1;
                break;
            }
        }
    } while (!net_err && (len_sent < len) && (t_end - sys_time_get(NULL) > 0));

    if (net_err) {
        return -1;
    } else {
        return len_sent;
    }
}


int32_t HAL_TCP_Read(uintptr_t fd, char *buf, uint32_t len, uint32_t timeout_ms)
{
    int ret, err_code;
    uint32_t len_recv;
    uint64_t t_end, t_left;
    fd_set sets;
    struct timeval timeout;

    t_end = sys_time_get(NULL) + timeout_ms;
    len_recv = 0;
    err_code = 0;

    do {
        t_left = t_end - sys_time_get(NULL);
        if (t_left <= 0) {
            break;
        }
        FD_ZERO(&sets);
        FD_SET(fd, &sets);

        timeout.tv_sec = t_left / 1000;
        timeout.tv_usec = (t_left % 1000) * 1000;

        ret = select(fd + 1, &sets, NULL, NULL, &timeout);
        if (ret > 0) {
            ret = recv(fd, buf + len_recv, len - len_recv, 0);
            if (ret > 0) {
                len_recv += ret;
            } else if (0 == ret) {
                hal_err("connection is closed");
                err_code = -1;
                break;
            } else {
                if (EINTR == errno) {
                    hal_err("EINTR be caught");
                    continue;
                }
                hal_err("recv fail");
                err_code = -2;
                break;
            }
        } else if (0 == ret) {
            break;
        } else {
            hal_err("select-recv fail");
            err_code = -2;
            break;
        }
    } while ((len_recv < len));

    /* priority to return data bytes if any data be received from TCP connection. */
    /* It will get error code on next calling */
    return (0 != len_recv) ? len_recv : err_code;
}
#endif /* CONFIG_ALICLOUD_SUPPORT */
