/*!
    \file    trace_ext.h
    \brief   Declaration for trace buffer.

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

#ifndef _TRACE_EXT_H_
#define _TRACE_EXT_H_

#include "util_config.h"
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#define NEW_TRACE_USED

#ifdef NEW_TRACE_USED
/* Log Section Definition */
#define TRACE_DATA __attribute__((section(".GDTRACE"))) __attribute__((aligned(4))) __attribute__((used))
#endif

#define TRACE_TYPE_BLE        0x01
#define TRACE_TYPE_BTSNOOP    0x02
#define TRACE_TYPE_WIFI       0x03
#define TRACE_TYPE_CONSOLE    0x04
#define TRACE_TYPE_NEW        0x08

/**
 * \name    TRACE_LEVEL
 * \brief   Log Level Definition.
 * \anchor  TRACE_LEVEL
 */

#define LEVEL_OFF       (-1)
#define LEVEL_ERROR     (0)
#define LEVEL_WARN      (1)
#define LEVEL_INFO      (2)
#define LEVEL_TRACE     (3)
#define LEVEL_VERBOSE   (4)
#define LEVEL_NUM       (5)

/*
 * DBG_LEVEL is used to control the log printed by DBG_BUFFER(), see log.h.
 * LEVEL_OFF   : Print None
 * LEVEL_ERROR : Print ERROR
 * LEVEL_WARN  : Print ERROR, WARN
 * LEVEL_INFO  : Print ERROR, WARN, INFO
 * LEVEL_TRACE : Print ERROR, WARN, INFO, TRACE
 */
#if defined CFG_GD_TRACE_EXT
#define DBG_LEVEL               LEVEL_TRACE
#else
#define DBG_LEVEL               LEVEL_ERROR
#endif

/**
 * \name    MODULE_ID
 * \brief   Module ID definition.
 * \anchor  MODULE_ID
 */

typedef enum
{
    /* stack modules */
    MODULE_APP                      = 30,
    MODULE_PROF                     = 31,
    MODULE_GAP                      = 32,
    MODULE_GATT                     = 33,
    MODULE_SMP                      = 34,
    MODULE_L2CAP                    = 35,
    MODULE_HCI                      = 36,
    MODULE_LLM                      = 37,
    MODULE_LLM_SCAN                 = 38,
    MODULE_LLM_ADV                  = 39,
    MODULE_LLC                      = 40,
    MODULE_LLI                      = 41,
    MODULE_LLI_BI                   = 42,
    MODULE_LLI_CI                   = 43,
    MODULE_LLD                      = 44,
    MODULE_LLD_ADV                  = 45,
    MODULE_LLD_SCAN                 = 46,
    MODULE_LLD_INIT                 = 47,
    MODULE_LLD_CONN                 = 48,
    MODULE_LLD_SYNC                 = 49,
    MODULE_LLD_BI                   = 50,
    MODULE_LLD_CI                   = 51,
    MODULE_LLD_ISOAL                = 52,
    MODULE_SCH                      = 53,
    MODULE_KE                       = 54,
    MODULE_IP                       = 55,
    MODULE_AES                      = 56,

    MODULE_EVT_TS                   = 63,
    MODULE_NUM                      = 64
} T_MODULE_ID;


/**
 ******************************************************************************
 * @addtogroup TRACE
 * @details
 *
 * @b Usage: \n
 * You can add:
 * - an unfiltered trace point using @ref TRACE macro for printf like trace and
 * @ref TRACE_BUF to trace a buffer.
 * - a filterable trace point (i.e. that can be disabled/enabled at run-time)
 * using the @ref TRACE_FILT or @ref TRACE_BUF_FILT macros.
 *
 * Unfiltered trace points are always traced.\n
 * A filterable trace point is traced if the bit of the trace point level is
 * set in the current filter for the component. See trace_level_<compo> for
 * trace level specific to a given component.\n
 *
 * You can then use @ref trace_set_filter to change the current filter of a
 * given component. As @ref trace_compo_level is stored in shared_memory Host
 * can also directly modify its content.
 * @{
 ******************************************************************************
 */

void trace_dma_transfer_cmplt();

/**
 ******************************************************************************
 * @brief Initialize trace buffer.
 *
 * Normal usage would be to initialize trace buffer at boot with:
 * - @p force set to false, so that if fw jumps back to boot code (e.g. by
 *      dereferencing invalid pointer), it will appear in the trace.
 * - @p loop set to true to keep the latest trace.
 *
 * In some specific debug cases, it may be useful to start the trace after a
 * specific event and to not use a circular buffer so that it ensure that trace
 * that happened just after that specific event is kept. In this case set @p
 * force to true and @p loop to false.
 *
 * @param[in] force Force buffer initialization even if trace buffer has
 *                  already been initialized.
 * @param[in] loop  Whether trace buffer must be used circular buffer or not
 ******************************************************************************
 */
void trace_ext_init(bool force, bool loop);

/**
 ******************************************************************************
 * @brief Add a trace point in the buffer for ble
 *
 * This function must NOT be called directly. use @ref TRACE, @ref TRACE_BUF
 * or any of the TRACE_<LVL> macro instead.
 *
 * @param[in] id        Unique trace id
 * @param[in] nb_param  Number of parameters to trace
 * @param[in] param     Table of parameters
 * @param[in] trace_buf boolean indicating if TRACE_BUF macro is used. \n
 *                      if true, param = [buffer size, buffer address(16 LSB),
 *                                        buffer address(16 MSB)]
 ******************************************************************************
 */
void trace_ble(uint32_t id, uint16_t nb_param, uint16_t *param, bool trace_buf);

/**
******************************************************************************
 * @brief File ID offset in the 24bits trace ID.
 *
 * TRACE_FILE_ID_SIZE is passed on the compilation line so that its value can
 * easily be shared with the trace dictionary generator.
 * Its value is the same for all files.
 ******************************************************************************
 */
#define TRACE_FILE_ID_SIZE  9
#define TRACE_FILE_ID_OFT (24 - TRACE_FILE_ID_SIZE)

/**
 ******************************************************************************
 * @brief Unique Trace ID for current file/line
 *
 * TRACE_FILE_ID is passed on the compilation line and is different for each file.
 ******************************************************************************
 */
#define TRACE_ID ((TRACE_FILE_ID << TRACE_FILE_ID_OFT) + __LINE__)

/**
 ******************************************************************************
 * @brief Macro used to add a unfiltered trace point in the trace buffer
 *
 * It should be called with a printf like prototype, and will call @ref trace
 * function with a unique trace id (@ref TRACE_ID) and the parameters correctly
 * formatted.
 *
 * Current format string supported by the decoder are:
 * - For character :\n
 * use format \%c and pass directly the character as parameter
 * @verbatim TRACE("first char is %c", string[0]) @endverbatim
 *
 * - For 16bits value :\n
 * Use one of the following format \%d, \%u, \%x, \%X and pass directly the
 * value as parameter.
 * @verbatim TRACE("this is a 16bit value %d", myval) @endverbatim
 * It is also possible to add formatting option
 * @verbatim TRACE("this is a 16bit value 0x%04x", myval) @endverbatim
 *
 * - For 32bits value :\n
 * Use one of the following format \%ld, \%lu, \%lx, \%lX and pass the value
 * using @ref TR_32 macro.
 * @verbatim TRACE("this is a 32bit value 0x%08lx", TR_32(myval)) @endverbatim
 *
 * - For a string :\n
 * Use format \%s and pass string using @ref TR_STR_8 macro.
 * @verbatim TRACE("SSID = %s", TR_STR_8(param->ssid)) @endverbatim
 * @note As implied by the name, string are currently limited to 8 characters.
 *
 * - For a pointer :\n
 * Use format \%p and pass the value using @ref TR_PTR macro.
 * @verbatim TRACE("buffer address is %p", TR_PTR(buffer)) @endverbatim
 * @note \%p is actually just a shortcut for \%08lx
 *
 * - For a MAC address :\n
 * Use format \%pM and pass the value using @ref TR_MAC macro.
 * @verbatim TRACE("vif mac = %pM", TR_MAC(mac_addr)) @endverbatim
 *
 * - For a file name : \n
 * Use format \%F and pass TRACE_FILE_ID as parameter
 * @verbatim TRACE("Error in file %F", TRACE_FILE_ID) @endverbatim
 *
 * - For @ref ke_msg_id_t :\n
 * Use format \%kM and pass the message id directly. Using this format it just
 * like a simple 16bits value, but it also indicates the decoder to translate
 * the message id into its real name (if found inside the dictionary).
 * @verbatim TRACE("msg received is receive [%kM]", msg_id)
   => msg received is [MM_VERSION_REQ] @endverbatim
 *
 * - For @ref ke_task_id_t :\n
 * Use format \%kT and pass the task id directly. Using this format it just
 * like a simple 16bits value, but it also indicates the decoder to translate
 * the task id into its real name (if found inside the dictionary) and index.
 * @verbatim TRACE("send message to task [%kT]", msg_id)
   => send message to task [ME(0)] @endverbatim
 *
 * - For @ref ke_state_t :\n
 * Use format \%kS and pass \b both the task id and the state index. Using this
 * format is like tracing 2 16bits value, but it also indicates the decoder to
 * translate these values into a task name and a state name. (task id is needed
 * because state enum is specific to each task).
 * @verbatim TRACE("set [%kS]", task_id, state)
   => set SCANU(0):SCANU_IDLE @endverbatim
 *
 * - For a timestamp :\n
 * Use format \%t and pass the timestamp value as a 32bits value in microsecond
 * (same format as returned by @ref hal_machw_time).
 * The decoder will format the timestamp value like the timestamp of the trace
 * (i.e. \<s>.\<ms>_\<us>) and will show the delta between the timestamp parameter
 * and the timestamp of the trace.
 * @verbatim TRACE("Timer set at %t", TR_32(timer_date))
   => Timer set at @23.234_789 (+25.35 ms) @endverbatim
 *
 * - For a Frame Control :\n
 * Use format \%fc and pass the value as a 16bits value.
 * The decoder will parse this value as a frame control field (as found in MAC header)
 * @verbatim TRACE("Received frame: %fc", mac_hdr->fctl)
   => Received frame: Association response @endverbatim
 *
 * - For a IPv4 address :\n
 * Use format \%pI4 and pass the value using @ref TR_IP4 macro.
 * @verbatim TRACE("ip = %pI4", TR_IP4(ip4_addr)) @endverbatim
 *
 * - For a IPv6 address :\n
 * Use format \%pI6 or \%pI6c and pass the value using @ref TR_IP6 macro.
 * @verbatim TRACE("ipv6 = %pI6", TR_IP6(ip6_addr)) @endverbatim
 *
 ******************************************************************************
 */
#define GD_TRACE(a,...) {                                                  \
        uint16_t __p[] = {__VA_ARGS__} ;                               \
        trace_ble(TRACE_ID, sizeof(__p)/sizeof(__p[0]), sizeof(__p) > 0 ? __p : NULL, false);        \
    }

/**
 ******************************************************************************
 * @brief Macro used to add an unfiltered trace containing a dump of a buffer in
 * the trace buffer
 *
 * It will call @ref trace function with a unique trace id (@ref TRACE_ID) and
 * the parameters correctly formatted.
 *
 * The format string supported by the decoder is:\n
 *  <b> %pB(F|s)?(number_of_characters)?(word_size)?(representation)? </b>
 *
 * - number_of_characters :\n
 * optional group indicating the number of words to print in one line.\n
 * Default value : 8\n
 * @verbatim TRACE_BUF("my_buf: %pB4", my_buf_len, my_buf);
   => my_buf: 00 10 00 20
              40 FF 00 1A @endverbatim
 *
 * - word_size :\n
 * Optional group indication the size of a word.\n
 * Default value : b\n
 * Possible values: \n
 *                  -# b: byte
 *                  -# h: half-word(16 bits)
 *                  -# w: word(32 bits)\n
 * @verbatim TRACE_BUF("my_buf: %pB4h", my_buf_len, my_buf);
   => my_buf: 0010 0020 40FF 001A@endverbatim

 * - representation :\n
 * Optional group indicating the print format of a word\n
 * Default value: x\n
 * Possible values: \n
 *                  -# d: decimal
 *                  -# x: hexadecimal
 *                  -# c: char\n
 * @verbatim TRACE_BUF("my_buf: %pB4hd", my_buf_len, my_buf);
   => my_buf: 16 32 16639 26@endverbatim
 *
 * - 802.11 Frame: \n
 * If format \%pBF is used (instead of only \%pB), then the decoder will treat the buffer
 * as a 802.11 frame. It will then parse the MAC header and dump the rest of the frame
 * just like any other buffer using number_of_characters/word_size/representation
 * parameters if set.
 * @verbatim TRACE_BUF("received frame: %pBF12bx", my_buf_len, my_buf);
   => received frame: Authentication Dur=60us Seq=1322 frag=0
                      A1=aa:06:cc:dd:ee:fe A2=c4:04:15:3d:41:e8 A3=c4:04:15:3d:41:e8
                      00 00 02 00 00 00 dd 09 00 10 18 02
                      00 00 1c 00 00  @endverbatim
 *
 * - String buffer: \n
 * If the format \%pBs is used (instead of only \%pB), then the decoder will treat the
 * buffer as a string buffer. The decoder will stop buffer processing on first null
 * byte (even if there is more data dumped). number_of_characters can be set to limit
 * the size of the decoder output per line. If the limit is reached the decoder acts as
 * if a '\n' character was present in the dump.
 * If set word_size and representation are ignored.
 * @verbatim TRACE_BUF("command buffer: %pBs", my_buf_len, my_buf);
   => command buffer: This is a string split
                      into 2 lines @endverbatim
 *
 * @verbatim TRACE_BUF("command buffer: %pBs8", my_buf_len, my_buf);
   => command buffer: This is
                      a string
                       split
                      into 2 l
                      ines @endverbatim
 *
 *
 *
 * @param[in] format    string specifying how to interpret the data
 * @param[in] size      size of the buffer in bytes (independent from
 *                      number_of_characters in the conversion specification)
 * @param[in] buf       buffer to dump
 ******************************************************************************
 */
#ifdef NEW_TRACE_USED
#define TRACE_BLE_BUF(fmt, size, buf) {                                  \
        static const char format[] TRACE_DATA = fmt; \
        uint16_t __p[] = {(uint16_t)(size), (uint16_t)((uint32_t)buf),  \
                                  (uint16_t)(((uint32_t)buf) >> 16)};           \
        uint16_t __size = 1 + ((size + 1 + ((uint32_t)buf & 0x1)) >> 1);   \
        trace_ble((uint32_t)format, __size, __p, true);                             \
    }
#else
#define TRACE_BLE_BUF(format, size, buf) {                                  \
        uint16_t __p[] = {(uint16_t)(size), (uint16_t)((uint32_t)buf),  \
                          (uint16_t)(((uint32_t)buf) >> 16)};           \
        uint16_t __size = 1 + ((size + 1 + ((uint32_t)buf & 0x1)) >> 1);   \
        trace_ble(TRACE_ID, __size, __p, true);                             \
    }
#endif
/**
 ******************************************************************************
 * @brief Macro to trace a 32bits parameters
 *
 * To be used with \%ld, \%lu, \%lx, \%lX format
 * @param[in] a A 32bit value
 ******************************************************************************
 */
#define TR_32(a) (uint16_t)((uint32_t)(a) >> 16), (uint16_t)((uint32_t)(a))

/**
 ******************************************************************************
 * @brief Macro to trace a 64bits parameters
 *
 * To be used with \%lld, \%llu, \%llx, \%llX format
 * @param[in] a A 64bit value
 ******************************************************************************
 */
#define TR_64(a) TR_32(((uint64_t)(a) >> 32)), TR_32(a)

/**
 ******************************************************************************
 * @brief Macro to trace a pointer
 *
 * To be used with \%p format
 * @param[in] p A 32bit address
 ******************************************************************************
 */
#define TR_PTR(p) TR_32(p)

/**
 ******************************************************************************
 * @brief Macro to trace a MAC address
 *
 * To be used with \%pM format
 * @param[in] m Address on a MAC address
 ******************************************************************************
 */
#define TR_MAC(m) ((uint16_t *)(m))[0], ((uint16_t *)(m))[1], ((uint16_t *)(m))[2]

/**
 ******************************************************************************
 * @brief Macro to trace a IPv4 address
 *
 * To be used with \%pI4 format
 * @param[in] m Address on an IPv4 address (Not the ip address as 32bit value)
 ******************************************************************************
 */
#define TR_IP4(m) TR_32(*((uint32_t *)m))

/**
 ******************************************************************************
 * @brief Macro to trace a IPv6 address
 *
 * To be used with \%pI6, \%ipI6c format
 * @param[in] m Address on an IPv6 address
 ******************************************************************************
 */
#define TR_IP6(m) ((uint16_t *)(m))[0], ((uint16_t *)(m))[1], ((uint16_t *)(m))[2] \
        ((uint16_t *)(m))[3], ((uint16_t *)(m))[4], ((uint16_t *)(m))[5] \
        ((uint16_t *)(m))[6], ((uint16_t *)(m))[7]

// internal
#ifdef CFG_RWTL
#define _PTR_ALIGN(p) ((uint16_t *) p)
/**
 ******************************************************************************
 * @brief Macro to trace a string
 *
 * To be used with \%s format. Trace only the 8 first characters of the string
 * @note Using this macro will always store 8bytes in the trace buffer, and
 * decoder will stop as soon as it read 8 characters or '\0' is found.
 ******************************************************************************
 */
#define TR_STR_8(s) (0x0800) ,  \
        (_PTR_ALIGN(s)[0] & 0xFF) + ((_PTR_ALIGN(s)[1] & 0xFF) << 8),   \
        (_PTR_ALIGN(s)[2] & 0xFF) + ((_PTR_ALIGN(s)[3] & 0xFF) << 8),   \
        (_PTR_ALIGN(s)[4] & 0xFF) + ((_PTR_ALIGN(s)[5] & 0xFF) << 8),   \
        (_PTR_ALIGN(s)[6] & 0xFF) + ((_PTR_ALIGN(s)[7] & 0xFF) << 8)


#else
#define _PTR_ALIGN(p) ((uint16_t *)((uint32_t)p & ~0x1))
/**
 ******************************************************************************
 * @brief Macro to trace a string
 *
 * To be used with \%s format. Trace only the 8 first characters of the string
 * @note Using this macro will always store 8bytes in the trace buffer, and
 * decoder will stop as soon as it read 8 characters or '\0' is found.
 ******************************************************************************
 */
#define TR_STR_8(s) (0x0800 + (uint16_t)((uint32_t)(s) & 0x1)) ,  \
        _PTR_ALIGN(s)[0], _PTR_ALIGN(s)[1],                       \
        _PTR_ALIGN(s)[2], _PTR_ALIGN(s)[3]

#endif


//ble log api
bool trace_ble_filter_set(uint8_t module, uint8_t trace_mask);
void trace_ble_log(uint32_t id, uint16_t nb_param, uint16_t *param, uint8_t type, uint8_t module, uint8_t trace_level);
void trace_btsnoop(uint8_t *p_buf, uint16_t len, uint8_t direction, uint8_t hci_type);
void trace_btsnoop_payload(uint8_t *p_buf, uint16_t len, uint8_t direction, uint8_t hci_type, uint8_t *p_payload, uint16_t payload_len);

#if (DBG_LEVEL >= LEVEL_ERROR)
#define TRACE_BTSNOOP(p_buf, len, direction, hci_type) {      \
        trace_btsnoop(p_buf, len, direction, hci_type);        \
    }

#define TRACE_BTSNOOP_PAYLOAD(p_buf, len, direction, hci_type, p_payload, payload_len) {      \
        trace_btsnoop_payload(p_buf, len, direction, hci_type, p_payload, payload_len);        \
    }
#else
#define TRACE_BTSNOOP(p_buf, len, direction, hci_type)
#define TRACE_BTSNOOP_PAYLOAD(p_buf, len, direction, hci_type, p_payload, payload_len)
#endif
//wifi trace api
bool trace_wifi_filter_set(void);
/**
 ******************************************************************************
 * @brief Add a trace point in the buffer for wifi
 *
 * This function must NOT be called directly. use @ref TRACE, @ref TRACE_BUF
 * or any of the TRACE_<LVL> macro instead.
 *
 * @param[in] id        Unique trace id
 * @param[in] nb_param  Number of parameters to trace
 * @param[in] param     Table of parameters
 * @param[in] trace_buf boolean indicating if TRACE_BUF macro is used. \n
 *                      if true, param = [buffer size, buffer address(16 LSB),
 *                                        buffer address(16 MSB)]
 ******************************************************************************
 */
void trace_wifi(uint32_t id, uint16_t nb_param, uint16_t *param, bool trace_buf);

void trace_console(uint16_t length, uint8_t *p_buf);
uint16_t trace_count(void);
uint16_t trace_print(uint16_t max_bytes);

void trace_dma_print(void);

#endif /* _TRACE_EXT_H_ */
