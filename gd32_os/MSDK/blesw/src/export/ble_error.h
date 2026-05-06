/*!
    \file    ble_error.h
    \brief   High layer error codes.

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

#ifndef BLE_ERROR_H_
#define BLE_ERROR_H_

/* Error code from HL TO HCI Range - from 0x90 to 0xD0 */
#define BLE_ERROR_HL_TO_HCI(err)     (((err) > 0x90) ? ((err) - 0x90) : (0))

/* List all BLE error codes */
typedef enum
{
    BLE_ERR_NO_ERROR                      = 0x00,   /*!< No error */

    /** ATT Specific Error **/
    BLE_ATT_ERR_INVALID_HANDLE            = 0x01,   /*!< Handle is invalid */
    BLE_ATT_ERR_READ_NOT_PERMITTED        = 0x02,   /*!< Read permission disabled */
    BLE_ATT_ERR_WRITE_NOT_PERMITTED       = 0x03,   /*!< Write permission disabled */
    BLE_ATT_ERR_INVALID_PDU               = 0x04,   /*!< Incorrect PDU */
    BLE_ATT_ERR_INSUFF_AUTHEN             = 0x05,   /*!< Authentication privilege not enough */
    BLE_ATT_ERR_REQUEST_NOT_SUPPORTED     = 0x06,   /*!< Request not supported or not understood */
    BLE_ATT_ERR_INVALID_OFFSET            = 0x07,   /*!< Incorrect offset value */
    BLE_ATT_ERR_INSUFF_AUTHOR             = 0x08,   /*!< Authorization privilege not enough */
    BLE_ATT_ERR_PREPARE_QUEUE_FULL        = 0x09,   /*!< Capacity queue for reliable write reached */
    BLE_ATT_ERR_ATTRIBUTE_NOT_FOUND       = 0x0A,   /*!< Attribute requested not existing */
    BLE_ATT_ERR_ATTRIBUTE_NOT_LONG        = 0x0B,   /*!< Attribute requested not long */
    BLE_ATT_ERR_INSUFF_ENC_KEY_SIZE       = 0x0C,   /*!< Encryption size not sufficient */
    BLE_ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN = 0x0D,   /*!< Invalid length of the attribute value */
    BLE_ATT_ERR_UNLIKELY_ERR              = 0x0E,   /*!< Operation not fit to condition */
    BLE_ATT_ERR_INSUFF_ENC                = 0x0F,   /*!< Attribute requires encryption before operation */
    BLE_ATT_ERR_UNSUPP_GRP_TYPE           = 0x10,   /*!< Attribute grouping not supported */
    BLE_ATT_ERR_INSUFF_RESOURCE           = 0x11,   /*!< Resources not sufficient to complete the request */
    BLE_ATT_ERR_DB_OUT_OF_SYNC            = 0x12,   /*!< The server requests the client to rediscover the database */
    BLE_ATT_ERR_VALUE_NOT_ALLOWED         = 0x13,   /*!< The attribute parameter value was not allowed */
    BLE_ATT_ERR_PRF_ALREADY_EXIST         = 0x14,   /*!< The profile has been registered */
    BLE_ATT_ERR_CANNOT_FOUND_IN_DB        = 0x15,   /*!< The profile can not be found in database */
    BLE_ATT_ERR_DISC_ALREADY_PERFORMED    = 0x16,   /*!< The discovery action has already been performed */
    BLE_ATT_ERR_APP_ERROR                 = 0x80,   /*!< Application error (also used in PRF Errors) */

    /* L2CAP Specific Error */
    BLE_L2CAP_ERR_CONNECTION_LOST     = 0x30,   /*!< Message cannot be sent because of connection lost */
    BLE_L2CAP_ERR_INVALID_MTU         = 0x31,   /*!< MTU size exceed or invalid MTU proposed */
    BLE_L2CAP_ERR_INVALID_MPS         = 0x32,   /*!< MPS size exceed or invalid MPS proposed */
    BLE_L2CAP_ERR_INVALID_CID         = 0x33,   /*!< Invalid Channel ID */
    BLE_L2CAP_ERR_INVALID_PDU         = 0x34,   /*!< Invalid PDU */
    BLE_L2CAP_ERR_UNACCEPTABLE_PARAM  = 0x35,   /*!< Connection refused - unacceptable parameters */
    BLE_L2CAP_ERR_INSUFF_AUTHEN       = 0x36,   /*!< Connection refused - insufficient authentication */
    BLE_L2CAP_ERR_INSUFF_AUTHOR       = 0x37,   /*!< Connection refused - insufficient authorization */
    BLE_L2CAP_ERR_INSUFF_ENC_KEY_SIZE = 0x38,   /*!< Connection refused - insufficient encryption key size */
    BLE_L2CAP_ERR_INSUFF_ENC          = 0x39,   /*!< Connection Refused - insufficient encryption */
    BLE_L2CAP_ERR_PSM_SPSM_NOT_SUPP   = 0x3A,   /*!< Connection refused - PSM/SPSM not supported */
    BLE_L2CAP_ERR_INSUFF_CREDIT       = 0x3B,   /*!< No more credit */
    BLE_L2CAP_ERR_NOT_UNDERSTOOD      = 0x3C,   /*!< Command not understood by peer device */
    BLE_L2CAP_ERR_CREDIT_ERROR        = 0x3D,   /*!< Credit error, invalid number of credit received */
    BLE_L2CAP_ERR_CID_ALREADY_ALLOC   = 0x3E,   /*!< Channel identifier already allocated */
    BLE_L2CAP_ERR_UNKNOWN_PDU         = 0x3F,   /*!< Unknown PDU */

    /* GAP Specific Error */
    BLE_GAP_ERR_INVALID_PARAM      = 0x40,      /*!< Invalid parameters set */
    BLE_GAP_ERR_PROTOCOL_PROBLEM   = 0x41,      /*!< Problem with protocol exchange, get unexpected response */
    BLE_GAP_ERR_NOT_SUPPORTED      = 0x42,      /*!< Request not supported by configuration */
    BLE_GAP_ERR_COMMAND_DISALLOWED = 0x43,      /*!< Request not allowed in current state */
    BLE_GAP_ERR_CANCELED           = 0x44,      /*!< Requested operation canceled */
    BLE_GAP_ERR_TIMEOUT            = 0x45,      /*!< Requested operation timeout */
    BLE_GAP_ERR_DISCONNECTED       = 0x46,      /*!< Link connection lost during operation */
    BLE_GAP_ERR_NOT_FOUND          = 0x47,      /*!< Search algorithm finished, but no result found */
    BLE_GAP_ERR_REJECTED           = 0x48,      /*!< Request rejected by peer device */
    BLE_GAP_ERR_PRIVACY_CFG_PB     = 0x49,      /*!< Problem with privacy configuration */
    BLE_GAP_ERR_ADV_DATA_INVALID   = 0x4A,      /*!< Duplicate or invalid advertising data */
    BLE_GAP_ERR_INSUFF_RESOURCES   = 0x4B,      /*!< Insufficient resources */
    BLE_GAP_ERR_UNEXPECTED         = 0x4C,      /*!< Unexpected Error */
    BLE_GAP_ERR_MISSING_CALLBACK   = 0x4D,      /*!< A required callback has not been configured */
    BLE_GAP_ERR_INVALID_BUFFER     = 0x4E,      /*!< Buffer cannot be used due to invalid header or tail length */
    BLE_GAP_ERR_BUSY               = 0x4F,      /*!< Request cannot be performed because an on-going procedure blocks it */
    BLE_GAP_ERR_ALREADY_REGISTERED = 0x5A,      /*!< Resource is already registered, cannot be registered twice */

    /* GATT Specific Error */
    BLE_GATT_ERR_INVALID_ATT_LEN            = 0x50,     /*!< Problem with ATTC protocol response */
    BLE_GATT_ERR_INVALID_TYPE_IN_SVC_SEARCH = 0x51,     /*!< Error in service search */
    BLE_GATT_ERR_WRITE                      = 0x52,     /*!< Invalid write data */
    BLE_GATT_ERR_SIGNED_WRITE               = 0x53,     /*!< Signed write error */
    BLE_GATT_ERR_ATTRIBUTE_CLIENT_MISSING   = 0x54,     /*!< No attribute client defined */
    BLE_GATT_ERR_ATTRIBUTE_SERVER_MISSING   = 0x55,     /*!< No attribute server defined */
    BLE_GATT_ERR_INVALID_PERM               = 0x56,     /*!< Permission set in service/attribute are invalid */
    BLE_GATT_ERR_ATT_BEARER_CLOSE           = 0x57,     /*!< Attribute bearer is closed */
    BLE_GATT_ERR_NO_MORE_BEARER             = 0x58,     /*!< No more Attribute bearer available */

    /* SMP Specific Error */
    /* SMP Protocol Errors detected on local device */
    BLE_SMP_ERR_LOC_PASSKEY_ENTRY_FAILED      = 0x61,   /*!< User input of passkey failed */
    BLE_SMP_ERR_LOC_OOB_NOT_AVAILABLE         = 0x62,   /*!< OOB Data is not available */
    BLE_SMP_ERR_LOC_AUTH_REQ                  = 0x63,   /*!< Authentication requirements cannot be met due to IO capabilities */
    BLE_SMP_ERR_LOC_CONF_VAL_FAILED           = 0x64,   /*!< The confirm value does not match the calculated confirm value */
    BLE_SMP_ERR_LOC_PAIRING_NOT_SUPP          = 0x65,   /*!< Pairing is not supported by the device */
    BLE_SMP_ERR_LOC_ENC_KEY_SIZE              = 0x66,   /*!< Encryption key size is insufficient for the security requirements */
    BLE_SMP_ERR_LOC_CMD_NOT_SUPPORTED         = 0x67,   /*!< SMP command received is not supported */
    BLE_SMP_ERR_LOC_UNSPECIFIED_REASON        = 0x68,   /*!< Pairing failed due to an unspecified reason */
    BLE_SMP_ERR_LOC_REPEATED_ATTEMPTS         = 0x69,   /*!< Pairing or Authentication procedure is disallowed because too little time has elapsed since last pairing request or security request */
    BLE_SMP_ERR_LOC_INVALID_PARAM             = 0x6A,   /*!< The command length is invalid or a parameter is outside of the specified range */
    BLE_SMP_ERR_LOC_DHKEY_CHECK_FAILED        = 0x6B,   /*!< DHKey Check value received doesn't match the one calculated by the local device */
    BLE_SMP_ERR_LOC_NUMERIC_COMPARISON_FAILED = 0x6C,   /*!< Confirm values in the numeric comparison protocol do not match */
    BLE_SMP_ERR_LOC_BREDR_PAIRING_IN_PROGRESS = 0x6D,   /*!< Pairing over LE failed due to a Pairing Request sent over BR/EDR transport in process */
    BLE_SMP_ERR_LOC_CROSS_TRANSPORT_KEY_GEN_NOT_ALLOWED = 0x6E, /*!< Link Key generated on BR/EDR cannot be used to derive and distribute keys for LE */
    /* SMP Protocol Errors detected by remote device */
    BLE_SMP_ERR_REM_PASSKEY_ENTRY_FAILED      = 0x71,   /*!< User input of passkey failed */
    BLE_SMP_ERR_REM_OOB_NOT_AVAILABLE         = 0x72,   /*!< OOB Data is not available */
    BLE_SMP_ERR_REM_AUTH_REQ                  = 0x73,   /*!< Authentication requirements cannot be met due to IO capabilities */
    BLE_SMP_ERR_REM_CONF_VAL_FAILED           = 0x74,   /*!< Confirm value does not match the calculated confirm value */
    BLE_SMP_ERR_REM_PAIRING_NOT_SUPP          = 0x75,   /*!< Pairing is not supported by the device */
    BLE_SMP_ERR_REM_ENC_KEY_SIZE              = 0x76,   /*!< Encryption key size is insufficient for the security requirements */
    BLE_SMP_ERR_REM_CMD_NOT_SUPPORTED         = 0x77,   /*!< SMP command received is not supported */
    BLE_SMP_ERR_REM_UNSPECIFIED_REASON        = 0x78,   /*!< Pairing failed due to an unspecified reason */
    BLE_SMP_ERR_REM_REPEATED_ATTEMPTS         = 0x79,   /*!< Pairing or Authentication procedure is disallowed because too little time has elapsed since last pairing request or security request */
    BLE_SMP_ERR_REM_INVALID_PARAM             = 0x7A,   /*!< The command length is invalid or a parameter is outside of the specified range */
    BLE_SMP_ERR_REM_DHKEY_CHECK_FAILED        = 0x7B,   /*!< DHKey Check value received doesn't match the one calculated by the local device */
    BLE_SMP_ERR_REM_NUMERIC_COMPARISON_FAILED = 0x7C,   /*!< Confirm values in the numeric comparison protocol do not match */
    BLE_SMP_ERR_REM_BREDR_PAIRING_IN_PROGRESS = 0x7D,   /*!< Pairing over LE failed due to a Pairing Request sent over BR/EDR transport in process */
    BLE_SMP_ERR_REM_CROSS_TRANSPORT_KEY_GEN_NOT_ALLOWED = 0x7E, /*!< Link Key generated on BR/EDR cannot be used to derive and distribute keys for LE */
    /* SMP Errors triggered by local device */
    BLE_SMP_ERR_ADDR_RESOLV_FAIL              = 0x20,   /*!< Provided resolvable address has not been resolved */
    BLE_SMP_ERR_SIGN_VERIF_FAIL               = 0x21,   /*!< Signature Verification Failed */
    BLE_SMP_ERR_ENC_KEY_MISSING               = 0x22,   /*!< Encryption failed because slave didn't find the LTK */
    BLE_SMP_ERR_ENC_NOT_SUPPORTED             = 0x23,   /*!< Encryption failed because slave doesn't support encryption feature */
    BLE_SMP_ERR_ENC_TIMEOUT                   = 0x24,   /*!< A timeout has occurred during the start encryption session */
    BLE_SMP_ERR_NOT_BONDED                    = 0x25,   /*!< Encryption cannot start because peer device not bonded */

    /* Profiles Specific Error */
    BLE_PRF_APP_ERROR                     = 0x80,   /*!< Application Error */
    BLE_PRF_ERR_INVALID_PARAM             = 0x81,   /*!< Invalid parameter in request */
    BLE_PRF_ERR_INEXISTENT_HDL            = 0x82,   /*!< Inexistent handle for sending a read/write characteristic request */
    BLE_PRF_ERR_STOP_DISC_CHAR_MISSING    = 0x83,   /*!< Discovery stopped due to missing attribute according to specification */
    BLE_PRF_ERR_MULTIPLE_SVC              = 0x84,   /*!< Too many service instances found */
    BLE_PRF_ERR_STOP_DISC_WRONG_CHAR_PROP = 0x85,   /*!< Discovery stopped due to found attribute with incorrect properties */
    BLE_PRF_ERR_MULTIPLE_CHAR             = 0x86,   /*!< Too many characteristic instances found */
    BLE_PRF_ERR_MISMATCH                  = 0x87,   /*!< Feature mismatch */
    BLE_PRF_ERR_REQ_DISALLOWED            = 0x89,   /*!< Request not allowed */
    BLE_PRF_ERR_NTF_DISABLED              = 0x8A,   /*!< Notification Not Enabled */
    BLE_PRF_ERR_IND_DISABLED              = 0x8B,   /*!< Indication Not Enabled */
    BLE_PRF_ERR_FEATURE_NOT_SUPPORTED     = 0x8C,   /*!< Feature not supported by profile */
    BLE_PRF_ERR_UNEXPECTED_LEN            = 0x8D,   /*!< Read value has an unexpected length */
    BLE_PRF_ERR_DISCONNECTED              = 0x8E,   /*!< Disconnection occurs */
    BLE_PRF_ERR_PROC_TIMEOUT              = 0x8F,   /*!< Procedure Timeout */
    BLE_PRF_ERR_WRITE_REQ_REJECTED        = 0xFC,   /*!< Requested write operation cannot be fulfilled for reasons other than permissions */
    BLE_PRF_CCCD_IMPR_CONFIGURED          = 0xFD,   /*!< Client characteristic configuration improperly configured */
    BLE_PRF_PROC_IN_PROGRESS              = 0xFE,   /*!< Procedure already in progress */
    BLE_PRF_OUT_OF_RANGE                  = 0xFF,   /*!< Out of Range */
    BLE_PRF_ERR_PRF_MGR                   = 0xF1,   /*!< Profile manager internal error */

    /* LL Error codes conveyed to upper layer */
    BLE_LL_ERR_UNKNOWN_HCI_COMMAND           = 0x91,    /*!< Unknown HCI Command */
    BLE_LL_ERR_UNKNOWN_CONNECTION_ID         = 0x92,    /*!< Unknown Connection Identifier */
    BLE_LL_ERR_HARDWARE_FAILURE              = 0x93,    /*!< Hardware Failure */
    BLE_LL_ERR_PAGE_TIMEOUT                  = 0x94,    /*!< BT Page Timeout */
    BLE_LL_ERR_AUTH_FAILURE                  = 0x95,    /*!< Authentication failure */
    BLE_LL_ERR_PIN_MISSING                   = 0x96,    /*!< Pin code missing */
    BLE_LL_ERR_MEMORY_CAPA_EXCEED            = 0x97,    /*!< Memory capacity exceed */
    BLE_LL_ERR_CON_TIMEOUT                   = 0x98,    /*!< Connection Timeout */
    BLE_LL_ERR_CON_LIMIT_EXCEED              = 0x99,    /*!< Connection limit Exceed */
    BLE_LL_ERR_SYNC_CON_LIMIT_DEV_EXCEED     = 0x9A,    /*!< Synchronous Connection limit exceed */
    BLE_LL_ERR_ACL_CON_EXISTS                = 0x9B,    /*!< ACL Connection exits */
    BLE_LL_ERR_COMMAND_DISALLOWED            = 0x9C,    /*!< Command Disallowed */
    BLE_LL_ERR_CONN_REJ_LIMITED_RESOURCES    = 0x9D,    /*!< Connection rejected due to limited resources */
    BLE_LL_ERR_CONN_REJ_SECURITY_REASONS     = 0x9E,    /*!< Connection rejected due to security reason */
    BLE_LL_ERR_CONN_REJ_UNACCEPTABLE_BDADDR  = 0x9F,    /*!< Connection rejected due to unacceptable BD Addr */
    BLE_LL_ERR_CONN_ACCEPT_TIMEOUT_EXCEED    = 0xA0,    /*!< Connection rejected due to Accept connection timeout */
    BLE_LL_ERR_UNSUPPORTED                   = 0xA1,    /*!< Not Supported */
    BLE_LL_ERR_INVALID_HCI_PARAM             = 0xA2,    /*!< Invalid parameters */
    BLE_LL_ERR_REMOTE_USER_TERM_CON          = 0xA3,    /*!< Remote user terminate connection */
    BLE_LL_ERR_REMOTE_DEV_TERM_LOW_RESOURCES = 0xA4,    /*!< Remote device terminate connection due to low resources */
    BLE_LL_ERR_REMOTE_DEV_POWER_OFF          = 0xA5,    /*!< Remote device terminate connection due to power off */
    BLE_LL_ERR_CON_TERM_BY_LOCAL_HOST        = 0xA6,    /*!< Connection terminated by local host */
    BLE_LL_ERR_REPEATED_ATTEMPTS             = 0xA7,    /*!< Repeated attempts */
    BLE_LL_ERR_PAIRING_NOT_ALLOWED           = 0xA8,    /*!< Pairing not Allowed */
    BLE_LL_ERR_UNKNOWN_LMP_PDU               = 0xA9,    /*!< Unknown PDU Error */
    BLE_LL_ERR_UNSUPPORTED_REMOTE_FEATURE    = 0xAA,    /*!< Unsupported remote feature */
    BLE_LL_ERR_SCO_OFFSET_REJECTED           = 0xAB,    /*!< SCO Offset rejected */
    BLE_LL_ERR_SCO_INTERVAL_REJECTED         = 0xAC,    /*!< SCO Interval Rejected */
    BLE_LL_ERR_SCO_AIR_MODE_REJECTED         = 0xAD,    /*!< SCO air mode Rejected */
    BLE_LL_ERR_INVALID_LMP_PARAM             = 0xAE,    /*!< Invalid LMP parameters */
    BLE_LL_ERR_UNSPECIFIED_ERROR             = 0xAF,    /*!< Unspecified error */
    BLE_LL_ERR_UNSUPPORTED_LMP_PARAM_VALUE   = 0xB0,    /*!< Unsupported LMP Parameter value */
    BLE_LL_ERR_ROLE_CHANGE_NOT_ALLOWED       = 0xB1,    /*!< Role Change Not allowed */
    BLE_LL_ERR_LMP_RSP_TIMEOUT               = 0xB2,    /*!< LMP Response timeout */
    BLE_LL_ERR_LMP_COLLISION                 = 0xB3,    /*!< LMP Collision */
    BLE_LL_ERR_LMP_PDU_NOT_ALLOWED           = 0xB4,    /*!< LMP Pdu not allowed */
    BLE_LL_ERR_ENC_MODE_NOT_ACCEPT           = 0xB5,    /*!< Encryption mode not accepted */
    BLE_LL_ERR_LINK_KEY_CANT_CHANGE          = 0xB6,    /*!< Link Key Cannot be changed */
    BLE_LL_ERR_QOS_NOT_SUPPORTED             = 0xB7,    /*!< Quality of Service not supported */
    BLE_LL_ERR_INSTANT_PASSED                = 0xB8,    /*!< Error, instant passed */
    BLE_LL_ERR_PAIRING_WITH_UNIT_KEY_NOT_SUP = 0xB9,    /*!< Pairing with unit key not supported */
    BLE_LL_ERR_DIFF_TRANSACTION_COLLISION    = 0xBA,    /*!< Transaction collision */
    BLE_LL_ERR_QOS_UNACCEPTABLE_PARAM        = 0xBC,    /*!< Unacceptable parameters */
    BLE_LL_ERR_QOS_REJECTED                  = 0xBD,    /*!< Quality of Service rejected */
    BLE_LL_ERR_CHANNEL_CLASS_NOT_SUP         = 0xBE,    /*!< Channel class not supported */
    BLE_LL_ERR_INSUFFICIENT_SECURITY         = 0xBF,    /*!< Insufficient security */
    BLE_LL_ERR_PARAM_OUT_OF_MAND_RANGE       = 0xC0,    /*!< Parameters out of mandatory range */
    BLE_LL_ERR_ROLE_SWITCH_PEND              = 0xC2,    /*!< Role switch pending */
    BLE_LL_ERR_RESERVED_SLOT_VIOLATION       = 0xC4,    /*!< Reserved slot violation */
    BLE_LL_ERR_ROLE_SWITCH_FAIL              = 0xC5,    /*!< Role Switch fail */
    BLE_LL_ERR_EIR_TOO_LARGE                 = 0xC6,    /*!< Error, EIR too large */
    BLE_LL_ERR_SP_NOT_SUPPORTED_HOST         = 0xC7,    /*!< Simple pairing not supported by host */
    BLE_LL_ERR_HOST_BUSY_PAIRING             = 0xC8,    /*!< Host pairing is busy */
    BLE_LL_ERR_CONTROLLER_BUSY               = 0xCA,    /*!< Controller is busy */
    BLE_LL_ERR_UNACCEPTABLE_CONN_PARAM       = 0xCB,    /*!< Unacceptable connection parameters */
    BLE_LL_ERR_DIRECT_ADV_TO                 = 0xCC,    /*!< Direct Advertising Timeout */
    BLE_LL_ERR_TERMINATED_MIC_FAILURE        = 0xCD,    /*!< Connection Terminated due to a MIC failure */
    BLE_LL_ERR_CONN_FAILED_TO_BE_EST         = 0xCE,    /*!< Connection failed to be established */
    BLE_LL_ERR_MAC_CONN_FAILED               = 0xCF,    /*!< MAC Connection Failed */
    BLE_LL_ERR_CCA_REJ_USE_CLOCK_DRAG        = 0xD0,    /*!< Coarse Clock Adjustment Rejected but Will Try to Adjust Using Clock Dragging */
    BLE_LL_ERR_TYPE0_SUBMAP_NOT_DEFINED      = 0xD1,    /*!< Type0 Submap Not Defined */
    BLE_LL_ERR_UNKNOWN_ADVERTISING_ID        = 0xD2,    /*!< Unknown Advertising Identifier */
    BLE_LL_ERR_LIMIT_REACHED                 = 0xD3,    /*!< Limit Reached */
    BLE_LL_ERR_OPERATION_CANCELED_BY_HOST    = 0xD4,    /*!< Operation Cancelled by Host */
    BLE_LL_ERR_PKT_TOO_LONG                  = 0xD5,    /*!< Packet Too Long */

    BLE_ERR_PROCESSING            = 0xE0,   /*!< Processing error */
    BLE_ERR_NO_MEM_AVAIL          = 0xE1,   /*!< sys_malloc not memory */
    BLE_ERR_NO_RESOURCES          = 0xE2,   /*!< Not resources */
    BLE_ERR_STORAGE_NOT_FOUND     = 0xE3,   /*!< BLE storage not found device */
    BLE_ERR_STORAGE_FLASH         = 0xE4,   /*!< BLE storage operation flash error */
    BLE_ERR_STORAGE_PARAM         = 0xE5,   /*!< BLE storage parameter error */
    BLE_ERR_NEED_BONDING          = 0xE6,   /*!< BLE storage need bonding erro */

} ble_err_t;

typedef ble_err_t ble_status_t;

#endif // BLE_ERROR_H_
