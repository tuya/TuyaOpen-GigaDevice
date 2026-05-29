/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2024, GigaDevice Semiconductor Inc.
 */

#include <stdint.h>

#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "blufi_int.h"

#include "ble_error.h"
#include "ble_export.h"
#include "ble_conn.h"
#include "dbg_print.h"
#include "wrapper_os.h"
#include "wifi_management.h"
#include "wifi_vif.h"
#include "wifi_net_ip.h"
#include "dhcpd.h"
#include "blufi_adapter.h"

/* Wifi information */
typedef struct {
    int8_t          sta_rssi;
    uint8_t         sta_bssid[MAC_ADDR_LEN];
    struct mac_ssid sta_ssid;
    uint8_t sta_password[WPA_MAX_PSK_LEN + 1];
    uint8_t sta_password_len;
    uint8_t         ap_status;
    struct mac_ssid ap_ssid;
    uint8_t ap_password[WPA_MAX_PSK_LEN + 1];
    uint8_t ap_password_len;
    uint8_t auth_mode;
    uint8_t channel;
} blufi_wifi_t;

blufi_wifi_t blufi_wifi = {0};

/*!
    \brief      Get wifi scan result information after scan is complete
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void blufi_wifi_scan_list_get(void)
{
    struct macif_scan_results *results;
    struct mac_scan_result *result;
    uint16_t j;
    esp_blufi_ap_record_t *ap_record;
    dbg_print(NOTICE, "[Scanned AP list]\r\n");

    results = (struct macif_scan_results *)sys_malloc(sizeof(struct macif_scan_results));
    if (NULL == results)
        return;

    if (wifi_netlink_scan_results_get(WIFI_VIF_INDEX_DEFAULT, results)) {
        btc_blufi_send_error_info(ESP_BLUFI_WIFI_SCAN_FAIL);
        sys_mfree(results);
        return;
    }

    ap_record = (esp_blufi_ap_record_t *)sys_malloc(sizeof(esp_blufi_ap_record_t) * results->result_cnt);
    for (j = 0; j < results->result_cnt; j++) {
        result = &results->result[j];
        if (result->ssid.length == 0)
            continue;

        dbg_print(NOTICE, "(%d dBm) SSID=%s \r\n", result->rssi, (char *)result->ssid.array);
        sys_memcpy(ap_record[j].ssid, result->ssid.array, result->ssid.length);
        ap_record[j].ssid[result->ssid.length] = '\0';
        ap_record[j].rssi = result->rssi;
    }

    btc_blufi_send_wifi_list(results->result_cnt, ap_record);

    dbg_print(NOTICE, "[scan finished, Scanned AP number: %d]\r\n", results->result_cnt);
    sys_mfree(results);
    sys_mfree(ap_record);
}

/*!
    \brief      Blue courier get wifi status information
    \param[in]  none
    \param[out] none
    \retval     none
*/
void blufi_wifi_status_get(void)
{
    struct wifi_ip_addr_cfg ip_cfg;
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(WIFI_VIF_INDEX_DEFAULT);
    struct mac_addr cli_mac[CFG_STA_NUM];
    uint32_t i, ip;
    esp_blufi_extra_info_t info = {0};
    uint8_t sta_conn_state = ESP_BLUFI_STA_CONN_FAIL;
    uint8_t softap_conn_num = 0;

    if ((blufi_env.wifi_mode == 1) && wvif->wvif_type == WVIF_STA) {
        info.sta_bssid_set = true;
        sys_memcpy(info.sta_bssid, wvif->sta.cfg.bssid, MAC_ADDR_LEN);
        info.sta_conn_rssi_set = true;
        info.sta_conn_rssi = macif_vif_sta_rssi_get(WIFI_VIF_INDEX_DEFAULT);

        info.sta_ssid = blufi_wifi.sta_ssid.array;
        info.sta_ssid_len = blufi_wifi.sta_ssid.length;
        info.sta_passwd = blufi_wifi.sta_password;
        info.sta_passwd_len = blufi_wifi.sta_password_len;
        sta_conn_state = wvif->sta.state == WIFI_STA_STATE_CONNECTED ? ESP_BLUFI_STA_CONN_SUCCESS : sta_conn_state;
    } else if ((blufi_env.wifi_mode == 2) && wvif->wvif_type == WVIF_AP) {
        info.softap_ssid = blufi_wifi.ap_ssid.array;
        info.softap_ssid_len = blufi_wifi.ap_ssid.length;
        info.softap_passwd = blufi_wifi.ap_password;
        info.softap_passwd_len = blufi_wifi.ap_password_len;
        info.softap_authmode_set = true;
        info.softap_authmode = blufi_wifi.auth_mode;
        info.softap_channel_set = true;
        info.softap_channel = blufi_wifi.channel;
        softap_conn_num = macif_vif_ap_assoc_info_get(WIFI_VIF_INDEX_DEFAULT, (uint16_t *)&cli_mac);
    }

    btc_blufi_wifi_conn_report(blufi_env.wifi_mode, sta_conn_state, softap_conn_num, &info, 0);
}

/*!
    \brief      Wifi scan complete handler
    \param[in]  eloop_data: callback context data
    \param[in]  user_ctx: callback context data
    \param[out] none
    \retval     none
*/
static void blufi_cb_scan_fail(void *eloop_data, void *user_ctx);
static void blufi_cb_scan_done(void *eloop_data, void *user_ctx)
{
    blufi_wifi_scan_list_get();
    eloop_event_unregister(WIFI_MGMT_EVENT_SCAN_DONE, blufi_cb_scan_done);
    eloop_event_unregister(WIFI_MGMT_EVENT_SCAN_FAIL, blufi_cb_scan_fail);
}

/*!
    \brief      Wifi scan fail handler
    \param[in]  eloop_data: callback context data
    \param[in]  user_ctx: callback context data
    \param[out] none
    \retval     none
*/
static void blufi_cb_scan_fail(void *eloop_data, void *user_ctx)
{
    dbg_print(ERR, "ble config wifi scan cb failed\r\n");
    eloop_event_unregister(WIFI_MGMT_EVENT_SCAN_DONE, blufi_cb_scan_done);
    eloop_event_unregister(WIFI_MGMT_EVENT_SCAN_FAIL, blufi_cb_scan_fail);

    btc_blufi_send_error_info(ESP_BLUFI_WIFI_SCAN_FAIL);
}

/*!
    \brief      Blue courier triggers wifi scan in station mode
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void blufi_wifi_scan(void)
{
    eloop_event_register(WIFI_MGMT_EVENT_SCAN_DONE, blufi_cb_scan_done, NULL, NULL);
    eloop_event_register(WIFI_MGMT_EVENT_SCAN_FAIL, blufi_cb_scan_fail, NULL, NULL);

    if (wifi_management_scan(false, NULL) == -1) {
        eloop_event_unregister(WIFI_MGMT_EVENT_SCAN_DONE, blufi_cb_scan_done);
        eloop_event_unregister(WIFI_MGMT_EVENT_SCAN_FAIL, blufi_cb_scan_fail);
        dbg_print(ERR, "ble config wifi scan failed\r\n");
    }
}

/*!
    \brief      Wifi connect success handler
    \param[in]  eloop_data: callback context data
    \param[in]  user_ctx: callback context data
    \param[out] none
    \retval     none
*/
static void blufi_wifi_connect_success(void *eloop_data, void *user_ctx)
{
    uint8_t state = 0;

    eloop_event_unregister(WIFI_MGMT_EVENT_DHCP_SUCCESS, blufi_wifi_connect_success);
    blufi_wifi_status_get();
}

/*!
    \brief      Blue courier triggers wifi to connect AP in station mode
    \param[in]  data: pointer to wifi ssid and password information (1 byte ssid length + ssid + 1 byte passwaord length + password + 1 byte random)
    \param[in]  len: data length
    \param[out] none
    \retval     none
*/
void blufi_wifi_connect(void)
{
    uint8_t state = 1;
    uint8_t ssid_len, password_len;
    uint8_t wifi_ssid[MAC_SSID_LEN + 1];
    uint8_t wifi_password[WPA_MAX_PSK_LEN + 1];
    char *password = NULL;
    esp_blufi_extra_info_t info = {0};


    if (blufi_wifi.sta_password_len != 0) {
        password = (char *)blufi_wifi.sta_password;
    }

    eloop_event_register(WIFI_MGMT_EVENT_DHCP_SUCCESS, blufi_wifi_connect_success, NULL, NULL);
    if (wifi_management_connect((char *)blufi_wifi.sta_ssid.array, password, true)) {
        dbg_print(ERR, "ble config wifi connect failed\r\n");
        eloop_event_unregister(WIFI_MGMT_EVENT_DHCP_SUCCESS, blufi_wifi_connect_success);

        info.sta_ssid = blufi_wifi.sta_ssid.array;
        info.sta_ssid_len = blufi_wifi.sta_ssid.length;
        info.sta_passwd = blufi_wifi.sta_password;
        info.sta_passwd_len = blufi_wifi.sta_password_len;
        info.sta_conn_end_reason_set = true;
        info.sta_conn_end_reason = 205; // WIFI_REASON_CONNECTION_FAIL in wifi_err_reason_t

        btc_blufi_wifi_conn_report(blufi_env.wifi_mode, ESP_BLUFI_STA_CONN_FAIL, 0, &info, 0);
        blufi_env.wifi_mode = 0;
    }
}

/*!
    \brief      Blue courier triggers wifi to disconnect current connection in station mode
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void blufi_wifi_disconnect(void)
{
    uint8_t state = 0;
    wifi_management_disconnect();
    blufi_env.wifi_mode = 0;
    blufi_wifi_status_get();
}

/*!
    \brief      Blue courier triggers wifi to create AP in softAP mode
    \param[in]  data: pointer to wifi information(1byte ssid length + ssid + 1byte passwaord length + password + 1byte channel + 1byte akm + 1byte hide + 1byte random)
    \param[in]  len: data length
    \param[out] none
    \retval     none
*/
static void blufi_wifi_ap_start(void)
{
    uint8_t state = 1;
    uint8_t ssid_len, password_len, channel, akm, hide;
    uint8_t wifi_ssid[MAC_SSID_LEN + 1];
    uint8_t wifi_password[WPA_MAX_PSK_LEN + 1];
    char *password = NULL;
    wifi_ap_auth_mode_t auth_mode;

    if (blufi_wifi.ap_password_len != 0) {
        password = (char *)blufi_wifi.ap_password;
    }

    if (wifi_management_ap_start((char *)blufi_wifi.ap_ssid.array, password, blufi_wifi.channel, blufi_wifi.auth_mode, false)) {
        dbg_print(ERR, "blufi failed to start AP, check your configuration.\r\n");
        goto exit;
    }

    state = 0;
    dbg_print(NOTICE, "blufi softAP successfully started!\r\n");

    blufi_wifi_status_get();
    return;
exit:
    blufi_env.wifi_mode = 0;
    blufi_wifi_status_get();
}

/*!
    \brief      Blue courier triggers wifi to stop AP in softAP mode
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void blufi_wifi_ap_stop(void)
{
    uint8_t state = 0;

    if (wifi_management_ap_stop() == 0) {
        dbg_print(NOTICE, "blufi softAP successfully stoped!\r\n");
        blufi_env.wifi_mode = 0;
        blufi_wifi_status_get();
    } else {
        state = 1;
        blufi_wifi_status_get();
    }
}

void btc_blufi_protocol_handler(uint8_t type, uint8_t *data, int len)
{
    uint8_t *output_data = NULL;
    int output_len = 0;
    bool need_free = false;
    uint8_t *custom_data = NULL;

    dbg_print(INFO, "%s type %02x\n", __func__, type);

    switch (BLUFI_GET_TYPE(type)) {
    case BLUFI_TYPE_CTRL:
        switch (BLUFI_GET_SUBTYPE(type)) {
        case BLUFI_TYPE_CTRL_SUBTYPE_ACK:
            /* TODO: check sequence */
            break;
        case BLUFI_TYPE_CTRL_SUBTYPE_SET_SEC_MODE:
            blufi_env.sec_mode = data[0];
            break;
        case BLUFI_TYPE_CTRL_SUBTYPE_SET_WIFI_OPMODE:
            if (data[0] == 3) {
                dbg_print(ERR, "Unsupport sta&wifi\n");
                break;
            }
            if (data[0] == 0) {
                if ((blufi_env.wifi_mode == 1)) {
                    blufi_wifi_disconnect();
                } else if ((blufi_env.wifi_mode == 2)) {
                    blufi_wifi_ap_stop();
                }
            } else
                blufi_env.wifi_mode = data[0];
            break;
        case BLUFI_TYPE_CTRL_SUBTYPE_CONN_TO_AP:
            blufi_wifi_connect();
            break;
        case BLUFI_TYPE_CTRL_SUBTYPE_DISCONN_FROM_AP:
            blufi_wifi_disconnect();
            break;
        case BLUFI_TYPE_CTRL_SUBTYPE_GET_WIFI_STATUS:
            blufi_wifi_status_get();
            break;
        case BLUFI_TYPE_CTRL_SUBTYPE_DEAUTHENTICATE_STA:
            dbg_print(ERR, "Unsupport deauth station\n");
            // TODO
            break;
        case BLUFI_TYPE_CTRL_SUBTYPE_GET_VERSION: {
            uint8_t type = BLUFI_BUILD_TYPE(BLUFI_TYPE_DATA, BLUFI_TYPE_DATA_SUBTYPE_REPLY_VERSION);
            uint8_t data[2];

            data[0] = BTC_BLUFI_GREAT_VER;
            data[1] = BTC_BLUFI_SUB_VER;
            btc_blufi_send_encap(type, &data[0], sizeof(data));
            break;
        }
        case BLUFI_TYPE_CTRL_SUBTYPE_DISCONNECT_BLE:
            ble_conn_disconnect(blufi_adapter_env.conn_id, BLE_ERROR_HL_TO_HCI(BLE_LL_ERR_REMOTE_USER_TERM_CON));
            break;
        case BLUFI_TYPE_CTRL_SUBTYPE_GET_WIFI_LIST:
            blufi_wifi_scan();
            break;
        default:
            dbg_print(ERR, "%s Unkown Ctrl pkt %02x\n", __func__, type);
            break;
        }
        break;
    case BLUFI_TYPE_DATA:
        switch (BLUFI_GET_SUBTYPE(type)) {
        case BLUFI_TYPE_DATA_SUBTYPE_NEG:
            dbg_print(ERR, "Unsupport negotiate\n");
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_STA_BSSID:
            memcpy(blufi_wifi.sta_bssid, &data[0], 6);
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_STA_SSID:
            blufi_wifi.sta_ssid.length = len;
            memcpy(blufi_wifi.sta_ssid.array, &data[0], len);
            blufi_wifi.sta_ssid.array[blufi_wifi.sta_ssid.length] = '\0';
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_STA_PASSWD:
            blufi_wifi.sta_password_len = len;
            memcpy(blufi_wifi.sta_password, &data[0], len);
            blufi_wifi.sta_password[blufi_wifi.sta_password_len] = '\0';
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_SSID:
            blufi_wifi.ap_ssid.length = len;
            memcpy(blufi_wifi.ap_ssid.array, &data[0], len);
            blufi_wifi.ap_ssid.array[blufi_wifi.ap_ssid.length] = '\0';
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_PASSWD:
            blufi_wifi.ap_password_len = len;
            memcpy(blufi_wifi.ap_password, &data[0], len);
            blufi_wifi.ap_password[blufi_wifi.ap_password_len] = '\0';
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_MAX_CONN_NUM:
            /* Nothing */
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_AUTH_MODE:
            blufi_wifi.auth_mode = data[0];
            blufi_wifi_ap_start();
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_CHANNEL:
            blufi_wifi.channel = data[0];
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_USERNAME:
            /* TODO */
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_CA:
            /* TODO */
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_CLIENT_CERT:
            /* TODO */
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_SERVER_CERT:
            /* TODO */
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_CLIENT_PRIV_KEY:
            /* TODO */
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_SERVER_PRIV_KEY:
            /* TODO */
            break;
        case BLUFI_TYPE_DATA_SUBTYPE_CUSTOM_DATA:
            custom_data = (uint8_t *)sys_malloc(len + 1);
            sys_memcpy(custom_data, data, len);
            custom_data[len] = '\0';
            dbg_print(NOTICE, "blufi receive custom data: %s\r\n", custom_data);
            sys_mfree(custom_data);
            custom_data = NULL;
            break;
        default:
            dbg_print(ERR, "%s Unkown Ctrl pkt %02x\n", __func__, type);
            break;
        }
        break;
    default:
        break;
    }
}
