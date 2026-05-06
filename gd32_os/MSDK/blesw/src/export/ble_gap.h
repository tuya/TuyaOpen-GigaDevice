/*!
    \file    ble_gap.h
    \brief   Definitions and prototypes for the BLE GAP interface.

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

#ifndef _BLE_GAP_H_
#define _BLE_GAP_H_

#include <stdint.h>
#include <stdbool.h>

#include "ble_types.h"
#include "ble_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* BLE gd company id */
#define BLE_GD_COMP_ID                  0x0C2B

/* BLE address length */
#define BLE_GAP_ADDR_LEN                (6)

/* BLE key length */
#define BLE_GAP_KEY_LEN                 (16)

/* BLE AES data length */
#define BLE_GAP_AES_LEN                 (16)

/* BLE P256 key length */
#define BLE_GAP_P256_KEY_LEN            (32)

/* BLE link layer feature set value length */
#define BLE_GAP_FEATURES_LEN            (8)

/* BLE channel map value length */
#define BLE_GAP_CHANNEL_MAP_LEN         (5)

/* BLE random number value length */
#define BLE_GAP_RANDOM_NUMBER_LEN       (8)

/* BLE legacy advertising max len */
#define BLE_GAP_LEGACY_ADV_MAX_LEN      (31)

#define BLE_GAP_ADV_SCAN_UNIT(_ms)      ((_ms) * 8 / 5)

/* Advertising_Tx_Power, Host has no preference */
#define BLE_GAP_ADV_TX_PWR_NO_PREF              (127)

/* BLE privacy mode */
#define BLE_GAP_PRIVACY_MODE_OFF                    0x00 /*!< No privacy, only use identity address */
#define BLE_GAP_PRIVACY_MODE_DEVICE_PRIVACY         0x01 /*!< Use device privacy, accept both the peer's identity address and resolvable private address*/
#define BLE_GAP_PRIVACY_MODE_NETWORK_PRIVACY        0x02 /*!< Use network privacy, only accept resolvable private address */

/* Invalid power level */
#define BLE_GAP_POWER_LEVEL_INVALID     127

/* BLE GAP advertisement flags */
#define BLE_GAP_ADV_FLAG_LE_LIMITED_DISC_MODE         (0x01)   /*!< LE Limited Discoverable Mode */
#define BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE         (0x02)   /*!< LE General Discoverable Mode */
#define BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED         (0x04)   /*!< BR/EDR not supported */
#define BLE_GAP_ADV_FLAG_LE_BR_EDR_CONTROLLER         (0x08)   /*!< Simultaneous LE and BR/EDR, Controller */
#define BLE_GAP_ADV_FLAG_LE_BR_EDR_HOST               (0x10)   /*!< Simultaneous LE and BR/EDR, Host */
#define BLE_GAP_ADV_FLAG_LE_ONLY_LIMITED_DISC_MODE    (BLE_GAP_ADV_FLAG_LE_LIMITED_DISC_MODE | BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED)   /*!< LE Limited Discoverable Mode, BR/EDR not supported */
#define BLE_GAP_ADV_FLAG_LE_ONLY_GENERAL_DISC_MODE    (BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE | BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED)   /*!< LE General Discoverable Mode, BR/EDR not supported */

/* BLE GAP legacy advertising interval limit in 625us units */
#define BLE_GAP_LEGACY_ADV_INTERVAL_MIN 0x0020      /*!< Minimum legacy advertising interval, i.e. 20ms */
#define BLE_GAP_LEGACY_ADV_INTERVAL_MAX 0x4000      /*!< Maximum legacy advertising interval, i.e. 10.24s */

/* BLE GAP extended advertising interval limit in 625us units */
#define BLE_GAP_EXT_ADV_INTERVAL_MIN    0x000020    /*!< Minimum extended advertising interval, i.e. 20ms */
#define BLE_GAP_EXT_ADV_INTERVAL_MAX    0xFFFFFF    /*!< Maximum extended advertising interval, i.e. 10485.759375s */

/* BLE GAP periodic advertising interval limit in 1.25ms units */
#define BLE_GAP_PER_ADV_INTERVAL_MIN    0x0006      /*!< Minimum periodic advertising interval, i.e. 7.5ms */
#define BLE_GAP_PER_ADV_INTERVAL_MAX    0xFFFF      /*!< Maximum periodic advertising interval, i.e. 81.91875s */

/* BLE GAP scan interval limit in 625us units */
#define BLE_GAP_SCAN_INTERVAL_MIN       0x0004      /*!< Minimum scan interval, i.e. 2.5ms */
#define BLE_GAP_SCAN_INTERVAL_MAX       0xFFFF      /*!< Maximum scan interval, i.e. 40.959375s */

/* BLE GAP scan window limit in 625us units */
#define BLE_GAP_SCAN_WINDOW_MIN         0x0004      /*!< Minimum scan window, i.e. 2.5ms */
#define BLE_GAP_SCAN_WINDOW_MAX         0xFFFF      /*!< Maximum scan window, i.e. 40.959375s */

/* BLE GAP scan timeout limit in 10ms units */
#define BLE_GAP_SCAN_TIMEOUT_MIN        0x0001      /*!< Minimum scan timeout, i.e 10ms */
#define BLE_GAP_SCAN_TIMEOUT_UNLIMITED  0x0000      /*!< Continuous scan, no time limit */

/* BLE GAP authenticated payload timeout limit in 10ms units */
#define BLE_GAP_AUTH_PAYLOAD_TOUT_MIN   0x0001      /*!< Minimum authenticated payload timeout, i.e. 10ms */
#define BLE_GAP_AUTH_PAYLOAD_TOUT_MAX   0xFFFF      /*!< Maximum authenticated payload timeout, i.e. 655350ms */

/* BLE GAP payload data length limit in octets or us */
#define BLE_GAP_MIN_OCTETS              (27)    /*!< Minimum data packet transmission size in number of octets */
#define BLE_GAP_MIN_TIME                (328)   /*!< Minimum data packet transmission duration in us */
#define BLE_GAP_MAX_OCTETS              (251)   /*!< Maximum data packet transmission size in number of octets */
#define BLE_GAP_MAX_TIME                (17040) /*!< Maximum data packet transmission duration in us */

/* BLE GAP address type */
typedef enum ble_gap_addr_type
{
    BLE_GAP_ADDR_TYPE_PUBLIC = 0x00,    /*!< Public device address */
    BLE_GAP_ADDR_TYPE_RANDOM = 0x01,    /*!< Random device address */
} ble_gap_addr_type_t;

/* BLE GAP role */
typedef enum ble_gap_role
{
    BLE_GAP_ROLE_OBSERVER       = 0x01,                                             /*!< BLE observer role */
    BLE_GAP_ROLE_BROADCASTER    = 0x02,                                             /*!< BLE broadcaster role */
    BLE_GAP_ROLE_CENTRAL        = (0x04 | BLE_GAP_ROLE_OBSERVER),                   /*!< BLE central role */
    BLE_GAP_ROLE_PERIPHERAL     = (0x08 | BLE_GAP_ROLE_BROADCASTER),                /*!< BLE peripheral role */
    BLE_GAP_ROLE_ALL            = (BLE_GAP_ROLE_CENTRAL | BLE_GAP_ROLE_PERIPHERAL), /*!< BLE all roles: both peripheral and central */
} ble_gap_role_t;

/* BLE GAP pairing mode */
typedef enum ble_gap_pairing_mode
{
    BLE_GAP_PAIRING_DISABLE             = 0,            /*!< No pairing authorized */
    BLE_GAP_PAIRING_LEGACY              = (1 << 0),     /*!< Legacy pairing authorized */
    BLE_GAP_PAIRING_SECURE_CONNECTION   = (1 << 1),     /*!< Secure Connection pairing authorized */
} ble_gap_pairing_mode_t;

/* BLE GAP local address type used for advertising/scan/connection */
typedef enum ble_gap_local_addr_type
{
   BLE_GAP_LOCAL_ADDR_STATIC = 0,       /*!< Use public address or private static address as local address */
   BLE_GAP_LOCAL_ADDR_RESOLVABLE,       /*!< Use generated resolvable private random address as local address */
   BLE_GAP_LOCAL_ADDR_NONE_RESOLVABLE,  /*!< Use generated non-resolvable private random address as local address */
} ble_gap_local_addr_type_t;

/* BLE GAP advertising type */
typedef enum ble_gap_adv_type
{
    BLE_GAP_ADV_TYPE_LEGACY = 0,    /*!< Legacy advertising */
    BLE_GAP_ADV_TYPE_EXTENDED,      /*!< Extended advertising */
    BLE_GAP_ADV_TYPE_PERIODIC,      /*!< Periodic advertising */
} ble_gap_adv_type_t;

/* BLE GAP advertising data type */
typedef enum ble_gap_adv_data_type
{
    BLE_GAP_ADV_DATA_TYPE_ADV = 0,      /*!< Advertising data */
    BLE_GAP_ADV_DATA_TYPE_SCAN_RSP,     /*!< Scan response data */
    BLE_GAP_ADV_DATA_TYPE_PER_ADV,      /*!< Periodic advertising data */
} ble_gap_adv_data_type_t;

/* BLE advertising properties bitfield */
typedef enum ble_gap_adv_prop_bf
{
    BLE_GAP_ADV_PROP_CONNECTABLE_BIT = 0x0001,  /*!< Bitmask for connectable advertising */
    BLE_GAP_ADV_PROP_SCANNABLE_BIT   = 0x0002,  /*!< Bitmask for scannable advertising */
    BLE_GAP_ADV_PROP_DIRECTED_BIT    = 0x0004,  /*!< Bitmask for directed advertising */
    BLE_GAP_ADV_PROP_HIGH_DUTY_BIT   = 0x0008,  /*!< Bitmask for high duty advertising */
    BLE_GAP_ADV_PROP_ANONYMOUS_BIT   = 0x0020,  /*!< Bitmask for anonymous advertising. No adva will be set in adversing packets */
} ble_gap_adv_prop_bf_t;

/* BLE GAP advertising properties for legacy advertising */
typedef enum ble_gap_legacy_adv_prop
{
    BLE_GAP_ADV_PROP_NON_CONN_NON_SCAN     = 0x0000,                                                                /*!< Non-connectable non-scannable advertising */
    BLE_GAP_ADV_PROP_NON_CONN_SCAN         = BLE_GAP_ADV_PROP_SCANNABLE_BIT,                                        /*!< Non-connectable scannable advertising */
    BLE_GAP_ADV_PROP_UNDIR_CONN            = (BLE_GAP_ADV_PROP_CONNECTABLE_BIT | BLE_GAP_ADV_PROP_SCANNABLE_BIT),   /*!< Undirected connectable advertising */
    BLE_GAP_ADV_PROP_CONN_DIRECT           = (BLE_GAP_ADV_PROP_CONNECTABLE_BIT | BLE_GAP_ADV_PROP_DIRECTED_BIT),    /*!< Directed connectable advertising */
    BLE_GAP_ADV_PROP_HIGH_DUTY_CONN_DIRECT = (BLE_GAP_ADV_PROP_CONN_DIRECT | BLE_GAP_ADV_PROP_HIGH_DUTY_BIT),       /*!< High duty cycle directed connectable advertising */
} ble_gap_legacy_adv_prop_t;

/* BLE GAP advertising properties for extended advertising */
typedef enum ble_gap_extended_adv_prop
{
    BLE_GAP_EXT_ADV_PROP_NON_CONN_NON_SCAN    = 0x0000,                                                             /*!< Non-connectable non-scannable extended advertising */
    BLE_GAP_EXT_ADV_PROP_NON_CONN_SCAN        = BLE_GAP_ADV_PROP_SCANNABLE_BIT,                                     /*!< Non-connectable scannable extended advertising */
    BLE_GAP_EXT_ADV_PROP_NON_CONN_SCAN_DIRECT = (BLE_GAP_ADV_PROP_SCANNABLE_BIT | BLE_GAP_ADV_PROP_DIRECTED_BIT),   /*!< Non-connectable scannable directed extended advertising */
    BLE_GAP_EXT_ADV_PROP_ANONYMOUS_DIRECT     = (BLE_GAP_ADV_PROP_DIRECTED_BIT | BLE_GAP_ADV_PROP_ANONYMOUS_BIT),   /*!< Non-connectable anonymous directed extended advertising */
    BLE_GAP_EXT_ADV_PROP_CONN_UNDIRECT        = BLE_GAP_ADV_PROP_CONNECTABLE_BIT,                                   /*!< Undirected connectable extended advertising */
    BLE_GAP_EXT_ADV_PROP_CONN_DIRECT          = (BLE_GAP_ADV_PROP_CONNECTABLE_BIT | BLE_GAP_ADV_PROP_DIRECTED_BIT), /*!< Directed connectable extended advertising */
} ble_gap_extended_adv_prop_t;

/* BLE GAP advertising properties for periodic advertising */
typedef enum ble_gap_periodic_adv_prop
{
    BLE_GAP_PER_ADV_PROP_UNDIRECT = 0x0000,                         /*!< Undirected periodic advertising */
    BLE_GAP_PER_ADV_PROP_DIRECT   = BLE_GAP_ADV_PROP_DIRECTED_BIT,  /*!< Directed periodic advertising */
} ble_gap_periodic_adv_prop_t;

/* BLE advertising discovery mode */
typedef enum ble_gap_adv_discovery_mode
{
    BLE_GAP_ADV_MODE_NON_DISC = 0,  /*!< Non-discoverable advertising */
    BLE_GAP_ADV_MODE_GEN_DISC,      /*!< General discoverable advertising */
    BLE_GAP_ADV_MODE_LIM_DISC,      /*!< Limited discoverable advertising */
    BLE_GAP_ADV_MODE_BEACON         /*!< Broadcast mode without presence of BLE_AD_TYPE_FLAGS in advertising data */
} ble_gap_adv_discovery_mode_t;

/* BLE advertising filter policy */
typedef enum ble_gap_adv_filter_policy
{
    BLE_GAP_ADV_ALLOW_SCAN_ANY_CON_ANY = 0x00,  /*!< Allow scan and connection requests from all devices */
    BLE_GAP_ADV_ALLOW_SCAN_FAL_CON_ANY,         /*!< Allow scan request from FAL devices only and connection request from all devices */
    BLE_GAP_ADV_ALLOW_SCAN_ANY_CON_FAL,         /*!< Allow scan request from all devices and connection request from FAL devices only */
    BLE_GAP_ADV_ALLOW_SCAN_FAL_CON_FAL,         /*!< Allow scan and connection requests from FAL devices only */
} ble_gap_adv_filter_policy_t;

/* BLE advertising channel map bitfield */
typedef enum ble_gap_adv_chann_map_bf
{
    BLE_GAP_ADV_CHANN_37 = 0x01,        /*!< Channel 37 is used */
    BLE_GAP_ADV_CHANN_38 = 0x02,        /*!< Channel 38 is used */
    BLE_GAP_ADV_CHANN_39 = 0x04,        /*!< Channel 39 is used */
} ble_gap_adv_chann_map_bf_t;

/* BLE GAP PHY value */
typedef enum ble_gap_phy
{
    BLE_GAP_PHY_1MBPS = 1,  /*!< BLE 1M PHY */
    BLE_GAP_PHY_2MBPS = 2,  /*!< BLE 2M PHY */
    BLE_GAP_PHY_CODED = 3,  /*!< BLE Coded PHY */
} ble_gap_phy_t;

/* BLE PHY value used for specifying transmit power */
typedef enum ble_gap_phy_pwr_value
{
    BLE_GAP_PHY_PWR_1MBPS_VALUE = 1,        /*!< BLE 1M PHY */
    BLE_GAP_PHY_PWR_2MBPS_VALUE = 2,        /*!< BLE 2M PHY */
    BLE_GAP_PHY_PWR_S8_CODED_VALUE = 3,     /*!< BLE Coded PHY with S=8 data coding */
    BLE_GAP_PHY_PWR_S2_CODED_VALUE = 4,     /*!< BLE Coded PHY with S=2 data coding */
} ble_gap_phy_pwr_value_t;

/* BLE clock accuracy values */
typedef enum ble_gap_clock_accuracy
{
    BLE_GAP_CLK_ACC_500 = 0,    /*!< 500 ppm */
    BLE_GAP_CLK_ACC_250,        /*!< 250 ppm */
    BLE_GAP_CLK_ACC_150,        /*!< 150 ppm */
    BLE_GAP_CLK_ACC_100,        /*!< 100 ppm */
    BLE_GAP_CLK_ACC_75,         /*!< 75 ppm */
    BLE_GAP_CLK_ACC_50,         /*!< 50 ppm */
    BLE_GAP_CLK_ACC_30,         /*!< 30 ppm */
    BLE_GAP_CLK_ACC_20,         /*!< 20 ppm */
} ble_gap_clock_accuracy_t;

/* BLE GAP authentication mask used for SMP */
typedef enum ble_gap_auth_mask
{
    BLE_GAP_AUTH_MASK_NONE      = 0,            /*!< No Flag set */
    BLE_GAP_AUTH_MASK_BOND      = (1 << 0),     /*!< Bond authentication bitmask */
    BLE_GAP_AUTH_MASK_MITM      = (1 << 2),     /*!< Man In the middle protection bitmask */
    BLE_GAP_AUTH_MASK_SEC_CON   = (1 << 3),     /*!< Secure Connection bitmask */
    BLE_GAP_AUTH_MASK_KEY_NOTIF = (1 << 4),     /*!< Key Notification bitmask */
    BLE_GAP_AUTH_MASK_CT2       = (1 << 5),     /*!< Cross Transport Key Derivation bitmask */
} ble_gap_auth_mask_t;

/* BLE GAP authentication requirements */
typedef enum ble_gap_auth
{
    BLE_GAP_AUTH_REQ_NO_MITM_NO_BOND    = BLE_GAP_AUTH_MASK_NONE,                                                           /*!< No MITM No Bonding */
    BLE_GAP_AUTH_REQ_NO_MITM_BOND       = BLE_GAP_AUTH_MASK_BOND,                                                           /*!< No MITM and Bonding */
    BLE_GAP_AUTH_REQ_MITM_NO_BOND       = BLE_GAP_AUTH_MASK_MITM,                                                           /*!< MITM and No Bonding */
    BLE_GAP_AUTH_REQ_MITM_BOND          = (BLE_GAP_AUTH_MASK_MITM | BLE_GAP_AUTH_MASK_BOND),                                /*!< MITM and Bonding */
    BLE_GAP_AUTH_REQ_SEC_CON_NO_BOND    = (BLE_GAP_AUTH_MASK_SEC_CON | BLE_GAP_AUTH_MASK_MITM),                             /*!< Secure connection and No Bonding */
    BLE_GAP_AUTH_REQ_SEC_CON_BOND       = (BLE_GAP_AUTH_MASK_SEC_CON | BLE_GAP_AUTH_MASK_MITM | BLE_GAP_AUTH_MASK_BOND),    /*!< Secure connection and Bonding */
} ble_gap_auth_t;

/* BLE GAP IO Capabilities */
typedef enum ble_gap_io_cap
{
    BLE_GAP_IO_CAP_DISPLAY_ONLY     = 0x00,     /*!< Display Only */
    BLE_GAP_IO_CAP_DISPLAY_YESNO    = 0x01,     /*!< Display and Yes/No */
    BLE_GAP_IO_CAP_KEYBOARD_ONLY    = 0x02,     /*!< Keyboard Only */
    BLE_GAP_IO_CAP_NO_IO            = 0x03,     /*!< No Input No Output */
    BLE_GAP_IO_CAP_KEYBOARD_DISPLAY = 0x04,     /*!< Keyboard and Display */
} ble_gap_io_cap_t;

/* BLE SMP key distribution flags */
typedef enum ble_gap_key_dist
{
    BLE_GAP_KDIST_NONE      = 0x00,         /*!< No Keys to distribute */
    BLE_GAP_KDIST_ENCKEY    = (1 << 0),     /*!< Encryption key (LTK) in distribution */
    BLE_GAP_KDIST_IDKEY     = (1 << 1),     /*!< ID key (IRK) in distribution */
    BLE_GAP_KDIST_SIGNKEY   = (1 << 2)      /*!< Signature key (CSRK) in distribution */
} ble_gap_key_dist_t;

/* BLE GAP pairing level achieved */
typedef enum ble_gap_pairing_level
{
    BLE_GAP_PAIRING_UNAUTH          = 0x00,     /*!< Unauthenticated pairing without bond data */
    BLE_GAP_PAIRING_AUTH            = 0x04,     /*!< Authenticated pairing without bond data */
    BLE_GAP_PAIRING_SECURE_CON      = 0x0C,     /*!< Secure connection pairing without bond data.*/

    BLE_GAP_PAIRING_NO_BOND         = 0x00,     /*!< No pairing performed with peer device */
    BLE_GAP_PAIRING_BOND_UNAUTH     = 0x01,     /*!< Unauthenticated pairing with bond data.*/
    BLE_GAP_PAIRING_BOND_AUTH       = 0x05,     /*!< Authenticated pairing with bond data */
    BLE_GAP_PAIRING_BOND_SECURE_CON = 0x0D,     /*!< Secure connection pairing with bond data.*/

    BLE_GAP_PAIRING_BOND_PRESENT_BIT = 0x01,    /*!< Pairing with bond data present Bit */
} ble_gap_pairing_level_t;

/* BLE security level */
typedef enum ble_gap_sec_lvl
{
    BLE_GAP_SEC_NOT_ENC = 0,    /*!< Security Level 1, service accessible through an un-encrypted link */
    BLE_GAP_SEC_UNAUTH,         /*!< Security Level 2, service require an unauthenticated pairing */
    BLE_GAP_SEC_AUTH,           /*!< Security Level 3, service require an authenticated pairing */
    BLE_GAP_SEC_SECURE_CON,     /*!< Security Level 4, service require a secure connection pairing */
} ble_gap_sec_lvl_t;

/* BLE security request level */
typedef enum ble_gap_sec_req
{
    BLE_GAP_NO_SEC = 0x00,          /*!< No security (no authentication and encryption) */
    BLE_GAP_SEC1_NOAUTH_PAIR_ENC,   /*!< Unauthenticated pairing with encryption */
    BLE_GAP_SEC1_AUTH_PAIR_ENC,     /*!< Authenticated pairing with encryption */
    BLE_GAP_SEC2_NOAUTH_DATA_SGN,   /*!< Unauthenticated pairing with data signing */
    BLE_GAP_SEC2_AUTH_DATA_SGN,     /*!< Authentication pairing with data signing */
    BLE_GAP_SEC1_SEC_CON_PAIR_ENC,  /*!< Secure Connection pairing with encryption */
} ble_gap_sec_req_t;

/* BLE GAP Keypress Notification Type */
typedef enum ble_gap_kp_ntf_type
{
    BLE_GAP_KP_NTF_TYPE_PASSKEY_START     = 0x00,   /*!< Passkey entry started */
    BLE_GAP_KP_NTF_TYPE_PASSKEY_DIGIT_IN  = 0x01,   /*!< Passkey digit entered */
    BLE_GAP_KP_NTF_TYPE_PASSKEY_DIGIT_OUT = 0x02,   /*!< Passkey digit erased */
    BLE_GAP_KP_NTF_TYPE_PASSKEY_CLEAR     = 0x03,   /*!< Passkey cleared */
    BLE_GAP_KP_NTF_TYPE_PASSKEY_END       = 0x04,   /*!< Passkey entry completed */
} ble_gap_kp_ntf_type_t;

/* BLE privacy mode */
typedef enum ble_gap_privacy_mode
{
    BLE_GAP_PRIVACY_TYPE_NETWORK = 0x00,    /*!< Network privacy mode */
    BLE_GAP_PRIVACY_TYPE_DEVICE = 0x01,     /*!< Device privacy mode */
} ble_gap_privacy_mode_t;

/* BLE GATT client infomation */
typedef enum ble_gap_client_info
{
    BLE_GAP_CLI_SVC_CHANGED_IND_EN_BIT = (1 << 0),  /*!< Service changed indication enabled */
    BLE_GAP_CLI_DB_UPDATED_BIT         = (1 << 1),  /*!< Database updated since last connection */
} ble_gap_client_info_t;

/* BLE GATT client features */
typedef enum ble_gap_client_feat
{
    BLE_GAP_CLI_ROBUST_CACHE_EN_BIT    = (1 << 0),  /*!< Robust Cache feature enabled */
    BLE_GAP_CLI_EATT_SUPPORTED_BIT     = (1 << 1),  /*!< Enhanced ATT bearer support */
    BLE_GAP_CLI_MULT_NTF_SUPPORTED_BIT = (1 << 2),  /*!< Multiple Handle Value Notifications support */
} ble_gap_client_feat_t;

/* BLE GATT server features */
typedef enum ble_gap_server_feat
{
    BLE_GAP_SRV_EATT_SUPPORTED_BIT = (1 << 0),      /*!< Enhanced ATT bearer supported */
} ble_gap_server_feat_t;

/* BLE power control flags bit field parameters */
typedef enum ble_gap_pwr_ctrl_flags
{
    BLE_GAP_PWR_CTRL_MIN_BIT = 0x01,    /*!< Minimum supported power level is reached */
    BLE_GAP_PWR_CTRL_MAX_BIT = 0x02,    /*!< Maximum supported power level is reached */
} ble_gap_pwr_ctrl_flags_t;

/* BLE GAP path loss zone definitions */
typedef enum ble_gap_path_loss_zone
{
    BLE_GAP_PATH_LOSS_LOW  = 0,     /*!< Low zone entered */
    BLE_GAP_PATH_LOSS_MID  = 1,     /*!< Middle zone entered */
    BLE_GAP_PATH_LOSS_HIGH = 2,     /*!< High zone entered */
} ble_gap_path_loss_zone_t;

/* BLE GAP scan type */
typedef enum ble_gap_scan_type
{
    BLE_GAP_SCAN_TYPE_GEN_DISC = 0,     /*!< General discovery */
    BLE_GAP_SCAN_TYPE_LIM_DISC,         /*!< Limited discovery */
    BLE_GAP_SCAN_TYPE_OBSERVER,         /*!< Observer */
    BLE_GAP_SCAN_TYPE_SEL_OBSERVER,     /*!< Observer with FAL used */
    BLE_GAP_SCAN_TYPE_CONN_DISC,        /*!< Connectable discovery, only accept connectable advertising report */
    BLE_GAP_SCAN_TYPE_SEL_CONN_DISC,    /*!< Connectable discovery with FAL used */
} ble_gap_scan_type_t;

/* BLE GAP scan properties */
typedef enum ble_gap_scan_prop
{
    BLE_GAP_SCAN_PROP_PHY_1M_BIT       = (1 << 0),  /*!< Scan advertisement on the LE 1M PHY */
    BLE_GAP_SCAN_PROP_PHY_CODED_BIT    = (1 << 1),  /*!< Scan advertisement on the LE Coded PHY */
    BLE_GAP_SCAN_PROP_ACTIVE_1M_BIT    = (1 << 2),  /*!< Active scan on LE 1M PHY (Scan Request PDUs may be sent) */
    BLE_GAP_SCAN_PROP_ACTIVE_CODED_BIT = (1 << 3),  /*!< Active scan on LE Coded PHY (Scan Request PDUs may be sent)*/
    BLE_GAP_SCAN_PROP_ACCEPT_RPA_BIT   = (1 << 4),  /*!< Accept directed advertising packets if we use a RPA and target address cannot be solved */
    BLE_GAP_SCAN_PROP_FILT_TRUNC_BIT   = (1 << 5),  /*!< Filter truncated advertising or scan response reports */
} ble_gap_scan_prop_t;

/* BLE GAP scan duplicated filter setting */
typedef enum ble_gap_scan_dup_filter
{
    BLE_GAP_DUP_FILT_DIS = 0,       /*!< Disable filtering of duplicated packets */
    BLE_GAP_DUP_FILT_EN,            /*!< Enable filtering of duplicated packets */
    BLE_GAP_DUP_FILT_EN_PERIOD,     /*!< Enable filtering of duplicated packets, reset for each scan period */
} ble_gap_scan_dup_filter_t;

/* BLE GAP init type */
typedef enum ble_gap_init_type
{
    BLE_GAP_INIT_TYPE_DIRECT_CONN_EST = 0,  /*!< Direct connection establishment, establish a connection with an indicated device */
    BLE_GAP_INIT_TYPE_AUTO_CONN_EST,        /*!< Automatic connection establishment, establish a connection with devices in FAL */
} ble_gap_init_type_t;

/* BLE GAP init properties */
typedef enum ble_gap_init_prop
{
    BLE_GAP_INIT_PROP_1M_BIT    = (1 << 0), /*!< Scan connectable advertisements on 1M PHY. Connection parameters for 1M PHY are provided */
    BLE_GAP_INIT_PROP_2M_BIT    = (1 << 1), /*!< Connection parameters for the LE 2M PHY are provided */
    BLE_GAP_INIT_PROP_CODED_BIT = (1 << 2), /*!< Scan connectable advertisements on Coded PHY. Connection parameters for Coded PHY are provided */
} ble_gap_init_prop_t;

/* BLE periodic synchronization type */
typedef enum ble_gap_per_sync_type
{
    BLE_GAP_PER_SYNC_TYPE_GENERAL = 0,  /*!< Use advertiser information provided for synchronization */
    BLE_GAP_PER_SYNC_TYPE_SELECTIVE,    /*!< Use periodic advertiser list for synchronization */
    BLE_GAP_PER_SYNC_TYPE_PAST,         /*!< Use periodic advertising sync transfer information for synchronization */
} ble_gap_per_sync_type_t;

/* BLE periodic synchronization report control bitfiled */
typedef enum ble_gap_per_sync_report_en_bf
{
    BLE_GAP_REPORT_ADV_EN_BIT = 0x01,               /*!< Periodic advertising reports reception enabled */
    BLE_GAP_REPORT_BIG_INFO_EN_BIT = 0x02,          /*!< BIG Info advertising reports reception enabled */
    BLE_GAP_REPORT_DUPLICATE_FILTER_EN_BIT = 0x04,  /*!< Duplicate filtering enabled.  */
} ble_gap_per_sync_report_en_bf_t;

/* BLE privacy configuration */
typedef enum ble_gap_privacy_cfg
{
    BLE_GAP_PRIV_CFG_PRIV_ADDR_BIT = (1 << 0),      /*!< Indicate identity address type, 0 for public address and 1 for static private random address */
    BLE_GAP_PRIV_CFG_PRIV_EN_BIT   = (1 << 2),      /*!< Indicate if controller privacy should be enabled */
} ble_gap_privacy_cfg_t;

/* BLE GATT attribute write permission requirement */
typedef enum ble_gap_write_att_perm
{
    BLE_GAP_WRITE_DISABLE = 0,  /*!< Disable write access */
    BLE_GAP_WRITE_NOT_ENC = 1,  /*!< Write access with no encryption required */
    BLE_GAP_WRITE_UNAUTH  = 2,  /*!< Write access requires unauthenticated link */
    BLE_GAP_WRITE_AUTH    = 3,  /*!< Write access requires authenticated link */
    BLE_GAP_WRITE_SEC_CON = 4   /*!< Write access requires secure connections link */
} ble_gap_write_att_perm_t;

/* Bit field use to select the preferred TX or RX LE PHY */
typedef enum ble_gap_phy_bf
{
    BLE_GAP_PHY_ANY       = 0x00,       /*!< No preferred PHY */
    BLE_GAP_PHY_BIT_1MBPS = (1 << 0),   /*!< 1M PHY preferred */
    BLE_GAP_PHY_BIT_2MBPS = (1 << 1),   /*!< 2M PHY preferred */
    BLE_GAP_PHY_BIT_CODED = (1 << 2),   /*!< Coded PHY preferred */
} ble_gap_phy_bf_t;

/* BLE PHY options used when select Coded PHY */
typedef enum ble_gap_phy_option
{
    BLE_GAP_PHY_OPT_CODED_ALL  = 0,     /*!< No preference for rate used when transmitting on the LE Coded PHY */
    BLE_GAP_PHY_OPT_CODED_500K = 1,     /*!< 500 kbps rate preferred when transmitting on the LE Coded PHY */
    BLE_GAP_PHY_OPT_CODED_125K = 2,     /*!< 125 kbps rate preferred when transmitting on the LE Coded PHY */
} ble_gap_phy_option_t;

/* BLE GAP attribute database configuration
 *    15   14   13   12   11     10      9    8    7    6    5    4    3    2    1    0
 *  +----+----+----+----+----+---------+----+----+----+----+----+----+----+----+----+----+
 *  |           RFU     |RPAO|BOND_INFO|EATT| FE |MTU |PCP |   APP_PERM   |   NAME_PERM  |
 *  +----+----+----+----+----+---------+----+----+----+----+----+----+----+----+----+----+
 */
typedef enum ble_gap_att_cfg_flag
{
    BLE_GAP_ATT_NAME_PERM_MASK             = 0x0007,    /*!< Device Name write permission, @ref ble_gap_write_att_perm_t */
    BLE_GAP_ATT_NAME_PERM_LSB              = 0,
    BLE_GAP_ATT_APPEARENCE_PERM_MASK       = 0x0038,    /*!< Device Appearance write permission, @ref ble_gap_write_att_perm_t */
    BLE_GAP_ATT_APPEARENCE_PERM_LSB        = 3,
    BLE_GAP_ATT_SLV_PREF_CON_PAR_EN_MASK   = 0x0040,    /*!< Slave Preferred Connection Parameters present in GAP attribute database */
    BLE_GAP_ATT_SLV_PREF_CON_PAR_EN_LSB    = 6,
    BLE_GAP_ATT_CLI_DIS_AUTO_MTU_EXCH_MASK = 0x0080,    /*!< Disable automatic MTU exchange at connection establishment */
    BLE_GAP_ATT_CLI_DIS_AUTO_MTU_EXCH_LSB  = 7,
    BLE_GAP_ATT_CLI_DIS_AUTO_FEAT_EN_MASK  = 0x0100,    /*!< Disable automatic client feature enable setup at connection establishment */
    BLE_GAP_ATT_CLI_DIS_AUTO_FEAT_EN_LSB   = 8,
    BLE_GAP_ATT_CLI_DIS_AUTO_EATT_MASK     = 0x0200,    /*!< Disable automatic establishment of Enhanced ATT bearers */
    BLE_GAP_ATT_CLI_DIS_AUTO_EATT_LSB      = 9,
    BLE_GAP_ATT_RSLV_PRIV_ADDR_ONLY_MASK   = 0x0400,    /*!< Enable presence of RPA only which means that after a bond, device must only use RPA */
    BLE_GAP_ATT_RSLV_PRIV_ADDR_ONLY_LSB    = 10,
}ble_gap_att_cfg_flag_t;

/* BLE packet payload type for test mode */
typedef enum ble_gap_packet_payload_type
{
    BLE_GAP_PKT_PLD_PRBS9,              /*!< PRBS9 sequence */
    BLE_GAP_PKT_PLD_REPEATED_11110000,  /*!< Repeated "11110000" */
    BLE_GAP_PKT_PLD_REPEATED_10101010,  /*!< Repeated "10101010" */
    BLE_GAP_PKT_PLD_PRBS15,             /*!< PRBS15 sequence */
    BLE_GAP_PKT_PLD_REPEATED_11111111,  /*!< Repeated "11111111" sequence */
    BLE_GAP_PKT_PLD_REPEATED_00000000,  /*!< Repeated "00000000" sequence */
    BLE_GAP_PKT_PLD_REPEATED_00001111,  /*!< Repeated "00001111" sequence */
    BLE_GAP_PKT_PLD_REPEATED_01010101,  /*!< Repeated "01010101" sequence */
} ble_gap_packet_payload_type_t;

/* BIG synchronization status */
typedef enum ble_gap_big_sync_status
{
    BLE_GAP_BIG_SYNC_STATUS_ESTABLISHED = 0,    /*!< BIG synchronization has been established */
    BLE_GAP_BIG_SYNC_STATUS_FAILED,             /*!< BIG synchronization has failed */
    BLE_GAP_BIG_SYNC_STATUS_CANCELLED,          /*!< BIG synchronization establishment has been cancelled */
    BLE_GAP_BIG_SYNC_STATUS_LOST,               /*!< BIG synchronization has been lost */
    BLE_GAP_BIG_SYNC_STATUS_PEER_TERMINATE,     /*!< BIG synchronization stopped due to peer termination */
    BLE_GAP_BIG_SYNC_STATUS_UPPER_TERMINATE,    /*!< BIG synchronization stopped due to upper layer termination */
    BLE_GAP_BIG_SYNC_STATUS_MIC_FAILURE,        /*!< BIG synchronization stopped due to encryption error */
} ble_gap_big_sync_status_t;

/* Bluetooth Low Energy address */
typedef struct
{
  uint8_t   addr_type;                  /*!< Address type, @ref ble_gap_addr_type_t */
  uint8_t   addr[BLE_GAP_ADDR_LEN];     /*!< BLE Address */
} ble_gap_addr_t;

/* BLE resolvable private address list item information */
typedef struct
{
    ble_gap_addr_t  addr;                           /*!< Peer device identity address */
    uint8_t         mode;                           /*!< Privacy mode @ref ble_gap_privacy_mode_t */
    uint8_t         peer_irk[BLE_GAP_KEY_LEN];      /*!< Peer IRK */
    uint8_t         local_irk[BLE_GAP_KEY_LEN];     /*!< Local IRK */
} ble_gap_ral_info_t;

/* BLE periodic advertising list item information */
typedef struct
{
    uint8_t     addr[BLE_GAP_ADDR_LEN];     /*!< BD Address of peer device */
    uint8_t     addr_type;                  /*!< Address type of peer device, @ref ble_gap_addr_type_t */
    uint8_t     adv_sid;                    /*!< Advertising SID */
} ble_gap_pal_info_t;

/* BLE advertising parameters */
typedef struct
{
    uint8_t         own_addr_type;  /*!< Own address type, @ref ble_gap_local_addr_type_t */
    uint8_t         type;           /*!< Advertising type, @ref ble_gap_adv_type_t */
    uint16_t        prop;           /*!< Advertising properties. @ref ble_gap_legacy_adv_prop_t for legacy advertising,
                                         @ref ble_gap_extended_adv_prop_t for extended advertising,
                                         @ref ble_gap_periodic_adv_prop_t for periodic advertising */
    uint8_t         disc_mode;      /*!< Discovery mode, @ref ble_gap_adv_discovery_mode_t */
    int8_t          max_tx_pwr;     /*!< Maximum power level to transmitted advertising packets */
    uint8_t         filter_pol;     /*!< Advertising filtering policy, @ref ble_gap_adv_filter_policy_t.*/
    ble_gap_addr_t  peer_addr;      /*!< Peer address only used in case of directed advertising */
    uint32_t        adv_intv_min;   /*!< Minimum advertising interval in unit of 625us */
    uint32_t        adv_intv_max;   /*!< Maximum advertising interval in unit of 625us */
    uint8_t         ch_map;         /*!< Advertisng channel map. Combination of @ref ble_gap_adv_chann_map_bf_t */
    uint8_t         primary_phy;    /*!< Indicate on which PHY primary advertising has to be performed, @ref ble_gap_phy_t */
    uint8_t         max_skip;       /*!< Maximum number of advertising events can skip before sending AUX_ADV_IND packets */
    uint8_t         secondary_phy;  /*!< Indicate on which PHY secondary advertising has to be performed, @ref ble_gap_phy_t */
    uint8_t         adv_sid;        /*!< Advertising SID */
    uint16_t        per_intv_min;   /*!< Minimum periodic advertising interval in unit of 1.25ms */
    uint16_t        per_intv_max;   /*!< Maximum periodic advertising interval in unit of 1.25ms */
    uint16_t        duration;       /*!< Advertising duration in unit of 10ms, 0 means continues advertising */
    uint8_t         max_adv_evt;    /*!< Maximum number of extended advertising events the controller shall attempt to send prior to terminating the extending advertising */
} ble_gap_adv_param_t;

/* BLE scan parameters */
typedef struct
{
    uint8_t     type;               /*!< Scan type, @ref ble_gap_scan_type_t */
    uint8_t     prop;               /*!< Scan properties, @ref ble_gap_scan_prop_t */
    uint8_t     dup_filt_pol;       /*!< Duplicate packet filtering policy */
    uint16_t    scan_intv_1m;       /*!< Scan interval in unit of 625us for 1M PHY */
    uint16_t    scan_win_1m;        /*!< Scan window in unit of 625us for 1M PHY */
    uint16_t    scan_intv_coded;    /*!< Scan interval in unit of 625us for Coded PHY */
    uint16_t    scan_win_coded;     /*!< Scan window in unit of 625us for Coded PHY */
    uint16_t    duration;           /*!< Scan duration in unit of 10ms, 0 means continuous scan */
    uint16_t    period;             /*!< Scan period in unit of 1.28s. Time interval betweem two consequent starts of scan duration */
} ble_gap_scan_param_t;

/* BLE connection parameters */
typedef struct
{
    uint16_t    conn_intv_min;      /*!< Minimum value for the connection interval in unit of 1.25ms. Allowed range is 7.5ms to 4s */
    uint16_t    conn_intv_max;      /*!< Maximum value for the connection interval in unit of 1.25ms. Allowed range is 7.5ms to 4s */
    uint16_t    conn_latency;       /*!< Slave latency. Number of events that can be missed by slave device */
    uint16_t    supv_tout;          /*!< Link supervision timeout in unit of 10ms. Allowed range is 100ms to 32s */
    uint16_t    ce_len_min;         /*!< Minimum duration of connection events in unit of 625us */
    uint16_t    ce_len_max;         /*!< Maximum duration of connection events in unit of 625us */
} ble_gap_conn_param_t;

/*!< @brief BLE initiating parameters */
typedef struct
{
    uint8_t              prop;              /*!< Initiating properties,  @ref ble_gap_init_prop_t */
    uint16_t             conn_tout;         /*!< Timeout for automatic connection establishment in unit of 10ms. 0 means there is no timeout.
                                                 Initiating will be canceled if indicated devices have not been connected when timeout occurs */
    uint16_t             scan_intv_1m;      /*!< Scan interval in unit of 625us for 1M PHY */
    uint16_t             scan_win_1m;       /*!< Scan window in unit of 625us for 1M PHY */
    uint16_t             scan_intv_coded;   /*!< Scan interval in unit of 625us for Coded PHY */
    uint16_t             scan_win_coded;    /*!< Scan window in unit of 625us for Coded PHY */
    ble_gap_conn_param_t conn_param_1m;     /*!< Connection parameters for 1M PHY */
    ble_gap_conn_param_t conn_param_2m;     /*!< Connection parameters for 2M PHY */
    ble_gap_conn_param_t conn_param_coded;  /*!< Connection parameters for Coded PHY */
} ble_gap_init_param_t;

/* BLE periodic synchronization parameters */
typedef struct
{
    uint16_t            skip;           /*!< Number of periodic advertising that can be skipped after a successful reception */
    uint16_t            sync_tout;      /*!< Synchronization timeout for the periodic advertising in unit of 10ms */
    uint8_t             type;           /*!< Periodic synchronization type,  @ref ble_gap_per_sync_type_t */
    uint8_t             conn_idx;       /*!< Connection index used for periodic sync info reception. Only valid for BLE_GAP_PER_SYNC_TYPE_PAST */
    ble_gap_pal_info_t  adv_addr;       /*!< Address of advertiser with which synchronization has to be established in case PAL is not used */
    uint8_t             report_en_bf;   /*!< Bit field that contains list of reports that are enabled or not, @ref ble_gap_per_sync_report_en_bf_t */
} ble_gap_per_sync_param_t;

/* Response of GAP reset */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
} ble_gap_reset_rsp_t;

/* Response of BLE disable */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
} ble_gap_disable_rsp_t;

/* Response of device configuration */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
} ble_gap_dev_cfg_rsp_t;

/* Response of set channel map */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
} ble_gap_chann_map_set_rsp_t;

/* Response of set local IRK */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
} ble_gap_irk_set_rsp_t;

/* Response of set local name */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
} ble_gap_name_set_rsp_t;

/* Local version information */
typedef struct
{
    uint8_t     hci_ver;        /*!< HCI version */
    uint8_t     lmp_ver;        /*!< LMP version */
    uint16_t    hci_subver;     /*!< HCI revision */
    uint16_t    lmp_subver;     /*!< LMP subversion */
    uint16_t    manuf_name;     /*!< Manufacturer name */
} ble_gap_local_ver_t;

/* Response of get local version */
typedef struct
{
    uint16_t             status;    /*!< Response status, @ref ble_status_t */
    ble_gap_local_ver_t  version;   /*!< Local version information */
} ble_gap_local_ver_get_rsp_t;

/* Response of get local identity address */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
} ble_gap_identity_addr_get_rsp_t;

/* Response of get advertising transmit power */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    int8_t      power_lvl;      /*!< Advertising transmit power level */
} ble_gap_adv_tx_pwr_get_rsp_t;

/* Suggested default data length information */
typedef struct
{
    uint16_t    sugg_max_tx_octets; /*!< Suggested value for maximum transmitted number of payload octets */
    uint16_t    sugg_max_tx_time;   /*!< Suggested value for maximum packet transmission time */
} ble_gap_sugg_dft_data_t;

/* Response of get suggested default data length */
typedef struct
{
    uint16_t                    status;         /*!< Response status, @ref ble_status_t */
    ble_gap_sugg_dft_data_t     sugg_dft_data;  /*!< Suggested default data length information */
} ble_gap_sugg_dft_data_get_rsp_t;

/* Maximum data length information */
typedef struct
{
    uint16_t    max_tx_octets;  /*!< Maximum number of payload octets supportted for transmission */
    uint16_t    max_tx_time;    /*!< Maximum time in microseconds supportted for transmission */
    uint16_t    max_rx_octets;  /*!< Maximum number of payload octets supportted for reception */
    uint16_t    max_rx_time;    /*!< Maximum time in microseconds supportted for reception */
} ble_gap_max_data_len_t;

/* Response of get max data length */
typedef struct
{
    uint16_t                  status;           /*!< Response status, @ref ble_status_t */
    ble_gap_max_data_len_t    max_data_len;     /*!< Maximum data length information */
} ble_gap_max_data_len_get_rsp_t;

/* Response of get available advertising sets number */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
    uint8_t     number;     /*!< Number of available advertising sets */
} ble_gap_adv_sets_num_get_rsp_t;

/* Response of get maximum advertising data length */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
    uint16_t    length;     /*!< Maximum advertising data length supported by controller */
} ble_gap_max_adv_data_len_get_rsp_t;

/* BLE transmit power range information */
typedef struct
{
    int8_t      min_tx_pwr;     /*!< Minimum TX power */
    int8_t      max_tx_pwr;     /*!< Maximum TX power */
} ble_gap_tx_pwr_range_t;

/* Response of get transmit power range */
typedef struct
{
    uint16_t                status;         /*!< Response status, @ref ble_status_t */
    ble_gap_tx_pwr_range_t  tx_pwr_range;   /*!< Transmit power range */
} ble_gap_tx_pwr_range_get_rsp_t;

/* Response of resolve a random address */
typedef struct
{
    uint16_t    status;                     /*!< Response status, @ref v */
    uint8_t     addr[BLE_GAP_ADDR_LEN];     /*!< Resolvable random address solved */
    uint8_t     irk[BLE_GAP_KEY_LEN];       /*!< IRK that correctly solved the random address */
} ble_gap_addr_resolve_rsp_t;

/* Response of generate a random address */
typedef struct
{
    uint16_t    status;                     /*!< Response status, @ref ble_status_t */
    uint8_t     addr[BLE_GAP_ADDR_LEN];     /*!< Generated random address */
} ble_gap_rand_addr_gen_rsp_t;

/* Response of generate a random number */
typedef struct
{
    uint16_t    status;                     /*!< Response status, @ref ble_status_t */
    uint8_t     number[BLE_GAP_AES_LEN];    /*!< Generated random number */
} ble_gap_rand_num_gen_rsp_t;

/* Response of compute a DH key */
typedef struct
{
    uint16_t    status;                     /*!< Response status, @ref ble_status_t */
    uint8_t     key[BLE_GAP_P256_KEY_LEN];  /*!< Compute DH key result */
} ble_gap_dh_key_compute_rsp_t;

/* Response of get public key */
typedef struct
{
    uint16_t    status;                             /*!< Response status, @ref ble_status_t */
    uint8_t     public_key_x[BLE_GAP_P256_KEY_LEN]; /*!< Public key X coordinate */
    uint8_t     public_key_y[BLE_GAP_P256_KEY_LEN]; /*!< Public key Y coordinate */
} ble_gap_public_key_get_rsp_t;

/* BLE GAP OOB data value */
typedef struct
{
    uint8_t     conf[BLE_GAP_KEY_LEN];      /*!< Confirm Value */
    uint8_t     rand[BLE_GAP_KEY_LEN];      /*!< Random Number */
} ble_gap_oob_data_t;

/* Response of generate OOB data */
typedef struct
{
    uint16_t            status;     /*!< Response status, @ref ble_status_t */
    ble_gap_oob_data_t  data;       /*!< Generated OOB data value */
} ble_gap_oob_data_gen_rsp_t;

/* Response of get a resolvable private address */
typedef struct
{
    uint16_t        status;     /*!< Response status, @ref ble_status_t */
    ble_gap_addr_t  addr;       /*!< Resolvable private address */
} ble_gap_ral_addr_get_rsp_t;

/* Response of get list size */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     size;           /*!< List size */
} ble_gap_list_size_get_rsp_t;

/* Response of set list */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
} ble_gap_list_set_rsp_t;

/* Local address information */
typedef struct
{
    ble_gap_addr_t  addr;       /*!< Bluetooth address of the device */
} ble_gap_local_addr_info_t;

/* Response of start transmit test */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
} ble_gap_test_tx_start_rsp_t;

/* Response of start receive test */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
} ble_gap_test_rx_start_rsp_t;

/* Response of stop transmit/receive test */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
} ble_gap_test_end_rsp_t;

/* Information of received packet number in receive test */
typedef struct
{
    uint16_t    rcv_pkt_num;    /*!< Number of received packets */
} ble_gap_test_rx_pkt_info_t;

/* Response of periodic synchronization report control */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     actv_idx;       /*!< Periodic synchronization index */
} ble_gap_per_sync_rpt_ctrl_rsp_t;

/* Advertising report type */
typedef struct
{
    uint8_t connectable   : 1;  /*!< Connectable advertising event type */
    uint8_t scannable     : 1;  /*!< Scannable advertising event type */
    uint8_t directed      : 1;  /*!< Directed advertising event type */
    uint8_t scan_response : 1;  /*!< Receive a scan response */
    uint8_t extended_pdu  : 1;  /*!< Receive an extended advertising data */
    uint8_t completed     : 1;  /*!< Receive a complete advertising pdu */
    uint8_t reserved      : 2;  /*!< Reserved for future use */
} ble_gap_adv_report_type_t;

/* Advertising report information */
typedef struct
{
    uint8_t                   actv_idx;         /*!< Scan or periodic synchronization index that receive the advertising data */
    ble_gap_adv_report_type_t type;             /*!< Advertising report type, @ref ble_gap_adv_report_type_t */
    ble_gap_addr_t            peer_addr;        /*!< Bluetooth address of the peer device */
    ble_gap_addr_t            direct_addr;      /*!< Target address of the advertising event if @ref ble_gap_adv_report_type_t::directed is set */
    uint8_t                   primary_phy;      /*!< Indicates the PHY on which the primary advertising packet was received, @ref ble_gap_phy_t */
    uint8_t                   secondary_phy;    /*!< Indicates the PHY on which the secondary advertising packet was received, @ref ble_gap_phy_t */
    int8_t                    tx_power;         /*!< TX power reported by the advertiser in the last packet header received */
    int8_t                    rssi;             /*!< Received Signal Strength Indication in dBm of the last packet received */
    uint8_t                   adv_sid;          /*!< Advertising set ID of the received advertising data */
    ble_data_t                data;             /*!< Received advertising or scan response or periodic advertising data */
    uint16_t                  period_adv_intv;  /*!< Periodic advertising interval in unit of 1.25ms, valid only for periodic advertising report */
} ble_gap_adv_report_info_t;

/* Periodic synchronization eatablished information */
typedef struct
{
    uint8_t         actv_idx;   /*!< Periodic synchronization index */
    uint8_t         phy;        /*!< Indicate on which PHY synchronization has been established, @ref ble_gap_phy */
    uint16_t        intv;       /*!< Periodic advertising interval in unit of 1.25ms */
    uint8_t         adv_sid;    /*!< Advertising SID */
    uint8_t         clk_acc;    /*!< Advertiser clock accuracy, @ref ble_gap_clock_accuracy_t */
    ble_gap_addr_t  addr;       /*!< Advertiser address */
    uint16_t        serv_data;  /*!< Service data, only valid for a Periodic Advertising Sync Transfer */
} ble_gap_per_sync_estab_info_t;

/* BIG information received in periodic advertising data */
typedef struct
{
    uint32_t    sdu_interval;   /*!< SDU interval in microseconds */
    uint16_t    iso_interval;   /*!< ISO Interval in unit of 1.25ms */
    uint16_t    max_pdu;        /*!< Maximum PDU size, range from 0x0000 to 0x00FB */
    uint16_t    max_sdu;        /*!< Maximum SDU size, range from 0x0000 to 0x0FFF */
    uint8_t     num_bis;        /*!< Number of BIS present in the group, range from 0x01 to 0x1F */
    uint8_t     nse;            /*!< Number of sub-events, range from 0x01 to 0x1F */
    uint8_t     bn;             /*!< Burst number, range from 0x01 to 0x07 */
    uint8_t     pto;            /*!< Pre-transmit offset, range from 0x00 to 0x0F */
    uint8_t     irc;            /*!< Initial retransmission count, range from 0x01 to 0x0F */
    uint8_t     phy;            /*!< PHY used for transmission, @ref ble_gap_phy_t */
    uint8_t     framing;        /*!< Framing mode, 0x00: Unframed, 0x01: Framed */
    bool        encrypted;      /*!< True if broadcast isochronous group is encrypted, otherwise false */
} ble_gap_big_info_t;

/* Peer name information */
typedef struct
{
    ble_gap_addr_t  addr;       /*!< Bluetooth address of peer device */
    uint8_t         name_len;   /*!< Peer device name length */
    uint8_t        *p_name;     /*!< Peer device name */
} ble_gap_peer_name_info_t;

/* BLE connection information */
typedef struct
{
    uint8_t         conn_idx;       /*!< Connection index */
    uint16_t        conn_hdl;       /*!< Connection handle */
    uint16_t        con_interval;   /*!< Connection interval */
    uint16_t        con_latency;    /*!< Connection latency */
    uint16_t        sup_to;         /*!< Link supervision timeout */
    uint8_t         clk_accuracy;   /*!< Clock accuracy */
    ble_gap_addr_t  peer_addr;      /*!< Bluetooth address of peer device */
    uint8_t         role;           /*!< Role of device in connection, 0: Central, 1: Peripheral */
    uint8_t         actv_idx;       /*!< Advertising or initiating local index */
} ble_gap_conn_info_t;

/* Response of disconnect BLE connection */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;       /*!< Connection index.*/
} ble_gap_disconn_rsp_t;

/* BLE connection disconnect information */
typedef struct
{
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    conn_hdl;       /*!< Connection handle */
    uint16_t    reason;         /*!< Disconnect reason */
} ble_gap_disconn_info_t;

/* Response of get peer name */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;   /*!< Connection index */
    uint16_t    handle;     /*!< Attribute handle */
    uint8_t     name_len;   /*!< Peer device name length */
    uint8_t    *p_name;     /*!< Peer device name */
} ble_gap_peer_name_get_rsp_t;

/* Response of get peer version */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    company_id;     /*!< Manufacturer name of peer device */
    uint8_t     lmp_version;    /*!< LMP version of peer device */
    uint16_t    lmp_subversion; /*!< LMP subversion of peer device */
} ble_gap_peer_ver_get_rsp_t;

/* Response of get peer features */
typedef struct
{
    uint16_t    status;                         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;                       /*!< Connection index */
    uint8_t     features[BLE_GAP_FEATURES_LEN]; /*!< Peer features */
} ble_gap_peer_feats_get_rsp_t;

/* Response of get peer appearance */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    handle;         /*!< Attribute handle */
    uint16_t    appearance;     /*!< Appearance value */
} ble_gap_peer_appearance_get_rsp_t;

/* Response of get peer slave prefer parameters attribute */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    handle;         /*!< Attribute handle */
    uint16_t    conn_intv_min;  /*!< Minimum connection interval */
    uint16_t    conn_intv_max;  /*!< Maximum connection interval */
    uint16_t    latency;        /*!< Connection latency */
    uint16_t    conn_timeout;   /*!< Connection supervision timeout */
} ble_gap_slave_prefer_param_get_rsp_t;

/* Response of get peer central address resolution attribute */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    handle;         /*!< Attribute handle */
    uint8_t     ctl_addr_resol; /*!< Central address resolution value */
} ble_gap_peer_addr_resol_get_rsp_t;

/* Response of get peer database hash attribute */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    handle;         /*!< Attribute handle */
    uint8_t     hash[16];       /*!< Database Hash value */
} ble_gap_peer_db_hash_get_rsp_t;

/* Response of get peer resolvable private address only attribute */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    handle;         /*!< Attribute handle */
    uint8_t     rpa_only;       /*!< Resolvable private address only value */
} ble_gap_peer_rpa_only_get_rsp_t;

/* Response of get RSSI of last received packet */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;       /*!< Connection index */
    int8_t      rssi;           /*!< RSSI value */
} ble_gap_rssi_get_rsp_t;

/* Response of get channel map */
typedef struct
{
    uint16_t    status;                             /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;                           /*!< Connection index */
    uint8_t     chann_map[BLE_GAP_CHANNEL_MAP_LEN]; /*!< Channel map value */
} ble_gap_chann_map_get_rsp_t;

/* Response of get ping timeout value */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    ping_tout;      /*!< Authenticated payload timeout value */
} ble_gap_ping_tout_get_rsp_t;

/* Response of set ping timeout value */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;       /*!< Connection index */
} ble_gap_ping_tout_set_rsp_t;

/* Ping timeout information */
typedef struct
{
    uint8_t     conn_idx;       /*!< Connection index */
} ble_gap_ping_tout_info_t;

/* Response of set packet size */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;       /*!< Connection index */
} ble_gap_pkt_size_set_rsp_t;

/* Packet size information */
typedef struct
{
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    max_tx_octets;  /*!< Maximum TX number of payload octets */
    uint16_t    max_tx_time;    /*!< Maximum TX time */
    uint16_t    max_rx_octets;  /*!< Maximum RX number of payload octets */
    uint16_t    max_rx_time;    /*!< Maximum RX time */
} ble_gap_pkt_size_info_t;

/* Response of get current used PHY */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;       /*!< Connection index */
} ble_gap_phy_get_rsp_t;

/* Response of set PHY */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;       /*!< Connection index */
} ble_gap_phy_set_rsp_t;

/* PHY information */
typedef struct
{
    uint8_t     conn_idx;       /*!< Connection index */
    uint8_t     tx_phy;         /*!< LE PHY for data transmission, @ref ble_gap_phy_t */
    uint8_t     rx_phy;         /*!< LE PHY for data reception, @ref ble_gap_phy_t */
} ble_gap_phy_info_t;

/* Response of local start connection parameter update procedure */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;       /*!< Connection index */
} ble_gap_conn_param_update_rsp_t;

/* Indication of peer start connection parameter update procedure */
typedef struct
{
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    intv_min;       /*!< Minimum connection interval */
    uint16_t    intv_max;       /*!< Maximum connection interval */
    uint16_t    latency;        /*!< Connection latency */
    uint16_t    supv_tout;      /*!< Supervision timeout */
} ble_gap_conn_param_update_ind_t;

/* Connection parameter update procedure result information */
typedef struct
{
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    interval;       /*!< Connection interval */
    uint16_t    latency;        /*!< Connection latency */
    uint16_t    supv_tout;      /*!< Supervision timeout */
} ble_gap_conn_param_info_t;

/* Indication of peer get name */
typedef struct
{
    uint8_t     conn_idx;           /*!< Connection index */
    uint16_t    token;              /*!< Token value that must be returned in confirmation */
    uint16_t    name_offset;        /*!< Name data offset */
    uint16_t    max_name_length;    /*!< Maximum name length starting from offset */
} ble_gap_name_get_ind_t;

/* Indication of peer get appearance */
typedef struct
{
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    token;          /*!< Token value that must be returned in confirmation */
} ble_gap_appearance_get_ind_t;

/* Indication of peer get slave prefer parameter attribute value */
typedef struct
{
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    token;          /*!< Token value that must be returned in confirmation */
} ble_gap_slave_prefer_param_get_ind_t;

/* Indication of peer set local device name */
typedef struct
{
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    token;          /*!< Token value that must be returned in confirmation */
    uint8_t     name_len;       /*!< Length of name to set */
    uint8_t    *p_name;         /*!< Pointer to name to set */
} ble_gap_name_set_ind_t;

/* Indication of peer set local appearance */
typedef struct
{
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    token;          /*!< Token value that must be returned in confirmation */
    uint16_t    appearance;     /*!< Appearance Icon */
} ble_gap_appearance_set_ind_t;

/* Response of periodic advertising sync transfer */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;       /*!< Connection index */
} ble_gap_per_adv_sync_trans_rsp_t;

/* BLE slave preferred connection parameters */
typedef struct
{
    uint16_t    conn_intv_min;  /*!< Minimum connection interval */
    uint16_t    conn_intv_max;  /*!< Maximum connection interval */
    uint16_t    latency;        /*!< Connection latency */
    uint16_t    conn_tout;      /*!< Connection supervision timeout */
} ble_gap_slave_prefer_param_t;

/* BLE pairing parameters used in pairing request/response */
typedef struct
{
    uint8_t iocap;          /*!< IO capabilities, @ref ble_gap_io_cap_t */
    bool    oob;            /*!< OOB flag */
    uint8_t auth;           /*!< Authentication request, @ref ble_gap_auth_t or combinaton of @ref ble_gap_auth_mask_t */
    uint8_t key_size;       /*!< Encryption key size range from 7 to 16 */
    uint8_t ikey_dist;      /*!< Initiator key distribution, combinaton of @ref ble_gap_key_dist_t */
    uint8_t rkey_dist;      /*!< Responder key distribution, combinaton of @ref ble_gap_key_dist_t  */
} ble_gap_pairing_param_t;

/* Response of local start bond procedure */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;   /*!< Connection index */
} ble_gap_bond_rsp_t;

/* Response of local start encryption procedure */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;   /*!< Connection index */
} ble_gap_encrypt_rsp_t;

/* Response of local start security */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;   /*!< Connection index */
} ble_gap_security_rsp_t;

/* Response of send key press notification */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;   /*!< Connection index */
} ble_gap_key_press_ntf_rsp_t;

/* Indication of peer pairing request */
typedef struct
{
    uint8_t     conn_idx;   /*!< Response status, @ref ble_status_t */
    uint8_t     authen_req; /*!< Authentication level (#ble_gap_auth)*/
} ble_gap_pairing_req_ind_t;

/* Indication of LTK request */
typedef struct
{
    uint8_t     conn_idx;   /*!< Connection index */
    uint8_t     key_size;   /*!< Requested LTK size */
} ble_gap_ltk_req_ind_t;

/* Indication of temp key request */
typedef struct
{
    uint8_t     conn_idx;   /*!< Connection index */
} ble_gap_tk_req_ind_t;

/* Indication of numeric comparison */
typedef struct
{
    uint8_t     conn_idx;       /*!< Connection index */
    uint32_t    numeric_value;  /*!< Numeric value */
} ble_gap_nc_ind_t;

/* Indication of IRK request */
typedef struct
{
    uint8_t     conn_idx;       /*!< Connection index */
} ble_gap_irk_req_ind_t;

/* Indication of CSRK request */
typedef struct
{
    uint8_t     conn_idx;       /*!< Connection index */
} ble_gap_csrk_req_ind_t;

/* Indication of OOB data request */
typedef struct
{
    uint8_t     conn_idx;       /*!< Connection index */
} ble_gap_oob_data_req_ind_t;

/* Information of pairing success */
typedef struct
{
    uint8_t     conn_idx;       /*!< Connection index */
    uint8_t     level;          /*!< Pairing level information, @ref ble_gap_pairing_level_t */
    bool        ltk_present;    /*!< LTK exchanged during pairing */
    bool        sc;             /*!< If this is a secure connection pairing */
} ble_gap_pairing_success_info_t;

/* Information of pairing fail */
typedef struct
{
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    reason;         /*!< Pairing failed reason, @ref ble_status_t */
} ble_gap_pairing_fail_info_t;

/* LTK data structure */
typedef struct
{
    uint8_t     ltk[BLE_GAP_KEY_LEN];               /*!< Long Term Key */
    uint16_t    ediv;                               /*!< Encryption Diversifier */
    uint8_t     rnd_num[BLE_GAP_RANDOM_NUMBER_LEN]; /*!< Random Number */
    uint8_t     key_size;                           /*!< Encryption key size range from 7 to 16 */
} ble_gap_ltk_t;

/* LTK information */
typedef struct
{
    uint8_t         conn_idx;   /*!< Connection index */
    ble_gap_ltk_t   data;       /*!< LTK information */
} ble_gap_ltk_info_t;

/* IRK data structure */
typedef struct
{
    uint8_t         irk[BLE_GAP_KEY_LEN];   /*!< Identity Resolving Key */
    ble_gap_addr_t  identity;               /*!< Device Identity Address */
} ble_gap_irk_t;

/* IRK information */
typedef struct
{
    uint8_t         conn_idx;   /*!< Connection index */
    ble_gap_irk_t   data;       /*!< IRK information */
} ble_gap_irk_info_t;

/* CSRK data structure */
typedef struct
{
    uint8_t         csrk[BLE_GAP_KEY_LEN];  /*!< Connection Signature Resolving Key */
} ble_gap_csrk_t;

/* CSRK information */
typedef struct
{
    uint8_t         conn_idx;   /*!< Connection index */
    ble_gap_csrk_t  data;       /*!< CSRK information */
} ble_gap_csrk_info_t;

/* Mask of keys in security bond data */
typedef enum ble_gap_key_mask
{
    BLE_KEYS_NONE       = 0x00,         /*!< No Keys */
    BLE_LOC_LTK_ENCKEY  = (1 << 0),     /*!< Local LTK present */
    BLE_PEER_LTK_ENCKEY = (1 << 1),     /*!< Peer LTK present */
    BLE_PEER_IDKEY      = (1 << 2),     /*!< Peer IRK present */
    BLE_LOC_CSRK        = (1 << 3),     /*!< Local CSRK present */
    BLE_PEER_CSRK       = (1 << 4),     /*!< Peer CSRK present */
} ble_gap_key_mask_t;

/* Bond data associated with peer device */
typedef struct
{
    uint8_t             pairing_lvl;        /*!< Pairing level, @ref ble_gap_pairing_level_t */
    bool                enc_key_present;    /*!< If LTK exchanged during pairing */
    ble_gap_key_mask_t  key_msk;            /*!< Key mask, @ref ble_gap_key_mask_t */
    ble_gap_irk_t       peer_irk;           /*!< Peer IRK information */
    ble_gap_ltk_t       peer_ltk;           /*!< Peer LTK information */
    ble_gap_ltk_t       local_ltk;          /*!< Local LTK information */
    ble_gap_csrk_t      peer_csrk;          /*!< Peer CSRK information */
    ble_gap_csrk_t      local_csrk;         /*!< Local CSRK information */
} ble_gap_sec_bond_data_t;

/* Key pressed information */
typedef struct
{
    uint8_t     conn_idx;   /*!< Connection index */
    uint8_t     type;       /*!< Key press notification type, @ref ble_gap_kp_ntf_type_t */
} ble_gap_key_pressed_info_t;

/* Information of peer security request */
typedef struct
{
    uint8_t     conn_idx;   /*!< Connection index */
    uint8_t     auth;       /*!< Authentication level, @ref ble_gap_auth_t */
} ble_gap_security_req_info_t;

/* Indicaiton of encryption request */
typedef struct
{
    uint8_t     conn_idx;                               /*!< Connection index.*/
    uint16_t    ediv;                                   /*!< Encryption Diversifier.*/
    uint8_t     rnd_num[BLE_GAP_RANDOM_NUMBER_LEN];     /*!< Random Number.*/
} ble_gap_encrypt_req_ind_t;

/* Encrypted information */
typedef struct
{
    uint8_t     conn_idx;       /*!< Connection index.*/
    uint8_t     pairing_lvl;    /*!< Pairing level, @ref ble_gap_pairing_level_t.*/
} ble_gap_encrypt_info_t;

/* Response of get local transmit power */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;       /*!< Connection index.*/
    uint8_t     phy;            /*!< PHY value, @ref ble_gap_phy_pwr_value_t */
    int8_t      tx_pwr;         /*!< Current transmit power level in dBm */
    int8_t      max_tx_pwr;     /*!< Max transmit power level in dBm */
} ble_gap_local_tx_pwr_get_rsp_t;

/* Response of get peer transmit power */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;       /*!< Connection index.*/
    uint8_t     phy;            /*!< PHY value, @ref ble_gap_phy_pwr_value_t */
    int8_t      tx_pwr;         /*!< Transmit power level in dBm */
    uint8_t     flags;          /*!< Transmit Power level flags, @ref ble_gap_pwr_ctrl_flags_t */
} ble_gap_peer_tx_pwr_get_rsp_t;

/* Response of control transmit power report */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;       /*!< Connection index.*/
} ble_gap_tx_pwr_report_ctrl_rsp_t;

/* Transmit power report information */
typedef struct
{
    uint8_t     conn_idx;       /*!< Connection index.*/
    uint8_t     phy;            /*!< PHY value, @ref ble_gap_phy_pwr_value_t */
    int8_t      tx_pwr;         /*!< Transmit power level in dBm */
    uint8_t     flags;          /*!< Transmit Power level flags, @ref ble_gap_pwr_ctrl_flags_t */
    int8_t      delta;          /*!< Power change delta value in dB */
} ble_gap_tx_pwr_report_info_t;

/* Response of control path loss */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;       /*!< Connection index.*/
} ble_gap_path_loss_ctrl_rsp_t;

/* BLE path loss threshold information */
typedef struct
{
    uint8_t     conn_idx;       /*!< Connection index.*/
    uint8_t     curr_path_loss; /*!< Current path loss value in dB.*/
    uint8_t     zone_entered;   /*!< Zone entered， @ref ble_gap_path_loss_zone_t */
} ble_gap_path_loss_threshold_info_t;

/* Response of register ISO interface */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
} ble_gap_iso_reg_rsp_t;

/* Response of ISO data path configuration */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
} ble_gap_iso_dp_cfg_rsp_t;

/* Response of ISO data path setup */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
} ble_gap_iso_dp_setup_rsp_t;

/* Response of ISO data path remove */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
} ble_gap_iso_dp_remove_rsp_t;

/* ISO data path update information */
typedef struct
{
    uint8_t     stream_lid;     /*!< Stream local index */
    uint8_t     dp_update;      /*!< Data path update type */
    uint8_t     direction;      /*!< Direction for setup update */
    uint16_t    status;         /*!< ISO data path status */
} ble_gap_iso_dp_update_info_t;

/* Response of ISO test start procedure */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     group_lid;      /*!< Group local index */
    uint8_t     stream_lid;     /*!< Stream local index */
} ble_gap_iso_test_start_rsp_t;

/* Response of get ISO test counter */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     group_lid;      /*!< Group local index */
    uint8_t     stream_lid;     /*!< Stream local index */
} ble_gap_iso_test_cnt_get_rsp_t;

/* ISO test counter information */
typedef struct
{
    uint8_t     stream_lid;     /*!< Stream local index */
    uint32_t    rx_pkt_num;     /*!< Number of received packets */
    uint32_t    miss_pkt_num;   /*!< Number of missed packets */
    uint32_t    fail_pkt_num;   /*!< Number of failed packets */
} ble_gap_iso_test_cnt_info_t;

/* Response of ISO test stop procedure */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     group_lid;      /*!< Group local index */
    uint8_t     stream_lid;     /*!< Stream local index */
} ble_gap_iso_test_stop_rsp_t;

/* Response of Add a CIG */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     cig_id;         /*!< CIG ID */
    uint8_t     group_lid;      /*!< Allocated Group local index */
} ble_gap_cig_add_rsp_t;

/* Response of update a CIG */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     group_lid;      /*!< Group local index */
} ble_gap_cig_update_rsp_t;

/* Response of remove a CIG */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     group_lid;      /*!< Group local index */
} ble_gap_cig_remove_rsp_t;

/* Response of configure a CIS */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     group_lid;      /*!< Group local index */
    uint8_t     cis_id;         /*!< CIS ID */
    uint8_t     stream_lid;     /*!< Allocated Stream local index */
} ble_gap_cis_cfg_rsp_t;

/* Response of prepare a CIS */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     conn_idx;       /*!< Connection index */
    uint8_t     cig_id;         /*!< CIG ID */
    uint8_t     cis_id;         /*!< CIS ID */
    uint8_t     group_lid;      /*!< Group local index */
    uint8_t     stream_lid;     /*!< Stream local index */
} ble_gap_cis_prepare_rsp_t;

/* Response of bind a CIS */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
} ble_gap_cis_bind_rsp_t;

/* Response of release a CIS */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
} ble_gap_cis_release_rsp_t;

/* Response of connect a CIS */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     group_lid;      /*!< Group local index */
    uint8_t     stream_lid;     /*!< Stream local index */
} ble_gap_cis_conn_rsp_t;

/* CIS connection information */
typedef struct
{
    uint8_t     group_lid;          /*!< Group local index */
    uint8_t     stream_lid;         /*!< Stream local index */
    uint32_t    sync_delay_us;      /*!< Group synchronization delay time in microseconds */
    uint32_t    tlatency_m2s_us;    /*!< Maximum time in microseconds for transmission of SDUs of all CISes from master to slave */
    uint32_t    tlatency_s2m_us;    /*!< Maximum time in microseconds for transmission of SDUs of all CISes from slave to master */
    uint16_t    iso_intv_frames;    /*!< ISO interval in 1.25ms unit */
    uint16_t    max_pdu_m2s;        /*!< Maximum size in octets of the payload from master to slave */
    uint16_t    max_pdu_s2m;        /*!< Maximum size in octets of the payload from slave to master */
    uint8_t     phy_m2s;            /*!< Master to slave PHY, @ref ble_gap_phy_bf_t */
    uint8_t     phy_s2m;            /*!< Slave to master PHY, @ref ble_gap_phy_bf_t */
    uint8_t     bn_m2s;             /*!< The burst number for master to slave transmission */
    uint8_t     bn_s2m;             /*!< The burst number for slave to master transmission */
    uint8_t     ft_m2s;             /*!< The flush timeout in multiples of ISO interval for each payload sent from master to slave */
    uint8_t     ft_s2m;             /*!< The flush timeout in multiples of ISO interval for each payload sent from slave to master*/
    uint8_t     nse;                /*!< Maximum number of subevents in each isochronous interval */
} ble_gap_cis_conn_info_t;

/* Response of disconnect a CIS */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     group_lid;      /*!< Group local index */
    uint8_t     stream_lid;     /*!< Stream local index */
} ble_gap_cis_disconn_rsp_t;

/* CIS disconnect information */
typedef struct
{
    uint8_t     stream_lid;     /*!< Stream local index */
    uint8_t     reason;         /*!< Disconnect reason */
} ble_gap_cis_disconn_info_t;

/* Response of add a BIG */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     big_handle;     /*!< BIG Handle */
    uint8_t     stream_num;     /*!< Number of streams in the group */
    uint8_t     group_lid;      /*!< Allocated group local index */
    uint8_t    *p_stream_lid;   /*!< List of allocated stream local index */
} ble_gap_big_add_rsp_t;

/* Response of enable a BIG */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     group_lid;      /*!< Group local index */
} ble_gap_big_enable_rsp_t;

/* Response of start BIG synchronization */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     group_lid;      /*!< Group local index */
} ble_gap_big_sync_rsp_t;

/* BIG synchronization status information */
typedef struct
{
    uint8_t     group_lid;      /*!< Group local index */
    uint8_t     status;         /*!< BIG sync status, @ref ble_gap_big_sync_status_t */
    uint32_t    tlatency_us;    /*!< Maximum delay time in microseconds for transmission of SDUs of all BISes */
    uint16_t    iso_intv;       /*!< ISO interval in 1.25ms uint */
    uint8_t     nse;            /*!< Number of subevents in each BIS event in the BIG */
    uint8_t     bn;             /*!< Number of new payloads in each BIS event */
    uint8_t     pto;            /*!< Offset used for pre-transmissions */
    uint8_t     irc;            /*!< The number of times a payload is transmitted in a BIS event */
    uint8_t     max_pdu;        /*!< Maximum size of the payload in octets */
    uint8_t     bis_num;        /*!< Number of BIS in the BIG */
    uint16_t    *p_conn_hdl;    /*!< List of connection handle values provided by the Controller */
} ble_gap_big_sync_status_info_t;

/* BIG synchronization added information */
typedef struct
{
    uint8_t     big_handle;     /*!< BIG handle */
    uint8_t     stream_num;     /*!< Number of streams to synchronize with */
    uint8_t     group_lid;      /*!< Group local index */
    uint8_t    *p_stream_lid;   /*!< List of stream local index */
} ble_gap_big_sync_added_info_t;

/* BIG create information */
typedef struct
{
    uint8_t     group_lid;      /*!< Group local index */
    uint32_t    sync_delay_us;  /*!< Transmission delay time in microseconds of all BISs in the BIG */
    uint32_t    tlatency_us;    /*!< Maximum delay time in microseconds for transmission of SDUs of all BISes */
    uint16_t    iso_intv;       /*!< ISO interval in 1.25ms uint */
    uint8_t     nse;            /*!< Number of subevents in each BIS event in the BIG */
    uint8_t     bn;             /*!< Number of new payloads in each BIS event */
    uint8_t     pto;            /*!< Offset used for pre-transmissions */
    uint8_t     irc;            /*!< Number of times a payload is transmitted in a BIS event */
    uint8_t     max_pdu;        /*!< Maximum size of the payload in octets */
    uint8_t     phy;            /*!< PHY value, @ref ble_gap_phy_t */
    uint8_t     bis_num;        /*!< Number of BISes */
    uint16_t   *p_conn_hdl;     /*!< List of Connection Handle values provided by the Controller */
} ble_gap_big_create_info_t;

/* Response of disable a BIG */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     group_lid;      /*!< Group local index */
} ble_gap_big_disable_rsp_t;

/* Response of remove a BIG */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     group_lid;      /*!< Group local index */
} ble_gap_big_remove_rsp_t;

/* Parameters used to create a BIG */
typedef struct
{
    uint32_t    sdu_intv_us;        /*!< SDU interval in microseconds */
    uint16_t    max_sdu;            /*!< Maximum size of an SDU */
    uint8_t     packing;            /*!< Sequential or interleaved scheduling */
    uint8_t     framing;            /*!< Unframed or framed mode */
    uint8_t     phy_bf;             /*!< Bitfield indicating PHYs than can be used, @ref ble_gap_phy_bf_t */
    uint16_t    max_tlatency_ms;    /*!< Maximum time in milliseconds between the first transmission of an SDU to the end of the last transmission of the same SDU */
    uint8_t     rtn;                /*!< Number of times every PDU should be retransmitted */
} ble_gap_big_param_t;

/* Parameters used to create a BIG using test command */
typedef struct
{
    uint32_t    sdu_intv_us;    /*!< SDU interval in microseconds */
    uint16_t    max_sdu;        /*!< Maximum size of an SDU */
    uint8_t     packing;        /*!< Sequential or interleaved scheduling */
    uint8_t     framing;        /*!< Unframed or framed mode */
    uint8_t     phy_bf;         /*!< Bitfield indicating PHYs than can be used, @ref ble_gap_phy_bf_t */
    uint16_t    iso_intv;       /*!< ISO interval in 1.25ms unit */
    uint8_t     nse;            /*!< Number of subevents in each interval of each stream in the group */
    uint8_t     max_pdu;        /*!< Maximum size of a PDU */
    uint8_t     bn;             /*!< Burst number (number of new payload in each interval) */
    uint8_t     irc;            /*!< Number of times the scheduled payload is transmitted in a given event */
    uint8_t     pto;            /*!< Offset used for pre-transmissions */
} ble_gap_big_test_param_t;

/* Parameters used to create a CIG */
typedef struct
{
    uint32_t    sdu_intv_m2s_us;    /*!< SDU interval from master to slave in microseconds */
    uint32_t    sdu_intv_s2m_us;    /*!< SDU interval from slave to master in microseconds */
    uint8_t     packing;            /*!< Sequential or interleaved scheduling */
    uint8_t     framing;            /*!< Unframed or framed mode */
    uint8_t     sca;                /*!< Worst slow clock accuracy of slaves */
    uint16_t    latency_m2s_ms;     /*!< Maximum time in milliseconds for SDU transported from master to slave */
    uint16_t    latency_s2m_ms;     /*!< Maximum time in milliseconds for SDU transported from slave to master */
} ble_gap_cig_param_t;

/* Parameters used to create a CIG using test command */
typedef struct
{
    uint32_t    sdu_intv_m2s_us;    /*!< SDU interval from master to slave in microseconds */
    uint32_t    sdu_intv_s2m_us;    /*!< SDU interval from slave to master in microseconds */
    uint8_t     packing;            /*!< Sequential or interleaved scheduling */
    uint8_t     framing;            /*!< Unframed or framed mode */
    uint8_t     sca;                /*!< Worst slow clock accuracy of slaves */
    uint16_t    ft_m2s_ms;          /*!< Flush timeout in milliseconds for each payload sent from master to slave */
    uint16_t    ft_s2m_ms;          /*!< Flush timeout in milliseconds for each payload sent from slave to master */
    uint16_t    iso_intv;           /*!< ISO interval in 1.25ms unit */
} ble_gap_cig_test_param_t;

/* Parameters used to connect a CIS */
typedef struct
{
    uint16_t    max_sdu_m2s;    /*!< Maximum size of an SDU provided by master host */
    uint16_t    max_sdu_s2m;    /*!< Maximum size of an SDU provided by slave host */
    uint8_t     phy_m2s;        /*!< PHYs on which packets may be transmitted from master to slave, @ref ble_gap_phy_bf_t */
    uint8_t     phy_s2m;        /*!< PHYs on which packets may be transmitted from slave to master, @ref ble_gap_phy_bf_t */
    uint8_t     rtn_m2s;        /*!< Maximum number of times every PDU should be retransmitted for master to slave */
    uint8_t     rtn_s2m;        /*!< Maximum number of times every PDU should be retransmitted for slave to master */
} ble_gap_cis_param_t;

/* Parameters used to connect a CIS using test command */
typedef struct
{
    uint16_t    max_sdu_m2s;    /*!< Maximum size of an SDU provided by master host */
    uint16_t    max_sdu_s2m;    /*!< Maximum size of an SDU provided by slave host */
    uint8_t     phy_m2s;        /*!< PHYs on which packets may be transmitted from master to slave, @ref ble_gap_phy_bf_t */
    uint8_t     phy_s2m;        /*!< PHYs on which packets may be transmitted from slave to master, @ref ble_gap_phy_bf_t */
    uint8_t     max_pdu_m2s;    /*!< Maximum size of the payload from master to slave */
    uint8_t     max_pdu_s2m;    /*!< Maximum size of the payload from slave to master */
    uint8_t     bn_m2s;         /*!< Burst number from master to slave */
    uint8_t     bn_s2m;         /*!< Burst number from slave to master */
    uint8_t     nse;            /*!< Maximum number of subevents */
} ble_gap_cis_test_param_t;

/*!
    \brief      Function to check if the RPA can be resolved by the provided IRK
    \param[in]  rpa: Resolvable Private Address
    \param[in]  irk: IRK value
    \param[out] none
    \retval     bool: true if the RPA can be resolved by the provided IRK, otherwise false
*/
bool ble_gap_rpa_matches_irk(uint8_t rpa[6], uint8_t irk[16]);

#ifdef __cplusplus
}
#endif

#endif // _BLE_GAP_H_
