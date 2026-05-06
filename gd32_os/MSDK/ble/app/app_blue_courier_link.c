/*!
    \file    app_blue_courier_link.c
    \brief   Implemetions of blue courier link

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
#include "app_blue_courier_link.h"

#if (BLE_PROFILE_BLUE_COURIER_SERVER)
#include "app_blue_courier.h"
#include "ble_gatts.h"
#include "ble_adapter.h"
#include "ble_adv.h"
#include "ble_conn.h"
#include "wrapper_os.h"
#include "dbg_print.h"
#include "co_math.h"

bcwl_env_t bcwl_env = {0};
/* Profile id. blue courier wifi profile identity */
uint8_t    prf_id;

static bool ble_enabled = false;
static bool bcwl_enable_pending = false;

/* Blue courier wifi profile attribute database */
const ble_gatt_attr_desc_t bcw_att_db[BCW_IDX_NUMBER] = {
    [BCW_IDX_PRIM_SVC]   = { UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_PRIMARY_SERVICE), PROP(RD),             0                                 },
    [BCW_IDX_CHAR_WRITE] = { UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD),             0                                 },
    [BCW_IDX_WRITE]      = { UUID_16BIT_TO_ARRAY(BCW_GATT_WRITE_UUID),           PROP(WR) | SEC_LVL(WP, UNAUTH), BCW_VALUE_LEN           },
    [BCW_IDX_CHAR_NTF]   = { UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),  PROP(RD),             0                                 },
    [BCW_IDX_NTF]        = { UUID_16BIT_TO_ARRAY(BCW_GATT_NTF_UUID),             PROP(NTF),            BCW_VALUE_LEN                     },
    [BCW_IDX_NTF_CFG]    = { UUID_16BIT_TO_ARRAY(BLE_GATT_DESC_CLIENT_CHAR_CFG), PROP(RD) | PROP(WR),  OPT(NO_OFFSET) | sizeof(uint16_t) },
};

/*!
    \brief      Blue courier wifi link report error message to peer device
    \param[in]  reason: error code
    \param[out] none
    \retval     none
*/
void bcwl_error_report(uint8_t reason)
{
    bcwl_send(BCWL_OPCODE_BUILD(BCWL_OPCODE_TYPE_MGMT, BCWL_OPCODE_MGMT_SUBTYPE_ERROR_REPORT), &reason, sizeof(uint8_t));
}

/*!
    \brief      Blue courier wifi link send ack message to peer device
    \param[in]  seq: sequence number
    \param[out] none
    \retval     none
*/
void bcwl_send_ack(uint8_t seq)
{
    bcwl_send(BCWL_OPCODE_BUILD(BCWL_OPCODE_TYPE_MGMT, BCWL_OPCODE_MGMT_SUBTYPE_ACK), &seq, sizeof(uint8_t));
}

/*!
    \brief      Handle handshake message and send response message
    \param[in]  data: pointer to handshake information, @ref bcwl_mgmt_handshake_t
    \param[in]  len: data length
    \param[out] none
    \retval     none
*/
void bcwl_handle_mgmt_handshake(uint8_t *data, uint16_t len)
{
    uint16_t mtu;
    bcwl_mgmt_handshake_t handshake_rsp;
    bcwl_mgmt_handshake_t *handshake = (bcwl_mgmt_handshake_t *)data;

    if (len != sizeof(bcwl_mgmt_handshake_t)) {
        dbg_print(ERR, "%s len err %d\n", __func__, len);
        bcwl_error_report(BCWL_ERR_PACKET_LEN_ERROR);
        return;
    }

    ble_gatts_mtu_get(bcwl_env.conn_id, &mtu);
    handshake_rsp.mtu = co_min(co_min(handshake->mtu, mtu), BCW_FRAG_MAX_LEN);
    handshake_rsp.recv_size = BCW_VALUE_LEN;
    bcwl_env.peer_recv_size = co_min(handshake->recv_size, BCW_VALUE_LEN);
    bcwl_env.frag_size = handshake_rsp.mtu - sizeof(bcwl_header_t) - BLE_GATT_HEADER_LEN - 2/*crc */;
    bcwl_env.handshake_success = true;

    dbg_print(NOTICE, "%s handshake success\n", __func__);

    bcwl_send(BCWL_OPCODE_BUILD(BCWL_OPCODE_TYPE_MGMT, BCWL_OPCODE_MGMT_SUBTYPE_HANDSHAKE),
        (uint8_t *)&handshake_rsp, sizeof(bcwl_mgmt_handshake_t));
}

/*!
    \brief      Blue courier wifi link message handler
    \param[in]  opcode: opcode of type and subtype
    \param[in]  data: pointer to received data
    \param[in]  len: received data length
    \param[out] none
    \retval     none
*/
void bcwl_msg_handler(uint8_t opcode, uint8_t *data, uint16_t len)
{
    switch (BCWL_OPCODE_GET_TYPE(opcode)) {
    case BCWL_OPCODE_TYPE_MGMT:
        switch (BCWL_OPCODE_GET_SUBTYPE(opcode)) {
        case BCWL_OPCODE_MGMT_SUBTYPE_HANDSHAKE:
            bcwl_handle_mgmt_handshake(data, len);
            break;
        case BCWL_OPCODE_MGMT_SUBTYPE_ACK:
        case BCWL_OPCODE_MGMT_SUBTYPE_ERROR_REPORT:
            /* TODO */
            break;
        default:
            dbg_print(ERR, "%s unknown opcode %x\n", __func__, opcode);
            bcwl_error_report(BCWL_ERR_UNKNOWN_OPCODE);
            break;
        }
        break;
    case BCWL_OPCODE_TYPE_DATA:
        bcwp_msg_handler(BCWL_OPCODE_GET_SUBTYPE(opcode), data, len);
        break;
    default:
        dbg_print(ERR, "%s unknown opcode %x\n", __func__, opcode);
        bcwl_error_report(BCWL_ERR_UNKNOWN_OPCODE);
    }
}

/*!
    \brief      Blue courier wifi link send notify through GATT
    \param[in]  p_val: pointer to notification value to send
    \param[in]  len: notification value length
    \param[out] none
    \retval     none
*/
static void bcwl_ntf_event_send(uint8_t *p_val, uint16_t len)
{
    if (bcwl_env.ntf_cfg == 0) {
        dbg_print(ERR, "%s fail\r\n", __func__);
        return;
    }

    ble_gatts_ntf_ind_send(bcwl_env.conn_id, prf_id, BCW_IDX_NTF, p_val, len, BLE_GATT_NOTIFY);
}

/*!
    \brief      Blue courier wifi link send message to peer device
    \param[in]  opcode: opcode
    \param[in]  data: pointer to message data
    \param[in]  len: message length
    \param[out] none
    \retval     none
*/
void bcwl_send(uint8_t opcode, uint8_t *data, uint16_t len)
{
    uint16_t crc;
    bcwl_header_t *hdr = NULL;
    uint16_t remain_len = len;

    if (len > bcwl_env.peer_recv_size) {
        dbg_print(ERR, "%s send len exceed the maximum, %d\n", __func__, len);
        return;
    }

    while (remain_len > 0) {
        if (remain_len > bcwl_env.frag_size) {
            hdr = sys_malloc(sizeof(bcwl_header_t) + bcwl_env.frag_size + 2);
            if (hdr == NULL) {
                dbg_print(ERR, "%s no mem\n", __func__);
                bcwl_error_report(BCWL_ERR_SEND_NO_MEM);
                return;
            }

            hdr->flag = 0;
            hdr->data_len = bcwl_env.frag_size;
            if (remain_len == len) {
                /* start segment */
                hdr->flag |= BCWL_FLAG_BEGIN_MASK;
                hdr->data[0] = remain_len & 0xff;
                hdr->data[1] = (remain_len >> 8) & 0xff;
                sys_memcpy(hdr->data + 2, &data[len - remain_len], hdr->data_len - 2); //copy first
                remain_len -= (hdr->data_len - 2);
            } else {
                /* continue segment */
                sys_memcpy(hdr->data, &data[len - remain_len], hdr->data_len); //copy continue
                remain_len -= hdr->data_len;
            }
        } else {
            /* end or complete segment */
            hdr = sys_malloc(sizeof(bcwl_header_t) + remain_len + 2);
            if (hdr == NULL) {
                dbg_print(ERR, "%s no mem\n", __func__);
                bcwl_error_report(BCWL_ERR_SEND_NO_MEM);
                return;
            }

            hdr->flag = (len <= bcwl_env.frag_size ? BCWL_FLAG_BEGIN_MASK : 0) | BCWL_FLAG_END_MASK;
            hdr->data_len = remain_len;
            sys_memcpy(hdr->data, &data[len - remain_len], hdr->data_len); //copy end
            remain_len -= hdr->data_len;
        }

        hdr->opcode = opcode;
        hdr->seq = bcwl_env.send_seq++;

        crc = co_crc16(&hdr->seq, hdr->data_len + sizeof(bcwl_header_t) - 1, 0);
        hdr->data[hdr->data_len] = crc & 0xff;
        hdr->data[hdr->data_len + 1] = (crc >> 8) & 0xff;

        bcwl_ntf_event_send((uint8_t *)hdr, hdr->data_len + sizeof(bcwl_header_t) + 2);

        sys_mfree(hdr);
        hdr =  NULL;
    }

}

/*!
    \brief      Blue courier wifi link receive message from peer device
    \param[in]  data: pointer to message data
    \param[in]  len: message length
    \param[out] none
    \retval     none
*/
void bcwl_receive(uint8_t *data, uint16_t len)
{
    uint8_t status = BCWL_ERR_RECV_ERROR;
    uint16_t crc, crc_pkt;
    bcwl_header_t *hdr = (bcwl_header_t *)data;

    if (len < sizeof(bcwl_header_t)) {
        dbg_print(ERR, "%s size error %d\n", __func__, len);
        bcwl_error_report(BCWL_ERR_PACKET_LEN_ERROR);
        return;
    }

    if (!bcwl_env.handshake_success &&
        hdr->opcode != BCWL_OPCODE_BUILD(BCWL_OPCODE_TYPE_MGMT, BCWL_OPCODE_MGMT_SUBTYPE_HANDSHAKE)) {
        dbg_print(ERR, "%s not handshake %d\n", __func__, hdr->opcode);
        bcwl_error_report(BCWL_ERR_NO_HANDSHAKE);
        return;
    }

    if (BCWL_FLAG_IS_REQ_ACK(hdr->flag))
        bcwl_send_ack(hdr->seq);

    if (hdr->seq != bcwl_env.recv_seq) {
        dbg_print(ERR, "%s seq %d is not expect %d\n", __func__, hdr->seq, bcwl_env.recv_seq);
        bcwl_error_report(BCWL_ERR_SEQUENCE_ERROR);
        return;
    }

    bcwl_env.recv_seq++;

    crc = co_crc16(&hdr->seq, hdr->data_len + sizeof(bcwl_header_t) - 1, 0);
    crc_pkt = hdr->data[hdr->data_len] | (((uint16_t) hdr->data[hdr->data_len + 1]) << 8);
    if (crc != crc_pkt) {
        status = BCWL_ERR_CRC_CHECK;
        goto err_recv;
    }

    if (BCWL_FLAG_IS_BEGIN(hdr->flag)) {
        if (BCWL_FLAG_IS_END(hdr->flag)) {
            /* receive complete segment */
            bcwl_msg_handler(hdr->opcode, hdr->data, hdr->data_len);
        } else {
            /* receive start segment */
            if (bcwl_env.offset != 0)
                goto err_recv;

            bcwl_env.total_len = hdr->data[0] | (((uint16_t) hdr->data[1]) << 8);
            if (bcwl_env.total_len > BCW_VALUE_LEN)
                goto err_recv;

            bcwl_env.recv_buf = sys_malloc(bcwl_env.total_len);
            if (bcwl_env.recv_buf == NULL) {
                status = BCWL_ERR_RECV_NO_MEM;
                goto err_recv;
            }

            sys_memcpy(bcwl_env.recv_buf + bcwl_env.offset, hdr->data + 2, hdr->data_len - 2);
            bcwl_env.offset = hdr->data_len - 2;
        }
    } else if (BCWL_FLAG_IS_END(hdr->flag)) {
        /* receive end segment */
        if (bcwl_env.offset == 0 || bcwl_env.offset + hdr->data_len != bcwl_env.total_len)
            goto err_recv;

        sys_memcpy(bcwl_env.recv_buf + bcwl_env.offset, hdr->data, hdr->data_len);

        bcwl_msg_handler(hdr->opcode, bcwl_env.recv_buf, bcwl_env.total_len);

        bcwl_env.offset = 0;
        if (bcwl_env.recv_buf != NULL) {
            sys_mfree(bcwl_env.recv_buf);
            bcwl_env.recv_buf = NULL;
        }
    } else {
        /* receive continue segment */
        if (bcwl_env.offset == 0 || bcwl_env.offset + hdr->data_len >= bcwl_env.total_len)
            goto err_recv;

        sys_memcpy(bcwl_env.recv_buf + bcwl_env.offset, hdr->data, hdr->data_len);
        bcwl_env.offset += hdr->data_len;
    }

    return;

err_recv:
    bcwl_env.offset = 0;
    if (bcwl_env.recv_buf != NULL) {
        sys_mfree(bcwl_env.recv_buf);
        bcwl_env.recv_buf = NULL;
    }
    bcwl_error_report(status);
    dbg_print(ERR, "%s error %u\n", __func__, status);
}

/*!
    \brief      Callback function to handle GATT server messages
    \param[in]  p_srv_msg_info: pointer to GATT server message information
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t bcwl_gatts_msg_cb(ble_gatts_msg_info_t *p_srv_msg_info)
{
    uint8_t  att_idx;
    uint8_t *data;
    uint16_t data_len, i;

    if (p_srv_msg_info->srv_msg_type == BLE_SRV_EVT_GATT_OPERATION) {
        /* Other connection operations are ignored */
        if (p_srv_msg_info->msg_data.gatts_op_info.conn_idx != bcwl_env.conn_id) {
            return BLE_ERR_NO_ERROR;
        }

        if (p_srv_msg_info->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_WRITE_REQ) {
            att_idx = p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.write_req.att_idx;
            data = p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.write_req.p_val;
            data_len = p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.write_req.val_len;

            dbg_print(INFO, "%s att idx %d, value_len %d, value: ", __func__, att_idx, data_len);
            for (i = 0; i < data_len; i++)
                dbg_print(INFO, "%02x", data[i]);
            dbg_print(INFO, "\r\n");

            switch (att_idx) {
            case BCW_IDX_WRITE:
                bcwl_receive(data, data_len);
                break;
            case BCW_IDX_NTF_CFG:
                if (data_len == sizeof(uint16_t))
                    bcwl_env.ntf_cfg = *(uint16_t *)data;
                else
                    dbg_print(ERR, "%s ntf cfg invalid length\r\n", __func__);
                break;
            default:
                break;
            }
        }
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Callback function to handle BLE connection events
    \param[in]  event: BLE connection event type
    \param[in]  p_data: pointer to BLE connection event data
    \param[out] none
    \retval     none
*/
void bcwl_conn_evt_handler(ble_conn_evt_t event, ble_conn_data_u *p_data)
{
    if (event == BLE_CONN_EVT_STATE_CHG &&
        p_data->conn_state.state == BLE_CONN_STATE_CONNECTED &&
        p_data->conn_state.info.conn_info.actv_idx == bcwl_env.adv_idx) {
        bcwl_env.conn_id = p_data->conn_state.info.conn_info.conn_idx;
        /* reinit parameter */
        bcwl_env.ntf_cfg = 0;
        bcwl_env.recv_seq = 0;
        bcwl_env.send_seq = 0;
        bcwl_env.total_len = 0;
        bcwl_env.offset = 0;
        bcwl_env.frag_size = BLE_GATT_MTU_MIN - sizeof(bcwl_header_t) - BLE_GATT_HEADER_LEN - 2/*crc */;
        bcwl_env.peer_recv_size = BLE_GATT_MTU_MIN;
        bcwl_env.handshake_success = false;
        if (bcwl_env.recv_buf != NULL) {
            sys_mfree(bcwl_env.recv_buf);
            bcwl_env.recv_buf = NULL;
        }
    }
}

/*!
    \brief      Blue courier wifi link start advertising
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t bcwl_adv_start(void)
{
    ble_data_t adv_data;
    ble_data_t adv_scanrsp_data;
    ble_adv_data_set_t adv;
    ble_adv_data_set_t scan_rsp;
    uint8_t data[BLE_GAP_LEGACY_ADV_MAX_LEN];
    uint8_t index = 0;

    sys_memset(data, 0, BLE_GAP_LEGACY_ADV_MAX_LEN);
    data[index++] = 2;                                       // length
    data[index++] = BLE_AD_TYPE_FLAGS;                       // AD type : flags
    data[index++] = 0x6;                                     // AD value
    data[index++] = 3;                                       // length
    data[index++] = BLE_AD_TYPE_SERVICE_UUID_16_COMPLETE;    // AD type: (Service Data - 16-bit UUID)
    data[index++] = (BCW_GATT_SERVICE_UUID & 0xFF);          // AD value
    data[index++] = ((BCW_GATT_SERVICE_UUID >> 8) & 0xFF);   // AD value

    adv_data.len = index;
    adv_data.p_data = data;
    adv_scanrsp_data.len = index - 3;
    adv_scanrsp_data.p_data = &data[3]; // not include AD type flags
    adv.data_force = true;
    adv.data.p_data_force = &adv_data;
    scan_rsp.data_force = true;
    scan_rsp.data.p_data_force = &adv_scanrsp_data;

    return ble_adv_start(bcwl_env.adv_idx, &adv, &scan_rsp, NULL);
}

/*!
    \brief      Callback function to handle BLE advertising events
    \param[in]  adv_evt: BLE advertising event type
    \param[in]  p_data: pointer to BLE advertising event data
    \param[in]  p_context: context used when create advertising
    \param[out] none
    \retval     none
*/
static void bcwl_adv_mgr_evt_hdlr(ble_adv_evt_t adv_evt, void *p_data, void *p_context)
{
    if (adv_evt == BLE_ADV_EVT_STATE_CHG) {
        ble_adv_state_chg_t *p_chg = (ble_adv_state_chg_t *)p_data;
        ble_adv_state_t old_state = bcwl_env.adv_state;

        dbg_print(NOTICE, "%s state change 0x%x ==> 0x%x, reason 0x%x\r\n", __func__,
            old_state, p_chg->state, p_chg->reason);

        bcwl_env.adv_state = p_chg->state;

        if ((p_chg->state == BLE_ADV_STATE_CREATE) && (old_state == BLE_ADV_STATE_CREATING)) {
            bcwl_env.adv_idx = p_chg->adv_idx;
            bcwl_adv_start();
        } else if ((p_chg->state == BLE_ADV_STATE_CREATE) && (old_state == BLE_ADV_STATE_START)) {
            if (bcwl_env.remove_after_stop) {
                ble_adv_remove(bcwl_env.adv_idx);
                bcwl_env.remove_after_stop = false;
            }
        } else if ((p_chg->reason != BLE_ERR_NO_ERROR) &&  (p_chg->state == BLE_ADV_STATE_IDLE) &&
                   (old_state == BLE_ADV_STATE_CREATING)) {
            ble_conn_callback_unregister(bcwl_conn_evt_handler);
            bcwl_env.mode = 0;
        }
    }
}

/*!
    \brief      Callback function to handle BLE adapter events
    \param[in]  event: BLE adapter event type
    \param[in]  p_data: pointer to BLE adapter event data
    \param[out] none
    \retval     none
*/
static void bcwl_adp_evt_handler(ble_adp_evt_t event, ble_adp_data_u *p_data)
{
    switch (event) {

    case BLE_ADP_EVT_ENABLE_CMPL_INFO:
        if (p_data->adapter_info.status == BLE_ERR_NO_ERROR) {
            ble_enabled = true;
            if (bcwl_enable_pending) {
                bcwl_enable_pending = false;
                bcw_prf_enable(true);
            }
        }
        break;

    case BLE_ADP_EVT_DISABLE_CMPL_INFO:
        if (!p_data->status) {
            ble_enabled = false;
        }
        break;

    default:
        break;
    }
}

/*!
    \brief      Blue courier wifi link create advertising
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t bcwl_adv_create(void)
{
    ble_adv_param_t adv_param = {0};

    adv_param.param.own_addr_type = BLE_GAP_LOCAL_ADDR_STATIC;
    adv_param.param.type = BLE_GAP_ADV_TYPE_LEGACY;
    adv_param.param.prop = BLE_GAP_ADV_PROP_UNDIR_CONN;
    adv_param.param.filter_pol = BLE_GAP_ADV_ALLOW_SCAN_ANY_CON_ANY;
    adv_param.param.disc_mode = BLE_GAP_ADV_MODE_GEN_DISC;
    adv_param.param.ch_map = 0x07; // Advertising channel map - 37, 38, 39
    adv_param.param.primary_phy = BLE_GAP_PHY_1MBPS;
    adv_param.param.adv_intv_min = 160; // Advertising minimum interval - 100ms (160*0.625ms)
    adv_param.param.adv_intv_max = 160; // Advertising maximum interval - 100ms (160*0.625ms)
    adv_param.restart_after_disconn = true;

    return ble_adv_create(&adv_param, bcwl_adv_mgr_evt_hdlr, NULL);
}

/*!
    \brief      Blue courier wifi link stop advertising
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t bcwl_adv_stop(void)
{
    if (bcwl_env.adv_state == BLE_ADV_STATE_START) {
        bcwl_env.remove_after_stop = true;
        return ble_adv_stop(bcwl_env.adv_idx);
    } else if (bcwl_env.adv_state != BLE_ADV_STATE_IDLE) {
        return ble_adv_remove(bcwl_env.adv_idx);
    }

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Enable/disable blue courier wifi service
    \param[in]  enable: 1 to enable and 0 to disable
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t bcw_prf_enable(uint8_t enable)
{
    ble_status_t ret = BLE_ERR_NO_ERROR;

    if (bcwl_env.mode == enable)
        return BLE_ERR_NO_ERROR;

    if (enable) {
        if (ble_enabled == false) {
            bcwl_enable_pending = true;
            return BLE_ERR_NO_ERROR;
        }

        sys_memset(&bcwl_env, 0, sizeof(bcwl_env));

        ble_conn_callback_register(bcwl_conn_evt_handler);

        ret = bcwl_adv_create();
        if (ret != BLE_ERR_NO_ERROR) {
            ble_conn_callback_unregister(bcwl_conn_evt_handler);
            return ret;
        }
    } else {
        ret = bcwl_adv_stop();
        if (ret != BLE_ERR_NO_ERROR)
            return ret;

        ble_conn_callback_unregister(bcwl_conn_evt_handler);
    }

    bcwl_env.mode = enable;
    return ret;
}

/*!
    \brief      Init BLE courier server
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_blue_courier_init(void)
{
#ifdef CFG_WLAN_SUPPORT
    uint8_t bcw_svc_uuid[16] = UUID_16BIT_TO_ARRAY(BCW_GATT_SERVICE_UUID);

    // add blue courier wifi profile
    ble_gatts_svc_add(&prf_id, bcw_svc_uuid, 0, 0, bcw_att_db, BCW_IDX_NUMBER, bcwl_gatts_msg_cb);

    ble_adp_callback_register(bcwl_adp_evt_handler);
#endif
}

/*!
    \brief      Deinit BLE courier server
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_blue_courier_deinit(void)
{
    ble_gatts_svc_rmv(prf_id);

    ble_adp_callback_unregister(bcwl_adp_evt_handler);
}
#else
/*!
    \brief      Enable/disable blue courier wifi service
    \param[in]  enable: 1 to enable and 0 to disable
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t bcw_prf_enable(uint8_t enable)
{
    return BLE_PRF_ERR_FEATURE_NOT_SUPPORTED;
}
#endif // (BLE_APP_SUPPORT)
