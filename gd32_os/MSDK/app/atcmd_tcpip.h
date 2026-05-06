/*!
    \file    atcmd_tcpip.h
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

#ifndef __ATCMD_TCPIP_H__
#define __ATCMD_TCPIP_H__

#include "lwipopts.h"
#include "lwip/netdb.h"
#include "slist.h"

#define CIP_TYPE_TCP                 0
#define CIP_TYPE_UDP                 1
#define CIP_ROLE_CLIENT              0
#define CIP_ROLE_SERVER              1
#define MAX_CLIENT_NUM               (MEMP_NUM_NETCONN - 1 - 2)  /* Reserved 1 netconn for tcp server, 2 for local socket */

#define CIP_RECV_STACK_SIZE          512
#define CIP_RECV_PRIO                OS_TASK_PRIORITY(1)

#define PASSTH_TX_BUF_LEN               8192
#define PASSTH_START_TRANSFER_LEN       2920
#define PASSTH_TERMINATE_STR            "+++"
#define CIP_TRANSFER_INTERVAL_DEFAULT   20 //ms

#define FILE_MAX_LEN                    0x6400000 //100MB
#define FILE_MAX_SEGMENT_LEN            0x100000  //1MB
#define FILE_SEGMENT_CRC_LEN            4 //4Bytes

typedef enum _trans_mode {
    CIP_TRANS_MODE_NORMAL = 0,
#ifndef CONFIG_ATCMD_SPI
    CIP_TRANS_MODE_PASSTHROUGH,
#else
    CIP_TRANS_MODE_FILE_TRANSFER,
#endif
} trans_mode_t;

typedef enum _mux_mode {
    CIP_MUX_MODE_SIGNLE = 0,
    CIP_MUX_MODE_MULTIPLE,
} mux_mode_t;

#define MAX_RECV_DATA_NUM_IN_LIST 20
typedef struct _client_info {
    int         fd;
    uint8_t     type;
    uint8_t     role;
    uint8_t     stop_flag;
    uint32_t    remote_ip;
    uint16_t    remote_port;
    uint16_t    local_port;
} client_info_t;

typedef struct _cip_info {
    volatile uint8_t trans_mode;
    uint32_t        trans_intvl;
    int             local_srv_fd;
    uint16_t        local_srv_port;
    // local_srv_type: CIP_TYPE_TCP, CIP_TYPE_UDP.
    uint8_t         local_srv_type;
    uint8_t         local_srv_stop;
    client_info_t   cli[MAX_CLIENT_NUM];
    uint32_t        cli_num;
    uint8_t         tcp_udp_start_process_num;
#ifdef CONFIG_ATCMD_SPI
    struct list recv_data_list;
    os_mutex_t list_lock;
    uint8_t list_data_count;
    uint8_t triger_count;
#endif
} cip_info_t;

#ifdef CONFIG_ATCMD_SPI
#define AT_SPI_MAX_DATA_LEN         2048
struct recv_data_node {
    struct list_hdr list_hdr;
    int fd; // store which fd store this data
    uint8_t *data;
    size_t  data_len;
};
#endif

enum at_local_event_id
{
    AT_LOCAL_TCP_SEND_EVENT = 1,
    AT_LOCAL_UDP_SEND_EVENT = 2,
    AT_LOCAL_MAX_EVENT_IDX
};

struct at_local_tcp_send {
    uint16_t event_id;
    int16_t sock_fd;
    uint32_t send_data_addr;
    uint32_t send_data_len;
};

struct at_local_udp_send {
    uint16_t event_id;
    int16_t sock_fd;
    uint32_t send_data_addr;
    uint32_t send_data_len;
    struct sockaddr_in to;
    socklen_t tolen;
};

void cip_info_init(void);
void at_cip_ping(int argc, char **argv);
void at_cip_sta_ip(int argc, char **argv);
void at_cip_start(int argc, char **argv);
void at_cip_send(int argc, char **argv);
void at_cip_send_file(int argc, char **argv);
void at_cip_server(int argc, char **argv);
void at_cip_close(int argc, char **argv);
void at_cip_status(int argc, char **argv);
void at_cip_mode(int argc, char **argv);
void at_trans_interval(int argc, char **argv);
void at_cip_ip_addr_get(int argc, char **argv);
void cip_info_reset(void);

#endif  /* __ATCMD_TCPIP_H__ */
