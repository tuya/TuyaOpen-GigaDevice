/*!
    \file    atcmd_azure.h
    \brief   AT command AZURE part for GD32VW55x SDK

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

#ifndef _ATCMD_AZURE_H_
#define _ATCMD_AZURE_H_

#include <app_cfg.h>
#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
#ifndef AES_KEY_SZ
    #define AES_KEY_SZ              16
#endif
#ifndef AES_BLOCK_SZ
    #define AES_BLOCK_SZ            16
#endif
#define HAU_DMA_BLOCK_SIZE          32768  /* MAX: 65535 bytes, preferably 64-byte aligned */

#define WIFI_CONNECTED_LED_GPIO     GPIOA
#define WIFI_CONNECTED_LED_PIN      GPIO_PIN_2

#define AZURE_CONNECTED_LED_GPIO    GPIOA
#define AZURE_CONNECTED_LED_PIN     GPIO_PIN_3

enum wifi_conn_rsp_t {
    WIFI_CONN_OK = 0,
    WIFI_CONN_UNSPECIFIED,
    WIFI_CONN_NO_AP,
    WIFI_CONN_AUTH_FAIL,
    WIFI_CONN_ASSOC_FAIL,
    WIFI_CONN_HANDSHAKE_FAIL,
    WIFI_CONN_DHCP_FAIL,
};

enum azure_conn_rsp_t {
    AZURE_CONN_OK = 0,
    AZURE_CONN_INTERNET_FAIL, // Connect to Internet Fail
    AZURE_CONN_CERT_FAIL,
    AZURE_CONN_SYMKEY_FAIL,
    AZURE_CONN_OTHER_FAIL,
};

enum azure_state_t {
    AZURE_STATE_IDLE = 0,
    AZURE_STATE_WIFI_CONNECTING,
    AZURE_STATE_WIFI_CONNECTED,
    AZURE_STATE_HUB_CONNECTING,
    AZURE_STATE_HUB_CONNECTED,
};

enum {
    AT_ERR = -1,
    AT_OK = 0,
    AT_RSP_ERR = 1,
    AT_RSP_TIMEOUT = 2,
};

void azure_led_init();
void wifi_connected_led(int state);
void azure_connected_led(int state);
int atcmd_wifi_conn_rsp(enum wifi_conn_rsp_t result);
int atcmd_azure_conn_rsp(enum azure_conn_rsp_t result);
int atcmd_azure_dev_set(char *dev_type, char *dev_val);
int atcmd_azure_prop_req(char *topic, uint32_t topic_len,
                                 char *payload, uint32_t payload_len);
int atcmd_azure_cmd_req(char *topic, uint32_t topic_len,
                                 char *payload, uint32_t payload_len);
int atcmd_azure_c2dmsg_send(char *topic, uint32_t topic_len,
                                 char *payload, uint32_t payload_len);
int atcmd_azure_ota_ind_send(char *ver, uint32_t fw_len);
int atcmd_azure_ota_result_send(char *ver, uint32_t result);

int atcmd_azure_ota_block_send(char *buf, uint32_t len);
int atcmd_azure_ota_hash_send(char *buf, uint32_t len);
void hash_sha256_dma(uint8_t *input, uint32_t len, uint8_t *hash_result);
int atcmd_azure_ota_hash_recv(char *hash, uint32_t hash_len);
#endif
#endif /* _ATCMD_AZURE_H_ */
