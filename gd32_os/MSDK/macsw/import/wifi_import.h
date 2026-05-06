/*!
    \file    wifi_import.h
    \brief   wifi functions import for WIFI lib.

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

#ifndef __WIFI_IMPORT_H__
#define __WIFI_IMPORT_H__

#include "macif_types.h"

/*============================ PROTOTYPES ====================================*/
extern void *vif_idx_to_mac_vif(uint8_t vif_idx);
extern void *wvif_to_mac_vif(void *wvif);
extern void *vif_idx_to_net_if(uint8_t vif_idx);
extern void *vif_idx_to_wvif(uint8_t vif_idx);
extern int wvif_to_vif_idx(void *wvif);
extern uint8_t wifi_vif_sta_uapsd_get(int vif_idx);
extern int wifi_vif_uapsd_queues_set(int vif_idx, uint8_t uapsd_queues);
extern uint8_t * wifi_vif_mac_addr_get(int vif_idx);
extern void wifi_vif_mac_vif_set(int vif_idx, void *mac_vif);
extern void wifi_vif_ap_id_set(int vif_idx, uint8_t ap_id);
extern uint8_t wifi_vif_ap_id_get(int vif_idx);
extern int wifi_vifs_init(struct mac_addr *base_mac_addr);

extern int wifi_wpa_scan_sock_get(int vif_idx);
extern int wifi_wpa_scan_sock_set(int vif_idx, int scan_sock);
extern int wifi_wpa_conn_sock_get(int vif_idx);
extern int wifi_wpa_conn_sock_set(int vif_idx, int conn_sock);
extern int wifi_wpa_ftm_sock_get(int vif_idx);
extern int wifi_wpa_ftm_sock_set(int vif_idx, int ftm_sock);

extern int wifi_netlink_msg_forward(int vif_idx, void *msg, bool from_wpa);

extern void dhcpd_delete_ipaddr_by_macaddr(uint8_t *mac_addr);

extern void wlan_exti_enter(void);
extern void wlan_exti_exit(void);

extern void wifi_task_ready(enum wifi_task_id task_id);
extern void wifi_task_terminated(enum wifi_task_id task_id);
extern int wifi_wait_terminated(enum wifi_task_id task_id);

extern void hw_crc32_enable(void);
extern uint32_t hw_crc32_single(uint32_t data);
extern void hw_crc32_disable(void);
extern bool wifi_is_not_idle(void);
extern void wifi_wakelock_acquire(void);
extern void wifi_wakelock_release(void);
extern void wifi_lpds_enter(void);
extern void wifi_lpds_exit(void);
extern void wifi_lpds_preconfig(uint8_t settle_time);
extern int rf_efuse_is_wifi_disabled(void);

#endif /* __WIFI_IMPORT_H__ */
