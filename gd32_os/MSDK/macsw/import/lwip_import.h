/*!
    \file    lwip_import.h
    \brief   lwip functions import for WIFI lib.

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

#ifndef __LWIP_IMPORT_H__
#define __LWIP_IMPORT_H__

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "stdint.h"

/*
 * DEFINITIONS
 ****************************************************************************************
 */

// Net buffer
typedef struct pbuf_custom  net_buf_rx_t;

// Net TX buffer
typedef struct pbuf         net_buf_tx_t;

// Minimum headroom to include in all TX buffer
#define NET_AL_TX_HEADROOM 348

// Prototype for a function to free a network buffer */
typedef void (*net_buf_free_fn)(void *net_buf);

/*
 * TYPES
 ****************************************************************************************
 */
/*
 *!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * The following structures MUST BE EXACTLY THE SAME as defined in lwip/pbuf.h
 *!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */
#ifndef LWIP_HDR_PBUF_H

#ifndef LWIP_PBUF_REF_T
#define LWIP_PBUF_REF_T     uint8_t
#endif

/** Main packet buffer struct */
struct pbuf {
    /** next pbuf in singly linked pbuf chain */
    struct pbuf *next;

    /** pointer to the actual data in the buffer */
    void *payload;

    /**
     * total length of this buffer and all next buffers in chain
     * belonging to the same packet.
     *
     * For non-queue packet chains this is the invariant:
     * p->tot_len == p->len + (p->next? p->next->tot_len: 0)
     */
    uint16_t tot_len;

    /** length of this buffer */
    uint16_t len;

    /** a bit field indicating pbuf type and allocation sources
     (see PBUF_TYPE_FLAG_*, PBUF_ALLOC_FLAG_* and PBUF_TYPE_ALLOC_SRC_MASK)
    */
    uint8_t type_internal;

    /** misc flags */
    uint8_t flags;

    /**
     * the reference count always equals the number of pointers
     * that refer to this pbuf. This can be pointers from an application,
     * the stack itself, or pbuf->next pointers from a chain.
     */
    LWIP_PBUF_REF_T ref;

    /** For incoming packets, this contains the input netif's index */
    uint8_t if_idx;
};

/** Prototype for a function to free a custom pbuf */
typedef void (*pbuf_free_custom_fn)(struct pbuf *p);

/** A custom pbuf: like a pbuf, but following a function pointer to free it. */
struct pbuf_custom {
    /** The actual pbuf */
    struct pbuf pbuf;
    /** This function is called when pbuf_free deallocates this pbuf(_custom) */
    pbuf_free_custom_fn custom_free_function;
};
#endif /* LWIP_HDR_PBUF_H */

/*
 *!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * The above structures MUST BE EXACTLY THE SAME as defined in lwip/pbuf.h
 *!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/*============================ PROTOTYPES ====================================*/
extern int lwip_socket(int domain, int type, int protocol);
extern int lwip_send(int s, const void *dataptr, size_t size, int flags);
extern int lwip_recv(int s, void *mem, size_t len, int flags);
extern int lwip_close(int s);
extern int net_lpbk_socket_create(int protocol);
extern int net_lpbk_socket_bind(int sock_recv, uint32_t port);
extern int net_lpbk_socket_connect(int sock_send, uint32_t port);
extern void net_if_up(void *net_if);
extern void net_if_down(void *net_if);
extern net_buf_tx_t *net_buf_tx_alloc(uint32_t length);
extern net_buf_tx_t *net_buf_tx_alloc_ref(uint32_t length);
extern void net_buf_tx_pbuf_free(net_buf_tx_t *buf);
extern void net_buf_tx_cat(net_buf_tx_t *buf1, net_buf_tx_t *buf2);
extern int net_if_input(net_buf_rx_t *buf, void *net_if, void *addr, uint16_t len,
                        net_buf_free_fn free_fn);
extern void net_buf_tx_free(net_buf_tx_t *buf);
extern void *net_buf_tx_info(net_buf_tx_t *buf, uint16_t *tot_len, int *seg_cnt,
                            uint32_t seg_addr[], uint16_t seg_len[]);

#endif /* __LWIP_IMPORT_H__ */
