/*
 * coap_config.h.lwip -- LwIP configuration for libcoap
 *
 * Copyright (C) 2021-2024 Olaf Bergmann <bergmann@tzi.org> and others
 * Copyright (c) 2024, GigaDevice Semiconductor Inc.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * This file is part of the CoAP library libcoap. Please see README for terms
 * of use.
 */

#ifndef COAP_CONFIG_H_
#define COAP_CONFIG_H_

#include <lwip/opt.h>
#include <lwip/debug.h>
#include <lwip/def.h> /* provide ntohs, htons */

#define WITH_LWIP 1

#if LWIP_IPV4
#define COAP_IPV4_SUPPORT 1
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
#define COAP_IPV6_SUPPORT 1
#endif /* LWIP_IPV6 */

// #ifndef COAP_CONSTRAINED_STACK
// #define COAP_CONSTRAINED_STACK 0
// #endif

#ifndef COAP_DISABLE_TCP
#define COAP_DISABLE_TCP 1
#endif

#ifndef COAP_ASYNC_SUPPORT
#define COAP_ASYNC_SUPPORT 1
#endif

#define PACKAGE_NAME "libcoap"
#define PACKAGE_VERSION "4.3.4"
#define PACKAGE_STRING "libcoap 4.3.4"

#define assert(x) LWIP_ASSERT("CoAP assert failed", x)

/* it's just provided by libc. i hope we don't get too many of those, as
 * actually we'd need autotools again to find out what environment we're
 * building in */
#define HAVE_STRNLEN 1
#define HAVE_LIMITS_H
#define HAVE_NETDB_H
#define HAVE_SNPRINTF

#define HAVE_GETRANDOM
#define HAVE_STDIO_H
#define HAVE_INTTYPES_H
#define COAP_RESOURCES_NOHASH
#define HAVE_MALLOC

#define COAP_THREAD_SAFE 1
#define COAP_THREAD_RECURSIVE_CHECK 0
#define LWIP_TCPIP_CORE_LOCKING 1

#endif /* COAP_CONFIG_H_ */
