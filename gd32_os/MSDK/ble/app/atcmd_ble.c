/*!
    \file    atcmd_ble.c
    \brief   AT command BLE part for GD32VW55x SDK

    \version 2024-08-19, V1.0.0, firmware for GD32VW55x
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
typedef struct
{
    bool disconn_flag;
    uint8_t sync_idx;
    bool passth_auto_enable_flag;
    uint8_t at_svc_id;
    bool passth_mode_on;
} at_ble_cb_t;

#define ATBLE_PASSTH_MAX_SIZE            2048

static at_ble_cb_t at_ble_cb;

extern void ble_app_scan_mgr_evt_handler(ble_scan_evt_t event, ble_scan_data_u *p_data);
static ble_status_t at_ble_gattc_co_cb(ble_gattc_co_msg_info_t *p_info);

static void at_ble_sec_authen_cmpl(uint8_t conn_idx, uint8_t result);
static void at_ble_sec_input_key_req(uint8_t conn_idx);
static void at_ble_sec_key_cfm_req(uint8_t conn_idx, uint32_t number);

static const app_sec_callbacks at_ble_sec_cb = {
    .authen_cmpl = at_ble_sec_authen_cmpl,
    .input_key_req = at_ble_sec_input_key_req,
    .key_cfm_req = at_ble_sec_key_cfm_req,
};


/*!
    \brief      Check the terminate string
    \param[in]  str: string to be check
    \param[out] none
    \retval     True if check success, false if check fail
*/
bool at_ble_terminate_string_check(uint8_t *str)
{
    if (strlen(PASSTH_TERMINATE_STR) == strlen((char *)str) && strncmp((char *)str, PASSTH_TERMINATE_STR, strlen(PASSTH_TERMINATE_STR)) == 0)
        return true;
    return false;
}

/*!
    \brief      RX callback of passthrough
    \param[in]  conn_idx: connection index
    \param[in]  data_len: data length
    \param[in]  p_data: data pointer
    \param[out] none
    \retval     none
*/
void at_ble_passth_rx_callback(uint8_t conn_idx, uint16_t data_len, uint8_t *p_data)
{
    AT_RSP_DIRECT((char *)p_data, data_len);
}

/*!
    \brief      Enable passthrough mode
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/

void at_ble_passth_mode_enable(int argc, char **argv)
{
    uint32_t cur_cnt = 0;
    uint8_t *tx_buf = sys_malloc(ATBLE_PASSTH_MAX_SIZE);
    uint8_t conn_idx;
    char *endptr = NULL;
    bool reset = true;
    ble_device_t *p_dev = NULL;

    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
    } else {
        goto Error;
    }

    conn_idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
    if (!dm_check_connection_valid(conn_idx)) {
        AT_TRACE("link has not been established\r\n");
        goto Error;
    }
    p_dev = dm_find_dev_by_conidx(conn_idx);

    if (p_dev->role == BLE_SLAVE) {
        ble_datatrans_srv_rx_cb_reg(at_ble_passth_rx_callback);
    }
#if (BLE_APP_GATT_CLIENT_SUPPORT)
    else if (p_dev->role == BLE_MASTER) {
        ble_datatrans_cli_rx_cb_reg(at_ble_passth_rx_callback);
        ble_gattc_co_cb_unreg(at_ble_gattc_co_cb);
    }
#endif
    if (!tx_buf)
        goto Error;

#ifndef SUPER_UART_DMA_RX
    while (1) {
        if(reset) {
            at_hw_dma_receive_config();   //have to reconfig hw here, or one byte left data will be transfered by dma
#ifndef CONFIG_ATCMD_SPI
            while(RESET == usart_flag_get(at_uart_conf.usart_periph, USART_FLAG_IDLE));
            usart_flag_clear(at_uart_conf.usart_periph, USART_FLAG_IDLE);
#endif
            reset = false;
            sys_memset(tx_buf, 0, ATBLE_PASSTH_MAX_SIZE);
            at_hw_dma_receive_start((uint32_t)tx_buf, 0, ATBLE_PASSTH_MAX_SIZE);
        }

        sys_ms_sleep(1);

        if (at_ble_cb.disconn_flag == true) {
            at_ble_cb.disconn_flag = false;
            break;
        }
#ifdef CONFIG_ATCMD_SPI
        if (RESET == spi_flag_get( SPI_FLAG_RBNE))
#else
        if (RESET != usart_flag_get(at_uart_conf.usart_periph, USART_FLAG_IDLE))
#endif
        {
#ifndef CONFIG_ATCMD_SPI
            usart_flag_clear(at_uart_conf.usart_periph, USART_FLAG_IDLE);
#endif
            cur_cnt = at_dma_get_cur_received_num(ATBLE_PASSTH_MAX_SIZE);
            if (cur_cnt == 0)
                continue;

            reset = true;
            at_hw_dma_receive_stop();

            if (at_ble_terminate_string_check(tx_buf))
                break;

            if (p_dev->role == BLE_SLAVE) {
                if (ble_datatrans_srv_tx(0, tx_buf, cur_cnt) != BLE_ERR_NO_ERROR)
                    AT_TRACE("data send fail\r\n");
            }
#if (BLE_APP_GATT_CLIENT_SUPPORT)
            else if (p_dev->role == BLE_MASTER) {
                if (ble_datatrans_cli_write_char(0, tx_buf, cur_cnt) != BLE_ERR_NO_ERROR)
                    AT_TRACE("data send fail\r\n");
            }
#endif
        }
    }
    at_hw_dma_receive_stop();
    at_hw_irq_receive_config();
    sys_mfree(tx_buf);

    if (p_dev->role == BLE_SLAVE) {
        ble_datatrans_srv_rx_cb_unreg();
    }
#if (BLE_APP_GATT_CLIENT_SUPPORT)
    else if (p_dev->role == BLE_MASTER) {
        ble_datatrans_cli_rx_cb_unreg();
        ble_gattc_co_cb_reg(at_ble_gattc_co_cb);
    }
#endif
#else
    while (1) {
        if(reset) {
            reset = false;
            sys_memset(tx_buf, 0, ATBLE_PASSTH_MAX_SIZE);
        }
        cur_cnt = at_hw_dma_receive_start((uint32_t)tx_buf, ATBLE_PASSTH_MAX_SIZE, 5);

        if (at_ble_cb.disconn_flag == true) {
            at_ble_cb.disconn_flag = false;
            break;
        }

        if (cur_cnt == 0) {
            continue;
        }
        else {
            reset = true;

            if (at_ble_terminate_string_check(tx_buf))
                break;

            if (ble_datatrans_srv_tx(conn_idx, tx_buf, cur_cnt) != BLE_ERR_NO_ERROR) {
                AT_TRACE("data send fail\r\n");
            }
        }
    }

    at_hw_dma_receive_stop();
    sys_mfree(tx_buf);
    ble_datatrans_srv_rx_cb_unreg();
#endif

    return;

Error:
    if (tx_buf)
        sys_mfree(tx_buf);

    if (p_dev) {
        if (p_dev->role == BLE_SLAVE) {
            ble_datatrans_srv_rx_cb_unreg();
        }
#if (BLE_APP_GATT_CLIENT_SUPPORT)
        else if (p_dev->role == BLE_MASTER) {
            ble_datatrans_cli_rx_cb_unreg();
            ble_gattc_co_cb_reg(at_ble_gattc_co_cb);
        }
#endif
    }
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEPASSTH=<conn_idx>\r\n");
    AT_RSP_OK();
    return;
}

/*!
    \brief      nornal trans mode server rx callback
    \param[in]  conn_idx: connection index
    \param[in]  data_len: data_length
    \param[in]  p_data: pointer to data to handle
    \param[out] none
    \retval     none
*/
void at_ble_normal_trans_rx_callback(uint8_t conn_id, uint16_t data_len, uint8_t *p_data)
{
    uint32_t j;

    AT_RSP_START(64 + data_len);
    AT_RSP("+BLEDATA:%d,%d,", conn_id, data_len);
    AT_RSP_IMMEDIATE();
    AT_RSP_DIRECT((char *)p_data, data_len);
    AT_RSP("\r\n");
    AT_RSP_OK();
}

/*!
    \brief      Enable noraml trans  mode
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ble_normal_trans_mode_enable(int argc, char **argv)
{

    char *endptr = NULL;
    uint8_t enable = 0;

    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else {
            enable = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
            if (enable) {
                ble_datatrans_srv_rx_cb_reg(at_ble_normal_trans_rx_callback);
            } else {
                ble_datatrans_srv_rx_cb_unreg();
            }
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEDADATRANS=<enable>\r\n");
    AT_RSP_OK();
}

/*!
    \brief      noraml trans mode send data
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ble_normal_trans_mode_send(int argc, char **argv)
{
    uint8_t conn_idx = 0;
    uint8_t tx_len = 0;
    char *endptr = NULL;
    uint8_t *tx_buf = NULL;

    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;
    } else if (argc == 3) {
        conn_idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
        tx_len = (uint16_t)strtoul((const char *)argv[2], &endptr, 10);

        tx_buf = sys_malloc(tx_len);
        if (NULL == tx_buf) {
            AT_TRACE("buffer failed (len = %u).\r\n", tx_len);
            goto Error;
        }
        AT_RSP(">\r\n");
        AT_RSP_IMMEDIATE();
        at_hw_dma_receive((uint32_t)tx_buf, tx_len);
        if (ble_datatrans_srv_tx(0, tx_buf, tx_len) != BLE_ERR_NO_ERROR) {
            AT_TRACE("data send fail\r\n");
        }

        sys_mfree(tx_buf);
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEDADATRANSSEND=<conn_idx>,<tx_len>\r\n");
    AT_RSP_OK();
}


/*!
    \brief      Auto enable passthrough mode
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ble_passth_mode_auto_enable(int argc, char **argv)
{
    ble_status_t ret = BLE_ERR_NO_ERROR;
    char *endptr = NULL;
    uint8_t enable = 0;

    AT_RSP_START(128);

    if (argc != 2) {
        goto Error;
    }

    if (argv[1][0] == AT_QUESTION) {
        goto Usage;
    }

    enable = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
    if (enable) {
        at_ble_cb.passth_auto_enable_flag = 1;
    } else {
        at_ble_cb.passth_auto_enable_flag = 0;
    }

    AT_RSP_OK();
    return;

Usage:
    AT_RSP("+BLEPASSTHAUTO=<enable>\r\n");
    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();

}

void at_ble_gatts_ntf(int argc, char **argv)
{
    uint8_t conn_idx = 0;
    uint8_t svc_id = 0;
    uint8_t char_idx = 0;
    uint8_t tx_len = 0;
    char *endptr = NULL;
    ble_status_t ret = BLE_ERR_NO_ERROR;
    uint8_t *tx_buf = NULL;

    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;
    } else if (argc == 5) {
        conn_idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
        svc_id = (uint8_t)strtoul((const char *)argv[2], &endptr, 10);
        char_idx = (uint8_t)strtoul((const char *)argv[3], &endptr, 10);
        tx_len = (uint16_t)strtoul((const char *)argv[4], &endptr, 10);

        tx_buf = sys_malloc(tx_len);
        if (NULL == tx_buf) {
            AT_TRACE("buffer failed (len = %u).\r\n", tx_len);
            goto Error;
        }
        AT_RSP(">\r\n");
        AT_RSP_IMMEDIATE();
        at_hw_dma_receive((uint32_t)tx_buf, tx_len);
        ret = ble_gatts_ntf_ind_send(conn_idx, svc_id, char_idx, tx_buf, tx_len, BLE_GATT_NOTIFY);
        if (ret)
            AT_TRACE("Notification send fail\r\n", tx_len);
        sys_mfree(tx_buf);
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEGATTSNTF=<conn_idx>,<svc_id>,<char_idx>,<tx_len>\r\n");
    AT_RSP_OK();
}

void at_ble_gatts_ind(int argc, char **argv)
{
    uint8_t conn_idx = 0;
    uint8_t svc_id = 0;
    uint8_t char_idx = 0;
    uint8_t tx_len = 0;
    char *endptr = NULL;
    ble_status_t ret = BLE_ERR_NO_ERROR;
    uint8_t *tx_buf = NULL;

    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;
    } else if (argc == 5) {
        conn_idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
        svc_id = (uint8_t)strtoul((const char *)argv[2], &endptr, 10);
        char_idx = (uint8_t)strtoul((const char *)argv[3], &endptr, 10);
        tx_len = (uint16_t)strtoul((const char *)argv[4], &endptr, 10);

        tx_buf = sys_malloc(tx_len);
        if (NULL == tx_buf) {
            AT_TRACE("buffer failed (len = %u).\r\n", tx_len);
            goto Error;
        }
        AT_RSP(">\r\n");
        AT_RSP_IMMEDIATE();
        at_hw_dma_receive((uint32_t)tx_buf, tx_len);
        ret = ble_gatts_ntf_ind_send(conn_idx, svc_id, char_idx, tx_buf, tx_len, BLE_GATT_INDICATE);
        if (ret)
            AT_TRACE("Indication send fail\r\n", tx_len);
        sys_mfree(tx_buf);
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEGATTSIND=<conn_idx>,<svc_id>,<char_idx>,<tx_len>\r\n");
    AT_RSP_OK();
}

void at_ble_gatts_set_attr_val(int argc, char **argv)
{
    uint8_t svc_id = 0;
    uint8_t char_idx = 0;
    uint8_t tx_len = 0;
    char *endptr = NULL;
    ble_status_t ret = BLE_ERR_NO_ERROR;
    uint8_t *tx_buf = NULL;
    uint8_t conn_idx = 0;

    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;
    } else if (argc == 5) {
        conn_idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
        svc_id = (uint8_t)strtoul((const char *)argv[2], &endptr, 10);
        char_idx = (uint8_t)strtoul((const char *)argv[3], &endptr, 10);
        tx_len = (uint16_t)strtoul((const char *)argv[4], &endptr, 10);

        tx_buf = sys_malloc(tx_len);
        if (NULL == tx_buf) {
            AT_TRACE("buffer failed (len = %u).\r\n", tx_len);
            goto Error;
        }
        AT_RSP(">\r\n");
        AT_RSP_IMMEDIATE();
        at_hw_dma_receive((uint32_t)tx_buf, tx_len);
        ret = ble_gatts_set_attr_val(conn_idx, svc_id, char_idx, tx_len, tx_buf);
        sys_mfree(tx_buf);
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEGATTSSETATTRVAL=<conn_idx>,<svc_id>,<char_idx>,<tx_len>\r\n");
    AT_RSP_OK();
}

#ifdef CFG_WLAN_SUPPORT
void at_ble_courier_wifi(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t enable;
    ble_status_t ret;
    AT_RSP_START(128);

    if (argc != 2) {
        goto Error;
    }

    if (argv[1][0] == AT_QUESTION) {
        goto Usage;
    }

    enable = (uint8_t)strtoul((const char *)argv[1], &endptr, 16);

    if (enable) {
        if (ble_work_status_get() != BLE_WORK_STATUS_ENABLE) {
            app_ble_enable();
        }
        ret = bcw_prf_enable(enable);
    } else {
        ret = bcw_prf_enable(enable);

#ifndef CONFIG_BLE_ALWAYS_ENABLE
        app_ble_disable();
#endif
    }

    if (ret)
        goto Error;

    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLECOURIER=<enable>\r\n");
    AT_RSP_OK();

}
#endif


/*!
    \brief      Callback function to handle BLE connection events
    \param[in]  event: BLE connection event type
    \param[in]  p_data: pointer to BLE connection event data
    \param[out] none
    \retval     none
*/
void at_ble_conn_evt_handler(ble_conn_evt_t event, ble_conn_data_u *p_data)
{
    char cmd_passth_srv[] = {"AT+BLEPASSTH=0"};
    char cmd_passth_cli[] = {"AT+BLEPASSTHCLI=0"};
    AT_RSP_START(128);

    if (event == BLE_CONN_EVT_STATE_CHG) {
        if (p_data->conn_state.state == BLE_CONN_STATE_DISCONNECTD) {
            AT_RSP("+BLEDISCONN:%d,%d\r\n", p_data->conn_state.info.discon_info.conn_idx, p_data->conn_state.info.discon_info.reason);
            AT_RSP_IMMEDIATE();
            at_ble_cb.disconn_flag = true;
        }
        else if (p_data->conn_state.state == BLE_CONN_STATE_CONNECTED) {
            AT_RSP("+BLECONN:%d,%d,%02x:%02x:%02x:%02x:%02x:%02x\r\n", p_data->conn_state.info.conn_info.conn_idx, p_data->conn_state.info.conn_info.peer_addr.addr_type,
                    p_data->conn_state.info.conn_info.peer_addr.addr[5], p_data->conn_state.info.conn_info.peer_addr.addr[4],
                    p_data->conn_state.info.conn_info.peer_addr.addr[3], p_data->conn_state.info.conn_info.peer_addr.addr[2],
                    p_data->conn_state.info.conn_info.peer_addr.addr[1], p_data->conn_state.info.conn_info.peer_addr.addr[0]);
            AT_RSP_IMMEDIATE();
            at_ble_cb.disconn_flag = false;
#ifdef BLE_GATT_CLIENT_SUPPORT
            ble_gattc_mtu_update(0, ATBLE_PASSTH_MAX_SIZE);
#endif
            if (at_ble_cb.passth_auto_enable_flag) {
                if (p_data->conn_state.info.conn_info.role == 1) {
                    at_hw_fill_rx_buf(cmd_passth_srv, strlen(cmd_passth_srv) + 1);
                } else if(p_data->conn_state.info.conn_info.role == 0) {
                    at_hw_fill_rx_buf(cmd_passth_cli, strlen(cmd_passth_cli) + 1);
                }
            }
        }

    }

    AT_RSP_FREE();
}

/*!
    \brief      Enable ble
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ble_enable(int argc, char **argv)
{
    char *endptr = NULL;
    char *p_str;

    AT_RSP_START(128);

    if (argc > 1) {
        goto Error;
    }

    app_ble_enable();
    AT_RSP_OK();

    return;

Error:
    AT_RSP_ERR();
    return;

}

/*!
    \brief      Disable ble
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ble_disable(int argc, char **argv)
{
    char *endptr = NULL;
    char *p_str;

    AT_RSP_START(128);

    if (argc > 1) {
        goto Error;
    }

    app_ble_disable();

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;

}

/*!
    \brief      Ble start adv
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ble_adv_start(int argc, char **argv)
{
    char *endptr = NULL;
    app_adv_param_t adv_param = {0};
    char *p_str = NULL;
    ble_status_t ret = BLE_ERR_NO_ERROR;

    AT_RSP_START(128);

    if (argc == 1) {
        goto Error;
    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
    } else if (argc > 12) {
        goto Error;
    }

    adv_param.type = DEFAULT_ADV_TYPE;
    adv_param.adv_intv = APP_ADV_INT_MAX;
    adv_param.max_data_len = 0x1F;
    adv_param.ch_map = BLE_GAP_ADV_CHANN_37 | BLE_GAP_ADV_CHANN_38 | BLE_GAP_ADV_CHANN_39;
    adv_param.prop = 0;
    adv_param.pri_phy = BLE_GAP_PHY_1MBPS;
    adv_param.sec_phy = BLE_GAP_PHY_1MBPS;
    adv_param.wl_enable = false;
    adv_param.own_addr_type = BLE_GAP_LOCAL_ADDR_STATIC;
    adv_param.disc_mode = BLE_GAP_ADV_MODE_GEN_DISC;

    adv_param.type = (uint8_t)strtoul((const char *)argv[1], &endptr, 16);
    if (adv_param.type == BLE_ADV_TYPE_LEGACY) {
        adv_param.prop = BLE_GAP_ADV_PROP_UNDIR_CONN;           // 0x0003,scannable connectable undirected
    } else if (adv_param.type == BLE_ADV_TYPE_EXTENDED) {
        adv_param.prop = BLE_GAP_EXT_ADV_PROP_CONN_UNDIRECT;    // 0x0001,connectable undirected
    } else {
        adv_param.prop = BLE_GAP_PER_ADV_PROP_UNDIRECT;         // 0x0000, undirected periodic adv
    }

    if (argc > 2) {
        adv_param.adv_intv = (uint32_t)strtoul((const char *)argv[2], &endptr, 16);
    }

    if (argc > 3) {
        adv_param.ch_map = (uint8_t)strtoul((const char *)argv[3], &endptr, 16);
    }

    if (argc > 4) {
        adv_param.prop = (uint16_t)strtoul((const char *)argv[4], &endptr, 16);
    }

    if (argc > 5) {
        adv_param.pri_phy = (uint8_t)strtoul((const char *)argv[5], &endptr, 16);
    }

    if (argc > 6) {
        adv_param.sec_phy = (uint8_t)strtoul((const char *)argv[6], &endptr, 16);
    }

    if (argc > 7) {
        adv_param.wl_enable = (bool)strtoul((const char *)argv[7], &endptr, 16);
    }

    if (argc > 8) {
        adv_param.own_addr_type = (uint8_t)strtoul((const char *)argv[8], &endptr, 16);
    }

    if (argc > 9) {
        adv_param.disc_mode = (uint8_t)strtoul((const char *)argv[9], &endptr, 16);
    }

    if (argc > 11) {
        adv_param.peer_addr.addr_type = (uint8_t)strtoul((const char *)argv[10], &endptr, 16);

        p_str = argv[11];
        adv_param.peer_addr.addr[5] = (uint8_t)strtoul((const char *)strtok(p_str, ":"), &endptr, 16);
        adv_param.peer_addr.addr[4] = (uint8_t)strtoul((const char *)strtok(NULL, ":"), &endptr, 16);
        adv_param.peer_addr.addr[3] = (uint8_t)strtoul((const char *)strtok(NULL, ":"), &endptr, 16);
        adv_param.peer_addr.addr[2] = (uint8_t)strtoul((const char *)strtok(NULL, ":"), &endptr, 16);
        adv_param.peer_addr.addr[1] = (uint8_t)strtoul((const char *)strtok(NULL, ":"), &endptr, 16);
        adv_param.peer_addr.addr[0] = (uint8_t)strtoul((const char *)strtok(NULL, ":"), &endptr, 16);

        AT_TRACE("set peer addr to 0x%02x:%02x:%02x:%02x:%02x:%02x\r\n", adv_param.peer_addr.addr[5],
               adv_param.peer_addr.addr[4],
               adv_param.peer_addr.addr[3], adv_param.peer_addr.addr[2],
               adv_param.peer_addr.addr[1], adv_param.peer_addr.addr[0]);
    }

    ret = app_adv_create(&adv_param);
    if (ret) {
        AT_TRACE("adv start fail status 0x%x\r\n", ret);
        goto Error;
    }
    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEADVSTART=<type>,[intv],[ch_map],[prop],[pri_phy],[sec_phy],[wl_enable],[own_addr_type],[disc_mode],[addr_type],[addr]\r\n");
    AT_RSP_OK();
}

/*!
    \brief      Ble stop adv
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ble_adv_stop(int argc, char **argv)
{
    char *endptr = NULL;
    bool rmv_adv = true;
    ble_status_t ret = BLE_ERR_NO_ERROR;

    AT_RSP_START(128);

    if (argc > 1) {
        goto Error;
    }

    ret = app_adv_stop(0, rmv_adv);
    if (ret) {
        AT_TRACE("stop adv fail status 0x%x\r\n", ret);
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
}

/*!
    \brief      Ble set/get device name
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ble_name(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t *p_dev_name = NULL;
    bool ret = false;

    AT_RSP_START(128);

    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            app_adp_get_name(&p_dev_name);
            AT_RSP("+BLENAME:%s\r\n", p_dev_name);
        } else {
            goto Error;
        }
    } else if (argc == 2) {
         if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        }

        ret = app_adp_set_name(argv[1], strlen(argv[1]));
        if (!ret) {
            AT_TRACE("set device name fail status 0x%x\r\n", ret);
            goto Error;
        } else {
            app_adv_data_update_all();
            AT_TRACE("set device name to %s\r\n", argv[1]);
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLENAME=<name>\r\n");
    AT_RSP_OK();
}

void at_ble_bd_addr(int argc, char **argv)
{
    char *endptr = NULL;
    bool ret = false;
    uint8_t bd_addr[6] = {0};
    char *p_str = NULL;
    AT_RSP_START(128);

    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            if (ble_adp_public_addr_get(bd_addr)) {
                AT_TRACE("no bd addr in flash, use defalt value or bd addr in efuse 0x%x\r\n", ret);
                goto Error;
            } else {
                AT_RSP("+BLEBDADDR:%02x:%02x:%02x:%02x:%02x:%02x\r\n", bd_addr[5], bd_addr[4], bd_addr[3], bd_addr[2], bd_addr[1], bd_addr[0]);
            }
        } else {
            goto Error;
        }
    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        }

        p_str = argv[1];
        bd_addr[5] = (uint8_t)strtoul((const char *)strtok(p_str, ":"), &endptr, 16);
        bd_addr[4] = (uint8_t)strtoul((const char *)strtok(NULL, ":"), &endptr, 16);
        bd_addr[3] = (uint8_t)strtoul((const char *)strtok(NULL, ":"), &endptr, 16);
        bd_addr[2] = (uint8_t)strtoul((const char *)strtok(NULL, ":"), &endptr, 16);
        bd_addr[1] = (uint8_t)strtoul((const char *)strtok(NULL, ":"), &endptr, 16);
        bd_addr[0] = (uint8_t)strtoul((const char *)strtok(NULL, ":"), &endptr, 16);

        ret = ble_adp_public_addr_set(bd_addr);

        if (ret) {
            AT_TRACE("set bd addr fail status 0x%x\r\n", ret);
            goto Error;
        } else {
            AT_TRACE("set bd addr to 0x%02x:%02x:%02x:%02x:%02x:%02x\r\n", bd_addr[5], bd_addr[4], bd_addr[3], bd_addr[2], bd_addr[1], bd_addr[0]);
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEBDADDR=<bd_addr>\r\n");
    AT_RSP_OK();
}



/*!
    \brief      Initialize ble atcmd
    \param[in]  none
    \param[out] none
    \retval     none
*/
int atcmd_ble_init(void)
{
    if (ble_datatrans_srv_init())
        return -1;
    if (ble_conn_callback_register(at_ble_conn_evt_handler))
        return -2;
#ifdef BLE_GATT_CLIENT_SUPPORT
    if (ble_gattc_co_cb_reg(at_ble_gattc_co_cb))
        return -3;
    if (ble_datatrans_cli_init())
        return -4;
#endif
    app_sec_pin_code_set(888888);
    app_sec_callbacks_set(at_ble_sec_cb);
    return 0;
}

/*!
    \brief      Deinitialize ble atcmd
    \param[in]  none
    \param[out] none
    \retval     none
*/
int atcmd_ble_deinit(void)
{
    if (ble_datatrans_srv_deinit())
        return -1;
    if (ble_conn_callback_unregister(at_ble_conn_evt_handler))
        return -2;
#ifdef BLE_GATT_CLIENT_SUPPORT
    if (ble_gattc_co_cb_unreg(at_ble_gattc_co_cb))
        return -3;
    if (ble_datatrans_cli_deinit())
        return -4;
#endif

    return 0;
}

/*!
    \brief      Ble set adv data
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ble_adv_data(int argc, char **argv)
{
    ble_status_t ret = BLE_ERR_NO_ERROR;
    uint16_t len = 0;
    uint8_t adv_data[BLE_GAP_LEGACY_ADV_MAX_LEN] = {0};

    AT_RSP_START(128);

    if (argc != 2) {
        goto Error;
    }

    if (argv[1][0] == AT_QUESTION) {
        goto Usage;
    }

    if (strlen(argv[1]) % 2 == 0 && (strlen(argv[1]) - 2) > 0 && argv[1][strlen(argv[1]) - 1] == '"' && argv[1][0] == '"') {
        len = strlen(argv[1]) - 2;
        str2hex(&argv[1][1], len, adv_data, BLE_GAP_LEGACY_ADV_MAX_LEN);
        ret = app_adv_set_adv_data(adv_data, len / 2);
        if (ret) {
            AT_TRACE("set adv data fail status 0x%x\r\n", ret);
            goto Error;
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEADVDATA=<data>\r\n");
    AT_RSP_OK();
}

/*!
    \brief      Ble set adv data by type
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ble_adv_data_ex(int argc, char **argv)
{
    ble_status_t ret = BLE_ERR_NO_ERROR;;
    uint16_t len = 0;
    uint16_t idx = 0;
    uint8_t include_power = 0;
    uint8_t buf_temp[BLE_GAP_LEGACY_ADV_MAX_LEN] = {0};
    uint8_t adv_data[BLE_GAP_LEGACY_ADV_MAX_LEN] = {0};

    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;
    }

    if (argc != 5) {
        goto Error;
    }

    include_power = strtoul(argv[4], NULL, 10);
    if (include_power)
        len = strlen(argv[1]) + (strlen(argv[2]) -2) / 2 + 2 + (strlen(argv[3]) -2) / 2 + 2 + 3 + 3;
    else
        len = strlen(argv[1]) + (strlen(argv[2]) / 2) + (strlen(argv[3]) / 2) + 3;

    if (len > BLE_GAP_LEGACY_ADV_MAX_LEN)
        goto Error;

    if (len > BLE_GAP_LEGACY_ADV_MAX_LEN
        || argv[1][strlen(argv[1]) - 1] != '"' || argv[1][0] != '"'
        || argv[2][strlen(argv[2]) - 1] != '"' || argv[2][0] != '"'
        || argv[3][strlen(argv[3]) - 1] != '"' || argv[3][0] != '"')
        goto Error;

    adv_data[idx++] = 02;
    adv_data[idx++] = BLE_AD_TYPE_FLAGS;
    adv_data[idx++] = BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE | BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;

    adv_data[idx++] = strlen(argv[1]) + 1 - 2;
    adv_data[idx++] = BLE_AD_TYPE_COMPLETE_LOCAL_NAME;
    sys_memcpy(&adv_data[idx], &argv[1][1], strlen(argv[1]) - 2);

    idx += (strlen(argv[1]) - 2);
    adv_data[idx++] = (strlen(argv[2]) - 2) / 2 + 1;
    adv_data[idx++] = BLE_AD_TYPE_SERVICE_UUID_16_COMPLETE;
    str2hex(&argv[2][1], strlen(argv[2])-2, buf_temp, BLE_GAP_LEGACY_ADV_MAX_LEN);
    adv_data[idx++] = buf_temp[1];
    adv_data[idx++] = buf_temp[0];

    adv_data[idx++] = (strlen(argv[3]) - 2) / 2 + 1;
    adv_data[idx++] = BLE_AD_TYPE_MANUFACTURER_SPECIFIC_DATA;
    str2hex(&argv[3][1], strlen(argv[3])-2, buf_temp, BLE_GAP_LEGACY_ADV_MAX_LEN);
    sys_memcpy(&adv_data[idx], buf_temp, (strlen(argv[3])-2) / 2);
    idx += (strlen(argv[3])-2) / 2;

    if (include_power) {
        adv_data[idx++] = 2;
        adv_data[idx++] = BLE_AD_TYPE_TX_POWER_LEVEL;
        adv_data[idx++] = 0;
    }

    ret = app_adv_set_adv_data(adv_data, len);
    if (ret) {
        AT_TRACE("set adv data fail status 0x%x\r\n", ret);
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEADVDATA=<dev_name>,<uuid>,<manufacturer_data>,<include_power>\r\n");
    AT_RSP_OK();
}

/*!
    \brief      Ble set scan rsp data
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ble_scan_rsp_data(int argc, char **argv)
{
    uint16_t len = 0;
    uint8_t scan_rsp[BLE_GAP_LEGACY_ADV_MAX_LEN] = {0};
    ble_status_t ret = BLE_ERR_NO_ERROR;

    AT_RSP_START(128);

    if (argc != 2) {
        goto Error;
    }

    if (argv[1][0] == AT_QUESTION) {
        goto Usage;
    }

    if (strlen(argv[1]) % 2 == 0 && (strlen(argv[1]) - 2) > 0 && argv[1][strlen(argv[1]) - 1] == '"' && argv[1][0] == '"') {
        len = (strlen(argv[1])-2);
        str2hex(&argv[1][1], len, scan_rsp, BLE_GAP_LEGACY_ADV_MAX_LEN);
        ret = app_adv_set_scan_rsp_data(scan_rsp, len / 2);
        if (ret) {
            AT_TRACE("set scan rsp data fail status 0x%x\r\n", ret);
            goto Error;
        }
    } else {
        goto Error;
    }


    AT_RSP_OK();
    return;

Usage:
    AT_RSP("+BLESCANRSPDATA=<data>\r\n");
    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
}

/*!
    \brief      Ble set/get connect parameter
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ble_conn_param(int argc, char **argv)
{
    ble_device_t *p_device = NULL;
    uint8_t i = 0;
    char *endptr = NULL;
    uint8_t conn_idx = 0;
    uint16_t interval = 0;
    uint16_t latency = 0;
    uint16_t supv_to = 0;
    uint16_t ce_len = 0;
    ble_status_t ret = BLE_ERR_NO_ERROR;

    AT_RSP_START(128);

    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            for (i = 0; i < BLE_MAX_CONN_NUM; i++) {
                p_device = dm_find_dev_by_conidx(i);
                if (p_device)
                    AT_RSP("+BLECONNPARM:%d,%x,%x,%x\r\n",
                            p_device->conn_idx, p_device->conn_info.interval, p_device->conn_info.latency, p_device->conn_info.supv_tout);
            }
        } else {
            goto Error;
        }
    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;
    } else if (argc == 5) {
        conn_idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
        interval = (uint16_t)strtoul((const char *)argv[2], &endptr, 16);
        latency  = (uint16_t)strtoul((const char *)argv[3], &endptr, 16);
        supv_to  = (uint16_t)strtoul((const char *)argv[4], &endptr, 16);
        ret = ble_conn_param_update_req(conn_idx, interval, interval, latency, supv_to, ce_len, ce_len);
        if (ret) {
            AT_TRACE("update param fail status 0x%x\r\n", ret);
            goto Error;
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLECONNPARM=<conn_idx>,<interval>,<latency>,<supv_to>\r\n");
    AT_RSP_OK();
}

/*!
    \brief      Ble disconnect
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ble_dis_conn(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t conn_idx = 0;
    ble_status_t ret = BLE_ERR_NO_ERROR;
    AT_RSP_START(128);

    if (argc != 2) {
        goto Error;
    }

    if (argv[1][0] == AT_QUESTION) {
        goto Usage;
    }

    conn_idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);

    ret = ble_conn_disconnect(conn_idx, BLE_ERROR_HL_TO_HCI(BLE_LL_ERR_REMOTE_USER_TERM_CON));
    if (ret) {
        AT_TRACE("disconnect connection fail status 0x%x\r\n", ret);
        goto Error;
    }

    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEDISCONN=<conn_idx>\r\n");
    AT_RSP_OK();

}

/*!
    \brief      Ble data length extension
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ble_data_len(int argc, char **argv)
{
    char *endptr = NULL;
    uint16_t tx_oct = 0;
    uint8_t conn_idx = 0xff;
    ble_status_t ret = BLE_ERR_NO_ERROR;

    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;
    } else if (argc == 3) {
        conn_idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
        tx_oct = (uint16_t)strtoul((const char *)argv[2], &endptr, 10);
    } else {
        goto Error;
    }

    ret = ble_conn_pkt_size_set(conn_idx, tx_oct, 17040);
    if (ret) {
        AT_TRACE("set pkt size fail status 0x%x\r\n", ret);
        goto Error;
    }
    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEDATALEN=<conn_idx>,<tx_oct>\r\n");
    AT_RSP_OK();
}

/*!
    \brief      Ble get/set mtu
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ble_mtu(int argc, char **argv)
{
    ble_device_t *p_device = NULL;
    uint8_t i = 0;
    char *endptr = NULL;
    uint8_t conn_idx = 0xff;
    ble_status_t ret = BLE_ERR_NO_ERROR;
    uint16_t mtu = 0;

    AT_RSP_START(128);

    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            for (i = 0; i < 10; i++) {
                p_device = dm_find_dev_by_conidx(i);
                if (p_device) {
                    ble_gatts_mtu_get(i, &mtu);
                    AT_RSP("+BLEMTU:%d,%d\r\n", i, mtu);
                }
            }
        } else {
            goto Error;
        }
    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;
    } else if (argc == 3) {
#ifdef BLE_GATT_CLIENT_SUPPORT
        conn_idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
        mtu = (uint16_t)strtoul((const char *)argv[2], &endptr, 10);

        ret = ble_gattc_mtu_update(conn_idx, mtu);
        if (ret) {
            AT_TRACE("mtu exchange fail status 0x%x\r\n", ret);
            goto Error;
        }
#endif
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEMTU=<conn_idx>,<pref_mtu>\r\n");
    AT_RSP_OK();
}

/*!
    \brief      Ble get/set phy
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ble_phy(int argc, char **argv)
{
    ble_device_t *p_device = NULL;
    uint8_t i = 0;
    char *endptr = NULL;
    uint8_t conn_idx = 0;
    ble_status_t ret = BLE_ERR_NO_ERROR;
    uint8_t tx_phy = 0;
    uint8_t rx_phy = 0;
    uint8_t phy_opt = 0;

    AT_RSP_START(128);

    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            for (i = 0; i < BLE_MAX_CONN_NUM; i++) {
                p_device = dm_find_dev_by_conidx(i);
                if (p_device)
                    if (app_conn_phy_get(i, &tx_phy, &rx_phy))
                        AT_RSP("+BLEPHY:%d,%d,%d\r\n", i, tx_phy, rx_phy);
            }
        } else {
            goto Error;
        }
    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;
    } else if (argc == 5) {
        conn_idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
        tx_phy = (uint8_t)strtoul((const char *)argv[2], &endptr, 10);
        rx_phy = (uint8_t)strtoul((const char *)argv[3], &endptr, 10);
        phy_opt = (uint8_t)strtoul((const char *)argv[4], &endptr, 10);
        ret = ble_conn_phy_set(conn_idx, tx_phy, rx_phy, phy_opt);
        if (ret) {
            AT_TRACE("phy set fail status 0x%x\r\n", ret);
            goto Error;
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEPHY=<conn_idx>,<tx_phy>,<rx_phy>,<phy_opt>\r\n");
    AT_RSP_OK();
}

/*!
    \brief      periodic sync event handler
    \param[in]  event: event type
    \param[in]  p_data: the pointer to data
    \param[out] none
    \retval     none
*/
static void at_ble_per_sync_evt_handler(ble_per_sync_evt_t event, ble_per_sync_data_u *p_data)
{
    per_dev_info_t *p_sync_dev = NULL;
    AT_RSP_START(128);

    switch (event) {
    case BLE_PER_SYNC_EVT_STATE_CHG:
        break;

    case BLE_PER_SYNC_EVT_ESTABLISHED:
        at_ble_cb.sync_idx = p_data->establish.param.actv_idx;
        break;

    case BLE_PER_SYNC_EVT_REPORT: {
        ble_gap_adv_report_info_t  *p_report = p_data->report.p_report;
        p_sync_dev = sync_mgr_find_device_by_idx(p_report->actv_idx);
        if (p_sync_dev) {
            AT_RSP("+BLESYNC=%02X:%02X:%02X:%02X:%02X:%02X\r\n",
                   p_sync_dev->sync_info.addr[5], p_sync_dev->sync_info.addr[4], p_sync_dev->sync_info.addr[3],
                   p_sync_dev->sync_info.addr[2], p_sync_dev->sync_info.addr[1], p_sync_dev->sync_info.addr[0]);
            AT_RSP_IMMEDIATE();
            AT_RSP_FREE();
        }
    }
    break;

    default:
        break;
    }
}

/*!
    \brief      Ble set/get scan parameter
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ble_scan_param(int argc, char **argv)
{
    char *endptr = NULL;
    ble_gap_scan_param_t param = {0};
    ble_status_t ret = BLE_ERR_NO_ERROR;
    ble_gap_local_addr_type_t own_addr_type = BLE_GAP_LOCAL_ADDR_STATIC;

    AT_RSP_START(128);

    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            ble_scan_param_get(&own_addr_type, &param);
            AT_RSP("+BLESCANPARAM:%d,%d,%d,%x,%x,%x,%x\r\n", param.type, own_addr_type, param.dup_filt_pol, param.scan_intv_1m, param.scan_win_1m, param.scan_intv_coded, param.scan_win_coded);
            AT_RSP_OK();
            return;
        } else {
            goto Error;
        }
    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;
    } else if (argc == 8) {
        param.type = BLE_GAP_SCAN_TYPE_GEN_DISC;
        param.prop = BLE_GAP_SCAN_PROP_PHY_1M_BIT | BLE_GAP_SCAN_PROP_ACTIVE_1M_BIT;
#if BLE_APP_PHY_UPDATE_SUPPORT
        param.prop |= BLE_GAP_SCAN_PROP_PHY_CODED_BIT | BLE_GAP_SCAN_PROP_ACTIVE_CODED_BIT;
#endif
        param.dup_filt_pol = BLE_GAP_DUP_FILT_EN;
        param.scan_intv_1m    = 0xa0; // 100ms
        param.scan_intv_coded = 0xa0; // 100ms
        param.scan_win_1m    = 0x30;  // 30ms
        param.scan_win_coded = 0x30;  // 30ms
        param.duration = 0;
        param.period   = 0;
        own_addr_type = BLE_GAP_LOCAL_ADDR_STATIC;

        param.type = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
        own_addr_type = (uint8_t)strtoul((const char *)argv[2], &endptr, 10);
        param.dup_filt_pol = (uint8_t)strtoul((const char *)argv[3], &endptr, 10);
        param.scan_intv_1m = (uint8_t)strtoul((const char *)argv[4], &endptr, 16);
        param.scan_win_1m = (uint16_t)strtoul((const char *)argv[5], &endptr, 16);
        param.scan_intv_coded = (uint8_t)strtoul((const char *)argv[6], &endptr, 16);
        param.scan_win_coded = (uint16_t)strtoul((const char *)argv[7], &endptr, 16);

        ret = ble_scan_param_set(own_addr_type, &param);
        if (ret) {
            AT_TRACE("scan param set fail status 0x%x\r\n", ret);
            goto Error;
        }
    }else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLESCANPARAM=<type>,<own_addr_type>,<dup_filt_pol>,<scan_intv_1m>,<scan_win_1m>,<scan_intv_coded>,<scan_win_coded>\r\n");
    AT_RSP_OK();
}

/*!
    \brief      scan report handler
    \param[in]  p_info: pointer to information
    \param[out] none
    \retval     none
*/
static void at_ble_scan_mgr_report_hdlr(ble_gap_adv_report_info_t *p_info)
{
    uint8_t *p_name = NULL;
    uint8_t name_len;
    uint8_t name[31] = {'\0'};
    dev_info_t *p_dev_info = scan_mgr_find_device(&p_info->peer_addr);

    if (p_info->period_adv_intv) {
        #if BLE_APP_PER_ADV_SUPPORT
        ble_per_sync_mgr_find_alloc_device(&p_info->peer_addr, p_info->adv_sid, p_info->period_adv_intv);
        #endif
    }

    if (p_dev_info == NULL || p_dev_info->recv_name_flag == 0) {
        p_name = ble_adv_find(p_info->data.p_data, p_info->data.len, BLE_AD_TYPE_COMPLETE_LOCAL_NAME,
                              &name_len);
        if (p_name == NULL) {
            p_name = ble_adv_find(p_info->data.p_data, p_info->data.len, BLE_AD_TYPE_SHORT_LOCAL_NAME,
                                  &name_len);
        }

        if (p_name) {
            memcpy(name, p_name, name_len > 30 ? 30 : name_len);
        }

        if (p_dev_info == NULL) {
            uint8_t idx = scan_mgr_add_device(&p_info->peer_addr);
            AT_RSP_START(256);

            p_dev_info = scan_mgr_find_dev_by_idx(idx);
            p_dev_info->adv_sid = p_info->adv_sid;
            p_dev_info->idx = idx;

            AT_RSP("+BLESCAN: %02X:%02X:%02X:%02X:%02X:%02X,%d,%d,%d,%s\r\n",
                   p_info->peer_addr.addr[5], p_info->peer_addr.addr[4], p_info->peer_addr.addr[3],
                   p_info->peer_addr.addr[2], p_info->peer_addr.addr[1], p_info->peer_addr.addr[0],
                   p_info->peer_addr.addr_type, p_info->rssi, idx, name);
            AT_RSP_IMMEDIATE();
            AT_RSP_FREE();
            dbg_print(NOTICE, "new device addr %02X:%02X:%02X:%02X:%02X:%02X, addr type 0x%x, rssi %d, sid 0x%x, dev idx %u, peri_adv_int %u, name %s\r\n",
                       p_info->peer_addr.addr[5], p_info->peer_addr.addr[4], p_info->peer_addr.addr[3],
                       p_info->peer_addr.addr[2], p_info->peer_addr.addr[1], p_info->peer_addr.addr[0],
                       p_info->peer_addr.addr_type, p_info->rssi, p_info->adv_sid, idx, p_info->period_adv_intv, name);
        }

        p_dev_info->recv_name_flag = (p_name == NULL ? 0 : 1);
    }
}

/*!
    \brief      scan event handler
    \param[in]  event: event type
    \param[in]  p_data: the pointer to data
    \param[out] none
    \retval     none
*/
void at_ble_scan_mgr_evt_handler(ble_scan_evt_t event, ble_scan_data_u *p_data)
{
    switch (event) {
    case BLE_SCAN_EVT_STATE_CHG:
        if (p_data->scan_state.scan_state == BLE_SCAN_STATE_ENABLED) {
            dbg_print(NOTICE, "Ble Scan enabled status 0x%x\r\n", p_data->scan_state.reason);
        }

        else if (p_data->scan_state.scan_state == BLE_SCAN_STATE_ENABLING) {
            scan_mgr_clear_dev_list();
        }

        else if (p_data->scan_state.scan_state == BLE_SCAN_STATE_DISABLED) {
            dbg_print(NOTICE, "Ble Scan disabled status 0x%x\r\n", p_data->scan_state.reason);
        }
        break;

    case BLE_SCAN_EVT_ADV_RPT:
        at_ble_scan_mgr_report_hdlr(p_data->p_adv_rpt);
        break;

    default:
        break;
    }
}

/*!
    \brief      Ble scan
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ble_scan(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t enable = 0;

    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else {
            enable = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
            if (enable) {
                ble_scan_callback_register(at_ble_scan_mgr_evt_handler);
                ble_scan_callback_unregister(ble_app_scan_mgr_evt_handler);
                app_scan_enable(0);
            } else {
                ble_scan_callback_unregister(at_ble_scan_mgr_evt_handler);
                ble_scan_callback_register(ble_app_scan_mgr_evt_handler);
                app_scan_disable();
            }
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLESCAN=<enable>\r\n");
    AT_RSP_OK();
}

/*!
    \brief      Ble conn
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ble_conn(int argc, char **argv)
{
    char *endptr = NULL;
    ble_gap_addr_t peer_addr = {0};
    ble_status_t ret = BLE_ERR_NO_ERROR;
    char *p_str = NULL;
    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;
    } else if (argc == 3) {
        peer_addr.addr_type = (uint8_t)strtoul((const char *)argv[1], &endptr, 16);

        p_str = argv[2];
        peer_addr.addr[5] = (uint8_t)strtoul((const char *)strtok(p_str, ":"), &endptr, 16);
        peer_addr.addr[4] = (uint8_t)strtoul((const char *)strtok(NULL, ":"), &endptr, 16);
        peer_addr.addr[3] = (uint8_t)strtoul((const char *)strtok(NULL, ":"), &endptr, 16);
        peer_addr.addr[2] = (uint8_t)strtoul((const char *)strtok(NULL, ":"), &endptr, 16);
        peer_addr.addr[1] = (uint8_t)strtoul((const char *)strtok(NULL, ":"), &endptr, 16);
        peer_addr.addr[0] = (uint8_t)strtoul((const char *)strtok(NULL, ":"), &endptr, 16);

        ret = ble_conn_connect(NULL, BLE_GAP_LOCAL_ADDR_STATIC, &peer_addr, false);
        if ( ret != BLE_ERR_NO_ERROR) {
            AT_TRACE("connect fail status 0x%x\r\n", ret);
        }
    } else  {
        goto Error;
    }

    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLECONN=<addr_type>,<addr>\r\n");
    AT_RSP_OK();
}

void at_ble_svc_list_cb(uint8_t svc_id, const uint8_t *p_svc_uuid, uint8_t svc_type, uint8_t svc_info)
{
    int8_t i = 0;
    AT_RSP_START(256);
    uint8_t uuid_len = 0;

    AT_RSP("+BLEGATTSSVC:%d,",svc_id);

    if ((svc_info & BLE_GATT_SVC_UUID_TYPE_MASK) == SVC_UUID(16)) {
        uuid_len = 2;
    } else if ((svc_info & BLE_GATT_SVC_UUID_TYPE_MASK) == SVC_UUID(32)) {
        uuid_len = 4;
    } else if ((svc_info & BLE_GATT_SVC_UUID_TYPE_MASK) == SVC_UUID(128)) {
        uuid_len = 16;
    }

    for (i = uuid_len -1; i >= 0; i--)
        AT_RSP("%02X", p_svc_uuid[i]);
    AT_RSP(",%d\r\n",svc_type);
    AT_RSP_IMMEDIATE();
    AT_RSP_FREE();
}

void at_ble_gatts_list_svc(int argc, char **argv)
{
    AT_RSP_START(128);

    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            ble_gatts_list_svc(at_ble_svc_list_cb);
            AT_RSP_OK();
        } else {
            goto Error;
        }
    } else {
        goto Error;
    }

    return;
Error:
    AT_RSP_ERR();
    return;

}

void at_ble_desc_list_all_cb(uint8_t svc_id, const uint8_t *p_desc_uuid, uint16_t desc_idx, uint16_t char_info)
{
    int8_t i = 0;
    uint8_t uuid_len = 0;
    AT_RSP_START(256);

    AT_RSP("+BLEGATTSDESC:");
    AT_RSP(",%d",svc_id);
    AT_RSP(",%d,",desc_idx);
    if (((char_info & BLE_GATT_ATTR_UUID_TYPE_MASK) >> BLE_GATT_ATTR_UUID_TYPE_LSB) == BLE_GATT_UUID_16) {
        uuid_len = 2;
    } else if (((char_info & BLE_GATT_ATTR_UUID_TYPE_MASK) >> BLE_GATT_ATTR_UUID_TYPE_LSB) == BLE_GATT_UUID_32) {
        uuid_len = 4;
    } else if (((char_info & BLE_GATT_ATTR_UUID_TYPE_MASK) >> BLE_GATT_ATTR_UUID_TYPE_LSB) == BLE_GATT_UUID_128) {
        uuid_len = 16;
    }
    for (i = uuid_len -1; i >= 0; i--)
        AT_RSP("%02X", p_desc_uuid[i]);
    AT_RSP("\r\n");
    AT_RSP_IMMEDIATE();
    AT_RSP_FREE();
}

void at_ble_char_list_all_cb(uint8_t svc_id, const uint8_t *p_char_uuid, uint16_t char_val_idx, uint16_t char_info)
{
    int8_t i = 0;
    uint8_t uuid_len = 0;
    AT_RSP_START(256);

    AT_RSP("+BLEGATTSCHAR:");
    AT_RSP(",%d",svc_id);
    AT_RSP(",%d,",char_val_idx);

    if (((char_info & BLE_GATT_ATTR_UUID_TYPE_MASK) >> BLE_GATT_ATTR_UUID_TYPE_LSB) == BLE_GATT_UUID_16) {
        uuid_len = 2;
    } else if (((char_info & BLE_GATT_ATTR_UUID_TYPE_MASK) >> BLE_GATT_ATTR_UUID_TYPE_LSB) == BLE_GATT_UUID_32) {
        uuid_len = 4;
    } else if (((char_info & BLE_GATT_ATTR_UUID_TYPE_MASK) >> BLE_GATT_ATTR_UUID_TYPE_LSB) == BLE_GATT_UUID_128) {
        uuid_len = 16;
    }
    for (i = uuid_len -1; i >= 0; i--)
        AT_RSP("%02X", p_char_uuid[i]);
    AT_RSP("\r\n");
    AT_RSP_IMMEDIATE();
    AT_RSP_FREE();
    ble_gatts_list_desc(at_ble_cb.at_svc_id, char_val_idx, at_ble_desc_list_all_cb);
}

void at_ble_svc_list_all_cb(uint8_t svc_id, const uint8_t *p_svc_uuid, uint8_t svc_type, uint8_t svc_info)
{
    int8_t i = 0;
    AT_RSP_START(256);
    uint8_t uuid_len = 0;

    AT_RSP("+BLEGATTSSVC:%d,",svc_id);

    if ((svc_info & BLE_GATT_SVC_UUID_TYPE_MASK) == SVC_UUID(16)) {
        uuid_len = 2;
    } else if ((svc_info & BLE_GATT_SVC_UUID_TYPE_MASK) == SVC_UUID(32)) {
        uuid_len = 4;
    } else if ((svc_info & BLE_GATT_SVC_UUID_TYPE_MASK) == SVC_UUID(128)) {
        uuid_len = 16;
    }
    for (i = uuid_len -1; i >= 0; i--) {
        AT_RSP("%02X", p_svc_uuid[i]);
    }
    AT_RSP(",%d\r\n",svc_type);
    AT_RSP_IMMEDIATE();
    AT_RSP_FREE();
    at_ble_cb.at_svc_id = svc_id;
    ble_gatts_list_char(svc_id, at_ble_char_list_all_cb);
}

void at_ble_gatts_list_all(int argc, char **argv)
{
    AT_RSP_START(128);

    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            ble_gatts_list_svc(at_ble_svc_list_all_cb);
            AT_RSP_OK();
        } else {
            goto Error;
        }
    } else {
        goto Error;
    }

    return;
Error:
    AT_RSP_ERR();
    return;
}

static ble_status_t at_ble_gattc_co_cb(ble_gattc_co_msg_info_t *p_info)
{
    int8_t i = 0;
    AT_RSP_START(128);
    uint8_t uuid_len = 0;

    switch (p_info->cli_cb_msg_type) {

    case BLE_CLI_CO_EVT_DISC_SVC_INFO_IND: {
        ble_gattc_co_disc_svc_ind_t *p_disc_svc_ind = &p_info->msg_data.disc_svc_ind;

        AT_RSP("+BLEGATTCDISCSVC:%02x,%02x,",p_disc_svc_ind->start_hdl, p_disc_svc_ind->end_hdl);
        if (p_disc_svc_ind->ble_uuid.type == BLE_UUID_TYPE_16)
            uuid_len = 2;
        else if (p_disc_svc_ind->ble_uuid.type == BLE_UUID_TYPE_32)
            uuid_len = 4;
        else if (p_disc_svc_ind->ble_uuid.type == BLE_UUID_TYPE_128)
            uuid_len = 16;
        for (i = uuid_len -1; i >= 0; i--) {
            AT_RSP("%02X", p_disc_svc_ind->ble_uuid.data.uuid_128[i]);
        }
        AT_RSP("\r\n");
        AT_RSP_IMMEDIATE();
    } break;

    case BLE_CLI_CO_EVT_DISC_CHAR_INFO_IND: {
        ble_gattc_co_disc_char_ind_t *p_disc_char_ind = &p_info->msg_data.disc_char_ind;

        AT_RSP("+BLEGATTCDISCCHAR:%02x,%02x,%02x,", p_disc_char_ind->char_hdl, p_disc_char_ind->val_hdl, p_disc_char_ind->prop);
        if (p_disc_char_ind->ble_uuid.type == BLE_UUID_TYPE_16)
            uuid_len = 2;
        else if (p_disc_char_ind->ble_uuid.type == BLE_UUID_TYPE_32)
            uuid_len = 4;
        else if (p_disc_char_ind->ble_uuid.type == BLE_UUID_TYPE_128)
            uuid_len = 16;

        for (i = uuid_len -1; i >= 0; i--) {
            AT_RSP("%02X", p_disc_char_ind->ble_uuid.data.uuid_128[i]);
        }
        AT_RSP("\r\n");
        AT_RSP_IMMEDIATE();
    } break;

    case BLE_CLI_CO_EVT_DISC_DESC_INFO_IND: {
        ble_gattc_co_disc_desc_ind_t *p_disc_desc_ind = &p_info->msg_data.disc_desc_ind;

        AT_RSP("+BLEGATTCDISCDESC:%02x,", p_disc_desc_ind->desc_hdl);
        if (p_disc_desc_ind->ble_uuid.type == BLE_UUID_TYPE_16)
            uuid_len = 2;
        else if (p_disc_desc_ind->ble_uuid.type == BLE_UUID_TYPE_32)
            uuid_len = 4;
        else if (p_disc_desc_ind->ble_uuid.type == BLE_UUID_TYPE_128)
            uuid_len = 16;
        for (i = uuid_len -1; i >= 0; i--) {
            AT_RSP("%02X", p_disc_desc_ind->ble_uuid.data.uuid_128[i]);
        }
        AT_RSP("\r\n");
        AT_RSP_IMMEDIATE();
    } break;

    case BLE_CLI_CO_EVT_DISC_SVC_RSP:
        sys_sema_up(&at_ble_async_sema);
        break;

    case BLE_CLI_CO_EVT_DISC_CHAR_RSP:
        sys_sema_up(&at_ble_async_sema);
        break;

    case BLE_CLI_CO_EVT_DISC_DESC_RSP:
        sys_sema_up(&at_ble_async_sema);
        break;

    case BLE_CLI_CO_EVT_READ_RSP: {
        ble_gattc_co_read_rsp_t *p_read_rsp = &p_info->msg_data.read_rsp;

        AT_RSP("+BLEGATTCRD:%d,%d,", p_info->conn_idx , p_read_rsp->length);
        AT_RSP_IMMEDIATE();
        AT_RSP_DIRECT((char *)p_read_rsp->p_value, p_read_rsp->length);
        AT_RSP("\r\n");
        AT_RSP_IMMEDIATE();
        sys_sema_up(&at_ble_async_sema);
    } break;

    case BLE_CLI_CO_EVT_WRITE_RSP:
        sys_sema_up(&at_ble_async_sema);
        break;

    case BLE_CLI_CO_EVT_NTF_IND: {
        ble_gattc_co_ntf_ind_t *p_ntf_ind = &p_info->msg_data.ntf_ind;
        AT_RSP("receive notification. conn_idx: %d,handle: %02x, is_ntf: %d, value_len: %d, value: 0x", p_info->conn_idx , p_ntf_ind->handle, p_ntf_ind->is_ntf, p_ntf_ind->length);
        for (i = p_ntf_ind->length -1; i >= 0; i--) {
            AT_RSP("%02X", p_ntf_ind->p_value[i]);
        }
        AT_RSP("\r\n");
        AT_RSP_IMMEDIATE();
    } break;

    default:
        break;
    }

    AT_RSP_FREE();

    return BLE_ERR_NO_ERROR;
}

void at_ble_gattc_disc_svc(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t conidx = 0;
    uint16_t start_hdl = 0;
    uint16_t end_hdl = 0;
    ble_status_t ret = BLE_ERR_NO_ERROR;
    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;
    } else if (argc == 4) {
        conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
        start_hdl = (uint16_t)strtoul((const char *)argv[2], &endptr, 16);
        end_hdl = (uint16_t)strtoul((const char *)argv[3], &endptr, 16);
        ret = ble_gattc_co_disc_svc(conidx, start_hdl, end_hdl);
        if (!ret)
            sys_sema_down(&at_ble_async_sema, 0);
    } else {
        goto Error;
    }


    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEGATTCDISCSVC=<conn_idx>,<start_hdl>,<end_hdl>\r\n");
    AT_RSP_OK();
}

void at_ble_gattc_disc_char(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t conidx = 0;
    uint16_t start_hdl = 0;
    uint16_t end_hdl = 0;
    ble_status_t ret = BLE_ERR_NO_ERROR;
    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;
    } else if (argc == 4) {
        conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
        start_hdl = (uint16_t)strtoul((const char *)argv[2], &endptr, 16);
        end_hdl = (uint16_t)strtoul((const char *)argv[3], &endptr, 16);
        ret = ble_gattc_co_disc_char(conidx, start_hdl, end_hdl);
        if (!ret)
            sys_sema_down(&at_ble_async_sema, 0);
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEGATTCDISCCHAR=<conn_idx>,<start_hdl>,<end_hdl>\r\n");
    AT_RSP_OK();

}

void at_ble_gattc_disc_desc(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t conidx = 0;
    uint16_t start_hdl = 0;
    uint16_t end_hdl = 0;
    ble_status_t ret = BLE_ERR_NO_ERROR;
    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;
    } else if (argc == 4) {
        conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
        start_hdl = (uint16_t)strtoul((const char *)argv[2], &endptr, 16);
        end_hdl = (uint16_t)strtoul((const char *)argv[3], &endptr, 16);
        ret = ble_gattc_co_disc_desc(conidx, start_hdl, end_hdl);
        if (!ret)
            sys_sema_down(&at_ble_async_sema, 0);
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEGATTCDISCDESC=<conn_idx>,<start_hdl>,<end_hdl>\r\n");
    AT_RSP_OK();

}



void at_ble_gattc_read(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t conidx = 0;
    uint16_t handle = 0;
    uint16_t max_len = 0xff;
    ble_status_t ret = BLE_ERR_NO_ERROR;
    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;
    } else if (argc == 4) {
        conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
        handle = (uint16_t)strtoul((const char *)argv[2], &endptr, 16);
        max_len = (uint16_t)strtoul((const char *)argv[3], &endptr, 16);
        ret = ble_gattc_co_read(conidx, handle, 0, max_len);
        if (!ret)
            sys_sema_down(&at_ble_async_sema, 0);
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEGATTCRD=<conn_idx>,<handle>,<max_len>\r\n");
    AT_RSP_OK();
}

void at_ble_gattc_write(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t conidx = 0;
    uint16_t handle = 0;
    uint16_t len = 0;
    ble_status_t ret = BLE_ERR_NO_ERROR;
    uint8_t *tx_buf = NULL;
    uint8_t write_type = 0;
    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;
    } else if (argc == 5) {
        conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
        handle = (uint16_t)strtoul((const char *)argv[2], &endptr, 16);
        write_type = (uint8_t)strtoul((const char *)argv[3], &endptr, 16);
        len = (uint16_t)strtoul((const char *)argv[4], &endptr, 16);

        tx_buf = sys_malloc(len);
        if (NULL == tx_buf) {
            AT_TRACE("buffer failed (len = %u).\r\n", len);
            goto Error;
        }
        AT_RSP(">\r\n");
        AT_RSP_IMMEDIATE();
        at_hw_dma_receive((uint32_t)tx_buf, len);
        if (write_type == BLE_GATT_WRITE)
            ret = ble_gattc_co_write_req(conidx, handle, len, tx_buf);
        else if (write_type == BLE_GATT_WRITE_NO_RESP)
            ret = ble_gattc_co_write_cmd(conidx, handle, len, tx_buf);
        if (!ret)
            sys_sema_down(&at_ble_async_sema, 0);
        sys_mfree(tx_buf);
    } else {
        goto Error;
    }


    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEGATTCWR=<conn_idx>,<handle>,<write_type>,<len>\r\n");
    AT_RSP_OK();
}

/*!
    \brief      Enable passthrough client mode
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_ble_passth_cli_mode_enable(int argc, char **argv)
{
    uint32_t cur_cnt = 0;
    uint8_t *tx_buf = sys_malloc(ATBLE_PASSTH_MAX_SIZE);
    uint8_t conn_idx;
    char *endptr = NULL;
    bool reset = true;

    AT_RSP_START(128);

    ble_datatrans_cli_rx_cb_reg(at_ble_passth_rx_callback);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
    } else {
        AT_TRACE("argc fail\r\n");
        goto Error;
    }

    conn_idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
    if (!dm_check_connection_valid(conn_idx)) {
        AT_TRACE("link has not been established\r\n");
        goto Error;
    }

    ble_gattc_co_cb_unreg(at_ble_gattc_co_cb);

    if (!tx_buf)
        goto Error;

#ifndef SUPER_UART_DMA_RX
    while (1) {
        if(reset) {
            at_hw_dma_receive_config();   //have to reconfig hw here, or one byte left data will be transfered by dma
#ifndef CONFIG_ATCMD_SPI
            while(RESET == usart_flag_get(at_uart_conf.usart_periph, USART_FLAG_IDLE));
            usart_flag_clear(at_uart_conf.usart_periph, USART_FLAG_IDLE);
#endif
            reset = false;
            sys_memset(tx_buf, 0, ATBLE_PASSTH_MAX_SIZE);
            at_hw_dma_receive_start((uint32_t)tx_buf, 0, ATBLE_PASSTH_MAX_SIZE);
        }

        sys_ms_sleep(1);

        if (at_ble_cb.disconn_flag == true) {
            at_ble_cb.disconn_flag = false;
            break;
        }
#ifdef CONFIG_ATCMD_SPI
        if (RESET == spi_flag_get( SPI_FLAG_RBNE))
#else
        if (RESET != usart_flag_get(at_uart_conf.usart_periph, USART_FLAG_IDLE))
#endif
        {
#ifndef CONFIG_ATCMD_SPI
            usart_flag_clear(at_uart_conf.usart_periph, USART_FLAG_IDLE);
#endif
            cur_cnt = at_dma_get_cur_received_num(ATBLE_PASSTH_MAX_SIZE);
            if (cur_cnt == 0)
                continue;

            reset = true;
            at_hw_dma_receive_stop();

            if (at_ble_terminate_string_check(tx_buf))
                break;

            if (ble_datatrans_cli_write_char(conn_idx, tx_buf, cur_cnt) != BLE_ERR_NO_ERROR) {
                AT_TRACE("data send fail\r\n");
            }
        }
    }
    at_hw_dma_receive_stop();
    at_hw_irq_receive_config();
#else
    while (1) {
        if(reset) {
            reset = false;
            sys_memset(tx_buf, 0, ATBLE_PASSTH_MAX_SIZE);
        }
        cur_cnt = at_hw_dma_receive_start((uint32_t)tx_buf, ATBLE_PASSTH_MAX_SIZE, 10);

        if (at_ble_cb.disconn_flag == true) {
            at_ble_cb.disconn_flag = false;
            break;
        }

        if (cur_cnt == 0) {
            continue;
        }
        else {
            reset = true;
            if (at_ble_terminate_string_check(tx_buf))
                break;

            if (ble_datatrans_cli_write_char(conn_idx, tx_buf, cur_cnt) != BLE_ERR_NO_ERROR) {
                AT_TRACE("data send fail\r\n");
            }
        }
    }
    at_hw_dma_receive_stop();
#endif
    sys_mfree(tx_buf);
    ble_datatrans_cli_rx_cb_unreg();
    ble_gattc_co_cb_reg(at_ble_gattc_co_cb);

    return;

Error:
    if (tx_buf)
        sys_mfree(tx_buf);
    ble_datatrans_cli_rx_cb_unreg();
    ble_gattc_co_cb_reg(at_ble_gattc_co_cb);
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEPASSTHCLI=<conn_idx>\r\n");
    AT_RSP_OK();
    return;
}

void at_ble_set_key(int argc, char **argv)
{
    char *endptr = NULL;
    uint32_t key;

    AT_RSP_START(128);

    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            key = app_sec_pin_code_get();
            AT_RSP("+BLEKEY:%06u\r\n", (unsigned int)key);
            AT_RSP_OK();
            return;
        } else {
            goto Error;
        }
    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        }

        key = (uint32_t)strtoul((const char *)argv[1], &endptr, 10);
        if (app_sec_pin_code_set(key) == false) {
            goto Error;
        }
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLESETKEY=<key>\r\n");
    AT_RSP_OK();
}

void at_ble_set_auth(int argc, char **argv)
{
    char *endptr = NULL;
    bool bond;
    bool mitm;
    bool sc;
    uint8_t iocap;
    uint8_t oob;
    bool sc_only = false;
    uint8_t key_size = 16;
    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;
    } else if (argc == 7) {
        bond = (bool)strtoul((const char *)argv[1], &endptr, 0);
        mitm = (bool)strtoul((const char *)argv[2], &endptr, 0);
        sc = (bool)strtoul((const char *)argv[3], &endptr, 0);
        iocap = (uint8_t)strtoul((const char *)argv[4], &endptr, 0);
        oob = (uint8_t)strtoul((const char *)argv[5], &endptr, 0);
        key_size = (uint8_t)strtoul((const char *)argv[6], &endptr, 0);

        app_sec_set_authen(bond, mitm, sc, iocap, oob, sc_only, key_size);
    } else {
        goto Error;
    }


    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLESETAUTH=<bond>,<mitm>,<sc>,<iocap>,<oob>,<key_size>\r\n");
    AT_RSP_OK();
}

void at_ble_pair(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t conidx = 0;
    ble_device_t *p_device = NULL;
    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 0);
            p_device = dm_find_dev_by_conidx(conidx);
            if (p_device == NULL) {
                AT_TRACE("fail to find device\r\n");
                goto Error;
            }

            if (p_device->role == BLE_MASTER) {
                app_sec_send_bond_req(conidx);
            } else {
                app_sec_send_security_req(conidx);
            }
        }
    } else {
        goto Error;
    }


    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEPAIR=<conidx>\r\n");
    AT_RSP_OK();
}

void at_ble_encrypt(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t conidx = 0;
    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        }
        else {
            conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 0);
            app_sec_send_encrypt_req(conidx);
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEENCRYPT=<conidx>\r\n");
    AT_RSP_OK();
}

void at_ble_passkey(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t conidx = 0;
    uint32_t passkey = 0;
    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;
    } else if (argc == 3) {
        conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 0);
        passkey = (uint32_t)strtoul((const char *)argv[2], &endptr, 0);
        if (passkey > 999999) {
            goto Error;
        }

        app_sec_input_passkey(conidx, passkey);
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLEPASSKEY=<conidx>,<passkey>\r\n");
    AT_RSP_OK();
}

void at_ble_compare(int argc, char **argv)
{
    char *endptr = NULL;
    bool value = true;
    uint8_t conidx = 0;
    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;
    } else if (argc == 3) {
        conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 0);
        value = (bool)strtoul((const char *)argv[2], &endptr, 0);

        app_sec_num_compare(conidx, value);
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLECOMPARE=<conidx>,<value>\r\n");
    AT_RSP_OK();
}

void at_ble_list_enc_dev_cb(uint8_t dev_idx, ble_device_t *p_device)
{
    AT_RSP_START(128);

    AT_RSP("+BLELISTENCDEV:%d,%02X:%02X:%02X:%02X:%02X:%02X\r\n", dev_idx,
                   p_device->cur_addr.addr[5], p_device->cur_addr.addr[4], p_device->cur_addr.addr[3],
                   p_device->cur_addr.addr[2], p_device->cur_addr.addr[1], p_device->cur_addr.addr[0]);
    AT_RSP_IMMEDIATE();
    AT_RSP_FREE();
}

void at_ble_list_enc_dev(int argc, char **argv)
{
    AT_RSP_START(128);

    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            dm_list_sec_devices(at_ble_list_enc_dev_cb);
        } else {
            goto Error;
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;
}

void at_ble_clear_enc_dev(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t dev_idx = 0xFF;
    ble_device_t *p_dev = NULL;
    AT_RSP_START(128);

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            dev_idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
            p_dev = dm_find_dev_by_idx(dev_idx);
            if (p_dev == NULL) {
                AT_TRACE("fail to find device\r\n");
                goto Error;
            }

            if (!app_sec_remove_bond(p_dev->cur_addr)) {
                AT_TRACE("remove bond fail\r\n");
                goto Error;
            }

            AT_TRACE("remove bond success\r\n");
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;
Error:
    AT_RSP_ERR();
    return;

Usage:
    AT_RSP("+BLECLEARENCDEV=<dev_idx>\r\n");
    AT_RSP_OK();
}

static void at_ble_sec_authen_cmpl(uint8_t conn_idx, uint8_t result)
{
    char buffer[50] = {0};
    int data_size;

    data_size = co_snprintf(buffer, 50, "+BLEAUTHENCMPL:%d,%d\r\n", conn_idx, result);
    AT_RSP_DIRECT(buffer, data_size);
}

static void at_ble_sec_input_key_req(uint8_t conn_idx)
{
    char buffer[50] = {0};
    int data_size;

    data_size = co_snprintf(buffer, 50, "+BLEPASSKEYREQ:%d\r\n", conn_idx);
    AT_RSP_DIRECT(buffer, data_size);
}

static void at_ble_sec_key_cfm_req(uint8_t conn_idx, uint32_t number)
{
    char buffer[50] = {0};
    int data_size;

    data_size = co_snprintf(buffer, 50, "+BLECOMPAREREQ:%d,%d\r\n", conn_idx, number);
    AT_RSP_DIRECT(buffer, data_size);
}

