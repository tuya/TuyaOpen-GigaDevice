/*!
    \file    atcmd_mqtt.h
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

#ifndef _ATCMD_MQTT_H_
#define _ATCMD_MQTT_H_

#define MQTT_CLIENT_ID_LEN        256
#define MQTT_CLIENT_ID_MAX_LEN    1024
#define MQTT_USERNAME_LEN         64
#define MQTT_USERNAME_MAX_LEN     1024
#define MQTT_PASSWORD_LEN         64
#define MQTT_PASSWORD_MAX_LEN     1024
#define MQTT_MAX_PING_TIMEOUT     7200 //s
#define MQTT_DEFAULT_PING_TIMEOUT 120  //s
#define MQTT_TOPIC_MAX_LEN        128
#define MQTT_WILL_MSG_MAX_LEN     128
#define MQTT_HOST_MAX_LEN         128
#define MQTT_MAX_PORT             65535

void at_mqtt_user_cfg(int argc, char **argv);
void at_mqtt_set_client_id(int argc, char **argv);
void at_mqtt_set_username(int argc, char **argv);
void at_mqtt_set_password(int argc, char **argv);
void at_mqtt_conn_cfg(int argc, char **argv);
void at_mqtt_conn(int argc, char **argv);
void at_mqtt_pub(int argc, char **argv);
void at_mqtt_pub_raw(int argc, char **argv);
void at_mqtt_sub(int argc, char **argv);
void at_mqtt_unsub(int argc, char **argv);
void at_mqtt_clean(int argc, char **argv);

#endif /* _ATCMD_MQTT_H_ */