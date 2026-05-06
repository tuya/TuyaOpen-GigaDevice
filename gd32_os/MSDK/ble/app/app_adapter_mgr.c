/*!
    \file    app_adapter_mgr.c
    \brief   BLE Application adapter manager

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

#include "ble_app_config.h"

#if (BLE_APP_SUPPORT)
#include <string.h>

#include "ble_types.h"

#include "app_adapter_mgr.h"
#include "app_sec_mgr.h"
#include "app_l2cap.h"
#include "ble_scan.h"
#include "ble_conn.h"
#include "app_scan_mgr.h"
#include "app_conn_mgr.h"
#include "app_dev_mgr.h"
#include "app_per_sync_mgr.h"
#include "app_list_mgr.h"
#include "app_adv_mgr.h"
#include "app_iso_mgr.h"
#include "app_flash.h"
#include "app_diss.h"
#include "app_bass.h"
#include "app_cscss.h"
#include "app_blue_courier_link.h"
#include "ble_sample_srv.h"
#include "ble_throughput_srv.h"

#include "wrapper_os.h"
#include "ble_export.h"
#include "ble_list.h"
#include "ble_adapter.h"
#include "ble_utils.h"
#include "ble_init.h"
#include "ble_gatts.h"
#include "dbg_print.h"
#include "util.h"

/* Default Device Name */
#define APP_DFLT_DEVICE_NAME_MAX_LEN    31
#define APP_DFLT_DEVICE_NAME            ("GD-BLE")
#define APP_DFLT_DEVICE_NAME_LEN        (strlen(APP_DFLT_DEVICE_NAME))

/* BLE adapter application environment structure */
struct adapter_env_tag
{
    ble_gap_addr_t id_addr;                             /*!< Local identity address */
    uint8_t own_addr_type;                              /*!< Local address type */
    uint8_t loc_irk[BLE_GAP_KEY_LEN];                   /*!< Local IRK */
    uint8_t adapter_name[APP_DFLT_DEVICE_NAME_MAX_LEN]; /*!< Adapter name */
};

/* BLE adapter application environment data */
struct adapter_env_tag app_env;

/*!
    \brief      Function called after BLE stack enabled, used to get local IRK and set local name
    \param[in]  status: BLE stack enable status
    \param[out] none
    \retval     none
*/
static void app_enable_cmplt_handler(uint16_t status)
{
    //ASSERT_ERR(status == BLE_ERR_NO_ERROR);
    uint32_t str_len = 0;
#if FEAT_SUPPORT_ADV_AFTER_REBOOT
    ble_status_t ret = BLE_ERR_NO_ERROR;
    app_adv_param_t adv_param = {0};
#endif

    ble_adp_loc_irk_get(app_env.loc_irk);

#if FEAT_SUPPORT_SAVE_DEV_NAME
    if (!app_flash_load(BLE_DEV_NAME, app_env.adapter_name, &str_len, APP_DFLT_DEVICE_NAME_MAX_LEN))
#endif
    {
        str_len = snprintf((char *)app_env.adapter_name, APP_DFLT_DEVICE_NAME_MAX_LEN,
                           "%s-%02x:%02x:%02x:%02x:%02x:%02x", APP_DFLT_DEVICE_NAME,
                           app_env.id_addr.addr[0], app_env.id_addr.addr[1],
                           app_env.id_addr.addr[2], app_env.id_addr.addr[3],
                           app_env.id_addr.addr[4], app_env.id_addr.addr[5]);

        str_len = str_len > APP_DFLT_DEVICE_NAME_MAX_LEN ? APP_DFLT_DEVICE_NAME_MAX_LEN : str_len;
    }

    ble_adp_name_set((uint8_t *)app_env.adapter_name, str_len);

#if FEAT_SUPPORT_ADV_AFTER_REBOOT
    adv_param.type = BLE_ADV_TYPE_LEGACY;
    adv_param.prop = 0x0003;    // scannable connectable undirected
    adv_param.pri_phy = BLE_GAP_PHY_1MBPS;
    adv_param.sec_phy = BLE_GAP_PHY_1MBPS;
    adv_param.wl_enable = false;
    adv_param.own_addr_type = BLE_GAP_LOCAL_ADDR_STATIC;
    adv_param.disc_mode = BLE_GAP_ADV_MODE_GEN_DISC;
    adv_param.max_data_len = 0x1F;
    adv_param.adv_intv = APP_ADV_INT_MAX;
    adv_param.ch_map = BLE_GAP_ADV_CHANN_37 | BLE_GAP_ADV_CHANN_38 | BLE_GAP_ADV_CHANN_39;

    ret = app_adv_create(&adv_param);

    if (ret != BLE_ERR_NO_ERROR) {
        dbg_print(NOTICE, "create adv fail status 0x%x\r\n", ret);
    }
#endif

}

/*!
    \brief      Reset BLE app modules and profiles, this function is called after BLE stack is reset and reconfigured
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void app_reset(void)
{
#if (BLE_CFG_ROLE & (BLE_CFG_ROLE_BROADCASTER | BLE_CFG_ROLE_PERIPHERAL))
    app_adv_mgr_reset();
#endif

#if (BLE_CFG_ROLE & (BLE_CFG_ROLE_PERIPHERAL | BLE_CFG_ROLE_CENTRAL))
    app_sec_mgr_reset();
    app_conn_mgr_reset();
#endif

    app_list_mgr_reset();

    app_l2cap_reset();

    app_dm_reset();

#if (BLE_CFG_ROLE & (BLE_CFG_ROLE_OBSERVER | BLE_CFG_ROLE_CENTRAL))
    app_scan_mgr_reset();
#endif

#if (BLE_APP_PER_ADV_SUPPORT)
    app_per_sync_mgr_reset();
#endif

    ble_adp_loc_irk_get(app_env.loc_irk);
    ble_adp_name_set((uint8_t *)app_env.adapter_name, strlen((char *)app_env.adapter_name));
}

/*!
    \brief      Callback function to handle @ref BLE_ADP_EVT_LOC_ADDR_INFO event
    \param[in]  p_info: pointer to local address information
    \param[out] none
    \retval     none
*/
static void app_local_addr_hdlr(ble_gap_local_addr_info_t *p_info)
{
    dbg_print(NOTICE, "BLE local addr: %02X:%02X:%02X:%02X:%02X:%02X, type 0x%x\r\n",
           p_info->addr.addr[5], p_info->addr.addr[4], p_info->addr.addr[3],
           p_info->addr.addr[2], p_info->addr.addr[1], p_info->addr.addr[0],
           p_info->addr.addr_type);

    app_env.id_addr.addr_type = p_info->addr.addr_type;
    memcpy(app_env.id_addr.addr, p_info->addr.addr, BLE_GAP_ADDR_LEN);
}

/*!
    \brief      Callback function to handle @ref BLE_ADP_EVT_RAND_ADDR_GEN_RSP event
    \param[in]  p_rsp: pointer to response data of generate random address
    \param[out] none
    \retval     none
*/
static void app_random_addr_gen_hdlr(ble_gap_rand_addr_gen_rsp_t *p_rsp)
{
    if (!p_rsp->status) {
        dbg_print(NOTICE, "BLE generated random addr: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
               p_rsp->addr[5], p_rsp->addr[4], p_rsp->addr[3],
               p_rsp->addr[2], p_rsp->addr[1], p_rsp->addr[0]);
    }
}

/*!
    \brief      Callback function to handle @ref BLE_ADP_EVT_TEST_END_RSP event
    \param[in]  p_rsp: pointer to BLE test end response data
    \param[out] none
    \retval     none
*/
static void app_le_test_ended_hdlr(ble_gap_test_end_rsp_t *p_rsp)
{
    dbg_print(NOTICE, "le test end, status 0x%x\r\n", p_rsp->status);
}

/*!
    \brief      Callback function to handle @ref BLE_ADP_EVT_TEST_RX_PKT_INFO event
    \param[in]  p_info: pointer to BLE test rx packet information data
    \param[out] none
    \retval     none
*/
static void app_le_test_rx_pkt_info_hdlr(ble_gap_test_rx_pkt_info_t *p_info)
{
    dbg_print(NOTICE, "le test rx received pkt num: %d\r\n", p_info->rcv_pkt_num);
}

/*!
    \brief      Callback function to handle BLE adapter events
    \param[in]  event: BLE adapter event type
    \param[in]  p_data: pointer to BLE adapter event data
    \param[out] none
    \retval     none
*/
static void ble_adp_evt_handler(ble_adp_evt_t event, ble_adp_data_u *p_data)
{
    uint8_t i = 0;
    switch (event) {
    case BLE_ADP_EVT_RESET_CMPL_INFO:
        dbg_print(NOTICE, "adapter reset complete, status 0x%x\r\n", p_data->status);
        app_reset();
        break;

    case BLE_ADP_EVT_ENABLE_CMPL_INFO:
        if (p_data->adapter_info.status == BLE_ERR_NO_ERROR) {
            dbg_print(INFO, "=== Adapter enable success ===\r\n");
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

            dbg_print(NOTICE, "=== BLE Adapter enable complete ===\r\n");
        }

        app_enable_cmplt_handler(p_data->status);
        ble_task_ready();
        break;

    case BLE_ADP_EVT_DISABLE_CMPL_INFO:
        if (!p_data->status) {
            ble_deinit();
            dbg_print(NOTICE,"ble disable success\r\n");
        }
        break;

    case BLE_ADP_EVT_LOC_ADDR_INFO:
        app_local_addr_hdlr(&p_data->loc_addr);
        break;

    case BLE_ADP_EVT_RAND_ADDR_GEN_RSP:
        app_random_addr_gen_hdlr(&p_data->rand_addr);
        break;

    case BLE_ADP_EVT_TEST_END_RSP:
        app_le_test_ended_hdlr(&p_data->test_end_rsp);
        break;

    case BLE_ADP_EVT_TEST_RX_PKT_INFO:
        app_le_test_rx_pkt_info_hdlr(&p_data->test_rx_pkt_info);
        break;

    case BLE_ADP_EVT_TEST_TX_RSP:
        dbg_print(NOTICE, "ble test tx status %x\r\n", p_data->status);
        break;

    case BLE_ADP_EVT_TEST_RX_RSP:
        dbg_print(NOTICE, "ble test rx status %x\r\n", p_data->status);
        break;

    default:
        break;
    }
}

/*!
    \brief      Init adapter application module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_adapter_init(void)
{
    // Reset the application manager environment
    memset(&app_env, 0, sizeof(app_env));

    ble_adp_callback_register(ble_adp_evt_handler);
}

/*!
    \brief      Deinit adapter application module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_adapter_deinit(void)
{
    ble_adp_callback_unregister(ble_adp_evt_handler);
}

/*!
    \brief      Set BLE channel map
    \param[in]  map: BLE channel map
    \param[out] none
    \retval     none
*/
void app_le_set_chann_map(uint8_t map[5])
{
    ble_adp_chann_map_set(map);
}

/*!
    \brief      Reset adapter application module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_ble_reset(void)
{
    ble_adp_reset();
}

/*!
    \brief      Get BLE adapter name
    \param[in]  none
    \param[out] p_name: pointer to address of adapter name
    \retval     uint8_t: name length
*/
uint8_t app_adp_get_name(uint8_t **p_name)
{
    *p_name = app_env.adapter_name;
    return (strlen((char *)app_env.adapter_name));
}

/*!
    \brief      Set BLE adapter name
    \param[in]  p_name: pointer to device name which need to change
    \param[in]  len: name length
    \param[out] none
    \retval     bool: true if set device name successfully, otherwise false
*/
bool app_adp_set_name(char *p_name, uint16_t len)
{
    if (!p_name || len == 0 || len > APP_DFLT_DEVICE_NAME_MAX_LEN)
        return false;

    memset(app_env.adapter_name, 0, APP_DFLT_DEVICE_NAME_MAX_LEN);
    memcpy(app_env.adapter_name, p_name, len);
    ble_adp_name_set((uint8_t *)app_env.adapter_name, len);
#if FEAT_SUPPORT_SAVE_DEV_NAME
    app_flash_save(BLE_DEV_NAME, app_env.adapter_name, len);
#endif
    return true;
}

/*!
    \brief      Control local adapter to enter BLE Test TX Mode
    \param[in]  chann: TX channel, range from 0x00 to 0x27
    \param[in]  tx_data_len: length in bytes of payload data in each packet, range from 0x00 to 0xFF
    \param[in]  tx_pkt_pl: payload type, @ref ble_gap_packet_payload_type
    \param[in]  phy: test PHY rate, @ref ble_gap_phy_pwr_value
    \param[in]  tx_pwr_lvl: transmit power level in dBm range from -127 to +20, 0x7E for minimum and 0x7F for maximum
    \param[out] none
    \retval     none
*/
void app_le_tx_test(uint8_t chann, uint8_t tx_data_len, uint8_t tx_pkt_pl, uint8_t phy,
                    int8_t tx_pwr_lvl)
{
    ble_adp_test_tx(chann, tx_data_len, tx_pkt_pl, phy, tx_pwr_lvl);
}

/*!
    \brief      Control local adapter to enter BLE Test RX Mode
    \param[in]  chann: RX channel, range from 0x00 to 0x27
    \param[in]  phy: test PHY rate, @ref ble_gap_phy_pwr_value
    \param[in]  modulation_idx: modulation Index
    \param[out] none
    \retval     none
*/
void app_le_rx_test(uint8_t chann, uint8_t phy, uint8_t modulation_idx)
{
    ble_adp_test_rx(chann, phy, modulation_idx);
}

/*!
    \brief      Control local adapter to exit BLE Test Mode
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_le_test_end(void)
{
    ble_adp_test_end();
}

/*!
    \brief      Enable adapter application module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_ble_enable(void)
{
    if (ble_work_status_get() == BLE_WORK_STATUS_ENABLE) {
        dbg_print(NOTICE,"ble already enable\r\n");
        return;
    }

    ble_init(true);
}

/*!
    \brief      Disable adapter application module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_ble_disable(void)
{
    if (ble_work_status_get() == BLE_WORK_STATUS_DISABLE) {
        dbg_print(NOTICE,"ble already disable\r\n");
        return;
    }

    ble_sw_deinit();
}
#endif // (BLE_APP_SUPPORT)
