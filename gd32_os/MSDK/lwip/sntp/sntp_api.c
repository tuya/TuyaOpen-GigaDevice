/*!
    \file    sntp_api.c
    \brief   the api of sntp service

    \version 2025-04-23, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2025, GigaDevice Semiconductor Inc.

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
#include <string.h>
#include <time.h>
#include "app_cfg.h"
#include "wrapper_os.h"
#include "sntp_api.h"

#ifdef CONFIG_SNTP

#define SNTP_SERVER_0       "cn.pool.ntp.org"

int sntp_timezone = 0;
uint64_t sntp_update_local_time = 0;
uint32_t sntp_update_time = 0;
uint32_t sntp_update_intv = 86400;

#define SNTP_SERVER_USER_NUM 3
char *sntp_server_s[SNTP_SERVER_USER_NUM] = {NULL, NULL, NULL};

extern uint64_t get_sys_local_time_us();
// Redefine SNTP_SET_SYSTEM_TIME for debug
void sntp_set_system_time(uint32_t sec)
{
    sntp_update_local_time = get_sys_local_time_us();

    sntp_update_time = sec;

#ifdef CONFIG_ATCMD
extern void at_cip_sntp_update_time_succ(void);
    at_cip_sntp_update_time_succ();
#endif
}

int timezone_parse(char *argv, int *timezone)
{
    char *str = argv;
    uint32_t data;
    uint8_t hour, minute;
    uint8_t negative_flag = 0;
    char *endptr = NULL;

    if (argv[0] == '-') {
        str = argv + 1;
        negative_flag = 1;
    }

    data = (uint32_t)strtoul((const char *)str, &endptr, 10);
    if (*endptr != '\0') {
        return -1;
    }

    hour = data / 100;
    minute = data % 100;
    if ((minute > 59) || (hour > 14) || (negative_flag && (hour > 12))) {
        return -1;
    }

    *timezone = hour * 3600 + minute * 60;
    if (negative_flag) {
        *timezone = -(*timezone);
    }

    return 0;
}

void sntp_set_update_intv(uint32_t interval_ms)
{
    // SNTPv4 RFC 4330 enforces a minimum update time of 15 seconds.
    if (interval_ms < 15000) {
        interval_ms = 15000;
    }
    sntp_update_intv = interval_ms;
}

uint32_t sntp_get_update_intv(void)
{
    return sntp_update_intv;
}

int sntp_get_timezone(void)
{
    uint8_t hour, minute;
    uint8_t negative_flag = 0;
    int timezone = sntp_timezone;

    if (timezone < 0) {
        negative_flag = 1;
        timezone = -timezone;
    }

    hour = timezone / 3600;
    minute = (timezone % 3600) / 60;
    timezone = hour * 100 + minute;
    if (negative_flag) {
        timezone = -timezone;
    }

    return timezone;
}

int sntp_get_time(char *buf, uint32_t buf_len)
{
    uint64_t local_time;
    uint32_t diff_time;

    struct tm current_time_val;
    time_t current_time;

    if (!sntp_enabled() || (sntp_update_time == 0)) {
        return -1;
    }

    local_time = get_sys_local_time_us();
    diff_time = ((local_time - sntp_update_local_time) / 1000000) & 0xffffffff;
    current_time = (time_t)(sntp_update_time + diff_time + sntp_timezone);
    localtime_r(&current_time, &current_time_val);
    strftime(buf, buf_len, "%Y-%m-%d %A %H:%M:%S", &current_time_val);

    return 0;
}

int sntp_server_set(uint8_t idx, char *server)
{
    if (sntp_server_s[idx]) {
        printf("old sntp server need free.\r\n");
        return -1;
    }

    sntp_server_s[idx] = sys_zalloc(strlen(server) + 1);
    if (!sntp_server_s[idx]) {
        printf("sntp server malloc fail.\r\n");
        return -1;
    }

    sys_memcpy(sntp_server_s[idx], server, strlen(server));
    sntp_setservername(idx, sntp_server_s[idx]);

    return 0;
}

int sntp_enable(int timezone, char *server_1,  char *server_2, char *server_3)
{
    uint8_t idx = 1;
    int ret;

    sntp_timezone = timezone;

    if (sntp_enabled()) {
        sntp_disable();
    }

    sntp_setoperatingmode(SNTP_OPMODE_POLL);

    sntp_setservername(0, SNTP_SERVER_0);
    if (server_1) {
        ret = sntp_server_set(idx, server_1);
        if (ret == 0) {
            idx++;
        }
    }
    if (server_2) {
        ret = sntp_server_set(idx, server_2);
        if (ret == 0) {
            idx++;
        }
    }
    if (server_3) {
        ret = sntp_server_set(idx, server_3);
        if (ret == 0) {
            idx++;
        }
    }

    sntp_init();

    return 0;
}

void sntp_disable(void)
{
    uint8_t i;

    for (i = 1; i < SNTP_SERVER_USER_NUM; i++) {
        if (sntp_server_s[i]) {
            sys_mfree(sntp_server_s[i]);
            sntp_server_s[i] = NULL;
            sntp_setservername(i, NULL);
        }
    }
    sntp_stop();
    sntp_update_time = 0;
}

#endif /* CONFIG_SNTP */
