/*!
    \file    gd_wifi_ota.h
    \brief   OTA program for gd

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

#ifndef _GD_WIFI_OTA_H_
#define _GD_WIFI_OTA_H_


#include <stddef.h>
#include "cyclic_buffer.h"

#define CONFIG_SUPER_DEBUG
#ifdef CONFIG_SUPER_DEBUG
#define gd_printf  printf
#else /* CONFIG_SUPER_DEBUG */
#define gd_printf(...)
#endif

#define gd_info(fmt, ...)          \
    do {                                        \
        if (gd_info_print) {      \
            printf(fmt, ## __VA_ARGS__);     \
        }                                       \
    } while (0)

#define GD_NO_ERR                        0
#define GD_INV_CMD_FORMAT_ERR            1
#define GD_UNKOWN_CMD                    2
#define GD_INV_OPERATOR                  3
#define GD_INV_PARAMS                    4
#define GD_OPERATOR_NOT_PERMIT           5
#define GD_FILE_NOT_FOUND                6
#define GD_LACK_OF_DATA                  7


#define GD_MAX_URL_LEN                     (1024)
#define GD_MAX_URL_JSON_LEN                (1024)
#define GD_MAX_SERVER_HOST_LEN             (256)
#define GD_MAX_SERVER_HEAER_LEN            (1024)

#define GD_OTA_SEGMENT_LEN                 (4096) //  Music


#define GD_OTA_TIMEOUT_LIMIT               (15000) //15s

typedef enum {
    GD_OTA_ST_IDLE = 0,
    GD_OTA_ST_QUERY,           // Querying OTA fw version
    GD_OTA_ST_READY,           // Query OK, ready for update
    GD_OTA_ST_IN_PROGRESS,
    GD_OTA_ST_PENDING,         // Error occurs
    GD_OTA_ST_COMPLETED,
} gd_ota_state_t;


typedef enum {
    GD_OTA_QUERY_OK = 0,
    GD_OTA_QUERY_PARAM_ERR = -1,
    GD_OTA_QUERY_URL_ERR = -2,
    GD_OTA_QUERY_CONTENT_ERR = -3,
    GD_OTA_QUERY_ERR = -4,
} gd_ota_query_err_t;

typedef enum {
    /* Rsp to Controller */
    GD_OTA_OK = 0,
    GD_OTA_PARAM_ERR = -1,
    GD_OTA_SERVER_ACCESS_ERR = -2,
    GD_OTA_DOWNLOAD_ERR = -3,
    GD_OTA_VERIFY_ERR = -4,
    GD_OTA_TIMEOUT = -5,
    GD_OTA_CONTOLLER_END = -6,
    GD_OTA_ERR = -7,

    /* Internal Error */
    GD_OTA_INTERNAL_BUSY = -10,         // Another ota is in progress
    GD_OTA_INTERNAL_MEM_ERR = -11,
    GD_OTA_INTERNAL_PARAM_ERR = -12,
} gd_ota_err_t;

typedef struct gd_ota_context {
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
    uint32_t buf_offset;        // buf contains http header, the body offset
    os_sema_t buf_sema;
    os_mutex_t buf_lock;

    os_timer_t ota_tmr;

    char version[32];           // for VW553 or music
    unsigned char checksum[16];        // md5 of VW553 or music

    gd_ota_state_t state;

    gd_ota_err_t reason;
} gd_ota_ctx_t;

void gd_at_music_update(int argc, char **argv);

#endif /* _GD_WIFI_OTA_H_ */
