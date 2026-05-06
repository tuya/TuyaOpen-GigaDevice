/*!
    \file    sha256_alt.c
    \brief   SHA256 block cipher implemented for GD32VW55x SDK.

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

#if defined(MBEDTLS_SHA256_C)

#include <string.h>
#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#define mbedtls_printf printf
#endif /* MBEDTLS_PLATFORM_C */

#include "mbedtls/sha256.h"

#if defined(MBEDTLS_SHA256_ALT)
static ErrStatus hau_hash_cal(int is256,uint8_t *input, uint32_t in_length);
static ErrStatus hau_hash_cal_end(int is256, uint8_t *input, uint32_t in_length, uint8_t *output);
static void hau_digest_get(uint32_t algo, uint8_t *output);

/* Implementation that should never be optimized out by the compiler */
static void mbedtls_zeroize(void *v, size_t n)
{
    volatile unsigned char *p = (unsigned char*)v;
    while(n--)
        *p++ = 0;
}

void hau_sha256_context_init(mbedtls_sha256_context *ctx)
{
    uint32_t i;

    ctx->buf_len = 0;
    ctx->is256   = 0;

    for (i = 0; i < SHA256_BLOCK_SIZE; i++) {
        ctx->buf[i] = 0;
    }
    /*initialize the struct context*/
    hau_context_struct_para_init(&(ctx->context_para));
}

void hau_sha256_start(mbedtls_sha256_context *ctx, int is256)
{
    hau_init_parameter_struct init_para;

    /* HAU peripheral initialization */
    hau_deinit();
    /* init mbedtls_sha256_context struct */
    hau_sha256_context_init(ctx);

    /* HAU configuration */
    if (is256) {
        init_para.algo = HAU_ALGO_SHA256;
    } else {
        init_para.algo = HAU_ALGO_SHA224;
    }
    init_para.mode = HAU_MODE_HASH;
    init_para.datatype = HAU_SWAPPING_8BIT;
    hau_init(&init_para);

    ctx->is256 = is256;

    /* save HAU context */
    hau_context_save(&(ctx->context_para));
}

ErrStatus hau_sha256_update(mbedtls_sha256_context *ctx, uint8_t *input, uint32_t in_length)
{
    uint32_t current_len = in_length;
    uint32_t times = 0;

    if (current_len < (SHA256_BLOCK_SIZE - ctx->buf_len)) {
        /* only store input data in context buffer */
        memcpy(ctx->buf + ctx->buf_len, input, current_len);
        ctx->buf_len += current_len;
    } else {
        /* restore HAU context */
        hau_context_restore(&(ctx->context_para));

        /* fill context buffer until 64 bytes, and process it */
        memcpy(ctx->buf + ctx->buf_len, input, (SHA256_BLOCK_SIZE - ctx->buf_len));
        current_len -= (SHA256_BLOCK_SIZE - ctx->buf_len);
        if (SUCCESS != hau_hash_cal(ctx->is256, (uint8_t *)(ctx->buf), SHA256_BLOCK_SIZE)) {
            return ERROR;
        }

        /* Process input data with size multiple of 64 bytes */
        times = current_len / SHA256_BLOCK_SIZE;
        if (times != 0) {
            if (SUCCESS != hau_hash_cal(ctx->is256, (uint8_t *)(input + SHA256_BLOCK_SIZE - ctx->buf_len), (times * SHA256_BLOCK_SIZE))) {
                return ERROR;
            }
        }
        /* save HAU context */
        hau_context_save(&(ctx->context_para));

        /*Store remaining input data to ctx->buf*/
        ctx->buf_len = current_len % SHA256_BLOCK_SIZE;
        if (ctx->buf_len != 0) {
            memcpy(ctx->buf, input + in_length - ctx->buf_len, ctx->buf_len);
        }
    }

    return SUCCESS;
}

ErrStatus hau_sha256_finish(mbedtls_sha256_context *ctx, uint8_t output[32])
{
    /* restore HAU context */
    hau_context_restore(&(ctx->context_para));

    /*Last accumulation for bytes in buf_len,then trig processing and get digest*/
    if (SUCCESS != hau_hash_cal_end(ctx->is256, ctx->buf, ctx->buf_len, output))
        return ERROR;

    ctx->buf_len = 0;

    return SUCCESS;
}

static ErrStatus hau_hash_cal(int is256,uint8_t *input, uint32_t in_length)
{
    uint32_t i = 0U;
    uint32_t inputaddr  = (uint32_t)input;

    if ((in_length == 0) || (input == NULL) ||(in_length % 4U != 0))
        return ERROR;

    /*config hau algorithm*/
    if (is256)
        HAU_CTL |= HAU_ALGO_SHA256;
    else
        HAU_CTL |= HAU_ALGO_SHA224;

    /* write data to the IN FIFO */
    for (i = 0U; i < in_length; i += 4U) {
        hau_data_write(*(uint32_t*)inputaddr);
        inputaddr += 4U;
    }

    return SUCCESS;
}

static ErrStatus hau_hash_cal_end(int is256, uint8_t *input, uint32_t in_length, uint8_t *output)
{
    __IO uint32_t num_last_valid = 0U;
    uint32_t i = 0U;
    uint32_t inputaddr  = (uint32_t)input;
    __IO uint32_t counter = 0U;
    uint32_t busystatus = 0U;
    uint32_t algo;

    if ((input == NULL) || (output == NULL))
        return ERROR;

    if (is256)
        algo = HAU_ALGO_SHA256;
    else
        algo = HAU_ALGO_SHA224;

    /*config hau algorithm*/
    HAU_CTL |= algo;

    /* number of valid bits in last word */
    num_last_valid = 8U * (in_length % 4U);
    /* configure the number of valid bits in last word of the message */
    hau_last_word_validbits_num_config(num_last_valid);

    /* write data to the IN FIFO */
    for (i = 0U; i < in_length; i += 4U) {
        hau_data_write(*(uint32_t*)inputaddr);
        inputaddr += 4U;
    }

    /* enable digest calculation */
    hau_digest_calculation_enable();

    /* wait until the busy flag is reset */
    do {
        busystatus = hau_flag_get(HAU_FLAG_BUSY);
        counter++;
    } while((SHAMD5_BSY_TIMEOUT != counter) && (RESET != busystatus));

    if (RESET != busystatus)
        return ERROR;

    /* read the message digest */
    hau_digest_get(algo, output);
    return SUCCESS;
}

/*!
    \brief      HAU SHA/MD5 digest read
    \param[in]  algo: algorithm selection
    \param[out] output: the result digest
    \retval     none
*/
static void hau_digest_get(uint32_t algo, uint8_t *output)
{
    hau_digest_parameter_struct digest_para;
    uint32_t outputaddr = (uint32_t)output;

    switch(algo) {
    case HAU_ALGO_SHA1:
        hau_digest_read(&digest_para);
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[0]);
        outputaddr += 4U;
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[1]);
        outputaddr += 4U;
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[2]);
        outputaddr += 4U;
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[3]);
        outputaddr += 4U;
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[4]);
        break;
    case HAU_ALGO_SHA224:
        hau_digest_read(&digest_para);
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[0]);
        outputaddr += 4U;
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[1]);
        outputaddr += 4U;
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[2]);
        outputaddr += 4U;
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[3]);
        outputaddr += 4U;
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[4]);
        outputaddr += 4U;
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[5]);
        outputaddr += 4U;
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[6]);
        break;
    case HAU_ALGO_SHA256:
        hau_digest_read(&digest_para);
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[0]);
        outputaddr += 4U;
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[1]);
        outputaddr += 4U;
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[2]);
        outputaddr += 4U;
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[3]);
        outputaddr += 4U;
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[4]);
        outputaddr += 4U;
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[5]);
        outputaddr += 4U;
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[6]);
        outputaddr += 4U;
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[7]);
        break;
    case HAU_ALGO_MD5:
        hau_digest_read(&digest_para);
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[0]);
        outputaddr += 4U;
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[1]);
        outputaddr += 4U;
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[2]);
        outputaddr += 4U;
        *(uint32_t*)(outputaddr)  = __REV(digest_para.out[3]);
        break;
    default:
        break;
    }
}

int mbedtls_internal_sha256_process( mbedtls_sha256_context *ctx, const unsigned char data[SHA256_BLOCK_SIZE] )
{
    int ret = SUCCESS;

    //GLOBAL_INT_DISABLE();
    /* restore HAU context */
    hau_context_restore(&(ctx->context_para));
    if (SUCCESS != hau_hash_cal(ctx->is256, (uint8_t *)(data), SHA256_BLOCK_SIZE)) {
        ret = ERROR;
        goto Exit;
    }
    /* save HAU context */
    hau_context_save(&(ctx->context_para));
Exit:
    //GLOBAL_INT_RESTORE();
    return ret;
}

void mbedtls_sha256_init(mbedtls_sha256_context *ctx)
{
    hau_sha256_context_init(ctx);
}

void mbedtls_sha256_free(mbedtls_sha256_context *ctx)
{
    if(ctx == NULL)
        return;

    mbedtls_zeroize(ctx, sizeof(mbedtls_sha256_context));
}

void mbedtls_sha256_clone(mbedtls_sha256_context *dst,
                           const mbedtls_sha256_context *src)
{
    *dst = *src;
}

/*
 * SHA-256 context setup
 */
int mbedtls_sha256_starts_ret(mbedtls_sha256_context *ctx, int is224)
{
    int is256 = 0;

    if (!is224)
        is256 = 1;

    // GLOBAL_INT_DISABLE();
    hau_sha256_start(ctx, is256);
    // GLOBAL_INT_RESTORE();

    return 0;
}


#if !defined(MBEDTLS_DEPRECATED_REMOVED)
int mbedtls_sha256_starts(mbedtls_sha256_context *ctx,
                            int is224)
{
    return mbedtls_sha256_starts_ret(ctx, is224);
}
#endif

/*
 * SHA-256 process buffer
 */
int mbedtls_sha256_update_ret(mbedtls_sha256_context *ctx,
                               const unsigned char *input,
                               size_t ilen)
{
    ErrStatus ret = ERROR;

    // GLOBAL_INT_DISABLE();
    ret = hau_sha256_update(ctx, (uint8_t *)input, ilen);
    return (ret == ERROR) ? 1 : 0;
    // GLOBAL_INT_RESTORE();
}

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
int mbedtls_sha256_update(mbedtls_sha256_context *ctx,
                            const unsigned char *input,
                            size_t ilen)
{
    return mbedtls_sha256_update_ret(ctx, (uint8_t *)input, ilen);
}
#endif

/*
 * SHA-256 final digest
 */
int mbedtls_sha256_finish_ret(mbedtls_sha256_context *ctx,
                               unsigned char output[32])
{
    ErrStatus ret = ERROR;

    // GLOBAL_INT_DISABLE();
    ret = hau_sha256_finish(ctx, (uint8_t *)output);
    return (ret == ERROR) ? 1 : 0;
    // GLOBAL_INT_RESTORE();

//    return(0);
}

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
int mbedtls_sha256_finish(mbedtls_sha256_context *ctx,
                            unsigned char output[32])
{
    return mbedtls_sha256_finish_ret(ctx, output);
}
#endif

#endif /* MBEDTLS_SHA256_ALT */
#endif /* MBEDTLS_SHA256_C */
