/** @file
 *  @brief Bluetooth subsystem log APIs.
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _MESH_BLUETOOTH_LOG_H_
#define _MESH_BLUETOOTH_LOG_H_

#include <stdbool.h>
#include <string.h>
#include "debug_print.h"
#include "trace_ext.h"
#include "ll.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LOG_LEVEL
#define LOG_LEVEL       0
#endif

#if 1
extern uint8_t mesh_log_mask[];
enum {
    NONE, ERR, WARN, INFO, DEBUG, DUMP
};

extern uint8_t mesh_debug_level;

#define MESH_PRINT(level, fmt, ...)             \
    do {                                        \
        if (LOG_LEVEL % 2) {                    \
            if (level <= (mesh_log_mask[LOG_LEVEL >> 1] >> 4)) {\
                co_printf(fmt, ## __VA_ARGS__); \
            }                                   \
        }                                       \
        else {                                  \
            if (level <= (mesh_log_mask[LOG_LEVEL >> 1] & 0x0F)) { \
                co_printf(fmt, ## __VA_ARGS__); \
            }                                   \
        }                                       \
    } while (0)

#define LOG_ERR(fmt, ...)     MESH_PRINT(ERR, "MESH.E: #%s# " fmt "\r\n", __func__ , ##__VA_ARGS__)
#define LOG_WRN(fmt, ...)     MESH_PRINT(WARN, "MESH.W: #%s# " fmt "\r\n", __func__, ##__VA_ARGS__)
#define LOG_INF(fmt, ...)     MESH_PRINT(INFO, "MESH.I: #%s# " fmt "\r\n", __func__ , ##__VA_ARGS__)
#define LOG_DBG(fmt, ...)     MESH_PRINT(DEBUG, "MESH.D: #%s# " fmt "\r\n", __func__ , ##__VA_ARGS__)
#define LOG_DUMP(fmt, ...)     MESH_PRINT(DUMP, "MESH.D: #%s# " fmt "\r\n", __func__ , ##__VA_ARGS__)

#define LOG_HEXDUMP_DBG(x, y, z)    debug_print_dump_data(z, (char *)x, y)

#define NET_BUF_SIMPLE_DBG(fmt, ...)    MESH_PRINT(DUMP, "MESH.ND: #%s# " fmt "\r\n", __func__ , ##__VA_ARGS__)

#else
#if (DBG_LEVEL >= LEVEL_ERROR)
#define MESH_DBG_BUFFER_LEVEL_ERROR(type, module, fmt, ...) do {\
                  static const char format[] TRACE_DATA = fmt; \
                  uint16_t __p[] = {__VA_ARGS__} ; \
                  trace_ble_log((uint32_t)format, sizeof(__p)/sizeof(__p[0]), sizeof(__p) > 0 ? __p : NULL, type, module, LEVEL_ERROR); \
                } while (0)
#else
#define MESH_DBG_BUFFER_LEVEL_ERROR(type, module, fmt, ...)
#endif

#if (DBG_LEVEL >= LEVEL_WARN)
#define MESH_DBG_BUFFER_LEVEL_WARN(type, module, fmt, ...)  do {\
                  static const char format[] TRACE_DATA = fmt; \
                  uint16_t __p[] = {__VA_ARGS__} ; \
                  trace_ble_log((uint32_t)format, sizeof(__p)/sizeof(__p[0]), sizeof(__p) > 0 ? __p : NULL, type, module, LEVEL_WARN); \
                } while (0)
#else
#define MESH_DBG_BUFFER_LEVEL_WARN(type, module, fmt, ...)
#endif

#if (DBG_LEVEL >= LEVEL_INFO)
#define MESH_DBG_BUFFER_LEVEL_INFO(type, module, fmt, ...)  do {\
                  static const char format[] TRACE_DATA = fmt; \
                  uint16_t __p[] = {__VA_ARGS__} ;                               \
                  trace_ble_log((uint32_t)format, sizeof(__p)/sizeof(__p[0]), sizeof(__p) > 0 ? __p : NULL, type, module, LEVEL_INFO);        \
                } while (0)
#else
#define MESH_DBG_BUFFER_LEVEL_INFO(type, module, fmt, ...)
#endif

#if (DBG_LEVEL >= LEVEL_TRACE)
#define MESH_DBG_BUFFER_LEVEL_TRACE(type, module, fmt, ...) do {\
                  static const char format[] TRACE_DATA = fmt; \
                  uint16_t __p[] = {__VA_ARGS__} ;                               \
                  trace_ble_log((uint32_t)format, sizeof(__p)/sizeof(__p[0]), sizeof(__p) > 0 ? __p : NULL, type, module, LEVEL_TRACE);        \
                } while (0)
#else
#define MESH_DBG_BUFFER_LEVEL_TRACE(type, module, fmt, ...)
#endif

#if (DBG_LEVEL >= LEVEL_VERBOSE)
#define MESH_DBG_BUFFER_LEVEL_VERBOSE(type, module, fmt, ...) do {\
                  static const char format[] TRACE_DATA = fmt; \
                  uint16_t __p[] = {__VA_ARGS__} ;                               \
                  trace_ble_log((uint32_t)format, sizeof(__p)/sizeof(__p[0]), sizeof(__p) > 0 ? __p : NULL, type, module, LEVEL_VERBOSE);        \
                } while (0)
#else
#define MESH_DBG_BUFFER_LEVEL_VERBOSE(type, module, fmt, ...)
#endif

#define MESH_DBG_BUFFER(type, module, level, fmt, ...)   \
           MESH_DBG_BUFFER_##level(type, module, fmt, ##__VA_ARGS__)


#define LOG_ERR(fmt, ...)   \
        MESH_DBG_BUFFER(TRACE_TYPE_BLE, MODULE_MESH, LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define LOG_WRN(fmt, ...)   \
        MESH_DBG_BUFFER(TRACE_TYPE_BLE, MODULE_MESH, LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOG_INF(fmt, ...)   \
        MESH_DBG_BUFFER(TRACE_TYPE_BLE, MODULE_MESH, LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_DBG(fmt, ...)   \
        MESH_DBG_BUFFER(TRACE_TYPE_BLE, MODULE_MESH, LEVEL_TRACE, fmt, ##__VA_ARGS__)
#define LOG_DUMP(fmt, ...)   \
        MESH_DBG_BUFFER(TRACE_TYPE_BLE, MODULE_MESH, LEVEL_VERBOSE, fmt, ##__VA_ARGS__)

#define LOG_HEXDUMP_DBG(x, y, z)

#define NET_BUF_SIMPLE_DBG(fmt, ...)
#endif

#define _DO_CONCAT(x, y) x ## y ## _T
#define _CONCAT(x, y) _DO_CONCAT(x, y)

#define BUILD_ASSERT(EXPR, MSG...) \
	  enum _CONCAT(__build_assert_enum, __LINE__) { \
		_CONCAT(__build_assert_enum, __LINE__) = 1 / !!(EXPR) \
	}

#define __ASSERT_NO_MSG(cond)                          \
  do {                                              \
      if (!(cond)) {                                \
          co_printf("ASSERT '" #cond "' %s", __FUNCTION__);     \
          GLOBAL_INT_STOP();                        \
          while(1);                                 \
      }       \
  } while(0)

#define __ASSERT(cond, res, ...)                         \
  do {                                              \
      if (!(cond)) {                                \
          co_printf("ASSERT: "res, ##__VA_ARGS__);                 \
          GLOBAL_INT_STOP();                        \
          while(1);                                  \
      }                                             \
  } while(0)

#if defined(CONFIG_ASSERT_ON_ERRORS)
#define CHECKIF(expr) \
	__ASSERT_NO_MSG(!(expr));   \
	if (0)
#elif defined(CONFIG_NO_RUNTIME_CHECKS)
#define CHECKIF(...) \
	if (0)
#else
#define CHECKIF(expr) \
	if (expr)
#endif


void mesh_log_init(void);

void mesh_log_set_dbg_level(uint16_t mask, uint8_t level);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* _MESH_BLUETOOTH_LOG_H_ */
