/*!
    \file    scan.c
    \brief   Implementation of BLE mesh scanning adapter.

    \version 2024-05-24, V1.0.0, firmware for GD32VW55x
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
#include <stdbool.h>
#include "scan.h"
#include "mesh_cfg.h"
#include "net/buf.h"
#include "api/mesh.h"
#include "bluetooth/bt_str.h"
#include "net.h"
#include "src/solicitation.h"
#include "beacon.h"
#include "prov.h"

#include "ble_gap.h"
#include "ble_scan.h"
#include "wrapper_os.h"
#include "ble_utils.h"
#include "bluetooth/mesh_bluetooth.h"

#define LOG_LEVEL CONFIG_BT_MESH_SCAN_LOG_LEVEL

#include "api/mesh_log.h"

/* Window and Interval are equal for continuous scanning */
#define MESH_SCAN_INTERVAL          BLE_GAP_ADV_SCAN_UNIT(BT_MESH_SCAN_INTERVAL_MS)
#define MESH_SCAN_WINDOW            BLE_GAP_ADV_SCAN_UNIT(BT_MESH_SCAN_WINDOW_MS)

enum {
    SCAN_STATUS_PENDING_NONE,
    SCAN_STATUS_PENDING_ENABLE,
    SCAN_STATUS_PENDING_DISABLE
};

struct ble_mesh_scan_env {
    uint8_t scan_state;
    uint8_t scan_state_pending;
    bool scan_enable;
    bool active_scanning;
    sys_slist_t scan_cb_queue;
};

static struct ble_mesh_scan_env mesh_scan_env = {0};

static void bt_mesh_scan_convert_adv_type(ble_gap_adv_report_type_t adv_type, struct bt_le_scan_recv_info *info)
{
    if (adv_type.extended_pdu) {
        info->adv_type = BT_GAP_ADV_TYPE_EXT_ADV;
    } else {
        info->adv_props = 0;
        if (adv_type.connectable) {
            if (adv_type.scannable) {
                info->adv_type = BT_GAP_ADV_TYPE_ADV_IND;
            } else if (adv_type.directed) {
                info->adv_type = BT_GAP_ADV_TYPE_ADV_DIRECT_IND;
            }
        } else {
            if (adv_type.scannable) {
                info->adv_type = BT_GAP_ADV_TYPE_ADV_SCAN_IND;
            } else if (adv_type.scan_response) {
                info->adv_type = BT_GAP_ADV_TYPE_SCAN_RSP;
            } else {
                info->adv_type = BT_GAP_ADV_TYPE_ADV_NONCONN_IND;
            }
        }
    }

    if (adv_type.connectable) {
        info->adv_props |= BT_GAP_ADV_PROP_CONNECTABLE;
    }

    if (adv_type.scannable) {
        info->adv_props |= BT_GAP_ADV_PROP_SCANNABLE;
    }

    if (adv_type.directed) {
        info->adv_props |= BT_GAP_ADV_PROP_DIRECTED;
    }

    if (adv_type.scan_response) {
        info->adv_props |= BT_GAP_ADV_PROP_SCAN_RESPONSE;
    }
}

static void bt_mesh_adv_reg_scan_list_cb(ble_gap_adv_report_info_t *p_info, ble_data_t *data)
{
    struct net_buf_simple buf = {0};
    struct net_buf_simple_state state;
    struct bt_le_scan_cb *listener, *next;
    struct bt_le_scan_recv_info info;

    if (!sys_slist_is_empty(&mesh_scan_env.scan_cb_queue)) {
        buf.data = data->p_data;
        buf.len = data->len;
        buf.size = data->len;
        buf.__buf = data->p_data;

        info.addr = (bt_addr_le_t *)&p_info->peer_addr;
        info.rssi = p_info->rssi;
        bt_mesh_scan_convert_adv_type(p_info->type, &info);
        SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&mesh_scan_env.scan_cb_queue, listener, next, node) {
            if (listener->recv) {
                net_buf_simple_save(&buf, &state);

                listener->recv(&info, &buf);

                net_buf_simple_restore(&buf, &state);
            }
        }
    }
}

static void bt_mesh_scan_cb( ble_gap_addr_t *addr, int8_t rssi, ble_gap_adv_report_type_t adv_type, ble_data_t *data)
{
    uint8_t len, type;
    struct net_buf_simple buf = {0};
    struct net_buf_simple_state state;

    if (adv_type.connectable != 0 || adv_type.scannable != 0)
        return;

    buf.data = data->p_data;
    buf.len = data->len;
    buf.size = data->len;
    buf.__buf = data->p_data;

    while (buf.len > 1) {
        len = net_buf_simple_pull_u8(&buf);
        /* Check for early termination */
        if (len == 0U) {
            return;
        }

        if (len > buf.len) {
            LOG_ERR("AD malformed\r\n");
            return;
        }

        net_buf_simple_save(&buf, &state);

        type = net_buf_simple_pull_u8(&buf);

        buf.len = len - 1;

        switch (type) {
        case BLE_AD_TYPE_MESH_MESSAGE:
            LOG_DBG("recv message, len %u: %s", buf.len, bt_hex(buf.data, buf.len));
            bt_mesh_net_recv(&buf, rssi, BT_MESH_NET_IF_ADV);
            break;
#if defined(CONFIG_BT_MESH_PB_ADV)
        case BLE_AD_TYPE_MESH_PROV:
            bt_mesh_pb_adv_recv(&buf);
            break;
#endif
        case BLE_AD_TYPE_MESH_BEACON:
            LOG_DBG("recv beacon, len %u: %s", buf.len, bt_hex(buf.data, buf.len));
            bt_mesh_beacon_recv(&buf);
            break;
        case BLE_AD_TYPE_SERVICE_UUID_16_MORE:
            /* Fall through */
        case BLE_AD_TYPE_SERVICE_UUID_16_COMPLETE:
            if (IS_ENABLED(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV)) {
                /* Restore buffer with Solicitation PDU */
                net_buf_simple_restore(&buf, &state);
                bt_mesh_sol_recv(&buf, len - 1);
            }
            break;
        default:
            break;
        }

        net_buf_simple_restore(&buf, &state);
        net_buf_simple_pull(&buf, len);
    }
}

static void ble_mesh_scan_mgr_evt_handler(ble_scan_evt_t event, ble_scan_data_u *p_data)
{
    ble_gap_adv_report_info_t *p_info;

    if (!mesh_scan_env.scan_enable)
        return;

    switch (event) {
    case BLE_SCAN_EVT_STATE_CHG:
        mesh_scan_env.scan_state = p_data->scan_state.scan_state;
        if (p_data->scan_state.scan_state == BLE_SCAN_STATE_ENABLED) {
            LOG_INF("Ble Scan enabled status 0x%x", p_data->scan_state.reason);

            if (mesh_scan_env.scan_state_pending == SCAN_STATUS_PENDING_DISABLE) {
                bt_mesh_scan_disable();
            }
            mesh_scan_env.scan_state_pending = SCAN_STATUS_PENDING_NONE;
        } else if (p_data->scan_state.scan_state == BLE_SCAN_STATE_DISABLED) {
            LOG_INF("Ble Scan disabled  status 0x%x", p_data->scan_state.reason);
            mesh_scan_env.scan_enable = false;

            if (mesh_scan_env.scan_state_pending == SCAN_STATUS_PENDING_ENABLE) {
                bt_mesh_scan_enable();
            }
            mesh_scan_env.scan_state_pending = SCAN_STATUS_PENDING_NONE;
        }
        break;

    case BLE_SCAN_EVT_ADV_RPT:
        p_info = p_data->p_adv_rpt;
        bt_mesh_scan_cb(&p_info->peer_addr, p_info->rssi, p_info->type, &p_info->data);
        bt_mesh_adv_reg_scan_list_cb(p_info, &p_info->data);
        break;

    default:
        break;
    }
}

int bt_mesh_scan_enable(void)
{
    ble_status_t err;
    ble_gap_scan_param_t scan_param = {
        .type = BLE_GAP_SCAN_TYPE_OBSERVER,
        .prop = BLE_GAP_SCAN_PROP_PHY_1M_BIT | (mesh_scan_env.active_scanning ? BLE_GAP_SCAN_PROP_ACTIVE_1M_BIT : 0),
        .dup_filt_pol = BLE_GAP_DUP_FILT_DIS,
        .scan_intv_1m = MESH_SCAN_INTERVAL,
        .scan_win_1m = MESH_SCAN_WINDOW,
        .duration = 0,
        .period = 0,
    };

    if (mesh_scan_env.scan_state == BLE_SCAN_STATE_ENABLED) {
        LOG_DBG("Already enable");
        return 0;
    }

    err = ble_scan_param_set(BLE_GAP_LOCAL_ADDR_STATIC, &scan_param);
    if (err != BLE_ERR_NO_ERROR) {
        if (err == BLE_GAP_ERR_COMMAND_DISALLOWED && mesh_scan_env.scan_state == BLE_SCAN_STATE_DISABLING) {
            mesh_scan_env.scan_state_pending = SCAN_STATUS_PENDING_ENABLE;
            LOG_DBG("Pending scan enable");
            return 0;
        }

        LOG_ERR("set scan param failed (err %d), state %d, pending state %d", err, mesh_scan_env.scan_state, mesh_scan_env.scan_state_pending);
        return err;
    }

    mesh_scan_env.scan_enable = true;

    err = ble_scan_enable();
    if (err != BLE_ERR_NO_ERROR) {
        LOG_ERR("starting scan failed (err %d)", err);
        return err;
    }

    return 0;
}

int bt_mesh_scan_disable(void)
{
    ble_status_t err;

    if (mesh_scan_env.scan_state == BLE_SCAN_STATE_DISABLED) {
        LOG_DBG("Already disable");
        return 0;
    }

    err = ble_scan_disable();
    if (err != BLE_ERR_NO_ERROR) {
        if (err != BLE_GAP_ERR_COMMAND_DISALLOWED) {
            LOG_ERR("stopping scan failed (err %d)", err);
            return err;
        }

        if (mesh_scan_env.scan_state == BLE_SCAN_STATE_DISABLING) {
            LOG_DBG("Duplicate scan disable");
            return 0;
        }

        if (mesh_scan_env.scan_state == BLE_SCAN_STATE_ENABLING) {
            mesh_scan_env.scan_state_pending = SCAN_STATUS_PENDING_DISABLE;
            LOG_DBG("Pending scan disable");
            return 0;
        }
    }

    return 0;
}


int bt_mesh_scan_active_set(bool active)
{
    if (mesh_scan_env.active_scanning == active)
        return 0;

    mesh_scan_env.active_scanning = active;
    bt_mesh_scan_disable();
    mesh_scan_env.scan_state_pending = SCAN_STATUS_PENDING_ENABLE;
    return 0;
}

void bt_le_scan_cb_register(struct bt_le_scan_cb *cb)
{
    // FIX TODO we may need a mutex
    sys_slist_append(&mesh_scan_env.scan_cb_queue, &cb->node);
}

void bt_le_scan_cb_unregister(struct bt_le_scan_cb *cb)
{
    // FIX TODO we may need a mutex
    sys_slist_find_and_remove(&mesh_scan_env.scan_cb_queue, &cb->node);
}

int ble_mesh_scan_init(void)
{
    ble_scan_callback_register(ble_mesh_scan_mgr_evt_handler);

    return 0;
}

