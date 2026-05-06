/*!
    \file    ble_types.h
    \brief   Common types and macro definitions for BLE.

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

#ifndef _BLE_TYPES_H_
#define _BLE_TYPES_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Invalid connection index */
#define BLE_CONN_CONIDX_INVALID                       0xFF

/* Invalid connection handle */
#define BLE_CONN_HANDLE_INVALID                       0xFFFF

/* Generic UUIDs, applicable to all services */
#define BLE_UUID_UNKNOWN                              0x0000    /*!< Reserved UUID */
#define BLE_UUID_SERVICE_PRIMARY                      0x2800    /*!< Primary Service */
#define BLE_UUID_SERVICE_SECONDARY                    0x2801    /*!< Secondary Service */
#define BLE_UUID_SERVICE_INCLUDE                      0x2802    /*!< Include */
#define BLE_UUID_CHARACTERISTIC                       0x2803    /*!< Characteristic */
#define BLE_UUID_DESCRIPTOR_CHAR_EXT_PROP             0x2900    /*!< Characteristic Extended Properties Descriptor */
#define BLE_UUID_DESCRIPTOR_CHAR_USER_DESC            0x2901    /*!< Characteristic User Description Descriptor */
#define BLE_UUID_DESCRIPTOR_CLIENT_CHAR_CONFIG        0x2902    /*!< Client Characteristic Configuration Descriptor */
#define BLE_UUID_DESCRIPTOR_SERVER_CHAR_CONFIG        0x2903    /*!< Server Characteristic Configuration Descriptor */
#define BLE_UUID_DESCRIPTOR_CHAR_PRESENTATION_FORMAT  0x2904    /*!< Characteristic Presentation Format Descriptor */
#define BLE_UUID_DESCRIPTOR_CHAR_AGGREGATE_FORMAT     0x2905    /*!< Characteristic Aggregate Format Descriptor */
#define BLE_UUID_GATT                                 0x1801    /*!< Generic Attribute Profile */
#define BLE_UUID_GATT_CHARACTERISTIC_SERVICE_CHANGED  0x2A05    /*!< Service Changed Characteristic */
#define BLE_UUID_GAP                                  0x1800    /*!< Generic Access Profile */
#define BLE_UUID_GAP_CHARACTERISTIC_DEVICE_NAME       0x2A00    /*!< Device Name Characteristic */
#define BLE_UUID_GAP_CHARACTERISTIC_APPEARANCE        0x2A01    /*!< Appearance Characteristic */
#define BLE_UUID_GAP_CHARACTERISTIC_RECONN_ADDR       0x2A03    /*!< Reconnection Address Characteristic */
#define BLE_UUID_GAP_CHARACTERISTIC_PPCP              0x2A04    /*!< Peripheral Preferred Connection Parameters Characteristic */
#define BLE_UUID_GAP_CHARACTERISTIC_CAR               0x2AA6    /*!< Central Address Resolution Characteristic */
#define BLE_UUID_GAP_CHARACTERISTIC_RPA_ONLY          0x2AC9    /*!< Resolvable Private Address Only Characteristic */

/* AD types used in advertising and scan response data */
#define BLE_AD_TYPE_FLAGS                               0x01    /*!< Flags for discoverability */
#define BLE_AD_TYPE_SERVICE_UUID_16_MORE                0x02    /*!< Partial list of 16 bit service UUIDs */
#define BLE_AD_TYPE_SERVICE_UUID_16_COMPLETE            0x03    /*!< Complete list of 16 bit service UUIDs */
#define BLE_AD_TYPE_SERVICE_UUID_32_MORE                0x04    /*!< Partial list of 32 bit service UUIDs */
#define BLE_AD_TYPE_SERVICE_UUID_32_COMPLETE            0x05    /*!< Complete list of 32 bit service UUIDs */
#define BLE_AD_TYPE_SERVICE_UUID_128_MORE               0x06    /*!< Partial list of 128 bit service UUIDs */
#define BLE_AD_TYPE_SERVICE_UUID_128_COMPLETE           0x07    /*!< Complete list of 128 bit service UUIDs */
#define BLE_AD_TYPE_SHORT_LOCAL_NAME                    0x08    /*!< Short local device name */
#define BLE_AD_TYPE_COMPLETE_LOCAL_NAME                 0x09    /*!< Complete local device name */
#define BLE_AD_TYPE_TX_POWER_LEVEL                      0x0A    /*!< Transmit power level */
#define BLE_AD_TYPE_CLASS_OF_DEVICE                     0x0D    /*!< Class of device */
#define BLE_AD_TYPE_SIMPLE_PAIRING_HASH_C               0x0E    /*!< Simple Pairing Hash C */
#define BLE_AD_TYPE_SIMPLE_PAIRING_RANDOMIZER_R         0x0F    /*!< Simple Pairing Randomizer R */
#define BLE_AD_TYPE_SECURITY_MANAGER_TK_VALUE           0x10    /*!< Security Manager TK Value */
#define BLE_AD_TYPE_SECURITY_MANAGER_OOB_FLAGS          0x11    /*!< Security Manager Out Of Band Flags */
#define BLE_AD_TYPE_SLAVE_CONNECTION_INTERVAL_RANGE     0x12    /*!< Slave Connection Interval Range */
#define BLE_AD_TYPE_SOLICITED_SERVICE_UUID_16           0x14    /*!< List of 16-bit Service Solicitation UUIDs */
#define BLE_AD_TYPE_SOLICITED_SERVICE_UUID_128          0x15    /*!< List of 128-bit Service Solicitation UUIDs */
#define BLE_AD_TYPE_SERVICE_DATA_UUID_16                0x16    /*!< Service Data - 16-bit UUID */
#define BLE_AD_TYPE_PUBLIC_TARGET_ADDRESS               0x17    /*!< Public Target Address */
#define BLE_AD_TYPE_RANDOM_TARGET_ADDRESS               0x18    /*!< Random Target Address */
#define BLE_AD_TYPE_APPEARANCE                          0x19    /*!< Appearance */
#define BLE_AD_TYPE_ADVERTISING_INTERVAL                0x1A    /*!< Advertising Interval */
#define BLE_AD_TYPE_LE_BLUETOOTH_DEVICE_ADDRESS         0x1B    /*!< LE Bluetooth Device Address */
#define BLE_AD_TYPE_LE_ROLE                             0x1C    /*!< LE Role */
#define BLE_AD_TYPE_SIMPLE_PAIRING_HASH_C256            0x1D    /*!< Simple Pairing Hash C-256 */
#define BLE_AD_TYPE_SIMPLE_PAIRING_RANDOMIZER_R256      0x1E    /*!< Simple Pairing Randomizer R-256 */
#define BLE_AD_TYPE_SOLICITED_SERVICE_UUID_32           0x1F    /*!< List of 32-bit Service Solicitation UUIDs */
#define BLE_AD_TYPE_SERVICE_DATA_UUID_32                0x20    /*!< Service Data - 32-bit UUID */
#define BLE_AD_TYPE_SERVICE_DATA_UUID_128               0x21    /*!< Service Data - 128-bit UUID */
#define BLE_AD_TYPE_LESC_CONFIRMATION_VALUE             0x22    /*!< LE Secure Connections Confirmation Value */
#define BLE_AD_TYPE_LESC_RANDOM_VALUE                   0x23    /*!< LE Secure Connections Random Value */
#define BLE_AD_TYPE_URI                                 0x24    /*!< URI */
#define BLE_AD_TYPE_MESH_PROV                           0x29    /*!< Mesh Provisioning PDU */
#define BLE_AD_TYPE_MESH_MESSAGE                        0x2a    /*!< Mesh Networking PDU */
#define BLE_AD_TYPE_MESH_BEACON                         0x2b    /*!< Mesh Beacon */
#define BLE_AD_TYPE_ADVERTISING_INTERVAL_LONG           0x2F    /*!< Advertising Interval Long */
#define BLE_AD_TYPE_3D_INFORMATION_DATA                 0x3D    /*!< 3D Information Data */
#define BLE_AD_TYPE_MANUFACTURER_SPECIFIC_DATA          0xFF    /*!< Manufacturer Specific Data */

/* BLE appearance values */
#define BLE_APPEARANCE_UNKNOWN                                0 /*!< Unknown */
#define BLE_APPEARANCE_GENERIC_PHONE                         64 /*!< Generic Phone */
#define BLE_APPEARANCE_GENERIC_COMPUTER                     128 /*!< Generic Computer */
#define BLE_APPEARANCE_GENERIC_WATCH                        192 /*!< Generic Watch */
#define BLE_APPEARANCE_WATCH_SPORTS_WATCH                   193 /*!< Watch: Sports Watch */
#define BLE_APPEARANCE_GENERIC_CLOCK                        256 /*!< Generic Clock */
#define BLE_APPEARANCE_GENERIC_DISPLAY                      320 /*!< Generic Display */
#define BLE_APPEARANCE_GENERIC_REMOTE_CONTROL               384 /*!< Generic Remote Control */
#define BLE_APPEARANCE_GENERIC_EYE_GLASSES                  448 /*!< Generic Eye-glasses */
#define BLE_APPEARANCE_GENERIC_TAG                          512 /*!< Generic Tag */
#define BLE_APPEARANCE_GENERIC_KEYRING                      576 /*!< Generic Keyring */
#define BLE_APPEARANCE_GENERIC_MEDIA_PLAYER                 640 /*!< Generic Media Player */
#define BLE_APPEARANCE_GENERIC_BARCODE_SCANNER              704 /*!< Generic Barcode Scanner */
#define BLE_APPEARANCE_GENERIC_THERMOMETER                  768 /*!< Generic Thermometer */
#define BLE_APPEARANCE_THERMOMETER_EAR                      769 /*!< Thermometer: Ear */
#define BLE_APPEARANCE_GENERIC_HEART_RATE_SENSOR            832 /*!< Generic Heart rate Sensor */
#define BLE_APPEARANCE_HEART_RATE_SENSOR_HEART_RATE_BELT    833 /*!< Heart Rate Sensor: Heart Rate Belt */
#define BLE_APPEARANCE_GENERIC_BLOOD_PRESSURE               896 /*!< Generic Blood Pressure */
#define BLE_APPEARANCE_BLOOD_PRESSURE_ARM                   897 /*!< Blood Pressure: Arm */
#define BLE_APPEARANCE_BLOOD_PRESSURE_WRIST                 898 /*!< Blood Pressure: Wrist */
#define BLE_APPEARANCE_GENERIC_HID                          960 /*!< Human Interface Device (HID) */
#define BLE_APPEARANCE_HID_KEYBOARD                         961 /*!< Keyboard (HID Subtype) */
#define BLE_APPEARANCE_HID_MOUSE                            962 /*!< Mouse (HID Subtype) */
#define BLE_APPEARANCE_HID_JOYSTICK                         963 /*!< Joystick (HID Subtype) */
#define BLE_APPEARANCE_HID_GAMEPAD                          964 /*!< Gamepad (HID Subtype) */
#define BLE_APPEARANCE_HID_DIGITIZERSUBTYPE                 965 /*!< Digitizer Tablet (HID Subtype) */
#define BLE_APPEARANCE_HID_CARD_READER                      966 /*!< Card Reader (HID Subtype) */
#define BLE_APPEARANCE_HID_DIGITAL_PEN                      967 /*!< Digital Pen (HID Subtype) */
#define BLE_APPEARANCE_HID_BARCODE                          968 /*!< Barcode Scanner (HID Subtype) */
#define BLE_APPEARANCE_GENERIC_GLUCOSE_METER               1024 /*!< Generic Glucose Meter */
#define BLE_APPEARANCE_GENERIC_RUNNING_WALKING_SENSOR      1088 /*!< Generic Running Walking Sensor */
#define BLE_APPEARANCE_RUNNING_WALKING_SENSOR_IN_SHOE      1089 /*!< Running Walking Sensor: In-Shoe */
#define BLE_APPEARANCE_RUNNING_WALKING_SENSOR_ON_SHOE      1090 /*!< Running Walking Sensor: On-Shoe */
#define BLE_APPEARANCE_RUNNING_WALKING_SENSOR_ON_HIP       1091 /*!< Running Walking Sensor: On-Hip */
#define BLE_APPEARANCE_GENERIC_CYCLING                     1152 /*!< Generic Cycling */
#define BLE_APPEARANCE_CYCLING_CYCLING_COMPUTER            1153 /*!< Cycling: Cycling Computer */
#define BLE_APPEARANCE_CYCLING_SPEED_SENSOR                1154 /*!< Cycling: Speed Sensor */
#define BLE_APPEARANCE_CYCLING_CADENCE_SENSOR              1155 /*!< Cycling: Cadence Sensor */
#define BLE_APPEARANCE_CYCLING_POWER_SENSOR                1156 /*!< Cycling: Power Sensor */
#define BLE_APPEARANCE_CYCLING_SPEED_CADENCE_SENSOR        1157 /*!< Cycling: Speed and Cadence Sensor */
#define BLE_APPEARANCE_GENERIC_PULSE_OXIMETER              3136 /*!< Generic Pulse Oximeter */
#define BLE_APPEARANCE_PULSE_OXIMETER_FINGERTIP            3137 /*!< Fingertip (Pulse Oximeter subtype) */
#define BLE_APPEARANCE_PULSE_OXIMETER_WRIST_WORN           3138 /*!< Wrist Worn(Pulse Oximeter subtype) */
#define BLE_APPEARANCE_GENERIC_WEIGHT_SCALE                3200 /*!< Generic Weight Scale */
#define BLE_APPEARANCE_GENERIC_OUTDOOR_SPORTS_ACT          5184 /*!< Generic Outdoor Sports Activity */
#define BLE_APPEARANCE_OUTDOOR_SPORTS_ACT_LOC_DISP         5185 /*!< Location Display Device (Outdoor Sports Activity subtype) */
#define BLE_APPEARANCE_OUTDOOR_SPORTS_ACT_LOC_AND_NAV_DISP 5186 /*!< Location and Navigation Display Device (Outdoor Sports Activity subtype) */
#define BLE_APPEARANCE_OUTDOOR_SPORTS_ACT_LOC_POD          5187 /*!< Location Pod (Outdoor Sports Activity subtype) */
#define BLE_APPEARANCE_OUTDOOR_SPORTS_ACT_LOC_AND_NAV_POD  5188 /*!< Location and Navigation Pod (Outdoor Sports Activity subtype) */

/* BLE UUID type */
typedef enum
{
    BLE_UUID_TYPE_16,       /*!< 16-bit UUID */
    BLE_UUID_TYPE_32,       /*!< 32-bit UUID */
    BLE_UUID_TYPE_128       /*!< 128-bit UUID */
} ble_uuid_type_t;

/*  BLE UUID structure */
typedef struct
{
    ble_uuid_type_t     type;           /*!< UUID type */
    union
    {
        uint16_t        uuid_16;        /*!< UUID data in 16-bit type */
        uint32_t        uuid_32;        /*!< UUID data in 32-bit type */
        uint8_t         uuid_128[16];   /*!< UUID data in 128-bit type */
    } data;
} ble_uuid_t;

#define BLE_UUID_INIT_16(value) \
{                               \
    .type = (BLE_UUID_TYPE_16), \
    .data = {(value)},          \
}

/* BLE basic data structure */
typedef struct
{
  uint8_t     *p_data;  /*!< Pointer to the data buffer */
  uint16_t     len;     /*!< Length of the data buffer in bytes */
} ble_data_t;

#ifdef __cplusplus
}
#endif

#endif /* _BLE_TYPES_H_ */
