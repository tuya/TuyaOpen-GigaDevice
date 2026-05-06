/*!
    \file    ble_hps_comm.h
    \brief   Header file of http proxy service.

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

#ifndef _BLE_HPS_COMM_H_
#define _BLE_HPS_COMM_H_

#include <stdint.h>
#include "ble_types.h"
#include "ble_error.h"
#include "ble_gatt.h"

/* Max length for Characteristic values */
#define BLE_HPS_VAL_MAX_LEN                     (512)
#define HPS_STATUS_CODE_LEN                     3

#define HPS_INV_REQ_ERR             0x81
#define HPS_NETWORK_UNAVL_ERR       0x82

#define HPS_HEADERS_RECVD_BIT           0x01
#define HPS_HEADERS_TRUNC_BIT           0x02
#define HPS_BODY_RECVD_BIT              0x04
#define HPS_BODY_TRUNC_BIT              0x08

#define HTTP_CERT_URI                   0x01
#define HTTP_UNCERT_URI                 0x00

/* HPS control point op code list */
typedef enum
{
    HTTP_RSVF = 0,
    HTTP_GET_REQUEST,
    HTTP_HEAD_REQUEST,
    HTTP_POST_REQUEST,
    HTTP_PUT_REQUEST,
    HTTP_DELETE_REQUEST,
    HTTPS_GET_REQUEST,
    HTTPS_HEAD_REQUEST,
    HTTPS_POST_REQUEST,
    HTTPS_PUT_REQUEST,
    HTTPS_DELETE_REQUEST,
    HTTP_REQUEST_CANCEL
} ble_hps_op_code_t;

#endif // _BLE_HPS_COMM_H_
