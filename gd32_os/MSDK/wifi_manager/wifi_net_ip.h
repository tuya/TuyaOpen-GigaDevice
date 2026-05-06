/*!
    \file    wifi_net_ip.h
    \brief   Header file for definition of IP configuration related.

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

#ifndef _WIFI_NET_IP_H_
#define _WIFI_NET_IP_H_

#include "app_cfg.h"

/**
 * Enum for IP address configuration mode
 */
enum wifi_ip_addr_mode
{
    IP_ADDR_NONE,
    IP_ADDR_STATIC_IPV4,
    IP_ADDR_DHCP_CLIENT,
    IP_ADDR_DHCP_SERVER,
};

#ifdef CONFIG_IPV6_SUPPORT
enum wifi_ip6_addr_mode
{
    IP6_ADDR_NONE,
    IP6_ADDR_SERVER,
};
#endif /* CONFIG_IPV6_SUPPORT */

/**
 * Fully Hosted IP address configuration (only IPv4 for now)
 */
struct wifi_ip_addr_cfg
{
    /**
     * Select how to configure ip address when calling @ref wifi_set_vif_ip
     * Indicate how ip was configured when updated by @ref wifi_get_vif_ip
     */
    enum wifi_ip_addr_mode mode;
#ifdef CONFIG_IPV6_SUPPORT
    enum wifi_ip6_addr_mode ip6_mode;
#endif /* CONFIG_IPV6_SUPPORT */
    /**
     * Whether interface must be the default output interface
     * (Unspecified when calling @ref wifi_get_vif_ip)
     */
    bool default_output;
    union
    {
        /**
         * IPv4 config.
         * Must be set when calling @ref wifi_set_vif_ip with @p
         * mode==IP_ADDR_STATIC_IPV4
         * It is always updated by @ref wifi_get_vif_ip independently of @p mode value
         */
        struct
        {
            /**
             * IPv4 address
             */
            uint32_t addr;
            /**
             * IPv4 address mask
             */
            uint32_t mask;
            /**
             * IPv4 address of the gateway
             */
            uint32_t gw;
            /**
             * DNS server to use. (Ignored if set to 0)
             */
            uint32_t dns;
        } ipv4;
        /**
         * DHCP config.
         * Must be set when calling @ref wifi_set_vif_ip with @p
         * addr_mode==IP_ADDR_DHCP_CLIENT
         */
        struct
        {
            /**
             * Timeout, in ms, to obtained an IP address
             */
            uint32_t to_ms;
        } dhcp;
    };
};

uint16_t wifi_ip_chksum(const void *dataptr, int len);
int wifi_set_vif_ip(int vif_idx, struct wifi_ip_addr_cfg *cfg);
int wifi_get_vif_ip(int vif_idx, struct wifi_ip_addr_cfg *cfg);
#ifdef CONFIG_NAPT
int wifi_set_softap_napt_enable(int vif_idx);
#endif
#ifdef CONFIG_IPV6_SUPPORT
int wifi_get_vif_ip6(int vif_idx, char *ip6_local, char *ip6_unique);
uint8_t wifi_ipv6_is_got(int vif_idx);
void wifi_ip6_unique_addr_set_invalid(void *net_if);
#endif /* CONFIG_IPV6_SUPPORT */

#endif /* _WIFI_NET_IP_H_ */
