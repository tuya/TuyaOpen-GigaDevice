/*!
    \file    app_adv_mgr.c
    \brief   Implementation of BLE application advertising manager to advertise.

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

#if (BLE_APP_SUPPORT && (BLE_CFG_ROLE & (BLE_CFG_ROLE_BROADCASTER | BLE_CFG_ROLE_PERIPHERAL)))
#include <string.h>
#include <stdlib.h>
#include "app_adv_mgr.h"
#include "app_adapter_mgr.h"

#include "ble_adv_data.h"
#include "ble_adv.h"
#include "ble_conn.h"

#include "wrapper_os.h"
#include "dbg_print.h"

/* Max advertising set number */
#define APP_ADV_SET_NUM         2

/* Invalid advertising index */
#define ADV_INVALID_IDX         (0xFF)

/* Advertising channel map */
#define APP_ADV_CHMAP           (0x07)

/* Advertising data used for test */
static uint8_t adv_data_1[7] = {0x06, 0x16, 0x52, 0x18, 0x18, 0x36, 0x9A};

/* Periodic advertising data used for test */
static uint8_t per_data_1[52] = {0x33, 0x16, 0x51, 0x18, 0x40, 0x9c, 0x00, 0x01, 0x02, 0x06,
                                 0x00, 0x00, 0x00, 0x00, 0x0d, 0x02, 0x01, 0x08, 0x02, 0x02,
                                 0x01, 0x03, 0x04, 0x78, 0x00, 0x02, 0x05, 0x01, 0x07, 0x03,
                                 0x02, 0x04, 0x00, 0x02, 0x04, 0x80, 0x01, 0x06, 0x05, 0x03,
                                 0x00, 0x04, 0x00, 0x00, 0x02, 0x06, 0x05, 0x03, 0x00, 0x08,
                                 0x00, 0x00
                                };

/* Advertising data used for HOGP test */
static uint8_t adv_data_hogp[] = {0x03, 0x19, 0x80, 0x01, 0x03, 0x02, 0x12, 0x18};

/* Advertising data used for BQB test */
static uint8_t bqb_adv_data_1[25] = {
    0x03,
    BLE_AD_TYPE_SERVICE_UUID_16_COMPLETE,
    0x0A,
    0x18,

    0x02,
    BLE_AD_TYPE_TX_POWER_LEVEL,
    -60,

    0x03,
    BLE_AD_TYPE_ADVERTISING_INTERVAL,
    0xA0,
    0x00,

    0x03,
    BLE_AD_TYPE_SERVICE_DATA_UUID_16,
    0x01,
    0x02,

    0x05,
    BLE_AD_TYPE_SLAVE_CONNECTION_INTERVAL_RANGE,
    0x10,
    0x00,
    0x00,
    0x01,

    0x03,
    BLE_AD_TYPE_SOLICITED_SERVICE_UUID_16,
    0x0A,
    0x18,
};

/* Advertising data used for BQB test */
static uint8_t bqb_adv_data_2[24] = {
    0x05, 0x03, 0x00, 0x18, 0x01, 0x18,
    0x0D, 0x09, 0x50, 0x54, 0x53, 0x2D, 0x47, 0x41, 0x50, 0x2D, 0x30, 0x36, 0x42, 0x38,
    0x03, 0x19, 0x00, 0x00
};

/* Advertising data used for BQB test */
static uint8_t bqb_adv_data_3[22] = {
    0x07,
    BLE_AD_TYPE_PUBLIC_TARGET_ADDRESS,
    0x11,
    0x22,
    0x33,
    0x44,
    0x55,
    0x66,

    0x07,
    BLE_AD_TYPE_RANDOM_TARGET_ADDRESS,
    0x11,
    0x22,
    0x33,
    0x44,
    0x55,
    0x66,

    0x05,
    BLE_AD_TYPE_ADVERTISING_INTERVAL_LONG,
    0x00,
    0x00,
    0x11,
    0x00,
};

/* Advertising data used for BQB test */
static uint8_t bqb_adv_data_4[28] = {
    0x1B,
    BLE_AD_TYPE_URI,
    0x00,
    0x01,
    0x68,
    0x74,
    0x74,
    0x70,
    0x73,
    0x3A,
    0x2F,
    0x2F,
    0x77,
    0x77,
    0x77,
    0x2E,
    0x62,
    0x6C,
    0x75,
    0x65,
    0x74,
    0x6F,
    0x6F,
    0x74,
    0x2E,
    0x63,
    0x6F,
    0x6D
};

/* Scan response data used for BQB test */
static uint8_t bqb_scan_rsp_data_2[27] = {
    0x02, 0x01, 0x04,
    0x05, 0x03, 0x00, 0x18, 0x01, 0x18,
    0x0D, 0x09, 0x50, 0x54, 0x53, 0x2D, 0x47, 0x41, 0x50, 0x2D, 0x30, 0x36, 0x42, 0x38,
    0x03, 0x19, 0x00, 0x00
};

/* Periodic advertising data used for BQB test */
static uint8_t bqb_per_adv_data_2[27] = {
    0x02, 0x01, 0x04,
    0x05, 0x03, 0x00, 0x18, 0x01, 0x18,
    0x0D, 0x09, 0x50, 0x54, 0x53, 0x2D, 0x47, 0x41, 0x50, 0x2D, 0x30, 0x36, 0x42, 0x38,
    0x03, 0x19, 0x00, 0x00
};

/* Service data used to put into advertising data */
static uint8_t service_data[8] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};

/* URL data used to put into advertising data */
static uint8_t url[25] = {
    0x68,
    0x74,
    0x74,
    0x70,
    0x73,
    0x3A,
    0x2F,
    0x2F,
    0x77,
    0x77,
    0x77,
    0x2E,
    0x62,
    0x6C,
    0x75,
    0x65,
    0x74,
    0x6F,
    0x6F,
    0x74,
    0x68,
    0x2E,
    0x63,
    0x6F,
    0x6D
};

/* APP advertising set information structure */
typedef struct
{
    uint8_t idx                 /*!< Advertising set index */;
    uint8_t type;               /*!< advertising type, @ref ble_gap_adv_type_t */
    uint16_t prop;              /*!< Advertising properties.
                                    @ref ble_gap_legacy_adv_prop_t for legacy advertising,
                                    @ref ble_gap_extended_adv_prop_t for extended advertising,
                                    @ref ble_gap_periodic_adv_prop_t for periodic advertising */
    uint8_t pri_phy;            /*!< Indicate on which PHY primary advertising has to be performed, @ref ble_gap_phy_t */
    uint8_t sec_phy;            /*!< Indicate on which PHY secondary advertising has to be performed, @ref ble_gap_phy_t */
    bool wl_enable;             /*!< True to use whitelist, otherwise do not use */
    uint8_t own_addr_type;      /*!< Own address type used in advertising, @ref ble_gap_local_addr_type_t */
    uint8_t disc_mode;          /*!< Discovery mode, @ref ble_gap_adv_discovery_mode_t */
    uint16_t max_data_len;      /*!< Max advertising data length */
    bool remove_after_stop;     /*!< True to remove advertising set after stopped */
    ble_gap_addr_t peer_addr;   /*!< Peer address, used for directed advertising */
    ble_adv_state_t state;      /*!< Advertising state */
} app_adv_set_t;

/* APP advertising environment structure */
typedef struct
{
    uint8_t adv_sid;                            /*!< Advertising SID */
    uint8_t adv_data_type;                      /* advertising data type. 0: app generate, 1: fix data for bis test, 2: fix adv data for sync test,
                                                   3,4,5,6: fix adv data for GAP bqb test, others: reserved */
    app_adv_set_t adv_set[APP_ADV_SET_NUM];     /*!< Advertising set information */
    ble_adv_data_set_t cus_adv;                 /*!< Customized adv data*/
    ble_adv_data_set_t cus_scan_rsp;            /*!< Customized scan response*/
} app_adv_env_t;

/* APP advertising environment data */
static app_adv_env_t app_adv_env;

/*!
    \brief      Get free advertising set
    \param[in]  none
    \param[out] none
    \retval     app_adv_set_t *: pointer to available BLE advertising set
*/
static app_adv_set_t *app_get_free_adv(void)
{
    uint8_t i;

    for (i = 0; i < APP_ADV_SET_NUM; i++) {
        if (app_adv_env.adv_set[i].idx == ADV_INVALID_IDX) {
            return &app_adv_env.adv_set[i];
        }
    }

    return NULL;
}

/*!
    \brief      Free advertising set
    \param[in]  p_adv: pointer to BLE advertising set
    \param[out] none
    \retval     none
*/
static void app_free_adv_set(app_adv_set_t *p_adv)
{
    p_adv->idx = ADV_INVALID_IDX;
    p_adv->state = BLE_ADV_STATE_IDLE;
}

/*!
    \brief      Get available advertising set ID to be used
    \param[in]  none
    \param[out] none
    \retval     uint8_t: advertising set ID value
*/
static uint8_t app_get_adv_sid(void)
{
    app_adv_env.adv_sid++;

    if (app_adv_env.adv_sid > 0x0F) {
        app_adv_env.adv_sid = 0x00;
    }

    return app_adv_env.adv_sid;
}

/*!
    \brief      Build advertising data
    \details    Use name + multi manufacturer data to fill different length advertising data
    \param[in]  p_adv: pointer to BLE advertising set
    \param[in]  p_buf: buffer address to fill advertising data
    \param[in]  max_length: max length of advertising data
    \param[in]  flag: true if flag is needed in advertising data, otherwise false
    \param[out] none
    \retval     uint16_t: total length of the result advertising data
*/
static uint16_t app_build_adv_data(app_adv_set_t *p_adv, uint8_t *p_buf, uint16_t max_length, bool flag)
{
    uint16_t rem_len = max_length;
    uint8_t dev_name_length;
    uint8_t manu_len;
    uint8_t i;
    uint8_t *p_adp_name = NULL;
    uint8_t adp_name_length;

    if (flag && (p_adv->disc_mode != BLE_GAP_ADV_MODE_BEACON)) {
        // Check if additional data can be added to the Advertising data - 2 bytes needed for type and length
        if (rem_len < 3) {
            return 0;
        }

        *p_buf++ = 0x02;
        *p_buf++ = BLE_AD_TYPE_FLAGS;

        *p_buf = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;
        if (p_adv->disc_mode == BLE_GAP_ADV_MODE_GEN_DISC) {
            *p_buf |= BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE;
        } else if (p_adv->disc_mode == BLE_GAP_ADV_MODE_LIM_DISC) {
            *p_buf |= BLE_GAP_ADV_FLAG_LE_LIMITED_DISC_MODE;
        }
        p_buf++;

        rem_len -= 3;
    }

    if (rem_len <= 2) {
        return (max_length - rem_len);
    }

    adp_name_length = app_adp_get_name(&p_adp_name);

    dev_name_length = adp_name_length > (rem_len - 2) ? (rem_len - 2) :
                      adp_name_length;

    // Device name length
    *p_buf = dev_name_length + 1;
    // Device name flag (check if device name is complete or not)
    *(p_buf + 1) = (dev_name_length == adp_name_length) ? '\x09' : '\x08';
    // Copy device name
    memcpy(p_buf + 2, p_adp_name, dev_name_length);

    // Update advertising data length
    rem_len -= (dev_name_length + 2);
    p_buf += (dev_name_length + 2);

    while (rem_len >= 4) {
        // add manufacturer data
        // length, type(0xFF), 2 bytes company id
        manu_len = rem_len < 0xFF ? (uint8_t)rem_len : 0xFF;
        *p_buf++ = manu_len - 1;    // length
        *p_buf++ = 0xFF;            // type
        *p_buf++ = 0x2B;            // company id
        *p_buf++ = 0x0C;            // company id
        for (i = 0; i < (manu_len - 4); i++) {
            *p_buf++ = (uint8_t)(rand() & 0xFF);
        };

        rem_len -= manu_len;
    }

    return (max_length - rem_len);
}

/*!
    \brief      Start advertising
    \param[in]  p_adv: pointer to BLE advertising set
    \param[out] none
    \retval     none
*/
static void app_adv_start(app_adv_set_t *p_adv)
{
    ble_adv_data_set_t adv;
    ble_adv_data_set_t scan_rsp;
    ble_adv_data_set_t per_adv;
    uint8_t *p_adp_name = NULL;
    uint8_t adp_name_length;

    adv.data_force = true;
    scan_rsp.data_force = true;
    per_adv.data_force = true;
    adp_name_length = app_adp_get_name(&p_adp_name);


    switch (app_adv_env.adv_data_type) {
    case 1: {
        ble_data_t adv_data;
        ble_data_t per_adv_data;

        adv_data.len = 7;
        adv_data.p_data = adv_data_1;

        per_adv_data.len = 52;
        per_adv_data.p_data = per_data_1;

        adv.data.p_data_force = &adv_data;
        scan_rsp.data.p_data_force = &adv_data;
        per_adv.data.p_data_force = &per_adv_data;

        ble_adv_start(p_adv->idx, &adv, &scan_rsp, &per_adv);
    }
    break;

    case 2: {
        ble_data_t adv_data;
        ble_data_t sr_data;
        uint8_t *p_buf;
        uint16_t len;

        adv_data.len = 7;
        adv_data.p_data = adv_data_1;
        adv.data.p_data_force = &adv_data;

        p_buf = sys_malloc(p_adv->max_data_len);
        if (p_buf) {
            len = app_build_adv_data(p_adv, p_buf, p_adv->max_data_len, false);

            sr_data.p_data = p_buf;
            sr_data.len = len;

            scan_rsp.data.p_data_force = &sr_data;
            per_adv.data.p_data_force = &sr_data;

            ble_adv_start(p_adv->idx, &adv, &scan_rsp, &per_adv);

            sys_mfree(p_buf);
        }
    }
    break;

    case 3: {
        ble_data_t adv_data;

        adv_data.len = 25;
        adv_data.p_data = bqb_adv_data_1;

        adv.data.p_data_force = &adv_data;

        ble_adv_start(p_adv->idx, &adv, NULL, NULL);
    }
    break;

    case 4: {
        ble_data_t adv_data;
        ble_data_t scan_rsp_data;
        ble_data_t per_adv_data;

        adv_data.len = 24;
        adv_data.p_data = bqb_adv_data_2;

        scan_rsp_data.len = 27;
        scan_rsp_data.p_data = bqb_scan_rsp_data_2;

        per_adv_data.len = 27;
        per_adv_data.p_data = bqb_per_adv_data_2;

        adv.data.p_data_force = &adv_data;
        scan_rsp.data.p_data_force = &scan_rsp_data;
        per_adv.data.p_data_force = &per_adv_data;

        ble_adv_start(p_adv->idx, &adv, &scan_rsp, &per_adv);
    }
    break;

    case 5: {
        ble_data_t adv_data;

        adv_data.len = 22;
        adv_data.p_data = bqb_adv_data_3;

        adv.data.p_data_force = &adv_data;

        ble_adv_start(p_adv->idx, &adv, NULL, NULL);
    }
    break;

    case 6: {
        ble_data_t adv_data;

        adv_data.len = 28;
        adv_data.p_data = bqb_adv_data_4;

        adv.data.p_data_force = &adv_data;

        ble_adv_start(p_adv->idx, &adv, NULL, NULL);
    }
    break;

    case 7: {
        ble_data_t adv_data;

        adv_data.len = sizeof(adv_data_hogp);
        adv_data.p_data = adv_data_hogp;

        adv.data.p_data_force = &adv_data;

        ble_adv_start(p_adv->idx, &adv, NULL, NULL);
    }
    break;

    case 8: {
        uint8_t i;
        uint8_t s_data[3] = {0x00, 0x11, 0x22};
        int8_t tx_pwr = -80;
        ble_adv_data_conn_intv_t slave_conn_intv;
        ble_adv_data_srv_data_t srv_data[2];
        ble_adv_data_manuf_data_t manuf_data;
        ble_uuid_t uuid_more[2];
        ble_uuid_t uuid_cmpl[3];
        ble_uuid_t uuid_solicited[6];
        ble_adv_data_t adv_data;

        memset(&adv_data, 0, sizeof(ble_adv_data_t));

        adv_data.local_name.type = BLE_ADV_DATA_FULL_NAME;
        adv_data.local_name.name_len = adp_name_length;
        adv_data.local_name.p_name = (uint8_t *)p_adp_name;

        if (p_adv->disc_mode == BLE_GAP_ADV_MODE_LIM_DISC) {
            adv_data.flags = BLE_GAP_ADV_FLAG_LE_ONLY_LIMITED_DISC_MODE;
        }

        else {
            adv_data.flags = BLE_GAP_ADV_FLAG_LE_ONLY_GENERAL_DISC_MODE;
        }

        adv_data.appearance = BLE_APPEARANCE_GENERIC_REMOTE_CONTROL;
        adv_data.p_tx_pwr = &tx_pwr;

        adv_data.p_pub_tgt_addr = p_adv->peer_addr.addr;
        adv_data.p_rand_tgt_addr = p_adv->peer_addr.addr;

        adv_data.adv_intv = 0x00A0;

        slave_conn_intv.min_conn_intv = 0x0010;
        slave_conn_intv.max_conn_intv = 0x0100;
        adv_data.p_slave_conn_intv = &slave_conn_intv;

        uuid_more[0].type = BLE_UUID_TYPE_16;
        uuid_more[0].data.uuid_16 = 0x1234;
        uuid_more[1].type = BLE_UUID_TYPE_16;
        uuid_more[1].data.uuid_16 = 0x5678;
        adv_data.uuid_more_available.uuid_cnt = 2;
        adv_data.uuid_more_available.p_uuid = uuid_more;

        uuid_cmpl[0].type = BLE_UUID_TYPE_16;
        uuid_cmpl[0].data.uuid_16 = 0x1234;
        uuid_cmpl[1].type = BLE_UUID_TYPE_32;
        uuid_cmpl[1].data.uuid_32 = 0x56789ABC;
        uuid_cmpl[2].type = BLE_UUID_TYPE_128;
        for (i = 0; i < 16; i++) {
            uuid_cmpl[2].data.uuid_128[i] = i;
        }
        adv_data.uuid_complete.uuid_cnt = 3;
        adv_data.uuid_complete.p_uuid = uuid_cmpl;

        uuid_solicited[0].type = BLE_UUID_TYPE_16;
        uuid_solicited[0].data.uuid_16 = 0x1234;
        uuid_solicited[1].type = BLE_UUID_TYPE_32;
        uuid_solicited[1].data.uuid_32 = 0x11223344;
        uuid_solicited[2].type = BLE_UUID_TYPE_16;
        uuid_solicited[2].data.uuid_16 = 0x5678;
        uuid_solicited[3].type = BLE_UUID_TYPE_32;
        uuid_solicited[3].data.uuid_32 = 0x55667788;
        uuid_solicited[4].type = BLE_UUID_TYPE_16;
        uuid_solicited[4].data.uuid_16 = 0xABCD;
        uuid_solicited[5].type = BLE_UUID_TYPE_32;
        uuid_solicited[5].data.uuid_32 = 0xAABBCCDD;
        adv_data.uuid_solicited.uuid_cnt = 6;
        adv_data.uuid_solicited.p_uuid = uuid_solicited;

        srv_data[0].uuid.type = BLE_UUID_TYPE_16;
        srv_data[0].uuid.data.uuid_16 = 0x1199;
        srv_data[0].data_len = 8;
        srv_data[0].p_data = service_data;
        srv_data[1].uuid.type = BLE_UUID_TYPE_32;
        srv_data[1].uuid.data.uuid_32 = 0x12345678;
        srv_data[1].data_len = 3;
        srv_data[1].p_data = s_data;
        adv_data.srv_data.cnt = 2;
        adv_data.srv_data.p_data = srv_data;

        adv_data.url.url_len = 25;
        adv_data.url.p_url = url;

        manuf_data.company_id = 0x0C2B;
        manuf_data.data_len = 25;
        manuf_data.p_data = url;
        adv_data.p_manuf_specific_data = &manuf_data;

        adv.data_force = false;
        adv.data.p_data_enc = &adv_data;

        ble_adv_start(p_adv->idx, &adv, NULL, NULL);
    }
    break;

    default: {
        ble_data_t adv_data;
        ble_data_t sr_data;
        uint8_t *p_buf;
        uint16_t len;

        p_buf = sys_malloc(p_adv->max_data_len);
        if (p_buf) {
            len = app_build_adv_data(p_adv, p_buf, p_adv->max_data_len, true);
            adv_data.len = len;
            adv_data.p_data = p_buf;

            adv.data.p_data_force = &adv_data;
            per_adv.data.p_data_force = &adv_data;

            sr_data.p_data = NULL;
            sr_data.len = 0;

            scan_rsp.data.p_data_force = &sr_data;

            if (app_adv_env.cus_adv.data_force
                && app_adv_env.cus_adv.data.p_data_force
                && app_adv_env.cus_adv.data.p_data_force->p_data
                && app_adv_env.cus_adv.data.p_data_force->len) {
                adv_data.p_data = app_adv_env.cus_adv.data.p_data_force->p_data;
                adv_data.len = app_adv_env.cus_adv.data.p_data_force->len;
            }

            if (app_adv_env.cus_scan_rsp.data_force
                && app_adv_env.cus_scan_rsp.data.p_data_force
                && app_adv_env.cus_scan_rsp.data.p_data_force->p_data
                && app_adv_env.cus_scan_rsp.data.p_data_force->len) {
                sr_data.p_data = app_adv_env.cus_scan_rsp.data.p_data_force->p_data;
                sr_data.len = app_adv_env.cus_scan_rsp.data.p_data_force->len;
            }

            ble_adv_start(p_adv->idx, &adv, &scan_rsp, &per_adv);

            sys_mfree(p_buf);
        }
    }
    break;
    }
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
    app_adv_set_t *p_adv = (app_adv_set_t *)p_context;

    switch (adv_evt) {
    case BLE_ADV_EVT_OP_RSP: {
        ble_adv_op_rsp_t *p_rsp = (ble_adv_op_rsp_t *)p_data;

        if (p_rsp->status) {
            dbg_print(NOTICE, "adv op rsp, op code 0x%x, status 0x%x\r\n", p_rsp->op, p_rsp->status);
        }
    } break;

    case BLE_ADV_EVT_STATE_CHG: {
        ble_adv_state_chg_t *p_chg = (ble_adv_state_chg_t *)p_data;
        ble_adv_state_t old_state = p_adv->state;

        dbg_print(NOTICE, "adv state change 0x%x ==> 0x%x, reason 0x%x\r\n", old_state, p_chg->state, p_chg->reason);

        p_adv->state = p_chg->state;

        if ((p_chg->state == BLE_ADV_STATE_CREATE) && (old_state == BLE_ADV_STATE_CREATING)) {
            p_adv->idx = p_chg->adv_idx;
            dbg_print(NOTICE, "adv index %d\r\n", p_adv->idx);

            app_adv_start(p_adv);
        } else if ((p_chg->state == BLE_ADV_STATE_CREATE) && (old_state == BLE_ADV_STATE_START)) {
            dbg_print(NOTICE, "adv stopped, remove %d\r\n", p_adv->remove_after_stop);

            if (p_adv->remove_after_stop) {
                ble_adv_remove(p_adv->idx);
                p_adv->remove_after_stop = false;
            }
        } else if (p_chg->state == BLE_ADV_STATE_IDLE) {
            app_free_adv_set(p_adv);
        }
    } break;

    case BLE_ADV_EVT_DATA_UPDATE_INFO: {
        ble_adv_data_update_info_t *p_info = (ble_adv_data_update_info_t *)p_data;

        dbg_print(NOTICE, "adv data update info, type %d, status 0x%x\r\n", p_info->type, p_info->status);
    } break;

    case BLE_ADV_EVT_SCAN_REQ_RCV: {
        ble_adv_scan_req_rcv_t *p_req = (ble_adv_scan_req_rcv_t *)p_data;

        dbg_print(NOTICE, "scan req rcv, device addr %02X:%02X:%02X:%02X:%02X:%02X\r\n",
               p_req->peer_addr.addr[5], p_req->peer_addr.addr[4], p_req->peer_addr.addr[3],
               p_req->peer_addr.addr[2], p_req->peer_addr.addr[1], p_req->peer_addr.addr[0]);
    } break;

    default:
        break;
    }
}

/*!
    \brief      Set advertising data
    \param[in]  p_data: pointer to data to set to adv data
    \param[in]  len: data length
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t app_adv_set_adv_data(uint8_t *p_data, uint16_t len)
{
    if (len > BLE_GAP_LEGACY_ADV_MAX_LEN)
        return BLE_ERR_NO_RESOURCES;

    app_adv_env.cus_adv.data_force = true;
    if (!app_adv_env.cus_adv.data.p_data_enc) {
        app_adv_env.cus_adv.data.p_data_enc = sys_malloc(sizeof(ble_adv_data_t));
        if (!app_adv_env.cus_adv.data.p_data_enc)
            return BLE_ERR_NO_MEM_AVAIL;
    }

    sys_memset(app_adv_env.cus_adv.data.p_data_force, 0, sizeof(ble_adv_data_t));

    if (!app_adv_env.cus_adv.data.p_data_force->p_data) {
        app_adv_env.cus_adv.data.p_data_force->p_data = sys_malloc(BLE_GAP_LEGACY_ADV_MAX_LEN);
        if (!app_adv_env.cus_adv.data.p_data_force->p_data)
            return BLE_ERR_NO_MEM_AVAIL;
    }

    sys_memset(app_adv_env.cus_adv.data.p_data_force->p_data, 0, BLE_GAP_LEGACY_ADV_MAX_LEN);
    sys_memcpy(app_adv_env.cus_adv.data.p_data_force->p_data, p_data, len);
    app_adv_env.cus_adv.data.p_data_force->len = len;

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Set scan response data
    \param[in]  p_data: pointer to data to set to scan response data
    \param[in]  len: data length
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t app_adv_set_scan_rsp_data(uint8_t *p_data, uint16_t len)
{
    if (len > BLE_GAP_LEGACY_ADV_MAX_LEN)
        return BLE_ERR_NO_RESOURCES;

    app_adv_env.cus_scan_rsp.data_force = true;
    if (!app_adv_env.cus_scan_rsp.data.p_data_enc) {
        app_adv_env.cus_scan_rsp.data.p_data_enc = sys_malloc(sizeof(ble_adv_data_t));
        if (!app_adv_env.cus_scan_rsp.data.p_data_enc)
            return BLE_ERR_NO_MEM_AVAIL;
    }

    sys_memset(app_adv_env.cus_scan_rsp.data.p_data_force, 0, sizeof(ble_adv_data_t));

    if (!app_adv_env.cus_scan_rsp.data.p_data_force->p_data) {
        app_adv_env.cus_scan_rsp.data.p_data_force->p_data = sys_malloc(BLE_GAP_LEGACY_ADV_MAX_LEN);
        if (!app_adv_env.cus_scan_rsp.data.p_data_force->p_data)
            return BLE_ERR_NO_MEM_AVAIL;
    }

    sys_memset(app_adv_env.cus_scan_rsp.data.p_data_force->p_data, 0, BLE_GAP_LEGACY_ADV_MAX_LEN);
    sys_memcpy(app_adv_env.cus_scan_rsp.data.p_data_force->p_data, p_data, len);
    app_adv_env.cus_scan_rsp.data.p_data_force->len = len;

    return BLE_ERR_NO_ERROR;
}

/*!
    \brief      Create an advertising
    \param[in]  p_param: pointer to advertising parameters
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t app_adv_create(app_adv_param_t *p_param)
{
    app_adv_set_t *p_adv;
    ble_adv_param_t adv_param = {0};

    p_adv = app_get_free_adv();
    if (p_adv == NULL) {
        return BLE_ERR_NO_RESOURCES;
    }

    p_adv->max_data_len = p_param->max_data_len;

    adv_param.param.own_addr_type = p_param->own_addr_type;

    if (p_param->type == BLE_ADV_TYPE_LEGACY) {
        adv_param.param.type = BLE_GAP_ADV_TYPE_LEGACY;
        adv_param.param.prop = p_param->prop;

        if (p_param->wl_enable) {
            adv_param.param.filter_pol = BLE_GAP_ADV_ALLOW_SCAN_FAL_CON_FAL;
            adv_param.param.disc_mode = BLE_GAP_ADV_MODE_NON_DISC;
        } else {
            adv_param.param.filter_pol = BLE_GAP_ADV_ALLOW_SCAN_ANY_CON_ANY;
            adv_param.param.disc_mode = p_param->disc_mode;
        }

        adv_param.param.ch_map = p_param->ch_map;
        adv_param.param.primary_phy = p_param->pri_phy;
    } else if (p_param->type == BLE_ADV_TYPE_EXTENDED) {
        adv_param.param.type = BLE_GAP_ADV_TYPE_EXTENDED;
        adv_param.param.prop = p_param->prop;

        if (p_param->wl_enable) {
            adv_param.param.filter_pol = BLE_GAP_ADV_ALLOW_SCAN_FAL_CON_FAL;
            adv_param.param.disc_mode = BLE_GAP_ADV_MODE_NON_DISC;
        } else {
            adv_param.param.filter_pol = BLE_GAP_ADV_ALLOW_SCAN_ANY_CON_ANY;
            adv_param.param.disc_mode = p_param->disc_mode;
        }

        adv_param.param.ch_map = p_param->ch_map;
        adv_param.param.primary_phy = p_param->pri_phy;
        adv_param.param.adv_sid = app_get_adv_sid();
        adv_param.param.max_skip = 0x00;
        adv_param.param.secondary_phy = p_param->sec_phy;
    }
#if (BLE_APP_PER_ADV_SUPPORT)
    else if (p_param->type == BLE_ADV_TYPE_PERIODIC) {
        adv_param.param.type = BLE_GAP_ADV_TYPE_PERIODIC;
        adv_param.param.prop = p_param->prop;
        adv_param.param.filter_pol = BLE_GAP_ADV_ALLOW_SCAN_ANY_CON_ANY;
        adv_param.param.disc_mode = BLE_GAP_ADV_MODE_GEN_DISC;
        adv_param.param.ch_map = p_param->ch_map;
        adv_param.param.primary_phy = p_param->pri_phy;
        adv_param.param.adv_sid = app_get_adv_sid();
        adv_param.param.max_skip = 0x00;
        adv_param.param.secondary_phy = p_param->sec_phy;
        adv_param.param.per_intv_min = 80;    // 100ms
        adv_param.param.per_intv_max = 80;    // 100ms
    }
#endif // (BLE_PERIODIC_ADV)
    else {
        return BLE_GAP_ERR_INVALID_PARAM;
    }

    if (adv_param.param.prop & BLE_GAP_ADV_PROP_DIRECTED_BIT) {
        adv_param.param.peer_addr = p_param->peer_addr;
        adv_param.param.disc_mode = BLE_GAP_ADV_MODE_NON_DISC;
        p_adv->peer_addr = p_param->peer_addr;
    }

    if (adv_param.param.prop & BLE_GAP_ADV_PROP_ANONYMOUS_BIT) {
        adv_param.param.disc_mode = BLE_GAP_ADV_MODE_NON_DISC;
    }

    p_adv->disc_mode = adv_param.param.disc_mode;

    adv_param.param.adv_intv_min = p_param->adv_intv;
    adv_param.param.adv_intv_max = p_param->adv_intv;

    if (p_adv->disc_mode == BLE_GAP_ADV_MODE_LIM_DISC) {
        adv_param.param.duration = 1000;       // 10s
    }

    if (p_param->type != BLE_ADV_TYPE_LEGACY) {
        adv_param.include_tx_pwr = true;
        adv_param.scan_req_ntf = true;
    }

    adv_param.param.max_tx_pwr = BLE_GAP_ADV_TX_PWR_NO_PREF;
    p_adv->remove_after_stop = true;

    return ble_adv_create(&adv_param, app_adv_mgr_evt_hdlr, p_adv);
}

/*!
    \brief      Stop an advertising if it is started
    \param[in]  idx: local advertising set index
    \param[in]  rmv_adv: true to remove advertising set after it is stopped, otherwise false
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t app_adv_stop(uint8_t idx, bool rmv_adv)
{
    uint8_t i;
    app_adv_set_t *p_adv = NULL;
    ble_status_t ret = BLE_ERR_NO_ERROR;

    for (i = 0; i < APP_ADV_SET_NUM; i++) {
        if (app_adv_env.adv_set[i].idx == idx) {
            p_adv = &app_adv_env.adv_set[i];
            break;
        }
    }

    if (p_adv == NULL) {
        return BLE_GAP_ERR_INVALID_PARAM;
    }

    if (p_adv->state == BLE_ADV_STATE_START) {
        ret = ble_adv_stop(p_adv->idx);
        if (ret == BLE_ERR_NO_ERROR) {
            p_adv->remove_after_stop = rmv_adv;
        }
    } else if (rmv_adv) {
        ret = ble_adv_remove(p_adv->idx);
    }

    return ret;
}

/*!
    \brief      Restart an advertising if it is stopped
    \param[in]  idx: local advertising set index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t app_adv_restart(uint8_t idx)
{
    uint8_t i;
    app_adv_set_t *p_adv = NULL;

    for (i = 0; i < APP_ADV_SET_NUM; i++) {
        if (app_adv_env.adv_set[i].idx == idx) {
            p_adv = &app_adv_env.adv_set[i];
            break;
        }
    }

    if (p_adv == NULL) {
        return BLE_GAP_ERR_INVALID_PARAM;
    }

    return ble_adv_restart(p_adv->idx);
}

/*!
    \brief      Update advertising data
    \param[in]  idx: local advertising set index
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t app_adv_data_update(uint8_t idx)
{
    uint8_t i;
    app_adv_set_t *p_adv = NULL;
    ble_adv_data_set_t adv;
    ble_adv_data_set_t scan_rsp;
    ble_adv_data_set_t per_adv;
    ble_data_t adv_data;
    ble_data_t sr_data;
    uint8_t *p_buf;
    uint16_t len;
    ble_status_t ret;

    for (i = 0; i < APP_ADV_SET_NUM; i++) {
        if (app_adv_env.adv_set[i].idx == idx) {
            p_adv = &app_adv_env.adv_set[i];
            break;
        }
    }

    if (p_adv == NULL)
        return BLE_ERR_NO_RESOURCES;

    if (p_adv->state == BLE_ADV_STATE_START) {
        len = (p_adv->max_data_len > 251) ? 251 : p_adv->max_data_len;
    } else {
        len = p_adv->max_data_len;
    }

    p_buf = sys_malloc(len);

    if (p_buf) {
        len = app_build_adv_data(p_adv, p_buf, len, true);
        adv_data.len = len;
        adv_data.p_data = p_buf;

        adv.data_force = true;
        adv.data.p_data_force = &adv_data;

        per_adv.data_force = true;
        per_adv.data.p_data_force = &adv_data;

        if (len > 3) {
            sr_data.p_data = p_buf + 3;
            sr_data.len = len - 3;
        } else {
            sr_data.p_data = NULL;
            sr_data.len = 0;
        }
        scan_rsp.data_force = true;
        scan_rsp.data.p_data_force = &sr_data;

        if (app_adv_env.cus_adv.data_force
            && app_adv_env.cus_adv.data.p_data_force
            && app_adv_env.cus_adv.data.p_data_force->p_data
            && app_adv_env.cus_adv.data.p_data_force->len) {
            adv_data.p_data = app_adv_env.cus_adv.data.p_data_force->p_data;
            adv_data.len = app_adv_env.cus_adv.data.p_data_force->len;
        }

        if (app_adv_env.cus_scan_rsp.data_force
            && app_adv_env.cus_scan_rsp.data.p_data_force
            && app_adv_env.cus_scan_rsp.data.p_data_force->p_data
            && app_adv_env.cus_scan_rsp.data.p_data_force->len) {
            sr_data.p_data = app_adv_env.cus_scan_rsp.data.p_data_force->p_data;
            sr_data.len = app_adv_env.cus_scan_rsp.data.p_data_force->len;
        }

        if (p_adv->state == BLE_ADV_STATE_CREATE)
            ret = ble_adv_start(p_adv->idx, &adv, &scan_rsp, &per_adv);
        else if (p_adv->state == BLE_ADV_STATE_START)
            ret = ble_adv_data_update(p_adv->idx, &adv, &scan_rsp, &per_adv);
        else
            ret = BLE_ERR_PROCESSING;

        sys_mfree(p_buf);
    } else {
        ret = BLE_ERR_NO_MEM_AVAIL;
    }

    return ret;
}

/*!
    \brief      Update advertising data for all advertising set
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_adv_data_update_all(void)
{
    uint8_t i = 0;

    for (i = 0; i < APP_ADV_SET_NUM; i++) {
        app_adv_data_update(i);
    }
}

/*!
    \brief      Choose advertising data to be used
    \param[in]  adv_data_type: advertising data type index
    \param[out] none
    \retval     none
*/
void app_set_adv_data_type(uint8_t adv_data_type)
{
    app_adv_env.adv_data_type = adv_data_type;
}

/*!
    \brief      Reset APP advertising manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_adv_mgr_reset(void)
{
    uint8_t i;
    memset(app_adv_env.adv_set, 0, APP_ADV_SET_NUM * sizeof(app_adv_set_t));
    for (i = 0; i < APP_ADV_SET_NUM; i++) {
        app_adv_env.adv_set[i].idx = ADV_INVALID_IDX;
        app_adv_env.adv_set[i].type = DEFAULT_ADV_TYPE;
        app_adv_env.adv_set[i].prop = 0x0000;
        app_adv_env.adv_set[i].pri_phy = BLE_GAP_PHY_1MBPS;
        app_adv_env.adv_set[i].sec_phy = BLE_GAP_PHY_1MBPS;
        app_adv_env.adv_set[i].wl_enable = false;
        app_adv_env.adv_set[i].own_addr_type = BLE_GAP_LOCAL_ADDR_STATIC;
        app_adv_env.adv_set[i].remove_after_stop = false;
    }
}

void app_adv_conn_evt_handler(ble_conn_evt_t event, ble_conn_data_u *p_data)
{
    app_adv_param_t adv_param = {0};
    ble_status_t ret;

#if (FEAT_SUPPORT_ADV_AFTER_DISCONN)
    if (event == BLE_CONN_EVT_STATE_CHG && p_data->conn_state.state == BLE_CONN_STATE_DISCONNECTD) {
        adv_param.type = BLE_ADV_TYPE_LEGACY;
        adv_param.prop = BLE_GAP_ADV_PROP_UNDIR_CONN;           // 0x0003,scannable connectable undirected
        adv_param.adv_intv = APP_ADV_INT_MAX;
        adv_param.ch_map = BLE_GAP_ADV_CHANN_37 | BLE_GAP_ADV_CHANN_38 | BLE_GAP_ADV_CHANN_39;
        adv_param.max_data_len = 0x1F;
        adv_param.pri_phy = BLE_GAP_PHY_1MBPS;
        adv_param.sec_phy = BLE_GAP_PHY_1MBPS;
        adv_param.wl_enable = false;
        adv_param.own_addr_type = BLE_GAP_LOCAL_ADDR_STATIC;
        adv_param.disc_mode = BLE_GAP_ADV_MODE_GEN_DISC;

        ret = app_adv_create(&adv_param);

        if (ret != BLE_ERR_NO_ERROR) {
            dbg_print(NOTICE, "create adv fail status 0x%x\r\n", ret);
        }
    }
#endif
}

/*!
    \brief      Init APP advertising manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_adv_mgr_init(void)
{
    app_adv_mgr_reset();
    ble_conn_callback_register(app_adv_conn_evt_handler);
}

/*!
    \brief      Deinit APP advertising manager module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_adv_mgr_deinit(void)
{
    app_adv_mgr_reset();
    ble_conn_callback_unregister(app_adv_conn_evt_handler);
}

#endif // (BLE_APP_SUPPORT && (BLE_CFG_ROLE & (BLE_CFG_ROLE_BROADCASTER | BLE_CFG_ROLE_PERIPHERAL)))
