/*!
    \file    atcmd_mqtt.c
    \brief   AT command MQTT part for GD32VW55x SDK

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

#ifdef CONFIG_MQTT
#include "mqtt_cmd.h"
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"
#include "mqtt_client_config.h"
#include "mqtt5_client_config.h"
#include "co_utils.h"

static bool mqtt_usercfg_setted = false;

void at_mqtt_receive_pub_msg_print(void *inpub_arg, const char *data, uint16_t payload_length)
{
    AT_RSP_START(payload_length + 64);
    AT_RSP("+MQTTSUBRECV:0,\"%s\",", data);
    AT_RSP_IMMEDIATE();
    AT_RSP_FREE();
    return;
}

void at_mqtt_receive_msg_print(void *inpub_arg, const uint8_t *data, uint16_t payload_length, uint8_t flags, uint8_t retain)
{
    AT_RSP_START(payload_length + 64);
    AT_RSP("%d,", payload_length);
    for (uint16_t i = 0; i < payload_length; i++) {
        AT_RSP("%c", data[i]);
    }
    AT_RSP("\r\n");
    AT_RSP_IMMEDIATE();
    AT_RSP_FREE();
    return;
}

/*!
    \brief      the AT command set user properties
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_mqtt_user_cfg(int argc, char **argv)
{
    uint16_t link_id, scheme, cert_key_id, ca_id;
    char *endptr = NULL, *client_id, *username, *password;

    AT_RSP_START(512);
    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
            goto Error;
        }
    } else if (argc == 8) {
        link_id = (uint16_t)strtoul((const char *)argv[1], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_READ_FAILED);
            goto Error;
        }
        if (link_id != 0) {
            AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_VALUE_IS_WRONG);
            goto Error;
        }
        scheme = (uint16_t)strtoul((const char *)argv[2], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT scheme, ERR CODE:0x%08x\r\n", AT_MQTT_SCHEME_READ_FAILED);
            goto Error;
        }
        if (scheme < 1 || scheme > 5) {
            AT_TRACE("invalid MQTT scheme, ERR CODE:0x%08x\r\n", AT_MQTT_SCHEME_VALUE_IS_WRONG);
            goto Error;
        }
        client_id = at_string_parse(argv[3]);
        if (client_id == NULL) {
            AT_TRACE("invalid MQTT client_id, ERR CODE:0x%08x\r\n", AT_MQTT_CLIENT_ID_READ_FAILED);
            goto Error;
        }
        if (strlen(client_id) > MQTT_CLIENT_ID_LEN) {
            AT_TRACE("invalid MQTT client_id, ERR CODE:0x%08x\r\n", AT_MQTT_CLIENT_ID_IS_OVERLENGTH);
            goto Error;
        }
        username = at_string_parse(argv[4]);
        if (username == NULL) {
            AT_TRACE("invalid MQTT username, ERR CODE:0x%08x\r\n", AT_MQTT_USERNAME_READ_FAILED);
            goto Error;
        }
        if (strlen(username) > MQTT_USERNAME_LEN) {
            AT_TRACE("invalid MQTT username, ERR CODE:0x%08x\r\n", AT_MQTT_USERNAME_IS_OVERLENGTH);
            goto Error;
        }
        password = at_string_parse(argv[5]);
        if (password == NULL) {
            AT_TRACE("invalid MQTT password, ERR CODE:0x%08x\r\n", AT_MQTT_PASSWORD_READ_FAILED);
            goto Error;
        }
        if (strlen(password) > MQTT_PASSWORD_LEN) {
            AT_TRACE("invalid MQTT password, ERR CODE:0x%08x\r\n", AT_MQTT_PASSWORD_IS_OVERLENGTH);
            goto Error;
        }
        cert_key_id = (uint16_t)strtoul((const char *)argv[6], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT cert_key_id, ERR CODE:0x%08x\r\n", AT_MQTT_CERT_KEY_ID_READ_FAILED);
            goto Error;
        }
        if (cert_key_id != 0) {
            AT_TRACE("invalid MQTT cert_key_id, ERR CODE:0x%08x\r\n", AT_MQTT_CERT_KEY_ID_VALUE_IS_WRONG);
            goto Error;
        }
        ca_id = (uint16_t)strtoul((const char *)argv[7], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT ca_id, ERR CODE:0x%08x\r\n", AT_MQTT_CA_ID_READ_FAILED);
            goto Error;
        }
        if (ca_id != 0) {
            AT_TRACE("invalid MQTT ca_id, ERR CODE:0x%08x\r\n", AT_MQTT_CA_ID_VALUE_IS_WRONG);
            goto Error;
        }
        if (mqtt_client_id_set(client_id, strlen(client_id)) != 0) {
            AT_TRACE("MQTT: client id set failed\r\n");
            goto Error;
        }
        if (mqtt_client_user_set(username, strlen(username)) != 0) {
            AT_TRACE("MQTT: user name set failed\r\n");
            goto Error;
        }
        if (mqtt_client_pass_set(password, strlen(password)) != 0) {
            AT_TRACE("MQTT: user password set failed\r\n");
            goto Error;
        }
        mqtt_scheme_set(scheme);
    } else {
        AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
        goto Error;
    }

    mqtt_usercfg_setted = true;
    AT_RSP_OK();
    return;

Error:
    client_user_info_free();
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+MQTTUSERCFG=<LinkID>,<scheme>,<\"client_id\">,<\"username\">,<\"password\">,<cert_key_ID>,<CA_ID>\r\n");
    AT_RSP_OK();
    return;
}

/*!
    \brief      the AT command set client ID
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_mqtt_set_client_id(int argc, char **argv)
{
    uint16_t link_id, length;
    char *endptr = NULL, *client_id;

    AT_RSP_START(128);
    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
            goto Error;
        }
    } else if (argc == 3) {
        if (mqtt_usercfg_setted) {
            link_id = (uint16_t)strtoul((const char *)argv[1], &endptr, 10);
            if ((*endptr != '\0')) {
                AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_READ_FAILED);
                goto Error;
            }
            if (link_id != 0) {
                AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_VALUE_IS_WRONG);
                goto Error;
            }
            length = (uint16_t)strtoul((const char *)argv[2], &endptr, 10);
            if ((*endptr != '\0')) {
                AT_TRACE("invalid MQTT length\r\n");
                goto Error;
            }
            if (length < 1) {
                AT_TRACE("invalid MQTT length, ERR CODE:0x%08x\r\n", AT_MQTT_CLIENT_ID_IS_NULL);
                goto Error;
            }
            if (length > MQTT_CLIENT_ID_MAX_LEN) {
                AT_TRACE("invalid MQTT length, ERR CODE:0x%08x\r\n", AT_MQTT_CLIENT_ID_IS_OVERLENGTH);
                goto Error;
            }
            AT_RSP_DIRECT("OK\r\n", 4);
            AT_RSP_DIRECT(">\r\n", 3);
            client_id = sys_malloc(length);
            if (NULL == client_id) {
                AT_TRACE("Allocate cilentID buffer failed, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
                goto Error;
            }

            // Block here to wait dma receive done
            at_hw_dma_receive((uint32_t)client_id, length);
            if (mqtt_client_id_set(client_id, length) == 0) {
                AT_TRACE("MQTT: client id set successful\r\n");
            } else {
                AT_TRACE("MQTT: client id set failed\r\n");
                sys_mfree(client_id);
                goto Error;
            }
        } else {
            AT_TRACE("not user configure, ERR CODE:0x%08x\r\n", AT_MQTT_NO_CONFIGURED);
            goto Error;
        }
    } else {
        AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
        goto Error;
    }

    sys_mfree(client_id);
    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+MQTTLONGCLIENTID=<LinkID>,<length>\r\n");
    AT_RSP_OK();
    return;
}

/*!
    \brief      the AT command set user name
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_mqtt_set_username(int argc, char **argv)
{
    uint16_t link_id, length;
    char *endptr = NULL, *client_user;

    AT_RSP_START(128);
    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
            goto Error;
        }
    } else if (argc == 3) {
        if (mqtt_usercfg_setted) {
            link_id = (uint16_t)strtoul((const char *)argv[1], &endptr, 10);
            if ((*endptr != '\0')) {
                AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_READ_FAILED);
                goto Error;
            }
            if (link_id != 0) {
                AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_VALUE_IS_WRONG);
                goto Error;
            }
            length = (uint16_t)strtoul((const char *)argv[2], &endptr, 10);
            if ((*endptr != '\0')) {
                AT_TRACE("invalid MQTT length\r\n");
                goto Error;
            }
            if (length < 1) {
                AT_TRACE("invalid MQTT length, ERR CODE:0x%08x\r\n", AT_MQTT_USERNAME_IS_NULL);
                goto Error;
            }
            if (length > MQTT_USERNAME_MAX_LEN) {
                AT_TRACE("invalid MQTT length, ERR CODE:0x%08x\r\n", AT_MQTT_USERNAME_IS_OVERLENGTH);
                goto Error;
            }
            AT_RSP_DIRECT("OK\r\n", 4);
            AT_RSP_DIRECT(">\r\n", 3);
            client_user = sys_malloc(length);
            if (NULL == client_user) {
                AT_TRACE("Allocate user name buffer failed, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
                goto Error;
            }

            // Block here to wait dma receive done
            at_hw_dma_receive((uint32_t)client_user, length);
            if (mqtt_client_user_set(client_user, length) == 0) {
                AT_TRACE("MQTT: user name set successful\r\n");
            } else {
                AT_TRACE("MQTT: user name set failed\r\n");
                sys_mfree(client_user);
                goto Error;
            }
        } else {
            AT_TRACE("not user configure, ERR CODE:0x%08x\r\n", AT_MQTT_NO_CONFIGURED);
            goto Error;
        }
    } else {
        AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
        goto Error;
    }

    sys_mfree(client_user);
    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+MQTTLONGUSERNAME=<LinkID>,<length>\r\n");
    AT_RSP_OK();
    return;
}

/*!
    \brief      the AT command set password
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_mqtt_set_password(int argc, char **argv)
{
    uint16_t link_id, length;
    char *endptr = NULL, *client_pass;

    AT_RSP_START(128);
    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
            goto Error;
        }
    } else if (argc == 3) {
        if (mqtt_usercfg_setted) {
            link_id = (uint16_t)strtoul((const char *)argv[1], &endptr, 10);
            if ((*endptr != '\0')) {
                AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_READ_FAILED);
                goto Error;
            }
            if (link_id != 0) {
                AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_VALUE_IS_WRONG);
                goto Error;
            }
            length = (uint16_t)strtoul((const char *)argv[2], &endptr, 10);
            if ((*endptr != '\0')) {
                AT_TRACE("invalid MQTT length\r\n");
                goto Error;
            }
            if (length < 1) {
                AT_TRACE("invalid MQTT length, ERR CODE:0x%08x\r\n", AT_MQTT_PASSWORD_IS_NULL);
                goto Error;
            }
            if (length > MQTT_PASSWORD_MAX_LEN) {
                AT_TRACE("invalid MQTT length, ERR CODE:0x%08x\r\n", AT_MQTT_PASSWORD_IS_OVERLENGTH);
                goto Error;
            }
            AT_RSP_DIRECT("OK\r\n", 4);
            AT_RSP_DIRECT(">\r\n", 3);
            client_pass = sys_malloc(length);
            if (NULL == client_pass) {
                AT_TRACE("Allocate user password buffer failed, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
                goto Error;
            }

            // Block here to wait dma receive done
            at_hw_dma_receive((uint32_t)client_pass, length);
            if (mqtt_client_pass_set(client_pass, length) == 0) {
                AT_TRACE("MQTT: user password set successful\r\n");
            } else {
                AT_TRACE("MQTT: user password set failed\r\n");
                sys_mfree(client_pass);
                goto Error;
            }
        } else {
            AT_TRACE("not user configure, ERR CODE:0x%08x\r\n", AT_MQTT_NO_CONFIGURED);
            goto Error;
        }
    } else {
        AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
        goto Error;
    }

    sys_mfree(client_pass);
    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+MQTTLONGPASSWORD=<LinkID>,<length>\r\n");
    AT_RSP_OK();
    return;
}

/*!
    \brief      the AT command set connection properties
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_mqtt_conn_cfg(int argc, char **argv)
{
    uint16_t link_id, keep_alive = MQTT_DEFAULT_PING_TIMEOUT;
    uint8_t clean_session_disabled, will_qos = 0, will_retain = 0;
    char *endptr = NULL, *will_topic = NULL, *will_msg = NULL;

    AT_RSP_START(512);
    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
            goto Error;
        }
    } else if (argc == 8) {
        link_id = (uint16_t)strtoul((const char *)argv[1], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_READ_FAILED);
            goto Error;
        }
        if (link_id != 0) {
            AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_VALUE_IS_WRONG);
            goto Error;
        }
        keep_alive = (uint16_t)strtoul((const char *)argv[2], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT keep_alive, ERR CODE:0x%08x\r\n", AT_MQTT_KEEPALIVE_READ_FAILED);
            goto Error;
        }
        if (keep_alive > MQTT_MAX_PING_TIMEOUT) {
            AT_TRACE("invalid MQTT keep_alive, ERR CODE:0x%08x\r\n", AT_MQTT_KEEPALIVE_VALUE_IS_WRONG);
            goto Error;
        }
        if (keep_alive == 0) {
            keep_alive = MQTT_DEFAULT_PING_TIMEOUT;
        }
        clean_session_disabled = (uint8_t)strtoul((const char *)argv[3], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT clean_session_disabled, ERR CODE:0x%08x\r\n", AT_MQTT_DISABLE_CLEAN_SESSION_READ_FAILED);
            goto Error;
        }
        if (clean_session_disabled > 1) {
            AT_TRACE("invalid MQTT clean_session_disabled, ERR CODE:0x%08x\r\n", AT_MQTT_DISABLE_CLEAN_SESSION_VALUE_IS_WRONG);
            goto Error;
        }
        will_topic = at_string_parse(argv[4]);
        if (will_topic == NULL) {
            AT_TRACE("invalid MQTT will_topic, ERR CODE:0x%08x\r\n", AT_MQTT_LWT_TOPIC_READ_FAILED);
            goto Error;
        }
        if (strlen(will_topic) > MQTT_TOPIC_MAX_LEN) {
            AT_TRACE("invalid MQTT will_topic, ERR CODE:0x%08x\r\n", AT_MQTT_LWT_TOPIC_IS_OVERLENGTH);
            goto Error;
        }
        will_msg = at_string_parse(argv[5]);
        if (will_msg == NULL) {
            AT_TRACE("invalid MQTT will_msg, ERR CODE:0x%08x\r\n", AT_MQTT_LWT_MSG_READ_FAILED);
            goto Error;
        }
        if (strlen(will_msg) > MQTT_WILL_MSG_MAX_LEN) {
            AT_TRACE("invalid MQTT will_msg, ERR CODE:0x%08x\r\n", AT_MQTT_LWT_MSG_IS_OVERLENGTH);
            goto Error;
        }
        will_qos = (uint8_t)strtoul((const char *)argv[6], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT will_qos, ERR CODE:0x%08x\r\n", AT_MQTT_LWT_QOS_READ_FAILED);
            goto Error;
        }
        if (will_qos > 2) {
            AT_TRACE("invalid MQTT will_qos, ERR CODE:0x%08x\r\n", AT_MQTT_LWT_QOS_VALUE_IS_WRONG);
            goto Error;
        }
        will_retain = (uint8_t)strtoul((const char *)argv[7], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT will_retain, ERR CODE:0x%08x\r\n", AT_MQTT_LWT_RETAIN_READ_FAILED);
            goto Error;
        }
        if (will_retain > 1) {
            AT_TRACE("invalid MQTT will_retain, ERR CODE:0x%08x\r\n", AT_MQTT_LWT_RETAIN_VALUE_IS_WRONG);
            goto Error;
        }
        if (mqtt_client_conn_set(keep_alive, clean_session_disabled, will_topic, will_msg, will_qos, will_retain) == 0) {
            AT_TRACE("MQTT: connection properties set successful\r\n");
        } else {
            AT_TRACE("MQTT: connection properties set failed\r\n");
            goto Error;
        }
    } else {
        AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+MQTTCONNCFG=<LinkID>,<keepalive>,<disable_clean_session>,<\"lwt_topic\">,<\"lwt_msg\">,<lwt_qos>,<lwt_retain>\r\n");
    AT_RSP_OK();
    return;
}

extern cmd_msg_sub_t at_topic_sub_list;
/*!
    \brief      the AT command MQTT connect
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_mqtt_conn(int argc, char **argv)
{
    uint16_t link_id = 0, scheme;
    uint32_t port;
    uint8_t state, reconnect;
    char *endptr = NULL, *host = NULL;

    AT_RSP_START(512);
    scheme = mqtt_scheme_get();
    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            mqtt_client_t *at_mqtt_client = mqtt_client_get();
            if (mqtt_usercfg_setted == false) {
                state = 0;
                AT_RSP("+MQTTCONN:0,%d\r\n", state);
            } else if ((mqtt_usercfg_setted == true) && (at_mqtt_client == NULL)) {
                state = 1;
                AT_RSP("+MQTTCONN:0,%d,%d\r\n", state, scheme);
            } else if ((mqtt_usercfg_setted == true) && (at_mqtt_client != NULL)) {
                host = mqtt_host_get();
                port = mqtt_port_get();
                reconnect = mqtt_reconnect_get();
                if (mqtt_client_is_connected(at_mqtt_client) && !co_list_is_empty(&(at_topic_sub_list.cmd_msg_sub_list))) {
                    state = 6;
                } else if (mqtt_client_is_connected(at_mqtt_client) && co_list_is_empty(&(at_topic_sub_list.cmd_msg_sub_list))) {
                    state = 5;
                } else {
                    state = 3;
                }
                AT_RSP("+MQTTCONN:0,%d,%d,\"%s\",%d,%d\r\n", state, scheme, host, port, reconnect);
            }
        } else {
            AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
            goto Error;
        }
    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
            goto Error;
        }
    } else if (argc == 5) {
        link_id = (uint16_t)strtoul((const char *)argv[1], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_READ_FAILED);
            goto Error;
        }
        if (link_id != 0) {
            AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_VALUE_IS_WRONG);
            goto Error;
        }
        host = at_string_parse(argv[2]);
        if (host == NULL) {
            AT_TRACE("invalid MQTT host, ERR CODE:0x%08x\r\n", AT_MQTT_HOST_READ_FAILED);
            goto Error;
        }
        if (strlen(host) > MQTT_HOST_MAX_LEN) {
            AT_TRACE("invalid MQTT host, ERR CODE:0x%08x\r\n", AT_MQTT_HOST_IS_OVERLENGTH);
            goto Error;
        }
        port = (uint32_t)strtoul((const char *)argv[3], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT port, ERR CODE:0x%08x\r\n", AT_MQTT_PORT_READ_FAILED);
            goto Error;
        }
        if (port > MQTT_MAX_PORT) {
            AT_TRACE("invalid MQTT port, ERR CODE:0x%08x\r\n", AT_MQTT_PORT_VALUE_IS_WRONG);
            goto Error;
        }
        reconnect = (uint8_t)strtoul((const char *)argv[4], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT reconnect, ERR CODE:0x%08x\r\n", AT_MQTT_RECONNECT_READ_FAILED);
            goto Error;
        }
        if (reconnect > 1) {
            AT_TRACE("invalid MQTT reconnect, ERR CODE:0x%08x\r\n", AT_MQTT_RECONNECT_VALUE_IS_WRONG);
            goto Error;
        }
        bool at_ota_enabled = false;
        if (at_mqtt_connect_server(host, port, reconnect, at_ota_enabled) == 0) {
            uint32_t connect_time = sys_current_time_get();
            while (sys_current_time_get() - connect_time <= MQTT_LINK_TIME_LIMIT * 2) {
                mqtt_client_t *at_mqtt_client = mqtt_client_get();
                if (at_mqtt_client != NULL &&  mqtt_client_is_connected(at_mqtt_client)) {
                    mqtt_set_inpub_callback(at_mqtt_client, at_mqtt_receive_pub_msg_print, at_mqtt_receive_msg_print, client_user_info_get());
                    AT_RSP("+MQTTCONNECTED:0,%d,\"%s\",%d,%d\r\n", scheme, host, port, reconnect);
                    break;
                } else if (at_mqtt_client == NULL) {
                    AT_TRACE("MQTT: connect failed\r\n");
                    goto Error;
                }
                sys_ms_sleep(100);
            }
        } else {
            AT_TRACE("MQTT: connect failed\r\n");
            goto Error;
        }
    } else {
        AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+MQTTCONN=<LinkID>,<\"host\">,<port>,<reconnect>\r\n");
    AT_RSP_OK();
    return;
}

void at_mqtt_pub_result_cb(void *arg, err_t status)
{
    AT_RSP_START(64);
    switch (status) {
        case ERR_OK:
            AT_TRACE("MQTT: publish successful\r\n");
            break;
        case ERR_TIMEOUT:
            AT_TRACE("MQTT: publish time out\r\n");
            break;
        default:
            AT_TRACE("MQTT: publish failed\r\n");
            break;
    }

    if (status == ERR_OK) {
        AT_RSP_OK();
    } else {
        AT_RSP_ERR();
    }
    return;
}

void at_mqtt_pub_err_print(publish_msg_t *pub_msg, err_t status)
{
    AT_RSP_START(32);
    switch (status) {
        case ERR_MEM:
            AT_TRACE("MQTT: at_mqtt_pub_err_print malloc fail, ERR CODE:0x%08x", AT_MQTT_MALLOC_FAILED);
            break;
        case ERR_CONN:
            AT_TRACE("MQTT: at_mqtt_pub_err_print in disconnected state, ERR CODE:0x%08x", AT_MQTT_IN_DISCONNECTED_STATE);
            break;
        case ERR_VAL:
            AT_TRACE("MQTT: at_mqtt_pub_err_print in disconnected state, ERR CODE:0x%08x", AT_MQTT_PARAM_PREPARE_ERROR);
            break;
        default:
            AT_TRACE("MQTT: publish failed");
            break;
    }
    if (pub_msg && pub_msg->topic) {
        AT_TRACE(", topic:\"%s\"\r\n", pub_msg->topic);
    } else {
        AT_TRACE("\r\n");
    }
    AT_RSP_ERR();
    return;
}

/*!
    \brief      the AT command publish message
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_mqtt_pub(int argc, char **argv)
{
    uint16_t link_id;
    uint8_t qos = 0, retain;
    char *endptr = NULL, *topic = NULL, *data = NULL;

    AT_RSP_START(512);
    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
            goto Error;
        }
    } else if (argc == 6) {
        link_id = (uint16_t)strtoul((const char *)argv[1], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_READ_FAILED);
            goto Error;
        }
        if (link_id != 0) {
            AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_VALUE_IS_WRONG);
            goto Error;
        }
        topic = at_string_parse(argv[2]);
        if (topic == NULL) {
            AT_TRACE("invalid MQTT topic, ERR CODE:0x%08x\r\n", AT_MQTT_TOPIC_READ_FAILED);
            goto Error;
        }
        if (strlen(topic) > MQTT_TOPIC_MAX_LEN) {
            AT_TRACE("invalid MQTT topic, ERR CODE:0x%08x\r\n", AT_MQTT_TOPIC_IS_OVERLENGTH);
            goto Error;
        }
        data = at_string_parse(argv[3]);
        if (data == NULL) {
            AT_TRACE("invalid MQTT data, ERR CODE:0x%08x\r\n", AT_MQTT_DATA_READ_FAILED);
            goto Error;
        }
        qos = (uint8_t)strtoul((const char *)argv[4], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT qos, ERR CODE:0x%08x\r\n", AT_MQTT_QOS_READ_FAILED);
            goto Error;
        }
        if (qos > 2) {
            AT_TRACE("invalid MQTT qos, ERR CODE:0x%08x\r\n", AT_MQTT_QOS_VALUE_IS_WRONG);
            goto Error;
        }
        retain = (uint8_t)strtoul((const char *)argv[5], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT retain, ERR CODE:0x%08x\r\n", AT_MQTT_RETAIN_READ_FAILED);
            goto Error;
        }
        if (retain > 1) {
            AT_TRACE("invalid MQTT retain, ERR CODE:0x%08x\r\n", AT_MQTT_RETAIN_VALUE_IS_WRONG);
            goto Error;
        }
        int res = at_mqtt_msg_pub(topic, data, strlen(data), qos, retain);
        if (res != 0) {
            AT_TRACE("MQTT: publish failed, ERR CODE:0x%08x\r\n", AT_MQTT_CLIENT_PUBLISH_FAILED);
            goto Error;
        } else {
            AT_TRACE("MQTT: waiting for publish result callback\r\n");
        }
    } else {
        AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
        goto Error;
    }

    AT_RSP_FREE();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+MQTTPUB=<LinkID>,<\"topic\">,<\"data\">,<qos>,<retain>\r\n");
    AT_RSP_OK();
    return;
}

/*!
    \brief      the AT command publish raw message
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_mqtt_pub_raw(int argc, char **argv)
{
    uint16_t link_id;
    uint8_t qos = 0, retain;
    char *endptr = NULL, *topic = NULL, *data = NULL;
    uint16_t length;

    AT_RSP_START(256);
    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
            goto Error;
        }
    } else if (argc == 6) {
        link_id = (uint16_t)strtoul((const char *)argv[1], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_READ_FAILED);
            goto Error;
        }
        if (link_id != 0) {
            AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_VALUE_IS_WRONG);
            goto Error;
        }
        topic = at_string_parse(argv[2]);
        if (topic == NULL) {
            AT_TRACE("invalid MQTT topic, ERR CODE:0x%08x\r\n", AT_MQTT_TOPIC_READ_FAILED);
            goto Error;
        }
        if (strlen(topic) > MQTT_TOPIC_MAX_LEN) {
            AT_TRACE("invalid MQTT topic, ERR CODE:0x%08x\r\n", AT_MQTT_TOPIC_IS_OVERLENGTH);
            goto Error;
        }
        length = (uint16_t)strtoul((const char *)argv[3], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT length\r\n");
            goto Error;
        }
        qos = (uint8_t)strtoul((const char *)argv[4], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT qos, ERR CODE:0x%08x\r\n", AT_MQTT_QOS_READ_FAILED);
            goto Error;
        }
        if (qos > 2) {
            AT_TRACE("invalid MQTT qos, ERR CODE:0x%08x\r\n", AT_MQTT_QOS_VALUE_IS_WRONG);
            goto Error;
        }
        retain = (uint8_t)strtoul((const char *)argv[5], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT retain, ERR CODE:0x%08x\r\n", AT_MQTT_RETAIN_READ_FAILED);
            goto Error;
        }
        if (retain > 1) {
            AT_TRACE("invalid MQTT retain, ERR CODE:0x%08x\r\n", AT_MQTT_RETAIN_VALUE_IS_WRONG);
            goto Error;
        }

        data = sys_malloc(length + 1);
        if (NULL == data) {
            AT_TRACE("Allocate data buffer failed, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
            goto Error;
        }

        AT_RSP_DIRECT("OK\r\n", 4);
        AT_RSP_DIRECT(">\r\n", 3);
        // Block here to wait dma receive done
        at_hw_dma_receive((uint32_t)data, length);
        data[length] = '\0';

        if (at_mqtt_msg_pub(topic, data, length, qos, retain) == 0) {
            sys_mfree(data);
            AT_TRACE("MQTT: waiting for publish result callback\r\n");
            AT_RSP("+MQTTPUB:");
        } else {
            AT_TRACE("MQTT: publish failed, ERR CODE:0x%08x\r\n", AT_MQTT_CLIENT_PUBLISH_FAILED);
            goto Fail;
        }
    } else {
        AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Fail:
    if (data) {
        sys_mfree(data);
    }
    AT_RSP("+MQTTPUB:FAIL\r\n");
    AT_RSP_IMMEDIATE();
    AT_RSP_FREE();
    return;
Usage:
    AT_RSP("+MQTTPUBRAW=<LinkID>,<\"topic\">,<length>,<qos>,<retain>\r\n");
    AT_RSP_OK();
    return;
}

void at_mqtt_sub_result_cb(void *arg, err_t status)
{
    sub_msg_t *sub_msg = (sub_msg_t *)arg;
    sub_msg_t *sub_topic = NULL;

    AT_RSP_START(64);
    if (status == ERR_OK) {
        AT_TRACE("massage subscribe success\r\n");
        if (sub_msg) {
            sub_topic = sys_calloc(1, sizeof(sub_msg_t));
            if (sub_topic == NULL) {
                AT_TRACE("MQTT at_mqtt_sub_result_cb: rtos malloc fail, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
                goto Error;
            }
            sub_topic->topic = (char*)sys_malloc(strlen(sub_msg->topic) + 1);
            if (sub_topic->topic == NULL) {
                AT_TRACE("MQTT at_mqtt_sub_result_cb: rtos malloc fail, ERR CODE:0x%08x\r\n", AT_MQTT_MALLOC_FAILED);
                sys_mfree(sub_topic);
                goto Error;
            }
            memcpy(sub_topic->topic, sub_msg->topic, strlen(sub_msg->topic) + 1);
            sub_topic->qos = (uint8_t)sub_msg->qos;
            sys_sched_lock();
            co_list_push_back(&(at_topic_sub_list.cmd_msg_sub_list), &(sub_topic->hdr));
            sys_sched_unlock();
            sys_mfree(sub_msg->topic);
            sys_mfree(sub_msg);
            sub_msg = NULL;
        }
        AT_RSP_OK();
        return;
    } else if (status == ERR_TIMEOUT) {
        AT_TRACE("massage subscribe time out\r\n");
    }
Error:
    if (sub_msg) {
        if (sub_msg->topic) {
            sys_mfree(sub_msg->topic);
        }
        sys_mfree(sub_msg);
        sub_msg = NULL;
    }
    AT_RSP_ERR();
    return;
}

void at_mqtt_unsub_result_cb(void *arg, err_t status)
{
    sub_msg_t *sub_msg = (sub_msg_t *)arg;
    sub_msg_t *sub_topic = NULL;

    AT_RSP_START(64);
    if (status == ERR_OK) {
        AT_TRACE("massage unsubscribe success\r\n");
        if (sub_msg) {
            struct co_list_hdr *curr = at_topic_sub_list.cmd_msg_sub_list.first;
            while (curr) {
                sub_topic = CONTAINER_OF(curr, sub_msg_t, hdr);
                struct co_list_hdr *next = curr->next;
                if (sub_topic && strcmp(sub_topic->topic, sub_msg->topic) == 0) {
                    sys_sched_lock();
                    co_list_extract(&(at_topic_sub_list.cmd_msg_sub_list), curr);
                    sys_sched_unlock();
                    curr = NULL;
                    sys_mfree(sub_topic->topic);
                    sys_mfree(sub_topic);
                    sub_topic = NULL;
                    sys_mfree(sub_msg->topic);
                    sys_mfree(sub_msg);
                    sub_msg = NULL;
                    break;
                }
                curr = next;
            }
        }
        AT_RSP_OK();
        return;
    } else if (status == ERR_TIMEOUT) {
        AT_TRACE("massage unsubscribe time out\r\n");
    }
    if (sub_msg) {
        if (sub_msg->topic) {
            sys_mfree(sub_msg->topic);
        }
        sys_mfree(sub_msg);
        sub_msg = NULL;
    }
    AT_RSP_ERR();
    return;
}

void at_mqtt_sub_or_unsub_err_print(sub_msg_t *sub_msg, err_t status)
{
    AT_RSP_START(32);
    switch (status) {
        case ERR_MEM:
            AT_TRACE("MQTT: at_mqtt_sub_or_unsub_err_print malloc fail, ERR CODE:0x%08x", AT_MQTT_MALLOC_FAILED);
            break;
        case ERR_CONN:
            AT_TRACE("MQTT: at_mqtt_sub_or_unsub_err_print in disconnected state, ERR CODE:0x%08x", AT_MQTT_IN_DISCONNECTED_STATE);
            break;
        default:
            AT_TRACE("MQTT: sub_or_unsub failed");
            break;
    }
    if (sub_msg && sub_msg->topic) {
        AT_TRACE(", topic:\"%s\"\r\n", sub_msg->topic);
    } else {
        AT_TRACE("\r\n");
    }
    AT_RSP_ERR();
    return;
}

/*!
    \brief      the AT command subscribe message
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_mqtt_sub(int argc, char **argv)
{
    uint16_t link_id = 0;
    uint8_t qos = 0, state;
    char *endptr = NULL, *topic = NULL;

    AT_RSP_START(512);
    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            mqtt_client_t *at_mqtt_client = mqtt_client_get();
            if (at_mqtt_client != NULL && !co_list_is_empty(&(at_topic_sub_list.cmd_msg_sub_list))) {
                if (mqtt_client_is_connected(at_mqtt_client)) {
                    state = 6;
                } else {
                    state = 3;
                }
                sys_sched_lock();
                struct co_list_hdr *curr = at_topic_sub_list.cmd_msg_sub_list.first;
                while (curr) {
                    sub_msg_t *sub_topic = CONTAINER_OF(curr, sub_msg_t, hdr);
                    AT_RSP("+MQTTSUB:%d,%d,\"%s\",%d\r\n", link_id, state, sub_topic->topic, sub_topic->qos);
                    curr = curr->next;
                }
                sys_sched_unlock();
            }
            AT_RSP("OK\r\n");
            AT_RSP_IMMEDIATE();
        } else {
            AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
            goto Error;
        }
    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
            goto Error;
        }
    } else if (argc == 4) {
        link_id = (uint16_t)strtoul((const char *)argv[1], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_READ_FAILED);
            goto Error;
        }
        if (link_id != 0) {
            AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_VALUE_IS_WRONG);
            goto Error;
        }
        topic = at_string_parse(argv[2]);
        if (topic == NULL) {
            AT_TRACE("invalid MQTT topic, ERR CODE:0x%08x\r\n", AT_MQTT_TOPIC_READ_FAILED);
            goto Error;
        }
        if (strlen(topic) > MQTT_TOPIC_MAX_LEN) {
            AT_TRACE("invalid MQTT topic, ERR CODE:0x%08x\r\n", AT_MQTT_TOPIC_IS_OVERLENGTH);
            goto Error;
        }
        qos = (uint8_t)strtoul((const char *)argv[3], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT qos, ERR CODE:0x%08x\r\n", AT_MQTT_QOS_READ_FAILED);
            goto Error;
        }
        if (qos > 2) {
            AT_TRACE("invalid MQTT qos, ERR CODE:0x%08x\r\n", AT_MQTT_QOS_VALUE_IS_WRONG);
            goto Error;
        }
        int res = at_mqtt_msg_sub(topic, qos, true);
        if (res == -2) {
            AT_RSP("ALREADY SUBSCRIBE\r\n");
            AT_RSP("OK\r\n");
            AT_RSP_IMMEDIATE();
        } else if (res == -1) {
            AT_TRACE("MQTT: subscribe failed, ERR CODE:0x%08x\r\n", AT_MQTT_CLIENT_SUBSCRIBE_FAILED);
            goto Error;
        } else if (res == 0) {
            AT_TRACE("MQTT: waiting for subscribe result callback\r\n");
        }
    } else {
        AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
        goto Error;
    }

    AT_RSP_FREE();
    return;

Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+MQTTSUB=<LinkID>,<\"topic\">,<qos>\r\n");
    AT_RSP_OK();
    return;
}

/*!
    \brief      the AT command unsubscribe message
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_mqtt_unsub(int argc, char **argv)
{
    uint16_t link_id = 0;
    char *endptr = NULL, *topic = NULL;

    AT_RSP_START(256);
    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
            goto Error;
        }
    } else if (argc == 3) {
        link_id = (uint16_t)strtoul((const char *)argv[1], &endptr, 10);
        if ((*endptr != '\0')) {
            AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_READ_FAILED);
            goto Error;
        }
        if (link_id != 0) {
            AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_VALUE_IS_WRONG);
            goto Error;
        }
        topic = at_string_parse(argv[2]);
        if (topic == NULL) {
            AT_TRACE("invalid MQTT topic, ERR CODE:0x%08x\r\n", AT_MQTT_TOPIC_READ_FAILED);
            goto Error;
        }
        if (strlen(topic) > MQTT_TOPIC_MAX_LEN) {
            AT_TRACE("invalid MQTT topic, ERR CODE:0x%08x\r\n", AT_MQTT_TOPIC_IS_OVERLENGTH);
            goto Error;
        }
        int res = at_mqtt_msg_sub(topic, 1, false);
        if (res == -2) {
            AT_RSP("NO UNSUBSCRIBE\r\n");
            AT_RSP("OK\r\n");
            AT_RSP_IMMEDIATE();
        } else if (res == -1) {
            AT_TRACE("MQTT: unsubscribe failed, ERR CODE:0x%08x\r\n", AT_MQTT_CLIENT_UNSUBSCRIBE_FAILED);
            goto Error;
        } else if (res == 0) {
            AT_TRACE("MQTT: waiting for unsubscribe result callback\r\n");
        }
    } else {
        AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
        goto Error;
    }

    AT_RSP_FREE();
    return;

Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+MQTTUNSUB=<LinkID>,<\"topic\">\r\n");
    AT_RSP_OK();
    return;
}

void at_mqtt_disconn_print(void)
{
    AT_RSP_START(64);
    AT_RSP("+MQTTDISCONNECTED:0\r\n");
    AT_RSP_IMMEDIATE();
    AT_RSP_FREE();
    return;
}

/*!
    \brief      the AT command clean MQTT
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_mqtt_clean(int argc, char **argv)
{
    uint16_t link_id = 0;
    char *endptr = NULL;

    AT_RSP_START(128);
    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            link_id = (uint16_t)strtoul((const char *)argv[1], &endptr, 10);
            if ((*endptr != '\0')) {
                AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_READ_FAILED);
                goto Error;
            }
            if (link_id != 0) {
                AT_TRACE("invalid MQTT link_id, ERR CODE:0x%08x\r\n", AT_MQTT_LINK_ID_VALUE_IS_WRONG);
                goto Error;
            }
            mqtt_client_t *mqtt_client = mqtt_client_get();
            if (mqtt_client == NULL || mqtt_client->run == false) {
                AT_TRACE("MQTT client is not running, ERR CODE:0x%08x\r\n", AT_MQTT_UNINITIATED_OR_ALREADY_CLEAN);
                goto Error;
            }
            mqtt_client_disconnect(0, NULL);
            mqtt_usercfg_setted = false;
        }
    } else {
        AT_TRACE("MQTT: wrong parameter counts, ERR CODE:0x%08x\r\n", AT_MQTT_PARAMETER_COUNTS_IS_WRONG);
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+MQTTCLEAN=<LinkID>\r\n");
    AT_RSP_OK();
    return;
}
#endif