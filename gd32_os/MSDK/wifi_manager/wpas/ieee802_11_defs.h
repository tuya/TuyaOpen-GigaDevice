/*
 * IEEE 802.11 Frame type definitions
 * Copyright (c) 2002-2019, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2007-2008 Intel Corporation
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

/*!
    \file    ieee802_11_defs.h
    \brief   ieee802.11 definitions

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

#ifndef _IEEE802_11_DEFS_H_
#define _IEEE802_11_DEFS_H_

#if defined(__CC_ARM)
#define STRUCT_PACKED           __attribute__ ((packed))
#elif defined(__ICCARM__)
#define STRUCT_PACKED           __attribute__((packed, aligned(1)))
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#define STRUCT_PACKED           __attribute__((packed, aligned(1)))
#elif defined ( __GNUC__ )
#define STRUCT_PACKED           __attribute__ ((packed))
#else
#define STRUCT_PACKED
#endif

/* IEEE 802.11 defines */
#ifndef WIFI_SSID_MAX_LEN
#define WIFI_SSID_MAX_LEN       32
#endif
#ifndef WIFI_ALEN
#define WIFI_ALEN               6
#endif

#define WLAN_FC_PVER            0x0003
#define WLAN_FC_TODS            0x0100
#define WLAN_FC_FROMDS          0x0200
#define WLAN_FC_MOREFRAG        0x0400
#define WLAN_FC_RETRY           0x0800
#define WLAN_FC_PWRMGT          0x1000
#define WLAN_FC_MOREDATA        0x2000
#define WLAN_FC_ISWEP           0x4000
#define WLAN_FC_HTC             0x8000

#define WLAN_FC_GET_TYPE(fc)    (((fc) & 0x000c) >> 2)
#define WLAN_FC_GET_STYPE(fc)   (((fc) & 0x00f0) >> 4)

#define WLAN_INVALID_MGMT_SEQ   0xFFFF

#define WLAN_GET_SEQ_FRAG(seq)  ((seq) & (BIT(3) | BIT(2) | BIT(1) | BIT(0)))
#define WLAN_GET_SEQ_SEQ(seq) \
    (((seq) & (~(BIT(3) | BIT(2) | BIT(1) | BIT(0)))) >> 4)

#define WLAN_FC_TYPE_MGMT       0
#define WLAN_FC_TYPE_CTRL       1
#define WLAN_FC_TYPE_DATA       2

/* management */
#define WLAN_FC_STYPE_ASSOC_REQ             0
#define WLAN_FC_STYPE_ASSOC_RESP            1
#define WLAN_FC_STYPE_REASSOC_REQ           2
#define WLAN_FC_STYPE_REASSOC_RESP          3
#define WLAN_FC_STYPE_PROBE_REQ             4
#define WLAN_FC_STYPE_PROBE_RESP            5
#define WLAN_FC_STYPE_BEACON                8
#define WLAN_FC_STYPE_ATIM                  9
#define WLAN_FC_STYPE_DISASSOC              10
#define WLAN_FC_STYPE_AUTH                  11
#define WLAN_FC_STYPE_DEAUTH                12
#define WLAN_FC_STYPE_ACTION                13
#define WLAN_FC_STYPE_ACTION_NO_ACK         14

/* control */
#define WLAN_FC_STYPE_PSPOLL                10
#define WLAN_FC_STYPE_RTS                   11
#define WLAN_FC_STYPE_CTS                   12
#define WLAN_FC_STYPE_ACK                   13
#define WLAN_FC_STYPE_CFEND                 14
#define WLAN_FC_STYPE_CFENDACK              15

/* data */
#define WLAN_FC_STYPE_DATA                  0
#define WLAN_FC_STYPE_DATA_CFACK            1
#define WLAN_FC_STYPE_DATA_CFPOLL           2
#define WLAN_FC_STYPE_DATA_CFACKPOLL        3
#define WLAN_FC_STYPE_NULLFUNC              4
#define WLAN_FC_STYPE_CFACK                 5
#define WLAN_FC_STYPE_CFPOLL                6
#define WLAN_FC_STYPE_CFACKPOLL             7
#define WLAN_FC_STYPE_QOS_DATA              8
#define WLAN_FC_STYPE_QOS_DATA_CFACK        9
#define WLAN_FC_STYPE_QOS_DATA_CFPOLL       10
#define WLAN_FC_STYPE_QOS_DATA_CFACKPOLL    11
#define WLAN_FC_STYPE_QOS_NULL              12
#define WLAN_FC_STYPE_QOS_CFPOLL            14
#define WLAN_FC_STYPE_QOS_CFACKPOLL         15

/* Authentication algorithms */
#define WLAN_AUTH_OPEN                      0
#define WLAN_AUTH_SHARED_KEY                1
#define WLAN_AUTH_FT                        2
#define WLAN_AUTH_SAE                       3
#define WLAN_AUTH_FILS_SK                   4
#define WLAN_AUTH_FILS_SK_PFS               5
#define WLAN_AUTH_FILS_PK                   6
#define WLAN_AUTH_PASN                      7
#define WLAN_AUTH_LEAP                      128

#define WLAN_AUTH_CHALLENGE_LEN             128

#define WLAN_CAPABILITY_ESS                 BIT(0)
#define WLAN_CAPABILITY_IBSS                BIT(1)
#define WLAN_CAPABILITY_CF_POLLABLE         BIT(2)
#define WLAN_CAPABILITY_CF_POLL_REQUEST     BIT(3)
#define WLAN_CAPABILITY_PRIVACY             BIT(4)
#define WLAN_CAPABILITY_SHORT_PREAMBLE      BIT(5)
#define WLAN_CAPABILITY_PBCC                BIT(6)
#define WLAN_CAPABILITY_CHANNEL_AGILITY     BIT(7)
#define WLAN_CAPABILITY_SPECTRUM_MGMT       BIT(8)
#define WLAN_CAPABILITY_QOS                 BIT(9)
#define WLAN_CAPABILITY_SHORT_SLOT_TIME     BIT(10)
#define WLAN_CAPABILITY_APSD                BIT(11)
#define WLAN_CAPABILITY_RADIO_MEASUREMENT   BIT(12)
#define WLAN_CAPABILITY_DSSS_OFDM           BIT(13)
#define WLAN_CAPABILITY_DELAYED_BLOCK_ACK   BIT(14)
#define WLAN_CAPABILITY_IMM_BLOCK_ACK       BIT(15)

/* Status codes (IEEE Std 802.11-2016, 9.4.1.9, Table 9-46) */
#define WLAN_STATUS_SUCCESS                                 0
#define WLAN_STATUS_UNSPECIFIED_FAILURE                     1
#define WLAN_STATUS_TDLS_WAKEUP_ALTERNATE                   2
#define WLAN_STATUS_TDLS_WAKEUP_REJECT                      3
#define WLAN_STATUS_SECURITY_DISABLED                       5
#define WLAN_STATUS_UNACCEPTABLE_LIFETIME                   6
#define WLAN_STATUS_NOT_IN_SAME_BSS                         7
#define WLAN_STATUS_CAPS_UNSUPPORTED                        10
#define WLAN_STATUS_REASSOC_NO_ASSOC                        11
#define WLAN_STATUS_ASSOC_DENIED_UNSPEC                     12
#define WLAN_STATUS_NOT_SUPPORTED_AUTH_ALG                  13
#define WLAN_STATUS_UNKNOWN_AUTH_TRANSACTION                14
#define WLAN_STATUS_CHALLENGE_FAIL                          15
#define WLAN_STATUS_AUTH_TIMEOUT                            16
#define WLAN_STATUS_AP_UNABLE_TO_HANDLE_NEW_STA             17
#define WLAN_STATUS_ASSOC_DENIED_RATES                      18
#define WLAN_STATUS_ASSOC_DENIED_NOSHORT                    19
#define WLAN_STATUS_SPEC_MGMT_REQUIRED                      22
#define WLAN_STATUS_PWR_CAPABILITY_NOT_VALID                23
#define WLAN_STATUS_SUPPORTED_CHANNEL_NOT_VALID             24
#define WLAN_STATUS_ASSOC_DENIED_NO_SHORT_SLOT_TIME         25
#define WLAN_STATUS_ASSOC_DENIED_NO_HT                      27
#define WLAN_STATUS_R0KH_UNREACHABLE                        28
#define WLAN_STATUS_ASSOC_DENIED_NO_PCO                     29
#define WLAN_STATUS_ASSOC_REJECTED_TEMPORARILY              30
#define WLAN_STATUS_ROBUST_MGMT_FRAME_POLICY_VIOLATION      31
#define WLAN_STATUS_UNSPECIFIED_QOS_FAILURE                 32
#define WLAN_STATUS_DENIED_INSUFFICIENT_BANDWIDTH           33
#define WLAN_STATUS_DENIED_POOR_CHANNEL_CONDITIONS          34
#define WLAN_STATUS_DENIED_QOS_NOT_SUPPORTED                35
#define WLAN_STATUS_REQUEST_DECLINED                        37
#define WLAN_STATUS_INVALID_PARAMETERS                      38
#define WLAN_STATUS_REJECTED_WITH_SUGGESTED_CHANGES         39
#define WLAN_STATUS_INVALID_IE                              40
#define WLAN_STATUS_GROUP_CIPHER_NOT_VALID                  41
#define WLAN_STATUS_PAIRWISE_CIPHER_NOT_VALID               42
#define WLAN_STATUS_AKMP_NOT_VALID                          43
#define WLAN_STATUS_UNSUPPORTED_RSN_IE_VERSION              44
#define WLAN_STATUS_INVALID_RSN_IE_CAPAB                    45
#define WLAN_STATUS_CIPHER_REJECTED_PER_POLICY              46
#define WLAN_STATUS_TS_NOT_CREATED                          47
#define WLAN_STATUS_DIRECT_LINK_NOT_ALLOWED                 48
#define WLAN_STATUS_DEST_STA_NOT_PRESENT                    49
#define WLAN_STATUS_DEST_STA_NOT_QOS_STA                    50
#define WLAN_STATUS_ASSOC_DENIED_LISTEN_INT_TOO_LARGE       51
#define WLAN_STATUS_INVALID_FT_ACTION_FRAME_COUNT           52
#define WLAN_STATUS_INVALID_PMKID                           53
#define WLAN_STATUS_INVALID_MDIE                            54
#define WLAN_STATUS_INVALID_FTIE                            55
#define WLAN_STATUS_REQUESTED_TCLAS_NOT_SUPPORTED           56
#define WLAN_STATUS_INSUFFICIENT_TCLAS_PROCESSING_RESOURCES 57
#define WLAN_STATUS_TRY_ANOTHER_BSS                         58
#define WLAN_STATUS_GAS_ADV_PROTO_NOT_SUPPORTED             59
#define WLAN_STATUS_NO_OUTSTANDING_GAS_REQ                  60
#define WLAN_STATUS_GAS_RESP_NOT_RECEIVED                   61
#define WLAN_STATUS_STA_TIMED_OUT_WAITING_FOR_GAS_RESP      62
#define WLAN_STATUS_GAS_RESP_LARGER_THAN_LIMIT              63
#define WLAN_STATUS_REQ_REFUSED_HOME                        64
#define WLAN_STATUS_ADV_SRV_UNREACHABLE                     65
#define WLAN_STATUS_REQ_REFUSED_SSPN                        67
#define WLAN_STATUS_REQ_REFUSED_UNAUTH_ACCESS               68
#define WLAN_STATUS_INVALID_RSNIE                           72
#define WLAN_STATUS_U_APSD_COEX_NOT_SUPPORTED               73
#define WLAN_STATUS_U_APSD_COEX_MODE_NOT_SUPPORTED          74
#define WLAN_STATUS_BAD_INTERVAL_WITH_U_APSD_COEX           75
#define WLAN_STATUS_ANTI_CLOGGING_TOKEN_REQ                 76
#define WLAN_STATUS_FINITE_CYCLIC_GROUP_NOT_SUPPORTED       77
#define WLAN_STATUS_CANNOT_FIND_ALT_TBTT                    78
#define WLAN_STATUS_TRANSMISSION_FAILURE                    79
#define WLAN_STATUS_REQ_TCLAS_NOT_SUPPORTED                 80
#define WLAN_STATUS_TCLAS_RESOURCES_EXCHAUSTED              81
#define WLAN_STATUS_REJECTED_WITH_SUGGESTED_BSS_TRANSITION  82
#define WLAN_STATUS_REJECT_WITH_SCHEDULE                    83
#define WLAN_STATUS_REJECT_NO_WAKEUP_SPECIFIED              84
#define WLAN_STATUS_SUCCESS_POWER_SAVE_MODE                 85
#define WLAN_STATUS_PENDING_ADMITTING_FST_SESSION           86
#define WLAN_STATUS_PERFORMING_FST_NOW                      87
#define WLAN_STATUS_PENDING_GAP_IN_BA_WINDOW                88
#define WLAN_STATUS_REJECT_U_PID_SETTING                    89
#define WLAN_STATUS_REFUSED_EXTERNAL_REASON                 92
#define WLAN_STATUS_REFUSED_AP_OUT_OF_MEMORY                93
#define WLAN_STATUS_REJECTED_EMERGENCY_SERVICE_NOT_SUPPORTED 94
#define WLAN_STATUS_QUERY_RESP_OUTSTANDING                  95
#define WLAN_STATUS_REJECT_DSE_BAND                         96
#define WLAN_STATUS_TCLAS_PROCESSING_TERMINATED             97
#define WLAN_STATUS_TS_SCHEDULE_CONFLICT                    98
#define WLAN_STATUS_DENIED_WITH_SUGGESTED_BAND_AND_CHANNEL  99
#define WLAN_STATUS_MCCAOP_RESERVATION_CONFLICT             100
#define WLAN_STATUS_MAF_LIMIT_EXCEEDED                      101
#define WLAN_STATUS_MCCA_TRACK_LIMIT_EXCEEDED               102
#define WLAN_STATUS_DENIED_DUE_TO_SPECTRUM_MANAGEMENT       103
#define WLAN_STATUS_ASSOC_DENIED_NO_VHT                     104
#define WLAN_STATUS_ENABLEMENT_DENIED                       105
#define WLAN_STATUS_RESTRICTION_FROM_AUTHORIZED_GDB         106
#define WLAN_STATUS_AUTHORIZATION_DEENABLED                 107
#define WLAN_STATUS_FILS_AUTHENTICATION_FAILURE             112
#define WLAN_STATUS_UNKNOWN_AUTHENTICATION_SERVER           113
#define WLAN_STATUS_UNKNOWN_PASSWORD_IDENTIFIER             123
#define WLAN_STATUS_DENIED_HE_NOT_SUPPORTED                 124
#define WLAN_STATUS_SAE_HASH_TO_ELEMENT                     126
#define WLAN_STATUS_SAE_PK                                  127

/* Reason codes (IEEE Std 802.11-2016, 9.4.1.7, Table 9-45) */
#define WLAN_REASON_UNSPECIFIED                         1
#define WLAN_REASON_PREV_AUTH_NOT_VALID                 2
#define WLAN_REASON_DEAUTH_LEAVING                      3
#define WLAN_REASON_DISASSOC_DUE_TO_INACTIVITY          4
#define WLAN_REASON_DISASSOC_AP_BUSY                    5
#define WLAN_REASON_CLASS2_FRAME_FROM_NONAUTH_STA       6
#define WLAN_REASON_CLASS3_FRAME_FROM_NONASSOC_STA      7
#define WLAN_REASON_DISASSOC_STA_HAS_LEFT               8
#define WLAN_REASON_STA_REQ_ASSOC_WITHOUT_AUTH          9
#define WLAN_REASON_PWR_CAPABILITY_NOT_VALID            10
#define WLAN_REASON_SUPPORTED_CHANNEL_NOT_VALID         11
#define WLAN_REASON_BSS_TRANSITION_DISASSOC             12
#define WLAN_REASON_INVALID_IE                          13
#define WLAN_REASON_MICHAEL_MIC_FAILURE                 14
#define WLAN_REASON_4WAY_HANDSHAKE_TIMEOUT              15
#define WLAN_REASON_GROUP_KEY_UPDATE_TIMEOUT            16
#define WLAN_REASON_IE_IN_4WAY_DIFFERS                  17
#define WLAN_REASON_GROUP_CIPHER_NOT_VALID              18
#define WLAN_REASON_PAIRWISE_CIPHER_NOT_VALID           19
#define WLAN_REASON_AKMP_NOT_VALID                      20
#define WLAN_REASON_UNSUPPORTED_RSN_IE_VERSION          21
#define WLAN_REASON_INVALID_RSN_IE_CAPAB                22
#define WLAN_REASON_IEEE_802_1X_AUTH_FAILED             23
#define WLAN_REASON_CIPHER_SUITE_REJECTED               24
#define WLAN_REASON_TDLS_TEARDOWN_UNREACHABLE           25
#define WLAN_REASON_TDLS_TEARDOWN_UNSPECIFIED           26
#define WLAN_REASON_SSP_REQUESTED_DISASSOC              27
#define WLAN_REASON_NO_SSP_ROAMING_AGREEMENT            28
#define WLAN_REASON_BAD_CIPHER_OR_AKM                   29
#define WLAN_REASON_NOT_AUTHORIZED_THIS_LOCATION        30
#define WLAN_REASON_SERVICE_CHANGE_PRECLUDES_TS         31
#define WLAN_REASON_UNSPECIFIED_QOS_REASON              32
#define WLAN_REASON_NOT_ENOUGH_BANDWIDTH                33
#define WLAN_REASON_DISASSOC_LOW_ACK                    34
#define WLAN_REASON_EXCEEDED_TXOP                       35
#define WLAN_REASON_STA_LEAVING                         36
#define WLAN_REASON_END_TS_BA_DLS                       37
#define WLAN_REASON_UNKNOWN_TS_BA                       38
#define WLAN_REASON_TIMEOUT                             39
// reason 40-44 is reserved
#define WLAN_REASON_GD_MESH_SMART_AP_RESET              40 // GD modified
#define WLAN_REASON_PEERKEY_MISMATCH                    45
#define WLAN_REASON_AUTHORIZED_ACCESS_LIMIT_REACHED     46
#define WLAN_REASON_EXTERNAL_SERVICE_REQUIREMENTS       47
#define WLAN_REASON_INVALID_FT_ACTION_FRAME_COUNT       48
#define WLAN_REASON_INVALID_PMKID                       49
#define WLAN_REASON_INVALID_MDE                         50
#define WLAN_REASON_INVALID_FTE                         51
#define WLAN_REASON_MESH_PEERING_CANCELLED              52
#define WLAN_REASON_MESH_MAX_PEERS                      53
#define WLAN_REASON_MESH_CONFIG_POLICY_VIOLATION        54
#define WLAN_REASON_MESH_CLOSE_RCVD                     55
#define WLAN_REASON_MESH_MAX_RETRIES                    56
#define WLAN_REASON_MESH_CONFIRM_TIMEOUT                57
#define WLAN_REASON_MESH_INVALID_GTK                    58
#define WLAN_REASON_MESH_INCONSISTENT_PARAMS            59
#define WLAN_REASON_MESH_INVALID_SECURITY_CAP           60
#define WLAN_REASON_MESH_PATH_ERROR_NO_PROXY_INFO       61
#define WLAN_REASON_MESH_PATH_ERROR_NO_FORWARDING_INFO  62
#define WLAN_REASON_MESH_PATH_ERROR_DEST_UNREACHABLE    63
#define WLAN_REASON_MAC_ADDRESS_ALREADY_EXISTS_IN_MBSS  64
#define WLAN_REASON_MESH_CHANNEL_SWITCH_REGULATORY_REQ  65
#define WLAN_REASON_MESH_CHANNEL_SWITCH_UNSPECIFIED     66

/* Information Element IDs (IEEE Std 802.11-2016, 9.4.2.1, Table 9-77) */
#define WLAN_EID_SSID                                   0
#define WLAN_EID_SUPP_RATES                             1
#define WLAN_EID_DS_PARAMS                              3
#define WLAN_EID_CF_PARAMS                              4
#define WLAN_EID_TIM                                    5
#define WLAN_EID_IBSS_PARAMS                            6
#define WLAN_EID_COUNTRY                                7
#define WLAN_EID_REQUEST                                10
#define WLAN_EID_BSS_LOAD                               11
#define WLAN_EID_EDCA_PARAM_SET                         12
#define WLAN_EID_TSPEC                                  13
#define WLAN_EID_TCLAS                                  14
#define WLAN_EID_SCHEDULE                               15
#define WLAN_EID_CHALLENGE                              16
#define WLAN_EID_PWR_CONSTRAINT                         32
#define WLAN_EID_PWR_CAPABILITY                         33
#define WLAN_EID_TPC_REQUEST                            34
#define WLAN_EID_TPC_REPORT                             35
#define WLAN_EID_SUPPORTED_CHANNELS                     36
#define WLAN_EID_CHANNEL_SWITCH                         37
#define WLAN_EID_MEASURE_REQUEST                        38
#define WLAN_EID_MEASURE_REPORT                         39
#define WLAN_EID_QUIET                                  40
#define WLAN_EID_IBSS_DFS                               41
#define WLAN_EID_ERP_INFO                               42
#define WLAN_EID_TS_DELAY                               43
#define WLAN_EID_TCLAS_PROCESSING                       44
#define WLAN_EID_HT_CAP                                 45
#define WLAN_EID_QOS                                    46
#define WLAN_EID_RSN                                    48
#define WLAN_EID_EXT_SUPP_RATES                         50
#define WLAN_EID_AP_CHANNEL_REPORT                      51
#define WLAN_EID_NEIGHBOR_REPORT                        52
#define WLAN_EID_RCPI                                   53
#define WLAN_EID_MOBILITY_DOMAIN                        54
#define WLAN_EID_FAST_BSS_TRANSITION                    55
#define WLAN_EID_TIMEOUT_INTERVAL                       56
#define WLAN_EID_RIC_DATA                               57
#define WLAN_EID_DSE_REGISTERED_LOCATION                58
#define WLAN_EID_SUPPORTED_OPERATING_CLASSES            59
#define WLAN_EID_EXT_CHANSWITCH_ANN                     60
#define WLAN_EID_HT_OPERATION                           61
#define WLAN_EID_SECONDARY_CHANNEL_OFFSET               62
#define WLAN_EID_BSS_AVERAGE_ACCESS_DELAY               63
#define WLAN_EID_ANTENNA                                64
#define WLAN_EID_RSNI                                   65
#define WLAN_EID_MEASUREMENT_PILOT_TRANSMISSION         66
#define WLAN_EID_BSS_AVAILABLE_ADM_CAPA                 67
#define WLAN_EID_BSS_AC_ACCESS_DELAY                    68 /* note: also used by WAPI */
#define WLAN_EID_TIME_ADVERTISEMENT                     69
#define WLAN_EID_RRM_ENABLED_CAPABILITIES               70
#define WLAN_EID_MULTIPLE_BSSID                         71
#define WLAN_EID_20_40_BSS_COEXISTENCE                  72
#define WLAN_EID_20_40_BSS_INTOLERANT                   73
#define WLAN_EID_OVERLAPPING_BSS_SCAN_PARAMS            74
#define WLAN_EID_RIC_DESCRIPTOR                         75
#define WLAN_EID_MMIE                                   76
#define WLAN_EID_EVENT_REQUEST                          78
#define WLAN_EID_EVENT_REPORT                           79
#define WLAN_EID_DIAGNOSTIC_REQUEST                     80
#define WLAN_EID_DIAGNOSTIC_REPORT                      81
#define WLAN_EID_LOCATION_PARAMETERS                    82
#define WLAN_EID_NONTRANSMITTED_BSSID_CAPA              83
#define WLAN_EID_SSID_LIST                              84
#define WLAN_EID_MULTIPLE_BSSID_INDEX                   85
#define WLAN_EID_FMS_DESCRIPTOR                         86
#define WLAN_EID_FMS_REQUEST                            87
#define WLAN_EID_FMS_RESPONSE                           88
#define WLAN_EID_QOS_TRAFFIC_CAPABILITY                 89
#define WLAN_EID_BSS_MAX_IDLE_PERIOD                    90
#define WLAN_EID_TFS_REQ                                91
#define WLAN_EID_TFS_RESP                               92
#define WLAN_EID_WNMSLEEP                               93
#define WLAN_EID_TIM_BROADCAST_REQUEST                  94
#define WLAN_EID_TIM_BROADCAST_RESPONSE                 95
#define WLAN_EID_COLLOCATED_INTERFERENCE_REPORT         96
#define WLAN_EID_CHANNEL_USAGE                          97
#define WLAN_EID_TIME_ZONE                              98
#define WLAN_EID_DMS_REQUEST                            99
#define WLAN_EID_DMS_RESPONSE                           100
#define WLAN_EID_LINK_ID                                101
#define WLAN_EID_WAKEUP_SCHEDULE                        102
#define WLAN_EID_CHANNEL_SWITCH_TIMING                  104
#define WLAN_EID_PTI_CONTROL                            105
#define WLAN_EID_TPU_BUFFER_STATUS                      106
#define WLAN_EID_INTERWORKING                           107
#define WLAN_EID_ADV_PROTO                              108
#define WLAN_EID_EXPEDITED_BANDWIDTH_REQ                109
#define WLAN_EID_QOS_MAP_SET                            110
#define WLAN_EID_ROAMING_CONSORTIUM                     111
#define WLAN_EID_EMERGENCY_ALERT_ID                     112
#define WLAN_EID_MESH_CONFIG                            113
#define WLAN_EID_MESH_ID                                114
#define WLAN_EID_MESH_LINK_METRIC_REPORT                115
#define WLAN_EID_CONGESTION_NOTIFICATION                116
#define WLAN_EID_PEER_MGMT                              117
#define WLAN_EID_MESH_CHANNEL_SWITCH_PARAMETERS         118
#define WLAN_EID_MESH_AWAKE_WINDOW                      119
#define WLAN_EID_BEACON_TIMING                          120
#define WLAN_EID_MCCAOP_SETUP_REQUEST                   121
#define WLAN_EID_MCCAOP_SETUP_REPLY                     122
#define WLAN_EID_MCCAOP_ADVERTISEMENT                   123
#define WLAN_EID_MCCAOP_TEARDOWN                        124
#define WLAN_EID_GANN                                   125
#define WLAN_EID_RANN                                   126
#define WLAN_EID_EXT_CAPAB                              127
#define WLAN_EID_PREQ                                   130
#define WLAN_EID_PREP                                   131
#define WLAN_EID_PERR                                   132
#define WLAN_EID_PXU                                    137
#define WLAN_EID_PXUC                                   138
#define WLAN_EID_AMPE                                   139
#define WLAN_EID_MIC                                    140
#define WLAN_EID_DESTINATION_URI                        141
#define WLAN_EID_U_APSD_COEX                            142
#define WLAN_EID_DMG_WAKEUP_SCHEDULE                    143
#define WLAN_EID_EXTENDED_SCHEDULE                      144
#define WLAN_EID_STA_AVAILABILITY                       145
#define WLAN_EID_DMG_TSPEC                              146
#define WLAN_EID_NEXT_DMG_ATI                           147
#define WLAN_EID_DMG_CAPABILITIES                       148
#define WLAN_EID_DMG_OPERATION                          151
#define WLAN_EID_DMG_BSS_PARAMETER_CHANGE               152
#define WLAN_EID_DMG_BEAM_REFINEMENT                    153
#define WLAN_EID_CHANNEL_MEASUREMENT_FEEDBACK           154
#define WLAN_EID_CCKM                                   156
#define WLAN_EID_AWAKE_WINDOW                           157
#define WLAN_EID_MULTI_BAND                             158
#define WLAN_EID_ADDBA_EXTENSION                        159
#define WLAN_EID_NEXTPCP_LIST                           160
#define WLAN_EID_PCP_HANDOVER                           161
#define WLAN_EID_DMG_LINK_MARGIN                        162
#define WLAN_EID_SWITCHING_STREAM                       163
#define WLAN_EID_SESSION_TRANSITION                     164
#define WLAN_EID_DYNAMIC_TONE_PAIRING_REPORT            165
#define WLAN_EID_CLUSTER_REPORT                         166
#define WLAN_EID_REPLAY_CAPABILITIES                    167
#define WLAN_EID_RELAY_TRANSFER_PARAM_SET               168
#define WLAN_EID_BEAMLINK_MAINTENANCE                   169
#define WLAN_EID_MULTIPLE_MAC_SUBLAYERS                 170
#define WLAN_EID_U_PID                                  171
#define WLAN_EID_DMG_LINK_ADAPTATION_ACK                172
#define WLAN_EID_MCCAOP_ADVERTISEMENT_OVERVIEW          174
#define WLAN_EID_QUIET_PERIOD_REQUEST                   175
#define WLAN_EID_QUIET_PERIOD_RESPONSE                  177
#define WLAN_EID_QMF_POLICY                             181
#define WLAN_EID_ECAPC_POLICY                           182
#define WLAN_EID_CLUSTER_TIME_OFFSET                    183
#define WLAN_EID_INTRA_ACCESS_CATEGORY_PRIORITY         184
#define WLAN_EID_SCS_DESCRIPTOR                         185
#define WLAN_EID_QLOAD_REPORT                           186
#define WLAN_EID_HCCA_TXOP_UPDATE_COUNT                 187
#define WLAN_EID_HIGHER_LAYER_STREAM_ID                 188
#define WLAN_EID_GCR_GROUP_ADDRESS                      189
#define WLAN_EID_ANTENNA_SECTOR_ID_PATTERN              190
#define WLAN_EID_VHT_CAP                                191
#define WLAN_EID_VHT_OPERATION                          192
#define WLAN_EID_VHT_EXTENDED_BSS_LOAD                  193
#define WLAN_EID_VHT_WIDE_BW_CHSWITCH                   194
#define WLAN_EID_TRANSMIT_POWER_ENVELOPE                195
#define WLAN_EID_VHT_CHANNEL_SWITCH_WRAPPER             196
#define WLAN_EID_VHT_AID                                197
#define WLAN_EID_VHT_QUIET_CHANNEL                      198
#define WLAN_EID_VHT_OPERATING_MODE_NOTIFICATION        199
#define WLAN_EID_UPSIM                                  200
#define WLAN_EID_REDUCED_NEIGHBOR_REPORT                201
#define WLAN_EID_TVHT_OPERATION                         202
#define WLAN_EID_DEVICE_LOCATION                        204
#define WLAN_EID_WHITE_SPACE_MAP                        205
#define WLAN_EID_FTM_PARAMETERS                         206
#define WLAN_EID_S1G_BCN_COMPAT                         213
#define WLAN_EID_TWT                                    216
#define WLAN_EID_S1G_CAPABILITIES                       217
#define WLAN_EID_VENDOR_SPECIFIC                        221
#define WLAN_EID_S1G_OPERATION                          232
#define WLAN_EID_CAG_NUMBER                             237
#define WLAN_EID_AP_CSN                                 239
#define WLAN_EID_FILS_INDICATION                        240
#define WLAN_EID_DILS                                   241
#define WLAN_EID_FRAGMENT                               242
#define WLAN_EID_RSNX                                   244
#define WLAN_EID_EXTENSION                              255

/* Element ID Extension (EID 255) values */
#define WLAN_EID_EXT_ASSOC_DELAY_INFO                   1
#define WLAN_EID_EXT_FILS_REQ_PARAMS                    2
#define WLAN_EID_EXT_FILS_KEY_CONFIRM                   3
#define WLAN_EID_EXT_FILS_SESSION                       4
#define WLAN_EID_EXT_FILS_HLP_CONTAINER                 5
#define WLAN_EID_EXT_FILS_IP_ADDR_ASSIGN                6
#define WLAN_EID_EXT_KEY_DELIVERY                       7
#define WLAN_EID_EXT_WRAPPED_DATA                       8
#define WLAN_EID_EXT_FTM_SYNC_INFO                      9
#define WLAN_EID_EXT_EXTENDED_REQUEST                   10
#define WLAN_EID_EXT_ESTIMATED_SERVICE_PARAMS           11
#define WLAN_EID_EXT_FILS_PUBLIC_KEY                    12
#define WLAN_EID_EXT_FILS_NONCE                         13
#define WLAN_EID_EXT_FUTURE_CHANNEL_GUIDANCE            14
#define WLAN_EID_EXT_OWE_DH_PARAM                       32
#define WLAN_EID_EXT_PASSWORD_IDENTIFIER                33
#define WLAN_EID_EXT_HE_CAPABILITIES                    35
#define WLAN_EID_EXT_HE_OPERATION                       36
#define WLAN_EID_EXT_HE_MU_EDCA_PARAMS                  38
#define WLAN_EID_EXT_SPATIAL_REUSE                      39
#define WLAN_EID_EXT_OCV_OCI                            54
#define WLAN_EID_EXT_SHORT_SSID_LIST                    58
#define WLAN_EID_EXT_HE_6GHZ_BAND_CAP                   59
#define WLAN_EID_EXT_EDMG_CAPABILITIES                  61
#define WLAN_EID_EXT_EDMG_OPERATION                     62
#define WLAN_EID_EXT_MSCS_DESCRIPTOR                    88
#define WLAN_EID_EXT_TCLAS_MASK                         89
#define WLAN_EID_EXT_REJECTED_GROUPS                    92
#define WLAN_EID_EXT_ANTI_CLOGGING_TOKEN                93
#define WLAN_EID_EXT_PASN_PARAMS                        100

/* Extended Capabilities field */
#define WLAN_EXT_CAPAB_20_40_COEX                       0
#define WLAN_EXT_CAPAB_GLK                              1
#define WLAN_EXT_CAPAB_EXT_CHAN_SWITCH                  2
#define WLAN_EXT_CAPAB_GLK_GCR                          3
#define WLAN_EXT_CAPAB_PSMP                             4
/* 5 - Reserved */
#define WLAN_EXT_CAPAB_S_PSMP                           6
#define WLAN_EXT_CAPAB_EVENT                            7
#define WLAN_EXT_CAPAB_DIAGNOSTICS                      8
#define WLAN_EXT_CAPAB_MULTICAST_DIAGNOSTICS            9
#define WLAN_EXT_CAPAB_LOCATION_TRACKING                10
#define WLAN_EXT_CAPAB_FMS                              11
#define WLAN_EXT_CAPAB_PROXY_ARP                        12
#define WLAN_EXT_CAPAB_COLL_INTERF_REP                  13
#define WLAN_EXT_CAPAB_CIVIC_LOCATION                   14
#define WLAN_EXT_CAPAB_GEOSPATIAL_LOCATION              15
#define WLAN_EXT_CAPAB_TFS                              16
#define WLAN_EXT_CAPAB_WNM_SLEEP_MODE                   17
#define WLAN_EXT_CAPAB_TIM_BROADCAST                    18
#define WLAN_EXT_CAPAB_BSS_TRANSITION                   19
#define WLAN_EXT_CAPAB_QOS_TRAFFIC                      20
#define WLAN_EXT_CAPAB_AC_STA_COUNT                     21
#define WLAN_EXT_CAPAB_MULTIPLE_BSSID                   22
#define WLAN_EXT_CAPAB_TIMING_MEASUREMENT               23
#define WLAN_EXT_CAPAB_CHANNEL_USAGE                    24
#define WLAN_EXT_CAPAB_SSID_LIST                        25
#define WLAN_EXT_CAPAB_DMS                              26
#define WLAN_EXT_CAPAB_UTF_TSF_OFFSET                   27
#define WLAN_EXT_CAPAB_TPU_BUFFER_STA                   28
#define WLAN_EXT_CAPAB_TDLS_PEER_PSM                    29
#define WLAN_EXT_CAPAB_TDLS_CHANNEL_SWITCH              30
#define WLAN_EXT_CAPAB_INTERWORKING                     31
#define WLAN_EXT_CAPAB_QOS_MAP                          32
#define WLAN_EXT_CAPAB_EBR                              33
#define WLAN_EXT_CAPAB_SSPN_INTERFACE                   34
/* 35 - Reserved */
#define WLAN_EXT_CAPAB_MSGCF                            36
#define WLAN_EXT_CAPAB_TDLS                             37
#define WLAN_EXT_CAPAB_TDLS_PROHIBITED                  38
#define WLAN_EXT_CAPAB_TDLS_CHANNEL_SWITCH_PROHIBITED   39
#define WLAN_EXT_CAPAB_REJECT_UNADMITTED_FRAME          40
/* 41-43 - Service Interval Granularity */
#define WLAN_EXT_CAPAB_IDENTIFIER_LOCATION              44
#define WLAN_EXT_CAPAB_U_APSD_COEX                      45
#define WLAN_EXT_CAPAB_WNM_NOTIFCATION                  46
#define WLAN_EXT_CAPAB_QAB                              47
#define WLAN_EXT_CAPAB_UTF_8_SSID                       48
#define WLAN_EXT_CAPAB_QMF                              49
#define WLAN_EXT_CAPAB_QMF_RECONFIG                     50
#define WLAN_EXT_CAPAB_ROBUST_AV_STREAMING              51
#define WLAN_EXT_CAPAB_ADVANCED_GCR                     52
#define WLAN_EXT_CAPAB_MESH_GCR                         53
#define WLAN_EXT_CAPAB_SCS                              54
#define WLAN_EXT_CAPAB_QLOAD_REPORT                     55
#define WLAN_EXT_CAPAB_ALT_EDCA                         56
#define WLAN_EXT_CAPAB_UNPROT_TXOP_NEG                  57
#define WLAN_EXT_CAPAB_PROT_TXOP_NEG                    58
/* 59 - Reserved */
#define WLAN_EXT_CAPAB_PROT_QLOAD_REPORT                60
#define WLAN_EXT_CAPAB_TDLS_WIDER_BW                    61
#define WLAN_EXT_CAPAB_OPMODE_NOTIF                     62
/* 63-64 - Max Number of MSDUs In A-MSDU */
#define WLAN_EXT_CAPAB_CHANNEL_SCHEDULE_MGMT            65
#define WLAN_EXT_CAPAB_GEODB_INBAND_ENABLING_SIGNAL     66
#define WLAN_EXT_CAPAB_NETWORK_CHANNEL_CTRL             67
#define WLAN_EXT_CAPAB_WHITE_SPACE_MAP                  68
#define WLAN_EXT_CAPAB_CHANNEL_AVAIL_QUERY              69
#define WLAN_EXT_CAPAB_FTM_RESPONDER                    70
#define WLAN_EXT_CAPAB_FTM_INITIATOR                    71
#define WLAN_EXT_CAPAB_FILS                             72
#define WLAN_EXT_CAPAB_EXT_SPECTRUM_MGMT                73
#define WLAN_EXT_CAPAB_FUTURE_CHANNEL_GUIDANCE          74
#define WLAN_EXT_CAPAB_PAD                              75
/* 76-79 - Reserved */
#define WLAN_EXT_CAPAB_COMPLETE_NON_TX_BSSID_PROFILE    80
#define WLAN_EXT_CAPAB_SAE_PW_ID                        81
#define WLAN_EXT_CAPAB_SAE_PW_ID_EXCLUSIVELY            82
#define WLAN_EXT_CAPAB_BEACON_PROTECTION                84
#define WLAN_EXT_CAPAB_MSCS                             85
#define WLAN_EXT_CAPAB_SAE_PK_EXCLUSIVELY               88

/* Extended RSN Capabilities */
/* bits 0-3: Field length (n-1) */
#define WLAN_RSNX_CAPAB_PROTECTED_TWT           4
#define WLAN_RSNX_CAPAB_SAE_H2E                 5
#define WLAN_RSNX_CAPAB_SAE_PK                  6
#define WLAN_RSNX_CAPAB_SECURE_LTF              8
#define WLAN_RSNX_CAPAB_SECURE_RTT              9
#define WLAN_RSNX_CAPAB_PROT_RANGE_NEG          10

/* Action frame categories (IEEE Std 802.11-2016, 9.4.1.11, Table 9-76) */
#define WLAN_ACTION_SPECTRUM_MGMT               0
#define WLAN_ACTION_QOS                         1
#define WLAN_ACTION_DLS                         2
#define WLAN_ACTION_BLOCK_ACK                   3
#define WLAN_ACTION_PUBLIC                      4
#define WLAN_ACTION_RADIO_MEASUREMENT           5
#define WLAN_ACTION_FT                          6
#define WLAN_ACTION_HT                          7
#define WLAN_ACTION_SA_QUERY                    8
#define WLAN_ACTION_PROTECTED_DUAL              9
#define WLAN_ACTION_WNM                         10
#define WLAN_ACTION_UNPROTECTED_WNM             11
#define WLAN_ACTION_TDLS                        12
#define WLAN_ACTION_MESH                        13
#define WLAN_ACTION_MULTIHOP                    14
#define WLAN_ACTION_SELF_PROTECTED              15
#define WLAN_ACTION_DMG                         16
#define WLAN_ACTION_WMM                         17 /* WMM Specification 1.1 */
#define WLAN_ACTION_FST                         18
#define WLAN_ACTION_ROBUST_AV_STREAMING         19
#define WLAN_ACTION_UNPROTECTED_DMG             20
#define WLAN_ACTION_VHT                         21
#define WLAN_ACTION_S1G                         22
#define WLAN_ACTION_S1G_RELAY                   23
#define WLAN_ACTION_FLOW_CONTROL                24
#define WLAN_ACTION_CTRL_RESP_MCS_NEG           25
#define WLAN_ACTION_FILS                        26
#define WLAN_ACTION_PROTECTED_FTM               34
#define WLAN_ACTION_VENDOR_SPECIFIC_PROTECTED   126
#define WLAN_ACTION_VENDOR_SPECIFIC             127
/* Note: 128-255 used to report errors by setting category | 0x80 */

/* Public action codes (IEEE Std 802.11-2016, 9.6.8.1, Table 9-307) */
#define WLAN_PA_20_40_BSS_COEX                  0
#define WLAN_PA_DSE_ENABLEMENT                  1
#define WLAN_PA_DSE_DEENABLEMENT                2
#define WLAN_PA_DSE_REG_LOCATION_ANNOUNCE       3
#define WLAN_PA_EXT_CHANNEL_SWITCH_ANNOUNCE     4
#define WLAN_PA_DSE_MEASUREMENT_REQ             5
#define WLAN_PA_DSE_MEASUREMENT_RESP            6
#define WLAN_PA_MEASUREMENT_PILOT               7
#define WLAN_PA_DSE_POWER_CONSTRAINT            8
#define WLAN_PA_VENDOR_SPECIFIC                 9
#define WLAN_PA_GAS_INITIAL_REQ                 10
#define WLAN_PA_GAS_INITIAL_RESP                11
#define WLAN_PA_GAS_COMEBACK_REQ                12
#define WLAN_PA_GAS_COMEBACK_RESP               13
#define WLAN_TDLS_DISCOVERY_RESPONSE            14
#define WLAN_PA_LOCATION_TRACK_NOTIFICATION     15
#define WLAN_PA_QAB_REQUEST_FRAME               16
#define WLAN_PA_QAB_RESPONSE_FRAME              17
#define WLAN_PA_QMF_POLICY                      18
#define WLAN_PA_QMF_POLICY_CHANGE               19
#define WLAN_PA_QLOAD_REQUEST                   20
#define WLAN_PA_QLOAD_REPORT                    21
#define WLAN_PA_HCCA_TXOP_ADVERTISEMENT         22
#define WLAN_PA_HCCA_TXOP_RESPONSE              23
#define WLAN_PA_PUBLIC_KEY                      24
#define WLAN_PA_CHANNEL_AVAILABILITY_QUERY      25
#define WLAN_PA_CHANNEL_SCHEDULE_MANAGEMENT     26
#define WLAN_PA_CONTACT_VERIFICATION_SIGNAL     27
#define WLAN_PA_GDD_ENABLEMENT_REQ              28
#define WLAN_PA_GDD_ENABLEMENT_RESP             29
#define WLAN_PA_NETWORK_CHANNEL_CONTROL         30
#define WLAN_PA_WHITE_SPACE_MAP_ANNOUNCEMENT    31
#define WLAN_PA_FTM_REQUEST                     32
#define WLAN_PA_FTM                             33
#define WLAN_PA_FILS_DISCOVERY                  34
#define WLAN_PA_LOCATION_MEASUREMENT_REPORT     47

/* Protected Dual of Public Action frames (IEEE Std 802.11-2016, 9.6.11,
 * Table 9-332) */
#define WLAN_PROT_DSE_ENABLEMENT                1
#define WLAN_PROT_DSE_DEENABLEMENT              2
#define WLAN_PROT_EXT_CSA                       4
#define WLAN_PROT_MEASUREMENT_REQ               5
#define WLAN_PROT_MEASUREMENT_REPORT            6
#define WLAN_PROT_DSE_POWER_CONSTRAINT          8
#define WLAN_PROT_VENDOR_SPECIFIC               9
#define WLAN_PROT_GAS_INITIAL_REQ               10
#define WLAN_PROT_GAS_INITIAL_RESP              11
#define WLAN_PROT_GAS_COMEBACK_REQ              12
#define WLAN_PROT_GAS_COMEBACK_RESP             13
#define WLAN_PROT_QAB_REQUEST_FRAME             16
#define WLAN_PROT_QAB_RESPONSE_FRAME            17
#define WLAN_PROT_QMF_POLICY                    18
#define WLAN_PROT_QMF_POLICY_CHANGE             19
#define WLAN_PROT_QLOAD_REQUEST                 20
#define WLAN_PROT_QLOAD_REPORT                  21
#define WLAN_PROT_HCCA_TXOP_ADVERTISEMENT       22
#define WLAN_PROT_HCCA_TXOP_RESPONSE            23
#define WLAN_PROT_CHANNEL_AVAILABILITY_QUERY    25
#define WLAN_PROT_CHANNEL_SCHEDULE_MANAGEMENT   26
#define WLAN_PROT_CONTACT_VERIFICATION_SIGNAL   27
#define WLAN_PROT_GDD_ENABLEMENT_REQ            28
#define WLAN_PROT_GDD_ENABLEMENT_RESP           29
#define WLAN_PROT_NETWORK_CHANNEL_CONTROL       30
#define WLAN_PROT_WHITE_SPACE_MAP_ANNOUNCEMENT  31

/* SA Query Action frame (IEEE 802.11w/D8.0, 7.4.9) */
#define WLAN_SA_QUERY_REQUEST               0
#define WLAN_SA_QUERY_RESPONSE              1

#define WLAN_SA_QUERY_TR_ID_LEN             2

/* TDLS action codes */
#define WLAN_TDLS_SETUP_REQUEST             0
#define WLAN_TDLS_SETUP_RESPONSE            1
#define WLAN_TDLS_SETUP_CONFIRM             2
#define WLAN_TDLS_TEARDOWN                  3
#define WLAN_TDLS_PEER_TRAFFIC_INDICATION   4
#define WLAN_TDLS_CHANNEL_SWITCH_REQUEST    5
#define WLAN_TDLS_CHANNEL_SWITCH_RESPONSE   6
#define WLAN_TDLS_PEER_PSM_REQUEST          7
#define WLAN_TDLS_PEER_PSM_RESPONSE         8
#define WLAN_TDLS_PEER_TRAFFIC_RESPONSE     9
#define WLAN_TDLS_DISCOVERY_REQUEST         10

/* Radio Measurement Action codes */
#define WLAN_RRM_RADIO_MEASUREMENT_REQUEST  0
#define WLAN_RRM_RADIO_MEASUREMENT_REPORT   1
#define WLAN_RRM_LINK_MEASUREMENT_REQUEST   2
#define WLAN_RRM_LINK_MEASUREMENT_REPORT    3
#define WLAN_RRM_NEIGHBOR_REPORT_REQUEST    4
#define WLAN_RRM_NEIGHBOR_REPORT_RESPONSE   5

/* Protected Fine Timing Frame Action Field value */
#define WLAN_PROT_FTM_REQUEST   1
#define WLAN_PROT_FTM           2
#define WLAN_PROT_FTM_REPORT    3

#define OUI_MICROSOFT           0x0050f2 /* Microsoft (also used in Wi-Fi specs)*/
#define WPA_IE_VENDOR_TYPE      0x0050f201
#define WMM_IE_VENDOR_TYPE      0x0050f202
#define WPS_IE_VENDOR_TYPE      0x0050f204
#define OUI_WFA                 0x506f9a
#define P2P_IE_VENDOR_TYPE      0x506f9a09
#define WFD_IE_VENDOR_TYPE      0x506f9a0a
#define WFD_OUI_TYPE            10
#define HS20_IE_VENDOR_TYPE     0x506f9a10
#define OSEN_IE_VENDOR_TYPE     0x506f9a12
#define MBO_IE_VENDOR_TYPE      0x506f9a16
#define MBO_OUI_TYPE            22
#define OWE_IE_VENDOR_TYPE      0x506f9a1c
#define OWE_OUI_TYPE            28
#define MULTI_AP_OUI_TYPE       0x1B
#define DPP_CC_IE_VENDOR_TYPE   0x506f9a1e
#define DPP_CC_OUI_TYPE         0x1e
#define SAE_PK_IE_VENDOR_TYPE   0x506f9a1f
#define SAE_PK_OUI_TYPE         0x1f
#define QM_IE_VENDOR_TYPE       0x506f9a22
#define QM_IE_OUI_TYPE          0x22
#define WFA_CAPA_IE_VENDOR_TYPE 0x506f9a23
#define WFA_CAPA_OUI_TYPE       0x23

/* Radio Measurement capabilities (from RM Enabled Capabilities element)
 * IEEE Std 802.11-2016, 9.4.2.45, Table 9-157 */
/* byte 1 (out of 5) */
#define WLAN_RRM_CAPS_LINK_MEASUREMENT      BIT(0)
#define WLAN_RRM_CAPS_NEIGHBOR_REPORT       BIT(1)
#define WLAN_RRM_CAPS_BEACON_REPORT_PASSIVE BIT(4)
#define WLAN_RRM_CAPS_BEACON_REPORT_ACTIVE  BIT(5)
#define WLAN_RRM_CAPS_BEACON_REPORT_TABLE   BIT(6)
/* byte 2 (out of 5) */
#define WLAN_RRM_CAPS_LCI_MEASUREMENT       BIT(4)
/* byte 5 (out of 5) */
#define WLAN_RRM_CAPS_FTM_RANGE_REPORT      BIT(2)

/*
 * IEEE P802.11-REVmc/D5.0, 9.4.2.21.19 (Fine Timing Measurement Range
 * request) - Minimum AP count
 */
#define WLAN_RRM_RANGE_REQ_MAX_MIN_AP   15

/* Timeout Interval Type */
#define WLAN_TIMEOUT_REASSOC_DEADLINE   1
#define WLAN_TIMEOUT_KEY_LIFETIME       2
#define WLAN_TIMEOUT_ASSOC_COMEBACK     3

#define FILS_NONCE_LEN      16
#define FILS_SESSION_LEN    8
#define FILS_CACHE_ID_LEN   2
#define FILS_MAX_KEY_AUTH_LEN 48

struct ieee80211_hdr {
    uint16_t frame_control;
    uint16_t duration_id;
    uint8_t  addr1[6];
    uint8_t  addr2[6];
    uint8_t  addr3[6];
    uint16_t seq_ctrl;
    /* followed by 'uint8_t addr4[6];' if ToDS and FromDS is set in data frame
     */
} STRUCT_PACKED;


#define IEEE80211_HDRLEN (sizeof(struct ieee80211_hdr))

#define IEEE80211_FC(type, stype) host_to_le16((type << 2) | (stype << 4))

#define WPAS_MIN_PASSPHRASE_LEN 8
#define WPAS_MAX_PASSPHRASE_LEN 63

// #define AUTH_IE_OFFSET       6

struct ieee80211_mgmt {
    uint16_t frame_control;
    uint16_t duration;
    uint8_t  da[6];
    uint8_t  sa[6];
    uint8_t  bssid[6];
    uint16_t seq_ctrl;
    union {
        struct {
            uint16_t auth_alg;
            uint16_t auth_transaction;
            uint16_t status_code;
            /* possibly followed by Challenge text */
            uint8_t variable[];
        } STRUCT_PACKED auth;
        struct {
            uint16_t reason_code;
            uint8_t variable[];
        } STRUCT_PACKED deauth;
        struct {
            uint16_t capab_info;
            uint16_t listen_interval;
            /* followed by SSID and Supported rates */
            uint8_t variable[];
        } STRUCT_PACKED assoc_req;
        struct {
            uint16_t capab_info;
            uint16_t status_code;
            uint16_t aid;
            /* followed by Supported rates */
            uint8_t variable[];
        } STRUCT_PACKED assoc_resp, reassoc_resp;
        struct {
            uint16_t capab_info;
            uint16_t listen_interval;
            uint8_t current_ap[6];
            /* followed by SSID and Supported rates */
            uint8_t variable[];
        } STRUCT_PACKED reassoc_req;
        struct {
            uint16_t reason_code;
            uint8_t variable[];
        } STRUCT_PACKED disassoc;
        struct {
            uint8_t timestamp[8];
            uint16_t beacon_int;
            uint16_t capab_info;
            /* followed by some of SSID, Supported rates,
             * FH Params, DS Params, CF Params, IBSS Params, TIM */
            uint8_t variable[];
        } STRUCT_PACKED beacon;
        /* probe_req: only variable items: SSID, Supported rates */
        struct {
            uint8_t timestamp[8];
            uint16_t beacon_int;
            uint16_t capab_info;
            /* followed by some of SSID, Supported rates,
             * FH Params, DS Params, CF Params, IBSS Params */
            uint8_t variable[];
        } STRUCT_PACKED probe_resp;
        struct {
            uint8_t category;
            union {
                struct {
                    uint8_t action_code;
                    uint8_t dialog_token;
                    uint8_t status_code;
                    uint8_t variable[];
                } STRUCT_PACKED wmm_action;
                struct{
                    uint8_t action_code;
                    uint8_t element_id;
                    uint8_t length;
                    uint8_t switch_mode;
                    uint8_t new_chan;
                    uint8_t switch_count;
                } STRUCT_PACKED chan_switch;
                struct{
                    uint8_t action_code;
                    uint16_t trans_id;
                } STRUCT_PACKED sa_query;
                struct {
                    uint8_t action;
                    uint8_t variable[];
                } STRUCT_PACKED public_action;

            } u;
        } STRUCT_PACKED action;
    } u;
} STRUCT_PACKED;

/* HT Capabilities element */
struct ieee80211_ht_capabilities {
    uint16_t ht_capabilities_info;
    uint8_t a_mpdu_params; /* Maximum A-MPDU Length Exponent B0..B1
               * Minimum MPDU Start Spacing B2..B4
               * Reserved B5..B7 */
    uint8_t supported_mcs_set[16];
    uint16_t ht_extended_capabilities;
    uint32_t tx_bf_capability_info;
    uint8_t asel_capabilities;
} STRUCT_PACKED;


/* HT Operation element */
struct ieee80211_ht_operation {
    uint8_t primary_chan;
    /* Five octets of HT Operation Information */
    uint8_t ht_param; /* B0..B7 */
    uint16_t operation_mode; /* B8..B23 */
    uint16_t param; /* B24..B39 */
    uint8_t basic_mcs_set[16];
} STRUCT_PACKED;

struct ieee80211_vht_capabilities {
    uint32_t vht_capabilities_info;
    struct {
        uint16_t rx_map;
        uint16_t rx_highest;
        uint16_t tx_map;
        uint16_t tx_highest;
    } vht_supported_mcs_set;
} STRUCT_PACKED;

struct ieee80211_vht_operation {
    uint8_t vht_op_info_chwidth;
    uint8_t vht_op_info_chan_center_freq_seg0_idx;
    uint8_t vht_op_info_chan_center_freq_seg1_idx;
    uint16_t vht_basic_mcs_set;
} STRUCT_PACKED;

/* HT Capabilities Info field within HT Capabilities element */
#define HT_CAP_INFO_LDPC_CODING_CAP             ((uint16_t) BIT(0))
#define HT_CAP_INFO_SUPP_CHANNEL_WIDTH_SET      ((uint16_t) BIT(1))
#define HT_CAP_INFO_SMPS_MASK                   ((uint16_t) (BIT(2) | BIT(3)))
#define HT_CAP_INFO_SMPS_STATIC                 ((uint16_t) 0)
#define HT_CAP_INFO_SMPS_DYNAMIC                ((uint16_t) BIT(2))
#define HT_CAP_INFO_SMPS_DISABLED               ((uint16_t) (BIT(2) | BIT(3)))
#define HT_CAP_INFO_GREEN_FIELD                 ((uint16_t) BIT(4))
#define HT_CAP_INFO_SHORT_GI20MHZ               ((uint16_t) BIT(5))
#define HT_CAP_INFO_SHORT_GI40MHZ               ((uint16_t) BIT(6))
#define HT_CAP_INFO_TX_STBC                     ((uint16_t) BIT(7))
#define HT_CAP_INFO_RX_STBC_MASK                ((uint16_t) (BIT(8) | BIT(9)))
#define HT_CAP_INFO_RX_STBC_1                   ((uint16_t) BIT(8))
#define HT_CAP_INFO_RX_STBC_12                  ((uint16_t) BIT(9))
#define HT_CAP_INFO_RX_STBC_123                 ((uint16_t) (BIT(8) | BIT(9)))
#define HT_CAP_INFO_DELAYED_BA                  ((uint16_t) BIT(10))
#define HT_CAP_INFO_MAX_AMSDU_SIZE              ((uint16_t) BIT(11))
#define HT_CAP_INFO_DSSS_CCK40MHZ               ((uint16_t) BIT(12))
/* B13 - Reserved (was PSMP support during P802.11n development) */
#define HT_CAP_INFO_40MHZ_INTOLERANT            ((uint16_t) BIT(14))
#define HT_CAP_INFO_LSIG_TXOP_PROTECT_SUPPORT   ((uint16_t) BIT(15))

#define ERP_INFO_NON_ERP_PRESENT BIT(0)
#define ERP_INFO_USE_PROTECTION BIT(1)
#define ERP_INFO_BARKER_PREAMBLE_MODE BIT(2)

/* HT Protection (B8..B9 of HT Operation Information) */
#define HT_PROT_NO_PROTECTION           0
#define HT_PROT_NONMEMBER_PROTECTION    1
#define HT_PROT_20MHZ_PROTECTION        2
#define HT_PROT_NON_HT_MIXED            3
/* Bits within ieee80211_ht_operation::operation_mode (BIT(0) maps to B8 in
 * HT Operation Information) */
#define HT_OPER_OP_MODE_HT_PROT_MASK ((uint16_t)    (BIT(0) | BIT(1))) /* B8..B9 */
#define HT_OPER_OP_MODE_NON_GF_HT_STAS_PRESENT      ((uint16_t) BIT(2)) /* B10 */
/* BIT(3), i.e., B11 in HT Operation Information field - Reserved */
#define HT_OPER_OP_MODE_OBSS_NON_HT_STAS_PRESENT    ((uint16_t) BIT(4)) /* B12 */
/* BIT(5)..BIT(15), i.e., B13..B23 - Reserved */

#define HE_NSS_MAX_STREAMS                8

#define MULTI_AP_SUB_ELEM_TYPE  0x06
#define MULTI_AP_TEAR_DOWN      BIT(4)
#define MULTI_AP_FRONTHAUL_BSS  BIT(5)
#define MULTI_AP_BACKHAUL_BSS   BIT(6)
#define MULTI_AP_BACKHAUL_STA   BIT(7)

#define WMM_OUI_TYPE                        2
#define WMM_OUI_SUBTYPE_INFORMATION_ELEMENT 0
#define WMM_OUI_SUBTYPE_PARAMETER_ELEMENT   1
#define WMM_OUI_SUBTYPE_TSPEC_ELEMENT       2
#define WMM_VERSION                         1

/*
 * WMM Information Element (used in (Re)Association Request frames; may also be
 * used in Beacon frames)
 */
struct wmm_information_element {
    /* Element ID: 221 (0xdd); Length: 7 */
    /* required fields for WMM version 1 */
    uint8_t oui[3]; /* 00:50:f2 */
    uint8_t oui_type; /* 2 */
    uint8_t oui_subtype; /* 0 */
    uint8_t version; /* 1 for WMM version 1.0 */
    uint8_t qos_info; /* AP/STA specific QoS info */

} STRUCT_PACKED;

#define WMM_AC_AIFSN_MASK 0x0f
#define WMM_AC_AIFNS_SHIFT 0
#define WMM_AC_ACM 0x10
#define WMM_AC_ACI_MASK 0x60
#define WMM_AC_ACI_SHIFT 5

#define WMM_AC_ECWMIN_MASK 0x0f
#define WMM_AC_ECWMIN_SHIFT 0
#define WMM_AC_ECWMAX_MASK 0xf0
#define WMM_AC_ECWMAX_SHIFT 4

struct wmm_ac_parameter {
    uint8_t aci_aifsn; /* AIFSN, ACM, ACI */
    uint8_t cw; /* ECWmin, ECWmax (CW = 2^ECW - 1) */
    uint16_t txop_limit;
} STRUCT_PACKED;

/*
 * WMM Parameter Element (used in Beacon, Probe Response, and (Re)Association
 * Response frmaes)
 */
struct wmm_parameter_element {
    /* Element ID: 221 (0xdd); Length: 24 */
    /* required fields for WMM version 1 */
    uint8_t oui[3]; /* 00:50:f2 */
    uint8_t oui_type; /* 2 */
    uint8_t oui_subtype; /* 1 */
    uint8_t version; /* 1 for WMM version 1.0 */
    uint8_t qos_info; /* AP/STA specific QoS info */
    uint8_t reserved; /* 0 */
    struct wmm_ac_parameter ac[4]; /* AC_BE, AC_BK, AC_VI, AC_VO */
} STRUCT_PACKED;

/* Access Categories / ACI to AC coding */
enum wmm_ac {
    WMM_AC_BE = 0 /* Best Effort */,
    WMM_AC_BK = 1 /* Background */,
    WMM_AC_VI = 2 /* Video */,
    WMM_AC_VO = 3 /* Voice */,
    WMM_AC_NUM = 4
};

#define OUI_BROADCOM        0x00904c /* Broadcom (Epigram) */
#define VENDOR_VHT_TYPE     0x04
#define VENDOR_VHT_SUBTYPE  0x08
#define VENDOR_VHT_SUBTYPE2 0x00

#define VENDOR_HT_CAPAB_OUI_TYPE 0x33 /* 00-90-4c:0x33 */

struct ieee80211_he_capabilities {
    uint8_t he_mac_capab_info[6];
    uint8_t he_phy_capab_info[11];
    /* Followed by 4, 8, or 12 octets of Supported HE-MCS And NSS Set field
    * and optional variable length PPE Thresholds field. */
    uint8_t optional[37];
} STRUCT_PACKED;

#define IEEE80211_HE_CAPAB_MIN_LEN (6 + 11)
struct ieee80211_he_operation {
    uint32_t he_oper_params; /* HE Operation Parameters[3] and
                  * BSS Color Information[1] */
    uint16_t he_mcs_nss_set;
    /* Followed by conditional VHT Operation Information (3 octets),
     * Max Co-Hosted BSSID Indicator subfield (1 octet), and/or 6 GHz
     * Operation Information subfield (5 octets). */
} STRUCT_PACKED;

/* HE Capabilities Information defines */
#define HE_MACCAP_TWT_RESPONDER                         ((uint8_t) BIT(2))

#define HE_PHYCAP_CHANNEL_WIDTH_SET_IDX                 0
#define HE_PHYCAP_CHANNEL_WIDTH_MASK                    ((uint8_t) (BIT(1) | BIT(2) | \
                                                                    BIT(3) | BIT(4)))
#define HE_PHYCAP_CHANNEL_WIDTH_SET_40MHZ_IN_2G         ((uint8_t) BIT(1))
#define HE_PHYCAP_CHANNEL_WIDTH_SET_40MHZ_80MHZ_IN_5G   ((uint8_t) BIT(2))
#define HE_PHYCAP_CHANNEL_WIDTH_SET_160MHZ_IN_5G        ((uint8_t) BIT(3))
#define HE_PHYCAP_CHANNEL_WIDTH_SET_80PLUS80MHZ_IN_5G   ((uint8_t) BIT(4))

#define HE_PHYCAP_SU_BEAMFORMER_CAPAB_IDX   3
#define HE_PHYCAP_SU_BEAMFORMER_CAPAB       ((uint8_t) BIT(7))
#define HE_PHYCAP_SU_BEAMFORMEE_CAPAB_IDX   4
#define HE_PHYCAP_SU_BEAMFORMEE_CAPAB       ((uint8_t) BIT(0))
#define HE_PHYCAP_MU_BEAMFORMER_CAPAB_IDX   4
#define HE_PHYCAP_MU_BEAMFORMER_CAPAB       ((uint8_t) BIT(1))

#define HE_PHYCAP_PPE_THRESHOLD_PRESENT_IDX 6
#define HE_PHYCAP_PPE_THRESHOLD_PRESENT     ((uint8_t) BIT(7))

/* HE PPE Threshold define */
#define HE_PPE_THRES_RU_INDEX_BITMASK_MASK  0xf
#define HE_PPE_THRES_RU_INDEX_BITMASK_SHIFT 3
#define HE_PPE_THRES_NSS_MASK               0x7

/* HE Operation defines */
/* HE Operation Parameters and BSS Color Information fields */
#define HE_OPERATION_DFLT_PE_DURATION_MASK      ((uint32_t) (BIT(0) | BIT(1) | \
                                                            BIT(2)))
#define HE_OPERATION_DFLT_PE_DURATION_OFFSET    0
#define HE_OPERATION_TWT_REQUIRED               ((uint32_t) BIT(3))
#define HE_OPERATION_RTS_THRESHOLD_MASK         ((uint32_t) (BIT(4) | BIT(5) | \
                                                            BIT(6) | BIT(7) | \
                                                            BIT(8) | BIT(9) | \
                                                            BIT(10) | BIT(11) | \
                                                            BIT(12) | BIT(13)))
#define HE_OPERATION_RTS_THRESHOLD_OFFSET   4
#define HE_OPERATION_VHT_OPER_INFO          ((uint32_t) BIT(14))
#define HE_OPERATION_COHOSTED_BSS           ((uint32_t) BIT(15))
#define HE_OPERATION_ER_SU_DISABLE          ((uint32_t) BIT(16))
#define HE_OPERATION_6GHZ_OPER_INFO         ((uint32_t) BIT(17))
#define HE_OPERATION_BSS_COLOR_MASK         ((uint32_t) (BIT(24) | BIT(25) | \
                                                        BIT(26) | BIT(27) | \
                                                        BIT(28) | BIT(29)))
#define HE_OPERATION_BSS_COLOR_PARTIAL      ((uint32_t) BIT(30))
#define HE_OPERATION_BSS_COLOR_DISABLED     ((uint32_t) BIT(31))
#define HE_OPERATION_BSS_COLOR_OFFSET       24

#ifndef offsetof
#define offsetof(type, member) ((long) &((type *) 0)->member)
#endif

#define MAX_NOF_MB_IES_SUPPORTED    5
#define MAX_NUM_FRAG_IES_SUPPORTED  3

struct mb_ies_info {
    struct {
        const uint8_t *ie;
        uint8_t ie_len;
    } ies[MAX_NOF_MB_IES_SUPPORTED];
    uint8_t nof_ies;
};

struct frag_ies_info {
    struct {
        uint8_t eid;
        uint8_t eid_ext;
        const uint8_t *ie;
        uint8_t ie_len;
    } frags[MAX_NUM_FRAG_IES_SUPPORTED];

    uint8_t n_frags;

    /* the last parsed element ID and element extension ID */
    uint8_t last_eid;
    uint8_t last_eid_ext;
};

/* Parsed Information Elements */
struct ieee802_11_elems {
    const uint8_t *ssid;
    const uint8_t *supp_rates;
    const uint8_t *ds_params;
    const uint8_t *challenge;
    const uint8_t *erp_info;
    const uint8_t *ext_supp_rates;
    const uint8_t *wpa_ie;
    const uint8_t *rsn_ie;
    const uint8_t *rsnxe;
    const uint8_t *wmm; /* WMM Information or Parameter Element */
    const uint8_t *wmm_tspec;
    const uint8_t *wps_ie;
    const uint8_t *supp_channels;
    const uint8_t *mdie;
    const uint8_t *ftie;
    const uint8_t *timeout_int;
    const uint8_t *ht_capabilities;
    const uint8_t *ht_operation;
    const uint8_t *mesh_config;
    const uint8_t *mesh_id;
    const uint8_t *peer_mgmt;
    const uint8_t *vht_capabilities;
    const uint8_t *vht_operation;
    const uint8_t *vht_opmode_notif;
    const uint8_t *vendor_ht_cap;
    const uint8_t *vendor_vht;
    const uint8_t *p2p;
    const uint8_t *wfd;
    const uint8_t *link_id;
    const uint8_t *interworking;
    const uint8_t *qos_map_set;
    const uint8_t *hs20;
    const uint8_t *ext_capab;
    const uint8_t *bss_max_idle_period;
    const uint8_t *ssid_list;
    const uint8_t *osen;
    const uint8_t *mbo;
    const uint8_t *ampe;
    const uint8_t *mic;
    const uint8_t *pref_freq_list;
    const uint8_t *supp_op_classes;
    const uint8_t *rrm_enabled;
    const uint8_t *cag_number;
    const uint8_t *ap_csn;
    const uint8_t *fils_indic;
    const uint8_t *dils;
    const uint8_t *assoc_delay_info;
    const uint8_t *fils_req_params;
    const uint8_t *fils_key_confirm;
    const uint8_t *fils_session;
    const uint8_t *fils_hlp;
    const uint8_t *fils_ip_addr_assign;
    const uint8_t *key_delivery;
    const uint8_t *wrapped_data;
    const uint8_t *fils_pk;
    const uint8_t *fils_nonce;
    const uint8_t *owe_dh;
    const uint8_t *power_capab;
    const uint8_t *roaming_cons_sel;
    const uint8_t *password_id;
    const uint8_t *oci;
    const uint8_t *multi_ap;
    const uint8_t *he_capabilities;
    const uint8_t *he_operation;
    const uint8_t *short_ssid_list;
    const uint8_t *he_6ghz_band_cap;
    const uint8_t *sae_pk;
    const uint8_t *s1g_capab;
    const uint8_t *pasn_params;

    uint8_t ssid_len;
    uint8_t supp_rates_len;
    uint8_t challenge_len;
    uint8_t ext_supp_rates_len;
    uint8_t wpa_ie_len;
    uint8_t rsn_ie_len;
    uint8_t rsnxe_len;
    uint8_t wmm_len; /* 7 = WMM Information; 24 = WMM Parameter */
    uint8_t wmm_tspec_len;
    uint8_t wps_ie_len;
    uint8_t supp_channels_len;
    uint8_t mdie_len;
    uint8_t ftie_len;
    uint8_t mesh_config_len;
    uint8_t mesh_id_len;
    uint8_t peer_mgmt_len;
    uint8_t vendor_ht_cap_len;
    uint8_t vendor_vht_len;
    uint8_t p2p_len;
    uint8_t wfd_len;
    uint8_t interworking_len;
    uint8_t qos_map_set_len;
    uint8_t hs20_len;
    uint8_t ext_capab_len;
    uint8_t ssid_list_len;
    uint8_t osen_len;
    uint8_t mbo_len;
    uint8_t ampe_len;
    uint8_t mic_len;
    uint8_t pref_freq_list_len;
    uint8_t supp_op_classes_len;
    uint8_t rrm_enabled_len;
    uint8_t cag_number_len;
    uint8_t fils_indic_len;
    uint8_t dils_len;
    uint8_t fils_req_params_len;
    uint8_t fils_key_confirm_len;
    uint8_t fils_hlp_len;
    uint8_t fils_ip_addr_assign_len;
    uint8_t key_delivery_len;
    uint8_t wrapped_data_len;
    uint8_t fils_pk_len;
    uint8_t owe_dh_len;
    uint8_t power_capab_len;
    uint8_t roaming_cons_sel_len;
    uint8_t password_id_len;
    uint8_t oci_len;
    uint8_t multi_ap_len;
    uint8_t he_capabilities_len;
    uint8_t he_operation_len;
    uint8_t short_ssid_list_len;
    uint8_t sae_pk_len;
    uint8_t pasn_params_len;

    struct mb_ies_info mb_ies;
    struct frag_ies_info frag_ies;
};

#endif /* _IEEE802_11_DEFS_H_ */
