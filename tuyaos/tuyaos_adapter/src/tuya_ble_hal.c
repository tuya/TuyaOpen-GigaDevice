/*!
    \file    tuya_ble_hal.c
    \brief   BLE HAL for TUYA

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

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "ble_export.h"
#include "ble_adapter.h"
#include "ble_adv.h"
#include "ble_scan.h"
#include "ble_conn.h"
#include "ble_l2cap_coc.h"
#include "ble_sec.h"
#include "ble_gatts.h"
#include "ble_gattc_co.h"
#include "ble_gattc.h"
#include "ble_storage.h"
#include "dbg_print.h"
#include "ble_utils.h"

#include "wrapper_os.h"

#include "ble_gap.h"
#include "dlist.h"
#include "tuya_ble_hal.h"
#include "ble_sec.h"
#include "gd32vw55x_platform.h"

/*
 * DEFINES
 ****************************************************************************************
 */
#define TUYA_BLE_ROLE_SERVER    (0x01)  /**< Gatt Server Role, for ble peripheral.*/
#define TUYA_BLE_ROLE_CLIENT    (0x02)  /**< Gatt Client Role, for ble central. */

/// Adapter state machine
typedef enum
{
    TUYA_ADP_IDLE = 0,
    TUYA_ADP_INITIALING,
    TUYA_ADP_RESETTING,
    TUYA_ADP_INITIALED,
    TUYA_ADP_INITFAIL,
} tuya_adp_state_t;

/// Definitions of the different ble task priorities
enum
{
    // Priority of the BLE stack task
    TKL_BLE_STACK_TASK_PRIORITY = OS_TASK_PRIORITY(2),

    // Priority of the BLE APP task
    TKL_BLE_APP_TASK_PRIORITY   = OS_TASK_PRIORITY(1),
};

// Definitions of the different ble task stack size requirements
enum
{
    // BLE task stack size
    TKL_BLE_STACK_TASK_STACK_SIZE = 1024,//768,

    // BLE APP task stack size
    TKL_BLE_APP_TASK_STACK_SIZE   = 1024,//512
};

typedef struct tkl_read_info {
    dlist_t     list;
    uint8_t     conn_idx;
    uint16_t    char_handle;
    uint16_t    token;
    uint16_t    max_len;
    uint16_t    offset;
} tkl_read_info_t;

enum tkl_ble_app_evt_id
{
    TKL_BLE_APP_EVT_RSV               = 0x00,
    TKL_BLE_APP_ADP,
    TKL_BLE_APP_SCAN,
    TKL_BLE_APP_ADV,
    TKL_BLE_APP_CONN,
    TKL_BLE_APP_GATTS,
    TKL_BLE_APP_GATTC,
};

typedef struct ble_scan_info
{
    TKL_BLE_GAP_SCAN_PARAMS_T     params;
} ble_scan_info_t;

typedef struct {
    uint16_t    length;
    uint8_t     data[0];
} tkl_ble_adv_data_t;

typedef struct {
    tkl_ble_adv_data_t    *p_adv_data;
    tkl_ble_adv_data_t    *p_rsp_data;
} tkl_ble_adv_rsp_data_t;


typedef struct ble_adv_info
{
    union
    {
        TKL_BLE_GAP_ADV_PARAMS_T     params;
        tkl_ble_adv_rsp_data_t       adv_data;
    } data;
} ble_adv_info_t;

typedef struct ble_init_data
{
    TKL_BLE_GAP_ADDR_T            peer_addr;
    TKL_BLE_GAP_SCAN_PARAMS_T     scan_params;
    TKL_BLE_GAP_CONN_PARAMS_T     conn_params;
} ble_init_data_t;


typedef struct ble_conn_info
{
    uint16_t    conn_handle;
    union
    {
        ble_init_data_t              conn_params;
        TKL_BLE_GAP_CONN_PARAMS_T    conn_upd_params;
        uint8_t                      disconn_reason;
        char                       *p_name;
    } data;
} ble_conn_info_t;

typedef struct ble_gatts_add_srvs
{
    uint8_t                 uuid_len;
    uint8_t                 uuid128[16];
    uint16_t                start_handle;
    uint16_t                total_handles;
    uint16_t                cccd_num;
    ble_gatt_attr_desc_t    *p_srv_table;
} ble_gatts_add_srvs_t;

typedef struct ble_gatts_val_info
{
    uint16_t                conn_handle;
    uint16_t                char_handle;
    uint16_t                length;
    uint8_t                 *p_data;
} ble_gatts_val_info_t;

typedef struct ble_gatts_info
{
    union
    {
        ble_gatts_add_srvs_t       add_srvs;
        ble_gatts_val_info_t       val_info;
    } data;
} ble_gatts_info_t;


typedef struct ble_gattc_disc_info
{
    uint16_t                start_handle;
    uint16_t                end_handle;
} ble_gattc_disc_info_t;

typedef struct ble_gattc_write_info
{
    uint16_t                char_handle;
    uint16_t                val_len;
    uint8_t                 *p_value;
} ble_gattc_write_info_t;

typedef struct ble_gattc_info
{
    uint16_t conn_handle;
    union
    {
        ble_gattc_disc_info_t     disc_info;
        ble_gattc_write_info_t    write_info;
        uint16_t                  read_char_handle;
    } data;
} ble_gattc_info_t;

typedef struct tkl_adv_actv
{
    uint8_t idx                 /**< Advertising set index. */;
    uint8_t type;               /**< advertising type, @ref ble_gap_adv_type_t. */
    uint16_t prop;              /**< Advertising properties.
                                    @ref ble_gap_legacy_adv_prop_t for legacy advertising,
                                    @ref ble_gap_extended_adv_prop_t for extended advertising,
                                    @ref ble_gap_periodic_adv_prop_t for periodic advertising. */
    uint8_t pri_phy;            /**< Indicate on which PHY primary advertising has to be performed, @ref ble_gap_phy_t. */
    uint8_t sec_phy;            /**< Indicate on which PHY secondary advertising has to be performed, @ref ble_gap_phy_t. */
    bool wl_enable;             /**< True to use whitelist, otherwise do not use. */
    uint8_t own_addr_type;      /**< Own address type used in advertising, @ref ble_gap_local_addr_type_t. */
    uint8_t disc_mode;          /**< Discovery mode, @ref ble_gap_adv_discovery_mode_t. */
    ble_gap_addr_t peer_addr;   /**< Peer address, used for directed advertising. */
    ble_adv_state_t state;      /**< Advertising state. */

    tkl_ble_adv_data_t *p_adv;
    tkl_ble_adv_data_t *p_rsp;
} tkl_adv_actv_t;

typedef struct {
    dlist_t       list;
    uint8_t       srv_id;
    uint16_t      svc_handle;
    uint16_t      cccd_num;
    uint32_t      cccd_handle_tuple[0];       // char_handle | cccd_handle
} ble_cccd_info_t;

typedef struct
{
    dlist_t       list;
    uint8_t       role;
    uint8_t       conidx;
    uint16_t      conn_handle;
} tkl_conn_dev_node_t;

// Adapter environment structure
struct tkl_adp_env_tag
{
    uint8_t                 role;
    BOOL_T                  user_enable;
    uint8_t                 adapter_state;
    dlist_t                 read_list;
    dlist_t                 srv_cccd_list;
    dlist_t                 conn_dev_list;
    uint16_t                srv_add_handle;
    tkl_adv_actv_t          adv_info;
};

typedef struct tkl_app_msg
{
    uint16_t      id;

    union
    {
        ble_scan_info_t       scan_info;
        ble_adv_info_t        adv_info;
        ble_conn_info_t       conn_info;
        ble_gatts_info_t      gatts_info;
        ble_gattc_info_t      gattc_info;
    } data;
} tkl_app_msg_t;

#define TKL_ADV_INVALID_IDX         (0xFF)
/** @brief BLE advertising type. */
#define TKL_BLE_ADV_TYPE_LEGACY     0       /**< Legacy advertising. */
#define TKL_BLE_ADV_TYPE_EXTENDED   1       /**< Extended advertising. */
#define TKL_BLE_ADV_TYPE_PERIODIC   2       /**< Periodic advertising. */

#define TKL_BLE_EVT_ID_TYPE_GET(id)         ((id & 0xFF00) >> 8)
#define TKL_BLE_EVT_ID_SUBTYPE_GET(id)      (id & 0xFF)

#define TKL_BLE_EVT_ID_ADP(subtype)         ((TKL_BLE_APP_ADP << 8)   | subtype)
#define ADP_SUBTYPE_DISABLE       0
#define ADP_SUBTYPE_ENABLE        1

#define TKL_BLE_EVT_ID_SCAN(subtype)         ((TKL_BLE_APP_SCAN << 8)   | subtype)
#define SCAN_SUBTYPE_DISABLE       0
#define SCAN_SUBTYPE_ENABLE        1

#define TKL_BLE_EVT_ID_ADV(subtype)         ((TKL_BLE_APP_ADV << 8)   | subtype)
#define ADV_SUBTYPE_STOP          0
#define ADV_SUBTYPE_START         1
#define ADV_SUBTYPE_DATA_SET      2
#define ADV_SUBTYPE_DATA_UPD      3

#define TKL_BLE_EVT_ID_CONN(subtype)         ((TKL_BLE_APP_CONN << 8)   | subtype)
#define CONN_SUBTYPE_DISCON                   0
#define CONN_SUBTYPE_CONN                     1
#define CONN_SUBTYPE_CONN_PARAM_UPD           2
#define CONN_SUBTYPE_CONN_RSSI_GET            3
#define CONN_SUBTYPE_NAME_SET                 4

#define TKL_BLE_EVT_ID_GATTS(subtype)         ((TKL_BLE_APP_GATTS << 8)   | subtype)
#define GATTS_SUBTYPE_ADD_SRVS                0
#define GATTS_SUBTYPE_SET_VAL                 1
#define GATTS_SUBTYPE_VAL_NOTIFY              2
#define GATTS_SUBTYPE_VAL_IND                 3

#define TKL_BLE_EVT_ID_GATTC(subtype)         ((TKL_BLE_APP_GATTC << 8)   | subtype)
#define GATTC_SUBTYPE_DISC_SRV                0
#define GATTC_SUBTYPE_DISC_CHAR               1
#define GATTC_SUBTYPE_DISC_DESC               2
#define GATTC_SUBTYPE_WRITE_CMD               3
#define GATTC_SUBTYPE_WRITE_REQ               4
#define GATTC_SUBTYPE_READ_REQ                5

extern ble_status_t ble_gatts_ntf_ind_send_by_handle(uint8_t conn_idx, uint16_t handle, uint8_t *p_val,
                                              uint16_t len, ble_gatt_evt_type_t evt_type);

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/*
 * LOCAL VARIABLES DEFINITIONS
 ****************************************************************************************
 */
static uint16_t tkl_dev_appearance = 0x0000;    //Generic Unknown

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
struct tkl_adp_env_tag adp_env = {0};
static TKL_BLE_GAP_EVT_FUNC_CB gap_evt_cb = NULL;
static TKL_BLE_GATT_EVT_FUNC_CB gatt_evt_cb = NULL;
static os_sema_t tuya_ble_sema;

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/*
 * Adapter Application
 ****************************************************************************************
 */
static void handle_adp_msg(uint8_t type)
{
    switch(type) {
    case ADP_SUBTYPE_DISABLE: {
    } break;

    case ADP_SUBTYPE_ENABLE: {
        TKL_BLE_GAP_PARAMS_EVT_T params_evt;

        if (adp_env.adapter_state == TUYA_ADP_INITIALED) {
            params_evt.type = TKL_BLE_EVT_STACK_INIT;
            params_evt.result = OPRT_OK;
            if (gap_evt_cb) {
                gap_evt_cb(&params_evt);
            }
        }
        else if (adp_env.adapter_state == TUYA_ADP_INITFAIL) {
            params_evt.type = TKL_BLE_EVT_STACK_INIT;
            params_evt.result = OPRT_OS_ADAPTER_BLE_INIT_FAILED;
            if (gap_evt_cb) {
                gap_evt_cb(&params_evt);
            }
        }
        else {
            adp_env.user_enable = TRUE;
        }
    } break;

    default:
      break;
    }
}

static void tkl_ble_adp_evt_handler(ble_adp_evt_t event, ble_adp_data_u *p_data)
{
    uint8_t i = 0;
    switch (event) {
    case BLE_ADP_EVT_ENABLE_CMPL_INFO: {
        TKL_BLE_GAP_PARAMS_EVT_T params_evt;

        params_evt.type = TKL_BLE_EVT_STACK_INIT;
        if (p_data->adapter_info.status == BLE_ERR_NO_ERROR) {
            dbg_print(NOTICE, "=== Adapter enable success ===\r\n");
            dbg_print(INFO, "hci_ver 0x%x, hci_subver 0x%x, lmp_ver 0x%x, lmp_subver 0x%x, manuf_name 0x%x\r\n",
                   p_data->adapter_info.version.hci_ver, p_data->adapter_info.version.hci_subver,
                   p_data->adapter_info.version.lmp_ver, p_data->adapter_info.version.lmp_subver,
                   p_data->adapter_info.version.manuf_name);

            dbg_print(INFO, "adv_set_num %u, min_tx_pwr %d, max_tx_pwr %d, max_adv_data_len %d \r\n",
                   p_data->adapter_info.adv_set_num, p_data->adapter_info.tx_pwr_range.min_tx_pwr,
                   p_data->adapter_info.tx_pwr_range.max_tx_pwr, p_data->adapter_info.max_adv_data_len);
            dbg_print(INFO, "sugg_max_tx_octets %u, sugg_max_tx_time %u \r\n",
                   p_data->adapter_info.sugg_dft_data.sugg_max_tx_octets,
                   p_data->adapter_info.sugg_dft_data.sugg_max_tx_time);

            dbg_print(INFO, "loc irk:");

            for (i = 0; i < BLE_GAP_KEY_LEN; i++) {
                dbg_print(INFO, " %02x", p_data->adapter_info.loc_irk_info.irk[i]);
            }

            dbg_print(INFO, "\r\n");
            dbg_print(INFO, "identity addr %02X:%02X:%02X:%02X:%02X:%02X \r\n ",
                   p_data->adapter_info.loc_irk_info.identity.addr[5],
                   p_data->adapter_info.loc_irk_info.identity.addr[4],
                   p_data->adapter_info.loc_irk_info.identity.addr[3],
                   p_data->adapter_info.loc_irk_info.identity.addr[2],
                   p_data->adapter_info.loc_irk_info.identity.addr[1],
                   p_data->adapter_info.loc_irk_info.identity.addr[0]);

            dbg_print(NOTICE, "=== Tuya BLE Adapter enable complete ===\r\n");

            adp_env.adapter_state = TUYA_ADP_INITIALED;
            params_evt.result = OPRT_OK;
        }
        else {
            dbg_print(NOTICE, "=== Tuya BLE Adapter enable fail ===\r\n");
            params_evt.result = OPRT_OS_ADAPTER_BLE_INIT_FAILED;
            adp_env.adapter_state = TUYA_ADP_INITFAIL;
        }

        if (gap_evt_cb && adp_env.user_enable) {
            gap_evt_cb(&params_evt);
        }

        sys_sema_up(&tuya_ble_sema);
    } break;

    default:
        break;
    }
}

/*
 * Scan Application
 ****************************************************************************************
 */
static void tkl_ble_app_scan_mgr_evt_handler(ble_scan_evt_t scan_event, ble_scan_data_u *p_data)
{
    TKL_BLE_GAP_PARAMS_EVT_T event;

    switch (scan_event) {
    case BLE_SCAN_EVT_STATE_CHG:
        if (p_data->scan_state.scan_state == BLE_SCAN_STATE_ENABLED) {
            dbg_print(NOTICE, "Ble Scan enabled status 0x%x\r\n", p_data->scan_state.reason);
        } else if (p_data->scan_state.scan_state == BLE_SCAN_STATE_DISABLED) {
            dbg_print(NOTICE, "Ble Scan disabled  status 0x%x\r\n", p_data->scan_state.reason);
        }
        break;

    case BLE_SCAN_EVT_ADV_RPT:
    {
        event.type = TKL_BLE_GAP_EVT_ADV_REPORT;
        event.result = OPRT_OK;

        if (p_data->p_adv_rpt->type.extended_pdu) {
            event.gap_event.adv_report.adv_type = TKL_BLE_EXTENDED_ADV_DATA;
        } else {
            if (p_data->p_adv_rpt->type.scan_response) {
                event.gap_event.adv_report.adv_type = TKL_BLE_RSP_DATA;
            } else if (p_data->p_adv_rpt->type.connectable) {
                event.gap_event.adv_report.adv_type = TKL_BLE_ADV_DATA;
            } else {
                event.gap_event.adv_report.adv_type = TKL_BLE_NONCONN_ADV_DATA;
            }
        }
        event.gap_event.adv_report.peer_addr.type = p_data->p_adv_rpt->peer_addr.addr_type;
        sys_memcpy(event.gap_event.adv_report.peer_addr.addr, p_data->p_adv_rpt->peer_addr.addr, 6);

        event.gap_event.adv_report.rssi = p_data->p_adv_rpt->rssi;
        event.gap_event.adv_report.channel_index = 0xFF;
        event.gap_event.adv_report.data.length = p_data->p_adv_rpt->data.len;
        event.gap_event.adv_report.data.p_data = p_data->p_adv_rpt->data.p_data;

        if (gap_evt_cb)
            gap_evt_cb(&event);
    }
    break;
    }
}

static void handle_scan_msg(uint8_t type, ble_scan_info_t *p_scan_info)
{
    switch(type) {
    case SCAN_SUBTYPE_DISABLE: {
        ble_scan_disable();
    } break;

    case SCAN_SUBTYPE_ENABLE: {
        ble_gap_scan_param_t param;

        param.type = BLE_GAP_SCAN_TYPE_GEN_DISC;
        param.prop = 0;

        if (p_scan_info->params.scan_phys == TKL_BLE_GAP_PHY_AUTO) {
            param.prop |= (BLE_GAP_SCAN_PROP_PHY_1M_BIT | BLE_GAP_SCAN_PROP_ACTIVE_CODED_BIT);
        } else {
            if (p_scan_info->params.scan_phys & TKL_BLE_GAP_PHY_1MBPS) {
                param.prop |= BLE_GAP_SCAN_PROP_PHY_1M_BIT;
                if (p_scan_info->params.active) {
                     param.prop |= BLE_GAP_SCAN_PROP_ACTIVE_1M_BIT;
                }
            }

            if (p_scan_info->params.scan_phys & TKL_BLE_GAP_PHY_CODED) {
                param.prop |= BLE_GAP_SCAN_PROP_ACTIVE_CODED_BIT;
                if (p_scan_info->params.active) {
                     param.prop |= BLE_GAP_SCAN_PROP_ACTIVE_CODED_BIT;
                }
            }
        }

        param.dup_filt_pol = BLE_GAP_DUP_FILT_EN;

        param.scan_intv_1m    = p_scan_info->params.interval;
        param.scan_intv_coded = p_scan_info->params.interval;
        param.scan_win_1m    = p_scan_info->params.window;
        param.scan_win_coded = p_scan_info->params.window;
        param.duration = p_scan_info->params.timeout;
        param.period   = 0;

        dbg_print(NOTICE,
                  "ScanParam: type=%u prop=0x%08X dup_filt=%u intv_1m=%u win_1m=%u intv_coded=%u win_coded=%u duration=%u period=%u active=%u phys=0x%02X\r\n",
                  param.type,
                  param.prop,
                  param.dup_filt_pol,
                  param.scan_intv_1m,
                  param.scan_win_1m,
                  param.scan_intv_coded,
                  param.scan_win_coded,
                  param.duration,
                  param.period,
                  p_scan_info->params.active,
                  p_scan_info->params.scan_phys);

        ble_scan_param_set(BLE_GAP_LOCAL_ADDR_STATIC, &param);
        ble_scan_enable();

    } break;

    default:
      break;
    }
}

/*
 * Security Application
 ****************************************************************************************
 */
static void tkl_ble_app_sec_evt_handler(ble_sec_evt_t event, ble_sec_data_u *p_data)
{

    switch (event) {
    case BLE_SEC_EVT_PAIRING_REQ_IND:
        ble_sec_pairing_req_cfm(p_data->pairing_req_ind.conn_idx, false, NULL, 0);
        break;

    case BLE_SEC_EVT_LTK_REQ_IND:
        ble_sec_ltk_req_cfm(p_data->ltk_req_ind.conn_idx, false, NULL);
        break;

    case BLE_SEC_EVT_KEY_DISPLAY_REQ_IND:
        ble_sec_key_display_enter_cfm(p_data->tk_req_ind.conn_idx, false, 0);
        break;

    case BLE_SEC_EVT_KEY_ENTER_REQ_IND:
        ble_sec_key_display_enter_cfm(p_data->tk_req_ind.conn_idx, false, 0);
        break;

    case BLE_SEC_EVT_KEY_OOB_REQ_IND:
        ble_sec_oob_req_cfm(p_data->tk_req_ind.conn_idx, false, NULL);
        break;

    case BLE_SEC_EVT_NUMERIC_COMPARISON_IND:
        ble_sec_nc_cfm(p_data->nc_ind.conn_idx, false);
        break;

    case BLE_SEC_EVT_IRK_REQ_IND:
        ble_sec_irk_req_cfm(p_data->irk_req_ind.conn_idx, false, NULL);
        break;

    case BLE_SEC_EVT_CSRK_REQ_IND:
        ble_sec_csrk_req_cfm(p_data->csrk_req_ind.conn_idx, false, NULL);
        break;

    case BLE_SEC_EVT_OOB_DATA_REQ_IND:
        ble_sec_oob_data_req_cfm(p_data->oob_data_req_ind.conn_idx, false, NULL, NULL);
        break;

    case BLE_SEC_EVT_PAIRING_SUCCESS_INFO:
        break;

    case BLE_SEC_EVT_PAIRING_FAIL_INFO:
        break;

    case BLE_SEC_EVT_SECURITY_REQ_INFO:
        break;

    case BLE_SEC_EVT_ENCRYPT_REQ_IND:
        ble_sec_encrypt_req_cfm(p_data->enc_req_ind.conn_idx, false, NULL, 0);
        break;

    case BLE_SEC_EVT_ENCRYPT_INFO:
        break;

    case BLE_SEC_EVT_OOB_DATA_GEN_INFO:
        break;

    case BLE_SEC_EVT_KEY_PRESS_INFO:
        dbg_print(NOTICE, "conidx %u key press info type %d\r\n", p_data->key_press_info.conn_idx,
               p_data->key_press_info.type);
        break;
    default:
        break;
    }
}

/*
 * Connection Application
 ****************************************************************************************
 */
static tkl_conn_dev_node_t *tkl_ble_find_dev_by_conidx(uint8_t conidx)
{
    dlist_t *pos, *n;
    tkl_conn_dev_node_t *p_device = NULL;

    if (list_empty(&adp_env.conn_dev_list)) {
        return NULL;
    }

    list_for_each_safe(pos, n, &adp_env.conn_dev_list) {
        p_device = list_entry(pos, tkl_conn_dev_node_t, list);
        if (p_device->conidx == conidx) {
            return p_device;
        }
    }

    return NULL;
}

static tkl_conn_dev_node_t *tkl_ble_find_dev_by_conn_handle(uint16_t conn_handle)
{
    dlist_t *pos, *n;
    tkl_conn_dev_node_t *p_device = NULL;

    if (list_empty(&adp_env.conn_dev_list)) {
        return NULL;
    }

    list_for_each_safe(pos, n, &adp_env.conn_dev_list) {
        p_device = list_entry(pos, tkl_conn_dev_node_t, list);
        if (p_device->conn_handle == conn_handle) {
            return p_device;
        }
    }

    return NULL;
}

static tkl_conn_dev_node_t *tkl_ble_alloc_dev_by_conn_handle(uint16_t conn_handle)
{
    tkl_conn_dev_node_t *p_device = NULL;

    p_device = (tkl_conn_dev_node_t *)sys_malloc(sizeof(tkl_conn_dev_node_t));

    if (p_device == NULL) {
        return NULL;
    }
    sys_memset(p_device, 0, sizeof(tkl_conn_dev_node_t));

    INIT_DLIST_HEAD(&p_device->list);

    p_device->conn_handle = conn_handle;

    list_add_tail(&p_device->list, &adp_env.conn_dev_list);
    return p_device;
}

static tkl_conn_dev_node_t *tkl_ble_find_alloc_dev_by_conn_handle(uint16_t conn_handle)
{
    tkl_conn_dev_node_t *p_device = tkl_ble_find_dev_by_conn_handle(conn_handle);

    if (p_device != NULL) {
        return p_device;
    }

    p_device = tkl_ble_alloc_dev_by_conn_handle(conn_handle);
    return p_device;
}

static void tkl_ble_remove_dev_by_conn_handle(uint16_t conn_handle)
{
    tkl_conn_dev_node_t *p_device = tkl_ble_find_dev_by_conn_handle(conn_handle);

    if (p_device != NULL) {
        list_del(&p_device->list);
        sys_mfree(p_device);
    }
}

static void tkl_ble_app_conn_evt_handler(ble_conn_evt_t event, ble_conn_data_u *p_data)
{
    switch (event) {
    case BLE_CONN_EVT_INIT_STATE_CHG: {
        if (p_data->init_state.state == BLE_INIT_STATE_IDLE) {
            dbg_print(NOTICE, "===> init conn idle idx %u, wl_used %d reason 0x%x\r\n",
                   p_data->init_state.init_idx, p_data->init_state.wl_used, p_data->init_state.reason);
        } else if (p_data->init_state.state == BLE_INIT_STATE_STARTING) {
            dbg_print(NOTICE, "===> init conn starting idx %u, wl_used %d\r\n",
                   p_data->init_state.init_idx, p_data->init_state.wl_used);
        } else if (p_data->init_state.state == BLE_INIT_STATE_STARTED) {
            dbg_print(NOTICE, "===> init conn started idx %u, wl_used %d\r\n",
                   p_data->init_state.init_idx, p_data->init_state.wl_used);
        } else if (p_data->init_state.state == BLE_INIT_STATE_DISABLING) {
            dbg_print(NOTICE, "===> init conn disabling idx %u, wl_used %d reason 0x%x\r\n",
                   p_data->init_state.init_idx, p_data->init_state.wl_used, p_data->init_state.reason);
            // Disabling state with reason is not BLE_ERR_NO_ERROR which means init connecting not clear completely.
            // Needs to call ble_conn_connect_cancel again
            if (p_data->init_state.reason != BLE_ERR_NO_ERROR) {
                if (ble_conn_connect_cancel() != BLE_ERR_NO_ERROR) {
                    dbg_print(NOTICE, "===> init conn disabling idx %u, cancel connecting fail!\r\n",
                           p_data->init_state.init_idx);
                }
            }
        }
    }
    break;

    case BLE_CONN_EVT_STATE_CHG: {
        TKL_BLE_GAP_PARAMS_EVT_T params_evt;

        if (p_data->conn_state.state == BLE_CONN_STATE_DISCONNECTD) {
            dbg_print(NOTICE, "disconnected. conn idx: %u, conn_hdl: 0x%x reason 0x%x\r\n",
                   p_data->conn_state.info.discon_info.conn_idx, p_data->conn_state.info.discon_info.conn_hdl,
                   p_data->conn_state.info.discon_info.reason);

            tkl_conn_dev_node_t *p_dev = tkl_ble_find_dev_by_conn_handle(p_data->conn_state.info.discon_info.conn_hdl);

            if (p_dev) {
                params_evt.type = TKL_BLE_GAP_EVT_DISCONNECT;
                params_evt.conn_handle = p_data->conn_state.info.discon_info.conn_hdl;
                params_evt.result = OPRT_OK;
                params_evt.gap_event.disconnect.role = p_dev->role;
                params_evt.gap_event.disconnect.reason = p_data->conn_state.info.discon_info.reason;

                if (gap_evt_cb) {
                    gap_evt_cb(&params_evt);
                }
            }
        } else if (p_data->conn_state.state == BLE_CONN_STATE_CONNECTED) {
            tkl_conn_dev_node_t *p_dev = tkl_ble_find_alloc_dev_by_conn_handle(p_data->conn_state.info.conn_info.conn_hdl);

            if (p_dev) {
                p_dev->conidx = p_data->conn_state.info.conn_info.conn_idx;
                p_dev->role = p_data->conn_state.info.conn_info.role == 0 ? TKL_BLE_ROLE_CLIENT : TKL_BLE_ROLE_SERVER;
            }

            params_evt.type = TKL_BLE_GAP_EVT_CONNECT;
            params_evt.conn_handle = p_data->conn_state.info.conn_info.conn_hdl;
            params_evt.result = OPRT_OK;
            params_evt.gap_event.connect.role = p_data->conn_state.info.conn_info.role == 0 ? TKL_BLE_ROLE_CLIENT : TKL_BLE_ROLE_SERVER;
            params_evt.gap_event.connect.peer_addr.type = p_data->conn_state.info.conn_info.peer_addr.addr_type;
            sys_memcpy(params_evt.gap_event.connect.peer_addr.addr, p_data->conn_state.info.conn_info.peer_addr.addr, BLE_GAP_ADDR_LEN);
            params_evt.gap_event.connect.conn_params.conn_interval_min = p_data->conn_state.info.conn_info.con_interval;
            params_evt.gap_event.connect.conn_params.conn_interval_max = p_data->conn_state.info.conn_info.con_interval;
            params_evt.gap_event.connect.conn_params.conn_latency = p_data->conn_state.info.conn_info.con_latency;
            params_evt.gap_event.connect.conn_params.conn_sup_timeout = p_data->conn_state.info.conn_info.sup_to;

            if (gap_evt_cb) {
                gap_evt_cb(&params_evt);
            }

        }
    }
    break;

    case BLE_CONN_EVT_DISCONN_RSP: {
        dbg_print(NOTICE, "disconnect rsp. conn idx %u, status 0x%x\r\n",
                  p_data->disconn_rsp.conn_idx, p_data->disconn_rsp.status);
    }
    break;

    case BLE_CONN_EVT_PEER_NAME_GET_RSP: {
        if (p_data->peer_name.status == BLE_ERR_NO_ERROR) {
            dbg_print(NOTICE, "conn idx: %u, peer name: %s\r\n",
                   p_data->peer_name.conn_idx, p_data->peer_name.p_name);
        }
    }
    break;

    case BLE_CONN_EVT_PEER_VERSION_GET_RSP: {
        if (p_data->peer_version.status == BLE_ERR_NO_ERROR) {
            dbg_print(NOTICE, "conn idx: %u, peer version: 0x%x, subversion: 0x%x, comp id 0x%x\r\n",
                   p_data->peer_version.conn_idx,
                   p_data->peer_version.lmp_version, p_data->peer_version.lmp_subversion,
                   p_data->peer_version.company_id);
        }
    }
    break;

    case BLE_CONN_EVT_PEER_FEATS_GET_RSP: {
        if (p_data->peer_features.status == BLE_ERR_NO_ERROR) {
            dbg_print(NOTICE, "conn idx: %u, peer feature: 0x%02x%02x%02x%02x%02x%02x%02x%02x\r\n",
                   p_data->peer_features.conn_idx,
                   p_data->peer_features.features[7], p_data->peer_features.features[6],
                   p_data->peer_features.features[5], p_data->peer_features.features[4],
                   p_data->peer_features.features[3], p_data->peer_features.features[2],
                   p_data->peer_features.features[1], p_data->peer_features.features[0]);
        }
    }
    break;

    case BLE_CONN_EVT_PEER_APPEARANCE_GET_RSP: {
        if (p_data->peer_appearance.status == BLE_ERR_NO_ERROR) {
            dbg_print(NOTICE, "conn idx: %u, peer appearance: 0x%x\r\n", p_data->peer_appearance.conn_idx,
                   p_data->peer_appearance.appearance);
        }
    }
    break;

    case BLE_CONN_EVT_PEER_SLV_PRF_PARAM_GET_RSP: {
        if (p_data->peer_slv_prf_param.status == BLE_ERR_NO_ERROR) {
            dbg_print(NOTICE, "conn idx: %u, conn_intv_min: 0x%x, conn_intv_max: 0x%x, latency: %d, timeout: %d\r\n",
                   p_data->peer_slv_prf_param.conn_idx,
                   p_data->peer_slv_prf_param.conn_intv_min, p_data->peer_slv_prf_param.conn_intv_max,
                   p_data->peer_slv_prf_param.latency,
                   p_data->peer_slv_prf_param.conn_timeout);
        }
    }
    break;

    case BLE_CONN_EVT_PEER_ADDR_RESLV_GET_RSP: {
        if (p_data->peer_addr_reslv_sup.status == BLE_ERR_NO_ERROR) {
            dbg_print(NOTICE, "conn idx: %u, central address resolution support %d\r\n", p_data->peer_addr_reslv_sup.conn_idx,
                   p_data->peer_addr_reslv_sup.ctl_addr_resol);
        }
    }
    break;

    case BLE_CONN_EVT_PEER_RPA_ONLY_GET_RSP: {
        if (p_data->rpa_only.status == BLE_ERR_NO_ERROR) {
            dbg_print(NOTICE, "conn idx: %u, central rpa only %d\r\n", p_data->rpa_only.conn_idx,
                   p_data->rpa_only.rpa_only);
        }
    }
    break;

    case BLE_CONN_EVT_PEER_DB_HASH_GET_RSP: {
        if (p_data->db_hash.status == BLE_ERR_NO_ERROR) {
            uint8_t i;
            dbg_print(NOTICE, "conn idx: %u, db_hash\r\n", p_data->db_hash.conn_idx);
            for (i = 0; i < 16; i++) {
                dbg_print(NOTICE, "%02x", p_data->db_hash.hash[i]);
            }

            dbg_print(NOTICE, "\r\n");
        }
    }
    break;

    case BLE_CONN_EVT_PING_TO_VAL_GET_RSP: {
        if (p_data->ping_to_val.status == BLE_ERR_NO_ERROR) {
            dbg_print(NOTICE, "conn idx %u ping timeout %d\r\n", p_data->ping_to_val.conn_idx,
                   p_data->ping_to_val.ping_tout);
        }
    }
    break;

    case BLE_CONN_EVT_PING_TO_INFO: {
        dbg_print(NOTICE, "conn idx %u ping timeout\r\n", p_data->ping_timeout.conn_idx);
    }
    break;

    case BLE_CONN_EVT_PING_TO_SET_RSP: {
        dbg_print(NOTICE, "conn idx %u ping timeout set status 0x%x\r\n", p_data->ping_to_set.conn_idx,
               p_data->ping_to_set.status);
    }
    break;

    case BLE_CONN_EVT_RSSI_GET_RSP: {
        TKL_BLE_GAP_PARAMS_EVT_T params_evt;
        tkl_conn_dev_node_t *p_dev = tkl_ble_find_dev_by_conidx(p_data->rssi_ind.conn_idx);

        if (p_dev == NULL) {
            break;
        }

        dbg_print(NOTICE, "conn idx %u rssi: %d\r\n", p_data->rssi_ind.conn_idx, p_data->rssi_ind.rssi);

        params_evt.type = TKL_BLE_GAP_EVT_CONN_RSSI;
        params_evt.conn_handle = p_dev->conn_handle;

        if (p_data->rssi_ind.status != BLE_ERR_NO_ERROR) {
            params_evt.result = OPRT_OS_ADAPTER_BLE_RESERVED1;
        } else {
            params_evt.result = OPRT_OK;
            params_evt.gap_event.link_rssi = p_data->rssi_ind.rssi;
        }

        if (gap_evt_cb) {
            gap_evt_cb(&params_evt);
        }
    }
    break;

    case BLE_CONN_EVT_CHANN_MAP_GET_RSP: {
        dbg_print(NOTICE, "channel map: 0x%02x%02x%02x%02x%02x\r\n", p_data->chnl_map_ind.chann_map[4],
               p_data->chnl_map_ind.chann_map[3], p_data->chnl_map_ind.chann_map[2],
               p_data->chnl_map_ind.chann_map[1], p_data->chnl_map_ind.chann_map[0]);
    }
    break;

    case BLE_CONN_EVT_NAME_GET_IND: {
        ble_conn_name_get_cfm(p_data->name_get_ind.conn_idx, BLE_GAP_ERR_NOT_SUPPORTED, p_data->name_get_ind.token,
                              0, NULL, 0);
    }
    break;

    case BLE_CONN_EVT_APPEARANCE_GET_IND: {
        dbg_print(NOTICE, "conn idx %u appearance acquire \r\n", p_data->appearance_get_ind.conn_idx);

        ble_conn_appearance_get_cfm(p_data->appearance_get_ind.conn_idx, BLE_ERR_NO_ERROR,
                                    p_data->appearance_get_ind.token, tkl_dev_appearance);
    }
    break;

    case BLE_CONN_EVT_SLAVE_PREFER_PARAM_GET_IND: {
        ble_gap_slave_prefer_param_t param;

        dbg_print(NOTICE, "conn idx %u slave prefer parameters acquire \r\n",
               p_data->slave_prefer_param_get_ind.conn_idx);

        param.conn_intv_min = 8;
        param.conn_intv_max = 10;
        param.latency = 0;
        param.conn_tout = 200;  //2s

        ble_conn_slave_prefer_param_get_cfm(p_data->slave_prefer_param_get_ind.conn_idx, BLE_ERR_NO_ERROR,
                                            p_data->slave_prefer_param_get_ind.token, &param);
    }
    break;

    case BLE_CONN_EVT_NAME_SET_IND: {
        ble_adp_name_set(p_data->name_set_ind.p_name, p_data->name_set_ind.name_len);

        dbg_print(NOTICE, "conn idx %u, name set %s, name len %u \r\n",
               p_data->name_set_ind.conn_idx, p_data->name_set_ind.p_name,
               p_data->name_set_ind.name_len);

        ble_conn_name_set_cfm(p_data->name_set_ind.conn_idx, BLE_ERR_NO_ERROR, p_data->name_set_ind.token);
    }
    break;

    case BLE_CONN_EVT_APPEARANCE_SET_IND: {
        tkl_dev_appearance = p_data->appearance_set_ind.appearance;

        dbg_print(NOTICE, "conn idx %u, appearance set 0x%x\r\n", p_data->appearance_set_ind.conn_idx,
               p_data->appearance_set_ind.appearance);

        ble_conn_appearance_set_cfm(p_data->appearance_set_ind.conn_idx, BLE_ERR_NO_ERROR,
                                    p_data->appearance_set_ind.token);
    }
    break;

    case BLE_CONN_EVT_PARAM_UPDATE_IND: {
        dbg_print(NOTICE, "conn idx %u, intv_min 0x%x, intv_max 0x%x, latency %u, supv_tout %u\r\n",
               p_data->conn_param_req_ind.conn_idx, p_data->conn_param_req_ind.intv_min,
               p_data->conn_param_req_ind.intv_max, p_data->conn_param_req_ind.latency,
               p_data->conn_param_req_ind.supv_tout);

        if(p_data->conn_param_req_ind.intv_max < p_data->conn_param_req_ind.intv_min) {
            ble_conn_param_update_cfm(p_data->conn_param_req_ind.conn_idx, false, 0, 0);
        } else {
            TKL_BLE_GAP_PARAMS_EVT_T params_evt;
            tkl_conn_dev_node_t *p_dev = tkl_ble_find_dev_by_conidx(p_data->conn_param_req_ind.conn_idx);

            if (p_dev == NULL) {
                break;
            }

            params_evt.type = TKL_BLE_GAP_EVT_CONN_PARAM_REQ;
            params_evt.conn_handle = p_dev->conn_handle;
            params_evt.gap_event.conn_param.conn_interval_min = p_data->conn_param_req_ind.intv_min;
            params_evt.gap_event.conn_param.conn_interval_max = p_data->conn_param_req_ind.intv_max;
            params_evt.gap_event.conn_param.conn_latency = p_data->conn_param_req_ind.latency;
            params_evt.gap_event.conn_param.conn_sup_timeout = p_data->conn_param_req_ind.supv_tout;

            if (gap_evt_cb) {
                gap_evt_cb(&params_evt);
            }

            ble_conn_param_update_cfm(p_data->conn_param_req_ind.conn_idx, true, 0, 0);
        }
    }
    break;

    case BLE_CONN_EVT_PARAM_UPDATE_RSP: {
        dbg_print(NOTICE, "conn idx %u, param update result status: 0x%x\r\n",
               p_data->conn_param_rsp.conn_idx, p_data->conn_param_rsp.status);

    }
    break;

    case BLE_CONN_EVT_PARAM_UPDATE_INFO: {
        TKL_BLE_GAP_PARAMS_EVT_T params_evt;
        tkl_conn_dev_node_t *p_dev = tkl_ble_find_dev_by_conidx(p_data->conn_params.conn_idx);

        if (p_dev == NULL) {
            break;
        }

        dbg_print(NOTICE, "conn idx %u, param update ind: interval %d, latency %d, sup to %d\r\n",
               p_data->conn_params.conn_idx,
               p_data->conn_params.interval, p_data->conn_params.latency, p_data->conn_params.supv_tout);

        params_evt.type = TKL_BLE_GAP_EVT_CONN_PARAM_UPDATE;
        params_evt.result = OPRT_OK;
        params_evt.conn_handle = p_dev->conn_handle;
        params_evt.gap_event.conn_param.conn_interval_min = p_data->conn_params.interval;
        params_evt.gap_event.conn_param.conn_interval_max = p_data->conn_params.interval;
        params_evt.gap_event.conn_param.conn_latency = p_data->conn_params.latency;
        params_evt.gap_event.conn_param.conn_sup_timeout = p_data->conn_params.supv_tout;

        if (gap_evt_cb) {
            gap_evt_cb(&params_evt);
        }
    }
    break;

    case BLE_CONN_EVT_PKT_SIZE_SET_RSP: {
        dbg_print(NOTICE, "conn idx %u, packet size set status 0x%x\r\n",
               p_data->pkt_size_set_rsp.conn_idx, p_data->pkt_size_set_rsp.status);
    }
    break;

    case BLE_CONN_EVT_PKT_SIZE_INFO: {
        dbg_print(NOTICE, "le pkt size info: conn idx %u, tx oct %d, tx time %d, rx oct %d, rx time %d\r\n",
               p_data->pkt_size_info.conn_idx, p_data->pkt_size_info.max_tx_octets,
               p_data->pkt_size_info.max_tx_time,
               p_data->pkt_size_info.max_rx_octets, p_data->pkt_size_info.max_rx_time);
    }
    break;

    case BLE_CONN_EVT_PHY_GET_RSP: {
        dbg_print(NOTICE, "conn idx %u le phy get status 0x%x\r\n", p_data->phy_get.conn_idx,
               p_data->phy_get.status);
    }
    break;

    case BLE_CONN_EVT_PHY_SET_RSP: {
        dbg_print(NOTICE, "conn idx %u le phy set status 0x%x\r\n", p_data->phy_set.conn_idx,
               p_data->phy_set.status);
    }
    break;

    case BLE_CONN_EVT_PHY_INFO: {
        dbg_print(NOTICE, "le phy ind conn idx %u: tx phy 0x%x, rx phy 0x%x\r\n", p_data->phy_val.conn_idx,
               p_data->phy_val.tx_phy, p_data->phy_val.rx_phy);
    }
    break;

    case BLE_CONN_EVT_LOC_TX_PWR_GET_RSP: {
        dbg_print(NOTICE, "local tx pwr conn idx %u, phy %d, pwr %d, max %d\r\n", p_data->loc_tx_pwr.conn_idx,
               p_data->loc_tx_pwr.phy, p_data->loc_tx_pwr.tx_pwr, p_data->loc_tx_pwr.max_tx_pwr);
    }
    break;

    case BLE_CONN_EVT_PEER_TX_PWR_GET_RSP: {
        dbg_print(NOTICE, "peer tx pwr conidx %u, pwr %d, flag 0x%x \r\n", p_data->peer_tx_pwr.conn_idx,
               p_data->peer_tx_pwr.tx_pwr, p_data->peer_tx_pwr.flags);
    }
    break;

    case BLE_CONN_EVT_LOC_TX_PWR_RPT_INFO: {
        dbg_print(NOTICE, "local tx pwr report conn idx %u, phy %d, pwr %d, flag 0x%x, delta %d\r\n",
               p_data->loc_tx_pwr_rpt.conn_idx, p_data->loc_tx_pwr_rpt.phy, p_data->loc_tx_pwr_rpt.tx_pwr,
               p_data->loc_tx_pwr_rpt.flags, p_data->loc_tx_pwr_rpt.delta);
    }
    break;

    case BLE_CONN_EVT_PEER_TX_PWR_RPT_INFO: {
        dbg_print(NOTICE, "peer tx pwr report conn idx %u, phy %d, pwr %d, flag 0x%x, delta %d\r\n",
               p_data->peer_tx_pwr_rpt.conn_idx, p_data->loc_tx_pwr_rpt.phy, p_data->loc_tx_pwr_rpt.tx_pwr,
               p_data->loc_tx_pwr_rpt.flags, p_data->loc_tx_pwr_rpt.delta);
    }
    break;

    case BLE_CONN_EVT_PATH_LOSS_THRESHOLD_INFO: {
        dbg_print(NOTICE, "path loss threshold conn idx %u, curr %d, zone %d\r\n",
               p_data->loc_tx_pwr_rpt.conn_idx, p_data->path_loss_thr.curr_path_loss,
               p_data->path_loss_thr.zone_entered);
    }
    break;

    case BLE_CONN_EVT_PATH_LOSS_CTRL_RSP: {
        dbg_print(NOTICE, "path loss ctrl conn idx %u, status 0x%x\r\n",
               p_data->path_ctrl.conn_idx, p_data->path_ctrl.status);
    }
    break;

    case BLE_CONN_EVT_PER_SYNC_TRANS_RSP: {
        dbg_print(NOTICE, "periodic sync transfer result conn idx %u, status 0x%x\r\n",
               p_data->sync_trans_rsp.conn_idx, p_data->sync_trans_rsp.status);
    }
    break;

    case BLE_CONN_EVT_TX_PWR_RPT_CTRL_RSP: {
        dbg_print(NOTICE, "Tx power report contrl result conn idx %u, status 0x%x\r\n",
               p_data->tx_pwr_rpt_ctrl_rsp.conn_idx, p_data->tx_pwr_rpt_ctrl_rsp.status);
    }
    break;

    default:
        break;
    }
}

static void handle_conn_msg(uint8_t type, ble_conn_info_t *p_conn_info)
{
    tkl_conn_dev_node_t *p_dev = tkl_ble_find_dev_by_conn_handle(p_conn_info->conn_handle);

    if (p_dev == NULL && type != CONN_SUBTYPE_CONN) {
        return;
    }

    switch(type) {
    case CONN_SUBTYPE_DISCON: {
        if (ble_conn_disconnect(p_dev->conidx, BLE_ERROR_HL_TO_HCI(p_conn_info->data.disconn_reason)) != BLE_ERR_NO_ERROR) {
            app_print("disconnect connection fail \r\n");
        }
    } break;

    case CONN_SUBTYPE_CONN: {
        ble_gap_init_param_t param;
        ble_gap_addr_t peer_addr_info;
        ble_init_data_t *p_conn_data = &p_conn_info->data.conn_params;

        // param.type = BLE_GAP_INIT_TYPE_DIRECT_CONN_EST;

        param.prop = BLE_GAP_INIT_PROP_1M_BIT | BLE_GAP_INIT_PROP_2M_BIT |
                                     BLE_GAP_INIT_PROP_CODED_BIT;
        param.conn_tout = p_conn_data->conn_params.connection_timeout;

        param.scan_intv_1m      = p_conn_data->scan_params.interval;
        param.scan_win_1m       = p_conn_data->scan_params.window;
        param.scan_intv_coded   = p_conn_data->scan_params.interval;
        param.scan_win_coded    = p_conn_data->scan_params.window;

        param.conn_param_1m.conn_intv_min   = p_conn_data->conn_params.conn_interval_min;
        param.conn_param_1m.conn_intv_max   = p_conn_data->conn_params.conn_interval_max;
        param.conn_param_1m.conn_latency    = p_conn_data->conn_params.conn_latency;
        param.conn_param_1m.supv_tout       = p_conn_data->conn_params.conn_sup_timeout;
        param.conn_param_1m.ce_len_min      = 0;
        param.conn_param_1m.ce_len_max      = 0;

        param.conn_param_2m.conn_intv_min   = p_conn_data->conn_params.conn_interval_min;
        param.conn_param_2m.conn_intv_max   = p_conn_data->conn_params.conn_interval_max;
        param.conn_param_2m.conn_latency    = p_conn_data->conn_params.conn_latency;
        param.conn_param_2m.supv_tout       = p_conn_data->conn_params.conn_sup_timeout;
        param.conn_param_2m.ce_len_min      = 0;
        param.conn_param_2m.ce_len_max      = 0;

        param.conn_param_coded.conn_intv_min   = p_conn_data->conn_params.conn_interval_min;
        param.conn_param_coded.conn_intv_max   = p_conn_data->conn_params.conn_interval_max;
        param.conn_param_coded.conn_latency    = p_conn_data->conn_params.conn_latency;
        param.conn_param_coded.supv_tout       = p_conn_data->conn_params.conn_sup_timeout;
        param.conn_param_coded.ce_len_min      = 0;
        param.conn_param_coded.ce_len_max      = 0;

        peer_addr_info.addr_type = p_conn_data->peer_addr.type;
        sys_memcpy(peer_addr_info.addr, p_conn_data->peer_addr.addr, 6);

        if (ble_conn_connect(&param, BLE_GAP_LOCAL_ADDR_STATIC, &peer_addr_info, false) != BLE_ERR_NO_ERROR) {
            app_print("connect fail\r\n");
        }
    } break;

    case CONN_SUBTYPE_CONN_PARAM_UPD: {
        TKL_BLE_GAP_CONN_PARAMS_T *p_param_data = &p_conn_info->data.conn_upd_params;

        if (ble_conn_param_update_req(p_dev->conidx, p_param_data->conn_interval_min, p_param_data->conn_interval_max,
                                      p_param_data->conn_latency, p_param_data->conn_sup_timeout, 0, 0) != BLE_ERR_NO_ERROR) {
            app_print("update param fail\r\n");
        }
    } break;

    case CONN_SUBTYPE_CONN_RSSI_GET: {
        if (ble_conn_rssi_get(p_dev->conidx) != BLE_ERR_NO_ERROR) {
            app_print("get rssi fail\r\n");
        }
    } break;

    case CONN_SUBTYPE_NAME_SET: {
        ble_adp_name_set((uint8_t *)(p_conn_info->data.p_name), strlen(p_conn_info->data.p_name));
        sys_mfree(p_conn_info->data.p_name);
    } break;

    default:
      break;
    }
}

/*
 * Advertising Application
 ****************************************************************************************
 */
static void tkl_ble_app_adv_start(tkl_adv_actv_t *p_adv)
{
    ble_adv_data_set_t adv;
    ble_adv_data_set_t scan_rsp;
    ble_adv_data_set_t per_adv;
    ble_data_t adv_data;
    ble_data_t rsp_data;
    ble_data_t per_adv_data;

    adv.data_force = true;
    adv_data.len = p_adv->p_adv->length;
    adv_data.p_data = p_adv->p_adv->data;
    adv.data.p_data_force = &adv_data;

    scan_rsp.data_force = true;
    rsp_data.len = p_adv->p_rsp->length;
    rsp_data.p_data = p_adv->p_rsp->data;
    scan_rsp.data.p_data_force = &rsp_data;

    per_adv.data_force = true;
    per_adv_data.len = 0;
    per_adv_data.p_data = NULL;
    per_adv.data.p_data_force = &per_adv_data;

    ble_adv_start(p_adv->idx, &adv, &scan_rsp, &per_adv);
}

static void tkl_ble_app_adv_mgr_evt_hdlr(ble_adv_evt_t adv_evt, void *p_data, void *p_context)
{
    tkl_adv_actv_t *p_adv = (tkl_adv_actv_t *)p_context;
    TKL_BLE_GAP_PARAMS_EVT_T event;
    event.type = TKL_BLE_GAP_EVT_ADV_STATE;

    switch (adv_evt) {
    case BLE_ADV_EVT_STATE_CHG: {
        ble_adv_state_chg_t *p_chg = (ble_adv_state_chg_t *)p_data;
        ble_adv_state_t old_state = p_adv->state;

        dbg_print(NOTICE, "adv state change 0x%x ==> 0x%x, reason 0x%x\r\n", old_state, p_chg->state, p_chg->reason);

        p_adv->state = p_chg->state;

        if ((p_chg->state == BLE_ADV_STATE_CREATE) && (old_state == BLE_ADV_STATE_CREATING)) {
            p_adv->idx = p_chg->adv_idx;
            dbg_print(NOTICE, "adv index %d\r\n", p_adv->idx);

            tkl_ble_app_adv_start(p_adv);
        } else if ((p_chg->state == BLE_ADV_STATE_CREATE) && (old_state == BLE_ADV_STATE_START)) {

            ble_adv_remove(p_adv->idx);
        } else if (p_chg->state == BLE_ADV_STATE_IDLE) {
            p_adv->idx = TKL_ADV_INVALID_IDX;
            p_adv->state = BLE_ADV_STATE_IDLE;
            event.result = TKL_BLE_GAP_ADV_STATE_IDLE;

            if (gap_evt_cb) {
               gap_evt_cb(&event);
            }
        } else if (p_chg->state == BLE_ADV_STATE_START) {
            event.result = TKL_BLE_GAP_ADV_STATE_ADVERTISING;

            if (gap_evt_cb) {
                gap_evt_cb(&event);
            }
        }
    } break;

    case BLE_ADV_EVT_DATA_UPDATE_INFO: {
        ble_adv_data_update_info_t *p_info = (ble_adv_data_update_info_t *)p_data;

        dbg_print(NOTICE, "adv data update info, type %d, status 0x%x\r\n", p_info->type, p_info->status);
    } break;

    default:
        break;
    }
}

static void handle_adv_msg(uint8_t type, ble_adv_info_t *p_adv_info)
{
    TKL_BLE_GAP_PARAMS_EVT_T params_evt;
    params_evt.type = TKL_BLE_GAP_EVT_ADV_STATE;

    switch(type) {
    case ADV_SUBTYPE_STOP: {
        if (adp_env.adv_info.state == BLE_ADV_STATE_START) {
            if (ble_adv_stop(adp_env.adv_info.idx) == BLE_ERR_NO_ERROR) {
                params_evt.result = TKL_BLE_GAP_ADV_STATE_STOP;
            } else {
                // Stop fail fix todo
                params_evt.result = TKL_BLE_GAP_ADV_STATE_ADVERTISING;
            }

            if (gap_evt_cb) {
                gap_evt_cb(&params_evt);
            }
        }
    } break;

    case ADV_SUBTYPE_START: {
        ble_adv_param_t adv_param = {0};
        TKL_BLE_GAP_ADV_PARAMS_T *p_adv_params = &(p_adv_info->data.params);

        adv_param.param.own_addr_type = BLE_GAP_LOCAL_ADDR_STATIC;
        adv_param.param.type = BLE_GAP_ADV_TYPE_LEGACY;
        adv_param.param.filter_pol = BLE_GAP_ADV_ALLOW_SCAN_ANY_CON_ANY;
        adv_param.param.disc_mode = BLE_GAP_ADV_MODE_GEN_DISC;
        adv_param.param.ch_map = p_adv_params->adv_channel_map;
        adv_param.param.primary_phy = BLE_GAP_PHY_1MBPS;
        adv_param.param.adv_sid = 0;
        adv_param.param.max_skip = 0x00;
        adv_param.param.secondary_phy = BLE_GAP_PHY_1MBPS;
        adv_param.param.peer_addr.addr_type = p_adv_params->direct_addr.type;
        sys_memcpy(adv_param.param.peer_addr.addr, p_adv_params->direct_addr.addr, 6);
        adv_param.param.adv_intv_min = (p_adv_params->adv_interval_min / 5) * 8;
        adv_param.param.adv_intv_max = (p_adv_params->adv_interval_max / 5) * 8;

        switch (p_adv_params->adv_type) {
        case TKL_BLE_GAP_ADV_TYPE_CONN_SCANNABLE_UNDIRECTED: {
            adv_param.param.prop = BLE_GAP_ADV_PROP_UNDIR_CONN;
        } break;

        case TKL_BLE_GAP_ADV_TYPE_CONN_NONSCANNABLE_DIR_HIGHDUTY_CYCLE: {
            adv_param.param.prop = BLE_GAP_ADV_PROP_HIGH_DUTY_CONN_DIRECT;
        } break;

        case TKL_BLE_GAP_ADV_TYPE_CONN_NONSCANNABLE_DIRECTED: {
            adv_param.param.prop = BLE_GAP_ADV_PROP_CONN_DIRECT;
        } break;

        case TKL_BLE_GAP_ADV_TYPE_NONCONN_SCANNABLE_UNDIRECTED: {
            adv_param.param.prop = BLE_GAP_ADV_PROP_NON_CONN_SCAN;
        } break;

        case TKL_BLE_GAP_ADV_TYPE_NONCONN_NONSCANNABLE_UNDIRECTED: {
            adv_param.param.prop = BLE_GAP_ADV_PROP_NON_CONN_NON_SCAN;
        } break;

        case TKL_BLE_GAP_ADV_TYPE_EXTENDED_CONN_NONSCANNABLE_UNDIRECTED: {
            adv_param.param.type = BLE_GAP_ADV_TYPE_EXTENDED;
            adv_param.param.prop = BLE_GAP_EXT_ADV_PROP_CONN_UNDIRECT;
        } break;

        case TKL_BLE_GAP_ADV_TYPE_EXTENDED_NONCONN_SCANNABLE_UNDIRECTED: {
            adv_param.param.type = BLE_GAP_ADV_TYPE_EXTENDED;
            adv_param.param.prop = BLE_GAP_EXT_ADV_PROP_NON_CONN_SCAN;
        } break;

        case TKL_BLE_GAP_ADV_TYPE_EXTENDED_NONCONN_SCANNABLE_DIRECTED: {
            adv_param.param.type = BLE_GAP_ADV_TYPE_EXTENDED;
            adv_param.param.prop = BLE_GAP_EXT_ADV_PROP_NON_CONN_SCAN_DIRECT;
        } break;

        case TKL_BLE_GAP_ADV_TYPE_EXTENDED_NONCONN_NONSCANNABLE_UNDIRECTED: {
            adv_param.param.type = BLE_GAP_ADV_TYPE_EXTENDED;
            adv_param.param.prop = BLE_GAP_EXT_ADV_PROP_NON_CONN_NON_SCAN;
        } break;

        case TKL_BLE_GAP_ADV_TYPE_EXTENDED_NONCONN_NONSCANNABLE_DIRECTED: {
            return;
        } break;
        }

        adp_env.adv_info.type = adv_param.param.type;
        adp_env.adv_info.disc_mode = adv_param.param.disc_mode;
        adp_env.adv_info.peer_addr = adv_param.param.peer_addr;

        if (ble_adv_create(&adv_param, tkl_ble_app_adv_mgr_evt_hdlr, &adp_env.adv_info) != BLE_ERR_NO_ERROR) {
            params_evt.result = TKL_BLE_GAP_ADV_STATE_IDLE;
        } else {
            params_evt.result = TKL_BLE_GAP_ADV_STATE_START;
        }

        if (gap_evt_cb) {
            gap_evt_cb(&params_evt);
        }
    } break;

    case ADV_SUBTYPE_DATA_SET: {
        if (p_adv_info->data.adv_data.p_adv_data != NULL) {
            if (adp_env.adv_info.p_adv != NULL) {
                sys_mfree(adp_env.adv_info.p_adv);
            }
            adp_env.adv_info.p_adv = p_adv_info->data.adv_data.p_adv_data;
        }

        if (p_adv_info->data.adv_data.p_rsp_data != NULL) {
            if (adp_env.adv_info.p_rsp != NULL) {
                sys_mfree(adp_env.adv_info.p_rsp);
            }
            adp_env.adv_info.p_rsp = p_adv_info->data.adv_data.p_rsp_data;
        }

    } break;

    case ADV_SUBTYPE_DATA_UPD: {
        if (adp_env.adv_info.state == BLE_ADV_STATE_START) {
            ble_adv_data_set_t adv;
            ble_adv_data_set_t *p_adv = NULL;
            ble_adv_data_set_t scan_rsp;
            ble_adv_data_set_t *p_scan_rsp = NULL;
            ble_data_t adv_data;
            ble_data_t rsp_data;
            if (p_adv_info->data.adv_data.p_adv_data != NULL) {
                if (adp_env.adv_info.p_adv != NULL) {

                    sys_mfree(adp_env.adv_info.p_adv);
                }
                adp_env.adv_info.p_adv = p_adv_info->data.adv_data.p_adv_data;
                adv.data_force = true;
                adv_data.len = adp_env.adv_info.p_adv->length;
                adv_data.p_data = adp_env.adv_info.p_adv->data;
                adv.data.p_data_force = &adv_data;
                p_adv = &adv;
            }

            if (p_adv_info->data.adv_data.p_rsp_data != NULL) {
                if (adp_env.adv_info.p_rsp != NULL) {
                    sys_mfree(adp_env.adv_info.p_rsp);
                }
                adp_env.adv_info.p_rsp = p_adv_info->data.adv_data.p_rsp_data;
                scan_rsp.data_force = true;
                rsp_data.len = adp_env.adv_info.p_rsp->length;
                rsp_data.p_data = adp_env.adv_info.p_rsp->data;

                scan_rsp.data.p_data_force = &rsp_data;
                p_scan_rsp = &scan_rsp;
            }

            ble_adv_data_update(adp_env.adv_info.idx, p_adv, p_scan_rsp, NULL);
        } else {
            if (p_adv_info->data.adv_data.p_adv_data != NULL) {
                sys_mfree(p_adv_info->data.adv_data.p_adv_data);
            }

            if (p_adv_info->data.adv_data.p_rsp_data != NULL) {
                sys_mfree(p_adv_info->data.adv_data.p_rsp_data);
            }
        }
    } break;

    default:
      break;
    }
}

/*
 * Gatts Application
 ****************************************************************************************
 */
static uint16_t calc_char_handle_num(TKL_BLE_SERVICE_PARAMS_T *p_service)
{
    uint8_t i;
    uint16_t total_num = 0;
    TKL_BLE_CHAR_PARAMS_T *p_cur_char = NULL;

    for (i = 0; i < p_service->char_num; i++) {
        p_cur_char = &(p_service->p_char[i]);

        total_num += 2;   // charc decl and charc value

        if (p_cur_char->property & (TKL_BLE_GATT_CHAR_PROP_NOTIFY | TKL_BLE_GATT_CHAR_PROP_INDICATE)) {
            // need cccd
            total_num++;
        }

        if (p_cur_char->property & TKL_BLE_GATT_CHAR_PROP_EXT_PROP) {
            // need cccd
            total_num++;
        }
    }

    return total_num;
}

static tkl_read_info_t* pop_front_read_op(uint8_t conn_idx)
{
    dlist_t *pos, *n;
    tkl_read_info_t *p_read_info = NULL;
    bool found = false;

    if (list_empty(&adp_env.read_list)) {
        return NULL;
    }

    list_for_each_safe(pos, n, &adp_env.read_list) {
        p_read_info = list_entry(pos, tkl_read_info_t, list);
        if (conn_idx == p_read_info->conn_idx) {
            found = true;
            break;
        }
    }

    if (found) {
        list_del(&p_read_info->list);
        return p_read_info;
    }

    return NULL;
}

static bool is_cccd(uint8_t svc_id, uint16_t cccd_handle)
{
    dlist_t *pos, *n;
    uint16_t i;
    ble_cccd_info_t *p_info = NULL;
    if (!list_empty(&adp_env.srv_cccd_list)) {
        list_for_each_safe(pos, n, &adp_env.srv_cccd_list) {
            p_info = list_entry(pos, ble_cccd_info_t, list);
            if (p_info->srv_id == svc_id) {
                for(i = 0; i < p_info->cccd_num; i++) {
                    if ((p_info->cccd_handle_tuple[i] & 0xFFFF) == cccd_handle) {
                        return true;
                    }
                }

                return false;
            }
        }
    }

    return false;
}

static uint16_t get_char_handle_by_cccd_handle(uint8_t svc_id, uint16_t cccd_handle)
{
    dlist_t *pos, *n;
    uint16_t i;
    ble_cccd_info_t *p_info = NULL;
    if (!list_empty(&adp_env.srv_cccd_list)) {
        list_for_each_safe(pos, n, &adp_env.srv_cccd_list) {
            p_info = list_entry(pos, ble_cccd_info_t, list);
            if (p_info->srv_id == svc_id) {
                for(i = 0; i < p_info->cccd_num; i++) {
                    if ((p_info->cccd_handle_tuple[i] & 0xFFFF) == cccd_handle) {
                        return (p_info->cccd_handle_tuple[i] >> 16) & 0xFFFF;
                    }
                }
                return 0;
            }
        }
    }

    return 0;
}


static ble_status_t ble_tuya_rw_cb(ble_gatts_msg_info_t *p_cb_data)
{
    uint8_t status = BLE_ERR_NO_ERROR;
    TKL_BLE_GATT_PARAMS_EVT_T event;
    uint16_t srv_handle;
    tkl_conn_dev_node_t *p_dev = NULL;

    if (p_cb_data->srv_msg_type == BLE_SRV_EVT_GATT_OPERATION) {
        p_dev = tkl_ble_find_dev_by_conidx(p_cb_data->msg_data.gatts_op_info.conn_idx);
        if (p_dev == NULL) {
            return BLE_ATT_ERR_INVALID_HANDLE;
        }

        if (p_cb_data->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_READ_REQ) {
            ble_gatts_read_req_t *p_read_req = &(p_cb_data->msg_data.gatts_op_info.gatts_op_data.read_req);

            tkl_read_info_t *p_read_info = sys_malloc(sizeof(tkl_read_info_t));
            if (p_read_info == NULL) {
                status = BLE_ATT_ERR_INSUFF_RESOURCE;
            } else {
                p_read_req->pending_cfm = true;

                event.type = TKL_BLE_GATT_EVT_READ_CHAR_VALUE;
                event.conn_handle = p_dev->conn_handle;
                ble_gatts_get_start_hdl(p_read_req->svc_id, &srv_handle);
                event.gatt_event.char_read.char_handle = srv_handle + p_read_req->att_idx;
                event.gatt_event.char_read.offset = p_read_req->offset;

                p_read_info->conn_idx = p_cb_data->msg_data.gatts_op_info.conn_idx;
                p_read_info->char_handle = event.gatt_event.char_read.char_handle;
                p_read_info->token = p_read_req->token;
                p_read_info->max_len = p_read_req->max_len;
                p_read_info->offset = p_read_req->offset;
                INIT_DLIST_HEAD(&p_read_info->list);
                list_add_tail(&p_read_info->list, &adp_env.read_list);

                if (gatt_evt_cb) {
                    gatt_evt_cb(&event);
                }
            }
        } else if (p_cb_data->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_WRITE_REQ) {
            ble_gatts_write_req_t *p_write_req = &(p_cb_data->msg_data.gatts_op_info.gatts_op_data.write_req);
            ble_gatts_get_start_hdl(p_write_req->svc_id, &srv_handle);
            event.conn_handle = p_dev->conn_handle;

            if (is_cccd(p_write_req->svc_id, srv_handle + p_write_req->att_idx)) {
                event.type = TKL_BLE_GATT_EVT_SUBSCRIBE;
                event.gatt_event.subscribe.char_handle = get_char_handle_by_cccd_handle(p_write_req->svc_id, srv_handle + p_write_req->att_idx);
                // FIX TODO we now not record cccd value
                event.gatt_event.subscribe.prev_notify = 0;
                event.gatt_event.subscribe.cur_notify = (*p_write_req->p_val) & 0x01;
                event.gatt_event.subscribe.prev_indicate = 0;
                event.gatt_event.subscribe.cur_indicate = ((*p_write_req->p_val) & 0x02) >> 1;
            } else {
                event.type = TKL_BLE_GATT_EVT_WRITE_REQ;
                event.gatt_event.write_report.char_handle = srv_handle + p_write_req->att_idx;
                event.gatt_event.write_report.report.length = p_write_req->val_len;
                event.gatt_event.write_report.report.p_data = p_write_req->p_val;
            }

            if (gatt_evt_cb) {
                gatt_evt_cb(&event);
            }
        } else if (p_cb_data->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_NTF_IND_SEND_RSP) {
            ble_gatts_ntf_ind_send_rsp_t *p_ntf_ind = &(p_cb_data->msg_data.gatts_op_info.gatts_op_data.ntf_ind_send_rsp);

            event.type = TKL_BLE_GATT_EVT_NOTIFY_TX;
            event.conn_handle = p_dev->conn_handle;
            ble_gatts_get_start_hdl(p_ntf_ind->svc_id, &srv_handle);

            event.gatt_event.notify_result.char_handle = srv_handle + p_ntf_ind->att_idx;
            event.gatt_event.notify_result.result = p_ntf_ind->status;
            if (gatt_evt_cb) {
                gatt_evt_cb(&event);
            }
        }
    } else if (p_cb_data->srv_msg_type == BLE_SRV_EVT_CONN_STATE_CHANGE_IND) {
        if (p_cb_data->msg_data.conn_state_change_ind.conn_state == BLE_CONN_STATE_DISCONNECTD) {
        }
    } else if (p_cb_data->srv_msg_type == BLE_SRV_EVT_SVC_ADD_RSP) {
        if (p_cb_data->msg_data.svc_add_rsp.status != BLE_ERR_NO_ERROR) {
            dlist_t *pos, *n;
            ble_cccd_info_t *p_info = NULL;
            if (!list_empty(&adp_env.srv_cccd_list)) {
                list_for_each_safe(pos, n, &adp_env.srv_cccd_list) {
                    p_info = list_entry(pos, ble_cccd_info_t, list);
                    if (p_info->srv_id == p_cb_data->msg_data.svc_add_rsp.svc_id) {
                        list_del(&p_info->list);
                        sys_mfree(p_info);
                        break;
                    }
                }
            }
        }
    }

    return status;
}

static void handle_gatts_msg(uint8_t type, ble_gatts_info_t *p_gatts_info)
{
    tkl_conn_dev_node_t *p_dev = NULL;

    if (type != GATTS_SUBTYPE_ADD_SRVS) {
        p_dev = tkl_ble_find_dev_by_conn_handle(p_gatts_info->data.val_info.conn_handle);
        if (p_dev == NULL) {
            return;
        }
    }

    switch(type) {
    case GATTS_SUBTYPE_ADD_SRVS: {
        uint8_t srv_id;
        uint8_t info;

        if (p_gatts_info->data.add_srvs.uuid_len == BLE_GATT_UUID_16_LEN) {
            info = SVC_UUID(16);
        } else if (p_gatts_info->data.add_srvs.uuid_len == BLE_GATT_UUID_32_LEN) {
            info = SVC_UUID(32);
        } else {
            info = SVC_UUID(128);
        }

        if (ble_gatts_svc_add(&srv_id, p_gatts_info->data.add_srvs.uuid128, p_gatts_info->data.add_srvs.start_handle, info, p_gatts_info->data.add_srvs.p_srv_table,
                                p_gatts_info->data.add_srvs.total_handles, ble_tuya_rw_cb) == BLE_ERR_NO_ERROR) {

            uint16_t i;
            ble_cccd_info_t *p_info = sys_malloc(sizeof(ble_cccd_info_t) + p_gatts_info->data.add_srvs.cccd_num * sizeof(uint32_t));
            uint16_t char_handle = 0;

            if (p_info) {
                p_info->cccd_num = 0;
                p_info->srv_id = srv_id;
                p_info->svc_handle = p_gatts_info->data.add_srvs.start_handle;
                INIT_DLIST_HEAD(&p_info->list);

                for(i = 0; i < p_gatts_info->data.add_srvs.total_handles; i++) {
                    if (*((uint16_t *)(p_gatts_info->data.add_srvs.p_srv_table[i].uuid)) == BLE_GATT_DECL_CHARACTERISTIC) {
                        char_handle = p_info->svc_handle + i + 1;
                    } else if (*((uint16_t *)(p_gatts_info->data.add_srvs.p_srv_table[i].uuid)) == BLE_GATT_DESC_CLIENT_CHAR_CFG) {
                        p_info->cccd_handle_tuple[p_info->cccd_num] = (p_info->svc_handle + i) | (char_handle << 16);
                        p_info->cccd_num++;
                    }
                }
                list_add_tail(&adp_env.srv_cccd_list, &p_info->list);
            } else {
                dbg_print(ERR, "alloc cccd array fail \r\n");
            }
        }

        sys_mfree(p_gatts_info->data.add_srvs.p_srv_table);
    } break;

    case GATTS_SUBTYPE_SET_VAL: {
        tkl_read_info_t* p_read_info;
        uint16_t len;

        p_read_info = pop_front_read_op(p_dev->conidx);

        if (p_read_info != NULL) {
            len = ble_min(p_read_info->max_len, p_gatts_info->data.val_info.length);
            if (ble_gatts_svc_attr_read_cfm(p_read_info->conn_idx, p_read_info->token,
                                            BLE_ERR_NO_ERROR, p_gatts_info->data.val_info.length + p_read_info->offset,
                                            len, p_gatts_info->data.val_info.p_data) != BLE_ERR_NO_ERROR) {
                dbg_print(ERR, "read confirm fail \r\n");
            }

            sys_mfree(p_read_info);
        }
        sys_mfree(p_gatts_info->data.val_info.p_data);
    } break;

    case GATTS_SUBTYPE_VAL_NOTIFY: {
        if (ble_gatts_ntf_ind_send_by_handle(p_dev->conidx, p_gatts_info->data.val_info.char_handle, p_gatts_info->data.val_info.p_data,
                                         p_gatts_info->data.val_info.length, BLE_GATT_NOTIFY) != BLE_ERR_NO_ERROR) {
            dbg_print(ERR, "notify fail \r\n");
        }

        sys_mfree(p_gatts_info->data.val_info.p_data);
    } break;

    case GATTS_SUBTYPE_VAL_IND: {
        ble_gatts_ntf_ind_send_by_handle(p_dev->conidx, p_gatts_info->data.val_info.char_handle, p_gatts_info->data.val_info.p_data,
                                         p_gatts_info->data.val_info.length, BLE_GATT_INDICATE);

        sys_mfree(p_gatts_info->data.val_info.p_data);
    } break;

    default:
      break;
    }
}

/*
 * Gattc Application
 ****************************************************************************************
 */
static ble_status_t tkl_ble_app_gattc_cb(ble_gattc_co_msg_info_t *p_msg_info)
{
    TKL_BLE_GATT_PARAMS_EVT_T event;
    tkl_conn_dev_node_t *p_dev = tkl_ble_find_dev_by_conidx(p_msg_info->conn_idx);

    if (p_dev == NULL) {
        dbg_print(ERR, "tkl_ble_app_gattc_cb can't find conn_idx 0x%0x device\r\n", p_msg_info->conn_idx);
        return BLE_ERR_NO_ERROR;
    }

    sys_memset(&event, 0, sizeof(TKL_BLE_GATT_PARAMS_EVT_T));

    switch (p_msg_info->cli_cb_msg_type) {
    case BLE_CLI_CO_EVT_DISC_SVC_INFO_IND:
        dbg_print(NOTICE, "BLE_CLI_CO_EVT_DISC_SVC_INFO_IND start_hdl = 0x%x, end_hdl = 0x%x\r\n", p_msg_info->msg_data.disc_svc_ind.start_hdl, p_msg_info->msg_data.disc_svc_ind.end_hdl);

        if (gatt_evt_cb) {
            event.type = TKL_BLE_GATT_EVT_PRIM_SEV_DISCOVERY;
            event.conn_handle = p_dev->conn_handle;
            event.result = BLE_ERR_NO_ERROR;
            event.gatt_event.svc_disc.svc_num = 1;
            event.gatt_event.svc_disc.services[0].start_handle = p_msg_info->msg_data.disc_svc_ind.start_hdl;
            event.gatt_event.svc_disc.services[0].end_handle = p_msg_info->msg_data.disc_svc_ind.end_hdl;
            event.gatt_event.svc_disc.services[0].uuid.uuid_type = p_msg_info->msg_data.disc_svc_ind.ble_uuid.type;
            sys_memcpy(event.gatt_event.svc_disc.services[0].uuid.uuid.uuid128, p_msg_info->msg_data.disc_svc_ind.ble_uuid.data.uuid_128, 16);

            gatt_evt_cb(&event);
        }

        break;

    case BLE_CLI_CO_EVT_DISC_SVC_RSP:
        if (p_msg_info->msg_data.disc_svc_rsp.status != BLE_ERR_NO_ERROR) {
            dbg_print(NOTICE, "BLE_CLI_CO_EVT_DISC_SVC_RSP fail status 0x%x\r\n", p_msg_info->msg_data.disc_svc_rsp.status);
        }

        break;

    case BLE_CLI_CO_EVT_DISC_CHAR_INFO_IND:
        dbg_print(NOTICE, "BLE_CLI_CO_EVT_DISC_CHAR_INFO_IND char_hdl = 0x%x\r\n", p_msg_info->msg_data.disc_char_ind.char_hdl);
        if (gatt_evt_cb) {
            event.type = TKL_BLE_GATT_EVT_CHAR_DISCOVERY;
            event.conn_handle = p_dev->conn_handle;
            event.result = BLE_ERR_NO_ERROR;
            event.gatt_event.char_disc.char_num = 1;
            event.gatt_event.char_disc.characteristics[0].handle = p_msg_info->msg_data.disc_char_ind.char_hdl;
            event.gatt_event.char_disc.characteristics[0].uuid.uuid_type = p_msg_info->msg_data.disc_char_ind.ble_uuid.type;
            sys_memcpy(event.gatt_event.char_disc.characteristics[0].uuid.uuid.uuid128, p_msg_info->msg_data.disc_char_ind.ble_uuid.data.uuid_128, 16);

            gatt_evt_cb(&event);
        }
        break;

     case BLE_CLI_CO_EVT_DISC_CHAR_RSP:
        if (p_msg_info->msg_data.disc_char_rsp.status != BLE_ERR_NO_ERROR) {
            dbg_print(NOTICE, "BLE_CLI_CO_EVT_DISC_CHAR_RSP fail status 0x%x\r\n", p_msg_info->msg_data.disc_char_rsp.status);
        }
        break;

     case BLE_CLI_CO_EVT_DISC_DESC_INFO_IND:
        dbg_print(NOTICE, "BLE_CLI_CO_EVT_DISC_DESC_INFO_IND desc_hdl = 0x%x\r\n", p_msg_info->msg_data.disc_desc_ind.desc_hdl);

        if (p_msg_info->msg_data.disc_desc_ind.ble_uuid.data.uuid_16 == BLE_GATT_DESC_CLIENT_CHAR_CFG) {
            if (gatt_evt_cb) {
                event.type = TKL_BLE_GATT_EVT_CHAR_DESC_DISCOVERY;
                event.conn_handle = p_dev->conn_handle;
                event.result = BLE_ERR_NO_ERROR;
                event.gatt_event.desc_disc.cccd_handle = p_msg_info->msg_data.disc_desc_ind.desc_hdl;

                gatt_evt_cb(&event);
            }
        }
        break;

     case BLE_CLI_CO_EVT_DISC_DESC_RSP:
        if (p_msg_info->msg_data.disc_desc_rsp.status != BLE_ERR_NO_ERROR) {
            dbg_print(NOTICE, "BLE_CLI_CO_EVT_DISC_DESC_RSP fail status 0x%x\r\n", p_msg_info->msg_data.disc_desc_rsp.status);
        }
        break;

     case BLE_CLI_CO_EVT_READ_RSP:
        if (gatt_evt_cb) {
            event.conn_handle = p_dev->conn_handle;
            event.type = TKL_BLE_GATT_EVT_READ_RX;
            event.result = p_msg_info->msg_data.read_rsp.status;
            if (p_msg_info->msg_data.read_rsp.status == BLE_ERR_NO_ERROR) {
                event.gatt_event.data_read.char_handle = p_msg_info->msg_data.read_rsp.handle;
                event.gatt_event.data_read.report.length = p_msg_info->msg_data.read_rsp.length;
                event.gatt_event.data_read.report.p_data = p_msg_info->msg_data.read_rsp.p_value;
            }

            gatt_evt_cb(&event);
        }
        break;

     case BLE_CLI_CO_EVT_NTF_IND:
        if (gatt_evt_cb) {
            event.conn_handle = p_dev->conn_handle;
            event.type = TKL_BLE_GATT_EVT_NOTIFY_INDICATE_RX;
            event.gatt_event.data_report.char_handle = p_msg_info->msg_data.ntf_ind.handle;
            event.gatt_event.data_report.report.length = p_msg_info->msg_data.ntf_ind.length;
            event.gatt_event.data_report.report.p_data = p_msg_info->msg_data.ntf_ind.p_value;

            gatt_evt_cb(&event);
        }
        break;

     default:
         break;
     }

    return BLE_ERR_NO_ERROR;
}

static void handle_gattc_msg(uint8_t type, ble_gattc_info_t *p_gattc_info)
{
    tkl_conn_dev_node_t *p_dev = tkl_ble_find_dev_by_conn_handle(p_gattc_info->conn_handle);

    if (p_dev == NULL) {
        dbg_print(ERR, "handle_gattc_msg can't find conn_handle 0x%04x device\r\n", p_gattc_info->conn_handle);
        return;
    }

    //TKL_BLE_GATT_PARAMS_EVT_T event;
    dbg_print(NOTICE, "handle_gattc_msg type %d\r\n", type);
    switch(type) {
    case GATTC_SUBTYPE_DISC_SRV: {
        if(ble_gattc_co_disc_svc(p_dev->conidx, p_gattc_info->data.disc_info.start_handle, p_gattc_info->data.disc_info.end_handle) != BLE_ERR_NO_ERROR) {
            dbg_print(ERR, "handle_gattc_msg discover service fail\r\n");
        }
    } break;

    case GATTC_SUBTYPE_DISC_CHAR: {
        if(ble_gattc_co_disc_char(p_dev->conidx, p_gattc_info->data.disc_info.start_handle, p_gattc_info->data.disc_info.end_handle) != BLE_ERR_NO_ERROR) {
            dbg_print(ERR, "handle_gattc_msg discover char fail\r\n");
        }
    } break;

    case GATTC_SUBTYPE_DISC_DESC: {
        if(ble_gattc_co_disc_desc(p_dev->conidx, p_gattc_info->data.disc_info.start_handle, p_gattc_info->data.disc_info.end_handle) != BLE_ERR_NO_ERROR) {
            dbg_print(ERR, "handle_gattc_msg discover desc fail\r\n");
        }
    } break;

    case GATTC_SUBTYPE_WRITE_CMD: {
        if(ble_gattc_co_write_cmd(p_dev->conidx, p_gattc_info->data.write_info.char_handle,
                               p_gattc_info->data.write_info.val_len, p_gattc_info->data.write_info.p_value) != BLE_ERR_NO_ERROR) {
            dbg_print(ERR, "handle_gattc_msg write cmd fail\r\n");
        }
    } break;

    case GATTC_SUBTYPE_WRITE_REQ: {
        if(ble_gattc_co_write_req(p_dev->conidx, p_gattc_info->data.write_info.char_handle,
                               p_gattc_info->data.write_info.val_len, p_gattc_info->data.write_info.p_value) != BLE_ERR_NO_ERROR) {
            dbg_print(ERR, "handle_gattc_msg write request fail\r\n");
        }
    } break;

    case GATTC_SUBTYPE_READ_REQ: {
        if(ble_gattc_co_read(p_dev->conidx, p_gattc_info->data.read_char_handle, 0, 0) != BLE_ERR_NO_ERROR) {
            dbg_print(ERR, "handle_gattc_msg read request fail\r\n");
        }
    } break;

    default:
      break;
    }
}


static bool tkl_ble_app_msg_hdl(void *p_buf)
{
    uint8_t subtype;
    tkl_app_msg_t *p_msg = (tkl_app_msg_t *)p_buf;
    subtype = TKL_BLE_EVT_ID_SUBTYPE_GET(p_msg->id);

    switch (TKL_BLE_EVT_ID_TYPE_GET(p_msg->id)) {
    case TKL_BLE_APP_ADP: {
        handle_adp_msg(subtype);
    } break;

    case TKL_BLE_APP_SCAN: {
        handle_scan_msg(subtype, &p_msg->data.scan_info);
    } break;

    case TKL_BLE_APP_ADV: {
        handle_adv_msg(subtype,  &p_msg->data.adv_info);
    } break;

    case TKL_BLE_APP_CONN: {
        handle_conn_msg(subtype,  &p_msg->data.conn_info);
    } break;

    case TKL_BLE_APP_GATTS: {
        handle_gatts_msg(subtype,  &p_msg->data.gatts_info);
    } break;

    case TKL_BLE_APP_GATTC: {
        handle_gattc_msg(subtype,  &p_msg->data.gattc_info);
    } break;

    default:
      break;
    }

    return true;
}

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
OPERATE_RET tuya_hal_gap_callback_register(const TKL_BLE_GAP_EVT_FUNC_CB gap_evt)
{
    gap_evt_cb = gap_evt;
    return OPRT_OK;
}

OPERATE_RET tuya_hal_gatt_callback_register(const TKL_BLE_GATT_EVT_FUNC_CB gatt_evt)
{
    gatt_evt_cb = gatt_evt;
    return OPRT_OK;
}

// FIX TODO as tuya register gatt service before hal init, so we have to call tuya_adp_init
// in main function
OPERATE_RET tuya_hal_init(uint8_t role)
{
    tkl_app_msg_t adp_msg;

    adp_msg.id = TKL_BLE_EVT_ID_ADP(ADP_SUBTYPE_ENABLE);

    if (!ble_local_app_msg_send(&adp_msg, sizeof(tkl_app_msg_t))) {
        return OPRT_OS_ADAPTER_BLE_INIT_FAILED;
    }

    return OPRT_OK;
}

void tuya_adp_init(uint8_t role)
{
    ble_init_param_t param = {0};

    ble_os_api_t tuya_os_interface = {
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

    sys_sema_init(&tuya_ble_sema, 0);
    sys_memset(&adp_env, 0, sizeof(struct tkl_adp_env_tag));

    ble_power_on();

    if (role & TUYA_BLE_ROLE_SERVER) {
        param.role |= BLE_GAP_ROLE_PERIPHERAL;
    }

    if (role & TUYA_BLE_ROLE_CLIENT) {
        param.role |= BLE_GAP_ROLE_CENTRAL;
    }

    param.keys_user_mgr = false;
    param.pairing_mode = BLE_GAP_PAIRING_SECURE_CONNECTION | BLE_GAP_PAIRING_LEGACY;
    param.privacy_cfg = BLE_GAP_PRIV_CFG_PRIV_EN_BIT;

    param.ble_task_stack_size = TKL_BLE_STACK_TASK_STACK_SIZE;
    param.ble_task_priority = TKL_BLE_STACK_TASK_PRIORITY;

    param.ble_app_task_stack_size = TKL_BLE_APP_TASK_STACK_SIZE;
    param.ble_app_task_priority = TKL_BLE_APP_TASK_PRIORITY;

    param.name_perm = BLE_GAP_WRITE_NOT_ENC;
    param.appearance_perm = BLE_GAP_WRITE_NOT_ENC;

    param.en_cfg = 0;
    param.p_os_api = &tuya_os_interface;
    param.p_hci_uart_func = NULL;

    ble_sw_init(&param);

    ble_app_msg_hdl_reg(tkl_ble_app_msg_hdl);

    ble_conn_callback_register(tkl_ble_app_conn_evt_handler);
    ble_sec_callback_register(tkl_ble_app_sec_evt_handler);

    if (role & TUYA_BLE_ROLE_CLIENT) {
        ble_scan_callback_register(tkl_ble_app_scan_mgr_evt_handler);
        ble_gattc_co_cb_reg(tkl_ble_app_gattc_cb);
    }

    adp_env.role = role;

    INIT_DLIST_HEAD(&adp_env.read_list);
    INIT_DLIST_HEAD(&adp_env.srv_cccd_list);
    INIT_DLIST_HEAD(&adp_env.conn_dev_list);

    adp_env.adv_info.idx = TKL_ADV_INVALID_IDX;
    adp_env.adv_info.type = TKL_BLE_ADV_TYPE_LEGACY;
    adp_env.adv_info.prop = 0x0000;
    adp_env.adv_info.pri_phy = BLE_GAP_PHY_1MBPS;
    adp_env.adv_info.sec_phy = BLE_GAP_PHY_1MBPS;
    adp_env.adv_info.wl_enable = false;
    adp_env.adv_info.own_addr_type = BLE_GAP_LOCAL_ADDR_STATIC;
    adp_env.srv_add_handle = 0x100;
    adp_env.adapter_state = TUYA_ADP_INITIALING;

    ble_adp_callback_register(tkl_ble_adp_evt_handler);
    /* The BLE interrupt must be enabled after ble_sw_init. */
    ble_irq_enable();

    return;
}

OPERATE_RET tuya_hal_scan_start(TKL_BLE_GAP_SCAN_PARAMS_T const *p_scan_params)
{
    tkl_app_msg_t scan_msg;

    if (p_scan_params->extended != 0 || p_scan_params->scan_channel_map != 0x07) {
        return OPRT_INVALID_PARM;
    }

    scan_msg.id = TKL_BLE_EVT_ID_SCAN(SCAN_SUBTYPE_ENABLE);
    scan_msg.data.scan_info.params = *p_scan_params;

    if (!ble_local_app_msg_send(&scan_msg, sizeof(tkl_app_msg_t))) {
        return OPRT_OS_ADAPTER_BLE_SCAN_START_FAILED;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_hal_scan_stop()
{
    tkl_app_msg_t scan_msg;

    scan_msg.id = TKL_BLE_EVT_ID_SCAN(SCAN_SUBTYPE_DISABLE);

    if (!ble_local_app_msg_send(&scan_msg, sizeof(tkl_app_msg_t))) {
        return OPRT_OS_ADAPTER_BLE_SCAN_STAOP_FAILED;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_hal_adv_start(TKL_BLE_GAP_ADV_PARAMS_T const *p_adv_params)
{
    tkl_app_msg_t adv_msg;

    if (p_adv_params->adv_type == TKL_BLE_GAP_ADV_TYPE_EXTENDED_NONCONN_NONSCANNABLE_DIRECTED) {
        return OPRT_INVALID_PARM;
    }

    adv_msg.id = TKL_BLE_EVT_ID_ADV(ADV_SUBTYPE_START);
    adv_msg.data.adv_info.data.params = *p_adv_params;

    if (!ble_local_app_msg_send(&adv_msg, sizeof(tkl_app_msg_t))) {
        return OPRT_OS_ADAPTER_BLE_ADV_START_FAILED;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_hal_adv_stop()
{
    tkl_app_msg_t adv_msg;

    adv_msg.id = TKL_BLE_EVT_ID_ADV(ADV_SUBTYPE_STOP);

    if (!ble_local_app_msg_send(&adv_msg, sizeof(tkl_app_msg_t))) {
        return OPRT_OS_ADAPTER_BLE_ADV_STOP_FAILED;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_hal_adv_rsp_data_set(TKL_BLE_DATA_T const *p_adv, TKL_BLE_DATA_T const *p_scan_rsp)
{
    tkl_app_msg_t adv_msg;

    tkl_ble_adv_data_t * p_data;

    if (p_adv != NULL) {
        p_data = sys_malloc(sizeof(tkl_ble_adv_data_t) + p_adv->length);
        if (p_data == NULL) {
            return OPRT_MALLOC_FAILED;
        }

        adv_msg.data.adv_info.data.adv_data.p_adv_data = p_data;
        sys_memcpy(p_data->data, p_adv->p_data, p_adv->length);
        p_data->length = p_adv->length;
    } else {
        adv_msg.data.adv_info.data.adv_data.p_adv_data = NULL;
    }

    if (p_scan_rsp != NULL) {
        p_data = sys_malloc(sizeof(tkl_ble_adv_data_t) + p_scan_rsp->length);

        if (p_data == NULL) {
            sys_mfree(adv_msg.data.adv_info.data.adv_data.p_adv_data);
            return OPRT_MALLOC_FAILED;
        }

        adv_msg.data.adv_info.data.adv_data.p_rsp_data = p_data;
        sys_memcpy(p_data->data, p_scan_rsp->p_data, p_scan_rsp->length);
        p_data->length = p_scan_rsp->length;
    } else {
        adv_msg.data.adv_info.data.adv_data.p_rsp_data = NULL;
    }

    adv_msg.id = TKL_BLE_EVT_ID_ADV(ADV_SUBTYPE_DATA_SET);

    if (!ble_local_app_msg_send(&adv_msg, sizeof(tkl_app_msg_t))) {
        return OPRT_RESOURCE_NOT_READY;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_hal_adv_rsp_data_update(TKL_BLE_DATA_T const *p_adv, TKL_BLE_DATA_T const *p_scan_rsp)
{
    tkl_app_msg_t adv_msg;

    tkl_ble_adv_data_t * p_data;

    if (p_adv != NULL) {
        p_data = sys_malloc(sizeof(tkl_ble_adv_data_t) + p_adv->length);
        if (p_data == NULL) {
            return OPRT_MALLOC_FAILED;
        }

        adv_msg.data.adv_info.data.adv_data.p_adv_data = p_data;
        sys_memcpy(p_data->data, p_adv->p_data, p_adv->length);
        p_data->length = p_adv->length;
    } else {
        adv_msg.data.adv_info.data.adv_data.p_adv_data = NULL;
    }

    if (p_scan_rsp != NULL) {
        p_data = sys_malloc(sizeof(tkl_ble_adv_data_t) + p_scan_rsp->length);

        if (p_data == NULL) {
            sys_mfree(adv_msg.data.adv_info.data.adv_data.p_adv_data);
            return OPRT_MALLOC_FAILED;
        }

        adv_msg.data.adv_info.data.adv_data.p_rsp_data = p_data;
        sys_memcpy(p_data->data, p_scan_rsp->p_data, p_scan_rsp->length);
        p_data->length = p_scan_rsp->length;
    } else {
        adv_msg.data.adv_info.data.adv_data.p_rsp_data = NULL;
    }

    adv_msg.id = TKL_BLE_EVT_ID_ADV(ADV_SUBTYPE_DATA_UPD);

    if (!ble_local_app_msg_send(&adv_msg, sizeof(tkl_app_msg_t))) {
        return OPRT_RESOURCE_NOT_READY;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_hal_ble_connect(TKL_BLE_GAP_ADDR_T const *p_peer_addr, TKL_BLE_GAP_SCAN_PARAMS_T const *p_scan_params, TKL_BLE_GAP_CONN_PARAMS_T const *p_conn_params)
{
    tkl_app_msg_t conn_msg;

    conn_msg.data.conn_info.data.conn_params.peer_addr = *p_peer_addr;
    conn_msg.data.conn_info.data.conn_params.scan_params = *p_scan_params;
    conn_msg.data.conn_info.data.conn_params.conn_params = *p_conn_params;

    conn_msg.id = TKL_BLE_EVT_ID_CONN(CONN_SUBTYPE_CONN);

    if (!ble_local_app_msg_send(&conn_msg, sizeof(tkl_app_msg_t))) {
        return OPRT_OS_ADAPTER_BLE_GATT_CONN_FAILED;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_hal_ble_disconnect(uint16_t conn_handle, uint8_t hci_reason)
{
    tkl_app_msg_t disconn_msg;

    disconn_msg.data.conn_info.data.disconn_reason = hci_reason;
    disconn_msg.data.conn_info.conn_handle = conn_handle;

    disconn_msg.id = TKL_BLE_EVT_ID_CONN(CONN_SUBTYPE_DISCON);

    if (!ble_local_app_msg_send(&disconn_msg, sizeof(tkl_app_msg_t))) {
        return OPRT_OS_ADAPTER_BLE_GATT_DISCONN_FAILED;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_hal_ble_conn_param_update(uint16_t conn_handle, TKL_BLE_GAP_CONN_PARAMS_T const *p_conn_params)
{
    tkl_app_msg_t params_upd_msg;

    params_upd_msg.data.conn_info.data.conn_upd_params = *p_conn_params;
    params_upd_msg.data.conn_info.conn_handle = conn_handle;

    params_upd_msg.id = TKL_BLE_EVT_ID_CONN(CONN_SUBTYPE_CONN_PARAM_UPD);

    if (!ble_local_app_msg_send(&params_upd_msg, sizeof(tkl_app_msg_t))) {
        return OPRT_RESOURCE_NOT_READY;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_hal_ble_rssi_get(uint16_t conn_handle)
{
    tkl_app_msg_t rssi_msg;

    rssi_msg.data.conn_info.conn_handle = conn_handle;

    rssi_msg.id = TKL_BLE_EVT_ID_CONN(CONN_SUBTYPE_CONN_RSSI_GET);

    if (!ble_local_app_msg_send(&rssi_msg, sizeof(tkl_app_msg_t))) {
        return OPRT_RESOURCE_NOT_READY;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_hal_ble_name_set(char *p_name)
{
    tkl_app_msg_t name_msg;
    uint16_t len;

    if (p_name == NULL) {
        return OPRT_NOT_SUPPORTED;
    }

    len = strlen(p_name);

    if (len > 512) {
        return OPRT_NOT_SUPPORTED;
    }

    name_msg.data.conn_info.data.p_name = sys_malloc(len + 1);

    if (name_msg.data.conn_info.data.p_name == NULL) {
        return OPRT_MALLOC_FAILED;
    }

    name_msg.id = TKL_BLE_EVT_ID_CONN(CONN_SUBTYPE_NAME_SET);
    sys_memcpy(name_msg.data.conn_info.data.p_name, p_name, len + 1);

    if (!ble_local_app_msg_send(&name_msg, sizeof(tkl_app_msg_t))) {
        sys_mfree(name_msg.data.conn_info.data.p_name);
        return OPRT_RESOURCE_NOT_READY;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_hal_gatts_service_add(TKL_BLE_GATTS_PARAMS_T *p_service)
{
    tkl_app_msg_t srv_msg;
    uint8_t i, j;
    TKL_BLE_SERVICE_PARAMS_T *p_cur_service;
    TKL_BLE_CHAR_PARAMS_T *p_cur_char;
    uint16_t total_handle = 0;
    ble_gatt_attr_desc_t *p_srv_table = NULL;

    for (i = 0; i < p_service->svc_num; i++) {
        p_cur_service = &(p_service->p_service[i]);
        total_handle = 1;

        total_handle += calc_char_handle_num(p_cur_service);
        p_srv_table = sys_malloc(sizeof(ble_gatt_attr_desc_t) * total_handle);

        if (p_srv_table == NULL) {
            // Just continue
            continue;
        }

        sys_memset(p_srv_table, 0, sizeof(ble_gatt_attr_desc_t) * total_handle);
        if (p_cur_service->svc_uuid.uuid_type == TKL_BLE_UUID_TYPE_32) {
            srv_msg.data.gatts_info.data.add_srvs.uuid_len = BLE_GATT_UUID_32_LEN;
        } else if (p_cur_service->svc_uuid.uuid_type == TKL_BLE_UUID_TYPE_128) {
            srv_msg.data.gatts_info.data.add_srvs.uuid_len = BLE_GATT_UUID_128_LEN;
        } else {
            srv_msg.data.gatts_info.data.add_srvs.uuid_len = BLE_GATT_UUID_16_LEN;
        }

        sys_memcpy(srv_msg.data.gatts_info.data.add_srvs.uuid128, p_cur_service->svc_uuid.uuid.uuid128, BLE_GATT_UUID_128_LEN);
        srv_msg.id = TKL_BLE_EVT_ID_GATTS(GATTS_SUBTYPE_ADD_SRVS);
        srv_msg.data.gatts_info.data.add_srvs.p_srv_table = p_srv_table;
        srv_msg.data.gatts_info.data.add_srvs.total_handles = total_handle;
        srv_msg.data.gatts_info.data.add_srvs.cccd_num = 0;
        srv_msg.data.gatts_info.data.add_srvs.start_handle = adp_env.srv_add_handle;
        p_cur_service->handle = adp_env.srv_add_handle;
        adp_env.srv_add_handle += total_handle;

        total_handle = 0;
        if (p_cur_service->type == TKL_BLE_UUID_SERVICE_PRIMARY) {
            *((uint16_t *)(p_srv_table[0].uuid)) = BLE_GATT_DECL_PRIMARY_SERVICE;
        } else if (p_cur_service->type == TKL_BLE_UUID_SERVICE_SECONDARY) {
            *((uint16_t *)(p_srv_table[0].uuid)) = BLE_GATT_DECL_SECONDARY_SERVICE;
        } else {
            sys_mfree(p_srv_table);
            dbg_print(ERR, "tuya_hal_gatts_service_add wrong service type\r\n");
            continue;
        }
        p_srv_table[total_handle].info = PROP(RD);
        p_srv_table[total_handle].ext_info = 0;
        total_handle++;

        for (j = 0; j < p_cur_service->char_num; j++) {
            p_cur_char = &(p_cur_service->p_char[j]);

            // character decl
            *((uint16_t *)(p_srv_table[total_handle].uuid)) = BLE_GATT_DECL_CHARACTERISTIC;
            p_srv_table[total_handle].info = PROP(RD);
            p_srv_table[total_handle].ext_info = 0;
            total_handle++;

            p_cur_char->handle = p_cur_service->handle + total_handle;
            // character value
            if (p_cur_char->char_uuid.uuid_type == TKL_BLE_UUID_TYPE_32) {
                p_srv_table[total_handle].info |= ATT_UUID(32);
            } else if (p_cur_char->char_uuid.uuid_type == TKL_BLE_UUID_TYPE_128) {
                p_srv_table[total_handle].info |= ATT_UUID(128);
            }

            sys_memcpy(p_srv_table[total_handle].uuid, p_cur_char->char_uuid.uuid.uuid128, BLE_GATT_UUID_128_LEN);
            p_srv_table[total_handle].ext_info = p_cur_char->value_len;

            if (p_cur_char->property & TKL_BLE_GATT_CHAR_PROP_BROADCAST) {
                p_srv_table[total_handle].info |= PROP(BC);
            }

            if (p_cur_char->property & TKL_BLE_GATT_CHAR_PROP_READ) {
                p_srv_table[total_handle].info |= PROP(RD);
            }

            if (p_cur_char->property & TKL_BLE_GATT_CHAR_PROP_WRITE_NO_RSP) {
                p_srv_table[total_handle].info |= PROP(WC);
            }

            if (p_cur_char->property & TKL_BLE_GATT_CHAR_PROP_WRITE) {
                p_srv_table[total_handle].ext_info = 1024; // workaround for ios MTU size
                p_srv_table[total_handle].info |= PROP(WR);
            }

            if (p_cur_char->property & TKL_BLE_GATT_CHAR_PROP_NOTIFY) {
                p_srv_table[total_handle].info |= PROP(NTF);
            }

            if (p_cur_char->property & TKL_BLE_GATT_CHAR_PROP_INDICATE) {
                p_srv_table[total_handle].info |= PROP(IND);
            }

            if (p_cur_char->property & TKL_BLE_GATT_CHAR_PROP_WRITE_AUTHEN_SIGNED) {
                p_srv_table[total_handle].info |= PROP(WS);
            }

            if (p_cur_char->permission & TKL_BLE_GATT_PERM_READ_AUTHEN) {
                p_srv_table[total_handle].info |= SEC_LVL(RP, AUTH);
            } else if (p_cur_char->permission & TKL_BLE_GATT_PERM_READ_ENCRYPT) {
                p_srv_table[total_handle].info |= SEC_LVL(RP, UNAUTH);
            }

            if (p_cur_char->permission & TKL_BLE_GATT_PERM_WRITE_AUTHEN) {
                p_srv_table[total_handle].info |= SEC_LVL(WP, AUTH);
            } else if (p_cur_char->permission & TKL_BLE_GATT_PERM_WRITE_ENCRYPT) {
                p_srv_table[total_handle].info |= SEC_LVL(WP, UNAUTH);
            }

            if ((p_cur_char->permission & TKL_BLE_GATT_PERM_PREPARE_WRITE) == 0) {
                p_srv_table[total_handle].ext_info |= OPT(NO_OFFSET);
            }

            total_handle++;

            // cccd
            if (p_cur_char->property & (TKL_BLE_GATT_CHAR_PROP_NOTIFY | TKL_BLE_GATT_CHAR_PROP_INDICATE)) {
                *((uint16_t *)(p_srv_table[total_handle].uuid)) = BLE_GATT_DESC_CLIENT_CHAR_CFG;
                p_srv_table[total_handle].info = PROP(RD) | PROP(WR);
                p_srv_table[total_handle].ext_info = OPT(NO_OFFSET);
                total_handle++;
                srv_msg.data.gatts_info.data.add_srvs.cccd_num++;
            }

            // extended character
            if (p_cur_char->property & TKL_BLE_GATT_CHAR_PROP_EXT_PROP) {
                *((uint16_t *)(p_srv_table[total_handle].uuid)) = BLE_GATT_DESC_CHAR_EXT_PROPERTIES;
                p_srv_table[total_handle].info = PROP(RD);
                p_srv_table[total_handle].ext_info = OPT(NO_OFFSET);

                if (p_cur_char->permission & TKL_BLE_GATT_PERM_PREPARE_WRITE){
                    p_srv_table[total_handle].ext_info |= 0x0001;
                }
                total_handle++;
            }
        }

        if (!ble_local_app_msg_send(&srv_msg, sizeof(tkl_app_msg_t))) {
            sys_mfree(srv_msg.data.gatts_info.data.add_srvs.p_srv_table);
            adp_env.srv_add_handle -= total_handle;
            p_cur_service->handle = 0xFFFF;
            continue;
        }
    }

    return OPRT_OK;
}

OPERATE_RET tuya_hal_gatts_value_set(uint16_t conn_handle, uint16_t char_handle, uint8_t *p_data, uint16_t length)
{
    tkl_app_msg_t set_val_msg;

    set_val_msg.data.gatts_info.data.val_info.p_data = sys_malloc(length);

    if (set_val_msg.data.gatts_info.data.val_info.p_data == NULL) {
        return OPRT_MALLOC_FAILED;
    }

    sys_memcpy(set_val_msg.data.gatts_info.data.val_info.p_data, p_data, length);
    set_val_msg.data.gatts_info.data.val_info.length = length;
    set_val_msg.data.gatts_info.data.val_info.conn_handle = conn_handle;
    set_val_msg.data.gatts_info.data.val_info.char_handle = char_handle;
    set_val_msg.id = TKL_BLE_EVT_ID_GATTS(GATTS_SUBTYPE_SET_VAL);

    if (!ble_local_app_msg_send(&set_val_msg, sizeof(tkl_app_msg_t))) {
        sys_mfree(set_val_msg.data.gatts_info.data.val_info.p_data);
        return OPRT_RESOURCE_NOT_READY;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_hal_gatts_value_notify(uint16_t conn_handle, uint16_t char_handle, uint8_t *p_data, uint16_t length)
{
    tkl_app_msg_t notify_val_msg;
    notify_val_msg.data.gatts_info.data.val_info.p_data = sys_malloc(length);

    if (notify_val_msg.data.gatts_info.data.val_info.p_data == NULL) {
        return OPRT_MALLOC_FAILED;
    }

    sys_memcpy(notify_val_msg.data.gatts_info.data.val_info.p_data, p_data, length);
    notify_val_msg.data.gatts_info.data.val_info.length = length;
    notify_val_msg.data.gatts_info.data.val_info.conn_handle = conn_handle;
    notify_val_msg.data.gatts_info.data.val_info.char_handle = char_handle;
    notify_val_msg.id = TKL_BLE_EVT_ID_GATTS(GATTS_SUBTYPE_VAL_NOTIFY);

    if (!ble_local_app_msg_send(&notify_val_msg, sizeof(tkl_app_msg_t))) {
        sys_mfree(notify_val_msg.data.gatts_info.data.val_info.p_data);
        return OPRT_OS_ADAPTER_BLE_NOTIFY_FAILED;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_hal_gatts_value_indicate(uint16_t conn_handle, uint16_t char_handle, uint8_t *p_data, uint16_t length)
{
    tkl_app_msg_t notify_val_msg;
    notify_val_msg.data.gatts_info.data.val_info.p_data = sys_malloc(length);

    if (notify_val_msg.data.gatts_info.data.val_info.p_data == NULL) {
        return OPRT_MALLOC_FAILED;
    }

    sys_memcpy(notify_val_msg.data.gatts_info.data.val_info.p_data, p_data, length);
    notify_val_msg.data.gatts_info.data.val_info.length = length;
    notify_val_msg.data.gatts_info.data.val_info.conn_handle = conn_handle;
    notify_val_msg.data.gatts_info.data.val_info.char_handle = char_handle;
    notify_val_msg.id = TKL_BLE_EVT_ID_GATTS(GATTS_SUBTYPE_VAL_IND);

    if (!ble_local_app_msg_send(&notify_val_msg, sizeof(tkl_app_msg_t))) {
        sys_mfree(notify_val_msg.data.gatts_info.data.val_info.p_data);
        return OPRT_OS_ADAPTER_BLE_INDICATE_FAILED;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_hal_gattc_all_service_discovery(uint16_t conn_handle)
{
    tkl_app_msg_t disc_srv_msg;

    disc_srv_msg.id = TKL_BLE_EVT_ID_GATTC(GATTC_SUBTYPE_DISC_SRV);
    disc_srv_msg.data.gattc_info.conn_handle = conn_handle;
    disc_srv_msg.data.gattc_info.data.disc_info.start_handle = 0x0001;
    disc_srv_msg.data.gattc_info.data.disc_info.end_handle = 0xFFFF;

    if (!ble_local_app_msg_send(&disc_srv_msg, sizeof(tkl_app_msg_t))) {
        return OPRT_OS_ADAPTER_BLE_SVC_DISC_FAILED;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_hal_gattc_all_char_discovery(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle)
{
    tkl_app_msg_t disc_char_msg;

    disc_char_msg.id = TKL_BLE_EVT_ID_GATTC(GATTC_SUBTYPE_DISC_CHAR);
    disc_char_msg.data.gattc_info.conn_handle = conn_handle;
    disc_char_msg.data.gattc_info.data.disc_info.start_handle = start_handle;
    disc_char_msg.data.gattc_info.data.disc_info.end_handle = end_handle;

    if (!ble_local_app_msg_send(&disc_char_msg, sizeof(tkl_app_msg_t))) {
        return OPRT_OS_ADAPTER_BLE_CHAR_DISC_FAILED;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_hal_gattc_char_desc_discovery(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle)
{
    tkl_app_msg_t disc_desc_msg;

    disc_desc_msg.id = TKL_BLE_EVT_ID_GATTC(GATTC_SUBTYPE_DISC_DESC);
    disc_desc_msg.data.gattc_info.conn_handle = conn_handle;
    disc_desc_msg.data.gattc_info.data.disc_info.start_handle = start_handle;
    disc_desc_msg.data.gattc_info.data.disc_info.end_handle = end_handle;

    if (!ble_local_app_msg_send(&disc_desc_msg, sizeof(tkl_app_msg_t))) {
        return OPRT_OS_ADAPTER_BLE_DESC_DISC_FAILED;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_hal_gattc_write_without_rsp(uint16_t conn_handle, uint16_t char_handle, uint8_t *p_data, uint16_t length)
{
    tkl_app_msg_t write_msg;

    write_msg.data.gattc_info.data.write_info.p_value = sys_malloc(length);

    if (write_msg.data.gattc_info.data.write_info.p_value == NULL) {
        return OPRT_MALLOC_FAILED;
    }

    write_msg.id = TKL_BLE_EVT_ID_GATTC(GATTC_SUBTYPE_WRITE_CMD);
    write_msg.data.gattc_info.conn_handle = conn_handle;
    write_msg.data.gattc_info.data.write_info.char_handle = char_handle;
    write_msg.data.gattc_info.data.write_info.val_len = length;
    sys_memcpy(write_msg.data.gattc_info.data.write_info.p_value, p_data, length);

    if (!ble_local_app_msg_send(&write_msg, sizeof(tkl_app_msg_t))) {
        sys_mfree(write_msg.data.gattc_info.data.write_info.p_value);
        return OPRT_OS_ADAPTER_BLE_WRITE_FAILED;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_hal_gattc_write(uint16_t conn_handle, uint16_t char_handle, uint8_t *p_data, uint16_t length)
{
    tkl_app_msg_t write_msg;

    write_msg.data.gattc_info.data.write_info.p_value = sys_malloc(length);

    if (write_msg.data.gattc_info.data.write_info.p_value == NULL) {
        return OPRT_MALLOC_FAILED;
    }

    write_msg.id = TKL_BLE_EVT_ID_GATTC(GATTC_SUBTYPE_WRITE_REQ);
    write_msg.data.gattc_info.conn_handle = conn_handle;
    write_msg.data.gattc_info.data.write_info.char_handle = char_handle;
    write_msg.data.gattc_info.data.write_info.val_len = length;
    sys_memcpy(write_msg.data.gattc_info.data.write_info.p_value, p_data, length);

    if (!ble_local_app_msg_send(&write_msg, sizeof(tkl_app_msg_t))) {
        sys_mfree(write_msg.data.gattc_info.data.write_info.p_value);
        return OPRT_OS_ADAPTER_BLE_WRITE_FAILED;
    }

    return OPRT_OK;
}

OPERATE_RET tuya_hal_gattc_read(uint16_t conn_handle, uint16_t char_handle)
{
      tkl_app_msg_t write_msg;

      write_msg.data.gattc_info.data.read_char_handle = char_handle;

      write_msg.id = TKL_BLE_EVT_ID_GATTC(GATTC_SUBTYPE_READ_REQ);
      write_msg.data.gattc_info.conn_handle = conn_handle;

      if (!ble_local_app_msg_send(&write_msg, sizeof(tkl_app_msg_t))) {
          return OPRT_OS_ADAPTER_BLE_WRITE_FAILED;
      }

      return OPRT_OK;

}

void tuya_wait_ble_ready(void)
{
    sys_sema_down(&tuya_ble_sema, 0);
}
