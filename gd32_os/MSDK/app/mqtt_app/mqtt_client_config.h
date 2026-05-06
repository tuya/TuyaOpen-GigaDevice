/*!
    \file    mqtt_client_config.h
    \brief   mqtt client config for GD32VW55x SDK.

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

#ifndef _MQTT_CLIENT_CONFIG_H_
#define _MQTT_CLIENT_CONFIG_H_

#include "app_cfg.h"

#ifdef CONFIG_MQTT
#include "lwip/apps/mqtt.h"
#include "dbg_print.h"
#define MQTT_LINK_TIME_LIMIT 30000 //5000

void mqtt_pub_cb(void *arg, err_t status);
void mqtt_sub_cb(void *arg, err_t status);
void mqtt_unsub_cb(void *arg, err_t status);
void mqtt_receive_msg_print(void *inpub_arg, const uint8_t *data, uint16_t payload_length, uint8_t flags, uint8_t retain);
void mqtt_receive_pub_msg_print(void *inpub_arg, const char *data, uint16_t payload_length);
void mqtt_connect_callback(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
struct mqtt_connect_client_info_t* get_client_param_data_get(void);
void client_user_info_free(void);
void client_will_info_free(void);
int mqtt_client_id_set(char *new_client_id, int16_t len);
int mqtt_client_user_set(char *new_client_user, int16_t len);
int mqtt_client_pass_set(char *new_client_pass, int16_t len);
int mqtt_client_conn_set(u16_t new_keep_alive, u8_t new_clean_session_disabled, char *new_will_topic, char *new_will_msg, u8_t new_will_qos, u8_t new_will_retain);
char *mqtt_client_id_get(void);
void mqtt_client_info_init(void);

#endif // CONFIG_MQTT

#endif // _MQTT_CLIENT_CONFIG_H_
