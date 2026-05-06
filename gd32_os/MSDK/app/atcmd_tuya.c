/*!
    \file    atcmd_tuya.c
    \brief   AT command for Tuya cloud

    \version 2025-02-10, V1.0.0, firmware for GD32VW55x
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
#include "atcmd_tuya.h"
// #include "tuya_cloud_types.h"

#if 1//def TUYAOS_SUPPORT

/* External function to queue DP data for reporting */
extern int tuya_queue_send_controller_dp(uint8_t dpid, uint8_t type, int32_t value);

/* DPID mapping callback function type */
typedef uint8_t (*tuya_dpid_map_cb_t)(const char *dp_name);

/* DPID mapping callback - to be registered by application */
static tuya_dpid_map_cb_t g_dpid_map_cb = NULL;

/**
 * @brief Register DPID mapping callback
 * @param cb Callback function to map DP name to DPID
 */
void tuya_at_register_dpid_map(tuya_dpid_map_cb_t cb)
{
    g_dpid_map_cb = cb;
}

/**
 * @brief Get DPID by name
 * @param dp_name DP name string
 * @return DPID value, or 0 if not found
 */
static uint8_t get_dpid(const char *dp_name)
{
    if (g_dpid_map_cb) {
        return g_dpid_map_cb(dp_name);
    }
    /* Fallback to default DPID if no callback registered */
    AT_TRACE("Warning: No DPID mapping callback registered\r\n");
    return 0;
}


/* AT response timeout (based on 160MHz system clock) */
#define AT_RSP_TIMEOUT_US       85500000   /* ~534ms timeout */

/*=============================================================================*/
/*                  Internal Helper Functions                                 */
/*=============================================================================*/
static int at_uart_send_wait_rsp(char *cmd, int cmd_len,
                                 char *data, int data_len,
                                 char *rsp, int rsp_len)
{
    int count = 100;
    int ret = AT_RSP_TIMEOUT;
    char *p = rsp;
    char ch;

    if ((cmd == NULL && data == NULL) || rsp == NULL)
        return -1;

    sys_memset(rsp, 0, rsp_len);

    /* 1. wait uart rx process done */
    while (at_uart_rx_is_ongoing() && count-- > 0) {
        sys_ms_sleep(1);
    }

    /* 2. disable uart rx buffer not empty interrupt */
    usart_interrupt_disable(at_uart_conf.usart_periph, USART_INT_RBNE);

    /* 3. wait uart tx done */
    sys_sema_down(&at_hw_tx_sema, 0);

    /* 4. transmit atcmd */
    while (cmd_len > 0) {
        while (RESET == usart_flag_get(at_uart_conf.usart_periph, USART_FLAG_TBE));
        usart_data_transmit(at_uart_conf.usart_periph, *cmd++);
        cmd_len--;
    }

    /* 5. transmit data parameter */
    if (data_len) {
        while (data_len > 0) {
            while (RESET == usart_flag_get(at_uart_conf.usart_periph, USART_FLAG_TBE));
            usart_data_transmit(at_uart_conf.usart_periph, *data++);
            data_len--;
        }
    }

    /* 6. clear rx buf */
    uart_rx_flush(at_uart_conf.usart_periph);

    /* 7. transmit '\r' to finish command */
    while (RESET == usart_flag_get(at_uart_conf.usart_periph, USART_FLAG_TBE));
    usart_data_transmit(at_uart_conf.usart_periph, '\r');
    while (RESET == usart_flag_get(at_uart_conf.usart_periph, USART_FLAG_TC));

    /* 8. wait rx feedback, OK or ERROR */
    do {
        if (uart_getc_with_timeout(at_uart_conf.usart_periph, &ch, AT_RSP_TIMEOUT_US)) {
            AT_TRACE("rsp timeout\r\n");
            break;
        }

        *p++ = ch;
        if (ch == '\r' || ch == '\n')
            break;

        if (((uint32_t)p - (uint32_t)rsp) > rsp_len)
            break;
    } while (1);

    /* 9. check response */
    AT_TRACE("rsp=%s\r\n", rsp);
    if (strstr(rsp, "OK")) {
        ret = AT_OK;
    } else if (strstr(rsp, "ERROR")) {
        ret = AT_RSP_ERR;
    }

    /* 10. release tx lock */
    sys_sema_up(&at_hw_tx_sema);

    /* 11. enable rx interrupt */
    usart_interrupt_enable(at_uart_conf.usart_periph, USART_INT_RBNE);

    return ret;
}


/*=============================================================================*/
/*                  controller-> VW553: AT Command Handlers                        */
/*  These functions handle AT commands received from controller                    */
/*=============================================================================*/

/**
 * @brief AT+TYTELS command handler
 * @brief Receive telemetry data or work mode from controller
 * @param argc Number of arguments
 * @param argv Argument array
 * @return none
 *
 * Command format 1: AT+TYTELS=<mode>
 *   Report work mode
 *   mode: 0=charge mode, 1=discharge mode
 *
 * Command format 2: AT+TYTELS=<mode>,<idx>,<value>
 *   Report telemetry data (mode can be 0 or 1, not used in data reporting)
 *   idx: Data type index
 *     - 0: Power percentage (uint8_t, 0-100)
 *     - 1: Voltage (float, 0-30V)
 *     - 2: Current (float, 0-30A)
 *     - 3: LED status (uint8_t, 0=off, 1=on)
 *
 * Examples:
 *   AT+TYTELS=0          // Report charge mode
 *   AT+TYTELS=1          // Report discharge mode
 *   AT+TYTELS=0,0,85     // Report power 85%
 *   AT+TYTELS=0,1,20.1   // Report voltage 20.1V
 *   AT+TYTELS=1,2,3.5    // Report current 3.5A
 *   AT+TYTELS=0,3,1      // Report LED on
 */
void at_tytels(int argc, char **argv)
{
    int mode, idx;
    float value_f;
    int value_int;
    uint8_t dpid;
    uint8_t dp_type;
    const char *mode_str[] = {"Charge", "Discharge"};
    const char *idx_str[] = {"Power%", "Voltage", "Current", "LED"};

    AT_RSP_START(128);

    /* Check argument count */
    if (argc == 2) {
        /* Format 1: AT+TYTELS=<mode> - Report work mode */
        mode = atoi(argv[1]);

        /* Validate mode */
        if (mode < 0 || mode > 1) {
            AT_TRACE("AT+TYTELS: Invalid mode %d (must be 0 or 1).\r\n", mode);
            AT_RSP_ERR();
            return;
        }

        /* Get DPID for work mode */
        dpid = get_dpid("work_mode");

        /* Send to queue for DP reporting */
        int ret = tuya_queue_send_controller_dp(dpid, 1, mode);
        if (ret != 0) {
            AT_TRACE("AT+TYTELS: Failed to queue work mode, ret=%d\r\n", ret);
            AT_RSP_ERR();
            return;
        }

        AT_TRACE("AT+TYTELS: Work mode=%s queued (dpid=%d, value=%d)\r\n",
                 mode_str[mode], dpid, mode);
        AT_RSP_OK();
        return;

    } else if (argc == 4) {
        /* Format 2: AT+TYTELS=<mode>,<idx>,<value> - Report telemetry data */
        mode = atoi(argv[1]);
        idx = atoi(argv[2]);
        value_f = atof(argv[3]);

        /* Validate mode (0 or 1, not used but must be valid) */
        if (mode < 0 || mode > 1) {
            AT_TRACE("AT+TYTELS: Invalid mode %d (must be 0 or 1).\r\n", mode);
            AT_RSP_ERR();
            return;
        }

        /* Validate idx */
        if (idx < 0 || idx > 3) {
            AT_TRACE("AT+TYTELS: Invalid idx %d (must be 0-3).\r\n", idx);
            AT_RSP_ERR();
            return;
        }

        /* Determine DPID and validate value based on idx */
        switch (idx) {
        case 0:  /* Power percentage */
            if (value_f < 0 || value_f > 100) {
                AT_TRACE("AT+TYTELS: Invalid power %% %.1f (must be 0-100).\r\n", value_f);
                AT_RSP_ERR();
                return;
            }
            dpid = get_dpid("power_percentage");
            value_int = (int)value_f;
            dp_type = 1;  /* PROP_VALUE */
            break;

        case 1:  /* Voltage */
            if (value_f < 0 || value_f > 30.0) {
                AT_TRACE("AT+TYTELS: Invalid voltage %.2f (must be 0-30V).\r\n", value_f);
                AT_RSP_ERR();
                return;
            }
            dpid = get_dpid("voltage");
            value_int = (int)(value_f * 100);  /* Convert to centivolt */
            dp_type = 1;  /* PROP_VALUE */
            break;

        case 2:  /* Current */
            if (value_f < 0 || value_f > 30.0) {
                AT_TRACE("AT+TYTELS: Invalid current %.2f (must be 0-30A).\r\n", value_f);
                AT_RSP_ERR();
                return;
            }
            dpid = get_dpid("current");
            value_int = (int)(value_f * 100);  /* Convert to centiampere */
            dp_type = 1;  /* PROP_VALUE */
            break;

        case 3:  /* LED status */
            value_int = (int)value_f;
            if (value_int < 0 || value_int > 1) {
                AT_TRACE("AT+TYTELS: Invalid LED status %d (must be 0 or 1).\r\n", value_int);
                AT_RSP_ERR();
                return;
            }
            dpid = get_dpid("led_status");
            dp_type = 0;  /* PROP_BOOL */
            break;

        default:
            AT_RSP_ERR();
            return;
        }

        /* Send to queue for DP reporting */
        int ret = tuya_queue_send_controller_dp(dpid, dp_type, value_int);
        if (ret != 0) {
            AT_TRACE("AT+TYTELS: Failed to queue DP data, ret=%d\r\n", ret);
            AT_RSP_ERR();
            return;
        }

        AT_TRACE("AT+TYTELS: %s=%.2f queued (dpid=%d, value=%d)\r\n",
                 idx_str[idx], value_f, dpid, value_int);
        AT_RSP_OK();
        return;

    } else {
        /* Invalid argument count */
        AT_TRACE("AT+TYTELS: argc(%d) must be 2 or 4.\r\n", argc);
        AT_RSP_ERR();
        return;
    }
}

/**
 * @brief AT+TYERRCODE command handler
 * @brief Receive error code from controller
 * @param argc Number of arguments
 * @param argv Argument array
 * @return none
 *
 * Command format: AT+TYERRCODE=<err_code>
 * Parameters:
 *   err_code: Error code (uint8_t)
 */
void at_tuya_error_code(int argc, char **argv)
{
    int err_code;
    uint8_t dpid;

    AT_RSP_START(128);

    if (argc != 2) {
        AT_TRACE("AT+TYERRCODE: argc(%d) is not 2.\r\n", argc);
        AT_RSP_ERR();
        return;
    }

    /* Parse error code */
    err_code = atoi(argv[1]);

    /* Validate range: 0-255 (uint8_t) */
    if (err_code < 0 || err_code > 255) {
        AT_TRACE("AT+TYERRCODE: Invalid error code %d (must be 0-255).\r\n", err_code);
        AT_RSP_ERR();
        return;
    }

    /* Get DPID for error code */
    dpid = get_dpid("error_code");

    /* Send to queue for DP reporting */
    int ret = tuya_queue_send_controller_dp(dpid, 1, err_code);
    if (ret != 0) {
        AT_TRACE("AT+TYERRCODE: Failed to queue DP data, ret=%d\r\n", ret);
        AT_RSP_ERR();
        return;
    }

    AT_TRACE("AT+TYERRCODE: Error code=%d queued (dpid=%d)\r\n", err_code, dpid);
    AT_RSP_OK();
}

/**
 * @brief AT+TYPROPS command handler
 * @brief Receive property status from controller
 * @param argc Number of arguments
 * @param argv Argument array
 * @return none
 *
 * Command format: AT+TYPROPS=<idx>,<status>
 * Parameters:
 *   idx (uint8_t): Property type index
 *     - 0: LED status
 *     - 1: Energy storage working status
 *   status (uint8_t):
 *     - For idx=0 (LED): 0=off, 1=on
 *     - For idx=1 (Work status): status value
 *
 * Examples:
 *   AT+TYPROPS=0,1   // LED on
 *   AT+TYPROPS=0,0   // LED off
 *   AT+TYPROPS=1,5   // Energy storage work status 5
 */
void at_typrops(int argc, char **argv)
{
    int idx, status;
    uint8_t dpid;
    uint8_t dp_type;
    const char *idx_str[] = {"LED", "WorkStatus"};

    AT_RSP_START(128);

    if (argc != 3) {
        AT_TRACE("AT+TYPROPS: argc(%d) is not 3.\r\n", argc);
        AT_RSP_ERR();
        return;
    }

    /* Parse parameters */
    idx = atoi(argv[1]);
    status = atoi(argv[2]);

    /* Validate idx */
    if (idx < 0 || idx > 1) {
        AT_TRACE("AT+TYPROPS: Invalid idx %d (must be 0 or 1).\r\n", idx);
        AT_RSP_ERR();
        return;
    }

    /* Determine DPID and validate based on idx */
    if (idx == 0) {
        /* LED status */
        if (status < 0 || status > 1) {
            AT_TRACE("AT+TYPROPS: Invalid LED status %d (must be 0 or 1).\r\n", status);
            AT_RSP_ERR();
            return;
        }
        dpid = get_dpid("led_status");
        dp_type = 0;  /* PROP_BOOL */
    } else {
        /* Energy storage working status */
        if (status < 0 || status > 255) {
            AT_TRACE("AT+TYPROPS: Invalid work status %d (must be 0-255).\r\n", status);
            AT_RSP_ERR();
            return;
        }
        dpid = get_dpid("work_status");
        dp_type = 3;  /* PROP_ENUM */
    }

    /* Send to queue for DP reporting */
    int ret = tuya_queue_send_controller_dp(dpid, dp_type, status);
    if (ret != 0) {
        AT_TRACE("AT+TYPROPS: Failed to queue DP data, ret=%d\r\n", ret);
        AT_RSP_ERR();
        return;
    }

    AT_TRACE("AT+TYPROPS: %s status=%d queued (dpid=%d)\r\n",
             idx_str[idx], status, dpid);
    AT_RSP_OK();
}

/*=============================================================================*/
/*                  VW553 -> controller: API Functions                              */
/*  These functions send AT commands to controller                                 */
/*=============================================================================*/

/**
 * @brief Send connection response to controller
 * @param status Connection status (0:success, 1:fail)
 * @return AT_OK on success, error code on failure
 *
 * This function sends: AT+TYCONNRSP=<status>
 */
int tuya_send_conn_response(uint8_t status)
{
    char atcmd[32], rsp[32];
    int ret;

    if (status > 1) {
        AT_TRACE("tuya_send_conn_response: Invalid status %d\r\n", status);
        return AT_ERR;
    }

    co_snprintf(atcmd, sizeof(atcmd), "AT+TYCONNRSP=%u", status);

    ret = at_uart_send_wait_rsp(atcmd, strlen(atcmd),
                                NULL, 0, rsp, sizeof(rsp));
    if (ret != AT_OK) {
        AT_TRACE("tuya_send_conn_response: Send failed\r\n");
    } else {
        AT_TRACE("tuya_send_conn_response: Sent status=%d (%s)\r\n",
                 status, status == 0 ? "success" : "fail");
    }

    return ret;
}

/**
 * @brief Send property control request to controller
 * @param idx Control type index
 *   - 0: LED control
 *   - 1: Energy storage main switch control
 *   - 2: Energy storage charge control
 *   - 3: Energy storage discharge control
 * @param cmd Command value
 *   - For idx=0 (LED): 0=off, 1=on
 *   - For idx=1 (Main switch): 0=off, 1=on
 *   - For idx=2 (Charge): 0=on, 1=off
 *   - For idx=3 (Discharge): 0=on, 1=off
 * @return AT_OK on success, error code on failure
 *
 * This function sends: AT+TYPROPREQ=<idx>,<cmd>
 */
int tuya_send_property_request(uint8_t idx, uint8_t cmd)
{
    char atcmd[32], rsp[32];
    int ret;
    const char *idx_str[] = {"LED", "MainSwitch", "Charge", "Discharge"};
    const char *cmd_str[4][2] = {
        {"off", "on"},         /* LED */
        {"off", "on"},         /* Main switch */
        {"on", "off"},         /* Charge */
        {"on", "off"}          /* Discharge */
    };

    if (idx > 3) {
        AT_TRACE("tuya_send_property_request: Invalid idx %d\r\n", idx);
        return AT_ERR;
    }

    if (cmd > 1) {
        AT_TRACE("tuya_send_property_request: Invalid cmd %d\r\n", cmd);
        return AT_ERR;
    }

    co_snprintf(atcmd, sizeof(atcmd), "AT+TYPROPREQ=%u,%u", idx, cmd);

    ret = at_uart_send_wait_rsp(atcmd, strlen(atcmd),
                                NULL, 0, rsp, sizeof(rsp));
    if (ret != AT_OK) {
        AT_TRACE("tuya_send_property_request: %s control=%d (%s) failed\r\n",
                 idx_str[idx], cmd, cmd_str[idx][cmd]);
    } else {
        AT_TRACE("tuya_send_property_request: %s control=%d (%s) success\r\n",
                 idx_str[idx], cmd, cmd_str[idx][cmd]);
    }

    return ret;
}

/**
 * @brief Send cloud-to-device message to controller
 * @param message Message string to send
 * @return AT_OK on success, error code on failure
 *
 * This function sends: AT+TYC2DMSGS=<message>
 */
int tuya_send_c2d_message(const char *message)
{
    char *atcmd, rsp[32];
    int ret;
    uint32_t msg_len, atcmd_len;

    if (message == NULL) {
        AT_TRACE("tuya_send_c2d_message: NULL message\r\n");
        return AT_ERR;
    }

    msg_len = strlen(message);
    atcmd_len = msg_len + 32;
    atcmd = sys_zalloc(atcmd_len);
    if (NULL == atcmd) {
        AT_TRACE("tuya_send_c2d_message: alloc failed\r\n");
        return AT_ERR;
    }

    co_snprintf(atcmd, atcmd_len, "AT+TYC2DMSGS=%s", message);

    ret = at_uart_send_wait_rsp(atcmd, strlen(atcmd),
                                NULL, 0, rsp, sizeof(rsp));
    if (ret != AT_OK) {
        AT_TRACE("tuya_send_c2d_message: Send failed\r\n");
    } else {
        AT_TRACE("tuya_send_c2d_message: Sent message: %s\r\n", message);
    }

    sys_mfree(atcmd);
    return ret;
}

#endif
