/*!
    \file    des_alt.c
    \brief   DES block cipher for GD32VW55x SDK.

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

#if defined(MBEDTLS_DES_C)

#include "mbedtls/des.h"

#include <string.h>

#if defined(MBEDTLS_DES_ALT)
#include "gd32vw55x_cau.h"

/* Implementation that should never be optimized out by the compiler */
static void mbedtls_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = (unsigned char*)v; while( n-- ) *p++ = 0;
}

void mbedtls_des_init( mbedtls_des_context *ctx )
{
    memset( ctx, 0, sizeof( mbedtls_des_context ) );
}

void mbedtls_des_free( mbedtls_des_context *ctx )
{
    if( ctx == NULL )
        return;

    mbedtls_zeroize( ctx, sizeof( mbedtls_des_context ) );
}

void mbedtls_des3_init( mbedtls_des3_context *ctx )
{
    memset( ctx, 0, sizeof( mbedtls_des3_context ) );
}

void mbedtls_des3_free( mbedtls_des3_context *ctx )
{
    if( ctx == NULL )
        return;

    mbedtls_zeroize( ctx, sizeof( mbedtls_des3_context ) );
}
static const unsigned char odd_parity_table[128] = { 1,  2,  4,  7,  8,
        11, 13, 14, 16, 19, 21, 22, 25, 26, 28, 31, 32, 35, 37, 38, 41, 42, 44,
        47, 49, 50, 52, 55, 56, 59, 61, 62, 64, 67, 69, 70, 73, 74, 76, 79, 81,
        82, 84, 87, 88, 91, 93, 94, 97, 98, 100, 103, 104, 107, 109, 110, 112,
        115, 117, 118, 121, 122, 124, 127, 128, 131, 133, 134, 137, 138, 140,
        143, 145, 146, 148, 151, 152, 155, 157, 158, 161, 162, 164, 167, 168,
        171, 173, 174, 176, 179, 181, 182, 185, 186, 188, 191, 193, 194, 196,
        199, 200, 203, 205, 206, 208, 211, 213, 214, 217, 218, 220, 223, 224,
        227, 229, 230, 233, 234, 236, 239, 241, 242, 244, 247, 248, 251, 253,
        254 };

void mbedtls_des_key_set_parity( unsigned char key[MBEDTLS_DES_KEY_SIZE] )
{
    int i;

    for( i = 0; i < MBEDTLS_DES_KEY_SIZE; i++ )
        key[i] = odd_parity_table[key[i] / 2];
}

/*
 * DES key schedule (56-bit, encryption)
 */
int mbedtls_des_setkey_enc( mbedtls_des_context *ctx, const unsigned char key[MBEDTLS_DES_KEY_SIZE] )
{
    ctx->mode = MBEDTLS_DES_ENCRYPT;
    memcpy(ctx->key, key, MBEDTLS_DES_KEY_SIZE);

    return( 0 );
}

/*
 * DES key schedule (56-bit, decryption)
 */
int mbedtls_des_setkey_dec( mbedtls_des_context *ctx, const unsigned char key[MBEDTLS_DES_KEY_SIZE] )
{
    ctx->mode = MBEDTLS_DES_DECRYPT;
    memcpy(ctx->key, key, MBEDTLS_DES_KEY_SIZE);

    return( 0 );
}

/*
 * Triple-DES key schedule (112-bit, encryption)
 */
int mbedtls_des3_set2key_enc( mbedtls_des3_context *ctx,
                      const unsigned char key[MBEDTLS_DES_KEY_SIZE * 2] )
{
    ctx->mode = MBEDTLS_DES_ENCRYPT;
    memcpy(ctx->key, key, (MBEDTLS_DES_KEY_SIZE * 2));
    memcpy(&(ctx->key[16]), key, MBEDTLS_DES_KEY_SIZE);

    return( 0 );
}

/*
 * Triple-DES key schedule (112-bit, decryption)
 */
int mbedtls_des3_set2key_dec( mbedtls_des3_context *ctx,
                      const unsigned char key[MBEDTLS_DES_KEY_SIZE * 2] )
{
    ctx->mode = MBEDTLS_DES_DECRYPT;
    memcpy(ctx->key, key, (MBEDTLS_DES_KEY_SIZE * 2));
    memcpy(&(ctx->key[16]), key, MBEDTLS_DES_KEY_SIZE);

    return( 0 );
}

/*
 * Triple-DES key schedule (168-bit, encryption)
 */
int mbedtls_des3_set3key_enc( mbedtls_des3_context *ctx,
                      const unsigned char key[MBEDTLS_DES_KEY_SIZE * 3] )
{
    ctx->mode = MBEDTLS_DES_ENCRYPT;
    memcpy(ctx->key, key, (MBEDTLS_DES_KEY_SIZE * 3));

    return( 0 );
}

/*
 * Triple-DES key schedule (168-bit, decryption)
 */
int mbedtls_des3_set3key_dec( mbedtls_des3_context *ctx,
                      const unsigned char key[MBEDTLS_DES_KEY_SIZE * 3] )
{
    ctx->mode = MBEDTLS_DES_DECRYPT;
    memcpy(ctx->key, key, (MBEDTLS_DES_KEY_SIZE * 3));

    return( 0 );
}

/*
 * DES-ECB block encryption/decryption
 */
int mbedtls_des_crypt_ecb( mbedtls_des_context *ctx,
                    const unsigned char input[8],
                    unsigned char output[8] )
{
    ErrStatus ret = ERROR;
    cau_parameter_struct cau_ecb_parameter;

    switch (ctx->mode) {
    case MBEDTLS_DES_ENCRYPT:
        cau_ecb_parameter.alg_dir = CAU_ENCRYPT;
        break;
    case MBEDTLS_DES_DECRYPT:
        cau_ecb_parameter.alg_dir = CAU_DECRYPT;
        break;
    default:
        break;
    }
    cau_ecb_parameter.key = ctx->key;
    cau_ecb_parameter.input = (uint8_t *)input;
    cau_ecb_parameter.in_length = 8;

    //GLOBAL_INT_DISABLE();
    ret = cau_des_ecb(&cau_ecb_parameter, output);
    //GLOBAL_INT_RESTORE();

    return (ret == ERROR) ? 1 : 0;
}

#if defined(MBEDTLS_CIPHER_MODE_CBC)
/*
 * DES-CBC buffer encryption/decryption
 */
int mbedtls_des_crypt_cbc( mbedtls_des_context *ctx,
                    int mode,
                    size_t length,
                    unsigned char iv[8],
                    const unsigned char *input,
                    unsigned char *output )
{
    unsigned char temp[8];
    cau_parameter_struct cau_cbc_parameter;
    ErrStatus ret = ERROR;

    switch (mode) {
    case MBEDTLS_DES_ENCRYPT:
        cau_cbc_parameter.alg_dir = CAU_ENCRYPT;
        break;
    case MBEDTLS_DES_DECRYPT:
        cau_cbc_parameter.alg_dir = CAU_DECRYPT;
        break;
    default:
        break;
    }

    cau_cbc_parameter.key = ctx->key;
    cau_cbc_parameter.iv = iv;
    cau_cbc_parameter.iv_size = 8;
    cau_cbc_parameter.input = (uint8_t *)input;
    cau_cbc_parameter.in_length = length;

    memcpy(temp, (input + length - 8), 8);
    //GLOBAL_INT_DISABLE();
    ret = cau_des_cbc(&cau_cbc_parameter, output);
    if(mode == MBEDTLS_DES_DECRYPT)
        memcpy(iv, temp, 8);
    else
        memcpy(iv, (output + length - 8), 8);
    //GLOBAL_INT_RESTORE();

    return (ret == ERROR) ? 1 : 0;
}
#endif /* MBEDTLS_CIPHER_MODE_CBC */

/*
 * 3DES-ECB block encryption/decryption
 */
int mbedtls_des3_crypt_ecb( mbedtls_des3_context *ctx,
                     const unsigned char input[8],
                     unsigned char output[8] )
{
    cau_parameter_struct cau_tdes_parameter;
    ErrStatus ret = ERROR;

    switch (ctx->mode) {
    case MBEDTLS_DES_ENCRYPT:
        cau_tdes_parameter.alg_dir = CAU_ENCRYPT;
        break;
    case MBEDTLS_DES_DECRYPT:
        cau_tdes_parameter.alg_dir = CAU_DECRYPT;
        break;
    default:
        break;
    }

    cau_tdes_parameter.key = ctx->key;
    cau_tdes_parameter.input = (uint8_t *)input;
    cau_tdes_parameter.in_length = 8;

    //GLOBAL_INT_DISABLE();
    ret = cau_tdes_ecb(&cau_tdes_parameter, output);
    //GLOBAL_INT_RESTORE();

    return (ret == ERROR) ? 1 : 0;
}

#if defined(MBEDTLS_CIPHER_MODE_CBC)
/*
 * 3DES-CBC buffer encryption/decryption
 */
int mbedtls_des3_crypt_cbc( mbedtls_des3_context *ctx,
                     int mode,
                     size_t length,
                     unsigned char iv[8],
                     const unsigned char *input,
                     unsigned char *output )
{
    unsigned char temp[8];
    ErrStatus ret = ERROR;
    cau_parameter_struct cau_des3_parameter;

    switch (mode) {
    case MBEDTLS_DES_ENCRYPT:
        cau_des3_parameter.alg_dir = CAU_ENCRYPT;
        break;
    case MBEDTLS_DES_DECRYPT:
        cau_des3_parameter.alg_dir = CAU_DECRYPT;
        break;
    default:
        break;
    }

    cau_des3_parameter.key = ctx->key;
    cau_des3_parameter.iv = iv;
    cau_des3_parameter.iv_size = 8;
    cau_des3_parameter.input = (uint8_t *)input;
    cau_des3_parameter.in_length = length;

    memcpy(temp, (input + length - 8), 8);
    //GLOBAL_INT_DISABLE();
    ret = cau_tdes_cbc(&cau_des3_parameter, output);
    if(mode == MBEDTLS_DES_DECRYPT)
        memcpy(iv, temp, 8);
    else
        memcpy(iv, (output + length - 8), 8);
    //GLOBAL_INT_RESTORE();

    return (ret == ERROR) ? 1 : 0;
}
#endif /* MBEDTLS_CIPHER_MODE_CBC */

#endif /* MBEDTLS_DES_ALT */

#endif /* MBEDTLS_DES_C */
