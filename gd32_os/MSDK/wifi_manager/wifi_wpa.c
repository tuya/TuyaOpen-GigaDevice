/*!
    \file    wifi_wpa.c
    \brief   wifi wpa interface for GD32VW55x SDK.

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

#include "wifi_export.h"
#include "sys/socket.h"
#include "wifi_vif.h"
#include "wifi_management.h"
#include "util.h"
#include "dbg_print.h"
#include "wifi_wpa.h"
#include "wifi_import.h"

// AKM string used by wpa_supplicant
const char * const wpa_akm_str[] =
{
    [MAC_AKM_NONE] = "NONE",
    [MAC_AKM_PRE_RSN] = NULL,
    [MAC_AKM_8021X] = "WPA-EAP",
    [MAC_AKM_PSK] = "WPA-PSK",
    [MAC_AKM_FT_8021X] = "FT_EAP",
    [MAC_AKM_FT_PSK] = "FT-PSK",
    [MAC_AKM_8021X_SHA256] = "WPA-EAP-SHA256",
    [MAC_AKM_PSK_SHA256] = "WPA-PSK-SHA256",
    [MAC_AKM_TDLS] = "TDLS",
    [MAC_AKM_SAE] = "SAE",
    [MAC_AKM_FT_OVER_SAE] = "FT-SAE",
    [MAC_AKM_8021X_SUITE_B] = "WPA-EAP-SUITE-B",
    [MAC_AKM_8021X_SUITE_B_192] = "WPA-EAP-SUITE-B-192",
    [MAC_AKM_FILS_SHA256] = "FILS-SHA256",
    [MAC_AKM_FILS_SHA384] = "FILS-SHA384",
    [MAC_AKM_FT_FILS_SHA256] = "FT-FILS-SHA256",
    [MAC_AKM_FT_FILS_SHA384] = "FT-FILS-SHA384",
    [MAC_AKM_OWE] = "OWE",
    [MAC_AKM_WAPI_CERT] = "WAPI-CERT",
    [MAC_AKM_WAPI_PSK] = "WAPI-PSK",
    [MAC_AKM_DPP] = "DPP"
};

// Cipher suites string used by wpa_supplicant
const char * const wpa_cipher_str[] =
{
    [MAC_CIPHER_WEP40] = "WEP40",
    [MAC_CIPHER_TKIP] = "TKIP",
    [MAC_CIPHER_CCMP] = "CCMP",
    [MAC_CIPHER_WEP104] = "WEP104",
    [MAC_CIPHER_WPI_SMS4] = "WPI_SMS4",
    [MAC_CIPHER_BIP_CMAC_128] = "AES-128-CMAC",
    [MAC_CIPHER_GCMP_128] = "GCMP",
    [MAC_CIPHER_GCMP_256] = "GCMP-256",
    [MAC_CIPHER_CCMP_256] = "CCMP-256",
    [MAC_CIPHER_BIP_GMAC_128] = "BIP-GMAC-128",
    [MAC_CIPHER_BIP_GMAC_256] = "BIP-GMAC-256",
    [MAC_CIPHER_BIP_CMAC_256] = "BIP-CMAC-256",
};

#ifdef CONFIG_WPA_SUPPLICANT

#ifdef CFG_WPS
#include "wps/wps.h"
#endif

#include "crypto_mbedtls.c"

// Global buffer to send wpa_supplicant command
static char wpa_cmd[WPA_MAX_CMD_SIZE];

#if defined (CFG_DPP)
// DPP bootstrapping method strings used by wpa_supplicant
const char *wpa_dpp_bootstrap_str[] =
{
    [WIFI_DPP_BOOTSTRAP_QRCODE] = "qrcode",
    [WIFI_DPP_BOOTSTRAP_PKEX] = "pkex",
};

// DPP curves strings used by wpa_supplicant
const char *wpa_dpp_curve_str[] =
{
    [DPP_CURVE_PRIME256v1] = "prime256v1",
    [DPP_CURVE_SECP384r1] = "secp384r1",
    [DPP_CURVE_SECP521r1] = "secp521r1",
    [DPP_CURVE_BRAINPOOLP256r1] = "brainpoolP256r1",
    [DPP_CURVE_BRAINPOOLP384r1] = "brainpoolP384r1",
    [DPP_CURVE_BRAINPOOLP512r1] = "brainpoolP512r1",
};
#endif

// wpa_supplicant configuration for all WIFI interfaces
struct wifi_wpa_tag wifi_wpa;

/**
 ****************************************************************************************
 * @brief  main function of wpa task
 *
 * @param[in] env  Not used
 ****************************************************************************************
 */
static void wifi_wpa_event_process(enum wifi_wpa_event event, void *param,
                                    int param_len, int vif_idx);

static int wifi_wpa_wait_event_unregister(int vif_idx);

/**
 ****************************************************************************************
 * @brief Get wpa configuration structure for an interface
 *
 * @param[in] vif_idx  Index of the WIFI interface
 * @return pointer to configuration structure for the interface or NULL for invalid index
 ****************************************************************************************
 */
__INLINE struct wifi_wpa_vif_tag *wifi_wpa_get_vif(int vif_idx)
{
    if ((vif_idx < 0) || (vif_idx >= CFG_VIF_NUM))
        return NULL;
    return &wifi_wpa.vifs[vif_idx];
}

/**
 ****************************************************************************************
 * @brief Get wpa configuration structure for an interface name
 *
 * @param[in] itf_name  Interface name
 * @return index of the interface or -1 for invalid name
 ****************************************************************************************
 */
int wifi_wpa_get_vif_idx(char *itf_name)
{
    for (int i = 0; i < CFG_VIF_NUM; i++)
    {
        if (strncmp(itf_name, wifi_wpa.vifs[i].iface_name, NET_AL_MAX_IFNAME) == 0)
            return i;
    }
    return -1;
}

/**
 ****************************************************************************************
 * @brief Reset interface parameters to its default values.
 *
 * @param[in] vif_idx  the vif index of WPA interface structure for the interface
 ****************************************************************************************
 */
void wifi_wpa_vif_reset(int vif_idx)
{
    struct wifi_wpa_vif_tag *wpa_vif = &wifi_wpa.vifs[vif_idx];

    wpa_vif->state = WIFI_WPA_STATE_STOPPED;
    wpa_vif->network_id = -1;
    wpa_vif->rx_filter = 0xFFFFFFF;
    #ifdef CFG_DPP
    wpa_vif->bootstrap_id = -1;
    wpa_vif->bootstrap_peer_id = -1;
    #endif
    wpa_vif->conn_sock = -1;
    wpa_vif->scan_sock = -1;
    wpa_vif->ftm_sock = -1;
}

int wifi_wpa_scan_sock_get(int vif_idx)
{
    if ((vif_idx < 0) || (vif_idx >= CFG_VIF_NUM))
        return -1;
    return wifi_wpa.vifs[vif_idx].scan_sock;
}

int wifi_wpa_scan_sock_set(int vif_idx, int scan_sock)
{
    if ((vif_idx < 0) || (vif_idx >= CFG_VIF_NUM))
        return -1;
    wifi_wpa.vifs[vif_idx].scan_sock = scan_sock;
    return 0;
}

int wifi_wpa_conn_sock_get(int vif_idx)
{
    if ((vif_idx < 0) || (vif_idx >= CFG_VIF_NUM))
        return -1;
    return wifi_wpa.vifs[vif_idx].conn_sock;
}

int wifi_wpa_conn_sock_set(int vif_idx, int conn_sock)
{
    if ((vif_idx < 0) || (vif_idx >= CFG_VIF_NUM))
        return -1;
    wifi_wpa.vifs[vif_idx].conn_sock = conn_sock;
    return 0;
}

int wifi_wpa_ftm_sock_get(int vif_idx)
{
    if ((vif_idx < 0) || (vif_idx >= CFG_VIF_NUM))
        return -1;
    return wifi_wpa.vifs[vif_idx].ftm_sock;
}

int wifi_wpa_ftm_sock_set(int vif_idx, int ftm_sock)
{
    if ((vif_idx < 0) || (vif_idx >= CFG_VIF_NUM))
        return -1;
    wifi_wpa.vifs[vif_idx].ftm_sock = ftm_sock;
    return 0;
}

/**
 ****************************************************************************************
 * @brief Reset all WPA environment to its default values.
 ****************************************************************************************
 */
static void wifi_wpa_reset()
{
    int i;

    wifi_wpa.task = NULL;
    wifi_wpa.ctrl_sock = -1;

    for (i = 0; i < CFG_VIF_NUM; i++)
    {
        wifi_wpa_vif_reset(i);
    }
}

static int wifi_wpa_open_loopback_udp_sock(int port)
{
    struct sockaddr_in cntrl;
    struct sockaddr_in wpa;
    int sock;

    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        dbg_print(ERR, "Failed to create UDP loopback socket\r\n");
        return sock;
    }

    cntrl.sin_family = AF_INET;
    cntrl.sin_addr.s_addr = htonl(INADDR_ANY);
    cntrl.sin_port =  htons(0);
    if (bind(sock, (struct sockaddr *)&cntrl, sizeof(cntrl)) < 0)
        goto err;

    wpa.sin_family = AF_INET;
    wpa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    wpa.sin_port = htons(port);
    if (connect(sock, (struct sockaddr *)&wpa, sizeof(wpa)) < 0)
        goto err;

    return sock;

  err:
    dbg_print(ERR, "Failed to connect UDP loopback socket (port %d)\r\n", port);
    close(sock);
    return -1;
}


/**
 ****************************************************************************************
 * @brief Process function for @ref WIFI_WPA_STARTED event
 *
 * This event is sent by WPA task after its initialization, and this function opens and
 * connects a UDP socket to send commands.
 *
 * @param[in] port  Control port id
 * @return 0 In case of success and !=0 if the control interface cannot be opened.
 * In this case the wpa_supplicant is killed.
 ****************************************************************************************
 */
static int wifi_wpa_started(int port)
{
    wifi_wpa.ctrl_sock = wifi_wpa_open_loopback_udp_sock(port);
    if (wifi_wpa.ctrl_sock < 0)
    {
        dbg_print(ERR, "Failed to connect to WPA ctrl interface (port=%d)\r\n", port);

        // Delete task (this will leak memory)
        sys_task_delete(wifi_wpa.task);

        // And process a fake EXIT event to clean everything
        wifi_wpa_event_process(WIFI_WPA_EXIT, (void *)-2, 0, WIFI_WPA_GLOBAL_VIF);
        return -1;
    }

    wifi_task_ready(SUPPLICANT_TASK);
    dbg_print(INFO, "WPA task started\r\n");
    return 0;
}

/**
 ****************************************************************************************
 * @brief Process function for @ref WIFI_WPA_EXIT event
 *
 * @param[in] exit_code  Exit code return by WPA task
 ****************************************************************************************
 */
static void wifi_wpa_exit(int exit_code)
{
    dbg_print(INFO, "WPA task exit (status = %d)\r\n", (int16_t)exit_code);

    if (wifi_wpa.ctrl_sock >= 0)
        close(wifi_wpa.ctrl_sock);

    macif_rx_set_mgmt_cb(NULL, NULL);
    wifi_wpa_reset();
}

/**
 ******************************************************************************
 * @brief Generic wpa event callback to notify a waiting task when expected
 * event is received
 *
 * Registered by @ref wifi_wpa_wait_event_register and unregistered by @ref
 * wifi_wpa_wait_event_unregister. The function unregister itself when the
 * waiting task is notified.
 *
 * @param[in] vif_idx  Index of the WIFI interface
 * @param[in] event          Event generated by the WPA task
 * @param[in] event_param    Event parameter (not used)
 * @param[in] arg            Registered private parameter, in this case info on
 *                           the expected event and the waiting task
 ******************************************************************************
 */
static void wifi_wpa_wait_event(int vif_idx, enum wifi_wpa_event event,
                                 void *event_param, void *arg)
{
    struct wifi_wpa_target_event *target = arg;
    int reason_or_status = (int)event_param;

    target->event = event;
    target->event_param = event_param;

    if (macif_vif_type_get(vif_idx) == VIF_STA) {
        wifi_netlink_msg_forward(vif_idx, (void *)target, 1);
    }

    wifi_wpa_wait_event_unregister(vif_idx);
}

/**
 ******************************************************************************
 * @brief Generic function to register WPA event callback that will notify
 * the calling task when a specific event occurs.
 *
 * Register @ref wifi_wpa_wait_event as callback. After calling this function
 * the user should call @ref rtos_task_wait_notification to wait for the
 * selected event. To avoid deadlock, the events @ref WIFI_WPA_INTERFACE_REMOVED
 * and @ref WIFI_WPA_EXIT are always included in the event set.
 * The caller must take care of race condition between wpa events and callback
 * registration.
 *
 * @param[in] vif_idx  Index of the WIFI interface.
 * @param[in] events         Expected Events, as bitfield.
 * @return 0 on success and != 0 otherwise
 ******************************************************************************
 */
static int wifi_wpa_wait_event_register(int vif_idx, int events)
{
    struct wifi_wpa_target_event *target;

    target = (struct wifi_wpa_target_event *)sys_malloc(sizeof(struct wifi_wpa_target_event));
    if (target == NULL)
        return -1;

    target->task = sys_current_task_handle_get();

    return wifi_wpa_cb_register(vif_idx,
                                 events | CO_BIT(WIFI_WPA_EXIT) |
                                 CO_BIT(WIFI_WPA_INTERFACE_REMOVED),
                                 wifi_wpa_wait_event, target);
}

/**
 ******************************************************************************
 * @brief Unregister WPA event callback
 *
 * Unregister the function registered by @ref wifi_wpa_wait_event_register.
 * There no need to call this function when a notification has been received.
 *
 * @param[in] vif_idx  Index of the WIFI interface
 * @return 0 on success and != 0 otherwise
 ******************************************************************************
 */
static int wifi_wpa_wait_event_unregister(int vif_idx)
{
    struct wifi_wpa_event_cb *cb;
    int i;

    if ((unsigned)vif_idx >= CFG_VIF_NUM)
        return -1;

    cb = wifi_wpa.vifs[vif_idx].cb;

    for (i = 0; i < WIFI_WPA_EVENT_CB_CNT; i++, cb++)
    {
        if (cb->func == wifi_wpa_wait_event)
        {
            cb->events = 0;
            cb->func = NULL;
            if (cb->arg)
                sys_mfree(cb->arg);
            cb->arg = NULL;
            return 0;
        }
    }

    return -1;
}

/**
 ****************************************************************************************
 * @brief Call registered callback when event is received from WPA task
 *
 * Loop over all registered callbacks and call them if associated to this event.
 *
 * @param[in] vif_idx  Index of the WIFI interface
 * @param[in] event          Event from WPA task
 * @param[in] param          Event parameter
 ****************************************************************************************
 */
static void wifi_wpa_call_event_cb(int vif_idx, enum wifi_wpa_event event,
                                    void *param)
{
    struct wifi_wpa_vif_tag *wpa_vif = wifi_wpa_get_vif(vif_idx);
    struct wifi_wpa_event_cb *cb = wpa_vif->cb;
    bool reset_cb = (event == WIFI_WPA_EXIT);
    int i;

    for (i = 0; i < WIFI_WPA_EVENT_CB_CNT; i++, cb++)
    {
        if (cb->events & CO_BIT(event))
            cb->func(vif_idx, event, param, cb->arg);

        if (reset_cb)
        {
            cb->events = 0;
            cb->func = NULL;
            cb->arg = NULL;
        }
    }
}

/**
 ****************************************************************************************
 * @brief Process events from WPA task
 *
 * @note This function is called in the context of the WPA task, and as such it cannot
 * block upon WPA task (e.g. it cannot send WPA command).
 *
 * @param[in] event          Event from WPA task
 * @param[in] param          Event parameter
 * @param[in] param_len      Length, in bytes, of the param buffer
 * @param[in] vif_idx  Index of the WIFI interface
 ****************************************************************************************
 */
static void wifi_wpa_event_process(enum wifi_wpa_event event, void *param,
                                    int param_len, int vif_idx)
{
    if (vif_idx == WIFI_WPA_GLOBAL_VIF)
    {
        int i;

        dbg_print(DEBUG, "Global event: %d\r\n", event);
        switch (event)
        {
            case WIFI_WPA_EXIT:
                wifi_wpa_exit((int)param);
                break;
            case WIFI_WPA_STARTED:
                if (wifi_wpa_started((int)param))
                    return;
                break;
            default:
                return;
        }

        // For global event callback registered on all interfaces
        for (i = 0; i < CFG_VIF_NUM; i++)
        {
            wifi_wpa_call_event_cb(i, event, param);
        }
    }
    else
    {
        struct wifi_wpa_vif_tag *wpa_vif = wifi_wpa_get_vif(vif_idx);
#ifdef CFG_WPS
        struct wifi_wpa_target_event target;
#endif

        if (!wpa_vif)
            return;

        dbg_print(DEBUG, "{FVIF-%d} event: %d\r\n", vif_idx, event);

        switch (event)
        {
            case WIFI_WPA_CONNECTED:
                dbg_print(DEBUG, "{FVIF-%d} enter WIFI_WPA_STATE_CONNECTED\r\n",
                            vif_idx);
                wpa_vif->state = WIFI_WPA_STATE_CONNECTED;
                break;
            case WIFI_WPA_DISCONNECTED:
                dbg_print(DEBUG, "{FVIF-%d} enter WIFI_WPA_STATE_NOT_CONNECTED\r\n",
                            vif_idx);
                wpa_vif->state = WIFI_WPA_STATE_NOT_CONNECTED;
                break;
            case WIFI_WPA_PROCESS_ERROR:
                dbg_print(DEBUG,
                            "{FVIF-%d} enter WIFI_WPA_STATE_NOT_CONNECTED after error %d\r\n",
                            vif_idx, (int)param);
                wpa_vif->state = WIFI_WPA_STATE_NOT_CONNECTED;
                break;
#ifdef CFG_WPS
            case WIFI_WPA_WPS_CRED:
            {
                struct wps_credential *input_cred = (struct wps_credential *)param;
                struct wps_cred_t *cred;

                cred = sys_zalloc(sizeof(struct wps_cred_t));
                if (cred == NULL)
                    return;

                target.event = event;
                target.event_param = cred;

                if (input_cred->ssid_len) {
                    sys_memcpy(cred->ssid, input_cred->ssid, input_cred->ssid_len);
                    cred->ssid_len = input_cred->ssid_len;
                }
                if (input_cred->key_len) {
                    sys_memcpy(cred->passphrase, input_cred->key, input_cred->key_len);
                    cred->passphrase_len = input_cred->key_len;
                }
                cred->channel = 0xFF;
                wifi_netlink_msg_forward(vif_idx, (void *)&target, 1);
                return;

            }
            case WIFI_WPA_WPS_SUCCESS:
            case WIFI_WPA_WPS_ERROR:
                target.event = event;
                target.event_param = NULL;
                wifi_netlink_msg_forward(vif_idx, (void *)&target, 1);
                break;
#endif
            default:
                break;
        }

        wifi_wpa_call_event_cb(vif_idx, event, param);
    }
}

/**
 ****************************************************************************************
 * @brief Callback function for non processed MGMT frames
 *
 * Management frames not processed by the wifi task are forwarded to wpa_supplicant
 *
 * @param[in] info  Frame information.
 * @param[in] arg   Callback parameter (unused)
 ****************************************************************************************
 */
static void wifi_wpa_rx_cb(struct wifi_frame_info *info, void *arg)
{
    struct mac_hdr *hdr = (struct mac_hdr *)info->payload;
    struct wifi_wpa_vif_tag *wpa_vif;
    struct macif_rx_mgmt_event event;

    if ((unsigned)info->vif_idx >= CFG_VIF_NUM)
    {
        dbg_print(INFO, "Ignore Management frame received on invalid VIF\r\n");
        return;
    }

    wpa_vif = &wifi_wpa.vifs[info->vif_idx];

    if ((info->payload == NULL) ||
        (wpa_vif->state == WIFI_WPA_STATE_STOPPED) ||
        ((hdr->fctl & MAC_FCTRL_TYPE_MASK) != MAC_FCTRL_MGT_T) ||
        (wpa_vif->rx_filter & CO_BIT(MAC_FCTRL_SUBTYPE(hdr->fctl))))
        return;

    if (wpa_vif->conn_sock < 0)
        return;

    event.hdr.id = MACIF_RX_MGMT_EVENT;
    event.hdr.len = sizeof(event);
    event.vif_idx = info->vif_idx;
    event.freq = info->freq;
    event.rssi = info->rssi;
    event.length = info->length;
    event.payload = sys_malloc(event.length);
    if (event.payload == NULL)
        return;

    memcpy(event.payload, info->payload, event.length);
    if (macif_cntrl_event_send(&event.hdr, wpa_vif->conn_sock))
        sys_mfree(event.payload);
}


void wifi_wpa_mbo_update_chan_req(struct wifi_mbo_update_chan_req *info)
{
    struct wifi_wpa_vif_tag *wpa_vif;
    struct macif_mbo_update_non_pre_chan_event event;

    if ((unsigned)info->vif_idx >= CFG_VIF_NUM)
    {
        dbg_print(INFO, "vif_idx >= CFG_VIF_NUM\r\n");
        return;
    }

    wpa_vif = &wifi_wpa.vifs[info->vif_idx];

    if (wpa_vif->conn_sock < 0)
        return;

    event.hdr.id = MACIF_MBO_UPDATE_CHAN_REQ;
    event.hdr.len = sizeof(event);
    event.vif_idx = info->vif_idx;
    if (info->non_pref_chan != NULL) {
        sys_memcpy((uint8_t *)event.non_pref_chan, (uint8_t *)info->non_pref_chan, 64);
    } else {
        sys_memset(event.non_pref_chan, 0, 64);
    }

    macif_cntrl_event_send(&event.hdr, wpa_vif->conn_sock);

    return;
}

/**
 ****************************************************************************************
 * @brief Send command to wpa_supplicant task and wait for the response
 *
 * The function first send the command to wpa_supplicant over the control interface.
 * It then waits up to @p timeout_ms ms for wpa_supplicant to send a response.
 * An error is returned if this is not the case.
 *
 * Then the response is read in the provided buffer. If no buffer is provided (or if the
 * buffer is too small) a temporary buffer is used to retrieve at up 4 characters.
 * In any cases the response is also copied in the @p resp buffer (as much as possible)
 * and the size written in updated in @p resp_len.
 *
 * If no error has been detected, the final return status is the command execution status
 * returned by wpa_supplicant task.
 *
 * @param[in]     wpa_vif     WPA structure for the interface, NULL for global command.
 * @param[in]     cmd_str     Command string (must be NULL terminated)
 * @param[in]     resp_buf    Buffer to retrieve the response.
 * @param[in,out] resp_len    Size, in bytes, of the response buffer.
 *                            If no error is reported, it is updated with the size
 *                            actually written in the response buffer.
 * @param[in]     timeout_ms  Timeout, in ms, allowed to the wpa_supplicant task to
 *                            respond (<0 means wait forever).

 * @return 0 if command has been successfully sent and executed by WPA task and != 0
 * otherwise
 ****************************************************************************************
 */
static int wifi_wpa_send_cmd(struct wifi_wpa_vif_tag *wpa_vif, char *cmd_str,
                              char *resp_buf, int *resp_len, int timeout_ms)
{
    struct wifi_wpa_cmd cmd;
    struct wifi_wpa_resp resp;
    char tmp_resp_buf[4];
    struct iovec iovec[4];
    struct msghdr msghdr;
    int res, recv_flags = 0;

    // Send Command
    memset(&cmd, 0, sizeof(cmd));
    memset(&msghdr, 0, sizeof(msghdr));
    msghdr.msg_iov = iovec;

    cmd.cmd = cmd_str;
    if (!resp_buf || !resp_len || (*resp_len < 4))
    {
        cmd.resp = tmp_resp_buf;
        cmd.resp_len = sizeof(tmp_resp_buf);
    }
    else
    {
        cmd.resp = resp_buf;
        cmd.resp_len = *resp_len;
    }

    if (wpa_vif)
    {
        memcpy(cmd.ifname, wpa_vif->iface_name, sizeof(cmd.ifname));
    }

    iovec[0].iov_base = &cmd;
    iovec[0].iov_len = sizeof(cmd);
    msghdr.msg_iovlen = 1;

    if (sendmsg(wifi_wpa.ctrl_sock, &msghdr, 0) < 0)
        return -1;

    // wait response
    if (timeout_ms >= 0)
    {
        struct timeval timeout;
        fd_set fds;

        FD_ZERO(&fds);
        FD_SET(wifi_wpa.ctrl_sock, &fds);
        timeout.tv_sec = (timeout_ms / 1000000);
        timeout.tv_usec = (timeout_ms - timeout.tv_sec * 100000) * 1000;

        if (select(wifi_wpa.ctrl_sock + 1, &fds, NULL, NULL, &timeout) <= 0)
            return -2;

        recv_flags = MSG_DONTWAIT;
    }

    res = recv(wifi_wpa.ctrl_sock, &resp, sizeof(resp), recv_flags);
    if (res < 0)
        return -3;

    if (resp.resp)
    {
        dbg_print(DEBUG, "RESP: %s len %d\r\n", resp.resp, resp.len);
    }
    else
    {
        dbg_print(DEBUG, "RESP: status=%d (no buffer)\r\n", resp.status);
    }

    if (resp_buf && resp_len)
    {
        if (resp.resp == tmp_resp_buf)
        {
            if (resp.len < *resp_len)
                *resp_len = resp.len;
            memcpy(resp_buf, tmp_resp_buf, *resp_len);
        }
        else
            *resp_len = resp.len;
    }

    return (resp.status == WIFI_WPA_CMD_FAILED);
}

/*
 ****************************************************************************************
 * Public functions
 ****************************************************************************************
 */
int wifi_wpa_init(void)
{
    memset(&wifi_wpa, 0, sizeof(wifi_wpa));
    wifi_wpa_reset();

    sys_mutex_init(&wifi_wpa.ctrl_mutex);
    if (wifi_wpa.ctrl_mutex == NULL)
    {
        dbg_print(ERR, "Failed to create WPA mutex\r\n");
        return -1;
    }

    wifi_wpa.task = sys_task_create_dynamic((const uint8_t *)"WPA",
                            WIFI_WPA_TASK_STACK_SIZE, WIFI_WPA_TASK_PRIORITY,
                            wpa_supplicant_main, NULL);
    if (wifi_wpa.task == NULL)
    {
        dbg_print(ERR, "Failed to create WPA task\r\n");
        return -1;
    }

    macif_rx_set_mgmt_cb(wifi_wpa_rx_cb, NULL);
    return 0;
}

void wifi_wpa_deinit(void)
{
    macif_rx_set_mgmt_cb(NULL, NULL);

    wifi_wpa_execute_cmd(WIFI_WPA_GLOBAL_VIF, NULL, 0, -1, "TERMINATE");

    wifi_wait_terminated(SUPPLICANT_TASK);

    if (wifi_wpa.ctrl_mutex) {
        sys_mutex_free(&wifi_wpa.ctrl_mutex);
        wifi_wpa.ctrl_mutex = NULL;
    }
}

int wifi_wpa_add_vif(int vif_idx)
{
    struct wifi_wpa_vif_tag *wpa_vif = wifi_wpa_get_vif(vif_idx);

    if ((wifi_wpa.ctrl_sock < 0) || !wpa_vif ||
        (wpa_vif->state != WIFI_WPA_STATE_STOPPED) ||
        (wifi_vif_name(vif_idx, wpa_vif->iface_name,
                        sizeof(wpa_vif->iface_name)) < 0)) {
        dbg_print(ERR, "wpa_vif->state = %d\r\n", wpa_vif->state);
        return -1;
    }
#if 0
    if (wifi_wpa_execute_cmd(WIFI_WPA_GLOBAL_VIF, NULL, 0, 300, "INTERFACE_ADD %s",
                              wpa_vif->iface_name))
#endif
    if (wifi_wpa_execute_cmd(WIFI_WPA_GLOBAL_VIF, NULL, 0, -1, "INTERFACE_ADD %s",
                              wpa_vif->iface_name))
    {
        dbg_print(ERR, "{FVIF-%d} Failed to add WPA interface\r\n", vif_idx);
        return -1;
    }

    wpa_vif->state = WIFI_WPA_STATE_NOT_CONNECTED;

    dbg_print(INFO, "{FVIF-%d} WPA interface added\r\n", vif_idx);
    return 0;
}

int wifi_wpa_remove_vif(int vif_idx)
{
    struct wifi_wpa_vif_tag *wpa_vif = wifi_wpa_get_vif(vif_idx);

    if ((wifi_wpa.ctrl_sock < 0) || !wpa_vif)
        return -1;

    if (wpa_vif->state == WIFI_WPA_STATE_STOPPED)
        return 0;

#if 0
    if (wifi_wpa_execute_cmd(WIFI_WPA_GLOBAL_VIF, NULL, 0, 300, "INTERFACE_REMOVE %s",
                              wpa_vif->iface_name))
#endif
    if (wifi_wpa_execute_cmd(WIFI_WPA_GLOBAL_VIF, NULL, 0, -1, "INTERFACE_REMOVE %s",
                              wpa_vif->iface_name))
    {
        dbg_print(ERR, "{FVIF-%d} Failed to remove WPA interface\r\n", vif_idx);
        return -1;
    }

    wifi_wpa_vif_reset(vif_idx);
    dbg_print(INFO, "{FVIF-%d} WPA interface removed\r\n", vif_idx);
    return 0;
}

enum wifi_wpa_state wifi_wpa_get_state(int vif_idx)
{
    struct wifi_wpa_vif_tag *wpa_vif = wifi_wpa_get_vif(vif_idx);

    if (!wpa_vif)
        return WIFI_WPA_STATE_STOPPED;

    return wpa_vif->state;
}

int wifi_wpa_cb_register(int vif_idx, int events, wifi_wpa_cb_t cb_func,
                          void *cb_arg)
{
    struct wifi_wpa_event_cb *cb;
    int i;

    if ((unsigned)vif_idx >= CFG_VIF_NUM)
        return -1;

    cb = wifi_wpa.vifs[vif_idx].cb;

    for (i = 0; i < WIFI_WPA_EVENT_CB_CNT; i++, cb++)
    {
        if (cb->events == 0)
        {
            cb->events = events;
            cb->func = cb_func;
            cb->arg = cb_arg;
            return 0;
        }
    }

    return -1;
}

int wifi_wpa_cb_unregister(int vif_idx, wifi_wpa_cb_t cb_func)
{
    struct wifi_wpa_event_cb *cb;
    int i;

    if ((unsigned)vif_idx >= CFG_VIF_NUM)
        return -1;

    cb = wifi_wpa.vifs[vif_idx].cb;

    for (i = 0; i < WIFI_WPA_EVENT_CB_CNT; i++, cb++)
    {
        if (cb->func == cb_func)
        {
            cb->events = 0;
            cb->func = NULL;
            cb->arg = NULL;
            return 0;
        }
    }

    return -1;
}

int wifi_wpa_send_event(enum wifi_wpa_event event, void *param, int param_len,
                         int vif_idx)
{
    if (((event <= WIFI_WPA_STARTED) && (vif_idx != WIFI_WPA_GLOBAL_VIF)) ||
        ((event > WIFI_WPA_STARTED) && (vif_idx > CFG_VIF_NUM)) ||
        (event >= WIFI_WPA_LAST))
        return -1;

    wifi_wpa_event_process(event, param, param_len, vif_idx);
    return 0;
}

int wifi_wpa_send_event_with_name(enum wifi_wpa_event event, void *param, int param_len,
                                   char *itf_name)
{
    return wifi_wpa_send_event(event, param, param_len, wifi_wpa_get_vif_idx(itf_name));
}

int wifi_wpa_execute_cmd(int vif_idx, char *resp_buf, int *resp_buf_len,
                          int timeout_ms, const char *fmt, ...)
{
    struct wifi_wpa_vif_tag *wpa_vif = wifi_wpa_get_vif(vif_idx);
    va_list args;
    unsigned int cmd_len;
    int res = -1;

    if ((vif_idx < WIFI_WPA_GLOBAL_VIF) &&
        (!wpa_vif || (wpa_vif->state == WIFI_WPA_STATE_STOPPED)))
        return -1;

    if (NULL == wifi_wpa.ctrl_mutex)
        return -2;

    sys_mutex_get(&wifi_wpa.ctrl_mutex);

    // Format command
    va_start(args, fmt);
    cmd_len = dbg_vsnprintf(wpa_cmd, sizeof(wpa_cmd), fmt, args);
    va_end(args);

    if (cmd_len >= sizeof(wpa_cmd))
    {
        dbg_print(ERR, "WPA Command truncated. need %d bytes\r\n", cmd_len);
        goto end;
    }

    dbg_print(DEBUG, "CMD: %s\r\n", wpa_cmd);

    // Send it and wait for response
    res = wifi_wpa_send_cmd(wpa_vif, wpa_cmd, resp_buf, resp_buf_len, timeout_ms);

  end:
    sys_mutex_put(&wifi_wpa.ctrl_mutex);
    return res;
}

int wifi_wpa_create_network(int vif_idx, char *net_cfg, bool enable)
{
    struct wifi_wpa_vif_tag *wpa_vif = wifi_wpa_get_vif(vif_idx);
    char res[5], *tok;
    int res_len;

    if (!net_cfg || !wpa_vif || wifi_wpa_add_vif(vif_idx))
        return -1;

    // Create and configure network block
    res_len = sizeof(res) - 1;
#if 0
    if (wifi_wpa_execute_cmd(vif_idx, res, &res_len, 100, "ADD_NETWORK"))
#endif
    if (wifi_wpa_execute_cmd(vif_idx, res, &res_len, -1, "ADD_NETWORK"))
        return -1;

    res[res_len] = '\0';
    wpa_vif->network_id = atoi(res);

    tok = strtok(net_cfg, ";");
    while (tok)
    {
        res_len = sizeof(res);
        if (wifi_wpa_execute_cmd(vif_idx, res, &res_len, 10000, "SET_NETWORK %d %s",
                                  wpa_vif->network_id, tok))
        {
            dbg_print(ERR, "SET_NETWORK (%s) failed\r\n", tok);
            wifi_wpa_remove_vif(vif_idx);
            return -1;
        }
        tok = strtok(NULL, ";");
    }

    dbg_print(INFO, "WPA network %d: created and configured\r\n", wpa_vif->network_id);

    // Connect to AP if requested
    if (enable && wifi_wpa_enable_network(vif_idx))
    {
        wifi_wpa_remove_vif(vif_idx);
        return -1;
    }

    return 0;
}

int wifi_wpa_check_network(int vif_idx, struct wifi_sta *sta)
{
    struct wifi_wpa_vif_tag *wpa_vif = wifi_wpa_get_vif(vif_idx);
    struct mac_ssid ssid;
    char sta_cfg_bssid[18], bssid[18];
    char key[63 + 1] = {0};
    char cur_key_mgmt[64] = {0};
    int res_len = 0;
    uint32_t key_mgmt = 0;

    //check if there a network added to the wpa_if
    if (!wpa_vif || (wpa_vif->network_id < 0))
        return -1;

    // settings of remote AP maybe changed, eg: security mode
    if (sta->last_reason == WIFI_MGMT_DISCON_RECV_DEAUTH)
        return -2;

    /* Check if bssid changed */
    res_len = sizeof(bssid);
    if (wifi_wpa_execute_cmd(vif_idx, bssid, &res_len, -1, "GET_NETWORK %d bssid", wpa_vif->network_id))
        return -3;

    dbg_snprintf(sta_cfg_bssid, 18, MAC_FMT, MAC_ARG_UINT8(sta->cfg.bssid));
    if (memcmp(bssid, sta_cfg_bssid, res_len) != 0)
        return -4;

    /* Check if ssid changed */
    memset(&ssid, 0, sizeof(ssid));
    res_len = sizeof(ssid);
    if (wifi_wpa_execute_cmd(vif_idx, (char *)ssid.array, &res_len, -1,
                              "GET_NETWORK %d ssid", wpa_vif->network_id))
        return -5;

    if ((res_len != sta->cfg.ssid_len) ||
        memcmp((char *)ssid.array, sta->cfg.ssid, res_len) != 0)
         return -6;

    /* Check if psk changed */
    res_len = sizeof(key);
    wifi_wpa_execute_cmd(vif_idx, key, &res_len, -1, "GET_NETWORK %d psk", wpa_vif->network_id);

    if (sta->cfg.passphrase_len != 0) {
        if ((res_len != sta->cfg.passphrase_len) ||
            memcmp(key, sta->cfg.passphrase, res_len) != 0)
            return -7;
    } else {
        if (res_len != 0) {
            return -8;
        }
    }

    /* Check if akm(key mgmt) changed */
    res_len = sizeof(cur_key_mgmt) -1;
    wifi_wpa_execute_cmd(vif_idx, cur_key_mgmt, &res_len, -1, "GET_NETWORK %d key_mgmt", wpa_vif->network_id);

    cur_key_mgmt[res_len] = '\0';
    key_mgmt = wifi_wpa_parse_key_mgmt(cur_key_mgmt);
    if (co_clz(key_mgmt) != co_clz(sta->cfg.akm)) {
        dbg_print(NOTICE, "Key mgmt changed!\r\n");
        return -9;
    }

    return 0;
}

int wifi_wpa_enable_network(int vif_idx)
{
    struct wifi_wpa_vif_tag *wpa_vif = wifi_wpa_get_vif(vif_idx);

    if (!wpa_vif || (wpa_vif->network_id < 0))
        return -1;

    if (wpa_vif->state == WIFI_WPA_STATE_CONNECTED)
        return 0;

    wpa_vif->state = WIFI_WPA_STATE_PROCESSING;
    dbg_print(DEBUG, "{FVIF-%d} enter WIFI_WPA_STATE_PROCESSING\r\n", vif_idx);

    if (wifi_wpa_wait_event_register(vif_idx,
                                      CO_BIT(WIFI_WPA_CONNECTED) |
                                      CO_BIT(WIFI_WPA_PROCESS_ERROR)))
        return -1;

#if 0
    if (wifi_wpa_execute_cmd(vif_idx, NULL, NULL, 100, "ENABLE_NETWORK %d ",
                              wpa_vif->network_id))
#endif
    if (wifi_wpa_execute_cmd(vif_idx, NULL, NULL, -1, "ENABLE_NETWORK %d ",
                              wpa_vif->network_id))
    {
        wifi_wpa_wait_event_unregister(vif_idx);
        return -1;
    }

    dbg_print(INFO, "WPA network %d: enabled\r\n", wpa_vif->network_id);
    return 0;
}

int wifi_wpa_disable_network(int vif_idx)
{
    struct wifi_wpa_vif_tag *wpa_vif = wifi_wpa_get_vif(vif_idx);

    if (!wpa_vif)
        return -1;

    if (wpa_vif->network_id == -1)
        return 0;

    if (wpa_vif->state != WIFI_WPA_STATE_CONNECTED)
        return 0;

    if (wifi_wpa_wait_event_register(vif_idx, CO_BIT(WIFI_WPA_DISCONNECTED)))
        return -2;

#if 0
    if (wifi_wpa_execute_cmd(vif_idx, NULL, NULL, 100, "DISABLE_NETWORK %d ",
                              wpa_vif->network_id))
#endif
    if (wifi_wpa_execute_cmd(vif_idx, NULL, NULL, -1, "DISABLE_NETWORK %d ",
                              wpa_vif->network_id))
    {
        wifi_wpa_wait_event_unregister(vif_idx);
        return -3;
    }

    dbg_print(INFO, "WPA network %d: disconnected\r\n", wpa_vif->network_id);
    return 0;
}

#ifdef CFG_80211R
int wifi_wpa_roaming_start(int vif_idx, char *bssid)
{
    struct wifi_wpa_vif_tag *wpa_vif = wifi_wpa_get_vif(vif_idx);

    if (!wpa_vif)
        return -1;

    if (wifi_wpa_execute_cmd(vif_idx, NULL, NULL, -1, "ROAM %s",
                              bssid))
    {
        return -1;
    }

    return 0;
}
#endif /* CFG_80211R */

int wifi_wpa_roaming_stop(int vif_idx)
{
    struct wifi_wpa_vif_tag *wpa_vif = wifi_wpa_get_vif(vif_idx);

    if (!wpa_vif || (wpa_vif->network_id < 0))
        return -1;

#if 0
    if (wifi_wpa_execute_cmd(vif_idx, NULL, NULL, 100, "DISABLE_NETWORK %d ",
                              wpa_vif->network_id))
#endif
    if (wifi_wpa_execute_cmd(vif_idx, NULL, NULL, -1, "DISABLE_NETWORK %d ",
                              wpa_vif->network_id))
    {
        return -1;
    }

    return 0;
}

int wifi_wpa_link_monitor(int vif_idx, int start)
{
    struct wifi_wpa_vif_tag *wpa_vif = wifi_wpa_get_vif(vif_idx);

    if (!wpa_vif || (wpa_vif->network_id < 0))
        return -1;

    if (wpa_vif->state != WIFI_WPA_STATE_CONNECTED)
        return 0;

    if (start)
    {
        if (wifi_wpa_wait_event_register(vif_idx, CO_BIT(WIFI_WPA_DISCONNECTED)))
            return -1;
    }
    else
    {
        if (wifi_wpa_wait_event_unregister(vif_idx))
            return -1;
    }

    return 0;
}

int wifi_wpa_set_mgmt_rx_filter(int vif_idx, uint32_t filter)
{
    struct wifi_wpa_vif_tag *wpa_vif = wifi_wpa_get_vif(vif_idx);
    if (!wpa_vif)
        return -1;

    wpa_vif->rx_filter = filter;

    macif_vif_wpa_rx_filter_set(vif_idx, filter);
    return 0;
}

int wifi_wpa_get_mgmt_rx_filter(int vif_idx)
{
    struct wifi_wpa_vif_tag *wpa_vif = wifi_wpa_get_vif(vif_idx);
    if (!wpa_vif)
        return 0;

    return wpa_vif->rx_filter;
}

#ifdef CFG_WPS
int wifi_wpa_wps_start(int vif_idx)
{
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(vif_idx);
    struct wps_config_t *cfg = &wvif->sta.cfg.wps_cfg;

    wifi_wpa_remove_vif(vif_idx);

    if (wifi_wpa_add_vif(vif_idx)) {
        dbg_print(WARNING, "WPS: add vif failed\r\n");
        return -1;
    }

    if (wifi_wpa_wait_event_register(vif_idx,
                                      CO_BIT(WIFI_WPA_DISCONNECTED) |
                                      CO_BIT(WIFI_WPA_PROCESS_ERROR)))
        return -2;

    if (cfg->pbc) {
        if (wifi_wpa_execute_cmd(vif_idx, NULL, NULL, -1, "WPS_PBC"))
        {
            dbg_print(WARNING, "WPS PBC: command execute failed\r\n");
            wifi_wpa_wait_event_unregister(vif_idx);
            return -3;
        }
        dbg_print(NOTICE, "WPS PBC started\r\n");
    } else {
        if (wifi_wpa_execute_cmd(vif_idx, NULL, NULL, -1, "WPS_PIN any %s", cfg->pin))
        {
            dbg_print(WARNING, "WPS PIN: command execute failed\r\n");
            wifi_wpa_wait_event_unregister(vif_idx);
            return -3;
        }
        dbg_print(NOTICE, "WPS PIN started\r\n");
    }

    return 0;
}

int wifi_wpa_wps_stop(int vif_idx)
{
    if (wifi_wpa_execute_cmd(vif_idx, NULL, NULL, -1, "WPS_CANCEL"))
    {
        dbg_print(WARNING, "WPS cancel failed\r\n");
        return -1;
    }

    // dbg_print(NOTICE, "WPS canceled\r\n");
    return 0;
}

int wifi_wpa_wps_associate(int vif_idx, uint8_t *frame, uint32_t frame_len)
{
    return 0;
}
int wifi_wpa_wps_associate_done(int vif_idx, void *ind_param)
{
    return 0;
}
int wifi_wpa_wps_ssid_bss_match(int vif_idx, uint8_t *frame, uint32_t frame_len)
{
    return 0;
}
void wifi_wpa_wps_scan_timer(void *eloop_data, void *user_ctx)
{
}

#endif /* CFG_WPS */

#ifdef CFG_SAE_PK
#include "common/sae_pk_gen.h"
int wifi_wpa_ap_sae_pk_password(struct ap_cfg *cfg, char *cfg_str,
                              int cfg_str_len)
{
    struct ap_sae_pk *pk = &cfg->sae_pk;

    if (!pk->private_key)
    {
        pk->private_key = sae_pk_key_gen(19, &pk->private_key_len);
        if (!pk->private_key)
            return -1;

        // a new key has been created so a new password is mandatory
        cfg->passphrase[0] = 0;
    }

    // create a new password if none provided
    if ((cfg->passphrase_len == 0) &&
        sae_pk_password_gen(pk->private_key, pk->private_key_len,
                            pk->modifier, sizeof(pk->modifier),
                            (uint8_t *)cfg->ssid, cfg->ssid_len,
                            pk->sec, pk->nb_part,
                            cfg->passphrase, cfg->passphrase_len))
        return -1;

    return sae_pk_password_write(cfg->passphrase, pk->private_key, pk->private_key_len,
                                 pk->modifier, cfg_str, cfg_str_len);
}
#endif // CFG_SAE_PK

int wifi_wpa_sta_cfg(int vif_idx, struct sta_cfg *cfg)
{
    char *cfg_str, *ptr;
    int res, cfg_str_len = 384;
    int key_len;

    if ((vif_idx >= CFG_VIF_NUM) || (cfg == NULL))
        return -1;
    if (macif_vif_type_get(vif_idx) != VIF_STA)
        return -2;

    cfg_str = sys_malloc(cfg_str_len + 1);
    if (!cfg_str)
        return -1;
    ptr = cfg_str;

    // SSID
    res = cfg->ssid_len + 8; // 8 = 'ssid "";'
    if (cfg_str_len < res)
        goto end;

    memcpy(ptr, "ssid \"", 6);
    ptr += 6;
    memcpy(ptr, cfg->ssid, cfg->ssid_len);
    ptr += cfg->ssid_len;
    *ptr++ = '"';
    *ptr++ = ';';
    cfg_str_len -= res;

    // AKM
    key_len = cfg->passphrase_len;
    if (!cfg->akm)
    {
        if (key_len < 8)
        {
            // If key is less than 8, assume WEP key
            cfg->akm = CO_BIT(MAC_AKM_NONE);
        }
        else
        {
            cfg->akm = CO_BIT(MAC_AKM_PSK) | CO_BIT(MAC_AKM_PSK_SHA256);
            cfg->akm |= CO_BIT(MAC_AKM_SAE);
        }
    }
    else
    {
        // remove unsupported AKM
        uint32_t akm_supported = CO_BIT(MAC_AKM_NONE) | CO_BIT(MAC_AKM_PSK) | CO_BIT(MAC_AKM_PSK_SHA256);
        akm_supported |= CO_BIT(MAC_AKM_SAE) | CO_BIT(MAC_AKM_OWE);
#ifdef CFG_8021x_EAP_TLS
        akm_supported |= CO_BIT(MAC_AKM_8021X) | CO_BIT(MAC_AKM_8021X_SHA256);
        akm_supported |= CO_BIT(MAC_AKM_8021X_SUITE_B_192) | CO_BIT(MAC_AKM_8021X_SUITE_B);
#endif
#ifdef CFG_80211R
        akm_supported |= CO_BIT(MAC_AKM_FT_PSK);
#endif
        // For WEP the user select only MAC_AKM_PRE_RSN
        if (cfg->akm == CO_BIT(MAC_AKM_PRE_RSN))
            cfg->akm = CO_BIT(MAC_AKM_NONE);
        else if (!(cfg->akm & CO_BIT(MAC_AKM_PRE_RSN)))
        {
            // User doesn't allow WPA1 AP
            res = dbg_snprintf(ptr, cfg_str_len, "proto RSN;");
            if (res >= cfg_str_len)
                goto end;
            ptr += res;
            cfg_str_len -= res;
        }

        cfg->akm &= akm_supported;
        if (cfg->akm == 0)
        {
            res = -1;
            goto end;
        }
    }
    res = 8;
    if (cfg_str_len < res)
        goto end;
    memcpy(ptr, "key_mgmt", res);
    ptr += res;
    cfg_str_len -= res;
    res = wifi_wpa_akm_name(cfg->akm, ptr, cfg_str_len);
    if (res < 0)
        goto end;
    ptr += res;
    cfg_str_len -= res;

    // Cipher suites for WPA
    if (cfg->akm & (CO_BIT(MAC_AKM_PSK) | CO_BIT(MAC_AKM_SAE)
        | CO_BIT(MAC_AKM_OWE)
#ifdef CFG_8021x_EAP_TLS
        | CO_BIT(MAC_AKM_8021X)
        | CO_BIT(MAC_AKM_8021X_SHA256)
        | CO_BIT(MAC_AKM_8021X_SUITE_B_192)
        | CO_BIT(MAC_AKM_8021X_SUITE_B)
#endif
        )) {
        uint32_t cipher_supported = 0;
        uint32_t cipher_pairwise, cipher_group;

        cipher_supported = macif_setting_supp_cipher_get();

        if (cfg->p_cipher)
            cipher_pairwise = cfg->p_cipher & cipher_supported;
        else
            cipher_pairwise = cipher_supported;

        if (cfg->g_cipher)
            cipher_group = cfg->g_cipher & cipher_supported;
        else
            cipher_group = cipher_supported;

        if (!cipher_pairwise || !cipher_group)
        {
            res = -1;
            goto end;
        }

        // By default wpa_supplicant enable TKIP and CCMP. If we support something else
        // need to configure wpa_supplicant accordingly
        if (cipher_pairwise != (CO_BIT(MAC_CIPHER_TKIP) | CO_BIT(MAC_CIPHER_CCMP)))
        {
            res = 8;
            if (cfg_str_len < res)
                goto end;
            memcpy(ptr, "pairwise", res);
            ptr += res;
            cfg_str_len -= res;
            res = wifi_wpa_cipher_name(cipher_pairwise, ptr, cfg_str_len);
            if (res < 0)
                goto end;
            ptr += res;
            cfg_str_len -= res;
        }

        if (cipher_group != (CO_BIT(MAC_CIPHER_TKIP) | CO_BIT(MAC_CIPHER_CCMP)))
        {
            res = 5;
            if (cfg_str_len < res)
                goto end;
            memcpy(ptr, "group", res);
            ptr += res;
            cfg_str_len -= res;
            res = wifi_wpa_cipher_name(cipher_group, ptr, cfg_str_len);
            if (res < 0)
                goto end;
            ptr += res;
            cfg_str_len -= res;
        }
    }

    // Keys
    if (key_len > 0
#ifdef CFG_8021x_EAP_TLS
        || (cfg->eap_cfg.conn_with_enterprise == 1)
#endif
        || (cfg->akm & CO_BIT(MAC_AKM_OWE))
    )
    {
        if ((cfg->akm & CO_BIT(MAC_AKM_NONE)) &&
            ((key_len == 5) || (key_len == 13) || (key_len == 16)))
        {
            // WEP keys
            res = dbg_snprintf(ptr, cfg_str_len, "wep_key0 \"%s\";auth_alg OPEN SHARED;",
                               cfg->passphrase);
        }
        else if (cfg->akm & (CO_BIT(MAC_AKM_PSK) | CO_BIT(MAC_AKM_SAE)))
        {
            // PSK (works also for SAE)
            res = dbg_snprintf(ptr, cfg_str_len, "psk \"%s\";", cfg->passphrase);
        } else {
            res = 0;
        }
        if (res >= cfg_str_len)
            goto end;

        ptr += res;
        cfg_str_len -= res;

        #ifdef CFG_MFP
        // Always try to use MFP
        if (cfg->mfpr)
            res = dbg_snprintf(ptr, cfg_str_len, "ieee80211w 2;");
        else
            res = dbg_snprintf(ptr, cfg_str_len, "ieee80211w 1;");
        if (res >= cfg_str_len)
            goto end;

        ptr += res;
        cfg_str_len -= res;
        #endif

        #ifdef CFG_SAE_PK
        res = dbg_snprintf(ptr, cfg_str_len, "sae_pk 1;");
        if (res >= cfg_str_len)
            goto end;

        ptr += res;
        cfg_str_len -= res;
        #endif

    }

    // BSSID (optional)
    if (cfg->bssid[0] || cfg->bssid[1] || cfg->bssid[2])
    {
        res = dbg_snprintf(ptr, cfg_str_len, "bssid %02x:%02x:%02x:%02x:%02x:%02x;",
                           cfg->bssid[0], cfg->bssid[1], cfg->bssid[2],
                           cfg->bssid[3], cfg->bssid[4], cfg->bssid[5]);
        if (res >= cfg_str_len)
            goto end;

        ptr += res;
        cfg_str_len -= res;
    }

    res = dbg_snprintf(ptr, cfg_str_len, "scan_ssid 1;");// to connect to hidden AP
    if (res >= cfg_str_len)
        goto end;
    ptr += res;
    cfg_str_len -= res;

#ifdef CFG_8021x_EAP_TLS
    if (cfg->eap_cfg.conn_with_enterprise) {
        res = dbg_snprintf(ptr, cfg_str_len, "eap TLS;");
        if (res >= cfg_str_len)
            goto end;
        ptr += res;
        cfg_str_len -= res;

        res = dbg_snprintf(ptr, cfg_str_len, "phase1 \"tls_disable_time_checks=1\";");
        if (res >= cfg_str_len)
            goto end;
        ptr += res;
        cfg_str_len -= res;

        res = dbg_snprintf(ptr, cfg_str_len, "eapol_flags 0;");
        if (res >= cfg_str_len)
            goto end;
        ptr += res;
        cfg_str_len -= res;

        res = dbg_snprintf(ptr, cfg_str_len, "identity \"%s\";", cfg->eap_cfg.identity);
        if (res >= cfg_str_len)
            goto end;
        ptr += res;
        cfg_str_len -= res;

        res = dbg_snprintf(ptr, cfg_str_len, "private_key_passwd \"%s\";", cfg->eap_cfg.client_key_password);
        if (res >= cfg_str_len)
            goto end;
        ptr += res;
        cfg_str_len -= res;

        res = dbg_snprintf(ptr, cfg_str_len, "private_key \"client.key\";");
        if (res >= cfg_str_len)
            goto end;
        ptr += res;
        cfg_str_len -= res;

        res = dbg_snprintf(ptr, cfg_str_len, "client_cert \"client.cert\";");
        if (res >= cfg_str_len)
            goto end;
        ptr += res;
        cfg_str_len -= res;

        res = dbg_snprintf(ptr, cfg_str_len, "ca_cert \"ca.cert\";");
        if (res >= cfg_str_len)
            goto end;
        ptr += res;
        cfg_str_len -= res;
    }
#endif /* CFG_8021x_EAP_TLS */

    *ptr = 0;
    res = wifi_wpa_create_network(vif_idx, cfg_str, true);

  end:
    if (res > 0)
    {
        dbg_print(ERR, "Missing at least %d character for wpa_supplicant config\r\n",
                    res - cfg_str_len);
        res = -1;
    }
    sys_mfree(cfg_str);
    return res;
}

#ifdef CFG_SOFTAP
int wifi_wpa_ap_cfg(int vif_idx, struct ap_cfg *cfg)
{
    struct mac_chan_def *chan = NULL;
    char *cfg_str, *ptr;
    int res, cfg_str_len = 300;
    int key_len;
    uint32_t akm;
    bool mesh_mode;
    struct mac_chan_op chan_cfg;
    uint8_t extra_cfg = 0;
    uint32_t unicast_cipher = 0, group_cipher = 0;

    if ((vif_idx >= CFG_VIF_NUM) || (cfg == NULL))
        return -1;

    sys_memset(&chan_cfg, 0, sizeof(chan_cfg));
    chan_cfg.prim20_freq = wifi_channel_to_freq(cfg->channel);
    if (chan_cfg.prim20_freq == 0)
        return -2;

    chan_cfg.type = PHY_CHNL_BW_20;
    chan_cfg.band = PHY_BAND_2G4;
    chan_cfg.center1_freq = chan_cfg.prim20_freq;

    if (macif_vif_type_get(vif_idx) == VIF_AP)
    {
        mesh_mode = false;

        #ifdef CFG_SAE_PK
        if ((cfg->akm & CO_BIT(MAC_AKM_SAE)) && cfg->sae_pk.enable)
        {
            if (cfg->sae_pk.private_key)
                cfg_str_len += cfg->sae_pk.private_key_len * 2 + 50;
            else
                cfg_str_len += 150;

            if (cfg->akm != CO_BIT(MAC_AKM_SAE))
                cfg_str_len += 64;
        }
        #endif
    }
    else if (macif_vif_type_get(vif_idx) == VIF_MESH_POINT)
    {
        mesh_mode = true;
        cfg->akm &= (CO_BIT(MAC_AKM_NONE) | CO_BIT(MAC_AKM_SAE));
    }
    else
        return -3;

    cfg_str = sys_malloc(cfg_str_len + 1);
    if (!cfg_str)
        return -4;
    ptr = cfg_str;

    if (mesh_mode)
        // Enable MESH mode
        res = dbg_snprintf(ptr, cfg_str_len, "mode 5;");
    else
        // Enable AP mode
        res = dbg_snprintf(ptr, cfg_str_len, "mode 2;");
    ptr += res;
    cfg_str_len -= res;

    // SSID
    res = cfg->ssid_len + 8; // 8 = 'ssid "";'
    if (cfg_str_len < res)
        goto end;

    memcpy(ptr, "ssid \"", 6);
    ptr += 6;
    memcpy(ptr, cfg->ssid, cfg->ssid_len);
    ptr += cfg->ssid_len;
    *ptr++ = '"';
    *ptr++ = ';';
    cfg_str_len -= res;

    // Operating Channel and Mode
    chan = macif_wifi_chan_get(chan_cfg.prim20_freq);
    if (!chan || (chan->flags & (CHAN_NO_IR | CHAN_DISABLED | CHAN_RADAR)))
        goto end;

    res = dbg_snprintf(ptr, cfg_str_len, "frequency %d;", chan_cfg.prim20_freq);
    if (res >= cfg_str_len)
        goto end;
    ptr += res;
    cfg_str_len -= res;

    {
        int vht = 0, he = 0, ht40 = 0, chwidth = 0;

        #ifdef CFG_HE
        if (phy_he_supported() && (cfg->he_disabled == 0))
            he = 1;
        #endif

        res = dbg_snprintf(ptr, cfg_str_len, "vht %d;he %d;ht40 %d;max_oper_chwidth %d;"
                           "vht_center_freq1 %d;vht_center_freq2 %d;",
                           vht, he, ht40, chwidth, chan_cfg.center1_freq,
                           chan_cfg.center2_freq);
        if (res >= cfg_str_len)
            goto end;
        ptr += res;
        cfg_str_len -= res;
    }

    res = dbg_snprintf(ptr, cfg_str_len, "beacon_int %d;dtim_period %d;",
                       cfg->bcn_interval, cfg->dtim_period);
    if (res >= cfg_str_len)
        goto end;
    ptr += res;
    cfg_str_len -= res;

    // AKM (remove unsupported ones)
    akm = cfg->akm & (CO_BIT(MAC_AKM_PSK) |
                      CO_BIT(MAC_AKM_PRE_RSN) |
                      CO_BIT(MAC_AKM_SAE) |
                      CO_BIT(MAC_AKM_NONE));
    if (!akm)
        goto end;

    key_len = cfg->passphrase_len;
    if (akm & CO_BIT(MAC_AKM_NONE))
    {
        if (cfg->akm & ~CO_BIT(MAC_AKM_NONE))
            goto end;
    }
    else if (akm & CO_BIT(MAC_AKM_PRE_RSN))
    {
        if (akm & CO_BIT(MAC_AKM_PSK))
        {
            if (key_len < 8)
                goto end;

            akm = CO_BIT(MAC_AKM_PSK);
            unicast_cipher = CO_BIT(MAC_CIPHER_TKIP);
            // WEP is no longer allowed for group cipher so always use TKIP
            group_cipher =  CO_BIT(MAC_CIPHER_TKIP);
            res = dbg_snprintf(ptr, cfg_str_len, "proto WPA;");
        }
        else if ((key_len == 5) || (key_len == 13))
        {
            akm = CO_BIT(MAC_AKM_NONE);
            res = dbg_snprintf(ptr, cfg_str_len, "wep_key0 \"%s\";wep_tx_keyidx 0;",
                               cfg->passphrase);
        }
        else
            goto end;

        if (res >= cfg_str_len)
            goto end;
        ptr += res;
        cfg_str_len -= res;
    }
    else
    {
        if ((key_len < 8)
            #ifdef CFG_SAE_PK
            && !((key_len == 0) && akm & CO_BIT(MAC_AKM_SAE) && cfg->sae_pk.enable)
            #endif
           )
            goto end;
        res = dbg_snprintf(ptr, cfg_str_len, "proto RSN;");
        if (res >= cfg_str_len)
            goto end;
        ptr += res;
        cfg_str_len -= res;
    }
    res = 8;
    if (cfg_str_len < res)
        goto end;
    memcpy(ptr, "key_mgmt", res);
    ptr += res;
    cfg_str_len -= res;
    res = wifi_wpa_akm_name(akm, ptr, cfg_str_len);
    if (res < 0)
        goto end;
    ptr += res;
    cfg_str_len -= res;

    // WPAx config (cipher, PSK, MFP)
    if (akm & (CO_BIT(MAC_AKM_PSK) | CO_BIT(MAC_AKM_SAE) | CO_BIT(MAC_AKM_DPP)))
    {
        uint32_t group, pairwise;
        uint32_t cipher_supported = 0;

        cipher_supported = macif_setting_supp_cipher_get();

        // Remove unsupported cipher or set default value if not set
        if (unicast_cipher)
        {
            pairwise = unicast_cipher & cipher_supported;
            if (!pairwise)
                goto end;
        }
        else
            pairwise = CO_BIT(MAC_CIPHER_CCMP);

        res = 8;
        if (cfg_str_len < res)
            goto end;
        memcpy(ptr, "pairwise", res);
        ptr += res;
        cfg_str_len -= res;
        res = wifi_wpa_cipher_name(pairwise, ptr, cfg_str_len);
        if (res < 0)
            goto end;
        ptr += res;
        cfg_str_len -= res;

        if (group_cipher)
        {
            group = group_cipher & cipher_supported;
            if (!group)
                goto end;
        }
        else
            group = CO_BIT(MAC_CIPHER_CCMP);

        res = 5;
        if (cfg_str_len < res)
            goto end;
        memcpy(ptr, "group", res);
        ptr += res;
        cfg_str_len -= res;
        res = wifi_wpa_cipher_name(group, ptr, cfg_str_len);
        if (res < 0)
            goto end;
        ptr += res;
        cfg_str_len -= res;

        #ifdef CFG_SAE_PK
        if ((cfg->akm & CO_BIT(MAC_AKM_SAE)) && (cfg->sae_pk.enable))
        {
            res = wifi_wpa_ap_sae_pk_password(cfg, ptr, cfg_str_len);
            if (res < 0)
                goto end;
            ptr += res;
            cfg_str_len -= res;

            if (cfg->akm == CO_BIT(MAC_AKM_SAE))
                // If only SAE is enabled no need to also set psk
                key_len = 0;
            else
                // If other AKM are enabled, re-use the SAE-PK password for other mode
                key_len = cfg->passphrase_len;
        }
        #endif

        if (key_len)
        {
            res = dbg_snprintf(ptr, cfg_str_len, "psk \"%s\";", cfg->passphrase);
            if (res >= cfg_str_len)
                goto end;
            ptr += res;
            cfg_str_len -= res;
        }

        if (akm & CO_BIT(MAC_AKM_SAE)) {
            if (akm & CO_BIT(MAC_AKM_PSK)) {
                // MFP is optional in WPA2/WP3 transition mode
                cfg->mfp = 1;
            } else {
                // MFP is mandatory in SAE
                cfg->mfp = 2;
            }
        } else {
            cfg->mfp = 0;
        }

        #ifdef CFG_MFP
        if (cfg->mfp > 2)
            goto end;

        res = dbg_snprintf(ptr, cfg_str_len, "ieee80211w %d;", cfg->mfp);
        if (res >= cfg_str_len)
            goto end;
        ptr += res;
        cfg_str_len -= res;
        #endif
    }

    // Extra user config
    if (extra_cfg)
    {
        res = dbg_snprintf(ptr, cfg_str_len, "%s", extra_cfg);
        if (res >= cfg_str_len)
            goto end;
        ptr += res;
        cfg_str_len -= res;
    }

    // ignore broadcast ssid
    if (cfg->hidden)
    {
        res = dbg_snprintf(ptr, cfg_str_len, "ignore_broadcast_ssid %u;", cfg->hidden);
        if (res >= cfg_str_len)
            goto end;
        ptr += res;
        cfg_str_len -= res;
    }

    res = dbg_snprintf(ptr, cfg_str_len, "wps_disabled 1;");
    if (res >= cfg_str_len)
        goto end;
    ptr += res;
    cfg_str_len -= res;

    *ptr = 0;

    if (mesh_mode)
        res = wifi_wpa_create_network(vif_idx, cfg_str, true);
    else
        res = (wifi_wpa_create_network(vif_idx, cfg_str, false) |
               wifi_wpa_execute_cmd(vif_idx, NULL, 0, 300, "AP_SCAN 2") |
               wifi_wpa_enable_network(vif_idx));

    if (res)
        wifi_wpa_remove_vif(vif_idx);

  end:
    if (res > 0)
    {
        if (res >= cfg_str_len)
        {
            dbg_print(ERR, "Missing at least %d char for wpa_supplicant config (AP)\r\n",
                        res - cfg_str_len);
        }
        else
        {
            dbg_print(ERR, "Invalid AP config: chan_freq=%d chan_flags=%x "
                        "akm=%x unicast=%x group=%x key_len=%d, cfg_str=%s\r\n",
                        chan_cfg.prim20_freq, (chan) ? chan->flags : 0xffff,
                        cfg->akm, unicast_cipher,
                        group_cipher, cfg->passphrase_len, cfg_str);
        }
        res = -1;
    }
    sys_mfree(cfg_str);
    return res;
}

int wifi_wpa_ap_delete_client(int vif_idx, const uint8_t *mac_addr, int reason)
{
    char *cfg_str, *ptr;
    int res, cfg_str_len = 30; // xx:xx:xx:xx:xx:xx reason=1

    cfg_str = sys_zalloc(cfg_str_len + 1);
    if (!cfg_str)
        return -1;
    ptr = cfg_str;

    res = dbg_snprintf(ptr, cfg_str_len, "%02x:%02x:%02x:%02x:%02x:%02x ",
                           mac_addr[0], mac_addr[1], mac_addr[2],
                           mac_addr[3], mac_addr[4], mac_addr[5]);
    if (res >= cfg_str_len)
        goto end;
    ptr += res;
    cfg_str_len -= res;

    res = dbg_snprintf(ptr, cfg_str_len, "reason=%d ", reason);
    if (res >= cfg_str_len)
        goto end;
    ptr += res;
    cfg_str_len -= res;

    *ptr = 0;

    if (wifi_wpa_execute_cmd(vif_idx, NULL, NULL, -1, "DEAUTHENTICATE %s", cfg_str)) {
        res = -1;
    } else {
        res = 0;
    }

end:
    if (res > 0) {
        dbg_print(ERR, "Missing at least %d character for wpa_supplicant config\r\n",
                    res - cfg_str_len);
        res = -1;
    }
    sys_mfree(cfg_str);
    return res;
}
#endif

int wifi_wpa_ap_sm_step(int vif_idx, uint16_t event, uint8_t *data, uint32_t data_len)
{
    if (event == WIFI_MGMT_EVENT_START_AP_CMD)
        return wifi_wpa_ap_cfg(vif_idx, (struct ap_cfg *)data);
    else if (event == WIFI_MGMT_EVENT_STOP_AP_CMD)
        return wifi_wpa_remove_vif(vif_idx);
    return 0;
}

#ifdef CFG_8021x_EAP_TLS
char *wifi_wpa_sta_read_eap_tls_files(const char *name, size_t *len)
{
    int vif_idx = WIFI_VIF_INDEX_STA_MODE;
    struct wifi_vif_tag *wvif;
    struct sta_cfg *cfg;

    wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(vif_idx);
    cfg = &wvif->sta.cfg;

    if (strncmp(name, "client.cert", 11) == 0) {
        *len = strlen(cfg->eap_cfg.client_cert);
        return (char *)cfg->eap_cfg.client_cert;
    } else if (strncmp(name, "client.key", 10) == 0) {
        *len = strlen(cfg->eap_cfg.client_key);
        return (char *)cfg->eap_cfg.client_key;
    } else if (strncmp(name, "ca.cert", 7) == 0) {
        *len = strlen(cfg->eap_cfg.ca_cert);
        return (char *)cfg->eap_cfg.ca_cert;
    } else {
        dbg_print(WARNING, "TODO: os_readfile:%s\r\n",name);
    }
    return NULL;
}
#endif /* CFG_8021x_EAP_TLS */

#else /* CONFIG_WPA_SUPPLICANT */
#include "wpas_wps.h"
#include "wpas_eap.h"

#include "wpas_sae_crypto_mbedtls.c"

int wifi_wpa_scan_sock_get(int vif_idx)
{
    return -1;
}

int wifi_wpa_scan_sock_set(int vif_idx, int scan_sock)
{
    return -1;
}

int wifi_wpa_conn_sock_get(int vif_idx)
{
    return -1;
}

int wifi_wpa_conn_sock_set(int vif_idx, int conn_sock)
{
    return -1;
}

int wifi_wpa_ftm_sock_get(int vif_idx)
{
    return -1;
}

int wifi_wpa_ftm_sock_set(int vif_idx, int ftm_sock)
{
    return -1;
}

int wifi_wpa_rx_eapol_event(void *wvif, uint16_t type, uint8_t *data, uint32_t len)
{
    uint8_t *frame = NULL;

    if (type != ETH_P_EAPOL) {
        // wpa_printf("WPAS: Not a eapol frame\r\n");
        return -1;
    }
    frame = sys_malloc(len);
    if (!frame) {
        wpa_printf("WPAS: failed to alloc rx eapol frame\r\n");
        return -2;
    }
    sys_memcpy(frame, data, len);
    if (eloop_message_send(wvif_to_vif_idx(wvif), WIFI_MGMT_EVENT_RX_EAPOL, 0,
                       frame, len)) {
        sys_mfree(frame);
    }
    return 0;
}

void wifi_wpa_rx_mgmt_cb(struct wifi_frame_info *info, void *arg)
{
    struct mac_hdr *hdr = (struct mac_hdr *)info->payload;
    struct wifi_vif_tag *wvif;
    struct ieee80211_mgmt *mgmt;
    uint16_t subtype;
    uint8_t *frm = NULL;

    if (info->vif_idx >= CFG_VIF_NUM)
    {
        wpa_printf("WPAS: Ignore Management frame received on invalid VIF\r\n");
        return;
    }

    wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(info->vif_idx);

    /* Only Management Frame */
    if ((info->length == 0) ||
        (info->payload == NULL) ||
        ((hdr->fctl & MAC_FCTRL_TYPE_MASK) != MAC_FCTRL_MGT_T))
        return;

    subtype = (hdr->fctl & MAC_FCTRL_SUBT_MASK);
    if (wvif->wvif_type == WVIF_STA) {
        if (subtype != MAC_FCTRL_ACTION_ST && subtype != MAC_FCTRL_AUTHENT_ST \
            && subtype != MAC_FCTRL_DISASSOC_ST && subtype != MAC_FCTRL_DEAUTHENT_ST // for sa query
            ) {
            return;
        }
    } else if (wvif->wvif_type == WVIF_AP) {
        if (subtype == MAC_FCTRL_BEACON_ST)
            return;
    }

    mgmt = (struct ieee80211_mgmt *)info->payload;
    if (wvif->wvif_type == WVIF_STA) {
        if ((subtype == MAC_FCTRL_AUTHENT_ST) && (mgmt->u.auth.auth_alg == MAC_AUTH_ALGO_SAE)) {
            frm = sys_malloc(info->length);
            if (frm) {
                sys_memcpy(frm, info->payload, info->length);
                if (eloop_message_send(info->vif_idx, WIFI_MGMT_EVENT_RX_MGMT, 0, frm, info->length)) {
                    sys_mfree(frm);
                }
            }
        } else if (subtype == MAC_FCTRL_DISASSOC_ST || subtype == MAC_FCTRL_DEAUTHENT_ST) {
            frm = sys_malloc(info->length);
            if (frm) {
                sys_memcpy(frm, info->payload, info->length);
                if (eloop_message_send(info->vif_idx, WIFI_MGMT_EVENT_RX_UNPROT_DEAUTH, 0, frm, info->length)) {
                    sys_mfree(frm);
                }
            }
        } else if (subtype == MAC_FCTRL_ACTION_ST) {
            frm = sys_malloc(info->length);
            if (frm) {
                sys_memcpy(frm, info->payload, info->length);
                if (eloop_message_send(info->vif_idx, WIFI_MGMT_EVENT_RX_ACTION, 0, frm, info->length)) {
                    sys_mfree(frm);
                }
            }
        }
    } else if (wvif->wvif_type == WVIF_AP) {
        uint8_t frm_info_len = sizeof(struct wifi_frame_info);
        struct ieee80211_mgmt *mgmt;
        mgmt = (struct ieee80211_mgmt *)(info->payload);

        // ignore when management frame subtype is 0xF(Reserved)
        if ( (((mgmt->frame_control & 0x000c) >> 2) == 0) && (((mgmt->frame_control & 0x00f0) >> 4) == 0xf) ) {
            return;
        }

        frm = sys_malloc(frm_info_len + info->length);
        if (frm) {
            sys_memcpy(frm, info, frm_info_len);
            sys_memcpy((frm + frm_info_len), info->payload, info->length);
            if (eloop_message_send(info->vif_idx, WIFI_MGMT_EVENT_RX_MGMT, 0, frm, (frm_info_len + info->length))) {
                sys_mfree(frm);
            }
        }
    }
}

int wifi_wpa_eapol_to_vif_idx(struct wpas_eapol *eapol)
{
    struct wifi_sta *sta = (struct wifi_sta *)((uint32_t)eapol - offsetof(struct wifi_sta, w_eapol));
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)((uint32_t)sta - offsetof(struct wifi_vif_tag, sta));
    int vif_idx = wvif_to_vif_idx(wvif);

    //printf("===============>wifi_wpa_eapol_to_vif_idx: vif_idx %d\r\n", vif_idx);
    //printf("eapol = %p, sta = %p, wvif %p, wifi_vif_tab[0] %p, wifi_vif_tab[1] %p\r\n",
    //        eapol, sta, wvif, &wifi_vif_tab[0], &wifi_vif_tab[1]);
    return vif_idx;
}

uint16_t wifi_wpa_get_disconnect_reason(void)
{
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(WIFI_VIF_INDEX_STA_MODE);

    return wvif->sta.reason_code;
}

uint16_t wifi_wpa_get_connect_fail_status(void)
{
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(WIFI_VIF_INDEX_STA_MODE);

    return wvif->sta.status_code;
}

int wifi_wpa_sae_to_vif_idx(struct wpas_sae *sae)
{
    struct wifi_sta *sta = (struct wifi_sta *)((uint32_t)sae - offsetof(struct wifi_sta, w_sae));
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)((uint32_t)sta - offsetof(struct wifi_vif_tag, sta));
    int vif_idx = wvif_to_vif_idx(wvif);

    //printf("===============>wifi_wpa_eapol_to_vif_idx: \r\n");
    //printf("sae = %p, sta = %p, wvif %p, wifi_vif_tab[0] %p, wifi_vif_tab[1] %p\r\n",
    //        sae, sta, wvif, &wifi_vif_tab[0], &wifi_vif_tab[1]);
    return vif_idx;
}

int wifi_wpa_sa_query_to_vif_idx(struct sa_query_data *sa_query)
{
    struct wifi_sta *sta = (struct wifi_sta *)((uint32_t)sa_query - offsetof(struct wifi_sta, sa_query));
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)((uint32_t)sta - offsetof(struct wifi_vif_tag, sta));
    int vif_idx = wvif_to_vif_idx(wvif);

    return vif_idx;
}

int wifi_wpa_ap_to_vif_idx(struct wpas_ap *w_ap)
{
    struct wifi_ap *ap = (struct wifi_ap *)((uint32_t)w_ap - offsetof(struct wifi_ap, w_ap));
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)((uint32_t)ap - offsetof(struct wifi_vif_tag, ap));
    int vif_idx = wvif_to_vif_idx(wvif);

    //printf("===============>wifi_wpa_eapol_to_vif_idx: \r\n");
    //printf("w_ap = %p, ap = %p, wvif %p, wifi_vif_tab[0] %p, wifi_vif_tab[1] %p\r\n",
    //        w_ap, ap, wvif, &wifi_vif_tab[0], &wifi_vif_tab[1]);
    return vif_idx;
}

void *wifi_wpa_w_eapol_get(int vif_idx)
{
    struct wifi_vif_tag *wvif = vif_idx_to_wvif(vif_idx);

    if (NULL == wvif)
        return NULL;

    return &wvif->sta.w_eapol;
}

void *wifi_wpa_w_sae_get(int vif_idx)
{
    struct wifi_vif_tag *wvif = vif_idx_to_wvif(vif_idx);

    if (NULL == wvif)
        return NULL;

    return &wvif->sta.w_sae;
}

void *wifi_wpa_w_sa_query_get(int vif_idx)
{
    struct wifi_vif_tag *wvif = vif_idx_to_wvif(vif_idx);

    if (NULL == wvif)
        return NULL;

    return &wvif->sta.sa_query;
}

void *wifi_wpa_w_ap_get(int vif_idx)
{
    struct wifi_vif_tag *wvif = vif_idx_to_wvif(vif_idx);

    if (NULL == wvif)
        return NULL;

    return &wvif->ap.w_ap;
}

void *wifi_wpa_sta_eapol_cache_get(struct wpas_eapol *eapol)
{
    struct wifi_sta *sta = (struct wifi_sta *)((uint32_t)eapol - offsetof(struct wifi_sta, w_eapol));

    return &sta->cache;
}

void *wifi_wpa_sta_sae_cache_get(struct wpas_sae *w_sae)
{
    struct wifi_sta *sta = (struct wifi_sta *)((uint32_t)w_sae - offsetof(struct wifi_sta, w_sae));

    return &sta->cache;
}

#ifdef CFG_WPS
void *wifi_wpa_sta_wps_ctx_get(int vif_idx)
{
    struct wifi_vif_tag *wvif = vif_idx_to_wvif(vif_idx);

    return wvif->sta.wps_ctx;
}

struct wps_context *wifi_wpa_wps_ctx_init(int vif_idx, struct wps_config_t *wps_cfg)
{
    struct wps_context *wps_ctx;
    uint8_t *mac_addr = wifi_vif_mac_addr_get(vif_idx);

    wps_ctx = sys_zalloc(sizeof(*wps_ctx));
    if (wps_ctx == NULL)
        return NULL;

    if (wps_cfg->registrar) {
        wps_ctx->is_ap = 1;
        wps_ctx->is_registrar = 1;
    }
    if (wps_cfg->pbc) {
        wps_ctx->is_pbc = 1;
    } else {
        wps_ctx->is_pbc = 0;
        sys_memcpy(wps_ctx->pin, wps_cfg->pin, sizeof(wps_ctx->pin));
    }
    wps_ctx->dev.config_methods = (WPS_CONFIG_VIRT_DISPLAY | WPS_CONFIG_KEYPAD);

    wps_ctx->dev.device_name = "";
    wps_ctx->dev.manufacturer = "";
    wps_ctx->dev.model_name = "";
    wps_ctx->dev.model_number = "";
    wps_ctx->dev.serial_number = "";
    //wps_ctx->dev.config_methods = wps_ctx->config_methods;
    //sys_memcpy(wps->dev.pri_dev_type, wpa_s->conf->device_type, WPS_DEV_TYPE_LEN);
    //wpas_wps_set_vendor_ext_m1(wpa_s, wps);
    //wps->dev.os_version = WPA_GET_BE32(wpa_s->conf->os_version);
    wps_ctx->dev.rf_bands = WPS_RF_24GHZ;
    sys_memcpy(wps_ctx->dev.mac_addr, mac_addr, ETH_ALEN);
    uuid_gen_by_mac_addr(wps_ctx->dev.mac_addr, wps_ctx->uuid);

    if (wps_ctx->is_registrar) {
        wps_ctx->identity = (uint8_t *)WSC_ID_REGISTRAR;
        wps_ctx->identity_len = WSC_ID_REGISTRAR_LEN;
    } else {
        wps_ctx->identity = (uint8_t *)WSC_ID_ENROLLEE;
        wps_ctx->identity_len = WSC_ID_ENROLLEE_LEN;
    }

    wps_ctx->wps_pin_start_time = sys_current_time_get();

#ifdef CONFIG_WPS_AP
#ifdef CONFIG_NO_TKIP
    wps_ctx->auth_types = WPS_AUTH_WPA2PSK;
    wps_ctx->encr_types = WPS_ENCR_AES;
#else /* CONFIG_NO_TKIP */
    wps_ctx->auth_types = WPS_AUTH_WPA2PSK | WPS_AUTH_WPAPSK;
    wps_ctx->encr_types = WPS_ENCR_AES | WPS_ENCR_TKIP;
#endif /* CONFIG_NO_TKIP */

    struct wps_registrar_config rcfg;
    os_memset(&rcfg, 0, sizeof(rcfg));
    rcfg.new_psk_cb = wpas_wps_new_psk_cb;
    rcfg.pin_needed_cb = wpas_wps_pin_needed_cb;
    rcfg.set_sel_reg_cb = wpas_wps_set_sel_reg_cb;
    rcfg.cb_ctx = wpa_s;

    struct hostapd_hw_modes *modes;
    wps_ctx->registrar = wps_registrar_init(wps_ctx, &rcfg);
    if (wps_ctx->registrar == NULL) {
        wpa_printf("Failed to initialize WPS Registrar\r\n");
        sys_mfree(wps_ctx);
        return NULL;
    }
#endif

    return wps_ctx;
}

void wifi_wpa_wps_ctx_deinit(struct wps_context *wps_ctx)
{
    if (NULL == wps_ctx)
        return;

    if (wps_ctx->probe_req_extra_ie) {
        sys_mfree(wps_ctx->probe_req_extra_ie);
        wps_ctx->probe_req_extra_ie = NULL;
    }

    sys_mfree(wps_ctx);
}

int wifi_wpa_wps_associate(int vif_idx, uint8_t *frame, uint32_t frame_len)
{
    struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *)(frame + 1);
    struct mac_scan_result candidate;
    struct wifi_vif_tag *wvif = vif_idx_to_wvif(vif_idx);
    struct sta_cfg *cfg = &wvif->sta.cfg;
    struct wpabuf *wps_ie;
    int ret, res = 0;

    sys_memset(&candidate, 0, sizeof(candidate));
    ret = wpas_get_mac_scan_result(vif_idx, mgmt->bssid, &candidate);
    if (ret) {
        return WIFI_MGMT_CONN_NO_AP;
    }
    wpa_printf("WPS: selected BSS "MACSTR" ssid=%s\r\n", MAC2STR(mgmt->bssid), candidate.ssid);

    wps_ie = wps_build_assoc_req_ie(WPS_REQ_ENROLLEE);
    if (NULL == wps_ie) {
        return WIFI_MGMT_CONN_UNSPECIFIED;
    }

    cfg->ssid_len = candidate.ssid.length;
    if (cfg->ssid_len)
        sys_memcpy(cfg->ssid, candidate.ssid.array, cfg->ssid_len);
    sys_memcpy(cfg->bssid, (uint8_t *)candidate.bssid.array, MAC_ADDR_LEN);
    cfg->channel = wifi_freq_to_channel(candidate.chan->freq);

    ret = wpas_set_mac_connect(vif_idx, &candidate, wps_ie->buf, wps_ie->size, true);
    if (ret == -1) {
        res = WIFI_MGMT_CONN_UNSPECIFIED;
    } else if (ret == -2) {
        res = WIFI_MGMT_CONN_ASSOC_FAIL;
    }

    wpabuf_free(wps_ie);
    wpa_printf("   start wps associate(res %d, ret %d)...\r\n", res, ret);

    return res;
}

int wifi_wpa_wps_associate_done(int vif_idx, void *ind_param)
{
    struct wifi_vif_tag *wvif = vif_idx_to_wvif(vif_idx);
    struct macif_connect_ind *ind_info = (struct macif_connect_ind *)ind_param;
    struct wifi_sta *config_sta = &wvif->sta;

    config_sta->ap_id = ind_info->ap_idx;
    config_sta->aid = ind_info->aid;

#ifdef CFG_WIFI_RX_STATS
    macif_alloc_rx_rates(config_sta->ap_id);
#endif
    macif_tx_sta_add(config_sta->ap_id, 0);
    net_if_up(&wvif->net_if);

    wpas_eap_start(wvif->sta.esm);

    return 0;
}

int wifi_wpa_send_wps_cred_event(int vif_idx, struct wps_credential *cred)
{
    struct wps_cred_t *ev_cred;
    uint32_t ev_cred_len = sizeof(struct wps_cred_t);

    wpa_printf("WPS: credit got event\r\n");

    ev_cred = sys_malloc(ev_cred_len);
    if (!ev_cred)
        return -1;
    if (cred->ssid_len) {
        sys_memcpy(ev_cred->ssid, cred->ssid, cred->ssid_len);
        ev_cred->ssid_len = cred->ssid_len;
    }
    if (cred->key_len) {
        sys_memcpy(ev_cred->passphrase, cred->key, cred->key_len);
        ev_cred->passphrase_len = cred->key_len;
    }

    if (eloop_message_send(vif_idx, WIFI_MGMT_EVENT_WPS_CRED,
                            0,(uint8_t *)ev_cred, ev_cred_len)) {
        sys_mfree(ev_cred);
        return -1;
    }

    return 0;
}

int wifi_wpa_send_wps_success_event(int vif_idx)
{
    wpa_printf("WPS: success event\r\n");
    return eloop_message_send(vif_idx, WIFI_MGMT_EVENT_WPS_SUCCESS,
                            0, NULL, 0);
}

int wifi_wpa_send_wps_fail_event(int vif_idx)
{
    wpa_printf("WPS: fail event\r\n");
    return eloop_message_send(vif_idx, WIFI_MGMT_EVENT_WPS_FAIL,
                            0, NULL, 0);
}

void wifi_wpa_wps_scan_timer(void *eloop_data, void *user_ctx)
{
    int vif_idx = (int)eloop_data;
    struct wifi_vif_tag *wvif = &wifi_vif_tab[vif_idx];
    struct wps_context *wps_ctx = wvif->sta.wps_ctx;
    int ret;

    if (NULL == wps_ctx) {
        return;
    }
    ret = wifi_netlink_scan_set_with_extraie(vif_idx, 0xFF,
                            wps_ctx->probe_req_extra_ie, wps_ctx->probe_req_extra_ie_len);
    if (ret) {
        eloop_message_send(vif_idx, WIFI_MGMT_EVENT_SCAN_FAIL, 0, NULL, 0);
    }
}

int wifi_wpa_wps_ssid_bss_match(int vif_idx, uint8_t *frame, uint32_t frame_len)
{
    return wpas_wps_ssid_bss_match(vif_idx, frame, frame_len);
}

/*!
    \brief      start wps pbc with the wps ap
    \param[in]  vif_idx: index of the wifi vif
    \param[out] none
    \retval     0 on success and != 0 if error occured.
*/
int wifi_wpa_wps_start(int vif_idx)
{
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(vif_idx);
    struct wps_config_t *wps_cfg = &wvif->sta.cfg.wps_cfg;
    struct wps_context *wps_ctx;
    int ret;

    /* 1. init wps_context */
    wvif->sta.wps_ctx = wifi_wpa_wps_ctx_init(vif_idx, wps_cfg);
    if (!wvif->sta.wps_ctx) {
        netlink_printf("Netlink: wps start failed, wps_ctx is null.\r\n");
        return -1;
    }
    wps_ctx = wvif->sta.wps_ctx;

    /* 2. init eapol_sm, eap_sm */
    wvif->sta.esm = eapol_sm_init(vif_idx, WORK_TYPE_WPS);
    if (!wvif->sta.esm) {
        netlink_printf("Netlink: wps start failed, esm is null.\r\n");
        return -2;
    }

    /* 3. register methods to global eap_methods */
    eap_register_methods();

    /* 4. Allocate and build wps extra ie for probe req */
    ret = wpas_wps_build_probe_req_ie(vif_idx);
    if (ret) {
        netlink_printf("Netlink: wps start failed, build probe req ie return %d.\r\n", ret);
        return -3;
    }

    /* 5. Start scan with extra ie */
    ret = wifi_netlink_scan_set_with_extraie(vif_idx, 0xFF,
                            wps_ctx->probe_req_extra_ie, wps_ctx->probe_req_extra_ie_len);
    if (ret) {
        netlink_printf("Netlink: wps start failed, scan set return %d.\r\n", ret);
        return -4;
    }
    return 0;
}

/*!
    \brief      stop wps
    \param[in]  vif_idx: index of the wifi vif
    \param[out] none
    \retval     0 on success and != 0 if error occured.
*/
int wifi_wpa_wps_stop(int vif_idx)
{
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(vif_idx);
    struct eapol_sm *esm = wvif->sta.esm;
    struct wps_context *wps_ctx = wvif->sta.wps_ctx;

    eloop_timeout_cancel(wifi_wpa_wps_scan_timer, (void *)vif_idx, NULL);

    eapol_sm_deinit(esm);
    wvif->sta.esm = NULL;

    wifi_wpa_wps_ctx_deinit(wps_ctx);
    wvif->sta.wps_ctx = NULL;

    eap_unregister_methods();

    return 0;
}
#endif

#ifdef CFG_8021x_EAP_TLS
void *wifi_wpa_sta_eap_ctx_get(int vif_idx)
{
    struct wifi_vif_tag *wvif = vif_idx_to_wvif(vif_idx);

    return wvif->sta.eap_ctx;
}

struct eap_context *wifi_wpa_eap_ctx_init(int vif_idx, struct eap_config_t *eap_cfg)
{
    struct eap_context *eap_ctx;

    eap_ctx = sys_zalloc(sizeof(*eap_ctx));
    if (eap_ctx == NULL)
        return NULL;

    eap_ctx->ca_cert             = eap_cfg->ca_cert;
    eap_ctx->client_key          = eap_cfg->client_key;
    eap_ctx->client_key_password = eap_cfg->client_key_password;
    eap_ctx->identity            = eap_cfg->identity;
    eap_ctx->identity_len        = strlen(eap_cfg->identity);
    eap_ctx->client_cert         = eap_cfg->client_cert;
    eap_ctx->phase1              = eap_cfg->phase1;

    return eap_ctx;
}

void wifi_wpa_eap_ctx_deinit(struct eap_context *eap_ctx)
{
    sys_mfree(eap_ctx);
}

int wifi_wpa_eap_init(int vif_idx)
{
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(vif_idx);
    struct eap_config_t *eap_cfg = &wvif->sta.cfg.eap_cfg;

    if ((wvif->sta.eap_ctx != NULL) || (wvif->sta.esm != NULL)) {
        wifi_wpa_eap_deinit(vif_idx);
    }

    wvif->sta.eap_ctx = wifi_wpa_eap_ctx_init(vif_idx, eap_cfg);
    if (!wvif->sta.eap_ctx) {
        netlink_printf("Netlink: eap start failed, eap_ctx is null.\r\n");
        return -1;
    }

    wvif->sta.esm = eapol_sm_init(vif_idx, WORK_TYPE_EAP_TLS);
    if (!wvif->sta.esm) {
        netlink_printf("Netlink: eap start failed, esm is null.\r\n");
        return -2;
    }

    eap_register_methods();

    return 0;
}

/*!
    \brief      stop wps
    \param[in]  vif_idx: index of the wifi vif
    \param[out] none
    \retval     0 on success and != 0 if error occured.
*/
void wifi_wpa_eap_deinit(int vif_idx)
{
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(vif_idx);
    struct eap_config_t *eap_cfg = &wvif->sta.cfg.eap_cfg;

    if (wvif->sta.eap_ctx) {
        sys_mfree(wvif->sta.eap_ctx);
        wvif->sta.eap_ctx = NULL;
        wpa_printf("EAP: eap_ctx deinit.\r\n");
    }

    if (wvif->sta.esm) {
        eapol_sm_deinit(wvif->sta.esm);
        wvif->sta.esm = NULL;
        wpa_printf("EAP: eapol_sm deinit.\r\n");
    }

    eap_unregister_methods();
}
#endif

int wifi_wpa_eapol_info_get(int vif_idx, struct eapol_info *info)
{
    struct wifi_vif_tag *wvif = vif_idx_to_wvif(vif_idx);
    struct sta_cfg *cfg;

    if (NULL == wvif)
        return -1;
    cfg = &wvif->sta.cfg;

    /*  set own mac addr  */
    sys_memcpy(info->own_addr, (uint8_t *)wvif->mac_addr.array, sizeof(info->own_addr));

    /*  set bssid  */
    if (is_zero_ether_addr(cfg->bssid)) {
        wpa_printf("EAPOL: no bssid set of interface %d\r\n", vif_idx);
        return -2;
    }
    sys_memcpy(info->bssid, cfg->bssid, sizeof(info->bssid));

    /* save akm */
    info->mac_akm = cfg->akm;

    /*  set keymgmt  */
    if (cfg->akm != 0) {
        info->key_mgmt = wpas_mac_2_wpa_keymgmt(cfg->akm);
        if (info->key_mgmt == -1) {
            wpa_printf("EAPOL: no supported keymgmt\r\n");
            return -3;
        }
    }

    /*  set group / pairwise / management cipher  */
    if ((cfg->g_cipher !=0) && (cfg->p_cipher != 0)) {
        info->group_cipher = wpas_mac_2_wpa_cipher(cfg->g_cipher);
        info->pairwise_cipher = wpas_mac_2_wpa_cipher(cfg->p_cipher);
        info->mgmt_group_cipher = wpas_mac_2_wpa_management_cipher(cfg->g_cipher);

        if ((info->pairwise_cipher == -1) || (info->group_cipher == -1)) {
            wpa_printf("EAPOL: unsupported group or pariwise cipher\r\n");
            return -4;
        }
    }

    return 0;
}

char * wifi_wpa_sta_cfg_ssid_get(int vif_idx, uint32_t *ssid_len)
{
    struct wifi_vif_tag *wvif = vif_idx_to_wvif(vif_idx);

    if (NULL == wvif)
        return NULL;

    if (ssid_len)
        *ssid_len = wvif->sta.cfg.ssid_len;
    return wvif->sta.cfg.ssid;
}

char * wifi_wpa_sta_cfg_passphrase_get(int vif_idx, uint32_t *pwd_len)
{
    struct wifi_vif_tag *wvif = vif_idx_to_wvif(vif_idx);

    if (NULL == wvif)
        return NULL;

    if (pwd_len)
        *pwd_len = wvif->sta.cfg.passphrase_len;
    return wvif->sta.cfg.passphrase;
}

uint8_t * wifi_wpa_sta_cfg_bssid_get(int vif_idx)
{
    struct wifi_vif_tag *wvif = vif_idx_to_wvif(vif_idx);

    if (NULL == wvif)
        return NULL;

    return wvif->sta.cfg.bssid;
}

uint32_t wifi_wpa_sta_cfg_akm_get(int vif_idx)
{
    struct wifi_vif_tag *wvif = vif_idx_to_wvif(vif_idx);

    if (NULL == wvif)
        return 0;

    return wvif->sta.cfg.akm;
}

uint8_t wifi_wpa_sta_cfg_flush_cache_req_get(int vif_idx)
{
    struct wifi_vif_tag *wvif = vif_idx_to_wvif(vif_idx);

    if (NULL == wvif)
        return 0;

    return wvif->sta.cfg.flush_cache_req;
}

char * wifi_wpa_ap_cfg_ssid_get(struct wpas_ap *w_ap, uint32_t *ssid_len)
{
    struct wifi_ap *ap = (struct wifi_ap *)((uint32_t)w_ap - offsetof(struct wifi_ap, w_ap));

    if (ssid_len)
        *ssid_len = ap->cfg.ssid_len;
    return ap->cfg.ssid;
}

char * wifi_wpa_ap_cfg_passphrase_get(struct wpas_ap *w_ap, uint32_t *pwd_len)
{
    struct wifi_ap *ap = (struct wifi_ap *)((uint32_t)w_ap - offsetof(struct wifi_ap, w_ap));

    if (pwd_len)
        *pwd_len = ap->cfg.passphrase_len;
    return ap->cfg.passphrase;
}

uint8_t * wifi_wpa_ap_cfg_bssid_get(struct wpas_ap *w_ap)
{
    struct wifi_ap *ap = (struct wifi_ap *)((uint32_t)w_ap - offsetof(struct wifi_ap, w_ap));

    return ap->cfg.bssid;
}

uint8_t wifi_wpa_ap_cfg_he_disabled_get(struct wpas_ap *w_ap)
{
    struct wifi_ap *ap = (struct wifi_ap *)((uint32_t)w_ap - offsetof(struct wifi_ap, w_ap));

    return ap->cfg.he_disabled;
}

uint8_t wifi_wpa_ap_cfg_mfp_get(struct wpas_ap *w_ap)
{
    struct wifi_ap *ap = (struct wifi_ap *)((uint32_t)w_ap - offsetof(struct wifi_ap, w_ap));

    return ap->cfg.mfp;
}

uint8_t wifi_wpa_ap_cfg_channel_get(struct wpas_ap *w_ap)
{
    struct wifi_ap *ap = (struct wifi_ap *)((uint32_t)w_ap - offsetof(struct wifi_ap, w_ap));

    return ap->cfg.channel;
}

uint8_t wifi_wpa_ap_cfg_dtim_period_get(struct wpas_ap *w_ap)
{
    struct wifi_ap *ap = (struct wifi_ap *)((uint32_t)w_ap - offsetof(struct wifi_ap, w_ap));

    return ap->cfg.dtim_period;
}

uint8_t wifi_wpa_ap_cfg_bcn_interval_get(struct wpas_ap *w_ap)
{
    struct wifi_ap *ap = (struct wifi_ap *)((uint32_t)w_ap - offsetof(struct wifi_ap, w_ap));

    return ap->cfg.bcn_interval;
}

uint8_t wifi_wpa_ap_cfg_hidden_get(struct wpas_ap *w_ap)
{
    struct wifi_ap *ap = (struct wifi_ap *)((uint32_t)w_ap - offsetof(struct wifi_ap, w_ap));

    return ap->cfg.hidden;
}

uint32_t wifi_wpa_ap_cfg_akm_get(struct wpas_ap *w_ap)
{
    struct wifi_ap *ap = (struct wifi_ap *)((uint32_t)w_ap - offsetof(struct wifi_ap, w_ap));

    return ap->cfg.akm;
}

uint8_t wifi_wpa_ap_cfg_max_conn_get(struct wpas_ap *w_ap)
{
    struct wifi_ap *ap = (struct wifi_ap *)((uint32_t)w_ap - offsetof(struct wifi_ap, w_ap));

    return ap->cfg.max_conn;
}

int wifi_wpa_send_connect_fail_event(int vif_idx)
{
    return eloop_message_send(vif_idx, WIFI_MGMT_EVENT_CONNECT_FAIL,
                            WIFI_MGMT_CONN_HANDSHAKE_FAIL,
                            NULL, 0);
}

int wifi_wpa_send_rx_mgmt_done_event(int vif_idx, uint8_t *param, uint32_t param_len)
{
    return eloop_message_send(vif_idx, WIFI_MGMT_EVENT_TX_MGMT_DONE, 0,
                            param, param_len);
}

int wifi_wpa_send_disconnect_event(int vif_idx, uint8_t *param, uint32_t param_len)
{
    return eloop_message_send(vif_idx, WIFI_MGMT_EVENT_DISCONNECT, WIFI_MGMT_DISCON_SA_QUERY_FAIL, NULL, 0);
}

int wifi_wpa_send_eap_success_event(int vif_idx)
{
    return eloop_message_send(vif_idx, WIFI_MGMT_EVENT_EAP_SUCCESS, 0, NULL, 0);
}

int wifi_wpa_gen_wpa_or_rsn_ie(int vif_idx)
{
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(vif_idx);
    struct wifi_sta *sta = &wvif->sta;

    wpas_eapol_reset(&sta->w_eapol);
    wifi_wpa_eapol_info_get(vif_idx, &sta->w_eapol.info);

#ifdef CFG_80211R
    if (wpa_key_mgmt_ft(sta->w_eapol.info.key_mgmt)) {
        struct ft_params *ft = sta->ft;
        if (ft) {
            // set md ie parameters
            sys_memcpy(ft->mobility_domain,
                       sta->cfg.md.mobility_domain, MAC_INFOELT_MDE_MDID_LEN);
            ft->mdie_ft_capab = sta->cfg.md.ft_capab;

            wpa_ft_prepare_auth_request(&sta->w_eapol, NULL);
        }

        return 0;
    }
#endif /* CFG_80211R */

    return wpas_gen_wpa_or_rsn_ie(&sta->w_eapol);
}

int wifi_wpa_sta_eapol_key_sm_step(int vif_idx, uint16_t event, uint8_t *data, uint32_t data_len)
{
    struct wpas_eapol *eapol = (struct wpas_eapol *)wifi_wpa_w_eapol_get(vif_idx);
    enum eapol_state_t state = eapol->state;
    int ret = 0;

    if (event == WIFI_MGMT_EVENT_DISCONNECT) {
        wpas_eapol_stop(eapol);
        return 0;
    }

    if (state == EAPOL_STATE_NOTHING) {
        switch(event) {
        case WIFI_MGMT_EVENT_SCAN_RESULT:
        {
            uint8_t from_beacon = *data;
            uint32_t ies_offset;
            if (from_beacon) {
                ies_offset = 1 + offsetof(struct ieee80211_mgmt, u.beacon.variable);  // 1 for "from_beacon" byte
            } else {
                ies_offset = 1 + offsetof(struct ieee80211_mgmt, u.probe_resp.variable);
            }
            wpas_set_wpa_rsn_ie(eapol, (data + ies_offset), (data_len - ies_offset));
            break;
        }
        case WIFI_MGMT_EVENT_ASSOC_SUCCESS:
        case WIFI_MGMT_EVENT_EAP_SUCCESS:
            wpas_eapol_start(eapol, data, data_len);
            break;
        default:
            goto unexpected_events;
        }
     } else if ((state == EAPOL_STATE_PAIRWISE) || (state == EAPOL_STATE_GROUP) || (state == EAPOL_STATE_ESTABLISHED)) {
        switch(event) {
        case WIFI_MGMT_EVENT_RX_EAPOL:
            if (data_len < ETH_HLEN) {
                wpa_printf("EAPOL: rx eapol length < %d\r\n", ETH_HLEN);
                ret = -1;
                break;
            }
            ret = wpas_rx_eapol(eapol, (data + ETH_HLEN), (data_len - ETH_HLEN));
            break;
#ifdef CFG_80211R
        case WIFI_MGMT_EVENT_FT_ROAMING_CMD:
            ret = wifi_wpa_ft_reassociate(vif_idx, (uint8_t *)data);
            break;
#endif /* CFG_80211R */
        default:
            goto unexpected_events;
        }
    } else {
        goto unexpected_events;
    }

    if (ret < 0) {
        wifi_wpa_send_connect_fail_event(vif_idx);
        wpas_eapol_stop(eapol);
        wpa_printf("EAPOL: handshake stop since event %d ret %d\r\n", event, ret);
    }
    return ret;

unexpected_events:
    wpa_printf("EAPOL: vif %u state %u unexpected event %u\r\n", vif_idx, state, event);
    return ret;
}

int wifi_wpa_sta_eapol_sm_step(int vif_idx, uint16_t event, uint8_t *data, uint32_t data_len)
{
    int ret = 0;

#ifdef IEEE8021X_EAPOL
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(vif_idx);
    struct eapol_sm *esm = wvif->sta.esm;
    bool in_eapol_sm = esm ? true : false;

    // wpa_info("%s: ev %d in_eapol_sm = %d data = %p\r\n", __func__, event, in_eapol_sm, data);
    if (in_eapol_sm) {
        switch (event) {
#ifdef CFG_8021x_EAP_TLS
            case WIFI_MGMT_EVENT_EAP_SUCCESS:
                wifi_wpa_eap_deinit(vif_idx);
                ret = wifi_wpa_sta_eapol_key_sm_step(vif_idx, event, data, data_len);
                break;
            case WIFI_MGMT_EVENT_DISCONNECT:
                wifi_wpa_eap_deinit(vif_idx);
                break;
#endif
            case WIFI_MGMT_EVENT_RX_EAPOL:
                if (data_len < ETH_HLEN) {
                    ret = -1;
                    break;
                }
                ret = wpas_eap_rx_eapol(esm, (data + ETH_HLEN), (data_len - ETH_HLEN));
                break;
            case WIFI_MGMT_EVENT_SCAN_RESULT:
                ret = wifi_wpa_sta_eapol_key_sm_step(vif_idx, event, data, data_len);
                break;
            default:
                return 0;
        }
        if (ret < 0) {
            wifi_wpa_send_connect_fail_event(vif_idx);
#ifdef CFG_8021x_EAP_TLS
            if (esm->workType == WORK_TYPE_EAP_TLS)
                wifi_wpa_eap_deinit(vif_idx);
#endif
#ifdef CFG_WPS
            if (esm->workType == WORK_TYPE_WPS)
                wifi_wpa_wps_stop(vif_idx);
#endif
            wpa_printf("EAPOL: eap failed since event %d ret %d\r\n", event, ret);
        }

    } else
#endif /* IEEE8021X_EAPOL */
    {
        ret = wifi_wpa_sta_eapol_key_sm_step(vif_idx, event, data, data_len);
    }

    return ret;
}

int wifi_wpa_sta_sae_sm_step(int vif_idx, uint16_t event, uint8_t *data, uint32_t data_len)
{
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(vif_idx);
    struct wifi_sta *sta = &wvif->sta;
    enum sae_state state = sae_get_state(&sta->w_sae.sae);
    int ret = 0;

    if (event == WIFI_MGMT_EVENT_DISCONNECT) {
        wpas_sae_stop(&sta->w_sae);
        return 0;
    }

    if (state == SAE_NOTHING) {
        switch(event) {
        case WIFI_MGMT_EVENT_EXTERNAL_AUTH_REQUIRED:
            wpas_sae_start(&sta->w_sae);
            break;
        default:
            goto unexpected_events;
        }
     } else if ((state == SAE_COMMITTED) || (state == SAE_CONFIRMED)) {
        switch(event) {
        case WIFI_MGMT_EVENT_RX_MGMT:
            ret = wpas_sae_frame_recved(&sta->w_sae, data, data_len);
            break;
        default:
            goto unexpected_events;
        }
    } else {
        goto unexpected_events;
    }

    if (ret < 0) {
        wpas_set_mac_ext_auth_resp(vif_idx, MAC_ST_FAILURE);
        wpas_sae_stop(&sta->w_sae);
        wpa_printf("WPAS: sae stop since event %d ret %d\r\n", event, ret);
    }
    return ret;

unexpected_events:
    wpa_printf("SAE: vif %u state %u unexpected event %u\r\n",
                   vif_idx, state, event);
    return 0;
}

#ifdef CONFIG_IEEE80211R
void *wifi_wpa_sta_ft_params_get(struct wpas_eapol *eapol)
{
    struct wifi_sta *sta = (struct wifi_sta *)((uint32_t)eapol - offsetof(struct wifi_sta, w_eapol));

    return sta->ft;
}

void wifi_wpa_sta_ft_params_free(struct wpas_eapol *eapol)
{
    struct wifi_sta *sta = (struct wifi_sta *)((uint32_t)eapol - offsetof(struct wifi_sta, w_eapol));

    sta->ft = NULL;
}

void wifi_wpa_ft_auth_rsp(int vif_idx, void *ind_param)
{
    struct macif_ft_auth_ind *ind = (struct macif_ft_auth_ind *)ind_param;
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(vif_idx);

    wpa_printf("FT Auth Response: vif_idx=%d, data_len=%d, bssid=" MAC_FMT "\n",
                        vif_idx,
                        ind->auth_ie_len,
                        MAC_ARG_UINT8(wvif->sta.cfg.bssid));

    wpa_ft_process_response(&wvif->sta.w_eapol, (const uint8_t *)ind->auth_ie_buf, ind->auth_ie_len,
                0, (const uint8_t *)wvif->sta.cfg.bssid,
                NULL, 0);
}

void wifi_wpa_ft_ies_set(int vif_idx, uint8_t *ie, uint32_t ie_len)
{
    wpa_printf("FT IEs set: vif_idx=%d, ie_len=%d\n", vif_idx, ie_len);
    wpas_set_ft_ies(vif_idx, ie, ie_len);
}

int wifi_wpa_ft_reassociate(int vif_idx, uint8_t *data)
{
    struct mac_scan_result candidate = *(struct mac_scan_result *)data;
    struct wifi_vif_tag *wvif = vif_idx_to_wvif(vif_idx);
    struct sta_cfg *cfg = &wvif->sta.cfg;
    struct ft_params *ft;
    int ret, res = 0;

    cfg->ssid_len = candidate.ssid.length;
    if (cfg->ssid_len)
        sys_memcpy(cfg->ssid, candidate.ssid.array, cfg->ssid_len);
    sys_memcpy(cfg->bssid, (uint8_t *)candidate.bssid.array, MAC_ADDR_LEN);
    cfg->channel = wifi_freq_to_channel(candidate.chan->freq);

    ret = wifi_wpa_gen_wpa_or_rsn_ie(vif_idx);
    if (ret) {
        return ret;
    }

    ft = wvif->sta.ft;
    if (ft)
        wvif->sta.w_eapol.keys_cleared = ft->keys_cleared_backup;
    wpa_clear_keys(&wvif->sta.w_eapol, cfg->bssid);

    ret = wpas_set_mac_connect(vif_idx, &candidate, wvif->sta.w_eapol.assoc_wpa_ie, wvif->sta.w_eapol.assoc_wpa_ie_len, false);
    if (ret == -1) {
        res = WIFI_MGMT_CONN_UNSPECIFIED;
    } else if (ret == -2) {
        res = WIFI_MGMT_CONN_ASSOC_FAIL;
    }

    wpa_printf("start ft reassociate(res %d, ret %d)...\r\n", res, ret);

    return res;
}

int wifi_wpa_ft_reassociate_done(int vif_idx, void *ind_param)
{
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(vif_idx);
    struct macif_connect_ind *ind_info = (struct macif_connect_ind *)ind_param;
    struct wifi_sta *config_sta = &wvif->sta;
    int ret = 0;

    config_sta->ap_id = ind_info->ap_idx;
    config_sta->aid = ind_info->aid;

    // validate the reassociation response
    ret = wpa_ft_validate_reassoc_resp(&config_sta->w_eapol,
                        ((const uint8_t *)ind_info->assoc_ie_buf + ind_info->assoc_req_ie_len),
                        ind_info->assoc_rsp_ie_len,
                        (const uint8_t *)wvif->sta.cfg.bssid);

    macif_tx_sta_add(config_sta->ap_id, 0);

    wpa_key_neg_complete(&config_sta->w_eapol, (const uint8_t *)wvif->sta.cfg.bssid, 1);

    return ret;
}
#endif /* CONFIG_IEEE80211R */

#ifdef CFG_SOFTAP
int wifi_wpa_ap_sm_step(int vif_idx, uint16_t event, uint8_t *data, uint32_t data_len)
{
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(vif_idx);
    struct wifi_ap *ap;
    int ret = 0;
    uint16_t deauth_reason = WLAN_REASON_DEAUTH_LEAVING;

    if (NULL == wvif)
        return -1;

    ap = &wvif->ap;

    if (event == WIFI_MGMT_EVENT_START_AP_CMD)
        macif_vif_wpa_rx_filter_set(vif_idx, MAC_AP_MGMT_RX_FILTER);
    else if (event == WIFI_MGMT_EVENT_STOP_AP_CMD)
        macif_vif_wpa_rx_filter_set(vif_idx, MAC_STA_MGMT_RX_FILTER);

    if (ap->ap_state == WIFI_AP_STATE_INIT) {
        switch (event)
        {
            case WIFI_MGMT_EVENT_START_AP_CMD:
                ret = wpas_ap_start(vif_idx);
                break;
            default:
                goto unexpected_events;
        }
    } else if (ap->ap_state == WIFI_AP_STATE_STARTED) {
        switch (event)
        {
            case WIFI_MGMT_EVENT_RX_MGMT:
                handle_ieee802_11_mgmt(&ap->w_ap, (struct wifi_frame_info *)data, (struct ieee80211_mgmt *)(data + sizeof(struct wifi_frame_info)));
                break;
            case WIFI_MGMT_EVENT_RX_EAPOL:
                wpa_ap_rx_eapol(&ap->w_ap, (data + ETH_HLEN), (data_len - ETH_HLEN), (data + WIFI_ALEN));
                break;
            case WIFI_MGMT_EVENT_TX_MGMT_DONE:
                ap_mgmt_tx_cb_handler(&ap->w_ap, (data + 1), (data_len - 1), data);
                break;
            case WIFI_MGMT_EVENT_STOP_AP_CMD:
                if (data != NULL && data_len == 2) {
                    deauth_reason = (data[0] | (data[1] << 8));
                }
                ret = wpas_ap_stop(vif_idx, deauth_reason);
                break;
            default:
                goto unexpected_events;
        }
    } else {
        switch (event)
        {
            case WIFI_MGMT_EVENT_STOP_AP_CMD:
                ret = wpas_ap_stop(vif_idx, deauth_reason);
                break;
            default:
                goto unexpected_events;
        }
    }
    if (ret) {
        wpa_printf("WPAS AP: Vif %u state %u event %d, ret %d\r\n", vif_idx, ap->ap_state, event, ret);
    }
    return ret;

unexpected_events:
    wpa_printf("WPAS AP: Vif %u AP state %u, unsupported event received %d\r\n", vif_idx, ap->ap_state, event);
    return 0;
}
#endif  /* CFG_SOFTAP */

#endif  /* CONFIG_WPA_SUPPLICANT */
uint32_t wifi_wpa_parse_key_mgmt(char *key_mgmt_str)
{
    uint32_t val = 0;
    int last, errors = 0;
    char *start, *end;

    start = key_mgmt_str;

    while (*start != '\0') {
        while (*start == ' ' || *start == '\t')
            start++;
        if (*start == '\0')
            break;
        end = start;
        while (*end != ' ' && *end != '\t' && *end != '\0')
            end++;
        last = *end == '\0';
        *end = '\0';
        if (strcmp(start, "WPA-PSK") == 0)
            val |= (CO_BIT(MAC_AKM_PSK) | CO_BIT(MAC_AKM_PRE_RSN));
        else if (strcmp(start, "NONE") == 0)
            val |= CO_BIT(MAC_AKM_NONE);
        else if (strcmp(start, "SAE") == 0)
            val |= CO_BIT(MAC_AKM_SAE);
        else if (strcmp(start, "WPA-EAP") == 0)
            val |= CO_BIT(MAC_AKM_8021X);
        else if (strcmp(start, "WPA-EAP-SHA256") == 0)
            val |= CO_BIT(MAC_AKM_8021X_SHA256);
        else if (strcmp(start, "WPA-EAP-SUITE-B") == 0)
            val |= CO_BIT(MAC_AKM_8021X_SUITE_B);
        else if (strcmp(start, "WPA-EAP-SUITE-B-192") == 0)
            val |= CO_BIT(MAC_AKM_8021X_SUITE_B_192);
#ifdef CONFIG_OWE
        else if (strcmp(start, "OWE") == 0)
            val |= CO_BIT(MAC_AKM_OWE);
#endif /* CONFIG_OWE */
#ifdef CFG_80211R
        else if (strcmp(start, "FT-PSK") == 0)
            val |= CO_BIT(MAC_AKM_FT_PSK);
#endif /* CFG_80211R */
        else {
            errors++;
        }

        if (last)
            break;
        start = end + 1;
    }

    if (val == 0) {
        errors++;
    }

    return errors ? -1 : val;
}

uint32_t wifi_wpa_auth_mode_2_akm(uint32_t auth_mode)
{
    uint32_t wpa_akm = 0;

    switch (auth_mode) {
    case AUTH_MODE_OPEN:
        wpa_akm = CO_BIT(MAC_AKM_NONE);
        break;
    case AUTH_MODE_WEP:
        wpa_akm = CO_BIT(MAC_AKM_PRE_RSN);
        break;
    case AUTH_MODE_WPA:
        wpa_akm = CO_BIT(MAC_AKM_PSK) | CO_BIT(MAC_AKM_PRE_RSN);
        break;
    case AUTH_MODE_WPA2:
    case AUTH_MODE_WPA_WPA2:
        wpa_akm = CO_BIT(MAC_AKM_PSK);
        break;
    case AUTH_MODE_WPA2_WPA3:
        wpa_akm = CO_BIT(MAC_AKM_PSK) | CO_BIT(MAC_AKM_SAE);
        break;
    case AUTH_MODE_WPA3:
        wpa_akm = CO_BIT(MAC_AKM_SAE);
        break;
    case AUTH_MODE_UNKNOWN:
    default:
        wpa_akm = CO_BIT(MAC_AKM_NONE);
        break;
    }

    return wpa_akm;
}

int wifi_wpa_akm_name(uint32_t akm, char *buf, int len)
{
    unsigned int i;
    int written = 1;

    if (len < 2)
        return -1;
    len -= 2; // keep space for ';' and '\0'

    for (i = 0; i < ARRAY_SIZE(wpa_akm_str); i++)
    {
        if ((akm & CO_BIT(i)) && wpa_akm_str[i])
        {
            int akm_len = strlen(wpa_akm_str[i]);
            if (len < (akm_len + 1))
                return -1;
            *buf++ = ' ';
            memcpy(buf, wpa_akm_str[i], akm_len);
            buf += akm_len;
            len -= akm_len + 1;
            written += akm_len + 1;
        }
    }

    *buf++ = ';';
    *buf = 0;
    return written;
}

int wifi_wpa_cipher_name(uint32_t cipher, char *buf, int len)
{
    unsigned int i;
    int written = 1;

    if (len < 2)
        return -1;
    len -= 2; // keep space for ';' and '\0'

    for (i = 0; i < ARRAY_SIZE(wpa_cipher_str); i++)
    {
        if ((cipher & CO_BIT(i)) && wpa_cipher_str[i])
        {
            int cipher_len = strlen(wpa_cipher_str[i]);
            if (len < (cipher_len + 1))
                return -1;
            *buf++ = ' ';
            memcpy(buf, wpa_cipher_str[i], cipher_len);
            buf += cipher_len;
            len -= cipher_len + 1;
            written += cipher_len + 1;
        }
    }

    *buf++ = ';';
    *buf = 0;
    return written;
}

int wifi_wpa_send_client_add_event(int vif_idx, uint8_t *param, uint32_t param_len)
{
    return eloop_message_send(vif_idx, WIFI_MGMT_EVENT_CLIENT_ADDED, 0, param, param_len);
}

int wifi_wpa_send_client_remove_event(int vif_idx, uint8_t *param, uint32_t param_len)
{
    return eloop_message_send(vif_idx, WIFI_MGMT_EVENT_CLIENT_REMOVED, 0, param, param_len);
}

void wifi_wpa_sta_pmksa_cache_flush(int vif_idx, int flush_all)
{
#ifdef CONFIG_WPA_SUPPLICANT
    return;
#else  /* CONFIG_WPA_SUPPLICANT */
    struct wifi_vif_tag *wvif = vif_idx_to_wvif(vif_idx);

    if (NULL == wvif || wvif->wvif_type != WVIF_STA)
        return;

    if (flush_all)
        pmksa_cache_flush_all(&wvif->sta.cache);
    else
        pmksa_cache_flush(&wvif->sta.cache, NULL, SAE_PMK_LEN);
#endif  /* CONFIG_WPA_SUPPLICANT */
}

int wifi_wpa_sta_sm_step(int vif_idx, uint16_t event, uint8_t *data, uint32_t data_len, int sm)
{
#ifdef CONFIG_WPA_SUPPLICANT
    return 0;
#else  /* CONFIG_WPA_SUPPLICANT */
    if (WIFI_STA_SM_SAE == sm)
        return wifi_wpa_sta_sae_sm_step(vif_idx, event, data, data_len);
    else if (WIFI_STA_SM_EAPOL == sm)
        return wifi_wpa_sta_eapol_sm_step(vif_idx, event, data, data_len);
    return 0;
#endif  /* CONFIG_WPA_SUPPLICANT */
}
