/*!
    \file    ble_central.c
    \brief   Implementation of ble datatrans client demo.

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

#include <stdint.h>
#include "dbg_print.h"
#include "gd32vw55x_platform.h"
#include "wrapper_os.h"
#include "ble_datatrans_cli.h"
#include "ble_adapter.h"
#include "ble_scan.h"
#include "ble_conn.h"
#include "ble_utils.h"
#include "ble_export.h"
#include "ble_gattc.h"
#include "ble_adv_data.h"
#include "ble_sec.h"
//#include "app_uart.h"

/* Device name */
#define DEV_NAME "GD-BLE-DEV"

/* Device name length*/
#define DEV_NAME_LEN strlen(dev_name)

/* Definitions of the different task priorities */
enum
{
    BLE_STACK_TASK_PRIORITY = OS_TASK_PRIORITY(2),      /*!< Priority of the BLE stack task */
    BLE_APP_TASK_PRIORITY   = OS_TASK_PRIORITY(1),      /*!< Priority of the BLE APP task */
};

/* Definitions of the different BLE task stack size requirements */
enum
{
    BLE_STACK_TASK_STACK_SIZE = 768,        /*!< BLE stack task stack size */
    BLE_APP_TASK_STACK_SIZE   = 512,        /*!< BLE APP task stack size */
};

/* Device name array*/
static char dev_name[] = {DEV_NAME};

/* connection index */
static uint8_t conn_idx = 0;

uint8_t central_connected = 0;

/*!
    \brief      Function to execute app code after stack ready
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void ble_task_ready(void)
{
    ble_scan_enable();
}

/*!
    \brief      Callback function to handle BLE adapter events
    \param[in]  event: BLE adapter event type
    \param[in]  p_data: pointer to BLE adapter event data
    \param[out] none
    \retval     none
*/
static void app_adp_evt_handler(ble_adp_evt_t event, ble_adp_data_u *p_data)
{
    uint8_t i = 0;

    if (event == BLE_ADP_EVT_ENABLE_CMPL_INFO) {
         if (p_data->adapter_info.status == BLE_ERR_NO_ERROR) {
            app_print("=== Adapter enable success ===\r\n");
            app_print("hci_ver 0x%x, hci_subver 0x%x, lmp_ver 0x%x, lmp_subver 0x%x, manuf_name 0x%x\r\n",
                   p_data->adapter_info.version.hci_ver, p_data->adapter_info.version.hci_subver,
                   p_data->adapter_info.version.lmp_ver, p_data->adapter_info.version.lmp_subver,
                   p_data->adapter_info.version.manuf_name);

            app_print("adv_set_num %u, min_tx_pwr %d, max_tx_pwr %d, max_adv_data_len %d \r\n",
                   p_data->adapter_info.adv_set_num, p_data->adapter_info.tx_pwr_range.min_tx_pwr,
                   p_data->adapter_info.tx_pwr_range.max_tx_pwr, p_data->adapter_info.max_adv_data_len);
            app_print("sugg_max_tx_octets %u, sugg_max_tx_time %u \r\n",
                   p_data->adapter_info.sugg_dft_data.sugg_max_tx_octets,
                   p_data->adapter_info.sugg_dft_data.sugg_max_tx_time);

            app_print("loc irk:");

            for (i = 0; i < BLE_GAP_KEY_LEN; i++) {
                app_print(" %02x", p_data->adapter_info.loc_irk_info.irk[i]);
            }

            app_print("\r\n");
            app_print("identity addr %02X:%02X:%02X:%02X:%02X:%02X \r\n ",
                   p_data->adapter_info.loc_irk_info.identity.addr[5],
                   p_data->adapter_info.loc_irk_info.identity.addr[4],
                   p_data->adapter_info.loc_irk_info.identity.addr[3],
                   p_data->adapter_info.loc_irk_info.identity.addr[2],
                   p_data->adapter_info.loc_irk_info.identity.addr[1],
                   p_data->adapter_info.loc_irk_info.identity.addr[0]);

            app_print("=== BLE Adapter enable complete ===\r\n");
            ble_task_ready();
        } else {
            app_print("=== BLE Adapter enable fail ===\r\n");
        }

    }
}

/*!
    \brief      Init adapter application module
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void app_adapter_init(void)
{
    ble_adp_callback_register(app_adp_evt_handler);
}

/*!
    \brief      Callback function to handle when GATT discovery is done
    \param[in]  conn_idx: connection index
    \param[in]  status: GATT discovery status
    \param[out] none
    \retval     none
*/
static void app_conn_gatt_discovery_callback(uint8_t conn_idx, uint16_t status)
{
    ble_conn_enable_central_feat(conn_idx);
    central_connected = 1;
}

/*!
    \brief      Callback function to handle BLE connection event
    \param[in]  event: BLE connection event type
    \param[in]  p_data: pointer to BLE connection event data
    \param[out] none
    \retval     none
*/
static void app_conn_evt_handler(ble_conn_evt_t event, ble_conn_data_u *p_data)
{
    if (event == BLE_CONN_EVT_STATE_CHG) {
        if (p_data->conn_state.state == BLE_CONN_STATE_DISCONNECTD) {
            dbg_print(NOTICE, "disconnected. conn idx: %u, conn_hdl: 0x%x reason 0x%x\r\n",
                      p_data->conn_state.info.discon_info.conn_idx,
                      p_data->conn_state.info.discon_info.conn_hdl,
                      p_data->conn_state.info.discon_info.reason);
            central_connected = 0;
            ble_scan_enable();
        } else if (p_data->conn_state.state == BLE_CONN_STATE_CONNECTED) {
            dbg_print(NOTICE, "connect success. conn idx:%u, conn_hdl:0x%x \r\n",
                      p_data->conn_state.info.conn_info.conn_idx,
                      p_data->conn_state.info.conn_info.conn_hdl);

            conn_idx = p_data->conn_state.info.conn_info.conn_idx;

            if (p_data->conn_state.info.conn_info.role == BLE_MASTER) {
                ble_gattc_mtu_update(p_data->conn_state.info.conn_info.conn_idx, 0);
                ble_gattc_start_discovery(p_data->conn_state.info.conn_info.conn_idx,
                                          app_conn_gatt_discovery_callback);
            }
        }
    }
}

/*!
    \brief      Callback function to handle @ref BLE_SEC_EVT_PAIRING_FAIL_INFO event
    \param[in]  p_info: pointer to the pairing fail information data
    \param[out] none
    \retval     none
*/
static void app_pairing_fail_hdlr(ble_sec_pairing_fail_t *p_info)
{
    app_print("pairing fail reason 0x%x\r\n", p_info->param.reason);
}

/*!
    \brief      Callback function to handle @ref BLE_SEC_EVT_PAIRING_SUCCESS_INFO event
    \param[in]  p_info: pointer to the pairing success information data
    \param[out] none
    \retval     none
*/
static void app_pairing_success_hdlr(ble_sec_pairing_success_t *p_info)
{
    app_print("conn_idx %u pairing success, level 0x%x ltk_present %d sc %d\r\n", p_info->conidx,
              p_info->bond_info.pairing_lvl, p_info->bond_info.enc_key_present, p_info->sc);
}

/*!
    \brief      Callback function to handle @ref BLE_SEC_EVT_SECURITY_REQ_INFO event
    \param[in]  p_info: pointer to the security request information data
    \param[out] none
    \retval     none
*/
static void app_security_req_info_hdlr(ble_sec_security_req_info_t *p_info)
{
    ble_gap_pairing_param_t param = {0};

    param.auth      = BLE_GAP_AUTH_REQ_NO_MITM_NO_BOND;
    param.iocap     = BLE_GAP_IO_CAP_NO_IO;
    param.key_size  = 16;
    param.ikey_dist = BLE_GAP_KDIST_IDKEY | BLE_GAP_KDIST_SIGNKEY | BLE_GAP_KDIST_ENCKEY;
    param.rkey_dist = BLE_GAP_KDIST_IDKEY | BLE_GAP_KDIST_SIGNKEY | BLE_GAP_KDIST_ENCKEY;

    ble_sec_bond_req(p_info->param.conn_idx, &param, BLE_GAP_NO_SEC);
}

/*!
    \brief      Callback function to handle BLE security events
    \param[in]  event: BLE security event type
    \param[in]  p_data: pointer to the BLE security event data
    \param[out] none
    \retval     none
*/
static void app_sec_evt_handler(ble_sec_evt_t event, ble_sec_data_u *p_data)
{
    switch (event) {
    case BLE_SEC_EVT_SECURITY_REQ_INFO:
        app_security_req_info_hdlr((ble_sec_security_req_info_t *)p_data);
        break;

    case BLE_SEC_EVT_PAIRING_SUCCESS_INFO:
        app_pairing_success_hdlr((ble_sec_pairing_success_t *)p_data);
        break;

    case BLE_SEC_EVT_PAIRING_FAIL_INFO:
        app_pairing_fail_hdlr((ble_sec_pairing_fail_t *)p_data);
        break;
    default:
        break;
    }
}

/*!
    \brief      Init application security module
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void app_sec_mgr_init(void)
{
    ble_sec_callback_register(app_sec_evt_handler);
}

/*!
    \brief      Init APP connection manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void app_conn_mgr_init(void)
{
    ble_conn_callback_register(app_conn_evt_handler);
}

/*!
    \brief      Function to handle @ref BLE_SCAN_EVT_ADV_RPT event
    \param[in]  p_info: pointer to advertising report information
    \param[out] none
    \retval     none
*/
static void app_scan_mgr_report_hdlr(ble_gap_adv_report_info_t *p_info)
{
    uint8_t *p_data = NULL;
    uint8_t len = 0;

    p_data = ble_adv_find(p_info->data.p_data, p_info->data.len, BLE_AD_TYPE_COMPLETE_LOCAL_NAME, &len);
    if (p_data != NULL && !memcmp(p_data, dev_name, len)) {
        ble_scan_disable();
        ble_conn_connect(NULL, BLE_GAP_LOCAL_ADDR_STATIC, &p_info->peer_addr, false);
    }
}

/*!
    \brief      Callback function to handle BLE scan events
    \param[in]  event: BLE scan event type
    \param[in]  p_data: pointer to the BLE scan event data
    \param[out] none
    \retval     none
*/
static void app_scan_mgr_evt_handler(ble_scan_evt_t event, ble_scan_data_u *p_data)
{
    switch (event) {
    case BLE_SCAN_EVT_STATE_CHG:
        if (p_data->scan_state.scan_state == BLE_SCAN_STATE_ENABLED) {
            dbg_print(NOTICE, "Ble Scan enabled status 0x%x\r\n", p_data->scan_state.reason);
        } else if (p_data->scan_state.scan_state == BLE_SCAN_STATE_DISABLED) {
            dbg_print(NOTICE, "Ble Scan disabled  status 0x%x\r\n", p_data->scan_state.reason);
        }
        break;

    case BLE_SCAN_EVT_ADV_RPT:
        app_scan_mgr_report_hdlr(p_data->p_adv_rpt);
        break;

    default:
        break;
    }
}

/*!
    \brief      Callback function to handle data received by datatrans client service
    \param[in]  conn_idx: connection index
    \param[in]  data_len: received data length
    \param[in]  p_data: pointer to received data
    \param[out] none
    \retval     none
*/
static void app_datatrans_cli_rx_callback(uint8_t conn_idx, uint16_t data_len, uint8_t *p_data)
{
#if 1
    app_print("[BLE] RX: %x, %u\r\n", p_data[0], data_len);
#else
    uint8_t *p_str = sys_malloc(data_len + 1);

    if (p_str) {
        app_print("datatrans cli receive data: \r\n");
        memset(p_str, 0, data_len + 1);
        memcpy(p_str, p_data, data_len);
        app_print("%s\r\n", p_str);
        sys_mfree(p_str);
    }
#endif
}

/*!
    \brief      Init application scan manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void app_scan_mgr_init(void)
{
    ble_scan_callback_register(app_scan_mgr_evt_handler);
}

/*!
    \brief      Init BLE component modules needed
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_central_init(void)
{
    ble_init_param_t param = {0};
    ble_os_api_t os_interface = {
      .os_malloc = sys_malloc,
      .os_calloc = sys_calloc,
      .os_mfree = sys_mfree,
      .os_memset = sys_memset,
      .os_memcpy = sys_memcpy,
      .os_memcmp = sys_memcmp,
      .os_task_create = sys_task_create,
      .os_task_init_notification = sys_task_init_notification,
      .os_task_wait_notification = sys_task_wait_notification,
      .os_task_notify = sys_task_notify,
      .os_task_delete = sys_task_delete,
      .os_ms_sleep = sys_ms_sleep,
      .os_current_task_handle_get = sys_current_task_handle_get,
      .os_queue_init = sys_queue_init,
      .os_queue_free = sys_queue_free,
      .os_queue_write = sys_queue_write,
      .os_queue_read = sys_queue_read,
      .os_random_bytes_get = sys_random_bytes_get,
    };

    ble_power_on();

    param.role = BLE_GAP_ROLE_CENTRAL;
    param.keys_user_mgr = false;
    param.pairing_mode = BLE_GAP_PAIRING_SECURE_CONNECTION | BLE_GAP_PAIRING_LEGACY;
    param.privacy_cfg = BLE_GAP_PRIV_CFG_PRIV_EN_BIT;
    param.ble_task_stack_size = BLE_STACK_TASK_STACK_SIZE;
    param.ble_task_priority = BLE_STACK_TASK_PRIORITY;
    param.ble_app_task_stack_size = BLE_APP_TASK_STACK_SIZE;
    param.ble_app_task_priority = BLE_APP_TASK_PRIORITY;
    param.en_cfg = 0;
    param.p_os_api = &os_interface;
    ble_sw_init(&param);

    app_adapter_init();
    app_scan_mgr_init();
    app_conn_mgr_init();
    app_sec_mgr_init();
    ble_datatrans_cli_init();
    ble_datatrans_cli_rx_cb_reg(app_datatrans_cli_rx_callback);
    central_connected = 0;
    /* The BLE interrupt must be enabled after ble_sw_init. */
    ble_irq_enable();
}

int ble_central_tx(uint8_t *p_buf, uint16_t len)
{
    if (ble_datatrans_cli_write_char(conn_idx, p_buf, len) == BLE_ERR_NO_ERROR) {
        return 0;
    } else {
        return -1;
    }
}

