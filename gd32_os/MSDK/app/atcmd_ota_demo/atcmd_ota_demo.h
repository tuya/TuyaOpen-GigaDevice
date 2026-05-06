/*!
    \file    atcmd_ota_demo.h
    \brief   AT command to demo OTA run on alibaba ecs server for GD32VW55x SDK

    \version 2025-08-21, V1.0.0, firmware for GD32VW55x
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

#ifndef _ATCMD_OTA_DEMO_H_
#define _ATCMD_OTA_DEMO_H_

#include "app_cfg.h"

#ifdef CONFIG_ATCMD_OTA_DEMO

#include "atcmd_mqtt.h"

/*-----------------OTA Definitions-----------------------*/
#define OTA_MAX_URL_LEN                         (512)
#define OTA_MAX_URL_JSON_LEN                    (1024)
#define OTA_MAX_SERVER_HOST_LEN                 (256)
#define OTA_MAX_SERVER_HEAER_LEN                (1024)

#define OTA_FW_SEGMENT_LEN                      (4096) // FW1 or Music

#define OTA_TIMEOUT_LIMIT                       (50000) //50s
#define OTA_MUSIC_TIMEOUT_LIMIT                 (40000) //40s

#define OTA_FW_CHECKSUM_LEN                     (16)
#define OTA_FW_CHECKSUM_STR_LEN                 (32)


/*-----------------OTA Demo Options-----------------------*/
#define OTA_TASK_STK_SIZE                       2048  /* 1024 */
#define OTA_TASK_PRIO                           OS_TASK_PRIORITY(1)

typedef enum {
    OTA_OK = 0,
    OTA_PARAM_ERR = -1,
    OTA_SERVER_ACCESS_ERR = -2,
    OTA_QUERY_URL_ERR = -3,
    OTA_QUERY_CONTENT_ERR = -4,
    OTA_DOWNLOAD_ERR = -5,
    OTA_VERIFY_ERR = -6,
    OTA_TIMEOUT = -7,
    OTA_CONTOLLER_END = -8,
    OTA_UNKOWN_ERR = -9,

    /* Internal Error */
    OTA_INTERNAL_BUSY = -10,      // Another ota is in progress
    OTA_INTERNAL_MEM_ERR = -11,
    OTA_INTERNAL_FLASH_ERR = -12,
    OTA_INTERNAL_PARAM_ERR = -13,
} ota_err_t;

typedef enum {
    OTA_ST_IDLE = 0,
    OTA_ST_QUERY,           // Querying OTA fw version
    OTA_ST_READY,           // Query OK, ready for update
    OTA_ST_IN_PROGRESS,
    OTA_ST_PENDING,         // Error occurs
    OTA_ST_COMPLETED,
} ota_state_t;

typedef struct ota_context {
    char fw_name[8];            // FW0/FW1/MUSIC
    char *query_url;
    char *update_url;
    uint32_t file_length;
    int32_t current_offset;
    int32_t request_offset;

    uint32_t segment_length;    // Max fixed segment length
    uint32_t real_length;       // real length of the current transfer

    union {
        uint8_t *buf;
        cyclic_buf_t cyc_buf;
    };
    uint32_t buf_len;
    uint32_t buf_offset;        // buf contains http header, the body offset


    os_timer_t ota_tmr;

    char version[32];           // for VW553 or music
    unsigned char checksum[16];        // md5 of VW553 or music

    ota_state_t state;

    ota_err_t reason;
} ota_ctx_t;

void at_ota_demo_start(int argc, char **argv);
void at_ota_demo_stop(int argc, char **argv);

#endif /* CONFIG_ATCMD_OTA_DEMO */

#endif /* _ATCMD_OTA_DEMO_H_ */