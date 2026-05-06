/*!
    \file    net_iperf_al.c
    \brief   the header file of tcp/udp operations for iperf.

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

#ifndef _NET_IPERF_AL_H__
#define _NET_IPERF_AL_H__

/*
 * DEFINITIONS
 ****************************************************************************************
 */
// Forward declarations of iperf stream
struct net_iperf_stream;

/*
 * GLOBAL VARIABLES
 ****************************************************************************************
 */

/*
 * FUNCTIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialize iperf buffers
 *
 * @param[in] stream   Pointer to iperf stream
 * @return  0 if successful, -1 otherwise
 ****************************************************************************************
 **/
int net_iperf_buf_init(struct net_iperf_stream* stream);

/**
 ****************************************************************************************
 * @brief Release iperf buffers
 *
 * @param[in] iperf_stream  Pointer to iperf stream
 ****************************************************************************************
 **/
void net_iperf_buf_deinit(struct net_iperf_stream* stream);

/**
 ****************************************************************************************
 * @brief Open UDP connection as client and connect to UDP server.
 * Initialize UDP transmission and send UDP datagram to server. After sending a burst of
 * UDP frames, wait in order to meet bandwidth constraints.
 * The last UDP datagram needs to have a negative packet id to indicate the server that
 * transmission is over.
 *
 * @param[in] iperf_stream_ptr  Pointer to iperf stream
 * @return  0 if successful, -1 otherwise.
 ****************************************************************************************
 **/
int net_iperf_udp_client_run(struct net_iperf_stream *iperf_stream_ptr);

/**
 ****************************************************************************************
 * @brief Open UDP connection as server and listen to UDP port. Set packet reception
 * callback function to handle UDP packets. When a packet with ID < 0 is
 * received, the server report is sent to the client and statistics are printed.
 *
 * @param[in] iperf_stream_ptr  Pointer to iperf stream
 * @return  0 if successful, -1 otherwise.
 ****************************************************************************************
 **/
int net_iperf_udp_server_run(struct net_iperf_stream *iperf_stream_ptr);

/**
 ****************************************************************************************
 * @brief Open TCP connection as server and listen to TCP port. Wait for TCP traffic
 * to end before returning
 *
 * @param[in] iperf_stream_ptr  Pointer to iperf stream
 * @return  0 if successful, -1 otherwise.
 ****************************************************************************************
 **/
int net_iperf_tcp_server_run(struct net_iperf_stream *iperf_stream_ptr);

/**
 ****************************************************************************************
 * @brief Open UDP connection as client and connect to UDP server.
 * Initialize UDP transmission and send UDP datagram to server. After sending a burst of
 * UDP frames, wait in order to meet bandwidth constraints.
 * The last UDP datagram needs to have a negative packet id to indicate the server that
 * transmission is over.
 *
 * @param[in] stream  Pointer to iperf stream
 * @return  0 if successful, -1 otherwise.
 ****************************************************************************************
 **/
int net_iperf_tcp_client_run(struct net_iperf_stream *stream);

/**
 ****************************************************************************************
 * @brief TCP closing function. Remove TCP callbacks, print final stats,
 * close TCP protocol control block and wake-up iperf task.
 *
 * @param[in] stream  Pointer to iperf stream
 ****************************************************************************************
 **/
void net_iperf_tcp_close(struct net_iperf_stream *stream);
#endif //_NET_IPERF_AL_H__
