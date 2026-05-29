/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "app_cfg.h"

#ifdef CONFIG_ALICLOUD_SUPPORT

#include <string.h>

#include "iot_import.h"
#include "wrapper_os.h"

#include "wifi_net_ip.h"
#include "wifi_management.h"


int HAL_Wifi_Enable_Mgmt_Frame_Filter(
    _IN_ uint32_t filter_mask,
    _IN_OPT_ uint8_t vendor_oui[3],
    _IN_ awss_wifi_mgmt_frame_cb_t callback)
{
    hal_info("=> HAL_Wifi_Enable_Mgmt_Frame_Filter, TODO\r\n");
    return 0;
}

int HAL_Wifi_Get_Ap_Info(char ssid[HAL_MAX_SSID_LEN], char passwd[HAL_MAX_PASSWD_LEN], uint8_t bssid[ETH_ALEN])
{
    struct mac_vif_status vif_status;
    struct wifi_vif_tag *wvif;

    sys_memset(ssid, 0, HAL_MAX_SSID_LEN);
    sys_memset(passwd, 0, HAL_MAX_PASSWD_LEN);
    sys_memset(bssid, 0, ETH_ALEN);

    wvif = vif_idx_to_wvif(WIFI_VIF_INDEX_DEFAULT);
    sys_memcpy(passwd, wvif->sta.cfg.passphrase , wvif->sta.cfg.passphrase_len);

    if (macif_vif_status_get(WIFI_VIF_INDEX_DEFAULT, &vif_status) ||
            (vif_status.type != VIF_STA))
        return -1;

    sys_memcpy(ssid, vif_status.sta.ssid.array, vif_status.sta.ssid.length);
    sys_memcpy(bssid, vif_status.sta.bssid.array, ETH_ALEN);

    return 0;
}

uint32_t HAL_Wifi_Get_IP(char ip_str[NETWORK_ADDR_LEN], const char *ifname)
{
    struct wifi_ip_addr_cfg ip_cfg;

    if (wifi_get_vif_ip(WIFI_VIF_INDEX_DEFAULT, &ip_cfg)) {
        sys_memset(ip_str, 0, NETWORK_ADDR_LEN);
        return -1;
    }

    snprintf(ip_str, NETWORK_ADDR_LEN, "%d.%d.%d.%d",
                            ip_cfg.ipv4.addr & 0xff, (ip_cfg.ipv4.addr >> 8) & 0xff,
                            (ip_cfg.ipv4.addr >> 16) & 0xff,
                            (ip_cfg.ipv4.addr >> 24) & 0xff);

    return 0;
}

char *HAL_Wifi_Get_Mac(char mac_str[HAL_MAC_LEN])
{
    uint8_t *mac = NULL;

    mac = wifi_vif_mac_addr_get(WIFI_VIF_INDEX_DEFAULT);

    snprintf(mac_str, HAL_MAC_LEN, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0],
            mac[1], mac[2], mac[3], mac[4], mac[5]);

    return mac_str;
}

int HAL_Wifi_Scan(awss_wifi_scan_result_cb_t cb)
{
    hal_info("=>%s, TODO\r\n", __func__);
    return 0;
}

int HAL_Wifi_Send_80211_Raw_Frame(_IN_ enum HAL_Awss_Frame_Type type,
                                  _IN_ uint8_t *buffer, _IN_ int len)
{
    wifi_send_80211_frame(WIFI_VIF_INDEX_DEFAULT,
                buffer, len - 4, 0, NULL, NULL);

    return 0;
}
#endif /* CONFIG_ALICLOUD_SUPPORT */
