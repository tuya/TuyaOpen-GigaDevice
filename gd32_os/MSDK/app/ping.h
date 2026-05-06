/*!
    \file    ping.h
    \brief   Header file of ping .

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

#ifndef _PING_H_
#define _PING_H_

#include "lwip/ip_addr.h"
#include "wrapper_os.h"

#define PING_TASK_STACK_SIZE                   512
#define PING_TASK_PRIO                         OS_TASK_PRIORITY(1)

struct ping_info_t {
    /* ping parameters */
#if LWIP_IPV6
    char ping_ip[64];
    u8_t ip_type;
#else
    char ping_ip[16];
#endif
    u32_t ping_cnt;
    size_t ping_size;
    u32_t ping_interval;

    /* ping variables */
    u16_t ping_seq_num;
    u16_t ping_max_delay;
    u16_t ping_min_delay;
    u32_t ping_avg_delay;
    u32_t ping_time;
    u32_t ping_recv_count;
    u8_t *reply_buf;
    u8_t *send_buf; /* will not be released until recv echo reply or timeout*/
#ifdef CONFIG_ATCMD
    char ping_res[512];
    u32_t ping_res_len;
#endif
};

err_t ping(struct ping_info_t *ping_info);
void cmd_ping(int argc, char **argv);

#endif /* _PING_H_ */
