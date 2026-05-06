/*!
    \file    ble_adapter.h
    \brief   Module for handling the BLE adapter.

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

#ifndef _BLE_ADAPTER_H__
#define _BLE_ADAPTER_H__

#include <stdint.h>

#include "ble_gap.h"
#include "ble_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* BLE adapter event */
typedef enum
{
    BLE_ADP_EVT_ENABLE_CMPL_INFO,           /*!< Event notify for adapter enable complete */
    BLE_ADP_EVT_RESET_CMPL_INFO,            /*!< Event notify for adapter reset complete */
    BLE_ADP_EVT_DISABLE_CMPL_INFO,          /*!< Event notify for adapter disable complete */
    BLE_ADP_EVT_CHANN_MAP_SET_RSP,          /*!< Event notify for set channel map response */
    BLE_ADP_EVT_LOC_IRK_SET_RSP,            /*!< Event notify for set local irk response */
    BLE_ADP_EVT_LOC_ADDR_INFO,              /*!< Event notify for local address information */
    BLE_ADP_EVT_NAME_SET_RSP,               /*!< Event notify for set name response */
    BLE_ADP_EVT_ADDR_RESLV_RSP,             /*!< Event notify for resolve address response */
    BLE_ADP_EVT_RAND_ADDR_GEN_RSP,          /*!< Event notify for generate random address response */
    BLE_ADP_EVT_TEST_TX_RSP,                /*!< Event notify for test tx mode response */
    BLE_ADP_EVT_TEST_RX_RSP,                /*!< Event notify for test rx mode response */
    BLE_ADP_EVT_TEST_END_RSP,               /*!< Event notify for end test mode response */
    BLE_ADP_EVT_TEST_RX_PKT_INFO,           /*!< Event notify for received packet information in test rx mode */
    BLE_ADP_EVT_PRIVACY_RECFG_RSP,
} ble_adp_evt_t;

/* BLE adapter information structure */
typedef struct
{
    uint16_t                status;             /*!< Adapter enable status */
    uint8_t                 adv_set_num;        /*!< Number of advertising sets */
    ble_gap_tx_pwr_range_t  tx_pwr_range;       /*!< TX power range */
    ble_gap_sugg_dft_data_t sugg_dft_data;      /*!< Suggested default data length */
    ble_gap_max_data_len_t  max_data_len;       /*!< Maximum data length */
    ble_gap_irk_t           loc_irk_info;       /*!< Local IRK */
    ble_gap_local_ver_t     version;            /*!< Local version */
    uint16_t                max_adv_data_len;   /*!< Maximum advertising data length */
} ble_adp_info_t;

/* Union data for adapter event data */
typedef union ble_adp_data
{
    uint16_t                    status;             /*!< Opration status */
    ble_gap_local_addr_info_t   loc_addr;           /*!< Local address information */
    ble_gap_rand_addr_gen_rsp_t rand_addr;          /*!< Generate random address response */
    ble_gap_addr_resolve_rsp_t  addr_reslv;         /*!< Resolve address response */
    ble_gap_rand_num_gen_rsp_t  random_num;         /*!< Generate random number response */
    ble_adp_info_t              adapter_info;       /*!< Adapter information */
    ble_gap_test_end_rsp_t      test_end_rsp;       /*!< Test end response */
    ble_gap_test_rx_pkt_info_t  test_rx_pkt_info;   /*!< Test rx received packet information */
} ble_adp_data_u;

/* Prototype of BLE adapter event handler */
typedef void (*ble_adp_evt_handler_t)(ble_adp_evt_t event, ble_adp_data_u *p_data);

/*!
    \brief      Reset BLE adapter module
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_reset(void);

/*!
    \brief      Register callback function to BLE adapter module
    \param[in]  callback: callback function to handle BLE adapter events
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_callback_register(ble_adp_evt_handler_t callback);

/*!
    \brief      Unregister callback function from BLE adapter module
    \param[in]  callback: callback function to handle BLE adapter events
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_callback_unregister(ble_adp_evt_handler_t callback);

/*!
    \brief      Set BLE channel map
    \param[in]  p_chann_map: pointer to channel map array
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_chann_map_set(uint8_t *p_chann_map);

/*!
    \brief      Set local irk
    \param[in]  p_irk: pointer to local irk array
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_loc_irk_set(uint8_t *p_irk);

/*!
    \brief      Get local irk
    \param[in]  none
    \param[out] p_irk: pointer to local irk array
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_loc_irk_get(uint8_t *p_irk);

/*!
    \brief      Get local identity address
    \param[in]  none
    \param[out] p_id_addr: pointer to local identity address
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_identity_addr_get(ble_gap_addr_t *p_id_addr);

/*!
    \brief      Get local public address
    \param[out]  p_addr: pointer to local public address
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_public_addr_get(uint8_t *p_addr);

/*!
    \brief      Set local public address
    \param[in]  p_addr: pointer to local public address, the address will be used after next reboot
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_public_addr_set(uint8_t *p_addr);

/*!
    \brief      Set local device name
    \param[in]  p_name: pointer to local device name
    \param[in]  name_len: name length
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_name_set(uint8_t *p_name, uint8_t name_len);

/*!
    \brief      Get local version
    \param[in]  none
    \param[out] p_ver: pointer to local version
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_local_ver_get(ble_gap_local_ver_t *p_ver);

/*!
    \brief      Get local suggested default data packet length
    \param[in]  none
    \param[out] p_data: pointer to local suggested default data packet length
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_sugg_dft_data_len_get(ble_gap_sugg_dft_data_t *p_data);

/*!
    \brief      Get local tx power range
    \param[in]  none
    \param[out] p_val: pointer to local tx power range
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_tx_pwr_range_get(ble_gap_tx_pwr_range_t *p_val);

/*!
    \brief      Get local maximum data packet length
    \param[in]  none
    \param[out] p_len: pointer to maximum data packet length
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_max_data_len_get(ble_gap_max_data_len_t *p_len);

/*!
    \brief      Get local maximum number of advertising sets
    \param[in]  none
    \param[out] p_val: pointer to maximum number of advertising sets
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_adv_sets_num_get(uint8_t *p_val);

/*!
    \brief      Resolve address with given irks
    \param[in]  p_addr: address needs to resolve
    \param[in]  p_irk: array of IRKs used for address resolution
    \param[in]  irk_num: number of provided IRKs
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_addr_resolve(uint8_t *p_addr, uint8_t *p_irk, uint8_t irk_num);

/*!
    \brief      Generate static random address
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_static_random_addr_gen(void);

/*!
    \brief      Generate resolveable private random address
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_resolvable_private_addr_gen(void);

/*!
    \brief      Generate none resolveable private random address
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_none_resolvable_private_addr_gen(void);

/*!
    \brief      Control local adapter to enter BLE Test TX Mode
    \param[in]  chann: TX channel, range from 0x00 to 0x27
    \param[in]  tx_data_len: length in bytes of payload data in each packet, range from 0x00 to 0xFF
    \param[in]  tx_pkt_payload: payload type, @ref ble_gap_packet_payload_type
    \param[in]  phy: test PHY rate, @ref ble_gap_phy_pwr_value
    \param[in]  tx_pwr_lvl: transmit power level in dBm range from -127 to +20, 0x7E for minimum and 0x7F for maximum
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_test_tx(uint8_t chann, uint8_t tx_data_len, uint8_t tx_pkt_payload,
                uint8_t phy, int8_t tx_pwr_lvl);

/*!
    \brief      Control local adapter to enter BLE Test RX Mode
    \param[in]  chann: RX channel, range from 0x00 to 0x27
    \param[in]  phy: test PHY rate, @ref ble_gap_phy_pwr_value
    \param[in]  modulation_idx: modulation Index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_test_rx(uint8_t chann, uint8_t phy, uint8_t modulation_idx);

/*!
    \brief      Control local adapter to exit BLE Test Mode
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_adp_test_end(void);

ble_status_t ble_adp_privacy_recfg(uint8_t privacy_cfg, uint8_t *p_private_identity);

#ifdef __cplusplus
}
#endif

#endif // _BLE_ADAPTER_H__
