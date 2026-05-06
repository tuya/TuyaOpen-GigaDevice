/*!
    \file    wifi_management.h
    \brief   WiFi management for GD32VW55x SDK.

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

#ifndef _WIFI_MANAGEMENT_H_
#define _WIFI_MANAGEMENT_H_

#ifdef __cplusplus
extern "C" {
#endif
/*============================ INCLUDES ======================================*/
#include "wifi_eloop.h"
#include "wifi_netlink.h"

/*============================ MACROS ========================================*/
#define WIFI_SM_ERROR                           1
#define WIFI_SM_WARNING                         2
#define WIFI_SM_NOTICE                          3
#define WIFI_SM_INFO                            4
#define WIFI_SM_DEBUG                           5

#define WIFI_SM_LOG_LEVEL                       WIFI_SM_NOTICE
#define WIFI_SM_DEBUG_ENABLE

#define WIFI_MGMT_ROAMING_RETRY_LIMIT           100
#define WIFI_MGMT_ROAMING_RETRY_INTERVAL        2500    // unit: ms (not less than EAPOL_TIMEROUT)
#define WIFI_MGMT_ROAMING_RETRY_UNLIMITED       0xFFFFFFFFUL

#define WIFI_MGMT_CONNECT_RETRY_LIMIT           3       // max retry times
#define WIFI_MGMT_CONNECT_RETRY_INTERVAL        2000    // unit: ms (not less than EAPOL_TIMEROUT)

#define WIFI_MGMT_MAX_RETRY_INTERVAL            7200000 //300000  // unit: ms

#define WIFI_MGMT_CONNECT_BLOCK_TIME    \
    (((WIFI_MGMT_CONNECT_RETRY_LIMIT) * (WIFI_MGMT_CONNECT_RETRY_LIMIT - 1) * \
    (WIFI_MGMT_CONNECT_RETRY_INTERVAL) >> 1) + 14000)   // 20s in total
#define WIFI_MGMT_WPS_CONNECT_BLOCK_TIME        120000  // 2 minutes
#define WIFI_MGMT_DHCP_POLLING_LIMIT            200     // max polling times
#define WIFI_MGMT_DHCP_POLLING_INTERVAL         100     // unit: ms

#ifdef CONFIG_IPV6_SUPPORT
/** Router solicitations are sent in 4 second intervals (see RFC 4861, ch. 6.3.7) */
#define WIFI_MGMT_IPV6_POLLING_INTERVAL         4000     // unit: ms
#endif /* CONFIG_IPV6_SUPPORT */

#define WIFI_MGMT_LINK_POLLING_INTERVAL         3000   // unit: ms
#define WIFI_MGMT_POLLING_SCAN_TRIGGER_POINT    10      // 10 * WIFI_MGMT_LINK_POLLING_INTERVAL
#define WIFI_MGMT_ROAMING_RSSI_RELATIVE_GAIN    10      // dBm

#define MGMT_TASK_STACK_SIZE                    1500 //672 //1024
#define MGMT_TASK_PRIORITY                      OS_TASK_PRIORITY(2) //OS_TASK_PRIORITY(1)
#define MGMT_TASK_QUEUE_SIZE                    24
#define MGMT_TASK_QUEUE_ITEM_SIZE               sizeof(eloop_message_t)

#define MGMT_WAIT_QUEUE_MSG_SIZE                5

/*============================ MACRO FUNCTIONS ===============================*/
#if defined(WIFI_SM_DEBUG_ENABLE)
#define wifi_sm_printf(level, fmt, ...) do { \
        if (level <= WIFI_SM_LOG_LEVEL) \
            dbg_print(NOTICE, fmt, ## __VA_ARGS__);     \
    } while (0)
#else
#define wifi_sm_printf(...)
#endif

#define COMPILE_TIME_ASSERT(constant_expr)    \
do {                        \
    switch(0) {             \
        case 0:             \
        case constant_expr: \
        ;                   \
    }                       \
} while(0)


/* TODO: To retrieve the roaming retry strategy from the configuration space */
#define WIFI_MGMT_UNLIMITED_ROAMING_RETRY()            (0)

#if WIFI_MGMT_EVENT_MAX > 0xFFF
#error max. of eloop event should not exceed 0xFFF
#endif

/*============================ TYPES =========================================*/
typedef enum {
    MAINTAIN_CONNECTION_IDLE,
    MAINTAIN_CONNECTION_WPS,
    MAINTAIN_CONNECTION_SCAN,
    MAINTAIN_CONNECTION_CONNECT,
    MAINTAIN_CONNECTION_HANDSHAKE,
    MAINTAIN_CONNECTION_DHCP,
    MAINTAIN_CONNECTION_CONNECTED,
} maintain_conn_state_t;

typedef enum {
    MAINTAIN_SOFTAP_INIT,
    MAINTAIN_SOFTAP_STARTED,
} maintain_softap_state_t;

typedef enum {
    MAINTAIN_MONITOR_INIT,
    MAINTAIN_MONITOR_STARTED,
} maintain_monitor_state_t;

#ifdef CFG_WPS
typedef enum {
    WPS_STATE_INIT = 0,
    WPS_STATE_SCAN,
    WPS_STATE_CONNECT,
    WPS_STATE_EAP_HANDSHAKE,
    WPS_STATE_DONE,
} wps_state_t;
#endif

typedef enum {
    WIFI_MGMT_EVENT_START = ELOOP_EVENT_MAX,

    /* For both STA and SoftAP */
    WIFI_MGMT_EVENT_INIT,  //5
    WIFI_MGMT_EVENT_SWITCH_MODE_CMD,
    WIFI_MGMT_EVENT_RX_MGMT,
    WIFI_MGMT_EVENT_RX_EAPOL,

    /* For STA only */
    WIFI_MGMT_EVENT_SCAN_CMD,
    WIFI_MGMT_EVENT_CONNECT_CMD,  //10
    WIFI_MGMT_EVENT_DISCONNECT_CMD,
    WIFI_MGMT_EVENT_AUTO_CONNECT_CMD,
    WIFI_MGMT_EVENT_WPS_CMD,

    WIFI_MGMT_EVENT_SCAN_DONE,
    WIFI_MGMT_EVENT_SCAN_FAIL,
    WIFI_MGMT_EVENT_SCAN_RESULT,  //16

    WIFI_MGMT_EVENT_EXTERNAL_AUTH_REQUIRED,  //17

    WIFI_MGMT_EVENT_ASSOC_SUCCESS,  //18

    WIFI_MGMT_EVENT_DHCP_START,
    WIFI_MGMT_EVENT_DHCP_SUCCESS,
    WIFI_MGMT_EVENT_DHCP_FAIL, //21

    WIFI_MGMT_EVENT_CONNECT_SUCCESS,
    WIFI_MGMT_EVENT_CONNECT_FAIL,

    WIFI_MGMT_EVENT_DISCONNECT,
    WIFI_MGMT_EVENT_ROAMING_START,

    WIFI_MGMT_EVENT_RX_UNPROT_DEAUTH, //26
    WIFI_MGMT_EVENT_RX_ACTION,

    /* For STA WPS */
    WIFI_MGMT_EVENT_WPS_SUCCESS, //28
    WIFI_MGMT_EVENT_WPS_FAIL,
    WIFI_MGMT_EVENT_WPS_CRED,

    /* For SoftAP only */
    WIFI_MGMT_EVENT_START_AP_CMD,  //31
    WIFI_MGMT_EVENT_STOP_AP_CMD,
    WIFI_MGMT_EVENT_AP_SWITCH_CHNL_CMD,

    WIFI_MGMT_EVENT_TX_MGMT_DONE, //34
    WIFI_MGMT_EVENT_CLIENT_ADDED,
    WIFI_MGMT_EVENT_CLIENT_REMOVED, //36

    /* For Monitor only */
    WIFI_MGMT_EVENT_MONITOR_START_CMD,

    /* For STA 802.1x EAP */
    WIFI_MGMT_EVENT_EAP_SUCCESS,

    WIFI_MGMT_EVENT_FT_AUTH, //39

    WIFI_MGMT_EVENT_FT_ROAMING_CMD,

    WIFI_MGMT_EVENT_MAX,
    WIFI_MGMT_EVENT_NUM = WIFI_MGMT_EVENT_MAX - WIFI_MGMT_EVENT_START - 1,
} wifi_management_event_t;

typedef enum {
    WIFI_MGMT_CONN_UNSPECIFIED = 1,
    WIFI_MGMT_CONN_NO_AP,
    WIFI_MGMT_CONN_AUTH_FAIL,
    WIFI_MGMT_CONN_ASSOC_FAIL,
    WIFI_MGMT_CONN_HANDSHAKE_FAIL,
    WIFI_MGMT_CONN_DHCP_FAIL,
    WIFI_MGMT_CONN_DPP_FAIL,
    WIFI_MGMT_CONN_WPS_FAIL,

    WIFI_MGMT_DISCON_REKEY_FAIL, //9
    WIFI_MGMT_DISCON_MIC_FAIL,
    WIFI_MGMT_DISCON_RECV_DEAUTH,
    WIFI_MGMT_DISCON_NO_BEACON,
    WIFI_MGMT_DISCON_AP_CHANGED,
    WIFI_MGMT_DISCON_FROM_UI,
    WIFI_MGMT_DISCON_UNSPECIFIED,
    WIFI_MGMT_DISCON_SA_QUERY_FAIL,
} wifi_discon_reason_t;

typedef enum {
    WIFI_MGMT_SCAN_SUCCESS,
    WIFI_MGMT_SCAN_START_FAIL,
    WIFI_MGMT_SCAN_FAIL,
} wifi_scan_fail_reason_t;

typedef enum {
    AUTH_MODE_OPEN = 0,
    AUTH_MODE_WEP,
    AUTH_MODE_WPA,
    AUTH_MODE_WPA2,
    AUTH_MODE_WPA_WPA2,
    AUTH_MODE_WPA2_WPA3,
    AUTH_MODE_WPA3,
    AUTH_MODE_UNKNOWN,
} wifi_ap_auth_mode_t;

typedef struct wifi_management_sm_data {
    uint32_t vif_idx;
    uint8_t init;
    union {
        maintain_conn_state_t MAINTAIN_CONNECTION_state;
        maintain_softap_state_t MAINTAIN_SOFTAP_state;
        maintain_monitor_state_t MAINTAIN_MONITOR_state;
    };
    wifi_management_event_t event;
    uint16_t reason;
    uint8_t *param;
    uint32_t param_len;

#ifdef CFG_WPS
    wps_state_t wps_state;
    struct wps_cred_t *wps_cred;
    uint8_t *wps_bcn;
    uint32_t wps_bcn_len;
#endif
    uint8_t dhcp_polling_count;
    uint8_t delayed_connect_retry;
    uint32_t retry_count;
    uint32_t retry_limit;
    uint32_t retry_interval;    /* in milliseconds */
//    uint8_t changed;

    uint8_t preroam_enable;
    uint8_t preroam_start;
    uint8_t polling_scan;
    uint16_t polling_scan_count;
    uint8_t preroam_bssid_bk[WIFI_ALEN];

    uint8_t scan_blocked;
} wifi_management_sm_data_t;

enum {
    MGMT_WAIT_EVT_SCAN_DONE = 0,
    MGMT_WAIT_EVT_CONN_DONE,
    MGMT_WAIT_EVT_DISCONN_DONE,
    MGMT_WAIT_EVT_AP_START_DONE,
    MGMT_WAIT_EVT_MONITOR_START_DONE,
    MGMT_WAIT_EVT_UNKONWN,
};

typedef struct mgmt_wait_evt {
    uint8_t vif;
    uint8_t evt;
    uint16_t reason;
} mgmt_wait_evt_t;

/*============================ GLOBAL VARIABLES ==============================*/
extern wifi_management_sm_data_t wifi_sm_data[];
extern void * wifi_mgmt_task_tcb;

/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/
void mgmt_connect_retry_param_set(wifi_management_sm_data_t *sm, uint8_t roaming_required);
void wifi_mgmt_cb_run_state_machine(void *eloop_data, void *user_ctx);
int wifi_management_concurrent_set(uint8_t enable);
int wifi_management_concurrent_get(void);
int wifi_management_roaming_set(uint8_t enable, int8_t rssi_th);
int wifi_management_roaming_get(int8_t *rssi_th);
int wifi_management_scan(uint8_t blocked, const char *ssid);
int wifi_management_connect(char *ssid, char *password, uint8_t blocked);
int wifi_management_connect_with_bssid(uint8_t *bssid, char *password, uint8_t blocked);
#ifdef CFG_8021x_EAP_TLS
int wifi_management_connect_with_eap_tls(char *ssid, const char *identity, const char *ca_cert,
                                    const char *client_key, const char *client_cert,
                                    const char *client_key_password, const char *phase1,
                                    uint8_t blocked);
#endif /* CFG_8021x_EAP_TLS */
int wifi_management_disconnect(void);
int wifi_management_ap_start(char *ssid, char *passwd, uint32_t channel, wifi_ap_auth_mode_t auth_mode, uint32_t hidden);
int wifi_management_ap_delete_client(uint8_t *client_mac_addr);
int wifi_management_ap_stop(void);
int wifi_management_sta_start(void);
int wifi_management_sta_auto_connect(void);
int wifi_management_monitor_start(uint8_t channel, cb_macif_rx monitor_cb);
int wifi_management_wps_start(bool is_pbc, char *pin, uint8_t blocked);
int wifi_management_init(void);
void wifi_management_deinit(void);

/*============================ IMPLEMENTATION ================================*/
#ifdef __cplusplus
}
#endif
#endif  /* _WIFI_MANAGEMENT_H_ */
