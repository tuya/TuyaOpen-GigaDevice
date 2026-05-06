/*!
    \file    ble_hpss.h
    \brief   Header file of http proxy service server .

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

#ifndef _BLE_HPSS_H_
#define _BLE_HPSS_H_

#include <stdint.h>
#include "ble_hps_comm.h"

/* Request information of the Http Proxy Service */
typedef struct
{
    uint8_t conn_id;
    const uint8_t *p_uri;
    uint16_t uri_len;

    const uint8_t *p_headers;
    uint16_t headers_len;

    const uint8_t *p_body;
    uint16_t body_len;

    ble_hps_op_code_t ctrl_op_code;
} ble_hps_req_info_t;

/* Response information of the Http Proxy Service */
typedef struct
{
    uint8_t conn_id;
    uint8_t *p_headers;
    uint16_t headers_len;

    uint8_t *p_body;
    uint16_t body_len;

    uint16_t status_code;
} ble_hps_resp_info_t;

/* Application callbacks for Http Proxy Service */
typedef struct ble_hpss_callbacks
{
    bool (*check_certs_cb)(uint8_t conn_id, uint8_t *p_uri, uint16_t uri_len);
    bool (*check_network_cb)(void);
    bool (*http_request_cb)(ble_hps_req_info_t info);
} ble_hpss_callbacks_t;

/*!
    \brief      Set Http Proxy Server response
    \param[in]  response: response information
    \param[out] none
    \retval     none
*/
void ble_hpss_response_set(ble_hps_resp_info_t response);

/*!
    \brief      Init Http Proxy Service server
    \param[in]  callbacks: HPS server callback set
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_hpss_init(ble_hpss_callbacks_t callbacks);

#endif // _BLE_HPSS_H_
