/*!
    \file    app_iso_mgr.c
    \brief   Implementation of BLE application iso manager.

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

#if (BLE_APP_BIS_SUPPORT || BLE_APP_CIS_SUPPORT)
#include <string.h>
#include <stdio.h>

#include "ble_iso.h"
#include "app_iso_mgr.h"
#include "dbg_print.h"

/* BIG handle value */
#define APP_BIG_HDL             0x10

/* Invalid BIG index value */
#define BIG_INVALID_IDX         (0xFF)

/* CIG ID */
#define APP_CIG_ID              0x20

/* BIG broadcast code value */
static uint8_t big_bc[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

#if (BLE_APP_BIS_SUPPORT)
/*!
    \brief      Callback function to handle @ref BLE_ISO_EVT_BIG_INFO_RX event
    \param[in]  p_info: pointer to received BIG information
    \param[out] none
    \retval     none
*/
static void app_big_info_rx_hdlr(ble_gap_big_info_t *p_info)
{
    dbg_print(NOTICE, "big info rcvd, sdu int:%d, iso int %d, pdu %d, sdu %d, bis num %d, se num %d, bn %d, pto %d, irc %d, phy %d, framing %d, encrypt %d\r\n",
           p_info->sdu_interval, p_info->iso_interval, p_info->max_pdu,
           p_info->max_sdu, p_info->num_bis, p_info->nse, p_info->bn,
           p_info->pto, p_info->irc, p_info->phy, p_info->framing,
           p_info->encrypted);
}

/*!
    \brief      Callback function to handle @ref BLE_ISO_EVT_BIG_CREATE_FAIL event
    \param[in]  p_info: pointer to BIG create fail information
    \param[out] none
    \retval     none
*/
static void app_big_create_fail_hdlr(ble_iso_create_fail_info_t *p_info)
{
    dbg_print(NOTICE, "big create fail, status 0x%x\r\n", p_info->status);
}

/*!
    \brief      Callback function to handle @ref BLE_ISO_EVT_BIG_CREATE_INFO event
    \param[in]  p_info: pointer to BIG created information
    \param[out] none
    \retval     none
*/
static void  app_big_create_info_hdlr(ble_gap_big_create_info_t *p_info)
{
    uint8_t i = 0;

    dbg_print(NOTICE, "big create success, group lid 0x%x, sync delay %d, latency %d, phy %d, nse %d, bn %d, pto %d, irc %d, max pdu %d, intv %d, bis num %d",
            p_info->group_lid, p_info->sync_delay_us, p_info->tlatency_us, p_info->phy, p_info->nse,
            p_info->bn, p_info->pto, p_info->irc, p_info->max_pdu, p_info->iso_intv, p_info->bis_num);

    for (i = 0; i < p_info->bis_num; i++)
        dbg_print(NOTICE, ", hdl[%d] 0x%x", i, p_info->p_conn_hdl[i]);

    dbg_print(NOTICE, "\r\n");
}

/*!
    \brief      Callback function to handle @ref BLE_ISO_EVT_BIG_SYNC_STATUS event
    \param[in]  p_info: pointer to BIG sync status information
    \param[out] none
    \retval     none
*/
static void app_big_sync_status_hdlr(ble_gap_big_sync_status_info_t *p_info)
{
    switch (p_info->status) {
    case BLE_GAP_BIG_SYNC_STATUS_ESTABLISHED: {
        uint8_t i = 0;
        dbg_print(NOTICE, "big sync established, group lid 0x%x, latency %d, nse %d, bn %d, pto %d, irc %d, max pdu %d, intv %d, bis num %d",
               p_info->group_lid, p_info->tlatency_us, p_info->nse, p_info->bn, p_info->pto,
               p_info->irc, p_info->max_pdu, p_info->iso_intv, p_info->bis_num);

        for (i = 0; i < p_info->bis_num; i++)
            dbg_print(NOTICE, ", hdl[%d] 0x%x", i, p_info->p_conn_hdl[i]);

        dbg_print(NOTICE, "\r\n");
    } break;

    case BLE_GAP_BIG_SYNC_STATUS_LOST:
        dbg_print(NOTICE, "big sync lost, group lid 0x%x\r\n", p_info->group_lid);
        break;

    case BLE_GAP_BIG_SYNC_STATUS_MIC_FAILURE:
        dbg_print(NOTICE, "big sync mic failure, group lid 0x%x\r\n", p_info->group_lid);
        break;

    case BLE_GAP_BIG_SYNC_STATUS_UPPER_TERMINATE:
        dbg_print(NOTICE, "big sync local terminate, group lid 0x%x\r\n", p_info->group_lid);
        break;

    case BLE_GAP_BIG_SYNC_STATUS_PEER_TERMINATE:
        dbg_print(NOTICE, "big sync peer terminate, group lid 0x%x\r\n", p_info->group_lid);
        break;

    default:
        break;
    }
}

/*!
    \brief      Callback function to handle @ref BLE_ISO_EVT_BIG_SYNC_INFO event
    \param[in]  p_info: pointer to BIG sync added information
    \param[out] none
    \retval     none
*/
void app_big_sync_info_hdlr(ble_gap_big_sync_added_info_t *p_info)
{
    uint8_t i = 0;

    dbg_print(NOTICE, "big sync added, big handle 0x%x, group lid 0x%x, steam num %d",
           p_info->big_handle, p_info->group_lid, p_info->stream_num);

    for (i = 0; i < p_info->stream_num; i++)
        dbg_print(NOTICE, ", stream lid[%d] 0x%x", i, p_info->p_stream_lid[i]);

    dbg_print(NOTICE, "\r\n");
}
#endif // (BLE_APP_BIS_SUPPORT)

#if (BLE_APP_CIS_SUPPORT)
/*!
    \brief      Callback function to handle @ref BLE_ISO_EVT_CIG_CREATE_FAIL event
    \param[in]  p_info: pointer to CIS create fail information
    \param[out] none
    \retval     none
*/
static void app_cig_create_fail_hdlr(ble_iso_create_fail_info_t *p_info)
{
    dbg_print(NOTICE, "cig create fail, status 0x%x\r\n", p_info->status);
}

/*!
    \brief      Callback function to handle @ref BLE_ISO_EVT_CIS_CONN_INFO event
    \param[in]  p_info: pointer to CIS connection information
    \param[out] none
    \retval     none
*/
void app_cis_conn_info_hdlr(ble_gap_cis_conn_info_t *p_info)
{
    dbg_print(NOTICE, "cis conn est, group lid 0x%x, stream lid 0x%x, sync delay(cig %d, cis %d), latency(m2s %d, s2m %d), phy(m2s %d, s2m %d), nse %d, bn(m2s %d, s2m %d), ft(m2s %d, s2m %d), max pdu(m2s %d, s2m %d), interval %d \r\n",
           p_info->group_lid, p_info->stream_lid, p_info->sync_delay_us, p_info->sync_delay_us,
           p_info->tlatency_m2s_us, p_info->tlatency_s2m_us, p_info->phy_m2s, p_info->phy_s2m, p_info->nse,
           p_info->bn_m2s, p_info->bn_s2m, p_info->ft_m2s, p_info->ft_s2m,
           p_info->max_pdu_m2s, p_info->max_pdu_s2m, p_info->iso_intv_frames);
}

/*!
    \brief      Callback function to handle @ref BLE_ISO_EVT_CIS_DISCONN_INFO event
    \param[in]  p_info: pointer to CIS disconnect information
    \param[out] none
    \retval     none
*/
void app_cis_disconn_info_hdlr(ble_gap_cis_disconn_info_t *p_info)
{
    dbg_print(NOTICE, "cis disconnected, stream lid 0x%x, reason 0x%x \r\n", p_info->stream_lid, p_info->reason);
}

/*!
    \brief      Callback function to handle @ref BLE_ISO_EVT_ISO_TEST_CNT event
    \param[in]  p_info: pointer to ISO test count information
    \param[out] none
    \retval     none
*/
void app_iso_test_cnt_hdlr(ble_gap_iso_test_cnt_info_t *p_info)
{
    dbg_print(NOTICE, "iso test cnt, stream lid 0x%x, rcvd cnt %d, missed cnt %d, failed cnt %d\r\n",
           p_info->stream_lid, p_info->rx_pkt_num, p_info->miss_pkt_num, p_info->fail_pkt_num);
}
#endif // (BLE_APP_CIS_SUPPORT)

/*!
    \brief      Callback function to handle ISO events
    \param[in]  event: ISO event type
    \param[in]  p_data: pointer to ISO event data
    \param[out] none
    \retval     none
*/
void app_iso_callback(ble_iso_evt_t event, void *p_data)
{
    switch (event) {
    #if (BLE_APP_BIS_SUPPORT)
    case BLE_ISO_EVT_BIG_INFO_RX:
        app_big_info_rx_hdlr((ble_gap_big_info_t *)p_data);
        break;

    case BLE_ISO_EVT_BIG_CREATE_FAIL:
        app_big_create_fail_hdlr((ble_iso_create_fail_info_t *)p_data);
        break;

    case BLE_ISO_EVT_BIG_CREATE_INFO:
        app_big_create_info_hdlr((ble_gap_big_create_info_t *)p_data);
        break;

    case BLE_ISO_EVT_BIG_SYNC_STATUS:
        app_big_sync_status_hdlr((ble_gap_big_sync_status_info_t *)p_data);
        break;

    case BLE_ISO_EVT_BIG_SYNC_INFO:
        app_big_sync_info_hdlr((ble_gap_big_sync_added_info_t *)p_data);
        break;
    #endif  // (BLE_APP_BIS_SUPPORT)

    #if (BLE_APP_CIS_SUPPORT)
    case BLE_ISO_EVT_CIG_CREATE_FAIL:
        app_cig_create_fail_hdlr((ble_iso_create_fail_info_t *)p_data);
        break;

    case BLE_ISO_EVT_CIS_CONN_INFO:
        app_cis_conn_info_hdlr((ble_gap_cis_conn_info_t *)p_data);
        break;

    case BLE_ISO_EVT_CIS_DISCONN_INFO:
        app_cis_disconn_info_hdlr((ble_gap_cis_disconn_info_t *)p_data);
        break;
    #endif  // (BLE_APP_CIS_SUPPORT)

    case BLE_ISO_EVT_ISO_TEST_CNT:
        app_iso_test_cnt_hdlr((ble_gap_iso_test_cnt_info_t *)p_data);
        break;

    default:
        break;
    }
}

#if (BLE_APP_BIS_SUPPORT)
/*!
    \brief      Create a BIG
    \param[in]  adv_idx: associated periodic advertising index
    \param[in]  p_param: pointer to BIG parameter
    \param[out] none
    \retval     none
*/
void app_big_create(uint8_t adv_idx, app_big_param_t *p_param)
{
    ble_iso_big_param_t big_param;

    big_param.test_param.sdu_intv_us = p_param->interval * 1000;
    big_param.test_param.max_sdu = p_param->max_sdu;
    big_param.test_param.packing = p_param->packing;
    big_param.test_param.framing = p_param->framing;
    big_param.test_param.phy_bf = p_param->phy;
    big_param.test_param.iso_intv = (p_param->interval * 4) / 5;
    big_param.test_param.nse = p_param->nse;
    big_param.test_param.max_pdu = 0xFB;
    big_param.test_param.bn = p_param->bn;
    big_param.test_param.irc = p_param->irc;
    big_param.test_param.pto = p_param->pto;

    if (p_param->encryption) {
        uint8_t i = 0;

        dbg_print(NOTICE, "broadcase code: 0x");
        for (i = 0; i < BLE_GAP_KEY_LEN; i++) {
            dbg_print(NOTICE, "%02x", big_bc[i]);
        }

        dbg_print(NOTICE, "\r\n");
    }

    ble_iso_big_create(adv_idx, true, APP_BIG_HDL, p_param->num_bis, &big_param, p_param->encryption, big_bc);
}

/*!
    \brief      Stop a BIG
    \param[in]  group_lid: group local index
    \param[out] none
    \retval     none
*/
void app_big_stop(uint8_t group_lid)
{
    ble_iso_big_terminate(group_lid);
}

/*!
    \brief      Create a BIG sync
    \param[in]  sync_idx: periodic sync index
    \param[in]  num_bis: BIS number to sync
    \param[in]  encyption: true to sync encrypted BIS, otherwise false
    \param[out] none
    \retval     none
*/
void app_big_sync_create(uint8_t sync_idx, uint8_t num_bis, bool encyption)
{
    ble_iso_big_sync_create(sync_idx, APP_BIG_HDL, num_bis, 0x100, 0, encyption, big_bc);
}

/*!
    \brief      Stop a BIG sync
    \param[in]  group_lid: group local index
    \param[out] none
    \retval     none
*/
void app_big_sync_stop(uint8_t group_lid)
{
    ble_iso_big_sync_terminate(group_lid);
}
#endif // (BLE_APP_BIS_SUPPORT)

#if (BLE_APP_CIS_SUPPORT)
/*!
    \brief      Create a CIG
    \param[in]  conn_idx: associated BLE connection index
    \param[in]  p_param: pointer to CIG parameter
    \param[out] none
    \retval     none
*/
void app_cig_create(uint8_t conn_idx, app_cig_param_t *p_param)
{
    ble_iso_cig_param_t cig_param;
    ble_iso_cis_param_t cis_param;

    cig_param.test_param.sdu_intv_m2s_us = 100000;     // 100ms
    cig_param.test_param.sdu_intv_s2m_us = 100000;     // 100ms
    cig_param.test_param.packing = p_param->packing;
    cig_param.test_param.framing = p_param->framing;
    cig_param.test_param.sca = 0;
    cig_param.test_param.ft_m2s_ms = p_param->ft;
    cig_param.test_param.ft_s2m_ms = p_param->ft;
    cig_param.test_param.iso_intv = 80;         // 100ms

    cis_param.test_param.max_sdu_m2s = p_param->max_sdu;
    cis_param.test_param.max_sdu_s2m = p_param->max_sdu;
    cis_param.test_param.phy_m2s = p_param->phy;
    cis_param.test_param.phy_s2m = p_param->phy;
    cis_param.test_param.max_pdu_m2s = 0xFB;
    cis_param.test_param.max_pdu_s2m = 0xFB;
    cis_param.test_param.bn_m2s = p_param->bn;
    cis_param.test_param.bn_s2m = p_param->bn;
    cis_param.test_param.nse = p_param->nse;

    ble_iso_cig_create(conn_idx, APP_CIG_ID, p_param->num_cis, true, &cig_param, &cis_param);
}

/*!
    \brief      Prepare a CIS to be connect
    \param[in]  conn_idx: associated BLE connection index
    \param[in]  cis_id: CIS ID
    \param[out] none
    \retval     none
*/
void app_cis_prepare(uint8_t conidx, uint8_t cis_id)
{
    ble_iso_cis_prepare(conidx, APP_CIG_ID, cis_id);
}

/*!
    \brief      Disconnect a CIS
    \param[in]  stream_lid: stream local index
    \param[out] none
    \retval     none
*/
void app_cis_disconn(uint8_t stream_lid)
{
    ble_iso_cis_disconn(stream_lid);
}

/*!
    \brief      Stop a CIG
    \param[in]  group_lid: group local index
    \param[out] none
    \retval     none
*/
void app_cig_stop(uint8_t group_lid)
{
    ble_iso_cig_terminate(group_lid);
}
#endif // (BLE_APP_CIS_SUPPORT)

#if (BLE_APP_BIS_SUPPORT || BLE_APP_CIS_SUPPORT)
/*!
    \brief      Start ISO tx test
    \param[in]  stream_lid: stream local index
    \param[in]  type: ISO tx payload type
      \arg        0x00: zero length payload
      \arg        0x01: variable length payload
      \arg        0x02: maximum length payload
    \param[out] none
    \retval     none
*/
void app_iso_tx_test(uint8_t stream_lid, uint8_t type)
{
    ble_iso_test_tx(stream_lid, type);
}

/*!
    \brief      Start ISO rx test
    \param[in]  stream_lid: stream local index
    \param[in]  type: ISO rx payload type
      \arg        0x00: zero length payload
      \arg        0x01: variable length payload
      \arg        0x02: maximum length payload
    \param[out] none
    \retval     none
*/
void app_iso_rx_test(uint8_t stream_lid, uint8_t type)
{
    ble_iso_test_rx(stream_lid, type);
}

/*!
    \brief      Stop ISO test
    \param[in]  stream_lid: stream local index
    \param[out] none
    \retval     none
*/
void app_iso_test_end(uint8_t stream_lid)
{
    ble_iso_test_end(stream_lid);
}

/*!
    \brief      Read ISO test count
    \param[in]  stream_lid: stream local index
    \param[out] none
    \retval     none
*/
void app_iso_read_test_cnt(uint8_t stream_lid)
{
    ble_iso_test_cnt_read(stream_lid);
}

/*!
    \brief      Init APP ISO manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_iso_mgr_init(void)
{
    ble_iso_callback_register(app_iso_callback);
}

/*!
    \brief      Deinit APP ISO manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_iso_mgr_deinit(void)
{
    ble_iso_callback_unregister(app_iso_callback);
}

#endif // (BLE_APP_BIS_SUPPORT || BLE_APP_CIS_SUPPORT)
#endif // (BLE_APP_SUPPORT && (BLE_APP_BIS_SUPPORT || BLE_APP_CIS_SUPPORT))
