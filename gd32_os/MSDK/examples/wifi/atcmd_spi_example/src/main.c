/*!
    \file    main.c
    \brief   Main function of (atcmd over spi) example.

    \version 2023-07-20, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2023, GigaDevice Semiconductor Inc.

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
#include <string.h>
#include "app_cfg.h"
#include "gd32vw55x_platform.h"
#include "gd32vw55x_spi.h"
#include "slist.h"

#include "dbg_print.h"
#include "wrapper_os.h"
#include "spi_master.h"

#if (SPI_ROLE == SPI_ROLE_MASTER)

/* Please set the correct AP information */
#define SSID                            "xiaomi_6004"//"TP-LINK5660_5006"//
#define PASSWORD                        "12345678" // "NULL"

/* Please set the correct TCP server information */
//ip_addr_t MQTT_SERVER_IP = IPADDR4_INIT_BYTES(192, 168, 1, 12);
#define TCP_SERVER_IP                   "192.168.4.230"//"192.168.6.100"//
#define TCP_SERVER_PORT                 5201


/* Please set round number */
#define TEST_ROUND                      1000000
/*========================= USER CONFIGURATION END ====================*/
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
#define ATCMD_MAX_LEN                   128

#define SPI_MASTER_SEND_ARRAY
#define FILE_TOTAL_LEN                  1 * 1024 *1024 //100MBytes
#define FILE_SEGMENT_LEN                1460 //1460Bytes
#define SPI_MAX_RCV_DATA_LEN            (2048)

char atcmd[ATCMD_FIXED_LEN];
char atcmd2[ATCMD_FIXED_LEN];
char atcmd_rsp[256];

char *spi_master_send_array = NULL;
char *exit_passth_str = "+++";
#define SEND_LEN 2048
os_sema_t spi_data_sema = NULL;
os_sema_t spi_slave_ready_sema = NULL;
os_sema_t spi_slave_notify_data_sema = NULL;
os_mutex_t spi_master_send_mutex = NULL;

struct spi_manager_s spi_manager;

os_task_t at_cmd_handle_task_handle;

/* Priority of the ble datatrans task */
/*==========================================================================================*/
static void print_status(void)
{
    int total, used, free, max_used;

    sys_heap_info(&total, &free, &max_used);
    used = total - free;
    max_used = total - max_used;

    app_print("\r\n=================================================\r\n");
    app_print("RTOS HEAP: free=%d used=%d max_used=%d/%d\n\n",
                free, used, max_used, total);

    app_print("TaskName\t\tState\tPri\tStack\tID\tStackBase\r\n");
    app_print("--------------------------------------------------\r\n");
}

static void random_delay(void)
{
    uint32_t rand;

    sys_random_bytes_get(&rand, sizeof(rand));

    rand = rand % 9000 + 1000;

    sys_ms_sleep(rand);
}

static int at_cmd_post_wait_rsp(char *cmd, size_t cmd_len, uint8_t *buf, size_t buf_len, at_cmd_recv_info_t *recv_ack)
{
    at_cmd_send_info_t at_cmd_send_info = {0};
    os_task_t task_handle;

    task_handle = sys_current_task_handle_get();
    if (task_handle == NULL) {
        app_print("task_handle get error\r\n");
        return -1;
    }

    sys_memcpy(&at_cmd_send_info.cmd, cmd, cmd_len);
    at_cmd_send_info.cmd_len = cmd_len;
    at_cmd_send_info.tx_buffer = buf;
    at_cmd_send_info.tx_buffer_len = buf_len;
    at_cmd_send_info.task_handle = task_handle;

    at_cmd_send_info.segment_len = FILE_SEGMENT_LEN;//1460;

    if (sys_task_post(at_cmd_handle_task_handle, &at_cmd_send_info, 0) == OS_ERROR) {
        app_print("queue post error\r\n");
        return -2;
    }

    sys_task_wait(0, recv_ack); // wait forever
    if (recv_ack->status != 0) {
        app_print("at cmd excute error\r\n");
    } else {
        if (strstr(recv_ack->ack, "OK") || strstr(recv_ack->ack, ">")) {
            return 0;
        } else if (strstr(recv_ack->ack, "ERROR")) {
            return -3;
        } else if (recv_ack->ack_size > 0) {
            return 1;
        }
    }

    return 0; // TODO
}

static void spi_master_at_cmd_send_task(void *param)
{
    int ret = 0, loop = 0, i = 0, cmd_len = 0;
    at_cmd_recv_info_t recv_info = {0};

    print_status();

    /**
     * 2. Wait SPI Slave ready
     */
Retry_wifi_slave_ready:
    app_print("Waiting SPI Slave ready\r\n");
    sys_memset(&recv_info, 0, sizeof(at_cmd_recv_info_t));
    ret = at_cmd_post_wait_rsp("AT", 2, NULL, 0, &recv_info);
    if (ret < 0) {
        app_print("SPI Slave not ready (ret %d).\r\n", ret);
        sys_ms_sleep(2000);
        goto Retry_wifi_slave_ready;
    }
    sys_memset(&recv_info, 0, sizeof(at_cmd_recv_info_t));

    /**
     * 3. Start WiFi connect
     */
    app_print("Wi-Fi connect with %s (%s)...\r\n", SSID, PASSWORD);
    sys_memset(atcmd, 0, ATCMD_FIXED_LEN);
    cmd_len = co_snprintf(atcmd, ATCMD_FIXED_LEN, "AT+CWJAP_CUR=\"%s\",\"%s\"", SSID, PASSWORD);
Retry_wifi_connect:
    ret = at_cmd_post_wait_rsp(atcmd, cmd_len, NULL, 0, &recv_info);
    if (ret < 0 || strstr(recv_info.ack, "ERROR")) {
        app_print("Wi-Fi connect failed (ret %d).\r\n", ret);
        sys_ms_sleep(2000);
        goto Retry_wifi_connect;
    }
    sys_memset(&recv_info, 0, sizeof(at_cmd_recv_info_t));
    sys_ms_sleep(100);

    /*
    * 4. Start TCP client
    */
    app_print("Start TCP client.\r\n");
    sys_memset(atcmd, 0, ATCMD_FIXED_LEN);
    cmd_len = co_snprintf(atcmd, ATCMD_FIXED_LEN, "AT+CIPSTART=0,\"TCP\",\"%s\",%d,0",
                            TCP_SERVER_IP, TCP_SERVER_PORT);
Retry_tcp_connect:
    ret = at_cmd_post_wait_rsp(atcmd, cmd_len, NULL, 0, &recv_info);
    if (ret < 0 || strstr(atcmd_rsp, "ERROR")) {
        app_print("TCP connect failed (ret %d).\r\n", ret);
        sys_ms_sleep(3000);
        goto Retry_tcp_connect;
    }
    sys_memset(&recv_info, 0, sizeof(at_cmd_recv_info_t));

    sys_ms_sleep(20);
    /* 5.1 Switch to normal mode */
    sys_memset(atcmd, 0, ATCMD_FIXED_LEN);
    cmd_len = co_snprintf(atcmd, ATCMD_FIXED_LEN, "AT+CIPMODE=0");

    ret = at_cmd_post_wait_rsp(atcmd, cmd_len, NULL, 0, &recv_info);
    if (ret < 0 || strstr(recv_info.ack, "ERROR")) {
        app_print("Switch to normal mode failed (ret %d).\r\n", ret);
    }
    sys_memset(&recv_info, 0, sizeof(at_cmd_recv_info_t));

    sys_ms_sleep(20);

    /* 5.2 Open cipmux */
    sys_memset(atcmd, 0, ATCMD_FIXED_LEN);
    cmd_len = co_snprintf(atcmd, ATCMD_FIXED_LEN, "AT+CIPMUX=1");

    ret = at_cmd_post_wait_rsp(atcmd, cmd_len, NULL, 0, &recv_info);
    if (ret < 0 || strstr(recv_info.ack, "ERROR")) {
        app_print("Open cipmux failed (ret %d).\r\n", ret);
    }
    sys_memset(&recv_info, 0, sizeof(at_cmd_recv_info_t));

    /* 5. Start SPI ATCMD Test */
    for (i = 0; i < TEST_ROUND; i++) {
        sys_ms_sleep(200); //10
#ifdef SPI_MASTER_DEBUG_PRINT
        app_print("===== SPI Master test atcmd [%d] =====\r\n", i);
#endif
        /* 5.1 Send data to tcp server in normal mode */
        sys_memset(atcmd, ATCMD_FIXED_LEN, 0);
#if 1
        cmd_len = co_snprintf(atcmd, ATCMD_FIXED_LEN, "AT+CIPSEND=0,%d", SEND_LEN);
#ifdef SPI_MASTER_DEBUG_PRINT
        app_print("---%s test---\r\n", atcmd);
#endif
        ret = at_cmd_post_wait_rsp(atcmd, cmd_len, (uint8_t *)spi_master_send_array, SEND_LEN, &recv_info);

        if (ret < 0) {
            app_print("Send data in normal mode failed (ret %d).\r\n", ret);
            break;
        }
#else
        /* 5.2 Send data to TCP server in File Transfer mode */
        sys_memset(atcmd, 0, ATCMD_MAX_LEN);
        cmd_len = co_snprintf(atcmd, ATCMD_MAX_LEN, "AT+CIPSDFILE=0,%d,%d",
                                FILE_TOTAL_LEN, FILE_SEGMENT_LEN);
        app_print("---%s test---\r\n", atcmd);
        ret = at_cmd_post_wait_rsp(atcmd, cmd_len, (uint8_t *)spi_master_send_array, FILE_TOTAL_LEN, &recv_info);

        if (ret < 0) {
            app_print("Send data in normal mode failed (ret %d).\r\n", ret);
            //break;
        }
#endif
    }

    /* 6 Close TCP connection */
    app_print("Close TCP connection.\r\n");
    sys_memset(atcmd, ATCMD_MAX_LEN, 0);
    co_snprintf(atcmd, ATCMD_MAX_LEN, "AT+CIPCLOSE=0");
    at_spi_send_cmd_wait_rsp(atcmd, strlen(atcmd), atcmd_rsp, sizeof(atcmd_rsp));


    app_print("=====SPI Test End, PASS: %d, Fail: %d.=====\r\n", i, TEST_ROUND - i);
    sys_task_delete(NULL);
}

static void spi_master_at_cmd_handle_task(void *param)
{
    int ret;
    uint8_t *data = NULL;
    int rsp_len = 0;
    at_cmd_send_info_t at_cmd;
    at_cmd_recv_info_t at_ack;

    while (1) {
        if (sys_task_wait(0, &at_cmd) == OS_OK) {
            sys_memset(&at_ack, 0, sizeof(at_cmd_recv_info_t));

            //sys_us_delay(50);
            sys_ms_sleep(1);
            if (spi_manager.stat != SPI_Master_Idle) {
                app_print("Wrong spi manager state: %d, But Idle is expected\r\n", spi_manager.stat);
                ret = -1;
                goto ack;
            }

            spi_manager.stat = SPI_Master_AT_Sent;
            // app_print("at cmd:%s\r\n", at_cmd.cmd);
            if (strstr(at_cmd.cmd, "AT+CIPSEND") != NULL) {
                if (at_cmd.tx_buffer == NULL || at_cmd.tx_buffer_len == 0 ) {
                    app_print("at cmd:%s, should with payload\r\n", at_cmd.cmd);
                    ret = -1;
                } else {
                    ret = at_spi_send_cmd_wait_rsp(at_cmd.cmd, at_cmd.cmd_len, at_ack.ack, 32);
                    if (!ret && strstr(at_ack.ack, ">") != NULL) {
                        ret = at_spi_send_data_wait_rsp(at_cmd.tx_buffer, at_cmd.tx_buffer_len, at_ack.ack, 32);
                    }
                }
            } else if (strstr(at_cmd.cmd, "AT+CIPRECVDATA") != NULL) {
                ret = at_spi_send_cmd_read_data(at_cmd.cmd, at_cmd.cmd_len, &at_ack);
            } else if (strstr(at_cmd.cmd, "AT+CIPSDFILE") != NULL) {
                if (at_cmd.tx_buffer == NULL || at_cmd.tx_buffer_len == 0) {
                    app_print("at cmd:%s, should with payload\r\n", at_cmd.cmd);
                    ret = -1;
                } else {
                    ret = at_spi_send_cmd_wait_rsp(at_cmd.cmd, at_cmd.cmd_len, at_ack.ack, 32);
                    if (!ret && strstr(at_ack.ack, "OK") != NULL) {
                        ret = at_spi_send_file_wait_rsp(at_cmd.tx_buffer, at_cmd.tx_buffer_len, at_cmd.segment_len, at_ack.ack, 32);
                    }
                }
            } else {
                ret = at_spi_send_cmd_wait_rsp(at_cmd.cmd, at_cmd.cmd_len, at_ack.ack, 32);
            }

            spi_manager.stat = SPI_Master_Idle;

ack:
            if (ret) {
                app_print("at cmd:%s handle error:%d\r\n", at_cmd.cmd, ret);
                at_ack.status = -1;
            } else {
                at_ack.status = 0;
            }
            sys_task_post(at_cmd.task_handle, &at_ack, 0);
        }
    }
}

static void spi_master_tcp_recv_task(void *param)
{
    int ret = 0, i = 0, cmd_len = 0;
    at_cmd_recv_info_t recv_info = {0};

    do {
        for (i = 0; i < TEST_ROUND; i++) {
            sys_sema_down(&spi_slave_notify_data_sema, 0);
#ifdef SPI_MASTER_DEBUG_PRINT
            app_print("===== TCP Rcv Data [%d] =====\r\n", i);
#endif
            /* 5.1 Switch to normal mode */
            sys_memset(atcmd2, 0, ATCMD_FIXED_LEN);
            cmd_len = co_snprintf(atcmd2, ATCMD_FIXED_LEN, "AT+CIPRECVDATA=%d", SPI_MAX_RCV_DATA_LEN);
            sys_memset(&recv_info, 0, sizeof(at_cmd_recv_info_t));

#ifdef SPI_MASTER_DEBUG_PRINT
            app_print("---%s test---\r\n", atcmd2);
#endif
            ret = at_cmd_post_wait_rsp(atcmd2, cmd_len, NULL, 0, &recv_info);
            if (ret < 0) {
                app_print("AT+CIPRECVDATA failed (ret %d).\r\n", ret);
                break;
            } else {
                if (recv_info.rx_buffer != NULL) {
                    app_print("recv data, len:%d\r\n", recv_info.rx_buffer_len);
                    sys_mfree(recv_info.rx_buffer);
                } else {
                    if (recv_info.status == -1)
                        app_print("cmd:%s excute error\r\n", atcmd2);
                }
            }
        }
    } while(1);
    sys_task_delete(NULL);
}

static void start_task(void *param)
{
    /**
     * 1. Init SPI Master
     */
    spi_master_demo_init();
    char chac = '!';
    spi_master_send_array = (char *)sys_malloc(SEND_LEN);
    for (int i = 0; i < SEND_LEN; i++) {
        if (chac == '~')
            chac = '!';
        spi_master_send_array[i] = chac++;
    }

    spi_enable();

    if (sys_sema_init(&spi_slave_ready_sema, 0)) {
        goto Exit;
    }

    if (sys_mutex_init(&spi_master_send_mutex)) {
        goto Exit;
    }

    if (sys_sema_init(&spi_slave_notify_data_sema, 0)) {
        goto Exit;
    }

    // task 2 tcp recv data
    at_cmd_handle_task_handle = sys_task_create(NULL, (const uint8_t *)"at handle task", NULL,
                                    512, 10, sizeof(at_cmd_send_info_t), OS_TASK_PRIORITY(2),
                                    (task_func_t)spi_master_at_cmd_handle_task, NULL);
    if (at_cmd_handle_task_handle == NULL) {
        goto Exit;
    }

    if (sys_task_create(NULL, (const uint8_t *)"at cmd task", NULL,
                    512, 2, sizeof(at_cmd_recv_info_t), OS_TASK_PRIORITY(1),
                    (task_func_t)spi_master_at_cmd_send_task, NULL) == NULL) {
        goto Exit;
    }

    if (sys_task_create(NULL, (const uint8_t *)"cip recv task", NULL,
                    512, 2, sizeof(at_cmd_recv_info_t), OS_TASK_PRIORITY(1),
                    (task_func_t)spi_master_tcp_recv_task, NULL) == NULL) {
        goto Exit;
    }

    sys_task_delete(NULL);
    return;
Exit:
    if (spi_slave_ready_sema != NULL)
        sys_sema_free(&spi_slave_ready_sema);

    if (spi_master_send_mutex != NULL)
        sys_mutex_free(&spi_master_send_mutex);

    sys_task_delete(NULL);
}


int main(void)
{
    platform_init();

#if (SPI_ROLE == SPI_ROLE_MASTER)
    if (sys_task_create_dynamic((const uint8_t *)"start task", 512, OS_TASK_PRIORITY(0),
            start_task, NULL) == NULL) {
        return -2;
    }
#endif
    sys_os_start();

    for ( ; ; );
}

#endif
