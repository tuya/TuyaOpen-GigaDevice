/*!
    \file    net_iperf.h
    \brief   definitions and structures for iperf test.

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

#ifndef __NET_IPERF_H__
#define __NET_IPERF_H__

#include "co_int.h"
#include "co_bool.h"
#include "lwip/sockets.h"
#include "wrapper_os.h"
#include "util.h"
#include "systime.h"

/*
 * DEFINITIONS
 ****************************************************************************************
 */
/// Maximum number of iperf streams
#define IPERF_MAX_STREAMS               2

/// UDP Rate
#define IPERF_DEFAULT_UDPRATE           1024 * 1024     // 1 Mbit/sec (-u)
/// UDP buffer length
#define IPERF_DEFAULT_UDPBUFLEN         1472            // read/write 1472 bytes (-u)
/// Number of IPERF send buffers (credits)
#define IPERF_SEND_BUF_CNT              32

/// Type of traffic generation
enum iperf_test_mode
{
    /// Unidirectional test
    IPERF_TEST_NORMAL = 0,
    /// Bidirectional test simultaneously
    IPERF_TEST_DUALTEST,
    /// Unkwnow test mode
    IPERF_TEST_UNKNOWN,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Datagram for UDP packets
struct iperf_UDP_datagram
{
    /// Datagram ID
    int32_t id;
    /// Sending time (seconds)
    uint32_t sec;
    /// Sending time (microseconds)
    uint32_t usec;
};

/// Iperf timer
struct iperf_time
{
    /// Second
    uint32_t sec;
    /// Microsecond
    uint32_t usec;
};

/// Iperf configuration flags
struct iperf_flags
{
    /// Command option: buffer len option set (-l)
    bool is_buf_len_set : 1;
    /// Command option: udp mode enable (-u)
    bool is_udp         : 1;
    /// Command option: time mode option set (-t)
    bool is_time_mode   : 1;
    /// Command option: bandwidth option set (-b)
    bool is_bw_set      : 1;
    /// Command option: server mode enable (-s)
    bool is_server      : 1;
    /// Command option: peer version detect option set(-X)
    bool is_peer_ver    : 1;
    /// Command option: Show interval stats (-i)
    bool show_int_stats : 1;
};

/// Iperf configuration
struct iperf_settings_t
{
    /// Iperf server IP (-c)
#ifdef CONFIG_IPV6_SUPPORT
    ip_addr_t host_ip;
#else
    uint32_t host_ip;
#endif
    /// Iperf UDP buffer length (-l)
    uint32_t buf_len;
    /// Iperf test mode
    enum iperf_test_mode test_mode;
    /// Iperf printing format (-f):
    /// Possible values are: 'a', 'A', 'b', 'B', 'k', 'K', 'g', 'G'
    char format;
    /// Iperf TCP client listen port (-L)
    uint16_t listen_port;
    /// Iperf server port (-p)
    uint16_t port;
    /// IP type-of-service (-S)
    uint16_t tos;
    /// Time to live (-T)
    uint16_t ttl;
    /// Iperf udp rate (-b)
    uint64_t udprate;
    /// Iperf amount of data to send in bytes or test duration in 0.1s
    uint64_t amount;
    /// Iperf interval
    struct iperf_time interval;
    /// Setting flags
    struct iperf_flags flags;
};

/// Iperf statistics
struct iperf_stats
{
    /// Transferred bytes
    uint64_t bytes;
    /// Number of transferred datagrams
    uint32_t nb_datagrams;
    /// Number of errors
    uint32_t nb_error;
    /// Number of out of order datagrams
    uint32_t nb_out_of_orded;
    /// Jitter in microseconds
    uint32_t jitter_us;
};

/// Report for TCP and UDP client/server
struct iperf_report
{
    /// Packet ID
    int32_t packet_id;
    /// Current statistics
    struct iperf_stats stats;
    /// Statistics at the last interval
    struct iperf_stats last_stats;
    /// Timestamp of the last sent/received packet
    struct iperf_time packet_time;
    /// Timestamp of the first packet
    struct iperf_time start_time;
    /// End of reception/transmission time
    struct iperf_time end_time;
    /// Sending time included in the received UDP datagram
    struct iperf_time sent_time;
    /// Transit time (RX time - TX time) of last received UDP datagram
    struct iperf_time last_transit;
    /// Last interval time
    struct iperf_time last_interval;
    /// Target interval timestamp
    struct iperf_time interval_target;
    /// UDP client IP address
    ip_addr_t addr;
    /// UDP client port
    u16_t port;
};

/// Iperf stream related info
struct net_iperf_stream
{
    /// Ping thread ID
    uint32_t id;
    /// State of ping thread (true for active, false for inactive)
    bool active;
    /// Iperf settings
    struct iperf_settings_t iperf_settings;
    /// Handle of iperf send task
    os_task_t iperf_handle;
    /// Iperf semaphore used to wake up the iperf thread to close the task
    os_sema_t iperf_task_semaphore;
    /// Semaphore used to protect Iperf buffer pool
    os_sema_t send_buf_semaphore;
    /// Iperf timeout semaphore
    os_sema_t to_semaphore;
    /// Iperf mutex used when we modify the credits for sending process
    os_mutex_t iperf_mutex;
    /// Param pointer for NET IPERF AL
    void *arg;
    /// TCP/UDP report
    struct iperf_report report;
};

/// Table of iperf streams
extern struct net_iperf_stream streams[IPERF_MAX_STREAMS];

/* Macros for timers */

/// Current time
#define iperf_current_time(a) \
    get_time(SINCE_BOOT, &(a)->sec, &(a)->usec);

/// Timer addition
#define iperf_timeradd(a, b, result)        \
    (result)->sec = (a)->sec + (b)->sec;    \
    (result)->usec = (a)->usec + (b)->usec; \
    while ((result)->usec > 1000000)        \
    {                                       \
        (result)->usec -= 1000000;          \
        (result)->sec++;                    \
    }

/// Timer subtraction
#define iperf_timersub(a, b, result)         \
    do                                       \
    {                                        \
        int32_t usec;                        \
        (result)->sec = (a)->sec - (b)->sec; \
        usec = (a)->usec - (b)->usec;        \
        (result)->usec = usec;               \
        if (usec < 0)                        \
        {                                    \
            --(result)->sec;                 \
            (result)->usec = usec + 1000000; \
        }                                    \
    } while (0)

/// Check if timer a is before timer b
#define iperf_timerbefore(a, b) \
    (((a)->sec < (b)->sec) ||   \
     ((a)->sec == (b)->sec &&   \
      (a)->usec < (b)->usec))

/// Check if timer a is after timer b
#define iperf_timerafter(a, b) iperf_timerbefore(b, a)

/// Extract timer milliseconds
#define iperf_timermsec(a) (((a)->sec * 1000) + ((a)->usec + 500) / 1000)

/// Extract timer microseconds
#define iperf_timerusec(a) (((a)->sec * 1000000ULL) + (a)->usec)

/*
 * GLOBAL VARIABLES
 ****************************************************************************************
 */
/// Iperf help string
extern const char iperf_long_help[];

/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize iperf settings
 *
 * @param[in] iperf_settings  Iperf settings to initialize
 ****************************************************************************************
 **/
void iperf_settings_init(struct iperf_settings_t *iperf_settings);

/**
 ****************************************************************************************
 * @brief Start iperf command with certain configuration options
 * This function creates the RTOS task dedicated to the iperf command.
 *
 * @param[in] args  Iperf configuration
 *
 * @return task handle of the created task
 ****************************************************************************************
 */
os_task_t iperf_start(struct iperf_settings_t *iperf_settings);

/**
 ****************************************************************************************
 * @brief Initialize Iperf Statistics.
 *
 * To be called before each test.
 * This initialize statistics and other test variables (start_time, interval_time ...)
 *
 * @param[in] stream  Iperf stream
 ****************************************************************************************
 **/
void iperf_init_stats(struct net_iperf_stream *stream);

/**
 ****************************************************************************************
 * @brief Print interval statistics
 *
 * Does nothing if interval statistics are not enabled or this is not the time to print
 * them. 'stream->report.packet_time' is used to test the current time so it has to be
 * set by the caller.
 *
 * @param[in] stream  Iperf stream
 ****************************************************************************************
 **/
void iperf_print_interv_stats(struct net_iperf_stream *stream);

/**
 ****************************************************************************************
 * @brief Print iperf statistics for a given interval.
 *
 * Statistics format is different for TCP/UDP Server and Client.
 *
 * @param[in] stream      Iperf stream
 * @param[in] start_time  Global start time
 * @param[in] end_time    Global end time
 * @param[in] stats       Statistics for this period
 ****************************************************************************************
 **/
void iperf_print_stats(const struct net_iperf_stream *stream,
                             struct iperf_time *start_time,
                             struct iperf_time *end_time,
                             const struct iperf_stats *stats);

/**
 ****************************************************************************************
 * @brief Stop all iperf streams.
 ****************************************************************************************
 **/
void iperf_stop_all(void);

#endif /* __NET_IPERF_H__ */
/// @}
