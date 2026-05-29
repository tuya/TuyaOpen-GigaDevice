/*!
    \file    mqtts_client.c
    \brief   the example of mqtt client in station mode

    \version 2024-03-22, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2021, GigaDevice Semiconductor Inc.

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

#include <stdint.h>
#include <stdio.h>
#include "app_cfg.h"
#include "gd32vw55x_platform.h"
#include "wifi_management.h"
#include "wifi_init.h"
#include "dbg_print.h"

#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt5.h"
#include "lwip/apps/mqtt_priv.h"

#include "mqtt_ssl_config.c"

#include "mqtt5_client_config.c"

enum mqtt_mode {
    MODE_TYPE_MQTT = 1U,  // MQTT 3.1.1
    MODE_TYPE_MQTT5 = 2U, // MQTT 5.0
};

//static char client_id[] = {'G', 'i', 'g', 'a', 'D', 'e', 'v', 'i', 'c', 'e', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
extern char mqtt_client_id[];
struct mqtt_connect_client_info_t client_info = {
    //client_id
    mqtt_client_id,
    //client_user
    "user",//NULL,
    //client_password
    "123456",//NULL,
    //keep_alive
    120,
    //topic
    NULL,
    //msg
    NULL,
    //qos
    0,
    //retain
    0
};

uint8_t current_mqtt_mode = MODE_TYPE_MQTT5;

uint8_t tls_auth_mode = TLS_AUTH_MODE_CERT_1WAY;

/*
qos 0: The receiver receives the massage at most once.
qos 1: The receiver receives the massage at least once.
qos 2: The receiver receives the massage just once.
*/
#define TOPIC_QOS_SUB    1
#define TOPIC_QOS_PUB    1
#define TOPIC_RETAIN     1
char* topic_sub  = "topic_sub_test";
char* topic_pub  = "topic_pub_test";
char context[]   = "helloworld";

static mqtt_client_t *mqtt_client = NULL;
int16_t connect_fail_reason = -1;

bool is_mqtt_connected(void)
{
    if (mqtt_client) {
        if (mqtt_client_is_connected(mqtt_client))
            return true;
        else
            return false;
    } else {
        return false;
    }
}

struct mqtt_connect_client_info_t* get_client_param_data_get(void)
{
    return &client_info;
}

void mqtt_receive_pub_msg_print(void *inpub_arg, const uint8_t *data, uint16_t payload_length, uint8_t flags, uint8_t retain)
{
    if (retain > 0 ) {
        app_print("retain: ");
    }

    app_print("payload: ");
    for (uint16_t idx = 0; idx < payload_length; idx++) {
        app_print("%c", *(data + idx));
    }
    app_print("\r\n");

    return;
}

void mqtt_receive_pub_topic_print(void *inpub_arg, const char *data, uint16_t payload_length)
{
    app_print("receiced topic: ");
    for (uint16_t idx = 0; idx < payload_length; idx++) {
        app_print("%c", *(data + idx));
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
        return;;
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

    app_print("%s%s, id is %d.\r\n", prefix, reason, status);
}

void mqtt_fail_reason_display(mqtt_connect_return_res_t fail_reason)
{
    char *prefix = "MQTT mqtt_client: connection refused reason is ";
    char *reason = NULL;

    switch(fail_reason) {
    case MQTT_CONNECTION_REFUSE_PROTOCOL:
        reason = "Bad protocol";
        break;
    case MQTT_CONNECTION_REFUSE_ID_REJECTED:
        reason = "ID rejected";
        break;
    case MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE:
        reason = "Server unavailable";
        break;
    case MQTT_CONNECTION_REFUSE_BAD_USERNAME:
        reason = "Bad username or password";
        break;
    case MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED:
        reason = "Not authorized";
        break;

    default:
        reason = "Unknown reason";
        break;
    }
    app_print("%s%s, id is %d.\r\n", prefix, reason, fail_reason);

    return;
}

static int client_connect(ip_addr_t *server_ip, uint16_t port)
{
    err_t ret = ERR_OK;
    uint32_t connect_time = 0;

    mqtt_client = mqtt_client_new();
    if (mqtt_client == NULL) {
        app_print("Can't get mqtt client.\r\n");
        return -1;
    }

    app_print("MQTT: start link server...\r\n");

    mqtt_ssl_cfg(mqtt_client, tls_auth_mode);
    mqtt_set_inpub_callback(mqtt_client, mqtt_receive_pub_topic_print, mqtt_receive_pub_msg_print, &client_info);

    if (current_mqtt_mode == MODE_TYPE_MQTT5) {
        if (mqtt5_param_cfg(mqtt_client)) {
            app_print("MQTT: Configuration MQTT parameters failed, stop connection.\r\n");
            return -2;
        }

        connect_time = sys_current_time_get();
        ret = mqtt5_client_connect(mqtt_client, server_ip, port, NULL, mqtt_connect_callback, NULL, &client_info,
                            &(mqtt_client->mqtt5_config->connect_property_info),
                            &(mqtt_client->mqtt5_config->will_property_info));
        if (ret != ERR_OK) {
            app_print("MQTT mqtt_client: connect to server failed.\r\n");
            return ret;
        }

        while (mqtt_client_is_connected(mqtt_client) == false) {
            if ((sys_current_time_get() - connect_time) > 5000) {
               app_print("MQTT mqtt_client: connect to server timeout.\r\n");
               return -3;
            }
            if (connect_fail_reason == MQTT_CONNECTION_REFUSE_PROTOCOL) {
                mqtt5_disconnect(mqtt_client);
                mqtt5_param_delete(mqtt_client);
                app_print("MQTT: The server does not support version 5.0, now switch to version 3.1.1.\r\n");
                current_mqtt_mode = MODE_TYPE_MQTT;
                connect_fail_reason = -1;
                break;
            } else if (connect_fail_reason > 0) {
                mqtt5_fail_reason_display((mqtt5_connect_return_res_t)connect_fail_reason);
                return connect_fail_reason;
            }
            sys_yield();
        }
    }

    if (current_mqtt_mode == MODE_TYPE_MQTT) {
        connect_time = sys_current_time_get();
        ret = mqtt_client_connect(mqtt_client, server_ip, port, NULL, mqtt_connect_callback, NULL, &client_info);
        if (ret != ERR_OK) {
            app_print("MQTT mqtt_client: connect to server failed.\r\n");
            return ret;
        }

        while(mqtt_client_is_connected(mqtt_client) == false) {
            if ((sys_current_time_get() - connect_time) > 5000) {
               app_print("MQTT mqtt_client: connect to server timeout.\r\n");
               return -3;
            }
            if (connect_fail_reason > 0) {
                mqtt_fail_reason_display((mqtt_connect_return_res_t)connect_fail_reason);
                return connect_fail_reason;
            }
            sys_yield();
        }
    }

    app_print("MQTT: Successfully connected to server.\r\n");

    return 0;
}

void client_disconnect(void)
{
    connect_fail_reason = -1;
    if (mqtt_client == NULL)
        return;

    if (current_mqtt_mode == MODE_TYPE_MQTT5) {
        mqtt5_disconnect(mqtt_client);
        mqtt5_param_delete(mqtt_client);
    } else {
        mqtt_disconnect(mqtt_client);
    }
    mqtt_ssl_cfg_free(mqtt_client);
    mqtt_client_free(mqtt_client);
    mqtt_client = NULL;
}

void mqtt_pub_cb(void *arg, err_t status)
{
    switch (status) {
        case ERR_OK:
            app_print("topic publish success.\r\n");
            app_print("# \r\n");
            break;
        case ERR_TIMEOUT:;
            app_print("topic publish time out.\r\n");
            app_print("# \r\n");
            break;
        default:
            app_print("topic publish failed.\r\n");
            break;
    }

    return;
}

void mqtt_sub_cb(void *arg, err_t status)
{
    if (status == ERR_OK) {
        app_print("topic subscribe success.\r\n");
    } else if (status == ERR_TIMEOUT) {
        app_print("topic subscribe time out.\r\n");
    }
    app_print("# \r\n");

    return;
}

void mqtt_unsub_cb(void *arg, err_t status)
{
    if (status == ERR_OK) {
        app_print("topic unsubscribe success.\r\n");
    } else if (status == ERR_TIMEOUT) {
        app_print("topic unsubscribe time out.\r\n");
    }
    app_print("# \r\n");

    return;
}

static int client_subscribe(void)
{
    err_t ret = ERR_OK;

    if (current_mqtt_mode == MODE_TYPE_MQTT5) {
        mqtt5_topic_t topic_info;
        topic_info.filter = topic_sub;
        topic_info.qos = TOPIC_QOS_SUB;

        ret = mqtt5_msg_subscribe(mqtt_client, mqtt_sub_cb, &client_info, &topic_info,
                            1, mqtt_client->mqtt5_config->subscribe_property_info);
    } else {
        ret = mqtt_sub_unsub(mqtt_client, topic_sub, TOPIC_QOS_SUB, mqtt_sub_cb,
                            &client_info, 1);
    }
    return ret;
}

static int client_unsubscribe(void)
{
    err_t ret = ERR_OK;

    if (current_mqtt_mode == MODE_TYPE_MQTT5) {
        ret = mqtt5_msg_unsub(mqtt_client, topic_sub, TOPIC_QOS_SUB, mqtt_unsub_cb,
                        &client_info, mqtt_client->mqtt5_config->unsubscribe_property_info);
    } else {
        ret = mqtt_sub_unsub(mqtt_client, topic_sub, TOPIC_QOS_SUB, mqtt_unsub_cb,
                        &client_info, 0);
    }
    return ret;
}

static int client_publish(char *topic, char *context, uint16_t length)
{
    err_t ret = ERR_OK;

    if (current_mqtt_mode == MODE_TYPE_MQTT5) {
        ret = mqtt5_msg_publish(mqtt_client, topic, context, length, TOPIC_QOS_PUB, TOPIC_RETAIN, mqtt_pub_cb, NULL, mqtt_client->mqtt5_config->publish_property_info,
                    mqtt_client->mqtt5_config->server_resp_property_info.response_info);
    } else {
        ret = mqtt_msg_publish(mqtt_client, topic, context, length, TOPIC_QOS_PUB, TOPIC_RETAIN, mqtt_pub_cb, NULL);
    }
    return ret;
}

int mqtt_client_start(ip_addr_t *server_ip, uint16_t port)
{
    return client_connect(server_ip, port);
}

int mqtt_client_publish(void)
{
     return client_publish(topic_pub, context, strlen(context));
}

int mqtt_client_subscribe(void)
{
    return client_subscribe();
}

void mqtt_client_stop(void)
{
    client_unsubscribe();
    client_disconnect();
}
