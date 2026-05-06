/*!
    \file    macif_rx_def.h
    \brief   Definitions of the macif RX task related.

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

#ifndef _MACIF_RX_DEF_H_
#define _MACIF_RX_DEF_H_

// Number of RX buffers
#if CFG_BARX
#if 1//WIFI_AMSDU_UPLOAD_WITH_HEAP
#define MACIF_RX_BUF_CNT            (CFG_BARX * CFG_REORD_BUF + 1)
#else
#define MACIF_RX_BUF_CNT            (WIFI_MAX_BA_RX * WIFI_AMPDU_RX_BUF_SIZE + 1 + \
                                    WIFI_DMADESC_PER_RX_PDB_CNT)
#endif
#else /* CFG_BARX */
#define MAX_MSDU_PER_RX_AMSDU       8
#define MACIF_RX_BUF_CNT            3 + MAX_MSDU_PER_RX_AMSDU
#endif /* CFG_BARX */

// Number of elements in the RX descriptor queue
#define MACIF_RX_QUEUE_DESC_ELT_CNT (MACIF_RX_BUF_CNT * 2)

#endif // _MACIF_RX_DEF_H_
