/*!
    \file    cmd_shell.c
    \brief   Command shell for GD32VW55x SDK.

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

#include "config_gdm32.h"
#include "app_cfg.h"
#include "wlan_config.h"
#include "build_config.h"
#include "gd32vw55x.h"
#include "lwip/igmp.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "wifi_vif.h"
#include "wifi_net_ip.h"
#include "wifi_management.h"
#include "wifi_export.h"
#include "wifi_init.h"
#include "cmd_shell.h"
#include "dbg_print.h"
#include "uart.h"
#include "uart_config.h"
#include "version.h"
#include "log_uart.h"
#include "wakelock.h"
#include "trace_uart.h"


#if NVDS_FLASH_SUPPORT
#include "nvds_flash.h"
#endif


// CLI task message queue size
#define CLI_QUEUE_SIZE 3

struct cmd_module_info {
    enum cmd_mode_type cmd_mode;
    struct cmd_module_reg_info cmd_reg_infos[CMD_MODULE_MAX];
};

static cyclic_buf_t uart_cyc_buf;
static char uart_buf[UART_BUFFER_SIZE];
static uint32_t uart_index = 0;
static os_queue_t cmd_queue;
static struct cmd_module_info cmd_info;
static const struct cmd_entry cmd_table[];
static cmd_unkwn_handle_cb unkwn_cmd_handler = NULL;

int cmd_info_send(int id, void *msg_data, uint16_t len);
int cli_parse_ip4(char *str, uint32_t *ip, uint32_t *mask);

#ifdef CONFIG_IPERF_TEST
extern void cmd_iperf(int argc, char **argv);
#endif /* CONFIG_IPERF_TEST */

#ifdef CONFIG_IPERF3_TEST
extern void cmd_iperf3(int argc, char **argv);
#endif

#ifdef CONFIG_SSL_TEST
extern void cmd_ssl_client(int argc, char **argv);
#endif

static void uart_cmd_rx_indicate(void)
{
    if (cmd_info_send(0x23, (void *)(&uart_cyc_buf), uart_index + 1) == 0) {
        if (cyclic_buf_write(&uart_cyc_buf, (uint8_t *)uart_buf, uart_index + 1) == false) {
            dbg_print(ERR, "uart cyclic buffer full\r\n");
        }
    } else {
        /* queue was full */
        dbg_print(ERR, "queue full\r\n");
        /* TODO: report 'message ignored' status */
    }

    uart_index = 0;
}

#undef isprint
#define in_range(c, lo, up)  ((uint8_t)c >= lo && (uint8_t)c <= up)
#define isprint(c)           in_range(c, 0x20, 0xff)
static void log_uart_rx_irq_hdl(uint32_t uart_port)
{
    uint8_t ch;
    usart_interrupt_disable(uart_port, USART_INT_RBNE);
    while (1) {
        // We should have chance to check overflow error
        // Otherwise it may cause dead loop handle rx interrupt
        if (RESET != usart_flag_get(uart_port, USART_FLAG_ORERR)) {
            usart_flag_clear(uart_port, USART_FLAG_ORERR);
        }

        if ((RESET != usart_flag_get(uart_port, USART_FLAG_RBNE))) {
            ch = usart_data_receive(uart_port);
        } else {
            break;
        }

        if (ch == '\0') {
            break;
        }

        if (isprint(ch)) {
            uart_buf[uart_index++] = ch;
            if (uart_index >= UART_BUFFER_SIZE) {
                uart_index = 0;
            }
            log_uart_putc_noint(ch);
        } else if (ch == '\r') { /* putty doesn't transmit '\n' */
            uart_buf[uart_index] = '\0';

            log_uart_putc_noint('\r');
            log_uart_putc_noint('\n');

            if (uart_index > 0) {
                uart_cmd_rx_indicate();
            } else {
                log_uart_putc_noint('#');
                log_uart_putc_noint(' ');
            }
            sys_wakelock_release(LOCK_ID_USART);
        } else if (ch == '\b') { /* non-destructive backspace */
            if (uart_index > 0) {
                uart_buf[--uart_index] = '\0';
            }
        }
    }

    usart_interrupt_enable(uart_port, USART_INT_RBNE);
}

void log_uart_rx_init(void)
{
    memset(uart_buf, 0, UART_BUFFER_SIZE);
    uart_index = 0;
    cyclic_buf_init(&uart_cyc_buf, 4 * UART_BUFFER_SIZE);
    uart_irq_callback_register(LOG_UART, log_uart_rx_irq_hdl);
}

static void uart_cmd_rx_handle_done(cyclic_buf_t *uart_cyc_buf, uint8_t *buf, uint16_t *len)
{
    if (*len > cyclic_buf_count(uart_cyc_buf)) {
        *len = cyclic_buf_count(uart_cyc_buf);
    }

    if (buf == NULL) {
        cyclic_buf_drop(uart_cyc_buf, *len);
    } else {
        cyclic_buf_read(uart_cyc_buf, buf, *len);
    }
}

/**
 ****************************************************************************************
 * @brief Convert string containing ip address
 *
 * The string may should be of the form a.b.c.d/e (/e being optional)
 *
 * @param[in]  str   String to parse
 * @param[out] ip    Updated with the numerical value of the ip address
 * @param[out] mask  Updated with the numerical value of the network mask
 *                   (or 32 if not present)
 * @return 0 if string contained what looks like a valid ip address and -1 otherwise
 ****************************************************************************************
 */
int cli_parse_ip4(char *str, uint32_t *ip, uint32_t *mask)
{
    char *token;
    uint32_t a, i, j;

    #define check_is_num(_str)  for (j = 0; j < strlen(_str); j++) \
        {                                                          \
            if (_str[j] < '0' || _str[j] > '9')                    \
            return -1;                                             \
        }

    // Check if mask is present
    token = strchr(str, '/');
    if (token && mask) {
        *token++ = '\0';
        check_is_num(token);
        a = atoi(token);
        if ((a == 0) || (a > 32))
            return -1;
        *mask = (1 << a) - 1;
    } else if (mask) {
        *mask = 0xffffffff;
    }

    // parse the ip part
    *ip = 0;
    for (i = 0; i < 4; i++) {
        if (i < 3) {
            token = strchr(str, '.');
            if (!token)
                return -1;
            *token++ = '\0';
        }
        check_is_num(str);
        a = atoi(str);
        if (a > 255)
            return -1;
        str = token;
        *ip += (a << (i * 8));
    }

    return 0;
}

/**
 ****************************************************************************************
 * @brief Convert string mac address
 *
 * The string may should be of the form xx:xx:xx:xx:xx:xx
 *
 * @param[in]  str   String to parse
 * @param[out] bssid BSSID
 * @return 0 if string contained what looks like a valid ip address and -1 otherwise
 ****************************************************************************************
 */
int cli_parse_macaddr(char *str, uint8_t *bssid)
{
    char *token;
    char *endptr = NULL;
    uint32_t a, i, j;

    #define check_is_hex(_str)  for (j = 0; j < 2; j++)            \
        {                                                          \
            if (!((_str[j] >= '0' && _str[j] <= '9')               \
                || (_str[j] >= 'a' && _str[j] <= 'f')              \
                || (_str[j] >= 'A' && _str[j] <= 'F'))) {          \
                return -1;                                         \
            }                                                      \
        }

    // parse the mac address
    for (i = 0; i < 6; i++) {
        if (i < 5) {
            token = strchr(str, ':');
            if (!token)
                return -1;
            *token++ = '\0';
        }
        check_is_hex(str);
        a = (uint32_t)strtoul(str, &endptr, 16);
        if (a > 255)
            return -1;

        bssid[i] = a;
        str = token;
    }

    return 0;
}

static void cmd_help(int argc, char **argv)
{
    uint8_t i;

    for (i = 0; cmd_table[i].function != NULL; i++) {
        app_print("%s\n", cmd_table[i].command);
    }

    return;
}

static void cmd_reboot(int argc, char **argv)
{
    printf("\r\n#");
    SysTimer_SoftwareReset();
}

static void cmd_wifi_connect(int argc, char **argv)
{
    int status = 0;
    char *ssid;
    char *password;

    if(argc == 2) {
        ssid = argv[1];
        password = NULL;
    } else if(argc == 3) {
        ssid = argv[1];
        password = argv[2];
    } else {
        app_print("\rUsage: wifi_connect <SSID> [PASSWORD]\r\n");
        return;
    }
#if 1
    status = wifi_management_connect(ssid, password, true);
#else
    eloop_event_register(WIFI_MGMT_EVENT_CONNECT_SUCCESS, cb_connect_success, NULL, NULL);
    eloop_event_register(WIFI_MGMT_EVENT_CONNECT_FAIL, cb_connect_fail, NULL, NULL);

    status = wifi_management_connect(ssid, password, false);
#endif

    if (status != 0) {
        app_print("start wifi_connect failed %d\r\n", status);
    }
}

extern char echo_srv_ip[100];
static void cmd_aws_test(int argc, char **argv)
{
    if (argc == 2) {
        sys_memcpy(echo_srv_ip, argv[1], strlen(argv[1]));
        RunQualificationTest();
    }
}

static void cmd_mqtt_demo(int argc, char **argv)
{
    vStartSimpleMQTTDemo();
}

#ifdef CFG_HEAP_MEM_CHECK
static void cmd_heap_malloc_dump(int argc, char **argv)
{
    extern void sys_heap_malloc_dump(bool all);
    bool all = false;

    if (argc == 2 && atoi(argv[1]) != 0) {
        all = true;
    }
    sys_heap_malloc_dump(all);
    app_print("heap malloc done! \n");
}
#endif

// Array of supported CLI command
static const struct cmd_entry cmd_table[] =
{
    {"help", cmd_help},
    {"reboot", cmd_reboot},
    {"wifi_connect", cmd_wifi_connect},
    {"aws_test", cmd_aws_test},
    {"aws_mqtt_demo", cmd_mqtt_demo},
#ifdef CFG_HEAP_MEM_CHECK
    {"heap_malloc_dump", cmd_heap_malloc_dump},
#endif
    {"", NULL}
};



/**
 ****************************************************************************************
 * @brief Extract parameter from list
 *
 * Extract the parameter of the string. Parameters are separatd with space unless
 * it starts with " (or ') in which case it extract the parameter until " (or ') is reached.
 * " (or ') are then removed from the parameter.
 *
 * @param[in, out] params Pointer to parameters string to parse. Updated with remaining
 *                        parameters to parse.
 * @return pointer on first parameter
 ****************************************************************************************
 */
static char *get_next_param(char **params)
{
    char *ptr = *params, *next;
    char sep = ' ';

    if (!ptr)
        return NULL;

    if ((ptr[0] == '"') || (ptr[0] == '\'')) {
        sep = ptr[0];
        ptr++;
    }

    next = strchr(ptr, sep);
    if (next) {
        *next++ = '\0';
        while (*next == ' ')
            next++;
        if (*next == '\0')
            next = NULL;
    }
    *params = next;
    return ptr;
}

static int parse_cmd(char *buf, char **argv)
{
    int argc = 0;
    char *param;

    if (buf == NULL)
        return 0;

    param = get_next_param(&buf);
    while (param && (argc < MAX_ARGC)) {
        argv[argc] = param;
        argc++;
        param = get_next_param(&buf);
    }

    return argc;
}

static void cmd_common_help(void)
{
#if (!defined(CONFIG_RF_TEST_SUPPORT)) && defined(CONFIG_BASECMD)
    uint8_t i;
    for (i = 0; cmd_table[i].function != NULL; i++) {
        app_print("\t%s\n", cmd_table[i].command);
    }
#endif

#if defined(CONFIG_RF_TEST_SUPPORT) || defined(CONFIG_INTERNAL_DEBUG)
    app_print("==============================\r\n");
    wifi_rftest_cmd_help();
#endif

#if defined(CONFIG_INTERNAL_DEBUG)
    app_print("==============================\r\n");
    wifi_inner_cmd_help();
#endif
    return;
}

static uint8_t cmd_common_handle(void *data, void **cmd)
{
    const struct cmd_entry *w_cmd = cmd_table;

    while (w_cmd->function) {
        if (!strcmp((char *)data, w_cmd->command)) {
            *cmd = w_cmd->function;
            break;
        }
        w_cmd++;
    }

#if defined(CONFIG_RF_TEST_SUPPORT) || defined(CONFIG_INTERNAL_DEBUG)
    if (w_cmd->function == NULL) {
        w_cmd = wifi_rftest_get_handle_cb(data, cmd);
    }
#endif

#if defined(CONFIG_INTERNAL_DEBUG)
    if (w_cmd->function == NULL) {
        w_cmd = wifi_inner_get_handle_cb(data, cmd);
    }
#endif

    return w_cmd->function == NULL ? CLI_UNKWN_CMD : CLI_SUCCESS;
}

/**
 ****************************************************************************************
 * @brief separate the param and cmd from the msg separated by symbol of ' '
 *
 * @param[in] command          point to msg
 * @param[in] command_len      length of msg
 * @param[out] param           point to the param of cmd
 ****************************************************************************************
 */
static char* cmd_param_separate(char *command, uint16_t command_len)
{
    char *param = strchr(command, ' ');

    if (param) {
        *param++ = '\0';
        while (*param == ' ') {
            param++;
        }
    } else {
         command[command_len - 1] = '\0'; //be sure to have \0 in command
    }
    return param;
}

static void cmn_cmd_exec(struct cmd_msg *msg)
{
    char *command, *param;
    uint32_t res = CLI_UNKWN_CMD;
    cmd_handle_cb handle_cb = NULL;
    cmd_parse_cb parse_cb = parse_cmd; // default parse function.
    char *argv[MAX_ARGC];
    uint32_t argc;
    uint8_t idx;

    command = sys_malloc(msg->len);
    uart_cmd_rx_handle_done((cyclic_buf_t *)msg->data, (uint8_t *)command, &msg->len);

    if (command == NULL)
    {
        app_print("No buffer alloc for cmd ! \r\n");
        return;
    }

    param = cmd_param_separate(command, msg->len);

    if (!strcmp(command, "help")) {
        for (idx = 0; idx < CMD_MODULE_MAX; idx++) {
            if (cmd_info.cmd_reg_infos[idx].help_cb != NULL) {
                app_print("==============================\r\n");
                cmd_info.cmd_reg_infos[idx].help_cb();
            }
        }
        goto symbol_print;
    }

    for (idx = 0; idx < CMD_MODULE_MAX; idx++) {
        if (cmd_info.cmd_reg_infos[idx].get_handle_cb != NULL &&
            (cmd_info.cmd_reg_infos[idx].prefix == NULL ||
            !memcmp(command, cmd_info.cmd_reg_infos[idx].prefix, strlen(cmd_info.cmd_reg_infos[idx].prefix)))) {
            res = cmd_info.cmd_reg_infos[idx].get_handle_cb(command, (void **)&handle_cb);
            if (res == CLI_SUCCESS) {
                parse_cb = cmd_info.cmd_reg_infos[idx].parse_cb != NULL ? cmd_info.cmd_reg_infos[idx].parse_cb : parse_cb;
                break;
            } else if (res == CLI_ERROR) {
                break;
            }
        }
    }

    switch (res) {
    case CLI_SUCCESS :
        argv[0] = command;
        argc = parse_cb(param, &argv[1]) + 1;
        handle_cb(argc, argv);
        break;

    case CLI_UNKWN_CMD :
        app_print("Unknown command - %s!\r\n", command);
        if (unkwn_cmd_handler)
            unkwn_cmd_handler(strlen(command), (uint8_t *)command);
        break;

    case  CLI_ERROR:
    default :
        app_print("Error!\r\n");
        break;
    }

symbol_print:
    sys_mfree(command);
    app_print("# ");
    return;
}

static void cmd_msg_process(struct cmd_msg *msg)
{
    switch (cmd_mode_type_get()) {
#if 0//def CONFIG_ATCMD
    case CMD_MODE_TYPE_AT:
        at_cmd_exec(msg);
        break;
#endif
    case CMD_MODE_TYPE_NORMAL:
    default :
        cmn_cmd_exec(msg);
        break;
    }

    return;
}

uint8_t cmd_module_reg(enum cmd_module_id id, char *prefix, cmd_module_get_handle_cb get_handle_cb,
    cmd_module_help_cb help_cb, cmd_parse_cb parse_cb)
{
    if (id >= CMD_MODULE_MAX || get_handle_cb == NULL)
        return CLI_ERROR;

    cmd_info.cmd_reg_infos[id].prefix = prefix;
    cmd_info.cmd_reg_infos[id].get_handle_cb = get_handle_cb;
    cmd_info.cmd_reg_infos[id].help_cb = help_cb;
    cmd_info.cmd_reg_infos[id].parse_cb = parse_cb;

    return CLI_SUCCESS;
}

void cmd_mode_type_set(enum cmd_mode_type cmd_mode)
{
    cmd_info.cmd_mode = cmd_mode;
}

enum cmd_mode_type cmd_mode_type_get(void)
{
    return cmd_info.cmd_mode;
}

/**
 ****************************************************************************************
 * @brief CLI task main loop
 *
 * CLI task may received command and process it
 ****************************************************************************************
 */
static void cmd_cli_task(void *param)
{
    struct cmd_msg msg;

#ifdef CFG_WLAN_SUPPORT
    wifi_wait_ready();
#endif

    for (;;) {
        sys_queue_read(&cmd_queue, &msg, -1, false);
        cmd_msg_process(&msg);
    }
}

int cmd_shell_init(void)
{
    log_uart_rx_init();

    if (sys_queue_init(&cmd_queue, CLI_QUEUE_SIZE, sizeof(struct cmd_msg))) {
        return -2;
    }

    sys_memset(&cmd_info, 0, sizeof(struct cmd_module_info));
    cmd_mode_type_set(CMD_MODE_TYPE_NORMAL);
    if (cmd_module_reg(CMD_MODULE_COMMON, NULL, cmd_common_handle, cmd_common_help, NULL))
        return -1;

    if (sys_task_create_dynamic((const uint8_t *)"CLI task", CLI_STACK_SIZE, CLI_PRIORITY, cmd_cli_task, NULL) == NULL) {
        return -1;
    }
//    trace_uart_rx_cb_register(cmd_info_send);

    return 0;
}

int cmd_info_send(int id, void *msg_data, uint16_t len)
{
    struct cmd_msg msg;

    msg.id   = CMD_MSG_ID(0, id);
    msg.len  = len;
    msg.data = msg_data;

    return sys_queue_write(&cmd_queue, &msg, 0, true);
}
