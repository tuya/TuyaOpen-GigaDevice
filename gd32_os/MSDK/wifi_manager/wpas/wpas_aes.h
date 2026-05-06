/*
 * AES functions
 * Copyright (c) 2003-2006, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

/*!
    \file    wpas_aes.h
    \brief   Header file for wpas AES.

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

#ifndef _WPAS_AES_H_
#define _WPAS_AES_H_

/* #define FULL_UNROLL */
#define AES_SMALL_TABLES

extern const uint32_t Te0[256];
#ifndef AES_SMALL_TABLES
extern const uint32_t Te1[256];
extern const uint32_t Te2[256];
extern const uint32_t Te3[256];
extern const uint32_t Te4[256];
#endif
extern const uint32_t Td0[256];
#ifndef AES_SMALL_TABLES
extern const uint32_t Td1[256];
extern const uint32_t Td2[256];
extern const uint32_t Td3[256];
extern const uint32_t Td4[256];
extern const uint32_t rcon[10];
#else
extern const uint8_t Td4s[256];
extern const uint8_t rcons[10];
#endif

#ifndef AES_SMALL_TABLES

#define RCON(i) rcon[(i)]

#define TE0(i) Te0[((i) >> 24) & 0xff]
#define TE1(i) Te1[((i) >> 16) & 0xff]
#define TE2(i) Te2[((i) >> 8) & 0xff]
#define TE3(i) Te3[(i) & 0xff]
#define TE41(i) (Te4[((i) >> 24) & 0xff] & 0xff000000)
#define TE42(i) (Te4[((i) >> 16) & 0xff] & 0x00ff0000)
#define TE43(i) (Te4[((i) >> 8) & 0xff] & 0x0000ff00)
#define TE44(i) (Te4[(i) & 0xff] & 0x000000ff)
#define TE421(i) (Te4[((i) >> 16) & 0xff] & 0xff000000)
#define TE432(i) (Te4[((i) >> 8) & 0xff] & 0x00ff0000)
#define TE443(i) (Te4[(i) & 0xff] & 0x0000ff00)
#define TE414(i) (Te4[((i) >> 24) & 0xff] & 0x000000ff)
#define TE411(i) (Te4[((i) >> 24) & 0xff] & 0xff000000)
#define TE422(i) (Te4[((i) >> 16) & 0xff] & 0x00ff0000)
#define TE433(i) (Te4[((i) >> 8) & 0xff] & 0x0000ff00)
#define TE444(i) (Te4[(i) & 0xff] & 0x000000ff)
#define TE4(i) (Te4[(i)] & 0x000000ff)

#define TD0(i) Td0[((i) >> 24) & 0xff]
#define TD1(i) Td1[((i) >> 16) & 0xff]
#define TD2(i) Td2[((i) >> 8) & 0xff]
#define TD3(i) Td3[(i) & 0xff]
#define TD41(i) (Td4[((i) >> 24) & 0xff] & 0xff000000)
#define TD42(i) (Td4[((i) >> 16) & 0xff] & 0x00ff0000)
#define TD43(i) (Td4[((i) >> 8) & 0xff] & 0x0000ff00)
#define TD44(i) (Td4[(i) & 0xff] & 0x000000ff)
#define TD0_(i) Td0[(i) & 0xff]
#define TD1_(i) Td1[(i) & 0xff]
#define TD2_(i) Td2[(i) & 0xff]
#define TD3_(i) Td3[(i) & 0xff]

#else /* AES_SMALL_TABLES */

#define RCON(i) ((uint32_t) rcons[(i)] << 24)

static inline uint32_t rotr(uint32_t val, int bits)
{
    return (val >> bits) | (val << (32 - bits));
}

#define TE0(i)      Te0[((i) >> 24) & 0xff]
#define TE1(i)      rotr(Te0[((i) >> 16) & 0xff], 8)
#define TE2(i)      rotr(Te0[((i) >> 8) & 0xff], 16)
#define TE3(i)      rotr(Te0[(i) & 0xff], 24)
#define TE41(i)     ((Te0[((i) >> 24) & 0xff] << 8) & 0xff000000)
#define TE42(i)     (Te0[((i) >> 16) & 0xff] & 0x00ff0000)
#define TE43(i)     (Te0[((i) >> 8) & 0xff] & 0x0000ff00)
#define TE44(i)     ((Te0[(i) & 0xff] >> 8) & 0x000000ff)
#define TE421(i)    ((Te0[((i) >> 16) & 0xff] << 8) & 0xff000000)
#define TE432(i)    (Te0[((i) >> 8) & 0xff] & 0x00ff0000)
#define TE443(i)    (Te0[(i) & 0xff] & 0x0000ff00)
#define TE414(i)    ((Te0[((i) >> 24) & 0xff] >> 8) & 0x000000ff)
#define TE411(i)    ((Te0[((i) >> 24) & 0xff] << 8) & 0xff000000)
#define TE422(i)    (Te0[((i) >> 16) & 0xff] & 0x00ff0000)
#define TE433(i)    (Te0[((i) >> 8) & 0xff] & 0x0000ff00)
#define TE444(i)    ((Te0[(i) & 0xff] >> 8) & 0x000000ff)
#define TE4(i)      ((Te0[(i)] >> 8) & 0x000000ff)

#define TD0(i)  Td0[((i) >> 24) & 0xff]
#define TD1(i)  rotr(Td0[((i) >> 16) & 0xff], 8)
#define TD2(i)  rotr(Td0[((i) >> 8) & 0xff], 16)
#define TD3(i)  rotr(Td0[(i) & 0xff], 24)
#define TD41(i) ((uint32_t) Td4s[((i) >> 24) & 0xff] << 24)
#define TD42(i) ((uint32_t) Td4s[((i) >> 16) & 0xff] << 16)
#define TD43(i) ((uint32_t) Td4s[((i) >> 8) & 0xff] << 8)
#define TD44(i) ((uint32_t) Td4s[(i) & 0xff])
#define TD0_(i) Td0[(i) & 0xff]
#define TD1_(i) rotr(Td0[(i) & 0xff], 8)
#define TD2_(i) rotr(Td0[(i) & 0xff], 16)
#define TD3_(i) rotr(Td0[(i) & 0xff], 24)

#endif /* AES_SMALL_TABLES */

#ifdef _MSC_VER
#define SWAP(x)         (_lrotl(x, 8) & 0x00ff00ff | _lrotr(x, 8) & 0xff00ff00)
#define GETU32(p)       SWAP(*((uint32_t *)(p)))
#define PUTU32(ct, st)  { *((uint32_t *)(ct)) = SWAP((st)); }
#else
#define GETU32(pt) (((uint32_t)(pt)[0] << 24) ^ ((uint32_t)(pt)[1] << 16) ^ \
((uint32_t)(pt)[2] <<  8) ^ ((uint32_t)(pt)[3]))
#define PUTU32(ct, st) { \
(ct)[0] = (uint8_t)((st) >> 24); (ct)[1] = (uint8_t)((st) >> 16); \
(ct)[2] = (uint8_t)((st) >>  8); (ct)[3] = (uint8_t)(st); }
#endif

#define AES_BLOCK_SIZE  16
#define AES_PRIV_SIZE   (4 * 4 * 15 + 4)
#define AES_PRIV_NR_POS (4 * 15)

void * aes_encrypt_init(const uint8_t *key, size_t len);
int aes_encrypt(void *ctx, const uint8_t *plain, uint8_t *crypt);
void aes_encrypt_deinit(void *ctx);
int aes_wrap(const uint8_t *kek, size_t kek_len, int n, const uint8_t *plain, uint8_t *cipher);
void * aes_decrypt_init(const uint8_t *key, size_t len);
int aes_decrypt(void *ctx, const uint8_t *crypt, uint8_t *plain);
void aes_decrypt_deinit(void *ctx);
int aes_unwrap(const uint8_t *kek, size_t kek_len, int n, const uint8_t *cipher,
                uint8_t *plain);
int omac1_aes_128_vector(const uint8_t *key, size_t num_elem,
                        const uint8_t *addr[], const size_t *len,
                        uint8_t *mac);
int omac1_aes_128(const uint8_t *key, const uint8_t *data, size_t data_len,
                    uint8_t *mac);
int aes_128_cbc_decrypt(const uint8_t *key, const uint8_t *iv, uint8_t *data, size_t data_len);
int aes_128_cbc_encrypt(const uint8_t *key, const uint8_t *iv, uint8_t *data, size_t data_len);

#endif /* _WPAS_AES_H_ */
