/*!
    \file    wifi_mesh_smart.h
    \brief   Header file of wifi mesh smart.

    \version 2025-07-20, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2025, GigaDevice Semiconductor Inc.

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

#ifndef _WIFI_MESH_SMART_H_
#define _WIFI_MESH_SMART_H_

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "slist.h"
#include "mac_types.h"
#include "wifi_export.h"
#include "gd32vw55x_platform.h"
#include "ieee802_11_defs.h"

// mesh neteork config define
#define DEFAULT_MESH_SMART_SOFTAP_SSID      "GD-MeshSmartSoftAP"
#define DEFAULT_MESH_SMART_SOFTAP_PASSWORD  "12345678"

#define WIFI_MESH_SMART_VERSION             0x1
#define CONFIG_MESH_SMART_MAX_LEVEL         4

#define DEFAULT_VENDOR_IE_OUI_0             0x76
#define DEFAULT_VENDOR_IE_OUI_1             0xBA
#define DEFAULT_MESH_SMART_NETWORK_ID       0xED

// mesh feature config define
#define WIFI_MESH_SMART_SOFTAP_MAX_CLIENTS_NUMBER   (MAX_STA_NUM - 1)

typedef enum {
    // start
    MESH_SMART_EVENT_START,
    MESH_SMART_EVENT_RESTART,
    // provision
    MESH_SMART_EVENT_PROVISION_START,
    // station
    MESH_SMART_EVENT_STA_DISCONNECTED,
    MESH_SMART_EVENT_SCAN_DONE,
    MESH_SMART_EVENT_SCAN_FAIL,
    MESH_SMART_EVENT_SCAN_NO_NODE,
    MESH_SMART_EVENT_STA_START_CONNECT_NODE,
    MESH_SMART_EVENT_STA_START_CONNECT_ROOTAP,
    MESH_SMART_EVENT_STA_CONNECT_FAIL,
    MESH_SMART_EVENT_STA_CONNECT_SUCCESS,
    // softap
    MESH_SMART_EVENT_SOFTAP_CLIENT_ADD,
    MESH_SMART_EVENT_SOFTAP_CLIENT_DELETED,
    MESH_SMART_EVENT_SOFTAP_START_SUCCESS,
    MESH_SMART_EVENT_SOFTAP_START_FAIL,
    // ...
    MESH_SMART_EVENT_ROOTAP_CONFIGURED,

} mesh_smart_event_type_t;

typedef enum {
    MESH_SMART_STATUS_INIT = 0,
    MESH_SMART_STATUS_SCAN,
    MESH_SMART_STATUS_SOFTAP_PROVISIONING,
    MESH_SMART_STATUS_STA_CONNECTING,
    MESH_SMART_STATUS_STA_CONNECTED_SOFTAP_NOT_STARTED,
    MESH_SMART_STATUS_STA_CONNECTED_SOFTAP_STARTED,
    MESH_SMART_STATUS_STA_DISCONNECTED_SOFTAP_STARTED,
} mesh_smart_state;

typedef enum {
    MESH_SMART_NODE_TYPE_ROOT = 0,
    MESH_SMART_NODE_TYPE_ROUTER,
    MESH_SMART_NODE_TYPE_LEAF
} mesh_smart_node_type_t;

typedef struct wifi_mesh_smart_cfg {
    uint8_t vendor_id[2];                                       // The VID_1, VID_2 of Mesh-Lite
    uint8_t mesh_smart_network_id;                              // The ID of current Mesh-smart network
    uint8_t mesh_smart_max_clients_number;                      // Max client numer
    uint8_t mesh_smart_max_level;                               // The maximum route level
    // uint8_t force_level;                                        // Force the node to a specified level
    // uint8_t max_node_number;                                 // The maximum node number
    // bool join_mesh_ignore_router_status;                     // Join Mesh no matter whether the node is connected to router
    // bool join_mesh_without_configured_wifi;                  // Join Mesh without configured wifi information
    uint8_t mesh_role_type;                                     // Role in the mesh network
    // uint32_t ota_data_len;                                   // The maximum length of an OTA data transmission
    // uint16_t ota_wnd;                                        // OTA data transfer window size
    uint8_t softap_ip_segment;                                  // softAP ip segment
    char mesh_smart_softap_ssid[WIFI_SSID_MAX_LEN];             // SoftAP SSID
    char mesh_smart_softap_password[WPAS_MAX_PASSPHRASE_LEN];   // SoftAP Password
    uint8_t rootap_configured;                                  // Whether the root AP is configured
    char mesh_smart_rootap_ssid[WIFI_SSID_MAX_LEN];             // ROOT AP SSID, only valid for root node
    char mesh_smart_rootap_password[WPAS_MAX_PASSPHRASE_LEN];   // ROOT AP Password, only valid for root node
} wifi_mesh_smart_cfg_t;

typedef struct wifi_mesh_smart_status {
    uint8_t node_id;                        // unique ID in the whore network
    uint8_t node_status;                    // current node status
    uint8_t current_level;                  // Current level in the mesh network
    uint8_t mesh_clients_number;            // Number of clients in the mesh network
    uint8_t scan_none_count;                // count to check if scan result is empty
    uint8_t history_connect_ok;             // whether the history connected root ap is ok
    uint8_t history_level;                  // History level when connected to mesh network
    uint32_t station_connect_retry_count;
} wifi_mesh_smart_status_t;

typedef struct wifi_mesh_smart_info {
    bool mesh_smart_network_enabled;        // Flag to indicate if mesh smart is enabled
    struct wifi_mesh_smart_cfg cfg;         // Configuration for mesh smart
    wifi_mesh_smart_status_t status;        // Status of the mesh smart connection
} wifi_mesh_smart_info_t;

typedef struct wifi_mesh_smart_ap_element {
    // when changed, need to update MESH_VENDOR_IE_LEN in wlan_config.h
    uint8_t vendor_id[2];                   // First two bytes of OUI for mesh smart(default: 76:ba)
    uint8_t mesh_smart_network_id;          // Mesh smart network ID, third byte of OUI
    uint8_t oui_type;                       // Type of OUI, also version of mesh
    uint8_t node_id;                        // unique ID in the whore network
    uint8_t node_type;                      // Type of node
    uint8_t node_status;                    // Status of node
    uint8_t ap_max_clients;                 // Maximum clients supported by the AP
    uint8_t ap_current_clients;             // Current clients connected to the softAP
    uint8_t mesh_current_level;             // Current level in the mesh network
    // uint8_t root_ap_ssid_len;
    // char    root_ap_ssid[0];                // SSID of the root AP, if connected
} wifi_mesh_smart_ap_element_t;

typedef struct mesh_scan_result {
    struct list_hdr list_hdr;
    uint8_t ssid[WIFI_SSID_MAX_LEN];
    uint8_t bssid[WIFI_ALEN];
    int8_t rssi;
    // uint16_t channel;
    // uint8_t security;
    struct wifi_mesh_smart_ap_element mesh_smart_ap_ele;
    uint8_t root_ap_ssid[WIFI_SSID_MAX_LEN];
} mesh_scan_result_t;

int wifi_mesh_smart_task_start(void);
void wifi_mesh_smart_network_init(void);
int wifi_mesh_smart_network_config(wifi_mesh_smart_cfg_t *mesh_smart_cfg, uint8_t use_stored_cfg);
int wifi_mesh_smart_config_rootap_info(char *ssid, char *password);

// api for test
void wifi_mesh_smart_status_print(void);
int wifi_mesh_smart_softap_stop(void);

#endif /* _WIFI_MESH_SMART_H_ */
