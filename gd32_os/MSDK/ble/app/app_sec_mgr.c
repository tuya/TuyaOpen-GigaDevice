/*!
    \file    app_sec_mgr.c
    \brief   Implementation of BLE application security manager.

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
#include <stdlib.h>

#include "app_sec_mgr.h"
#include "ble_sec.h"
#include "app_dev_mgr.h"
#include "ble_storage.h"
#include "ble_adapter.h"
#include "dbg_print.h"
#include "wrapper_os.h"

#define APP_SEC_INVALID_PIN_CODE            (1000000)       /* pin code should between 000000 ~ 999999 */

/* BLE bond state */
typedef enum
{
    BLE_BOND_STATE_NONE,                /*!< The device bond state none */
    BLE_BOND_STATE_BONDING,             /*!< The device is initiating connection and pairing */
    BLE_BOND_STATE_BONDED,              /*!< The device pairing success */
} ble_bond_state_t;

/* Structure of pairing procedure callback */
typedef struct
{
    ble_bond_state_t  state;                /*!< Bond state */
    ble_gap_addr_t    addr;                 /*!< Address of pairing device */
    bool              is_local_initiated;   /*!< If pairing is initiated by local device */
} pairing_cb_t;

/* Application security manager module structure */
typedef struct
{
    bool authen_bond;                   /*!< Is support bond authentication */
    bool authen_mitm;                   /*!< Is support man in the middle protection */
    bool authen_sc;                     /*!< Is support secure connection */
    bool sc_only;                       /*!< Is secure connection pairing with encryption */
    uint8_t io_capability;              /*!< IO capabilities. @def ble_gap_io_cap_t */
    uint8_t key_size;                   /*!< LTK Key Size */
    bool                oob;            /*!< Is support OOB information */
    ble_gap_oob_data_t  oob_data;       /*!< OOB information */
    uint32_t pin_code;
} app_sec_env_t;

/* Application security manager module data */
static app_sec_env_t app_sec_env;

/* Pairing procedure callback data */
static pairing_cb_t pairing_cb;

/* Security key managered by APP */
static bool app_sec_mgr_key = false;

static app_sec_callbacks app_sec_cb;

/*!
    \brief      Callback function to handle @ref BLE_SEC_EVT_PAIRING_REQ_IND event
    \param[in]  p_ind: pointer to the pairing request indication data
    \param[out] none
    \retval     none
*/
static void app_pairing_req_hdlr(ble_gap_pairing_req_ind_t *p_ind)
{
    ble_device_t *p_device = dm_find_dev_by_conidx(p_ind->conn_idx);
    ble_gap_pairing_param_t param;
    uint8_t sec_req_lvl;

    dbg_print(NOTICE, "app receivce pairing request conidx %u \r\n", p_ind->conn_idx);

    if (p_device == NULL) {
        dbg_print(NOTICE, "app_pairing_req_hdlr can't find device !\r\n");
        ble_sec_pairing_req_cfm(p_ind->conn_idx, false, NULL, 0);
        return;
    }

    if (p_device->bonded) {
        dbg_print(NOTICE, "remote master key missing, reject pairing request !\r\n");

        if (!app_sec_remove_bond(p_device->cur_addr)) {
            dbg_print(NOTICE,"app_pairing_req_hdlr remove bond fail\r\n");
        }

        ble_sec_pairing_req_cfm(p_ind->conn_idx, false, NULL, 0);
        return;
    }

    sec_req_lvl = BLE_GAP_NO_SEC;
    param.auth = BLE_GAP_AUTH_MASK_NONE;

    if (app_sec_env.authen_bond) {
        param.auth |= BLE_GAP_AUTH_MASK_BOND;
    }

    if (app_sec_env.authen_mitm) {
        param.auth |= BLE_GAP_AUTH_MASK_MITM;
    }

    if (app_sec_env.authen_sc) {
        param.auth |= BLE_GAP_AUTH_MASK_SEC_CON;
    }

    param.iocap = app_sec_env.io_capability;
    param.key_size = app_sec_env.key_size;
    param.oob = app_sec_env.oob;

    if (app_sec_env.sc_only) {
        sec_req_lvl = BLE_GAP_SEC1_SEC_CON_PAIR_ENC;
    }

    if (app_sec_env.authen_sc) {
        param.ikey_dist = BLE_GAP_KDIST_IDKEY | BLE_GAP_KDIST_SIGNKEY;
        param.rkey_dist = BLE_GAP_KDIST_IDKEY | BLE_GAP_KDIST_SIGNKEY;
    } else {
        param.ikey_dist = BLE_GAP_KDIST_IDKEY | BLE_GAP_KDIST_SIGNKEY | BLE_GAP_KDIST_ENCKEY;
        param.rkey_dist = BLE_GAP_KDIST_IDKEY | BLE_GAP_KDIST_SIGNKEY | BLE_GAP_KDIST_ENCKEY;
    }

    ble_sec_pairing_req_cfm(p_ind->conn_idx, true, &param, sec_req_lvl);
}

/*!
    \brief      Callback function to handle @ref BLE_SEC_EVT_LTK_REQ_IND event
    \param[in]  p_ind: pointer to the LTK request indication data
    \param[out] none
    \retval     none
*/
static void app_ltk_req_hdlr(ble_gap_ltk_req_ind_t *p_ind)
{
    uint8_t i;
    ble_gap_ltk_t ltk;
    ble_device_t *p_device = dm_find_dev_by_conidx(p_ind->conn_idx);

    if (p_device == NULL) {
        dbg_print(NOTICE, "app_ltk_req_hdlr can't find device !\r\n");
        ble_sec_ltk_req_cfm(p_ind->conn_idx, false, NULL);
        return;
    }

    ltk.key_size = p_ind->key_size;
    // Generate all the values
    sys_random_bytes_get((void *)&ltk.ediv, 2);
    sys_random_bytes_get(ltk.rnd_num, BLE_GAP_RANDOM_NUMBER_LEN);
    sys_random_bytes_get(ltk.ltk, BLE_GAP_KEY_LEN);

    if (app_sec_env.authen_bond) {
        p_device->bond_info.local_ltk.key_size = p_ind->key_size;
        p_device->bond_info.local_ltk.ediv = ltk.ediv;
        memcpy(p_device->bond_info.local_ltk.ltk, ltk.ltk, BLE_GAP_KEY_LEN);
        memcpy(p_device->bond_info.local_ltk.rnd_num, ltk.rnd_num, BLE_GAP_RANDOM_NUMBER_LEN);
    }

    p_device->bond_info.key_msk |= BLE_LOC_LTK_ENCKEY;

    dbg_print(NOTICE, "conn_idx %u bond ltk req, key size %d, ltk: 0x", p_ind->conn_idx, p_ind->key_size);
    for (i = 0; i < BLE_GAP_KEY_LEN; i++) {
        dbg_print(NOTICE, "%x", ltk.ltk[i]);
    }

    dbg_print(NOTICE, "\r\n");

    ble_sec_ltk_req_cfm(p_ind->conn_idx, true, &ltk);
}

/*!
    \brief      Callback function to handle @ref BLE_SEC_EVT_KEY_DISPLAY_REQ_IND event
    \param[in]  p_ind: pointer to the passkey display TK request indication data
    \param[out] none
    \retval     none
*/
static void app_key_display_req_hdlr(ble_gap_tk_req_ind_t *p_ind)
{
    // Generate a PIN Code- (Between 100000 and 999999)
    uint32_t pin_code = (100000 + (((uint32_t)rand()) % 900000));
    ble_device_t *p_device = dm_find_dev_by_conidx(p_ind->conn_idx);

    if (p_device == NULL) {
        dbg_print(NOTICE, "app_key_display_req_hdlr can't find device !\r\n");
        ble_sec_key_display_enter_cfm(p_ind->conn_idx, false, pin_code);
        return;
    }

    if ((app_sec_env.pin_code != APP_SEC_INVALID_PIN_CODE) && (app_sec_env.pin_code <= 999999)) {
        pin_code = app_sec_env.pin_code;
    }

    dbg_print(NOTICE, "pin code %d\r\n", pin_code);

    ble_sec_key_display_enter_cfm(p_ind->conn_idx, true, pin_code);
}

/*!
    \brief      Callback function to handle @ref BLE_SEC_EVT_KEY_ENTER_REQ_IND event
    \param[in]  p_ind: pointer to the input passkey TK request indication data
    \param[out] none
    \retval     none
*/
static void app_key_enter_req_hdlr(ble_gap_tk_req_ind_t *p_ind)
{
    uint32_t pin_code = 0;
    ble_device_t *p_device = dm_find_dev_by_conidx(p_ind->conn_idx);

    if (p_device == NULL) {
        dbg_print(NOTICE, "app_key_enter_req_hdlr can't find device !\r\n");
        ble_sec_key_display_enter_cfm(p_ind->conn_idx, false, pin_code);
        return;
    }

    dbg_print(NOTICE, "conn_idx %u waiting for user to input key ......\r\n", p_ind->conn_idx);

    if (app_sec_cb.input_key_req) {
        app_sec_cb.input_key_req(p_ind->conn_idx);
    }
}

/*!
    \brief      Callback function to handle @ref BLE_SEC_EVT_KEY_OOB_REQ_IND event
    \param[in]  p_ind: pointer to the OOB TK request indication data
    \param[out] none
    \retval     none
*/
static void app_key_oob_req_hdlr(ble_gap_tk_req_ind_t *p_ind)
{
    ble_device_t *p_device = dm_find_dev_by_conidx(p_ind->conn_idx);

    if (p_device == NULL) {
        dbg_print(NOTICE, "app_key_oob_req_hdlr can't find device !\r\n");
        ble_sec_oob_req_cfm(p_ind->conn_idx, false, NULL);
        return;
    }

    dbg_print(NOTICE, "conn_idx %u waiting for user to input oob ......\r\n", p_ind->conn_idx);
}

/*!
    \brief      Callback function to handle @ref BLE_SEC_EVT_NUMERIC_COMPARISON_IND event
    \param[in]  p_ind: pointer to the numeric comparison indication data
    \param[out] none
    \retval     none
*/
static void app_nc_hdlr(ble_gap_nc_ind_t *p_ind)
{
    ble_device_t *p_device = dm_find_dev_by_conidx(p_ind->conn_idx);

    if (p_device == NULL) {
        dbg_print(NOTICE, "app_nc_hdlr can't find device !\r\n");
        ble_sec_nc_cfm(p_ind->conn_idx, false);
        return;
    }

    dbg_print(NOTICE, "conn_idx %u num val: %d\r\n", p_ind->conn_idx, p_ind->numeric_value);
    dbg_print(NOTICE, "waiting for user to compare......\r\n");

    if (app_sec_cb.key_cfm_req) {
        app_sec_cb.key_cfm_req(p_ind->conn_idx, p_ind->numeric_value);
    }
}

/*!
    \brief      Callback function to handle @ref BLE_SEC_EVT_IRK_REQ_IND event
    \param[in]  p_ind: pointer to the IRK request indication data
    \param[out] none
    \retval     none
*/
static void app_irk_req_hdlr(ble_gap_irk_req_ind_t *p_ind)
{
    uint8_t i;
    ble_gap_irk_t irk;
    ble_device_t *p_device = dm_find_dev_by_conidx(p_ind->conn_idx);

    if (p_device == NULL) {
        dbg_print(NOTICE, "app_irk_req_hdlr can't find device !\r\n");
        ble_sec_irk_req_cfm(p_ind->conn_idx, false, NULL);
        return;
    }

    ble_adp_loc_irk_get(irk.irk);
    ble_adp_identity_addr_get(&irk.identity);

    dbg_print(NOTICE, "conn_idx %u bond irk request:", p_ind->conn_idx);

    for (i = 0; i < BLE_GAP_KEY_LEN; i++) {
        dbg_print(NOTICE, " %02x", irk.irk[i]);
    }

    dbg_print(NOTICE, "\r\n");

    dbg_print(NOTICE, "identity addr %02X:%02X:%02X:%02X:%02X:%02X \r\n",
           irk.identity.addr[5], irk.identity.addr[4], irk.identity.addr[3],
           irk.identity.addr[2], irk.identity.addr[1], irk.identity.addr[0]);

    ble_sec_irk_req_cfm(p_ind->conn_idx, true, &irk);
}

/*!
    \brief      Callback function to handle @ref BLE_SEC_EVT_CSRK_REQ_IND event
    \param[in]  p_ind: pointer to the CSRK request indication data
    \param[out] none
    \retval     none
*/
static void app_csrk_req_hdlr(ble_gap_csrk_req_ind_t *p_ind)
{
    uint8_t i;
    ble_gap_csrk_t csrk;
    ble_device_t *p_device = dm_find_dev_by_conidx(p_ind->conn_idx);

    sys_random_bytes_get(csrk.csrk, BLE_GAP_KEY_LEN);

    if (p_device == NULL) {
        dbg_print(NOTICE, "app_csrk_req_hdlr can't find device !\r\n");
        ble_sec_csrk_req_cfm(p_ind->conn_idx, false, NULL);
        return;
    }

    p_device->bond_info.key_msk |= BLE_LOC_CSRK;
    memcpy(p_device->bond_info.local_csrk.csrk, csrk.csrk, BLE_GAP_KEY_LEN);

    dbg_print(NOTICE, "conn_idx %u bond csrk request:", p_ind->conn_idx);

    for (i = 0; i < BLE_GAP_KEY_LEN; i++) {
        dbg_print(NOTICE, " %02x", csrk.csrk[i]);
    }

    dbg_print(NOTICE, "\r\n");

    ble_sec_csrk_req_cfm(p_ind->conn_idx, true, &csrk);
}

/*!
    \brief      Callback function to handle @ref BLE_SEC_EVT_OOB_DATA_REQ_IND event
    \param[in]  p_ind: pointer to the OOB data request indication data
    \param[out] none
    \retval     none
*/
static void app_oob_data_req_hdlr(ble_gap_oob_data_req_ind_t *p_ind)
{
    ble_device_t *p_device = dm_find_dev_by_conidx(p_ind->conn_idx);

    if (p_device == NULL) {
        dbg_print(NOTICE, "app_oob_data_req_hdlr can't find device !\r\n");
        ble_sec_oob_data_req_cfm(p_ind->conn_idx, false, NULL, NULL);
        return;
    }

    ble_sec_oob_data_req_cfm(p_ind->conn_idx, true, app_sec_env.oob_data.conf, app_sec_env.oob_data.rand);
}

/*!
    \brief      Callback function to handle @ref BLE_SEC_EVT_PAIRING_SUCCESS_INFO event
    \param[in]  p_info: pointer to the pairing success information data
    \param[out] none
    \retval     none
*/
static void app_pairing_success_hdlr(ble_sec_pairing_success_t *p_info)
{
    uint8_t i;
    ble_device_t *p_device = dm_find_dev_by_conidx(p_info->conidx);

    if (p_device == NULL) {
        dbg_print(NOTICE, "app_pairing_success_hdlr can't find device !\r\n");
        return;
    }

    dbg_print(NOTICE, "conn_idx %u pairing success, level 0x%x ltk_present %d sc %d\r\n", p_info->conidx,
           p_info->bond_info.pairing_lvl, p_info->bond_info.enc_key_present, p_info->sc);

    p_device->bonded = (p_info->bond_info.pairing_lvl & BLE_GAP_PAIRING_BOND_PRESENT_BIT) != 0 ? true : false;
    p_device->bond_info = p_info->bond_info;

    if (p_device->bond_info.key_msk & BLE_LOC_LTK_ENCKEY) {
        dbg_print(NOTICE, "local key size %d, ltk(hex): ", p_device->bond_info.local_ltk.key_size);

        for (i = 0; i < BLE_GAP_KEY_LEN; i++) {
            dbg_print(NOTICE, "%02x", p_device->bond_info.local_ltk.ltk[i]);
        }
        dbg_print(NOTICE, "\r\n");
    }

    if (p_device->bond_info.key_msk & BLE_PEER_LTK_ENCKEY) {
        dbg_print(NOTICE, "peer key size %d, ltk(hex): ", p_device->bond_info.peer_ltk.key_size);

        for (i = 0; i < BLE_GAP_KEY_LEN; i++) {
            dbg_print(NOTICE, "%02x", p_device->bond_info.peer_ltk.ltk[i]);
        }
        dbg_print(NOTICE, "\r\n");
    }

    if (p_device->bond_info.key_msk & BLE_PEER_IDKEY) {
        dbg_print(NOTICE, "peer irk(hex): ");

        for (i = 0; i < BLE_GAP_KEY_LEN; i++) {
            dbg_print(NOTICE, "%02x", p_device->bond_info.peer_irk.irk[i]);
        }
        dbg_print(NOTICE, "\r\n");
    }

    if (p_device->bond_info.key_msk & BLE_LOC_CSRK) {
        dbg_print(NOTICE, "local csrk(hex): ");

        for (i = 0; i < BLE_GAP_KEY_LEN; i++) {
            dbg_print(NOTICE, "%02x", p_device->bond_info.local_csrk.csrk[i]);
        }
        dbg_print(NOTICE, "\r\n");
    }

    if (p_device->bond_info.key_msk & BLE_PEER_CSRK) {
        dbg_print(NOTICE, "peer csrk(hex): ");

        for (i = 0; i < BLE_GAP_KEY_LEN; i++) {
            dbg_print(NOTICE, "%02x", p_device->bond_info.peer_csrk.csrk[i]);
        }
        dbg_print(NOTICE, "\r\n");
    }

    // If application manager keys, need to store bond data.
    if (app_sec_mgr_key && (p_device->bond_info.pairing_lvl & BLE_GAP_PAIRING_BOND_PRESENT_BIT)) {
        // If has identity address use identity address as store key
        if (p_device->bond_info.key_msk & BLE_PEER_IDKEY) {
            ble_peer_data_bond_store(&p_device->bond_info.peer_irk.identity, &p_device->bond_info);
        } else {
            ble_peer_data_bond_store(&p_device->cur_addr, &p_device->bond_info);
        }
    }

    if (pairing_cb.state == BLE_BOND_STATE_BONDING &&
        memcmp(&pairing_cb.addr, &p_device->cur_addr, sizeof(ble_gap_addr_t)) == 0) {
        memset(&pairing_cb.addr, 0, sizeof(ble_gap_addr_t));
        pairing_cb.state = BLE_BOND_STATE_NONE;
    }

    if (app_sec_cb.authen_cmpl) {
        app_sec_cb.authen_cmpl(p_info->conidx, BLE_ERR_NO_ERROR);
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
    ble_device_t *p_device = dm_find_dev_by_conidx(p_info->param.conn_idx);

    if (p_device == NULL) {
        dbg_print(NOTICE, "app_pairing_fail_hdlr can't find device !\r\n");
        return;
    }

    if (pairing_cb.state == BLE_BOND_STATE_BONDING &&
        memcmp(&pairing_cb.addr, &p_device->cur_addr, sizeof(ble_gap_addr_t)) == 0) {
        memset(&pairing_cb, 0, sizeof(pairing_cb));
    }

    dbg_print(NOTICE, "pairing fail reason 0x%x\r\n", p_info->param.reason);

    if (app_sec_cb.authen_cmpl) {
        app_sec_cb.authen_cmpl(p_info->param.conn_idx, p_info->param.reason);
    }
}

/*!
    \brief      Callback function to handle @ref BLE_SEC_EVT_SECURITY_REQ_INFO event
    \param[in]  p_info: pointer to the security request information data
    \param[out] none
    \retval     none
*/
void app_security_req_info_hdlr(ble_sec_security_req_info_t *p_info)
{
    ble_device_t *p_device = dm_find_dev_by_conidx(p_info->param.conn_idx);

    if (p_device == NULL) {
        dbg_print(NOTICE, "app_security_req_info_hdlr can't find device !\r\n");
        return;
    }

    if (p_device->bonded) {
        ble_sec_encrypt_req(p_info->param.conn_idx, &p_device->bond_info.peer_ltk);
    } else {
        app_sec_send_bond_req(p_info->param.conn_idx);
    }
}

/*!
    \brief      Callback function to handle @ref BLE_SEC_EVT_ENCRYPT_REQ_IND event
    \param[in]  p_ind: pointer to the encryption request indication data
    \param[out] none
    \retval     none
*/
static void app_encrypt_req_hdlr(ble_gap_encrypt_req_ind_t *p_ind)
{
    ble_device_t *p_device = dm_find_dev_by_conidx(p_ind->conn_idx);

    if (p_device == NULL) {
        dbg_print(NOTICE, "app_encrypt_req_hdlr can't find device !\r\n");
        ble_sec_encrypt_req_cfm(p_ind->conn_idx, false, NULL, 0);
        return;
    }

    if (p_device->bonded) {
        if ((p_ind->ediv == p_device->bond_info.local_ltk.ediv) &&
            !memcmp(p_ind->rnd_num, p_device->bond_info.local_ltk.rnd_num, BLE_GAP_RANDOM_NUMBER_LEN)) {
            ble_sec_encrypt_req_cfm(p_ind->conn_idx, true, p_device->bond_info.local_ltk.ltk,
                                    p_device->bond_info.local_ltk.key_size);
            return;
        }
    }

    ble_sec_encrypt_req_cfm(p_ind->conn_idx, false, NULL, 0);
}

/*!
    \brief      Callback function to handle @ref BLE_SEC_EVT_ENCRYPT_INFO event
    \param[in]  p_info: pointer to the encryption result information data
    \param[out] none
    \retval     none
*/
static void app_encrypted_hdlr(ble_sec_encrypt_info_t *p_info)
{
    ble_device_t *p_device = dm_find_dev_by_conidx(p_info->param.conn_idx);

    if (p_device == NULL) {
        dbg_print(NOTICE, "app_encrypted_hdlr can't find device !\r\n");
        return;
    }

    if (p_info->status != BLE_ERR_NO_ERROR) {
        dbg_print(NOTICE, "conn_idx %u encrypt fail, status 0x%x\r\n", p_device->conn_idx, p_info->status);
        p_device->encry_cmplt = false;

        // Key missing just remove keys
        if (p_info->status == BLE_SMP_ERR_ENC_KEY_MISSING) {
            memset(&p_device->bond_info, 0, sizeof(ble_gap_sec_bond_data_t));
            p_device->bonded = false;

            if (app_sec_mgr_key) {
                ble_peer_data_delete(&p_device->cur_addr);
            }
        }
    } else {
        dbg_print(NOTICE, "conn_idx %u encrypt success, pairing_lvl 0x%x\r\n", p_device->conn_idx, p_info->param.pairing_lvl);
        p_device->encry_cmplt = true;
        p_device->bond_info.pairing_lvl = p_info->param.pairing_lvl;

        #if (BLE_APP_PING_SUPPORT)
        ble_conn_ping_to_set(p_info->param.conn_idx, 1000); // 10 Sec
        #endif // (BLE_APP_PING_SUPPORT)

        if (p_device->role == BLE_SLAVE) {
            ble_conn_peer_version_get(p_device->conn_idx);
            ble_conn_peer_feats_get(p_device->conn_idx);
        }
    }
}

/*!
    \brief      Callback function to handle @ref BLE_SEC_EVT_OOB_DATA_GEN_INFO event
    \param[in]  p_info: pointer to the generated OOB data information data
    \param[out] none
    \retval     none
*/
static void app_oob_data_hdlr(ble_sec_oob_data_info_t *p_info)
{
    int i;

    dbg_print(NOTICE, "le oob data ind, conf:");
    for (i = BLE_GAP_KEY_LEN - 1; i >= 0; i--) {
        dbg_print(NOTICE, "%02x", p_info->param.conf[i]);
    }

    dbg_print(NOTICE, ", random:");
    for (i = BLE_GAP_KEY_LEN - 1; i >= 0; i--) {
        dbg_print(NOTICE, "%02x", p_info->param.rand[i]);
    }

    dbg_print(NOTICE, "\r\n");
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

    case BLE_SEC_EVT_LTK_REQ_IND:
        app_ltk_req_hdlr((ble_gap_ltk_req_ind_t *)p_data);
        break;

    case BLE_SEC_EVT_KEY_DISPLAY_REQ_IND:
        app_key_display_req_hdlr((ble_gap_tk_req_ind_t *)p_data);
        break;

    case BLE_SEC_EVT_KEY_ENTER_REQ_IND:
        app_key_enter_req_hdlr((ble_gap_tk_req_ind_t *)p_data);
        break;

    case BLE_SEC_EVT_KEY_OOB_REQ_IND:
        app_key_oob_req_hdlr((ble_gap_tk_req_ind_t *)p_data);
        break;

    case BLE_SEC_EVT_NUMERIC_COMPARISON_IND:
        app_nc_hdlr((ble_gap_nc_ind_t *)p_data);
        break;

    case BLE_SEC_EVT_IRK_REQ_IND:
        app_irk_req_hdlr((ble_gap_irk_req_ind_t *)p_data);
        break;

    case BLE_SEC_EVT_CSRK_REQ_IND:
        app_csrk_req_hdlr((ble_gap_csrk_req_ind_t *)p_data);
        break;

    case BLE_SEC_EVT_OOB_DATA_REQ_IND:
        app_oob_data_req_hdlr((ble_gap_oob_data_req_ind_t *)p_data);
        break;

    case BLE_SEC_EVT_PAIRING_SUCCESS_INFO:
        app_pairing_success_hdlr((ble_sec_pairing_success_t *)p_data);
        break;

    case BLE_SEC_EVT_PAIRING_FAIL_INFO:
        app_pairing_fail_hdlr((ble_sec_pairing_fail_t *)p_data);
        break;

    case BLE_SEC_EVT_SECURITY_REQ_INFO:
        app_security_req_info_hdlr((ble_sec_security_req_info_t *)p_data);
        break;

    case BLE_SEC_EVT_ENCRYPT_REQ_IND:
        app_encrypt_req_hdlr((ble_gap_encrypt_req_ind_t *)p_data);
        break;

    case BLE_SEC_EVT_ENCRYPT_INFO:
        app_encrypted_hdlr((ble_sec_encrypt_info_t *)p_data);
        break;

    case BLE_SEC_EVT_OOB_DATA_GEN_INFO:
        app_oob_data_hdlr((ble_sec_oob_data_info_t *)p_data);
        break;

    case BLE_SEC_EVT_KEY_PRESS_INFO:
        dbg_print(NOTICE, "conidx %u key press info type %d\r\n", p_data->key_press_info.conn_idx,
               p_data->key_press_info.type);
        break;
    default:
        break;
    }
}

/*!
    \brief      Reset application security module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_sec_mgr_reset(void)
{
    memset(&app_sec_env, 0, sizeof(app_sec_env_t));
    app_sec_env.authen_bond = true;
    app_sec_env.key_size = 16;
    app_sec_env.io_capability = BLE_GAP_IO_CAP_NO_IO;
    app_sec_env.pin_code = APP_SEC_INVALID_PIN_CODE;
    memset(&pairing_cb, 0, sizeof(pairing_cb));
    memset(&app_sec_cb, 0 , sizeof(app_sec_cb));
}

/*!
    \brief      Init application security module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_sec_mgr_init(void)
{
    app_sec_mgr_reset();
    ble_sec_callback_register(app_sec_evt_handler);
}

/*!
    \brief      Deinit application security module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_sec_mgr_deinit(void)
{
    ble_sec_callback_unregister(app_sec_evt_handler);
}

/*!
    \brief      Set authentication parameters
    \param[in]  bond: true to support bond authentication, otherwise false
    \param[in]  mitm: true to support man-in-the-middle protection, otherwise false
    \param[in]  sc: true to support secure connection, otherwise false
    \param[in]  iocap: IO capabilities. @ref ble_gap_io_cap_t
    \param[in]  oob: true to support OOB, otherwise false
    \param[in]  sc_only: true to support secure connection only, otherwise false
    \param[in]  key_size: encrytion key size
    \param[out] none
    \retval     none
*/
void app_sec_set_authen(bool bond, bool mitm, bool sc, uint8_t iocap, bool oob, bool sc_only, uint8_t key_size)
{
    app_sec_env.authen_bond = bond;
    app_sec_env.authen_mitm = mitm;
    app_sec_env.authen_sc = sc;
    app_sec_env.sc_only = sc_only;
    app_sec_env.io_capability = iocap;
    app_sec_env.oob = oob;
    app_sec_env.key_size = key_size;
}

/*!
    \brief      Check if bond is needed
    \param[in]  none
    \param[out] none
    \retval     bool: true if bond is needed, otherwise false
*/
bool app_sec_need_authen_bond(void)
{
    return app_sec_env.authen_bond;
}

/*!
    \brief      Check if a device is the one under pairing
    \param[in]  address: peer device address
    \param[out] none
    \retval     bool: true if the device is the pairing one, otherwise false
*/
bool app_sec_is_pairing_device(ble_gap_addr_t address)
{
    return (pairing_cb.state == BLE_BOND_STATE_BONDING &&
            memcmp(&pairing_cb.addr, &address, sizeof(ble_gap_addr_t)) == 0);
}

/*!
    \brief      Cancel the ongoing pairing procedure
    \param[in]  none
    \param[out] none
    \retval     bool: true if pairing can be cancelled, otherwise false
*/
bool app_sec_cancel_bonding(void)
{
    ble_device_t *p_dev;
    if (pairing_cb.state != BLE_BOND_STATE_BONDING) {
        return false;
    }

    p_dev = dm_find_dev_by_addr(pairing_cb.addr);
    if (p_dev == NULL || p_dev->state != BLE_CONN_STATE_CONNECTED) {
        ble_conn_connect_cancel();
    }

    memset(&pairing_cb, 0, sizeof(pairing_cb));
    return true;
}

/*!
    \brief      Initiate pairing procedure
    \param[in]  address: peer device address
    \param[out] none
    \retval     bool: true if pairing is initiated successfully, otherwise false
*/
bool app_sec_create_bond(ble_gap_addr_t address)
{
    ble_device_t *p_dev = dm_find_alloc_dev_by_addr(address);

    if (p_dev == NULL) {
        return false;
    }

    if (pairing_cb.state != BLE_BOND_STATE_NONE || p_dev->bonded) {
        return false;
    }

    if (ble_conn_connect(NULL, BLE_GAP_LOCAL_ADDR_STATIC, &address, false) != BLE_ERR_NO_ERROR) {
        return false;
    }

    pairing_cb.addr = address;
    pairing_cb.is_local_initiated = true;
    pairing_cb.state = BLE_BOND_STATE_BONDING;

    return true;
}

/*!
    \brief      Remove bond information
    \param[in]  address: peer device address
    \param[out] none
    \retval     bool: true if bond information can be removed, otherwise false
*/
bool app_sec_remove_bond(ble_gap_addr_t address)
{
    ble_device_t *p_dev = dm_find_alloc_dev_by_addr(address);

    if (p_dev == NULL) {
        return false;
    }

    if (p_dev->bonded) {
        if (p_dev->state == BLE_CONN_STATE_CONNECTED) {
            p_dev->pending_remove = true;
            ble_conn_disconnect(p_dev->conn_idx, BLE_ERROR_HL_TO_HCI(BLE_LL_ERR_REMOTE_USER_TERM_CON));
        } else {
            dm_remove_dev_by_addr(p_dev->cur_addr);
            ble_peer_data_delete(&p_dev->cur_addr);
        }
    } else {
        dbg_print(NOTICE, "device connect but no bond!\r\n");
        return false;
    }

    return true;
}

/*!
    \brief      Send security request
    \param[in]  conidx: connection index
    \param[out] none
    \retval     none
*/
void app_sec_send_security_req(uint8_t conidx)
{
    uint8_t auth = BLE_GAP_AUTH_REQ_NO_MITM_NO_BOND;

    if (app_sec_env.authen_bond) {
        auth |= BLE_GAP_AUTH_MASK_BOND;
    }

    if (app_sec_env.authen_mitm) {
        auth |= BLE_GAP_AUTH_MASK_MITM;
    }

    if (app_sec_env.authen_sc) {
        auth |= BLE_GAP_AUTH_MASK_SEC_CON;
    }

    if (ble_sec_security_req(conidx, auth) != BLE_ERR_NO_ERROR) {
        dbg_print(NOTICE, "app_sec_send_security_req fail! \r\n");
    }
}

/*!
    \brief      Send pairing request
    \param[in]  conidx: connection index
    \param[out] none
    \retval     none
*/
void app_sec_send_bond_req(uint8_t conidx)
{
    ble_gap_pairing_param_t param;
    uint8_t sec_req_level = BLE_GAP_NO_SEC;

    param.auth = BLE_GAP_AUTH_REQ_NO_MITM_NO_BOND;

    if (app_sec_env.authen_bond) {
        param.auth |= BLE_GAP_AUTH_MASK_BOND;
    }

    if (app_sec_env.authen_mitm) {
        param.auth |= BLE_GAP_AUTH_MASK_MITM;
    }

    if (app_sec_env.authen_sc) {
        param.auth |= BLE_GAP_AUTH_MASK_SEC_CON;
    }

    param.iocap     = app_sec_env.io_capability;
    param.key_size  = app_sec_env.key_size;
    param.oob       = app_sec_env.oob;

    if (app_sec_env.authen_sc) {
        param.ikey_dist = BLE_GAP_KDIST_IDKEY | BLE_GAP_KDIST_SIGNKEY;
        param.rkey_dist = BLE_GAP_KDIST_IDKEY | BLE_GAP_KDIST_SIGNKEY;
    } else {
        param.ikey_dist = BLE_GAP_KDIST_IDKEY | BLE_GAP_KDIST_SIGNKEY | BLE_GAP_KDIST_ENCKEY;
        param.rkey_dist = BLE_GAP_KDIST_IDKEY | BLE_GAP_KDIST_SIGNKEY | BLE_GAP_KDIST_ENCKEY;
    }

    if (app_sec_env.sc_only) {
        sec_req_level = BLE_GAP_SEC1_SEC_CON_PAIR_ENC;
    }

    if (ble_sec_bond_req(conidx, &param, sec_req_level) != BLE_ERR_NO_ERROR) {
        dbg_print(NOTICE, "app_sec_send_bond_req fail! \r\n");
    }
}

/*!
    \brief      Send encryption request
    \param[in]  conidx: connection index
    \param[out] none
    \retval     none
*/
void app_sec_send_encrypt_req(uint8_t conidx)
{
    ble_device_t *p_device = dm_find_dev_by_conidx(conidx);

    if (p_device == NULL) {
        dbg_print(NOTICE, "app_sec_send_encrypt_req can't find device !\r\n");
        return;
    }

    if (!p_device->bonded) {
        dbg_print(NOTICE, "app_sec_send_encrypt_req no bonded !\r\n");
        return;
    }

    if ((p_device->bond_info.key_msk & BLE_PEER_LTK_ENCKEY) == 0) {
        dbg_print(NOTICE, "app_sec_send_encrypt_req no ltk !\r\n");
        return;
    }

    if (ble_sec_encrypt_req(conidx, &p_device->bond_info.peer_ltk) != BLE_ERR_NO_ERROR) {
        dbg_print(NOTICE, "app_sec_send_encrypt_req fail !\r\n");
    }
}

/*!
    \brief      Input passkey for pairing
    \param[in]  conidx: connection index
    \param[in]  passkey: passkey value
    \param[out] none
    \retval     none
*/
void app_sec_input_passkey(uint8_t conidx, uint32_t passkey)
{
    ble_device_t *p_device = dm_find_dev_by_conidx(conidx);

    if (p_device == NULL) {
        dbg_print(NOTICE, "app_sec_input_passkey can't find device !\r\n");
        ble_sec_key_display_enter_cfm(conidx, false, passkey);
        return;
    }

    dbg_print(NOTICE, "input passkey: %d\r\n", passkey);

    ble_sec_key_display_enter_cfm(conidx, true, passkey);
}

/*!
    \brief      Input OOB key for TK
    \param[in]  conidx: connection index
    \param[in]  p_oob: pointer to the OOB key data
    \param[out] none
    \retval     none
*/
void app_sec_input_oob(uint8_t conidx, uint8_t *p_oob)
{
    ble_device_t *p_device = dm_find_dev_by_conidx(conidx);

    if (p_device == NULL) {
        dbg_print(NOTICE, "app_sec_input_oob can't find device !\r\n");
        ble_sec_oob_req_cfm(conidx, false, p_oob);
        return;
    }

    ble_sec_oob_req_cfm(conidx, true, p_oob);
}

/*!
    \brief      Set numeric comparison result
    \param[in]  conidx: connection index
    \param[in]  accept: true to accept the numeric comparison, otherwisw false
    \param[out] none
    \retval     none
*/
void app_sec_num_compare(uint8_t conidx, bool accept)
{
    ble_device_t *p_device = dm_find_dev_by_conidx(conidx);

    if (p_device == NULL) {
        dbg_print(NOTICE, "app_sec_num_compare can't find device !\r\n");
        ble_sec_nc_cfm(conidx, false);
        return;
    }

    dbg_print(NOTICE, "compare result: %d\r\n", accept);

    ble_sec_nc_cfm(conidx, accept);
}

/*!
    \brief      Set OOB data
    \param[in]  p_conf: pointer to confirm value
    \param[in]  p_rand: pointer to random number
    \param[out] none
    \retval     none
*/
void app_set_oob_data(uint8_t *p_conf, uint8_t *p_rand)
{
    memcpy(app_sec_env.oob_data.conf, p_conf, BLE_GAP_KEY_LEN);
    memcpy(app_sec_env.oob_data.rand, p_rand, BLE_GAP_KEY_LEN);
}

/*!
    \brief      Generate OOB data
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_gen_oob_data(void)
{
    ble_sec_oob_data_gen();
}

/*!
    \brief      Get if security keys are managered by application
    \param[in]  none
    \param[out] none
    \retval     bool: true if security keys are managered by application, otherwise false
*/
bool app_sec_user_key_mgr_get(void)
{
    return app_sec_mgr_key;
}

bool app_sec_pin_code_set(uint32_t pin_code)
{
    if (pin_code <= 999999) {
        app_sec_env.pin_code = pin_code;
        return true;
    }

    return false;
}

uint32_t app_sec_pin_code_get(void)
{
    return app_sec_env.pin_code;
}

bool app_sec_callbacks_set(app_sec_callbacks cb)
{
    app_sec_cb = cb;

    return true;
}


#endif //(BLE_APP_SUPPORT && (BLE_CFG_ROLE & (BLE_CFG_ROLE_PERIPHERAL | BLE_CFG_ROLE_CENTRAL)))
