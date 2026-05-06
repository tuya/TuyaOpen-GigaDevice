/*
 * wpa_supplicant/hostapd - Build time configuration defines
 * Copyright (c) 2005-2006, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2025, GigaDevice Semiconductor Inc.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 * This header file can be used to define configuration defines that were
 * originally defined in Makefile. This is mainly meant for IDE use or for
 * systems that do not have suitable 'make' tool. In these cases, it may be
 * easier to have a single place for defining all the needed C pre-processor
 * defines.
 */

#ifndef BUILD_CONFIG_H
#define BUILD_CONFIG_H

#include "wlan_config.h"

/* Insert configuration defines, e.g., #define EAP_MD5, here, if needed. */

#define CONFIG_GDWIFI
#define CONFIG_LWIP
#define CONFIG_DRIVER_GDWIFI
#define OS_NO_C_LIB_DEFINES
#ifndef CFG_MAC_DBG
#define CONFIG_NO_WPA_MSG
#define CONFIG_NO_STDOUT_DEBUG
#endif
#define CONFIG_NO_HOSTAPD_LOGGER
#define CONFIG_CTRL_IFACE
#define CONFIG_CTRL_IFACE_UDP

#define CONFIG_CRYPTO_INTERNAL

// for now only used for SAE
#define CONFIG_SME
#define CONFIG_SAE
#define CONFIG_OWE
#define CONFIG_ECC
#ifdef CFG_SAE_PK
#define CONFIG_SAE_PK
#define CONFIG_SHA384
#define CONFIG_SHA512
#endif

// AP support
#ifdef CFG_SOFTAP
#define CONFIG_AP
#define NEED_AP_MLME
#define CONFIG_NO_RADIUS
#define CONFIG_NO_ACCOUNTING
#define CONFIG_NO_VLAN
#define CONFIG_NO_ROBUST_AV
// #define EAP_SERVER_IDENTITY
// #define CONFIG_WNM_AP
#define CONFIG_AP_NO_DFS
#define CONFIG_AP_NO_MBSSID
#define CONFIG_AP_NO_AUTHSRV
#define CONFIG_AP_NO_RNR
#define CONFIG_AP_NO_FT
#define CONFIG_AP_NO_CSA
#define CONFIG_AP_NO_40MHZ_AND_MORE
#define CONFIG_AP_NO_5GHZ_AND_MORE
#define CONFIG_NO_EDMG_SUPPORT
#define CONFIG_IEEE80211AX
#endif // CFG_SOFTAP

#if defined CONFIG_AP || defined CONFIG_IBSS_MESH_SUPPORT
#define CONFIG_HOSTAP_SET_FREQ
#endif

#define CONFIG_NO_ROAMING
#ifdef CONFIG_NO_ROAMING
#define CONFIG_NO_EST_IN_SCAN
#endif

#define CONFIG_SME_EXT_AUTH_ONLY
// #define CONFIG_FFC_GROUP_SUPPORT
#define CONFIG_NO_WMM_AC
#define CONFIG_NO_RRM
#define CONFIG_NO_PNO_OR_SCHE_SCAN
#define CONFIG_NO_RAND_ADDR
#define CONFIG_NO_CONFIG_WRITE
#define CONFIG_NO_CONFIG_BLOBS
#define CONFIG_REMOVE_UNUSED_WIFI_DRIVER
#define CONFIG_REMOVE_UNUSED_EVENTS
#define CONFIG_WPA_CLI_DISABLE
#define CONFIG_WPA3_PMK_CACHE_ENABLE

#if (CONFIG_PLATFORM != PLATFORM_FPGA_32103_V7)
#define CONFIG_NO_RANDOM_POOL
#define WPA_HW_SECURITY_ENGINE_ENABLE
#define HW_ACC_ENGINE_LOCK() GLOBAL_INT_DISABLE()
#define HW_ACC_ENGINE_UNLOCK() GLOBAL_INT_RESTORE()
#endif

#ifdef CFG_WFA_HE
#define CONFIG_WEP
#define CONFIG_WNM
// #define CONFIG_MBO
#undef CONFIG_NO_EST_IN_SCAN
#undef CONFIG_NO_RRM
#endif

#ifdef CFG_WPS
#define IEEE8021X_EAPOL
#define CONFIG_WPS
#define EAP_WSC
#endif

#ifdef CFG_8021x_EAP_TLS
#undef IEEE8021X_EAPOL
#undef CONFIG_SHA384
#undef CONFIG_SHA512

#define CONFIG_TLS_INTERNAL_CLIENT
#define CONFIG_INTERNAL_LIBTOMMATH
#define IEEE8021X_EAPOL
#define EAP_TLS
#define CONFIG_SUITEB
#define CONFIG_SUITEB192
#define CONFIG_SHA256
#define CONFIG_SHA384
#define CONFIG_INTERNAL_SHA384
#define CONFIG_SHA512
#define CONFIG_INTERNAL_SHA512
#define CONFIG_TLSV12
#endif

#ifdef CFG_80211R
#define CONFIG_IEEE80211R
#endif

#endif /* BUILD_CONFIG_H */
