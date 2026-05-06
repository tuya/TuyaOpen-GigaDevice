/*!
    \file    atcmd_wifi.c
    \brief   AT command WiFi part for GD32VW55x SDK

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

/*!
    \brief      the AT command connect the AP
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cw_ap_cur_join(int argc, char **argv)
{
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(WIFI_VIF_INDEX_DEFAULT);
    struct sta_cfg *cfg = &wvif->sta.cfg;
    int vif_idx = WIFI_VIF_INDEX_DEFAULT;
    int8_t sta_rssi;
    int reason;

    AT_RSP_START(512);
    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            sta_rssi = macif_vif_sta_rssi_get(vif_idx);
            AT_RSP("+CWJAP_CUR:%s,"MAC_FMT",%d,%d\r\n", cfg->ssid,
                MAC_ARG_UINT8(cfg->bssid), cfg->channel, sta_rssi);
        } else {
            goto Error;
        }
    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION)
            goto Usage;
        else
            goto Error;
    } else if (argc == 3) {
        char *ssid = at_string_parse(argv[1]);
        char *password = at_string_parse(argv[2]);
        if (ssid == NULL) {
            goto Error;
        }
        if ((reason = wifi_management_connect(ssid, password, true))) {
            goto Error;
        }
        if (!wifi_vif_is_sta_connected(vif_idx)) {
            AT_TRACE("AT+CWJAP_CUR=%s failed\r\n", ssid);
            goto Error;
        } else {
            AT_RSP("WIFI CONNECTED\r\n");
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();

    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+CWJAP_CUR=<ssid>,<pwd>\r\n");
    AT_RSP_OK();
    return;
}

static void at_scan_result_print(char *out, int space, int idx, struct mac_scan_result *result)
{
    char *akm;
    char cipher[64];

    if (result->ssid.length < sizeof(result->ssid.array))
        result->ssid.array[result->ssid.length] = '\0';
    else
        result->ssid.array[result->ssid.length - 1] = '\0';

    if (result->akm & CO_BIT(MAC_AKM_NONE)) {
        akm = "OPEN";
    } else if (result->akm == CO_BIT(MAC_AKM_PRE_RSN)) {
        akm = "WEP";
    } else if (result->akm & CO_BIT(MAC_AKM_WAPI_CERT) ||
        result->akm & CO_BIT(MAC_AKM_WAPI_PSK)) {
        akm = "WAPI";
    } else if ((result->akm & CO_BIT(MAC_AKM_SAE))
        && (result->akm & CO_BIT(MAC_AKM_PSK))) {
        akm = "WPA2/WPA3";
    } else if (result->akm & CO_BIT(MAC_AKM_SAE)) {
        akm = "WPA3";
    } else if ((result->akm & CO_BIT(MAC_AKM_PRE_RSN))
        && (result->akm & CO_BIT(MAC_AKM_PSK))) {
        akm = "WPA/WPA2";
    } else if (result->akm & CO_BIT(MAC_AKM_PRE_RSN)) {
        akm = "WPA";
    } else {
        akm = "WPA2";
    }

    sys_memset(cipher, 0, sizeof(cipher));
    wifi_wpa_cipher_name(result->pairwise_cipher, cipher, 64);

    co_snprintf(out, space, "+CWLAP: %s, %d, "MAC_FMT", %2d, %s %s\r\n",
                (char *)result->ssid.array,
                result->rssi,
                MAC_ARG(result->bssid.array),
                wifi_freq_to_channel(result->chan->freq),
                akm, cipher);
    return;
}

/*!
    \brief      the AT command scan and show nearby AP information
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cw_ap_list(int argc, char **argv)
{
    char *ssid = NULL;
    uint32_t ssid_len;
    uint32_t result_cnt = 0;
    struct macif_scan_results *results = NULL;
    char *out = NULL;
    int idx;

    AT_RSP_START(2048);
    if (argc > 2 || argc < 1) {
        goto Error;
    }

    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            ssid_len = strlen((const char *)argv[1]) - 2;
            if (ssid_len > MAC_SSID_LEN)
            {
                AT_TRACE("SSID's length should be less than %d\r\n", MAC_SSID_LEN);
                goto Error;
            }
            ssid = at_string_parse(argv[1]);
            if (ssid == NULL) {
                goto Error;
            }
        }
    }

    results = (struct macif_scan_results *)sys_malloc(sizeof(struct macif_scan_results));
    if (NULL == results) {
        AT_TRACE("alloc results failed\r\n");
        goto Error;
    }

    out = sys_zalloc(256);
    if (NULL == out) {
        AT_TRACE("alloc out failed\r\n");
        goto Error;
    }

    if (wifi_management_scan(true, ssid)) {
        AT_TRACE("scan failed\r\n");
        goto Error;
    }

    if (wifi_netlink_scan_results_get(WIFI_VIF_INDEX_DEFAULT, results)) {
        AT_TRACE("get scan results failed\r\n");
        goto Error;
    }

    result_cnt = results->result_cnt;
    for (idx = 0; idx < result_cnt; idx++) {
        at_scan_result_print(out, 256, idx, &results->result[idx]);
        AT_RSP(out);
    }

    sys_mfree(results);
    sys_mfree(out);

    AT_RSP_OK();
    return;

Error:
    if (results)
        sys_mfree(results);
    if (out)
        sys_mfree((out));

    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+CWLAP=[ssid]\r\n");
    AT_RSP_OK();
    return;
}

void at_cw_mode_cur(int argc, char **argv)
{
    char *endptr = NULL;
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(WIFI_VIF_INDEX_DEFAULT);
    uint32_t wvif_type = wvif->wvif_type;
    int mode = -1;
    wifi_ap_auth_mode_t auth_mode = AUTH_MODE_WPA2_WPA3;

    AT_RSP_START(256);
    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] != AT_QUESTION) {
            goto Error;
        }
#ifdef CFG_WIFI_CONCURRENT
        if (wifi_management_concurrent_get()) {
            mode = 3;
        } else
#endif
        if (wvif_type == WVIF_STA) {
            mode = 1;
        } else if (wvif_type == WVIF_MONITOR) {
            mode = 0;
        } else if (wvif_type == WVIF_AP) {
            mode = 2;
        }
        AT_RSP("+CWMODE_CUR:%d\r\n", mode);
    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            mode = (uint32_t)strtoul((const char *)argv[1], &endptr, 10);
#ifdef CFG_WIFI_CONCURRENT
            if ((*endptr != '\0') || (mode > 3)) {
#else
            if ((*endptr != '\0') || (mode > 2)) {
#endif /* CFG_WIFI_CONCURRENT */
                goto Error;
            }
#ifdef CFG_WIFI_CONCURRENT
            if (mode == 3) {
                wifi_management_concurrent_set(1);
            } else {
                wifi_management_concurrent_set(0);
            }
#endif /* CFG_WIFI_CONCURRENT */
            if (mode == 2) {
                if (wifi_management_ap_start("GigaDevice", "GDSU@2022", 11, auth_mode, false)) {
                    goto Error;
                }
            }
            if (mode == 1) {
                if (wifi_management_sta_start()) {
                    goto Error;
                }
            }
            if (mode == 0) {
                if (wifi_management_monitor_start(1, NULL)) {
                    goto Error;
                }
            }
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
#ifdef CFG_WIFI_CONCURRENT
    AT_RSP("+CWMODE_CUR=<mode:0-3>\r\n");
#else
    AT_RSP("+CWMODE_CUR=<mode:0-2>\r\n");
#endif /* CFG_WIFI_CONCURRENT */
    AT_RSP_OK();
    return;
}

/*!
    \brief      the AT command show wifi status
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cw_status(int argc, char **argv)
{
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(WIFI_VIF_INDEX_DEFAULT);

    AT_RSP_START(1024);
    if (argc == 1) {
        if ((wvif->wvif_type == WVIF_AP)
            && (wvif->ap.ap_state == WIFI_AP_STATE_STARTED)) {
            AT_RSP("+CWSTATUS: SoftAP, %s, %s, %d\r\n",
                wvif->ap.cfg.ssid,
                wvif->ap.cfg.passphrase,
                wvif->ap.cfg.channel);
        } else if (wvif->wvif_type == WVIF_MONITOR) {
            AT_RSP("+CWSTATUS: MONITOR, %d, "MAC_FMT"\r\n",
                wvif->monitor.channel,
                MAC_ARG(wvif->mac_addr.array));
        } else if (wvif->wvif_type == WVIF_STA) {
            AT_RSP("+CWSTATUS: STA, ");
            if (wvif->sta.state == WIFI_STA_STATE_CONNECTED) {
                AT_RSP("connected, %s, %d, "MAC_FMT"\r\n",
                    wvif->sta.cfg.ssid,
                    wvif->sta.cfg.channel,
                    MAC_ARG(wvif->sta.cfg.bssid));
            } else {
                AT_RSP("disconnected\r\n");
            }
        }
#ifdef CFG_WIFI_CONCURRENT
        if (wifi_management_concurrent_get()) {
            wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(WIFI_VIF_INDEX_SOFTAP_MODE);
            if ((wvif->wvif_type == WVIF_AP)
            && (wvif->ap.ap_state == WIFI_AP_STATE_STARTED)) {
                AT_RSP("+CWSTATUS: SoftAP, %s, %s, %d\r\n",
                    wvif->ap.cfg.ssid,
                    wvif->ap.cfg.passphrase,
                    wvif->ap.cfg.channel);
            }
        }
#endif /* CFG_WIFI_CONCURRENT */
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
}

/*!
    \brief      the AT command disconnect WiFi from AP
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cw_ap_quit(int argc, char **argv)
{
    if (argc == 1) {
        wifi_management_disconnect();
    } else {
        goto Error;
    }

    AT_RSP_DIRECT("OK\r\n", 4);
    return;

Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
}

/*!
    \brief      the AT command start softAP
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cw_ap_cur_start(int argc, char **argv)
{
    AT_RSP_START(128);
    if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            goto Error;
        }
    } else if (argc == 5) {
        char *ssid = at_string_parse(argv[1]);
        char *pwd = at_string_parse(argv[2]);
        uint32_t chl, hidden;
        char *endptr = NULL;
        wifi_ap_auth_mode_t auth_mode = AUTH_MODE_WPA2_WPA3;

        if ((ssid == NULL) || (pwd == NULL)) {
            goto Error;
        }

        chl = (uint32_t)strtoul((const char *)argv[3], &endptr, 10);
        if ((*endptr != '\0') || (chl > 13) || (chl < 1)) {
            goto Error;
        }

        hidden = (uint32_t)strtoul((const char *)argv[4], &endptr, 10);
        if ((*endptr != '\0') || (hidden > 1)) {
            goto Error;
        }

        if (wifi_management_ap_start(ssid, pwd, chl, auth_mode, hidden)) {
            goto Error;
        }

        AT_TRACE("SoftAP successfully started!\r\n");
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+CWSAP_CUR=<ssid>,<pwd>,<chl:1-13>,<hidden:0-1>\r\n");
    AT_RSP_OK();
    return;
}

/*!
    \brief      the AT command set softAP IPv4 address
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cip_ap(int argc, char **argv)
{
    int vif_idx = WIFI_VIF_INDEX_DEFAULT;
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(vif_idx);
    struct wifi_ip_addr_cfg ip_cfg;

    AT_RSP_START(256);
    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
#ifdef CFG_WIFI_CONCURRENT
            if (wifi_management_concurrent_get()) {
                vif_idx = WIFI_VIF_INDEX_SOFTAP_MODE;
                wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(vif_idx);
            }
#endif
            if (wvif->wvif_type == WVIF_AP) {
                if (!wifi_get_vif_ip(vif_idx, &ip_cfg)) {
                    AT_RSP("+CIPAP:ip:\""IP_FMT"\"\r\n", IP_ARG(ip_cfg.ipv4.addr));
                    AT_RSP("+CIPAP:gateway:\""IP_FMT"\"\r\n", IP_ARG(ip_cfg.ipv4.gw));
                    AT_RSP("+CIPAP:netmask:\""IP_FMT"\"\r\n", IP_ARG(ip_cfg.ipv4.mask));
#ifdef CONFIG_IPV6_SUPPORT
                    char ip6_local[IPV6_ADDR_STRING_LENGTH_MAX] = {0};
                    char ip6_unique[IPV6_ADDR_STRING_LENGTH_MAX] = {0};
                    if (!wifi_get_vif_ip6(vif_idx, ip6_local, ip6_unique)) {
                        AT_RSP("+CIPAP:ip6ll:\"%s\"\r\n", ip6_local);
                    } else {
                        goto Error;
                    }
#endif
                } else {
                    AT_TRACE("get ip error.\r\n");
                    goto Error;
                }
            } else {
                AT_TRACE("please start softap.\r\n");
                goto Error;
            }
        } else {
            goto Error;
        }
    } else if (argc >= 2 && argc <= 4) {
        char *ap_ip = NULL, *ap_gw = NULL, *ap_mask = NULL;
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            ap_ip = at_string_parse(argv[1]);
            if (ap_ip == NULL) {
                goto Error;
            }
            if (argc >= 3) {
                ap_gw = at_string_parse(argv[2]);
                if (ap_gw == NULL) {
                    goto Error;
                }
            }
            if (argc == 4) {
                ap_mask = at_string_parse(argv[3]);
                if (ap_mask == NULL) {
                    goto Error;
                }
            }
#ifdef CFG_WIFI_CONCURRENT
            if (wifi_management_concurrent_get()) {
                vif_idx = WIFI_VIF_INDEX_SOFTAP_MODE;
                wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(vif_idx);
            }
#endif
            if (wvif->wvif_type == WVIF_AP) {
                ip_cfg.mode = IP_ADDR_DHCP_SERVER;
                if (cli_parse_ip4(ap_ip, &ip_cfg.ipv4.addr, NULL) != 0) {
                    goto Error;
                }

                if (ap_gw == NULL) {
                    sys_memcpy(&ip_cfg.ipv4.gw, &ip_cfg.ipv4.addr, sizeof(ip_cfg.ipv4.gw));
                } else {
                    if (cli_parse_ip4(ap_gw, &ip_cfg.ipv4.gw, NULL) != 0) {
                        goto Error;
                    }
                }

                if (ap_mask == NULL) {
                    ip_cfg.ipv4.mask = PP_HTONL(0xFFFFFF00UL);
                } else {
                    if (cli_parse_ip4(ap_mask, &ip_cfg.ipv4.mask, NULL) != 0) {
                        goto Error;
                    }
                }

                if (wifi_set_vif_ip(vif_idx, &ip_cfg)) {
                    AT_TRACE("failed to set softap ip.\r\n");
                    goto Error;
                }
            } else {
                AT_TRACE("please start softap.\r\n");
                goto Error;
            }
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+CIPAP=<\"ip\">[,<\"gateway\">,<\"netmask\">]\r\n");
    AT_RSP_OK();
    return;
}

/*!
    \brief      the AT command show station information that has been connected to the softAP
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cw_ap_client_list(int argc, char **argv)
{
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(WIFI_VIF_INDEX_DEFAULT);

    AT_RSP_START(1024);
    if (argc == 1) {
        if ((wvif->wvif_type == WVIF_AP)
            && (wvif->ap.ap_state == WIFI_AP_STATE_STARTED)) {
            uint16_t info[AT_MAX_STATION_NUM * AT_ETH_ALEN];
            uint32_t client_num, i;
            client_num = macif_vif_ap_assoc_info_get(WIFI_VIF_INDEX_DEFAULT, info);
            for (i = 0; i < client_num; i++) {
                AT_RSP("+CWLIF: [%d] "MAC_FMT"\r\n", i, MAC_ARG(info + i * AT_ETH_ALEN));
            }
        } else {
#ifdef CFG_WIFI_CONCURRENT
            if (wifi_management_concurrent_get()) {
                wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(WIFI_VIF_INDEX_SOFTAP_MODE);
                if ((wvif->wvif_type == WVIF_AP)
                    && (wvif->ap.ap_state == WIFI_AP_STATE_STARTED)) {
                    uint16_t info[AT_MAX_STATION_NUM * AT_ETH_ALEN];
                    uint32_t client_num, i;
                    client_num = macif_vif_ap_assoc_info_get(WIFI_VIF_INDEX_SOFTAP_MODE, info);
                    for (i = 0; i < client_num; i++) {
                        AT_RSP("+CWLIF: [%d] "MAC_FMT"\r\n", i, MAC_ARG(info + i * AT_ETH_ALEN));
                    }
                } else {
                    goto Error;
                }
            } else
#endif /* CFG_WIFI_CONCURRENT */
            {
                goto Error;
            }
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
}

/*!
    \brief      the AT command configure whether to connect AP automatically after power on
    \param[in]  argc: number of parameters
    \param[in]  argv: the pointer to the array that holds the parameters
    \param[out] none
    \retval     none
*/
void at_cw_auto_connect(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t auto_conn;

    AT_RSP_START(128);
    if (argc == 1) {
        if (argv[0][strlen(argv[0]) - 1] == AT_QUESTION) {
            auto_conn = wifi_netlink_auto_conn_get();
            AT_RSP("+CWAUTOCONN: %d\r\n", auto_conn ? 1 : 0);
        } else {
            goto Error;
        }
    } else if (argc == 2) {
        if (argv[1][0] == AT_QUESTION) {
            goto Usage;
        } else {
            auto_conn = (uint32_t)strtoul((const char *)argv[1], &endptr, 10);
            if ((*endptr != '\0') || (auto_conn > 1)) {
                goto Error;
            }
            auto_conn = auto_conn ? 1 : 0;
            wifi_netlink_auto_conn_set(auto_conn);
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
Usage:
    AT_RSP("+CWAUTOCONN=<enable>\r\n");
    AT_RSP_OK();
    return;
}
