/*!
    \file    wifi_netlink.h
    \brief   Header file of wifi netlink.

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

#ifndef _WIFI_NETLINK_H_
#define _WIFI_NETLINK_H_

#ifdef __cplusplus
extern "C" {
#endif
/*============================ INCLUDES ======================================*/
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "mac_types.h"
#include "wifi_vif.h"
#include "dbg_print.h"

/*============================ MACROS ========================================*/
// NVDS keys for the namespace "wifi_info"
#define WIFI_AUTO_CONN_EN               "auto_conn_en"
#define WIFI_AUTO_CONN_AP_INFO          "joined_ap"

/*============================ MACRO FUNCTIONS ===============================*/
#define netlink_printf(fmt, ...)        dbg_print(NOTICE, fmt, ## __VA_ARGS__)

/*============================ STRUCTURES ===============================*/
// WiFi work status
enum wifi_work_status_t {
    WIFI_CLOSED,            /* WiFi closed, pmu/rcc off and task deleted. */
    WIFI_CLOSING,           /* WiFi cloing, shutdown wifi tasks and power off. */
    WIFI_RUNNING,           /* WiFi running, pmu/rcc on and task running. */
};

typedef struct auto_conn_info {
    /* ip_addr */
    uint32_t ip_addr;
    /* channel */
    uint8_t channel;
    /* SSID to connect to */
    struct mac_ssid ssid;
    /* AP password/PSK passed as a string (i.e. MUST be terminated by a null byte) */
    struct key_info key;
} auto_conn_info_t;

/*============================ LOCAL VARIABLES ===============================*/
extern uint8_t wifi_work_status;
extern const char *wifi_closed_warn;
extern const char *wifi_closing_warn;
#define WIFI_CLOSED_CHECK(verbose) \
    do {    \
        if (wifi_work_status == WIFI_CLOSED) {                       \
            if (verbose)                                                \
                netlink_printf("%s(): %s", __func__, wifi_closed_warn); \
            return -1;                                                  \
        } else if (wifi_work_status == WIFI_CLOSING) {               \
            if (verbose)                                                \
                netlink_printf("%s(): %s", __func__, wifi_closing_warn);\
            return -1;                                                  \
        }                                                               \
    } while (0);

/*============================ PROTOTYPES ====================================*/
int wifi_netlink_dbg_close(void);
int wifi_netlink_dbg_open(void);
void wifi_netlink_wireless_mode_print(uint32_t wireless_mode);
int wifi_netlink_status_print(void);
int wifi_netlink_scan_set(int vif_idx, uint8_t channel);
int wifi_netlink_scan_set_with_ssid(int vif_idx, char *ssid, uint8_t channel);
int wifi_netlink_scan_set_with_extraie(int vif_idx, uint8_t channel,
                                            uint8_t *extra_ie, uint32_t extra_ie_len);
int wifi_netlink_scan_results_get(int vif_idx, struct macif_scan_results *results);
void wifi_netlink_scan_result_print(int idx, struct mac_scan_result *result);
int wifi_netlink_scan_results_print(int vif_idx, void (*callback)(int, struct mac_scan_result *));
int wifi_netlink_candidate_ap_find(int vif_idx, uint8_t *bssid, char *ssid, struct mac_scan_result *candidate);
int wifi_netlink_connect_req(int vif_idx, struct sta_cfg *cfg);
int wifi_netlink_associate_done(int vif_idx, void *ind_param);
int wifi_netlink_dhcp_done(int vif_idx);
int wifi_netlink_disconnect_req(int vif_idx);
int wifi_netlink_auto_conn_set(uint8_t auto_conn_enable);
uint8_t wifi_netlink_auto_conn_get(void);
int wifi_netlink_joined_ap_store(struct sta_cfg *cfg, uint32_t ip);
int wifi_netlink_joined_ap_load(int vif_idx);
int wifi_netlink_ps_mode_set(int vif_idx, uint8_t psmode);
int wifi_netlink_enable_vif_ps(int vif_idx);
int wifi_netlink_ap_start(int vif_idx, struct ap_cfg *cfg);
int wifi_netlink_ap_stop(int vif_idx, uint16_t deauth_reason);

int wifi_netlink_channel_set(uint32_t channel);
int wifi_netlink_monitor_start(int vif_idx, struct wifi_monitor *cfg);

int wifi_netlink_priv_req(uint32_t type, uint32_t param1, uint32_t param2, uint32_t *result);
int wifi_netlink_ext_priv_req(uint32_t type, uint32_t param1, uint32_t param2, uint32_t param3,
                                        uint32_t param4, uint32_t *result);
int wifi_netlink_listen_interval_set(uint8_t interval);

int wifi_netlink_twt_setup(int vif_idx, struct macif_twt_setup_t *param);
int wifi_netlink_twt_teardown(int vif_idx, uint8_t id, uint8_t neg_type);
int wifi_netlink_fix_rate_set(int sta_idx, int fixed_rate_idx);
int wifi_netlink_sys_stats_get(uint32_t *doze_time, uint32_t *stats_time);
int wifi_netlink_roaming_rssi_set(int vif_idx, int8_t rssi_thresh);
int8_t wifi_netlink_roaming_rssi_get(int vif_idx);

int wifi_netlink_start(void);
void wifi_netlink_stop(void);
int wifi_netlink_wifi_open(void);
void wifi_netlink_wifi_close(void);

uint8_t wifi_netlink_status_get(void);

/*============================ IMPLEMENTATION ================================*/
#ifdef __cplusplus
}
#endif

#endif /* _WIFI_NETLINK_H_ */
