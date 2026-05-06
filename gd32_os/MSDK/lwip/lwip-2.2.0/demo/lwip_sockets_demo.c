/*!
    \file    lwip_sockets_demo.c
    \brief   LwIP sockets API demo for GD32VW55x SDK

    \version 2023-07-20, V1.0.0, firmware for GD32VW55x
*/

/*
 * Copyright (c) 2023, GigaDevice Semiconductor Inc.

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

#include "lwip/opt.h"

#if LWIP_SOCKET /* don't build if not configured for use in lwipopts.h */
#ifdef CONFIG_LWIP_SOCKETS_TEST
#include "lwip/sockets.h"
#include "lwip/priv/sockets_priv.h"
#include "wrapper_os.h"

struct sock_fd_info_t {
    int fd;
    uint16_t port;
    uint8_t mode;
    uint8_t index;
    uint8_t terminate;
};

#define FD_NUM_MAX              4
#define TCP_SEVER_LISTEN_NUM    2

#define SOCKET_SERVER           0
#define SOCKET_CLIENT           1

uint8_t sock_init_flag = 0;
struct sock_fd_info_t sock_fd_info[FD_NUM_MAX];

static struct sock_fd_info_t *free_sock_fd_info_get(void)
{
    uint8_t idx;

    for (idx = 0; idx < FD_NUM_MAX; idx++) {
        if (sock_fd_info[idx].fd == -1) {
            return &sock_fd_info[idx];
        }
    }
    return NULL;
}

static struct sock_fd_info_t *sock_fd_info_get_by_fd(int fd)
{
    uint8_t idx;

    for (idx = 0; idx < FD_NUM_MAX; idx++) {
        if (sock_fd_info[idx].fd == fd)
            return &sock_fd_info[idx];
    }
    return NULL;
}

static void sock_fd_info_init(void)
{
    uint8_t idx;
    struct sock_fd_info_t *sock_fd = NULL;

    for (idx = 0; idx < FD_NUM_MAX; idx++) {
        sock_fd = &sock_fd_info[idx];
        sock_fd->fd = -1;
        sock_fd->port = 0;
        sock_fd->index = idx;
        sock_fd->mode = -1;
        sock_fd->terminate = 0;
    }
}

static void socket_free(struct sock_fd_info_t *sock_fd)
{
    if (sock_fd == NULL)
        return;

    shutdown(sock_fd->fd, SHUT_RD);
    close(sock_fd->fd);

    sock_fd->fd = -1;
    sock_fd->port = 0;
    sock_fd->mode = -1;
    sock_fd->terminate = 0;
}

static void udp_cli_task(void *param)
{
    struct sock_fd_info_t *sock_fd = (struct sock_fd_info_t *)param;
    char recv_buf[128];
    struct sockaddr_in src;
    socklen_t src_len;
    int ret;
    fd_set read_set;
    struct timeval timeout;

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    sys_memset(&src, 0, sizeof(src));
    src_len = sizeof(struct sockaddr);
    sock_fd->terminate = 0;
    while (1) {
        if (sock_fd->terminate)
            break;

        FD_ZERO(&read_set);
        FD_SET(sock_fd->fd, &read_set);

        select(sock_fd->fd + 1, &read_set, NULL, NULL, &timeout);
        if (!FD_ISSET(sock_fd->fd, &read_set))
            continue;

        sys_memset(recv_buf, 0, 128);
        ret = recvfrom(sock_fd->fd, recv_buf, 128, 0, (struct sockaddr *)&src, &src_len);
        if (ret == 0) {
            printf("%d-%u remote close.\r\n", sock_fd->fd, sock_fd->port);
            break;
        } else if (ret > 0) {
            printf("%d-%u recv:%s\r\n", sock_fd->fd, sock_fd->port, recv_buf);
        } else {
            if (errno == EAGAIN) {
                continue;
            } else {
                printf("%d-%u recv error: %d.\r\n", sock_fd->fd, sock_fd->port, errno);
                break;
            }
        }
    }

    printf("UDP client is closed.\r\n");
    socket_free(sock_fd);
    sys_task_delete(NULL);
}
static uint8_t lwip_sockets_udp_client(char *remote_ip, uint16_t remote_port)
{
    int fd = -1, ret;
    struct sockaddr_in remote_addr;
    char *send_payload = "this is udp client test.";
    static os_task_t task_hdl = NULL;
    struct sock_fd_info_t *sock_fd = NULL;

    sock_fd = free_sock_fd_info_get();
    if (sock_fd == NULL) {
        printf("get free sock_fd_info failed!\r\n");
        goto Exit;
    }

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        printf("Create udp client socket fd error!\r\n");
        goto Exit;
    }
    printf("Create udp client socket: %d\r\n", fd);
    sock_fd->fd = fd;
    sock_fd->port = remote_port;
    sock_fd->mode = SOCKET_CLIENT;

    sys_memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_len = sizeof(remote_addr);
    remote_addr.sin_port = htons(remote_port);
    remote_addr.sin_addr.s_addr = inet_addr(remote_ip);

    ret = sendto(fd, send_payload, strlen(send_payload), 0, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr_in));
    if (ret <= 0) {
        printf("send error: %d.\r\n", ret);
        goto Exit;
    }

    task_hdl = sys_task_create_dynamic((const uint8_t *)"sock_udp_cli", 512, OS_TASK_PRIORITY(2),
                (task_func_t)udp_cli_task, sock_fd);
    if (task_hdl == NULL) {
        printf("ERROR: Create socket udp client task failed\r\n");
        goto Exit;
    }

    return 1;
Exit:
    if (fd > -1) {
        socket_free(sock_fd);
        printf("UDP client is closed.\r\n");
    }
    return 0;
}

static void tcp_cli_task(void *param)
{
    struct sock_fd_info_t *sock_fd = (struct sock_fd_info_t *)param;
    char recv_buf[128];
    int ret;
    fd_set read_set;
    struct timeval timeout;

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    sock_fd->terminate = 0;
    while (1) {
        if (sock_fd->terminate)
            break;

        FD_ZERO(&read_set);
        FD_SET(sock_fd->fd, &read_set);

        select(sock_fd->fd + 1, &read_set, NULL, NULL, &timeout);
        if (!FD_ISSET(sock_fd->fd, &read_set))
            continue;

        sys_memset(recv_buf, 0, 128);
        ret = recv(sock_fd->fd, recv_buf, 128, 0);
        if (ret == 0) {
            printf("%d-%u remote close.\r\n", sock_fd->fd, sock_fd->port);
            break;
        } else if (ret > 0) {
            printf("%d-%u recv:%s\r\n", sock_fd->fd, sock_fd->port, recv_buf);
        } else {
            if (errno == EAGAIN) {
                continue;
            } else {
                printf("%d-%u recv error: %d.\r\n", sock_fd->fd, sock_fd->port, errno);
                break;
            }
        }
    }

    printf("TCP client is closed.\r\n");
    socket_free(sock_fd);
    sys_task_delete(NULL);
}
static uint8_t lwip_sockets_tcp_client(char *remote_ip, uint16_t remote_port)
{
    int fd = -1, ret;
    struct sockaddr_in remote_addr;
    char *send_payload = "this is tcp client test.";
    static os_task_t task_hdl = NULL;
    struct sock_fd_info_t *sock_fd = NULL;

    sock_fd = free_sock_fd_info_get();
    if (sock_fd == NULL) {
        printf("get free sock_fd_info failed!\r\n");
        goto Exit;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        printf("Create tcp client socket fd error!\r\n");
        goto Exit;
    }
    printf("Create tcp client socket: %d\r\n", fd);
    sock_fd->fd = fd;
    sock_fd->port = remote_port;
    sock_fd->mode = SOCKET_CLIENT;

    sys_memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_len = sizeof(remote_addr);
    remote_addr.sin_port = htons(remote_port);
    remote_addr.sin_addr.s_addr = inet_addr(remote_ip);

    ret = connect(fd, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
    if (ret < 0) {
        printf("connect error: %d.\r\n", ret);
        goto Exit;
    }

    ret = send(fd, send_payload, strlen(send_payload), 0);
    if (ret <= 0) {
        printf("send error: %d.\r\n", ret);
        goto Exit;
    }

    task_hdl = sys_task_create_dynamic((const uint8_t *)"sock_tcp_cli", 512, OS_TASK_PRIORITY(2),
                (task_func_t)tcp_cli_task, sock_fd);
    if (task_hdl == NULL) {
        printf("ERROR: Create socket tcp client task failed\r\n");
        goto Exit;
    }

    return 1;
Exit:
    if (fd > -1) {
        socket_free(sock_fd);
        printf("TCP client is closed.\r\n");
    }
    return 0;
}

static void udp_srv_task(void *param)
{
    struct sock_fd_info_t *sock_fd = (struct sock_fd_info_t *)param;
    struct sockaddr_in client_addr;
    socklen_t len;
    char recv_buf[128];
    int ret;
    fd_set read_set;
    struct timeval timeout;

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    len = sizeof(struct sockaddr);
    sock_fd->terminate = 0;
    while (1) {
        if (sock_fd->terminate)
            break;

        FD_ZERO(&read_set);
        FD_SET(sock_fd->fd, &read_set);

        select(sock_fd->fd + 1, &read_set, NULL, NULL, &timeout);
        if (!FD_ISSET(sock_fd->fd, &read_set))
            continue;

        sys_memset(recv_buf, 0, 128);
        ret = recvfrom(sock_fd->fd, recv_buf, 128, 0, (struct sockaddr *)&client_addr, &len);
        printf("%d-%u from %s ", sock_fd->fd, sock_fd->port, inet_ntoa(client_addr.sin_addr));
        if (ret == 0) {
            printf("remote close.\r\n");
            continue;
        } else if (ret > 0) {
            printf("recv:%s\r\n", recv_buf);
        } else {
            printf("recv error: %d %d.\r\n", ret, errno);
            if (errno == EBADF) {
                break;
            }
            continue;
        }

        ret = sendto(sock_fd->fd, recv_buf, strlen(recv_buf), 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr_in));
        if (ret <= 0) {
            printf("send error: %d %d.\r\n", ret, errno);
            continue;
        }
    }
    printf("udp server is closed.\r\n");
    socket_free(sock_fd);
    sys_task_delete(NULL);
}
static uint8_t lwip_sockets_udp_server(uint16_t server_port)
{
    int fd = -1, ret;
    struct sockaddr_in server_addr;
    static os_task_t task_hdl = NULL;
    struct sock_fd_info_t *sock_fd = NULL;

    sock_fd = free_sock_fd_info_get();
    if (sock_fd == NULL) {
        printf("get free sock_fd_info failed!\r\n");
        goto Exit;
    }

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        printf("Create udp server socket fd error!\r\n");
        goto Exit;
    }
    printf("Create udp server socket: %d\r\n", fd);
    sock_fd->fd = fd;
    sock_fd->port = server_port;
    sock_fd->mode = SOCKET_SERVER;

    sys_memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_len = sizeof(server_addr);
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(ret < 0) {
        printf("Bind udp server socket fd error!\r\n");
        goto Exit;
    }

    task_hdl = sys_task_create_dynamic((const uint8_t *)"sock_udp_srv", 512, OS_TASK_PRIORITY(2),
                (task_func_t)udp_srv_task, sock_fd);
    if (task_hdl == NULL) {
        printf("ERROR: Create socket udp server task failed\r\n");
        goto Exit;
    }

    return 1;
Exit:
    if (fd > -1) {
        socket_free(sock_fd);
        printf("udp server is closed.\r\n");
    }
    return 0;
}

static void tcp_srv_task(void *param)
{
    struct sock_fd_info_t *sock_fd = (struct sock_fd_info_t *)param;
    struct sockaddr_in client_addr;
    socklen_t len;
    char recv_buf[128];
    int ret, idx;
    uint8_t cli_count = 0;
    int cli_fd[TCP_SEVER_LISTEN_NUM];
    fd_set read_set;
    struct timeval timeout;
    int max_fd_num = 0;
    int keepalive = 1;
    int keepidle = 60; //in seconds
    int keepcnt = 5;
    int keepinval = 10; //in seconds

    for (idx = 0; idx < TCP_SEVER_LISTEN_NUM; idx++)
        cli_fd[idx] = -1;

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    len = sizeof(struct sockaddr);
    sock_fd->terminate = 0;
    while (1) {
        if (sock_fd->terminate)
            break;

        FD_ZERO(&read_set);
        for (idx = 0; idx < TCP_SEVER_LISTEN_NUM; idx++) {
            if (cli_fd[idx] > 0) {
                FD_SET(cli_fd[idx], &read_set);
                if (cli_fd[idx] > max_fd_num)
                    max_fd_num = cli_fd[idx];
            }
        }
        if (cli_count < TCP_SEVER_LISTEN_NUM) {
            FD_SET(sock_fd->fd, &read_set);
            if (sock_fd->fd > max_fd_num)
                max_fd_num = sock_fd->fd;
        }

        select(max_fd_num + 1, &read_set, NULL, NULL, &timeout);

        if (FD_ISSET(sock_fd->fd, &read_set)) {
            for (idx = 0; idx < TCP_SEVER_LISTEN_NUM; idx++) {
                if (cli_fd[idx] == -1) {
                    break;
                }
            }
            if (idx == TCP_SEVER_LISTEN_NUM) {
                printf("cli count error!\r\n");
                goto exit;
            }
            cli_fd[idx] = accept(sock_fd->fd, (struct sockaddr *)&client_addr, (socklen_t *)&len);
            if (cli_fd[idx] < 0) {
                if (errno != EAGAIN)
                    printf("accept error. %d\r\n", errno);
                if (errno == EBADF) {
                    goto exit;
                }
                for (idx = 0; idx < TCP_SEVER_LISTEN_NUM; idx++) {
                    if (cli_fd[idx] != -1 && FD_ISSET(cli_fd[idx], &read_set))
                        break;
                }
                if (idx == TCP_SEVER_LISTEN_NUM) {
                    continue;
                }
            } else {
                cli_count++;
                setsockopt(cli_fd[idx], SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(int));
                setsockopt(cli_fd[idx], IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(int));
                setsockopt(cli_fd[idx], IPPROTO_TCP, TCP_KEEPINTVL, &keepinval, sizeof(int));
                setsockopt(cli_fd[idx], IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(int));
            }
        }

        for (idx = 0; idx < TCP_SEVER_LISTEN_NUM; idx++) {
            sys_memset(recv_buf, 0, 128);
            if (cli_fd[idx] == -1)
                continue;
            if (FD_ISSET(cli_fd[idx], &read_set)) {
                ret = recv(cli_fd[idx], recv_buf, 128, 0);
                if (ret == 0) {
                    printf("%d-%u remote close. from %d.\r\n", sock_fd->fd, sock_fd->port, cli_fd[idx]);
                    goto remove_client;
                } else if (ret > 0) {
                    printf("%d-%u recv:%s from %d.\r\n", sock_fd->fd, sock_fd->port, recv_buf, cli_fd[idx]);
                } else {
                    if (errno == EAGAIN) {
                        continue;
                    } else if (errno == EBADF) {
                        printf("%d-%u rev error: %d. from %d.\r\n", sock_fd->fd, sock_fd->port, errno, cli_fd[idx]);
                        goto exit;
                    } else if (errno == ECONNABORTED) {
                        printf("%d-%u connection aborted, maybe remote close. from %d.\r\n", sock_fd->fd, sock_fd->port, cli_fd[idx]);
                        goto remove_client;
                    } else {
                        printf("%d-%u rev error: %d. from %d.\r\n", sock_fd->fd, sock_fd->port, errno, cli_fd[idx]);
                        goto remove_client;
                    }
                }

                ret = send(cli_fd[idx], recv_buf, strlen(recv_buf), 0);
                if (ret <= 0) {
                    printf("send error: %d.\r\n", ret);
                    goto remove_client;
                }
            }
            continue;
remove_client:
            shutdown(cli_fd[idx], SHUT_RD);
            close(cli_fd[idx]);
            cli_fd[idx] = -1;
            cli_count--;
        }
    }
exit:
    printf("tcp server is closed.\r\n");
    for (idx = 0; idx < TCP_SEVER_LISTEN_NUM; idx++) {
        if (cli_fd[idx] != -1) {
            shutdown(cli_fd[idx], SHUT_RD);
            close(cli_fd[idx]);
        }
    }
    socket_free(sock_fd);
    sys_task_delete(NULL);
}
static uint8_t lwip_sockets_tcp_server(uint16_t server_port)
{
    int fd = -1, ret, reuse;
    struct sockaddr_in server_addr;
    static os_task_t task_hdl = NULL;
    struct sock_fd_info_t *sock_fd = NULL;

    sock_fd = free_sock_fd_info_get();
    if (sock_fd == NULL) {
        printf("get free sock_fd_info failed!\r\n");
        goto Exit;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        printf("Create tcp server socket fd error!\r\n");
        goto Exit;
    }
    printf("Create tcp server socket: %d\r\n", fd);
    sock_fd->fd = fd;
    sock_fd->port = server_port;
    sock_fd->mode = SOCKET_SERVER;

    reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));

    sys_memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_len = sizeof(server_addr);
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(ret < 0) {
        printf("Bind tcp server socket fd error!\r\n");
        goto Exit;
    }

    ret = listen(fd, TCP_SEVER_LISTEN_NUM);
    if(ret != 0) {
        printf("Listen tcp server socket fd error!\r\n");
        goto Exit;
    }

    task_hdl = sys_task_create_dynamic((const uint8_t *)"sock_tcp_srv", 512, OS_TASK_PRIORITY(2),
                (task_func_t)tcp_srv_task, sock_fd);
    if (task_hdl == NULL) {
        printf("ERROR: Create socket tcp server task failed\r\n");
        goto Exit;
    }

    return 1;
Exit:
    if (fd > -1) {
        socket_free(sock_fd);
        printf("tcp server is closed.\r\n");
    }
    return 0;
}

void cmd_lwip_sockets_client(int argc, char **argv)
{
    uint8_t ip_len = 0;
    uint8_t type, ret;
    char remote_ip[16];
    uint16_t remote_port;
    char *endptr = NULL;

    if (argc != 4)
        goto Usage;

    type = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
    if (*endptr != '\0' || type > 1) {
        goto Usage;
    }
    sys_memset(remote_ip, 0, 16);
    ip_len = (strlen((const char *)argv[2]) < 16)? (strlen((const char *)argv[2]) + 1) : 16;
    sys_memcpy(remote_ip, (const char *)argv[2], ip_len);
    remote_port = (uint16_t)strtoul((const char *)argv[3], &endptr, 10);
    if (*endptr != '\0') {
        goto Usage;
    }

    if (sock_init_flag == 0) {
        sock_fd_info_init();
        sock_init_flag = 1;
    }

    if (type == 0) {
        ret = lwip_sockets_tcp_client(remote_ip, remote_port);
    } else {
        ret = lwip_sockets_udp_client(remote_ip, remote_port);
    }
    if (!ret)
        printf("socket_client start failed!\r\n");

    return;
Usage:
    printf("socket_client <0:TCP or 1:UDP> <remote ip> <remote port>\r\n");
}

void cmd_lwip_sockets_server(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t type, ret;
    uint16_t remote_port;

    if (argc != 3)
        goto Usage;

    type = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
    if (*endptr != '\0' || type > 1) {
        goto Usage;
    }
    remote_port = (uint16_t)strtoul((const char *)argv[2], &endptr, 10);
    if (*endptr != '\0') {
        goto Usage;
    }

    if (sock_init_flag == 0) {
        sock_fd_info_init();
        sock_init_flag = 1;
    }

    if (type == 0) {
        ret = lwip_sockets_tcp_server(remote_port);
    } else {
        ret = lwip_sockets_udp_server(remote_port);
    }
    if (!ret)
        printf("socket_server start failed!\r\n");

    return;
Usage:
    printf("socket_server <0:TCP or 1:UDP> <server port>\r\n");
}

void cmd_lwip_sockets_close(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t fd;
    struct sock_fd_info_t *sock_fd = NULL;

    if (argc != 2)
        goto Usage;

    fd = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
    if (*endptr != '\0' || fd >= NUM_SOCKETS) {
        printf("Error fd.\r\n");
        goto Usage;
    }

    sock_fd = sock_fd_info_get_by_fd(fd);
    if (sock_fd == NULL || sock_init_flag == 0) {
        printf("Unknown fd.\r\n");
        return;
    }

    sock_fd->terminate = 1;

    return;
Usage:
    printf("socket_close <fd>\r\n");
    printf("\tfd: 0-%u\r\n", (NUM_SOCKETS - 1));
}

void cmd_lwip_sockets_get_status(int argc, char **argv)
{
    uint8_t idx, valid = 0;
    int type;
    socklen_t type_len = sizeof(type);
    struct sock_fd_info_t *sock_fd = NULL;

    if (sock_init_flag == 0)
        goto exit;

    for (idx = 0; idx < FD_NUM_MAX; idx++) {
        sock_fd = &sock_fd_info[idx];
        if (sock_fd->fd != -1) {
            valid = 1;
            if ((getsockopt(sock_fd->fd, SOL_SOCKET, SO_TYPE, (void *)&type, &type_len) < 0)
                    || (type < 1) || (type > 3)) {
                printf("socket[%d] fd:%d type:unknow.\r\n", idx, sock_fd->fd);
            } else {
                if (type == SOCK_STREAM) {
                    if (sock_fd->mode == SOCKET_SERVER)
                        printf("socket[%d] fd:%d type:TCP-server port:%u.\r\n", idx, sock_fd->fd, sock_fd->port);
                    else if (sock_fd->mode == SOCKET_CLIENT)
                        printf("socket[%d] fd:%d type:TCP-client port:%u.\r\n", idx, sock_fd->fd, sock_fd->port);
                } else if (type == SOCK_DGRAM) {
                    if (sock_fd->mode == SOCKET_SERVER)
                        printf("socket[%d] fd:%d type:UDP-server port:%u.\r\n", idx, sock_fd->fd, sock_fd->port);
                    else if (sock_fd->mode == SOCKET_CLIENT)
                        printf("socket[%d] fd:%d type:UDP-client port:%u.\r\n", idx, sock_fd->fd, sock_fd->port);
                }
            }
        }
    }
    if (valid == 0)
        goto exit;

    return;
exit:
    printf("no sockets status.\r\n");
}

#endif /* CONFIG_LWIP_SOCKETS_TEST */
#endif /* LWIP_SOCKET */
