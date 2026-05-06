/*!
    \file    atcmd_tcpip.c
    \brief   AT command TCPIP part for GD32VW55x SDK

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

#include "slist.h"

static cip_info_t cip_info;
static int cip_task_started = 0;
static int cip_task_terminate = 0;

static int multi_connection_enable = 0;

#ifndef MIN
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#endif

typedef struct _passth_tx_buf {
    char *buf;
    uint32_t size;
    uint32_t writeptr;
    uint32_t readptr;
} passth_tx_buf_t;

typedef struct _cip_passth_info {
    int passth_fd_idx;
    // Ping-Pong Buffer
    passth_tx_buf_t passth_buf[2];

    os_timer_t passth_timer;
    volatile uint8_t at_tx_passth_timeout;
    volatile uint8_t terminate_send_passth;
} cip_passth_info_t;
static cip_passth_info_t cip_passth_info;

int local_sock_send = -1;

#ifdef CONFIG_ATCMD_SPI
typedef struct _cip_file_transfer_info {
    int fd_idx;
    uint32_t file_len;
    uint32_t segment_len;
    uint32_t remaining_len;
    uint32_t cur_len; // current processed len
    uint8_t *s_buf;

    volatile uint8_t terminate;
} cip_file_transfer_info_t;
static cip_file_transfer_info_t cip_file_trans_info;
const char *ack = "ACK";
const char *nak = "NAK";
#endif

/*!
    \brief      initialize structure of tcpip information
    \param[in]  none
    \param[out] none
    \retval     none
*/
void cip_info_init(void)
{
    int i;

    cip_task_terminate = 0;

    sys_memset(&cip_info, 0, sizeof(cip_info));

#ifdef CONFIG_ATCMD_SPI
    if (cip_info.list_lock == NULL) {
        list_init(&cip_info.recv_data_list);
        sys_mutex_init(&cip_info.list_lock);
    }
#endif

    cip_info.trans_intvl = CIP_TRANSFER_INTERVAL_DEFAULT; //ms
    cip_info.local_srv_fd = -1;
    for (i = 0; i < MAX_CLIENT_NUM; i++) {
        cip_info.cli[i].fd = -1;
    }
    return;
}

/*!
    \brief      allocate storage space for tcpip information
    \param[in]  none
    \param[out] none
    \retval     the index of the array used to store tcpip information
*/
static int cip_info_cli_alloc(void)
{
    int i;

    if (cip_info.cli_num >= MAX_CLIENT_NUM) {
        return -1;
    }
    for (i = 0; i < MAX_CLIENT_NUM; i++) {
        if (cip_info.cli[i].fd < 0)
            return i;
    }
    return -1;
}

/*!
    \brief      Check if the specified array that stores tcpip information is free
    \param[in]  con_id: index of the client array
    \retval     1 if the array is free (not allocated), 0 if occupied
*/
static int cip_info_cli_is_free(int con_id)
{
    if (cip_info.cli[con_id].fd < 0) {
        return 1;
    }
    return 0;
}

/*!
    \brief      store tcpip information
    \param[in]  con_id: the index of the array used to store tcpip information
    \param[in]  fd: file descriptor
    \param[in]  type: the type of the client
    \param[in]  role: the role of the client
    \param[in]  remote_ip: remote ip
    \param[in]  remote_port: remote port
    \param[in]  local_port: local port
    \param[out] none
    \retval     the location of the array used to store tcpip information
*/
static int cip_info_cli_store(int con_id, int fd, char *type, uint8_t role,
                            uint32_t remote_ip, uint16_t remote_port, uint16_t local_port)
{
    int idx = -1;

    if (con_id < 0) {
        idx = cip_info_cli_alloc();
    } else {
        if (cip_info_cli_is_free(con_id)) {
            idx = con_id;
        }
    }

    if ((idx < 0) || (fd < 0))
        return -1;
    cip_info.cli[idx].fd = fd;
    if (strncmp(type, "TCP", 3) == 0)
        cip_info.cli[idx].type = CIP_TYPE_TCP;
    else
        cip_info.cli[idx].type = CIP_TYPE_UDP;
    cip_info.cli[idx].role = role;
    cip_info.cli[idx].stop_flag = 0;
    cip_info.cli[idx].remote_ip = remote_ip;
    cip_info.cli[idx].remote_port = remote_port;
    cip_info.cli[idx].local_port = local_port;

    cip_info.cli_num++;

    return idx;
}

/*!
    \brief      free the specified array that stores tcpip information
    \param[in]  index: the index of the array used to store tcpip information
    \param[out] none
    \retval     none
*/
static void cip_info_cli_free(int index)
{
    if ((index >= 0) && (index < MAX_CLIENT_NUM)) {
        if (cip_info.cli[index].fd != -1) {
    #ifdef CONFIG_ATCMD_SPI
            // free recv data list node for this fd
            struct recv_data_node *p_item, *p_next, *p_prev;
            sys_mutex_get(&cip_info.list_lock);
            p_prev = NULL;
            p_item = (struct recv_data_node *)list_pick(&cip_info.recv_data_list);
            while (p_item != NULL) {
                p_next = (struct recv_data_node *)list_next(&p_item->list_hdr);
                if (p_item->fd == cip_info.cli[index].fd) {
                    if (p_item->data && (p_item->data_len > 0)) {
                        sys_mfree(p_item->data);
                    }
                    list_remove(&cip_info.recv_data_list, (struct list_hdr *)p_prev, (struct list_hdr *)p_item);
                    cip_info.list_data_count--;
                    sys_mfree(p_item);
                } else {
                    p_prev = p_item;
                }
                p_item = p_next;
            }
            sys_mutex_put(&cip_info.list_lock);
    #endif /* CONFIG_ATCMD_SPI */
            sys_memset(&cip_info.cli[index], 0, sizeof(client_info_t));
            cip_info.cli[index].fd = -1;
            cip_info.cli_num--;
        }
    }
}

/*!
    \brief      find the specified array that stores tcpip information
    \param[in]  fd: file descriptor
    \param[out] none
    \retval     index of the array used to store tcpip information
*/
static int cip_info_cli_find(int fd)
{
    int i;

    for (i = 0; i < MAX_CLIENT_NUM; i++) {
        if (cip_info.cli[i].fd == fd)
            return i;
    }
    return -1;
}

/*!
    \brief      get the number of valid tcp/udp connection
    \param[in]  none
    \param[out] none
    \retval     the number of valid tcp/udp connection
*/
static int cip_info_valid_fd_cnt_get(void)
{
    int i, cnt = 0;

    for (i = 0; i < MAX_CLIENT_NUM; i++) {
        if (cip_info.cli[i].fd >= 0)
            cnt++;
    }
    return cnt;
}

/*!
    \brief      reset tcpip information
    \param[in]  none
    \param[out] none
    \retval     none
*/
void cip_info_reset(void)
{
    int i, fd;

    for (i = 0; i < MAX_CLIENT_NUM; i++) {
        if (cip_info.cli[i].fd >= 0) {
            fd = cip_info.cli[i].fd;
            cip_info_cli_free(i);
            close(fd);
        }
    }
    if (cip_info.local_srv_fd >= 0) {
        fd = cip_info.local_srv_fd;
        cip_info.local_srv_fd = -1;
        cip_info.local_srv_port = 0;
        close(fd);
    }
    cip_task_terminate = 1;
}

/*!
    \brief      close all socket
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void at_cip_close_all(void)
{
    cip_task_terminate = 1;
    while (sys_task_exist((const uint8_t *)"Cip Rcv")) {
        sys_ms_sleep(1);
    }
    cip_task_started = 0;
}

/*!
    \brief      start a tcp client
    \param[in]  con_id: the index of the array used to store tcpip information
    \param[in]  srv_ip: server IP address
    \param[in]  srv_port: server port
    \param[in]  bkeep_alive: time of keep alive
    \param[out] none
    \retval     the result of operation
*/
static int tcp_client_start(int con_id, char *srv_ip, uint16_t srv_port, uint32_t bkeep_alive)
{
    struct sockaddr_in saddr;
    socklen_t len = sizeof(saddr);
    uint32_t nodelay = 0;
    uint32_t keepalive = 1;
    uint32_t keepidle = 60; //in seconds
    uint32_t keepcnt = 3;
    uint32_t keepinval = 1; //in seconds
    int send_timeout;
    int fd, ret, idx;
    uint32_t srv_ip_int = inet_addr(srv_ip);

#ifndef CONFIG_ATCMD_SPI
    if (cip_info.trans_mode == CIP_TRANS_MODE_PASSTHROUGH &&
        (cip_info_valid_fd_cnt_get() > 0 || cip_info.local_srv_fd >= 0))
        return -1;
#endif

    cip_info.tcp_udp_start_process_num ++;

    sys_memset(&saddr, 0, len);
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(srv_port);
    saddr.sin_addr.s_addr = srv_ip_int;

    /* creating a TCP socket */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        AT_TRACE("Create tcp client socket fd error!\r\n");
        ret = -1;
        goto Exit;
    }
    nodelay = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
            (const char *) &nodelay, sizeof( nodelay ) );
    if (bkeep_alive != 0) {
        keepalive = 1;
        keepidle = bkeep_alive;
        setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(uint32_t));
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(uint32_t));
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepinval, sizeof(uint32_t));
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(uint32_t));
    }
    send_timeout = 1000;
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const void*)&send_timeout, sizeof(send_timeout));

    AT_TRACE("TCP: server IP=%s port=%d.\r\n", srv_ip, srv_port);

    /* connecting to TCP server */
    ret = connect(fd, (struct sockaddr *)&saddr, len);
    if (ret < 0) {
        AT_TRACE("Tcp client connect server error!\r\n");
        ret = -2;
        goto Exit;
    }
    /* Get local port */
    sys_memset(&saddr, 0, len);
    getsockname(fd, (struct sockaddr *)&saddr, &len);
    /* save client info */
    idx = cip_info_cli_store(con_id, fd, "TCP", CIP_ROLE_CLIENT,
                            srv_ip_int, srv_port, ntohs(saddr.sin_port));
    if (idx < 0) {
        AT_TRACE("Client num reached the maximum!\r\n");
        ret = -3;
        goto Exit;
    }
    AT_TRACE("TCP: create socket %d.\r\n", fd);
    cip_passth_info.passth_fd_idx = idx;

    cip_info.tcp_udp_start_process_num --;

    return 0;

Exit:
    cip_info.tcp_udp_start_process_num --;
    if (fd >= 0)
        close(fd);
    return ret;
}

/*!
    \brief      send tcp packet
    \param[in]  fd: the socket of the tcp client
    \param[in]  tx_len: length of tcp packet to be sent
    \param[out] none
    \retval     the result of operation
*/
static int at_tcp_send(int fd, uint32_t tx_len)
{
    char *tx_buf = NULL;
    int cnt = 0, dma_ret = 0;
    int retry_cnt = 10;
    struct at_local_tcp_send send_data;

    /* CRITICAL: Check if cip_recv_task is running and local socket is ready */
    if (local_sock_send < 0 || cip_task_started == 0) {
        AT_TRACE("!!!ERR: local_sock=%d, task_started=%d\r\n", local_sock_send, cip_task_started);
        return -1;
    }

    tx_buf = sys_zalloc(tx_len);
    if (NULL == tx_buf) {
        AT_TRACE("Allocate client buffer failed (len = %u).\r\n", tx_len);
        return -1;
    }

    AT_RSP_DIRECT(">\r\n", 3);

    // Block here to wait dma receive done
    // AT_TRACE("3Bef s%d(3),d%d\r\n", spi_manager.stat, spi_manager.direction);
    dma_ret = at_hw_dma_receive((uint32_t)tx_buf, tx_len);
    // AT_TRACE("3Aft s%d,cnt=%d,ret=%d\r\n", spi_manager.stat, tx_len, dma_ret);

    if (dma_ret != 0) {
        AT_TRACE("!!!DMA_RCV_FAIL: ret=%d, abort send\r\n", dma_ret);
        sys_mfree(tx_buf);
        return -1;
    }

    if (local_sock_send < 0 || cip_task_started == 0) {
        AT_TRACE("!!!ERR_AFTER_DMA: local_sock=%d, task_started=%d\r\n", local_sock_send, cip_task_started);
        sys_mfree(tx_buf);
        return -2;
    }

    send_data.event_id = AT_LOCAL_TCP_SEND_EVENT;
    send_data.sock_fd = fd;
    send_data.send_data_addr = (uint32_t)tx_buf;
    send_data.send_data_len = tx_len;

Retry:
    if (local_sock_send < 0) {
        AT_TRACE("!!!ERR_SEND: local_sock closed during retry\r\n");
        sys_mfree(tx_buf);
        return -2;
    }

    cnt = sendto(local_sock_send, (void *)&send_data, sizeof(send_data), 0, NULL, 0);
    if (cnt <= 0) {
        if ((errno == EAGAIN || errno == ENOMEM) && retry_cnt > 0) {
            sys_ms_sleep(20);
            retry_cnt--;
            AT_TRACE("local socket. errno:%d retry_cnt:%d!\r\n", errno, retry_cnt);
            goto Retry;
        }
        sys_mfree(tx_buf);
        AT_TRACE("local socket send tcp fail. %d, local_sock_send:%d!\r\n", errno, local_sock_send);
        return -2;
    }

    return cnt;
}

#ifndef CONFIG_ATCMD_SPI
static void cip_passth_tx_buf_deinit(uint8_t buf_idx)
{
    if (cip_passth_info.passth_buf[buf_idx].buf)
        sys_mfree(cip_passth_info.passth_buf[buf_idx].buf);

    cip_passth_info.passth_buf[buf_idx].buf= NULL;
    cip_passth_info.passth_buf[buf_idx].size = 0;

    cip_passth_info.passth_buf[buf_idx].writeptr = 0;
    cip_passth_info.passth_buf[buf_idx].readptr = 0;
}

static int cip_passth_tx_buf_init(uint8_t buf_idx)
{
    if (cip_passth_info.passth_buf[buf_idx].buf == NULL) {
        cip_passth_info.passth_buf[buf_idx].buf = (char *)sys_zalloc(PASSTH_TX_BUF_LEN);
        if (cip_passth_info.passth_buf[buf_idx].buf == NULL)
            return -1;
    }

    cip_passth_info.passth_buf[buf_idx].size = PASSTH_TX_BUF_LEN;
    cip_passth_info.passth_buf[buf_idx].writeptr = 0;
    cip_passth_info.passth_buf[buf_idx].readptr = 0;

    return 0;
}

static void cip_passth_info_deinit(void)
{
    if (cip_passth_info.passth_timer) {
        sys_timer_delete(&(cip_passth_info.passth_timer));
    }

    cip_passth_info.terminate_send_passth = 0;
    cip_passth_info.at_tx_passth_timeout = 0;

    for (int i = 0; i < 2; i++) {
        cip_passth_tx_buf_deinit(i);
    }
}

static int cip_passth_info_init(void)
{
    cip_passth_info.terminate_send_passth = 0;
    cip_passth_info.at_tx_passth_timeout = 0;

    for (int i = 0; i < 2; i++) {
        if (cip_passth_tx_buf_init(i) < 0) {
            goto fail;
        }
    }

    return 0;

fail:
    cip_passth_info_deinit();
    return -1;
}

static int at_passth_send_data(int fd, uint8_t flush, uint8_t type, uint8_t buf_idx)
{
    passth_tx_buf_t *passth_tx_buf = &(cip_passth_info.passth_buf[buf_idx]);
    char *start_addr = passth_tx_buf->buf + passth_tx_buf->readptr;
    int ret = 0, sent_cnt = 0, idx = 0;
    struct sockaddr_in saddr;
    int remaining_cnt = passth_tx_buf->writeptr - passth_tx_buf->readptr;
    void *send_data = NULL;
    size_t size;
    struct at_local_tcp_send send_data_tcp;
    struct at_local_udp_send send_data_udp;

    if (fd < 0 || ((type != CIP_TYPE_TCP) && (type != CIP_TYPE_UDP)))
        return -1;

    if (remaining_cnt == 0)
        return 0;

    if (type == CIP_TYPE_UDP) {
        idx = cip_info_cli_find(fd);
        if (idx == -1)
            return -1;
        sys_memset(&saddr, 0, sizeof(struct sockaddr_in));
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(cip_info.cli[idx].remote_port);
        saddr.sin_addr.s_addr = cip_info.cli[idx].remote_ip;
    }

    if ((remaining_cnt == strlen(PASSTH_TERMINATE_STR) &&
            strncmp(start_addr, PASSTH_TERMINATE_STR, strlen(PASSTH_TERMINATE_STR)) == 0)
        //|| (remaining_cnt == 5 && strncmp(passth_tx_buf->buf, PASSTH_TERMINATE_STR"\r\n", 5) == 0)
        ) {
        cip_passth_info.terminate_send_passth = 1;
        return 0;
    }

    while (remaining_cnt > 0) {
        if (remaining_cnt >= PASSTH_START_TRANSFER_LEN) {
            sent_cnt = PASSTH_START_TRANSFER_LEN;
        } else {
            if (flush == 1)
                sent_cnt = remaining_cnt;
            else
                return 0;
        }

        if (type == CIP_TYPE_TCP) {
            send_data_tcp.event_id = AT_LOCAL_TCP_SEND_EVENT;
            send_data_tcp.sock_fd = fd;
            send_data_tcp.send_data_addr = (uint32_t)start_addr;
            send_data_tcp.send_data_len = sent_cnt;
            send_data = (void *)&send_data_tcp;
            size = sizeof(send_data_tcp);
        } else {
            send_data_udp.event_id = AT_LOCAL_UDP_SEND_EVENT;
            send_data_udp.sock_fd = fd;
            send_data_udp.send_data_addr = (uint32_t)start_addr;
            send_data_udp.send_data_len = sent_cnt;
            sys_memcpy(&(send_data_udp.to), &saddr, sizeof(struct sockaddr_in));
            send_data_udp.tolen = sizeof(struct sockaddr_in);
            send_data = (void *)&send_data_udp;
            size = sizeof(send_data_udp);
        }
Retry:
        ret = sendto(local_sock_send, send_data, size, 0, NULL, 0);
        if (ret <= 0) {
            if (errno == EAGAIN || errno == ENOMEM) {
                sys_ms_sleep(1);
                goto Retry;
            }
            AT_TRACE("send error:%d\r\n", errno);
            goto exit;
        }

        passth_tx_buf->readptr += sent_cnt;
        // AT_TRACE("Sendout: %d OK,w:%d,r:%d,s:%d,f=%d,t=%d\r\n", ret, passth_tx_buf->writeptr, passth_tx_buf->readptr, sent_cnt, flush, cip_passth_info.at_tx_passth_timeout);
        start_addr += sent_cnt;
        remaining_cnt = remaining_cnt - sent_cnt;
    }

    return 0;
exit:
    cip_passth_info.terminate_send_passth = 1;
    cip_passth_info.passth_fd_idx = -1;
    return -1;
}

static void at_tx_passth_timeout_cb( void *ptmr, void *p_arg )
{
    cip_passth_info.at_tx_passth_timeout = 1;
}

uint32_t cur_dma_received_num = 0;
volatile uint8_t uart_rx_idle_flag = 0;
volatile uint32_t dma_rx_ftf_cnt = 0;
static void at_uart_rx_idle_irq_hdl(uint32_t usart_periph)
{
    uint32_t dma_channel;
    uint32_t size = cip_passth_info.passth_buf[0].size;

    if (RESET != usart_interrupt_flag_get(usart_periph, USART_INT_FLAG_IDLE)) {
        usart_interrupt_flag_clear(usart_periph, USART_INT_FLAG_IDLE);

        switch (usart_periph) {
        case USART0:
            dma_channel = DMA_CH2;
            break;
        case UART1:
            dma_channel = DMA_CH0;
            break;
        case UART2:
        default:
            dma_channel = DMA_CH5;
            break;
        }

        cur_dma_received_num = size - (dma_transfer_number_get(dma_channel));
        if ((cur_dma_received_num == size) || (cur_dma_received_num == 0)) {
            return;
        }
        uart_rx_idle_flag = 1;
        // AT_TRACE("IDLE receive, cur_dma_received_num=%d.\r\n", cur_dma_received_num);
    }
}

static int at_hw_passth_send(int fd, uint8_t type)
{
    passth_tx_buf_t *passth_tx_buf[2];
    int passth_timeout = 0, ret;
    uint8_t buf_idx = 0;
    uint8_t last_buf_idx = 0;
    uint32_t last_dma_received_num = 0;

    if (cip_passth_info_init()) {
        AT_RSP_DIRECT("ERROR\r\n", 7);
        return -1;
    }

    passth_tx_buf[0] = &(cip_passth_info.passth_buf[0]);
    passth_tx_buf[1] = &(cip_passth_info.passth_buf[1]);

    if (cip_info.trans_intvl != 0) {
        passth_timeout = cip_info.trans_intvl;

        sys_timer_init(&(cip_passth_info.passth_timer), (const uint8_t *)("passth_intvl_timer"),
            passth_timeout, 0, at_tx_passth_timeout_cb, NULL);
    }

#ifndef SUPER_UART_DMA_RX
    at_hw_dma_receive_config();

    at_hw_dma_receive_start((uint32_t)(passth_tx_buf[0]->buf), (uint32_t)(passth_tx_buf[1]->buf), passth_tx_buf[0]->size);

    uart_irq_callback_unregister(at_uart_conf.usart_periph);
    uart_irq_callback_register(at_uart_conf.usart_periph, at_uart_rx_idle_irq_hdl);

    while (RESET == usart_flag_get(at_uart_conf.usart_periph, USART_FLAG_IDLE)) {
    }
    usart_flag_clear(at_uart_conf.usart_periph, USART_FLAG_IDLE);
    usart_interrupt_enable(at_uart_conf.usart_periph, USART_INT_IDLE);

    dma_rx_ftf_cnt = 0;
    uart_rx_idle_flag = 0;

    while (cip_passth_info.terminate_send_passth != 1) {
        ret = sys_sema_down(&at_hw_dma_sema, 1); // wait 1ms
        if (ret == OS_OK) {
            buf_idx = dma_rx_ftf_cnt % 2;
            dma_rx_ftf_cnt++;
            passth_tx_buf[buf_idx]->writeptr = passth_tx_buf[buf_idx]->size;
            // AT_TRACE("MAX, b:%u, w:%d, r:%d\r\n", buf_idx, passth_tx_buf[buf_idx]->writeptr, passth_tx_buf[buf_idx]->readptr);
            if (passth_tx_buf[buf_idx]->writeptr > passth_tx_buf[buf_idx]->readptr) {
                at_passth_send_data(fd, 1, type, buf_idx);
            }
            passth_tx_buf[buf_idx]->writeptr = 0;
            passth_tx_buf[buf_idx]->readptr = 0;
            continue;
        }

        if (cip_passth_info.at_tx_passth_timeout == 1) {
            buf_idx = last_buf_idx;
            if (buf_idx != (dma_rx_ftf_cnt % 2)) {
                cip_passth_info.at_tx_passth_timeout = 0;
                continue;
            }
            passth_tx_buf[buf_idx]->writeptr = last_dma_received_num;
            // AT_TRACE("Timeout, b:%u, w:%u, r:%u, c=%u\r\n", buf_idx, passth_tx_buf[buf_idx]->writeptr, passth_tx_buf[buf_idx]->readptr, last_dma_received_num);
            if (passth_tx_buf[buf_idx]->writeptr > passth_tx_buf[buf_idx]->readptr) {
                at_passth_send_data(fd, 1, type, buf_idx);
            }
            cip_passth_info.at_tx_passth_timeout = 0;
            continue;
        }

        if (1 == uart_rx_idle_flag) {
            buf_idx = dma_rx_ftf_cnt % 2;
            passth_tx_buf[buf_idx]->writeptr = cur_dma_received_num;
            last_buf_idx = buf_idx;
            last_dma_received_num = cur_dma_received_num;

            if (cip_info.trans_intvl == 0) {
                // AT_TRACE("Receive, b:%u, w:%u, r:%u, c=%u\r\n", buf_idx, passth_tx_buf[buf_idx]->writeptr, passth_tx_buf[buf_idx]->readptr, cur_dma_received_num);
                at_passth_send_data(fd, 1, type, buf_idx);
            } else {
                // AT_TRACE("Wait, b:%u, w:%u, r:%u\r\n", buf_idx, passth_tx_buf[buf_idx]->writeptr, passth_tx_buf[buf_idx]->readptr);
                if ((passth_tx_buf[buf_idx]->writeptr - passth_tx_buf[buf_idx]->readptr) >= PASSTH_START_TRANSFER_LEN) {
                    at_passth_send_data(fd, 0, type, buf_idx);
                } else {
                    if (sys_timer_pending(&(cip_passth_info.passth_timer)) == 0) {
                        sys_timer_start(&(cip_passth_info.passth_timer), 0);
                    }
                }
            }
            uart_rx_idle_flag = 0;
        }
    }

//    AT_TRACE("PassThrough mode exit...\r\n");
    at_hw_dma_receive_stop();
    usart_interrupt_disable(at_uart_conf.usart_periph, USART_INT_IDLE);
    uart_irq_callback_unregister(at_uart_conf.usart_periph);
    uart_irq_callback_register(at_uart_conf.usart_periph, at_uart_rx_irq_hdl);
    at_hw_irq_receive_config();
#else
    dma_rx_ftf_cnt = 0;
    uart_rx_idle_flag = 0;
    passth_tx_buf[0]->readptr = 0;
    passth_tx_buf[0]->writeptr = 0;

    while (cip_passth_info.terminate_send_passth != 1) {
        passth_tx_buf[0]->writeptr = at_hw_dma_receive_start((uint32_t)(passth_tx_buf[0]->buf), PASSTH_START_TRANSFER_LEN, cip_info.trans_intvl);

        if (passth_tx_buf[0]->writeptr == 0) {
            continue;
        }
        else {
            at_passth_send_data(fd, 1, type, 0);
            passth_tx_buf[0]->readptr = 0;
            passth_tx_buf[0]->writeptr = 0;
        }
    }
    at_hw_dma_receive_stop();
#endif
    cip_passth_info_deinit();
    return 0;
}
#endif


#ifdef CONFIG_ATCMD_SPI
static int cip_file_transfer_info_init(int idx, uint32_t file_len, uint32_t segment_len)
{
    uint8_t *tx_buf;

    if (idx < 0 || file_len == 0 || segment_len == 0) {
        return -1;
    }

    tx_buf = sys_malloc(segment_len + FILE_SEGMENT_CRC_LEN);
    if (tx_buf == NULL) {
        return -2;
    }
    sys_memset((void *)&cip_file_trans_info, 0, sizeof(cip_file_trans_info));
    cip_file_trans_info.fd_idx = idx;
    cip_file_trans_info.file_len = file_len;
    cip_file_trans_info.segment_len = segment_len;
    cip_file_trans_info.remaining_len = file_len;
    cip_file_trans_info.s_buf = tx_buf;

    return 0;
}

static void cip_file_transfer_info_deinit(void)
{
    if (cip_file_trans_info.s_buf)
        sys_mfree(cip_file_trans_info.s_buf);

    sys_memset((void *)&cip_file_trans_info, 0, sizeof(cip_file_trans_info));
    cip_file_trans_info.fd_idx = -1;
    cip_file_trans_info.terminate = 1;
}

static int at_file_send_data(int fd_idx, uint8_t *tx_buf, int tx_len)
{
    int fd, type;
    int ret = 0, sent_cnt = 0, retry_cnt = 3;
    struct sockaddr_in saddr;

    if (fd_idx < 0 || tx_len <= 0)
        return -1;


    fd = cip_info_cli_find(fd_idx);
    if (fd == -1)
        return -1;

    type = cip_info.cli[fd_idx].type;
    if (type == CIP_TYPE_UDP) {
        sys_memset(&saddr, 0, sizeof(struct sockaddr_in));
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(cip_info.cli[fd_idx].remote_port);
        saddr.sin_addr.s_addr = cip_info.cli[fd_idx].remote_ip;
    }

Retry:
    if (type == CIP_TYPE_TCP)
        ret = send(fd, (void *)tx_buf, tx_len, 0);
    else
        ret = sendto(fd, (void *)tx_buf, tx_len, 0, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));

    if (ret <= 0) {
        if (errno == EAGAIN || errno == ENOMEM) {
            if (retry_cnt > 0) {
                retry_cnt--;
                goto Retry;
            }
        }
        //AT_TRACE("send to fail, ret=%d\r\n", ret);
    }

    return ret;
}

static int at_send_file(int fd_idx, uint32_t file_len, uint32_t segment_len)
{
    uint8_t *tx_buf = cip_file_trans_info.s_buf;
    uint32_t remaining_len = file_len, real_len, checksum, len_align;
    __IO uint32_t read_data;
    int loop = file_len/segment_len + 1;
    int remain;

    rcu_periph_clock_enable(RCU_CRC);

    while(remaining_len > 0 && (cip_file_trans_info.terminate == 0)) {
        real_len = min(segment_len, remaining_len);
        AT_TRACE("Waiting the %dth data\r\n", loop);

        spi_manager_state_set(SPI_Slave_File_Recv);
        spi_manager.direction = SPI_Slave_RX_Dir;
        at_hw_dma_receive((uint32_t)tx_buf, real_len + FILE_SEGMENT_CRC_LEN);
        loop--;

        remain = real_len & 0x03;
        len_align = real_len - remain;
        crc_data_register_reset();
        checksum = crc_block_data_calculate((uint32_t *)tx_buf, real_len / 4);
        if (remain) {
            read_data = *(uint32_t *)(tx_buf + len_align);
            read_data = ((read_data  << (8 * (4 - remain))) >> (8 * (4 - remain)));
            checksum = crc_single_data_calculate(read_data);
        }

        if (checksum == *(uint32_t *)(tx_buf + real_len)) {
            AT_TRACE("CRC Verify OK, %dth\r\n", loop);
            at_file_send_data(fd_idx, tx_buf, real_len);
            if (remaining_len == real_len)
                spi_manager_state_set(SPI_Slave_File_Done);
            at_hw_send((char *)ack, 3);
        } else {
            AT_TRACE("CRC Verify fail,  checksum=0x%x vs 0x%x\r\n",
                    checksum, *(uint32_t *)(tx_buf + real_len));
            at_hw_send((char *)nak, 3);
            continue;
        }
        AT_TRACE("Done, %d\r\n", loop);
        sys_memset(tx_buf, 0, segment_len + FILE_SEGMENT_CRC_LEN);
        remaining_len -= real_len;
    }

    cip_file_trans_info.terminate = 1;

    AT_TRACE("File Transfer Complete...\r\n");
//    at_hw_dma_receive_stop();
//    at_hw_irq_receive_config();

    cip_file_transfer_info_deinit();
    rcu_periph_clock_disable(RCU_CRC);

    return 0;
}
#endif
/*!
    \brief      start a udp client
    \param[in]  con_id: the index of the array used to store tcpip information
    \param[in]  srv_ip: server IP address
    \param[in]  srv_port: server port
    \param[in]  local_port: client local port
    \param[out] none
    \retval     the result of operation
*/
static int udp_client_start(int con_id, char *srv_ip, uint16_t srv_port, uint16_t local_port)
{
    struct sockaddr_in saddr;
    socklen_t len = sizeof(saddr);
    int reuse = 1;
    int fd, ret;

#ifndef CONFIG_ATCMD_SPI
    if (cip_info.trans_mode == CIP_TRANS_MODE_PASSTHROUGH &&
            (cip_info_valid_fd_cnt_get() > 0 || cip_info.local_srv_fd >= 0))
        return -1;
#endif

    cip_info.tcp_udp_start_process_num ++;

    /* creating a UDP socket */
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        AT_TRACE("Create udp client socket fd error!\r\n");
        ret = -1;
        goto Exit;
    }
    setsockopt(fd , SOL_SOCKET, SO_REUSEADDR,
            (const char *)&reuse, sizeof(reuse));

    sys_memset((char *)&saddr, 0, len);
    saddr.sin_family      = AF_INET;
    saddr.sin_len         = len;
    saddr.sin_port        = htons(local_port);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* binding the UDP socket to a random port */
    ret = bind(fd, (struct sockaddr *)&saddr, len);
    if (ret < 0) {
        AT_TRACE("Bind udp server socket fd error!\r\n");
        goto Exit;
    }
    /* Get local port */
    sys_memset(&saddr, 0, len);
    getsockname(fd, (struct sockaddr *)&saddr, &len);
    //AT_TRACE("UDP local port %d\r\n", ntohs(saddr.sin_port));
    /* save client info */
    ret = cip_info_cli_store(con_id, fd, "UDP", CIP_ROLE_CLIENT,
                            inet_addr(srv_ip), srv_port, ntohs(saddr.sin_port));
    if (ret < 0) {
        AT_TRACE("Client num reached the maximum!\r\n");
        ret = -2;
        goto Exit;
    }
    AT_TRACE("UDP: create socket %d.\r\n", fd);

    if (local_port > 0)
        cip_passth_info.passth_fd_idx = ret; // local port is specified
    else
        cip_passth_info.passth_fd_idx = -1;

    cip_info.tcp_udp_start_process_num --;
    return 0;

Exit:
    cip_info.tcp_udp_start_process_num --;
    if (fd >= 0)
        close(fd);
    return ret;
}

/*!
    \brief      send udp packet
    \param[in]  fd: the socket of the udp client
    \param[in]  tx_len: length of tcp packet to be sent
    \param[in]  srv_ip: server ip
    \param[in]  srv_port: server port
    \param[out] none
    \retval     the result of operation
*/
static int at_udp_send(int fd, uint32_t tx_len, char *srv_ip, uint16_t srv_port)
{
    char *tx_buf = NULL;
//    char ch;
    int cnt = 0;
    int retry_cnt = 10;
    struct sockaddr_in saddr;
    struct at_local_udp_send send_data;

    tx_buf = sys_zalloc(tx_len);
    if (NULL == tx_buf) {
        AT_TRACE("Allocate client buffer failed (len = %u).\r\n", tx_len);
        return -1;
    }
    AT_RSP_DIRECT(">\r\n", 3);
#if 0
    usart_interrupt_disable(at_uart_conf.usart_periph, USART_INT_RBNE);
    sys_priority_set(sys_current_task_handle_get(), MGMT_TASK_PRIORITY);
    while (1) {
        while (RESET == usart_flag_get(at_uart_conf.usart_periph, USART_FLAG_RBNE));
        ch = usart_data_receive(at_uart_conf.usart_periph);
        if ((ch == 0x0a || ch == 0x0d) && (cnt >= tx_len))
            break;
        if (cnt < tx_len) {
            tx_buf[cnt] = ch;
            cnt++;
        } else {
            AT_RSP("%c", ch);
        }
    }
    sys_priority_set(sys_current_task_handle_get(), CLI_PRIORITY);
    usart_interrupt_enable(at_uart_conf.usart_periph, USART_INT_RBNE);
    //buffer_dump("TX:", (uint8_t *)tx_buf, cnt);
#else
    // Block here to wait dma receive done
    at_hw_dma_receive((uint32_t)tx_buf, tx_len);
    send_data.event_id = AT_LOCAL_UDP_SEND_EVENT;
    send_data.sock_fd = fd;
    send_data.send_data_addr = (uint32_t)tx_buf;
    send_data.send_data_len = tx_len;

    // debug_print_dump_data("TX:", (char *)tx_buf, tx_len);
#endif

    sys_memset(&saddr, 0, sizeof(struct sockaddr_in));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(srv_port);
    saddr.sin_addr.s_addr = inet_addr(srv_ip);

    sys_memcpy(&(send_data.to), &saddr, sizeof(struct sockaddr_in));
    send_data.tolen = sizeof(struct sockaddr_in);

Retry:
    cnt = sendto(local_sock_send, (void *)&send_data, sizeof(send_data), 0, NULL, 0);
    if (cnt <= 0) {
        if ((errno == EAGAIN || errno == ENOMEM) && retry_cnt > 0) {
            sys_ms_sleep(20);
            retry_cnt--;
            goto Retry;
        }
        sys_mfree(tx_buf);
        AT_TRACE("local socket send udp fail. %d!\r\n", errno);
        AT_RSP_START(20);
        AT_RSP("SEND FAIL\r\n");
        AT_RSP_IMMEDIATE();
        AT_RSP_FREE();
    }
    return cnt;
}

/*!
    \brief      start a tcp/udp server
    \param[in]  type: tcp: CIP_TYPE_TCP; udp: CIP_TYPE_UDP.
    \param[in]  srv_port: server port
    \param[out] none
    \retval     the result of operation
*/
static int tcp_udp_server_start(uint8_t type, uint16_t srv_port)
{
    struct sockaddr_in server_addr;
    socklen_t len = sizeof(server_addr);
    int status, reuse;
    int srv_fd = -1;

    if (type == CIP_TYPE_TCP) {
        srv_fd = socket(AF_INET, SOCK_STREAM, 0);
    } else if (type == CIP_TYPE_UDP) {
        srv_fd = socket(AF_INET, SOCK_DGRAM, 0);
    }
    if (srv_fd < 0) {
        AT_TRACE("open socket failed\r\n");
        return -1;
    }

    if (type == CIP_TYPE_TCP) {
        AT_TRACE("Create TCP server socket %d.\r\n", srv_fd);
    } else if (type == CIP_TYPE_UDP) {
        AT_TRACE("Create UDP server socket %d.\r\n", srv_fd);
    }
    reuse = 1;
    setsockopt(srv_fd, SOL_SOCKET, SO_REUSEADDR,
            (const char *) &reuse, sizeof(reuse) );

    sys_memset((char *)&server_addr, 0, len);
    server_addr.sin_family      = AF_INET;
    server_addr.sin_len         = len;
    server_addr.sin_port        = htons(srv_port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* binding the socket to the server address */
    status = bind(srv_fd, (struct sockaddr *)&server_addr, len);
    if( status < 0 ) {
        AT_TRACE("Bind server socket fd error!\r\n");
        goto Exit;
    }
    AT_TRACE("Bind successfully.\r\n");

    if (type == CIP_TYPE_TCP) {
        /* putting the socket for listening to the incoming TCP connection */
        status = listen(srv_fd, MAX_CLIENT_NUM);
        if( status != 0 ) {
            AT_TRACE("Listen tcp server socket fd error!\r\n");
            goto Exit;
        }
    }
    /* Get local port */
    sys_memset(&server_addr, 0, len);
    getsockname(srv_fd, (struct sockaddr *)&server_addr, &len);
    cip_info.local_srv_fd = srv_fd;
    cip_info.local_srv_port = ntohs(server_addr.sin_port);
    cip_info.local_srv_stop = 0;
    cip_info.local_srv_type = type;
    AT_TRACE("Server port %d\r\n", cip_info.local_srv_port);

    return 0;

Exit:
    /* close the listening socket */
    close(srv_fd);
    return status;
}

/*!
    \brief      stop tcp/udp server
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void tcp_udp_server_stop(void)
{
    int i, fd;
    int active_sock_num = 0;

    if (cip_info.local_srv_fd < 0) {
        AT_TRACE("No server running.\r\n");
        return;
    }

    for (i = 0; i < MAX_CLIENT_NUM; i++) {
        if ((cip_info.cli[i].fd > -1) && (cip_info.cli[i].role == CIP_ROLE_CLIENT)) {
            active_sock_num++;
        }
        if (active_sock_num) {
            break;
        }
    }

    if (active_sock_num) {
        cip_info.local_srv_stop = 1;
    } else {
        cip_task_terminate = 1;
        while (sys_task_exist((const uint8_t *)"Cip Rcv")) {
            sys_ms_sleep(1);
        }
        cip_task_started = 0;
    }
}

#ifdef CONFIG_ATCMD_SPI
/*!
    \brief      process recieved data from socket server and
                list the data in the client_info_t.
    \param[in]  fd:     fd of this data recv from
    \param[in]  rx_buf: buffer that stored recieved data
    \param[in]  recv_sz: recieved data length
    \param[out] none
    \retval     none
*/
static void at_spi_recv_data_process(int fd, uint8_t *rx_buf, int recv_sz)
{
    int recv_processed = 0, currentdatasize = 0;
    struct recv_data_node *recv_data_node = NULL;
    uint8_t *data_recv = NULL;
    int wait_count = 0;
#define MAX_WAIT_COUNT 100
#define WAIT_INTERVAL_MS 50
    if (fd < 0 || cip_task_started != 1) {
        AT_TRACE("fd less than 0, or cip_recv task not started\r\n");
        return;
    }

    do {
        recv_data_node = sys_zalloc(sizeof(struct recv_data_node));
        currentdatasize = (recv_sz - recv_processed) > AT_SPI_MAX_DATA_LEN ? AT_SPI_MAX_DATA_LEN : (recv_sz - recv_processed);

        if (recv_data_node == NULL) {
            AT_TRACE("Allocate recv_data_node failed (len = %u).\r\n", sizeof(struct recv_data_node));
            break;
        }

        data_recv = sys_malloc(currentdatasize);// for data
        if (data_recv == NULL) {
            AT_TRACE("Allocate data_recv failed (len = %u).\r\n", currentdatasize);
            sys_mfree(recv_data_node);
            break;
        }

        sys_memcpy(data_recv, rx_buf + recv_processed, currentdatasize);// copy payload

        recv_data_node->data = data_recv;
        recv_data_node->data_len = currentdatasize;
        // AT_TRACE("list add,len:%d, fd:%d\r\n", currentdatasize, fd);
        recv_data_node->fd = fd;
        // block wait, until list have empty node
        wait_count = 0;
        while (1) {
            sys_mutex_get(&cip_info.list_lock);

            if (cip_info.list_data_count < MAX_RECV_DATA_NUM_IN_LIST) {
                list_push_back(&cip_info.recv_data_list, &recv_data_node->list_hdr);
                cip_info.list_data_count++;
                sys_mutex_put(&cip_info.list_lock);
                break;
            }

            sys_mutex_put(&cip_info.list_lock);

            // list full, wait
            wait_count++;
            if (wait_count >= MAX_WAIT_COUNT) {
                AT_TRACE("Wait timeout after %dms, drop NEW packet (len=%d), list=%d, triger_cnt=%d\r\n",
                        wait_count * WAIT_INTERVAL_MS, currentdatasize, MAX_RECV_DATA_NUM_IN_LIST, cip_info.triger_count);
                sys_mfree(data_recv);
                sys_mfree(recv_data_node);
                recv_processed += currentdatasize;
                break;
            }
            sys_ms_sleep(WAIT_INTERVAL_MS);
        }
        if (wait_count <= MAX_WAIT_COUNT) {
            recv_processed += currentdatasize;
        }
    } while (recv_processed < recv_sz);

    return;
}
#endif /* CONFIG_ATCMD_SPI */

int at_uart_write(uint8_t *buffer, uint32_t len)
{
    uint8_t *d = buffer;

    if (len == 0 || buffer == NULL) {
        return -1;
    }

    while (1) {
        while (RESET == usart_flag_get(AT_UART, USART_FLAG_TBE));
        usart_data_transmit(AT_UART, *d++);
        len--;
        if (len == 0) {
            break;
        }
    }
    while (RESET == usart_flag_get(AT_UART, USART_FLAG_TC));

    return 0;
}

extern int dhcpd_ipaddr_is_valid(uint32_t ipaddr);
/*!
    \brief      receive task
    \param[in]  param: the pointer of user parameter
    \param[out] none
    \retval     none
*/
static void cip_recv_task(void *param)
{
    struct timeval timeout;
    int max_fd_num = 0;
    int cli_fd, i, j, recv_sz;
    char *rx_buf;
    uint32_t rx_len = PASSTH_START_TRANSFER_LEN;
    struct sockaddr_in saddr;
    int addr_sz = sizeof(saddr);
    fd_set read_set, except_set;
    int status;
    int keepalive = 1;
    int keepidle = 20; //in seconds
    int keepcnt = 3;
    int keepinval = 10; //in seconds
    int vif_idx = WIFI_VIF_INDEX_DEFAULT;
    int send_timeout; // ms
    int send_cnt;
    struct linger ling;
    int close_fd = -1;

    int local_sock_recv = -1;
    int local_port = 1635;
    struct sockaddr_in local_addr_recv;
    struct sockaddr_in local_addr_send;
    int local_recv_sz = 0;
#define LOCAL_RECV_BUF_SIZE 50 // in bytes
    uint8_t local_recv_buf[LOCAL_RECV_BUF_SIZE] = {0};

    local_sock_recv = socket(AF_INET, SOCK_DGRAM, 0);
    if (local_sock_recv < 0) {
        AT_TRACE("Create local socket recv error!\r\n");
        goto Exit;
    }
    sys_memset(&local_addr_recv, 0, sizeof(local_addr_recv));
    local_addr_recv.sin_family = AF_INET;
    local_addr_recv.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    local_addr_recv.sin_port = htons(local_port);
    if (bind(local_sock_recv, (struct sockaddr *)&local_addr_recv, sizeof(local_addr_recv)) < 0) {
        AT_TRACE("bind local socket fail. %d!\r\n", errno);
        goto Exit;
    }
    local_sock_send = socket(AF_INET, SOCK_DGRAM, 0);
    if (local_sock_send < 0) {
        AT_TRACE("Create local socket send error!\r\n");
        goto Exit;
    }

    /* Set socket to non-blocking mode to prevent sendto() from blocking
     * if receiver (cip_recv_task) is not reading fast enough or has exited */
    {
        int flags = 1;
        if (ioctlsocket(local_sock_send, FIONBIO, &flags) < 0) {
            AT_TRACE("Set local socket non-blocking fail. %d!\r\n", errno);
        }
    }

    sys_memset(&local_addr_send, 0, sizeof(local_addr_send));
    local_addr_send.sin_family = AF_INET;
    local_addr_send.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    local_addr_send.sin_port = htons(local_port);
    if (connect(local_sock_send, (struct sockaddr *)&local_addr_send, sizeof(local_addr_send)) < 0) {
        AT_TRACE("connect local socket fail. %d!\r\n", errno);
        goto Exit;
    }

#ifdef CONFIG_ATCMD_SPI
    struct recv_data_node *p_item = NULL;
#endif

    rx_buf = sys_zalloc(rx_len);
    if(NULL == rx_buf){
        AT_TRACE("Allocate client buffer failed (len = %u).\r\n", rx_len);
        goto Exit;
    }

    timeout.tv_sec = 0;
    timeout.tv_usec = 10000; // 10ms

    cip_task_terminate = 0;
    while (1) {
        if (cip_task_terminate)
            break;

        FD_ZERO(&read_set);
        FD_ZERO(&except_set);
        if (cip_info.local_srv_fd >= 0) {
            if (cip_info.local_srv_stop == 0) {
                FD_SET(cip_info.local_srv_fd, &read_set);
                FD_SET(cip_info.local_srv_fd, &except_set);
                if (cip_info.local_srv_fd > max_fd_num)
                    max_fd_num = cip_info.local_srv_fd;
            } else {
                if (cip_info.local_srv_type == CIP_TYPE_TCP) {
                    for (i = 0; i < MAX_CLIENT_NUM; i++) {
                        if ((cip_info.cli[i].fd >= 0) && (cip_info.cli[i].role == CIP_ROLE_SERVER)) {
                            close_fd = cip_info.cli[i].fd;
                            cip_info_cli_free(i);
                            close(close_fd);
                        }
                    }
                }
                close_fd = cip_info.local_srv_fd;
                cip_info.local_srv_fd = -1;
                cip_info.local_srv_port = 0;
                close(close_fd);
            }
        }
        for (i = 0; i < MAX_CLIENT_NUM; i++) {
            //if ((cip_info.cli[i].fd >= 0) && (cip_info.cli[i].type == CIP_TYPE_TCP)) {
            if (cip_info.cli[i].fd >= 0) {
                FD_SET(cip_info.cli[i].fd, &read_set);
                FD_SET(cip_info.cli[i].fd, &except_set);
                if (cip_info.cli[i].fd > max_fd_num)
                    max_fd_num = cip_info.cli[i].fd;
            }
        }
        FD_SET(local_sock_recv, &read_set);
        if (local_sock_recv > max_fd_num) {
            max_fd_num = local_sock_recv;
        }
        status = select(max_fd_num + 1, &read_set, NULL, &except_set, &timeout);
        if ((cip_info.local_srv_fd >= 0) && FD_ISSET(cip_info.local_srv_fd, &read_set)) {
            if (cip_info.local_srv_type == CIP_TYPE_TCP) {
                sys_memset(&saddr, 0, sizeof(saddr));
                addr_sz = sizeof(saddr);
                cli_fd = accept(cip_info.local_srv_fd, (struct sockaddr *)&saddr, (socklen_t*)&addr_sz);
                if (cip_info.cli_num >= MAX_CLIENT_NUM) {
                    if (cli_fd >= 0) {
                        close(cli_fd);
                    }
                    AT_TRACE("client full\r\n");
                } else {
#ifndef CONFIG_ATCMD_SPI
                    if (cip_info.trans_mode == CIP_TRANS_MODE_PASSTHROUGH &&
                            cip_info_valid_fd_cnt_get() >= 1) {
                        if (cli_fd >= 0) {
                            close(cli_fd);
                        }
                        AT_TRACE("Only one connection is allowed in Passthrough mode\r\n");
                    } else
#endif
                    {
                        if (cli_fd >= 0) {
                            AT_TRACE("new client %d\r\n", cli_fd);
                            status = cip_info_cli_store(-1, cli_fd, "TCP", CIP_ROLE_SERVER,
                                                saddr.sin_addr.s_addr, ntohs(saddr.sin_port), cip_info.local_srv_port);
                            if (status < 0) {
                                AT_TRACE("Store client info error %d!\r\n", status);
                                close(cli_fd);
                            } else {
                                setsockopt(cli_fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(int));
                                setsockopt(cli_fd, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(int));
                                setsockopt(cli_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepinval, sizeof(int));
                                setsockopt(cli_fd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(int));

                                send_timeout = 3000;
                                setsockopt(cli_fd, SOL_SOCKET, SO_SNDTIMEO, (const void *)&send_timeout, sizeof(send_timeout));

                                ling.l_onoff = 1;  // enable
                                ling.l_linger = 3; // in seconds
                                setsockopt(cli_fd, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));
                            }
                            cip_passth_info.passth_fd_idx = status;
                        } else {
                            AT_TRACE("accept error %d!\r\n", errno);
                        }
                    }
                }
            } else if (cip_info.local_srv_type == CIP_TYPE_UDP) {
                sys_memset(rx_buf, 0, rx_len);
                sys_memset(&saddr, 0, sizeof(saddr));
                addr_sz = sizeof(saddr);
                recv_sz = recvfrom(cip_info.local_srv_fd, rx_buf, rx_len, 0, (struct sockaddr *)&saddr, (socklen_t *)&addr_sz);
                AT_TRACE("udp server recv from %s:%d.\r\n", inet_ntoa(saddr.sin_addr), ntohs(saddr.sin_port));
                if (recv_sz == 0) {
                    AT_TRACE("remote close %s:%d.\r\n", inet_ntoa(saddr.sin_addr), ntohs(saddr.sin_port));
                } else if (recv_sz > 0) {
                    AT_RSP_START(64);
                    AT_RSP("+IPD,%s:%d, %d: ", inet_ntoa(saddr.sin_addr), ntohs(saddr.sin_port), recv_sz);
                    AT_RSP_IMMEDIATE();
                    AT_RSP_DIRECT((char *)rx_buf, recv_sz);
                    AT_RSP("\r\n");
                    AT_RSP_OK();
                } else {
                    AT_TRACE("rx error %d, from %s:%d.\r\n", errno, inet_ntoa(saddr.sin_addr), ntohs(saddr.sin_port));
                }
            }
        }

        if (FD_ISSET(local_sock_recv, &read_set)) {
            sys_memset(local_recv_buf, 0, LOCAL_RECV_BUF_SIZE);
            local_recv_sz = recvfrom(local_sock_recv, local_recv_buf, LOCAL_RECV_BUF_SIZE, 0, NULL, NULL);
            if (local_recv_sz <= 0) {
                AT_TRACE("recv data from local fail, %d!\r\n", errno);
            } else {
                if (*((uint16_t *)local_recv_buf) == AT_LOCAL_TCP_SEND_EVENT) {
                    struct at_local_tcp_send *send_data_local = (struct at_local_tcp_send *)local_recv_buf;
                    AT_RSP_START(128);

                    /* Limited retry mechanism to prevent infinite blocking */
                    int tcp_retry_count = 0;
                    #define MAX_RETRIES 60 /* Maximum retry attempts */

TCP_RETRY_SEND:
                    send_cnt = send(send_data_local->sock_fd, (void *)(send_data_local->send_data_addr),
                                    send_data_local->send_data_len, MSG_DONTWAIT);
                    if (send_cnt <= 0) {
                        AT_TRACE("send data error. %d!\r\n", errno);
                        if ((errno == EAGAIN || errno == ENOMEM) && tcp_retry_count < MAX_RETRIES) {
                            tcp_retry_count++;
                            sys_ms_sleep(50);
                            goto TCP_RETRY_SEND;
                        }
                        /* If max retries exceeded or other error, treat as send failure */
                        if (tcp_retry_count >= MAX_RETRIES) {
                            AT_TRACE("send retry exhausted after %d attempts\r\n", tcp_retry_count);
                        }
                        /* CRITICAL FIX: Free tx_buf first to prevent memory leak */
                        sys_mfree((void *)(send_data_local->send_data_addr));
                        send_data_local->send_data_addr = 0;

                        /* Note: Don't close connection for temporary send failures (EAGAIN/ENOMEM)
                         * Connection may still be usable. Only close for fatal errors. */
                        if (errno != EAGAIN && errno != ENOMEM) {
                            int idx = cip_info_cli_find(send_data_local->sock_fd);
                            if ((idx != -1) && (cip_info.cli[idx].role == CIP_ROLE_CLIENT)) {
                                AT_TRACE("close tcp client. %d!\r\n", send_data_local->sock_fd);
                                close(send_data_local->sock_fd);  /* Close socket first */
                                cip_info_cli_free(idx);  /* Then free connection info */
                            }
                        }
#ifndef CONFIG_ATCMD_SPI
                        if (cip_info.trans_mode == CIP_TRANS_MODE_PASSTHROUGH) {
                            AT_RSP_FREE();
                        } else
#endif
                        {
                            AT_RSP("SEND FAIL\r\n");
                            AT_RSP_ERR();
                        }
                    } else {
#ifndef CONFIG_ATCMD_SPI
                        if (cip_info.trans_mode == CIP_TRANS_MODE_PASSTHROUGH) {
                            AT_RSP_FREE();
                        } else
#endif
                        {
                            AT_RSP("SEND OK\r\n");
                            AT_RSP_OK();
                        }
                    }
#ifndef CONFIG_ATCMD_SPI
                    if (cip_info.trans_mode != CIP_TRANS_MODE_PASSTHROUGH)
#endif
                    {
                        /* Only free if not already freed in error path */
                        if (send_data_local->send_data_addr != 0) {
                            sys_mfree((void *)(send_data_local->send_data_addr));
                        }
                    }
                } else if (*((uint16_t *)local_recv_buf) == AT_LOCAL_UDP_SEND_EVENT) {
                    struct at_local_udp_send *send_data_local = (struct at_local_udp_send *)local_recv_buf;
                    int udp_retry_count = 0;
                    AT_RSP_START(128);
UDP_RETRY_SEND:
                    send_cnt = sendto(send_data_local->sock_fd, (void *)(send_data_local->send_data_addr), send_data_local->send_data_len,
                                        0, (struct sockaddr *)&(send_data_local->to), send_data_local->tolen);
                    if (send_cnt <= 0) {
                        AT_TRACE("send data error. %d!\r\n", errno);
                        if ((errno == EAGAIN || errno == ENOMEM) && udp_retry_count < MAX_RETRIES) {
                            udp_retry_count++;
                            goto UDP_RETRY_SEND;
                        }
                        if (udp_retry_count >= MAX_RETRIES) {
                            AT_TRACE("send retry exhausted after %d attempts\r\n", udp_retry_count);
                        }
                        /* CRITICAL FIX: Free tx_buf first to prevent memory leak */
                        sys_mfree((void *)(send_data_local->send_data_addr));
                        send_data_local->send_data_addr = 0;

                        /* Don't close connection for temporary send failures */
                        if (errno != EAGAIN && errno != ENOMEM) {
                            int idx = cip_info_cli_find(send_data_local->sock_fd);
                            if (idx != -1) {
                                AT_TRACE("close udp client. %d!\r\n", send_data_local->sock_fd);
                                close(send_data_local->sock_fd);  /* Close socket first */
                                cip_info_cli_free(idx);  /* Then free connection info */
                            }
                        }
#ifndef CONFIG_ATCMD_SPI
                        if (cip_info.trans_mode == CIP_TRANS_MODE_PASSTHROUGH) {
                            AT_RSP_FREE();
                        } else
#endif
                        {
                            AT_RSP("SEND FAIL\r\n");
                            AT_RSP_ERR();
                        }
                    } else {
#ifndef CONFIG_ATCMD_SPI
                        if (cip_info.trans_mode == CIP_TRANS_MODE_PASSTHROUGH) {
                            AT_RSP_FREE();
                        } else
#endif
                        {
                            AT_RSP("SEND OK\r\n");
                            AT_RSP_OK();
                        }
                    }
#ifndef CONFIG_ATCMD_SPI
                    if (cip_info.trans_mode != CIP_TRANS_MODE_PASSTHROUGH)
#endif
                    {
                        /* Only free if not already freed in error path */
                        if (send_data_local->send_data_addr != 0) {
                            sys_mfree((void *)(send_data_local->send_data_addr));
                        }
                    }
                } else {
                    AT_TRACE("Invalid local event.\r\n");
                }
            }
        }

        for (i = 0; i < MAX_CLIENT_NUM; i++) {
            if ((cip_info.cli[i].fd >= 0) && (FD_ISSET(cip_info.cli[i].fd, &except_set) ||
                (wifi_vif_is_softap(vif_idx) && !dhcpd_ipaddr_is_valid(cip_info.cli[i].remote_ip)))) {
                close_fd = cip_info.cli[i].fd;
                AT_TRACE("error %d\r\n", cip_info.cli[i].fd);
                close(close_fd);
                cip_info_cli_free(i);
            }

            if ((cip_info.cli[i].fd >= 0) && (cip_info.cli[i].stop_flag == 1)) {
                close_fd = cip_info.cli[i].fd;
                cip_info_cli_free(i);
                close(close_fd);
                AT_TRACE("close %d.\r\n", close_fd);
            }

#ifdef CONFIG_ATCMD_SPI
            if (cip_info.list_data_count >= (MAX_RECV_DATA_NUM_IN_LIST - 5)) {
                // List count is not enought
                break;
            }
#endif

            if ((cip_info.cli[i].fd >= 0) && FD_ISSET(cip_info.cli[i].fd, &read_set)) {
                sys_memset(rx_buf, 0, rx_len);
                if (cip_info.cli[i].type == CIP_TYPE_TCP) {
                    recv_sz = recv(cip_info.cli[i].fd, rx_buf, rx_len, 0);
                } else {
                    sys_memset(&saddr, 0, sizeof(saddr));
                    addr_sz = sizeof(saddr);
                    recv_sz = recvfrom(cip_info.cli[i].fd, rx_buf, rx_len,
                                        0, (struct sockaddr *)&saddr, (socklen_t*)&addr_sz);
                }

                if (recv_sz < 0) { /* Recv error */
                    AT_TRACE("rx error %d, errno:%d\r\n", recv_sz, errno);
                    close_fd = cip_info.cli[i].fd;
                    if (errno == ECONNABORTED) {
                        AT_TRACE("connection aborted, maybe remote close.\r\n");
                    }
#ifdef CONFIG_ATCMD_SPI
                    if (cip_info.trans_mode == CIP_TRANS_MODE_FILE_TRANSFER &&
                            cip_file_trans_info.fd_idx == i) {
                        cip_file_trans_info.terminate = 1;
                    }
#endif
                    close(close_fd);
                    cip_info_cli_free(i);
                } else if (recv_sz == 0) {
                    AT_TRACE("remote close %d\r\n", cip_info.cli[i].fd);
                    close(cip_info.cli[i].fd);
#ifndef CONFIG_ATCMD_SPI
                    if (cip_info.trans_mode == CIP_TRANS_MODE_PASSTHROUGH &&
                            cip_passth_info.passth_fd_idx == i) {
                        cip_passth_info.terminate_send_passth = 1;
                    }
#else
                    if (cip_info.trans_mode == CIP_TRANS_MODE_FILE_TRANSFER &&
                            cip_file_trans_info.fd_idx == i) {
                        cip_file_trans_info.terminate = 1;
                    }
#endif
                    cip_info_cli_free(i);
                } else {
#ifdef CONFIG_ATCMD_SPI
                    // Discard packets during file transfer
                    if (cip_info.trans_mode == CIP_TRANS_MODE_FILE_TRANSFER &&
                        cip_file_trans_info.terminate == 1)
                        break;
#else
                    if (cip_info.trans_mode == CIP_TRANS_MODE_PASSTHROUGH &&
                            cip_passth_info.passth_fd_idx == i) {
                        AT_RSP_DIRECT(rx_buf, recv_sz);
                    }
#endif
                    if (cip_info.trans_mode == CIP_TRANS_MODE_NORMAL) {
#ifdef CONFIG_ATCMD_SPI
                        at_spi_recv_data_process(cip_info.cli[i].fd, (uint8_t *)rx_buf, recv_sz);
#else
                        AT_RSP_START(20);
                        AT_RSP("+IPD,%d,%d: ", cip_info.cli[i].fd, recv_sz);
                        AT_RSP_IMMEDIATE();
                        AT_RSP_DIRECT((char *)rx_buf, recv_sz);
                        AT_RSP("\r\n");
                        AT_RSP_OK();
#endif
                    }
                }
            }
        }

#ifdef CONFIG_ATCMD_SPI
            sys_enter_critical();
            /* CRITICAL FIX: Don't trigger handshake if just sent TX response (within 50ms)
             * to avoid double-trigger that confuses Master causing tx:1/2069 error.
             * The spi_manager tracks state: if recently TX'd (AT_ACK/Data_ACK),
             * Master is still reading that TX data - don't interrupt with new handshake! */
            if (!list_is_empty(&cip_info.recv_data_list) && at_spi_hw_is_idle()
                && cip_info.triger_count == 0 && spi_manager.stat == SPI_Slave_AT_Recv) {
                /* Only trigger if we're in stable AT_Recv state, not just after TX */
                    spi_handshake_rising_trigger();
                    cip_info.triger_count ++;
            }
            sys_exit_critical();
#endif

        if ((cip_info_valid_fd_cnt_get() == 0) && cip_info.local_srv_fd < 0
            && cip_info.tcp_udp_start_process_num == 0) {
            cip_task_terminate = 1;
        }
    }

    /* Exit */
    for (i = 0; i < MAX_CLIENT_NUM; i++) {
        if (cip_info.cli[i].fd >= 0) {
            close_fd = cip_info.cli[i].fd;
            cip_info_cli_free(i);
            AT_TRACE("close client fd:%d.\r\n", close_fd);
            close(close_fd);
        }
    }
    /* close the listening socket */
    if (cip_info.local_srv_fd >= 0) {
        AT_TRACE("close local_srv_fd fd:%d.\r\n", cip_info.local_srv_fd);
        close(cip_info.local_srv_fd);
        cip_info.local_srv_fd = -1;
        cip_info.local_srv_port = 0;
    }

    sys_mfree(rx_buf);

Exit:
#ifdef CONFIG_ATCMD_SPI
    sys_mutex_get(&cip_info.list_lock);
    p_item = (struct recv_data_node *)list_pick(&cip_info.recv_data_list);

    while (p_item != NULL) {
        if (p_item->data && (p_item->data_len > 0)) {
            sys_mfree(p_item->data);
        }
        list_remove(&cip_info.recv_data_list, NULL, (struct list_hdr *)p_item);
        cip_info.list_data_count--;
        sys_mfree(p_item);
        p_item = (struct recv_data_node *)list_pick(&cip_info.recv_data_list);
    }
    if (cip_info.list_data_count != 0) {
        AT_TRACE("recv data list count error %d.\r\n", cip_info.list_data_count);
        cip_info.list_data_count = 0;
    }
    sys_mutex_put(&cip_info.list_lock);
#endif
    if (local_sock_send >= 0) {
        shutdown(local_sock_send, SHUT_RD);
        close(local_sock_send);
        AT_TRACE("close local_sock_send fd:%d.\r\n", local_sock_send);
        local_sock_send = -1;
    }
    if (local_sock_recv >= 0) {
        shutdown(local_sock_recv, SHUT_RD);
        close(local_sock_recv);
    }
    cip_task_started = 0;
    sys_task_delete(NULL);
}

/*!
    \brief      the AT command ping
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cip_ping(int argc, char **argv)
{
    struct ping_info_t *ping_info = NULL;

    AT_RSP_START(128);
    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            char *domain = at_string_parse(argv[1]);
            struct addrinfo hints, *res;
            void *ptr;
#ifdef CONFIG_IPV6_SUPPORT
            char ip_addr[64];
#else
            char ip_addr[32];
#endif /* CONFIG_IPV6_SUPPORT */
            if (domain == NULL) {
                goto Error;
            }

            memset(&hints, 0, sizeof(hints));
            if (getaddrinfo(domain, NULL, &hints, &res) != 0) {
                goto Error;
            }

            ping_info = sys_zalloc(sizeof(struct ping_info_t));
            if (ping_info == NULL) {
                freeaddrinfo(res);
                goto Error;
            }
#ifdef CONFIG_IPV6_SUPPORT
            if (res->ai_family == AF_INET6) {
                ptr = &((struct sockaddr_in6 *)res->ai_addr)->sin6_addr;
                ping_info->ip_type = IPADDR_TYPE_V6;
            } else
#endif /* CONFIG_IPV6_SUPPORT */
            {
                ptr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
            }
            inet_ntop(res->ai_family, ptr, ip_addr, sizeof(ip_addr));
            freeaddrinfo(res);
            memcpy(ping_info->ping_ip, ip_addr, sizeof(ping_info->ping_ip));
            ping_info->ping_cnt = 5;
            ping_info->ping_size = 120;
            ping_info->ping_interval = 1000;
            if (ping(ping_info) != ERR_OK)
                goto Error;
#ifdef CONFIG_ATCMD
            AT_RSP("%s", ping_info->ping_res);
#endif
        }
    } else {
        goto Error;
    }

    if (ping_info)
        sys_mfree(ping_info);

    AT_RSP_OK();
    return;

Error:
    if (ping_info)
        sys_mfree(ping_info);
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+PING=<ip or domain name>\r\n");
    AT_RSP_OK();
    return;
}

/*!
    \brief      the AT command start a tcp or udp client
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cip_start(int argc, char **argv)
{
    char *type, *srv_ip;
    uint16_t srv_port;
    uint32_t bkeep_alive = 0;
    char *endptr = NULL;
    int ret = -1;
    int con_id = -1;
    uint8_t idx = 1;
    uint16_t local_port = 0;

    AT_RSP_START(128);

    if (cip_info.cli_num >= MAX_CLIENT_NUM) {
        AT_TRACE("client full\r\n");
        goto Error;
    }

    if ((multi_connection_enable == 0) && ((cip_info.cli_num > 0) || (cip_info.local_srv_fd >= 0))) {
        AT_RSP("Only one connection is allowed to be established.\r\n");
        AT_RSP("Please use cmd AT+CIPMUX to enable multiple connections.\r\n");
        goto Error;
    }

    if (argc == 1) {
        goto Error;
    }
    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            goto Error;
        }
    }
    if (multi_connection_enable == 0) {
        if (argc < 4) {
            goto Usage;
        }
        con_id = 0;
    } else {
        if (argc < 5) {
            goto Usage;
        }
        con_id = (int)strtol((const char *)argv[idx++], &endptr, 10);
        if ((*endptr != '\0') || (con_id < 0) || (con_id > (MAX_CLIENT_NUM - 1))) {
            goto Error;
        }
    }
    type = at_string_parse(argv[idx++]);
    srv_ip = at_string_parse(argv[idx++]);
    if ((type == NULL) || (srv_ip == NULL)) {
        goto Error;
    }
    srv_port = (uint32_t)strtoul((const char *)argv[idx++], &endptr, 10);
    if (*endptr != '\0') {
        goto Error;
    }
    if (argc > idx) {
        if (strncmp(type, "TCP", 3) == 0) {
            bkeep_alive = (uint32_t)strtoul((const char *)argv[idx++], &endptr, 10);
            if (*endptr != '\0') {
                goto Error;
            }
        } else if (strncmp(type, "UDP", 3) == 0) {
            local_port = (uint32_t)strtoul((const char *)argv[idx++], &endptr, 10);
            if (*endptr != '\0') {
                goto Error;
            }
        } else {
            goto Error;
        }
    }
    if (argc > idx) {
        goto Error;
    }

    if (strncmp(type, "TCP", 3) == 0) {
        if ((ret = tcp_client_start(con_id, srv_ip, srv_port, bkeep_alive) < 0))
            goto Error;
    } else if (strncmp(type, "UDP", 3) == 0) {
        if ((ret = udp_client_start(con_id, srv_ip, srv_port, local_port) < 0))
            goto Error;
    } else {
        goto Error;
    }

    if (cip_task_started == 0) {
        if (sys_task_create_dynamic((const uint8_t *)"Cip Rcv",
                        CIP_RECV_STACK_SIZE, CIP_RECV_PRIO,
                        (task_func_t)cip_recv_task, NULL) == NULL)
            goto Error;
        cip_task_started = 1;
    }

    AT_RSP("%d,", con_id);
    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+CIPSTART=[<con_id>,]<type:TCP or UDP>,<remote ip>,<remote port>[,udp local port][,tcp keep alive]\r\n");
    AT_RSP_OK();
    return;
}

/*!
    \brief      the AT command send a tcp or udp packet
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cip_send(int argc, char **argv)
{
    int fd, idx, ret = 0;
    uint8_t type;
    uint32_t tx_len, file_len, segment_len;
    char *remote_ip;
    uint16_t remote_port;
    char *endptr = NULL;
#ifndef CONFIG_ATCMD_SPI
    int valid_fd_cnt = 0;
#endif

    AT_RSP_START(128);
    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            if (multi_connection_enable) {
                goto Error;
            }
            tx_len = (uint32_t)strtoul((const char *)argv[1], &endptr, 10);
            if ((*endptr != '\0') || (tx_len > 2048)) {
                goto Error;
            }
            fd = cip_info.cli[0].fd;
            type = cip_info.cli[0].type;
            if (type == CIP_TYPE_TCP) {
                if (at_tcp_send(fd, tx_len) <= 0) {
                    goto Error;
                }
            } else if (type == CIP_TYPE_UDP) {
                remote_ip = inet_ntoa(cip_info.cli[0].remote_ip);
                remote_port = cip_info.cli[0].remote_port;
                if (at_udp_send(fd, tx_len, remote_ip, remote_port) <= 0) {
                    goto Error;
                }
            }
            AT_RSP_FREE();
            return;
        }
    } else if (argc == 3) {
        idx = (int)strtol((const char *)argv[1], &endptr, 10);
        if ((*endptr != '\0') || (idx < 0) || idx >= MAX_CLIENT_NUM) {
            AT_TRACE("con_id error\r\n");
            goto Error;
        }
        if (cip_info_cli_is_free(idx)) {
            AT_TRACE("Invalid con_id\r\n");
            goto Error;
        }
        tx_len = (uint32_t)strtoul((const char *)argv[2], &endptr, 10);
        if ((*endptr != '\0') || (tx_len > 2048)) {
            goto Error;
        }
        //AT_TRACE("CON: %d, len %d\r\n", idx, tx_len);
        fd = cip_info.cli[idx].fd;
        type = cip_info.cli[idx].type;
        if (type == CIP_TYPE_TCP) {
            ret = at_tcp_send(fd, tx_len);
            if (ret <= 0)
                goto Error;
        } else if (type == CIP_TYPE_UDP) {
            remote_ip = inet_ntoa(cip_info.cli[idx].remote_ip);
            remote_port = cip_info.cli[idx].remote_port;
            if (at_udp_send(fd, tx_len, remote_ip, remote_port) <= 0) {
                goto Error;
            }
        } else {
            AT_TRACE("type error\r\n");
            goto Error;
        }
        AT_RSP_FREE();
        return;
    } else if (argc == 5) {
        idx = (int)strtol((const char *)argv[1], &endptr, 10);
        if ((*endptr != '\0') || (idx < 0) || idx > MAX_CLIENT_NUM) {
            AT_TRACE("con_id error\r\n");
            goto Error;
        }
        if (idx != MAX_CLIENT_NUM) {
            if (cip_info_cli_is_free(idx)) {
                AT_TRACE("Invalid con_id\r\n");
                goto Error;
            } else {
                fd = cip_info.cli[idx].fd;
                type = cip_info.cli[idx].type;
            }
        } else {
            if ((cip_info.local_srv_fd >= 0) && (cip_info.local_srv_type == CIP_TYPE_UDP)) {
                fd = cip_info.local_srv_fd;
                type = CIP_TYPE_UDP;
            } else {
                AT_TRACE("Invalid con_id\r\n");
                goto Error;
            }
        }

        tx_len = (uint32_t)strtoul((const char *)argv[2], &endptr, 10);
        if ((*endptr != '\0') || (tx_len > 2048)) {
            goto Error;
        }
        remote_ip = at_string_parse(argv[3]);
        if (remote_ip == NULL) {
            goto Error;
        }
        remote_port = (uint32_t)strtoul((const char *)argv[4], &endptr, 10);
        if (*endptr != '\0') {
            goto Error;
        }
        // AT_TRACE("CON: %d, len %d, ip %s, port %d\r\n", idx, tx_len, remote_ip, remote_port);
        if (type == CIP_TYPE_TCP) {
            ret = at_tcp_send(fd, tx_len);
            if (ret <= 0)
                goto Error;
        } else if (type == CIP_TYPE_UDP) {
            if (at_udp_send(fd, tx_len, remote_ip, remote_port) <= 0) {
                goto Error;
            }
        }

        AT_RSP_FREE();
        return;
#ifndef CONFIG_ATCMD_SPI
    } else if (argc == 1) {
        if (cip_info.trans_mode == CIP_TRANS_MODE_PASSTHROUGH) {
            valid_fd_cnt = cip_info_valid_fd_cnt_get();
            if (valid_fd_cnt > 1) {
                AT_TRACE("Passthrough mode support only 1 connection\r\n");
                goto Error;
            }

            idx = cip_passth_info.passth_fd_idx;
            if (idx == -1 || cip_info.cli[idx].fd < 0 || valid_fd_cnt == 0) {
                AT_TRACE("Invalid Passthrough fd\r\n");
                goto Error;
            }

            // AT_TRACE("PassThrough SEND mode enter, fd=%d\r\n", cip_info.cli[idx].fd);
            AT_RSP("OK\r\n");
            AT_RSP(">\r\n");
            AT_RSP_IMMEDIATE();
            AT_RSP_FREE();

            at_hw_passth_send(cip_info.cli[idx].fd, cip_info.cli[idx].type);

            return;
        } else {
            goto Error;
        }
#endif /* CONFIG_ATCMD_SPI */
    } else {
        goto Error;
    }
    AT_RSP("SEND OK\r\n");
    AT_RSP_OK();
    return;

Error:
#ifdef CONFIG_ATCMD_SPI
    if (spi_manager.stat == SPI_Slave_Data_Recv || spi_manager.stat == SPI_Slave_Data_ACK
        || spi_manager.stat == SPI_Slave_Idle) {
        if (ret == -1) {
            AT_RSP("error\r\n");
        } else {
            AT_RSP("SEND FAIL\r\n");
        }
        AT_RSP_IMMEDIATE();
        if (spi_manager.stat == SPI_Slave_Idle) {
            at_spi_rcv_atcmd_config(0);
        }
        return;
    }

    sys_memset(at_hw_rx_buf, 0, AT_HW_RX_BUF_SIZE);
#endif
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("Usage:\r\n");
    AT_RSP("Normal Mode Usage:\r\n");
    AT_RSP("    +CIPSEND=[con_id,]<len>[,<remote ip>,<remote port>]\r\n");
#ifndef CONFIG_ATCMD_SPI
    AT_RSP("PassThrough Mode Usage:\r\n");
    AT_RSP("    +CIPSEND\r\n");
#endif
    AT_RSP_OK();
    return;
}

#ifdef CONFIG_ATCMD_SPI
/*!
    \brief      the AT command send a tcp or udp file
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cip_send_file(int argc, char **argv)
{
    int fd, idx;
    uint32_t tx_len, file_len, segment_len;
    char *srv_ip;
    uint16_t srv_port;
    char *endptr = NULL;

    AT_RSP_START(128);

    if (cip_info.trans_mode != CIP_TRANS_MODE_FILE_TRANSFER)
        goto Error;

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            goto Error;
        }
    } else if (argc == 4) {
        idx = (int)strtol((const char *)argv[1], &endptr, 10);
        if ((*endptr != '\0') || (idx < 0) || idx >= MAX_CLIENT_NUM) {
            AT_TRACE("con_id error\r\n");
            goto Error;
        }
        if (cip_info_cli_is_free(idx)) {
            AT_TRACE("Invalid con_id\r\n");
            goto Error;
        }

        file_len = (uint32_t)strtoul((const char *)argv[2], &endptr, 10);
        if ((*endptr != '\0') || (file_len > FILE_MAX_LEN)) {
            goto Error;
        }

        segment_len = (uint32_t)strtoul((const char *)argv[3], &endptr, 10);
        if ((*endptr != '\0') || (segment_len > FILE_MAX_SEGMENT_LEN)) {
            goto Error;
        }

        AT_TRACE("CON: %d, flen %d, slen %d\r\n", idx, file_len, segment_len);
    } else {
        goto Error;
    }

    if (cip_file_transfer_info_init(idx, file_len, segment_len))
        goto Error;

    at_send_file(idx, file_len, segment_len);
    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("Usage:\r\n");
    AT_RSP("FileTransfer Mode Usage:\r\n");
    AT_RSP("    +CIPSEND=<con_id>,<file_len>,<segment_len>\r\n");
    AT_RSP_OK();
    return;
}
#endif

#ifdef CONFIG_ATCMD_SPI
void at_cip_recvdata(int argc, char **argv)
{
    int recv_len, idx;
    struct recv_data_node *p_item;
    uint8_t *data_remain = NULL;

    AT_RSP_START(AT_SPI_MAX_DATA_LEN + 30);
    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            if (cip_info.list_data_count > 0) {
                recv_len = atoi(argv[1]);
                if (recv_len < 0 || recv_len > AT_SPI_MAX_DATA_LEN) {
                    AT_TRACE("recv_len:%d error\r\n", recv_len);
                    goto Error;
                }

                sys_mutex_get(&cip_info.list_lock);
                p_item = (struct recv_data_node *)list_pop_front(&cip_info.recv_data_list);

                if ((p_item != NULL) && p_item->data && (p_item->data_len > 0)) {
                    cip_info.list_data_count--;
                    if (p_item->fd >= 0) {
                        /* Normal socket data */
                        idx = cip_info_cli_find(p_item->fd);
                        if (idx < 0 || cip_info.cli[idx].stop_flag == 1) {
                            AT_RSP("+IPD,-1,0:");
                            sys_mfree(p_item->data);
                            sys_mfree(p_item);
                        } else {
                            if (p_item->data_len <= recv_len) { // data length is smaller than request data length
                                AT_RSP("+IPD,%d,%d:", p_item->fd, p_item->data_len);
                                // copy data to AT_RSP
                                sys_memcpy((rsp_buf + rsp_buf_idx), p_item->data, p_item->data_len);
                                rsp_buf_idx += p_item->data_len;

                                sys_mfree(p_item->data);
                                sys_mfree(p_item);
                            } else {                            // data length is larger than request data length
                                AT_RSP("+IPD,%d,%d:", p_item->fd, recv_len);
                                // copy data to AT_RSP
                                sys_memcpy(rsp_buf + rsp_buf_idx, p_item->data, recv_len);
                                rsp_buf_idx += recv_len;

                                // copy remain data to a new buff, and then add it to list header again.
                                data_remain = sys_malloc(p_item->data_len - recv_len);
                                if (data_remain == NULL) {
                                    AT_TRACE("data_remain malloc failed, len:%d\r\n", (p_item->data_len - recv_len));
                                    sys_mfree(p_item->data); // when malloc fail, drop this data
                                    sys_mfree(p_item);
                                    goto Error;
                                }
                                sys_memcpy(data_remain, (p_item->data + recv_len), (p_item->data_len - recv_len));
                                sys_mfree(p_item->data);
                                p_item->data = data_remain;
                                p_item->data_len = (p_item->data_len - recv_len);
                                list_push_front(&cip_info.recv_data_list, &p_item->list_hdr);
                                cip_info.list_data_count++;
                            }
                        }
                    }
                }
                sys_mutex_put(&cip_info.list_lock);
            } else {
                AT_RSP("+IPD,-1,0:");
            }
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    cip_info.triger_count = 0;
    return;

Error:
    AT_RSP_ERR();
    cip_info.triger_count = 0;
    return;
Usage:
    AT_RSP("Usage:\r\n");
    AT_RSP("    +CIPRECVDATA=<recv_len>\r\n");
    AT_RSP_OK();
    return;
}
#endif

/*!
    \brief      the AT command start or stop a tcp/udp server
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cip_server(int argc, char **argv)
{
    uint8_t enable;
    uint16_t port = 0;
    char *type = NULL;
    char *endptr = NULL;

    AT_RSP_START(128);
    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            if (cip_info.local_srv_fd >= 0) {
                AT_RSP("+CIPSERVER:1,%s,%d,%d\r\n", (cip_info.local_srv_type == CIP_TYPE_TCP ? "TCP" : "UDP"), cip_info.local_srv_port, cip_info.local_srv_fd);
            } else {
                AT_RSP("+CIPSERVER:0\r\n");
            }
        } else {
            goto Error;
        }
    } else if ((argc == 2) || (argc == 4)) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            enable = (uint32_t)strtoul((const char *)argv[1], &endptr, 10);
            if ((*endptr != '\0') || (enable > 1)) {
                goto Error;
            }
            if (argc == 4) {
                type = at_string_parse(argv[2]);
                port = (uint32_t)strtoul((const char *)argv[3], &endptr, 10);
                if (*endptr != '\0') {
                    goto Error;
                }
            }
            if (enable) {
                if (multi_connection_enable == 0) {
                    AT_RSP("Please use cmd AT+CIPMUX to enable multiple connections.\r\n");
                    goto Error;
                }
                if (cip_info.local_srv_fd >= 0) {
                    AT_TRACE("Already run\r\n");
                    goto Error;
                }
#ifndef CONFIG_ATCMD_SPI
                if (cip_info.trans_mode == CIP_TRANS_MODE_PASSTHROUGH &&
                    cip_info_valid_fd_cnt_get() > 0)
                    goto Error;
#endif
                if (type == NULL) {
                    AT_TRACE("Invalid type.\r\n");
                    goto Error;
                }
                if (strncmp(type, "TCP", 3) == 0) {
                    if (tcp_udp_server_start(CIP_TYPE_TCP, port) < 0) {
                        goto Error;
                    }
                } else if (strncmp(type, "UDP", 3) == 0) {
                    if (tcp_udp_server_start(CIP_TYPE_UDP, port) < 0) {
                        goto Error;
                    }
                } else {
                    AT_TRACE("Invalid type.\r\n");
                    goto Error;
                }

                if (cip_task_started == 0) {
                    if (sys_task_create_dynamic((const uint8_t *)"Cip Rcv",
                                    CIP_RECV_STACK_SIZE, CIP_RECV_PRIO,
                                    (task_func_t)cip_recv_task, NULL) == NULL)
                        goto Error;
                    cip_task_started = 1;
                }
            } else {
                tcp_udp_server_stop();
            }
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+CIPSERVER=<mode:0-1>[,<type>,<port>]\r\n");
    AT_RSP_OK();
    return;
}

/*!
    \brief      the AT command free tcpip information, close client connection
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cip_close(int argc, char **argv)
{
    int con_id, i;
    char *endptr = NULL;
    int active_sock_num = 0;

    AT_RSP_START(128);
    if (argc == 1) {
        if (multi_connection_enable == 0) {
            if (cip_info.cli[0].fd > -1) {
                at_cip_close_all();
                AT_RSP("CLOSED\r\n");
            } else {
                AT_RSP("No active connection.\r\n");
            }
        } else {
            goto Error;
        }
    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            con_id = (int)strtol((const char *)argv[1], &endptr, 10);
            if ((*endptr != '\0') || (con_id < 0) || (con_id > MAX_CLIENT_NUM)) {
                goto Error;
            }
            if (con_id == MAX_CLIENT_NUM) {
                if (cip_info.local_srv_fd == -1) {
                    at_cip_close_all();
                } else {
                    for (i = 0; i < MAX_CLIENT_NUM; i++) {
                        if (cip_info.cli[i].fd >= 0) {
                            cip_info.cli[i].stop_flag = 1;
                        }
                    }
                }
                AT_RSP("CLOSED\r\n");
                AT_RSP_OK();
                return;
            }
            if (cip_info.cli[con_id].fd < 0) {
                AT_RSP("Not active connection.\r\n");
                AT_RSP_OK();
                return;
            }
            if (multi_connection_enable == 0) {
                at_cip_close_all();
            } else {
                if (cip_info.local_srv_fd != -1) {
                    active_sock_num++;
                }
                if (active_sock_num == 0) {
                    for (i = 0; i < MAX_CLIENT_NUM; i++) {
                        if (con_id == i) {
                            continue;
                        }
                        if (cip_info.cli[i].fd > -1) {
                            active_sock_num++;
                            break;
                        }
                    }
                }

                if (active_sock_num > 0) {
                    cip_info.cli[con_id].stop_flag = 1;
                } else {
                    at_cip_close_all();
                }
            }
            AT_RSP("CLOSED %d\r\n", con_id);
        }
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+CIPCLOSE=[con_id]\r\n");
    AT_RSP_OK();
    return;
}

/*!
    \brief      the AT command show wifi status
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cip_status(int argc, char **argv)
{
    int vif_idx = WIFI_VIF_INDEX_DEFAULT;

    AT_RSP_START(512);
    if (argc == 1) {
        if (wifi_vif_is_sta_connected(vif_idx)) {
            if (cip_info.cli_num > 0) {
                AT_RSP("STATUS: 3\r\n");
            } else {
                AT_RSP("STATUS: 2\r\n");
            }
        } else if (wifi_vif_is_sta_handshaked(vif_idx)) {
            AT_RSP("STATUS: 4\r\n");
        } else {
            AT_RSP("STATUS: 5\r\n");
        }
    } else {
        goto Error;
    }
    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
}

/*!
    \brief      the AT command show tcpip information
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cip_state(int argc, char **argv)
{
    int i = 0;
    char type[4];

    AT_RSP_START(512);
    if (argc == 1) {
        for (i = 0; i < MAX_CLIENT_NUM; i++) {
            if (cip_info.cli[i].fd >= 0) {
                if (cip_info.cli[i].type == CIP_TYPE_TCP)
                    strcpy(type, "TCP");
                else
                    strcpy(type, "UDP");
                AT_RSP("+CIPSTATE:%d,%s,"IP_FMT",%d,%d,%d,%d\r\n",
                        i, type, IP_ARG(cip_info.cli[i].remote_ip), cip_info.cli[i].remote_port,
                        cip_info.cli[i].local_port, cip_info.cli[i].fd, cip_info.cli[i].role);
            }
        }
    } else {
        goto Error;
    }
    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
}

/*!
    \brief      the AT command set transfer interval in passthrough mode
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_trans_interval(int argc, char **argv)
{
    char *endptr = NULL;
    uint32_t trans_intvl = 20;

    AT_RSP_START(32);

    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            AT_RSP("+TRANSINTVL:%u\r\n", cip_info.trans_intvl);
        } else {
            goto Error;
        }
    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            trans_intvl = (uint32_t)strtoul((const char *)argv[1], &endptr, 10);
            if ((*endptr != '\0') || (trans_intvl > 1000)) {
                goto Error;
            }
            cip_info.trans_intvl = trans_intvl;
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+TRANSINTVL=<interval>\r\n");
    AT_RSP_OK();
    return;
}

/*!
    \brief      the AT command set transfer mode
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cip_mode(int argc, char **argv)
{
    int mode;

    AT_RSP_START(32);
    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            AT_RSP("+CIPMODE:%d\r\n", cip_info.trans_mode);
        } else {
            goto Error;
        }
    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            mode = atoi(argv[1]);
            if (mode == CIP_TRANS_MODE_NORMAL) {
                cip_info.trans_mode = CIP_TRANS_MODE_NORMAL;
#ifndef CONFIG_ATCMD_SPI
            } else if (mode == CIP_TRANS_MODE_PASSTHROUGH) {
                if (cip_info_valid_fd_cnt_get() > 1) {
                    AT_TRACE("Passthrough mode support only 1 connection\r\n");
                    goto Error;
                }

                if (cip_passth_info.passth_fd_idx == -1) {
                    AT_TRACE("Invalid Passthrough fd\r\n");
                    goto Error;
                }
                cip_info.trans_mode = CIP_TRANS_MODE_PASSTHROUGH;
#else
            } else if (mode == CIP_TRANS_MODE_FILE_TRANSFER) {
                cip_info.trans_mode = CIP_TRANS_MODE_FILE_TRANSFER;
#endif
            } else {
                AT_TRACE("Unknown transfer mode:%d\r\n", mode);
                goto Error;
            }
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+CIPMODE=<mode:0-1>\r\n");
    AT_RSP_OK();
    return;
}

int at_parse_ip4(char *str, uint32_t *ip)
{
    char *token;
    uint32_t a, i, j;

    token = strchr(str, '/');
    *ip = 0;

    for (i = 0; i < 4; i++) {
        if (i < 3) {
            token = strchr(str, '.');
            if (!token)
                return -1;
            *token++ = '\0';
        }
        for (j = 0; j < strlen(str); j++) {
            if (str[j] < '0' || str[j] > '9')
            return -1;
        }

        a = atoi(str);
        if (a > 255)
            return -1;
        str = token;
        *ip += (a << (i * 8));
    }

    return 0;
}

/*!
    \brief      the AT command set station ip
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cip_sta_ip(int argc, char **argv)
{
    struct wifi_ip_addr_cfg ip_cfg;

    AT_RSP_START(256);
    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] != AT_QUESTION) {
            goto Error;
        }
        if (!wifi_get_vif_ip(WIFI_VIF_INDEX_DEFAULT, &ip_cfg))
        {
            AT_RSP("+CIPSTA: "IP_FMT"\r\n", IP_ARG(ip_cfg.ipv4.addr));
            AT_RSP("+CIPSTA: "IP_FMT"\r\n", IP_ARG(ip_cfg.ipv4.mask));
            AT_RSP("+CIPSTA: "IP_FMT"\r\n", IP_ARG(ip_cfg.ipv4.gw));
#ifdef CONFIG_IPV6_SUPPORT
            {
                char ip6_local[IPV6_ADDR_STRING_LENGTH_MAX] = {0};
                char ip6_unique[IPV6_ADDR_STRING_LENGTH_MAX] = {0};
                if (!wifi_get_vif_ip6(WIFI_VIF_INDEX_DEFAULT, ip6_local, ip6_unique)) {
                    AT_RSP("+CIPSTA: [%s]\r\n", ip6_local);
                    AT_RSP("+CIPSTA: [%s]\r\n", ip6_unique);
                }
            }
#endif
        } else {
            goto Usage;
        }

    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;

    } else if (argc == 4) {
        ip_cfg.mode = IP_ADDR_STATIC_IPV4;
        net_if_use_static_ip(true);
        if (at_parse_ip4(at_string_parse(argv[1]), &ip_cfg.ipv4.addr))
            goto Usage;
        if (at_parse_ip4(at_string_parse(argv[2]), &ip_cfg.ipv4.mask))
            goto Usage;
        if (at_parse_ip4(at_string_parse(argv[3]), &ip_cfg.ipv4.gw))
            goto Usage;
        AT_TRACE("+CIPSTA: set "IP_FMT", "IP_FMT", "IP_FMT"\r\n",
                    IP_ARG(ip_cfg.ipv4.addr), IP_ARG(ip_cfg.ipv4.mask), IP_ARG(ip_cfg.ipv4.gw));

        wifi_set_vif_ip(WIFI_VIF_INDEX_DEFAULT, &ip_cfg);

    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+CIPSTA=<ip>,<netmask>,<gw>\r\n");
    AT_RSP_OK();
    return;
}

/*!
    \brief      the AT command enable/disable multiple connections
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cip_mux(int argc, char **argv)
{
    AT_RSP_START(128);
    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            AT_RSP("+CIPMUX:%d\r\n", multi_connection_enable);
        } else {
            goto Error;
        }
    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            if ((cip_info.cli_num > 0) || (cip_info.local_srv_fd >= 0)) {
                AT_RSP("Please close all connections before setting multiple connections.\r\n");
                goto Error;
            }
            multi_connection_enable = atoi(argv[1]);
            if (multi_connection_enable != 0 && multi_connection_enable != 1) {
                goto Error;
            }
        }
    } else {
        goto Error;
    }
    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+CIPMUX=<mode:0-1>\r\n");
    AT_RSP_OK();
    return;
}

/*!
    \brief      the AT command show ap and station ip
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cip_ip_addr_get(int argc, char **argv)
{
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(WIFI_VIF_INDEX_DEFAULT);
    struct wifi_ip_addr_cfg ip_cfg;

    AT_RSP_START(256);
    if (argc == 1) {
        if (wifi_get_vif_ip(WIFI_VIF_INDEX_DEFAULT, &ip_cfg)) {
            goto Error;
        }
        if (wvif->wvif_type == WVIF_AP) {
            AT_RSP("+CIFSR:APIP,"IP_FMT"\r\n", IP_ARG(ip_cfg.ipv4.addr));
            AT_RSP("+CIFSR:APMAC,"MAC_FMT"\r\n", MAC_ARG(wvif->mac_addr.array));
        } else if(wvif->wvif_type == WVIF_STA) {
            AT_RSP("+CIFSR:STAIP,"IP_FMT"\r\n", IP_ARG(ip_cfg.ipv4.addr));
            AT_RSP("+CIFSR:STAMAC,"MAC_FMT"\r\n", MAC_ARG(wvif->mac_addr.array));
        }
    } else {
        goto Error;
    }
    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
}

/*!
    \brief      the AT command used for domain name resolution
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cip_domain(int argc, char **argv)
{
    char *domain, *endptr = NULL;
    uint32_t ip_network = 1;
    struct addrinfo hints, *res, *p;
    void *ptr;
#ifdef CONFIG_IPV6_SUPPORT
    char ip_addr[64];
#else
    char ip_addr[32];
#endif

    AT_RSP_START(128);
    if (argc >= 2 && argc <= 3) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            domain = at_string_parse(argv[1]);
            if (domain == NULL) {
                goto Error;
            }
            if (argc == 3) {
                ip_network = (uint32_t)strtoul((const char *)argv[2], &endptr, 10);
                if ((*endptr != '\0') || (ip_network < 1) || (ip_network > 3)) {
                    goto Error;
                }
            }
            memset(&hints, 0, sizeof(hints));
            hints.ai_socktype = SOCK_STREAM;
            if (ip_network == 1) {
#ifdef CONFIG_IPV6_SUPPORT
                hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
#else
                hints.ai_family = AF_INET; // IPv4 only
#endif
            } else if (ip_network == 2) {
                hints.ai_family = AF_INET; // IPv4
            } else if (ip_network == 3) {
#ifdef CONFIG_IPV6_SUPPORT
                hints.ai_family = AF_INET6; // IPv6
#else
                AT_TRACE("please enable macro CONFIG_IPV6_SUPPORT\r\n");
                goto Error;
#endif
            } else {
                goto Error;
            }
            if (getaddrinfo(domain, NULL, &hints, &res) != 0) {
                goto Error;
            }
            if (ip_network == 1) {
                int found = 0;
                for (p = res; p != NULL; p = p->ai_next) {
                    if (p->ai_family == AF_INET) {
                        ptr = &((struct sockaddr_in *)p->ai_addr)->sin_addr;
                        found = 1;
                        break;
                    }
                }
#ifdef CONFIG_IPV6_SUPPORT
                if (!found) {
                    for (p = res; p != NULL; p = p->ai_next) {
                        if (p->ai_family == AF_INET6) {
                            ptr = &((struct sockaddr_in6 *)p->ai_addr)->sin6_addr;
                            found = 1;
                            break;
                        }
                    }
                }
#endif
                if (!found) {
                    freeaddrinfo(res);
                    goto Error;
                }
                inet_ntop(p->ai_family, ptr, ip_addr, sizeof(ip_addr));
            } else if (ip_network == 2) {
                ptr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
                inet_ntop(res->ai_family, ptr, ip_addr, sizeof(ip_addr));
#ifdef CONFIG_IPV6_SUPPORT
            } else {
                ptr = &((struct sockaddr_in6 *)res->ai_addr)->sin6_addr;
                inet_ntop(res->ai_family, ptr, ip_addr, sizeof(ip_addr));
#endif
            }
            AT_RSP("+CIPDOMAIN:<\"%s\">\r\n", ip_addr);
            freeaddrinfo(res);
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+CIPDOMAIN=<\"domain name\">[,<ip network>]\r\n");
    AT_RSP_OK();
    return;
}

#ifdef CONFIG_SNTP
/*!
    \brief      the AT command enable/disable SNTP service
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cip_sntp_set(int argc, char **argv)
{
    int enable = 0;
    int timezone;
    uint8_t idx = 0;
    char *sntp_server_1 = NULL;
    char *sntp_server_2 = NULL;
    char *sntp_server_3 = NULL;

    AT_RSP_START(256);
    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            AT_RSP("+CIPSNTPCFG:%u,%d", sntp_enabled(), sntp_get_timezone());
            for (idx = 0; idx < SNTP_MAX_SERVERS; idx++) {
                if (sntp_getservername(idx))
                    AT_RSP(",%s", sntp_getservername(idx));
            }
            AT_RSP("\r\n");
        } else {
            goto Error;
        }
    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            goto Error;
        }
    } else if (argc < 7 && argc > 2) {
        enable = atoi(argv[1]);
        if ((enable != 0) && (enable != 1)) {
            goto Error;
        }
        if (enable == 0) {
            sntp_disable();
            AT_RSP_OK();
            return;
        }
        if (timezone_parse(argv[2], &timezone)) {
            goto Error;
        }
        if (argc > 3) {
            sntp_server_1 = argv[3];
        }
        if (argc > 4) {
            sntp_server_2 = argv[4];
        }
        if (argc > 5) {
            sntp_server_3 = argv[5];
        }
        sntp_enable(timezone, sntp_server_1, sntp_server_2, sntp_server_3);
    } else {
        goto Error;
    }
    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+CIPSNTPCFG:<enable>,<timezone>,[<SNTP server1>,<SNTP server2>,<SNTP server3>]\r\n");
    AT_RSP_OK();
    return;
}

void at_cip_sntp_update_time_succ(void)
{
    AT_RSP_START(30);
    AT_RSP("+TIME_UPDATED\r\n");
    AT_RSP_IMMEDIATE();
    AT_RSP_FREE();
}

/*!
    \brief      the AT command get SNTP time
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cip_sntp_get_time(int argc, char **argv)
{
    char buf[32] = {0};

    AT_RSP_START(256);
    if (argc == 1) {
        if (sntp_get_time(buf, sizeof(buf))) {
            AT_RSP("Please start the SNTP or wait for the SNTP time update.\r\n");
        } else {
            AT_RSP("SNTP time: %s\n", buf);
        }
    } else {
        goto Error;
    }
    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
}

/*!
    \brief      the AT command set SNTP update interval
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cip_sntp_set_intv(int argc, char **argv)
{
    uint32_t time_intv;
    char *endptr = NULL;

    AT_RSP_START(32);
    if (argc == 1) {
        AT_RSP("+CIPSNTPINTV:%u\r\n", (sntp_get_update_intv() / 1000));
    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            time_intv = (uint32_t)strtoul((const char *)argv[1], &endptr, 10);
            if (*endptr != '\0') {
                goto Error;
            }
            time_intv = time_intv * 1000;
            sntp_set_update_intv(time_intv);
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+CIPSNTPINTV=<interval second>\r\n");
    AT_RSP_OK();
    return;
}
#endif /* CONFIG_SNTP */

#ifdef CONFIG_TINY_WEBSOCKETS
typedef struct {
    uint8_t header_cnt;
    char *header[WS_MAX_REQ_HEADER_NUM];
    uint16_t header_len[WS_MAX_REQ_HEADER_NUM];
    uint32_t all_headers_len;
} ws_header_info_t;
static ws_header_info_t req_header_info;
static char *all_headers_buf = NULL;

static struct ws_session_info_t ws_session_info[WS_MAX_LINK_NUM] = {
    {10, 120, 1024},
    {10, 120, 1024},
    {10, 120, 1024}
};

struct ws_session *ws_links[WS_MAX_LINK_NUM];

static int compare_headers_key(const char *header1, const char *header2)
{
    if (!header1 || !header2) {
        return 0;
    }

    while (*header1 && *header2 && *header1 != ':' && *header2 != ':') {
        if (tolower((unsigned char)*header1) != tolower((unsigned char)*header2)) {
            return 0;
        }
        header1++;
        header2++;
    }
    return ((*header1 == ':') && (*header2 == ':'));
}

static void at_ws_session_event_ind(struct ws_session *ws, ws_session_event_t event, uint8_t *data, size_t len)
{
    uint32_t i;
    switch (event) {
    case WS_EVENT_CONNECTED:
        AT_TRACE("websocket connected\r\n");
        break;
    case WS_EVENT_RX_TXT_DATA:
        AT_TRACE("websocket RX text data:\r\n");
        for (i = 0; i < len; i++) {
            AT_TRACE("%c", data[i]);
        }
        AT_TRACE("\r\n");
        break;
    case WS_EVENT_RX_BIN_DATA:
        AT_TRACE("websocket RX binary data:\r\n");
        for (i = 0; i < len; i++) {
            AT_TRACE("0x%02x", data[i]);
        }
        AT_TRACE("\r\n");
        break;
    case WS_EVENT_DISCONNECT:
        AT_TRACE("websocket disconnect:\r\n");
        break;
    default:
        break;
    }
}

/*!
    \brief      the AT command configure WebSocket
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ws_cfg(int argc, char **argv)
{
    uint32_t link_id, ping_interval_sec, pingpong_timeout_sec, buffer_size;
    char *endptr = NULL;

    AT_RSP_START(128);
    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            goto Error;
        }
    } else if (argc == 4 || argc == 5) {
        link_id = (uint32_t)strtoul((const char *)argv[1], &endptr, 10);
        if ((*endptr != '\0') || (link_id < 0) || (link_id > 2)) {
            goto Error;
        }
        ping_interval_sec = (uint32_t)strtoul((const char *)argv[2], &endptr, 10);
        if ((*endptr != '\0') || (ping_interval_sec < 1) || (ping_interval_sec > 7200)) {
            goto Error;
        }
        pingpong_timeout_sec = (uint32_t)strtoul((const char *)argv[3], &endptr, 10);
        if ((*endptr != '\0') || (pingpong_timeout_sec < 1) || (pingpong_timeout_sec > 7200)) {
            goto Error;
        }
        if (argc == 5) {
            buffer_size = (uint32_t)strtoul((const char *)argv[4], &endptr, 10);
            if ((*endptr != '\0') || (buffer_size < 1) || (buffer_size > 8192)) {
                goto Error;
            }
        } else {
            buffer_size = 1024;
        }
        if ((ws_links[link_id] == NULL) || (!ws_links[link_id]->run) || (ws_links[link_id]->close_sended)) {
            ws_session_info[link_id].ping_interval_sec = ping_interval_sec;
            ws_session_info[link_id].pingpong_timeout_sec = pingpong_timeout_sec;
            ws_session_info[link_id].tx_buf_size = buffer_size;
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+WSCFG=<link_id>,<ping_intv_sec>,<ping_timeout_sec>[,<buffer_size>]\r\n");
    AT_RSP_OK();
    return;
}

/*!
    \brief      the AT command set or query WebSocket headers
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ws_head(int argc, char **argv)
{
    uint16_t req_header_len;
    int i;
    char *endptr = NULL, *header_buf = NULL;

    AT_RSP_START(768);
    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            for (i = 0; i < req_header_info.header_cnt; i++) {
                AT_RSP("+WSHEAD:%d,\"%s\"\r\n", i, req_header_info.header[i]);
            }
        } else {
            goto Error;
        }
    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            req_header_len = (uint16_t)strtoul((const char *)argv[1], &endptr, 10);
            if ((*endptr != '\0')) {
                goto Error;
            }
            if ((req_header_len > WS_MAX_REQ_HEADER_LEN) || (req_header_len < 0)) {
                AT_TRACE("invalid WebSocket req_header_len: %d\r\n", req_header_len);
                goto Error;
            }
            if (req_header_len == 0) {
                for (i = 0; i < WS_MAX_REQ_HEADER_NUM; i++) {
                    if (req_header_info.header[i] != NULL) {
                        sys_mfree(req_header_info.header[i]);
                        req_header_info.header[i] = NULL;
                        req_header_info.header_len[i] = 0;
                    }
                }
                req_header_info.all_headers_len = 0;
                req_header_info.header_cnt = 0;
                if (all_headers_buf != NULL) {
                    sys_mfree(all_headers_buf);
                    all_headers_buf = NULL;
                }
            } else {
                AT_RSP_DIRECT("OK\r\n", 4);
                AT_RSP_DIRECT(">\r\n", 3);
                header_buf = sys_malloc(req_header_len);
                if (NULL == header_buf) {
                    AT_TRACE("Allocate header buffer failed (len = %u).\r\n", req_header_len);
                    goto Error;
                }

                // Block here to wait dma receive done
                at_hw_dma_receive((uint32_t)header_buf, req_header_len);
                if (strchr(header_buf, ':') == NULL) {
                    AT_TRACE("Invalid header format.\r\n");
                    sys_mfree(header_buf);
                    goto Error;
                }

                int updated = 0;
                for (i = 0; i < req_header_info.header_cnt; i++) {
                    if (compare_headers_key((const char *)header_buf, (const char *)req_header_info.header[i])) {
                        sys_mfree(req_header_info.header[i]);
                        req_header_info.all_headers_len -= req_header_info.header_len[i];
                        req_header_info.header[i] = (char *)sys_malloc(req_header_len + 1);
                        if (req_header_info.header[i] == NULL) {
                            AT_TRACE("Allocate request header buffer failed (len = %u).\r\n", req_header_len);
                            sys_mfree(header_buf);
                            goto Error;
                        }
                        sys_memcpy(req_header_info.header[i], header_buf, req_header_len);
                        req_header_info.header[i][req_header_len] = '\0';
                        req_header_info.header_len[i] = req_header_len;
                        updated = 1;
                        break;
                    }
                }
                if (updated == 0) {
                    if (req_header_info.header_cnt >= WS_MAX_REQ_HEADER_NUM) {
                        AT_TRACE("Header storage full\r\n");
                        sys_mfree(header_buf);
                        goto Error;
                    } else {
                        req_header_info.header[req_header_info.header_cnt] = (char *)sys_malloc(req_header_len + 1);
                        if (req_header_info.header[req_header_info.header_cnt] == NULL) {
                            AT_TRACE("Allocate request header buffer failed (len = %u).\r\n", req_header_len);
                            sys_mfree(header_buf);
                            goto Error;
                        }
                        sys_memcpy(req_header_info.header[req_header_info.header_cnt], header_buf, req_header_len);
                        req_header_info.header[req_header_info.header_cnt][req_header_len] = '\0';
                        req_header_info.header_len[req_header_info.header_cnt] = req_header_len;
                        req_header_info.header_cnt++;
                    }
                }
                req_header_info.all_headers_len += req_header_len;
                sys_mfree(header_buf);
                if (all_headers_buf != NULL) {
                    sys_mfree(all_headers_buf);
                }
                uint32_t all_headers_buf_len = req_header_info.all_headers_len + req_header_info.header_cnt * 2 + 1;
                all_headers_buf = (char *)sys_malloc(all_headers_buf_len);
                if (all_headers_buf == NULL) {
                    AT_TRACE("Allocate all headers buffer failed (len = %u).\r\n", all_headers_buf_len);
                    goto Error;
                }
                int len = 0, r = 0;
                for (i = 0; i < req_header_info.header_cnt; i++) {
                    if (req_header_info.header[i] != NULL) {
                        r = snprintf((char *)all_headers_buf + len, all_headers_buf_len - len, "%s", req_header_info.header[i]);
                        len += r;
                        r = snprintf((char *)all_headers_buf + len, all_headers_buf_len - len, "\r\n");
                        len += r;
                    }
                }
                all_headers_buf[len] = '\0';
            }
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+WSHEAD=<req_header_len>\r\n");
    AT_RSP_OK();
    return;
}

/*!
    \brief      the AT command open or query a WebSocket link
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ws_open(int argc, char **argv)
{
    uint32_t link_id, timeout_ms = 15000;
    char *uri, *sub_protocol = NULL, *auth = NULL;
    int i;
    char *endptr = NULL;
    bool new_session = false;

    AT_RSP_START(1024);
    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            for (i = 0; i < WS_MAX_LINK_NUM; i++) {
                if (ws_links[i] != NULL) {
                    int ws_state = -1;
                    int uri_buf_len = sizeof(ws_links[i]->conf.host) + sizeof(ws_links[i]->conf.path) + 13;
                    char *uri_buf = (char *)sys_malloc(uri_buf_len);
                    if (uri_buf == NULL) {
                        AT_TRACE("Allocate uri buffer failed.\r\n");
                        goto Error;
                    }
                    if (ws_links[i]->conf.port_default) {
                        snprintf(uri_buf, uri_buf_len, "%s://%s%s",
                                ws_links[i]->conf.scheme,
                                ws_links[i]->conf.host,
                                ws_links[i]->conf.path);
                    } else {
                        snprintf(uri_buf, uri_buf_len, "%s://%s:%d%s",
                                ws_links[i]->conf.scheme,
                                ws_links[i]->conf.host,
                                ws_links[i]->conf.port,
                                ws_links[i]->conf.path);
                    }
                    if (ws_links[i]->state == WS_STATE_UNKNOW && !ws_links[i]->run) {
                        ws_state = 0;
                    } else if (ws_links[i]->run && ws_links[i]->state == WS_STATE_INIT && ws_links[i]->reconnect_tick_ms) {
                        ws_state = 1;
                    } else if (ws_links[i]->run && ws_links[i]->state == WS_STATE_CONNECTED) {
                        ws_state = 2;
                    } else if (ws_links[i]->run && ws_links[i]->state == WS_STATE_NET_ERROR) {
                        ws_state = 3;
                    } else if (ws_links[i]->run && ws_links[i]->state == WS_STATE_CLOSING && ws_links[i]->rx_frame.op == WS_OPCODE_CLOSE) {
                        ws_state = 4;
                    }
                    AT_RSP("+WSOPEN:%d,%d,\"%s\"\r\n", i, ws_state, uri_buf);
                    sys_mfree(uri_buf);
                }
            }
        } else {
            goto Error;
        }
    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            goto Error;
        }
    } else if (argc >= 3 && argc <= 6) {
        link_id = (uint32_t)strtoul((const char *)argv[1], &endptr, 10);
        if ((*endptr != '\0') || (link_id < 0) || (link_id > 2)) {
            goto Error;
        }
        uri = at_string_parse(argv[2]);
        if (uri == NULL) {
            goto Error;
        }
        if (argc > 3) {
            sub_protocol = at_string_parse(argv[3]);
            if (sub_protocol == NULL) {
                goto Error;
            }
        }
        if (argc > 4) {
            timeout_ms = (uint32_t)strtoul((const char *)argv[4], &endptr, 10);
            if ((*endptr != '\0') || (timeout_ms < 0) || (timeout_ms > 180000)) {
                goto Error;
            }
        }
        if (argc > 5) {
            auth = at_string_parse(argv[5]);
            if (auth == NULL) {
                goto Error;
            }
        }
        if (ws_links[link_id] == NULL) {
            ws_links[link_id] = (struct ws_session *)sys_malloc(sizeof(struct ws_session));
            if (ws_links[link_id] == NULL) {
                goto Error;
            }
            sys_memset(ws_links[link_id], 0, sizeof(struct ws_session));
            new_session = true;
        }
        if (!ws_links[link_id]->run && (ws_links[link_id]->state == WS_STATE_INIT || ws_links[link_id]->state == WS_STATE_UNKNOW)) {
            if (at_ws_session_init(&ws_links[link_id],
                    uri,
                    NULL,
                    sub_protocol,
                    auth,
                    all_headers_buf,
                    &ws_session_info[link_id],
                    timeout_ms,
                    (ws_event_indicate_fun_t)at_ws_session_event_ind) < 0) {
                goto Error;
            }
            if (ws_session_start(ws_links[link_id]) < 0) {
                if (ws_links[link_id]->state == WS_STATE_INIT) {
                    ws_session_free(ws_links[link_id]);
                    ws_links[link_id] = NULL;
                }
                goto Error;
            }
            uint32_t cur_time = sys_current_time_get();
            while (sys_current_time_get() - cur_time < timeout_ms) {
                if (ws_links[link_id]->state == WS_STATE_CONNECTED) {
                    AT_RSP("+WS_CONNECTED:%d\r\n", link_id);
                    break;
                }
                sys_ms_sleep(1000);
            }
            if (ws_links[link_id]->state != WS_STATE_CONNECTED) {
                if (new_session) {
                    ws_session_close(ws_links[link_id]);
                    ws_links[link_id] = NULL;
                } else {
                    ws_links[link_id]->run = false;
                }
                goto Error;
            }
        } else {
            goto Error;
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+WSOPEN=<link_id>,<\"uri\">[,<\"subprotocol\">][,<timeout_ms>][,<\"auth\">]\r\n");
    AT_RSP_OK();
    return;
}

/*!
    \brief      the AT command send data
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ws_send(int argc, char **argv)
{
    uint32_t link_id, length, opcode = 1, timeout_ms = 10000;
    char *endptr = NULL;

    AT_RSP_START(128);
    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            goto Error;
        }
    } else if (argc >= 3 && argc <= 5) {
        link_id = (uint32_t)strtoul((const char *)argv[1], &endptr, 10);
        if ((*endptr != '\0') || (link_id < 0) || (link_id > 2)) {
            goto Error;
        }
        length = (uint32_t)strtoul((const char *)argv[2], &endptr, 10);
        if ((*endptr != '\0')) {
            goto Error;
        }
        if (length > MIN(ws_session_info[link_id].tx_buf_size - 10, sys_free_heap_size())) {
            AT_TRACE("length exceeded.\r\n");
            goto Error;
        }
        if (argc > 3) {
            opcode = (uint32_t)strtoul((const char *)argv[3], &endptr, 10);
            if ((*endptr != '\0') || (opcode < 0) || (opcode > 0xF)) {
                goto Error;
            }
        }
        if (argc > 4) {
            timeout_ms = (uint32_t)strtoul((const char *)argv[4], &endptr, 10);
            if ((*endptr != '\0') || (timeout_ms < 0) || (timeout_ms > 60000)) {
                goto Error;
            }
        }

        char *write_buf = NULL;
        write_buf = sys_malloc(length);
        if (NULL == write_buf) {
            AT_TRACE("Allocate write buffer failed (len = %u).\r\n", length);
            goto Error;
        }

        AT_RSP_DIRECT("OK\r\n", 4);
        AT_RSP_DIRECT(">\r\n", 3);
        // Block here to wait dma receive done
        at_hw_dma_receive((uint32_t)write_buf, length);

        if (ws_links[link_id] == NULL || !ws_links[link_id]->run || ws_links[link_id]->state != WS_STATE_CONNECTED) {
            sys_mfree(write_buf);
            goto Error;
        } else {
            if (ws_session_write_op(ws_links[link_id], opcode, (uint8_t *)write_buf, length, timeout_ms) < 0) {
                sys_mfree(write_buf);
                goto Error;
            }
            sys_mfree(write_buf);
            AT_RSP("SEND ");
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+WSSEND=<link_id>,<length>[,<opcode>][,<timeout_ms>]\r\n");
    AT_RSP_OK();
    return;
}

/*!
    \brief      the AT command close a Websocket link
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ws_close(int argc, char **argv)
{
    uint32_t link_id;
    char *endptr = NULL;

    AT_RSP_START(128);
    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            link_id = (uint32_t)strtoul((const char *)argv[1], &endptr, 10);
            if ((*endptr != '\0') || (link_id < 0) || (link_id > 2)) {
                goto Error;
            }
            if (ws_links[link_id] != NULL) {
                if (ws_session_close(ws_links[link_id]) < 0) {
                    goto Error;
                }
                ws_links[link_id] = NULL;
            }
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+WSCLOSE=<link_id>\r\n");
    AT_RSP_OK();
    return;
}
#endif