/*!
    \file    wifi_net_ip.c
    \brief   Implementation of IP configuration related for GD32VW55x SDK.

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

#include "wrapper_os.h"
#include "wifi_net_ip.h"

#include "wifi_netif.h"
#include "wifi_vif.h"
#include "wifi_export.h"
#include "lwip/netif.h"
#include "lwip/lwip_napt.h"
#include "dbg_print.h"
#include "string.h"

/**
 ******************************************************************************
 * @brief Stop using DHCP
 *
 * Release DHCP lease for the specified interface and stop DHCP procedure.
 *
 * @param[in] net_if  Pointer to network interface structure
 *
 * @return 0 if DHCP is stopped, != 0 an error occurred.
 ******************************************************************************
 */
static int wifi_dhcp_stop(struct netif *net_if)
{
    // Release DHCP lease
    if (!net_dhcp_address_obtained(net_if))
    {
        if (net_dhcp_release(net_if))
        {
            dbg_print(ERR, "Failed to release DHCP\r\n");
            return -1;
        }

        dbg_print(INFO, "IP released\r\n");
    }

    // Stop DHCP
    net_dhcp_stop(net_if);

    return 0;
}

/**
 ******************************************************************************
 * @brief Retrieve IP address using DHCP
 *
 * Start DHCP procedure for the specified interface and wait until
 * it is completed using a timeout passed as parameter.
 *
 * @param[in] net_if  Pointer to network interface structure
 * @param[in] to_ms   Timeout in milliseconds
 *
 * @return 0 when ip address has been received, !=0 an error or timeout occurred.
 ******************************************************************************
 */
static int wifi_dhcp_start(struct netif *net_if, uint32_t to_ms)
{
    uint32_t start_ms;

    if (!netif_is_up(net_if)) {
        dbg_print(WARNING, "net_if is not up, stop dhcp\r\n");
        return -1;
    }

    // Run DHCP client
    if (net_dhcp_start(net_if))
    {
        dbg_print(ERR, "Failed to start DHCP\r\n");
        return -1;
    }

    if (to_ms == 0) {
        return 0;
    }

    start_ms = sys_os_now(false);
    while ((!net_dhcp_address_obtained(net_if)) &&
           (sys_os_now(false) - start_ms < to_ms) &&
           !net_if_is_static_ip())
    {
        sys_ms_sleep(100);
    }

    if (!net_dhcp_address_obtained(net_if) && !net_if_is_static_ip())
    {
        dbg_print(ERR, "DHCP timeout\r\n");
        wifi_dhcp_stop(net_if);
        return -1;
    } else {
        struct wifi_ip_addr_cfg cfg;
        net_if_get_ip(net_if, &(cfg.ipv4.addr), &(cfg.ipv4.mask), &(cfg.ipv4.gw));
        dbg_print(NOTICE, "Got IP  " IP_FMT "\r\n", IP_ARG(cfg.ipv4.addr));
    }

    return 0;
}

#if 0
int wifi_dhcp_start_no_wait(void *net_if)
{
    if (!netif_is_up((struct netif *)net_if)) {
        dbg_print(WARNING, "net_if is not up, stop dhcp\r\n");
        return -1;
    }

    // Run DHCP client
    if (net_dhcp_start((struct netif *)net_if))
    {
        dbg_print(ERR, "Failed to start DHCP\r\n");
        return -1;
    }

    return 0;
}
#endif

/*
 ****************************************************************************************
 * PUBLIC FUNCTIONS
 ****************************************************************************************
 */
#ifdef CONFIG_NAPT
/*!
    \brief      Enable NAPT for the specified VIF
    \param[in]  vif_idx: index of the wifi vif
    \param[out] none
    \retval     0 on success and != 0 if error occured.
*/
int wifi_set_softap_napt_enable(int vif_idx)
{
    struct netif *net_if;

    if (vif_idx >= CFG_VIF_NUM)
        return -1;

    net_if = vif_idx_to_net_if(vif_idx);
    if (!net_if)
        return -1;

    ip_napt_enable_netif(net_if, 1);

    return 0;
}
#endif /* CONFIG_NAPT */

/*!
    \brief      Config the IP address infomation
    \param[in]  vif_idx: index of the wifi vif
    \param[in]  cfg: pointer to the IP address infomation
    \param[out] none
    \retval     0 on success and != 0 if error occured.
*/
int wifi_set_vif_ip(int vif_idx, struct wifi_ip_addr_cfg *cfg)
{
    struct netif *net_if;

    if (vif_idx >= CFG_VIF_NUM)
        return -1;

    net_if = vif_idx_to_net_if(vif_idx);
    if (!net_if)
        return -1;

    if (cfg->mode != IP_ADDR_DHCP_SERVER)
    {
        net_dhcpd_stop(net_if);
    }
#ifdef CONFIG_IPV6_SUPPORT
    if (cfg->ip6_mode != IP6_ADDR_SERVER)
    {
        net_ip6_server_stop(net_if);
    }
    if (cfg->ip6_mode == IP6_ADDR_NONE) {
        wifi_ip6_unique_addr_set_invalid(net_if);
    }
#endif /* CONFIG_IPV6_SUPPORT */
    if (cfg->mode == IP_ADDR_NONE)
    {
        // clear current IP address
        if (!net_if_is_static_ip()) {
            wifi_dhcp_stop(net_if);
            net_if_set_ip(net_if, 0, 0, 0);
        }
        return 0;
    }
    if (cfg->mode == IP_ADDR_STATIC_IPV4)
    {
        // To be safe
        wifi_dhcp_stop(net_if);
        net_if_set_ip(net_if, cfg->ipv4.addr, cfg->ipv4.mask, cfg->ipv4.gw);

        if (cfg->ipv4.dns)
            net_set_dns(cfg->ipv4.dns);
        else
            net_get_dns(&cfg->ipv4.dns);
        net_if_send_gratuitous_arp(net_if);
    }
    else if (cfg->mode == IP_ADDR_DHCP_CLIENT)
    {
#ifdef CFG_COEX
        {
            uint8_t ble_pti;
            int32_t ret;

            ble_pti = coex_get_wlan_pti(WLAN_PTI_BE_DATA);
            coex_set_wlan_pti(WLAN_PTI_BE_DATA, 6);
            ret = wifi_dhcp_start(net_if, cfg->dhcp.to_ms);
            coex_set_wlan_pti(WLAN_PTI_BE_DATA, ble_pti);
            if (ret != 0)
                return -1;
        }
#else
        if (wifi_dhcp_start(net_if, cfg->dhcp.to_ms))
            return -1;
#endif

        net_if_get_ip(net_if, &(cfg->ipv4.addr), &(cfg->ipv4.mask), &(cfg->ipv4.gw));
        net_get_dns(&cfg->ipv4.dns);

        dbg_print(INFO, "{VIF-%d} ip=" IP_FMT " gw=" IP_FMT "\r\n",
                        vif_idx, IP_ARG(cfg->ipv4.addr), IP_ARG(cfg->ipv4.gw));
    }
    else if (cfg->mode == IP_ADDR_DHCP_SERVER)
    {
        wifi_dhcp_stop(net_if);
        net_if_set_ip(net_if, cfg->ipv4.addr, cfg->ipv4.mask, cfg->ipv4.gw);
        net_dhcpd_stop(net_if);
        if (net_dhcpd_start(net_if))
            return -1;
    }
    else
    {
        return -1;
    }

#ifdef CONFIG_IPV6_SUPPORT
    if (cfg->ip6_mode == IP6_ADDR_SERVER)
    {
        net_ip6_server_start(net_if);
    }
#endif /* CONFIG_IPV6_SUPPORT */

    if (cfg->default_output)
        net_if_set_default(net_if);


    return 0;
}

/*!
    \brief      Get IP address infomation
    \param[in]  vif_idx: index of the wifi vif
    \param[out] cfg: pointer to the IP address infomation
    \retval     0 on success and != 0 if error occured.
*/
int wifi_get_vif_ip(int vif_idx, struct wifi_ip_addr_cfg *cfg)
{
    struct netif *net_if;

    if (vif_idx >= CFG_VIF_NUM)
        return -1;

    net_if = vif_idx_to_net_if(vif_idx);
    if (!net_if)
        return -1;

    if (!net_dhcp_address_obtained(net_if))
        cfg->mode = IP_ADDR_DHCP_CLIENT;
    else
        cfg->mode = IP_ADDR_STATIC_IPV4;

    cfg->default_output = false;

    net_if_get_ip(net_if, &(cfg->ipv4.addr), &(cfg->ipv4.mask), &(cfg->ipv4.gw));
    net_get_dns(&(cfg->ipv4.dns));

    return 0;
}

#ifdef CONFIG_IPV6_SUPPORT
/*!
    \brief      Check if IPv6 address is got
    \param[in]  vif_idx: index of the wifi vif
    \param[out] none
    \retval     1 if is got and 0 otherwise.
*/
uint8_t wifi_ipv6_is_got(int vif_idx)
{
    struct netif *net_if;

    if (vif_idx >= CFG_VIF_NUM)
        return 0;

    net_if = vif_idx_to_net_if(vif_idx);
    if (!net_if)
        return 0;

    if (!ip6_addr_isany(ip_2_ip6(&net_if->ip6_addr[1])) && !ip6_addr_isinvalid(net_if->ip6_addr_state[1]))
        return 1;

    return 0;
}

/*!
    \brief      Get IPv6 address
    \param[in]  vif_idx: index of the wifi vif
    \param[out] ip6_local: pointer to the IPv6 local address
    \param[out] ip6_unique: pointer to the IPv6 unique address
    \retval     0 on success and != 0 if error occured.
*/
int wifi_get_vif_ip6(int vif_idx, char *ip6_local, char *ip6_unique)
{
    struct netif *net_if;
    char *local = NULL;
    char *unique = NULL;

    if (vif_idx >= CFG_VIF_NUM)
        return -1;

    net_if = vif_idx_to_net_if(vif_idx);
    if (!net_if)
        return -1;

    local = ip6addr_ntoa(ip_2_ip6(&net_if->ip6_addr[0]));
    sys_memcpy(ip6_local, local, strlen(local));
    if (wifi_ipv6_is_got(vif_idx)) {
        unique = ip6addr_ntoa(ip_2_ip6(&net_if->ip6_addr[1]));
        sys_memcpy(ip6_unique, unique, strlen(unique));
    }

    return 0;
}

/*!
    \brief      Set IPv6 unique address invalid
    \param[in]  net_if: Pointer to the net_if structure
    \param[out] none
    \retval     none
*/
void wifi_ip6_unique_addr_set_invalid(void *net_if)
{
    struct netif *netif = (struct netif *)net_if;

    if (!netif)
        return;

    netif->ip6_addr_state[1] = IP6_ADDR_INVALID;
    ip6_addr_set_zero(ip_2_ip6(&netif->ip6_addr[1]));
}
#endif /* CONFIG_IPV6_SUPPORT */

/*!
    \brief      Calculate checksum
    \param[in]  dataptr: data to calculate checksum
    \param[in]  len: data length
    \param[out] none
    \retval     checksum of data.
*/
uint16_t wifi_ip_chksum(const void *dataptr, int len)
{
    return net_ip_chksum(dataptr, len);
}
