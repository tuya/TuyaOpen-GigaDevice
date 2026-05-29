/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#include "app_cfg.h"

#if defined(CONFIG_ALICLOUD_SUPPORT)

#include <stdio.h>

#include "iot_import.h"
#include "config_gdm32.h"
#include "rom_export.h"
#include "wrapper_os.h"

#include "wifi_net_ip.h"
#include "wifi_management.h"


#define platform_warn(fmt, ...) printf("Livingkit HAL warn:" fmt, ##__VA_ARGS__)
#define platform_info(fmt, ...) printf("Livingkit HAL info:" fmt, ##__VA_ARGS__)
#define platform_err(fmt, ...) printf("Livingkit HAL error:" fmt, ##__VA_ARGS__)

static awss_recv_80211_frame_cb_t g_ieee80211_handler = NULL;

static void monitor_data_handler(struct wifi_frame_info *info, void *arg)
{
    int with_fcs = 0;
    enum AWSS_LINK_TYPE link_type = AWSS_LINK_TYPE_NONE;

    if (info == NULL || info->payload == NULL)
        return;

    if (g_ieee80211_handler != NULL) {
        //platform_warn("monitor_data_handler len %d rssi %d\r\n", (int)info->length, (signed char)info->rssi);
        (*g_ieee80211_handler)((char *)info->payload, (int)info->length, link_type, with_fcs, (signed char)info->rssi);
    }
}

void HAL_Awss_Open_Monitor(_IN_ awss_recv_80211_frame_cb_t cb)
{
    platform_warn("HAL_Awss_Open_Monitor\r\n");
    g_ieee80211_handler = cb;
    wifi_management_monitor_start(6, monitor_data_handler);
}

void HAL_Awss_Close_Monitor(void)
{
    platform_info("HAL_Awss_Close_Monitor\r\n");

    wifi_management_sta_start();
}

int HAL_Awss_Connect_Ap(
    _IN_ uint32_t connection_timeout_ms,
    _IN_ char ssid[HAL_MAX_SSID_LEN],
    _IN_ char passwd[HAL_MAX_PASSWD_LEN],
    _IN_OPT_ enum AWSS_AUTH_TYPE auth,
    _IN_OPT_ enum AWSS_ENC_TYPE encry,
    _IN_OPT_ uint8_t bssid[ETH_ALEN],
    _IN_OPT_ uint8_t channel)
{
    int32_t ret = 0;
    uint32_t ssid_len;
    uint32_t key_len;

    if (ssid == NULL)
        return -1;

    ssid_len = strlen(ssid);
    if (ssid_len <= 0 || ssid_len > MAC_SSID_LEN) {
        platform_warn("connect to AP, SSID length error\r\n");
        return -1;
    }

    if (passwd == NULL)
        key_len = 0;
    else
        key_len = strlen(passwd);

    if ((key_len != 0) && (key_len > WPA_MAX_PSK_LEN || key_len < WPA_MIN_PSK_LEN)) {
        platform_warn("PASSWORD length error\r\n");
        return -1;
    }

    ret = wifi_management_connect((char *)ssid, (char *)passwd, 1);
    if (ret != 0)
        platform_warn("start wifi_connect failed\r\n");

    return ret;
}
/*
range 0 1000
default 200
*/
int HAL_Awss_Get_Channelscan_Interval_Ms(void)
{
    return 200;
}
/*
range 0 1800000
default 180000
*/
int HAL_Awss_Get_Timeout_Interval_Ms(void)
{
    return 180000;
}

int HAL_Awss_Get_Encrypt_Type(void)
{
    return 3;
}

int HAL_Awss_Get_Conn_Encrypt_Type(void)
{

    return 3;

    char invalid_ds[DEVICE_SECRET_LEN + 1] = {0};
    char ds[DEVICE_SECRET_LEN + 1] = {0};

    HAL_GetDeviceSecret(ds);

    if (memcmp(invalid_ds, ds, sizeof(ds)) == 0)
        return 3;

    memset(invalid_ds, 0xff, sizeof(invalid_ds));
    if (memcmp(invalid_ds, ds, sizeof(ds)) == 0)
        return 3;

    return 4;
    //TODO return 3
}

int HAL_Awss_Open_Ap(const char *ssid, const char *passwd, int beacon_interval, int hide)
{
    int ret = -1;
    uint8_t channel = 1;

    ret = wifi_management_ap_start((char *)ssid, NULL, channel, AUTH_MODE_OPEN, 0);
    if (ret)
        platform_err("Failed to start AP, check your configuration.\r\n");
    else
        platform_warn("AP start successfully\r\n");

    return ret;
}

int HAL_Awss_Close_Ap(void)
{
    return wifi_management_ap_stop();

}

void HAL_Awss_Switch_Channel(char primary_channel, char secondary_channel, uint8_t bssid[ETH_ALEN])
{
    wifi_netlink_channel_set(primary_channel);
}
#endif /* CONFIG_ALICLOUD_SUPPORT */
