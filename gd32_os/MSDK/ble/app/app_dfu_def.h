/*!
    \file    app_dfu_def.h
    \brief   Header file of dfu defination .

    \version 2024-07-31, V1.0.0, firmware for GD32VW55x
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

#ifndef _APP_DFU_DEF_H_
#define _APP_DFU_DEF_H_

#include <stdint.h>
#include "ble_utils.h"
#include "mbedtls/version.h"
#if (MBEDTLS_VERSION_NUMBER == 0x02110000)
#include "rom_export_mbedtls.h"
#else
#include "mbedtls/sha256.h"
#endif

#define FEAT_VALIDATE_FW_SUPPORT      1
#define FEAT_CRC_SUPPORT              1

#define CMD_MAX_LEN             128
#define SHA256_RESULT_SIZE      32
#define FLASH_WRITE_SIZE        4096
#define BLE_TRANSMIT_SIZE       128
#define DFU_TIMEOUT_DEFAULT     5000

/* Connection parameters of ota */
#define BLE_CONN_OTA_INTV             7               // 7.5ms
#define BLE_CONN_OTA_LATENCY          0
#define BLE_CONN_OTA_SUPV_TOUT        500             // 5000ms

typedef enum
{
    DFU_MODE_BLE,
    DFU_MODE_UART,
} dfu_mode_t;

typedef enum
{
    DFU_OPCODE_MODE,
    DFU_OPCODE_IMAGE_SIZE,
    DFU_OPCODE_START_DFU,
    DFU_OPCODE_VERIFICATION,
    DFU_OPCODE_REBOOT,
    DFU_OPCODE_RESET,
#if FEAT_CRC_SUPPORT
    DFU_OPCODE_CRC_CHECK,
#endif
    DFU_OPCODE_MAX,
} dfu_opcode_t;

typedef enum
{
    DFU_ERROR_NO_ERROR,
    DFU_ERROR_MEMORY_CAPA_EXCEED,
    DFU_ERROR_STATE_ERROR,
    DFU_ERROR_HASH_ERROR,
    DFU_ERROR_WRONG_LENGTH,
    DFU_ERROR_TIMEOUT,
    DFU_ERROR_PARAM_UPD_FAIL,
#if FEAT_CRC_SUPPORT
    DFU_ERROR_CRC,
#endif
    DFU_ERROR_NO_MAX,
} dfu_error_t;

typedef struct
{
    uint16_t dfu_cmd_len;
    uint32_t timeout;
} dfu_cmd_cb_t;

#endif // _APP_DFU_DEF_H_
