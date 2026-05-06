/*!
    \file    app_http_proxy_client.c
    \brief   Http Proxy Service Client Application Module entry point

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

#include "ble_app_config.h"

#if (BLE_PROFILE_HPS_CLIENT)

#include "wrapper_os.h"
#include "dbg_print.h"
#include "app_http_proxy_client.h"

static void app_hpsc_read_cb(uint8_t conn_id, hps_read_result_t result);
static void app_hpsc_ntf_ind_cb(uint8_t conn_id, hps_status_code_ind_t result);

static ble_hpsc_callbacks_t hpsc_callbacks = {
    .read_cb = app_hpsc_read_cb,
    .write_cb = NULL,
    .ntf_ind_cb = app_hpsc_ntf_ind_cb,
};

/*!
    \brief      Http Proxy Service read callback
    \param[in]  conn_id: connection index
    \param[in]  result: read result
    \param[out] none
    \retval     none
*/
static void app_hpsc_read_cb(uint8_t conn_id, hps_read_result_t result)
{
    uint8_t temp_buf[513] = {0};

    sys_memcpy(temp_buf, result.p_value, result.value_len);

    if (result.type == HTTP_URI) {
        dbg_print(NOTICE, "uri: %s\r\n", temp_buf);
    } else if (result.type == HTTP_HEADERS) {
        dbg_print(NOTICE, "headers: %s\r\n", temp_buf);
    } else if (result.type == HTTP_ENTITY_BODY) {
        dbg_print(NOTICE, "entity body: %s\r\n", temp_buf);
    }
}

/*!
    \brief      Http Proxy Service notify or indicate callback
    \param[in]  conn_id: connection index
    \param[in]  result: status code
    \param[out] none
    \retval     none
*/
static void app_hpsc_ntf_ind_cb(uint8_t conn_id, hps_status_code_ind_t result)
{
    uint16_t status_code = *((uint16_t *)result.status_code);

    if (status_code == 200) {
        dbg_print(NOTICE, "app_hpsc_ntf_ind_cb status Ok!\r\n");
        ble_hpsc_read_char_value(conn_id, HTTP_HEADERS);
        ble_hpsc_read_char_value(conn_id, HTTP_ENTITY_BODY);
    }
}

/*!
    \brief      Http Proxy Service application write URI
    \param[in]  conn_id: connection index
    \param[in]  p_uri: pointer to URI value
    \param[in]  uri_len: URI length
    \param[out] none
    \retval     none
*/
void app_hpsc_write_uri(uint8_t conn_id, uint8_t *p_uri, uint16_t uri_len)
{
    ble_hpsc_write_char_value(conn_id, p_uri, uri_len, HTTP_URI);
}

/*!
    \brief      Http Proxy Service application write headers
    \param[in]  conn_id: connection index
    \param[in]  p_headers: pointer to headers value
    \param[in]  headers_len: headers length
    \param[out] none
    \retval     none
*/
void app_hpsc_write_headers(uint8_t conn_id, uint8_t *p_headers, uint16_t headers_len)
{
    ble_hpsc_write_char_value(conn_id, p_headers, headers_len, HTTP_HEADERS);
}

/*!
    \brief      Http Proxy Service application write entity body
    \param[in]  conn_id: connection index
    \param[in]  p_body: pointer to entity body value
    \param[in]  body_len: entity body length
    \param[out] none
    \retval     none
*/
void app_hpsc_write_entity_body(uint8_t conn_id, uint8_t *p_body, uint16_t body_len)
{
    ble_hpsc_write_char_value(conn_id, p_body, body_len, HTTP_ENTITY_BODY);
}

/*!
    \brief      Http Proxy Service application write control point character
    \param[in]  conn_id: connection index
    \param[in]  value: control point value
    \param[out] none
    \retval     none
*/
void app_hpsc_write_ctrl_point(uint8_t conn_id, uint8_t value)
{
    ble_hpsc_write_ctrl_point(conn_id, value);
}

/*!
    \brief      Init Http Proxy Service application module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_hpsc_init(void)
{
    ble_hpsc_init(hpsc_callbacks);
}

#endif // (BLE_PROFILE_HPS_CLIENT)
