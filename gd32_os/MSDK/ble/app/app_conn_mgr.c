/*!
    \file    app_scan_mgr.c
    \brief   Implementation of BLE application scan manager to record devices.

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

#include "ble_app_config.h"

#if (BLE_APP_SUPPORT && (BLE_CFG_ROLE & (BLE_CFG_ROLE_PERIPHERAL | BLE_CFG_ROLE_CENTRAL)))
#include <string.h>
#include "app_conn_mgr.h"
#include "app_dev_mgr.h"
#include "ble_conn.h"
#include "ble_adapter.h"
#include "app_adapter_mgr.h"

#include "wrapper_os.h"
#include "dlist.h"
#include "app_sec_mgr.h"
#include "dbg_print.h"
#include "ble_storage.h"
#include "ble_gattc.h"

/* Connection parameters of fast paramter setting */
#define BLE_CONN_FAST_INTV             6               // 7.5ms
#define BLE_CONN_FAST_LATENCY          0
#define BLE_CONN_FAST_SUPV_TOUT        100             // 1000ms

/* Local appearance */
static uint16_t dev_appearance = 0x0000;    // Generic Unknown

/* Record phy value when connection established */
ble_phy_params_t phy[BLE_MAX_CONN_NUM] = {0};

/*!
    \brief      Enable fast connection parameters
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     bool: true if fast parameter is successfully enabled, otherwise false
*/
bool app_conn_fast_param_enable(uint8_t conn_id)
{
    ble_device_t *p_device;
    ble_status_t status = BLE_ERR_NO_ERROR;

    p_device = dm_find_dev_by_conidx(conn_id);

    // Only central role can enable fast parameters update
    if (p_device == NULL || p_device->role != BLE_MASTER) {
        return false;
    }

    if (p_device->enable_fast_param) {
        return true;
    }

    if (p_device->update_state == IDLE_STATE) {
        status = ble_conn_param_update_req(conn_id, BLE_CONN_FAST_INTV, BLE_CONN_FAST_INTV,
                                           BLE_CONN_FAST_LATENCY, BLE_CONN_FAST_SUPV_TOUT, 0, 0);
        if (status == BLE_ERR_NO_ERROR) {
            p_device->update_state = FAST_PARAM_UPDATING_STATE;

            // No pending parameters
            if (p_device->expect_conn_info.interval == 0) {
                p_device->expect_conn_info = p_device->conn_info;
            }
        }
    }

    if (status == BLE_ERR_NO_ERROR) {
        p_device->enable_fast_param = true;
    }

    return status == BLE_ERR_NO_ERROR;
}

/*!
    \brief      Disable fast connection parameters
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     bool: true if fast parameter is successfully disabled, otherwise false
*/
bool app_conn_fast_param_disable(uint8_t conn_id)
{
    ble_device_t *p_device;
    ble_status_t status = BLE_ERR_NO_ERROR;

    p_device = dm_find_dev_by_conidx(conn_id);

    if (p_device == NULL) {
        return false;
    }

    if (!p_device->enable_fast_param) {
        return true;
    }

    if (p_device->update_state == FAST_PARAM_UPDATED_STATE) {
        if (p_device->expect_conn_info.interval != BLE_CONN_FAST_INTV ||
            p_device->expect_conn_info.latency != BLE_CONN_FAST_LATENCY ||
            p_device->expect_conn_info.supv_tout != BLE_CONN_FAST_SUPV_TOUT) {
            status = ble_conn_param_update_req(conn_id, p_device->expect_conn_info.interval, p_device->expect_conn_info.interval,
                                               p_device->expect_conn_info.latency, p_device->expect_conn_info.supv_tout, 0, 0);

            if (status == BLE_ERR_NO_ERROR) {
                p_device->update_state = UPDATING_STATE;
            } else {
                p_device->update_state = IDLE_STATE;
            }
        } else {
            p_device->update_state = IDLE_STATE;
        }

        memset(&p_device->expect_conn_info, 0, sizeof(ble_conn_params_t));
    }

    p_device->enable_fast_param = false;

    return true;
}

/*!
    \brief      Update connection parameters
    \param[in]  conn_id: connection index
    \param[in]  interval: connection interval
    \param[in]  latency: connection latency
    \param[in]  supv_to: supervision timeout
    \param[in]  ce_len: connection event length
    \param[out] none
    \retval     bool: true if parameter update is successfully started, otherwise false
*/
bool app_conn_update_param(uint8_t conn_id, uint16_t interval, uint16_t latency,
                           uint16_t supv_to, uint16_t ce_len)
{
    ble_device_t *p_device;
    ble_status_t status = BLE_ERR_NO_ERROR;

    p_device = dm_find_dev_by_conidx(conn_id);

    if (p_device == NULL) {
        return false;
    }

    if (p_device->update_state != IDLE_STATE) {
        p_device->expect_conn_info.interval = interval;
        p_device->expect_conn_info.latency = latency;
        p_device->expect_conn_info.supv_tout = supv_to;
        p_device->expect_conn_info.ce_len_min = p_device->expect_conn_info.ce_len_max = ce_len;
    } else {
        status = ble_conn_param_update_req(conn_id, interval, interval, latency, supv_to, ce_len, ce_len);

        if (status == BLE_ERR_NO_ERROR) {
            p_device->update_state = UPDATING_STATE;
        }
    }

    return status == BLE_ERR_NO_ERROR;
}

/*!
    \brief      Connection phy get
    \param[in]  tx_phy: pointer to tx_phy
    \param[in]  rx_phy: pointer to rx_phy
    \param[out] none
    \retval     bool: true if get phy value successfully started, otherwise false
*/
bool app_conn_phy_get(uint8_t conn_idx, uint8_t *tx_phy, uint8_t *rx_phy)
{
    ble_device_t *p_device;

    p_device = dm_find_dev_by_conidx(conn_idx);

    if (p_device == NULL) {
        return false;
    }

    *tx_phy = phy[conn_idx].tx_phy;
    *rx_phy = phy[conn_idx].rx_phy;

    return true;
}

/*!
    \brief      Check connection parameters and update if necessary
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     none
*/
static void check_param_update_op(uint8_t conn_id)
{
    ble_device_t *p_device = dm_find_dev_by_conidx(conn_id);

    if (p_device != NULL && p_device->expect_conn_info.interval != 0) {
        if (p_device->conn_info.interval != p_device->expect_conn_info.interval ||
            p_device->conn_info.latency != p_device->expect_conn_info.latency ||
            p_device->conn_info.supv_tout != p_device->expect_conn_info.supv_tout) {
            if (ble_conn_param_update_req(conn_id, p_device->expect_conn_info.interval, p_device->expect_conn_info.interval,
                                          p_device->expect_conn_info.latency, p_device->expect_conn_info.supv_tout, 0, 0) == BLE_ERR_NO_ERROR) {
                p_device->update_state = UPDATING_STATE;
            } else {
                dbg_print(WARNING, "check_param_update_op conn_id %d update param fail \r\n", conn_id);
                p_device->update_state = IDLE_STATE;
            }
        }

        memset(&p_device->expect_conn_info, 0, sizeof(ble_conn_params_t));
    }
}

/*!
    \brief      Function to handle connection established event
    \param[in]  p_data: pointer to BLE connection event data
    \param[out] none
    \retval     none
*/
static void app_conn_mgr_handle_connected(ble_conn_data_u *p_data)
{
    ble_device_t *p_device;
    dbg_print(NOTICE, "connect success. conn idx:%u, conn_hdl:0x%x, interval:0x%x, latancy:0x%x, supv_tout:0x%x\r\n",
              p_data->conn_state.info.conn_info.conn_idx, p_data->conn_state.info.conn_info.conn_hdl,
              p_data->conn_state.info.conn_info.con_interval, p_data->conn_state.info.conn_info.con_latency, p_data->conn_state.info.conn_info.sup_to);

    p_device = dm_find_alloc_dev_by_addr(p_data->conn_state.info.conn_info.peer_addr);
    if (p_device != NULL) {
        p_device->conn_idx = p_data->conn_state.info.conn_info.conn_idx;
        p_device->conn_hdl = p_data->conn_state.info.conn_info.conn_hdl;
        p_device->state = BLE_CONN_STATE_CONNECTED;
        p_device->role = p_data->conn_state.info.conn_info.role;
        p_device->conn_info.interval = p_data->conn_state.info.conn_info.con_interval;
        p_device->conn_info.latency = p_data->conn_state.info.conn_info.con_latency;
        p_device->conn_info.supv_tout = p_data->conn_state.info.conn_info.sup_to;
        p_device->conn_info.ce_len_min = p_device->conn_info.ce_len_max = 0;

        // If security keys are managered by APP, need to set sec infor
        if (app_sec_user_key_mgr_get()) {
            ble_gap_sec_bond_data_t bond_data = {0};

            ble_peer_data_bond_load(&p_data->conn_state.info.conn_info.peer_addr, &bond_data);
            ble_conn_sec_info_set(p_device->conn_idx, bond_data.local_csrk.csrk, bond_data.peer_csrk.csrk,
                                  bond_data.pairing_lvl, bond_data.enc_key_present);
        }

        if (p_device->role == BLE_MASTER) {
            ble_conn_peer_version_get(p_data->conn_state.info.conn_info.conn_idx);
            ble_conn_peer_feats_get(p_data->conn_state.info.conn_info.conn_idx);

            if (p_device->bonded && (p_device->bond_info.key_msk & BLE_PEER_LTK_ENCKEY)) {
                app_sec_send_encrypt_req(p_device->conn_idx);
            } else {
                if (app_sec_is_pairing_device(p_device->cur_addr)) {
                    app_sec_send_bond_req(p_device->conn_idx);
                }
            }
        } else {
            // Slave always need encryption or bond
            if (app_sec_need_authen_bond()) {
                app_sec_send_security_req(p_device->conn_idx);
            }
        }

        // FIX TODO enable profiles
    }
}

/*!
    \brief      Callback function to handle when GATT discovery is done
    \param[in]  conn_idx: connection index
    \param[in]  status: GATT discovery status
    \param[out] none
    \retval     none
*/
static void ble_app_conn_gatt_discovery_callback(uint8_t conn_idx, uint16_t status)
{
    ble_conn_enable_central_feat(conn_idx);
    app_conn_fast_param_disable(conn_idx);
}

/*!
    \brief      Callback function to handle BLE connection event
    \param[in]  event: BLE connection event type
    \param[in]  p_data: pointer to BLE connection event data
    \param[out] none
    \retval     none
*/
static void ble_app_conn_evt_handler(ble_conn_evt_t event, ble_conn_data_u *p_data)
{
    ble_device_t *p_device;

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
        if (p_data->conn_state.state == BLE_CONN_STATE_DISCONNECTD) {
            dbg_print(NOTICE, "disconnected. conn idx: %u, conn_hdl: 0x%x reason 0x%x\r\n",
                      p_data->conn_state.info.discon_info.conn_idx, p_data->conn_state.info.discon_info.conn_hdl,
                      p_data->conn_state.info.discon_info.reason);
            dm_handle_dev_disconnected(p_data->conn_state.info.discon_info.conn_idx);
        } else if (p_data->conn_state.state == BLE_CONN_STATE_CONNECTED) {
            app_conn_mgr_handle_connected(p_data);

#if (BLE_APP_GATT_CLIENT_SUPPORT)
            if (p_data->conn_state.info.conn_info.role == BLE_MASTER) {
                app_conn_fast_param_enable(p_data->conn_state.info.conn_info.conn_idx);
                ble_gattc_start_discovery(p_data->conn_state.info.conn_info.conn_idx,
                                          ble_app_conn_gatt_discovery_callback);
            }
#endif
        }
    }
    break;

    case BLE_CONN_EVT_DISCONN_RSP: {
        ble_device_t *p_device;

        if (p_data->disconn_rsp.status) {
            dbg_print(NOTICE, "disconnect fail. conn idx %u, reason 0x%x\r\n",
                      p_data->disconn_rsp.conn_idx, p_data->disconn_rsp.status);
            p_device = dm_find_dev_by_conidx(p_data->disconn_rsp.conn_idx);
            if (p_device != NULL) {
                p_device->state = BLE_CONN_STATE_CONNECTED;
            }
        }
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
            dbg_print(NOTICE,
                      "conn idx: %u, conn_intv_min: 0x%x, conn_intv_max: 0x%x, latency: %d, timeout: %d\r\n",
                      p_data->peer_slv_prf_param.conn_idx,
                      p_data->peer_slv_prf_param.conn_intv_min, p_data->peer_slv_prf_param.conn_intv_max,
                      p_data->peer_slv_prf_param.latency,
                      p_data->peer_slv_prf_param.conn_timeout);
        }
    }
    break;

    case BLE_CONN_EVT_PEER_ADDR_RESLV_GET_RSP: {
        if (p_data->peer_addr_reslv_sup.status == BLE_ERR_NO_ERROR) {
            dbg_print(NOTICE, "conn idx: %u, central address resolution support %d\r\n",
                      p_data->peer_addr_reslv_sup.conn_idx,
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
        dbg_print(NOTICE, "conn idx %u rssi: %d\r\n", p_data->rssi_ind.conn_idx, p_data->rssi_ind.rssi);
    }
    break;

    case BLE_CONN_EVT_CHANN_MAP_GET_RSP: {
        dbg_print(NOTICE, "channel map: 0x%02x%02x%02x%02x%02x\r\n", p_data->chnl_map_ind.chann_map[4],
                  p_data->chnl_map_ind.chann_map[3], p_data->chnl_map_ind.chann_map[2],
                  p_data->chnl_map_ind.chann_map[1], p_data->chnl_map_ind.chann_map[0]);
    }
    break;

    case BLE_CONN_EVT_NAME_GET_IND: {
        uint16_t cmpl_len;
        uint8_t *p_name;
        uint16_t name_len;

        cmpl_len = app_adp_get_name(&p_name);
        p_name += p_data->name_get_ind.name_offset;
        name_len = cmpl_len - p_data->name_get_ind.name_offset;
        if (name_len > p_data->name_get_ind.max_name_length) {
            name_len = p_data->name_get_ind.max_name_length;
        }

        ble_conn_name_get_cfm(p_data->name_get_ind.conn_idx, 0, p_data->name_get_ind.token,
                              cmpl_len, p_name, name_len);
    }
    break;

    case BLE_CONN_EVT_APPEARANCE_GET_IND: {
        dbg_print(NOTICE, "conn idx %u appearance acquire \r\n", p_data->appearance_get_ind.conn_idx);

        ble_conn_appearance_get_cfm(p_data->appearance_get_ind.conn_idx, 0,
                                    p_data->appearance_get_ind.token, dev_appearance);
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

        ble_conn_slave_prefer_param_get_cfm(p_data->slave_prefer_param_get_ind.conn_idx, 0,
                                            p_data->slave_prefer_param_get_ind.token, &param);
    }
    break;

    case BLE_CONN_EVT_NAME_SET_IND: {
        ble_adp_name_set(p_data->name_set_ind.p_name, p_data->name_set_ind.name_len);

        dbg_print(NOTICE, "conn idx %u, name set %s, name len %u \r\n",
                  p_data->name_set_ind.conn_idx, p_data->name_set_ind.p_name,
                  p_data->name_set_ind.name_len);

        ble_conn_name_set_cfm(p_data->name_set_ind.conn_idx, 0, p_data->name_set_ind.token);
    }
    break;

    case BLE_CONN_EVT_APPEARANCE_SET_IND: {
        dev_appearance = p_data->appearance_set_ind.appearance;

        dbg_print(NOTICE, "conn idx %u, appearance set 0x%x\r\n", p_data->appearance_set_ind.conn_idx,
                  p_data->appearance_set_ind.appearance);

        ble_conn_appearance_set_cfm(p_data->appearance_set_ind.conn_idx, 0,
                                    p_data->appearance_set_ind.token);
    }
    break;

    case BLE_CONN_EVT_PARAM_UPDATE_IND: {
        dbg_print(NOTICE, "conn idx %u, intv_min 0x%x, intv_max 0x%x, latency %u, supv_tout %u\r\n",
                  p_data->conn_param_req_ind.conn_idx, p_data->conn_param_req_ind.intv_min,
                  p_data->conn_param_req_ind.intv_max, p_data->conn_param_req_ind.latency,
                  p_data->conn_param_req_ind.supv_tout);
        p_device = dm_find_dev_by_conidx(p_data->conn_param_req_ind.conn_idx);

        if (p_device != NULL && p_device->enable_fast_param) {
            dbg_print(WARNING, "fast parameters enabled, reject remote param update indication\r\n");
            ble_conn_param_update_cfm(p_data->conn_param_req_ind.conn_idx, false, 0, 0);
        } else if (p_data->conn_param_req_ind.intv_max < p_data->conn_param_req_ind.intv_min) {
            ble_conn_param_update_cfm(p_data->conn_param_req_ind.conn_idx, false, 0, 0);
        } else {
            ble_conn_param_update_cfm(p_data->conn_param_req_ind.conn_idx, true, 2, 4);
        }
    }
    break;

    case BLE_CONN_EVT_PARAM_UPDATE_RSP: {
        dbg_print(NOTICE, "conn idx %u, param update result status: 0x%x\r\n",
                  p_data->conn_param_rsp.conn_idx, p_data->conn_param_rsp.status);

        p_device = dm_find_dev_by_conidx(p_data->conn_param_req_ind.conn_idx);

        if (p_device != NULL) {
            if (p_data->conn_param_rsp.status != BLE_ERR_NO_ERROR) {
                dbg_print(WARNING, "conn idx %u, param update fail update_state: 0x%x\r\n",
                          p_data->conn_param_rsp.conn_idx, p_device->update_state);
                p_device->enable_fast_param = false;
                p_device->update_state = IDLE_STATE;

                check_param_update_op(p_data->conn_param_rsp.conn_idx);
            }
        }
    }
    break;

    case BLE_CONN_EVT_PARAM_UPDATE_INFO: {
        dbg_print(NOTICE, "conn idx %u, param update ind: interval 0x%x, latency 0x%x, sup to 0x%x\r\n",
                  p_data->conn_params.conn_idx,
                  p_data->conn_params.interval, p_data->conn_params.latency, p_data->conn_params.supv_tout);

        p_device = dm_find_dev_by_conidx(p_data->conn_params.conn_idx);

        if (p_device != NULL) {
            p_device->conn_info.interval = p_data->conn_params.interval;
            p_device->conn_info.latency = p_data->conn_params.latency;
            p_device->conn_info.supv_tout = p_data->conn_params.supv_tout;

            if (p_device->enable_fast_param) {
                if (p_device->update_state == FAST_PARAM_UPDATING_STATE) {
                    p_device->update_state = FAST_PARAM_UPDATED_STATE;
                } else if (p_device->update_state == UPDATING_STATE) {
                    if (ble_conn_param_update_req(p_data->conn_params.conn_idx, BLE_CONN_FAST_INTV, BLE_CONN_FAST_INTV,
                                                  BLE_CONN_FAST_LATENCY, BLE_CONN_FAST_SUPV_TOUT, 0, 0) == BLE_ERR_NO_ERROR) {
                        p_device->update_state = FAST_PARAM_UPDATING_STATE;

                        // No pending parameters
                        if (p_device->expect_conn_info.interval == 0) {
                            p_device->expect_conn_info = p_device->conn_info;
                        }
                    } else {
                        p_device->update_state = IDLE_STATE;
                        p_device->enable_fast_param = false;
                        check_param_update_op(p_data->conn_params.conn_idx);
                    }
                }
            } else {
                p_device->update_state = IDLE_STATE;
                check_param_update_op(p_data->conn_params.conn_idx);
            }
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
        phy[p_data->phy_val.conn_idx].tx_phy = p_data->phy_val.tx_phy;
        phy[p_data->phy_val.conn_idx].rx_phy = p_data->phy_val.rx_phy;
    }
    break;

    case BLE_CONN_EVT_LOC_TX_PWR_GET_RSP: {
        dbg_print(NOTICE, "local tx pwr conn idx %u, phy %d, pwr %d, max %d\r\n",
                  p_data->loc_tx_pwr.conn_idx,
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

/*!
    \brief      Init APP connection manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_conn_mgr_init(void)
{
    ble_conn_callback_register(ble_app_conn_evt_handler);
}

/*!
    \brief      Deinit APP connection manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_conn_mgr_deinit(void)
{
    ble_conn_callback_unregister(ble_app_conn_evt_handler);
}

/*!
    \brief      Reset APP connection manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_conn_mgr_reset(void)
{

}

#endif // (BLE_APP_SUPPORT && (BLE_CFG_ROLE & (BLE_CFG_ROLE_PERIPHERAL | BLE_CFG_ROLE_CENTRAL)))
