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
#endif

enum tls_auth_mode {
    TLS_AUTH_MODE_NONE,
    TLS_AUTH_MODE_KEY_SHARE,
    TLS_AUTH_MODE_CERT_1WAY,
    TLS_AUTH_MODE_CERT_2WAY,
    TLS_AUTH_MODE_PSK,
    TLS_AUTH_MODE_CERT_CLIENT_ONLY,
};

mqtt_client_context_t* mqtt_client_context_init(void)
{
    mqtt_client_context_t* mqtt_ctx = sys_calloc(1, sizeof(mqtt_client_context_t));
    if (mqtt_ctx == NULL) {
#ifdef CONFIG_ATCMD
        app_print("mqtt_client_context malloc failed, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
#else
        app_print("mqtt_client_context malloc failed\r\n");
#endif
        return NULL;
    }

    mqtt_ctx->client_user_info = sys_calloc(1, sizeof(struct mqtt_connect_client_info_t));
    if (mqtt_ctx->client_user_info == NULL) {
#ifdef CONFIG_ATCMD
        app_print("mqtt client user info malloc failed, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
#else
        app_print("mqtt client user info malloc failed\r\n");
#endif
        sys_mfree(mqtt_ctx);
        mqtt_ctx = NULL;
        return NULL;
    }

    mqtt_ctx->port = MQTT_DEFAULT_PORT;
    mqtt_ctx->tls_encry_mode = TLS_AUTH_MODE_NONE;
    mqtt_ctx->mqtt_cmd_mode = MODE_TYPE_MQTT5;
    mqtt_ctx->auto_reconnect_limit = AUTO_RECONNECT_LIMIT;
    mqtt_ctx->auto_reconnect = false;
    mqtt_ctx->auto_reconnect_num = 0;
    mqtt_ctx->auto_reconnect_interval = 20000; // ms, 20s
    mqtt_ctx->mqtt_task_suspended = false;
    mqtt_ctx->mqtt_task_handle = NULL;
    memset(&(mqtt_ctx->server_ip_addr), 0, sizeof(ip_addr_t));
    mqtt_ctx->waiting_for_conn_cb = false;
#ifndef CONFIG_ATCMD
    mqtt_ctx->client_user_info->client_id = client_id;
#else
    mqtt_ctx->client_user_info->client_id = NULL;
#endif
    mqtt_ctx->client_user_info->keep_alive = 120;
    mqtt_ctx->client_user_info->clean_session_disabled = 0;
    mqtt_ctx->client_user_info->will_qos = 0;
    mqtt_ctx->client_user_info->will_retain = 0;

    return mqtt_ctx;
}

int mqtt_client_id_set(mqtt_client_context_t *mqtt_ctx, char *new_client_id, int16_t len)
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
    if (mqtt_ctx->client_user_info->client_id) {
        sys_mfree(mqtt_ctx->client_user_info->client_id);
        mqtt_ctx->client_user_info->client_id = NULL;
    }
    mqtt_ctx->client_user_info->client_id = sys_malloc(len + 1);
    if (mqtt_ctx->client_user_info->client_id == NULL) {
        app_print("client id malloc failed, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
        return -2;
    }
#endif

    sys_memcpy(mqtt_ctx->client_user_info->client_id, new_client_id, len);
    if (mqtt_ctx->client_user_info->client_id[len] != 0) {
        mqtt_ctx->client_user_info->client_id[len] = 0;
    }

    return 0;
}

int mqtt_client_user_set(mqtt_client_context_t *mqtt_ctx, char *new_client_user, int16_t len)
{
    if (new_client_user == NULL) {
#ifdef CONFIG_ATCMD
        app_print("client user is NULL, ERR CODE:0x%08x\r\n", AT_MQTT_USERNAME_IS_NULL);
#else
        app_print("client user is NULL\r\n");
#endif
        return -1;
    }

    if (mqtt_ctx->client_user_info->client_user) {
        sys_mfree(mqtt_ctx->client_user_info->client_user);
        mqtt_ctx->client_user_info->client_user = NULL;
    }
    mqtt_ctx->client_user_info->client_user = sys_zalloc(len + 1);
    if (mqtt_ctx->client_user_info->client_user == NULL) {
#ifdef CONFIG_ATCMD
        app_print("client user malloc failed, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
#else
        app_print("client user malloc failed\r\n");
#endif
        return -2;
    }

    sys_memcpy(mqtt_ctx->client_user_info->client_user, new_client_user, len);
    if (mqtt_ctx->client_user_info->client_user[len] != 0) {
        mqtt_ctx->client_user_info->client_user[len] = 0;
    }

    return 0;
}

int mqtt_client_pass_set(mqtt_client_context_t *mqtt_ctx, char *new_client_pass, int16_t len)
{
    if (new_client_pass == NULL) {
#ifdef CONFIG_ATCMD
        app_print("client password is NULL, ERR CODE:0x%08x\r\n", AT_MQTT_PASSWORD_IS_NULL);
#else
        app_print("client password is NULL\r\n");
#endif
        return -1;
    }

    if (mqtt_ctx->client_user_info->client_pass) {
        sys_mfree(mqtt_ctx->client_user_info->client_pass);
        mqtt_ctx->client_user_info->client_pass = NULL;
    }
    mqtt_ctx->client_user_info->client_pass = sys_zalloc(len + 1);
    if (mqtt_ctx->client_user_info->client_pass == NULL) {
#ifdef CONFIG_ATCMD
        app_print("client password malloc failed, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
#else
        app_print("client password malloc failed\r\n");
#endif
        return -2;
    }

    sys_memcpy(mqtt_ctx->client_user_info->client_pass, new_client_pass, len);
    if (mqtt_ctx->client_user_info->client_pass[len] != 0) {
        mqtt_ctx->client_user_info->client_pass[len] = 0;
    }

    return 0;
}

int mqtt_client_conn_set(mqtt_client_context_t *mqtt_ctx, u16_t new_keep_alive, u8_t new_clean_session_disabled, char *new_will_topic, char *new_will_msg, u8_t new_will_qos, u8_t new_will_retain)
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
    if (mqtt_ctx->client_user_info->will_topic) {
        sys_mfree(mqtt_ctx->client_user_info->will_topic);
        mqtt_ctx->client_user_info->will_topic = NULL;
    }
    len = strlen(new_will_topic);
    mqtt_ctx->client_user_info->will_topic = sys_malloc(len + 1);
    if (mqtt_ctx->client_user_info->will_topic == NULL) {
#ifdef CONFIG_ATCMD
        app_print("will topic malloc failed, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
#else
        app_print("will topic malloc failed\r\n");
#endif
        return -2;
    }
    sys_memcpy(mqtt_ctx->client_user_info->will_topic, new_will_topic, len);
    if (mqtt_ctx->client_user_info->will_topic[len] != 0) {
        mqtt_ctx->client_user_info->will_topic[len] = 0;
    }

    if (new_will_msg == NULL) {
#ifdef CONFIG_ATCMD
        app_print("will message is NULL, ERR CODE:0x%08x\r\n", AT_MQTT_LWT_MSG_IS_NULL);
#else
        app_print("will message is NULL\r\n");
#endif
        sys_mfree(mqtt_ctx->client_user_info->will_topic);
        mqtt_ctx->client_user_info->will_topic = NULL;
        return -1;
    }
    if (mqtt_ctx->client_user_info->will_msg) {
        sys_mfree(mqtt_ctx->client_user_info->will_msg);
        mqtt_ctx->client_user_info->will_msg = NULL;
    }
    len = strlen(new_will_msg);
    mqtt_ctx->client_user_info->will_msg = sys_malloc(len + 1);
    if (mqtt_ctx->client_user_info->will_msg == NULL) {
#ifdef CONFIG_ATCMD
        app_print("will message malloc failed, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
#else
        app_print("will message malloc failed\r\n");
#endif
        sys_mfree(mqtt_ctx->client_user_info->will_topic);
        mqtt_ctx->client_user_info->will_topic = NULL;
        return -2;
    }
    sys_memcpy(mqtt_ctx->client_user_info->will_msg, new_will_msg, len);
    if (mqtt_ctx->client_user_info->will_msg[len] != 0) {
        mqtt_ctx->client_user_info->will_msg[len] = 0;
    }

    mqtt_ctx->client_user_info->keep_alive = new_keep_alive;
    mqtt_ctx->client_user_info->clean_session_disabled = new_clean_session_disabled;
    mqtt_ctx->client_user_info->will_qos = new_will_qos;
    mqtt_ctx->client_user_info->will_retain = new_will_retain;

    return 0;
}

int mqtt_host_set(mqtt_client_context_t *mqtt_ctx, char *host, int16_t len)
{
    if (host == NULL) {
#ifdef CONFIG_ATCMD
        app_print("host is NULL, ERR CODE:0x%08x\r\n", AT_MQTT_HOST_IS_NULL);
#else
        app_print("host is NULL\r\n");
#endif
        return -1;
    }

    if (mqtt_ctx->mqtt_host) {
        sys_mfree(mqtt_ctx->mqtt_host);
        mqtt_ctx->mqtt_host = NULL;
    }
    mqtt_ctx->mqtt_host = sys_malloc(len + 1);
    if (mqtt_ctx->mqtt_host == NULL) {
#ifdef CONFIG_ATCMD
        app_print("host malloc failed, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
#else
        app_print("host malloc failed\r\n");
#endif
        return -2;
    }

    sys_memcpy(mqtt_ctx->mqtt_host, host, len);
    if (mqtt_ctx->mqtt_host[len] != 0) {
        mqtt_ctx->mqtt_host[len] = 0;
    }

    return 0;
}

int mqtt_ssl_cfg(mqtt_client_context_t *mqtt_ctx)
{
    int ret = 0;

    if (mqtt_ctx->tls_encry_mode == TLS_AUTH_MODE_CERT_2WAY) {
        ret = mqtt_ssl_cfg_with_cert(mqtt_ctx->mqtt_client, (u8_t *)(mqtt_ctx->ca_cert), mqtt_ctx->ca_cert_len, (u8_t *)(mqtt_ctx->client_key), mqtt_ctx->client_key_len, (u8_t *)(mqtt_ctx->client_cert), mqtt_ctx->client_cert_len);
    } else if (mqtt_ctx->tls_encry_mode == TLS_AUTH_MODE_CERT_1WAY) {
        ret = mqtt_ssl_cfg_with_cert(mqtt_ctx->mqtt_client, (u8_t *)(mqtt_ctx->ca_cert), mqtt_ctx->ca_cert_len, NULL, 0, NULL, 0);
    } else if (mqtt_ctx->tls_encry_mode == TLS_AUTH_MODE_KEY_SHARE) {
        ret = mqtt_ssl_cfg_without_cert(mqtt_ctx->mqtt_client, NULL, 0, NULL, 0);
    } else if (mqtt_ctx->tls_encry_mode == TLS_AUTH_MODE_PSK) {
        ret = mqtt_ssl_cfg_without_cert(mqtt_ctx->mqtt_client, mqtt_ctx->psk, mqtt_ctx->psk_len, (const u8_t *)mqtt_ctx->psk_identity, mqtt_ctx->psk_identity_len);
    } else if (mqtt_ctx->tls_encry_mode == TLS_AUTH_MODE_CERT_CLIENT_ONLY) {
        ret = mqtt_ssl_cfg_with_cert(mqtt_ctx->mqtt_client, NULL, 0, (uint8_t *)(mqtt_ctx->client_key), mqtt_ctx->client_key_len, (uint8_t *)(mqtt_ctx->client_cert), mqtt_ctx->client_cert_len);
    }

    return ret;
}

char *mqtt_client_id_get(mqtt_client_context_t *mqtt_ctx)
{
    return (char *) (mqtt_ctx->client_user_info->client_id);
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
    mqtt_client_context_t *mqtt_ctx = (mqtt_client_context_t *)arg;
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
    if (mqtt_ctx->waiting_for_conn_cb) {
        mqtt_ctx->waiting_for_conn_cb = false;
    }

resume_task:
    mqtt_task_resume(mqtt_ctx, false);
    return;
}

struct mqtt_connect_client_info_t* get_client_param_data_get(void)
{
    extern mqtt_client_context_t *g_mqtt_ctx;
    if (g_mqtt_ctx == NULL) {
        return NULL;
    }
    return g_mqtt_ctx->client_user_info;
}

void client_user_info_free(mqtt_client_context_t *mqtt_ctx)
{
    if (mqtt_ctx == NULL || mqtt_ctx->client_user_info == NULL) {
        return;
    }

#ifdef CONFIG_ATCMD
    if (mqtt_ctx->client_user_info->client_id != NULL) {
        sys_mfree(mqtt_ctx->client_user_info->client_id);
    }
    mqtt_ctx->client_user_info->client_id = NULL;
#endif

    if (mqtt_ctx->client_user_info->client_user != NULL) {
        sys_mfree(mqtt_ctx->client_user_info->client_user);
    }
    mqtt_ctx->client_user_info->client_user = NULL;

    if (mqtt_ctx->client_user_info->client_pass != NULL) {
        sys_mfree(mqtt_ctx->client_user_info->client_pass);
    }
    mqtt_ctx->client_user_info->client_pass = NULL;

    return;
}

void client_will_info_free(mqtt_client_context_t *mqtt_ctx)
{
    if (mqtt_ctx == NULL || mqtt_ctx->client_user_info == NULL) {
        return;
    }

    if (mqtt_ctx->client_user_info->will_topic != NULL) {
        sys_mfree(mqtt_ctx->client_user_info->will_topic);
    }
    mqtt_ctx->client_user_info->will_topic = NULL;

    if (mqtt_ctx->client_user_info->will_msg != NULL) {
        sys_mfree(mqtt_ctx->client_user_info->will_msg);
    }
    mqtt_ctx->client_user_info->will_msg = NULL;

    return;
}

void mqtt_host_free(mqtt_client_context_t *mqtt_ctx)
{
    if (mqtt_ctx->mqtt_host != NULL) {
        sys_mfree(mqtt_ctx->mqtt_host);
        mqtt_ctx->mqtt_host = NULL;
    }
}

void mqtt_resource_free(mqtt_client_context_t *mqtt_ctx)
{
    if (mqtt_ctx == NULL || mqtt_ctx->mqtt_client == NULL) {
        return;
    }
    mqtt_ssl_cfg_free(mqtt_ctx->mqtt_client);
    mqtt_host_free(mqtt_ctx);
    at_topic_sub_list_free(mqtt_ctx);
    extern void mqtt5_param_delete(mqtt_client_t *mqtt_client);
    mqtt5_param_delete(mqtt_ctx->mqtt_client);
    mqtt_client_free(mqtt_ctx->mqtt_client);
    mqtt_ctx->mqtt_client = NULL;

    return;
}

void mqtt_info_free(mqtt_client_context_t *mqtt_ctx)
{
    if (mqtt_ctx == NULL || mqtt_ctx->client_user_info == NULL) {
        return;
    }
    client_user_info_free(mqtt_ctx);
    client_will_info_free(mqtt_ctx);
    sys_mfree(mqtt_ctx->client_user_info);
    mqtt_ctx->client_user_info = NULL;

    return;
}

void mqtt_context_free(mqtt_client_context_t **mqtt_ctx)
{
    if (mqtt_ctx == NULL || *mqtt_ctx == NULL) {
        return;
    }
    sys_mfree(*mqtt_ctx);
    *mqtt_ctx = NULL;

    return;
}

#endif //CONFIG_MQTT
