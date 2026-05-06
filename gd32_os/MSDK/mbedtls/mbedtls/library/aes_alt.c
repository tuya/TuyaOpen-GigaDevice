/*!
    \file    aes_alt.c
    \brief   AES block cipher for GD32VW55x SDK.

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

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/mbedtls_config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_AES_C)

#include <string.h>
#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#define mbedtls_printf printf
#endif /* MBEDTLS_PLATFORM_C */

#include "mbedtls/aes.h"

#if defined(MBEDTLS_AES_ALT)
#include "gd32vw55x_cau.h"

/* Implementation that should never be optimized out by the compiler */
static void mbedtls_zeroize(void *v, size_t n)
{
    volatile unsigned char *p = (unsigned char*)v;
    while (n--)
        *p++ = 0;
}

void mbedtls_aes_init(mbedtls_aes_context *ctx)
{
    memset(ctx, 0, sizeof(mbedtls_aes_context));
}

void mbedtls_aes_free(mbedtls_aes_context *ctx)
{
    if (ctx == NULL)
        return;

    mbedtls_zeroize(ctx, sizeof(mbedtls_aes_context));
}

/*
 * AES key schedule (encryption)
 */
int mbedtls_aes_setkey_enc(mbedtls_aes_context *ctx, const unsigned char *key,
                    unsigned int keybits)
{
    ctx->keybits = keybits;
    memcpy(ctx->key, key, (keybits/8));

    return 0;
}

/*
 * AES key schedule (decryption)
 */
int mbedtls_aes_setkey_dec(mbedtls_aes_context *ctx, const unsigned char *key,
                    unsigned int keybits)
{
    ctx->keybits = keybits;
    memcpy(ctx->key, key, (keybits/8));

    return 0;
}

/*
 * AES-ECB block encryption/decryption
 */
int mbedtls_aes_crypt_ecb(mbedtls_aes_context *ctx,
                    int mode,
                    const unsigned char input[16],
                    unsigned char output[16])
{
    ErrStatus ret;
    cau_parameter_struct cau_aes_parameter;

    switch (mode) {
    case MBEDTLS_AES_DECRYPT:
        cau_aes_parameter.alg_dir = CAU_DECRYPT;
        break;
    case MBEDTLS_AES_ENCRYPT:
        cau_aes_parameter.alg_dir = CAU_ENCRYPT;
        break;
    default:
        break;
    }
    cau_aes_parameter.key = ctx->key;
    cau_aes_parameter.key_size = ctx->keybits;
    cau_aes_parameter.input = (uint8_t *)input;
    cau_aes_parameter.in_length = 16;

    //GLOBAL_INT_DISABLE();
    ret = cau_aes_ecb(&cau_aes_parameter, output);
    //GLOBAL_INT_RESTORE();
    return (ret == ERROR) ? 1 : 0;
}

#if defined(MBEDTLS_CIPHER_MODE_CBC)
/*
 * AES-CBC buffer encryption/decryption
 */
int mbedtls_aes_crypt_cbc(mbedtls_aes_context *ctx,
                    int mode,
                    size_t length,
                    unsigned char iv[16],
                    const unsigned char *input,
                    unsigned char *output)
{
    ErrStatus ret;
    unsigned char temp[16];
    cau_parameter_struct cau_cbc_parameter;

    if (length % 16)
        return MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH;

    switch (mode) {
    case MBEDTLS_AES_DECRYPT:
        cau_cbc_parameter.alg_dir = CAU_DECRYPT;
        break;
    case MBEDTLS_AES_ENCRYPT:
        cau_cbc_parameter.alg_dir = CAU_ENCRYPT;
        break;
    default:
        break;
    }
    cau_cbc_parameter.iv = (uint8_t *)iv;
    cau_cbc_parameter.iv_size = 16;
    cau_cbc_parameter.key = ctx->key;
    cau_cbc_parameter.key_size = ctx->keybits;
    cau_cbc_parameter.input = (uint8_t *)input;
    cau_cbc_parameter.in_length = length;

    memcpy(temp, (input + length - 16), 16);
    //GLOBAL_INT_DISABLE();
    ret = cau_aes_cbc(&cau_cbc_parameter, output);
    if (mode == MBEDTLS_AES_DECRYPT)
        memcpy(iv, temp, 16);
    else
        memcpy(iv, (output + length - 16), 16);
    //GLOBAL_INT_RESTORE();

    return (ret == ERROR) ? 1 : 0;
}
#endif /* MBEDTLS_CIPHER_MODE_CBC */

#if defined(MBEDTLS_CIPHER_MODE_CFB)
/*
 * AES-CFB128 buffer encryption/decryption
 */
int mbedtls_aes_crypt_cfb128(mbedtls_aes_context *ctx,
                       int mode,
                       size_t length,
                       size_t *iv_off,
                       unsigned char iv[16],
                       const unsigned char *input,
                       unsigned char *output)
{
    int c;
    size_t n = *iv_off;

    if (mode == MBEDTLS_AES_DECRYPT) {
        while (length--) {
            if (n == 0)
                mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_ENCRYPT, iv, iv);

            c = *input++;
            *output++ = (unsigned char)(c ^ iv[n]);
            iv[n] = (unsigned char)c;

            n = (n + 1) & 0x0F;
        }
    } else {
        while (length--) {
            if (n == 0)
                mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_ENCRYPT, iv, iv);

            iv[n] = *output++ = (unsigned char)(iv[n] ^ *input++);
            n = (n + 1) & 0x0F;
        }
    }

    *iv_off = n;

    return 0;
}

/*
 * AES-CFB8 buffer encryption/decryption
 */
int mbedtls_aes_crypt_cfb8(mbedtls_aes_context *ctx,
                       int mode,
                       size_t length,
                       unsigned char iv[16],
                       const unsigned char *input,
                       unsigned char *output)
{
    unsigned char c;
    unsigned char ov[17];

    while (length--) {
        memcpy(ov, iv, 16);
        mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_ENCRYPT, iv, iv);

        if (mode == MBEDTLS_AES_DECRYPT)
            ov[16] = *input;

        c = *output++ = (unsigned char)(iv[0] ^ *input++);

        if (mode == MBEDTLS_AES_ENCRYPT)
            ov[16] = c;

        memcpy(iv, ov + 1, 16);
    }

    return 0;
}
#endif /*MBEDTLS_CIPHER_MODE_CFB */

#if defined(MBEDTLS_CIPHER_MODE_CTR)
/*
 * AES-CTR buffer encryption/decryption
 */
int mbedtls_aes_crypt_ctr(mbedtls_aes_context *ctx,
                       size_t length,
                       size_t *nc_off,
                       unsigned char nonce_counter[16],
                       unsigned char stream_block[16],
                       const unsigned char *input,
                       unsigned char *output)
{
    int c, i;
    size_t n = *nc_off;

    while (length--) {
        if (n == 0) {
            mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_ENCRYPT, nonce_counter, stream_block);

            for (i = 16; i > 0; i--)
                if (++nonce_counter[i - 1] != 0)
                    break;
        }
        c = *input++;
        *output++ = (unsigned char)(c ^ stream_block[n]);

        n = (n + 1) & 0x0F;
    }

    *nc_off = n;

    return( 0 );
}
#endif /* MBEDTLS_CIPHER_MODE_CTR */
#if defined(MBEDTLS_CIPHER_MODE_OFB)
/*
 * AES-OFB (Output Feedback Mode) buffer encryption/decryption
 */
int mbedtls_aes_crypt_ofb(mbedtls_aes_context *ctx,
                           size_t length,
                           size_t *iv_off,
                           unsigned char iv[16],
                           const unsigned char *input,
                           unsigned char *output)
{
    int ret = 0;
    size_t n;

    n = *iv_off;

    if (n > 15)
        return MBEDTLS_ERR_AES_BAD_INPUT_DATA;

    while (length--) {
        if (n == 0) {
            ret = mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_ENCRYPT, iv, iv);
            if (ret != 0)
                goto exit;
        }
        *output++ = *input++ ^ iv[n];

        n = (n + 1) & 0x0F;
    }

    *iv_off = n;

exit:
    return ret;
}
#endif /* MBEDTLS_CIPHER_MODE_OFB */

#endif /* MBEDTLS_AES_ALT */

#endif /* MBEDTLS_AES_C */
