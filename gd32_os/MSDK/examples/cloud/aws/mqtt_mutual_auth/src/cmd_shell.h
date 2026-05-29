/*!
    \file    cmd_shell.h
    \brief   Header file of command shell for GD32VW55x SDK.

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

#ifndef _CMD_SHELL_H_
#define _CMD_SHELL_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "co_int.h"
#include "co_math.h"
#include "wrapper_os.h"
#include "dbg_print.h"

/*
 * DEFINITIONS
 ****************************************************************************************
 */
#define CUSTOM_IMG_VERSION   (((RE_IMG_VERSION & 0xFF) << 8) | 0x00)

// Generate cmd msg ID from a type and an index
#define CMD_MSG_ID(type, idx) ((type << 12) | (idx & 0xfff))
// Extract msg Type from msg ID
#define CMD_MSG_TYPE(id) ((id >> 12) & 0xf)
// Extract msg Index from msg ID
#define CMD_MSG_INDEX(id) (id & 0xfff)

typedef void (*cmd_handle_cb) (int, char **);
typedef uint8_t (*cmd_module_get_handle_cb)(void *, void **);
typedef void (*cmd_module_help_cb)(void);
typedef int (*cmd_parse_cb)(char *, char **);
typedef void (*cmd_unkwn_handle_cb)(uint16_t data_len, uint8_t *p_data);

struct cmd_entry {
    char *command;
    void (*function)(int, char **);
};

// Generic Message format
struct cmd_module_reg_info {
    // command prefix.
    char *prefix;
    // command get callback handle function.
    cmd_module_get_handle_cb get_handle_cb;
    // help handle function.
    cmd_module_help_cb help_cb;
    // parse command function.
    cmd_parse_cb parse_cb;
};

enum cmd_module_id {
    CMD_MODULE_WIFI,
    CMD_MODULE_BLE,
    CMD_MODULE_COMMON,
    CMD_MODULE_ATCMD,
    CMD_MODULE_MAX
};

enum cmd_mode_type {
    CMD_MODE_TYPE_NORMAL,
    //CMD_MODE_TYPE_AT,
};

// cmd Message format
struct cmd_msg {
    // ID of the message. Id is a combination of a type and an index.
    uint16_t id;
    // Length, in bytes, of the message
    uint16_t len;
    // Pointer to the message
    void *data;
};

// CLI Command result
enum cli_res {
    CLI_SUCCESS,
    CLI_ERROR,
    CLI_UNKWN_CMD,
    CLI_NO_RESP,
    CLI_SHOW_USAGE,
};

// Priorities of tasks created by the CLI application
enum {
    // Priority of the CLI task
    CLI_PRIORITY = OS_TASK_PRIORITY(4),
#ifdef CONFIG_ATCMD
    ATCMD_PRIORITY = OS_TASK_PRIORITY(4),
#endif
    WIFI_PKT_TX_PRIORITY = OS_TASK_PRIORITY(2),
#ifdef CONFIG_IPERF_TEST
    IPERF_TASK_PRIO = OS_TASK_PRIORITY(2),
#endif /* CONFIG_IPERF_TEST */
#ifdef CONFIG_IPERF3_TEST
    IPERF3_TASK_PRIO = OS_TASK_PRIORITY(2),
#endif /* CONFIG_IPERF3_TEST */
#ifdef CONFIG_MQTT
    MQTT_TASK_PRIO = OS_TASK_PRIORITY(1),
#endif /* CONFIG_MQTT */
#ifdef CONFIG_COAP
    COAP_CLIENT_TASK_PRIO = OS_TASK_PRIORITY(1),
    COAP_SERVER_TASK_PRIO = OS_TASK_PRIORITY(1),
#endif /* CONFIG_COAP */
};

// Stack size of tasks created by the CLI application
enum {
    // CLI task stack size
    CLI_STACK_SIZE = 400, //512, // 768, for compiler op level -o0
#ifdef CONFIG_ATCMD
    ATCMD_STACK_SIZE = 512,
#endif
    WIFI_PKT_TX_STACK_SIZE = 512,
#ifdef CONFIG_IPERF_TEST
    IPERF_STACK_SIZE = 512,
#endif /* CONFIG_IPERF_TEST */
#ifdef CONFIG_IPERF3_TEST
    IPERF3_STACK_SIZE = 512,
#endif /* CONFIG_IPERF3_TEST */
#ifdef CONFIG_MQTT
    MQTT_TASK_STACK_SIZE = 512,
#endif /* CONFIG_MQTT */
#ifdef CONFIG_COAP
    COAP_CLIENT_TASK_STACK_SIZE = 768,
    COAP_SERVER_TASK_STACK_SIZE = 384,
#endif /* CONFIG_COAP */
};

/**
 ****************************************************************************************
 * @brief Initialization of the application.
 *
 * Called during the initialization procedure (i.e. when RTOS scheduler is not yet
 * active).
 *
 * Implementation of this function will depends of the final application and in most
 * cases it will create one of several application tasks and their required communication
 * interface (queue, semaphore, ...)
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
int cmd_shell_init(void);

/**
 ****************************************************************************************
 * @brief command module register.
 *
 * @param[in] id             command module id. see#enum cmd_module_id.
 * @param[in] prefix         command prefix match rule.
 * @param[in] get_handle_cb  get the command handle callback.
 * @param[in] help_cb        help handle callback.
 * @param[in] parse_cb       prase handle callback.
 *
 * @param[out] 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
uint8_t cmd_module_reg(enum cmd_module_id id, char *prefix, cmd_module_get_handle_cb get_handle_cb,
    cmd_module_help_cb help_cb, cmd_parse_cb parse_cb);

/**
 ****************************************************************************************
 * @brief command module type set.
 *
 * @param[in] cmd_mode             command module need to set.
 ****************************************************************************************
 */
void cmd_mode_type_set(enum cmd_mode_type cmd_mode);

/**
 ****************************************************************************************
 * @brief command module type to get.
 *
 * @return type of command module.
 ****************************************************************************************
 */
enum cmd_mode_type cmd_mode_type_get(void);

/**
 ****************************************************************************************
 * @brief unkonw command handler register.
 *
 * @param[in] cb             unkonw command handler callback.
 ****************************************************************************************
 */
void cmd_unkwn_cmd_handler_reg(cmd_unkwn_handle_cb cb);

/**
 ****************************************************************************************
 * @brief unkonw command handler unregister.
 ****************************************************************************************
 */
void cmd_unkwn_cmd_handler_unreg(void);

int cli_parse_ip4(char *str, uint32_t *ip, uint32_t *mask);

#endif /* _CMD_SHELL_H_ */
