/*!
    \file    mqtt_cmd.c
    \brief   MQTT command shell for GD32VW55x SDK.

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
#include <stdbool.h>
#include "mqtt_cmd.h"
#include "cmd_shell.h"
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"
#include "lwip/apps/mqtt5.h"
#include "wrapper_os.h"
#include "lwip/netdb.h"
#include "lwip/tcpip.h"
#include "co_utils.h"

#include "mqtt_ssl_config.c"

#include "mqtt_client_config.c"

#include "mqtt5_client_config.c"

#define AUTO_RECONNECT_LIMIT    5
bool auto_reconnect = false;
uint8_t auto_reconnect_num = 0;
uint32_t auto_reconnect_interval = 20000; // ms, 20s

static bool mqtt_task_suspended;
static void *mqtt_task_handle = NULL;
static mqtt_client_t *mqtt_client = NULL;
struct mqtt_connect_client_info_t *client_user_info = NULL;

ip_addr_t server_ip_addr = IPADDR4_INIT_BYTES(0, 0, 0, 0);
uint16_t port;
char *mqtt_host = NULL;
char *mqtt_ws_path = NULL;
uint8_t tls_encry_mode = TLS_AUTH_MODE_NONE;
uint16_t mqtt_scheme;

int16_t connect_fail_reason = -1;

static cmd_msg_pub_t msg_pub_list;

static cmd_msg_sub_t msg_sub_list;
cmd_msg_sub_t at_topic_sub_list;

enum mqtt_mode mqtt_cmd_mode = 0;
void mqtt_mode_type_set(enum mqtt_mode cmd_mode)
{
    mqtt_cmd_mode = cmd_mode;
}

enum mqtt_mode mqtt_mode_type_get(void)
{
    return mqtt_cmd_mode;
}

mqtt_client_t* mqtt_client_get(void)
{
    return mqtt_client;
}

struct mqtt_connect_client_info_t *client_user_info_get(void)
{
    return client_user_info;
}

void mqtt_scheme_set(uint16_t scheme)
{
    mqtt_scheme = scheme;
}

uint16_t mqtt_scheme_get(void)
{
    return mqtt_scheme;
}

uint16_t mqtt_port_get(void)
{
    return port;
}

uint8_t mqtt_reconnect_get(void)
{
    return (uint8_t)auto_reconnect;
}

int mqtt_ws_path_set(char *path)
{
    uint8_t len = strlen(path);

    if (path == NULL) {
#ifdef CONFIG_ATCMD
        app_print("path is NULL, ERR CODE:0x%08x\r\n", AT_MQTT_PATH_IS_NULL);
#else
        app_print("path is NULL\r\n");
#endif
        return -1;
    }

    if (mqtt_ws_path) {
        sys_mfree(mqtt_ws_path);
        mqtt_ws_path = NULL;
    }
    mqtt_ws_path = sys_malloc(len + 1);
    if (mqtt_ws_path == NULL) {
#ifdef CONFIG_ATCMD
        app_print("mqtt_ws_path malloc failed, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
#else
        app_print("mqtt_ws_path malloc failed\r\n");
#endif
        return -2;
    }

    sys_memcpy(mqtt_ws_path, path, len);
    if (mqtt_ws_path[len] != 0) {
        mqtt_ws_path[len] = 0;
    }

    return 0;
}

void mqtt_ws_path_free(void)
{
    if (mqtt_ws_path) {
        sys_mfree(mqtt_ws_path);
        mqtt_ws_path = NULL;
    }
}

char *mqtt_ws_path_get(void)
{
    return mqtt_ws_path;
}

char *mqtt_host_get(void)
{
    return mqtt_host;
}

void mqtt_host_free(void)
{
    if (mqtt_host) {
        sys_mfree(mqtt_host);
        mqtt_host = NULL;
    }
}

#ifdef CONFIG_ATCMD
extern void at_mqtt_disconn_print(void);
extern void at_mqtt_pub_result_cb(void *arg, err_t status);
extern void at_mqtt_sub_result_cb(void *arg, err_t status);
extern void at_mqtt_unsub_result_cb(void *arg, err_t status);
extern void at_mqtt_pub_err_print(publish_msg_t *pub_msg, err_t status);
extern void at_mqtt_sub_or_unsub_err_print(sub_msg_t *sub_msg, err_t status);
#endif

static publish_msg_t* publish_msg_mem_malloc(uint16_t input_topic_len, uint16_t input_msg_len)
{
    publish_msg_t *pub_msg = sys_calloc(1, sizeof(publish_msg_t));
    if (pub_msg == NULL) {
        return NULL;
    }

    pub_msg->topic = (char*)sys_malloc(input_topic_len);
    if (pub_msg->topic == NULL) {
        sys_mfree(pub_msg);
        return NULL;
    }

    pub_msg->msg = (char*)sys_malloc(input_msg_len);
    if (pub_msg->msg == NULL) {
        sys_mfree(pub_msg->topic);
        sys_mfree(pub_msg);
        return NULL;
    }
    return pub_msg;
}

static void publish_msg_mem_free(publish_msg_t *pub_msg)
{
    if (pub_msg == NULL) {
        return;
    }
    sys_mfree(pub_msg->topic);
    sys_mfree(pub_msg->msg);
    sys_mfree(pub_msg);
    return;
}

static sub_msg_t* sub_msg_mem_malloc(uint16_t input_topic_len)
{
    sub_msg_t *sub_msg = sys_calloc(1, sizeof(sub_msg_t));
    if (sub_msg == NULL) {
        return NULL;
    }

    sub_msg->topic = (char*)sys_malloc(input_topic_len);
    if (sub_msg->topic == NULL) {
        sys_mfree(sub_msg);
        return NULL;
    }

    return sub_msg;
}

static void sub_msg_mem_free(sub_msg_t *msg_p)
{
    if (msg_p == NULL) {
        return;
    }
    sys_mfree(msg_p->topic);
    sys_mfree(msg_p);
    return;
}

static err_t client_user_info_malloc(uint16_t user_name_len, uint16_t user_password_len)
{
    client_user_info->client_user = (char*)sys_zalloc(user_name_len);
    if (client_user_info->client_user == NULL) {
        return ERR_MEM;
    }

    client_user_info->client_pass = (char*)sys_zalloc(user_password_len);
    if (client_user_info->client_pass == NULL) {
        sys_mfree(client_user_info->client_user);
        return ERR_MEM;
    }
    return ERR_OK;
}

void mqtt_task_suspend(void)
{
    mqtt_task_suspended = true;
    sys_task_wait_notification(-1);
    return;
}

void mqtt_task_resume(bool isr)
{
    if (!mqtt_task_suspended) {
        return;
    }

    mqtt_task_suspended = false;
    sys_task_notify(mqtt_task_handle, isr);
    return;
}

static void mqtt_resource_free(void)
{
    mqtt_ssl_cfg_free(mqtt_client);
    mqtt_host_free();
    at_topic_sub_list_free();
    mqtt5_param_delete(mqtt_client);
    mqtt_client_free(mqtt_client);
    mqtt_client = NULL;

    return;
}

static void mqtt_info_free(void)
{
    client_user_info_free();
    client_will_info_free();
    return;
}

void mqtt_publish_msg_handle(void)
{
    publish_msg_t *pub_msg = NULL;

    while (!co_list_is_empty(&(msg_pub_list.cmd_msg_pub_list))) {
        sys_sched_lock();
        pub_msg = (publish_msg_t *)co_list_pop_front(&(msg_pub_list.cmd_msg_pub_list));
        sys_sched_unlock();

        LOCK_TCPIP_CORE();
        if (mqtt_mode_type_get() == MODE_TYPE_MQTT5) {
#ifndef CONFIG_ATCMD
            mqtt5_msg_publish(mqtt_client, pub_msg->topic, pub_msg->msg, pub_msg->msg_len, pub_msg->qos, pub_msg->retain,
                            mqtt_pub_cb, (void *)pub_msg, mqtt_client->mqtt5_config->publish_property_info,
                            mqtt_client->mqtt5_config->server_resp_property_info.response_info);
#else
            int res = mqtt5_msg_publish(mqtt_client, pub_msg->topic, pub_msg->msg, pub_msg->msg_len, pub_msg->qos, pub_msg->retain,
                            at_mqtt_pub_result_cb, (void *)pub_msg, mqtt_client->mqtt5_config->publish_property_info,
                            mqtt_client->mqtt5_config->server_resp_property_info.response_info);
            if (res != ERR_OK) {
                at_mqtt_pub_err_print(pub_msg, res);
            }
#endif
        } else {
#ifndef CONFIG_ATCMD
            mqtt_msg_publish(mqtt_client, pub_msg->topic, pub_msg->msg, pub_msg->msg_len,
                            pub_msg->qos, pub_msg->retain, mqtt_pub_cb, (void *)pub_msg);
#else
            int res = mqtt_msg_publish(mqtt_client, pub_msg->topic, pub_msg->msg, pub_msg->msg_len,
                            pub_msg->qos, pub_msg->retain, at_mqtt_pub_result_cb, (void *)pub_msg);
            if (res != ERR_OK) {
                at_mqtt_pub_err_print(pub_msg, res);
            }
#endif
        }
        UNLOCK_TCPIP_CORE();
        publish_msg_mem_free(pub_msg);
    }
    return;
}

bool at_topic_exist(const char *topic)
{
    sys_sched_lock();
    struct co_list_hdr *curr = at_topic_sub_list.cmd_msg_sub_list.first;
    while (curr) {
        sub_msg_t *sub_topic = CONTAINER_OF(curr, sub_msg_t, hdr);
        if (strcmp(sub_topic->topic, topic) == 0) {
            sys_sched_unlock();
            return true;
        }
        curr = curr->next;
    }
    sys_sched_unlock();
    return false;
}

void at_topic_sub_list_free(void)
{
    sub_msg_t *sub_msg = NULL;
    while (!co_list_is_empty(&(at_topic_sub_list.cmd_msg_sub_list))) {
        sys_sched_lock();
        sub_msg = (sub_msg_t *)co_list_pop_front(&(at_topic_sub_list.cmd_msg_sub_list));
        sys_sched_unlock();
        sub_msg_mem_free(sub_msg);
    }
    return;
}

void mqtt_subscribe_or_unsubscribe_msg_handle(void)
{
    sub_msg_t *sub_msg = NULL, *sub_topic = NULL;
    mqtt5_topic_t sub_list;

    while (!co_list_is_empty(&(msg_sub_list.cmd_msg_sub_list))) {
        sys_sched_lock();
        sub_msg = (sub_msg_t *)co_list_pop_front(&(msg_sub_list.cmd_msg_sub_list));
        sys_sched_unlock();
#ifdef CONFIG_ATCMD
        sub_msg_t *persistent_msg = sub_msg_mem_malloc(strlen((const char *)sub_msg->topic) + 1);
        if (persistent_msg == NULL) {
            at_mqtt_sub_or_unsub_err_print(sub_msg, ERR_MEM);
            sub_msg_mem_free(sub_msg);
            return;
        }
        memcpy(persistent_msg->topic, sub_msg->topic, strlen(sub_msg->topic) + 1);
        persistent_msg->qos = sub_msg->qos;
#endif
        LOCK_TCPIP_CORE();
        if (mqtt_mode_type_get() == MODE_TYPE_MQTT5) {
            if (sub_msg->sub_or_unsub) {
                sub_list.filter = sub_msg->topic;
                sub_list.qos = (int)sub_msg->qos;
#ifndef CONFIG_ATCMD
                mqtt5_msg_subscribe(mqtt_client, mqtt_sub_cb, client_user_info, &sub_list,
                            1, mqtt_client->mqtt5_config->subscribe_property_info);
#else
                int res = mqtt5_msg_subscribe(mqtt_client, at_mqtt_sub_result_cb, persistent_msg, &sub_list,
                            1, mqtt_client->mqtt5_config->subscribe_property_info);
                if (res != ERR_OK) {
                    sys_mfree(persistent_msg->topic);
                    sys_mfree(persistent_msg);
                    persistent_msg = NULL;
                    at_mqtt_sub_or_unsub_err_print(sub_msg, res);
                }
#endif
            } else {
#ifndef CONFIG_ATCMD
                mqtt5_msg_unsub(mqtt_client, sub_msg->topic, sub_msg->qos, mqtt_unsub_cb,
                            client_user_info, mqtt_client->mqtt5_config->unsubscribe_property_info);
#else
                int res = mqtt5_msg_unsub(mqtt_client, sub_msg->topic, sub_msg->qos, at_mqtt_unsub_result_cb,
                            persistent_msg, mqtt_client->mqtt5_config->unsubscribe_property_info);
                if (res != ERR_OK) {
                    sys_mfree(persistent_msg->topic);
                    sys_mfree(persistent_msg);
                    persistent_msg = NULL;
                    at_mqtt_sub_or_unsub_err_print(sub_msg, res);
                }
#endif
            }
        } else {
#ifndef CONFIG_ATCMD
            void *mag_send_call_back = (sub_msg->sub_or_unsub) == true ? mqtt_sub_cb : mqtt_unsub_cb;
            mqtt_sub_unsub(mqtt_client, sub_msg->topic, sub_msg->qos, mag_send_call_back,
                            client_user_info, sub_msg->sub_or_unsub);
#else
            void *mag_send_call_back = (sub_msg->sub_or_unsub) == true ? at_mqtt_sub_result_cb : at_mqtt_unsub_result_cb;
            int res = mqtt_sub_unsub(mqtt_client, sub_msg->topic, sub_msg->qos, mag_send_call_back,
                            persistent_msg, sub_msg->sub_or_unsub);
            if (res != ERR_OK) {
                sys_mfree(persistent_msg->topic);
                sys_mfree(persistent_msg);
                persistent_msg = NULL;
                at_mqtt_sub_or_unsub_err_print(sub_msg, res);
            }
#endif
        }
        UNLOCK_TCPIP_CORE();
        sub_msg_mem_free(sub_msg);
    }
    return;
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
    app_print("%s%s, id is %d\r\n", prefix, reason, fail_reason);

    return;
}

void mqtt5_fail_reason_display(mqtt5_connect_return_res_t fail_reason)
{
    char *prefix = "MQTT mqtt_client: connection refused reason is ";
    char *reason = NULL;

    switch (fail_reason) {
    case MQTT5_UNSPECIFIED_ERROR:
        reason = "Unspecified error";
        break;
    case MQTT5_MALFORMED_PACKET:
        reason = "Malformed Packet";
        break;
    case MQTT5_PROTOCOL_ERROR:
        reason = "Protocol Error";
        break;
    case MQTT5_IMPLEMENT_SPECIFIC_ERROR:
        reason = "Implementation specific error";
        break;
    case MQTT5_UNSUPPORTED_PROTOCOL_VER:
        reason = "Unsupported Protocol Version";
        break;
    case MQTT5_INVAILD_CLIENT_ID:
        reason = "Client Identifier not valid";
        break;
    case MQTT5_BAD_USERNAME_OR_PWD:
        reason = "Bad User Name or Password";
        break;
    case MQTT5_NOT_AUTHORIZED:
        reason = "Not authorized";
        break;
    case MQTT5_SERVER_UNAVAILABLE:
        reason = "Server unavailable";
        break;
    case MQTT5_SERVER_BUSY:
        reason = "Server busy";
        break;
    case MQTT5_BANNED:
        reason = "Banned";
        break;
    case MQTT5_SERVER_SHUTTING_DOWN:
        reason = "Server shutting down";
        break;
    case MQTT5_BAD_AUTH_METHOD:
        reason = "Bad authentication method";
        break;
    case MQTT5_KEEP_ALIVE_TIMEOUT:
        reason = "Keep Alive timeout";
        break;
    case MQTT5_SESSION_TAKEN_OVER:
        reason = "Session taken over";
        break;
    case MQTT5_TOPIC_FILTER_INVAILD:
        reason = "Topic Filter invalid";
        break;
    case MQTT5_TOPIC_NAME_INVAILD:
        reason = "Topic Name invalid";
        break;
    case MQTT5_PACKET_IDENTIFIER_IN_USE:
        reason = "Packet Identifier in use";
        break;
    case MQTT5_PACKET_IDENTIFIER_NOT_FOUND:
        reason = "Packet Identifier not found";
        break;
    case MQTT5_RECEIVE_MAXIMUM_EXCEEDED:
        reason = "Receive Maximum exceeded";
        break;
    case MQTT5_TOPIC_ALIAS_INVAILD:
        reason = "Topic Alias invalid";
        break;
    case MQTT5_PACKET_TOO_LARGE:
        reason = "Packet too large";
        break;
    case MQTT5_MESSAGE_RATE_TOO_HIGH:
        reason = "Message rate too high";
        break;
    case MQTT5_QUOTA_EXCEEDED:
        reason = "Quota exceeded";
        break;
    case MQTT5_ADMINISTRATIVE_ACTION:
        reason = "Administrative action";
        break;
    case MQTT5_PAYLOAD_FORMAT_INVAILD:
        reason = "Payload format invalid";
        break;
    case MQTT5_RETAIN_NOT_SUPPORT:
        reason = "Retain not supported";
        break;
    case MQTT5_QOS_NOT_SUPPORT:
        reason = "QoS not supported";
        break;
    case MQTT5_USE_ANOTHER_SERVER:
        reason = "Use another server";
        break;
    case MQTT5_SERVER_MOVED:
        reason = "Server moved";
        break;
    case MQTT5_SHARED_SUBSCR_NOT_SUPPORTED:
        reason = "Shared Subscriptions not supported";
        break;
    case MQTT5_CONNECTION_RATE_EXCEEDED:
        reason = "Connection rate exceeded";
        break;
    case MQTT5_MAXIMUM_CONNECT_TIME:
        reason = "Maximum connect time";
        break;
    case MQTT5_SUBSCRIBE_IDENTIFIER_NOT_SUPPORT:
        reason = "Subscription Identifiers not supported";
        break;
    case MQTT5_WILDCARD_SUBSCRIBE_NOT_SUPPORT:
        reason = "Wildcard Subscriptions not supported";
        break;

    default:
        reason = "Unknown reason";
        break;
    }
    app_print("%s%s, id is %d\r\n", prefix, reason, fail_reason);

    return;
}

void mqtt_connect_severy_fail_reason_display(int16_t fail_reason)
{
    if (mqtt_mode_type_get() == MODE_TYPE_MQTT5) {
        mqtt5_fail_reason_display((mqtt5_connect_return_res_t)fail_reason);
    } else {
        mqtt_fail_reason_display((mqtt_connect_return_res_t)fail_reason);
    }

    return;
}

int16_t mqtt_connect_to_server(void)
{
    uint32_t connect_time = sys_current_time_get();

    connect_fail_reason = -1;
    app_print("\r\n");
    app_print("MQTT: Linking server...\r\n");

    LOCK_TCPIP_CORE();
    if (mqtt_mode_type_get() == MODE_TYPE_MQTT5) {
        if (client_user_info->clean_session_disabled) {
            mqtt_client->mqtt5_config->connect_property_info.session_expiry_interval = 0xFFFFFFFF;
        }
        if (mqtt5_client_connect(mqtt_client, &server_ip_addr, port, NULL, mqtt_connect_callback, NULL, client_user_info,
                                              &(mqtt_client->mqtt5_config->connect_property_info),
                                              &(mqtt_client->mqtt5_config->will_property_info)) != ERR_OK) {
            app_print("MQTT mqtt_client: connect to server failed\r\n");
            UNLOCK_TCPIP_CORE();
            mqtt_waiting_for_conn_cb = false;
            return connect_fail_reason;
        }
    } else {
        if (mqtt_client_connect(mqtt_client, &server_ip_addr, port, NULL, mqtt_connect_callback, NULL, client_user_info) != ERR_OK) {
            app_print("MQTT mqtt_client: connect to server failed\r\n");
            UNLOCK_TCPIP_CORE();
            mqtt_waiting_for_conn_cb = false;
            return connect_fail_reason;
        }
    }
    UNLOCK_TCPIP_CORE();

    mqtt_task_handle = sys_current_task_handle_get();

    while(mqtt_client_is_connected(mqtt_client) == false) {
        if (sys_current_time_get() - connect_time > MQTT_LINK_TIME_LIMIT) {
            app_print("MQTT: Connection timed out\r\n");
            return -1;
        }
        if ((mqtt_mode_type_get() == MODE_TYPE_MQTT5) && (connect_fail_reason == MQTT_CONNECTION_REFUSE_PROTOCOL)) {
            LOCK_TCPIP_CORE();
            mqtt5_disconnect(mqtt_client);
            UNLOCK_TCPIP_CORE();
            mqtt5_param_delete(mqtt_client);
            mqtt_mode_type_set(MODE_TYPE_MQTT);
            app_print("MQTT: The server does not support version 5.0, now switch to version 3.1.1\r\n");
            return mqtt_connect_to_server();
        } else if (connect_fail_reason > 0) {
            mqtt_connect_severy_fail_reason_display(connect_fail_reason);
            return connect_fail_reason;
        }
    }

    app_print("MQTT: Successfully connected to server\r\n");
    app_print("# ");
    mqtt_client->run = true;
    auto_reconnect_num = 0;

    return 0;
}

void mqtt_connect_free(void)
{
    LOCK_TCPIP_CORE();
    if (mqtt_mode_type_get() == MODE_TYPE_MQTT5) {
        mqtt5_disconnect(mqtt_client);
    } else {
        mqtt_disconnect(mqtt_client);
    }
    UNLOCK_TCPIP_CORE();
    connect_fail_reason = -1;
    app_print("MQTT: disconnect with server\r\n");

    return;
}

static void mqtt_task(void *param)
{
    uint16_t res = 0;
    bool mqtt_run_flag = false;

    if (mqtt5_param_cfg(mqtt_client)) {
        app_print("MQTT: Configuration parameters failed, stop connection\r\n");
        goto exit;
    }

    mqtt_client->run = false;
    mqtt_waiting_for_conn_cb = true;
    mqtt_run_flag = false;
connect:
    res = mqtt_connect_to_server();

    while (mqtt_client->run) {
        mqtt_run_flag = true;
        mqtt_publish_msg_handle();
        mqtt_subscribe_or_unsubscribe_msg_handle();

        if (mqtt_client_is_connected(mqtt_client) == false) {
            if (client_user_info->clean_session_disabled == 0) {
                at_topic_sub_list_free();
            }
            if (auto_reconnect && auto_reconnect_num < AUTO_RECONNECT_LIMIT) {
                if (auto_reconnect_num)
                    sys_ms_sleep(auto_reconnect_interval * auto_reconnect_num);
                auto_reconnect_num++;
                mqtt_waiting_for_conn_cb = true;
                goto connect;
            } else {
#ifdef CONFIG_ATCMD
                at_mqtt_disconn_print();
#endif
                while (mqtt_waiting_for_conn_cb) {
                    sys_ms_sleep(100);
                }
                mqtt_task_suspend();
                break;
            }
        }
        mqtt_task_suspend();
    }

    mqtt_connect_free();

exit:
#ifdef CONFIG_ATCMD
    if (res == 0 || mqtt_run_flag != false) {
        mqtt_info_free();
    }
#else
    mqtt_info_free();
#endif
    mqtt_resource_free();
    sys_task_delete(NULL);

    return;
}

int rtos_mqtt_task_create(void)
{
    mqtt_task_suspended = false;
    os_task_t task_handle = sys_task_create_dynamic((const uint8_t *)"MQTT task",
                                        MQTT_TASK_STACK_SIZE, MQTT_TASK_PRIO, (task_func_t)mqtt_task, NULL);
    if (task_handle == NULL) {
        return -1;
    }
    return 0;
}

static int mqtt_ip_prase(ip_addr_t *addr_ip, char *domain)
{
    int i;
    uint16_t addr[4] = {0};
    struct addrinfo hints, *res;
    void *ptr;
    char ip_addr[32];

    if (domain == NULL) {
        goto err_ip_addr;
    }

    memset(&hints, 0, sizeof(hints));
    if (getaddrinfo(domain, NULL, &hints, &res) != 0) {
        goto err_ip_addr;
    }
    if (res->ai_family != AF_INET) {
        app_print("MQTT: only support ipv4 address.\r\n");
        freeaddrinfo(res);
        goto err_ip_addr;
    }
    ptr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;

    inet_ntop(res->ai_family, ptr, ip_addr, sizeof(ip_addr));
    freeaddrinfo(res);

    char *p = ip_addr;
    for (i = 0; i < 4; i++) {
        do {
            addr[i] = 10 * addr[i] + (uint16_t)(*p - '0');
            p++;
        } while(*p != '.' && *p != 0);
        p++;
        if (addr[i] > 255) {
            goto err_ip_addr;
        }
    }
#ifdef CONFIG_IPV6_SUPPORT
    addr_ip->u_addr.ip4.addr = PP_HTONL(LWIP_MAKEU32(addr[0], addr[1], addr[2], addr[3]));
#else
    addr_ip->addr= PP_HTONL(LWIP_MAKEU32(addr[0], addr[1], addr[2], addr[3]));
#endif

    return 0;

err_ip_addr:
    app_print("MQTT: error ip address\r\n");
    return -1;
}

void mqtt_client_info_set(int argc, char **argv)
{
    uint32_t len;
    char *clinet_id = mqtt_client_id_get();

    if (argc == 2) {
        app_print("MQTT: client id is: %s\r\n", clinet_id);
    } else if (argc == 3) {
        if (*(argv[2]) == '?') {
            goto usage;
        }
        len = strlen(argv[2]);
        if (len > 20) {
            app_print("MQTT: client id len must <= 20\r\n");
            return;
        }

        app_print("MQTT: old client id is %s\r\n", clinet_id);
        if (mqtt_client_id_set(argv[2], strlen(argv[2])) == 0) {
            app_print("MQTT: new client id is %s\r\n", clinet_id);
        } else {
            app_print("MQTT: client id set failed\r\n");
            goto usage;
        }
    } else {
        goto usage;
    }

    return;
usage:
    app_print("MQTT Usage: mqtt client_id [new client id]\r\n");
    return;
}

void mqtt_connect_server(int argc, char **argv)
{
    /*# if use certificates to prove identity, put ca, client_crt and client_private_key
     *  in mqtt_ssl_config.c which provided by the server.
     *# If access server whitout SSL, just undefine LWIP_SSL_MQTT in lwipopts.h.
    */
    uint16_t name_len, pass_len, host_len;
    bool at_ota_enabled = false;

    if (mqtt_client != NULL) {
        app_print("MQTT: mqtt client is running, plese disconnect with the server first\r\n");
        return;
    }

    if ((argc == 3) && (*(argv[2]) == '?')) {
        goto usage;
    }
    if (argc < 5 || argc > 7) {
        goto usage;
    }

    mqtt_mode_type_set(MODE_TYPE_MQTT5);

    mqtt_client = mqtt_client_new();
    if (mqtt_client == NULL) {
        app_print("MQTT mqtt_client: rtos malloc mqtt client memory fail\r\n");
        return;
    }
    mqtt_client_info_init();
    client_user_info = get_client_param_data_get();
    client_user_info->client_user = NULL;
    client_user_info->client_pass = NULL;
    port = MQTT_DEFAULT_PORT;

    if (mqtt_ip_prase(&server_ip_addr, argv[2]) != 0) {
        app_print("MQTT mqtt_client: ip addrress error\r\n");
        goto usage;
    }

    if (mqtt_host) {
        sys_mfree(mqtt_host);
        mqtt_host = NULL;
    }
    host_len = strlen(argv[2]);
    mqtt_host = sys_malloc(host_len + 1);
    if (mqtt_host == NULL) {
        app_print("MQTT mqtt_client: rtos malloc mqtt host memory fail\r\n");
        goto exit;
    }
    sys_memcpy(mqtt_host, argv[2], host_len);
    if (mqtt_host[host_len] != 0) {
        mqtt_host[host_len] = 0;
    }

    port = (uint16_t)atoi(argv[3]);

    tls_encry_mode = (uint8_t)atoi(argv[4]);
    if (tls_encry_mode > 3) {
        app_print("MQTT mqtt_client: encryption set error\r\n");
        goto usage;
    }

    if (argc == 7) {
        name_len = strlen((const char *)argv[5]) + 1;
        pass_len = strlen((const char *)argv[6]) + 1;
        if (client_user_info_malloc(name_len, pass_len) != ERR_OK) {
            app_print("MQTT mqtt_client_user_info: rtos malloc client_user_info fail\r\n");
            goto exit;
        }
        memcpy(client_user_info->client_user, argv[5], name_len);
        memcpy(client_user_info->client_pass, argv[6], pass_len);
    }

    mqtt_set_inpub_callback(mqtt_client, mqtt_receive_pub_msg_print, mqtt_receive_msg_print, client_user_info);

    if (mqtt_ssl_cfg(mqtt_client, tls_encry_mode, at_ota_enabled)) {
        client_user_info_free();
        goto exit;
    }

    if(rtos_mqtt_task_create()) {
        app_print("MQTT mqtt_client: start mqtt task fail\r\n");
        client_user_info_free();
        goto exit;
    }
    return;

usage:
    app_print("MQTT Usage: mqtt connect <server_ip> <server_port default:1883> <encryption: 0-3> [<user_name> <user_password>]\r\n");
    app_print("                 encryption: 0-no encryption; 1-TLS without pre-shared key and certificate;\r\n");
    app_print("                 encryption: 2-TLS with one-way certificate; 3-TLS with two-way certificate;\r\n");
    app_print("  # Use user_name and user_password which have be registered on the server to prove identity.\r\n");
    app_print("eg: mqtt connect 192.168.3.101 8885 2 vic 123\r\n");
exit:
    mqtt_client_free(mqtt_client);
    mqtt_client = NULL;
    mqtt_host_free();
    return;
}

static int mqtt_msg_pub_func(const char *topic, const char *data, uint32_t data_len, uint8_t qos, uint8_t retain)
{
    publish_msg_t *cmd_msg_pub = NULL;
    uint32_t input_topic_len, input_data_len;

    if ((mqtt_client == NULL) || mqtt_client_is_connected(mqtt_client) == false) {
#ifndef CONFIG_ATCMD
        app_print("MQTT at_mqtt_msg_pub: client is disconnected, please connect it\r\n");
#else
        app_print("MQTT at_mqtt_msg_pub: client is disconnected, please connect it, ERR CODE:0x%08x\r\n", AT_MQTT_IN_DISCONNECTED_STATE);
#endif
        if ((auto_reconnect != true) || (auto_reconnect_num >= AUTO_RECONNECT_LIMIT)) {
            return -1;
        }
    }

    if (qos > 2) {
#ifndef CONFIG_ATCMD
        app_print("MQTT at_mqtt_msg_pub: qos value is wrong\r\n");
#else
        app_print("MQTT at_mqtt_msg_pub: qos value is wrong, ERR CODE:0x%08x\r\n", AT_MQTT_QOS_VALUE_IS_WRONG);
#endif
        return -2;
    }

    if (retain > 1) {
#ifndef CONFIG_ATCMD
        app_print("MQTT at_mqtt_msg_pub: retain value is wrong\r\n");
#else
        app_print("MQTT at_mqtt_msg_pub: retain value is wrong, ERR CODE:0x%08x\r\n", AT_MQTT_RETAIN_VALUE_IS_WRONG);
#endif
        return -2;
    }

    input_topic_len = strlen((const char *)topic) + 1;
    input_data_len = data_len + 1;
    cmd_msg_pub = publish_msg_mem_malloc(input_topic_len, input_data_len);
    if (cmd_msg_pub == NULL) {
#ifndef CONFIG_ATCMD
        app_print("MQTT at_mqtt_msg_pub: rtos malloc publish msg fail\r\n");
#else
        app_print("MQTT at_mqtt_msg_pub: rtos malloc publish msg fail, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
#endif
        return -1;
    }
    memcpy(cmd_msg_pub->topic, topic, input_topic_len);
    memcpy(cmd_msg_pub->msg, data, input_data_len);

    cmd_msg_pub->qos = qos;
    cmd_msg_pub->retain = retain;
    cmd_msg_pub->msg_len = data_len;

    sys_sched_lock();
    co_list_push_back(&(msg_pub_list.cmd_msg_pub_list), &(cmd_msg_pub->hdr));
    sys_sched_unlock();
    mqtt_task_resume(false);
    return 0;
}

void mqtt_msg_pub(int argc, char **argv)
{
    if ((argc == 3) && (*(argv[2]) == '?')) {
        goto usage;
    }

    if ((argc < 5) || (argc > 6)) {
        goto usage;
    }

    uint8_t retain = 0;
    if (argc == 6) {
        retain = (uint8_t)atoi(argv[5]);
        if (retain >= 2) {
            goto usage;
        }
    }

    uint32_t msg_len = strlen(argv[3]);
    int res = mqtt_msg_pub_func(argv[2], argv[3], msg_len, (uint8_t)atoi(argv[4]), retain);
    if (res != -2) {
        return;
    }

usage:
    app_print("MQTT Usage: mqtt publish <topic_name> <topic_content> <qos: 0~2> [retain: 0/1]\r\n");
    app_print("     qos 0: The receiver receives the massage at most once\r\n");
    app_print("     qos 1: The receiver receives the massage at least once\r\n");
    app_print("     qos 2: The receiver receives the massage just once\r\n");
    app_print("     retain 0: not retain the topic in server\r\n");
    app_print("     retain 1: retain the topic in server for send to subscriber in the future\r\n");
    return;
}

int mqtt_msg_sub_func(const char *topic, uint8_t qos, bool sub_or_unsub)
{
    sub_msg_t *cmd_msg_sub = NULL;
    uint32_t input_topic_len;

    if ((mqtt_client == NULL) || mqtt_client_is_connected(mqtt_client) == false) {
#ifndef CONFIG_ATCMD
        app_print("MQTT at_mqtt_msg_sub: client is disconnected, please connect it\r\n");
#else
        app_print("MQTT at_mqtt_msg_sub: client is disconnected, please connect it, ERR CODE:0x%08x\r\n", AT_MQTT_IN_DISCONNECTED_STATE);
#endif
        if ((auto_reconnect != true) || (auto_reconnect_num >= AUTO_RECONNECT_LIMIT)) {
            return -1;
        }
    }

    if (qos > 2) {
#ifndef CONFIG_ATCMD
        app_print("MQTT at_mqtt_msg_sub: qos value is wrong\r\n");
#else
        app_print("MQTT at_mqtt_msg_sub: qos value is wrong, ERR CODE:0x%08x\r\n", AT_MQTT_QOS_VALUE_IS_WRONG);
#endif
        return -2;
    }

    input_topic_len = strlen((const char *)topic) + 1;
    cmd_msg_sub = sub_msg_mem_malloc(input_topic_len);
    if (cmd_msg_sub == NULL) {
#ifndef CONFIG_ATCMD
        app_print("MQTT at_mqtt_msg_sub: rtos malloc subscribe msg fail\r\n");
#else
        app_print("MQTT at_mqtt_msg_sub: rtos malloc subscribe msg fail, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
#endif
        return -1;
    }
    memcpy(cmd_msg_sub->topic, topic, input_topic_len);

    cmd_msg_sub->qos = qos;

    if (sub_or_unsub != 0) {
        cmd_msg_sub->sub_or_unsub = true;
    } else {
        cmd_msg_sub->sub_or_unsub = false;
    }

    sys_sched_lock();
    co_list_push_back(&(msg_sub_list.cmd_msg_sub_list), &(cmd_msg_sub->hdr));
    sys_sched_unlock();
    mqtt_task_resume(false);
    return 0;
}

void mqtt_msg_sub(int argc, char **argv)
{
    if ((argc == 3) && (*(argv[2]) == '?')) {
        goto usage;
    }

    if ((argc < 5) || (argc > 6)) {
        goto usage;
    }

    bool sub_or_unsub = ((uint8_t)atoi(argv[4]) != 0);
    int res = mqtt_msg_sub_func(argv[2], (uint8_t)atoi(argv[3]), sub_or_unsub);
    if (res != -2) {
        return;
    }

usage:
    app_print("MQTT Usage: mqtt subscribe <topic_name> <qos: 0~2> <sub_or_unsub: 0/1>\r\n");
    app_print("     qos 0: The receiver receives the massage at most once\r\n");
    app_print("     qos 1: The receiver receives the massage at least once\r\n");
    app_print("     qos 2: The receiver receives the massage just once\r\n");
    app_print("     sub_or_unsub 0: unsubscribe the topic \r\n");
    app_print("     sub_or_unsub 1: subscribe the topic \r\n");
    return;
}

void mqtt_auto_reconnect_set(int argc, char **argv)
{
    if (argc == 2) {
        app_print("MQTT: current auto reconnect = %d\r\n", auto_reconnect);
    } else if (argc == 3) {
        if (*(argv[2]) == '?') {
            goto usage;
        }
        uint8_t get = (uint16_t)atoi(argv[2]);
        if (get > 0) {
            get = 1;
        }

        app_print("MQTT: current auto reconnect = %d, then auto reconnect = %d\r\n", auto_reconnect, get);
        auto_reconnect = (bool)get;
    } else {
        goto usage;
    }

    return;
usage:
    app_print("MQTT Usage: mqtt auto_reconnect [0: disable; 1: enable]\r\n");
    return;
}

void mqtt_client_disconnect(int argc, char **argv)
{
    if (mqtt_client != NULL) {
        mqtt_client->run = false;
    }

    mqtt_task_resume(false);
    return;
}

void cmd_mqtt(int argc, char **argv)
{
    /**
     * Test with mosquitto Server or Alibaba Cloud or baidu AI cloud(mqtt 3.1.1)
     * For mosquitto Server; execute CMD: mosquitto.exe -c mosquitto.conf to start svrver
     * in the server installation directory.
     * All server related configurations are in the mosquitto.conf file.
    */
    if (argc <= 1) {
        goto usage;
    }

    if (strcmp(argv[1], "connect") == 0) {
        mqtt_connect_server(argc, argv);
    } else if (strcmp(argv[1], "publish") == 0) {
        mqtt_msg_pub(argc, argv);
    } else if (strcmp(argv[1], "subscribe") == 0) {
        mqtt_msg_sub(argc, argv);
    } else if (strcmp(argv[1], "disconnect") == 0) {
        mqtt_client_disconnect(argc, argv);
    } else if (strcmp(argv[1], "auto_reconnect") == 0) {
        mqtt_auto_reconnect_set(argc, argv);
    } else if (strcmp(argv[1], "client_id") == 0) {
        mqtt_client_info_set(argc, argv);
    } else if (strcmp(argv[1], "help") == 0) {
        goto usage;
    } else {
        app_print("MQTT: mqtt command error\r\n");
    }

    return;

usage:
    app_print("Usage: \r\n");
    app_print("    mqtt <connect | publish | subscribe | help | ...> [param0] [param1]...\r\n");
    app_print("         connect <server_ip> <server_port default:1883> <encryption: 0-3> [<user_name> <user_password>]\r\n");
    app_print("                 encryption: 0-no encryption; 1-TLS without pre-shared key and certificate;\r\n");
    app_print("                 encryption: 2-TLS with one-way certificate; 3-TLS with two-way certificate;\r\n");
    app_print("         publish <topic_name> <topic_content> <qos: 0~2> [retain: 0/1]\r\n");
    app_print("         subscribe  <topic_name> <qos: 0~2> <sub_or_unsub: 0/1, 1 is sub and 0 is unsub>\r\n");
    app_print("         disconnect               --disconnect with server\r\n");
    app_print("         auto_reconnect           --set auto reconnect to server\r\n");
    app_print("         client_id [gigadevice2]  --check or change client_id\r\n");
    app_print("eg1.\r\n");
    app_print("    mqtt connect 192.168.3.101 8885 2 vic 123\r\n");
    app_print("eg2.\r\n");
    app_print("    mqtt publish topic helloworld 1 0\r\n");
    app_print("eg3.\r\n");
    app_print("    mqtt subscribe topic 0 1\r\n");
    app_print("eg4.\r\n");
    app_print("    mqtt subscribe ?\r\n");

    return;
}

#ifdef CONFIG_ATCMD
int at_mqtt_connect_server(const char *host, uint16_t at_port, uint8_t reconnect, bool at_ota_enabled)
{
    uint16_t len;

    if (mqtt_client != NULL) {
        app_print("MQTT: mqtt client is running, plese disconnect with the server first, ERR CODE:0x%08x\r\n", AT_MQTT_ALREADY_CONNECTED);
        return -1;
    }

    if (host == NULL) {
        app_print("MQTT at_mqtt_connect_server: host is null, ERR CODE:0x%08x\r\n", AT_MQTT_HOST_IS_NULL);
        return -1;
    }
    if (mqtt_host) {
        sys_mfree(mqtt_host);
        mqtt_host = NULL;
    }
    len = strlen(host);
    mqtt_host = sys_malloc(len + 1);
    if (mqtt_host == NULL) {
        app_print("mqtt_host malloc failed, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
        return -1;
    }
    sys_memcpy(mqtt_host, host, len);
    if (mqtt_host[len] != 0) {
        mqtt_host[len] = 0;
    }

    port = at_port;

    if (reconnect > 0) {
        reconnect = 1;
    }
    auto_reconnect = (bool)reconnect;

    mqtt_mode_type_set(MODE_TYPE_MQTT5);

    mqtt_client = mqtt_client_new();
    if (mqtt_client == NULL) {
        app_print("MQTT mqtt_client: rtos malloc mqtt client memory fail, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
        mqtt_host_free();
        return -1;
    }

    mqtt_client_info_init();
    client_user_info = get_client_param_data_get();
    if (client_user_info->client_id == NULL) {
        app_print("MQTT mqtt_client: client_id is null, ERR CODE:0x%08x\r\n", AT_MQTT_CLIENT_ID_IS_NULL);
        goto exit;
    }

    if (mqtt_ip_prase(&server_ip_addr, mqtt_host) != 0) {
        app_print("MQTT mqtt_client: ip addrress error, ERR CODE:0x%08x\r\n", AT_MQTT_URI_PARSE_FAILED);
        goto exit;
    }

    if (mqtt_scheme == 1) {
        tls_encry_mode = TLS_AUTH_MODE_NONE;
    } else if (mqtt_scheme == 2) {
        tls_encry_mode = TLS_AUTH_MODE_KEY_SHARE;
    } else if (mqtt_scheme == 3) {
        tls_encry_mode = TLS_AUTH_MODE_CERT_1WAY;
    } else if (mqtt_scheme == 4) {
        tls_encry_mode = TLS_AUTH_MODE_CERT_CLIENT_ONLY;
    } else if (mqtt_scheme == 5) {
        tls_encry_mode = TLS_AUTH_MODE_CERT_2WAY;
    }

    if (mqtt_ssl_cfg(mqtt_client, tls_encry_mode, at_ota_enabled)) {
        goto exit;
    }

    if (rtos_mqtt_task_create()) {
        app_print("MQTT mqtt_client: start mqtt task fail\r\n");
        goto exit;
    }
    return 0;

exit:
    mqtt_client_free(mqtt_client);
    mqtt_client = NULL;
    mqtt_host_free();
    return -1;
}

int at_mqtt_msg_pub(const char *topic, const char *data, uint32_t data_len, uint8_t qos, uint8_t retain) {
    return mqtt_msg_pub_func(topic, data, data_len, qos, retain);
}

int at_mqtt_msg_sub(const char *topic, uint8_t qos, bool sub_or_unsub)
{
    if ((at_topic_exist(topic) && sub_or_unsub) || (at_topic_exist(topic) == false && sub_or_unsub == false)) {
        return -2;
    }

    int res = mqtt_msg_sub_func(topic, qos, sub_or_unsub);
    if (res == -2) {
        res = -1;
    }
    return res;
}
#endif
#endif //CONFIG_MQTT
