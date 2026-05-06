/*!
    \file    app_blue_courier_prot.c
    \brief   Implemetions of blue courier wifi protocol

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
#include "app_blue_courier_link.h"
#include "app_blue_courier.h"
#include "ble_error.h"
#include "ble_export.h"
#include "dbg_print.h"
#include "wrapper_os.h"
#include "wifi_management.h"
#include "wifi_vif.h"
#include "wifi_net_ip.h"
#include "dhcpd.h"

/* Wifi client information in softAP mode */
struct cli_info {
    uint8_t mac[MAC_ADDR_LEN];
    uint8_t ip[4];
};

/* Wifi status information */
typedef struct {
    uint8_t         vif_idx;
    uint8_t         vif_name[4];
    uint8_t         vif_mac[MAC_ADDR_LEN];
    uint8_t         mode;
    uint8_t         channel;
    uint8_t         bw;
    uint8_t         akm;
    uint8_t         ip_addr[4];
    uint8_t         gw[4];
    uint8_t         sta_status;
    int8_t          sta_rssi;
    uint8_t         sta_bssid[MAC_ADDR_LEN];
    struct mac_ssid sta_ssid;
    uint8_t         ap_status;
    struct mac_ssid ap_ssid;
    uint8_t         cli_num;
    struct cli_info cli[CFG_STA_NUM];
} bcwp_wifi_status_t;

static void bcwp_cb_scan_done(void *eloop_data, void *user_ctx);
static void bcwp_cb_scan_fail(void *eloop_data, void *user_ctx);

/*!
    \brief      Get wifi scan result information after scan is complete
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void bcwp_wifi_scan_list_get(void)
{
    struct macif_scan_results *results;
    struct mac_scan_result *result;
    uint8_t ntf_result[BCW_VALUE_LEN];
    uint16_t i = 0, j;
    uint8_t mode = 0;

    dbg_print(INFO, "[Scanned AP list]\r\n");

    results = (struct macif_scan_results *)sys_malloc(sizeof(struct macif_scan_results));
    if (NULL == results)
        return;

    if (wifi_netlink_scan_results_get(WIFI_VIF_INDEX_DEFAULT, results)) {
        sys_mfree(results);
        return;
    }

    for (j = 0; j < results->result_cnt; j++) {
        result = &results->result[j];
        if (result->ssid.length < sizeof(result->ssid.array))
            result->ssid.array[result->ssid.length] = '\0';
        else
            result->ssid.array[result->ssid.length - 1] = '\0';

        if (result->ssid.length == 0)
            continue;

        if (i + result->ssid.length + 3 > sizeof(ntf_result))
            break;

        dbg_print(INFO, "(%d dBm) SSID=%s ", result->rssi, (char *)result->ssid.array);

        if (result->akm & CO_BIT(MAC_AKM_NONE)) {
            dbg_print(INFO, "[OPEN]\n");
            mode = 0;
        } else if (result->akm == CO_BIT(MAC_AKM_PRE_RSN)) {
            dbg_print(INFO, "[WEP]\n");
            mode = 1;
        } else if (result->akm & CO_BIT(MAC_AKM_WAPI_CERT) || result->akm & CO_BIT(MAC_AKM_WAPI_PSK)) {
            dbg_print(INFO, "[WAPI]\n");
            mode = 2;
        } else if (result->akm & CO_BIT(MAC_AKM_PRE_RSN)) {
            dbg_print(INFO, "[WPA]\n");
            mode = 3;
        } else {
            dbg_print(INFO, "[RSN]\n");
            mode = 4;
        }

        // len + rssi + mode + ssid
        ntf_result[i++] = result->ssid.length + 2;
        ntf_result[i++] = result->rssi;
        ntf_result[i++] = mode;
        sys_memcpy(&ntf_result[i], result->ssid.array, result->ssid.length);
        i += result->ssid.length;
    }

    dbg_print(INFO, "[scan finished, Scanned AP number: %d]\r\n", results->result_cnt);
    sys_mfree(results);

    bcwl_send(BCWL_OPCODE_BUILD(BCWL_OPCODE_TYPE_DATA, BCWL_OPCODE_DATA_SUBTYPE_GET_SCAN_LIST), ntf_result, i);
}

/*!
    \brief      Wifi scan complete handler
    \param[in]  eloop_data: callback context data
    \param[in]  user_ctx: callback context data
    \param[out] none
    \retval     none
*/
static void bcwp_cb_scan_done(void *eloop_data, void *user_ctx)
{
    bcwp_wifi_scan_list_get();
    eloop_event_unregister(WIFI_MGMT_EVENT_SCAN_DONE, bcwp_cb_scan_done);
    eloop_event_unregister(WIFI_MGMT_EVENT_SCAN_FAIL, bcwp_cb_scan_fail);
}

/*!
    \brief      Wifi scan fail handler
    \param[in]  eloop_data: callback context data
    \param[in]  user_ctx: callback context data
    \param[out] none
    \retval     none
*/
static void bcwp_cb_scan_fail(void *eloop_data, void *user_ctx)
{
    dbg_print(ERR, "ble config wifi scan cb failed\r\n");
    eloop_event_unregister(WIFI_MGMT_EVENT_SCAN_DONE, bcwp_cb_scan_done);
    eloop_event_unregister(WIFI_MGMT_EVENT_SCAN_FAIL, bcwp_cb_scan_fail);
}

/*!
    \brief      Blue courier triggers wifi scan in station mode
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void bcwp_wifi_scan(void)
{
    eloop_event_register(WIFI_MGMT_EVENT_SCAN_DONE, bcwp_cb_scan_done, NULL, NULL);
    eloop_event_register(WIFI_MGMT_EVENT_SCAN_FAIL, bcwp_cb_scan_fail, NULL, NULL);

    if (wifi_management_scan(false, NULL) == -1) {
    eloop_event_unregister(WIFI_MGMT_EVENT_SCAN_DONE, bcwp_cb_scan_done);
    eloop_event_unregister(WIFI_MGMT_EVENT_SCAN_FAIL, bcwp_cb_scan_fail);
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
static void bcwp_wifi_connect_success(void *eloop_data, void *user_ctx)
{
    uint8_t state = 0;

    eloop_event_unregister(WIFI_MGMT_EVENT_DHCP_SUCCESS, bcwp_wifi_connect_success);
    bcwl_send(BCWL_OPCODE_BUILD(BCWL_OPCODE_TYPE_DATA, BCWL_OPCODE_DATA_SUBTYPE_STAMODE_CONNECT), &state, 1);
}

/*!
    \brief      Blue courier triggers wifi to connect AP in station mode
    \param[in]  data: pointer to wifi ssid and password information (1 byte ssid length + ssid + 1 byte passwaord length + password + 1 byte random)
    \param[in]  len: data length
    \param[out] none
    \retval     none
*/
static void bcwp_wifi_connect(uint8_t *data, uint16_t len)
{
    uint8_t state = 1;
    uint8_t ssid_len, password_len;
    uint8_t wifi_ssid[MAC_SSID_LEN + 1];
    uint8_t wifi_password[WPA_MAX_PSK_LEN + 1];
    char *password = NULL;

    if (len <= 0)
        goto exit;

    /* decode */
    ble_internal_decode(data, len - 1, data[len - 1]);

    /* 1byte ssid_len + ssid + 1byte password_len + password + 1byte random */
    ssid_len = data[0];
    if (ssid_len > len || ssid_len > WPA_MAX_PSK_LEN)
        goto exit;

    password_len = data[ssid_len + 1];
    if ((ssid_len + password_len + 3) != len || password_len > WPA_MAX_PSK_LEN)
        goto exit;

    sys_memcpy(wifi_ssid, &data[1], ssid_len);
    wifi_ssid[ssid_len] = '\0';

    if (password_len != 0) {
        sys_memcpy(wifi_password, &data[ssid_len + 2], password_len);
        wifi_password[password_len] = '\0';
        password = (char *)wifi_password;
    }

    eloop_event_register(WIFI_MGMT_EVENT_DHCP_SUCCESS, bcwp_wifi_connect_success, NULL, NULL);
    if (wifi_management_connect((char *)wifi_ssid, password, true)) {
        goto exit;
    }

    return;
exit:
    bcwl_send(BCWL_OPCODE_BUILD(BCWL_OPCODE_TYPE_DATA, BCWL_OPCODE_DATA_SUBTYPE_STAMODE_CONNECT), &state, 1);
}

/*!
    \brief      Blue courier triggers wifi to disconnect current connection in station mode
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void bcwp_wifi_disconnect(void)
{
    uint8_t state = 0;
    wifi_management_disconnect();
    bcwl_send(BCWL_OPCODE_BUILD(BCWL_OPCODE_TYPE_DATA, BCWL_OPCODE_DATA_SUBTYPE_STAMODE_DISCONNECT), &state, 1);
}

/*!
    \brief      Blue courier triggers wifi to create AP in softAP mode
    \param[in]  data: pointer to wifi information(1byte ssid length + ssid + 1byte passwaord length + password + 1byte channel + 1byte akm + 1byte hide + 1byte random)
    \param[in]  len: data length
    \param[out] none
    \retval     none
*/
static void bcwp_wifi_ap_start(uint8_t *data, uint16_t len)
{
    uint8_t state = 1;
    uint8_t ssid_len, password_len, channel, akm, hide;
    uint8_t wifi_ssid[MAC_SSID_LEN + 1];
    uint8_t wifi_password[WPA_MAX_PSK_LEN + 1];
    char *password = NULL;
    wifi_ap_auth_mode_t auth_mode;

    if (len <= 0)
        goto exit;

    /* decode */
    ble_internal_decode(data, len - 1, data[len - 1]);

    /* 1byte ssid_len + ssid + 1byte password_len + password + 1byte channel + 1byte akm + 1byte hide + 1byte random*/
    ssid_len = data[0];
    if (ssid_len > len || ssid_len > WPA_MAX_PSK_LEN)
        goto exit;

    password_len = data[ssid_len + 1];
    if ((ssid_len + password_len + 6) != len || password_len > WPA_MAX_PSK_LEN)
        goto exit;

    sys_memcpy(wifi_ssid, &data[1], ssid_len);
    wifi_ssid[ssid_len] = '\0';

    if (password_len != 0) {
        sys_memcpy(wifi_password, &data[ssid_len + 2], password_len);
        wifi_password[password_len] = '\0';
        password = (char *)wifi_password;
    }

    channel = data[ssid_len + password_len + 2];
    akm = data[ssid_len + password_len + 3];
    hide = data[ssid_len + password_len + 4];

    switch(akm) {
    case 0:
        auth_mode = AUTH_MODE_OPEN;
        break;
    case 1:
        auth_mode = AUTH_MODE_WPA2;
        break;
    case 2:
        auth_mode = AUTH_MODE_WPA3;
        break;
    default:
        auth_mode= AUTH_MODE_WPA2_WPA3;
        break;
    }

    if (wifi_management_ap_start((char *)wifi_ssid, password, channel, auth_mode, hide)) {
        dbg_print(ERR, "bcwp failed to start AP, check your configuration.\r\n");
        goto exit;
    }

    state = 0;
    dbg_print(NOTICE, "bcwp softAP successfully started!\r\n");
exit:
    bcwl_send(BCWL_OPCODE_BUILD(BCWL_OPCODE_TYPE_DATA, BCWL_OPCODE_DATA_SUBTYPE_SOFTAPMODE_START), &state, 1);
}

/*!
    \brief      Blue courier triggers wifi to stop AP in softAP mode
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void bcwp_wifi_ap_stop(void)
{
    uint8_t state = 0;

    if (wifi_management_ap_stop() == 0) {
        dbg_print(NOTICE, "bcwp softAP successfully stoped!\r\n");
        bcwl_send(BCWL_OPCODE_BUILD(BCWL_OPCODE_TYPE_DATA, BCWL_OPCODE_DATA_SUBTYPE_SOFTAPMODE_STOP), &state, 1);
    } else {
        state = 1;
        bcwl_send(BCWL_OPCODE_BUILD(BCWL_OPCODE_TYPE_DATA, BCWL_OPCODE_DATA_SUBTYPE_SOFTAPMODE_STOP), &state, 1);
    }
}

/*!
    \brief      Blue courier get wifi status information
    \param[in]  none
    \param[out] none
    \retval     none
*/
void bcwp_wifi_status_get(void)
{
    struct wifi_ip_addr_cfg ip_cfg;
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(WIFI_VIF_INDEX_DEFAULT);
    struct mac_addr cli_mac[CFG_STA_NUM];
    bcwp_wifi_status_t wifi_status_rsp = {0};
    uint32_t i, ip;

    wifi_status_rsp.vif_idx = WIFI_VIF_INDEX_DEFAULT;
    wifi_vif_name(wifi_status_rsp.vif_idx, (char *)wifi_status_rsp.vif_name, sizeof(wifi_status_rsp.vif_name) - 1);

    wifi_status_rsp.mode = wvif->wvif_type;
    sys_memcpy(wifi_status_rsp.vif_mac, wvif->mac_addr.array, MAC_ADDR_LEN);

    if (!wifi_get_vif_ip(wifi_status_rsp.vif_idx, &ip_cfg)) {
        *(uint32_t *)wifi_status_rsp.ip_addr = ip_cfg.ipv4.addr;
        *(uint32_t *)wifi_status_rsp.gw = ip_cfg.ipv4.gw;
    }

    if (wvif->wvif_type == WVIF_STA) {
        wifi_status_rsp.sta_status = wvif->sta.state;
        wifi_status_rsp.sta_rssi = macif_vif_sta_rssi_get(wifi_status_rsp.vif_idx);
        wifi_status_rsp.sta_ssid.length = wvif->sta.cfg.ssid_len;
        sys_memcpy(wifi_status_rsp.sta_ssid.array, wvif->sta.cfg.ssid, wifi_status_rsp.sta_ssid.length);
        sys_memcpy(wifi_status_rsp.sta_bssid, wvif->sta.cfg.bssid, MAC_ADDR_LEN);
        wifi_status_rsp.bw = 20; //wvif->sta.cfg.bw;
        wifi_status_rsp.channel = wvif->sta.cfg.channel;

        if (wvif->sta.cfg.akm & CO_BIT(MAC_AKM_SAE))
            wifi_status_rsp.akm = 3;
        else if (wvif->sta.cfg.akm == (CO_BIT(MAC_AKM_PSK) | CO_BIT(MAC_AKM_PRE_RSN)))
            wifi_status_rsp.akm = 1;
        else if (wvif->sta.cfg.akm == CO_BIT(MAC_AKM_PSK))
            wifi_status_rsp.akm = 2;
        else if (wvif->sta.cfg.akm == CO_BIT(MAC_AKM_NONE))
            wifi_status_rsp.akm = 0;
        else
            wifi_status_rsp.akm = 5;
    } else if (wvif->wvif_type == WVIF_AP) {
        wifi_status_rsp.ap_status = wvif->ap.ap_state;
        wifi_status_rsp.ap_ssid.length = wvif->ap.cfg.ssid_len;
        sys_memcpy(wifi_status_rsp.ap_ssid.array, wvif->ap.cfg.ssid, wifi_status_rsp.ap_ssid.length);
        wifi_status_rsp.channel = wvif->ap.cfg.channel;
        wifi_status_rsp.cli_num = macif_vif_ap_assoc_info_get(wifi_status_rsp.vif_idx, (uint16_t *)&cli_mac);
        for (i = 0; i < wifi_status_rsp.cli_num; i++) {
            sys_memcpy(wifi_status_rsp.cli[i].mac, (uint8_t *)cli_mac[i].array, MAC_ADDR_LEN);
            ip = dhcpd_find_ipaddr_by_macaddr((uint8_t *)cli_mac->array);
            sys_memcpy(wifi_status_rsp.cli[i].ip, (uint8_t *)&ip, 4);
        }

        if (wvif->ap.cfg.akm == CO_BIT(MAC_AKM_NONE))
            wifi_status_rsp.akm = 0;
        else if (wvif->ap.cfg.akm == (CO_BIT(MAC_AKM_PSK) | CO_BIT(MAC_AKM_PRE_RSN)))
            wifi_status_rsp.akm = 1;
        else if (wvif->ap.cfg.akm == CO_BIT(MAC_AKM_PSK))
            wifi_status_rsp.akm = 2;
        else if (wvif->ap.cfg.akm == CO_BIT(MAC_AKM_SAE))
            wifi_status_rsp.akm = 3;
        else if (wvif->ap.cfg.akm == (CO_BIT(MAC_AKM_PSK) | CO_BIT(MAC_AKM_SAE)))
            wifi_status_rsp.akm = 4;
        else
            wifi_status_rsp.akm = 5;
    }

    bcwl_send(BCWL_OPCODE_BUILD(BCWL_OPCODE_TYPE_DATA, BCWL_OPCODE_DATA_SUBTYPE_STATUS_GET),
        (uint8_t *)&wifi_status_rsp, sizeof(wifi_status_rsp));
}

/*!
    \brief      Blue courier send custom data through BLE connection
    \param[in]  data: pointer to custom data
    \param[in]  len: data length
    \param[out] none
    \retval     none
*/
static void bcwp_send_custom_data(uint8_t *data, uint16_t len)
{
    bcwl_send(BCWL_OPCODE_BUILD(BCWL_OPCODE_TYPE_DATA, BCWL_OPCODE_DATA_SUBTYPE_CUSTOM_DATA), data, len);
}

/*!
    \brief      Blue courier wifi protocol message handler
    \param[in]  subtype: message subtype, @ref bcw_opcode_data_subtype
    \param[in]  data: pointer to message data
    \param[in]  len: message length
    \param[out] none
    \retval     none
*/
void bcwp_msg_handler(uint8_t subtype, uint8_t *data, uint16_t len)
{
    switch (subtype) {
        case BCWL_OPCODE_DATA_SUBTYPE_GET_SCAN_LIST: {
            bcwp_wifi_scan();
        } break;
        case BCWL_OPCODE_DATA_SUBTYPE_STAMODE_CONNECT: {
            bcwp_wifi_connect(data, len);
        } break;
        case BCWL_OPCODE_DATA_SUBTYPE_STAMODE_DISCONNECT: {
            bcwp_wifi_disconnect();
        } break;
        case BCWL_OPCODE_DATA_SUBTYPE_SOFTAPMODE_START: {
            bcwp_wifi_ap_start(data, len);
        } break;
        case BCWL_OPCODE_DATA_SUBTYPE_SOFTAPMODE_STOP: {
            bcwp_wifi_ap_stop();
        } break;
        case BCWL_OPCODE_DATA_SUBTYPE_STATUS_GET: {
            bcwp_wifi_status_get();
        } break;
        case BCWL_OPCODE_DATA_SUBTYPE_CUSTOM_DATA: {
            dbg_print(NOTICE, "bcwp receive custom data: ");
            for (uint8_t i = 0; i < len; i++)
                dbg_print(NOTICE, "%02x", data[i]);
            dbg_print(NOTICE, "\r\n");
            //bcwp_send_custom_data(data, len);
        } break;
        default: break;
    }
}

#endif // (BLE_APP_SUPPORT)
