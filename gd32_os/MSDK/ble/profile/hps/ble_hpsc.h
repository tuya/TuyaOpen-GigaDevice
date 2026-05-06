/*!
    \file    ble_hpsc.h
    \brief   Header file of http proxy service client .

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

#ifndef _BLE_HPSC_H_
#define _BLE_HPSC_H_

#include <stdint.h>
#include "ble_error.h"
#include "ble_hps_comm.h"

/* Character type of the Http Proxy Service */
typedef enum
{
    HTTP_RSVF_TYPE = 0,
    HTTP_URI = 1,
    HTTP_HEADERS,
    HTTP_ENTITY_BODY,
    HTTP_SECURITY,
} ble_hps_char_type;

/* Reading parameters of Http Proxy Service */
typedef struct
{
    ble_hps_char_type type;
    uint16_t value_len;
    uint8_t *p_value;
} hps_read_result_t;

/* Writing parameters of Http Proxy Service */
typedef struct
{
    ble_hps_char_type type;
    ble_status_t status;
} hps_write_result_t;

/* Status code of Http Proxy Service */
typedef struct
{
    uint8_t status_code[HPS_STATUS_CODE_LEN];
} hps_status_code_ind_t;

/* Http Proxy Service callback set */
typedef struct ble_hpsc_callbacks
{
    void (*read_cb)(uint8_t conn_id, hps_read_result_t result);
    void (*write_cb)(uint8_t conn_id, hps_write_result_t result);
    void (*ntf_ind_cb)(uint8_t conn_id, hps_status_code_ind_t result);
} ble_hpsc_callbacks_t;

/*!
    \brief      Write character value
    \param[in]  conn_id: connection index
    \param[in]  p_value: pointer to the value
    \param[in]  value_len: value length
    \param[in]  type: character type
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_hpsc_write_char_value(uint8_t conn_id, uint8_t *p_value, uint16_t value_len,
                                       ble_hps_char_type type);

/*!
    \brief      Read Http Proxy Service character
    \param[in]  conn_id: onnection index
    \param[in]  type: character type
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_hpsc_read_char_value(uint8_t conn_id, ble_hps_char_type type);

/*!
    \brief      Write Http Proxy Service control point character
    \param[in]  conn_id: connection index
    \param[in]  value: HPS control point operation code
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_hpsc_write_ctrl_point(uint8_t conn_id, ble_hps_op_code_t value);

/*!
    \brief      Init Http Proxy Service client
    \param[in]  callbacks: HPS client callback function set
    \param[out] none
    \retval     none
*/
void ble_hpsc_init(ble_hpsc_callbacks_t callbacks);

#endif // _BLE_HPSC_H_
