/*!
    \file    wifi_vif.c
    \brief   wifi virtual interface for GD32VW55x SDK.

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

#include "wifi_vif.h"
#include "wifi_net_ip.h"
#include <string.h>

struct wifi_vif_tag wifi_vif_tab[CFG_VIF_NUM];

/*!
    \brief      Get mac vif from wifi vif
    \param[in]  vif_idx: index of the wifi vif
    \param[out] none
    \retval     Pointer to the mac vif on success or NULL if error occured.
*/
void *vif_idx_to_mac_vif(uint8_t vif_idx)
{
    if (vif_idx >= CFG_VIF_NUM)
        return NULL;
    return wifi_vif_tab[vif_idx].mac_vif;
}

/*!
    \brief      Get mac vif from wifi vif
    \param[in]  wvif: Pointer to the wifi vif
    \param[out] none
    \retval     Pointer to the mac vif on success and NULL if error occured.
*/
void *wvif_to_mac_vif(void *wvif)
{
    return (((struct wifi_vif_tag *)wvif)->mac_vif);
}

/*!
    \brief      Get net interface from wifi vif
    \param[in]  vif_idx: index of the wifi vif
    \param[out] none
    \retval     Pointer to the net interface on success and NULL if error occured.
*/
void *vif_idx_to_net_if(uint8_t vif_idx)
{
    if (vif_idx >= CFG_VIF_NUM)
        return NULL;
    return &(wifi_vif_tab[vif_idx].net_if);
}

/*!
    \brief      Get wifi vif by index
    \param[in]  vif_idx: index of the wifi vif
    \param[out] none
    \retval     Pointer to the wifi vif on success and NULL if error occured.
*/
void *vif_idx_to_wvif(uint8_t vif_idx)
{
    if (vif_idx >= CFG_VIF_NUM)
        return NULL;
    return &(wifi_vif_tab[vif_idx]);
}

/*!
    \brief      Get index of the wifi vif
    \param[in]  wvif: Pointer to the wifi vif
    \param[out] none
    \retval     index of the wifi vif.
*/
int wvif_to_vif_idx(void *wvif)
{
    return ((struct wifi_vif_tag *)wvif - wifi_vif_tab);
}

/*!
    \brief      Convert the wifi vif type to the mac vif type
    \param[in]  wvif_type: wifi vif type
    \param[out] none
    \retval     mac vif type.
*/
uint32_t wvif_type_to_mvif_type(uint32_t wvif_type)
{
    enum mac_vif_type macvif_type;

    switch (wvif_type) {
        case WVIF_STA:
            macvif_type = VIF_STA;
            break;
        case WVIF_AP:
            macvif_type = VIF_AP;
            break;
        case WVIF_MONITOR:
            macvif_type = VIF_MONITOR;
            break;
        default:
            macvif_type = VIF_UNKNOWN;
            break;
    }
    return macvif_type;
}

/*!
    \brief      Convert the mac vif type to the wifi vif type
    \param[in]  macvif_type: mac vif type
    \param[out] none
    \retval     wifi vif type.
*/
uint32_t mvif_type_to_wvif_type(uint32_t macvif_type)
{
    enum wifi_vif_type wvif_type;

    switch (macvif_type) {
        case WVIF_STA:
            wvif_type = WVIF_STA;
            break;
        case WVIF_AP:
            wvif_type = WVIF_AP;
            break;
        case WVIF_MONITOR:
            wvif_type = WVIF_MONITOR;
            break;
        default:
            wvif_type = WVIF_UNKNOWN;
            break;
    }
    return wvif_type;
}

/*!
    \brief      Initialize WIFI VIF
                This include initialization of the wifi VIF structure (mac address, ...) and
                registration of the interface in the IP stack.
    \param[in]  vif_idx: Index of the WIFI VIF to initialize.
                         No check on the value, assume it is valid.
    \param[in]  base_mac_addr: Base MAC address
    \param[out] none
    \retval     none
*/
void wifi_vif_init(int vif_idx, struct mac_addr *base_mac_addr)
{
    struct wifi_vif_tag *wvif =  &wifi_vif_tab[vif_idx];

    sys_memset(wvif, 0, sizeof(struct wifi_vif_tag));

    wvif->mac_addr = *base_mac_addr;
    wvif->mac_addr.array[2] ^= (vif_idx << 8);
    wvif->mac_vif = NULL;
    wvif->sta.ap_id = 0xFF;
#ifdef CONFIG_WPA_SUPPLICANT
    wvif->wpa_vif = &wifi_wpa.vifs[vif_idx];
#endif
    net_if_add(&wvif->net_if, (uint8_t *)wvif->mac_addr.array,
               NULL, NULL, NULL, wvif);
}

/*!
    \brief      Initialize all the WIFI VIFs
                This include initialization of the all the wifi VIF structure (mac address,
                ip address...) and registration of the interface in the IP stack.
    \param[in]  base_mac_addr: Base MAC address(from which all VIF MAC addresses are computed)
    \param[out] none
    \retval     0 on success and != 0 if error occured.
*/
int wifi_vifs_init(struct mac_addr *base_mac_addr)
{
    if (base_mac_addr == NULL)
        return -1;

    /* Initialize wifi virtue interfaces */
    for (int i = 0; i < CFG_VIF_NUM; i++)
    {
        struct wifi_ip_addr_cfg ip_cfg;

        wifi_vif_init(i, base_mac_addr);

        ip_cfg.mode = IP_ADDR_STATIC_IPV4;
        ip_cfg.ipv4.addr = 0;
        ip_cfg.ipv4.mask =  0x00FFFFFF;
        ip_cfg.ipv4.gw = 0;
        ip_cfg.ipv4.dns = 0;

        if (i == 0)
            ip_cfg.default_output = true;
        else
            ip_cfg.default_output = false;

        if (wifi_set_vif_ip(i, &ip_cfg))
            return -2;
    }
    return 0;
}

/*!
    \brief      Reset all wifi vifs
    \param[in]  none
    \param[out] none
    \retval     none
*/
void wifi_vifs_deinit(void)
{
    struct wifi_vif_tag *wvif;
    struct wifi_ip_addr_cfg ip_cfg;

    /* deinitialize wifi virtue interfaces */
    for (int i = 0; i < CFG_VIF_NUM; i++)
    {
        wvif =  &wifi_vif_tab[i];

        ip_cfg.mode = IP_ADDR_NONE;
#ifdef CONFIG_IPV6_SUPPORT
        ip_cfg.ip6_mode = IP6_ADDR_NONE;
#endif
        wifi_set_vif_ip(i, &ip_cfg);

        net_if_remove(&wvif->net_if);

        sys_memset(wvif, 0, sizeof(struct wifi_vif_tag));

        wvif->wvif_type = WVIF_UNKNOWN;
        wvif->sta.ap_id = 0xFF;

        // clear static ip setting, because IP address configuration of
        // TCPIP network interface structure has been cleared.
        net_if_use_static_ip(false);
    }
}

/*!
    \brief      Reset a wifi vif
    \param[in]  vif_idx: index of the wifi vif
    \param[in]  type: type of the wifi vif, refer to enum wifi_vif_type
    \param[out] none
    \retval     none
*/
void wifi_vif_reset(int vif_idx, enum wifi_vif_type type)
{
    struct wifi_vif_tag *wvif =  &wifi_vif_tab[vif_idx];
    struct wifi_sta *sta = &wvif->sta;
    struct wifi_ap *ap = &wvif->ap;
    struct wifi_monitor *monitor = &wvif->monitor;

    wvif->wvif_type = type;

    if (type == WVIF_STA) {
        sta->state = WIFI_STA_STATE_IDLE;
        sys_memset(&sta->cfg, 0, sizeof(sta->cfg));
        sta->ap_id = 0xFF;
#ifndef CONFIG_WPA_SUPPLICANT
        sys_memset(&sta->cache, 0, sizeof(sta->cache));
#endif
    } else if (type == WVIF_AP) {
        ap->ap_state = WIFI_AP_STATE_INIT;
        sys_memset(&ap->cfg, 0, sizeof(ap->cfg));
    } else if (type == WVIF_MONITOR) {
        macif_rx_set_monitor_cb(NULL, NULL);
        sys_memset(monitor, 0, sizeof(*monitor));
    }

#ifdef CONFIG_WPA_SUPPLICANT
    wifi_wpa_link_monitor(vif_idx, 0);
    wifi_wpa_remove_vif(vif_idx);
#endif
}

/*!
    \brief      Get mac address of the wifi vif
    \param[in]  vif_idx: index of the wifi vif
    \param[out] none
    \retval     Pointer to the mac address on success and NULL if error occured.
*/
uint8_t *wifi_vif_mac_addr_get(int vif_idx)
{
    if (vif_idx >= CFG_VIF_NUM)
        return NULL;

    return (uint8_t *)wifi_vif_tab[vif_idx].mac_addr.array;
}

/*!
    \brief      Link a mac vif to the wifi vif
    \param[in]  vif_idx: index of the wifi vif
    \param[in]  mac_vif: pointer to the mac vif
    \param[out] none
    \retval     none
*/
void wifi_vif_mac_vif_set(int vif_idx, void *mac_vif)
{
    if (vif_idx >= CFG_VIF_NUM)
        return;

    wifi_vif_tab[vif_idx].mac_vif = mac_vif;
}

/*!
    \brief      Get UAPSD config of the wifi vif which is in station mode
    \param[in]  vif_idx: index of the wifi vif
    \param[out] none
    \retval     UAPSD config of the wifi vif on success and 0 if error occured.
*/
uint8_t wifi_vif_sta_uapsd_get(int vif_idx)
{
    if (vif_idx >= CFG_VIF_NUM)
        return 0;

    return (wifi_vif_tab[vif_idx].sta.uapsd_queues);
}

/*!
    \brief      Get status code from the station mode of the wifi vif
    \param[in]  vif_idx: index of the wifi vif
    \param[out] none
    \retval     status code
*/
int wifi_vif_sta_status_code_get(int vif_idx)
{
    struct wifi_vif_tag *wvif;

    if (vif_idx >= CFG_VIF_NUM)
        return -1;

    wvif =  &wifi_vif_tab[vif_idx];

    if (wvif->wvif_type == WVIF_STA) {
        return wvif->sta.status_code;
    }

    return -1;
}

/*!
    \brief      Set UAPSD config of the wifi vif which is in station mode
    \param[in]  vif_idx: index of the wifi vif
    \param[in]  uapsd_queues: UAPSD config
    \param[out] none
    \retval     0 on success and -1 if error occured.
*/
int wifi_vif_uapsd_queues_set(int vif_idx, uint8_t uapsd_queues)
{
    if (vif_idx < 0)
    {
        int i;
        for (i = 0; i < CFG_VIF_NUM; i++)
        {
            wifi_vif_tab[i].sta.uapsd_queues = uapsd_queues;
        }
    }
    else
    {
        if (vif_idx >= CFG_VIF_NUM)
            return -1;
        wifi_vif_tab[vif_idx].sta.uapsd_queues = uapsd_queues;
    }
    return 0;
}

/*!
    \brief      Set the ap index of the wifi vif which is in station mode
    \param[in]  vif_idx: index of the wifi vif
    \param[in]  ap_id: ap index
    \param[out] none
    \retval     none
*/
void wifi_vif_ap_id_set(int vif_idx, uint8_t ap_id)
{
    if (vif_idx >= CFG_VIF_NUM)
        return;

    wifi_vif_tab[vif_idx].sta.ap_id = ap_id;
}

/*!
    \brief      Get the ap index of the wifi vif which is in station mode
    \param[in]  vif_idx: index of the wifi vif
    \param[out] none
    \retval     ap index on success and 0xFF if error occured.
*/
uint8_t wifi_vif_ap_id_get(int vif_idx)
{
    if (vif_idx >= CFG_VIF_NUM)
        return 0xFF;

    return wifi_vif_tab[vif_idx].sta.ap_id;
}

/*!
    \brief      Get the history IP address of the wifi vif which is in station mode
    \param[in]  none
    \param[out] none
    \retval     history IP address.
*/
uint32_t wifi_vif_history_ip_get(void)
{
    return wifi_vif_tab[WIFI_VIF_INDEX_DEFAULT].sta.history_ip;
}

/*!
    \brief      Check if wifi vif is in softap mode
    \param[in]  vif_idx: index of the wifi vif
    \param[out] none
    \retval     true if in softap mode and false otherwise.
*/
int wifi_vif_is_softap(int vif_idx)
{
    struct wifi_vif_tag *wvif;

    if (vif_idx >= CFG_VIF_NUM)
        return false;

    wvif =  &wifi_vif_tab[vif_idx];
    if (NULL == wvif->mac_vif)
        return false;

    if (wvif->wvif_type == WVIF_AP && wvif->ap.ap_state == WIFI_AP_STATE_STARTED) {
        return true;
    }

    return false;
}

/*!
    \brief      Check if wifi vif which in station is in the connection state
    \param[in]  vif_idx: index of the wifi vif
    \param[out] none
    \retval     true if in the connection state and false otherwise.
*/
int wifi_vif_is_sta_connecting(int vif_idx)
{
    struct wifi_vif_tag *wvif;

    if (vif_idx >= CFG_VIF_NUM)
        return false;

    wvif =  &wifi_vif_tab[vif_idx];
    if (NULL == wvif->mac_vif)
        return false;

    if (wvif->wvif_type == WVIF_STA) {
        if ((wvif->sta.state >= WIFI_STA_STATE_SCAN)
            && (wvif->sta.state <= WIFI_STA_STATE_IP_GETTING))
            return true;
    }

    return false;
}

/*!
    \brief      Check if handshake of wifi vif which in station is completed
    \param[in]  vif_idx: index of the wifi vif
    \param[out] none
    \retval     true if handshake is completed and false otherwise.
*/
int wifi_vif_is_sta_handshaked(int vif_idx)
{
    struct wifi_vif_tag *wvif;

    if (vif_idx >= CFG_VIF_NUM)
        return false;

    wvif =  &wifi_vif_tab[vif_idx];
    if (NULL == wvif->mac_vif)
        return false;

    if (wvif->wvif_type == WVIF_STA) {
        if ((wvif->sta.state >= WIFI_STA_STATE_IP_GETTING)
            && (wvif->sta.state <= WIFI_STA_STATE_CONNECTED))
            return true;
    }

    return false;
}

/*!
    \brief      Check if wifi vif which in station is connected to an ap
    \param[in]  vif_idx: index of the wifi vif
    \param[out] none
    \retval     true if is connected and false otherwise.
*/
int wifi_vif_is_sta_connected(int vif_idx)
{
    struct wifi_vif_tag *wvif;

    if (vif_idx >= CFG_VIF_NUM)
        return false;

    wvif =  &wifi_vif_tab[vif_idx];
    if (NULL == wvif->mac_vif)
        return false;

    if (wvif->wvif_type == WVIF_STA) {
        if (wvif->sta.state == WIFI_STA_STATE_CONNECTED)
            return true;
    }

    return false;
}

/*!
    \brief      Get name of the wifi vif
    \param[in]  vif_idx: index of the wifi vif
    \param[in]  len: length of buffer
    \param[out] name: Buffer to write the wifi vif name
    \retval     length of the wifi vif name on success and -1 if error occured.
*/
int wifi_vif_name(int vif_idx, char *name, int len)
{
    if (vif_idx >= CFG_VIF_NUM)
        return -1;

    return net_if_get_name(&wifi_vif_tab[vif_idx].net_if, name, len);
}

/*!
    \brief      Set mac address of the wifi vif
    \param[in]  user_addr: pointer to the user MAC address of the device (from which all
                           VIF MAC addresses are computed)
    \param[out] none
    \retval     none
*/
void wifi_vif_user_addr_set(uint8_t *user_addr)
{
    macif_user_mac_addr_set(user_addr);
}

#ifndef CONFIG_WPA_SUPPLICANT
/*!
    \brief      Check if a client is connected to the softap
    \param[in]  ap: pointer to the softap
    \param[in]  sa: pointer to the mac address of the client
    \param[out] none
    \retval     true if is connected and false otherwise.
*/
int wifi_vif_is_cli_connected(struct wpas_ap *ap, uint8_t *sa)
{
    struct ap_cli *cli = ap->cli;

    while (cli) {
        if (sys_memcmp((uint8_t *)cli->addr.array, sa, WIFI_ALEN) == 0) {
            if (cli->cli_state == WIFI_STA_STATE_CONNECTED) {
                return true;
            }
        }
        cli = cli->next;
    }
    return false;
}

/*!
    \brief      Set type of the wifi vif
    \param[in]  vif_idx: index of the wifi vif
    \param[in]  wvif_type: type of the wifi vif, refer to enum wifi_vif_type
    \param[out] none
    \retval     0 on success and != 0 if error occured.
*/
int wifi_vif_type_set(int vif_idx, enum wifi_vif_type wvif_type)
{
    struct macif_cmd_set_vif_type cmd;
    struct macif_cmd_resp resp;

    if (vif_idx >= CFG_VIF_NUM)
        return -1;

    // Change type of the associated MAC vif
    cmd.hdr.len = sizeof(cmd);
    cmd.hdr.id = MACIF_SET_VIF_TYPE_CMD;
    cmd.vif_idx = vif_idx;
    cmd.type = wvif_type_to_mvif_type(wvif_type);

    if (macif_ctl_cmd_execute(&cmd.hdr, &resp.hdr)
        || resp.status != MACIF_STATUS_SUCCESS) {
        return -1;
    }
    return 0;
}

#else /* CONFIG_WPA_SUPPLICANT */

/*!
    \brief      Get index of the wifi vif
    \param[in]  name: pointer to the name of the wifi vif
    \param[out] none
    \retval     index of the wifi vif on success and -1 if error occured.
*/
int wifi_vif_idx_from_name(const char *name)
{
    struct netif *net_if;
    int i;

    if (name == NULL)
        return -1;

    net_if = (struct netif *)net_if_find_from_name(name);
    if (!net_if)
        return -1;

    for (i = 0; i < CFG_VIF_NUM; i++)
    {
        if (&(wifi_vif_tab[i].net_if) == net_if)
            return i;
    }

    return -1;
}

/*!
    \brief      Set type of the wifi vif
    \param[in]  vif_idx: index of the wifi vif
    \param[in]  wvif_type: type of the wifi vif, refer to enum wifi_vif_type
    \param[out] none
    \retval     0 on success and != 0 if error occured.
*/
int wifi_vif_type_set(int vif_idx, enum wifi_vif_type wvif_type)
{
    struct macif_cmd_set_vif_type cmd;
    struct macif_cmd_resp resp;

    cmd.hdr.len = sizeof(cmd);
    cmd.hdr.id = MACIF_SET_VIF_TYPE_CMD;
    cmd.vif_idx = vif_idx;
    cmd.type = wvif_type_to_mvif_type(wvif_type);
    cmd.p2p = 0;

    if (macif_cntrl_cmd_send_cli(&cmd.hdr, &resp.hdr) ||
        (resp.status != MACIF_STATUS_SUCCESS))
    {
        return -1;
    }

    return 0;
}

#endif /* CONFIG_WPA_SUPPLICANT */

/*!
    \brief      Set hostname of the STA
    \param[in]  vif_idx: index of the wifi vif
    \param[in]  hostname: pointer to the hostname
    \param[out] none
    \retval     0 on success and != 0 if error occurred.
*/
int wifi_vif_hostname_set(int vif_idx, const char *hostname)
{
    struct wifi_vif_tag *wvif;
    size_t len;

    if (vif_idx >= CFG_VIF_NUM || hostname == NULL) {
        return -1;
    }

    wvif = &wifi_vif_tab[vif_idx];

    if (wvif->wvif_type != WVIF_STA) {
        return -1;
    }

    len = strlen(hostname);
    if (len > WIFI_HOSTNAME_MAX_LEN) {
        return -1;
    }

    // Save user-defined hostname
    sys_memcpy(wvif->user_hostname, hostname, len);
    wvif->user_hostname[len] = '\0';

#if LWIP_NETIF_HOSTNAME
    netif_set_hostname(&wvif->net_if, wvif->user_hostname);
#endif
    return 0;
}

/*!
    \brief      Get hostname of the STA
    \param[in]  vif_idx: index of the wifi vif
    \param[out] none
    \retval     Pointer to the hostname on success and NULL if error occurred.
*/
const char *wifi_vif_hostname_get(int vif_idx)
{
    struct wifi_vif_tag *wvif;

    if (vif_idx >= CFG_VIF_NUM) {
        return NULL;
    }

    wvif = &wifi_vif_tab[vif_idx];

    if (wvif->wvif_type != WVIF_STA) {
        return NULL;
    }

    // If user has set a custom hostname, return it
    if (wvif->user_hostname[0] != '\0') {
        return wvif->user_hostname;
    }

#if LWIP_NETIF_HOSTNAME
    // Otherwise return the netif hostname
    return netif_get_hostname(&wvif->net_if);
#else
    return NULL;
#endif
}

/*!
    \brief      Set softAP max connection limit (must be called before wifi_management_ap_start)
    \param[in]  max_conn: maximum number of stations
    \param[out] none
    \retval     none
*/
int wifi_vif_ap_set_max_conn(int vif_idx, uint8_t max_conn)
{
    if (vif_idx >= CFG_VIF_NUM) {
        return -1;
    }

    if (vif_idx == WIFI_VIF_INDEX_DEFAULT) {
        if (max_conn <= CFG_STA_NUM) {
            wifi_vif_tab[vif_idx].ap.cfg.max_conn = max_conn;
            return 0;
        }
    } else {
        if (max_conn < CFG_STA_NUM) {
            wifi_vif_tab[vif_idx].ap.cfg.max_conn = max_conn;
            return 0;
        }
    }
    return -1;
}
