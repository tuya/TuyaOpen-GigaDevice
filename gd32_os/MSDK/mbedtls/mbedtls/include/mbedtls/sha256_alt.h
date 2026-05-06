/*!
    \file    sha256_alt.h
    \brief   SHA256 block cipher for GD32VW55x SDK.

    \version 2024-12-30, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2024, GigaDevice Semiconductor Inc.

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

#ifndef MBEDTLS_SHA256_ALT_H
#define MBEDTLS_SHA256_ALT_H

#if defined(MBEDTLS_SHA256_ALT)

#include "gd32vw55x.h"

#define SHAMD5_BSY_TIMEOUT    ((uint32_t)0x00010000U)
#define SHA256_BLOCK_SIZE     ((uint32_t)64)        /*!< HAU BLOCK_SIZE 512 bits, ie 64 bytes */
/**
 * \brief          SHA-256 context structure
 */
typedef struct mbedtls_sha256_context
{
    int is256;                                      /*!< 1 = use SHA256, 0 = use SHA224 */
    uint8_t buf[64];                               /*!< Buffer to store input data until SHA256_BLOCK_SIZE
                                                         is reached, or until last input data is reached */
    uint8_t buf_len;                               /*!< Number of bytes stored in sbuf */
    hau_context_parameter_struct context_para;     /* structure for context switch */
} mbedtls_sha256_context;


/*init mbedtls_sha256_context struct*/
void hau_sha256_context_init(mbedtls_sha256_context *ct);
/*interface function*/
void hau_sha256_start(mbedtls_sha256_context *ct, int is256);
ErrStatus hau_sha256_update(mbedtls_sha256_context *ct,uint8_t *input, uint32_t in_length);
ErrStatus hau_sha256_finish(mbedtls_sha256_context *ct,uint8_t output[32]);
/*process input data then read digest*/

#endif //MBEDTLS_SHA256_ALT
#endif //MBEDTLS_SHA256_ALT_H
