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

#include "app_cfg.h"
#include "gd32vw55x.h"
#include "cmd_shell.h"
#include "dbg_print.h"
#include "uart.h"
#include "uart_config.h"
#include <ctype.h>
#include "_build_date.h"


#include "log_uart.h"
#include "wakelock.h"
#include "trace_uart.h"

#ifdef CFG_TRACE
#include "trace_ext.h"
#endif

#if NVDS_FLASH_SUPPORT
#include "nvds_flash.h"
#endif

#if defined(CONFIG_INTERNAL_DEBUG)
#include "cmd_inner.c"
#endif

// CLI task message queue size
#define CLI_QUEUE_SIZE 3

struct cmd_module_info {
    enum cmd_mode_type cmd_mode;
    struct cmd_module_reg_info cmd_reg_infos[CMD_MODULE_MAX];
};

static cyclic_buf_t uart_cyc_buf;
char uart_buf[UART_BUFFER_SIZE];
uint32_t uart_index = 0;
static os_queue_t cmd_queue;
static struct cmd_module_info cmd_info;
static const struct cmd_entry cmd_table[];

int cmd_info_send(int id, void *msg_data, uint16_t len);

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

static void log_uart_rx_irq_hdl(uint32_t uart_port)
{
    char ch;
    usart_interrupt_disable(uart_port, USART_INT_RBNE);
    while (1) {
        // We should have chance to check overflow error
        // Otherwise it may cause dead loop handle rx interrupt
        if (RESET != usart_flag_get(uart_port, USART_FLAG_ORERR)) {
            usart_flag_clear(uart_port, USART_FLAG_ORERR);
        }

        if ((RESET != usart_flag_get(uart_port, USART_FLAG_RBNE))) {
            ch = (char)usart_data_receive(uart_port);
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
 * @brief Process function for 'list_cmd' command
 *
 * Simply list the commands sending a print message.
 *
 * @param[in] params Not used
 *
 * @return 0 on success and !=0 if error occurred
 ****************************************************************************************
 */
static void cmd_help(int argc, char **argv)
{
    uint8_t i;

    for (i = 0; cmd_table[i].function != NULL; i++) {
        app_print("%s\n", cmd_table[i].command);
    }

    return;
}

static void cmd_version(int argc, char **argv)
{
    app_print("Build date: %s\n", SDK_BUILD_DATE);
}


static void cmd_reboot(int argc, char **argv)
{
    printf("\r\n#");
    SysTimer_SoftwareReset();
}

static void cmd_task_list(int argc, char **argv)
{
    app_print("TaskName\t\tState\tPri\tStack\tID\tStackBase\r\n");
    app_print("--------------------------------------------------\r\n");
    sys_task_list(NULL);
}

/**
 ****************************************************************************************
 * @brief Process function for 'free' command
 *
 * Provides information about memory usage.
 *
 * @param[in] params Not used
 * @return 0 on success and !=0 if error occurred
 ****************************************************************************************
 */
static void cmd_free(int argc, char **argv)
{
    extern void dump_mem_block_list(void);

    int total, used, free, max_used;

    sys_heap_info(&total, &free, &max_used);
    used = total - free;
    max_used = total - max_used;

    app_print("RTOS HEAP: free=%d used=%d max_used=%d/%d\n",
                free, used, max_used, total);

    dump_mem_block_list();

    return;
}

static void cmd_sys_ps(int argc, char **argv)
{
    uint8_t ps_mode;

    if (argc == 2) {
        ps_mode = atoi(argv[1]);
        if (ps_mode == 1) {
            sys_ps_set(SYS_PS_DEEP_SLEEP);
        } else if (ps_mode == 0) {
            sys_ps_set(SYS_PS_OFF);
        } else {
            goto Usage;
        }
        return;
    }
    ps_mode = sys_ps_get();

    app_print("Current power save mode: %d\n\r", ps_mode);

Usage:
    app_print("Usage: sys_ps [mode]\n\r");
    app_print("\tmode: 0: None, 1: CPU Deep Sleep\r\n");
}

/**
 ****************************************************************************************
 * @brief Process function for 'cpu_stats' command
 *
 * @param[in] params Not used
 * @return 0 on success and !=0 if error occurred
 ****************************************************************************************
 */
static void cmd_cpu_stats(int argc, char **argv)
{
    sys_cpu_stats();
}


#if NVDS_FLASH_SUPPORT
static int nvds_hex_para(char *argv, char *para, uint16_t *para_len, uint16_t max_len)
{
    uint16_t offset, hex_len;
    char str[3];

    hex_len = (strlen(argv) - 3) / 2 + 1;
    if (hex_len > max_len)
        return -1;

    offset = hex_len - 1;
    for (int i = strlen(argv) - 1; i >= 2; i = i - 2) {
        if (i <= 2) {
            str[0] = argv[i];
            str[1] = '\0';
            para[offset--] = strtoul(str, NULL, 16);
        } else {
            str[0] = argv[i - 1];
            str[1] = argv[i];
            str[2] = '\0';
            para[offset--] = strtoul(str, NULL, 16);
        }
    }

    para[hex_len] = '\0';
    *para_len = hex_len;

    #if 0
    for(int i = 0; i < hex_len; i++)
        app_print("0x%02x ", para[i]);
    app_print("\r\n");
    #endif
    return 0;
}

/**
 ****************************************************************************************
 * @brief Process function for 'nvds' command
 *
 * nvds command can be used to clean/add/del/show nvds flash data
 *
   @verbatim
     nvds clean : Erase internal nvds flash
     nvds add namespace key value : Save data to nvds flash.
     nvds del namespace key : Delete data in nvds flash.
     nvds dump : Show all valid data stored in nvds flash.
     nvds dump verbose : Show all data include invalid stored in nvds flash.
     nvds dump namespace : Show all data in the specified namespace.
     nvds dump namespace key : Show data by specified namespace and key.
   @endverbatim
 *
 * @param[in] params clean/add/del/show commands above
 ****************************************************************************************
 */
static void cmd_nvds_handle(int argc, char **argv)
{
    char *option, *ns;
    int ret;
    char key[16], value[256];
    uint16_t key_len;
    uint16_t value_len;
    uint32_t length;
    uint8_t *buffer;

    if (argc < 2)
        goto usage;

    option = argv[1];
    if (!strcmp("clean", option)) {
        ret = nvds_clean(NULL);
        if (ret)
            app_print("NVDS flash erase failed, error code:%d\r\n", ret);
        else
            app_print("NVDS flash erase ok.\r\n");
    } else if (!strcmp("add", option)) {
        if (argc < 5)
            goto usage;

        // namespace
        ns = argv[2];
        if (strlen(ns) > 15) {
            app_print("Illeagl namespace, Maximum length is 15 characters.\r\n");
            goto usage;
        }

        // key
        if (!memcmp("0x", argv[3], 2)) {
            ret = nvds_hex_para(argv[3], key, &key_len, 15);
            if (ret) {
                app_print("Illeagl key, Maximum length is 15 characters.\r\n");
                goto usage;
            }
        } else {
            key_len = strlen(argv[3]) + 1;
            if (strlen(argv[3]) > 15) {
                app_print("Illeagl key, Maximum length is 15 characters.\r\n");
                goto usage;
            }
            memcpy(key, argv[3], key_len);
        }

        // value
        if (!memcmp("0x", argv[4], 2)) {
            ret = nvds_hex_para(argv[4], value, &value_len, 255);
            if (ret) {
                app_print("Illeagl value, Maximum length is 255 characters.\r\n");
                goto usage;
            }
        } else {
            value_len = strlen(argv[4]) + 1;
            if (strlen(argv[4]) > 255) {
                app_print("Illeagl value, Maximum length is 255 characters.\r\n");
                goto usage;
            }
            memcpy(value, argv[4], value_len);
        }

        ret = nvds_data_put(NULL, ns, key, (uint8_t*)value, value_len);
        if (ret)
            app_print("NVDS flash add key failed, error code:%d\r\n", ret);
        else
            app_print("NVDS flash add key ok\r\n");
    } else if (!strcmp("del", option)) {
        if (argc < 3)
            goto usage;

        // namespace
        ns = argv[2];
        if (strlen(ns) > 15) {
            app_print("Illeagl namespace, Maximum length is 15 characters.\r\n");
            goto usage;
        }

        if (argc == 3) {
            ret = nvds_del_keys_by_namespace(NULL, ns);
        } else {
            // key
            if (!memcmp("0x", argv[3], 2)) {
                ret = nvds_hex_para(argv[3], key, &key_len, 15);
                if (ret) {
                    app_print("Illeagl key, Maximum length is 15 characters.\r\n");
                    goto usage;
                }
            } else {
                key_len = strlen(argv[3]) + 1;
                if (strlen(argv[3]) > 15) {
                    app_print("Illeagl key, Maximum length is 15 characters.\r\n");
                    goto usage;
                }
                memcpy(key, argv[3], key_len);
            }

            ret = nvds_data_del(NULL, ns, key);
        }

        if (ret)
            app_print("NVDS flash delete key failed, error code:%d\r\n", ret);
        else
            app_print("NVDS flash delete key ok\r\n");
    } else if (!strcmp("dump", option)) {
        if (argc == 2) {
            nvds_dump(NULL, 0, NULL);
        } else {
            if (!strcmp("verbose", argv[2])) {
                nvds_dump(NULL, 1, NULL);
            } else {
                // namespace
                ns = argv[2];
                if (strlen(ns) > 15) {
                    app_print("Illeagl namespace, Maximum length is 15 characters.\r\n");
                    goto usage;
                }
                if (argc == 3) {
                    nvds_dump(NULL, 0, ns);
                }

                if (argc > 3) {
                    // key
                    if (!memcmp("0x", argv[3], 2)) {
                        ret = nvds_hex_para(argv[3], key, &key_len, 15);
                        if (ret) {
                            app_print("Illeagl key, Maximum length is 15 characters.\r\n");
                            goto usage;
                        }
                    } else {
                        key_len = strlen(argv[3]) + 1;
                        if (strlen(argv[3]) > 15) {
                            app_print("Illeagl key, Maximum length is 15 characters.\r\n");
                            goto usage;
                        }
                        memcpy(key, argv[3], key_len);
                    }

                    length = 0;
                    ret = nvds_data_get(NULL, ns, key, NULL, &length);
                    if (ret) {
                        app_print("NVDS flash get length failed, error code:%d\r\n", ret);
                        goto usage;
                    }
                    buffer = sys_malloc(length  + 1);
                    ret = nvds_data_get(NULL, ns, key, buffer, &length);
                    if (ret) {
                        app_print("NVDS flash get key value failed, error code:%d\r\n", ret);
                        goto usage;
                    } else {
                        app_print("NVDS flash get key: %s, value(str):%s, value(hex):", key, (char*)buffer);
                        for(int i = 0; i < length; i++) {
                            app_print("%02x ", buffer[i]);
                        }
                        app_print("\r\n");
                    }
                    sys_mfree(buffer);
                }
            }
        }
    } else {
        goto usage;
    }
    return;
usage:
    app_print("Usage: nvds clean | add | del | dump [options]\r\n");
    app_print("     : nvds clean : Erase internal nvds flash.\r\n");
    app_print("     : nvds add <namespace> <key> <value> : Save data to nvds flash.\r\n");
    app_print("     : nvds del <namespace> <key> : Delete data in nvds flash.\r\n");
    app_print("     : nvds del <namespace> : Delete all the data in the specified namespace.\r\n");
    app_print("     : nvds dump : Show all valid data stored in nvds flash.\r\n");
    app_print("     : nvds dump verbose : Show all data include invalid stored in nvds flash.\r\n");
    app_print("     : nvds dump <namespace> : Show all data in the specified namespace.\r\n");
    app_print("     : nvds dump <namespace> <key> : Show data by specified namespace and key.\r\n");
    app_print("     : Hexadecimals parmeter starts with 0x, else string.\r\n");
    app_print("Example:\r\n");
    app_print("     : nvds add wifi ip 0xc0a80064\r\n");
    app_print("     : nvds add wifi ssid gigadevice\r\n");
}
#endif /* NVDS_FLASH_SUPPORT */

extern uint8_t ble_nvds_get(uint8_t tag, uint8_t *lengthPtr, uint8_t *buf);
extern uint8_t ble_nvds_put(uint8_t tag, uint8_t length, uint8_t *buf);
extern uint8_t ble_nvds_del(uint8_t tag);

static void cmd_ble_flash_set(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t tag, ret, len, i;
    uint8_t data[0xff] = {0};

    if (argc < 4) {
        app_print("Usage: ble_flash_set <tag> <len> <data>\r\n");
        return;
    }

    tag = (uint8_t)strtoul((const char *)argv[1], &endptr, 16);
    len = (uint8_t)strtoul((const char *)argv[2], &endptr, 0);
    if (argc - 3 != len) {
        app_print("ble flash set invalid len\r\n");
        return;
    }

    for (i = 0; i < len; i++) {
        data[i] = (uint8_t)strtoul((const char *)argv[3 + i], &endptr, 16);
    }

    ret = ble_nvds_put(tag, len, data);
    app_print("ble nvds set ret:%u\r\n", ret);

    return;
}


static void cmd_ble_flash_get(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t tag, ret, i, len = 0xff;
    uint8_t data[0xff] = {0};

    if (argc != 2) {
        app_print("Usage: ble_flash_get <tag>\r\n");
        return;
    }

    tag = (uint8_t)strtoul((const char *)argv[1], &endptr, 16);

    ret = ble_nvds_get(tag, &len, data);
    app_print("ble nvds get ret:%u, len:%u, data:\r\n    ", ret, len);
    if (ret == 0) {
        for (i = 0; i < len; i++)
            app_print("%x ", data[i]);
        app_print("\r\n");
    }

    return;
}


static void cmd_ble_flash_del(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t tag, ret;

    if (argc != 2) {
        app_print("Usage: ble_flash_del <tag>\r\n");
        return;
    }

    tag = (uint8_t)strtoul((const char *)argv[1], &endptr, 16);

    ret = ble_nvds_del(tag);
    app_print("ble nvds del ret:%u\r\n", ret);

    return;
}

static void cmd_read_memory(int argc, char **argv)
{
    char *endptr = NULL;
    uint32_t addr = 0;
    uint32_t count = 4;
    uint32_t width = 4;

    if (argc > 1) {
        addr = (uint32_t)strtoul((const char *)argv[1], &endptr, 16);
        if (*endptr != '\0') {
            app_print("rmem: invalid address\r\n");
            goto exit;
        }
    }
    if (argc > 2) {
        count = (uint32_t)strtoul((const char *)argv[2], &endptr, 0);
        if (*endptr != '\0') {
            app_print("rmem: invalid count\r\n");
            goto exit;
        }
    }
    if (argc > 3) {
        width = (uint32_t)strtoul((const char *)argv[3], &endptr, 0);
        if (*endptr != '\0') {
            app_print("rmem: invalid width\r\n");
            goto exit;
        }
    }

    print_buffer(addr, (void *)addr, width, count, 0);
    return;
exit:
    app_print("Usage: rmem <addr> [count] [width]\r\n");
}


// Array of supported CLI command
static const struct cmd_entry cmd_table[] =
{
    {"help", cmd_help},
    {"reboot", cmd_reboot},
    {"version", cmd_version},
    {"rmem", cmd_read_memory},
#ifdef CONFIG_BASECMD
    {"tasks", cmd_task_list},
    {"free", cmd_free},
    {"sys_ps", cmd_sys_ps},
    {"cpu_stats", cmd_cpu_stats},

#if NVDS_FLASH_SUPPORT
    {"nvds", cmd_nvds_handle},
#endif /* NVDS_FLASH_SUPPORT */
#endif /* CONFIG_BASECMD */
    {"ble_flash_set", cmd_ble_flash_set},
    {"ble_flash_get", cmd_ble_flash_get},
    {"ble_flash_del", cmd_ble_flash_del},
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

    for (;;) {
        sys_queue_read(&cmd_queue, &msg, -1, false);
        cmd_msg_process(&msg);
    }
}

int cmd_shell_init(void)
{
    log_uart_rx_init();

    if (sys_task_create_dynamic((const uint8_t *)"CLI task", CLI_STACK_SIZE, CLI_PRIORITY, cmd_cli_task, NULL) == NULL) {
        return -1;
    }

    if (sys_queue_init(&cmd_queue, CLI_QUEUE_SIZE, sizeof(struct cmd_msg))) {
        return -2;
    }

    sys_memset(&cmd_info, 0, sizeof(struct cmd_module_info));
    cmd_mode_type_set(CMD_MODE_TYPE_NORMAL);
    if (cmd_module_reg(CMD_MODULE_COMMON, NULL, cmd_common_handle, cmd_common_help, NULL))
        return -1;

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
