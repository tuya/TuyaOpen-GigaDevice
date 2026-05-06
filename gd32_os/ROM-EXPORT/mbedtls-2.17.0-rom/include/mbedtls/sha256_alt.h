/**
 * \file sha256_alt.h
 *
 * \brief SHA256 block cipher implemented by Freethink
 *
 */
#ifndef MBEDTLS_SHA256_ALT_H
#define MBEDTLS_SHA256_ALT_H

#if defined(MBEDTLS_SHA256_ALT)
#include "gd32vw55x.h"

#define SHAMD5_BSY_TIMEOUT    ((uint32_t)0x00010000U)
#define SHA256_BLOCK_SIZE     ((uint32_t)  64)        /*!< HAU BLOCK_SIZE 512 bits, ie 64 bytes */
/**
 * \brief          SHA-256 context structure
 */
typedef struct mbedtls_sha256_context
{
    int is256;                                      /*!< 1 = use SHA256, 0 = use SHA224 */
    uint8_t buf[64];                               /*!< Buffer to store input data until SHA256_BLOCK_SIZE
                                                         is reached, or until last input data is reached */
    uint8_t buf_len;                               /*!< Number of bytes stored in sbuf */
    hau_context_parameter_struct  context_para;     /* structure for context switch */
}
mbedtls_sha256_context;


/*init mbedtls_sha256_context struct*/
void hau_sha256_context_init(mbedtls_sha256_context *ct);
/*interface function*/
void hau_sha256_start(mbedtls_sha256_context *ct, int is256);
ErrStatus hau_sha256_update(mbedtls_sha256_context *ct,uint8_t *input, uint32_t in_length);
ErrStatus hau_sha256_finish(mbedtls_sha256_context *ct,uint8_t output[32]);
/*process input data then read digest*/

#endif //MBEDTLS_SHA256_ALT
#endif //MBEDTLS_SHA256_ALT_H
