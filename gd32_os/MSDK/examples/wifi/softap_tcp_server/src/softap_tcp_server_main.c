/*!
    \file    softap_tcp_server_main.c
    \brief   the example of tcp server in softap mode

    \version 2024-03-22, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2021, GigaDevice Semiconductor Inc.

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

#include <stdint.h>
#include <stdio.h>
#include "app_cfg.h"
#include "gd32vw55x_platform.h"
#include "lwip/sockets.h"
#include "lwip/priv/sockets_priv.h"
#include "wifi_management.h"
#include "wifi_init.h"

#define SSID            "test_ap"
#define PASSWORD        "12345678" // NULL // ""

#define TCP_SERVER_LISTEN_PORT   4065
#define TCP_SERVER_LISTEN_NUM    8

static void tcp_server_test(void)
{
    int listen_fd = -1, ret, reuse, idx;
    struct sockaddr_in server_addr;
    int cli_fd[TCP_SERVER_LISTEN_NUM];
    struct sockaddr_in client_addr;
    uint8_t cli_count = 0;
    char recv_buf[128];
    socklen_t len;
    struct timeval timeout;
    fd_set read_set;
    int max_fd_num = 0;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        printf("Create tcp server socket fd error!\r\n");
        goto exit;
    }
    printf("Create tcp server, fd: %d, port: %u.\r\n", listen_fd, TCP_SERVER_LISTEN_PORT);

    reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));
    sys_memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_len = sizeof(server_addr);
    server_addr.sin_port = htons(TCP_SERVER_LISTEN_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(ret < 0) {
        printf("Bind tcp server socket fd error!\r\n");
        goto exit;
    }

    ret = listen(listen_fd, TCP_SERVER_LISTEN_NUM);
    if(ret != 0) {
        printf("Listen tcp server socket fd error!\r\n");
        goto exit;
    }

    for (idx = 0; idx < TCP_SERVER_LISTEN_NUM; idx++)
        cli_fd[idx] = -1;

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    len = sizeof(struct sockaddr);
    while (1) {
        FD_ZERO(&read_set);
        for (idx = 0; idx < TCP_SERVER_LISTEN_NUM; idx++) {
            if (cli_fd[idx] > 0) {
                FD_SET(cli_fd[idx], &read_set);
                if (cli_fd[idx] > max_fd_num)
                    max_fd_num = cli_fd[idx];
            }
        }
        if (cli_count < TCP_SERVER_LISTEN_NUM) {
            FD_SET(listen_fd, &read_set);
            if (listen_fd > max_fd_num)
                max_fd_num = listen_fd;
        }

        select(max_fd_num + 1, &read_set, NULL, NULL, &timeout);

        if (FD_ISSET(listen_fd, &read_set)) {
            for (idx = 0; idx < TCP_SERVER_LISTEN_NUM; idx++) {
                if (cli_fd[idx] == -1) {
                    break;
                }
            }
            if (idx == TCP_SERVER_LISTEN_NUM) {
                printf("cli count error!\r\n");
                goto exit;
            }
            cli_fd[idx] = accept(listen_fd, (struct sockaddr *)&client_addr, (socklen_t *)&len);
            if (cli_fd[idx] < 0) {
                if (errno != EAGAIN)
                    printf("accept error. %d\r\n", errno);
                if (errno == EBADF) {
                    goto exit;
                }
                for (idx = 0; idx < TCP_SERVER_LISTEN_NUM; idx++) {
                    if (cli_fd[idx] != -1 && FD_ISSET(cli_fd[idx], &read_set))
                        break;
                }
                if (idx == TCP_SERVER_LISTEN_NUM) {
                    continue;
                }
            } else {
                printf("Add tcp client, fd: %d.\r\n", cli_fd[idx]);
                cli_count++;
                FD_SET(cli_fd[idx], &read_set);
                if (cli_fd[idx] > max_fd_num)
                    max_fd_num = cli_fd[idx];
            }
        }

        for (idx = 0; idx < TCP_SERVER_LISTEN_NUM; idx++) {
            sys_memset(recv_buf, 0, 128);
            if (cli_fd[idx] == -1)
                continue;
            if (FD_ISSET(cli_fd[idx], &read_set)) {
                ret = recv(cli_fd[idx], recv_buf, 128, 0);
                if (ret == 0) {
                    printf("remote close, from client, fd: %d.\r\n", cli_fd[idx]);
                    goto remove_client;
                } else if (ret > 0) {
                    printf("recv:[%s], from client, fd: %d.\r\n", recv_buf, cli_fd[idx]);
                } else {
                    if (errno == EAGAIN) {
                        continue;
                    } else if (errno == EBADF) {
                        printf("rev error: %d, from client, fd: %d.\r\n", errno, cli_fd[idx]);
                        goto exit;
                    } else {
                        printf("rev error: %d, from client, fd: %d.\r\n", errno, cli_fd[idx]);
                        goto remove_client;
                    }
                }

                ret = send(cli_fd[idx], recv_buf, strlen(recv_buf), 0);
                if (ret <= 0) {
                    printf("send error: %d, send to client, fd: %d.\r\n", errno, cli_fd[idx]);
                    goto remove_client;
                }
            }
            continue;
remove_client:
            printf("Remove tcp client, fd: %d.\r\n", cli_fd[idx]);
            shutdown(cli_fd[idx], SHUT_RD);
            close(cli_fd[idx]);
            cli_fd[idx] = -1;
            cli_count--;
        }
    }

exit:
    printf("tcp server has closed.\r\n");
    for (idx = 0; idx < TCP_SERVER_LISTEN_NUM; idx++) {
        if (cli_fd[idx] != -1) {
            shutdown(cli_fd[idx], SHUT_RD);
            close(cli_fd[idx]);
        }
    }
    if (listen_fd > -1) {
        shutdown(listen_fd, SHUT_RD);
        close(listen_fd);
    }
}

static void softap_tcp_server_task(void *param)
{
    int ret = 0;
    char *ssid = SSID;
    char *password = PASSWORD;
    uint8_t channel = 11, is_hidden = 0;
    wifi_ap_auth_mode_t auth_mode = AUTH_MODE_WPA2_WPA3;

    if (ssid == NULL) {
        printf("ssid can not be NULL!\r\n");
        goto exit;
    }
    if (password && (strlen(password) == 0)) {
        password = NULL;
    }
    if (password == NULL) {
        auth_mode = AUTH_MODE_OPEN;
    }

    /*
    * 1. Start Wi-Fi softap
    */
    printf("Start Wi-Fi softap.\r\n");
    ret = wifi_management_ap_start(ssid, password, channel, auth_mode, is_hidden);
    if (ret != 0) {
        printf("Wi-Fi softap start failed.\r\n");
        goto exit;
    } else {
        printf("SoftAP:%s successfully started!\r\n", ssid);
    }

    /*
    * 2. Start TCP server
    */
    tcp_server_test();

    /*
    * 3. Stop Wi-Fi softap
    */
    wifi_management_ap_stop();

exit:
    printf("the test has ended.\r\n");
    sys_task_delete(NULL);
}

int main(void)
{
    platform_init();

    if (wifi_init()) {
        printf("wifi init failed.\r\n");
    }

    sys_task_create_dynamic((const uint8_t *)"softap tcp server", 4096, OS_TASK_PRIORITY(0), softap_tcp_server_task, NULL);

    sys_os_start();

    for ( ; ; );
}
