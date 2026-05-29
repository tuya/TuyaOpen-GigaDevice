/*
 * FreeRTOS V202212.01
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/**
 * @file sockets_wrapper.c
 * @brief FreeRTOS Sockets connect and disconnect wrapper implementation.
 */

/* Include header that defines log levels. */
#include "logging_levels.h"

/* Logging configuration for the Sockets. */
#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME     "SocketsWrapper"
#endif
#ifndef LIBRARY_LOG_LEVEL
    #define LIBRARY_LOG_LEVEL    LOG_INFO
#endif

extern void vLoggingPrintf( const char * pcFormatString,
                            ... );

#include "logging_stack.h"

/* Standard includes. */
#include <string.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"

/* FreeRTOS+TCP includes.
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_DNS.h"
*/
/* TCP Sockets Wrapper include.*/
/* Let sockets wrapper know that Socket_t is defined already. */
//#define SOCKET_T_TYPEDEFED
#include "tcp_sockets_wrapper.h"
#include "lwip/sockets.h"
#include "lwip/ip.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

char echo_srv_ip[100] = {0};      //for local test

#define SOCKET_T_TYPEDEFED

typedef struct xSOCKET {
	int32_t fd;
} lwipSokcetWrapper_t;
//typedef int32_t struct xSOCKET;
/**
 * @brief negative error code indicating a network failure.
 */
#define SOCKETS_WRAPPER_NETWORK_ERROR    ( -1 )
#define SOCKETS_INVALID_SOCKET      ( ( Socket_t ) ~0U )

/**
 * @brief Number of milliseconds in one second.
 */
#define ONE_SEC_TO_MS    ( 1000 )

/**
 * @brief Number of microseconds in one millisecond.
 */
#define ONE_MS_TO_US     ( 1000 )


/*-----------------------------------------------------------*/

static BaseType_t retrieveError( int32_t errorNumber )
{
    BaseType_t returnStatus = TCP_SOCKETS_ERRNO_ERROR;

    LogError( ( "A transport error occured: %s.", strerror( errorNumber ) ) );

    if( ( errorNumber == ENOMEM ) || ( errorNumber == ENOBUFS ) )
    {
        returnStatus = TCP_SOCKETS_ERRNO_ENOMEM;
    }
    else if( ( errorNumber == ENOTSOCK ) || ( errorNumber == EDOM ) || ( errorNumber == EBADF ) )
    {
        returnStatus = TCP_SOCKETS_ERRNO_EINVAL;
    }
    else
    {
        /* Empty else. */
    }

    return returnStatus;
}

/**
 * @brief Establish a connection to server.
 *
 * @param[out] pTcpSocket The output parameter to return the created socket descriptor.
 * @param[in] pHostName Server hostname to connect to.
 * @param[in] pServerInfo Server port to connect to.
 * @param[in] receiveTimeoutMs Timeout (in milliseconds) for transport receive.
 * @param[in] sendTimeoutMs Timeout (in milliseconds) for transport send.
 *
 * @note A timeout of 0 means infinite timeout.
 *
 * @return Non-zero value on error, 0 on success.
 */
BaseType_t TCP_Sockets_Connect( Socket_t * pTcpSocket,
                                const char * pHostName,
                                uint16_t port,
                                uint32_t receiveTimeoutMs,
                                uint32_t sendTimeoutMs )
{
    Socket_t tcpSocket = SOCKETS_INVALID_SOCKET;
    BaseType_t socketStatus = 0;
    struct sockaddr_in serverAddress = { 0 };
    struct hostent *hptr;
    int32_t setTimeoutStatus = -1;

    configASSERT( pTcpSocket != NULL );
    configASSERT( pHostName != NULL );

    tcpSocket = pvPortMalloc(sizeof(struct xSOCKET));
    configASSERT(tcpSocket != NULL);

    /* Create a new TCP socket. */
    tcpSocket->fd = socket(AF_INET, SOCK_STREAM, 0);
    if( tcpSocket->fd == SOCKETS_WRAPPER_NETWORK_ERROR )
    {
        LogError( ( "Failed to create new socket." ) );
        socketStatus = SOCKETS_WRAPPER_NETWORK_ERROR;
    }
    else
    {
        LogDebug( ( "Created new TCP socket." ) );

        hptr = gethostbyname(pHostName);
        if (hptr == NULL) {
            LogError( ( "Failed to connect to server: DNS resolution failed: Hostname=%s.",
                        pHostName ) );
            socketStatus = SOCKETS_WRAPPER_NETWORK_ERROR;
            return socketStatus;
        }

        /* Connection parameters. */
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);
        serverAddress.sin_addr.s_addr = *(uint32_t *)hptr->h_addr;
//        serverAddress.sin_addr.s_addr = inet_addr(echo_srv_ip);     //for aws local test

        /* Check for errors from DNS lookup. */
        if( serverAddress.sin_addr.s_addr == 0U )
        {
            LogError( ( "Failed to connect to server: DNS resolution failed: Hostname=%s.",
                        pHostName ) );
            socketStatus = SOCKETS_WRAPPER_NETWORK_ERROR;
        }
    }

    if( socketStatus == 0 )
    {
        /* Establish connection. */
        LogDebug( ( "Creating TCP Connection to %s.", pHostName ) );
        socketStatus = connect( tcpSocket->fd, (struct sockaddr *)&serverAddress, sizeof( serverAddress ) );

        if( socketStatus != 0 )
        {
            LogError( ( "Failed to connect to server: FreeRTOS_Connect failed: ReturnCode=%d,"
                        " Hostname=%s, Port=%u.",
                        socketStatus,
                        pHostName,
                        port ) );
        }
    }

    if( socketStatus == 0 )
    {
        /* Set socket receive timeout. */
        /* Setting the receive block time cannot fail. */
         setTimeoutStatus = setsockopt( tcpSocket->fd,
                                       SOL_SOCKET,
                                       SO_RCVTIMEO,
                                       &receiveTimeoutMs,
                                       sizeof(receiveTimeoutMs));
        if( setTimeoutStatus < 0 )
        {
            if( errno == ENOPROTOOPT )
            {
                LogInfo( ( "Setting socket receive timeout skipped." ) );
            }
            else
            {
                LogError( ( "Setting socket receive timeout failed.") );
                socketStatus = retrieveError( errno );
            }
        }

        if (socketStatus != 0) {
            /* Set socket send timeout. */
            setTimeoutStatus = setsockopt( tcpSocket->fd,
                                           SOL_SOCKET,
                                           SO_SNDTIMEO,
                                           &sendTimeoutMs,
                                           sizeof( sendTimeoutMs ) );

            if( setTimeoutStatus < 0 )
            {
                if( errno == ENOPROTOOPT )
                {
                    LogInfo( ( "Setting socket send timeout skipped." ) );
                }
                else
                {
                    LogError( ( "Setting socket send timeout failed." ) );
                    socketStatus = retrieveError( errno );
                }
            }
        }
    }

    /* Clean up on failure. */
    if( socketStatus != 0 )
    {
        if( tcpSocket != SOCKETS_INVALID_SOCKET )
        {
            ( void ) close( tcpSocket->fd );
            tcpSocket->fd = -1;
            vPortFree(tcpSocket);
        }
    }
    else
    {
        /* Set the socket. */
        *pTcpSocket = tcpSocket;
        LogInfo( ( "Established TCP connection with %s.", pHostName ) );
    }

    return socketStatus;
}

/**
 * @brief End connection to server.
 *
 * @param[in] tcpSocket The socket descriptor.
 */
void TCP_Sockets_Disconnect( Socket_t tcpSocket )
{
    if( tcpSocket )
    {
        ( void ) shutdown( tcpSocket->fd, SHUT_RDWR );
        ( void ) close( tcpSocket->fd );
        tcpSocket->fd = -1;
        vPortFree(tcpSocket);
        tcpSocket = NULL;
    }
    else
    {
        LogError( ( "Parameter check failed: tcpSocket was negative." ) );
    }
}

/**
 * @brief Transmit data to the remote socket.
 *
 * The socket must have already been created using a call to TCP_Sockets_Connect().
 *
 * @param[in] xSocket The handle of the sending socket.
 * @param[in] pvBuffer The buffer containing the data to be sent.
 * @param[in] xDataLength The length of the data to be sent.
 *
 * @return
 * * On success, the number of bytes actually sent is returned.
 * * If an error occurred, a negative value is returned. @ref SocketsErrors
 */
int32_t TCP_Sockets_Send( Socket_t xSocket,
                          const void * pvBuffer,
                          size_t xBufferLength )
{
    BaseType_t xSendStatus;
    int xReturnStatus = TCP_SOCKETS_ERRNO_ERROR;

    configASSERT( xSocket != NULL );
    configASSERT( pvBuffer != NULL );

    xSendStatus = send( xSocket->fd, pvBuffer, xBufferLength, 0 );

    if (xSendStatus < 0) {
        switch( errno )
        {
            /* Socket was closed or just got closed. */
            case EBADF:
                xReturnStatus = TCP_SOCKETS_ERRNO_ENOTCONN;
                break;
            /* Not enough memory for the socket to create either an Rx or Tx stream. */
            case ENOMEM:
            case ENOBUFS:
                xReturnStatus = TCP_SOCKETS_ERRNO_ENOMEM;
                break;
            /* Socket is not valid, is not a TCP socket, or is not bound. */
            case EINVAL:
                xReturnStatus = TCP_SOCKETS_ERRNO_EINVAL;
                break;

            /* Socket received a signal, causing the read operation to be aborted. */
            case EINTR:
                xReturnStatus = TCP_SOCKETS_ERRNO_EINTR;
                break;

            /* A timeout occurred before any data could be sent as the TCP buffer was full. */
            case ENOSPC:
                xReturnStatus = TCP_SOCKETS_ERRNO_ENOSPC;
                break;
            case EAGAIN:
                xReturnStatus = TCP_SOCKETS_ERRNO_EWOULDBLOCK;
                break;
            default:
                xReturnStatus = ( int ) xSendStatus;
                break;
        }
    }else {
        xReturnStatus = ( int ) xSendStatus;
    }

    return xReturnStatus;
}

/**
 * @brief Receive data from a TCP socket.
 *
 * The socket must have already been created using a call to TCP_Sockets_Connect().
 *
 * @param[in] xSocket The handle of the socket from which data is being received.
 * @param[out] pvBuffer The buffer into which the received data will be placed.
 * @param[in] xBufferLength The maximum number of bytes which can be received.
 * pvBuffer must be at least xBufferLength bytes long.
 *
 * @return
 * * If the receive was successful then the number of bytes received (placed in the
 *   buffer pointed to by pvBuffer) is returned.
 * * If a timeout occurred before data could be received then 0 is returned (timeout
 *   is set using @ref SOCKETS_SO_RCVTIMEO).
 * * If an error occurred, a negative value is returned. @ref SocketsErrors
 */
int32_t TCP_Sockets_Recv( Socket_t xSocket,
                          void * pvBuffer,
                          size_t xBufferLength )
{
    BaseType_t xRecvStatus;
    int xReturnStatus = TCP_SOCKETS_ERRNO_ERROR;

    configASSERT( xSocket != NULL );
    configASSERT( pvBuffer != NULL );

    xRecvStatus = read( xSocket->fd, pvBuffer, xBufferLength);
    if (xRecvStatus < 0) {
        switch( errno )
        {
            /* Socket was closed or just got closed. */
            case EBADF:
                xReturnStatus = TCP_SOCKETS_ERRNO_ENOTCONN;
                break;

            /* Not enough memory for the socket to create either an Rx or Tx stream. */
            case ENOMEM:
            case ENOBUFS:
                xReturnStatus = TCP_SOCKETS_ERRNO_ENOMEM;
                break;

            /* Socket is not valid, is not a TCP socket, or is not bound. */
            case EINVAL:
                xReturnStatus = TCP_SOCKETS_ERRNO_EINVAL;
                break;

            /* Socket received a signal, causing the read operation to be aborted. */
            case EINTR:
                xReturnStatus = TCP_SOCKETS_ERRNO_EINTR;
                break;
            case EAGAIN:
                xReturnStatus = TCP_SOCKETS_ERRNO_EWOULDBLOCK;
                break;
            default:
                xReturnStatus = ( int ) xRecvStatus;
                break;
        }
    }else {
        xReturnStatus = ( int ) xRecvStatus;
    }

    return xReturnStatus;
}

