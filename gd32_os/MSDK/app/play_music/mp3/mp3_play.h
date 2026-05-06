/*!
    \file    mp3_play.h
    \brief   mp3 player

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

#ifndef _KWS_MP3_PLAY_H_
#define _KWS_MP3_PLAY_H_

#include "fatfs.h"

#define START_0_MP3_FILE         "start_0_mp3"
#define TTS9_FILE                 "tts9"
#define TTS39_FILE                "tts39"
#define MP3_54_CMPLT_FILE        "mp3_54_complete"
#define VOICE_UPDATE_KEY         "SV_ST"

#define MP3_UPDATE_TEMP_BIN      "temp/temp.bin"

typedef enum {
    CH_IDX,
    HK_IDX,
    EN_IDX,
    DE_IDX,
    FR_IDX,
    IT_IDX,
    ES_IDX,
    RU_IDX,
    KR_IDX,
    JA_IDX,
    PL_IDX,
} country_index_t;

#define TMP_BIN_PATH            "temp/temp.bin"
#define TEMP_READ_BUF_LEN       1024
#define MP3_INFOS_FILE          "mp3I"
#define MP3_INFOS_NEW_FILE      "mp3I_new"

#define TMP_PARSING_DIR_PATH    "mp3_temp"
#define MP3_PATH                "mp3"
#define MP3_PATH_DEL            "mp3_delete"

#define MAX_DOC_NUM             11

#define PATH_SEP  "/"
#define CN_STR    "CN"
#define HK_STR    "HK"
#define EN_STR    "EN"
#define DE_STR    "DE"
#define FR_STR    "FR"
#define IT_STR    "IT"
#define ES_STR    "ES"
#define RU_STR    "RU"
#define KR_STR    "KR"
#define JA_STR    "JA"
#define PL_STR    "PL"

#define TMP_PARSING_CN_PATH  (TMP_PARSING_DIR_PATH PATH_SEP CN_STR)
#define TMP_PARSING_HK_PATH  (TMP_PARSING_DIR_PATH PATH_SEP HK_STR)
#define TMP_PARSING_EN_PATH  (TMP_PARSING_DIR_PATH PATH_SEP EN_STR)
#define TMP_PARSING_DE_PATH  (TMP_PARSING_DIR_PATH PATH_SEP DE_STR)
#define TMP_PARSING_FR_PATH  (TMP_PARSING_DIR_PATH PATH_SEP FR_STR)
#define TMP_PARSING_IT_PATH  (TMP_PARSING_DIR_PATH PATH_SEP IT_STR)
#define TMP_PARSING_ES_PATH  (TMP_PARSING_DIR_PATH PATH_SEP ES_STR)
#define TMP_PARSING_RU_PATH  (TMP_PARSING_DIR_PATH PATH_SEP RU_STR)
#define TMP_PARSING_KR_PATH  (TMP_PARSING_DIR_PATH PATH_SEP KR_STR)
#define TMP_PARSING_JA_PATH  (TMP_PARSING_DIR_PATH PATH_SEP JA_STR)
#define TMP_PARSING_PL_PATH  (TMP_PARSING_DIR_PATH PATH_SEP PL_STR)

#define MP3_FILE_PATH_MAX    (32)
#define MP3_DIR_NAME_LEN    (MP3_FILE_PATH_MAX >> 1)

extern const char* doc_name[MAX_DOC_NUM];
extern const char* doc_path[MAX_DOC_NUM];

typedef enum {
    OVERRIDE_ALL_OP = 1,
    OVERRIDE_PARTIAL_OP,
    UPDATE_PARTIAL_OP,
} update_op_t;

typedef struct {
    uint8_t mp3_doc_num;
    uint16_t mp3_version[3];
} mp3_file_info;

typedef enum {
    UPD_IDLE,
    RECVING_ST,
    PARSING_ST,
    UPD_CMPLT_ST,
} update_state_t;

uint8_t play_mp3(uint16_t idx);

int mp3_wait_ready(void);

const uint16_t * mp3_get_version(void);

bool mp3_get_ready(void);

bool mp3_update(uint8_t *p_val, uint16_t len);

bool mp3_finish(bool success, uint8_t *p_md5_array);

bool mp3_parse_temp_bin(FIL *file);

void mp3_init(void);

void mp3_file_mutex_acquire(void);

void mp3_file_mutex_free(void);

bool mp3_get_file_path(uint16_t idx, char *p_path, uint32_t max_len);

#endif // _KWS_MP3_PLAY_H_