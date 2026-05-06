/*!
    \file    ble_gatt.h
    \brief   Definitions and prototypes for the BLE GATT interface.

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

#ifndef _BLE_GATT_H_
#define _BLE_GATT_H_

#include <stdint.h>
#include <stdbool.h>

#include "arch.h"

#include "ble_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Minimal LE MTU valu */
#define BLE_GATT_MTU_MIN        (23)

/* Maximum number of handle that can be simultaneously read */
#define BLE_GATT_RD_MULTIPLE_MAX_NB_ATTR    8

/* Invalid attribute index */
#define BLE_GATT_INVALID_IDX        0xFF

/* Invalid attribute handle */
#define BLE_GATT_INVALID_HDL        0x0000

/* Minimum attribute handle */
#define BLE_GATT_MIN_HDL            0x0001
/* Maximum attribute handle */
#define BLE_GATT_MAX_HDL            0xFFFF

/* Length of an attribute handle */
#define BLE_GATT_HANDLE_LEN         2

/* Length of an attribute header(opcode + handle) */
#define BLE_GATT_HEADER_LEN         (1 + BLE_GATT_HANDLE_LEN)

/* Length of 16-bit UUID in octets */
#define BLE_GATT_UUID_16_LEN        2
/* Length of 32-bit UUID in octets */
#define BLE_GATT_UUID_32_LEN        4
/* Length of 128-bit UUID in octets */
#define BLE_GATT_UUID_128_LEN       16

/* Length of CCCD */
#define BLE_GATT_CCCD_LEN           2

/* Length of Database Hash in octets */
#define BLE_GATT_DB_HASH_LEN        16

/* Length of CCCD */
#define BLE_GATT_CCCD_LEN           2

/* Change 16-bit UUID to LSB type */
#if (!CPU_LE)
#define BLE_GATT_UUID_16_LSB(uuid)      (((uuid & 0xFF00) >> 8) | ((uuid & 0x00FF) << 8))
#else
#define BLE_GATT_UUID_16_LSB(uuid)      (uuid)
#endif

/* Put 16 bits attributes in a 16 bits array */
#define UUID_16BIT_TO_ARRAY(uuid16_bit) {(uuid16_bit & 0xff), ((uuid16_bit & 0xff00) >> 8)}

/* Put 16 bits attributes in a 128 bits array */
#define ATT_16_TO_128_ARRAY(uuid) { (uuid) & 0xFF, (uuid >> 8) & 0xFF, 0,0,0,0,0,0,0,0,0,0,0,0,0,0 }

/*!
    \brief      Helper to define an attribute property
    \param[in]  prop: property @ref ble_gatt_attr_info_bf_t
*/
#define PROP(prop)          (BLE_GATT_ATTR_##prop##_BIT)

/*!
    \brief      Helper to define an attribute option bit
    \param[in]  opt: options @ref ble_gatt_attr_info_bf_t or @ref ble_gatt_attr_ext_info_bf
*/
#define OPT(opt)            (BLE_GATT_ATTR_##opt##_BIT)

/*!
    \brief      Helper to set service UUID type in GATT service info
    \param[in]  uuid_type: UUID type, should be one of (16, 32, 128)
*/
#define SVC_UUID(uuid_type)      (((BLE_GATT_UUID_##uuid_type) << (BLE_GATT_SVC_UUID_TYPE_LSB)) & (BLE_GATT_SVC_UUID_TYPE_MASK))

/*!
    \brief      Helper to set service security level in GATT service info
    \param[in]  lvl_val: security level, @ref ble_gap_sec_lvl_t
*/
#define SVC_SEC_LVL_VAL(lvl_val)    (((lvl_val) << (BLE_GATT_SVC_SEC_LVL_LSB)) & (BLE_GATT_SVC_SEC_LVL_MASK))

/*!
    \brief      Helper to set attribute UUID type in GATT attribute info
    \param[in]  uuid_type: UUID type, should be one of (16, 32, 128)
*/
#define ATT_UUID(uuid_type)      (((BLE_GATT_UUID_##uuid_type) << (BLE_GATT_ATTR_UUID_TYPE_LSB)) & (BLE_GATT_ATTR_UUID_TYPE_MASK))

/*!
    \brief      Helper to set attribute security level on a specific permission in GATT attribute info
    \param[in]  lvl_name: security level, @ref ble_gap_sec_lvl_t
    \param[in]  perm: permission, @ref ble_gatt_attr_info_bf_t (only RP, WP, NIP authorized)
*/
#define SEC_LVL(perm, lvl_name)  (((BLE_GAP_SEC_##lvl_name) << (BLE_GATT_ATTR_##perm##_LSB)) & (BLE_GATT_ATTR_##perm##_MASK))

/* BLE GATT role */
enum ble_gatt_role
{
    BLE_GATT_ROLE_CLIENT = 0x00,    /*!< GATT client role */
    BLE_GATT_ROLE_SERVER = 0x01,    /*!< GATT server role */
    BLE_GATT_ROLE_NONE   = 0xFF,    /*!< Role not defined */
};

/* BLE GATT UUID type */
enum ble_gatt_uuid_type
{
    BLE_GATT_UUID_16      = 0x00,   /*!< 16-bit UUID */
    BLE_GATT_UUID_32      = 0x01,   /*!< 32-bit UUID */
    BLE_GATT_UUID_128     = 0x02,   /*!< 128-bit UUID */
    BLE_GATT_UUID_INVALID = 0x03,   /*!< Invalid UUID Type */
};

/* BLE GATT CCCD value bit field */
enum ble_gatt_cccd_bf
{
    BLE_GATT_CCCD_NTF_BIT = (0x0001 << 0),  /*!< Notification bit in CCCD value */
    BLE_GATT_CCCD_IND_BIT = (0x0001 << 1),  /*!< Indication bit in CCCD value */
};

/* GATT service information bit field
 *    7      6     5     4      3     2    1   0
 *  +-----+-----+-----+------+-----+-----+---+---+
 *  | RFU | UUID_TYPE | HIDE | DIS | EKS |SEC_LVL|
 *  +-----+-----+-----+------+-----+-----+---+---+
 */
enum ble_gatt_svc_info_bf
{
    BLE_GATT_SVC_SEC_LVL_MASK   = 0x03,     /*!< Service minimum required security level, @ref ble_gap_sec_lvl_t */
    BLE_GATT_SVC_SEC_LVL_LSB    = 0,
    BLE_GATT_SVC_EKS_BIT        = 0x04,     /*!< If set, access to value with encrypted security requirement also requires 128-bit encryption key size */
    BLE_GATT_SVC_EKS_POS        = 2,
    BLE_GATT_SVC_DIS_BIT        = 0x08,     /*!< If set, service is visible but cannot be used by peer device */
    BLE_GATT_SVC_DIS_POS        = 3,
    BLE_GATT_SVC_HIDE_BIT       = 0x10,     /*!< If set, hide the service */
    BLE_GATT_SVC_HIDE_POS       = 4,
    BLE_GATT_SVC_UUID_TYPE_MASK = 0x60,     /*!< Type of service UUID,  @ref ble_gatt_uuid_type */
    BLE_GATT_SVC_UUID_TYPE_LSB  = 5,
};

/* GATT attribute information bit field
 *     15   14    13  12 11 10  9  8   7    6    5   4   3    2    1    0
 *  +-----+-----+---+---+--+--+--+--+-----+----+---+---+----+----+----+---+
 *  | UUID_TYPE |  NIP  |  WP |  RP | EXT | WS | I | N | WR | WC | RD | B |
 *  +-----+-----+---+---+--+--+--+--+-----+----+---+---+----+----+----+---+
 */
typedef enum ble_gatt_attr_info_bf
{
    BLE_GATT_ATTR_BC_BIT            = 0x0001,   /*!< Broadcast descriptor present */
    BLE_GATT_ATTR_BC_POS            = 0,
    BLE_GATT_ATTR_RD_BIT            = 0x0002,   /*!< Read Access Mask */
    BLE_GATT_ATTR_RD_POS            = 1,
    BLE_GATT_ATTR_WC_BIT            = 0x0004,   /*!< Write Command Enabled attribute Mask */
    BLE_GATT_ATTR_WC_POS            = 2,
    BLE_GATT_ATTR_WR_BIT            = 0x0008,   /*!< Write Request Enabled attribute Mask */
    BLE_GATT_ATTR_WR_POS            = 3,
    BLE_GATT_ATTR_NTF_BIT           = 0x0010,   /*!< Notification Access Mask */
    BLE_GATT_ATTR_NTF_POS           = 4,
    BLE_GATT_ATTR_IND_BIT           = 0x0020,   /*!< Indication Access Mask */
    BLE_GATT_ATTR_IND_POS           = 5,
    BLE_GATT_ATTR_WS_BIT            = 0x0040,   /*!< Write Signed Enabled attribute Mask */
    BLE_GATT_ATTR_WS_POS            = 6,
    BLE_GATT_ATTR_EXT_BIT           = 0x0080,   /*!< Extended properties descriptor present */
    BLE_GATT_ATTR_EXT_POS           = 7,
    BLE_GATT_ATTR_RP_MASK           = 0x0300,   /*!< Read security level permission, @ref ble_gap_sec_lvl_t */
    BLE_GATT_ATTR_RP_LSB            = 8,
    BLE_GATT_ATTR_WP_MASK           = 0x0C00,   /*!< Write security level permission, @ref ble_gap_sec_lvl_t */
    BLE_GATT_ATTR_WP_LSB            = 10,
    BLE_GATT_ATTR_NIP_MASK          = 0x3000,   /*!< Notify and Indication security level permission, @ref ble_gap_sec_lvl_t */
    BLE_GATT_ATTR_NIP_LSB           = 12,
    BLE_GATT_ATTR_UUID_TYPE_MASK    = 0xC000,   /*!< Type of attribute UUID, @ref ble_gatt_uuid_type */
    BLE_GATT_ATTR_UUID_TYPE_LSB     = 14,
}ble_gatt_attr_info_bf_t;

/* GATT attribute extended information bit field */
enum ble_gatt_attr_ext_info_bf
{
    BLE_GATT_ATTR_WRITE_MAX_SIZE_MASK   = 0x7FFF,   /*!< Maximum value authorized for an attribute write */
    BLE_GATT_ATTR_WRITE_MAX_SIZE_LSB    = 0,
    BLE_GATT_ATTR_NO_OFFSET_BIT         = 0x8000,   /*!< 1: Do not authorize peer device to read or write an attribute with an offset != 0.
                                                         0: Authorize offset usage */
    BLE_GATT_ATTR_NO_OFFSET_POS         = 15,
    BLE_GATT_INC_SVC_HDL_BIT            = 0xFFFF,   /*!< Include Service handle value */
    BLE_GATT_INC_SVC_HDL_POS            = 0,
    BLE_GATT_ATTR_EXT_PROP_VALUE_MASK   = 0xFFFF,   /*!< Characteristic Extended Properties value */
    BLE_GATT_ATTR_EXT_PROP_VALUE_LSB    = 0,
};

/* GATT service discovery information */
enum ble_gatt_svc_disc_info
{
    BLE_GATT_SVC_CMPL  = 0x00,      /*!< Complete service present */
    BLE_GATT_SVC_START = 0x01,      /*!< First service attribute present */
    BLE_GATT_SVC_END   = 0x02,      /*!< Last service attribute present */
    BLE_GATT_SVC_CONT  = 0x03,      /*!< Following service attribute present */
};

/* GATT attribute type */
enum ble_gatt_attr_type
{
    BLE_GATT_ATTR_NONE          = 0x00,     /*!< No Attribute Information */
    BLE_GATT_ATTR_PRIMARY_SVC   = 0x01,     /*!< Primary service attribute */
    BLE_GATT_ATTR_SECONDARY_SVC = 0x02,     /*!< Secondary service attribute */
    BLE_GATT_ATTR_INCL_SVC      = 0x03,     /*!< Included service attribute */
    BLE_GATT_ATTR_CHAR          = 0x04,     /*!< Characteristic declaration */
    BLE_GATT_ATTR_VAL           = 0x05,     /*!< Attribute value */
    BLE_GATT_ATTR_DESC          = 0x06,     /*!< Attribute descriptor */
};

/* GATT event type */
typedef enum ble_gatt_evt_type
{
    BLE_GATT_NOTIFY   = 0x00,       /*!< Server initiated notification */
    BLE_GATT_INDICATE = 0x01,       /*!< Server initiated indication */
} ble_gatt_evt_type_t;

/* GATT 16-bit Universal Unique Identifier */
enum ble_gatt_char_16
{
    BLE_GATT_INVALID_UUID               = BLE_GATT_UUID_16_LSB(0x0000), /*!< Invalid UUID */

    /*----------------- SERVICES ---------------------*/
    BLE_GATT_SVC_GENERIC_ACCESS         = BLE_GATT_UUID_16_LSB(0x1800), /*!< Generic Access Profile */
    BLE_GATT_SVC_GENERIC_ATTRIBUTE      = BLE_GATT_UUID_16_LSB(0x1801), /*!< Attribute Profile */
    BLE_GATT_SVC_IMMEDIATE_ALERT        = BLE_GATT_UUID_16_LSB(0x1802), /*!< Immediate alert Service */
    BLE_GATT_SVC_LINK_LOSS              = BLE_GATT_UUID_16_LSB(0x1803), /*!< Link Loss Service */
    BLE_GATT_SVC_TX_POWER               = BLE_GATT_UUID_16_LSB(0x1804), /*!< Tx Power Service */
    BLE_GATT_SVC_CURRENT_TIME           = BLE_GATT_UUID_16_LSB(0x1805), /*!< Current Time Service Service */
    BLE_GATT_SVC_REF_TIME_UPDATE        = BLE_GATT_UUID_16_LSB(0x1806), /*!< Reference Time Update Service */
    BLE_GATT_SVC_NEXT_DST_CHANGE        = BLE_GATT_UUID_16_LSB(0x1807), /*!< Next DST Change Service */
    BLE_GATT_SVC_GLUCOSE                = BLE_GATT_UUID_16_LSB(0x1808), /*!< Glucose Service */
    BLE_GATT_SVC_HEALTH_THERMOM         = BLE_GATT_UUID_16_LSB(0x1809), /*!< Health Thermometer Service */
    BLE_GATT_SVC_DEVICE_INFO            = BLE_GATT_UUID_16_LSB(0x180A), /*!< Device Information Service */
    BLE_GATT_SVC_HEART_RATE             = BLE_GATT_UUID_16_LSB(0x180D), /*!< Heart Rate Service */
    BLE_GATT_SVC_PHONE_ALERT_STATUS     = BLE_GATT_UUID_16_LSB(0x180E), /*!< Phone Alert Status Service */
    BLE_GATT_SVC_BATTERY_SERVICE        = BLE_GATT_UUID_16_LSB(0x180F), /*!< Battery Service */
    BLE_GATT_SVC_BLOOD_PRESSURE         = BLE_GATT_UUID_16_LSB(0x1810), /*!< Blood Pressure Service */
    BLE_GATT_SVC_ALERT_NTF              = BLE_GATT_UUID_16_LSB(0x1811), /*!< Alert Notification Service */
    BLE_GATT_SVC_HID                    = BLE_GATT_UUID_16_LSB(0x1812), /*!< HID Service */
    BLE_GATT_SVC_SCAN_PARAMETERS        = BLE_GATT_UUID_16_LSB(0x1813), /*!< Scan Parameters Service */
    BLE_GATT_SVC_RUNNING_SPEED_CADENCE  = BLE_GATT_UUID_16_LSB(0x1814), /*!< Running Speed and Cadence Service */
    BLE_GATT_SVC_CYCLING_SPEED_CADENCE  = BLE_GATT_UUID_16_LSB(0x1816), /*!< Cycling Speed and Cadence Service */
    BLE_GATT_SVC_CYCLING_POWER          = BLE_GATT_UUID_16_LSB(0x1818), /*!< Cycling Power Service */
    BLE_GATT_SVC_LOCATION_AND_NAVIGATION = BLE_GATT_UUID_16_LSB(0x1819), /*!< Location and Navigation Service */
    BLE_GATT_SVC_ENVIRONMENTAL_SENSING  = BLE_GATT_UUID_16_LSB(0x181A), /*!< Environmental Sensing Service */
    BLE_GATT_SVC_BODY_COMPOSITION       = BLE_GATT_UUID_16_LSB(0x181B), /*!< Body Composition Service */
    BLE_GATT_SVC_USER_DATA              = BLE_GATT_UUID_16_LSB(0x181C), /*!< User Data Service */
    BLE_GATT_SVC_WEIGHT_SCALE           = BLE_GATT_UUID_16_LSB(0x181D), /*!< Weight Scale Service */
    BLE_GATT_SVC_BOND_MANAGEMENT        = BLE_GATT_UUID_16_LSB(0x181E), /*!< Bond Management Service */
    BLE_GATT_SVC_CONTINUOUS_GLUCOSE_MONITORING = BLE_GATT_UUID_16_LSB(0x181F), /*!< Continuous Glucose Monitoring Service */
    BLE_GATT_SVC_IP_SUPPORT             = BLE_GATT_UUID_16_LSB(0x1820), /*!< Internet Protocol Support Service */
    BLE_GATT_SVC_INDOOR_POSITIONING     = BLE_GATT_UUID_16_LSB(0x1821), /*!< Indoor Positioning Service */
    BLE_GATT_SVC_PULSE_OXIMETER         = BLE_GATT_UUID_16_LSB(0x1822), /*!< Pulse Oximeter Service */
    BLE_GATT_SVC_HTTP_PROXY             = BLE_GATT_UUID_16_LSB(0x1823), /*!< HTTP Proxy Service */
    BLE_GATT_SVC_TRANSPORT_DISCOVERY    = BLE_GATT_UUID_16_LSB(0x1824), /*!< Transport Discovery Service */
    BLE_GATT_SVC_OBJECT_TRANSFER        = BLE_GATT_UUID_16_LSB(0x1825), /*!< Object Transfer Service */

    BLE_GATT_SVC_MESH_PROVISIONING      = BLE_GATT_UUID_16_LSB(0x1827), /*!< Mesh Provisioning Service */
    BLE_GATT_SVC_MESH_PROXY             = BLE_GATT_UUID_16_LSB(0x1828), /*!< Mesh Proxy Service */

    BLE_GATT_SVC_AUDIO_INPUT_CONTROL    = BLE_GATT_UUID_16_LSB(0x1843), /*!< Audio Input Control Service */
    BLE_GATT_SVC_VOLUME_CONTROL         = BLE_GATT_UUID_16_LSB(0x1844), /*!< Volume Control Service */
    BLE_GATT_SVC_VOLUME_OFFSET_CONTROL  = BLE_GATT_UUID_16_LSB(0x1845), /*!< Volume Offset Control Service */
    BLE_GATT_SVC_MICROPHONE_CONTROL     = BLE_GATT_UUID_16_LSB(0x184D), /*!< Microphone Control Service */
    BLE_GATT_SVC_TELEPHONE_BEARER       = BLE_GATT_UUID_16_LSB(0x184B), /*!< Telephone Bearer Service */
    BLE_GATT_SVC_GENERIC_TELEPHONE_BEARER = BLE_GATT_UUID_16_LSB(0x184C), /*!< Generic Telephone Bearer Service */
    BLE_GATT_SVC_MEDIA_CONTROL          = BLE_GATT_UUID_16_LSB(0x1848), /*!< Media Control Service */
    BLE_GATT_SVC_GENERIC_MEDIA_CONTROL  = BLE_GATT_UUID_16_LSB(0x1849), /*!< Generic Media Control Service */
    BLE_GATT_SVC_PUBLISHED_AUDIO_CAPA   = BLE_GATT_UUID_16_LSB(0x1850), /*!< Published Audio Capabilities Service */
    BLE_GATT_SVC_BCAST_AUDIO_SCAN       = BLE_GATT_UUID_16_LSB(0x184F), /*!< Broadcast Audio Scan Service */
    BLE_GATT_SVC_AUDIO_STREAM_CTRL      = BLE_GATT_UUID_16_LSB(0x184E), /*!< Audio Stream Control Service */
    BLE_GATT_SVC_COORD_SET_IDENTIFICATION = BLE_GATT_UUID_16_LSB(0x1846), /*!< Coordinated Set Identification Service */
    BLE_GATT_SVC_COMMON_AUDIO           = BLE_GATT_UUID_16_LSB(0x8FDD), /*!< Common Audio Service */
    BLE_GATT_SVC_TELEPHONY_MEDIA_AUDIO  = BLE_GATT_UUID_16_LSB(0x8FE0), /*!< Telephony and Media Audio Service */
    BLE_GATT_SVC_HEARING_ACCESS         = BLE_GATT_UUID_16_LSB(0x8FE1), /*!< Hearing Access Service */

    BLE_GATT_SVC_BCAST_AUDIO_ANNOUNCEMENT = BLE_GATT_UUID_16_LSB(0x1852), /*!< Broadcast Audio Announcement UUID */
    BLE_GATT_SVC_BASIC_AUDIO_ANNOUNCEMENT = BLE_GATT_UUID_16_LSB(0x1851), /*!< Basic Audio Announcement UUID */
    BLE_GATT_SVC_PUBLIC_BROADCAST_ANNOUNCEMENT = BLE_GATT_UUID_16_LSB(0x1853), /*!< Public Broadcast Announcement Service UUID */

    /*------------------- UNITS ---------------------*/
    BLE_GATT_UNIT_UNITLESS              = BLE_GATT_UUID_16_LSB(0x2700), /*!<  No defined unit */
    BLE_GATT_UNIT_METRE                 = BLE_GATT_UUID_16_LSB(0x2701), /*!< Length Unit - Metre */
    BLE_GATT_UNIT_KG                    = BLE_GATT_UUID_16_LSB(0x2702), /*!< Mass unit - Kilogram */
    BLE_GATT_UNIT_SECOND                = BLE_GATT_UUID_16_LSB(0x2703), /*!< Time unit - second */
    BLE_GATT_UNIT_AMPERE                = BLE_GATT_UUID_16_LSB(0x2704), /*!< Electric current unit - Ampere */
    BLE_GATT_UNIT_KELVIN                = BLE_GATT_UUID_16_LSB(0x2705), /*!< Thermodynamic Temperature unit - Kelvin */
    BLE_GATT_UNIT_MOLE                  = BLE_GATT_UUID_16_LSB(0x2706), /*!< Amount of substance unit - mole */
    BLE_GATT_UNIT_CANDELA               = BLE_GATT_UUID_16_LSB(0x2707), /*!< Luminous intensity unit - candela */
    BLE_GATT_UNIT_SQ_METRE              = BLE_GATT_UUID_16_LSB(0x2710), /*!< Area unit - square metres */
    BLE_GATT_UNIT_CUBIC_METRE           = BLE_GATT_UUID_16_LSB(0x2710), /*!< Colume unit - cubic metres */
    BLE_GATT_UNIT_METRE_PER_SECOND      = BLE_GATT_UUID_16_LSB(0x2711), /*!< Velocity unit - metres per second */
    BLE_GATT_UNIT_METRES_PER_SEC_SQ     = BLE_GATT_UUID_16_LSB(0x2712), /*!< Acceleration unit - metres per second squared */
    BLE_GATT_UNIT_RECIPROCAL_METRE      = BLE_GATT_UUID_16_LSB(0x2713), /*!< Wavenumber unit - reciprocal metre */
    BLE_GATT_UNIT_DENS_KG_PER_CUBIC_METRE = BLE_GATT_UUID_16_LSB(0x2714), /*!< Density unit - kilogram per cubic metre */
    BLE_GATT_UNIT_KG_PER_SQ_METRE       = BLE_GATT_UUID_16_LSB(0x2715), /*!< Surface density unit - kilogram per square metre */
    BLE_GATT_UNIT_CUBIC_METRE_PER_KG    = BLE_GATT_UUID_16_LSB(0x2716), /*!< Specific volume unit - cubic metre per kilogram */
    BLE_GATT_UNIT_AMPERE_PER_SQ_METRE   = BLE_GATT_UUID_16_LSB(0x2717), /*!< Current density unit - ampere per square metre */
    BLE_GATT_UNIT_AMPERE_PER_METRE      = BLE_GATT_UUID_16_LSB(0x2718), /*!< Magnetic field strength unit - Ampere per metre */
    BLE_GATT_UNIT_MOLE_PER_CUBIC_METRE  = BLE_GATT_UUID_16_LSB(0x2719), /*!< Amount concentration unit - mole per cubic metre */
    BLE_GATT_UNIT_MASS_KG_PER_CUBIC_METRE = BLE_GATT_UUID_16_LSB(0x271A), /*!< Mass Concentration unit - kilogram per cubic metre */
    BLE_GATT_UNIT_CANDELA_PER_SQ_METRE  = BLE_GATT_UUID_16_LSB(0x271B), /*!< Luminance unit - candela per square metre */
    BLE_GATT_UNIT_REFRACTIVE_INDEX      = BLE_GATT_UUID_16_LSB(0x271C), /*!< Refractive index unit */
    BLE_GATT_UNIT_RELATIVE_PERMEABILITY = BLE_GATT_UUID_16_LSB(0x271D), /*!< Relative permeability unit */
    BLE_GATT_UNIT_RADIAN                = BLE_GATT_UUID_16_LSB(0x2720), /*!< Plane angle unit - radian */
    BLE_GATT_UNIT_STERADIAN             = BLE_GATT_UUID_16_LSB(0x2721), /*!< Solid angle unit - steradian */
    BLE_GATT_UNIT_HERTZ                 = BLE_GATT_UUID_16_LSB(0x2722), /*!< Frequency unit - Hertz */
    BLE_GATT_UNIT_NEWTON                = BLE_GATT_UUID_16_LSB(0x2723), /*!< Force unit - Newton */
    BLE_GATT_UNIT_PASCAL                = BLE_GATT_UUID_16_LSB(0x2724), /*!< Pressure unit - Pascal */
    BLE_GATT_UNIT_JOULE                 = BLE_GATT_UUID_16_LSB(0x2725), /*!< Energy unit - Joule */
    BLE_GATT_UNIT_WATT                  = BLE_GATT_UUID_16_LSB(0x2726), /*!< Power unit - Watt */
    BLE_GATT_UNIT_COULOMB               = BLE_GATT_UUID_16_LSB(0x2727), /*!< electric Charge unit - Coulomb */
    BLE_GATT_UNIT_VOLT                  = BLE_GATT_UUID_16_LSB(0x2728), /*!< Electric potential difference - Volt */
    BLE_GATT_UNIT_FARAD                 = BLE_GATT_UUID_16_LSB(0x2729), /*!< Capacitance unit - Farad */
    BLE_GATT_UNIT_OHM                   = BLE_GATT_UUID_16_LSB(0x272A), /*!< electric resistance unit - Ohm */
    BLE_GATT_UNIT_SIEMENS               = BLE_GATT_UUID_16_LSB(0x272B), /*!< Electric conductance - Siemens */
    BLE_GATT_UNIT_WEBER                 = BLE_GATT_UUID_16_LSB(0x272C), /*!< Magnetic flux unit - Weber */
    BLE_GATT_UNIT_TESLA                 = BLE_GATT_UUID_16_LSB(0x272D), /*!< Magnetic flux density unit - Tesla */
    BLE_GATT_UNIT_HENRY                 = BLE_GATT_UUID_16_LSB(0x272E), /*!< Inductance unit - Henry */
    BLE_GATT_UNIT_CELSIUS               = BLE_GATT_UUID_16_LSB(0x272F), /*!< Temperature unit - degree Celsius */
    BLE_GATT_UNIT_LUMEN                 = BLE_GATT_UUID_16_LSB(0x2730), /*!< Luminous flux unit - lumen */
    BLE_GATT_UNIT_LUX                   = BLE_GATT_UUID_16_LSB(0x2731), /*!< Illuminance unit - lux */
    BLE_GATT_UNIT_BECQUEREL             = BLE_GATT_UUID_16_LSB(0x2732), /*!< Activity referred to a radionuclide unit - becquerel */
    BLE_GATT_UNIT_GRAY                  = BLE_GATT_UUID_16_LSB(0x2733), /*!< Absorbed dose unit - Gray */
    BLE_GATT_UNIT_SIEVERT               = BLE_GATT_UUID_16_LSB(0x2734), /*!< Dose equivalent unit - Sievert */
    BLE_GATT_UNIT_KATAL                 = BLE_GATT_UUID_16_LSB(0x2735), /*!< Catalytic activity unit - Katal */
    BLE_GATT_UNIT_PASCAL_SECOND         = BLE_GATT_UUID_16_LSB(0x2740), /*!< Synamic viscosity unit - Pascal second */
    BLE_GATT_UNIT_NEWTON_METRE          = BLE_GATT_UUID_16_LSB(0x2741), /*!< Moment of force unit - Newton metre */
    BLE_GATT_UNIT_NEWTON_PER_METRE      = BLE_GATT_UUID_16_LSB(0x2742), /*!< surface tension unit - Newton per metre */
    BLE_GATT_UNIT_RADIAN_PER_SECOND     = BLE_GATT_UUID_16_LSB(0x2743), /*!< Angular velocity unit - radian per second */
    BLE_GATT_UNIT_RADIAN_PER_SECOND_SQ  = BLE_GATT_UUID_16_LSB(0x2744), /*!< Angular acceleration unit - radian per second squared */
    BLE_GATT_UNIT_WGATT_PER_SQ_METRE    = BLE_GATT_UUID_16_LSB(0x2745), /*!< Heat flux density unit - Watt per square metre */
    BLE_GATT_UNIT_JOULE_PER_KELVIN      = BLE_GATT_UUID_16_LSB(0x2746), /*!< Heat capacity unit - Joule per Kelvin */
    BLE_GATT_UNIT_JOULE_PER_KG_KELVIN   = BLE_GATT_UUID_16_LSB(0x2747), /*!< Specific heat capacity unit - Joule per kilogram kelvin */
    BLE_GATT_UNIT_JOULE_PER_KG          = BLE_GATT_UUID_16_LSB(0x2748), /*!< Specific Energy unit - Joule per kilogram */
    BLE_GATT_UNIT_WGATT_PER_METRE_KELVIN = BLE_GATT_UUID_16_LSB(0x2749), /*!< Thermal conductivity - Watt per metre Kelvin */
    BLE_GATT_UNIT_JOULE_PER_CUBIC_METRE = BLE_GATT_UUID_16_LSB(0x274A), /*!< Energy Density unit - joule per cubic metre */
    BLE_GATT_UNIT_VOLT_PER_METRE        = BLE_GATT_UUID_16_LSB(0x274B), /*!< Electric field strength unit - volt per metre */
    BLE_GATT_UNIT_COULOMB_PER_CUBIC_METRE = BLE_GATT_UUID_16_LSB(0x274C), /*!< Electric charge density unit - coulomb per cubic metre */
    BLE_GATT_UNIT_SURF_COULOMB_PER_SQ_METRE = BLE_GATT_UUID_16_LSB(0x274D), /*!< Surface charge density unit - coulomb per square metre */
    BLE_GATT_UNIT_FLUX_COULOMB_PER_SQ_METRE = BLE_GATT_UUID_16_LSB(0x274E), /*!< Electric flux density unit - coulomb per square metre */
    BLE_GATT_UNIT_FARAD_PER_METRE       = BLE_GATT_UUID_16_LSB(0x274F), /*!< Permittivity unit - farad per metre */
    BLE_GATT_UNIT_HENRY_PER_METRE       = BLE_GATT_UUID_16_LSB(0x2750), /*!< Permeability unit - henry per metre */
    BLE_GATT_UNIT_JOULE_PER_MOLE        = BLE_GATT_UUID_16_LSB(0x2751), /*!< Molar energy unit - joule per mole */
    BLE_GATT_UNIT_JOULE_PER_MOLE_KELVIN = BLE_GATT_UUID_16_LSB(0x2752), /*!< Molar entropy unit - joule per mole kelvin */
    BLE_GATT_UNIT_COULOMB_PER_KG        = BLE_GATT_UUID_16_LSB(0x2753), /*!< Exposure unit - coulomb per kilogram */
    BLE_GATT_UNIT_GRAY_PER_SECOND       = BLE_GATT_UUID_16_LSB(0x2754), /*!< Absorbed dose rate unit - gray per second */
    BLE_GATT_UNIT_WGATT_PER_STERADIAN   = BLE_GATT_UUID_16_LSB(0x2755), /*!< Radiant intensity unit - watt per steradian */
    BLE_GATT_UNIT_WGATT_PER_SQ_METRE_STERADIAN = BLE_GATT_UUID_16_LSB(0x2756), /*!< Radiance unit - watt per square meter steradian */
    BLE_GATT_UNIT_KATAL_PER_CUBIC_METRE = BLE_GATT_UUID_16_LSB(0x2757), /*!< Catalytic activity concentration unit - katal per cubic metre */
    BLE_GATT_UNIT_MINUTE                = BLE_GATT_UUID_16_LSB(0x2760), /*!< Time unit - minute */
    BLE_GATT_UNIT_HOUR                  = BLE_GATT_UUID_16_LSB(0x2761), /*!< Time unit - hour */
    BLE_GATT_UNIT_DAY                   = BLE_GATT_UUID_16_LSB(0x2762), /*!< Time unit - day */
    BLE_GATT_UNIT_ANGLE_DEGREE          = BLE_GATT_UUID_16_LSB(0x2763), /*!< Plane angle unit - degree */
    BLE_GATT_UNIT_ANGLE_MINUTE          = BLE_GATT_UUID_16_LSB(0x2764), /*!< Plane angle unit - minute */
    BLE_GATT_UNIT_ANGLE_SECOND          = BLE_GATT_UUID_16_LSB(0x2765), /*!< Plane angle unit - second */
    BLE_GATT_UNIT_HECTARE               = BLE_GATT_UUID_16_LSB(0x2766), /*!< Area unit - hectare */
    BLE_GATT_UNIT_LITRE                 = BLE_GATT_UUID_16_LSB(0x2767), /*!< Volume unit - litre */
    BLE_GATT_UNIT_TONNE                 = BLE_GATT_UUID_16_LSB(0x2768), /*!< Mass unit - tonne */
    BLE_GATT_UNIT_BAR                   = BLE_GATT_UUID_16_LSB(0x2780), /*!< Pressure unit - bar */
    BLE_GATT_UNIT_MM_MERCURY            = BLE_GATT_UUID_16_LSB(0x2781), /*!< Pressure unit - millimetre of mercury */
    BLE_GATT_UNIT_ANGSTROM              = BLE_GATT_UUID_16_LSB(0x2782), /*!< Length unit - angstrom */
    BLE_GATT_UNIT_NAUTICAL_MILE         = BLE_GATT_UUID_16_LSB(0x2783), /*!< Length unit - nautical mile */
    BLE_GATT_UNIT_BARN                  = BLE_GATT_UUID_16_LSB(0x2784), /*!< Area unit - barn */
    BLE_GATT_UNIT_KNOT                  = BLE_GATT_UUID_16_LSB(0x2785), /*!< Velocity unit - knot */
    BLE_GATT_UNIT_NEPER                 = BLE_GATT_UUID_16_LSB(0x2786), /*!< Logarithmic radio quantity unit - neper */
    BLE_GATT_UNIT_BEL                   = BLE_GATT_UUID_16_LSB(0x2787), /*!< Logarithmic radio quantity unit - bel */
    BLE_GATT_UNIT_YARD                  = BLE_GATT_UUID_16_LSB(0x27A0), /*!< Length unit - yard */
    BLE_GATT_UNIT_PARSEC                = BLE_GATT_UUID_16_LSB(0x27A1), /*!< Length unit - parsec */
    BLE_GATT_UNIT_INCH                  = BLE_GATT_UUID_16_LSB(0x27A2), /*!< length unit - inch */
    BLE_GATT_UNIT_FOOT                  = BLE_GATT_UUID_16_LSB(0x27A3), /*!< length unit - foot */
    BLE_GATT_UNIT_MILE                  = BLE_GATT_UUID_16_LSB(0x27A4), /*!< length unit - mile */
    BLE_GATT_UNIT_POUND_FORCE_PER_SQ_INCH = BLE_GATT_UUID_16_LSB(0x27A5), /*!< pressure unit - pound-force per square inch */
    BLE_GATT_UNIT_KM_PER_HOUR           = BLE_GATT_UUID_16_LSB(0x27A6), /*!< velocity unit - kilometre per hour */
    BLE_GATT_UNIT_MILE_PER_HOUR         = BLE_GATT_UUID_16_LSB(0x27A7), /*!< velocity unit - mile per hour */
    BLE_GATT_UNIT_REVOLUTION_PER_MINUTE = BLE_GATT_UUID_16_LSB(0x27A8), /*!< angular velocity unit - revolution per minute */
    BLE_GATT_UNIT_GRAM_CALORIE          = BLE_GATT_UUID_16_LSB(0x27A9), /*!< energy unit - gram calorie */
    BLE_GATT_UNIT_KG_CALORIE            = BLE_GATT_UUID_16_LSB(0x27AA), /*!< energy unit - kilogram calorie */
    BLE_GATT_UNIT_KILOWGATT_HOUR        = BLE_GATT_UUID_16_LSB(0x27AB), /*!< energy unit - kilowatt hour */
    BLE_GATT_UNIT_FAHRENHEIT            = BLE_GATT_UUID_16_LSB(0x27AC), /*!< thermodynamic temperature unit - degree Fahrenheit */
    BLE_GATT_UNIT_PERCENTAGE            = BLE_GATT_UUID_16_LSB(0x27AD), /*!< percentage */
    BLE_GATT_UNIT_PER_MILLE             = BLE_GATT_UUID_16_LSB(0x27AE), /*!< per mille */
    BLE_GATT_UNIT_BEATS_PER_MINUTE      = BLE_GATT_UUID_16_LSB(0x27AF), /*!< period unit - beats per minute) */
    BLE_GATT_UNIT_AMPERE_HOURS          = BLE_GATT_UUID_16_LSB(0x27B0), /*!< electric charge unit - ampere hours */
    BLE_GATT_UNIT_MILLIGRAM_PER_DECILITRE = BLE_GATT_UUID_16_LSB(0x27B1), /*!< mass density unit - milligram per decilitre */
    BLE_GATT_UNIT_MILLIMOLE_PER_LITRE   = BLE_GATT_UUID_16_LSB(0x27B2), /*!< mass density unit - millimole per litre */
    BLE_GATT_UNIT_YEAR                  = BLE_GATT_UUID_16_LSB(0x27B3), /*!< time unit - year */
    BLE_GATT_UNIT_MONTH                 = BLE_GATT_UUID_16_LSB(0x27B4), /*!< time unit - month */

    /*---------------- DECLARATIONS -----------------*/
    BLE_GATT_DECL_PRIMARY_SERVICE       = BLE_GATT_UUID_16_LSB(0x2800), /*!< Primary service Declaration */
    BLE_GATT_DECL_SECONDARY_SERVICE     = BLE_GATT_UUID_16_LSB(0x2801), /*!< Secondary service Declaration */
    BLE_GATT_DECL_INCLUDE               = BLE_GATT_UUID_16_LSB(0x2802), /*!< Include Declaration */
    BLE_GATT_DECL_CHARACTERISTIC        = BLE_GATT_UUID_16_LSB(0x2803), /*!< Characteristic Declaration */

    /*----------------- DESCRIPTORS -----------------*/
    BLE_GATT_DESC_CHAR_EXT_PROPERTIES   = BLE_GATT_UUID_16_LSB(0x2900), /*!< Characteristic extended properties */
    BLE_GATT_DESC_CHAR_USER_DESCRIPTION = BLE_GATT_UUID_16_LSB(0x2901), /*!< Characteristic user description */
    BLE_GATT_DESC_CLIENT_CHAR_CFG       = BLE_GATT_UUID_16_LSB(0x2902), /*!< Client characteristic configuration */
    BLE_GATT_DESC_SERVER_CHAR_CFG       = BLE_GATT_UUID_16_LSB(0x2903), /*!< Server characteristic configuration */
    BLE_GATT_DESC_CHAR_PRES_FORMAT      = BLE_GATT_UUID_16_LSB(0x2904), /*!< Characteristic Presentation Format */
    BLE_GATT_DESC_CHAR_AGGREGATE_FORMAT = BLE_GATT_UUID_16_LSB(0x2905), /*!< Characteristic Aggregate Format */
    BLE_GATT_DESC_VALID_RANGE           = BLE_GATT_UUID_16_LSB(0x2906), /*!< Valid Range */
    BLE_GATT_DESC_EXT_REPORT_REF        = BLE_GATT_UUID_16_LSB(0x2907), /*!< External Report Reference */
    BLE_GATT_DESC_REPORT_REF            = BLE_GATT_UUID_16_LSB(0x2908), /*!< Report Reference */
    BLE_GATT_DESC_ES_CONFIGURATION      = BLE_GATT_UUID_16_LSB(0x290B), /*!< Environmental Sensing Configuration */
    BLE_GATT_DESC_ES_MEASUREMENT        = BLE_GATT_UUID_16_LSB(0x290C), /*!< Environmental Sensing Measurement */
    BLE_GATT_DESC_ES_TRIGGER_SETTING    = BLE_GATT_UUID_16_LSB(0x290D), /*!< Environmental Sensing Trigger Setting */

    /*--------------- CHARACTERISTICS ---------------*/
    BLE_GATT_CHAR_DEVICE_NAME           = BLE_GATT_UUID_16_LSB(0x2A00), /*!< Device name */
    BLE_GATT_CHAR_APPEARANCE            = BLE_GATT_UUID_16_LSB(0x2A01), /*!< Appearance */
    BLE_GATT_CHAR_PRIVACY_FLAG          = BLE_GATT_UUID_16_LSB(0x2A02), /*!< Privacy flag */
    BLE_GATT_CHAR_RECONNECTION_ADDR     = BLE_GATT_UUID_16_LSB(0x2A03), /*!< Reconnection address */
    BLE_GATT_CHAR_PERIPH_PREF_CON_PARAM = BLE_GATT_UUID_16_LSB(0x2A04), /*!< Peripheral preferred connection parameters */
    BLE_GATT_CHAR_SERVICE_CHANGED       = BLE_GATT_UUID_16_LSB(0x2A05), /*!< Service handles changed */
    BLE_GATT_CHAR_ALERT_LEVEL           = BLE_GATT_UUID_16_LSB(0x2A06), /*!< Alert Level characteristic */
    BLE_GATT_CHAR_TX_POWER_LEVEL        = BLE_GATT_UUID_16_LSB(0x2A07), /*!< Tx Power Level */
    BLE_GATT_CHAR_DATE_TIME             = BLE_GATT_UUID_16_LSB(0x2A08), /*!< Date Time */
    BLE_GATT_CHAR_DAY_WEEK              = BLE_GATT_UUID_16_LSB(0x2A09), /*!< Day of Week */
    BLE_GATT_CHAR_DAY_DATE_TIME         = BLE_GATT_UUID_16_LSB(0x2A0A), /*!< Day Date Time */
    BLE_GATT_CHAR_EXACT_TIME_256        = BLE_GATT_UUID_16_LSB(0x2A0C), /*!< Exact time 256 */
    BLE_GATT_CHAR_DST_OFFSET            = BLE_GATT_UUID_16_LSB(0x2A0D), /*!< DST Offset */
    BLE_GATT_CHAR_TIME_ZONE             = BLE_GATT_UUID_16_LSB(0x2A0E), /*!< Time zone */
    BLE_GATT_CHAR_LOCAL_TIME_INFO       = BLE_GATT_UUID_16_LSB(0x2A0F), /*!< Local time Information */
    BLE_GATT_CHAR_TIME_WITH_DST         = BLE_GATT_UUID_16_LSB(0x2A11), /*!< Time with DST */
    BLE_GATT_CHAR_TIME_ACCURACY         = BLE_GATT_UUID_16_LSB(0x2A12), /*!< Time Accuracy */
    BLE_GATT_CHAR_TIME_SOURCE           = BLE_GATT_UUID_16_LSB(0x2A13), /*!< Time Source */
    BLE_GATT_CHAR_REFERENCE_TIME_INFO   = BLE_GATT_UUID_16_LSB(0x2A14), /*!< Reference Time Information */
    BLE_GATT_CHAR_TIME_UPDATE_CNTL_POINT = BLE_GATT_UUID_16_LSB(0x2A16), /*!< Time Update Control Point */
    BLE_GATT_CHAR_TIME_UPDATE_STATE     = BLE_GATT_UUID_16_LSB(0x2A17), /*!< Time Update State */
    BLE_GATT_CHAR_GLUCOSE_MEAS          = BLE_GATT_UUID_16_LSB(0x2A18), /*!< Glucose Measurement */
    BLE_GATT_CHAR_BATTERY_LEVEL         = BLE_GATT_UUID_16_LSB(0x2A19), /*!< Battery Level */
    BLE_GATT_CHAR_TEMPERATURE_MEAS      = BLE_GATT_UUID_16_LSB(0x2A1C), /*!< Temperature Measurement */
    BLE_GATT_CHAR_TEMPERATURE_TYPE      = BLE_GATT_UUID_16_LSB(0x2A1D), /*!< Temperature Type */
    BLE_GATT_CHAR_INTERMED_TEMPERATURE  = BLE_GATT_UUID_16_LSB(0x2A1E), /*!< Intermediate Temperature */
    BLE_GATT_CHAR_MEAS_INTERVAL         = BLE_GATT_UUID_16_LSB(0x2A21), /*!< Measurement Interval */
    BLE_GATT_CHAR_BOOT_KB_IN_REPORT     = BLE_GATT_UUID_16_LSB(0x2A22), /*!< Boot Keyboard Input Report */
    BLE_GATT_CHAR_SYS_ID                = BLE_GATT_UUID_16_LSB(0x2A23), /*!< System ID */
    BLE_GATT_CHAR_MODEL_NB              = BLE_GATT_UUID_16_LSB(0x2A24), /*!< Model Number String */
    BLE_GATT_CHAR_SERIAL_NB             = BLE_GATT_UUID_16_LSB(0x2A25), /*!< Serial Number String */
    BLE_GATT_CHAR_FW_REV                = BLE_GATT_UUID_16_LSB(0x2A26), /*!< Firmware Revision String */
    BLE_GATT_CHAR_HW_REV                = BLE_GATT_UUID_16_LSB(0x2A27), /*!< Hardware revision String */
    BLE_GATT_CHAR_SW_REV                = BLE_GATT_UUID_16_LSB(0x2A28), /*!< Software Revision String */
    BLE_GATT_CHAR_MANUF_NAME            = BLE_GATT_UUID_16_LSB(0x2A29), /*!< Manufacturer Name String */
    BLE_GATT_CHAR_IEEE_CERTIF           = BLE_GATT_UUID_16_LSB(0x2A2A), /*!< IEEE Regulatory Certification Data List */
    BLE_GATT_CHAR_CT_TIME               = BLE_GATT_UUID_16_LSB(0x2A2B), /*!< CT Time */
    BLE_GATT_CHAR_MAGN_DECLINE          = BLE_GATT_UUID_16_LSB(0x2A2C), /*!< Magnetic Declination */
    BLE_GATT_CHAR_SCAN_REFRESH          = BLE_GATT_UUID_16_LSB(0x2A31), /*!< Scan Refresh */
    BLE_GATT_CHAR_BOOT_KB_OUT_REPORT    = BLE_GATT_UUID_16_LSB(0x2A32), /*!< Boot Keyboard Output Report */
    BLE_GATT_CHAR_BOOT_MOUSE_IN_REPORT  = BLE_GATT_UUID_16_LSB(0x2A33), /*!< Boot Mouse Input Report */
    BLE_GATT_CHAR_GLUCOSE_MEAS_CTX      = BLE_GATT_UUID_16_LSB(0x2A34), /*!< Glucose Measurement Context */
    BLE_GATT_CHAR_BLOOD_PRESSURE_MEAS   = BLE_GATT_UUID_16_LSB(0x2A35), /*!< Blood Pressure Measurement */
    BLE_GATT_CHAR_INTERMEDIATE_CUFF_PRESSURE = BLE_GATT_UUID_16_LSB(0x2A36), /*!< Intermediate Cuff Pressure */
    BLE_GATT_CHAR_HEART_RATE_MEAS       = BLE_GATT_UUID_16_LSB(0x2A37), /*!< Heart Rate Measurement */
    BLE_GATT_CHAR_BODY_SENSOR_LOCATION  = BLE_GATT_UUID_16_LSB(0x2A38), /*!< Body Sensor Location */
    BLE_GATT_CHAR_HEART_RATE_CNTL_POINT = BLE_GATT_UUID_16_LSB(0x2A39), /*!< Heart Rate Control Point */
    BLE_GATT_CHAR_ALERT_STATUS          = BLE_GATT_UUID_16_LSB(0x2A3F), /*!< Alert Status */
    BLE_GATT_CHAR_RINGER_CNTL_POINT     = BLE_GATT_UUID_16_LSB(0x2A40), /*!< Ringer Control Point */
    BLE_GATT_CHAR_RINGER_SETTING        = BLE_GATT_UUID_16_LSB(0x2A41), /*!< Ringer Setting */
    BLE_GATT_CHAR_ALERT_CAT_ID_BIT_MASK = BLE_GATT_UUID_16_LSB(0x2A42), /*!< Alert Category ID Bit Mask */
    BLE_GATT_CHAR_ALERT_CAT_ID          = BLE_GATT_UUID_16_LSB(0x2A43), /*!< Alert Category ID */
    BLE_GATT_CHAR_ALERT_NTF_CTNL_PT     = BLE_GATT_UUID_16_LSB(0x2A44), /*!< Alert Notification Control Point */
    BLE_GATT_CHAR_UNREAD_ALERT_STATUS   = BLE_GATT_UUID_16_LSB(0x2A45), /*!< Unread Alert Status */
    BLE_GATT_CHAR_NEW_ALERT             = BLE_GATT_UUID_16_LSB(0x2A46), /*!< New Alert */
    BLE_GATT_CHAR_SUP_NEW_ALERT_CAT     = BLE_GATT_UUID_16_LSB(0x2A47), /*!< Supported New Alert Category */
    BLE_GATT_CHAR_SUP_UNREAD_ALERT_CAT  = BLE_GATT_UUID_16_LSB(0x2A48), /*!< Supported Unread Alert Category */
    BLE_GATT_CHAR_BLOOD_PRESSURE_FEATURE = BLE_GATT_UUID_16_LSB(0x2A49), /*!< Blood Pressure Feature */
    BLE_GATT_CHAR_HID_INFO              = BLE_GATT_UUID_16_LSB(0x2A4A), /*!< HID Information */
    BLE_GATT_CHAR_REPORT_MAP            = BLE_GATT_UUID_16_LSB(0x2A4B), /*!< Report Map */
    BLE_GATT_CHAR_HID_CTNL_PT           = BLE_GATT_UUID_16_LSB(0x2A4C), /*!< HID Control Point */
    BLE_GATT_CHAR_REPORT                = BLE_GATT_UUID_16_LSB(0x2A4D), /*!< Report */
    BLE_GATT_CHAR_PROTOCOL_MODE         = BLE_GATT_UUID_16_LSB(0x2A4E), /*!< Protocol Mode */
    BLE_GATT_CHAR_SCAN_INTV_WD          = BLE_GATT_UUID_16_LSB(0x2A4F), /*!< Scan Interval Window */
    BLE_GATT_CHAR_PNP_ID                = BLE_GATT_UUID_16_LSB(0x2A50), /*!< PnP ID */
    BLE_GATT_CHAR_GLUCOSE_FEATURE       = BLE_GATT_UUID_16_LSB(0x2A51), /*!< Glucose Feature */
    BLE_GATT_CHAR_REC_ACCESS_CTRL_PT    = BLE_GATT_UUID_16_LSB(0x2A52), /*!< Record access control point */
    BLE_GATT_CHAR_RSC_MEAS              = BLE_GATT_UUID_16_LSB(0x2A53), /*!< RSC Measurement */
    BLE_GATT_CHAR_RSC_FEAT              = BLE_GATT_UUID_16_LSB(0x2A54), /*!< RSC Feature */
    BLE_GATT_CHAR_SC_CTRL_PT            = BLE_GATT_UUID_16_LSB(0x2A55), /*!< SC Control Point */
    BLE_GATT_CHAR_CSC_MEAS              = BLE_GATT_UUID_16_LSB(0x2A5B), /*!< CSC Measurement */
    BLE_GATT_CHAR_CSC_FEAT              = BLE_GATT_UUID_16_LSB(0x2A5C), /*!< CSC Feature */
    BLE_GATT_CHAR_SENSOR_LOC            = BLE_GATT_UUID_16_LSB(0x2A5D), /*!< Sensor Location */
    BLE_GATT_CHAR_PLX_SPOT_CHECK_MEASUREMENT_LOC = BLE_GATT_UUID_16_LSB(0x2A5E), /*!< PLX Spot-Check Measurement */
    BLE_GATT_CHAR_PLX_CONTINUOUS_MEASUREMENT_LOC = BLE_GATT_UUID_16_LSB(0x2A5F), /*!< PLX Continuous Measurement */
    BLE_GATT_CHAR_PLX_FEATURES_LOC      = BLE_GATT_UUID_16_LSB(0x2A60), /*!< PLX Features */
    BLE_GATT_CHAR_CP_MEAS               = BLE_GATT_UUID_16_LSB(0x2A63), /*!< CP Measurement */
    BLE_GATT_CHAR_CP_VECTOR             = BLE_GATT_UUID_16_LSB(0x2A64), /*!< CP Vector */
    BLE_GATT_CHAR_CP_FEAT               = BLE_GATT_UUID_16_LSB(0x2A65), /*!< CP Feature */
    BLE_GATT_CHAR_CP_CNTL_PT            = BLE_GATT_UUID_16_LSB(0x2A66), /*!< CP Control Point */
    BLE_GATT_CHAR_LOC_SPEED             = BLE_GATT_UUID_16_LSB(0x2A67), /*!< Location and Speed */
    BLE_GATT_CHAR_NAVIGATION            = BLE_GATT_UUID_16_LSB(0x2A68), /*!< Navigation */
    BLE_GATT_CHAR_POS_QUALITY           = BLE_GATT_UUID_16_LSB(0x2A69), /*!< Position Quality */
    BLE_GATT_CHAR_LN_FEAT               = BLE_GATT_UUID_16_LSB(0x2A6A), /*!< LN Feature */
    BLE_GATT_CHAR_LN_CNTL_PT            = BLE_GATT_UUID_16_LSB(0x2A6B), /*!< LN Control Point */
    BLE_GATT_CHAR_ELEVATION             = BLE_GATT_UUID_16_LSB(0x2A6C), /*!< Elevation */
    BLE_GATT_CHAR_PRESSURE              = BLE_GATT_UUID_16_LSB(0x2A6D), /*!< Pressure */
    BLE_GATT_CHAR_TEMPERATURE           = BLE_GATT_UUID_16_LSB(0x2A6E), /*!< Temperature */
    BLE_GATT_CHAR_HUMIDITY              = BLE_GATT_UUID_16_LSB(0x2A6F), /*!< Humidity */
    BLE_GATT_CHAR_TRUE_WIND_SPEED       = BLE_GATT_UUID_16_LSB(0x2A70), /*!< True Wind Speed */
    BLE_GATT_CHAR_TRUE_WIND_DIR         = BLE_GATT_UUID_16_LSB(0x2A71), /*!< True Wind Direction */
    BLE_GATT_CHAR_APRNT_WIND_SPEED      = BLE_GATT_UUID_16_LSB(0x2A72), /*!< Apparent Wind Speed */
    BLE_GATT_CHAR_APRNT_WIND_DIRECTION  = BLE_GATT_UUID_16_LSB(0x2A73), /*!< Apparent Wind Direction */
    BLE_GATT_CHAR_GUST_FACTOR           = BLE_GATT_UUID_16_LSB(0x2A74), /*!< Gust Factor */
    BLE_GATT_CHAR_POLLEN_CONC           = BLE_GATT_UUID_16_LSB(0x2A75), /*!< Pollen Concentration */
    BLE_GATT_CHAR_UV_INDEX              = BLE_GATT_UUID_16_LSB(0x2A76), /*!< UV Index */
    BLE_GATT_CHAR_IRRADIANCE            = BLE_GATT_UUID_16_LSB(0x2A77), /*!< Irradiance */
    BLE_GATT_CHAR_RAINFALL              = BLE_GATT_UUID_16_LSB(0x2A78), /*!< Rainfall */
    BLE_GATT_CHAR_WIND_CHILL            = BLE_GATT_UUID_16_LSB(0x2A79), /*!< Wind Chill */
    BLE_GATT_CHAR_HEAT_INDEX            = BLE_GATT_UUID_16_LSB(0x2A7A), /*!< Heat Index */
    BLE_GATT_CHAR_DEW_POINT             = BLE_GATT_UUID_16_LSB(0x2A7B), /*!< Dew Point */
    BLE_GATT_CHAR_DESCRIPTOR_VALUE_CHANGED = BLE_GATT_UUID_16_LSB(0x2A7D), /*!< Descriptor Value Changed */
    BLE_GATT_CHAR_AEROBIC_HEART_RATE_LOW_LIM = BLE_GATT_UUID_16_LSB(0x2A7E), /*!< Aerobic Heart Rate Lower Limit */
    BLE_GATT_CHAR_AEROBIC_THR           = BLE_GATT_UUID_16_LSB(0x2A7F), /*!< Aerobic Threshhold */
    BLE_GATT_CHAR_AGE                   = BLE_GATT_UUID_16_LSB(0x2A80), /*!< Age */
    BLE_GATT_CHAR_ANAERO_HEART_RATE_LOW_LIM = BLE_GATT_UUID_16_LSB(0x2A81), /*!< Anaerobic Heart Rate Lower Limit */
    BLE_GATT_CHAR_ANAERO_HEART_RATE_UP_LIM = BLE_GATT_UUID_16_LSB(0x2A82), /*!< Anaerobic Heart Rate Upper Limit */
    BLE_GATT_CHAR_ANAERO_THR            = BLE_GATT_UUID_16_LSB(0x2A83), /*!< Anaerobic Threshhold */
    BLE_GATT_CHAR_AEROBIC_HEART_RATE_UP_LIM = BLE_GATT_UUID_16_LSB(0x2A84), /*!< Aerobic Heart Rate Upper Limit */
    BLE_GATT_CHAR_DATE_OF_BIRTH         = BLE_GATT_UUID_16_LSB(0x2A85), /*!< Date Of Birth */
    BLE_GATT_CHAR_DATE_OF_THR_ASSESS    = BLE_GATT_UUID_16_LSB(0x2A86), /*!< Date Of Threshold Assessment */
    BLE_GATT_CHAR_EMAIL_ADDRESS         = BLE_GATT_UUID_16_LSB(0x2A87), /*!< Email Address */
    BLE_GATT_CHAR_FAT_BURN_HEART_RATE_LOW_LIM = BLE_GATT_UUID_16_LSB(0x2A88), /*!< Fat Burn Heart Rate Lower Limit */
    BLE_GATT_CHAR_FAT_BURN_HEART_RATE_UP_LIM = BLE_GATT_UUID_16_LSB(0x2A89), /*!< Fat Burn Heart Rate Upper Limit */
    BLE_GATT_CHAR_FIRST_NAME            = BLE_GATT_UUID_16_LSB(0x2A8A), /*!< First Name */
    BLE_GATT_CHAR_FIVE_ZONE_HEART_RATE_LIMITS = BLE_GATT_UUID_16_LSB(0x2A8B), /*!< Five Zone Heart Rate Limits */
    BLE_GATT_CHAR_GENDER                = BLE_GATT_UUID_16_LSB(0x2A8C), /*!< Gender */
    BLE_GATT_CHAR_MAX_HEART_RATE        = BLE_GATT_UUID_16_LSB(0x2A8D), /*!< Max Heart Rate */
    BLE_GATT_CHAR_HEIGHT                = BLE_GATT_UUID_16_LSB(0x2A8E), /*!< Height */
    BLE_GATT_CHAR_HIP_CIRCUMFERENCE     = BLE_GATT_UUID_16_LSB(0x2A8F), /*!< Hip Circumference */
    BLE_GATT_CHAR_LAST_NAME             = BLE_GATT_UUID_16_LSB(0x2A90), /*!< Last Name */
    BLE_GATT_CHAR_MAX_RECO_HEART_RATE   = BLE_GATT_UUID_16_LSB(0x2A91), /*!< Maximum Recommended Heart Rate */
    BLE_GATT_CHAR_RESTING_HEART_RATE    = BLE_GATT_UUID_16_LSB(0x2A92), /*!< Resting Heart Rate */
    BLE_GATT_CHAR_SPORT_TYPE_FOR_AERO_ANAREO_THRS = BLE_GATT_UUID_16_LSB(0x2A93), /*!< Sport Type For Aerobic And Anaerobic Thresholds */
    BLE_GATT_CHAR_THREE_ZONE_HEART_RATE_LIMITS = BLE_GATT_UUID_16_LSB(0x2A94), /*!< Three Zone Heart Rate Limits */
    BLE_GATT_CHAR_TWO_ZONE_HEART_RATE_LIMIT = BLE_GATT_UUID_16_LSB(0x2A95), /*!< Two Zone Heart Rate Limit */
    BLE_GATT_CHAR_VO2_MAX               = BLE_GATT_UUID_16_LSB(0x2A96), /*!< Vo2 Max */
    BLE_GATT_CHAR_WAIST_CIRCUMFERENCE   = BLE_GATT_UUID_16_LSB(0x2A97), /*!< Waist Circumference */
    BLE_GATT_CHAR_WEIGHT                = BLE_GATT_UUID_16_LSB(0x2A98), /*!< Weight */
    BLE_GATT_CHAR_DB_CHG_INCREMENT      = BLE_GATT_UUID_16_LSB(0x2A99), /*!< Database Change Increment */
    BLE_GATT_CHAR_USER_INDEX            = BLE_GATT_UUID_16_LSB(0x2A9A), /*!< User Index */
    BLE_GATT_CHAR_BODY_COMPOSITION_FEATURE = BLE_GATT_UUID_16_LSB(0x2A9B), /*!< Body Composition Feature */
    BLE_GATT_CHAR_BODY_COMPOSITION_MEASUREMENT = BLE_GATT_UUID_16_LSB(0x2A9C), /*!< Body Composition Measurement */
    BLE_GATT_CHAR_WEIGHT_MEASUREMENT    = BLE_GATT_UUID_16_LSB(0x2A9D), /*!< Weight Measurement */
    BLE_GATT_CHAR_WEIGHT_SCALE_FEATURE  = BLE_GATT_UUID_16_LSB(0x2A9E), /*!< Weight Scale Feature */
    BLE_GATT_CHAR_USER_CONTROL_POINT    = BLE_GATT_UUID_16_LSB(0x2A9F), /*!< User Control Point */
    BLE_GATT_CHAR_MAGN_FLUX_2D          = BLE_GATT_UUID_16_LSB(0x2AA0), /*!< Flux Density - 2D */
    BLE_GATT_CHAR_MAGN_FLUX_3D          = BLE_GATT_UUID_16_LSB(0x2AA1), /*!< Magnetic Flux Density - 3D */
    BLE_GATT_CHAR_LANGUAGE              = BLE_GATT_UUID_16_LSB(0x2AA2), /*!< Language string */
    BLE_GATT_CHAR_BAR_PRES_TREND        = BLE_GATT_UUID_16_LSB(0x2AA3), /*!< Barometric Pressure Trend */
    BLE_GATT_CHAR_CTL_ADDR_RESOL_SUPP   = BLE_GATT_UUID_16_LSB(0x2AA6), /*!< Central Address Resolution Support */
    BLE_GATT_CHAR_CGM_MEASUREMENT       = BLE_GATT_UUID_16_LSB(0x2AA7), /*!< CGM Measurement */
    BLE_GATT_CHAR_CGM_FEATURES          = BLE_GATT_UUID_16_LSB(0x2AA8), /*!< CGM Features */
    BLE_GATT_CHAR_CGM_STATUS            = BLE_GATT_UUID_16_LSB(0x2AA9), /*!< CGM Status */
    BLE_GATT_CHAR_CGM_SESSION_START     = BLE_GATT_UUID_16_LSB(0x2AAA), /*!< CGM Session Start */
    BLE_GATT_CHAR_CGM_SESSION_RUN       = BLE_GATT_UUID_16_LSB(0x2AAB), /*!< CGM Session Run */
    BLE_GATT_CHAR_CGM_SPECIFIC_OPS_CTRL_PT = BLE_GATT_UUID_16_LSB(0x2AAC), /*!< CGM Specific Ops Control Point */

    BLE_GATT_CHAR_HPS_URI               = BLE_GATT_UUID_16_LSB(0x2AB6), /*!< Uri */
    BLE_GATT_CHAR_HPS_HEADERS           = BLE_GATT_UUID_16_LSB(0x2AB7), /*!< Headers */
    BLE_GATT_CHAR_HPS_STATUS_CODE       = BLE_GATT_UUID_16_LSB(0x2AB8), /*!< Status code */
    BLE_GATT_CHAR_HPS_ENTITY_BODY       = BLE_GATT_UUID_16_LSB(0x2AB9), /*!< Entity body */
    BLE_GATT_CHAR_HPS_CTRL_POINT        = BLE_GATT_UUID_16_LSB(0x2ABA), /*!< control point */
    BLE_GATT_CHAR_HPS_SECURITY          = BLE_GATT_UUID_16_LSB(0x2ABB), /*!< Security */

    BLE_GATT_CHAR_OTS_FEATURE           = BLE_GATT_UUID_16_LSB(0x2ABD), /*!< Object Transfer Service - Feature characteristic */
    BLE_GATT_CHAR_OTS_OBJECT_NAME       = BLE_GATT_UUID_16_LSB(0x2ABE), /*!< Object Transfer Service - Object Name characteristic */
    BLE_GATT_CHAR_OTS_OBJECT_TYPE       = BLE_GATT_UUID_16_LSB(0x2ABF), /*!< Object Transfer Service - Object Type characteristic */
    BLE_GATT_CHAR_OTS_OBJECT_SIZE       = BLE_GATT_UUID_16_LSB(0x2AC0), /*!< Object Transfer Service - Object Size characteristic */
    BLE_GATT_CHAR_OTS_OBJECT_FIRST_CRAETED = BLE_GATT_UUID_16_LSB(0x2AC1), /*!< Object Transfer Service - Object First-Created characteristic */
    BLE_GATT_CHAR_OTS_OBJECT_LAST_MODIFIED = BLE_GATT_UUID_16_LSB(0x2AC2), /*!< Object Transfer Service - Object Last-Modified characteristic */
    BLE_GATT_CHAR_OTS_OBJECT_ID         = BLE_GATT_UUID_16_LSB(0x2AC3), /*!< Object Transfer Service - Object ID characteristic */
    BLE_GATT_CHAR_OTS_OBJECT_PROPERTIES = BLE_GATT_UUID_16_LSB(0x2AC4), /*!< Object Transfer Service - Object Properties characteristic */
    BLE_GATT_CHAR_OTS_OACP              = BLE_GATT_UUID_16_LSB(0x2AC5), /*!< Object Transfer Service - Object Action Control Point characteristic */
    BLE_GATT_CHAR_OTS_OLCP              = BLE_GATT_UUID_16_LSB(0x2AC6), /*!< Object Transfer Service - Object List Control Point characteristic */
    BLE_GATT_CHAR_OTS_OBJECT_LIST_FILTER = BLE_GATT_UUID_16_LSB(0x2AC7), /*!< Object Transfer Service - Object List Filter characteristic */
    BLE_GATT_CHAR_OTS_OBJECT_CHANGED    = BLE_GATT_UUID_16_LSB(0x2AC8), /*!< Object Transfer Service - Object Changed characteristic */

    BLE_GATT_CHAR_RSLV_PRIV_ADDR_ONLY   = BLE_GATT_UUID_16_LSB(0x2AC9), /*!< Resolvable Private Address only */

    BLE_GATT_CHAR_MESH_PROV_DATA_IN     = BLE_GATT_UUID_16_LSB(0x2ADB), /*!< Mesh Provisioning Data In */
    BLE_GATT_CHAR_MESH_PROV_DATA_OUT    = BLE_GATT_UUID_16_LSB(0x2ADC), /*!< Mesh Provisioning Data Out */
    BLE_GATT_CHAR_MESH_PROXY_DATA_IN    = BLE_GATT_UUID_16_LSB(0x2ADD), /*!< Mesh Proxy Data In */
    BLE_GATT_CHAR_MESH_PROXY_DATA_OUT   = BLE_GATT_UUID_16_LSB(0x2ADE), /*!< Mesh Proxy Data Out */

    BLE_GATT_CHAR_VOLUME_STATE          = BLE_GATT_UUID_16_LSB(0x2B7D), /*!< Volume Control Service - Volume State characteristic */
    BLE_GATT_CHAR_VOLUME_CP             = BLE_GATT_UUID_16_LSB(0x2B7E), /*!< Volume Control Service - Volume Control Point characteristic */
    BLE_GATT_CHAR_VOLUME_FLAGS          = BLE_GATT_UUID_16_LSB(0x2B7F), /*!< Volume Control Service - Volume Flags characteristic */

    BLE_GATT_CHAR_MUTE                  = BLE_GATT_UUID_16_LSB(0x2BC3), /*!< Microphone Control Service - Mute characteristic */

    BLE_GATT_CHAR_OFFSET_STATE          = BLE_GATT_UUID_16_LSB(0x2B80), /*!< Volume Offset Control Service - Volume Offset State characteristic */
    BLE_GATT_CHAR_AUDIO_LOC             = BLE_GATT_UUID_16_LSB(0x2B81), /*!< Volume Offset Control Service - Audio Location characteristic */
    BLE_GATT_CHAR_OFFSET_CP             = BLE_GATT_UUID_16_LSB(0x2B82), /*!< Volume Offset Control Service - Volume Offset Control Point characteristic */
    BLE_GATT_CHAR_OUTPUT_DESC           = BLE_GATT_UUID_16_LSB(0x2B83), /*!< Volume Offset Control Service - Audio Output Description characteristic */

    BLE_GATT_CHAR_INPUT_STATE           = BLE_GATT_UUID_16_LSB(0x2B77), /*!< Audio Input Control Service - Audio Input State characteristic */
    BLE_GATT_CHAR_GAIN_PROP             = BLE_GATT_UUID_16_LSB(0x2B78), /*!< Audio Input Control Service - Gain Setting Properties characteristic */
    BLE_GATT_CHAR_INPUT_TYPE            = BLE_GATT_UUID_16_LSB(0x2B79), /*!< Audio Input Control Service - Audio Input Type characteristic */
    BLE_GATT_CHAR_INPUT_STATUS          = BLE_GATT_UUID_16_LSB(0x2B7A), /*!< Audio Input Control Service - Audio Input Status characteristic */
    BLE_GATT_CHAR_INPUT_CP              = BLE_GATT_UUID_16_LSB(0x2B7B), /*!< Audio Input Control Service - Audio Input Control Point characteristic */
    BLE_GATT_CHAR_INPUT_DESC            = BLE_GATT_UUID_16_LSB(0x2B7C), /*!< Audio Input Control Service - Audio Input Description characteristic */

    BLE_GATT_CHAR_PAC_SINK              = BLE_GATT_UUID_16_LSB(0x2BC9), /*!< Published Audio Capabilities Service - Sink PAC characteristic */
    BLE_GATT_CHAR_LOC_SINK              = BLE_GATT_UUID_16_LSB(0x2BCA), /*!< Published Audio Capabilities Service - Sink PAC characteristic */
    BLE_GATT_CHAR_PAC_SRC               = BLE_GATT_UUID_16_LSB(0x2BCB), /*!< Published Audio Capabilities Service - Sink PAC characteristic */
    BLE_GATT_CHAR_LOC_SRC               = BLE_GATT_UUID_16_LSB(0x2BCC), /*!< Published Audio Capabilities Service - Sink PAC characteristic */
    BLE_GATT_CHAR_CONTEXT_AVA           = BLE_GATT_UUID_16_LSB(0x2BCD), /*!< Published Audio Capabilities Service - Available Audio Contexts */
    BLE_GATT_CHAR_CONTEXT_SUPP          = BLE_GATT_UUID_16_LSB(0x2BCE), /*!< Published Audio Capabilities Service - Supported Audio Contexts */

    BLE_GATT_CHAR_BCAST_AUDIO_SCAN_CP   = BLE_GATT_UUID_16_LSB(0x2BC7), /*!< Broadcast Audio Scan Service - Broadcast Audio Scan Control Point characteristic */
    BLE_GATT_CHAR_BCAST_RX_STATE        = BLE_GATT_UUID_16_LSB(0x2BC8), /*!< Broadcast Audio Scan Service - Broadcast Receive State characteristic */

    BLE_GATT_CHAR_ASE_SINK              = BLE_GATT_UUID_16_LSB(0x2BC4), /*!< Audio Stream Control Service - Sink ASE characteristic */
    BLE_GATT_CHAR_ASE_SRC               = BLE_GATT_UUID_16_LSB(0x2BC5), /*!< Audio Stream Control Service - Source ASE characteristic */
    BLE_GATT_CHAR_ASE_CP                = BLE_GATT_UUID_16_LSB(0x2BC6), /*!< Audio Stream Control Service - ASE Control Point characteristic */

    BLE_GATT_CHAR_CSIS_SIRK             = BLE_GATT_UUID_16_LSB(0x2B84), /*!< Coordinated Set Identification Service - Set Identity Resolving Key characteristic */
    BLE_GATT_CHAR_CSIS_SIZE             = BLE_GATT_UUID_16_LSB(0x2B85), /*!< Coordinated Set Identification Service - Coordinated Set Size characteristic */
    BLE_GATT_CHAR_CSIS_LOCK             = BLE_GATT_UUID_16_LSB(0x2B86), /*!< Coordinated Set Identification Service - Set Member Lock characteristic */
    BLE_GATT_CHAR_CSIS_RANK             = BLE_GATT_UUID_16_LSB(0x2B87), /*!< Coordinated Set Identification Service - Set Member Rank characteristic */

    BLE_GATT_CHAR_TBS_PROV_NAME         = BLE_GATT_UUID_16_LSB(0x2BB3), /*!< Telephone Bearer Service - Bearer Provider Name characteristic */
    BLE_GATT_CHAR_TBS_UCI               = BLE_GATT_UUID_16_LSB(0x2BB4), /*!< Telephone Bearer Service - Bearer UCI characteristic */
    BLE_GATT_CHAR_TBS_TECHNO            = BLE_GATT_UUID_16_LSB(0x2BB5), /*!< Telephone Bearer Service - Bearer Technology characteristic */
    BLE_GATT_CHAR_TBS_URI_SCHEMES_LIST  = BLE_GATT_UUID_16_LSB(0x2BB6), /*!< Telephone Bearer Service - Bearer URI Schemes Supported List characteristic */
    BLE_GATT_CHAR_TBS_SIGN_STRENGTH     = BLE_GATT_UUID_16_LSB(0x2BB7), /*!< Telephone Bearer Service - Bearer Signal Strength characteristic */
    BLE_GATT_CHAR_TBS_SIGN_STRENGTH_INTV = BLE_GATT_UUID_16_LSB(0x2BB8), /*!< Telephone Bearer Service - Bearer Signal Strength Reporting Interval characteristic */
    BLE_GATT_CHAR_TBS_CURR_CALLS_LIST   = BLE_GATT_UUID_16_LSB(0x2BB9), /*!< Telephone Bearer Service - Bearer List Current Calls characteristic */
    BLE_GATT_CHAR_TBS_STATUS_FLAGS      = BLE_GATT_UUID_16_LSB(0x2BBB), /*!< Telephone Bearer Service - Status Flags characteristic */
    BLE_GATT_CHAR_TBS_IN_TGT_CALLER_ID  = BLE_GATT_UUID_16_LSB(0x2BBC), /*!< Telephone Bearer Service - Incoming Call Target Bearer URI characteristic */
    BLE_GATT_CHAR_TBS_CALL_STATE        = BLE_GATT_UUID_16_LSB(0x2BBD), /*!< Telephone Bearer Service - Call State characteristic */
    BLE_GATT_CHAR_TBS_CALL_CP           = BLE_GATT_UUID_16_LSB(0x2BBE), /*!< Telephone Bearer Service - Call Control Point characteristic */
    BLE_GATT_CHAR_TBS_CALL_CP_OPT_OPCODES = BLE_GATT_UUID_16_LSB(0x2BBF), /*!< Telephone Bearer Service - Call Control Point Optional Opcodes characteristic */
    BLE_GATT_CHAR_TBS_TERM_REASON       = BLE_GATT_UUID_16_LSB(0x2BC0), /*!< Telephone Bearer Service - Termination Reason characteristic */
    BLE_GATT_CHAR_TBS_INCOMING_CALL     = BLE_GATT_UUID_16_LSB(0x2BC1), /*!< Telephone Bearer Service - Incoming Call characteristic */
    BLE_GATT_CHAR_TBS_CALL_FRIENDLY_NAME = BLE_GATT_UUID_16_LSB(0x2BC2), /*!< Telephone Bearer Service - Call Friendly Name characteristic */

    BLE_GATT_CHAR_MCS_PLAYER_NAME       = BLE_GATT_UUID_16_LSB(0x2B93), /*!< Media Control Service - Media Player Name characteristic */
    BLE_GATT_CHAR_MCS_PLAYER_ICON_OBJ_ID = BLE_GATT_UUID_16_LSB(0x2B94), /*!< Media Control Service - Media Player Icon Object ID characteristic */
    BLE_GATT_CHAR_MCS_PLAYER_ICON_URL   = BLE_GATT_UUID_16_LSB(0x2B95), /*!< Media Control Service - Media Player Icon URL characteristic */
    BLE_GATT_CHAR_MCS_TRACK_CHANGED     = BLE_GATT_UUID_16_LSB(0x2B96), /*!< Media Control Service - Track Changed characteristic */
    BLE_GATT_CHAR_MCS_TRACK_TITLE       = BLE_GATT_UUID_16_LSB(0x2B97), /*!< Media Control Service - Track Title characteristic */
    BLE_GATT_CHAR_MCS_TRACK_DURATION    = BLE_GATT_UUID_16_LSB(0x2B98), /*!< Media Control Service - Track Duration characteristic */
    BLE_GATT_CHAR_MCS_TRACK_POSITION    = BLE_GATT_UUID_16_LSB(0x2B99), /*!< Media Control Service - Track Position characteristic */
    BLE_GATT_CHAR_MCS_PLAYBACK_SPEED    = BLE_GATT_UUID_16_LSB(0x2B9A), /*!< Media Control Service - Playback Speed characteristic */
    BLE_GATT_CHAR_MCS_SEEKING_SPEED     = BLE_GATT_UUID_16_LSB(0x2B9B), /*!< Media Control Service - Seeking Speed characteristic */
    BLE_GATT_CHAR_MCS_CUR_TRACK_SEG_OBJ_ID = BLE_GATT_UUID_16_LSB(0x2B9C), /*!< Media Control Service - Current Track Segments Object ID characteristic */
    BLE_GATT_CHAR_MCS_CUR_TRACK_OBJ_ID  = BLE_GATT_UUID_16_LSB(0x2B9D), /*!< Media Control Service - Current Track Object ID characteristic */
    BLE_GATT_CHAR_MCS_NEXT_TRACK_OBJ_ID = BLE_GATT_UUID_16_LSB(0x2B9E), /*!< Media Control Service - Next Track Object ID characteristic */
    BLE_GATT_CHAR_MCS_CUR_GROUP_OBJ_ID  = BLE_GATT_UUID_16_LSB(0x2BA0), /*!< Media Control Service - Current Group Object ID characteristic */
    BLE_GATT_CHAR_MCS_PARENT_GROUP_OBJ_ID = BLE_GATT_UUID_16_LSB(0x2B9F), /*!< Media Control Service - Parent Group Object ID characteristic */
    BLE_GATT_CHAR_MCS_PLAYING_ORDER     = BLE_GATT_UUID_16_LSB(0x2BA1), /*!< Media Control Service - Playing Order characteristic */
    BLE_GATT_CHAR_MCS_PLAYING_ORDER_SUPP = BLE_GATT_UUID_16_LSB(0x2BA2), /*!< Media Control Service - Playing Order Supported characteristic */
    BLE_GATT_CHAR_MCS_MEDIA_STATE       = BLE_GATT_UUID_16_LSB(0x2BA3), /*!< Media Control Service - Media State characteristic */
    BLE_GATT_CHAR_MCS_MEDIA_CP          = BLE_GATT_UUID_16_LSB(0x2BA4), /*!< Media Control Service - Media Control Point characteristic */
    BLE_GATT_CHAR_MCS_MEDIA_CP_OPCODES_SUPP = BLE_GATT_UUID_16_LSB(0x2BA5), /*!< Media Control Service - Media Control Point Opcodes Supported characteristic */
    BLE_GATT_CHAR_MCS_SEARCH_RESULTS_OBJ_ID = BLE_GATT_UUID_16_LSB(0x2BA6), /*!< Media Control Service - Search Results Object ID characteristic */
    BLE_GATT_CHAR_MCS_SEARCH_CP         = BLE_GATT_UUID_16_LSB(0x2BA7), /*!< Media Control Service - Search Control Point characteristic */

    BLE_GATT_CHAR_CCID                  = BLE_GATT_UUID_16_LSB(0x2BBA), /*!< Content Control ID characteristic */

    BLE_GATT_CHAR_TMAS_ROLE             = BLE_GATT_UUID_16_LSB(0x8FC9), /*!< Telephony and Media Audio Service - TMAP Role characteristic */

    BLE_GATT_CHAR_HAS_FEATURES          = BLE_GATT_UUID_16_LSB(0x8FCA), /*!< Hearing Access Service - Hearing Aid Features characteristic */
    BLE_GATT_CHAR_HAS_CP                = BLE_GATT_UUID_16_LSB(0x8FCB), /*!< Hearing Access Service - Hearing Aid Preset Control Point characteristic */
    BLE_GATT_CHAR_HAS_ACTIVE_PRESET_INDEX = BLE_GATT_UUID_16_LSB(0x8FCC), /*!< Hearing Access Service - Active Preset Index characteristic */


    BLE_GATT_CHAR_CLI_SUP_FEAT          = BLE_GATT_UUID_16_LSB(0x2B29), /*!< Client Supported Features */
    BLE_GATT_CHAR_DB_HASH               = BLE_GATT_UUID_16_LSB(0x2B2A), /*!< Database Hash */
    BLE_GATT_CHAR_SRV_SUP_FEAT          = BLE_GATT_UUID_16_LSB(0x2B3A), /*!< Server Supported Features */
};

/* GATT service discovery type */
enum ble_gatt_svc_discovery_type
{
    BLE_GATT_DISCOVER_SVC_PRIMARY_ALL = 0x00,       /*!< Discover all primary services */
    BLE_GATT_DISCOVER_SVC_PRIMARY_BY_UUID = 0x01,   /*!< Discover primary services using UUID value */
    BLE_GATT_DISCOVER_SVC_SECONDARY_ALL = 0x02,     /*!< Discover all secondary services */
    BLE_GATT_DISCOVER_SVC_SECONDARY_BY_UUID = 0x03, /*!< Discover secondary services using UUID value */
};

/* GATT characteristic discovery type */
enum ble_gatt_char_discovery_type
{
    BLE_GATT_DISCOVER_CHAR_ALL = 0x00,      /*!< Discover all characteristics */
    BLE_GATT_DISCOVER_CHAR_BY_UUID = 0x01,  /*!< Discover characteristics using UUID value */
};

/* GATT write type */
typedef enum
{
    BLE_GATT_WRITE = 0x00,          /*!< Write attribute */
    BLE_GATT_WRITE_NO_RESP = 0x01,  /*!< Write attribute without response */
    BLE_GATT_WRITE_SIGNED = 0x02,   /*!< Write attribute signed */
} ble_gatt_write_type_t;

/* GATT attribute descriptor */
typedef struct
{
    uint8_t  uuid[BLE_GATT_UUID_128_LEN];   /*!< Attribute UUID (LSB First) */
    uint16_t info;                          /*!< Attribute information bit field, @ref ble_gatt_attr_info_bf */
    uint16_t ext_info;                      /*!< Attribute extended information bit field, @ref ble_gatt_attr_ext_info_bf.
                                                    For Included Services and Characteristic Declarations, this field contains targeted handle.
                                                    For Characteristic Extended Properties, this field contains 2 byte value.
                                                    For Client Characteristic Configuration and Server Characteristic Configuration, this field is not used */
} ble_gatt_attr_desc_t;

/* GATT 16-bit UUID attribute descriptor */
typedef struct
{
    uint16_t uuid16;    /*!< 16-bit attribute UUID */
    uint16_t info;      /*!< Attribute information bit field, @ref ble_gatt_attr_info_bf */
    uint16_t ext_info;  /*!< Attribute extended information bit field, @ref ble_gatt_attr_ext_info_bf.
                                For Included Services and Characteristic Declarations, this field contains targeted handle.
                                For Characteristic Extended Properties, this field contains 2 byte value.
                                For Client Characteristic Configuration and Server Characteristic Configuration, this field is not used */
} ble_gatt_attr_16_desc_t;

/* GATT attribute value information */
typedef struct
{
    uint16_t    hdl;        /*!< Attribute handle */
    uint16_t    length;     /*!< Value length */
} ble_gatt_attr_t;

/* Response of register a GATT user */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;   /*!< GATT user local identifier */
    uint16_t    metainfo;   /*!< Metadata information provided by API */
} ble_gatt_user_reg_rsp_t;

/* Response of unregister a GATT user */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;   /*!< GATT user local identifier */
} ble_gatt_user_unreg_rsp_t;

/* Response of close a GATT bearer */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;   /*!< GATT user local identifier */
    uint8_t     conn_idx;   /*!< Connection index */
} ble_gatt_bearer_close_rsp_t;

/* Response of establish an EATT bearer */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;   /*!< GATT user local identifier */
    uint8_t     conn_idx;   /*!< Connection index */
} ble_gatt_bearer_eatt_estab_rsp_t;

typedef struct
{
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    mtu;            /*!< Mtu size */
} ble_gatt_mtu_exch_info_t;

/* Response of add a GATT service */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;   /*!< GATT user local identifier */
    uint16_t    start_hdl;  /*!< Service start handle */
    uint16_t    metainfo;   /*!< Metadata information provided by API */
} ble_gatt_svc_add_rsp_t;

/* Response of remove a GATT service */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;   /*!< GATT user local identifier */
    uint16_t    metainfo;   /*!< Metadata information provided by API */
} ble_gatt_svc_remove_rsp_t;

/* Response of get GATT database hash */
typedef struct
{
    uint16_t    status;                         /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;                       /*!< GATT user local identifier */
    uint8_t     hash[BLE_GATT_DB_HASH_LEN];     /*!< Database Hash */
} ble_gatt_db_hash_get_rsp_t;

/* Response of remove all GATT services */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;   /*!< GATT user local identifier */
} ble_gatt_svc_remove_all_rsp_t;

/* GATT service descriptor */
typedef struct
{
    uint8_t     user_lid;                       /*!< GATT User Local identifier */
    uint8_t     info;                           /*!< Service Information bit field, @ref ble_gatt_svc_info_bf */
    uint8_t     uuid[BLE_GATT_UUID_128_LEN];    /*!< Service UUID (LSB first) */
    uint16_t    start_hdl;                      /*!< Attribute start Handle */
    uint16_t    end_hdl;                        /*!< Attribute end Handle */
} ble_gatt_svc_desc_t;

/* Response of get GATT service list */
typedef struct
{
    uint16_t             status;        /*!< Response status, @ref ble_status_t */
    uint8_t              user_lid;      /*!< GATT user local identifier */
    uint8_t              svc_num;       /*!< Number of services */
    ble_gatt_svc_desc_t *p_svc;         /*!< List of service description */
} ble_gatt_svc_list_get_rsp_t;

/* Response of set GATT service */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;       /*!< GATT user local identifier */
} ble_gatt_svc_info_set_rsp_t;

/* Response of get GATT attribute information */
typedef struct
{
    uint16_t                status;     /*!< Response status, @ref ble_status_t */
    uint8_t                 user_lid;   /*!< GATT user local identifier */
    uint16_t                hdl;        /*!< Attribute handle */
    ble_gatt_attr_desc_t    attr;       /*!< Attribute description */
} ble_gatt_attr_info_get_rsp_t;

/* Response of set GATT attribute information */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;       /*!< GATT user local identifier */
} ble_gatt_attr_info_set_rsp_t;

/* Response of GATT server reliable send data */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;   /*!< GATT user local identifier */
    uint8_t     conn_idx;   /*!< Connection index */
    uint8_t     type;       /*!< Send type, @ref ble_gatt_evt_type_t */
} ble_gatt_srv_reliable_send_rsp_t;

/* Response of GATT server send data */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;   /*!< GATT user local identifier */
    uint8_t     conn_idx;   /*!< Connection index */
    uint16_t    hdl;        /*!< Attribute handle */
    uint8_t     type;       /*!< Send type, @ref ble_gatt_evt_type_t */
} ble_gatt_srv_send_rsp_t;

/* Response of GATT multiple send data */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;   /*!< GATT user local identifier */
    uint8_t     conn_idx;   /*!< Connection index */
    uint16_t    hdl;        /*!< Attribute handle */
    uint8_t     type;       /*!< Send type, @ref ble_gatt_evt_type_t */
} ble_gatt_srv_mtp_send_rsp_t;

/* Response of cancel GATT multiple send */
typedef struct
{
    uint16_t    status;     /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;   /*!< GATT user local identifier */
} ble_gatt_srv_mtp_cancel_rsp_t;

/* Indication of peer read attribute request */
typedef struct
{
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    token;          /*!< Token provided by GATT module that must be used in the confirm */
    uint16_t    hdl;            /*!< Attribute handle */
    uint16_t    offset;         /*!< Value offset */
    uint16_t    max_len;        /*!< Maximum value length to return */
} ble_gatt_srv_attr_read_req_ind_t;

/* Indication of peer get notification/inidcation */
typedef struct
{
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    token;          /*!< Token provided by GATT module that must be used in the confirm */
    uint16_t    hdl;            /*!< Attribute handle */
    uint16_t    max_len;        /*!< Maximum value length to return */
} ble_gatt_srv_attr_ntf_ind_get_ind_t;

/* Indication of peer get attribute */
typedef struct
{
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    token;          /*!< Token provided by GATT module that must be used in the confirm */
    uint16_t    hdl;            /*!< Attribute handle */
} ble_gatt_srv_attr_info_get_ind_t;

/* Indication of peer set attribute value */
typedef struct
{
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    token;          /*!< Token provided by GATT module that must be used in the confirm */
    uint16_t    hdl;            /*!< Attribute handle */
    uint16_t    offset;         /*!< Value offset */
    uint16_t    value_len;      /*!< Value length to write */
    uint8_t    *p_value;        /*!< Attribute value to update, starting from offset */
} ble_gatt_srv_attr_val_set_ind_t;

/* Response of discover services */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
    uint8_t     disc_type;       /*!< Discovery type\(see enum #ble_gatt_svc_discovery_type) */
} ble_gatt_cli_disc_svc_rsp_t;

/* Response of discover include services */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
} ble_gatt_cli_disc_inc_svc_rsp_t;

/* Response of discover characteristic */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
} ble_gatt_cli_disc_char_rsp_t;

/* Response of discover descriptor */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
} ble_gatt_cli_disc_desc_rsp_t;

/* Response of cancel discover procedure */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
} ble_gatt_cli_disc_cancel_rsp_t;

/* Response of GATT read */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    att_hdl;        /*!< Attribute handle */
} ble_gatt_cli_read_rsp_t;

/* Response of GATT read by UUID */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
} ble_gatt_cli_read_by_uuid_rsp_t;

/* Response of GATT multiple read */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
} ble_gatt_cli_read_multiple_rsp_t;

/* Response of reliable write */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    att_hdl;        /*!< Attribute handle */
} ble_gatt_cli_write_reliable_rsp_t;

/* Response of write procedure */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    att_hdl;        /*!< Attribute handle */
    uint8_t     type;           /*!< Write type, @ref ble_gatt_write_type_t */
} ble_gatt_cli_write_rsp_t;

/* Response of execute write */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
    bool        execute;        /*!< True for write execute, false for write cancel */
} ble_gatt_cli_write_exe_rsp_t;

/* Response of register notification/indication */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
} ble_gatt_cli_ntf_ind_reg_rsp_t;

/* Response of unregister notification/indication */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
} ble_gatt_cli_ntf_ind_unreg_rsp_t;

/* Response of GATT MTU update */
typedef struct
{
    uint16_t    status;         /*!< Response status, @ref ble_status_t */
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
} ble_gatt_cli_mtu_update_rsp_t;

/* GATT service attribute */
typedef struct
{
    uint8_t     attr_type;                      /*!< Attribute Type, @ref ble_gatt_attr_type */
    uint8_t     uuid_type;                      /*!< UUID type, @ref ble_gatt_uuid_type */
    uint8_t     uuid[BLE_GATT_UUID_128_LEN];    /*!< Attribute UUID */

    union ble_gatt_info
    {
        struct ble_gatt_svc_info
        {
            uint16_t    start_hdl;              /*!< Service start handle */
            uint16_t    end_hdl;                /*!< Service end handle */
        } svc;                                  /*!< Service information */

        struct ble_gatt_char_info
        {
            uint16_t    val_hdl;                /*!< Value handle */
            uint8_t     prop;                   /*!< Characteristic properties,  bits [0-7] of @ref ble_gatt_attr_info_bf */
        } charac;                               /*!< Characteristic information */
    } info;                                     /*!< Information about Service attribute */
} ble_gatt_svc_attr_t;

/* Information of service discovery result */
typedef struct
{
    uint8_t              user_lid;      /*!< GATT user local identifier */
    uint8_t              conn_idx;      /*!< Connection index */
    uint16_t             hdl;           /*!< First handle value of following list */
    uint8_t              disc_info;     /*!< Discovery information, @ref ble_gatt_svc_disc_info */
    uint8_t              attr_num;      /*!< Number of attribute */
    ble_gatt_svc_attr_t *p_attr;        /*!< Attribute information present in a service */
} ble_gatt_cli_svc_t;

/* GATT service information */
typedef struct
{
    uint8_t     user_lid;                       /*!< GATT user local identifier */
    uint8_t     conn_idx;                       /*!< Connection index */
    uint16_t    start_hdl;                      /*!< Service start handle */
    uint16_t    end_hdl;                        /*!< Service end handle */
    uint8_t     uuid_type;                      /*!< UUID Type, @ref ble_gatt_uuid_type */
    uint8_t     uuid[BLE_GATT_UUID_128_LEN];    /*!< Service UUID */
} ble_gatt_cli_svc_info_t;

/* GATT include service information */
typedef struct
{
    uint8_t     user_lid;                       /*!< GATT user local identifier */
    uint8_t     conn_idx;                       /*!< Connection index */
    uint16_t    inc_svc_hdl;                    /*!< Include service attribute handle */
    uint16_t    start_hdl;                      /*!< Service start handle */
    uint16_t    end_hdl;                        /*!< Service end handle */
    uint8_t     uuid_type;                      /*!< UUID Type, @ref ble_gatt_uuid_type */
    uint8_t     uuid[BLE_GATT_UUID_128_LEN];    /*!< Service UUID */
} ble_gatt_cli_inc_svc_info_t;

/* GATT characteristic information */
typedef struct
{
    uint8_t     user_lid;                       /*!< GATT user local identifier */
    uint8_t     conn_idx;                       /*!< Connection index */
    uint16_t    char_hdl;                       /*!< Characteristic attribute handle */
    uint16_t    value_hdl;                      /*!< Value handle */
    uint8_t     prop;                           /*!< Characteristic properties, bits [0-7] of @ref ble_gatt_attr_info_bf */
    uint8_t     uuid_type;                      /*!< UUID Type, @ref ble_gatt_uuid_type */
    uint8_t     uuid[BLE_GATT_UUID_128_LEN];    /*!< Characteristic value UUID */
} ble_gatt_cli_char_info_t;

/* GATT characteristic descriptor information */
typedef struct
{
    uint8_t     user_lid;                       /*!< GATT user local identifier */
    uint8_t     conn_idx;                       /*!< Connection index */
    uint16_t    desc_hdl;                       /*!< Characteristic descriptor attribute handle */
    uint8_t     uuid_type;                      /*!< UUID Type, @ref ble_gatt_uuid_type */
    uint8_t     uuid[BLE_GATT_UUID_128_LEN];    /*!< Attribute UUID */
} ble_gatt_cli_desc_info_t;

/* GATT attribute value information */
typedef struct
{
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    hdl;            /*!< Attribute handle */
    uint16_t    offset;         /*!< Data offset */
    uint16_t    value_len;      /*!< Value length */
    uint8_t    *p_value;        /*!< Attribute value starting from offset */
} ble_gatt_cli_attr_val_info_t;

/* GATT service changed information */
typedef struct
{
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
    bool        out_of_sync;    /*!< True if an out of sync error has been received */
    uint16_t    start_hdl;      /*!< Service start handle */
    uint16_t    end_hdl;        /*!< Service end handle */
} ble_gatt_cli_svc_changed_info_t;

/* Indication of peer get attribute value */
typedef struct
{
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    token;          /*!< Token provided by GATT module that must be used in the confirm */
    uint16_t    hdl;            /*!< Attribute handle */
    uint16_t    offset;         /*!< Data offset */
    uint16_t    max_len;        /*!< Maximum value length to return */
} ble_gatt_cli_attr_val_get_ind_t;

/* Indication of peer send notificaiton/indication */
typedef struct
{
    uint8_t     user_lid;       /*!< GATT user local identifier */
    uint8_t     conn_idx;       /*!< Connection index */
    uint16_t    token;          /*!< Token provided by GATT module that must be used in the confirm */
    bool        ntf;            /*!< True: notification, false: indication */
    bool        complete;       /*!< True if complete value has been received
                                     False if data received equals to max attribute protocol value. In such case client user should perform a read procedure */
    uint16_t    hdl;            /*!< Attribute handle */
    uint16_t    value_len;      /*!< Value length */
    uint8_t    *p_value;        /*!< Attribute value */
} ble_gatt_cli_attr_ntf_ind_ind_t;

bool ble_gatt_uuid_cmp(uint8_t *p_uuid_a, uint8_t uuid_a_type, uint8_t *p_uuid_b, uint8_t uuid_b_type);

#ifdef __cplusplus
}
#endif

#endif // _BLE_GATT_H_
