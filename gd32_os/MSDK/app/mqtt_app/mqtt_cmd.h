/*!
    \file    mqtt_cmd.h
    \brief   mqtt command for GD32VW55x SDK.

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

#ifndef _MQTT_CMD_H_
#define _MQTT_CMD_H_

#include "app_cfg.h"

#ifdef CONFIG_MQTT
#include "co_list.h"
#include "lwip/apps/mqtt.h"

#define MQTT_DEFAULT_PORT 1883

#ifdef CONFIG_ATCMD
#define AT_MQTT_NO_CONFIGURED                           0x6001
#define AT_MQTT_NOT_IN_CONFIGURED_STATE                 0x6002
#define AT_MQTT_UNINITIATED_OR_ALREADY_CLEAN            0x6003
#define AT_MQTT_ALREADY_CONNECTED                       0x6004
#define AT_MQTT_MALLOC_FAILED                           0x6005
#define AT_MQTT_NULL_LINK                               0x6006
#define AT_MQTT_NULL_PARAMTER                           0x6007
#define AT_MQTT_PARAMETER_COUNTS_IS_WRONG               0x6008
#define AT_MQTT_TLS_CONFIG_ERROR                        0x6009
#define AT_MQTT_PARAM_PREPARE_ERROR                     0x600A
#define AT_MQTT_CLIENT_START_FAILED                     0x600B
#define AT_MQTT_CLIENT_PUBLISH_FAILED                   0x600C
#define AT_MQTT_CLIENT_SUBSCRIBE_FAILED                 0x600D
#define AT_MQTT_CLIENT_UNSUBSCRIBE_FAILED               0x600E
#define AT_MQTT_CLIENT_DISCONNECT_FAILED                0x600F
#define AT_MQTT_LINK_ID_READ_FAILED                     0x6010
#define AT_MQTT_LINK_ID_VALUE_IS_WRONG                  0x6011
#define AT_MQTT_SCHEME_READ_FAILED                      0x6012
#define AT_MQTT_SCHEME_VALUE_IS_WRONG                   0x6013
#define AT_MQTT_CLIENT_ID_READ_FAILED                   0x6014
#define AT_MQTT_CLIENT_ID_IS_NULL                       0x6015
#define AT_MQTT_CLIENT_ID_IS_OVERLENGTH                 0x6016
#define AT_MQTT_USERNAME_READ_FAILED                    0x6017
#define AT_MQTT_USERNAME_IS_NULL                        0x6018
#define AT_MQTT_USERNAME_IS_OVERLENGTH                  0x6019
#define AT_MQTT_PASSWORD_READ_FAILED                    0x601A
#define AT_MQTT_PASSWORD_IS_NULL                        0x601B
#define AT_MQTT_PASSWORD_IS_OVERLENGTH                  0x601C
#define AT_MQTT_CERT_KEY_ID_READ_FAILED                 0x601D
#define AT_MQTT_CERT_KEY_ID_VALUE_IS_WRONG              0x601E
#define AT_MQTT_CA_ID_READ_FAILED                       0x601F
#define AT_MQTT_CA_ID_VALUE_IS_WRONG                    0x6020
#define AT_MQTT_CA_LENGTH_ERROR                         0x6021
#define AT_MQTT_CA_READ_FAILED                          0x6022
#define AT_MQTT_CERT_LENGTH_ERROR                       0x6023
#define AT_MQTT_CERT_READ_FAILED                        0x6024
#define AT_MQTT_KEY_LENGTH_ERROR                        0x6025
#define AT_MQTT_KEY_READ_FAILED                         0x6026
#define AT_MQTT_PATH_READ_FAILED                        0x6027
#define AT_MQTT_PATH_IS_NULL                            0x6028
#define AT_MQTT_PATH_IS_OVERLENGTH                      0x6029
#define AT_MQTT_VERSION_READ_FAILED                     0x602A
#define AT_MQTT_KEEPALIVE_READ_FAILED                   0x602B
#define AT_MQTT_KEEPALIVE_IS_NULL                       0x602C
#define AT_MQTT_KEEPALIVE_VALUE_IS_WRONG                0x602D
#define AT_MQTT_DISABLE_CLEAN_SESSION_READ_FAILED       0x602E
#define AT_MQTT_DISABLE_CLEAN_SESSION_VALUE_IS_WRONG    0x602F
#define AT_MQTT_LWT_TOPIC_READ_FAILED                   0x6030
#define AT_MQTT_LWT_TOPIC_IS_NULL                       0x6031
#define AT_MQTT_LWT_TOPIC_IS_OVERLENGTH                 0x6032
#define AT_MQTT_LWT_MSG_READ_FAILED                     0x6033
#define AT_MQTT_LWT_MSG_IS_NULL                         0x6034
#define AT_MQTT_LWT_MSG_IS_OVERLENGTH                   0x6035
#define AT_MQTT_LWT_QOS_READ_FAILED                     0x6036
#define AT_MQTT_LWT_QOS_VALUE_IS_WRONG                  0x6037
#define AT_MQTT_LWT_RETAIN_READ_FAILED                  0x6038
#define AT_MQTT_LWT_RETAIN_VALUE_IS_WRONG               0x6039
#define AT_MQTT_HOST_READ_FAILED                        0x603A
#define AT_MQTT_HOST_IS_NULL                            0x603B
#define AT_MQTT_HOST_IS_OVERLENGTH                      0x603C
#define AT_MQTT_PORT_READ_FAILED                        0x603D
#define AT_MQTT_PORT_VALUE_IS_WRONG                     0x603E
#define AT_MQTT_RECONNECT_READ_FAILED                   0x603F
#define AT_MQTT_RECONNECT_VALUE_IS_WRONG                0x6040
#define AT_MQTT_TOPIC_READ_FAILED                       0x6041
#define AT_MQTT_TOPIC_IS_NULL                           0x6042
#define AT_MQTT_TOPIC_IS_OVERLENGTH                     0x6043
#define AT_MQTT_DATA_READ_FAILED                        0x6044
#define AT_MQTT_DATA_IS_NULL                            0x6045
#define AT_MQTT_DATA_IS_OVERLENGTH                      0x6046
#define AT_MQTT_QOS_READ_FAILED                         0x6047
#define AT_MQTT_QOS_VALUE_IS_WRONG                      0x6048
#define AT_MQTT_RETAIN_READ_FAILED                      0x6049
#define AT_MQTT_RETAIN_VALUE_IS_WRONG                   0x604A
#define AT_MQTT_PUBLISH_LENGTH_READ_FAILED              0x604B
#define AT_MQTT_PUBLISH_LENGTH_VALUE_IS_WRONG           0x604C
#define AT_MQTT_RECV_LENGTH_IS_WRONG                    0x604D
#define AT_MQTT_CREATE_SEMA_FAILED                      0x604E
#define AT_MQTT_CREATE_EVENT_GROUP_FAILED               0x604F
#define AT_MQTT_URI_PARSE_FAILED                        0x6050
#define AT_MQTT_IN_DISCONNECTED_STATE                   0x6051
#define AT_MQTT_HOSTNAME_VERIFY_FAILED                  0x6052
#endif

enum mqtt_mode {
    MODE_TYPE_MQTT = 1U,
    MODE_TYPE_MQTT5 = 2U,
};

typedef struct publish_msg {
    struct co_list_hdr hdr;
    char* topic;
    char* msg;
    uint32_t msg_len;
    uint8_t qos;
    uint8_t retain;
} publish_msg_t;

typedef struct sub_msg {
    struct co_list_hdr hdr;
    char* topic;
    uint8_t qos;
    bool sub_or_unsub;
} sub_msg_t;

typedef struct cmd_msg_pub
{
    struct co_list cmd_msg_pub_list;
} cmd_msg_pub_t;

typedef struct cmd_msg_sub
{
    struct co_list cmd_msg_sub_list;
} cmd_msg_sub_t;

void mqtt_mode_type_set(enum mqtt_mode cmd_mode);
enum mqtt_mode mqtt_mode_type_get(void);
mqtt_client_t* mqtt_client_get(void);
struct mqtt_connect_client_info_t *client_user_info_get(void);
void mqtt_scheme_set(uint16_t scheme);
uint16_t mqtt_scheme_get(void);
uint16_t mqtt_port_get(void);
uint8_t mqtt_reconnect_get(void);
int mqtt_ws_path_set(char *path);
void mqtt_ws_path_free(void);
char *mqtt_ws_path_get(void);
char *mqtt_host_get(void);
void mqtt_host_free(void);
bool at_topic_exist(const char *topic);
void at_topic_sub_list_free(void);
void cmd_mqtt(int argc, char **argv);
void mqtt_connect_server(int argc, char **argv);
void mqtt_msg_pub(int argc, char **argv);
void mqtt_msg_sub(int argc, char **argv);
void mqtt_client_disconnect(int argc, char **argv);
void mqtt_auto_reconnect_set(int argc, char **argv);
void mqtt_task_resume(bool isr);
int at_mqtt_connect_server(const char *host, uint16_t at_port, uint8_t reconnect, bool at_ota_enabled);
int at_mqtt_msg_pub(const char *topic, const char *data, uint32_t data_len, uint8_t qos, uint8_t retain);
int at_mqtt_msg_sub(const char *topic, uint8_t qos, bool sub_or_unsub);

#endif

#endif // _MQTT_CMD_H_
