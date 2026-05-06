/*!
    \file    cmd_coap.c
    \brief   COAP demo for GD32VW55x SDK.

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

#include "client-coap.h"
#include "server-coap.h"

static os_task_t coap_client_task_hdl = NULL;
static os_task_t coap_server_task_hdl = NULL;

static void coap_client_task_func(void *param)
{
    int no_more = 0;
    struct coap_client_config *coap_cmd = (struct coap_client_config *)param;

    app_print("Client Application started.\n");

    client_coap_init(coap_cmd);

    while (!no_more) {
        no_more = client_coap_poll();
    }

    client_coap_finished();

    app_print("Client Application finished.\r\n");
    coap_client_task_hdl = NULL;
    sys_task_delete(NULL);
}

static void cmd_coap_client(int argc, char **argv)
{
    int arg_cnt = 1, log_level;
    struct coap_client_config *coap_client_cfg;

    if (argc == 1) {
        goto Usage;
    }

    if (coap_client_task_hdl != NULL || coap_server_task_hdl != NULL) {
        app_print("coap client or server task has been running, please stop it before start a new one\r\n");
        return;
    }

    coap_client_cfg = (struct coap_client_config *)sys_malloc(sizeof(struct coap_client_config));
    if (coap_client_cfg == NULL) {
        app_print("coap client malloc config fail.\r\n");
        return;
    }

    // set default parameters
    coap_client_cfg->log_level = COAP_LOG_INFO;
    coap_client_cfg->pdu_type = COAP_MESSAGE_CON;
    coap_client_cfg->pdu_code = COAP_REQUEST_CODE_GET;
    coap_client_cfg->use_uri = NULL;
    coap_client_cfg->put_data = NULL;

    while (arg_cnt < argc) {
        if (strcmp(argv[arg_cnt], "-m") == 0) {
            if (argc <= (arg_cnt + 1)) {
                goto Exit;
            }
            if (strcmp(argv[arg_cnt + 1], "get") == 0) {
                coap_client_cfg->pdu_code = COAP_REQUEST_CODE_GET;
            } else if (strcmp(argv[arg_cnt + 1], "put") == 0) {
                coap_client_cfg->pdu_code = COAP_REQUEST_CODE_PUT;
            } else {
                goto Exit;
            }
            arg_cnt += 2;
        } else if (strcmp(argv[arg_cnt], "-v") == 0) {
            if (argc <= (arg_cnt + 1)) {
                goto Exit;
            }
            log_level = (uint32_t)atoi(argv[arg_cnt + 1]);
            if (log_level > COAP_LOG_DTLS_BASE) {
                coap_client_cfg->log_level = COAP_LOG_DTLS_BASE;
            } else {
                coap_client_cfg->log_level = (coap_log_t)log_level;
            }
            arg_cnt += 2;
        } else if (strcmp(argv[arg_cnt], "-N") == 0) {
            coap_client_cfg->pdu_type = COAP_MESSAGE_NON;
            arg_cnt ++;
        } else if (strncmp(argv[arg_cnt], "coap", 4) == 0) {
            int uri_len = strlen(argv[arg_cnt]);
            coap_client_cfg->use_uri = sys_malloc(uri_len + 1);
            if (coap_client_cfg->use_uri != NULL) {
                sys_memcpy(coap_client_cfg->use_uri, argv[arg_cnt], uri_len);
                coap_client_cfg->use_uri[uri_len] = '\0';
            } else {
                app_print("coap client malloc uri fail.\r\n");
                goto Exit;
            }
            arg_cnt++;

            if ((coap_client_cfg->pdu_code == COAP_REQUEST_CODE_PUT) &&
                (arg_cnt < argc)) {
                int data_len = strlen(argv[arg_cnt]);
                coap_client_cfg->put_data = sys_malloc(data_len + 1);
                if (coap_client_cfg->put_data != NULL) {
                    sys_memcpy(coap_client_cfg->put_data, argv[arg_cnt], data_len);
                    coap_client_cfg->put_data[data_len] = '\0';
                } else {
                    app_print("coap client malloc data fail.\r\n");
                    goto Exit;
                }
                arg_cnt++;
            }
        } else {
            arg_cnt++;
        }
    }

    coap_client_task_hdl = sys_task_create_dynamic((const uint8_t *)"coap_client",
                                                    COAP_CLIENT_TASK_STACK_SIZE , COAP_CLIENT_TASK_PRIO,
                                                    coap_client_task_func, coap_client_cfg);
    if (coap_client_task_hdl == NULL) {
        app_print("ERROR: Create coap client task failed\r\n");
        goto Exit;
    }

    return;

Exit:
    if (coap_client_cfg->use_uri != NULL)
        sys_mfree(coap_client_cfg->use_uri);
    if (coap_client_cfg->put_data != NULL)
        sys_mfree(coap_client_cfg->put_data);
    if (coap_client_cfg)
        sys_mfree(coap_client_cfg);

    app_print("coap_client cmd format error!\r\n");
Usage:
    app_print("Usage: coap_client [-m get|put] [-v log_level] [-N] <URI> [data]\r\n");
    app_print("Example:\r\n");
    app_print("     : coap_client -m get -v 7 coap://californium.eclipseprojects.io\r\n");
    app_print("     : coap_client -m put coap://californium.eclipseprojects.io/test 12345678\r\n");
    return;
}

uint8_t coap_server_terminate;
static void coap_server_task_func(void *param)
{
    int running = 1;

    app_print("Server Application started.\n");

    server_coap_init();

    while (running && !coap_server_terminate) {
        running = server_coap_poll();
    }

    server_coap_finished();

    app_print("Server Application finished.\r\n");
    coap_server_task_hdl = NULL;
    sys_task_delete(NULL);
}

static void cmd_coap_server(int argc, char **argv)
{
    if (argc == 2) {
        if (strcmp(argv[1], "stop") == 0) {
            coap_server_terminate = 1;
            return;
        } else {
            goto Usage;
        }
    } else if (argc != 1) {
        goto Usage;
    }

    if (coap_client_task_hdl != NULL || coap_server_task_hdl != NULL) {
        app_print("coap client or server task has been running, please stop it before start a new one\r\n");
        return;
    }

    coap_server_terminate = 0;

    coap_server_task_hdl = sys_task_create_dynamic((const uint8_t *)"coap_server",
                                                    COAP_SERVER_TASK_STACK_SIZE, COAP_SERVER_TASK_PRIO,
                                                    coap_server_task_func, NULL);
    if (coap_server_task_hdl == NULL) {
        app_print("ERROR: Create coap server task failed\r\n");
        goto Exit;
    }
    return;

Exit:
    app_print("coap_server cmd format error!\r\n");
Usage:
    app_print("Usage: coap_server [stop]\r\n");
    app_print("Example:\r\n");
    app_print("     : coap_server\r\n");
    app_print("     : coap_server stop\r\n");
    return;
}
