/*!
    \file    iperf.c
    \brief   iperf test for GD32VW55x SDK

    \version 2023-5-20, V1.0.0, firmware for GD32VW55x
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

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "net_iperf.h"
#include "net_iperf_al.h"
#include "dbg_print.h"
#include "cmd_shell.h"
#include "tcpip.h"

#ifdef CONFIG_IPERF_TEST
/*
 * DEFINITIONS
 ****************************************************************************************
 */
/// Default Port
#define IPERF_DEFAULT_PORT              5001
/// Default test duration (in 0.1 seconds)
#define IPERF_DEFAULT_TIME_AMOUNT       10
/// Default buffer length (in bytes)
#define IPERF_DEFAULT_BUFFER_LEN        8 * 1024

/*
 * GLOBAL VARIABLES
 ****************************************************************************************
 */

/// Unit of measurement of data
enum data_type
{
    CONV_UNIT,
    CONV_KILO,
    CONV_MEGA,
    CONV_GIGA,
};

/// Unit of measurements for bytes
const char *net_iperf_byte_lbl[] =
{
    [CONV_UNIT] = "Byte",
    [CONV_KILO] = "KByte",
    [CONV_MEGA] = "MByte",
    [CONV_GIGA] = "GByte",
};

/// Unit of measurements for bit
const char *net_iperf_bit_lbl[] =
{
    [CONV_UNIT] = "bit",
    [CONV_KILO] = "Kbits",
    [CONV_MEGA] = "Mbits",
    [CONV_GIGA] = "Gbits",
};

#if 0
/// Long version of iperf helf
const char iperf_long_help[] =
    "\
Client/Server:\n\
  -i, --interval  #        seconds between periodic bandwidth reports\n\
  -l, --len       #[KM]    length of buffer to read or write (default 8 KB)\n\
  -p, --port      #        server port to listen on/connect to\n\
  -u, --udp                use UDP rather than TCP\n\n\
Client specific:\n\
  -b, --bandwidth #[KM]    for UDP, bandwidth to send at in bits/sec\n\
                         (default 1 Mbit/sec, implies -u)\n\
  -c, --client    <host>   run in client mode, connecting to <host>\n\
  -t, --time      #        time in seconds to transmit for (default 10 secs)\n\
  -S              #        type-of-service for outgoing packets.\n\
                           You may specify the value in hex with a '0x' prefix, in octal with a '0' prefix, or in decimal\n\
  -T              #        time-to-live for outgoing multicast packets\n\
  -w              #[KM]    TCP window size (ignored for now)\n\
Server specific:\n\
  -s, --server             run in server mode";
#endif

/// Table of iperf streams
struct net_iperf_stream streams[IPERF_MAX_STREAMS] = {0};

/*
 * FUNCTIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief IPERF task implementation.
 ****************************************************************************************
 */
static void iperf_main(void *param)
{
    struct net_iperf_stream *iperf_stream = param;
    struct iperf_settings_t *iperf_settings = &(iperf_stream->iperf_settings);

    iperf_stream->active = true;
    app_print("iperf: create stream [%d]\r\n", iperf_stream->id);

    if (net_iperf_buf_init(iperf_stream))
    {
        app_print("IPERF: Failed to alloc iperf buffer\r\n");
        goto end;
    }

    if (!iperf_settings->flags.is_server)
    {
        if (iperf_settings->flags.is_udp)
        {
            // UDP Client
            if (net_iperf_udp_client_run(iperf_stream))
            {
                app_print("IPERF: Failed to start UDP client\r\n");
                goto end;
            }
        }
        else
        {
            // TCP Client
            if (net_iperf_tcp_client_run(iperf_stream))
            {
                app_print("IPERF: Failed to start TCP client\r\n");
                goto end;
            }
        }
    }
    else
    {
        if (iperf_settings->flags.is_udp)
        {
            // UDP Server
            if (net_iperf_udp_server_run(iperf_stream))
            {
                app_print("IPERF : Failed to start UDP server\r\n");
                goto end;
            }
        }
        else
        {
            // TCP Server
            if (net_iperf_tcp_server_run(iperf_stream))
            {
                app_print("IPERF: Failed to start TCP server\r\n");
                goto end;
            }
        }
    }

  end:
    // Delete objects
    sys_sema_free(&iperf_stream->to_semaphore);
    sys_sema_free(&iperf_stream->iperf_task_semaphore);
    sys_sema_free(&iperf_stream->send_buf_semaphore);
    sys_mutex_free(&iperf_stream->iperf_mutex);
    iperf_stream->active = false;
    net_iperf_buf_deinit(iperf_stream);
    sys_task_delete(NULL);
}


/**
 ****************************************************************************************
 * @brief Search the stream id of a free iperf stream
 * Return the first free stream id in the table.
 *
 * @return iperf stream id, -1 if error
 ****************************************************************************************
 **/
static int iperf_find_free_stream_id(void)
{
    int stream_id;
    for (stream_id = 0; stream_id < IPERF_MAX_STREAMS; stream_id++)
    {
        if (streams[stream_id].active == false)
        {
            return stream_id;
        }
    }
    return -1;
}

/**
 ****************************************************************************************
 * @brief Convert a float value into string according to the specified format
 *
 * For example:
 * - amount = 12000 format = 'K' => out_str = "11.72 KByte"
 * - amount = 12000 format = 'k' => out_str = "96 Kbits"
 *
 * @param[out] out_str              Pointer to the string buffer to write
 * @param[in] len                   Length of out_str buffer
 * @param[in] amount                Float value to convert in string based on the format
 * @param[in] format                Format to use for converting amount to string
 ****************************************************************************************
 **/
static void iperf_snprintf(char *out_str, uint32_t len, float amount, char format)
{
    uint8_t conv = CONV_UNIT;
    const char *suffix;
    int int_amount, dec_amount;
    bool is_bit_value = false;
    float round;

    switch (format)
    {
        case 'b':
            is_bit_value = true;
        case 'B':
            conv = CONV_UNIT;
            break;
        case 'k':
            is_bit_value = true;
        case 'K':
            conv = CONV_KILO;
            break;
        case 'm':
            is_bit_value = true;
        case 'M':
            conv = CONV_MEGA;
            break;
        case 'g':
            is_bit_value = true;
        case 'G':
            conv = CONV_GIGA;
            break;

        default:
        case 'a':
            is_bit_value = true;
            for (float tmp_amount = amount * 8;
                 tmp_amount >= 1000.0 && conv <= CONV_GIGA;
                 conv++)
            {
                tmp_amount /= 1000.0;
            }
            break;
        case 'A':
            for (float tmp_amount = amount;
                 tmp_amount >= 1024.0 && conv <= CONV_GIGA;
                 conv++)
            {
                tmp_amount /= 1024.0;
            }
            break;
    }

    if (is_bit_value)
    {
        amount *= 8;
        suffix = net_iperf_bit_lbl[conv];
        while (conv-- > 0)
            amount /= 1000;
    }
    else
    {
        suffix = net_iperf_byte_lbl[conv];
        while (conv-- > 0)
            amount /= 1024;
    }

    int_amount = (int32_t)amount;
    if (amount < (float)9.995)
    {
        round = (float)0.005;
        amount += round;
        int_amount = (int32_t)amount;
        dec_amount = (amount * 100) - (int_amount * 100);
        dbg_snprintf((char *)out_str, len, "%d.%02d %s", int_amount, dec_amount, suffix);
    }
    else if (amount < (float)99.95)
    {
        round = (float)0.05;
        amount += round;
        int_amount = (int32_t)amount;
        dec_amount = (amount * 10) - (int_amount * 10);
        dbg_snprintf((char *)out_str, len, "%d.%d %s", int_amount, dec_amount, suffix);
    }
    else
    {
        round = (float)0.5;
        amount += round;
        int_amount = (int32_t)amount;
        dbg_snprintf((char *)out_str, len, "%4d %s", int_amount, suffix);
    }
}

void iperf_settings_init(struct iperf_settings_t *iperf_settings)
{
    sys_memset(iperf_settings, 0, sizeof(*iperf_settings));

    // option, defaults
    iperf_settings->flags.is_time_mode  = true;
    iperf_settings->flags.is_server     = 0; // -s or -c
    iperf_settings->buf_len             = IPERF_DEFAULT_BUFFER_LEN; // -l
    iperf_settings->port                = IPERF_DEFAULT_PORT;     // -p,  ttcp port
    iperf_settings->amount              = IPERF_DEFAULT_TIME_AMOUNT; // -t
    iperf_settings->interval.sec        = 1;
    iperf_settings->flags.show_int_stats = true;
    iperf_settings->format              = 'a';
    iperf_settings->ttl                 = 255;
}

static void iperf_dump_settings(struct iperf_settings_t *settings)
{
#ifdef CONFIG_IPV6_SUPPORT
    app_print("host_ip    : %s\r\n", ipaddr_ntoa(&settings->host_ip));
#else
    app_print("host_ip    : %08x\r\n", settings->host_ip);
#endif
    app_print("buf_len    : %u\r\n", settings->buf_len);
    app_print("test_mode  : %u\r\n", settings->test_mode);
    app_print("format     : %c\r\n", settings->format);
    app_print("listen_port: %u\r\n", settings->listen_port);
    app_print("port       : %u\r\n", settings->port);
    app_print("tos        : %u\r\n", settings->tos);
    app_print("udp rate   : %u\r\n", (uint32_t)settings->udprate);
    app_print("amount     : %u\r\n", (uint32_t)settings->amount);
    app_print("interval   : %u.%06u\r\n", settings->interval.sec, settings->interval.usec);
    app_print("flags      : %x\r\n", *(uint8_t *)(&settings->flags));
    app_print("             B[6-0]:int_stats|is_peer|is_server|bw_set|time_mode|is_udp|buflen_set\r\n");
}

void iperf_init_stats(struct net_iperf_stream *stream)
{
    struct iperf_report *report = &stream->report;
    struct iperf_settings_t *settings = &stream->iperf_settings;

    sys_memset(&report->stats, 0, sizeof(report->stats));
    sys_memset(&report->last_stats, 0, sizeof(report->last_stats));

    iperf_current_time(&report->start_time);
    if (settings->flags.show_int_stats)
    {
        iperf_timeradd(&report->start_time, &settings->interval,
                       &report->interval_target);
        report->last_interval = report->start_time;
    }

    if (!settings->flags.is_server && settings->flags.is_time_mode)
    {
        struct iperf_time duration;
        duration.sec = (uint32_t)settings->amount;// / 10;
        duration.usec = 100000;//(uint32_t)100000 * (settings->amount - (duration.sec * 10));
        iperf_timeradd(&report->start_time, &duration, &report->end_time);
    }
}

void iperf_print_interv_stats(struct net_iperf_stream *stream)
{
    struct iperf_settings_t *settings = &stream->iperf_settings;
    struct iperf_report *report = &stream->report;
    struct iperf_stats interval_stats;

    if (!settings->flags.show_int_stats ||
        iperf_timerafter(&report->interval_target, &report->packet_time))
        return;

    interval_stats.bytes = report->stats.bytes - report->last_stats.bytes;
    if (settings->flags.is_udp && settings->flags.is_server)
    {
        interval_stats.nb_datagrams = report->stats.nb_datagrams -
                                      report->last_stats.nb_datagrams;
        interval_stats.nb_error = report->stats.nb_error - report->last_stats.nb_error;
        interval_stats.nb_out_of_orded = report->stats.nb_out_of_orded -
                                         report->last_stats.nb_out_of_orded;
        if (interval_stats.nb_error > interval_stats.nb_out_of_orded)
            interval_stats.nb_error -= interval_stats.nb_out_of_orded;
        interval_stats.jitter_us = report->stats.jitter_us;
    }
    iperf_print_stats(stream, &report->last_interval, &report->packet_time,
                            &interval_stats);

    report->last_stats = report->stats;
    report->last_interval = report->packet_time;
    iperf_timeradd(&report->interval_target, &settings->interval,
                   &report->interval_target);
}


void iperf_print_stats(const struct net_iperf_stream *stream,
                             struct iperf_time *start_time,
                             struct iperf_time *end_time,
                             const struct iperf_stats *stats)
{
    const struct iperf_settings_t *iperf_settings = &stream->iperf_settings;
    const struct iperf_report *report = &stream->report;
    struct iperf_time duration_time;
    char data[11], bw[11];
    /* uint32_t int_amount, dec_amount; */
    /* float start_time_flt, end_time_flt; */
    uint64_t duration_usec;
    uint32_t start_sec, end_sec;
    int start_ds, end_ds;

    iperf_timersub(end_time, start_time, &duration_time);
    duration_usec = iperf_timerusec(&duration_time);

    // Convert in local time (i.e using report->start_time as reference) and in sec.ds
    // format
    start_sec = start_time->sec - report->start_time.sec;
    start_ds = start_time->usec - report->start_time.usec + 50000;
    if (start_ds < 0)
    {
        start_sec--;
        start_ds += 1000000;
    }
    start_ds = start_ds / 100000;

    end_sec = end_time->sec - report->start_time.sec;
    end_ds = end_time->usec - report->start_time.usec + 50000;
    if (end_ds < 0)
    {
        end_sec--;
        end_ds += 1000000;
    }
    end_ds = end_ds / 100000;

    iperf_snprintf(data, sizeof(data), (float)stats->bytes,
                         iperf_settings->format - 'a' + 'A');
    iperf_snprintf(bw, sizeof(bw), 1000000 * (float)stats->bytes / duration_usec,
                         iperf_settings->format);

    if (stream->iperf_settings.flags.is_udp && iperf_settings->flags.is_server)
    {
        uint32_t jitter_sec = stats->jitter_us / 1000;
        float lost_percent;
        uint32_t lost_percent_int, lost_percent_dec;

        lost_percent = (100 * stats->nb_error) / (float)stats->nb_datagrams;
        lost_percent += (float)0.05;
        lost_percent_int = (uint32_t)lost_percent;
        lost_percent_dec = (uint32_t)(lost_percent * 10 - lost_percent_int * 10);

        if (!stream->report.last_stats.bytes)
            app_print("[ ID]  Interval      Transfer     Bandwidth       Jitter   Lost/Total Datagrams\n");

        app_print("[%3d] %2d.%01d-%2d.%01d sec  %s  %s/sec  %d.%03d ms   %d/%d (%d.%1d%%)\n",
                    stream->id, start_sec, start_ds, end_sec, end_ds, data, bw,
                    jitter_sec, stats->jitter_us - (jitter_sec * 1000), stats->nb_error,
                    stats->nb_datagrams, lost_percent_int, lost_percent_dec);
    }
    else
    {
        if (!stream->report.last_stats.bytes)
            app_print("[ ID] Interval       Transfer     Bandwidth\n");

        app_print("[%3d] %2d.%1d-%2d.%1d sec  %s  %s/sec\n",
                    stream->id, start_sec, start_ds, end_sec, end_ds, data, bw);
    }
}

void iperf_stop_all(void)
{
    int stream_id;
    struct net_iperf_stream *iperf_stream = NULL;
    const struct iperf_settings_t *iperf_settings;

    // Search iperf stream
    for (stream_id = 0; stream_id < IPERF_MAX_STREAMS; stream_id++)
    {
        iperf_stream = &streams[stream_id];
        if (iperf_stream->active != true)
            continue;

        iperf_settings = &iperf_stream->iperf_settings;
        iperf_stream->active = false;
        if (iperf_settings->flags.is_udp) {
            sys_sema_up(&iperf_stream->iperf_task_semaphore);
        } else {
            LOCK_TCPIP_CORE();
            net_iperf_tcp_close(iperf_stream);
            UNLOCK_TCPIP_CORE();
        }
        net_iperf_buf_deinit(iperf_stream);
        app_print("iperf: delete stream [%d]\r\n", stream_id);
    }
}

os_task_t iperf_start(struct iperf_settings_t *iperf_settings)
{
    struct net_iperf_stream *iperf_stream = NULL;
    int stream_id = iperf_find_free_stream_id();

    // iperf_dump_settings(iperf_settings);

    if (stream_id == -1)
    {
        app_print("Couldn't find free stream");
        return NULL;
    }
    iperf_stream = &streams[stream_id];

    sys_memset(iperf_stream, 0, sizeof(struct net_iperf_stream));

    iperf_stream->id = stream_id;
    iperf_stream->iperf_settings = *iperf_settings;

    if (sys_sema_init_ext(&iperf_stream->iperf_task_semaphore, 1, 0))
        goto end;

    if (sys_sema_init_ext(&iperf_stream->send_buf_semaphore, IPERF_SEND_BUF_CNT,
                              IPERF_SEND_BUF_CNT))
        goto err_sem_net;

    if (sys_sema_init_ext(&iperf_stream->to_semaphore, 1, 0))
        goto err_sem_to;

    sys_mutex_new(&iperf_stream->iperf_mutex);

    if (!(iperf_stream->iperf_handle = sys_task_create_dynamic((uint8_t *)"iperf", IPERF_STACK_SIZE,
                                    IPERF_TASK_PRIO, iperf_main, iperf_stream)))
        goto err_task_create;

    return iperf_stream->iperf_handle;

  err_task_create:
    sys_mutex_free(&iperf_stream->iperf_mutex);
//  err_mutex:
    sys_sema_free(&iperf_stream->to_semaphore);
  err_sem_to:
    sys_sema_free(&iperf_stream->send_buf_semaphore);
  err_sem_net:
    sys_sema_free(&iperf_stream->iperf_task_semaphore);
  end:
    return NULL;
}

void cmd_iperf(int argc, char **argv)
{
    struct iperf_settings_t iperf_settings;
    bool client_server_set = false;
    int arg_cnt = 1;
    char *endptr = NULL;

    if (argc <= 1) {
        goto Usage;
    }

    iperf_settings_init(&iperf_settings);

    /* Parse main option */
    if (arg_cnt == 1) {
        if (strncmp(argv[1], "-s", 2) == 0) {
            if (client_server_set) {
                goto Exit;
            }
            app_print("\r\niperf: start server!\r\n");
            iperf_settings.flags.is_server = 1;
            client_server_set = true;
            arg_cnt += 1;
        } else if (strncmp(argv[1], "-c", 2) == 0) {
            app_print("\r\niperf: start client!\r\n");
            if (client_server_set)
                goto Exit;

            iperf_settings.flags.is_server = 0;
            client_server_set = true;

            if (argc >= 3) {
#ifdef CONFIG_IPV6_SUPPORT
                if (!ipaddr_aton(argv[2], &iperf_settings.host_ip))
                    goto Exit;
#else
                cli_parse_ip4(argv[2], &iperf_settings.host_ip, NULL);
#endif
            } else
                goto Exit;

            arg_cnt += 2;
        } else if (strncmp(argv[1], "stop", 4) == 0) {
            iperf_stop_all();
            return;
        } else {
            goto Exit;
        }
    }

    /* Parse other options */
    while (arg_cnt < argc) {
        /* Client/Server */
        if (strncmp(argv[arg_cnt], "-i", 2) == 0) {
            uint32_t intvl = 0;
            if (argc <= (arg_cnt + 1))
                goto Exit;
            intvl = (uint32_t)atoi(argv[arg_cnt + 1]);
            if (intvl > 3600 * OS_TICK_RATE_HZ) {
                app_print("UDP WARNNING: Report interval is larger than 3600 seconds. Use 3600 seconds instead.\r\n");
                intvl = 3600 * OS_TICK_RATE_HZ;
            }
            iperf_settings.interval.sec = intvl;
            iperf_settings.interval.usec = 0;
            iperf_settings.flags.show_int_stats = true;
            arg_cnt += 2;
        } else if (strncmp(argv[arg_cnt], "-l", 2) == 0) {
            uint32_t len = 0;
            uint32_t udp_min_size = sizeof(struct iperf_UDP_datagram);
            if (argc <= (arg_cnt + 1))
                goto Exit;
            len = (uint32_t)atoi(argv[arg_cnt + 1]);
            if (len > 5000) {
                app_print("UDP WARNNING: To save memory, the buffer size is preferably less than 5K. Use 5K instead.\r\n");
                len = 5000;
            }
            if (len > 0) {
                iperf_settings.buf_len = len;
                iperf_settings.flags.is_buf_len_set = true;
                if (iperf_settings.flags.is_udp &&
                    (iperf_settings.buf_len < udp_min_size)) {
                    iperf_settings.buf_len = udp_min_size;
                    app_print("UDP WARNNING: buffer length must be greater than or "
                                "equal to %d in UDP\n", udp_min_size);
                }
            }
            arg_cnt += 2;
        } else if (strncmp(argv[arg_cnt], "-p", 2) == 0) {
            if (argc <= (arg_cnt + 1))
                goto Exit;
            iperf_settings.port = (uint16_t)atoi(argv[arg_cnt + 1]);
            arg_cnt += 2;
        /* Client Only */
        } else if (strncmp(argv[arg_cnt], "-b", 2) == 0) {
            if (argc <= (arg_cnt + 1))
                goto Exit;
            if (iperf_settings.flags.is_server) {
                goto Exit;
            } else {
                iperf_settings.udprate = byte_atoi(argv[arg_cnt + 1]);
                iperf_settings.flags.is_udp = true;
                iperf_settings.flags.is_bw_set = true;
                // if -l has already been processed, is_buf_len_set is true so don't
                // overwrite that value.
                if (!iperf_settings.flags.is_buf_len_set)
                    iperf_settings.buf_len = IPERF_DEFAULT_UDPBUFLEN;
            }
            arg_cnt += 2;
        } else if (strncmp(argv[arg_cnt], "-n", 2) == 0) {
            if (argc <= (arg_cnt + 1))
                goto Exit;
            if (iperf_settings.flags.is_server) {
                goto Exit;
            } else {
                iperf_settings.flags.is_time_mode = false;
                iperf_settings.amount = byte_atoi(argv[arg_cnt + 1]);
            }
            arg_cnt += 2;
        } else if (strncmp(argv[arg_cnt], "-t", 2) == 0) {
            if (argc <= (arg_cnt + 1))
                goto Exit;
            if (iperf_settings.flags.is_server) {
                goto Exit;
            } else {
                iperf_settings.flags.is_time_mode = true;
                iperf_settings.amount = (uint32_t)atoi(argv[arg_cnt + 1]);
            }
            arg_cnt += 2;
        } else if (strncmp(argv[arg_cnt], "-u", 2) == 0){
            // if -b has already been processed, UDP rate will be non-zero, so don't
            // overwrite that value
            if (!iperf_settings.flags.is_udp)
            {
                iperf_settings.flags.is_udp = true;
                iperf_settings.udprate = IPERF_DEFAULT_UDPRATE;
            }

            // if -l has already been processed, is_buf_len_set is true, so don't
            // overwrite that value.
            if (!iperf_settings.flags.is_buf_len_set)
            {
                iperf_settings.buf_len = IPERF_DEFAULT_UDPBUFLEN;
            }
            arg_cnt += 1;/* ignore -u option */
        } else if (strncmp(argv[arg_cnt], "-S", 2) == 0){
            if (argc <= (arg_cnt + 1))
                goto Exit;
            if (iperf_settings.flags.is_server) {
                goto Exit;
            } else {
                // the zero base allows the user to specify
                // hexadecimals: "0x#"
                // octals: "0#"
                // decimal numbers: "#"
                iperf_settings.tos = (uint8_t)strtoul(argv[arg_cnt + 1], &endptr, 0);
                if (*endptr != '\0') {
                    app_print("iperf: invalid tos\r\n");
                    goto Exit;
                }
                switch(iperf_settings.tos) {
                    case 0x0:
                    case 0x60:
                        app_print("BE queue, tos 0x%x tid %d\r\n", iperf_settings.tos, (iperf_settings.tos >> 5));
                        break;
                    case 0x20:
                    case 0x40:
                        app_print("BK queue, tos 0x%x tid %d\r\n", iperf_settings.tos, (iperf_settings.tos >> 5));
                        break;
                    case 0x80:
                    case 0xa0:
                        app_print("VI queue, tos 0x%x tid %d\r\n", iperf_settings.tos, (iperf_settings.tos >> 5));
                        break;
                    case 0xc0:
                    case 0xe0:
                        app_print("VO queue, tos 0x%x tid %d\r\n", iperf_settings.tos, (iperf_settings.tos >> 5));
                        break;
                    default:
                        app_print("Unkonwn tos. Please enter 0, 0x20, 0x40, 0x60, 0x80, 0xa0, 0xc0 or 0xe0.\r\n");
                        break;
                }
            }
            arg_cnt += 2;
#if 0
        } else if (strncmp(argv[arg_cnt], "-T", 2) == 0) {
            if (argc <= (arg_cnt + 1))
                goto Usage;
            iperf_settings.ttl = (uint32_t)atoi(argv[arg_cnt + 1]);
            arg_cnt += 2;
        } else if (strncmp(argv[arg_cnt], "-f", 2) == 0) {
            if (argc <= (arg_cnt + 1))
                goto Usage;
            iperf_settings.format = *(argv[arg_cnt + 1]);
            arg_cnt += 2;
        } else if (strncmp(argv[arg_cnt], "-X", 2) == 0) {
            iperf_settings.flags.is_peer_ver = true;
            arg_cnt += 1;
#endif
        } else {
            goto Exit;
        }
    }

    if (!client_server_set)
        goto Exit;

    iperf_start(&iperf_settings);

    return;

Exit:
    app_print("\r\nIperf: command format error!\r\n");
Usage:
    app_print("\rUsage:\r\n");
    app_print("    iperf <-s|-c hostip|stop|-h> [options]\r\n");
    app_print("\rClient/Server:\r\n");
    app_print("    -u #      use UDP rather than TCP\r\n");
    app_print("    -i #      seconds between periodic bandwidth reports\r\n");
    app_print("    -l #      length of buffer to read or write (default 1460 Bytes)\r\n");
    app_print("    -p #      server port to listen on/connect to (default 5001)\r\n");
    app_print("\rServer specific:\r\n");
    app_print("    -s        run in server mode\r\n");
    app_print("\rClient specific:\r\n");
    app_print("    -b #      bandwidth to send at in bits/sec (default 1 Mbit/sec, implies -u)\r\n");
    app_print("    -S #      set the IP 'type of service'\r\n");
    app_print("    -c <host> run in client mode, connecting to <host>\r\n");
    app_print("    -t #      time in seconds to transmit for (default 10 secs)\r\n");
}
/// @}
#endif /* CONFIG_IPERF_TEST */
