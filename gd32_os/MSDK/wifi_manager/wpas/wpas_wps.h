/*
 * Wi-Fi Protected Setup
 * Copyright (c) 2007-2009, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

/*!
    \file    wpas_wps.h
    \brief   Header file for wpas wps.

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
#ifndef WPAS_WPS_H
#define WPAS_WPS_H

#include "build_config.h"

/*********************************************************/
/*              wps definitions start                    */
/*********************************************************/
#define WPS_VERSION 0x20

/* Diffie-Hellman 1536-bit MODP Group; RFC 3526, Group 5 */
#define WPS_DH_GROUP 5

#define WPS_UUID_LEN 16
#define WPS_NONCE_LEN 16
#define WPS_AUTHENTICATOR_LEN 8
#define WPS_AUTHKEY_LEN 32
#define WPS_KEYWRAPKEY_LEN 16
#define WPS_EMSK_LEN 32
#define WPS_PSK_LEN 16
#define WPS_SECRET_NONCE_LEN 16
#define WPS_HASH_LEN 32
#define WPS_KWA_LEN 8
#define WPS_MGMTAUTHKEY_LEN 32
#define WPS_MGMTENCKEY_LEN 16
#define WPS_MGMT_KEY_ID_LEN 16
#define WPS_OOB_DEVICE_PASSWORD_MIN_LEN 16
#define WPS_OOB_DEVICE_PASSWORD_LEN 32
#define WPS_OOB_PUBKEY_HASH_LEN 20
#define WPS_DEV_NAME_MAX_LEN 32
#define WPS_MANUFACTURER_MAX_LEN 64
#define WPS_MODEL_NAME_MAX_LEN 32
#define WPS_MODEL_NUMBER_MAX_LEN 32
#define WPS_SERIAL_NUMBER_MAX_LEN 32

/* Attribute Types */
enum wps_attribute {
    ATTR_AP_CHANNEL = 0x1001,
    ATTR_ASSOC_STATE = 0x1002,
    ATTR_AUTH_TYPE = 0x1003,
    ATTR_AUTH_TYPE_FLAGS = 0x1004,
    ATTR_AUTHENTICATOR = 0x1005,
    ATTR_CONFIG_METHODS = 0x1008,
    ATTR_CONFIG_ERROR = 0x1009,
    ATTR_CONFIRM_URL4 = 0x100a,
    ATTR_CONFIRM_URL6 = 0x100b,
    ATTR_CONN_TYPE = 0x100c,
    ATTR_CONN_TYPE_FLAGS = 0x100d,
    ATTR_CRED = 0x100e,
    ATTR_ENCR_TYPE = 0x100f,
    ATTR_ENCR_TYPE_FLAGS = 0x1010,
    ATTR_DEV_NAME = 0x1011,
    ATTR_DEV_PASSWORD_ID = 0x1012,
    ATTR_E_HASH1 = 0x1014,
    ATTR_E_HASH2 = 0x1015,
    ATTR_E_SNONCE1 = 0x1016,
    ATTR_E_SNONCE2 = 0x1017,
    ATTR_ENCR_SETTINGS = 0x1018,
    ATTR_ENROLLEE_NONCE = 0x101a,
    ATTR_FEATURE_ID = 0x101b,
    ATTR_IDENTITY = 0x101c,
    ATTR_IDENTITY_PROOF = 0x101d,
    ATTR_KEY_WRAP_AUTH = 0x101e,
    ATTR_KEY_ID = 0x101f,
    ATTR_MAC_ADDR = 0x1020,
    ATTR_MANUFACTURER = 0x1021,
    ATTR_MSG_TYPE = 0x1022,
    ATTR_MODEL_NAME = 0x1023,
    ATTR_MODEL_NUMBER = 0x1024,
    ATTR_NETWORK_INDEX = 0x1026,
    ATTR_NETWORK_KEY = 0x1027,
    ATTR_NETWORK_KEY_INDEX = 0x1028,
    ATTR_NEW_DEVICE_NAME = 0x1029,
    ATTR_NEW_PASSWORD = 0x102a,
    ATTR_OOB_DEVICE_PASSWORD = 0x102c,
    ATTR_OS_VERSION = 0x102d,
    ATTR_POWER_LEVEL = 0x102f,
    ATTR_PSK_CURRENT = 0x1030,
    ATTR_PSK_MAX = 0x1031,
    ATTR_PUBLIC_KEY = 0x1032,
    ATTR_RADIO_ENABLE = 0x1033,
    ATTR_REBOOT = 0x1034,
    ATTR_REGISTRAR_CURRENT = 0x1035,
    ATTR_REGISTRAR_ESTABLISHED = 0x1036,
    ATTR_REGISTRAR_LIST = 0x1037,
    ATTR_REGISTRAR_MAX = 0x1038,
    ATTR_REGISTRAR_NONCE = 0x1039,
    ATTR_REQUEST_TYPE = 0x103a,
    ATTR_RESPONSE_TYPE = 0x103b,
    ATTR_RF_BANDS = 0x103c,
    ATTR_R_HASH1 = 0x103d,
    ATTR_R_HASH2 = 0x103e,
    ATTR_R_SNONCE1 = 0x103f,
    ATTR_R_SNONCE2 = 0x1040,
    ATTR_SELECTED_REGISTRAR = 0x1041,
    ATTR_SERIAL_NUMBER = 0x1042,
    ATTR_WPS_STATE = 0x1044,
    ATTR_SSID = 0x1045,
    ATTR_TOTAL_NETWORKS = 0x1046,
    ATTR_UUID_E = 0x1047,
    ATTR_UUID_R = 0x1048,
    ATTR_VENDOR_EXT = 0x1049,
    ATTR_VERSION = 0x104a,
    ATTR_X509_CERT_REQ = 0x104b,
    ATTR_X509_CERT = 0x104c,
    ATTR_EAP_IDENTITY = 0x104d,
    ATTR_MSG_COUNTER = 0x104e,
    ATTR_PUBKEY_HASH = 0x104f,
    ATTR_REKEY_KEY = 0x1050,
    ATTR_KEY_LIFETIME = 0x1051,
    ATTR_PERMITTED_CFG_METHODS = 0x1052,
    ATTR_SELECTED_REGISTRAR_CONFIG_METHODS = 0x1053,
    ATTR_PRIMARY_DEV_TYPE = 0x1054,
    ATTR_SECONDARY_DEV_TYPE_LIST = 0x1055,
    ATTR_PORTABLE_DEV = 0x1056,
    ATTR_AP_SETUP_LOCKED = 0x1057,
    ATTR_APPLICATION_EXT = 0x1058,
    ATTR_EAP_TYPE = 0x1059,
    ATTR_IV = 0x1060,
    ATTR_KEY_PROVIDED_AUTO = 0x1061,
    ATTR_802_1X_ENABLED = 0x1062,
    ATTR_APPSESSIONKEY = 0x1063,
    ATTR_WEPTRANSMITKEY = 0x1064,
    ATTR_REQUESTED_DEV_TYPE = 0x106a,
    ATTR_EXTENSIBILITY_TEST = 0x10fa /* _NOT_ defined in the spec */
};

#define WPS_VENDOR_ID_WFA 14122

/* WFA Vendor Extension subelements */
enum {
    WFA_ELEM_VERSION2 = 0x00,
    WFA_ELEM_AUTHORIZEDMACS = 0x01,
    WFA_ELEM_NETWORK_KEY_SHAREABLE = 0x02,
    WFA_ELEM_REQUEST_TO_ENROLL = 0x03,
    WFA_ELEM_SETTINGS_DELAY_TIME = 0x04,
    WFA_ELEM_REGISTRAR_CONFIGURATION_METHODS = 0x05,
    WFA_ELEM_MULTI_AP = 0x06
};

/* Device Password ID */
enum wps_dev_password_id {
    DEV_PW_DEFAULT = 0x0000,
    DEV_PW_USER_SPECIFIED = 0x0001,
    DEV_PW_MACHINE_SPECIFIED = 0x0002,
    DEV_PW_REKEY = 0x0003,
    DEV_PW_PUSHBUTTON = 0x0004,
    DEV_PW_REGISTRAR_SPECIFIED = 0x0005,
    DEV_PW_NFC_CONNECTION_HANDOVER = 0x0007,
    DEV_PW_P2PS_DEFAULT = 0x0008
};

/* Message Type */
enum wps_msg_type {
    WPS_Beacon = 0x01,
    WPS_ProbeRequest = 0x02,
    WPS_ProbeResponse = 0x03,
    WPS_M1 = 0x04,
    WPS_M2 = 0x05,
    WPS_M2D = 0x06,
    WPS_M3 = 0x07,
    WPS_M4 = 0x08,
    WPS_M5 = 0x09,
    WPS_M6 = 0x0a,
    WPS_M7 = 0x0b,
    WPS_M8 = 0x0c,
    WPS_WSC_ACK = 0x0d,
    WPS_WSC_NACK = 0x0e,
    WPS_WSC_DONE = 0x0f
};

/* Authentication Type Flags */
#define WPS_AUTH_OPEN 0x0001
#define WPS_AUTH_WPAPSK 0x0002
#define WPS_AUTH_SHARED 0x0004 /* deprecated */
#define WPS_AUTH_WPA 0x0008
#define WPS_AUTH_WPA2 0x0010
#define WPS_AUTH_WPA2PSK 0x0020
#define WPS_AUTH_TYPES (WPS_AUTH_OPEN | WPS_AUTH_WPAPSK | WPS_AUTH_SHARED | \
            WPS_AUTH_WPA | WPS_AUTH_WPA2 | WPS_AUTH_WPA2PSK)

/* Encryption Type Flags */
#define WPS_ENCR_NONE 0x0001
#define WPS_ENCR_WEP 0x0002 /* deprecated */
#define WPS_ENCR_TKIP 0x0004
#define WPS_ENCR_AES 0x0008
#define WPS_ENCR_TYPES (WPS_ENCR_NONE | WPS_ENCR_WEP | WPS_ENCR_TKIP | \
            WPS_ENCR_AES)

/* Configuration Error */
enum wps_config_error {
    WPS_CFG_NO_ERROR = 0,
    WPS_CFG_OOB_IFACE_READ_ERROR = 1,
    WPS_CFG_DECRYPTION_CRC_FAILURE = 2,
    WPS_CFG_24_CHAN_NOT_SUPPORTED = 3,
    WPS_CFG_50_CHAN_NOT_SUPPORTED = 4,
    WPS_CFG_SIGNAL_TOO_WEAK = 5,
    WPS_CFG_NETWORK_AUTH_FAILURE = 6,
    WPS_CFG_NETWORK_ASSOC_FAILURE = 7,
    WPS_CFG_NO_DHCP_RESPONSE = 8,
    WPS_CFG_FAILED_DHCP_CONFIG = 9,
    WPS_CFG_IP_ADDR_CONFLICT = 10,
    WPS_CFG_NO_CONN_TO_REGISTRAR = 11,
    WPS_CFG_MULTIPLE_PBC_DETECTED = 12,
    WPS_CFG_ROGUE_SUSPECTED = 13,
    WPS_CFG_DEVICE_BUSY = 14,
    WPS_CFG_SETUP_LOCKED = 15,
    WPS_CFG_MSG_TIMEOUT = 16,
    WPS_CFG_REG_SESS_TIMEOUT = 17,
    WPS_CFG_DEV_PASSWORD_AUTH_FAILURE = 18,
    WPS_CFG_60G_CHAN_NOT_SUPPORTED = 19,
    WPS_CFG_PUBLIC_KEY_HASH_MISMATCH = 20
};

/* Vendor specific Error Indication for WPS event messages */
enum wps_error_indication {
    WPS_EI_NO_ERROR,
    WPS_EI_SECURITY_TKIP_ONLY_PROHIBITED,
    WPS_EI_SECURITY_WEP_PROHIBITED,
    WPS_EI_AUTH_FAILURE,
    NUM_WPS_EI_VALUES
};

/* RF Bands */
#define WPS_RF_24GHZ 0x01
#define WPS_RF_50GHZ 0x02
#define WPS_RF_60GHZ 0x04

/* Config Methods */
#define WPS_CONFIG_USBA 0x0001
#define WPS_CONFIG_ETHERNET 0x0002
#define WPS_CONFIG_LABEL 0x0004
#define WPS_CONFIG_DISPLAY 0x0008
#define WPS_CONFIG_EXT_NFC_TOKEN 0x0010
#define WPS_CONFIG_INT_NFC_TOKEN 0x0020
#define WPS_CONFIG_NFC_INTERFACE 0x0040
#define WPS_CONFIG_PUSHBUTTON 0x0080
#define WPS_CONFIG_KEYPAD 0x0100
#define WPS_CONFIG_VIRT_PUSHBUTTON 0x0280
#define WPS_CONFIG_PHY_PUSHBUTTON 0x0480
#define WPS_CONFIG_P2PS 0x1000
#define WPS_CONFIG_VIRT_DISPLAY 0x2008
#define WPS_CONFIG_PHY_DISPLAY 0x4008

/* Connection Type Flags */
#define WPS_CONN_ESS 0x01
#define WPS_CONN_IBSS 0x02

/* Wi-Fi Protected Setup State */
enum wps_state {
    WPS_STATE_NOT_CONFIGURED = 1,
    WPS_STATE_CONFIGURED = 2
};

/* Association State */
enum wps_assoc_state {
    WPS_ASSOC_NOT_ASSOC = 0,
    WPS_ASSOC_CONN_SUCCESS = 1,
    WPS_ASSOC_CFG_FAILURE = 2,
    WPS_ASSOC_FAILURE = 3,
    WPS_ASSOC_IP_FAILURE = 4
};


#define WPS_DEV_OUI_WFA 0x0050f204

enum wps_dev_categ {
    WPS_DEV_COMPUTER = 1,
    WPS_DEV_INPUT = 2,
    WPS_DEV_PRINTER = 3,
    WPS_DEV_CAMERA = 4,
    WPS_DEV_STORAGE = 5,
    WPS_DEV_NETWORK_INFRA = 6,
    WPS_DEV_DISPLAY = 7,
    WPS_DEV_MULTIMEDIA = 8,
    WPS_DEV_GAMING = 9,
    WPS_DEV_PHONE = 10,
    WPS_DEV_AUDIO = 11,
};

enum wps_dev_subcateg {
    WPS_DEV_COMPUTER_PC = 1,
    WPS_DEV_COMPUTER_SERVER = 2,
    WPS_DEV_COMPUTER_MEDIA_CENTER = 3,
    WPS_DEV_COMPUTER_ULTRA_MOBILE = 4,
    WPS_DEV_COMPUTER_NOTEBOOK = 5,
    WPS_DEV_COMPUTER_DESKTOP = 6,
    WPS_DEV_COMPUTER_MID = 7,
    WPS_DEV_COMPUTER_NETBOOK = 8,
    WPS_DEV_COMPUTER_TABLET = 9,
    WPS_DEV_INPUT_KEYBOARD = 1,
    WPS_DEV_INPUT_MOUSE = 2,
    WPS_DEV_INPUT_JOYSTICK = 3,
    WPS_DEV_INPUT_TRACKBALL = 4,
    WPS_DEV_INPUT_GAMING = 5,
    WPS_DEV_INPUT_REMOTE = 6,
    WPS_DEV_INPUT_TOUCHSCREEN = 7,
    WPS_DEV_INPUT_BIOMETRIC_READER = 8,
    WPS_DEV_INPUT_BARCODE_READER = 9,
    WPS_DEV_PRINTER_PRINTER = 1,
    WPS_DEV_PRINTER_SCANNER = 2,
    WPS_DEV_PRINTER_FAX = 3,
    WPS_DEV_PRINTER_COPIER = 4,
    WPS_DEV_PRINTER_ALL_IN_ONE = 5,
    WPS_DEV_CAMERA_DIGITAL_STILL_CAMERA = 1,
    WPS_DEV_CAMERA_VIDEO = 2,
    WPS_DEV_CAMERA_WEB = 3,
    WPS_DEV_CAMERA_SECURITY = 4,
    WPS_DEV_STORAGE_NAS = 1,
    WPS_DEV_NETWORK_INFRA_AP = 1,
    WPS_DEV_NETWORK_INFRA_ROUTER = 2,
    WPS_DEV_NETWORK_INFRA_SWITCH = 3,
    WPS_DEV_NETWORK_INFRA_GATEWAY = 4,
    WPS_DEV_NETWORK_INFRA_BRIDGE = 5,
    WPS_DEV_DISPLAY_TV = 1,
    WPS_DEV_DISPLAY_PICTURE_FRAME = 2,
    WPS_DEV_DISPLAY_PROJECTOR = 3,
    WPS_DEV_DISPLAY_MONITOR = 4,
    WPS_DEV_MULTIMEDIA_DAR = 1,
    WPS_DEV_MULTIMEDIA_PVR = 2,
    WPS_DEV_MULTIMEDIA_MCX = 3,
    WPS_DEV_MULTIMEDIA_SET_TOP_BOX = 4,
    WPS_DEV_MULTIMEDIA_MEDIA_SERVER = 5,
    WPS_DEV_MULTIMEDIA_PORTABLE_VIDEO_PLAYER = 6,
    WPS_DEV_GAMING_XBOX = 1,
    WPS_DEV_GAMING_XBOX360 = 2,
    WPS_DEV_GAMING_PLAYSTATION = 3,
    WPS_DEV_GAMING_GAME_CONSOLE = 4,
    WPS_DEV_GAMING_PORTABLE_DEVICE = 5,
    WPS_DEV_PHONE_WINDOWS_MOBILE = 1,
    WPS_DEV_PHONE_SINGLE_MODE = 2,
    WPS_DEV_PHONE_DUAL_MODE = 3,
    WPS_DEV_PHONE_SP_SINGLE_MODE = 4,
    WPS_DEV_PHONE_SP_DUAL_MODE = 5,
    WPS_DEV_AUDIO_TUNER_RECV = 1,
    WPS_DEV_AUDIO_SPEAKERS = 2,
    WPS_DEV_AUDIO_PMP = 3,
    WPS_DEV_AUDIO_HEADSET = 4,
    WPS_DEV_AUDIO_HEADPHONES = 5,
    WPS_DEV_AUDIO_MICROPHONE = 6,
    WPS_DEV_AUDIO_HOME_THEATRE = 7,
};


/* Request Type */
enum wps_request_type {
    WPS_REQ_ENROLLEE_INFO = 0,
    WPS_REQ_ENROLLEE = 1,
    WPS_REQ_REGISTRAR = 2,
    WPS_REQ_WLAN_MANAGER_REGISTRAR = 3
};

/* Response Type */
enum wps_response_type {
    WPS_RESP_ENROLLEE_INFO = 0,
    WPS_RESP_ENROLLEE = 1,
    WPS_RESP_REGISTRAR = 2,
    WPS_RESP_AP = 3
};

/* Walk Time for push button configuration (in seconds) */
#define WPS_PBC_WALK_TIME 120

#define WPS_MAX_AUTHORIZED_MACS 5

/*********************************************************/
/*              wps definitions end                      */
/*********************************************************/

/**
 * enum wsc_op_code - EAP-WSC OP-Code values
 */
enum wsc_op_code {
    WSC_UPnP = 0 /* No OP Code in UPnP transport */,
    WSC_Start = 0x01,
    WSC_ACK = 0x02,
    WSC_NACK = 0x03,
    WSC_MSG = 0x04,
    WSC_Done = 0x05,
    WSC_FRAG_ACK = 0x06
};


/**
 * struct wps_credential - WPS Credential
 * @ssid: SSID
 * @ssid_len: Length of SSID
 * @auth_type: Authentication Type (WPS_AUTH_OPEN, .. flags)
 * @encr_type: Encryption Type (WPS_ENCR_NONE, .. flags)
 * @key_idx: Key index
 * @key: Key
 * @key_len: Key length in octets
 * @mac_addr: MAC address of the Credential receiver
 * @cred_attr: Unparsed Credential attribute data (used only in cred_cb());
 *    this may be %NULL, if not used
 * @cred_attr_len: Length of cred_attr in octets
 */
struct wps_credential {
    uint8_t ssid[WIFI_SSID_MAX_LEN];
    size_t ssid_len;
    uint16_t auth_type;
    uint16_t encr_type;
    uint8_t key_idx;
    uint8_t key[64];
    size_t key_len;
    uint8_t mac_addr[ETH_ALEN];
    const uint8_t *cred_attr;
    size_t cred_attr_len;
};

#define WPS_DEV_TYPE_LEN 8
#define WPS_DEV_TYPE_BUFSIZE 21
#define WPS_SEC_DEV_TYPE_MAX_LEN 128
/* maximum number of advertised WPS vendor extension attributes */
#define MAX_WPS_VENDOR_EXTENSIONS 10
/* maximum size of WPS Vendor extension attribute */
#define WPS_MAX_VENDOR_EXT_LEN 1024
/* maximum number of parsed WPS vendor extension attributes */
#define MAX_WPS_PARSE_VENDOR_EXT 10

/**
 * struct wps_device_data - WPS Device Data
 * @mac_addr: Device MAC address
 * @device_name: Device Name (0..32 octets encoded in UTF-8)
 * @manufacturer: Manufacturer (0..64 octets encoded in UTF-8)
 * @model_name: Model Name (0..32 octets encoded in UTF-8)
 * @model_number: Model Number (0..32 octets encoded in UTF-8)
 * @serial_number: Serial Number (0..32 octets encoded in UTF-8)
 * @pri_dev_type: Primary Device Type
 * @sec_dev_type: Array of secondary device types
 * @num_sec_dev_type: Number of secondary device types
 * @os_version: OS Version
 * @rf_bands: RF bands (WPS_RF_24GHZ, WPS_RF_50GHZ, WPS_RF_60GHZ flags)
 * @p2p: Whether the device is a P2P device
 */
struct wps_device_data {
    uint8_t mac_addr[ETH_ALEN];
    char *device_name;
    char *manufacturer;
    char *model_name;
    char *model_number;
    char *serial_number;
    uint8_t pri_dev_type[WPS_DEV_TYPE_LEN];
#define WPS_SEC_DEVICE_TYPES 5
    uint8_t sec_dev_type[WPS_SEC_DEVICE_TYPES][WPS_DEV_TYPE_LEN];
    uint8_t num_sec_dev_types;
    uint32_t os_version;
    uint8_t rf_bands;
    uint16_t config_methods;
//    struct wpabuf *vendor_ext_m1;
    struct wpabuf *vendor_ext[MAX_WPS_VENDOR_EXTENSIONS];
    struct wpabuf *application_ext;

    int p2p;
    uint8_t multi_ap_ext;
};

struct wps_registrar;

/**
 * struct wps_context - Long term WPS context data
 *
 * This data is stored at the higher layer Authenticator or Supplicant data
 * structures and it is maintained over multiple registration protocol runs.
 */
struct wps_context {
    /**
     * is_ap - Whether the local end is an access point
     */
    uint8_t is_ap;

    /**
     * is_registrar - Whether the local end work as registrar
     */
    uint8_t is_registrar;

    /**
     * is_pbc - Whether using PBC
     */
    uint8_t is_pbc;

    /**
     * pin - PIN code
     */
    uint8_t pin[9];

    /**
     * pin - PIN start time (sys tick)
     */
    uint32_t wps_pin_start_time;

    /**
     * uuid - Own UUID
     */
    uint8_t uuid[16];

    /**
     * dev - Own WPS device data
     */
    struct wps_device_data dev;

    /**
     * dh_ctx - Context data for Diffie-Hellman operation
     */
    void *dh_ctx;

    /**
     * dh_privkey - Diffie-Hellman private key
     */
    struct wpabuf *dh_privkey;

    /**
     * dh_pubkey_oob - Diffie-Hellman public key
     */
    struct wpabuf *dh_pubkey;

    /**
     * identity - EAP Identity
     *
     * This field is used to set the real user identity or NAI (for
     * EAP-PSK/PAX/SAKE/GPSK).
     */
    uint8_t *identity;

    /**
     * identity_len - EAP Identity length
     */
    size_t identity_len;

    /**
     * probe_req_extra_ie - extra WPS IE added in probe request frame
     */
    uint8_t *probe_req_extra_ie;
    uint32_t probe_req_extra_ie_len;

#ifdef CONFIG_WPS_AP
    /**
     * ssid - SSID
     *
     * This SSID is used by the Registrar to fill in information for
     * Credentials. In addition, AP uses it when acting as an Enrollee to
     * notify Registrar of the current configuration.
     */
    uint8_t ssid[WIFI_SSID_MAX_LEN];

    /**
     * ssid_len - Length of ssid in octets
     */
    size_t ssid_len;

    /**
     * config_methods - Enabled configuration methods
     *
     * Bit field of WPS_CONFIG_*
     */
    uint16_t config_methods;
    /**
     * registrar - Pointer to WPS registrar data from wps_registrar_init()
     */
    struct wps_registrar *registrar;
    /**
     * ap_setup_locked - Whether AP setup is locked (only used at AP)
     */
    int ap_setup_locked;
    /**
     * wps_state - Current WPS state
     */
    enum wps_state wps_state;
    /**
     * encr_types - Enabled encryption types (bit field of WPS_ENCR_*)
     */
    uint16_t encr_types;

    /**
     * encr_types_rsn - Enabled encryption types for RSN (WPS_ENCR_*)
     */
    uint16_t encr_types_rsn;

    /**
     * encr_types_wpa - Enabled encryption types for WPA (WPS_ENCR_*)
     */
    uint16_t encr_types_wpa;
    /**
     * auth_types - Authentication types (bit field of WPS_AUTH_*)
     */
    uint16_t auth_types;

    /**
     * encr_types - Current AP encryption type (WPS_ENCR_*)
     */
    uint16_t ap_encr_type;

    /**
     * ap_auth_type - Current AP authentication types (WPS_AUTH_*)
     */
    uint16_t ap_auth_type;

    /**
     * network_key - The current Network Key (PSK) or %NULL to generate new
     *
     * If %NULL, Registrar will generate per-device PSK. In addition, AP
     * uses this when acting as an Enrollee to notify Registrar of the
     * current configuration.
     *
     * When using WPA/WPA2-Personal, this key can be either the ASCII
     * passphrase (8..63 characters) or the 32-octet PSK (64 hex
     * characters). When this is set to the ASCII passphrase, the PSK can
     * be provided in the psk buffer and used per-Enrollee to control which
     * key type is included in the Credential (e.g., to reduce calculation
     * need on low-powered devices by provisioning PSK while still allowing
     * other devices to get the passphrase).
     */
    uint8_t *network_key;

    /**
     * network_key_len - Length of network_key in octets
     */
    size_t network_key_len;

    /**
     * psk - The current network PSK
     *
     * This optional value can be used to provide the current PSK if
     * network_key is set to the ASCII passphrase.
     */
    uint8_t psk[32];

    /**
     * psk_set - Whether psk value is set
     */
    int psk_set;

    /**
     * ap_settings - AP Settings override for M7 (only used at AP)
     *
     * If %NULL, AP Settings attributes will be generated based on the
     * current network configuration.
     */
    uint8_t *ap_settings;

    /**
     * ap_settings_len - Length of ap_settings in octets
     */
    size_t ap_settings_len;

    /**
     * friendly_name - Friendly Name (required for UPnP)
     */
    char *friendly_name;

    /**
     * manufacturer_url - Manufacturer URL (optional for UPnP)
     */
    char *manufacturer_url;

    /**
     * model_description - Model Description (recommended for UPnP)
     */
    char *model_description;

    /**
     * model_url - Model URL (optional for UPnP)
     */
    char *model_url;

    /**
     * upc - Universal Product Code (optional for UPnP)
     */
    char *upc;

    /* Whether to send WPA2-PSK passphrase as a passphrase instead of PSK
     * for WPA3-Personal transition mode needs. */
    bool use_passphrase;
#endif
};


enum wps_data_state {
    /* Enrollee states */
    SEND_M1, RECV_M2, SEND_M3, RECV_M4, SEND_M5, RECV_M6, SEND_M7,
    RECV_M8, RECEIVED_M2D, WPS_MSG_DONE, RECV_ACK, WPS_FINISHED,
    SEND_WSC_NACK,

    /* Registrar states */
    RECV_M1, SEND_M2, RECV_M3, SEND_M4, RECV_M5, SEND_M6,
    RECV_M7, SEND_M8, RECV_DONE, SEND_M2D, RECV_M2D_ACK
};
/**
 * struct wps_data - WPS registration protocol data
 *
 * This data is stored at the EAP-WSC server/peer method and it is kept for a
 * single registration protocol run.
 */
struct wps_data {
    /**
     * wps - Pointer to long term WPS context
     */
    struct wps_context *wps;

    /**
     * vif_idx - index of the wifi virtual if
     */
    int vif_idx;

    /**
     * registrar - Pointer to WPS registrar data from wps_registrar_init()
     */
    struct wps_registrar *registrar;
    /**
     * er - Whether the local end is an external registrar
     */
    int er;

    enum wps_data_state state;

    uint8_t uuid_e[WPS_UUID_LEN];
    uint8_t uuid_r[WPS_UUID_LEN];
    uint8_t mac_addr_e[ETH_ALEN];
    uint8_t nonce_e[WPS_NONCE_LEN];
    uint8_t nonce_r[WPS_NONCE_LEN];
    uint8_t psk1[WPS_PSK_LEN];
    uint8_t psk2[WPS_PSK_LEN];
    uint8_t snonce[2 * WPS_SECRET_NONCE_LEN];
    uint8_t peer_hash1[WPS_HASH_LEN];
    uint8_t peer_hash2[WPS_HASH_LEN];

    struct wpabuf *dh_privkey;
    struct wpabuf *dh_pubkey_e;
    struct wpabuf *dh_pubkey_r;
    uint8_t authkey[WPS_AUTHKEY_LEN];
    uint8_t keywrapkey[WPS_KEYWRAPKEY_LEN];
    uint8_t emsk[WPS_EMSK_LEN];

    struct wpabuf *last_msg;

    uint8_t *dev_password;
    size_t dev_password_len;
    uint16_t dev_pw_id;
    int pbc;

#ifdef CONFIG_WPS_AP
    uint8_t *alt_dev_password;
    size_t alt_dev_password_len;
    uint16_t alt_dev_pw_id;

    int8_t peer_pubkey_hash[WPS_OOB_PUBKEY_HASH_LEN];
    int peer_pubkey_hash_set;

    /**
     * request_type - Request Type attribute from (Re)AssocReq
     */
    uint8_t request_type;

    /**
     * encr_type - Available encryption types
     */
    uint16_t encr_type;

    /**
     * auth_type - Available authentication types
     */
    uint16_t auth_type;

    uint8_t *new_psk;
    size_t new_psk_len;

    int wps_pin_revealed;
#endif
    struct wps_credential cred;

    struct wps_device_data dev;

    struct wps_device_data peer_dev;

    /**
     * config_error - Configuration Error value to be used in NACK
     */
    uint16_t config_error;
    uint16_t error_indication;

#ifdef CONFIG_WPS_AP
    int ext_reg;
    int int_reg;

    struct wps_credential *new_ap_settings;
#endif
    void *dh_ctx;

#ifdef CONFIG_WPS_AP
    void (*ap_settings_cb)(void *ctx, const struct wps_credential *cred);
    void *ap_settings_cb_ctx;

    struct wps_credential *use_cred;

    int use_psk_key;
    uint8_t p2p_dev_addr[ETH_ALEN]; /* P2P Device Address of the client or
                   * 00:00:00:00:00:00 if not a P2p client */
    int pbc_in_m1;

    int multi_ap_backhaul_sta;
#endif
};

/**
 * enum wps_process_res - WPS message processing result
 */
enum wps_process_res {
    /**
     * WPS_DONE - Processing done
     */
    WPS_DONE,

    /**
     * WPS_CONTINUE - Processing continues
     */
    WPS_CONTINUE,

    /**
     * WPS_FAILURE - Processing failed
     */
    WPS_FAILURE,

    /**
     * WPS_PENDING - Processing continues, but waiting for an external
     *    event (e.g., UPnP message from an external Registrar)
     */
    WPS_PENDING
};

char * wps_dev_type_bin2str(const uint8_t dev_type[WPS_DEV_TYPE_LEN], char *buf, size_t buf_len);
void uuid_gen_by_mac_addr(const uint8_t *mac_addr, uint8_t *uuid);
struct wpabuf * wps_build_assoc_req_ie(enum wps_request_type req_type);

/* wps_common.c */
void wps_kdf(const uint8_t *key, const uint8_t *label_prefix, size_t label_prefix_len,
         const char *label, uint8_t *res, size_t res_len);
int wps_derive_keys(struct wps_data *wps);
int wps_derive_psk(struct wps_data *wps, const uint8_t *dev_passwd,
           size_t dev_passwd_len);
struct wpabuf * wps_decrypt_encr_settings(struct wps_data *wps, const uint8_t *encr,
                      size_t encr_len);
void wps_fail_event(struct wps_data *wps, enum wps_msg_type msg,
            uint16_t config_error, uint16_t error_indication, const uint8_t *mac_addr);
void wps_success_event(struct wps_data *wps, const uint8_t *mac_addr);
void wps_pwd_auth_fail_event(struct wps_data *wps, int enrollee, int part,
                 const uint8_t *mac_addr);
void wps_pbc_overlap_event(struct wps_data *wps);
void wps_pbc_timeout_event(struct wps_data *wps);
void wps_pbc_active_event(struct wps_data *wps);
void wps_pbc_disable_event(struct wps_data *wps);

struct wpabuf * wps_build_wsc_ack(struct wps_data *wps);
struct wpabuf * wps_build_wsc_nack(struct wps_data *wps);

#ifdef CONFIG_WPS_STRICT
int wps_validate_beacon(const struct wpabuf *wps_ie);
int wps_validate_beacon_probe_resp(const struct wpabuf *wps_ie, int probe,
                   const uint8_t *addr);
int wps_validate_probe_req(const struct wpabuf *wps_ie, const uint8_t *addr);
int wps_validate_assoc_req(const struct wpabuf *wps_ie);
int wps_validate_assoc_resp(const struct wpabuf *wps_ie);
int wps_validate_m1(const struct wpabuf *tlvs);
int wps_validate_m2(const struct wpabuf *tlvs);
int wps_validate_m2d(const struct wpabuf *tlvs);
int wps_validate_m3(const struct wpabuf *tlvs);
int wps_validate_m4(const struct wpabuf *tlvs);
int wps_validate_m4_encr(const struct wpabuf *tlvs, int wps2);
int wps_validate_m5(const struct wpabuf *tlvs);
int wps_validate_m5_encr(const struct wpabuf *tlvs, int wps2);
int wps_validate_m6(const struct wpabuf *tlvs);
int wps_validate_m6_encr(const struct wpabuf *tlvs, int wps2);
int wps_validate_m7(const struct wpabuf *tlvs);
int wps_validate_m7_encr(const struct wpabuf *tlvs, int ap, int wps2);
int wps_validate_m8(const struct wpabuf *tlvs);
int wps_validate_m8_encr(const struct wpabuf *tlvs, int ap, int wps2);
int wps_validate_wsc_ack(const struct wpabuf *tlvs);
int wps_validate_wsc_nack(const struct wpabuf *tlvs);
int wps_validate_wsc_done(const struct wpabuf *tlvs);
int wps_validate_upnp_set_selected_registrar(const struct wpabuf *tlvs);
#else /* CONFIG_WPS_STRICT */
static inline int wps_validate_beacon(const struct wpabuf *wps_ie){
    return 0;
}

static inline int wps_validate_beacon_probe_resp(const struct wpabuf *wps_ie,
                         int probe, const uint8_t *addr)
{
    return 0;
}

static inline int wps_validate_probe_req(const struct wpabuf *wps_ie,
                     const uint8_t *addr)
{
    return 0;
}

static inline int wps_validate_assoc_req(const struct wpabuf *wps_ie)
{
    return 0;
}

static inline int wps_validate_assoc_resp(const struct wpabuf *wps_ie)
{
    return 0;
}

static inline int wps_validate_m1(const struct wpabuf *tlvs)
{
    return 0;
}

static inline int wps_validate_m2(const struct wpabuf *tlvs)
{
    return 0;
}

static inline int wps_validate_m2d(const struct wpabuf *tlvs)
{
    return 0;
}

static inline int wps_validate_m3(const struct wpabuf *tlvs)
{
    return 0;
}

static inline int wps_validate_m4(const struct wpabuf *tlvs)
{
    return 0;
}

static inline int wps_validate_m4_encr(const struct wpabuf *tlvs, int wps2)
{
    return 0;
}

static inline int wps_validate_m5(const struct wpabuf *tlvs)
{
    return 0;
}

static inline int wps_validate_m5_encr(const struct wpabuf *tlvs, int wps2)
{
    return 0;
}

static inline int wps_validate_m6(const struct wpabuf *tlvs)
{
    return 0;
}

static inline int wps_validate_m6_encr(const struct wpabuf *tlvs, int wps2)
{
    return 0;
}

static inline int wps_validate_m7(const struct wpabuf *tlvs)
{
    return 0;
}

static inline int wps_validate_m7_encr(const struct wpabuf *tlvs, int ap,
                       int wps2)
{
    return 0;
}

static inline int wps_validate_m8(const struct wpabuf *tlvs)
{
    return 0;
}

static inline int wps_validate_m8_encr(const struct wpabuf *tlvs, int ap,
                       int wps2)
{
    return 0;
}

static inline int wps_validate_wsc_ack(const struct wpabuf *tlvs)
{
    return 0;
}

static inline int wps_validate_wsc_nack(const struct wpabuf *tlvs)
{
    return 0;
}

static inline int wps_validate_wsc_done(const struct wpabuf *tlvs)
{
    return 0;
}

static inline int wps_validate_upnp_set_selected_registrar(
    const struct wpabuf *tlvs)
{
    return 0;
}
#endif /* CONFIG_WPS_STRICT */

/* wps_enrollee.c */
struct wpabuf * wps_enrollee_get_msg(struct wps_data *wps,
                     enum wsc_op_code *op_code);
enum wps_process_res wps_enrollee_process_msg(struct wps_data *wps,
                          enum wsc_op_code op_code,
                          const struct wpabuf *msg);

/* wps_registrar.c */
struct wpabuf * wps_registrar_get_msg(struct wps_data *wps,
                      enum wsc_op_code *op_code);
enum wps_process_res wps_registrar_process_msg(struct wps_data *wps,
                           enum wsc_op_code op_code,
                           const struct wpabuf *msg);
int wps_build_cred(struct wps_data *wps, struct wpabuf *msg);
int wps_device_store(struct wps_registrar *reg,
             struct wps_device_data *dev, const uint8_t *uuid);
void wps_registrar_selected_registrar_changed(struct wps_registrar *reg,
                          uint16_t dev_pw_id);
const uint8_t * wps_authorized_macs(struct wps_registrar *reg, size_t *count);
int wps_registrar_pbc_overlap(struct wps_registrar *reg,
                  const uint8_t *addr, const uint8_t *uuid_e);
#ifdef CONFIG_WPS_NFC
void wps_registrar_remove_nfc_pw_token(struct wps_registrar *reg,
                       struct wps_nfc_pw_token *token);
#endif
int wps_cb_new_psk(struct wps_registrar *reg, const uint8_t *mac_addr,
           const uint8_t *p2p_dev_addr, const uint8_t *psk, size_t psk_len);

struct wps_data * wps_init(struct wps_context *wps);
void wps_deinit(struct wps_data *data);
enum wps_process_res wps_process_msg(struct wps_data *wps,
                     enum wsc_op_code op_code,
                     const struct wpabuf *msg);
struct wpabuf * wps_get_msg(struct wps_data *wps, enum wsc_op_code *op_code);

int wpas_wps_cred_cb(struct wps_data *wps,       struct wps_credential *cred);
int wpas_wps_build_probe_req_ie(int vif_idx);
int wpas_wps_ssid_bss_match(int vif_idx, uint8_t *frame, uint32_t frame_len);

#endif /* WPAS_WPS_H */
