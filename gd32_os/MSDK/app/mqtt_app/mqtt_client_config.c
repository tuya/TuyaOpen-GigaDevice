/*!
    \file    mqtt_client_config.c
    \brief   MQTT client config for GD32VW55x SDK.

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

#include "app_cfg.h"

#ifdef CONFIG_MQTT
#include "mqtt_client_config.h"
#include "mqtt_cmd.h"
#include "util.h"

#ifndef CONFIG_ATCMD
char client_id[] = {'G', 'i', 'g', 'a', 'D', 'e', 'v', 'i', 'c', 'e', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#else
char *client_id = NULL;
#endif
char *client_user = NULL;
char *client_pass = NULL;
u16_t keep_alive = 120;
char *will_topic = NULL;
char *will_msg = NULL;
u8_t will_qos = 0;
u8_t will_retain = 0;
u8_t clean_session_disabled = 0;
struct mqtt_connect_client_info_t base_client_user_info;
bool mqtt_waiting_for_conn_cb = false;

int mqtt_client_id_set(char *new_client_id, int16_t len)
{
    if (new_client_id == NULL) {
#ifndef CONFIG_ATCMD
        app_print("client id is NULL\r\n");
#else
        app_print("client id is NULL, ERR CODE:0x%08x\r\n", AT_MQTT_CLIENT_ID_IS_NULL);
#endif
        return -1;
    }

#ifndef CONFIG_ATCMD
    if (len >= ARRAY_SIZE(client_id)) {
        app_print("name is too long\r\n");
        return -2;
    }
#else
    if (client_id) {
        sys_mfree(client_id);
        client_id = NULL;
        base_client_user_info.client_id = NULL;
    }
    client_id = sys_malloc(len + 1);
    if (client_id == NULL) {
        app_print("client id malloc failed, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
        return -2;
    }
#endif

    sys_memcpy(client_id, new_client_id, len);
    if (client_id[len] != 0) {
        client_id[len] = 0;
    }
    base_client_user_info.client_id = client_id;

    return 0;
}

int mqtt_client_user_set(char *new_client_user, int16_t len)
{
    if (new_client_user == NULL) {
#ifdef CONFIG_ATCMD
        app_print("client user is NULL, ERR CODE:0x%08x\r\n", AT_MQTT_USERNAME_IS_NULL);
#else
        app_print("client user is NULL\r\n");
#endif
        return -1;
    }

    if (client_user) {
        sys_mfree(client_user);
        client_user = NULL;
        base_client_user_info.client_user = NULL;
    }
    client_user = sys_malloc(len + 1);
    if (client_user == NULL) {
#ifdef CONFIG_ATCMD
        app_print("client user malloc failed, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
#else
        app_print("client user malloc failed\r\n");
#endif
        return -2;
    }

    sys_memcpy(client_user, new_client_user, len);
    if (client_user[len] != 0) {
        client_user[len] = 0;
    }
    base_client_user_info.client_user = client_user;

    return 0;
}

int mqtt_client_pass_set(char *new_client_pass, int16_t len)
{
    if (new_client_pass == NULL) {
#ifdef CONFIG_ATCMD
        app_print("client password is NULL, ERR CODE:0x%08x\r\n", AT_MQTT_PASSWORD_IS_NULL);
#else
        app_print("client password is NULL\r\n");
#endif
        return -1;
    }

    if (client_pass) {
        sys_mfree(client_pass);
        client_pass = NULL;
        base_client_user_info.client_pass = NULL;
    }
    client_pass = sys_malloc(len + 1);
    if (client_pass == NULL) {
#ifdef CONFIG_ATCMD
        app_print("client password malloc failed, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
#else
        app_print("client password malloc failed\r\n");
#endif
        return -2;
    }

    sys_memcpy(client_pass, new_client_pass, len);
    if (client_pass[len] != 0) {
        client_pass[len] = 0;
    }
    base_client_user_info.client_pass = client_pass;

    return 0;
}

int mqtt_client_conn_set(u16_t new_keep_alive, u8_t new_clean_session_disabled, char *new_will_topic, char *new_will_msg, u8_t new_will_qos, u8_t new_will_retain)
{
    u8_t len;

    if (new_will_topic == NULL) {
#ifdef CONFIG_ATCMD
        app_print("will topic is NULL, ERR CODE:0x%08x\r\n", AT_MQTT_LWT_TOPIC_IS_NULL);
#else
        app_print("will topic is NULL\r\n");
#endif
        return -1;
    }
    if (will_topic) {
        sys_mfree(will_topic);
        will_topic = NULL;
        base_client_user_info.will_topic = NULL;
    }
    len = strlen(new_will_topic);
    will_topic = sys_malloc(len + 1);
    if (will_topic == NULL) {
#ifdef CONFIG_ATCMD
        app_print("will topic malloc failed, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
#else
        app_print("will topic malloc failed\r\n");
#endif
        return -2;
    }
    sys_memcpy(will_topic, new_will_topic, len);
    if (will_topic[len] != 0) {
        will_topic[len] = 0;
    }
    base_client_user_info.will_topic = will_topic;

    if (new_will_msg == NULL) {
#ifdef CONFIG_ATCMD
        app_print("will message is NULL, ERR CODE:0x%08x\r\n", AT_MQTT_LWT_MSG_IS_NULL);
#else
        app_print("will message is NULL\r\n");
#endif
        sys_mfree(will_topic);
        will_topic = NULL;
        base_client_user_info.will_topic = NULL;
        return -1;
    }
    if (will_msg) {
        sys_mfree(will_msg);
        will_msg = NULL;
        base_client_user_info.will_msg = NULL;
    }
    len = strlen(new_will_msg);
    will_msg = sys_malloc(len + 1);
    if (will_msg == NULL) {
#ifdef CONFIG_ATCMD
        app_print("will message malloc failed, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
#else
        app_print("will message malloc failed\r\n");
#endif
        sys_mfree(will_topic);
        will_topic = NULL;
        base_client_user_info.will_topic = NULL;
        return -2;
    }
    sys_memcpy(will_msg, new_will_msg, len);
    if (will_msg[len] != 0) {
        will_msg[len] = 0;
    }
    base_client_user_info.will_msg = will_msg;

    keep_alive = new_keep_alive;
    clean_session_disabled = new_clean_session_disabled;
    will_qos = new_will_qos;
    will_retain = new_will_retain;
    base_client_user_info.keep_alive = keep_alive;
    base_client_user_info.will_qos = will_qos;
    base_client_user_info.will_retain = will_retain;
    base_client_user_info.clean_session_disabled = clean_session_disabled;

    return 0;
}

char *mqtt_client_id_get()
{
    return (char *) (client_id);
}

void mqtt_pub_cb(void *arg, err_t status)
{
    switch (status) {
        case ERR_OK:
            app_print("massage publish success\r\n");
            app_print("# \r\n");
            break;
        case ERR_TIMEOUT:;
            app_print("massage publish time out\r\n");
            app_print("# \r\n");
            break;
        default:
            app_print("massage publish failed\r\n");
            break;
    }

    return;
}

void mqtt_sub_cb(void *arg, err_t status)
{
    if (status == ERR_OK) {
        app_print("massage subscribe success\r\n");
    } else if (status == ERR_TIMEOUT) {
        app_print("massage subscribe time out\r\n");
    }
    app_print("# \r\n");

    return;
}

void mqtt_unsub_cb(void *arg, err_t status)
{
    if (status == ERR_OK) {
        app_print("massage unsubscribe success\r\n");
    } else if (status == ERR_TIMEOUT) {
        app_print("massage unsubscribe time out\r\n");
    }
    app_print("# \r\n");

    return;
}

void mqtt_receive_msg_print(void *inpub_arg, const uint8_t *data, uint16_t payload_length, uint8_t flags, uint8_t retain)
{
    if (retain > 0 ) {
        app_print("retain: ");
    }

    app_print("payload: ");
    for (uint16_t idx = 0; idx < payload_length; idx++) {
        app_print("%c", *data);
        data++;
    }
    app_print("\r\n");

    return;
}

void mqtt_receive_pub_msg_print(void *inpub_arg, const char *data, uint16_t payload_length)
{
    app_print("received topic: ");
    for (uint16_t idx = 0; idx < payload_length; idx++) {
        app_print("%c", *data);
        data++;
    }
    app_print("  ");

    return;
}

void mqtt_connect_callback(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    char *prefix = NULL;
    char *reason = NULL;

    if ((status == MQTT_CONNECT_ACCEPTED) ||
        (status == MQTT_CONNECT_REFUSED_PROTOCOL_VERSION)) {
        goto resume_task;
    }

    prefix = "MQTT: client will be closed, reason is ";
    switch (status) {
        case MQTT_CONNECT_DISCONNECTED:
            reason = "remote has closed connection";
            break;
        case MQTT_CONNECT_TIMEOUT:
            reason = "connect attempt to server timed out";
            break;
        default:
            reason = "others";
            break;
    }
    app_print("%s%s, id is %d\r\n", prefix, reason, status);
    if (mqtt_waiting_for_conn_cb) {
        mqtt_waiting_for_conn_cb = false;
    }

resume_task:
    mqtt_task_resume(false);
    return;
}

struct mqtt_connect_client_info_t* get_client_param_data_get(void)
{
    return &base_client_user_info;
}

void mqtt_client_info_init(void)
{
    base_client_user_info.client_id = client_id;
    base_client_user_info.client_user = client_user;
    base_client_user_info.client_pass = client_pass;
    base_client_user_info.keep_alive = keep_alive;
    base_client_user_info.will_topic = will_topic;
    base_client_user_info.will_msg = will_msg;
    base_client_user_info.will_qos = will_qos;
    base_client_user_info.will_retain = will_retain;
    base_client_user_info.clean_session_disabled = clean_session_disabled;
}

void client_user_info_free(void)
{
#ifdef CONFIG_ATCMD
    if (base_client_user_info.client_id != NULL) {
        sys_mfree(base_client_user_info.client_id);
    }
    base_client_user_info.client_id = NULL;
    client_id = NULL;
#endif

    if (base_client_user_info.client_user != NULL) {
        sys_mfree(base_client_user_info.client_user);
    }
    base_client_user_info.client_user = NULL;
    client_user = NULL;

    if (base_client_user_info.client_pass != NULL) {
        sys_mfree(base_client_user_info.client_pass);
    }
    base_client_user_info.client_pass = NULL;
    client_pass = NULL;

    return;
}

void client_will_info_free(void)
{
    if (base_client_user_info.will_topic != NULL) {
        sys_mfree(base_client_user_info.will_topic);
    }
    base_client_user_info.will_topic = NULL;
    will_topic = NULL;

    if (base_client_user_info.will_msg != NULL) {
        sys_mfree(base_client_user_info.will_msg);
    }
    base_client_user_info.will_msg = NULL;
    will_msg = NULL;

    return;
}

#endif //CONFIG_MQTT
