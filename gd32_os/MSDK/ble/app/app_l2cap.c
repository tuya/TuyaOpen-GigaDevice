/*!
    \file    app_l2cap.c
    \brief   l2cap Application Module entry point.

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

#if (BLE_APP_SUPPORT)
#include "ble_gap.h"

#include "app_l2cap.h"
#include "ble_l2cap_coc.h"

#include "wrapper_os.h"
#include "dbg_print.h"

/* Max L2CAP channel number per BLE connection */
#define BLE_L2CAP_CHANN_NUM_PER_CONN        10

/* Application L2CAP environment structure */
struct app_l2cap_env_tag
{
    uint8_t nb_chan;            /*!< L2CAP channel number, 0xff to reject connection due to not enough authorization */
    uint8_t new_chan_lid;       /*!< L2CAP channel local index */
};

/* Application L2CAP environment data */
struct app_l2cap_env_tag app_l2cap_env;

/*!
    \brief      Callback function to handle L2CAP COC events
    \param[in]  event: L2CAP COC event type
    \param[in]  p_data: pointer to L2CAP COC event data
    \param[out] none
    \retval     none
*/
static void app_l2cap_coc_evt_handler(ble_l2cap_coc_evt_t event, ble_l2cap_coc_data_u *p_data)
{

    switch (event) {
    case BLE_L2CAP_COC_EVT_REG_RSP:
        break;

    case BLE_L2CAP_COC_EVT_UNREG_RSP:
        break;

    case BLE_L2CAP_COC_EVT_CONN_IND: {
            ble_l2cap_coc_conn_cfm_t cfm;

            cfm.chann_num = app_l2cap_env.nb_chan == BLE_L2CAP_COC_NOT_AUTORIZED ?
                            app_l2cap_env.nb_chan : p_data->conn_ind.chann_num;
            cfm.local_rx_mtu = BLE_GAP_MAX_OCTETS - BLE_L2CAP_HEADER_LEN;
            cfm.token = p_data->conn_ind.token;

            ble_l2cap_coc_connection_cfm(p_data->conn_ind.conn_idx, p_data->conn_ind.spsm, cfm);
        } break;

    case BLE_L2CAP_COC_EVT_CONN_FAIL:
        dbg_print(NOTICE, "l2cap coc local connect fail, conn idx: %d, status 0x%x, spspm 0x%x, created chann num %d\r\n",
               p_data->conn_fail.conidx, p_data->conn_fail.status, p_data->conn_fail.spsm,
               p_data->conn_fail.channel_num);
        break;

    case BLE_L2CAP_COC_EVT_CONN_INFO:
        dbg_print(NOTICE, "l2cap coc connected, conn idx: %d,  spspm 0x%x, chann_lid %d, peer_rx_mtu %d, local_rx_mtu %d\r\n",
               p_data->conn_info.conn_idx, p_data->conn_info.spsm, p_data->conn_info.chann_lid,
               p_data->conn_info.peer_rx_mtu, p_data->conn_info.local_rx_mtu);
        app_l2cap_env.new_chan_lid = p_data->conn_info.chann_lid;
        break;

    case BLE_L2CAP_COC_EVT_RECFG_RSP:
        break;

    case BLE_L2CAP_COC_EVT_DISCONN_INFO:
        dbg_print(NOTICE, "l2cap disconnected, conn idx: %d, chann lid: %d, reason: 0x%x\r\n",
               p_data->disconn_info.conn_idx, p_data->disconn_info.chann_lid,
               p_data->disconn_info.reason);
        break;

    case BLE_L2CAP_COC_EVT_TX_RSP:
        break;

    case BLE_L2CAP_COC_EVT_RX_IND: {
        int i = 0;
        dbg_print(NOTICE, "l2cap disconnected, conn idx: %d, chann lid: %d, spsm: 0x%x, len: %u\r\n",
               p_data->rx_ind.conn_idx, p_data->rx_ind.chann_lid, p_data->rx_ind.spsm, p_data->rx_ind.len);
        while (i < p_data->rx_ind.len) {
            dbg_print(NOTICE, " %x", p_data->rx_ind.p_data[i++]);
        }
        dbg_print(NOTICE, "\r\n");
    } break;
    default:
        break;
    }
}

/*!
    \brief      Reset application L2CAP module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_l2cap_reset(void)
{
    sys_memset(&app_l2cap_env, 0, sizeof(struct app_l2cap_env_tag));
}

/*!
    \brief      Set application L2CAP channel number
    \param[in]  nb_chan: channel number to set
    \param[out] none
    \retval     none
*/
void app_l2cap_set_nb_chan(uint8_t nb_chan)
{
    app_l2cap_env.nb_chan = nb_chan;
}

/*!
    \brief      Enable enhanced L2CAP COC negotiations
    \param[in]  conidx: connection index
    \param[out] none
    \retval     none
*/
void app_l2cap_coc_enhanced_enable(uint8_t conidx)
{
    ble_l2cap_coc_enhanced_enable(conidx, true);
}

/*!
    \brief      Add Simplified Protocol/Service Multiplexer
    \param[in]  spsm: Simplified Protocol/Service Multiplexer
    \param[in]  sec_lvl_bf: security level bit field @ref ble_l2cap_sec_lvl_bf
    \param[out] none
    \retval     none
*/
void app_l2cap_spsm_add(uint16_t spsm, uint8_t sec_lvl_bf)
{
    if (ble_l2cap_spsm_register(spsm, sec_lvl_bf) != BLE_ERR_NO_ERROR) {
        dbg_print(NOTICE, "app_l2cap_spsm_add spsm 0x%x fail\r\n", spsm);
    }
}

/*!
    \brief      Remove Simplified Protocol/Service Multiplexer
    \param[in]  spsm: Simplified Protocol/Service Multiplexer
    \param[out] none
    \retval     none
*/
void app_l2cap_spsm_remove(uint16_t spsm)
{
    if (ble_l2cap_spsm_unregister(spsm) != BLE_ERR_NO_ERROR) {
        dbg_print(NOTICE, "app_l2cap_spsm_remove spsm 0x%x fail\r\n", spsm);
    }
}

/*!
    \brief      Create a L2CAP credit oriented connection
    \param[in]  conidx: connection index
    \param[in]  local_rx_mtu: local device reception maximum transmit unit size
    \param[in]  nb_chan: number of L2CAP channel
    \param[in]  spsm: Simplified Protocol/Service Multiplexer
    \param[in]  enhanced: true to use enhanced L2CAP COC negotiation, otherwise false
    \param[out] none
    \retval     none
*/
void app_l2cap_con_create(uint8_t conidx, uint16_t local_rx_mtu, uint8_t nb_chan, uint16_t spsm,
                         uint8_t enhanced)
{
    ble_l2cap_coc_param_t param;

    param.local_rx_mtu = local_rx_mtu;
    param.nb_chan = nb_chan;


    if (ble_l2cap_coc_connection_req(conidx, spsm, param, enhanced) != BLE_ERR_NO_ERROR) {
        dbg_print(NOTICE, "app_l2cap_con_creat conidx %u  spsm 0x%x fail\r\n", conidx, spsm);
    }
}

/*!
    \brief      Reconfig a L2CAP credit oriented connection parameter
    \param[in]  conidx: connection index
    \param[in]  nb_chan: number of L2CAP channels to reconfigure
    \param[in]  local_rx_mtu: local device rx mtu to reconfigure
    \param[in]  local_rx_mps: local device rx mps to reconfigure
    \param[out] none
    \retval     none
*/
void app_l2cap_con_reconfig(uint8_t conidx, uint8_t nb_chan, uint16_t local_rx_mtu,
                            uint16_t local_rx_mps)
{
    ble_l2cap_coc_connection_recfg(conidx, nb_chan, local_rx_mtu, local_rx_mps, &app_l2cap_env.new_chan_lid);
}

/*!
    \brief      Terminate a L2CAP credit oriented connection
    \param[in]  conidx: connection index
    \param[in]  chan_lid: L2CAP channel identifier
    \param[out] none
    \retval     none
*/
void app_l2cap_con_terminate(uint8_t conidx, uint8_t chan_lid)
{
    if (ble_l2cap_coc_terminate(conidx, chan_lid) != BLE_ERR_NO_ERROR) {
        dbg_print(NOTICE, "app_l2cap_con_terminate conidx %u  chan_lid 0x%x fail\r\n", conidx, chan_lid);
    }
}

/*!
    \brief      Transmit L2CAP segment packet
    \param[in]  conidx: connection index
    \param[in]  chan_lid: L2CAP channel identifier
    \param[in]  dbg_bf: debug bitfield
    \param[in]  length: transmit data length
    \param[out] none
    \retval     none
*/
void app_l2cap_sdu_send(uint8_t conidx, uint8_t chan_lid, uint8_t dbg_bf, uint16_t length)
{
    uint8_t *p_data = sys_malloc(length);
    if (p_data == NULL)
        return;

    sys_memset(p_data, 0x55, length);
    ble_l2cap_coc_sdu_send(conidx, chan_lid, length, p_data);
}

/*!
    \brief      Init application L2CAP module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_l2cap_mgr_init(void)
{
    app_l2cap_reset();
    ble_l2cap_coc_callback_register(app_l2cap_coc_evt_handler);
}

/*!
    \brief      Deinit application L2CAP module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_l2cap_mgr_deinit(void)
{
    app_l2cap_reset();
    ble_l2cap_coc_callback_unregister(app_l2cap_coc_evt_handler);
}

#endif // (BLE_APP_SUPPORT)
