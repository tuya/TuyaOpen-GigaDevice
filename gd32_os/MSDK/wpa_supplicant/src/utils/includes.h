/*
 * wpa_supplicant/hostapd - Default include files
 * Copyright (c) 2005-2006, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2025, GigaDevice Semiconductor Inc.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 * This header file is included into all C files so that commonly used header
 * files can be selected with OS specific ifdef blocks in one place instead of
 * having to have OS/C library specific selection in many files.
 */

#ifndef INCLUDES_H
#define INCLUDES_H

/* Include possible build time configuration before including anything else */
#include "build_config.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifndef _WIN32_WCE
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#endif /* _WIN32_WCE */
#include <ctype.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif /* _MSC_VER */

 /* GD modify */
#ifdef CONFIG_GDWIFI
#include "ll.h"
#include "wrapper_os.h"
#define vsnprintf dbg_vsnprintf
#include "wifi_vif.h"
#include "wifi_net_ip.h"
#include "dbg_print.h"
#endif /* CONFIG_GDWIFI */

#ifdef CONFIG_LWIP

#include <sys/socket.h>

#else /* ! CONFIG_LWIP */
 /* GD modify end */
#ifndef CONFIG_NATIVE_WINDOWS
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifndef __vxworks
#include <sys/uio.h>
#include <sys/time.h>
#endif /* __vxworks */
#endif /* CONFIG_NATIVE_WINDOWS */
#endif /* CONFIG_LWIP */ /* GD modify */

#endif /* INCLUDES_H */
