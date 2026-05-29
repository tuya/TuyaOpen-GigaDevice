/*!
    \file    ble_peripheral.c
    \brief   Implementation of ble datatrans server demo.

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
#include "ble_adapter.h"
#include "ble_adv.h"
#include "ble_conn.h"
#include "ble_utils.h"
#include "ble_export.h"
#include "ble_sec.h"
#include "ble_datatrans_srv.h"
//#include "app_uart.h"

/* Device name */
#define DEV_NAME "GD-BLE-DEV"

/* Device name length*/
#define DEV_NAME_LEN strlen(dev_name)

/* Advertising parameters */
typedef struct
{
    uint8_t         adv_idx;            /*!< Advertising id. use to stop advertising */
    ble_adv_state_t adv_state;          /*!< Advertising state */
} app_adv_param_t;

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
char dev_name[] = {DEV_NAME};

/* advertising env*/
static app_adv_param_t app_adv_env = {0};

/* connection index */
static uint8_t conn_idx = 0;

uint8_t peripheral_connected = 0;

/*!
    \brief      Start advertising
    \param[in]  p_adv: pointer to BLE advertising set
    \param[out] none
    \retval     none
*/
static ble_status_t app_adv_start(void)
{
    ble_data_t adv_data = {0};
    ble_data_t adv_scanrsp_data = {0};
    ble_adv_data_set_t adv = {0};
    ble_adv_data_set_t scan_rsp = {0};
    uint8_t data[BLE_GAP_LEGACY_ADV_MAX_LEN] = {0};
    uint8_t idx = 0;

    data[idx++] = 2;
    data[idx++] = BLE_AD_TYPE_FLAGS;
    data[idx++] = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED | BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE;
    data[idx++] = DEV_NAME_LEN + 1;
    data[idx++] = 0x09;
    memcpy(&data[idx], dev_name, DEV_NAME_LEN);
    idx += DEV_NAME_LEN;

    adv_data.len = idx;
    adv_data.p_data = data;
    adv_scanrsp_data.len = idx - 3;
    adv_scanrsp_data.p_data = &data[3];
    adv.data_force = true;
    adv.data.p_data_force = &adv_data;
    scan_rsp.data_force = true;
    scan_rsp.data.p_data_force = &adv_scanrsp_data;

    return ble_adv_start(app_adv_env.adv_idx, &adv, &scan_rsp, NULL);
}

/*!
    \brief      Callback function to handle BLE advertising events
    \param[in]  adv_evt: BLE advertising event type
    \param[in]  p_data: pointer to BLE advertising event data
    \param[in]  p_context: context data used when create advertising
    \param[out] none
    \retval     none
*/
static void app_adv_mgr_evt_hdlr(ble_adv_evt_t adv_evt, void *p_data, void *p_context)
{
    if (adv_evt == BLE_ADV_EVT_STATE_CHG) {
        ble_adv_state_chg_t *p_chg = (ble_adv_state_chg_t *)p_data;
        ble_adv_state_t old_state = app_adv_env.adv_state;

        app_print("%s state change 0x%x ==> 0x%x, reason 0x%x\r\n", __func__,
                  old_state, p_chg->state, p_chg->reason);

        app_adv_env.adv_state = p_chg->state;

        if ((p_chg->state == BLE_ADV_STATE_CREATE) && (old_state == BLE_ADV_STATE_CREATING)) {
            app_adv_env.adv_idx = p_chg->adv_idx;
            app_adv_start();
        }
    }
}

/*!
    \brief      Create an advertising
    \param[in]  p_param: pointer to advertising parameters
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t app_adv_create(void)
{
    ble_adv_param_t adv_param = {0};

    adv_param.param.disc_mode = BLE_GAP_ADV_MODE_GEN_DISC;
    adv_param.param.own_addr_type = BLE_GAP_LOCAL_ADDR_STATIC;
    adv_param.param.type = BLE_GAP_ADV_TYPE_LEGACY;
    adv_param.param.prop = BLE_GAP_ADV_PROP_UNDIR_CONN;
    adv_param.param.filter_pol = BLE_GAP_ADV_ALLOW_SCAN_ANY_CON_ANY;
    adv_param.param.ch_map = 0x07;
    adv_param.param.primary_phy = BLE_GAP_PHY_1MBPS;
    adv_param.param.adv_intv_min = 160;
    adv_param.param.adv_intv_max = 160;
    adv_param.restart_after_disconn = true;

    return ble_adv_create(&adv_param, app_adv_mgr_evt_hdlr, NULL);
}

/*!
    \brief      Function to execute app code after stack ready
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void ble_task_ready(void)
{
    app_adv_create();
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
    \brief      Send security request
    \param[in]  conidx: connection index
    \param[out] none
    \retval     none
*/
static void app_sec_send_security_req(uint8_t conidx)
{
    uint8_t auth = BLE_GAP_AUTH_REQ_NO_MITM_NO_BOND;

    if (ble_sec_security_req(conidx, auth) != BLE_ERR_NO_ERROR) {
        app_print("app_sec_send_security_req fail! \r\n");
    }
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
    switch (event) {
    case BLE_CONN_EVT_STATE_CHG: {
        if (p_data->conn_state.state == BLE_CONN_STATE_DISCONNECTD) {
            app_print("disconnected. conn idx: %u, conn_hdl: 0x%x reason 0x%x\r\n",
                      p_data->conn_state.info.discon_info.conn_idx,
                      p_data->conn_state.info.discon_info.conn_hdl,
                      p_data->conn_state.info.discon_info.reason);
            peripheral_connected = 0;
        } else if (p_data->conn_state.state == BLE_CONN_STATE_CONNECTED) {
            app_print("connect success. conn idx:%u, conn_hdl:0x%x \r\n",
                      p_data->conn_state.info.conn_info.conn_idx,
                      p_data->conn_state.info.conn_info.conn_hdl);
            if (p_data->conn_state.info.conn_info.role == BLE_SLAVE) {
                app_sec_send_security_req(p_data->conn_state.info.conn_info.conn_idx);
            }

            conn_idx = p_data->conn_state.info.conn_info.conn_idx;
        }
    }
    break;

    case BLE_CONN_EVT_PARAM_UPDATE_IND: {
        app_print("conn idx %u, intv_min 0x%x, intv_max 0x%x, latency %u, supv_tout %u\r\n",
                  p_data->conn_param_req_ind.conn_idx, p_data->conn_param_req_ind.intv_min,
                  p_data->conn_param_req_ind.intv_max, p_data->conn_param_req_ind.latency,
                  p_data->conn_param_req_ind.supv_tout);

        ble_conn_param_update_cfm(p_data->conn_param_req_ind.conn_idx, true, 2, 4);
    }
    break;

    case BLE_CONN_EVT_PARAM_UPDATE_RSP: {
        app_print("conn idx %u, param update result status: 0x%x\r\n",
                  p_data->conn_param_rsp.conn_idx, p_data->conn_param_rsp.status);
    }
    break;

    case BLE_CONN_EVT_PARAM_UPDATE_INFO: {
        app_print("conn idx %u, param update ind: interval %d, latency %d, sup to %d\r\n",
                  p_data->conn_params.conn_idx,
                  p_data->conn_params.interval, p_data->conn_params.latency, p_data->conn_params.supv_tout);
    }
    break;

    case BLE_CONN_EVT_PKT_SIZE_SET_RSP: {
        app_print("conn idx %u, packet size set status 0x%x\r\n",
                  p_data->pkt_size_set_rsp.conn_idx, p_data->pkt_size_set_rsp.status);
    }
    break;

    case BLE_CONN_EVT_PKT_SIZE_INFO: {
        app_print("le pkt size info: conn idx %u, tx oct %d, tx time %d, rx oct %d, rx time %d\r\n",
                  p_data->pkt_size_info.conn_idx, p_data->pkt_size_info.max_tx_octets,
                  p_data->pkt_size_info.max_tx_time,
                  p_data->pkt_size_info.max_rx_octets, p_data->pkt_size_info.max_rx_time);
    }
    break;

    case BLE_CONN_EVT_NAME_GET_IND: {
        ble_conn_name_get_cfm(p_data->name_get_ind.conn_idx, 0, p_data->name_get_ind.token,
                              DEV_NAME_LEN, (uint8_t *)dev_name, DEV_NAME_LEN);
    }
    break;

    case BLE_CONN_EVT_APPEARANCE_GET_IND: {
        ble_conn_appearance_get_cfm(p_data->appearance_get_ind.conn_idx, 0,
                                    p_data->appearance_get_ind.token, 0);
    }
    break;
    default:
        break;
    }
}

/*!
    \brief      Callback function to handle @ref BLE_SEC_EVT_PAIRING_REQ_IND event
    \param[in]  p_ind: pointer to the pairing request indication data
    \param[out] none
    \retval     none
*/
static void app_pairing_req_hdlr(ble_gap_pairing_req_ind_t *p_ind)
{
    ble_gap_pairing_param_t param = {0};

    param.auth = BLE_GAP_AUTH_MASK_NONE;
    param.iocap = BLE_GAP_IO_CAP_NO_IO;
    param.key_size = 16;
    param.ikey_dist = BLE_GAP_KDIST_IDKEY | BLE_GAP_KDIST_SIGNKEY | BLE_GAP_KDIST_ENCKEY;
    param.rkey_dist = BLE_GAP_KDIST_IDKEY | BLE_GAP_KDIST_SIGNKEY | BLE_GAP_KDIST_ENCKEY;

    ble_sec_pairing_req_cfm(p_ind->conn_idx, true, &param, BLE_GAP_NO_SEC);
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
    \brief      Callback function to handle BLE security events
    \param[in]  event: BLE security event type
    \param[in]  p_data: pointer to the BLE security event data
    \param[out] none
    \retval     none
*/
static void app_sec_evt_handler(ble_sec_evt_t event, ble_sec_data_u *p_data)
{
    switch (event) {
    case BLE_SEC_EVT_PAIRING_REQ_IND:
        app_pairing_req_hdlr((ble_gap_pairing_req_ind_t *)p_data);
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
    \brief      Callback function to handle data received by datatrans server service
    \param[in]  conn_idx: connection index
    \param[in]  data_len: received data length
    \param[in]  p_data: pointer to received data
    \param[out] none
    \retval     none
*/
static void app_datatrans_srv_rx_callback(uint8_t conn_idx, uint16_t data_len, uint8_t *p_data)
{
#if 1
    app_print("[BLE] RX: %x, %u\r\n", p_data[0], data_len);
    peripheral_connected = 1;
#else
    uint8_t *p_str = sys_malloc(data_len + 1);

    if (p_str) {
        app_print("datatrans srv receive data: \r\n");
        memset(p_str, 0, data_len + 1);
        memcpy(p_str, p_data, data_len);
        app_print("%s\r\n", p_str);
        sys_mfree(p_str);
    }
#endif
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
    \brief      Init BLE component modules needed
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_peripheral_init(void)
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

    param.role = BLE_GAP_ROLE_PERIPHERAL;
    param.keys_user_mgr = false;
    param.pairing_mode = BLE_GAP_PAIRING_SECURE_CONNECTION | BLE_GAP_PAIRING_LEGACY;
    param.privacy_cfg = BLE_GAP_PRIV_CFG_PRIV_EN_BIT;
    param.ble_task_stack_size = BLE_STACK_TASK_STACK_SIZE;
    param.ble_task_priority = BLE_STACK_TASK_PRIORITY;
    param.ble_app_task_stack_size = BLE_APP_TASK_STACK_SIZE;
    param.ble_app_task_priority = BLE_APP_TASK_PRIORITY;
    param.name_perm = BLE_GAP_WRITE_NOT_ENC;
    param.appearance_perm = BLE_GAP_WRITE_NOT_ENC;
    param.en_cfg = 0;
    param.p_os_api = &os_interface;
    ble_sw_init(&param);

    app_adapter_init();
    app_conn_mgr_init();
    app_sec_mgr_init();
    ble_datatrans_srv_init();
    ble_datatrans_srv_rx_cb_reg(app_datatrans_srv_rx_callback);
    ble_work_status_set(BLE_WORK_STATUS_ENABLE);
    peripheral_connected = 0;
    /* The BLE interrupt must be enabled after ble_sw_init. */
    ble_irq_enable();
}

int ble_peripheral_tx(uint8_t *p_buf, uint16_t len)
{
    uint16_t mtu;
    ble_gatts_mtu_get(conn_idx, &mtu);
    ble_status_t ret = ble_datatrans_srv_tx(conn_idx, p_buf, len);
    if (ret == BLE_ERR_NO_ERROR) {
        return 0;
    } else {
        app_print("tx fail mtu %d status 0x%x\r\n", mtu, ret);
        return -1;
    }
}

