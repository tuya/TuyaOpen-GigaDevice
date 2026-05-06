/*
 *  SSL client demonstration program
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

#include "mbedtls/config.h"

#define SSL_CLIENT_TASK_STK_SIZE               3072  /* 3072 */
#define SSL_CLIENT_TASK_PRIO                   OS_TASK_PRIORITY(2)

#include "wrapper_os.h"

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#include <stdlib.h>
#define mbedtls_time       time
#define mbedtls_time_t     time_t
#define mbedtls_fprintf    fprintf
#define mbedtls_printf     printf
#endif
#undef mbedtls_printf
#define mbedtls_printf     printf

#define MBEDTLS_SELF_TEST_OUT

#if defined(MBEDTLS_SELF_TEST_OUT)
#define MBEDTLS_SELF_TEST
#endif

//#if !defined(MBEDTLS_BIGNUM_C) || !defined(MBEDTLS_ENTROPY_C) || !defined(MBEDTLS_CERTS_C) ||
#if !defined(MBEDTLS_BIGNUM_C) || \
    !defined(MBEDTLS_SSL_TLS_C) || !defined(MBEDTLS_SSL_CLI_C) || \
    !defined(MBEDTLS_NET_C) || !defined(MBEDTLS_RSA_C) ||         \
    !defined(MBEDTLS_PEM_PARSE_C) || \
    !defined(MBEDTLS_CTR_DRBG_C) || !defined(MBEDTLS_X509_CRT_PARSE_C)
int ssl_self_test( void )
{
    mbedtls_printf("MBEDTLS_BIGNUM_C and/or MBEDTLS_ENTROPY_C and/or "
           "MBEDTLS_SSL_TLS_C and/or MBEDTLS_SSL_CLI_C and/or "
           "MBEDTLS_NET_C and/or MBEDTLS_RSA_C and/or "
           "MBEDTLS_CTR_DRBG_C and/or MBEDTLS_X509_CRT_PARSE_C "
           "not defined.\r\n");
    return( 0 );
}
#else

#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/certs.h"
#include "mbedtls/entropy.h"
#include "mbedtls/entropy_poll.h"
#include "mbedtls/hmac_drbg.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/dhm.h"
#include "mbedtls/gcm.h"
#include "mbedtls/ccm.h"
#include "mbedtls/cmac.h"
#include "mbedtls/md2.h"
#include "mbedtls/md4.h"
#include "mbedtls/md5.h"
#include "mbedtls/ripemd160.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/arc4.h"
#include "mbedtls/des.h"
#include "mbedtls/aes.h"
#include "mbedtls/camellia.h"
#include "mbedtls/base64.h"
#include "mbedtls/bignum.h"
#include "mbedtls/rsa.h"
#include "mbedtls/x509.h"
#include "mbedtls/xtea.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/ecp.h"
#include "mbedtls/ecjpake.h"
#include "mbedtls/timing.h"

#define mbedtls_hex_dump(title, buf, len) \
    do { \
        int i; \
        mbedtls_printf("\r\n=== %s (len=%d)===", title, len); \
        for (i = 0; i < len; i++) { \
            if (i % 16 == 0) \
                mbedtls_printf("\\\r\n"); \
            mbedtls_printf("0x%02x, ", *((uint8_t*)buf + i)); \
        } \
        mbedtls_printf("\r\n");\
    } while(0);

static void dump_buf( const char *title, unsigned char *buf, size_t len )
{
   size_t i;

   mbedtls_printf( "%s", title );
   for( i = 0; i < len; i++ )
       mbedtls_printf("%c%c", "0123456789ABCDEF" [buf[i] / 16],
                      "0123456789ABCDEF" [buf[i] % 16] );
   mbedtls_printf( "\n" );
}

static void dump_pubkey( const char *title, mbedtls_ecdsa_context *key7 )
{
   unsigned char buf[300];
   size_t len;

   if( mbedtls_ecp_point_write_binary( &key7->grp, &key7->Q,
               MBEDTLS_ECP_PF_UNCOMPRESSED, &len, buf, sizeof buf ) != 0 )
   {
       mbedtls_printf("internal error\n");
       return;
   }

   dump_buf( title, buf, len );
}

#ifdef CONFIG_HW_SECURITY_ENGINE
extern int mbedtls_mpi_exp_mod_self_test_512( int verbose );
extern int mbedtls_mpi_exp_mod_self_test_1024( int verbose );
#endif

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_SHA1_C)

/*
 * FIPS-180-1 test vectors
 */
static const unsigned char sha1_test_buf[3][57] =
{
    { "abc" },
    { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" },
    { "" }
};

static const size_t sha1_test_buflen[3] =
{
    3, 56, 1000
};

static const unsigned char sha1_test_sum[3][20] =
{
    { 0xA9, 0x99, 0x3E, 0x36, 0x47, 0x06, 0x81, 0x6A, 0xBA, 0x3E,
      0x25, 0x71, 0x78, 0x50, 0xC2, 0x6C, 0x9C, 0xD0, 0xD8, 0x9D },
    { 0x84, 0x98, 0x3E, 0x44, 0x1C, 0x3B, 0xD2, 0x6E, 0xBA, 0xAE,
      0x4A, 0xA1, 0xF9, 0x51, 0x29, 0xE5, 0xE5, 0x46, 0x70, 0xF1 },
    { 0x34, 0xAA, 0x97, 0x3C, 0xD4, 0xC4, 0xDA, 0xA4, 0xF6, 0x1E,
      0xEB, 0x2B, 0xDB, 0xAD, 0x27, 0x31, 0x65, 0x34, 0x01, 0x6F }
};

/*
 * Checkup routine
 */
int mbedtls_sha1_self_test( int verbose )
{
    int i, j, buflen, ret = 0;
    unsigned char buf[1024];
    unsigned char sha1sum[20];
    mbedtls_sha1_context ctx;

    mbedtls_sha1_init( &ctx );

    /*
     * SHA-1
     */
    for( i = 0; i < 3; i++ )
    {
        if( verbose != 0 )
            mbedtls_printf( "  SHA-1 test #%d: ", i + 1 );

#ifdef CONFIG_HW_SECURITY_ENGINE
        if (i == 2) {
            if( ( ret = mbedtls_sha1_starts_ret( &ctx ) ) != 0 )
                goto fail;
            memset( buf, 'a', buflen = 1000 );
            for( j = 0; j < 1000; j++ ) {
                ret = mbedtls_sha1_update_ret( &ctx, buf, buflen );
                if( ret != 0 )
                    goto fail;
            }
            ret = mbedtls_sha1_finish_ret( &ctx, sha1sum );
            if( ret != 0 )
                goto fail;
        } else {
            //GLOBAL_INT_DISABLE();
            ret = hau_hash_sha_1((unsigned char *)sha1_test_buf[i], sha1_test_buflen[i], sha1sum);
            ret = (ret == ERROR)? 1 : 0;
            //GLOBAL_INT_RESTORE();
        }
#else
        if( ( ret = mbedtls_sha1_starts_ret( &ctx ) ) != 0 )
            goto fail;

        if( i == 2 )
        {
            memset( buf, 'a', buflen = 1000 );

            for( j = 0; j < 1000; j++ )
            {
                ret = mbedtls_sha1_update_ret( &ctx, buf, buflen );
                if( ret != 0 )
                    goto fail;
            }
        }
        else
        {
            ret = mbedtls_sha1_update_ret( &ctx, sha1_test_buf[i],
                                           sha1_test_buflen[i] );
            if( ret != 0 )
                goto fail;
        }

        if( ( ret = mbedtls_sha1_finish_ret( &ctx, sha1sum ) ) != 0 )
            goto fail;
#endif
        if( memcmp( sha1sum, sha1_test_sum[i], 20 ) != 0 )
        {
            ret = 1;
            goto fail;
        }

        if( verbose != 0 )
            mbedtls_printf( "passed\n" );
    }

    if( verbose != 0 )
        mbedtls_printf( "\n" );

    goto exit;

fail:
    if( verbose != 0 )
        mbedtls_printf( "failed\n" );

exit:
    mbedtls_sha1_free( &ctx );

    return( ret );
}

#endif /* MBEDTLS_SELF_TEST_OUT */

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_SHA1_C) && defined(MBEDTLS_HMAC_DRBG_C)

#if !defined(MBEDTLS_SHA1_C)
/* Dummy checkup routine */
int mbedtls_hmac_drbg_self_test( int verbose )
{
    (void) verbose;
    return( 0 );
}
#else

#define OUTPUT_LEN  80

/* From a NIST PR=true test vector */
static const unsigned char entropy_pr[] = {
    0xa0, 0xc9, 0xab, 0x58, 0xf1, 0xe2, 0xe5, 0xa4, 0xde, 0x3e, 0xbd, 0x4f,
    0xf7, 0x3e, 0x9c, 0x5b, 0x64, 0xef, 0xd8, 0xca, 0x02, 0x8c, 0xf8, 0x11,
    0x48, 0xa5, 0x84, 0xfe, 0x69, 0xab, 0x5a, 0xee, 0x42, 0xaa, 0x4d, 0x42,
    0x17, 0x60, 0x99, 0xd4, 0x5e, 0x13, 0x97, 0xdc, 0x40, 0x4d, 0x86, 0xa3,
    0x7b, 0xf5, 0x59, 0x54, 0x75, 0x69, 0x51, 0xe4 };
static const unsigned char result_pr_1[OUTPUT_LEN] = {
    0x9a, 0x00, 0xa2, 0xd0, 0x0e, 0xd5, 0x9b, 0xfe, 0x31, 0xec, 0xb1, 0x39,
    0x9b, 0x60, 0x81, 0x48, 0xd1, 0x96, 0x9d, 0x25, 0x0d, 0x3c, 0x1e, 0x94,
    0x10, 0x10, 0x98, 0x12, 0x93, 0x25, 0xca, 0xb8, 0xfc, 0xcc, 0x2d, 0x54,
    0x73, 0x19, 0x70, 0xc0, 0x10, 0x7a, 0xa4, 0x89, 0x25, 0x19, 0x95, 0x5e,
    0x4b, 0xc6, 0x00, 0x1d, 0x7f, 0x4e, 0x6a, 0x2b, 0xf8, 0xa3, 0x01, 0xab,
    0x46, 0x05, 0x5c, 0x09, 0xa6, 0x71, 0x88, 0xf1, 0xa7, 0x40, 0xee, 0xf3,
    0xe1, 0x5c, 0x02, 0x9b, 0x44, 0xaf, 0x03, 0x44 };

/* From a NIST PR=false test vector */
static const unsigned char entropy_nopr[] = {
    0x79, 0x34, 0x9b, 0xbf, 0x7c, 0xdd, 0xa5, 0x79, 0x95, 0x57, 0x86, 0x66,
    0x21, 0xc9, 0x13, 0x83, 0x11, 0x46, 0x73, 0x3a, 0xbf, 0x8c, 0x35, 0xc8,
    0xc7, 0x21, 0x5b, 0x5b, 0x96, 0xc4, 0x8e, 0x9b, 0x33, 0x8c, 0x74, 0xe3,
    0xe9, 0x9d, 0xfe, 0xdf };
static const unsigned char result_nopr[OUTPUT_LEN] = {
    0xc6, 0xa1, 0x6a, 0xb8, 0xd4, 0x20, 0x70, 0x6f, 0x0f, 0x34, 0xab, 0x7f,
    0xec, 0x5a, 0xdc, 0xa9, 0xd8, 0xca, 0x3a, 0x13, 0x3e, 0x15, 0x9c, 0xa6,
    0xac, 0x43, 0xc6, 0xf8, 0xa2, 0xbe, 0x22, 0x83, 0x4a, 0x4c, 0x0a, 0x0a,
    0xff, 0xb1, 0x0d, 0x71, 0x94, 0xf1, 0xc1, 0xa5, 0xcf, 0x73, 0x22, 0xec,
    0x1a, 0xe0, 0x96, 0x4e, 0xd4, 0xbf, 0x12, 0x27, 0x46, 0xe0, 0x87, 0xfd,
    0xb5, 0xb3, 0xe9, 0x1b, 0x34, 0x93, 0xd5, 0xbb, 0x98, 0xfa, 0xed, 0x49,
    0xe8, 0x5f, 0x13, 0x0f, 0xc8, 0xa4, 0x59, 0xb7 };

/* "Entropy" from buffer */
static size_t test_offset;
static int hmac_drbg_self_test_entropy( void *data,
                                        unsigned char *buf, size_t len )
{
    const unsigned char *p = data;
    memcpy( buf, p + test_offset, len );
    test_offset += len;
    return( 0 );
}

#define CHK( c )    if( (c) != 0 )                          \
                    {                                       \
                        if( verbose != 0 )                  \
                            mbedtls_printf( "failed\n" );  \
                        return( 1 );                        \
                    }

/*
 * Checkup routine for HMAC_DRBG with SHA-1
 */
int mbedtls_hmac_drbg_self_test( int verbose )
{
    mbedtls_hmac_drbg_context ctx;
    unsigned char buf[OUTPUT_LEN];
    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type( MBEDTLS_MD_SHA1 );

    mbedtls_hmac_drbg_init( &ctx );

    /*
     * PR = True
     */
    if( verbose != 0 )
        mbedtls_printf( "  HMAC_DRBG (PR = True) : " );

    test_offset = 0;
    CHK( mbedtls_hmac_drbg_seed( &ctx, md_info,
                         hmac_drbg_self_test_entropy, (void *) entropy_pr,
                         NULL, 0 ) );
    mbedtls_hmac_drbg_set_prediction_resistance( &ctx, MBEDTLS_HMAC_DRBG_PR_ON );
    CHK( mbedtls_hmac_drbg_random( &ctx, buf, OUTPUT_LEN ) );
    CHK( mbedtls_hmac_drbg_random( &ctx, buf, OUTPUT_LEN ) );
    CHK( memcmp( buf, result_pr_1, OUTPUT_LEN ) );
    mbedtls_hmac_drbg_free( &ctx );

    mbedtls_hmac_drbg_free( &ctx );

    if( verbose != 0 )
        mbedtls_printf( "passed\n" );

    /*
     * PR = False
     */
    if( verbose != 0 )
        mbedtls_printf( "  HMAC_DRBG (PR = False) : " );

    mbedtls_hmac_drbg_init( &ctx );

    test_offset = 0;
    CHK( mbedtls_hmac_drbg_seed( &ctx, md_info,
                         hmac_drbg_self_test_entropy, (void *) entropy_nopr,
                         NULL, 0 ) );
    CHK( mbedtls_hmac_drbg_reseed( &ctx, NULL, 0 ) );
    CHK( mbedtls_hmac_drbg_random( &ctx, buf, OUTPUT_LEN ) );
    CHK( mbedtls_hmac_drbg_random( &ctx, buf, OUTPUT_LEN ) );
    CHK( memcmp( buf, result_nopr, OUTPUT_LEN ) );
    mbedtls_hmac_drbg_free( &ctx );

    mbedtls_hmac_drbg_free( &ctx );

    if( verbose != 0 )
        mbedtls_printf( "passed\n" );

    if( verbose != 0 )
        mbedtls_printf( "\n" );

    return( 0 );
}
#endif /* MBEDTLS_SHA1_C */
#endif /* MBEDTLS_SELF_TEST_OUT && MBEDTLS_SHA1_C && MBEDTLS_HMAC_DRBG_C */

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_SHA256_C)

/*
 * FIPS-180-2 test vectors
 */
static const unsigned char sha256_test_buf[3][57] =
{
    { "abc" },
    { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" },
    { "" }
};

static const int sha256_test_buflen[3] =
{
    3, 56, 1000
};

static const unsigned char sha256_test_sum[6][32] =
{
    /*
     * SHA-224 test vectors
     */
    { 0x23, 0x09, 0x7D, 0x22, 0x34, 0x05, 0xD8, 0x22,
      0x86, 0x42, 0xA4, 0x77, 0xBD, 0xA2, 0x55, 0xB3,
      0x2A, 0xAD, 0xBC, 0xE4, 0xBD, 0xA0, 0xB3, 0xF7,
      0xE3, 0x6C, 0x9D, 0xA7 },
    { 0x75, 0x38, 0x8B, 0x16, 0x51, 0x27, 0x76, 0xCC,
      0x5D, 0xBA, 0x5D, 0xA1, 0xFD, 0x89, 0x01, 0x50,
      0xB0, 0xC6, 0x45, 0x5C, 0xB4, 0xF5, 0x8B, 0x19,
      0x52, 0x52, 0x25, 0x25 },
    { 0x20, 0x79, 0x46, 0x55, 0x98, 0x0C, 0x91, 0xD8,
      0xBB, 0xB4, 0xC1, 0xEA, 0x97, 0x61, 0x8A, 0x4B,
      0xF0, 0x3F, 0x42, 0x58, 0x19, 0x48, 0xB2, 0xEE,
      0x4E, 0xE7, 0xAD, 0x67 },

    /*
     * SHA-256 test vectors
     */
    { 0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA,
      0x41, 0x41, 0x40, 0xDE, 0x5D, 0xAE, 0x22, 0x23,
      0xB0, 0x03, 0x61, 0xA3, 0x96, 0x17, 0x7A, 0x9C,
      0xB4, 0x10, 0xFF, 0x61, 0xF2, 0x00, 0x15, 0xAD },
    { 0x24, 0x8D, 0x6A, 0x61, 0xD2, 0x06, 0x38, 0xB8,
      0xE5, 0xC0, 0x26, 0x93, 0x0C, 0x3E, 0x60, 0x39,
      0xA3, 0x3C, 0xE4, 0x59, 0x64, 0xFF, 0x21, 0x67,
      0xF6, 0xEC, 0xED, 0xD4, 0x19, 0xDB, 0x06, 0xC1 },
    { 0xCD, 0xC7, 0x6E, 0x5C, 0x99, 0x14, 0xFB, 0x92,
      0x81, 0xA1, 0xC7, 0xE2, 0x84, 0xD7, 0x3E, 0x67,
      0xF1, 0x80, 0x9A, 0x48, 0xA4, 0x97, 0x20, 0x0E,
      0x04, 0x6D, 0x39, 0xCC, 0xC7, 0x11, 0x2C, 0xD0 }
};

/*
 * Checkup routine
 */
int mbedtls_sha256_self_test( int verbose )
{
    int i, j, k, buflen, ret = 0;
    unsigned char *buf;
    unsigned char sha256sum[32];
    mbedtls_sha256_context ctx;

    buf = mbedtls_calloc( 1024, sizeof(unsigned char) );
    if( NULL == buf )
    {
        if( verbose != 0 )
            mbedtls_printf( "Buffer allocation failed\n" );

        return( 1 );
    }

    mbedtls_sha256_init( &ctx );

    for( i = 0; i < 6; i++ )
    {
        j = i % 3;
        k = i < 3;

        if( verbose != 0 )
            mbedtls_printf( "  SHA-%d test #%d: ", 256 - k * 32, j + 1 );

        mbedtls_sha256_starts( &ctx, k );

        if( j == 2 )
        {
            memset( buf, 'a', buflen = 1000 );

            for( j = 0; j < 1000; j++ )
                mbedtls_sha256_update( &ctx, buf, buflen );
        }
        else
            mbedtls_sha256_update( &ctx, sha256_test_buf[j],
                                 sha256_test_buflen[j] );

        mbedtls_sha256_finish( &ctx, sha256sum );

        if( memcmp( sha256sum, sha256_test_sum[i], 32 - k * 4 ) != 0 )
        {
            if( verbose != 0 )
                mbedtls_printf( "failed\n" );

            ret = 1;
            goto exit;
        }

        if( verbose != 0 )
            mbedtls_printf( "passed\n" );
    }

    if( verbose != 0 )
        mbedtls_printf( "\n" );

exit:
    mbedtls_sha256_free( &ctx );
    mbedtls_free( buf );

    return( ret );
}

#endif /* MBEDTLS_SELF_TEST_OUT && MBEDTLS_SHA256_C */

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_SHA512_C)

/*
 * FIPS-180-2 test vectors
 */
static const unsigned char sha512_test_buf[3][113] =
{
    { "abc" },
    { "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
      "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu" },
    { "" }
};

static const size_t sha512_test_buflen[3] =
{
    3, 112, 1000
};

static const unsigned char sha512_test_sum[6][64] =
{
    /*
     * SHA-384 test vectors
     */
    { 0xCB, 0x00, 0x75, 0x3F, 0x45, 0xA3, 0x5E, 0x8B,
      0xB5, 0xA0, 0x3D, 0x69, 0x9A, 0xC6, 0x50, 0x07,
      0x27, 0x2C, 0x32, 0xAB, 0x0E, 0xDE, 0xD1, 0x63,
      0x1A, 0x8B, 0x60, 0x5A, 0x43, 0xFF, 0x5B, 0xED,
      0x80, 0x86, 0x07, 0x2B, 0xA1, 0xE7, 0xCC, 0x23,
      0x58, 0xBA, 0xEC, 0xA1, 0x34, 0xC8, 0x25, 0xA7 },
    { 0x09, 0x33, 0x0C, 0x33, 0xF7, 0x11, 0x47, 0xE8,
      0x3D, 0x19, 0x2F, 0xC7, 0x82, 0xCD, 0x1B, 0x47,
      0x53, 0x11, 0x1B, 0x17, 0x3B, 0x3B, 0x05, 0xD2,
      0x2F, 0xA0, 0x80, 0x86, 0xE3, 0xB0, 0xF7, 0x12,
      0xFC, 0xC7, 0xC7, 0x1A, 0x55, 0x7E, 0x2D, 0xB9,
      0x66, 0xC3, 0xE9, 0xFA, 0x91, 0x74, 0x60, 0x39 },
    { 0x9D, 0x0E, 0x18, 0x09, 0x71, 0x64, 0x74, 0xCB,
      0x08, 0x6E, 0x83, 0x4E, 0x31, 0x0A, 0x4A, 0x1C,
      0xED, 0x14, 0x9E, 0x9C, 0x00, 0xF2, 0x48, 0x52,
      0x79, 0x72, 0xCE, 0xC5, 0x70, 0x4C, 0x2A, 0x5B,
      0x07, 0xB8, 0xB3, 0xDC, 0x38, 0xEC, 0xC4, 0xEB,
      0xAE, 0x97, 0xDD, 0xD8, 0x7F, 0x3D, 0x89, 0x85 },

    /*
     * SHA-512 test vectors
     */
    { 0xDD, 0xAF, 0x35, 0xA1, 0x93, 0x61, 0x7A, 0xBA,
      0xCC, 0x41, 0x73, 0x49, 0xAE, 0x20, 0x41, 0x31,
      0x12, 0xE6, 0xFA, 0x4E, 0x89, 0xA9, 0x7E, 0xA2,
      0x0A, 0x9E, 0xEE, 0xE6, 0x4B, 0x55, 0xD3, 0x9A,
      0x21, 0x92, 0x99, 0x2A, 0x27, 0x4F, 0xC1, 0xA8,
      0x36, 0xBA, 0x3C, 0x23, 0xA3, 0xFE, 0xEB, 0xBD,
      0x45, 0x4D, 0x44, 0x23, 0x64, 0x3C, 0xE8, 0x0E,
      0x2A, 0x9A, 0xC9, 0x4F, 0xA5, 0x4C, 0xA4, 0x9F },
    { 0x8E, 0x95, 0x9B, 0x75, 0xDA, 0xE3, 0x13, 0xDA,
      0x8C, 0xF4, 0xF7, 0x28, 0x14, 0xFC, 0x14, 0x3F,
      0x8F, 0x77, 0x79, 0xC6, 0xEB, 0x9F, 0x7F, 0xA1,
      0x72, 0x99, 0xAE, 0xAD, 0xB6, 0x88, 0x90, 0x18,
      0x50, 0x1D, 0x28, 0x9E, 0x49, 0x00, 0xF7, 0xE4,
      0x33, 0x1B, 0x99, 0xDE, 0xC4, 0xB5, 0x43, 0x3A,
      0xC7, 0xD3, 0x29, 0xEE, 0xB6, 0xDD, 0x26, 0x54,
      0x5E, 0x96, 0xE5, 0x5B, 0x87, 0x4B, 0xE9, 0x09 },
    { 0xE7, 0x18, 0x48, 0x3D, 0x0C, 0xE7, 0x69, 0x64,
      0x4E, 0x2E, 0x42, 0xC7, 0xBC, 0x15, 0xB4, 0x63,
      0x8E, 0x1F, 0x98, 0xB1, 0x3B, 0x20, 0x44, 0x28,
      0x56, 0x32, 0xA8, 0x03, 0xAF, 0xA9, 0x73, 0xEB,
      0xDE, 0x0F, 0xF2, 0x44, 0x87, 0x7E, 0xA6, 0x0A,
      0x4C, 0xB0, 0x43, 0x2C, 0xE5, 0x77, 0xC3, 0x1B,
      0xEB, 0x00, 0x9C, 0x5C, 0x2C, 0x49, 0xAA, 0x2E,
      0x4E, 0xAD, 0xB2, 0x17, 0xAD, 0x8C, 0xC0, 0x9B }
};

/*
 * Checkup routine
 */
int mbedtls_sha512_self_test( int verbose )
{
    int i, j, k, buflen, ret = 0;
    unsigned char *buf;
    unsigned char sha512sum[64];
    mbedtls_sha512_context ctx;

    buf = mbedtls_calloc( 1024, sizeof(unsigned char) );
    if( NULL == buf )
    {
        if( verbose != 0 )
            mbedtls_printf( "Buffer allocation failed\n" );

        return( 1 );
    }

    mbedtls_sha512_init( &ctx );

    for( i = 0; i < 6; i++ )
    {
        j = i % 3;
        k = i < 3;

        if( verbose != 0 )
            mbedtls_printf( "  SHA-%d test #%d: ", 512 - k * 128, j + 1 );

        if( ( ret = mbedtls_sha512_starts_ret( &ctx, k ) ) != 0 )
            goto fail;

        if( j == 2 )
        {
            memset( buf, 'a', buflen = 1000 );

            for( j = 0; j < 1000; j++ )
            {
                ret = mbedtls_sha512_update_ret( &ctx, buf, buflen );
                if( ret != 0 )
                    goto fail;
            }
        }
        else
        {
            ret = mbedtls_sha512_update_ret( &ctx, sha512_test_buf[j],
                                             sha512_test_buflen[j] );
            if( ret != 0 )
                goto fail;
        }

        if( ( ret = mbedtls_sha512_finish_ret( &ctx, sha512sum ) ) != 0 )
            goto fail;

        if( memcmp( sha512sum, sha512_test_sum[i], 64 - k * 16 ) != 0 )
        {
            ret = 1;
            goto fail;
        }

        if( verbose != 0 )
            mbedtls_printf( "passed\n" );
    }

    if( verbose != 0 )
        mbedtls_printf( "\n" );

    goto exit;

fail:
    if( verbose != 0 )
        mbedtls_printf( "failed\n" );

exit:
    mbedtls_sha512_free( &ctx );
    mbedtls_free( buf );

    return( ret );
}
#endif /* MBEDTLS_SELF_TEST_OUT && MBEDTLS_SHA512_C */

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_MD5_C)

/*
 * RFC 1321 test vectors
 */
static const unsigned char md5_test_buf[7][81] =
{
    { "" },
    { "a" },
    { "abc" },
    { "message digest" },
    { "abcdefghijklmnopqrstuvwxyz" },
    { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" },
    { "12345678901234567890123456789012345678901234567890123456789012"
      "345678901234567890" }
};

static const size_t md5_test_buflen[7] =
{
    0,
    1, 3, 14, 26, 62, 80
};

static const unsigned char md5_test_sum[7][16] =
{
    { 0xD4, 0x1D, 0x8C, 0xD9, 0x8F, 0x00, 0xB2, 0x04,
      0xE9, 0x80, 0x09, 0x98, 0xEC, 0xF8, 0x42, 0x7E },
    { 0x0C, 0xC1, 0x75, 0xB9, 0xC0, 0xF1, 0xB6, 0xA8,
      0x31, 0xC3, 0x99, 0xE2, 0x69, 0x77, 0x26, 0x61 },
    { 0x90, 0x01, 0x50, 0x98, 0x3C, 0xD2, 0x4F, 0xB0,
      0xD6, 0x96, 0x3F, 0x7D, 0x28, 0xE1, 0x7F, 0x72 },
    { 0xF9, 0x6B, 0x69, 0x7D, 0x7C, 0xB7, 0x93, 0x8D,
      0x52, 0x5A, 0x2F, 0x31, 0xAA, 0xF1, 0x61, 0xD0 },
    { 0xC3, 0xFC, 0xD3, 0xD7, 0x61, 0x92, 0xE4, 0x00,
      0x7D, 0xFB, 0x49, 0x6C, 0xCA, 0x67, 0xE1, 0x3B },
    { 0xD1, 0x74, 0xAB, 0x98, 0xD2, 0x77, 0xD9, 0xF5,
      0xA5, 0x61, 0x1C, 0x2C, 0x9F, 0x41, 0x9D, 0x9F },
    { 0x57, 0xED, 0xF4, 0xA2, 0x2B, 0xE3, 0xC9, 0x55,
      0xAC, 0x49, 0xDA, 0x2E, 0x21, 0x07, 0xB6, 0x7A }
};

/*
 * Checkup routine
 */
int mbedtls_md5_self_test( int verbose )
{
    int i, ret = 0;
    unsigned char md5sum[16];

    for( i = 0; i < 7; i++ )
    {
        if( verbose != 0 )
            mbedtls_printf( "  MD5 test #%d: ", i + 1 );

        ret = mbedtls_md5_ret( md5_test_buf[i], md5_test_buflen[i], md5sum );
        if( ret != 0 )
            goto fail;

        if( memcmp( md5sum, md5_test_sum[i], 16 ) != 0 )
        {
            ret = 1;
            goto fail;
        }

        if( verbose != 0 )
            mbedtls_printf( "passed\n" );
    }

    if( verbose != 0 )
        mbedtls_printf( "\n" );

    return( 0 );

fail:
    if( verbose != 0 )
        mbedtls_printf( "failed\n" );

    return( ret );
}

#endif /* MBEDTLS_SELF_TEST_OUT && MBEDTLS_MD5_C */

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_ARC4_C)

/*
 * ARC4 tests vectors as posted by Eric Rescorla in sep. 1994:
 *
 * http://groups.google.com/group/comp.security.misc/msg/10a300c9d21afca0
 */
static const unsigned char arc4_test_key[3][8] =
{
    { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF },
    { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

static const unsigned char arc4_test_pt[3][8] =
{
    { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

static const unsigned char arc4_test_ct[3][8] =
{
    { 0x75, 0xB7, 0x87, 0x80, 0x99, 0xE0, 0xC5, 0x96 },
    { 0x74, 0x94, 0xC2, 0xE7, 0x10, 0x4B, 0x08, 0x79 },
    { 0xDE, 0x18, 0x89, 0x41, 0xA3, 0x37, 0x5D, 0x3A }
};

/*
 * Checkup routine
 */
int mbedtls_arc4_self_test( int verbose )
{
    int i, ret = 0;
    unsigned char ibuf[8];
    unsigned char obuf[8];
    mbedtls_arc4_context ctx;

    mbedtls_arc4_init( &ctx );

    for( i = 0; i < 3; i++ )
    {
        if( verbose != 0 )
            mbedtls_printf( "  ARC4 test #%d: ", i + 1 );

        memcpy( ibuf, arc4_test_pt[i], 8 );

        mbedtls_arc4_setup( &ctx, arc4_test_key[i], 8 );
        mbedtls_arc4_crypt( &ctx, 8, ibuf, obuf );

        if( memcmp( obuf, arc4_test_ct[i], 8 ) != 0 )
        {
            if( verbose != 0 )
                mbedtls_printf( "failed\n" );

            ret = 1;
            goto exit;
        }

        if( verbose != 0 )
            mbedtls_printf( "passed\n" );
    }

    if( verbose != 0 )
        mbedtls_printf( "\n" );

exit:
    mbedtls_arc4_free( &ctx );

    return( ret );
}

#endif /* MBEDTLS_SELF_TEST_OUT && MBEDTLS_ARC4_C */

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_DES_C)

/*
 * DES and 3DES test vectors from:
 *
 * http://csrc.nist.gov/groups/STM/cavp/documents/des/tripledes-vectors.zip
 */
static const unsigned char des3_test_keys[24] =
{
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01,
    0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01, 0x23
};

static const unsigned char des3_test_buf[8] =
{
    0x4E, 0x6F, 0x77, 0x20, 0x69, 0x73, 0x20, 0x74
};

static const unsigned char des3_test_ecb_dec[3][8] =
{
    { 0xCD, 0xD6, 0x4F, 0x2F, 0x94, 0x27, 0xC1, 0x5D },
    { 0x69, 0x96, 0xC8, 0xFA, 0x47, 0xA2, 0xAB, 0xEB },
    { 0x83, 0x25, 0x39, 0x76, 0x44, 0x09, 0x1A, 0x0A }
};

static const unsigned char des3_test_ecb_enc[3][8] =
{
    { 0x6A, 0x2A, 0x19, 0xF4, 0x1E, 0xCA, 0x85, 0x4B },
    { 0x03, 0xE6, 0x9F, 0x5B, 0xFA, 0x58, 0xEB, 0x42 },
    { 0xDD, 0x17, 0xE8, 0xB8, 0xB4, 0x37, 0xD2, 0x32 }
};

#if defined(MBEDTLS_CIPHER_MODE_CBC)
static const unsigned char des3_test_iv[8] =
{
    0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF,
};

static const unsigned char des3_test_cbc_dec[3][8] =
{
    { 0x12, 0x9F, 0x40, 0xB9, 0xD2, 0x00, 0x56, 0xB3 },
    { 0x47, 0x0E, 0xFC, 0x9A, 0x6B, 0x8E, 0xE3, 0x93 },
    { 0xC5, 0xCE, 0xCF, 0x63, 0xEC, 0xEC, 0x51, 0x4C }
};

static const unsigned char des3_test_cbc_enc[3][8] =
{
    { 0x54, 0xF1, 0x5A, 0xF6, 0xEB, 0xE3, 0xA4, 0xB4 },
    { 0x35, 0x76, 0x11, 0x56, 0x5F, 0xA1, 0x8E, 0x4D },
    { 0xCB, 0x19, 0x1F, 0x85, 0xD1, 0xED, 0x84, 0x39 }
};
#endif /* MBEDTLS_CIPHER_MODE_CBC */

/*
 * Checkup routine
 */
int mbedtls_des_self_test( int verbose )
{
    int i, j, u, v, ret = 0;
    mbedtls_des_context ctx;
    mbedtls_des3_context ctx3;
    unsigned char buf[8];
#if defined(MBEDTLS_CIPHER_MODE_CBC)
    unsigned char prv[8];
    unsigned char iv2[8];
#endif

    mbedtls_des_init( &ctx );
    mbedtls_des3_init( &ctx3 );
    /*
     * ECB mode
     */
    for( i = 0; i < 6; i++ )
    {
        u = i >> 1;
        v = i  & 1;

        if( verbose != 0 )
            mbedtls_printf( "  DES%c-ECB-%3d (%s): ",
                             ( u == 0 ) ? ' ' : '3', 56 + u * 56,
                             ( v == MBEDTLS_DES_DECRYPT ) ? "dec" : "enc" );

        memcpy( buf, des3_test_buf, 8 );

        switch( i )
        {
        case 0:
            mbedtls_des_setkey_dec( &ctx, des3_test_keys );
            break;

        case 1:
            mbedtls_des_setkey_enc( &ctx, des3_test_keys );
            break;

        case 2:
            mbedtls_des3_set2key_dec( &ctx3, des3_test_keys );
            break;

        case 3:
            mbedtls_des3_set2key_enc( &ctx3, des3_test_keys );
            break;

        case 4:
            mbedtls_des3_set3key_dec( &ctx3, des3_test_keys );
            break;

        case 5:
            mbedtls_des3_set3key_enc( &ctx3, des3_test_keys );
            break;

        default:
            return( 1 );
        }

        for( j = 0; j < 10000; j++ )
        {
            if( u == 0 )
                mbedtls_des_crypt_ecb( &ctx, buf, buf );
            else
                mbedtls_des3_crypt_ecb( &ctx3, buf, buf );
        }

        if( ( v == MBEDTLS_DES_DECRYPT &&
                memcmp( buf, des3_test_ecb_dec[u], 8 ) != 0 ) ||
            ( v != MBEDTLS_DES_DECRYPT &&
                memcmp( buf, des3_test_ecb_enc[u], 8 ) != 0 ) )
        {
            if( verbose != 0 )
                mbedtls_printf( "failed\n" );

            ret = 1;
            goto exit;
        }

        if( verbose != 0 )
            mbedtls_printf( "passed\n" );
    }

    if( verbose != 0 )
        mbedtls_printf( "\n" );

#if defined(MBEDTLS_CIPHER_MODE_CBC)
    /*
     * CBC mode
     */
    for( i = 0; i < 6; i++ )
    {
        u = i >> 1;
        v = i  & 1;

        if( verbose != 0 )
            mbedtls_printf( "  DES%c-CBC-%3d (%s): ",
                             ( u == 0 ) ? ' ' : '3', 56 + u * 56,
                             ( v == MBEDTLS_DES_DECRYPT ) ? "dec" : "enc" );

        memcpy( iv2,  des3_test_iv,  8 );
        memcpy( prv, des3_test_iv,  8 );
        memcpy( buf, des3_test_buf, 8 );

        switch( i )
        {
        case 0:
            mbedtls_des_setkey_dec( &ctx, des3_test_keys );
            break;

        case 1:
            mbedtls_des_setkey_enc( &ctx, des3_test_keys );
            break;

        case 2:
            mbedtls_des3_set2key_dec( &ctx3, des3_test_keys );
            break;

        case 3:
            mbedtls_des3_set2key_enc( &ctx3, des3_test_keys );
            break;

        case 4:
            mbedtls_des3_set3key_dec( &ctx3, des3_test_keys );
            break;

        case 5:
            mbedtls_des3_set3key_enc( &ctx3, des3_test_keys );
            break;

        default:
            return( 1 );
        }

        if( v == MBEDTLS_DES_DECRYPT )
        {
            for( j = 0; j < 10000; j++ )
            {
                if( u == 0 )
                    mbedtls_des_crypt_cbc( &ctx, v, 8, iv2, buf, buf );
                else
                    mbedtls_des3_crypt_cbc( &ctx3, v, 8, iv2, buf, buf );
            }
        }
        else
        {
            for( j = 0; j < 10000; j++ )
            {
                unsigned char tmp[8];

                if( u == 0 )
                    mbedtls_des_crypt_cbc( &ctx, v, 8, iv2, buf, buf );
                else
                    mbedtls_des3_crypt_cbc( &ctx3, v, 8, iv2, buf, buf );

                memcpy( tmp, prv, 8 );
                memcpy( prv, buf, 8 );
                memcpy( buf, tmp, 8 );
            }

            memcpy( buf, prv, 8 );
        }

        if( ( v == MBEDTLS_DES_DECRYPT &&
                memcmp( buf, des3_test_cbc_dec[u], 8 ) != 0 ) ||
            ( v != MBEDTLS_DES_DECRYPT &&
                memcmp( buf, des3_test_cbc_enc[u], 8 ) != 0 ) )
        {
            if( verbose != 0 )
                mbedtls_printf( "failed\n" );

            ret = 1;
            goto exit;
        }

        if( verbose != 0 )
            mbedtls_printf( "passed\n" );
    }
#endif /* MBEDTLS_CIPHER_MODE_CBC */

    if( verbose != 0 )
        mbedtls_printf( "\n" );

exit:
    mbedtls_des_free( &ctx );
    mbedtls_des3_free( &ctx3 );

    return( ret );
}

#endif /* MBEDTLS_SELF_TEST_OUT && MBEDTLS_DES_C */

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_AES_C)

/*
 * AES test vectors from:
 *
 * http://csrc.nist.gov/archive/aes/rijndael/rijndael-vals.zip
 */
static const unsigned char aes_test_ecb_dec[3][16] =
{
    { 0x44, 0x41, 0x6A, 0xC2, 0xD1, 0xF5, 0x3C, 0x58,
      0x33, 0x03, 0x91, 0x7E, 0x6B, 0xE9, 0xEB, 0xE0 },
    { 0x48, 0xE3, 0x1E, 0x9E, 0x25, 0x67, 0x18, 0xF2,
      0x92, 0x29, 0x31, 0x9C, 0x19, 0xF1, 0x5B, 0xA4 },
    { 0x05, 0x8C, 0xCF, 0xFD, 0xBB, 0xCB, 0x38, 0x2D,
      0x1F, 0x6F, 0x56, 0x58, 0x5D, 0x8A, 0x4A, 0xDE }
};

static const unsigned char aes_test_ecb_enc[3][16] =
{
    { 0xC3, 0x4C, 0x05, 0x2C, 0xC0, 0xDA, 0x8D, 0x73,
      0x45, 0x1A, 0xFE, 0x5F, 0x03, 0xBE, 0x29, 0x7F },
    { 0xF3, 0xF6, 0x75, 0x2A, 0xE8, 0xD7, 0x83, 0x11,
      0x38, 0xF0, 0x41, 0x56, 0x06, 0x31, 0xB1, 0x14 },
    { 0x8B, 0x79, 0xEE, 0xCC, 0x93, 0xA0, 0xEE, 0x5D,
      0xFF, 0x30, 0xB4, 0xEA, 0x21, 0x63, 0x6D, 0xA4 }
};

#if defined(MBEDTLS_CIPHER_MODE_CBC)
static const unsigned char aes_test_cbc_dec[3][16] =
{
    { 0xFA, 0xCA, 0x37, 0xE0, 0xB0, 0xC8, 0x53, 0x73,
      0xDF, 0x70, 0x6E, 0x73, 0xF7, 0xC9, 0xAF, 0x86 },
    { 0x5D, 0xF6, 0x78, 0xDD, 0x17, 0xBA, 0x4E, 0x75,
      0xB6, 0x17, 0x68, 0xC6, 0xAD, 0xEF, 0x7C, 0x7B },
    { 0x48, 0x04, 0xE1, 0x81, 0x8F, 0xE6, 0x29, 0x75,
      0x19, 0xA3, 0xE8, 0x8C, 0x57, 0x31, 0x04, 0x13 }
};

static const unsigned char aes_test_cbc_enc[3][16] =
{
    { 0x8A, 0x05, 0xFC, 0x5E, 0x09, 0x5A, 0xF4, 0x84,
      0x8A, 0x08, 0xD3, 0x28, 0xD3, 0x68, 0x8E, 0x3D },
    { 0x7B, 0xD9, 0x66, 0xD5, 0x3A, 0xD8, 0xC1, 0xBB,
      0x85, 0xD2, 0xAD, 0xFA, 0xE8, 0x7B, 0xB1, 0x04 },
    { 0xFE, 0x3C, 0x53, 0x65, 0x3E, 0x2F, 0x45, 0xB5,
      0x6F, 0xCD, 0x88, 0xB2, 0xCC, 0x89, 0x8F, 0xF0 }
};
#endif /* MBEDTLS_CIPHER_MODE_CBC */
static const unsigned char aes_test_ctr_key[3][16] =
{
    { 0xAE, 0x68, 0x52, 0xF8, 0x12, 0x10, 0x67, 0xCC,
      0x4B, 0xF7, 0xA5, 0x76, 0x55, 0x77, 0xF3, 0x9E },
    { 0x7E, 0x24, 0x06, 0x78, 0x17, 0xFA, 0xE0, 0xD7,
      0x43, 0xD6, 0xCE, 0x1F, 0x32, 0x53, 0x91, 0x63 },
    { 0x76, 0x91, 0xBE, 0x03, 0x5E, 0x50, 0x20, 0xA8,
      0xAC, 0x6E, 0x61, 0x85, 0x29, 0xF9, 0xA0, 0xDC }
};

static const unsigned char aes_test_ctr_nonce_counter[3][16] =
{
    { 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 },
    { 0x00, 0x6C, 0xB6, 0xDB, 0xC0, 0x54, 0x3B, 0x59,
      0xDA, 0x48, 0xD9, 0x0B, 0x00, 0x00, 0x00, 0x01 },
    { 0x00, 0xE0, 0x01, 0x7B, 0x27, 0x77, 0x7F, 0x3F,
      0x4A, 0x17, 0x86, 0xF0, 0x00, 0x00, 0x00, 0x01 }
};

static const unsigned char aes_test_ctr_pt[3][48] =
{
    { 0x53, 0x69, 0x6E, 0x67, 0x6C, 0x65, 0x20, 0x62,
      0x6C, 0x6F, 0x63, 0x6B, 0x20, 0x6D, 0x73, 0x67 },

    { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
      0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F },

    { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
      0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
      0x20, 0x21, 0x22, 0x23 }
};

static const unsigned char aes_test_ctr_ct[3][48] =
{
    { 0xE4, 0x09, 0x5D, 0x4F, 0xB7, 0xA7, 0xB3, 0x79,
      0x2D, 0x61, 0x75, 0xA3, 0x26, 0x13, 0x11, 0xB8 },
    { 0x51, 0x04, 0xA1, 0x06, 0x16, 0x8A, 0x72, 0xD9,
      0x79, 0x0D, 0x41, 0xEE, 0x8E, 0xDA, 0xD3, 0x88,
      0xEB, 0x2E, 0x1E, 0xFC, 0x46, 0xDA, 0x57, 0xC8,
      0xFC, 0xE6, 0x30, 0xDF, 0x91, 0x41, 0xBE, 0x28 },
    { 0xC1, 0xCF, 0x48, 0xA8, 0x9F, 0x2F, 0xFD, 0xD9,
      0xCF, 0x46, 0x52, 0xE9, 0xEF, 0xDB, 0x72, 0xD7,
      0x45, 0x40, 0xA4, 0x2B, 0xDE, 0x6D, 0x78, 0x36,
      0xD5, 0x9A, 0x5C, 0xEA, 0xAE, 0xF3, 0x10, 0x53,
      0x25, 0xB2, 0x07, 0x2F }
};

static const int aes_test_ctr_len[3] =
    { 16, 32, 36 };
#if defined(MBEDTLS_CIPHER_MODE_CFB)
/*
 * AES-CFB128 test vectors from:
 *
 * http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf
 */
static const unsigned char aes_test_cfb128_key[3][32] =
{
    { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
      0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C },
    { 0x8E, 0x73, 0xB0, 0xF7, 0xDA, 0x0E, 0x64, 0x52,
      0xC8, 0x10, 0xF3, 0x2B, 0x80, 0x90, 0x79, 0xE5,
      0x62, 0xF8, 0xEA, 0xD2, 0x52, 0x2C, 0x6B, 0x7B },
    { 0x60, 0x3D, 0xEB, 0x10, 0x15, 0xCA, 0x71, 0xBE,
      0x2B, 0x73, 0xAE, 0xF0, 0x85, 0x7D, 0x77, 0x81,
      0x1F, 0x35, 0x2C, 0x07, 0x3B, 0x61, 0x08, 0xD7,
      0x2D, 0x98, 0x10, 0xA3, 0x09, 0x14, 0xDF, 0xF4 }
};

static const unsigned char aes_test_cfb128_iv[16] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

static const unsigned char aes_test_cfb128_pt[64] =
{
    0x6B, 0xC1, 0xBE, 0xE2, 0x2E, 0x40, 0x9F, 0x96,
    0xE9, 0x3D, 0x7E, 0x11, 0x73, 0x93, 0x17, 0x2A,
    0xAE, 0x2D, 0x8A, 0x57, 0x1E, 0x03, 0xAC, 0x9C,
    0x9E, 0xB7, 0x6F, 0xAC, 0x45, 0xAF, 0x8E, 0x51,
    0x30, 0xC8, 0x1C, 0x46, 0xA3, 0x5C, 0xE4, 0x11,
    0xE5, 0xFB, 0xC1, 0x19, 0x1A, 0x0A, 0x52, 0xEF,
    0xF6, 0x9F, 0x24, 0x45, 0xDF, 0x4F, 0x9B, 0x17,
    0xAD, 0x2B, 0x41, 0x7B, 0xE6, 0x6C, 0x37, 0x10
};

static const unsigned char aes_test_cfb128_ct[3][64] =
{
    { 0x3B, 0x3F, 0xD9, 0x2E, 0xB7, 0x2D, 0xAD, 0x20,
      0x33, 0x34, 0x49, 0xF8, 0xE8, 0x3C, 0xFB, 0x4A,
      0xC8, 0xA6, 0x45, 0x37, 0xA0, 0xB3, 0xA9, 0x3F,
      0xCD, 0xE3, 0xCD, 0xAD, 0x9F, 0x1C, 0xE5, 0x8B,
      0x26, 0x75, 0x1F, 0x67, 0xA3, 0xCB, 0xB1, 0x40,
      0xB1, 0x80, 0x8C, 0xF1, 0x87, 0xA4, 0xF4, 0xDF,
      0xC0, 0x4B, 0x05, 0x35, 0x7C, 0x5D, 0x1C, 0x0E,
      0xEA, 0xC4, 0xC6, 0x6F, 0x9F, 0xF7, 0xF2, 0xE6 },
    { 0xCD, 0xC8, 0x0D, 0x6F, 0xDD, 0xF1, 0x8C, 0xAB,
      0x34, 0xC2, 0x59, 0x09, 0xC9, 0x9A, 0x41, 0x74,
      0x67, 0xCE, 0x7F, 0x7F, 0x81, 0x17, 0x36, 0x21,
      0x96, 0x1A, 0x2B, 0x70, 0x17, 0x1D, 0x3D, 0x7A,
      0x2E, 0x1E, 0x8A, 0x1D, 0xD5, 0x9B, 0x88, 0xB1,
      0xC8, 0xE6, 0x0F, 0xED, 0x1E, 0xFA, 0xC4, 0xC9,
      0xC0, 0x5F, 0x9F, 0x9C, 0xA9, 0x83, 0x4F, 0xA0,
      0x42, 0xAE, 0x8F, 0xBA, 0x58, 0x4B, 0x09, 0xFF },
    { 0xDC, 0x7E, 0x84, 0xBF, 0xDA, 0x79, 0x16, 0x4B,
      0x7E, 0xCD, 0x84, 0x86, 0x98, 0x5D, 0x38, 0x60,
      0x39, 0xFF, 0xED, 0x14, 0x3B, 0x28, 0xB1, 0xC8,
      0x32, 0x11, 0x3C, 0x63, 0x31, 0xE5, 0x40, 0x7B,
      0xDF, 0x10, 0x13, 0x24, 0x15, 0xE5, 0x4B, 0x92,
      0xA1, 0x3E, 0xD0, 0xA8, 0x26, 0x7A, 0xE2, 0xF9,
      0x75, 0xA3, 0x85, 0x74, 0x1A, 0xB9, 0xCE, 0xF8,
      0x20, 0x31, 0x62, 0x3D, 0x55, 0xB1, 0xE4, 0x71 }
};
#endif /* MBEDTLS_CIPHER_MODE_CFB */

/*
 * Checkup routine
 */
int mbedtls_aes_self_test( int verbose )
{
    int ret = 0, i, j, u, v;
    unsigned char key1[32];
    unsigned char buf[64];
#if defined(MBEDTLS_CIPHER_MODE_CBC) || defined(MBEDTLS_CIPHER_MODE_CFB)
    unsigned char iv1[16];
#endif
#if defined(MBEDTLS_CIPHER_MODE_CBC)
    unsigned char prv[16];
#endif
#if defined(MBEDTLS_CIPHER_MODE_CTR) || defined(MBEDTLS_CIPHER_MODE_CFB)
    size_t offset;
#endif
#if defined(MBEDTLS_CIPHER_MODE_CTR)
    int len;
    unsigned char nonce_counter[16];
    unsigned char stream_block[16];
#endif
    mbedtls_aes_context ctx;

    memset( key1, 0, 32 );
    mbedtls_aes_init( &ctx );

    /*
     * ECB mode
     */
    for( i = 0; i < 6; i++ )
    {
        u = i >> 1;
        v = i  & 1;

        if( verbose != 0 )
            mbedtls_printf( "  AES-ECB-%3d (%s): ", 128 + u * 64,
                             ( v == MBEDTLS_AES_DECRYPT ) ? "dec" : "enc" );

        memset( buf, 0, 16 );

        if( v == MBEDTLS_AES_DECRYPT )
        {
            mbedtls_aes_setkey_dec( &ctx, key1, 128 + u * 64 );

            for( j = 0; j < 10000; j++ )
                mbedtls_aes_crypt_ecb( &ctx, v, buf, buf );

            if( memcmp( buf, aes_test_ecb_dec[u], 16 ) != 0 )
            {
                if( verbose != 0 )
                    mbedtls_printf( "failed\n" );

                ret = 1;
                goto exit;
            }
        }
        else
        {
            mbedtls_aes_setkey_enc( &ctx, key1, 128 + u * 64 );

            for( j = 0; j < 10000; j++ )
                mbedtls_aes_crypt_ecb( &ctx, v, buf, buf );

            if( memcmp( buf, aes_test_ecb_enc[u], 16 ) != 0 )
            {
                if( verbose != 0 )
                    mbedtls_printf( "failed\n" );

                ret = 1;
                goto exit;
            }
        }

        if( verbose != 0 )
            mbedtls_printf( "passed\n" );
    }

    if( verbose != 0 )
        mbedtls_printf( "\n" );

#if defined(MBEDTLS_CIPHER_MODE_CBC)
    /*
     * CBC mode
     */
    for( i = 0; i < 6; i++ )
    {
        u = i >> 1;
        v = i  & 1;

        if( verbose != 0 )
            mbedtls_printf( "  AES-CBC-%3d (%s): ", 128 + u * 64,
                             ( v == MBEDTLS_AES_DECRYPT ) ? "dec" : "enc" );

        memset( iv1 , 0, 16 );
        memset( prv, 0, 16 );
        memset( buf, 0, 16 );

        if( v == MBEDTLS_AES_DECRYPT )
        {
            mbedtls_aes_setkey_dec( &ctx, key1, 128 + u * 64 );

            for( j = 0; j < 10000; j++ )
                mbedtls_aes_crypt_cbc( &ctx, v, 16, iv1, buf, buf );

            if( memcmp( buf, aes_test_cbc_dec[u], 16 ) != 0 )
            {
                if( verbose != 0 )
                    mbedtls_printf( "failed\n" );

                ret = 1;
                goto exit;
            }
        }
        else
        {
            mbedtls_aes_setkey_enc( &ctx, key1, 128 + u * 64 );

            for( j = 0; j < 10000; j++ )
            {
                unsigned char tmp[16];

                mbedtls_aes_crypt_cbc( &ctx, v, 16, iv1, buf, buf );

                memcpy( tmp, prv, 16 );
                memcpy( prv, buf, 16 );
                memcpy( buf, tmp, 16 );
            }

            if( memcmp( prv, aes_test_cbc_enc[u], 16 ) != 0 )
            {
                if( verbose != 0 )
                    mbedtls_printf( "failed\n" );

                ret = 1;
                goto exit;
            }
        }

        if( verbose != 0 )
            mbedtls_printf( "passed\n" );
    }

    if( verbose != 0 )
        mbedtls_printf( "\n" );
#endif /* MBEDTLS_CIPHER_MODE_CBC */

#if defined(MBEDTLS_CIPHER_MODE_CFB)
    /*
     * CFB128 mode
     */
    for( i = 0; i < 6; i++ )
    {
        u = i >> 1;
        v = i  & 1;

        if( verbose != 0 )
            mbedtls_printf( "  AES-CFB128-%3d (%s): ", 128 + u * 64,
                             ( v == MBEDTLS_AES_DECRYPT ) ? "dec" : "enc" );

        memcpy( iv1,  aes_test_cfb128_iv, 16 );
        memcpy( key1, aes_test_cfb128_key[u], 16 + u * 8 );

        offset = 0;
        mbedtls_aes_setkey_enc( &ctx, key1, 128 + u * 64 );

        if( v == MBEDTLS_AES_DECRYPT )
        {
            memcpy( buf, aes_test_cfb128_ct[u], 64 );
            mbedtls_aes_crypt_cfb128( &ctx, v, 64, &offset, iv1, buf, buf );

            if( memcmp( buf, aes_test_cfb128_pt, 64 ) != 0 )
            {
                if( verbose != 0 )
                    mbedtls_printf( "failed\n" );

                ret = 1;
                goto exit;
            }
        }
        else
        {
            memcpy( buf, aes_test_cfb128_pt, 64 );
            mbedtls_aes_crypt_cfb128( &ctx, v, 64, &offset, iv1, buf, buf );

            if( memcmp( buf, aes_test_cfb128_ct[u], 64 ) != 0 )
            {
                if( verbose != 0 )
                    mbedtls_printf( "failed\n" );

                ret = 1;
                goto exit;
            }
        }

        if( verbose != 0 )
            mbedtls_printf( "passed\n" );
    }

    if( verbose != 0 )
        mbedtls_printf( "\n" );
#endif /* MBEDTLS_CIPHER_MODE_CFB */

#if defined(MBEDTLS_CIPHER_MODE_CTR)
    /*
     * CTR mode
     */
    for( i = 0; i < 6; i++ )
    {
        u = i >> 1;
        v = i  & 1;

        if( verbose != 0 )
            mbedtls_printf( "  AES-CTR-128 (%s): ",
                             ( v == MBEDTLS_AES_DECRYPT ) ? "dec" : "enc" );

        memcpy( nonce_counter, aes_test_ctr_nonce_counter[u], 16 );
        memcpy( key1, aes_test_ctr_key[u], 16 );

        offset = 0;
        mbedtls_aes_setkey_enc( &ctx, key1, 128 );

        if( v == MBEDTLS_AES_DECRYPT )
        {
            len = aes_test_ctr_len[u];
            memcpy( buf, aes_test_ctr_ct[u], len );

            mbedtls_aes_crypt_ctr( &ctx, len, &offset, nonce_counter, stream_block,
                           buf, buf );

            if( memcmp( buf, aes_test_ctr_pt[u], len ) != 0 )
            {
                if( verbose != 0 )
                    mbedtls_printf( "failed\n" );

                ret = 1;
                goto exit;
            }
        }
        else
        {
            len = aes_test_ctr_len[u];
            memcpy( buf, aes_test_ctr_pt[u], len );

            mbedtls_aes_crypt_ctr( &ctx, len, &offset, nonce_counter, stream_block,
                           buf, buf );

            if( memcmp( buf, aes_test_ctr_ct[u], len ) != 0 )
            {
                if( verbose != 0 )
                    mbedtls_printf( "failed\n" );

                ret = 1;
                goto exit;
            }
        }

        if( verbose != 0 )
            mbedtls_printf( "passed\n" );
    }

    if( verbose != 0 )
        mbedtls_printf( "\n" );
#endif /* MBEDTLS_CIPHER_MODE_CTR */

    ret = 0;

exit:
    mbedtls_aes_free( &ctx );

    return( ret );
}

#endif /* MBEDTLS_SELF_TEST_OUT && MBEDTLS_AES_C */

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_AES_C) && defined(MBEDTLS_GCM_C)

/*
 * AES-GCM test vectors from:
 *
 * http://csrc.nist.gov/groups/STM/cavp/documents/mac/gcmtestvectors.zip
 */
#define MAX_TESTS   6

static const int key_index[MAX_TESTS] =
    { 0, 0, 1, 1, 1, 1 };

static const unsigned char key2[MAX_TESTS][32] =
{
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
      0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08,
      0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
      0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08 },
};

static const size_t iv_len[MAX_TESTS] =
    { 12, 12, 12, 12, 8, 60 };

static const int iv_index[MAX_TESTS] =
    { 0, 0, 1, 1, 1, 2 };

static const unsigned char iv3[MAX_TESTS][64] =
{
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00 },
    { 0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad,
      0xde, 0xca, 0xf8, 0x88 },
    { 0x93, 0x13, 0x22, 0x5d, 0xf8, 0x84, 0x06, 0xe5,
      0x55, 0x90, 0x9c, 0x5a, 0xff, 0x52, 0x69, 0xaa,
      0x6a, 0x7a, 0x95, 0x38, 0x53, 0x4f, 0x7d, 0xa1,
      0xe4, 0xc3, 0x03, 0xd2, 0xa3, 0x18, 0xa7, 0x28,
      0xc3, 0xc0, 0xc9, 0x51, 0x56, 0x80, 0x95, 0x39,
      0xfc, 0xf0, 0xe2, 0x42, 0x9a, 0x6b, 0x52, 0x54,
      0x16, 0xae, 0xdb, 0xf5, 0xa0, 0xde, 0x6a, 0x57,
      0xa6, 0x37, 0xb3, 0x9b },
};

static const size_t add_len[MAX_TESTS] =
    { 0, 0, 0, 20, 20, 20 };

static const int add_index[MAX_TESTS] =
    { 0, 0, 0, 1, 1, 1 };

static const unsigned char additional[MAX_TESTS][64] =
{
    { 0x00 },
    { 0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
      0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
      0xab, 0xad, 0xda, 0xd2 },
};

static const size_t pt_len[MAX_TESTS] =
    { 0, 16, 64, 60, 60, 60 };

static const int pt_index[MAX_TESTS] =
    { 0, 0, 1, 1, 1, 1 };

static const unsigned char pt[MAX_TESTS][64] =
{
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5,
      0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a,
      0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda,
      0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72,
      0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53,
      0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25,
      0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57,
      0xba, 0x63, 0x7b, 0x39, 0x1a, 0xaf, 0xd2, 0x55 },
};

static const unsigned char ct[MAX_TESTS * 3][64] =
{
    { 0x00 },
    { 0x03, 0x88, 0xda, 0xce, 0x60, 0xb6, 0xa3, 0x92,
      0xf3, 0x28, 0xc2, 0xb9, 0x71, 0xb2, 0xfe, 0x78 },
    { 0x42, 0x83, 0x1e, 0xc2, 0x21, 0x77, 0x74, 0x24,
      0x4b, 0x72, 0x21, 0xb7, 0x84, 0xd0, 0xd4, 0x9c,
      0xe3, 0xaa, 0x21, 0x2f, 0x2c, 0x02, 0xa4, 0xe0,
      0x35, 0xc1, 0x7e, 0x23, 0x29, 0xac, 0xa1, 0x2e,
      0x21, 0xd5, 0x14, 0xb2, 0x54, 0x66, 0x93, 0x1c,
      0x7d, 0x8f, 0x6a, 0x5a, 0xac, 0x84, 0xaa, 0x05,
      0x1b, 0xa3, 0x0b, 0x39, 0x6a, 0x0a, 0xac, 0x97,
      0x3d, 0x58, 0xe0, 0x91, 0x47, 0x3f, 0x59, 0x85 },
    { 0x42, 0x83, 0x1e, 0xc2, 0x21, 0x77, 0x74, 0x24,
      0x4b, 0x72, 0x21, 0xb7, 0x84, 0xd0, 0xd4, 0x9c,
      0xe3, 0xaa, 0x21, 0x2f, 0x2c, 0x02, 0xa4, 0xe0,
      0x35, 0xc1, 0x7e, 0x23, 0x29, 0xac, 0xa1, 0x2e,
      0x21, 0xd5, 0x14, 0xb2, 0x54, 0x66, 0x93, 0x1c,
      0x7d, 0x8f, 0x6a, 0x5a, 0xac, 0x84, 0xaa, 0x05,
      0x1b, 0xa3, 0x0b, 0x39, 0x6a, 0x0a, 0xac, 0x97,
      0x3d, 0x58, 0xe0, 0x91 },
    { 0x61, 0x35, 0x3b, 0x4c, 0x28, 0x06, 0x93, 0x4a,
      0x77, 0x7f, 0xf5, 0x1f, 0xa2, 0x2a, 0x47, 0x55,
      0x69, 0x9b, 0x2a, 0x71, 0x4f, 0xcd, 0xc6, 0xf8,
      0x37, 0x66, 0xe5, 0xf9, 0x7b, 0x6c, 0x74, 0x23,
      0x73, 0x80, 0x69, 0x00, 0xe4, 0x9f, 0x24, 0xb2,
      0x2b, 0x09, 0x75, 0x44, 0xd4, 0x89, 0x6b, 0x42,
      0x49, 0x89, 0xb5, 0xe1, 0xeb, 0xac, 0x0f, 0x07,
      0xc2, 0x3f, 0x45, 0x98 },
    { 0x8c, 0xe2, 0x49, 0x98, 0x62, 0x56, 0x15, 0xb6,
      0x03, 0xa0, 0x33, 0xac, 0xa1, 0x3f, 0xb8, 0x94,
      0xbe, 0x91, 0x12, 0xa5, 0xc3, 0xa2, 0x11, 0xa8,
      0xba, 0x26, 0x2a, 0x3c, 0xca, 0x7e, 0x2c, 0xa7,
      0x01, 0xe4, 0xa9, 0xa4, 0xfb, 0xa4, 0x3c, 0x90,
      0xcc, 0xdc, 0xb2, 0x81, 0xd4, 0x8c, 0x7c, 0x6f,
      0xd6, 0x28, 0x75, 0xd2, 0xac, 0xa4, 0x17, 0x03,
      0x4c, 0x34, 0xae, 0xe5 },
    { 0x00 },
    { 0x98, 0xe7, 0x24, 0x7c, 0x07, 0xf0, 0xfe, 0x41,
      0x1c, 0x26, 0x7e, 0x43, 0x84, 0xb0, 0xf6, 0x00 },
    { 0x39, 0x80, 0xca, 0x0b, 0x3c, 0x00, 0xe8, 0x41,
      0xeb, 0x06, 0xfa, 0xc4, 0x87, 0x2a, 0x27, 0x57,
      0x85, 0x9e, 0x1c, 0xea, 0xa6, 0xef, 0xd9, 0x84,
      0x62, 0x85, 0x93, 0xb4, 0x0c, 0xa1, 0xe1, 0x9c,
      0x7d, 0x77, 0x3d, 0x00, 0xc1, 0x44, 0xc5, 0x25,
      0xac, 0x61, 0x9d, 0x18, 0xc8, 0x4a, 0x3f, 0x47,
      0x18, 0xe2, 0x44, 0x8b, 0x2f, 0xe3, 0x24, 0xd9,
      0xcc, 0xda, 0x27, 0x10, 0xac, 0xad, 0xe2, 0x56 },
    { 0x39, 0x80, 0xca, 0x0b, 0x3c, 0x00, 0xe8, 0x41,
      0xeb, 0x06, 0xfa, 0xc4, 0x87, 0x2a, 0x27, 0x57,
      0x85, 0x9e, 0x1c, 0xea, 0xa6, 0xef, 0xd9, 0x84,
      0x62, 0x85, 0x93, 0xb4, 0x0c, 0xa1, 0xe1, 0x9c,
      0x7d, 0x77, 0x3d, 0x00, 0xc1, 0x44, 0xc5, 0x25,
      0xac, 0x61, 0x9d, 0x18, 0xc8, 0x4a, 0x3f, 0x47,
      0x18, 0xe2, 0x44, 0x8b, 0x2f, 0xe3, 0x24, 0xd9,
      0xcc, 0xda, 0x27, 0x10 },
    { 0x0f, 0x10, 0xf5, 0x99, 0xae, 0x14, 0xa1, 0x54,
      0xed, 0x24, 0xb3, 0x6e, 0x25, 0x32, 0x4d, 0xb8,
      0xc5, 0x66, 0x63, 0x2e, 0xf2, 0xbb, 0xb3, 0x4f,
      0x83, 0x47, 0x28, 0x0f, 0xc4, 0x50, 0x70, 0x57,
      0xfd, 0xdc, 0x29, 0xdf, 0x9a, 0x47, 0x1f, 0x75,
      0xc6, 0x65, 0x41, 0xd4, 0xd4, 0xda, 0xd1, 0xc9,
      0xe9, 0x3a, 0x19, 0xa5, 0x8e, 0x8b, 0x47, 0x3f,
      0xa0, 0xf0, 0x62, 0xf7 },
    { 0xd2, 0x7e, 0x88, 0x68, 0x1c, 0xe3, 0x24, 0x3c,
      0x48, 0x30, 0x16, 0x5a, 0x8f, 0xdc, 0xf9, 0xff,
      0x1d, 0xe9, 0xa1, 0xd8, 0xe6, 0xb4, 0x47, 0xef,
      0x6e, 0xf7, 0xb7, 0x98, 0x28, 0x66, 0x6e, 0x45,
      0x81, 0xe7, 0x90, 0x12, 0xaf, 0x34, 0xdd, 0xd9,
      0xe2, 0xf0, 0x37, 0x58, 0x9b, 0x29, 0x2d, 0xb3,
      0xe6, 0x7c, 0x03, 0x67, 0x45, 0xfa, 0x22, 0xe7,
      0xe9, 0xb7, 0x37, 0x3b },
    { 0x00 },
    { 0xce, 0xa7, 0x40, 0x3d, 0x4d, 0x60, 0x6b, 0x6e,
      0x07, 0x4e, 0xc5, 0xd3, 0xba, 0xf3, 0x9d, 0x18 },
    { 0x52, 0x2d, 0xc1, 0xf0, 0x99, 0x56, 0x7d, 0x07,
      0xf4, 0x7f, 0x37, 0xa3, 0x2a, 0x84, 0x42, 0x7d,
      0x64, 0x3a, 0x8c, 0xdc, 0xbf, 0xe5, 0xc0, 0xc9,
      0x75, 0x98, 0xa2, 0xbd, 0x25, 0x55, 0xd1, 0xaa,
      0x8c, 0xb0, 0x8e, 0x48, 0x59, 0x0d, 0xbb, 0x3d,
      0xa7, 0xb0, 0x8b, 0x10, 0x56, 0x82, 0x88, 0x38,
      0xc5, 0xf6, 0x1e, 0x63, 0x93, 0xba, 0x7a, 0x0a,
      0xbc, 0xc9, 0xf6, 0x62, 0x89, 0x80, 0x15, 0xad },
    { 0x52, 0x2d, 0xc1, 0xf0, 0x99, 0x56, 0x7d, 0x07,
      0xf4, 0x7f, 0x37, 0xa3, 0x2a, 0x84, 0x42, 0x7d,
      0x64, 0x3a, 0x8c, 0xdc, 0xbf, 0xe5, 0xc0, 0xc9,
      0x75, 0x98, 0xa2, 0xbd, 0x25, 0x55, 0xd1, 0xaa,
      0x8c, 0xb0, 0x8e, 0x48, 0x59, 0x0d, 0xbb, 0x3d,
      0xa7, 0xb0, 0x8b, 0x10, 0x56, 0x82, 0x88, 0x38,
      0xc5, 0xf6, 0x1e, 0x63, 0x93, 0xba, 0x7a, 0x0a,
      0xbc, 0xc9, 0xf6, 0x62 },
    { 0xc3, 0x76, 0x2d, 0xf1, 0xca, 0x78, 0x7d, 0x32,
      0xae, 0x47, 0xc1, 0x3b, 0xf1, 0x98, 0x44, 0xcb,
      0xaf, 0x1a, 0xe1, 0x4d, 0x0b, 0x97, 0x6a, 0xfa,
      0xc5, 0x2f, 0xf7, 0xd7, 0x9b, 0xba, 0x9d, 0xe0,
      0xfe, 0xb5, 0x82, 0xd3, 0x39, 0x34, 0xa4, 0xf0,
      0x95, 0x4c, 0xc2, 0x36, 0x3b, 0xc7, 0x3f, 0x78,
      0x62, 0xac, 0x43, 0x0e, 0x64, 0xab, 0xe4, 0x99,
      0xf4, 0x7c, 0x9b, 0x1f },
    { 0x5a, 0x8d, 0xef, 0x2f, 0x0c, 0x9e, 0x53, 0xf1,
      0xf7, 0x5d, 0x78, 0x53, 0x65, 0x9e, 0x2a, 0x20,
      0xee, 0xb2, 0xb2, 0x2a, 0xaf, 0xde, 0x64, 0x19,
      0xa0, 0x58, 0xab, 0x4f, 0x6f, 0x74, 0x6b, 0xf4,
      0x0f, 0xc0, 0xc3, 0xb7, 0x80, 0xf2, 0x44, 0x45,
      0x2d, 0xa3, 0xeb, 0xf1, 0xc5, 0xd8, 0x2c, 0xde,
      0xa2, 0x41, 0x89, 0x97, 0x20, 0x0e, 0xf8, 0x2e,
      0x44, 0xae, 0x7e, 0x3f },
};

static const unsigned char tag[MAX_TESTS * 3][16] =
{
    { 0x58, 0xe2, 0xfc, 0xce, 0xfa, 0x7e, 0x30, 0x61,
      0x36, 0x7f, 0x1d, 0x57, 0xa4, 0xe7, 0x45, 0x5a },
    { 0xab, 0x6e, 0x47, 0xd4, 0x2c, 0xec, 0x13, 0xbd,
      0xf5, 0x3a, 0x67, 0xb2, 0x12, 0x57, 0xbd, 0xdf },
    { 0x4d, 0x5c, 0x2a, 0xf3, 0x27, 0xcd, 0x64, 0xa6,
      0x2c, 0xf3, 0x5a, 0xbd, 0x2b, 0xa6, 0xfa, 0xb4 },
    { 0x5b, 0xc9, 0x4f, 0xbc, 0x32, 0x21, 0xa5, 0xdb,
      0x94, 0xfa, 0xe9, 0x5a, 0xe7, 0x12, 0x1a, 0x47 },
    { 0x36, 0x12, 0xd2, 0xe7, 0x9e, 0x3b, 0x07, 0x85,
      0x56, 0x1b, 0xe1, 0x4a, 0xac, 0xa2, 0xfc, 0xcb },
    { 0x61, 0x9c, 0xc5, 0xae, 0xff, 0xfe, 0x0b, 0xfa,
      0x46, 0x2a, 0xf4, 0x3c, 0x16, 0x99, 0xd0, 0x50 },
    { 0xcd, 0x33, 0xb2, 0x8a, 0xc7, 0x73, 0xf7, 0x4b,
      0xa0, 0x0e, 0xd1, 0xf3, 0x12, 0x57, 0x24, 0x35 },
    { 0x2f, 0xf5, 0x8d, 0x80, 0x03, 0x39, 0x27, 0xab,
      0x8e, 0xf4, 0xd4, 0x58, 0x75, 0x14, 0xf0, 0xfb },
    { 0x99, 0x24, 0xa7, 0xc8, 0x58, 0x73, 0x36, 0xbf,
      0xb1, 0x18, 0x02, 0x4d, 0xb8, 0x67, 0x4a, 0x14 },
    { 0x25, 0x19, 0x49, 0x8e, 0x80, 0xf1, 0x47, 0x8f,
      0x37, 0xba, 0x55, 0xbd, 0x6d, 0x27, 0x61, 0x8c },
    { 0x65, 0xdc, 0xc5, 0x7f, 0xcf, 0x62, 0x3a, 0x24,
      0x09, 0x4f, 0xcc, 0xa4, 0x0d, 0x35, 0x33, 0xf8 },
    { 0xdc, 0xf5, 0x66, 0xff, 0x29, 0x1c, 0x25, 0xbb,
      0xb8, 0x56, 0x8f, 0xc3, 0xd3, 0x76, 0xa6, 0xd9 },
    { 0x53, 0x0f, 0x8a, 0xfb, 0xc7, 0x45, 0x36, 0xb9,
      0xa9, 0x63, 0xb4, 0xf1, 0xc4, 0xcb, 0x73, 0x8b },
    { 0xd0, 0xd1, 0xc8, 0xa7, 0x99, 0x99, 0x6b, 0xf0,
      0x26, 0x5b, 0x98, 0xb5, 0xd4, 0x8a, 0xb9, 0x19 },
    { 0xb0, 0x94, 0xda, 0xc5, 0xd9, 0x34, 0x71, 0xbd,
      0xec, 0x1a, 0x50, 0x22, 0x70, 0xe3, 0xcc, 0x6c },
    { 0x76, 0xfc, 0x6e, 0xce, 0x0f, 0x4e, 0x17, 0x68,
      0xcd, 0xdf, 0x88, 0x53, 0xbb, 0x2d, 0x55, 0x1b },
    { 0x3a, 0x33, 0x7d, 0xbf, 0x46, 0xa7, 0x92, 0xc4,
      0x5e, 0x45, 0x49, 0x13, 0xfe, 0x2e, 0xa8, 0xf2 },
    { 0xa4, 0x4a, 0x82, 0x66, 0xee, 0x1c, 0x8e, 0xb0,
      0xc8, 0xb5, 0xd4, 0xcf, 0x5a, 0xe9, 0xf1, 0x9a },
};

int mbedtls_gcm_self_test( int verbose )
{
    mbedtls_gcm_context ctx;
    unsigned char buf[64];
    unsigned char tag_buf[16];
    int i, j, ret;
    mbedtls_cipher_id_t cipher = MBEDTLS_CIPHER_ID_AES;

    for( j = 0; j < 3; j++ )
    {
        int key_len = 128 + 64 * j;

        for( i = 0; i < MAX_TESTS; i++ )
        {
            mbedtls_gcm_init( &ctx );

            if( verbose != 0 )
                mbedtls_printf( "  AES-GCM-%3d #%d (%s): ",
                                key_len, i, "enc" );

            ret = mbedtls_gcm_setkey( &ctx, cipher, key2[key_index[i]],
                                      key_len );
            /*
             * AES-192 is an optional feature that may be unavailable when
             * there is an alternative underlying implementation i.e. when
             * MBEDTLS_AES_ALT is defined.
             */
            if( ret == MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED && key_len == 192 )
            {
                mbedtls_printf( "skipped\n" );
                break;
            }
            else if( ret != 0 )
            {
                goto exit;
            }

            ret = mbedtls_gcm_crypt_and_tag( &ctx, MBEDTLS_GCM_ENCRYPT,
                                        pt_len[i],
                                        iv3[iv_index[i]], iv_len[i],
                                        additional[add_index[i]], add_len[i],
                                        pt[pt_index[i]], buf, 16, tag_buf );
            if( ret != 0 )
                goto exit;

            if ( memcmp( buf, ct[j * 6 + i], pt_len[i] ) != 0 ||
                 memcmp( tag_buf, tag[j * 6 + i], 16 ) != 0 )
            {
                ret = 1;
                goto exit;
            }

            mbedtls_gcm_free( &ctx );

            if( verbose != 0 )
                mbedtls_printf( "passed\n" );

            mbedtls_gcm_init( &ctx );

            if( verbose != 0 )
                mbedtls_printf( "  AES-GCM-%3d #%d (%s): ",
                                key_len, i, "dec" );

            ret = mbedtls_gcm_setkey( &ctx, cipher, key2[key_index[i]],
                                      key_len );
            if( ret != 0 )
                goto exit;

            ret = mbedtls_gcm_crypt_and_tag( &ctx, MBEDTLS_GCM_DECRYPT,
                                        pt_len[i],
                                        iv3[iv_index[i]], iv_len[i],
                                        additional[add_index[i]], add_len[i],
                                        ct[j * 6 + i], buf, 16, tag_buf );

            if( ret != 0 )
                goto exit;

            if( memcmp( buf, pt[pt_index[i]], pt_len[i] ) != 0 ||
                memcmp( tag_buf, tag[j * 6 + i], 16 ) != 0 )
            {
                ret = 1;
                goto exit;
            }

            mbedtls_gcm_free( &ctx );

            if( verbose != 0 )
                mbedtls_printf( "passed\n" );

            mbedtls_gcm_init( &ctx );

            if( verbose != 0 )
                mbedtls_printf( "  AES-GCM-%3d #%d split (%s): ",
                                key_len, i, "enc" );

            ret = mbedtls_gcm_setkey( &ctx, cipher, key2[key_index[i]],
                                      key_len );
            if( ret != 0 )
                goto exit;

            ret = mbedtls_gcm_starts( &ctx, MBEDTLS_GCM_ENCRYPT,
                                      iv3[iv_index[i]], iv_len[i],
                                      additional[add_index[i]], add_len[i] );
            if( ret != 0 )
                goto exit;

            if( pt_len[i] > 32 )
            {
                size_t rest_len = pt_len[i] - 32;
                ret = mbedtls_gcm_update( &ctx, 32, pt[pt_index[i]], buf );
                if( ret != 0 )
                    goto exit;

                ret = mbedtls_gcm_update( &ctx, rest_len, pt[pt_index[i]] + 32,
                                  buf + 32 );
                if( ret != 0 )
                    goto exit;
            }
            else
            {
                ret = mbedtls_gcm_update( &ctx, pt_len[i], pt[pt_index[i]], buf );
                if( ret != 0 )
                    goto exit;
            }

            ret = mbedtls_gcm_finish( &ctx, tag_buf, 16 );
            if( ret != 0 )
                goto exit;

            if( memcmp( buf, ct[j * 6 + i], pt_len[i] ) != 0 ||
                memcmp( tag_buf, tag[j * 6 + i], 16 ) != 0 )
            {
                ret = 1;
                goto exit;
            }

            mbedtls_gcm_free( &ctx );

            if( verbose != 0 )
                mbedtls_printf( "passed\n" );

            mbedtls_gcm_init( &ctx );

            if( verbose != 0 )
                mbedtls_printf( "  AES-GCM-%3d #%d split (%s): ",
                                key_len, i, "dec" );

            ret = mbedtls_gcm_setkey( &ctx, cipher, key2[key_index[i]],
                                      key_len );
            if( ret != 0 )
                goto exit;

            ret = mbedtls_gcm_starts( &ctx, MBEDTLS_GCM_DECRYPT,
                              iv3[iv_index[i]], iv_len[i],
                              additional[add_index[i]], add_len[i] );
            if( ret != 0 )
                goto exit;

            if( pt_len[i] > 32 )
            {
                size_t rest_len = pt_len[i] - 32;
                ret = mbedtls_gcm_update( &ctx, 32, ct[j * 6 + i], buf );
                if( ret != 0 )
                    goto exit;

                ret = mbedtls_gcm_update( &ctx, rest_len, ct[j * 6 + i] + 32,
                                          buf + 32 );
                if( ret != 0 )
                    goto exit;
            }
            else
            {
                ret = mbedtls_gcm_update( &ctx, pt_len[i], ct[j * 6 + i],
                                          buf );
                if( ret != 0 )
                    goto exit;
            }

            ret = mbedtls_gcm_finish( &ctx, tag_buf, 16 );
            if( ret != 0 )
                goto exit;

            if( memcmp( buf, pt[pt_index[i]], pt_len[i] ) != 0 ||
                memcmp( tag_buf, tag[j * 6 + i], 16 ) != 0 )
            {
                ret = 1;
                goto exit;
            }

            mbedtls_gcm_free( &ctx );

            if( verbose != 0 )
                mbedtls_printf( "passed\n" );
        }
    }

    if( verbose != 0 )
        mbedtls_printf( "\n" );

    ret = 0;

exit:
    if( ret != 0 )
    {
        if( verbose != 0 )
            mbedtls_printf( "failed\n" );
        mbedtls_gcm_free( &ctx );
    }

    return( ret );
}
#endif /* MBEDTLS_SELF_TEST_OUT && MBEDTLS_AES_C && MBEDTLS_GCM_C */

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_AES_C) && defined(MBEDTLS_CCM_C)

/*
 * Examples 1 to 3 from SP800-38C Appendix C
 */

#define NB_TESTS 3
#define CCM_SELFTEST_PT_MAX_LEN 24
#define CCM_SELFTEST_CT_MAX_LEN 32
/*
 * The data is the same for all tests, only the used length changes
 */
static const unsigned char key3[] = {
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f
};

static const unsigned char iv4[] = {
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b
};

static const unsigned char ad[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13
};

static const unsigned char msg[CCM_SELFTEST_PT_MAX_LEN] = {
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
};

static const size_t iv_len_1[NB_TESTS] = { 7, 8,  12 };
static const size_t add_len_1[NB_TESTS] = { 8, 16, 20 };
static const size_t msg_len[NB_TESTS] = { 4, 16, 24 };
static const size_t tag_len[NB_TESTS] = { 4, 6,  8  };

static const unsigned char res[NB_TESTS][CCM_SELFTEST_CT_MAX_LEN] = {
    {   0x71, 0x62, 0x01, 0x5b, 0x4d, 0xac, 0x25, 0x5d },
    {   0xd2, 0xa1, 0xf0, 0xe0, 0x51, 0xea, 0x5f, 0x62,
        0x08, 0x1a, 0x77, 0x92, 0x07, 0x3d, 0x59, 0x3d,
        0x1f, 0xc6, 0x4f, 0xbf, 0xac, 0xcd },
    {   0xe3, 0xb2, 0x01, 0xa9, 0xf5, 0xb7, 0x1a, 0x7a,
        0x9b, 0x1c, 0xea, 0xec, 0xcd, 0x97, 0xe7, 0x0b,
        0x61, 0x76, 0xaa, 0xd9, 0xa4, 0x42, 0x8a, 0xa5,
        0x48, 0x43, 0x92, 0xfb, 0xc1, 0xb0, 0x99, 0x51 }
};

int mbedtls_ccm_self_test( int verbose )
{
    mbedtls_ccm_context ctx;
    /*
     * Some hardware accelerators require the input and output buffers
     * would be in RAM, because the flash is not accessible.
     * Use buffers on the stack to hold the test vectors data.
     */
    unsigned char plaintext[CCM_SELFTEST_PT_MAX_LEN];
    unsigned char ciphertext[CCM_SELFTEST_CT_MAX_LEN];
    size_t i;
    int ret;

    mbedtls_ccm_init( &ctx );

    if( mbedtls_ccm_setkey( &ctx, MBEDTLS_CIPHER_ID_AES, key3, 8 * sizeof key3 ) != 0 )
    {
        if( verbose != 0 )
            mbedtls_printf( "  CCM: setup failed" );

        return( 1 );
    }

    for( i = 0; i < NB_TESTS; i++ )
    {
        if( verbose != 0 )
            mbedtls_printf( "  CCM-AES #%u: ", (unsigned int) i + 1 );

        memset( plaintext, 0, CCM_SELFTEST_PT_MAX_LEN );
        memset( ciphertext, 0, CCM_SELFTEST_CT_MAX_LEN );
        memcpy( plaintext, msg, msg_len[i] );

        ret = mbedtls_ccm_encrypt_and_tag( &ctx, msg_len[i],
                                           iv4, iv_len_1[i], ad, add_len_1[i],
                                           plaintext, ciphertext,
                                           ciphertext + msg_len[i], tag_len[i] );

        if( ret != 0 ||
            memcmp( ciphertext, res[i], msg_len[i] + tag_len[i] ) != 0 )
        {
            if( verbose != 0 )
                mbedtls_printf( "failed\n" );

            return( 1 );
        }
        memset( plaintext, 0, CCM_SELFTEST_PT_MAX_LEN );

        ret = mbedtls_ccm_auth_decrypt( &ctx, msg_len[i],
                                        iv4, iv_len_1[i], ad, add_len_1[i],
                                        ciphertext, plaintext,
                                        ciphertext + msg_len[i], tag_len[i] );

        if( ret != 0 ||
            memcmp( plaintext, msg, msg_len[i] ) != 0 )
        {
            if( verbose != 0 )
                mbedtls_printf( "failed\n" );

            return( 1 );
        }

        if( verbose != 0 )
            mbedtls_printf( "passed\n" );
    }

    mbedtls_ccm_free( &ctx );

    if( verbose != 0 )
        mbedtls_printf( "\n" );

    return( 0 );
}

#endif /* MBEDTLS_SELF_TEST_OUT && MBEDTLS_AES_C && MBEDTLS_CCM_C */

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_CMAC_C)

/*
 * CMAC test data for SP800-38B
 * http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/AES_CMAC.pdf
 * http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/TDES_CMAC.pdf
 *
 * AES-CMAC-PRF-128 test data from RFC 4615
 * https://tools.ietf.org/html/rfc4615#page-4
 */

#define NB_CMAC_TESTS_PER_KEY 4
#define NB_PRF_TESTS 3

#if defined(MBEDTLS_AES_C) || defined(MBEDTLS_DES_C)
/* All CMAC test inputs are truncated from the same 64 byte buffer. */
static const unsigned char test_message[] = {
    /* PT */
    0x6b, 0xc1, 0xbe, 0xe2,     0x2e, 0x40, 0x9f, 0x96,
    0xe9, 0x3d, 0x7e, 0x11,     0x73, 0x93, 0x17, 0x2a,
    0xae, 0x2d, 0x8a, 0x57,     0x1e, 0x03, 0xac, 0x9c,
    0x9e, 0xb7, 0x6f, 0xac,     0x45, 0xaf, 0x8e, 0x51,
    0x30, 0xc8, 0x1c, 0x46,     0xa3, 0x5c, 0xe4, 0x11,
    0xe5, 0xfb, 0xc1, 0x19,     0x1a, 0x0a, 0x52, 0xef,
    0xf6, 0x9f, 0x24, 0x45,     0xdf, 0x4f, 0x9b, 0x17,
    0xad, 0x2b, 0x41, 0x7b,     0xe6, 0x6c, 0x37, 0x10
};
#endif /* MBEDTLS_AES_C || MBEDTLS_DES_C */

#if defined(MBEDTLS_AES_C)
/* Truncation point of message for AES CMAC tests  */
static const  unsigned int  aes_message_lengths[NB_CMAC_TESTS_PER_KEY] = {
    /* Mlen */
    0,
    16,
    20,
    64
};

/* CMAC-AES128 Test Data */
static const unsigned char aes_128_key[16] = {
    0x2b, 0x7e, 0x15, 0x16,     0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88,     0x09, 0xcf, 0x4f, 0x3c
};
static const unsigned char aes_128_subkeys[2][MBEDTLS_AES_BLOCK_SIZE] = {
    {
        /* K1 */
        0xfb, 0xee, 0xd6, 0x18,     0x35, 0x71, 0x33, 0x66,
        0x7c, 0x85, 0xe0, 0x8f,     0x72, 0x36, 0xa8, 0xde
    },
    {
        /* K2 */
        0xf7, 0xdd, 0xac, 0x30,     0x6a, 0xe2, 0x66, 0xcc,
        0xf9, 0x0b, 0xc1, 0x1e,     0xe4, 0x6d, 0x51, 0x3b
    }
};
static const unsigned char aes_128_expected_result[NB_CMAC_TESTS_PER_KEY][MBEDTLS_AES_BLOCK_SIZE] = {
    {
        /* Example #1 */
        0xbb, 0x1d, 0x69, 0x29,     0xe9, 0x59, 0x37, 0x28,
        0x7f, 0xa3, 0x7d, 0x12,     0x9b, 0x75, 0x67, 0x46
    },
    {
        /* Example #2 */
        0x07, 0x0a, 0x16, 0xb4,     0x6b, 0x4d, 0x41, 0x44,
        0xf7, 0x9b, 0xdd, 0x9d,     0xd0, 0x4a, 0x28, 0x7c
    },
    {
        /* Example #3 */
        0x7d, 0x85, 0x44, 0x9e,     0xa6, 0xea, 0x19, 0xc8,
        0x23, 0xa7, 0xbf, 0x78,     0x83, 0x7d, 0xfa, 0xde
    },
    {
        /* Example #4 */
        0x51, 0xf0, 0xbe, 0xbf,     0x7e, 0x3b, 0x9d, 0x92,
        0xfc, 0x49, 0x74, 0x17,     0x79, 0x36, 0x3c, 0xfe
    }
};

/* CMAC-AES192 Test Data */
static const unsigned char aes_192_key[24] = {
    0x8e, 0x73, 0xb0, 0xf7,     0xda, 0x0e, 0x64, 0x52,
    0xc8, 0x10, 0xf3, 0x2b,     0x80, 0x90, 0x79, 0xe5,
    0x62, 0xf8, 0xea, 0xd2,     0x52, 0x2c, 0x6b, 0x7b
};
static const unsigned char aes_192_subkeys[2][MBEDTLS_AES_BLOCK_SIZE] = {
    {
        /* K1 */
        0x44, 0x8a, 0x5b, 0x1c,     0x93, 0x51, 0x4b, 0x27,
        0x3e, 0xe6, 0x43, 0x9d,     0xd4, 0xda, 0xa2, 0x96
    },
    {
        /* K2 */
        0x89, 0x14, 0xb6, 0x39,     0x26, 0xa2, 0x96, 0x4e,
        0x7d, 0xcc, 0x87, 0x3b,     0xa9, 0xb5, 0x45, 0x2c
    }
};
static const unsigned char aes_192_expected_result[NB_CMAC_TESTS_PER_KEY][MBEDTLS_AES_BLOCK_SIZE] = {
    {
        /* Example #1 */
        0xd1, 0x7d, 0xdf, 0x46,     0xad, 0xaa, 0xcd, 0xe5,
        0x31, 0xca, 0xc4, 0x83,     0xde, 0x7a, 0x93, 0x67
    },
    {
        /* Example #2 */
        0x9e, 0x99, 0xa7, 0xbf,     0x31, 0xe7, 0x10, 0x90,
        0x06, 0x62, 0xf6, 0x5e,     0x61, 0x7c, 0x51, 0x84
    },
    {
        /* Example #3 */
        0x3d, 0x75, 0xc1, 0x94,     0xed, 0x96, 0x07, 0x04,
        0x44, 0xa9, 0xfa, 0x7e,     0xc7, 0x40, 0xec, 0xf8
    },
    {
        /* Example #4 */
        0xa1, 0xd5, 0xdf, 0x0e,     0xed, 0x79, 0x0f, 0x79,
        0x4d, 0x77, 0x58, 0x96,     0x59, 0xf3, 0x9a, 0x11
    }
};

/* CMAC-AES256 Test Data */
static const unsigned char aes_256_key[32] = {
    0x60, 0x3d, 0xeb, 0x10,     0x15, 0xca, 0x71, 0xbe,
    0x2b, 0x73, 0xae, 0xf0,     0x85, 0x7d, 0x77, 0x81,
    0x1f, 0x35, 0x2c, 0x07,     0x3b, 0x61, 0x08, 0xd7,
    0x2d, 0x98, 0x10, 0xa3,     0x09, 0x14, 0xdf, 0xf4
};
static const unsigned char aes_256_subkeys[2][MBEDTLS_AES_BLOCK_SIZE] = {
    {
        /* K1 */
        0xca, 0xd1, 0xed, 0x03,     0x29, 0x9e, 0xed, 0xac,
        0x2e, 0x9a, 0x99, 0x80,     0x86, 0x21, 0x50, 0x2f
    },
    {
        /* K2 */
        0x95, 0xa3, 0xda, 0x06,     0x53, 0x3d, 0xdb, 0x58,
        0x5d, 0x35, 0x33, 0x01,     0x0c, 0x42, 0xa0, 0xd9
    }
};
static const unsigned char aes_256_expected_result[NB_CMAC_TESTS_PER_KEY][MBEDTLS_AES_BLOCK_SIZE] = {
    {
        /* Example #1 */
        0x02, 0x89, 0x62, 0xf6,     0x1b, 0x7b, 0xf8, 0x9e,
        0xfc, 0x6b, 0x55, 0x1f,     0x46, 0x67, 0xd9, 0x83
    },
    {
        /* Example #2 */
        0x28, 0xa7, 0x02, 0x3f,     0x45, 0x2e, 0x8f, 0x82,
        0xbd, 0x4b, 0xf2, 0x8d,     0x8c, 0x37, 0xc3, 0x5c
    },
    {
        /* Example #3 */
        0x15, 0x67, 0x27, 0xdc,     0x08, 0x78, 0x94, 0x4a,
        0x02, 0x3c, 0x1f, 0xe0,     0x3b, 0xad, 0x6d, 0x93
    },
    {
        /* Example #4 */
        0xe1, 0x99, 0x21, 0x90,     0x54, 0x9f, 0x6e, 0xd5,
        0x69, 0x6a, 0x2c, 0x05,     0x6c, 0x31, 0x54, 0x10
    }
};
#endif /* MBEDTLS_AES_C */

#if defined(MBEDTLS_DES_C)
/* Truncation point of message for 3DES CMAC tests  */
static const unsigned int des3_message_lengths[NB_CMAC_TESTS_PER_KEY] = {
    0,
    16,
    20,
    32
};

/* CMAC-TDES (Generation) - 2 Key Test Data */
static const unsigned char des3_2key_key[24] = {
    /* Key1 */
    0x01, 0x23, 0x45, 0x67,     0x89, 0xab, 0xcd, 0xef,
    /* Key2 */
    0x23, 0x45, 0x67, 0x89,     0xab, 0xcd, 0xEF, 0x01,
    /* Key3 */
    0x01, 0x23, 0x45, 0x67,     0x89, 0xab, 0xcd, 0xef
};
static const unsigned char des3_2key_subkeys[2][8] = {
    {
        /* K1 */
        0x0d, 0xd2, 0xcb, 0x7a,     0x3d, 0x88, 0x88, 0xd9
    },
    {
        /* K2 */
        0x1b, 0xa5, 0x96, 0xf4,     0x7b, 0x11, 0x11, 0xb2
    }
};
static const unsigned char des3_2key_expected_result[NB_CMAC_TESTS_PER_KEY][MBEDTLS_DES3_BLOCK_SIZE] = {
    {
        /* Sample #1 */
        0x79, 0xce, 0x52, 0xa7,     0xf7, 0x86, 0xa9, 0x60
    },
    {
        /* Sample #2 */
        0xcc, 0x18, 0xa0, 0xb7,     0x9a, 0xf2, 0x41, 0x3b
    },
    {
        /* Sample #3 */
        0xc0, 0x6d, 0x37, 0x7e,     0xcd, 0x10, 0x19, 0x69
    },
    {
        /* Sample #4 */
        0x9c, 0xd3, 0x35, 0x80,     0xf9, 0xb6, 0x4d, 0xfb
    }
};

/* CMAC-TDES (Generation) - 3 Key Test Data */
static const unsigned char des3_3key_key[24] = {
    /* Key1 */
    0x01, 0x23, 0x45, 0x67,     0x89, 0xaa, 0xcd, 0xef,
    /* Key2 */
    0x23, 0x45, 0x67, 0x89,     0xab, 0xcd, 0xef, 0x01,
    /* Key3 */
    0x45, 0x67, 0x89, 0xab,     0xcd, 0xef, 0x01, 0x23
};
static const unsigned char des3_3key_subkeys[2][8] = {
    {
        /* K1 */
        0x9d, 0x74, 0xe7, 0x39,     0x33, 0x17, 0x96, 0xc0
    },
    {
        /* K2 */
        0x3a, 0xe9, 0xce, 0x72,     0x66, 0x2f, 0x2d, 0x9b
    }
};
static const unsigned char des3_3key_expected_result[NB_CMAC_TESTS_PER_KEY][MBEDTLS_DES3_BLOCK_SIZE] = {
    {
        /* Sample #1 */
        0x7d, 0xb0, 0xd3, 0x7d,     0xf9, 0x36, 0xc5, 0x50
    },
    {
        /* Sample #2 */
        0x30, 0x23, 0x9c, 0xf1,     0xf5, 0x2e, 0x66, 0x09
    },
    {
        /* Sample #3 */
        0x6c, 0x9f, 0x3e, 0xe4,     0x92, 0x3f, 0x6b, 0xe2
    },
    {
        /* Sample #4 */
        0x99, 0x42, 0x9b, 0xd0,     0xbF, 0x79, 0x04, 0xe5
    }
};

#endif /* MBEDTLS_DES_C */

#if defined(MBEDTLS_AES_C)
/* AES AES-CMAC-PRF-128 Test Data */
static const unsigned char PRFK[] = {
    /* Key */
    0x00, 0x01, 0x02, 0x03,     0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b,     0x0c, 0x0d, 0x0e, 0x0f,
    0xed, 0xcb
};

/* Sizes in bytes */
static const size_t PRFKlen[NB_PRF_TESTS] = {
    18,
    16,
    10
};

/* Message */
static const unsigned char PRFM[] = {
    0x00, 0x01, 0x02, 0x03,     0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b,     0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13
};

static const unsigned char PRFT[NB_PRF_TESTS][16] = {
    {
        0x84, 0xa3, 0x48, 0xa4,     0xa4, 0x5d, 0x23, 0x5b,
        0xab, 0xff, 0xfc, 0x0d,     0x2b, 0x4d, 0xa0, 0x9a
    },
    {
        0x98, 0x0a, 0xe8, 0x7b,     0x5f, 0x4c, 0x9c, 0x52,
        0x14, 0xf5, 0xb6, 0xa8,     0x45, 0x5e, 0x4c, 0x2d
    },
    {
        0x29, 0x0d, 0x9e, 0x11,     0x2e, 0xdb, 0x09, 0xee,
        0x14, 0x1f, 0xcf, 0x64,     0xc0, 0xb7, 0x2f, 0x3d
    }
};
#endif /* MBEDTLS_AES_C */

static int cmac_test_subkeys( int verbose,
                              const char* testname,
                              const unsigned char* key4,
                              int keybits,
                              const unsigned char* subkeys,
                              mbedtls_cipher_type_t cipher_type,
                              int block_size,
                              int num_tests )
{
    int i, ret = 0;
    mbedtls_cipher_context_t ctx;
    const mbedtls_cipher_info_t *cipher_info;
    unsigned char K1[MBEDTLS_CIPHER_BLKSIZE_MAX];
    unsigned char K2[MBEDTLS_CIPHER_BLKSIZE_MAX];

    cipher_info = mbedtls_cipher_info_from_type( cipher_type );
    if( cipher_info == NULL )
    {
        /* Failing at this point must be due to a build issue */
        return( MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE );
    }

    for( i = 0; i < num_tests; i++ )
    {
        if( verbose != 0 )
            mbedtls_printf( "  %s CMAC subkey #%u: ", testname, i + 1 );

        mbedtls_cipher_init( &ctx );

        if( ( ret = mbedtls_cipher_setup( &ctx, cipher_info ) ) != 0 )
        {
            if( verbose != 0 )
                mbedtls_printf( "test execution failed\n" );

            goto cleanup;
        }

        if( ( ret = mbedtls_cipher_setkey( &ctx, key4, keybits,
                                       MBEDTLS_ENCRYPT ) ) != 0 )
        {
            if( verbose != 0 )
                mbedtls_printf( "test execution failed\n" );

            goto cleanup;
        }

        ret = cmac_generate_subkeys( &ctx, K1, K2 );
        if( ret != 0 )
        {
           if( verbose != 0 )
                mbedtls_printf( "failed\n" );

            goto cleanup;
        }

        if( ( ret = memcmp( K1, subkeys, block_size ) ) != 0  ||
            ( ret = memcmp( K2, &subkeys[block_size], block_size ) ) != 0 )
        {
            if( verbose != 0 )
                mbedtls_printf( "failed\n" );

            goto cleanup;
        }

        if( verbose != 0 )
            mbedtls_printf( "passed\n" );

        mbedtls_cipher_free( &ctx );
    }

    ret = 0;
    goto exit;

cleanup:
    mbedtls_cipher_free( &ctx );

exit:
    return( ret );
}

static int cmac_test_wth_cipher( int verbose,
                                 const char* testname,
                                 const unsigned char* key5,
                                 int keybits,
                                 const unsigned char* messages,
                                 const unsigned int message_lengths[4],
                                 const unsigned char* expected_result,
                                 mbedtls_cipher_type_t cipher_type,
                                 int block_size,
                                 int num_tests )
{
    const mbedtls_cipher_info_t *cipher_info;
    int i, ret = 0;
    unsigned char output[MBEDTLS_CIPHER_BLKSIZE_MAX];

    cipher_info = mbedtls_cipher_info_from_type( cipher_type );
    if( cipher_info == NULL )
    {
        /* Failing at this point must be due to a build issue */
        ret = MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE;
        goto exit;
    }

    for( i = 0; i < num_tests; i++ )
    {
        if( verbose != 0 )
            mbedtls_printf( "  %s CMAC #%u: ", testname, i + 1 );

        if( ( ret = mbedtls_cipher_cmac( cipher_info, key5, keybits, messages,
                                         message_lengths[i], output ) ) != 0 )
        {
            if( verbose != 0 )
                mbedtls_printf( "failed\n" );
            goto exit;
        }

        if( ( ret = memcmp( output, &expected_result[i * block_size], block_size ) ) != 0 )
        {
            if( verbose != 0 )
                mbedtls_printf( "failed\n" );
            goto exit;
        }

        if( verbose != 0 )
            mbedtls_printf( "passed\n" );
    }
    ret = 0;

exit:
    return( ret );
}

#if defined(MBEDTLS_AES_C)
static int test_aes128_cmac_prf( int verbose )
{
    int i;
    int ret;
    unsigned char output[MBEDTLS_AES_BLOCK_SIZE];

    for( i = 0; i < NB_PRF_TESTS; i++ )
    {
        mbedtls_printf( "  AES CMAC 128 PRF #%u: ", i );
        ret = mbedtls_aes_cmac_prf_128( PRFK, PRFKlen[i], PRFM, 20, output );
        if( ret != 0 ||
            memcmp( output, PRFT[i], MBEDTLS_AES_BLOCK_SIZE ) != 0 )
        {

            if( verbose != 0 )
                mbedtls_printf( "failed\n" );

            return( ret );
        }
        else if( verbose != 0 )
        {
            mbedtls_printf( "passed\n" );
        }
    }
    return( ret );
}
#endif /* MBEDTLS_AES_C */

int mbedtls_cmac_self_test( int verbose )
{
    int ret;

#if defined(MBEDTLS_AES_C)
    /* AES-128 */
    if( ( ret = cmac_test_subkeys( verbose,
                                   "AES 128",
                                   aes_128_key,
                                   128,
                                   (const unsigned char*)aes_128_subkeys,
                                   MBEDTLS_CIPHER_AES_128_ECB,
                                   MBEDTLS_AES_BLOCK_SIZE,
                                   NB_CMAC_TESTS_PER_KEY ) ) != 0 )
    {
        return( ret );
    }

    if( ( ret = cmac_test_wth_cipher( verbose,
                                      "AES 128",
                                      aes_128_key,
                                      128,
                                      test_message,
                                      aes_message_lengths,
                                      (const unsigned char*)aes_128_expected_result,
                                      MBEDTLS_CIPHER_AES_128_ECB,
                                      MBEDTLS_AES_BLOCK_SIZE,
                                      NB_CMAC_TESTS_PER_KEY ) ) != 0 )
    {
        return( ret );
    }

    /* AES-192 */
    if( ( ret = cmac_test_subkeys( verbose,
                                   "AES 192",
                                   aes_192_key,
                                   192,
                                   (const unsigned char*)aes_192_subkeys,
                                   MBEDTLS_CIPHER_AES_192_ECB,
                                   MBEDTLS_AES_BLOCK_SIZE,
                                   NB_CMAC_TESTS_PER_KEY ) ) != 0 )
    {
        return( ret );
    }

    if( ( ret = cmac_test_wth_cipher( verbose,
                                      "AES 192",
                                      aes_192_key,
                                      192,
                                      test_message,
                                      aes_message_lengths,
                                      (const unsigned char*)aes_192_expected_result,
                                      MBEDTLS_CIPHER_AES_192_ECB,
                                      MBEDTLS_AES_BLOCK_SIZE,
                                      NB_CMAC_TESTS_PER_KEY ) ) != 0 )
    {
        return( ret );
    }

    /* AES-256 */
    if( ( ret = cmac_test_subkeys( verbose,
                                   "AES 256",
                                   aes_256_key,
                                   256,
                                   (const unsigned char*)aes_256_subkeys,
                                   MBEDTLS_CIPHER_AES_256_ECB,
                                   MBEDTLS_AES_BLOCK_SIZE,
                                   NB_CMAC_TESTS_PER_KEY ) ) != 0 )
    {
        return( ret );
    }

    if( ( ret = cmac_test_wth_cipher ( verbose,
                                       "AES 256",
                                       aes_256_key,
                                       256,
                                       test_message,
                                       aes_message_lengths,
                                       (const unsigned char*)aes_256_expected_result,
                                       MBEDTLS_CIPHER_AES_256_ECB,
                                       MBEDTLS_AES_BLOCK_SIZE,
                                       NB_CMAC_TESTS_PER_KEY ) ) != 0 )
    {
        return( ret );
    }
#endif /* MBEDTLS_AES_C */

#if defined(MBEDTLS_DES_C)
    /* 3DES 2 key */
    if( ( ret = cmac_test_subkeys( verbose,
                                   "3DES 2 key",
                                   des3_2key_key,
                                   192,
                                   (const unsigned char*)des3_2key_subkeys,
                                   MBEDTLS_CIPHER_DES_EDE3_ECB,
                                   MBEDTLS_DES3_BLOCK_SIZE,
                                   NB_CMAC_TESTS_PER_KEY ) ) != 0 )
    {
        return( ret );
    }

    if( ( ret = cmac_test_wth_cipher( verbose,
                                      "3DES 2 key",
                                      des3_2key_key,
                                      192,
                                      test_message,
                                      des3_message_lengths,
                                      (const unsigned char*)des3_2key_expected_result,
                                      MBEDTLS_CIPHER_DES_EDE3_ECB,
                                      MBEDTLS_DES3_BLOCK_SIZE,
                                      NB_CMAC_TESTS_PER_KEY ) ) != 0 )
    {
        return( ret );
    }

    /* 3DES 3 key */
    if( ( ret = cmac_test_subkeys( verbose,
                                   "3DES 3 key",
                                   des3_3key_key,
                                   192,
                                   (const unsigned char*)des3_3key_subkeys,
                                   MBEDTLS_CIPHER_DES_EDE3_ECB,
                                   MBEDTLS_DES3_BLOCK_SIZE,
                                   NB_CMAC_TESTS_PER_KEY ) ) != 0 )
    {
        return( ret );
    }

    if( ( ret = cmac_test_wth_cipher( verbose,
                                      "3DES 3 key",
                                      des3_3key_key,
                                      192,
                                      test_message,
                                      des3_message_lengths,
                                      (const unsigned char*)des3_3key_expected_result,
                                      MBEDTLS_CIPHER_DES_EDE3_ECB,
                                      MBEDTLS_DES3_BLOCK_SIZE,
                                      NB_CMAC_TESTS_PER_KEY ) ) != 0 )
    {
        return( ret );
    }
#endif /* MBEDTLS_DES_C */

#if defined(MBEDTLS_AES_C)
    if( ( ret = test_aes128_cmac_prf( verbose ) ) != 0 )
        return( ret );
#endif /* MBEDTLS_AES_C */

    if( verbose != 0 )
        mbedtls_printf( "\n" );

    return( 0 );
}

#endif /* MBEDTLS_SELF_TEST_OUT && MBEDTLS_CMAC_C */

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_BASE64_C)

static const unsigned char base64_test_dec[64] =
{
    0x24, 0x48, 0x6E, 0x56, 0x87, 0x62, 0x5A, 0xBD,
    0xBF, 0x17, 0xD9, 0xA2, 0xC4, 0x17, 0x1A, 0x01,
    0x94, 0xED, 0x8F, 0x1E, 0x11, 0xB3, 0xD7, 0x09,
    0x0C, 0xB6, 0xE9, 0x10, 0x6F, 0x22, 0xEE, 0x13,
    0xCA, 0xB3, 0x07, 0x05, 0x76, 0xC9, 0xFA, 0x31,
    0x6C, 0x08, 0x34, 0xFF, 0x8D, 0xC2, 0x6C, 0x38,
    0x00, 0x43, 0xE9, 0x54, 0x97, 0xAF, 0x50, 0x4B,
    0xD1, 0x41, 0xBA, 0x95, 0x31, 0x5A, 0x0B, 0x97
};

static const unsigned char base64_test_enc[] =
    "JEhuVodiWr2/F9mixBcaAZTtjx4Rs9cJDLbpEG8i7hPK"
    "swcFdsn6MWwINP+Nwmw4AEPpVJevUEvRQbqVMVoLlw==";

/*
 * Checkup routine
 */
int mbedtls_base64_self_test( int verbose )
{
    size_t len;
    const unsigned char *src;
    unsigned char buffer[128];

    if( verbose != 0 )
        mbedtls_printf( "  Base64 encoding test: " );

    src = base64_test_dec;

    if( mbedtls_base64_encode( buffer, sizeof( buffer ), &len, src, 64 ) != 0 ||
         memcmp( base64_test_enc, buffer, 88 ) != 0 )
    {
        if( verbose != 0 )
            mbedtls_printf( "failed\n" );

        return( 1 );
    }

    if( verbose != 0 )
        mbedtls_printf( "passed\n  Base64 decoding test: " );

    src = base64_test_enc;

    if( mbedtls_base64_decode( buffer, sizeof( buffer ), &len, src, 88 ) != 0 ||
         memcmp( base64_test_dec, buffer, 64 ) != 0 )
    {
        if( verbose != 0 )
            mbedtls_printf( "failed\n" );

        return( 1 );
    }

    if( verbose != 0 )
        mbedtls_printf( "passed\n\n" );

    return( 0 );
}

#endif /* MBEDTLS_SELF_TEST_OUT && MBEDTLS_BASE64_C*/

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_BIGNUM_C)

#define GCD_PAIR_COUNT  3

static const int gcd_pairs[GCD_PAIR_COUNT][3] =
{
    { 693, 609, 21 },
    { 1764, 868, 28 },
    { 768454923, 542167814, 1 }
};

/*
 * Checkup routine
 */
int mbedtls_mpi_self_test( int verbose )
{
    int ret, i;
    mbedtls_mpi A, E, N, X, Y, U, V;

    mbedtls_mpi_init( &A ); mbedtls_mpi_init( &E ); mbedtls_mpi_init( &N ); mbedtls_mpi_init( &X );
    mbedtls_mpi_init( &Y ); mbedtls_mpi_init( &U ); mbedtls_mpi_init( &V );

    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &A, 16,
        "EFE021C2645FD1DC586E69184AF4A31E" \
        "D5F53E93B5F123FA41680867BA110131" \
        "944FE7952E2517337780CB0DB80E61AA" \
        "E7C8DDC6C5C6AADEB34EB38A2F40D5E6" ) );

    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &E, 16,
        "B2E7EFD37075B9F03FF989C7C5051C20" \
        "34D2A323810251127E7BF8625A4F49A5" \
        "F3E27F4DA8BD59C47D6DAABA4C8127BD" \
        "5B5C25763222FEFCCFC38B832366C29E" ) );

    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &N, 16,
        "0066A198186C18C10B2F5ED9B522752A" \
        "9830B69916E535C8F047518A889A43A5" \
        "94B6BED27A168D31D4A52F88925AA8F5" ) );

    MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi( &X, &A, &N ) );

    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &U, 16,
        "602AB7ECA597A3D6B56FF9829A5E8B85" \
        "9E857EA95A03512E2BAE7391688D264A" \
        "A5663B0341DB9CCFD2C4C5F421FEC814" \
        "8001B72E848A38CAE1C65F78E56ABDEF" \
        "E12D3C039B8A02D6BE593F0BBBDA56F1" \
        "ECF677152EF804370C1A305CAF3B5BF1" \
        "30879B56C61DE584A0F53A2447A51E" ) );

    if( verbose != 0 )
        mbedtls_printf( "  MPI test #1 (mul_mpi): " );

    if( mbedtls_mpi_cmp_mpi( &X, &U ) != 0 )
    {
        if( verbose != 0 )
            mbedtls_printf( "failed\n" );

        ret = 1;
        goto cleanup;
    }

    if( verbose != 0 )
        mbedtls_printf( "passed\n" );

    MBEDTLS_MPI_CHK( mbedtls_mpi_div_mpi( &X, &Y, &A, &N ) );

    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &U, 16,
        "256567336059E52CAE22925474705F39A94" ) );

    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &V, 16,
        "6613F26162223DF488E9CD48CC132C7A" \
        "0AC93C701B001B092E4E5B9F73BCD27B" \
        "9EE50D0657C77F374E903CDFA4C642" ) );

    if( verbose != 0 )
        mbedtls_printf( "  MPI test #2 (div_mpi): " );

    if( mbedtls_mpi_cmp_mpi( &X, &U ) != 0 ||
        mbedtls_mpi_cmp_mpi( &Y, &V ) != 0 )
    {
        if( verbose != 0 )
            mbedtls_printf( "failed\n" );

        ret = 1;
        goto cleanup;
    }

    if( verbose != 0 )
        mbedtls_printf( "passed\n" );

    MBEDTLS_MPI_CHK( mbedtls_mpi_exp_mod( &X, &A, &E, &N, NULL ) );

    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &U, 16,
        "36E139AEA55215609D2816998ED020BB" \
        "BD96C37890F65171D948E9BC7CBAA4D9" \
        "325D24D6A3C12710F10A09FA08AB87" ) );

    if( verbose != 0 )
        mbedtls_printf( "  MPI test #3 (exp_mod): " );

    if( mbedtls_mpi_cmp_mpi( &X, &U ) != 0 )
    {
        if( verbose != 0 )
            mbedtls_printf( "failed\n" );

        ret = 1;
        goto cleanup;
    }

    if( verbose != 0 )
        mbedtls_printf( "passed\n" );

    MBEDTLS_MPI_CHK( mbedtls_mpi_inv_mod( &X, &A, &N ) );

    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &U, 16,
        "003A0AAEDD7E784FC07D8F9EC6E3BFD5" \
        "C3DBA76456363A10869622EAC2DD84EC" \
        "C5B8A74DAC4D09E03B5E0BE779F2DF61" ) );

    if( verbose != 0 )
        mbedtls_printf( "  MPI test #4 (inv_mod): " );

    if( mbedtls_mpi_cmp_mpi( &X, &U ) != 0 )
    {
        if( verbose != 0 )
            mbedtls_printf( "failed\n" );

        ret = 1;
        goto cleanup;
    }

    if( verbose != 0 )
        mbedtls_printf( "passed\n" );

    if( verbose != 0 )
        mbedtls_printf( "  MPI test #5 (simple gcd): " );

    for( i = 0; i < GCD_PAIR_COUNT; i++ )
    {
        MBEDTLS_MPI_CHK( mbedtls_mpi_lset( &X, gcd_pairs[i][0] ) );
        MBEDTLS_MPI_CHK( mbedtls_mpi_lset( &Y, gcd_pairs[i][1] ) );

        MBEDTLS_MPI_CHK( mbedtls_mpi_gcd( &A, &X, &Y ) );

        if( mbedtls_mpi_cmp_int( &A, gcd_pairs[i][2] ) != 0 )
        {
            if( verbose != 0 )
                mbedtls_printf( "failed at %d\n", i );

            ret = 1;
            goto cleanup;
        }
    }

    if( verbose != 0 )
        mbedtls_printf( "passed\n" );

cleanup:

    if( ret != 0 && verbose != 0 )
        mbedtls_printf( "Unexpected error, return code = %08X\n", ret );

    mbedtls_mpi_free( &A ); mbedtls_mpi_free( &E ); mbedtls_mpi_free( &N ); mbedtls_mpi_free( &X );
    mbedtls_mpi_free( &Y ); mbedtls_mpi_free( &U ); mbedtls_mpi_free( &V );

    if( verbose != 0 )
        mbedtls_printf( "\n" );

    return( ret );
}

#ifdef CONFIG_HW_SECURITY_ENGINE
#define DATA_LENGTH         16
#define DATA_LENGTH_1024    32
const uint32_t DIN_M_1024[DATA_LENGTH_1024] = {0x5884d1a0, 0xCAD23435, 0x511c06ff, 0xc06f0b56, 0x30220837, 0x359fa0a6, 0x36e403ec, 0x305f361c,
                            0xe5027eaa, 0xf5bce8ac, 0xfb24549b, 0x17107865, 0x1824daab, 0xc59ffade, 0x6bcf71eb, 0x9fc6be97,
                            0x0168c993, 0xd436a0e4, 0x635486e6, 0x602ef418, 0x4f7b1bbe, 0xee52df5c, 0x165e3058, 0x85edfb11,
                            0x6a3af995, 0x385b88bd, 0x42c20249, 0x2f4406f5, 0x7f763d49, 0x01348df6, 0x9a2d611e, 0x8e0bef74};

const uint32_t DIN_N_1024[DATA_LENGTH_1024] = {0xe9bf8549, 0x16d8ba65, 0x25f213f8, 0xe505f953, 0x67db31e1, 0x945a19db, 0x4f139c7c, 0x17f974ca,
                            0x5dcb6c9a, 0x43abcd85, 0x18b23167, 0x4e1788b7, 0xbe7d7d51, 0xd6e891c4, 0xc5853890, 0x197372b4,
                            0x114ad23c, 0x9edfe8c2, 0xfec830b3, 0x45472f87, 0x271b6693, 0xe8a24392, 0xc41fb462, 0x19f906f3,
                            0x687e80ba, 0xb2f0e540, 0x26954c0e, 0x3f3a1d9d, 0x81585acb, 0xb7b666d7, 0x3d3c5691, 0xb0607d89};

const uint32_t DIN_E_1024[DATA_LENGTH_1024] = {0x6a3921ab, 0xc1cc96fe, 0xc84d342f, 0xf9c0b12d, 0x09c1f7f8, 0x6137dcf5, 0xe9a9c38e, 0xc654f73e,
                            0x2c64bff9, 0x78edc66e, 0xfbc2a446, 0xbf336a12, 0x260e17d6, 0x5c3e1e2e, 0xd685ff19, 0x3ebe219b,
                            0x9d70b03d, 0xa35e98a8, 0xf9b1f0c2, 0x35b88715, 0xff012353, 0xd4010658, 0xf08f42ff, 0x8d14f341,
                            0x6971528b, 0x3a63e740, 0x043a4ae3, 0xc7507145, 0x7272a128, 0x2d560ec6, 0x5152d088, 0x499d46cf};

const uint32_t DIN_C_1024[DATA_LENGTH_1024] = {0x428b5d3b, 0x680eb1be, 0xe3386abe, 0x6895b9e6, 0x7ebb66a4, 0xe5087aee, 0x77a8f598, 0xaff76a2b,
                            0xcff059fb, 0x3e1bb5fd, 0xca129b90, 0x70082b8a, 0xc9fe365a, 0x9282d34e, 0xdf64e694, 0xa83cd527,
                            0x2a3f3b81, 0x93001d2a, 0x8028cc7d, 0x3cec5f3f, 0xdf818b4a, 0x4a6d1408, 0xcd8564c9, 0x8b3f2511,
                            0xdf617ac9, 0x35c0ba97, 0x7201b5ef, 0x9e3e8384, 0xfae0e7f0, 0xaa7ea82b, 0xbc539cc5, 0x2bf27be9};

const uint32_t RSA_W0_1024 = 0x99178307;
const uint32_t RSA_W1_1024 = 0x298bc7dc;

const uint32_t DIN_M[DATA_LENGTH] = {0x00000287, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000};

const uint32_t DIN_N[DATA_LENGTH] = {0x000003E5, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000};

const uint32_t DIN_E[DATA_LENGTH] = {0x000002E7, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000};

const uint32_t DIN_C[DATA_LENGTH] = {0x000000b9, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000};

const uint32_t RSA_W0 = 0xCE717E13;
const uint32_t RSA_W1 = 0x5C6FF3AC;

const uint32_t Expected512[DATA_LENGTH] = {0x000000ae, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000};

const uint32_t Expected1024[DATA_LENGTH_1024] =
                    {0xeab582dd, 0x72b641c8, 0x9923f874, 0xe6979fbf, 0xa8549c4b, 0xcfeb1b21, 0x9e74153c, 0x87d83c72,
                     0xf7964d37, 0x161593f1, 0x5553303e, 0xf8c41372, 0x1e37f9e5, 0x327855b6, 0xfd72fec7, 0x088f84e1,
                     0x5c477240, 0x69fb0dfc, 0x7a59cf64, 0x3f42d882, 0x2c6e049a, 0x3842f987, 0xa3b43198, 0x55b54d4e,
                     0x0bb81ba5, 0x44dbcd1e, 0x2eaedc6b, 0xf976a75c, 0x1b052d11, 0xd2fc0938, 0xb48a7f26, 0x091b98cb};

#define MPI_PRINT(en, string, X) do{\
    if(en){\
        int i;\
        mbedtls_printf(string " %d %d %p\n", (X)->s, (X)->n, (X)->p);\
        for(i=0; i<(X)->n;)\
        {\
            mbedtls_printf("0x%08x ", *((X)->p + i));\
            if(++i % 8 == 0)\
                mbedtls_printf("\n");\
        }\
        mbedtls_printf("\n");\
    }\
}while(0);

int mbedtls_mpi_exp_mod_self_test_512( int verbose )
{
    mbedtls_mpi M, E, N;
    int ret = 0;

    /*********************** RSA 512 *****************************/
    if(verbose)
        mbedtls_printf( "  MPI test #6 (exp mod 512): \n" );

    mbedtls_mpi_init( &M ); mbedtls_mpi_init( &E ); mbedtls_mpi_init( &N );

    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &M, 16,
        "00000000000000000000000000000000" \
        "00000000000000000000000000000000" \
        "00000000000000000000000000000000" \
        "00000000000000000000000000000287" ) );

    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &N, 16,
         "00000000000000000000000000000000" \
         "00000000000000000000000000000000" \
         "00000000000000000000000000000000" \
         "000000000000000000000000000003E5" ) );

    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &E, 16,
        "00000000000000000000000000000000" \
        "00000000000000000000000000000000" \
        "00000000000000000000000000000000" \
        "000000000000000000000000000002E7" ) );

    mbedtls_mpi_exp_mod( &M, &M, &E, &N, NULL);

    if (memcmp(M.p, Expected512, DATA_LENGTH * 4) == 0) {
        if(verbose)
            mbedtls_printf( "Pass ^_^ \n" );
    } else {
        if(verbose){
            mbedtls_printf( "Fail\n" );
            MPI_PRINT(1, "M: ", &M);
        }
        ret ++;
    }

cleanup:

    mbedtls_mpi_free( &M );
    mbedtls_mpi_free( &E );
    mbedtls_mpi_free( &N );

    return ret;
}

int mbedtls_mpi_exp_mod_self_test_1024( int verbose )
{
    mbedtls_mpi M, E, N;
    int ret = 0;

    /*********************** RSA 1024 *****************************/
    if(verbose)
        mbedtls_printf( "  MPI test #7 (exp mod 1024): \n" );

    mbedtls_mpi_init( &M ); mbedtls_mpi_init( &E ); mbedtls_mpi_init( &N );
    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &M, 16,
        "8e0bef749a2d611e01348df67f763d49" \
        "2f4406f542c20249385b88bd6a3af995" \
        "85edfb11165e3058ee52df5c4f7b1bbe" \
        "602ef418635486e6d436a0e40168c993" \
        "9fc6be976bcf71ebc59ffade1824daab" \
        "17107865fb24549bf5bce8ace5027eaa" \
        "305f361c36e403ec359fa0a630220837" \
        "c06f0b56511c06ffCAD234355884d1a0" ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &N, 16,
        "b0607d893d3c5691b7b666d781585acb" \
        "3f3a1d9d26954c0eb2f0e540687e80ba" \
        "19f906f3c41fb462e8a24392271b6693" \
        "45472f87fec830b39edfe8c2114ad23c" \
        "197372b4c5853890d6e891c4be7d7d51" \
        "4e1788b718b2316743abcd855dcb6c9a" \
        "17f974ca4f139c7c945a19db67db31e1" \
        "e505f95325f213f816d8ba65e9bf8549" ) );

    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &E, 16,
        "499d46cf5152d0882d560ec67272a128" \
        "c7507145043a4ae33a63e7406971528b" \
        "8d14f341f08f42ffd4010658ff012353" \
        "35b88715f9b1f0c2a35e98a89d70b03d" \
        "3ebe219bd685ff195c3e1e2e260e17d6" \
        "bf336a12fbc2a44678edc66e2c64bff9" \
        "c654f73ee9a9c38e6137dcf509c1f7f8" \
        "f9c0b12dc84d342fc1cc96fe6a3921ab" ) );

    mbedtls_mpi_exp_mod( &M, &M, &E, &N, NULL);

    if (memcmp(M.p, Expected1024, DATA_LENGTH_1024 * 4) == 0) {
        if(verbose)
            mbedtls_printf( "Pass ^_^ \n" );
    } else {
        if(verbose){
            mbedtls_printf( "Fail\n" );
            MPI_PRINT(1, "M: ", &M);
        }
        ret ++;
    }

cleanup:

    mbedtls_mpi_free( &M );
    mbedtls_mpi_free( &E );
    mbedtls_mpi_free( &N );

    return ret;
}
#endif //CONFIG_HW_SECURITY_ENGINE
#endif /* MBEDTLS_SELF_TEST_OUT && MBEDTLS_BIGNUM_C */

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_RSA_C)

/*
 * Example RSA-1024 keypair, for test purposes
 */
#define KEY_LEN 128

#define RSA_N   "9292758453063D803DD603D5E777D788" \
                "8ED1D5BF35786190FA2F23EBC0848AEA" \
                "DDA92CA6C3D80B32C4D109BE0F36D6AE" \
                "7130B9CED7ACDF54CFC7555AC14EEBAB" \
                "93A89813FBF3C4F8066D2D800F7C38A8" \
                "1AE31942917403FF4946B0A83D3D3E05" \
                "EE57C6F5F5606FB5D4BC6CD34EE0801A" \
                "5E94BB77B07507233A0BC7BAC8F90F79"

#define RSA_E   "10001"

#define RSA_D   "24BF6185468786FDD303083D25E64EFC" \
                "66CA472BC44D253102F8B4A9D3BFA750" \
                "91386C0077937FE33FA3252D28855837" \
                "AE1B484A8A9A45F7EE8C0C634F99E8CD" \
                "DF79C5CE07EE72C7F123142198164234" \
                "CABB724CF78B8173B9F880FC86322407" \
                "AF1FEDFDDE2BEB674CA15F3E81A1521E" \
                "071513A1E85B5DFA031F21ECAE91A34D"

#define RSA_P   "C36D0EB7FCD285223CFB5AABA5BDA3D8" \
                "2C01CAD19EA484A87EA4377637E75500" \
                "FCB2005C5C7DD6EC4AC023CDA285D796" \
                "C3D9E75E1EFC42488BB4F1D13AC30A57"

#define RSA_Q   "C000DF51A7C77AE8D7C7370C1FF55B69" \
                "E211C2B9E5DB1ED0BF61D0D9899620F4" \
                "910E4168387E3C30AA1E00C339A79508" \
                "8452DD96A9A5EA5D9DCA68DA636032AF"

#define PT_LEN  24
#define RSA_PT  "\xAA\xBB\xCC\x03\x02\x01\x00\xFF\xFF\xFF\xFF\xFF" \
                "\x11\x22\x33\x0A\x0B\x0C\xCC\xDD\xDD\xDD\xDD\xDD"

#if defined(MBEDTLS_PKCS1_V15)
static int myrand( void *rng_state, unsigned char *output, size_t len )
{
#if !defined(__OpenBSD__)
    if( rng_state != NULL )
        rng_state  = NULL;

    sys_random_bytes_get(output, len);
#else
    if( rng_state != NULL )
        rng_state = NULL;

    arc4random_buf( output, len );
#endif /* !OpenBSD */

    return( 0 );
}
#endif /* MBEDTLS_PKCS1_V15 */

/*
 * Checkup routine
 */
int mbedtls_rsa_self_test( int verbose )
{
    int ret = 0;
#if defined(MBEDTLS_PKCS1_V15)
    size_t len;
    mbedtls_rsa_context rsa;
    unsigned char rsa_plaintext[PT_LEN];
    unsigned char rsa_decrypted[PT_LEN];
    unsigned char rsa_ciphertext[384];
#if defined(MBEDTLS_SHA1_C)
    unsigned char sha1sum[20];
#endif
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "rsa_genkey";
    unsigned int array_key_sz[] = {512, 1024};
    unsigned int key_idx = 0;

    mbedtls_mpi K;

    mbedtls_mpi_init( &K );
    mbedtls_rsa_init( &rsa, MBEDTLS_RSA_PKCS_V15, 0 );
    mbedtls_ctr_drbg_init( &ctr_drbg );
    mbedtls_entropy_init( &entropy );
    if( ( ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret );
        goto cleanup;
    }

#if 0
    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &K, 16, RSA_N  ) );
    MBEDTLS_MPI_CHK( mbedtls_rsa_import( &rsa, &K, NULL, NULL, NULL, NULL ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &K, 16, RSA_P  ) );
    MBEDTLS_MPI_CHK( mbedtls_rsa_import( &rsa, NULL, &K, NULL, NULL, NULL ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &K, 16, RSA_Q  ) );
    MBEDTLS_MPI_CHK( mbedtls_rsa_import( &rsa, NULL, NULL, &K, NULL, NULL ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &K, 16, RSA_D  ) );
    MBEDTLS_MPI_CHK( mbedtls_rsa_import( &rsa, NULL, NULL, NULL, &K, NULL ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &K, 16, RSA_E  ) );
    MBEDTLS_MPI_CHK( mbedtls_rsa_import( &rsa, NULL, NULL, NULL, NULL, &K ) );

    MBEDTLS_MPI_CHK( mbedtls_rsa_complete( &rsa ) );
#else
Next:
    if(key_idx < 2) {
        mbedtls_rsa_free( &rsa );
        mbedtls_rsa_init( &rsa, MBEDTLS_RSA_PKCS_V15, 0 );
    } else {
        goto cleanup;
    }
    if( verbose != 0 )
        mbedtls_printf( " Generating the RSA key [ %d-bit ]...", array_key_sz[key_idx] );

    if( ( ret = mbedtls_rsa_gen_key( &rsa, mbedtls_ctr_drbg_random, &ctr_drbg, array_key_sz[key_idx],
                                     65537 ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_rsa_gen_key returned %d\n\n", ret );
        goto cleanup;
    }

    if( verbose != 0 )
        mbedtls_printf( "ok\n" );
#endif

    if( verbose != 0 )
        mbedtls_printf( "  RSA key validation: " );

    if( mbedtls_rsa_check_pubkey(  &rsa ) != 0 ||
        mbedtls_rsa_check_privkey( &rsa ) != 0 )
    {
        if( verbose != 0 )
            mbedtls_printf( "failed\n" );

        ret = 1;
        goto cleanup;
    }

    if( verbose != 0 )
        mbedtls_printf( "passed\n  PKCS#1 encryption : " );

    memcpy( rsa_plaintext, RSA_PT, PT_LEN );

    if( mbedtls_rsa_pkcs1_encrypt( &rsa, myrand, NULL, MBEDTLS_RSA_PUBLIC,
                                   PT_LEN, rsa_plaintext,
                                   rsa_ciphertext ) != 0 )
    {
        if( verbose != 0 )
            mbedtls_printf( "failed\n" );

        ret = 1;
        goto cleanup;
    }

    if( verbose != 0 )
        mbedtls_printf( "passed\n  PKCS#1 decryption : " );

    if( mbedtls_rsa_pkcs1_decrypt( &rsa, myrand, NULL, MBEDTLS_RSA_PRIVATE,
                                   &len, rsa_ciphertext, rsa_decrypted,
                                   sizeof(rsa_decrypted) ) != 0 )
    {
        if( verbose != 0 )
            mbedtls_printf( "failed\n" );

        ret = 1;
        goto cleanup;
    }

    if( memcmp( rsa_decrypted, rsa_plaintext, len ) != 0 )
    {
        if( verbose != 0 )
            mbedtls_printf( "failed\n" );

        ret = 1;
        goto cleanup;
    }

    if( verbose != 0 )
        mbedtls_printf( "passed\n" );

#if defined(MBEDTLS_SHA1_C)
    if( verbose != 0 )
        mbedtls_printf( "  PKCS#1 data sign  : " );

    if( mbedtls_sha1_ret( rsa_plaintext, PT_LEN, sha1sum ) != 0 )
    {
        if( verbose != 0 )
            mbedtls_printf( "failed\n" );

        return( 1 );
    }

    if( mbedtls_rsa_pkcs1_sign( &rsa, myrand, NULL,
                                MBEDTLS_RSA_PRIVATE, MBEDTLS_MD_SHA1, 0,
                                sha1sum, rsa_ciphertext ) != 0 )
    {
        if( verbose != 0 )
            mbedtls_printf( "failed\n" );

        ret = 1;
        goto cleanup;
    }

    if( verbose != 0 )
        mbedtls_printf( "passed\n  PKCS#1 sig. verify: " );

    if( mbedtls_rsa_pkcs1_verify( &rsa, NULL, NULL,
                                  MBEDTLS_RSA_PUBLIC, MBEDTLS_MD_SHA1, 0,
                                  sha1sum, rsa_ciphertext ) != 0 )
    {
        if( verbose != 0 )
            mbedtls_printf( "failed\n" );

        ret = 1;
        goto cleanup;
    }

    if( verbose != 0 )
        mbedtls_printf( "passed\n" );
#endif /* MBEDTLS_SHA1_C */

    if( verbose != 0 )
        mbedtls_printf( "\n" );

#if 1
    key_idx++;
    goto Next;
#endif

cleanup:
    mbedtls_mpi_free( &K );
    mbedtls_rsa_free( &rsa );
    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );
#else /* MBEDTLS_PKCS1_V15 */
    ((void) verbose);
#endif /* MBEDTLS_PKCS1_V15 */
    return( ret );
}

#endif /* MBEDTLS_SELF_TEST_OUT && MBEDTLS_RSA_C */

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_XTEA_C)

/*
 * XTEA tests vectors (non-official)
 */

static const unsigned char xtea_test_key[6][16] =
{
   { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
     0x0c, 0x0d, 0x0e, 0x0f },
   { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
     0x0c, 0x0d, 0x0e, 0x0f },
   { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
     0x0c, 0x0d, 0x0e, 0x0f },
   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00 },
   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00 },
   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00 }
};

static const unsigned char xtea_test_pt[6][8] =
{
    { 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48 },
    { 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41 },
    { 0x5a, 0x5b, 0x6e, 0x27, 0x89, 0x48, 0xd7, 0x7f },
    { 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48 },
    { 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41 },
    { 0x70, 0xe1, 0x22, 0x5d, 0x6e, 0x4e, 0x76, 0x55 }
};

static const unsigned char xtea_test_ct[6][8] =
{
    { 0x49, 0x7d, 0xf3, 0xd0, 0x72, 0x61, 0x2c, 0xb5 },
    { 0xe7, 0x8f, 0x2d, 0x13, 0x74, 0x43, 0x41, 0xd8 },
    { 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41 },
    { 0xa0, 0x39, 0x05, 0x89, 0xf8, 0xb8, 0xef, 0xa5 },
    { 0xed, 0x23, 0x37, 0x5a, 0x82, 0x1a, 0x8c, 0x2d },
    { 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41 }
};

/*
 * Checkup routine
 */
int mbedtls_xtea_self_test( int verbose )
{
    int i, ret = 0;
    unsigned char buf[8];
    mbedtls_xtea_context ctx;

    mbedtls_xtea_init( &ctx );
    for( i = 0; i < 6; i++ )
    {
        if( verbose != 0 )
            mbedtls_printf( "  XTEA test #%d: ", i + 1 );

        memcpy( buf, xtea_test_pt[i], 8 );

        mbedtls_xtea_setup( &ctx, xtea_test_key[i] );
        mbedtls_xtea_crypt_ecb( &ctx, MBEDTLS_XTEA_ENCRYPT, buf, buf );

        if( memcmp( buf, xtea_test_ct[i], 8 ) != 0 )
        {
            if( verbose != 0 )
                mbedtls_printf( "failed\n" );

            ret = 1;
            goto exit;
        }

        if( verbose != 0 )
            mbedtls_printf( "passed\n" );
    }

    if( verbose != 0 )
        mbedtls_printf( "\n" );

exit:
    mbedtls_xtea_free( &ctx );

    return( ret );
}

#endif /* MBEDTLS_SELF_TEST_OUT && MBEDTLS_XTEA_C */

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_CAMELLIA_C)

/*
 * Camellia test vectors from:
 *
 * http://info.isl.ntt.co.jp/crypt/eng/camellia/technology.html:
 *   http://info.isl.ntt.co.jp/crypt/eng/camellia/dl/cryptrec/intermediate.txt
 *   http://info.isl.ntt.co.jp/crypt/eng/camellia/dl/cryptrec/t_camellia.txt
 *                      (For each bitlength: Key 0, Nr 39)
 */
#define CAMELLIA_TESTS_ECB  2

static const unsigned char camellia_test_ecb_key[3][CAMELLIA_TESTS_ECB][32] =
{
    {
        { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
          0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10 },
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
    },
    {
        { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
          0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
          0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 },
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
    },
    {
        { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
          0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
          0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
          0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff },
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
    },
};

static const unsigned char camellia_test_ecb_plain[CAMELLIA_TESTS_ECB][16] =
{
    { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
      0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10 },
    { 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

static const unsigned char camellia_test_ecb_cipher[3][CAMELLIA_TESTS_ECB][16] =
{
    {
        { 0x67, 0x67, 0x31, 0x38, 0x54, 0x96, 0x69, 0x73,
          0x08, 0x57, 0x06, 0x56, 0x48, 0xea, 0xbe, 0x43 },
        { 0x38, 0x3C, 0x6C, 0x2A, 0xAB, 0xEF, 0x7F, 0xDE,
          0x25, 0xCD, 0x47, 0x0B, 0xF7, 0x74, 0xA3, 0x31 }
    },
    {
        { 0xb4, 0x99, 0x34, 0x01, 0xb3, 0xe9, 0x96, 0xf8,
          0x4e, 0xe5, 0xce, 0xe7, 0xd7, 0x9b, 0x09, 0xb9 },
        { 0xD1, 0x76, 0x3F, 0xC0, 0x19, 0xD7, 0x7C, 0xC9,
          0x30, 0xBF, 0xF2, 0xA5, 0x6F, 0x7C, 0x93, 0x64 }
    },
    {
        { 0x9a, 0xcc, 0x23, 0x7d, 0xff, 0x16, 0xd7, 0x6c,
          0x20, 0xef, 0x7c, 0x91, 0x9e, 0x3a, 0x75, 0x09 },
        { 0x05, 0x03, 0xFB, 0x10, 0xAB, 0x24, 0x1E, 0x7C,
          0xF4, 0x5D, 0x8C, 0xDE, 0xEE, 0x47, 0x43, 0x35 }
    }
};

#if defined(MBEDTLS_CIPHER_MODE_CBC)
#define CAMELLIA_TESTS_CBC  3

static const unsigned char camellia_test_cbc_key[3][32] =
{
        { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
          0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C }
    ,
        { 0x8E, 0x73, 0xB0, 0xF7, 0xDA, 0x0E, 0x64, 0x52,
          0xC8, 0x10, 0xF3, 0x2B, 0x80, 0x90, 0x79, 0xE5,
          0x62, 0xF8, 0xEA, 0xD2, 0x52, 0x2C, 0x6B, 0x7B }
    ,
        { 0x60, 0x3D, 0xEB, 0x10, 0x15, 0xCA, 0x71, 0xBE,
          0x2B, 0x73, 0xAE, 0xF0, 0x85, 0x7D, 0x77, 0x81,
          0x1F, 0x35, 0x2C, 0x07, 0x3B, 0x61, 0x08, 0xD7,
          0x2D, 0x98, 0x10, 0xA3, 0x09, 0x14, 0xDF, 0xF4 }
};

static const unsigned char camellia_test_cbc_iv[16] =

    { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F }
;

static const unsigned char camellia_test_cbc_plain[CAMELLIA_TESTS_CBC][16] =
{
    { 0x6B, 0xC1, 0xBE, 0xE2, 0x2E, 0x40, 0x9F, 0x96,
      0xE9, 0x3D, 0x7E, 0x11, 0x73, 0x93, 0x17, 0x2A },
    { 0xAE, 0x2D, 0x8A, 0x57, 0x1E, 0x03, 0xAC, 0x9C,
      0x9E, 0xB7, 0x6F, 0xAC, 0x45, 0xAF, 0x8E, 0x51 },
    { 0x30, 0xC8, 0x1C, 0x46, 0xA3, 0x5C, 0xE4, 0x11,
      0xE5, 0xFB, 0xC1, 0x19, 0x1A, 0x0A, 0x52, 0xEF }

};

static const unsigned char camellia_test_cbc_cipher[3][CAMELLIA_TESTS_CBC][16] =
{
    {
        { 0x16, 0x07, 0xCF, 0x49, 0x4B, 0x36, 0xBB, 0xF0,
          0x0D, 0xAE, 0xB0, 0xB5, 0x03, 0xC8, 0x31, 0xAB },
        { 0xA2, 0xF2, 0xCF, 0x67, 0x16, 0x29, 0xEF, 0x78,
          0x40, 0xC5, 0xA5, 0xDF, 0xB5, 0x07, 0x48, 0x87 },
        { 0x0F, 0x06, 0x16, 0x50, 0x08, 0xCF, 0x8B, 0x8B,
          0x5A, 0x63, 0x58, 0x63, 0x62, 0x54, 0x3E, 0x54 }
    },
    {
        { 0x2A, 0x48, 0x30, 0xAB, 0x5A, 0xC4, 0xA1, 0xA2,
          0x40, 0x59, 0x55, 0xFD, 0x21, 0x95, 0xCF, 0x93 },
        { 0x5D, 0x5A, 0x86, 0x9B, 0xD1, 0x4C, 0xE5, 0x42,
          0x64, 0xF8, 0x92, 0xA6, 0xDD, 0x2E, 0xC3, 0xD5 },
        { 0x37, 0xD3, 0x59, 0xC3, 0x34, 0x98, 0x36, 0xD8,
          0x84, 0xE3, 0x10, 0xAD, 0xDF, 0x68, 0xC4, 0x49 }
    },
    {
        { 0xE6, 0xCF, 0xA3, 0x5F, 0xC0, 0x2B, 0x13, 0x4A,
          0x4D, 0x2C, 0x0B, 0x67, 0x37, 0xAC, 0x3E, 0xDA },
        { 0x36, 0xCB, 0xEB, 0x73, 0xBD, 0x50, 0x4B, 0x40,
          0x70, 0xB1, 0xB7, 0xDE, 0x2B, 0x21, 0xEB, 0x50 },
        { 0xE3, 0x1A, 0x60, 0x55, 0x29, 0x7D, 0x96, 0xCA,
          0x33, 0x30, 0xCD, 0xF1, 0xB1, 0x86, 0x0A, 0x83 }
    }
};
#endif /* MBEDTLS_CIPHER_MODE_CBC */

#if defined(MBEDTLS_CIPHER_MODE_CTR)
/*
 * Camellia-CTR test vectors from:
 *
 * http://www.faqs.org/rfcs/rfc5528.html
 */

static const unsigned char camellia_test_ctr_key[3][16] =
{
    { 0xAE, 0x68, 0x52, 0xF8, 0x12, 0x10, 0x67, 0xCC,
      0x4B, 0xF7, 0xA5, 0x76, 0x55, 0x77, 0xF3, 0x9E },
    { 0x7E, 0x24, 0x06, 0x78, 0x17, 0xFA, 0xE0, 0xD7,
      0x43, 0xD6, 0xCE, 0x1F, 0x32, 0x53, 0x91, 0x63 },
    { 0x76, 0x91, 0xBE, 0x03, 0x5E, 0x50, 0x20, 0xA8,
      0xAC, 0x6E, 0x61, 0x85, 0x29, 0xF9, 0xA0, 0xDC }
};

static const unsigned char camellia_test_ctr_nonce_counter[3][16] =
{
    { 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 },
    { 0x00, 0x6C, 0xB6, 0xDB, 0xC0, 0x54, 0x3B, 0x59,
      0xDA, 0x48, 0xD9, 0x0B, 0x00, 0x00, 0x00, 0x01 },
    { 0x00, 0xE0, 0x01, 0x7B, 0x27, 0x77, 0x7F, 0x3F,
      0x4A, 0x17, 0x86, 0xF0, 0x00, 0x00, 0x00, 0x01 }
};

static const unsigned char camellia_test_ctr_pt[3][48] =
{
    { 0x53, 0x69, 0x6E, 0x67, 0x6C, 0x65, 0x20, 0x62,
      0x6C, 0x6F, 0x63, 0x6B, 0x20, 0x6D, 0x73, 0x67 },

    { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
      0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F },

    { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
      0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
      0x20, 0x21, 0x22, 0x23 }
};

static const unsigned char camellia_test_ctr_ct[3][48] =
{
    { 0xD0, 0x9D, 0xC2, 0x9A, 0x82, 0x14, 0x61, 0x9A,
      0x20, 0x87, 0x7C, 0x76, 0xDB, 0x1F, 0x0B, 0x3F },
    { 0xDB, 0xF3, 0xC7, 0x8D, 0xC0, 0x83, 0x96, 0xD4,
      0xDA, 0x7C, 0x90, 0x77, 0x65, 0xBB, 0xCB, 0x44,
      0x2B, 0x8E, 0x8E, 0x0F, 0x31, 0xF0, 0xDC, 0xA7,
      0x2C, 0x74, 0x17, 0xE3, 0x53, 0x60, 0xE0, 0x48 },
    { 0xB1, 0x9D, 0x1F, 0xCD, 0xCB, 0x75, 0xEB, 0x88,
      0x2F, 0x84, 0x9C, 0xE2, 0x4D, 0x85, 0xCF, 0x73,
      0x9C, 0xE6, 0x4B, 0x2B, 0x5C, 0x9D, 0x73, 0xF1,
      0x4F, 0x2D, 0x5D, 0x9D, 0xCE, 0x98, 0x89, 0xCD,
      0xDF, 0x50, 0x86, 0x96 }
};

static const int camellia_test_ctr_len[3] =
    { 16, 32, 36 };
#endif /* MBEDTLS_CIPHER_MODE_CTR */

/*
 * Checkup routine
 */
int mbedtls_camellia_self_test( int verbose )
{
    int i, j, u, v;
    unsigned char key6[32];
    unsigned char buf[64];
    unsigned char src[16];
    unsigned char dst[16];
#if defined(MBEDTLS_CIPHER_MODE_CBC)
    unsigned char iv5[16];
#endif
#if defined(MBEDTLS_CIPHER_MODE_CTR)
    size_t offset, len;
    unsigned char nonce_counter[16];
    unsigned char stream_block[16];
#endif

    mbedtls_camellia_context ctx;

    memset( key6, 0, 32 );

    for( j = 0; j < 6; j++ ) {
        u = j >> 1;
    v = j & 1;

    if( verbose != 0 )
        mbedtls_printf( "  CAMELLIA-ECB-%3d (%s): ", 128 + u * 64,
                         (v == MBEDTLS_CAMELLIA_DECRYPT) ? "dec" : "enc");

    for( i = 0; i < CAMELLIA_TESTS_ECB; i++ ) {
        memcpy( key6, camellia_test_ecb_key[u][i], 16 + 8 * u );

        if( v == MBEDTLS_CAMELLIA_DECRYPT ) {
            mbedtls_camellia_setkey_dec( &ctx, key6, 128 + u * 64 );
            memcpy( src, camellia_test_ecb_cipher[u][i], 16 );
            memcpy( dst, camellia_test_ecb_plain[i], 16 );
        } else { /* MBEDTLS_CAMELLIA_ENCRYPT */
            mbedtls_camellia_setkey_enc( &ctx, key6, 128 + u * 64 );
            memcpy( src, camellia_test_ecb_plain[i], 16 );
            memcpy( dst, camellia_test_ecb_cipher[u][i], 16 );
        }

        mbedtls_camellia_crypt_ecb( &ctx, v, src, buf );

        if( memcmp( buf, dst, 16 ) != 0 )
        {
            if( verbose != 0 )
                mbedtls_printf( "failed\n" );

            return( 1 );
        }
    }

    if( verbose != 0 )
        mbedtls_printf( "passed\n" );
    }

    if( verbose != 0 )
        mbedtls_printf( "\n" );

#if defined(MBEDTLS_CIPHER_MODE_CBC)
    /*
     * CBC mode
     */
    for( j = 0; j < 6; j++ )
    {
        u = j >> 1;
        v = j  & 1;

        if( verbose != 0 )
            mbedtls_printf( "  CAMELLIA-CBC-%3d (%s): ", 128 + u * 64,
                             ( v == MBEDTLS_CAMELLIA_DECRYPT ) ? "dec" : "enc" );

        memcpy( src, camellia_test_cbc_iv, 16 );
        memcpy( dst, camellia_test_cbc_iv, 16 );
        memcpy( key6, camellia_test_cbc_key[u], 16 + 8 * u );

        if( v == MBEDTLS_CAMELLIA_DECRYPT ) {
            mbedtls_camellia_setkey_dec( &ctx, key6, 128 + u * 64 );
        } else {
            mbedtls_camellia_setkey_enc( &ctx, key6, 128 + u * 64 );
        }

        for( i = 0; i < CAMELLIA_TESTS_CBC; i++ ) {

            if( v == MBEDTLS_CAMELLIA_DECRYPT ) {
                memcpy( iv5 , src, 16 );
                memcpy( src, camellia_test_cbc_cipher[u][i], 16 );
                memcpy( dst, camellia_test_cbc_plain[i], 16 );
            } else { /* MBEDTLS_CAMELLIA_ENCRYPT */
                memcpy( iv5 , dst, 16 );
                memcpy( src, camellia_test_cbc_plain[i], 16 );
                memcpy( dst, camellia_test_cbc_cipher[u][i], 16 );
            }

            mbedtls_camellia_crypt_cbc( &ctx, v, 16, iv5, src, buf );

            if( memcmp( buf, dst, 16 ) != 0 )
            {
                if( verbose != 0 )
                    mbedtls_printf( "failed\n" );

                return( 1 );
            }
        }

        if( verbose != 0 )
            mbedtls_printf( "passed\n" );
    }
#endif /* MBEDTLS_CIPHER_MODE_CBC */

    if( verbose != 0 )
        mbedtls_printf( "\n" );

#if defined(MBEDTLS_CIPHER_MODE_CTR)
    /*
     * CTR mode
     */
    for( i = 0; i < 6; i++ )
    {
        u = i >> 1;
        v = i  & 1;

        if( verbose != 0 )
            mbedtls_printf( "  CAMELLIA-CTR-128 (%s): ",
                             ( v == MBEDTLS_CAMELLIA_DECRYPT ) ? "dec" : "enc" );

        memcpy( nonce_counter, camellia_test_ctr_nonce_counter[u], 16 );
        memcpy( key6, camellia_test_ctr_key[u], 16 );

        offset = 0;
        mbedtls_camellia_setkey_enc( &ctx, key6, 128 );

        if( v == MBEDTLS_CAMELLIA_DECRYPT )
        {
            len = camellia_test_ctr_len[u];
            memcpy( buf, camellia_test_ctr_ct[u], len );

            mbedtls_camellia_crypt_ctr( &ctx, len, &offset, nonce_counter, stream_block,
                                buf, buf );

            if( memcmp( buf, camellia_test_ctr_pt[u], len ) != 0 )
            {
                if( verbose != 0 )
                    mbedtls_printf( "failed\n" );

                return( 1 );
            }
        }
        else
        {
            len = camellia_test_ctr_len[u];
            memcpy( buf, camellia_test_ctr_pt[u], len );

            mbedtls_camellia_crypt_ctr( &ctx, len, &offset, nonce_counter, stream_block,
                                buf, buf );

            if( memcmp( buf, camellia_test_ctr_ct[u], len ) != 0 )
            {
                if( verbose != 0 )
                    mbedtls_printf( "failed\n" );

                return( 1 );
            }
        }

        if( verbose != 0 )
            mbedtls_printf( "passed\n" );
    }

    if( verbose != 0 )
        mbedtls_printf( "\n" );
#endif /* MBEDTLS_CIPHER_MODE_CTR */

    return( 0 );
}

#endif /* MBEDTLS_SELF_TEST_OUT && MBEDTLS_CAMELLIA_C */

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_CTR_DRBG_C)

static const unsigned char entropy_source_pr[96] =
    { 0xc1, 0x80, 0x81, 0xa6, 0x5d, 0x44, 0x02, 0x16,
      0x19, 0xb3, 0xf1, 0x80, 0xb1, 0xc9, 0x20, 0x02,
      0x6a, 0x54, 0x6f, 0x0c, 0x70, 0x81, 0x49, 0x8b,
      0x6e, 0xa6, 0x62, 0x52, 0x6d, 0x51, 0xb1, 0xcb,
      0x58, 0x3b, 0xfa, 0xd5, 0x37, 0x5f, 0xfb, 0xc9,
      0xff, 0x46, 0xd2, 0x19, 0xc7, 0x22, 0x3e, 0x95,
      0x45, 0x9d, 0x82, 0xe1, 0xe7, 0x22, 0x9f, 0x63,
      0x31, 0x69, 0xd2, 0x6b, 0x57, 0x47, 0x4f, 0xa3,
      0x37, 0xc9, 0x98, 0x1c, 0x0b, 0xfb, 0x91, 0x31,
      0x4d, 0x55, 0xb9, 0xe9, 0x1c, 0x5a, 0x5e, 0xe4,
      0x93, 0x92, 0xcf, 0xc5, 0x23, 0x12, 0xd5, 0x56,
      0x2c, 0x4a, 0x6e, 0xff, 0xdc, 0x10, 0xd0, 0x68 };

static const unsigned char entropy_source_nopr[64] =
    { 0x5a, 0x19, 0x4d, 0x5e, 0x2b, 0x31, 0x58, 0x14,
      0x54, 0xde, 0xf6, 0x75, 0xfb, 0x79, 0x58, 0xfe,
      0xc7, 0xdb, 0x87, 0x3e, 0x56, 0x89, 0xfc, 0x9d,
      0x03, 0x21, 0x7c, 0x68, 0xd8, 0x03, 0x38, 0x20,
      0xf9, 0xe6, 0x5e, 0x04, 0xd8, 0x56, 0xf3, 0xa9,
      0xc4, 0x4a, 0x4c, 0xbd, 0xc1, 0xd0, 0x08, 0x46,
      0xf5, 0x98, 0x3d, 0x77, 0x1c, 0x1b, 0x13, 0x7e,
      0x4e, 0x0f, 0x9d, 0x8e, 0xf4, 0x09, 0xf9, 0x2e };

static const unsigned char nonce_pers_pr[16] =
    { 0xd2, 0x54, 0xfc, 0xff, 0x02, 0x1e, 0x69, 0xd2,
      0x29, 0xc9, 0xcf, 0xad, 0x85, 0xfa, 0x48, 0x6c };

static const unsigned char nonce_pers_nopr[16] =
    { 0x1b, 0x54, 0xb8, 0xff, 0x06, 0x42, 0xbf, 0xf5,
      0x21, 0xf1, 0x5c, 0x1c, 0x0b, 0x66, 0x5f, 0x3f };

static const unsigned char result_pr[16] =
    { 0x34, 0x01, 0x16, 0x56, 0xb4, 0x29, 0x00, 0x8f,
      0x35, 0x63, 0xec, 0xb5, 0xf2, 0x59, 0x07, 0x23 };

static const unsigned char result_nopr_1[16] =
    { 0xa0, 0x54, 0x30, 0x3d, 0x8a, 0x7e, 0xa9, 0x88,
      0x9d, 0x90, 0x3e, 0x07, 0x7c, 0x6f, 0x21, 0x8f };

static size_t test_offset;
static int ctr_drbg_self_test_entropy( void *data, unsigned char *buf,
                                       size_t len )
{
    const unsigned char *p = data;
    memcpy( buf, p + test_offset, len );
    test_offset += len;
    return( 0 );
}

#define CHK( c )    if( (c) != 0 )                          \
                    {                                       \
                        if( verbose != 0 )                  \
                            mbedtls_printf( "failed\n" );  \
                        return( 1 );                        \
                    }

/*
 * Checkup routine
 */
int mbedtls_ctr_drbg_self_test( int verbose )
{
    mbedtls_ctr_drbg_context ctx;
    unsigned char buf[16];

    mbedtls_ctr_drbg_init( &ctx );

    /*
     * Based on a NIST CTR_DRBG test vector (PR = True)
     */
    if( verbose != 0 )
        mbedtls_printf( "  CTR_DRBG (PR = TRUE) : " );

    test_offset = 0;
    CHK( mbedtls_ctr_drbg_seed_entropy_len( &ctx, ctr_drbg_self_test_entropy,
                         (void *) entropy_source_pr, nonce_pers_pr, 16, 32 ) );
    mbedtls_ctr_drbg_set_prediction_resistance( &ctx, MBEDTLS_CTR_DRBG_PR_ON );
    CHK( mbedtls_ctr_drbg_random( &ctx, buf, MBEDTLS_CTR_DRBG_BLOCKSIZE ) );
    CHK( mbedtls_ctr_drbg_random( &ctx, buf, MBEDTLS_CTR_DRBG_BLOCKSIZE ) );
    CHK( memcmp( buf, result_pr, MBEDTLS_CTR_DRBG_BLOCKSIZE ) );

    mbedtls_ctr_drbg_free( &ctx );

    if( verbose != 0 )
        mbedtls_printf( "passed\n" );

    /*
     * Based on a NIST CTR_DRBG test vector (PR = FALSE)
     */
    if( verbose != 0 )
        mbedtls_printf( "  CTR_DRBG (PR = FALSE): " );

    mbedtls_ctr_drbg_init( &ctx );

    test_offset = 0;
    CHK( mbedtls_ctr_drbg_seed_entropy_len( &ctx, ctr_drbg_self_test_entropy,
                     (void *) entropy_source_nopr, nonce_pers_nopr, 16, 32 ) );
    CHK( mbedtls_ctr_drbg_random( &ctx, buf, 16 ) );
    CHK( mbedtls_ctr_drbg_reseed( &ctx, NULL, 0 ) );
    CHK( mbedtls_ctr_drbg_random( &ctx, buf, 16 ) );
    CHK( memcmp( buf, result_nopr_1, 16 ) );

    mbedtls_ctr_drbg_free( &ctx );

    if( verbose != 0 )
        mbedtls_printf( "passed\n" );

    if( verbose != 0 )
            mbedtls_printf( "\n" );

    return( 0 );
}
#endif /* MBEDTLS_SELF_TEST_OUT && MBEDTLS_CTR_DRBG_C */

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_ECP_C)

#define ECP_NB_CURVES   12
static unsigned long add_count, dbl_count, mul_count;
//extern uint32_t ecp_supported_curves;

/*
 * Checkup routine
 */
int mbedtls_ecp_self_test( int verbose )
{
    int ret;
    size_t i;
    mbedtls_ecp_group grp;
    mbedtls_ecp_point R, P;
    mbedtls_mpi m;
    unsigned int nb_curves = 0, group_id;
    unsigned long add_c_prev, dbl_c_prev, mul_c_prev;
    /* exponents especially adapted for secp192r1 */
    const char *exponents[] =
    {
        "000000000000000000000000000000000000000000000001", /* one */
        //"FFFFFFFFFFFFFFFFFFFFFFFF99DEF836146BC9B1B4D22830", /* N - 1 */
        "FFFFFFFFFFFFFFFFFFFFFFFE26F2FC170F69466A74DEFD8C", /* N - 1 */ /* secp192k1 */
        "5EA6F389A38B8BC81E767753B15AA5569E1782E30ABE7D25", /* random */
        "400000000000000000000000000000000000000000000000", /* one and zeros */
        "7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", /* all ones */
        "555555555555555555555555555555555555555555555555", /* 101010... */
    };

    mbedtls_ecp_group_init( &grp );
    mbedtls_ecp_point_init( &R );
    mbedtls_ecp_point_init( &P );
    mbedtls_mpi_init( &m );

NextCurve:
    if ( nb_curves >= (ECP_NB_CURVES - 1) ) {
        goto cleanup;
    } else {
        mbedtls_ecp_group_free( &grp );
        mbedtls_ecp_point_free( &R );
        mbedtls_ecp_point_free( &P );
        mbedtls_mpi_free( &m );
        mbedtls_ecp_group_init( &grp );
        mbedtls_ecp_point_init( &R );
        mbedtls_ecp_point_init( &P );
        mbedtls_mpi_init( &m );
    }
    const mbedtls_ecp_curve_info *ecp_supported_curves_temp = (const mbedtls_ecp_curve_info *)ecp_supported_curves;
    if( verbose != 0 )
        mbedtls_printf( "  ====== ECC Group[%d] %s ====== \r\n", nb_curves, ecp_supported_curves_temp[nb_curves].name);

    group_id = ecp_supported_curves_temp[nb_curves].grp_id;
    MBEDTLS_MPI_CHK( mbedtls_ecp_group_load( &grp, group_id ) );

#if 0
    /* Use secp192r1 if available, or any available curve */
#if defined(MBEDTLS_ECP_DP_SECP192R1_ENABLED)
    MBEDTLS_MPI_CHK( mbedtls_ecp_group_load( &grp, MBEDTLS_ECP_DP_SECP192R1 ) );
#else
    MBEDTLS_MPI_CHK( mbedtls_ecp_group_load( &grp, mbedtls_ecp_curve_list()->grp_id ) );
#endif
#endif
    if( verbose != 0 )
        mbedtls_printf( "  ECP test #1 (constant op_count, base point G): " );

    /* Do a dummy multiplication first to trigger precomputation */
    MBEDTLS_MPI_CHK( mbedtls_mpi_lset( &m, 2 ) );
    MBEDTLS_MPI_CHK( mbedtls_ecp_mul( &grp, &P, &m, &grp.G, NULL, NULL ) );

    add_count = 0;
    dbl_count = 0;
    mul_count = 0;
    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &m, 16, exponents[0] ) );
    MBEDTLS_MPI_CHK( mbedtls_ecp_mul( &grp, &R, &m, &grp.G, NULL, NULL ) );

    for( i = 1; i < sizeof( exponents ) / sizeof( exponents[0] ); i++ )
    {
        add_c_prev = add_count;
        dbl_c_prev = dbl_count;
        mul_c_prev = mul_count;
        add_count = 0;
        dbl_count = 0;
        mul_count = 0;

        MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &m, 16, exponents[i] ) );
        MBEDTLS_MPI_CHK( mbedtls_ecp_mul( &grp, &R, &m, &grp.G, NULL, NULL ) );

        if( add_count != add_c_prev ||
            dbl_count != dbl_c_prev ||
            mul_count != mul_c_prev )
        {
            if( verbose != 0 )
                mbedtls_printf( "failed (%u)\n", (unsigned int) i );

            ret = 1;
            goto cleanup;
        }
    }

    if( verbose != 0 )
        mbedtls_printf( "passed\n" );

    if( verbose != 0 )
        mbedtls_printf( "  ECP test #2 (constant op_count, other point): " );
    /* We computed P = 2G last time, use it */

    add_count = 0;
    dbl_count = 0;
    mul_count = 0;
    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &m, 16, exponents[0] ) );
    MBEDTLS_MPI_CHK( mbedtls_ecp_mul( &grp, &R, &m, &P, NULL, NULL ) );

    for( i = 1; i < sizeof( exponents ) / sizeof( exponents[0] ); i++ )
    {
        add_c_prev = add_count;
        dbl_c_prev = dbl_count;
        mul_c_prev = mul_count;
        add_count = 0;
        dbl_count = 0;
        mul_count = 0;

        MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &m, 16, exponents[i] ) );
        MBEDTLS_MPI_CHK( mbedtls_ecp_mul( &grp, &R, &m, &P, NULL, NULL ) );

        if( add_count != add_c_prev ||
            dbl_count != dbl_c_prev ||
            mul_count != mul_c_prev )
        {
            if( verbose != 0 )
                mbedtls_printf( "failed (%u)\n", (unsigned int) i );

            ret = 1;
            goto cleanup;
        }
    }

    if( verbose != 0 )
        mbedtls_printf( "passed\n" );

    nb_curves++;
    goto NextCurve;

cleanup:

    if( ret < 0 && verbose != 0 )
        mbedtls_printf( "Unexpected error, return code = %08X\n", ret );

    mbedtls_ecp_group_free( &grp );
    mbedtls_ecp_point_free( &R );
    mbedtls_ecp_point_free( &P );
    mbedtls_mpi_free( &m );

    if( verbose != 0 )
        mbedtls_printf( "\n" );

    return( ret );
}

#endif /* MBEDTLS_SELF_TEST_OUT && MBEDTLS_ECP_C */

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_DHM_C)

static const char mbedtls_test_dhm_params[] =
"-----BEGIN DH PARAMETERS-----\r\n"
"MIGHAoGBAJ419DBEOgmQTzo5qXl5fQcN9TN455wkOL7052HzxxRVMyhYmwQcgJvh\r\n"
"1sa18fyfR9OiVEMYglOpkqVoGLN7qd5aQNNi5W7/C+VBdHTBJcGZJyyP5B3qcz32\r\n"
"9mLJKudlVudV0Qxk5qUJaPZ/xupz0NyoVpviuiBOI1gNi8ovSXWzAgEC\r\n"
"-----END DH PARAMETERS-----\r\n";

static const size_t mbedtls_test_dhm_params_len = sizeof( mbedtls_test_dhm_params );

/*
 * Checkup routine
 */
int mbedtls_dhm_self_test( int verbose )
{
    int ret;
    mbedtls_dhm_context dhm;

    mbedtls_dhm_init( &dhm );

    if( verbose != 0 )
        mbedtls_printf( "  DHM parameter load: " );

    if( ( ret = mbedtls_dhm_parse_dhm( &dhm,
                    (const unsigned char *) mbedtls_test_dhm_params,
                    mbedtls_test_dhm_params_len ) ) != 0 )
    {
        if( verbose != 0 )
            mbedtls_printf( "failed\n" );

        ret = 1;
        goto exit;
    }

    if( verbose != 0 )
        mbedtls_printf( "passed\n\n" );

exit:
    mbedtls_dhm_free( &dhm );

    return( ret );
}

#endif /* MBEDTLS_SELF_TEST_OUT && MBEDTLS_DHM_C */

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_ENTROPY_C)

#if !defined(MBEDTLS_TEST_NULL_ENTROPY)
/*
 * Dummy source function
 */
static int entropy_dummy_source( void *data, unsigned char *output,
                                 size_t len, size_t *olen )
{
    ((void) data);

    memset( output, 0x2a, len );
    *olen = len;

    return( 0 );
}
#endif /* !MBEDTLS_TEST_NULL_ENTROPY */

#if defined(MBEDTLS_ENTROPY_HARDWARE_ALT)

static int mbedtls_entropy_source_self_test_gather( unsigned char *buf, size_t buf_len )
{
    int ret = 0;
    size_t entropy_len = 0;
    size_t olen = 0;
    size_t attempts = buf_len;

    while( attempts > 0 && entropy_len < buf_len )
    {
        if( ( ret = mbedtls_hardware_poll( NULL, buf + entropy_len,
            buf_len - entropy_len, &olen ) ) != 0 )
            return( ret );

        entropy_len += olen;
        attempts--;
    }

    if( entropy_len < buf_len )
    {
        ret = 1;
    }

    return( ret );
}


static int mbedtls_entropy_source_self_test_check_bits( const unsigned char *buf,
                                                        size_t buf_len )
{
    unsigned char set= 0xFF;
    unsigned char unset = 0x00;
    size_t i;

    for( i = 0; i < buf_len; i++ )
    {
        set &= buf[i];
        unset |= buf[i];
    }

    return( set == 0xFF || unset == 0x00 );
}

/*
 * A test to ensure hat the entropy sources are functioning correctly
 * and there is no obvious failure. The test performs the following checks:
 *  - The entropy source is not providing only 0s (all bits unset) or 1s (all
 *    bits set).
 *  - The entropy source is not providing values in a pattern. Because the
 *    hardware could be providing data in an arbitrary length, this check polls
 *    the hardware entropy source twice and compares the result to ensure they
 *    are not equal.
 *  - The error code returned by the entropy source is not an error.
 */
int mbedtls_entropy_source_self_test( int verbose )
{
    int ret = 0;
    unsigned char buf0[2 * sizeof( unsigned long long int )];
    unsigned char buf1[2 * sizeof( unsigned long long int )];

    if( verbose != 0 )
        mbedtls_printf( "  ENTROPY_BIAS test: " );

    memset( buf0, 0x00, sizeof( buf0 ) );
    memset( buf1, 0x00, sizeof( buf1 ) );

    if( ( ret = mbedtls_entropy_source_self_test_gather( buf0, sizeof( buf0 ) ) ) != 0 )
        goto cleanup;
    if( ( ret = mbedtls_entropy_source_self_test_gather( buf1, sizeof( buf1 ) ) ) != 0 )
        goto cleanup;

    /* Make sure that the returned values are not all 0 or 1 */
    if( ( ret = mbedtls_entropy_source_self_test_check_bits( buf0, sizeof( buf0 ) ) ) != 0 )
        goto cleanup;
    if( ( ret = mbedtls_entropy_source_self_test_check_bits( buf1, sizeof( buf1 ) ) ) != 0 )
        goto cleanup;

    /* Make sure that the entropy source is not returning values in a
     * pattern */
    ret = memcmp( buf0, buf1, sizeof( buf0 ) ) == 0;

cleanup:
    if( verbose != 0 )
    {
        if( ret != 0 )
            mbedtls_printf( "failed\n" );
        else
            mbedtls_printf( "passed\n" );

        mbedtls_printf( "\n" );
    }

    return( ret != 0 );
}

#endif /* MBEDTLS_ENTROPY_HARDWARE_ALT */

/*
 * The actual entropy quality is hard to test, but we can at least
 * test that the functions don't cause errors and write the correct
 * amount of data to buffers.
 */
int mbedtls_entropy_self_test( int verbose )
{
    int ret = 1;
#if !defined(MBEDTLS_TEST_NULL_ENTROPY)
    mbedtls_entropy_context ctx;
    unsigned char buf[MBEDTLS_ENTROPY_BLOCK_SIZE] = { 0 };
    unsigned char acc[MBEDTLS_ENTROPY_BLOCK_SIZE] = { 0 };
    size_t i, j;
#endif /* !MBEDTLS_TEST_NULL_ENTROPY */

    if( verbose != 0 )
        mbedtls_printf( "  ENTROPY test: " );

#if !defined(MBEDTLS_TEST_NULL_ENTROPY)
    mbedtls_entropy_init( &ctx );

    /* First do a gather to make sure we have default sources */
    if( ( ret = mbedtls_entropy_gather( &ctx ) ) != 0 )
        goto cleanup;

    ret = mbedtls_entropy_add_source( &ctx, entropy_dummy_source, NULL, 16,
                                      MBEDTLS_ENTROPY_SOURCE_WEAK );
    if( ret != 0 )
        goto cleanup;

    if( ( ret = mbedtls_entropy_update_manual( &ctx, buf, sizeof buf ) ) != 0 )
        goto cleanup;

    /*
     * To test that mbedtls_entropy_func writes correct number of bytes:
     * - use the whole buffer and rely on ASan to detect overruns
     * - collect entropy 8 times and OR the result in an accumulator:
     *   any byte should then be 0 with probably 2^(-64), so requiring
     *   each of the 32 or 64 bytes to be non-zero has a false failure rate
     *   of at most 2^(-58) which is acceptable.
     */
    for( i = 0; i < 8; i++ )
    {
        if( ( ret = mbedtls_entropy_func( &ctx, buf, sizeof( buf ) ) ) != 0 )
            goto cleanup;

        for( j = 0; j < sizeof( buf ); j++ )
            acc[j] |= buf[j];
    }

    for( j = 0; j < sizeof( buf ); j++ )
    {
        if( acc[j] == 0 )
        {
            ret = 1;
            goto cleanup;
        }
    }

#if defined(MBEDTLS_ENTROPY_HARDWARE_ALT)
    if( ( ret = mbedtls_entropy_source_self_test( 0 ) ) != 0 )
        goto cleanup;
#endif

cleanup:
    mbedtls_entropy_free( &ctx );
#endif /* !MBEDTLS_TEST_NULL_ENTROPY */

    if( verbose != 0 )
    {
        if( ret != 0 )
            mbedtls_printf( "failed\n" );
        else
            mbedtls_printf( "passed\n" );

        mbedtls_printf( "\n" );
    }

    return( ret != 0 );
}
#endif /* MBEDTLS_SELF_TEST_OUT && MBEDTLS_ENTROPY_C */

#if defined(MBEDTLS_SELF_TEST_OUT) && defined(MBEDTLS_PKCS5_C)

#if !defined(MBEDTLS_SHA1_C)
int mbedtls_pkcs5_self_test( int verbose )
{
    if( verbose != 0 )
        mbedtls_printf( "  PBKDF2 (SHA1): skipped\n\n" );

    return( 0 );
}
#else

#define MAX_TESTS   6

static const size_t plen[MAX_TESTS] =
    { 8, 8, 8, 24, 9 };

static const unsigned char password[MAX_TESTS][32] =
{
    "password",
    "password",
    "password",
    "passwordPASSWORDpassword",
    "pass\0word",
};

static const size_t slen[MAX_TESTS] =
    { 4, 4, 4, 36, 5 };

static const unsigned char salt[MAX_TESTS][40] =
{
    "salt",
    "salt",
    "salt",
    "saltSALTsaltSALTsaltSALTsaltSALTsalt",
    "sa\0lt",
};

static const uint32_t it_cnt[MAX_TESTS] =
    { 1, 2, 4096, 4096, 4096 };

static const uint32_t key_len[MAX_TESTS] =
    { 20, 20, 20, 25, 16 };

static const unsigned char result_key[MAX_TESTS][32] =
{
    { 0x0c, 0x60, 0xc8, 0x0f, 0x96, 0x1f, 0x0e, 0x71,
      0xf3, 0xa9, 0xb5, 0x24, 0xaf, 0x60, 0x12, 0x06,
      0x2f, 0xe0, 0x37, 0xa6 },
    { 0xea, 0x6c, 0x01, 0x4d, 0xc7, 0x2d, 0x6f, 0x8c,
      0xcd, 0x1e, 0xd9, 0x2a, 0xce, 0x1d, 0x41, 0xf0,
      0xd8, 0xde, 0x89, 0x57 },
    { 0x4b, 0x00, 0x79, 0x01, 0xb7, 0x65, 0x48, 0x9a,
      0xbe, 0xad, 0x49, 0xd9, 0x26, 0xf7, 0x21, 0xd0,
      0x65, 0xa4, 0x29, 0xc1 },
    { 0x3d, 0x2e, 0xec, 0x4f, 0xe4, 0x1c, 0x84, 0x9b,
      0x80, 0xc8, 0xd8, 0x36, 0x62, 0xc0, 0xe4, 0x4a,
      0x8b, 0x29, 0x1a, 0x96, 0x4c, 0xf2, 0xf0, 0x70,
      0x38 },
    { 0x56, 0xfa, 0x6a, 0xa7, 0x55, 0x48, 0x09, 0x9d,
      0xcc, 0x37, 0xd7, 0xf0, 0x34, 0x25, 0xe0, 0xc3 },
};

int mbedtls_pkcs5_self_test( int verbose )
{
    mbedtls_md_context_t sha1_ctx;
    const mbedtls_md_info_t *info_sha1;
    int ret, i;
    unsigned char key7[64];

    mbedtls_md_init( &sha1_ctx );

    info_sha1 = mbedtls_md_info_from_type( MBEDTLS_MD_SHA1 );
    if( info_sha1 == NULL )
    {
        ret = 1;
        goto exit;
    }

    if( ( ret = mbedtls_md_setup( &sha1_ctx, info_sha1, 1 ) ) != 0 )
    {
        ret = 1;
        goto exit;
    }

    for( i = 0; i < MAX_TESTS; i++ )
    {
        if( verbose != 0 )
            mbedtls_printf( "  PBKDF2 (SHA1) #%d: ", i );

        ret = mbedtls_pkcs5_pbkdf2_hmac( &sha1_ctx, password[i], plen[i], salt[i],
                                  slen[i], it_cnt[i], key_len[i], key7 );
        if( ret != 0 ||
            memcmp( result_key[i], key7, key_len[i] ) != 0 )
        {
            if( verbose != 0 )
                mbedtls_printf( "failed\n" );

            ret = 1;
            goto exit;
        }

        if( verbose != 0 )
            mbedtls_printf( "passed\n" );
    }

    if( verbose != 0 )
        mbedtls_printf( "\n" );

exit:
    mbedtls_md_free( &sha1_ctx );

    return( ret );
}
#endif /* MBEDTLS_SHA1_C */

#endif /* MBEDTLS_SELF_TEST_OUT && MBEDTLS_PKCS5_C */

#if defined(MBEDTLS_SELF_TEST) && defined(MBEDTLS_ECDSA_C) && defined(CONFIG_HW_SECURITY_ENGINE)

const unsigned char ecdsa_d[11][68] =
{
    {
      0x00, 0x00, 0x01, 0xf8, 0x7d, 0x1e, 0xa4, 0xc7, 0x8f, 0x3d, 0xa1, 0x15, 0xcd, 0x2e, 0x3b, 0x5a,\
      0x02, 0x79, 0x75, 0x5f, 0x4d, 0x17, 0x62, 0x03, 0x1f, 0xf0, 0x15, 0x87, 0x7c, 0x41, 0xad, 0x4d,\
      0x43, 0x4c, 0x1a, 0x14, 0x9e, 0x3d, 0x41, 0x8f, 0x43, 0x18, 0x0a, 0x6f, 0x74, 0x23, 0x53, 0x5d,\
      0xe8, 0x68, 0x52, 0xa7, 0xb8, 0x61, 0xac, 0xee, 0x64, 0xf2, 0xf5, 0x99, 0x8e, 0x8f, 0xdd, 0xfd,
      0x86, 0x19, 0x1e, 0xf4,
    },
    {
      0x34, 0xc7, 0xca, 0x76, 0x79, 0x75, 0x6e, 0x1c, 0x01, 0x77, 0xa3, 0x8f, 0x25, 0x6c, 0xb9, 0x35,\
      0xd6, 0x58, 0x4e, 0x82, 0x8d, 0xfc, 0x52, 0x5b, 0x90, 0x95, 0x08, 0x60, 0xb1, 0x6b, 0x6c, 0xa1,\
      0x44, 0xec, 0x20, 0x98, 0x71, 0xda, 0xf6, 0x9c, 0xd7, 0xb8, 0xd3, 0x95, 0x31, 0x7e, 0xe3, 0x9d,\
      0x27, 0x7c, 0xdc, 0xce, 0x22, 0x93, 0x43, 0xba, 0x83, 0xfa, 0xbe, 0x2c, 0x89, 0x14, 0xb8, 0x32,
    },
    {
      0x40, 0xb2, 0x54, 0x40, 0x36, 0x9b, 0xc9, 0x7e, 0xc0, 0xed, 0xe1, 0x71, 0xcc, 0x59, 0x0f, 0xf6,\
      0x65, 0xae, 0xfd, 0x0b, 0xbd, 0xb4, 0xd0, 0xcf, 0x27, 0x22, 0x26, 0xb2, 0x0a, 0xb7, 0x33, 0x10,\
      0xfa, 0xcf, 0x57, 0xe4, 0x9e, 0x97, 0xf3, 0x1b, 0x1b, 0x18, 0x08, 0x99, 0x2b, 0x59, 0x43, 0x80,
    },
    {
      0x4a, 0xfd, 0x05, 0xaa, 0x41, 0x1a, 0x46, 0x93, 0x21, 0x3a, 0x65, 0x74, 0xd2, 0x92, 0x60, 0x7c,\
      0xf8, 0x15, 0x39, 0x8c, 0xa7, 0x27, 0xa6, 0xe8, 0x2a, 0xdc, 0x7c, 0x30, 0xc3, 0xc1, 0x7c, 0x90,\
      0xb0, 0x45, 0x57, 0x69, 0x39, 0x84, 0x14, 0xbd, 0xb5, 0x0d, 0x88, 0x94, 0x0e, 0x77, 0xa2, 0x14,
    },
    {
      0x86, 0xdc, 0x56, 0x92, 0x20, 0x4b, 0x0a, 0xfa, 0xbf, 0x0a, 0x55, 0xdd, 0x6d, 0xd6, 0x6a, 0x67,\
      0xb3, 0x92, 0x27, 0x61, 0x3d, 0x33, 0xb4, 0x83, 0x23, 0x74, 0x71, 0xa8, 0x17, 0xf4, 0x0c, 0x8c,
    },
    {
      0x2a, 0xad, 0xdd, 0x44, 0xa4, 0xef, 0xd0, 0x13, 0x04, 0x6b, 0x90, 0xb5, 0xc0, 0x43, 0xbc, 0xae,\
      0x8b, 0xa5, 0x39, 0xb8, 0xb0, 0x60, 0x71, 0xac, 0x44, 0x0c, 0xa8, 0xc8, 0xd8, 0xc4, 0xa3, 0x86,
    },
    {
      0x5c, 0x31, 0x7c, 0x09, 0x9e, 0x94, 0xac, 0xe4, 0xd1, 0xae, 0x39, 0xe8, 0xb2, 0x79, 0xaa, 0xd8,\
      0x51, 0x16, 0x48, 0x78, 0xbb, 0xf9, 0x77, 0xf3, 0xa6, 0xfc, 0x77, 0xdc, 0x3f, 0x17, 0xa7, 0x8e,
    },
    {
      0xc5, 0xa7, 0x91, 0xa3, 0x61, 0x0b, 0x22, 0x67, 0x11, 0xe0, 0xd1, 0xf6, 0x80, 0xd4, 0x6e, 0x3d,\
      0xa9, 0x6b, 0xd4, 0x00, 0x24, 0x62, 0x28, 0xc6, 0xa8, 0x73, 0x80, 0x0e,
    },
    {
      0x8f, 0x3e, 0xb7, 0x82, 0xd2, 0xbd, 0xaf, 0x60, 0x64, 0xd1, 0x0e, 0x07, 0x2b, 0xc5, 0x94, 0xf6,\
      0x2a, 0x67, 0x16, 0x39, 0xe1, 0x57, 0xf8, 0xd3, 0xcc, 0x5b, 0xae, 0x42,
    },
    {
      0xd8, 0xe3, 0x9c, 0x14, 0xbc, 0x49, 0xa9, 0x2d, 0xe8, 0xff, 0x16, 0x8a, 0x82, 0x83, 0x14, 0x93,\
      0x15, 0xcf, 0xb6, 0xe0, 0xe0, 0x7e, 0xda, 0x11,
    },
    {
      0x88, 0x9a, 0xf5, 0x5f, 0x45, 0xe4, 0x02, 0x01, 0xc8, 0x2d, 0xb3, 0x18, 0x7e, 0x5e, 0x4f, 0xc2,\
      0x54, 0x6d, 0xea, 0x2c, 0xe5, 0x0e, 0x02, 0x30,
    },
};
const unsigned char ecdsa_d_len[11] = {68, 64, 48, 48, 32, 32, 32, 28, 28, 24, 24};

const unsigned char ecdsa_qx[11][68] =
{
    {
      0x00, 0x00, 0x01, 0x5f, 0x20, 0x22, 0x9c, 0x31, 0x7c, 0xbb, 0xaf, 0xbf, 0xd8, 0xdd, 0x75, 0x9d,\
      0xd2, 0xa0, 0xa0, 0x0c, 0x7b, 0xe7, 0xcf, 0xa0, 0x22, 0xf2, 0x57, 0xa2, 0x0c, 0xd7, 0x6f, 0x05,\
      0xa6, 0x5a, 0x09, 0xd3, 0x2e, 0xb2, 0x04, 0xd8, 0x22, 0x87, 0x27, 0xae, 0x51, 0xbb, 0xba, 0x90,\
      0xab, 0xfd, 0xd4, 0x83, 0xb7, 0x35, 0xf0, 0x7f, 0xf1, 0x15, 0x97, 0x54, 0xd2, 0x70, 0xc3, 0xfe,
      0x9c, 0xcb, 0x4c, 0x6d,
    },
    {
      0x13, 0xbd, 0x19, 0x0c, 0x9b, 0x40, 0xad, 0x58, 0x58, 0xde, 0x34, 0x8c, 0xba, 0x54, 0x36, 0xcb,\
      0x3c, 0x2a, 0xbf, 0x8d, 0x7b, 0x4c, 0x0b, 0x49, 0xd6, 0x30, 0xf1, 0x05, 0xbc, 0xca, 0x81, 0x0e,\
      0x9a, 0x35, 0xdf, 0x66, 0x8f, 0x9a, 0x0e, 0x99, 0xca, 0x50, 0xfe, 0x4f, 0x55, 0x87, 0x69, 0x86,\
      0x1f, 0xf3, 0xbf, 0x78, 0x98, 0xb7, 0x65, 0x50, 0x02, 0x64, 0x7d, 0xfa, 0x8b, 0x09, 0x25, 0x8c,
    },
    {
      0xbb, 0xae, 0xed, 0xa7, 0xc6, 0xb7, 0x83, 0xcc, 0xd9, 0xad, 0x34, 0x9c, 0x1b, 0xe4, 0x04, 0x67,\
      0xd9, 0x06, 0x0c, 0x3d, 0x35, 0x3a, 0xb9, 0xea, 0x2d, 0x71, 0x3f, 0x65, 0x2f, 0xc5, 0xba, 0xae,\
      0x6f, 0xcf, 0xbb, 0x92, 0xc1, 0x1e, 0xc4, 0x55, 0x9d, 0xba, 0x2e, 0xdc, 0xfc, 0xa6, 0xd1, 0xfd,
    },
    {
      0x79, 0x4a, 0x19, 0x8e, 0x9c, 0x8e, 0x9c, 0x2a, 0xe0, 0x37, 0xbe, 0xb7, 0x0a, 0x19, 0x0a, 0x98,\
      0x6b, 0xa5, 0x8c, 0x6d, 0x5e, 0x62, 0x61, 0x3a, 0xaf, 0x31, 0x8f, 0x92, 0x1c, 0x6f, 0xe5, 0x3a,\
      0xb3, 0x14, 0x7a, 0xc1, 0xc8, 0x97, 0xde, 0xd5, 0x3e, 0xd7, 0x6d, 0xee, 0x12, 0xd4, 0x32, 0xf9,
    },
    {
      0x5d, 0x4f, 0xba, 0xcf, 0x54, 0x98, 0xf7, 0xc9, 0x1f, 0x14, 0x06, 0x6c, 0x48, 0xe1, 0xf7, 0xc0,\
      0xcc, 0x39, 0x7c, 0xc3, 0x2b, 0xcf, 0x42, 0x94, 0x14, 0xb4, 0x71, 0xfb, 0x88, 0xcf, 0xd5, 0x38,
    },
    {
      0xab, 0x98, 0x06, 0x43, 0x4b, 0xa2, 0xdc, 0x53, 0x6d, 0x4b, 0x5b, 0x05, 0x96, 0xf3, 0xd7, 0xa8,\
      0x72, 0xdd, 0x80, 0xfb, 0xf3, 0x7f, 0xa7, 0x29, 0xfa, 0x6b, 0xbe, 0xde, 0xcd, 0xdd, 0x39, 0x4f,
    },
    {
      0x36, 0x2d, 0x4d, 0x7f, 0x85, 0xac, 0xa4, 0x74, 0x17, 0x86, 0x82, 0x97, 0x9c, 0x00, 0x62, 0xee,\
      0xe1, 0x26, 0x5f, 0x64, 0x84, 0x83, 0xd6, 0x3d, 0x8d, 0x1c, 0x02, 0x0b, 0x45, 0x18, 0x05, 0xe0,
    },
    {
      0x36, 0x82, 0x42, 0xf7, 0x4c, 0xaf, 0x6a, 0x06, 0x39, 0xf7, 0x99, 0x03, 0x59, 0xae, 0xb3, 0xe0,\
      0x54, 0x5b, 0x61, 0x53, 0xb3, 0x93, 0xc6, 0x06, 0xd2, 0x2f, 0x96, 0x91,
    },
    {
      0xb2, 0xc4, 0xf2, 0x36, 0xa1, 0x0d, 0x22, 0x4f, 0x01, 0x70, 0x18, 0xdf, 0xe5, 0xc6, 0xda, 0x80,\
      0xb8, 0xe6, 0x6a, 0x86, 0xae, 0x10, 0xa3, 0xea, 0x31, 0x49, 0x7a, 0x3b,
    },
    {
      0xe4, 0xc0, 0x78, 0xdd, 0x11, 0x91, 0x75, 0xc6, 0x6b, 0x4d, 0x80, 0x97, 0x12, 0x21, 0xd5, 0xc1,\
      0x23, 0x9f, 0x28, 0xc8, 0xba, 0xde, 0x97, 0x61,
    },
    {
      0xd2, 0xe2, 0xe0, 0x2b, 0x34, 0x4b, 0x11, 0x7e, 0x90, 0x47, 0x2d, 0xe0, 0x92, 0x9d, 0x99, 0x70,\
      0x28, 0x33, 0xcb, 0xbd, 0x84, 0x34, 0x17, 0xb9,
    },
};
const unsigned char ecdsa_qx_len[11] = {68, 64, 48, 48, 32, 32, 32, 28, 28, 24, 24,};


const unsigned char ecdsa_qy[11][68] =
{
    {
      0x00, 0x00, 0x01, 0x98, 0x3d, 0x0f, 0x8a, 0x68, 0x72, 0xfc, 0xb5, 0xf3, 0xa0, 0x42, 0x64, 0x3c,\
      0xcc, 0x7c, 0x2a, 0x5e, 0x95, 0x1f, 0x68, 0x33, 0x67, 0xe5, 0xb0, 0xbf, 0x9f, 0xf6, 0x28, 0x40,\
      0x2e, 0x26, 0x00, 0xc0, 0x78, 0x87, 0x6a, 0xa6, 0xd3, 0xc6, 0x4f, 0x4b, 0xf1, 0x6a, 0x7f, 0x62,\
      0x90, 0x9b, 0x3e, 0x5f, 0xa9, 0xc0, 0xd0, 0xf8, 0x01, 0xe4, 0x65, 0xb8, 0xdf, 0xec, 0xfc, 0x4a,
      0x34, 0xa2, 0x69, 0xab,
    },
    {
      0x79, 0xeb, 0xf2, 0xb7, 0x4f, 0x43, 0xcb, 0xcb, 0x98, 0x22, 0x98, 0xd3, 0x28, 0xa2, 0x16, 0x95,\
      0x1b, 0x91, 0x8e, 0x72, 0xa8, 0x28, 0xba, 0xf0, 0x62, 0x41, 0x49, 0xc2, 0x80, 0x8f, 0xad, 0xba,\
      0xb1, 0x21, 0xd8, 0x37, 0x49, 0x35, 0xe5, 0xa8, 0x4e, 0x2f, 0x4b, 0x9c, 0xcf, 0x3d, 0x4d, 0xdf,\
      0xb1, 0x4d, 0xd6, 0x96, 0xab, 0x6e, 0x48, 0x3a, 0x68, 0x05, 0x62, 0xe2, 0x3c, 0x76, 0xdc, 0x68,
    },
    {
      0x46, 0xcf, 0x94, 0x25, 0x14, 0xb6, 0xd4, 0x56, 0x9d, 0xb6, 0xda, 0x01, 0x9e, 0x95, 0xba, 0x74,\
      0x41, 0x91, 0xb5, 0xdb, 0x74, 0x8d, 0xb0, 0x0f, 0xc1, 0x5c, 0xeb, 0x35, 0x80, 0x4d, 0xd1, 0x60,\
      0x03, 0xb6, 0x3f, 0x84, 0x49, 0x50, 0x59, 0x88, 0x67, 0x14, 0x1b, 0x4d, 0x0b, 0x46, 0xdd, 0xc7,
    },

    {
      0x3c, 0x86, 0x22, 0x46, 0x3e, 0x52, 0xd1, 0xa9, 0x24, 0x26, 0x87, 0x89, 0x97, 0x58, 0x44, 0x76,\
      0xed, 0xb3, 0xe0, 0x65, 0x01, 0xf5, 0x5f, 0x24, 0x20, 0x9b, 0x14, 0x5e, 0x5a, 0xc4, 0x3c, 0x9e,\
      0x8f, 0xcb, 0x51, 0x75, 0xa6, 0x6d, 0xda, 0x9f, 0x47, 0x18, 0x04, 0x0c, 0x75, 0x8a, 0x4d, 0x90,
    },
    {
      0x63, 0x47, 0x6e, 0x46, 0x3f, 0x57, 0x5b, 0x3b, 0x36, 0x87, 0x22, 0x17, 0x7a, 0xf9, 0x6c, 0x7e,\
      0xfb, 0x8f, 0x6d, 0x48, 0x1f, 0x0d, 0xf1, 0xbf, 0xc6, 0xe7, 0x15, 0x57, 0x41, 0xfe, 0x7e, 0x50,
    },
    {
      0xc1, 0x88, 0xa6, 0xe9, 0x87, 0xf6, 0x65, 0xc5, 0x5a, 0x1c, 0x0f, 0x14, 0xf5, 0x1c, 0x44, 0x80,\
      0x8c, 0xc7, 0xc9, 0x6a, 0x6b, 0x89, 0x9e, 0x43, 0xb8, 0xa4, 0x9b, 0x30, 0x62, 0x79, 0x33, 0x48,
    },
    {
      0x6c, 0x54, 0x90, 0xef, 0x53, 0x8e, 0x45, 0x5a, 0x54, 0xcb, 0xca, 0x05, 0x3b, 0xaf, 0x12, 0x0e,\
      0xa9, 0x74, 0x98, 0x58, 0xb3, 0x8f, 0x6d, 0x00, 0x99, 0xd1, 0x6f, 0x3f, 0x7c, 0x37, 0x89, 0x9f,
    },
    {
      0x7c, 0xf5, 0xe2, 0x06, 0x81, 0x10, 0xb6, 0xdf, 0xf3, 0xd4, 0x20, 0x93, 0xc3, 0x4d, 0x71, 0xe5,\
      0xe7, 0x6a, 0x43, 0xe5, 0x0a, 0xb6, 0xa4, 0x64, 0xc3, 0xae, 0xd9, 0xf5,
    },
    {
      0xf8, 0xd2, 0x10, 0xa6, 0xfe, 0x4e, 0x93, 0x1d, 0x80, 0xc2, 0x79, 0xd5, 0xbb, 0x0a, 0xd3, 0x98,\
      0x8b, 0xd6, 0x0d, 0x24, 0xa3, 0x1d, 0xdf, 0x3b, 0x80, 0xeb, 0x65, 0x26,
    },
    {
      0x97, 0x19, 0x20, 0xc9, 0xb1, 0x43, 0xb4, 0xd3, 0x34, 0x9a, 0xce, 0x86, 0x7e, 0x0b, 0x5b, 0x6d,\
      0xc5, 0xa2, 0x59, 0x5a, 0xb3, 0xf5, 0x4c, 0x76,
    },
    {
      0x6a, 0xc6, 0xd4, 0x73, 0xc5, 0x3d, 0xdd, 0x77, 0x7d, 0x59, 0xf0, 0x36, 0x36, 0xff, 0xda, 0xb4,\
      0xf2, 0x67, 0xd5, 0xb2, 0x65, 0x7d, 0xb5, 0xe5,
    },
};
const unsigned char ecdsa_qy_len[11] = {68, 64, 48, 48, 32, 32, 32, 28, 28, 24, 24,};

const unsigned char ecdsa_qz[4] = {0x00, 0x00, 0x00, 0x01,};

const unsigned char ecdsa_sig[11][MBEDTLS_ECDSA_MAX_LEN] =
{
    {
      0x30, 0x81, 0x87, 0x02, 0x41, 0x7b, 0xf6, 0xbd, 0xa9, 0x3b, 0x15, 0x03, 0xa1, 0x1f, 0x0a, 0xc4,\
      0xec, 0x54, 0x44, 0x88, 0x5e, 0x3a, 0x6f, 0x3a, 0xa7, 0x0e, 0x30, 0xc9, 0xed, 0x25, 0x0f, 0x94,\
      0x98, 0xb8, 0x81, 0x06, 0xfd, 0xd5, 0xfb, 0xa4, 0x57, 0xd2, 0x0d, 0xa5, 0x19, 0x0b, 0x7b, 0x71,\
      0x01, 0x17, 0x83, 0xb1, 0x34, 0xea, 0x70, 0xa3, 0x85, 0x49, 0x9a, 0x40, 0x20, 0x16, 0x42, 0x0f,\
      0x17, 0xf1, 0xc0, 0xa5, 0xaf, 0x24, 0x02, 0x42, 0x00, 0xee, 0xb7, 0xbd, 0x89, 0x84, 0x68, 0x1a,\
      0x26, 0x6b, 0x97, 0x47, 0xb9, 0x39, 0x40, 0x71, 0xf4, 0xda, 0x2a, 0xb6, 0x86, 0xdb, 0x23, 0x5e,\
      0xfa, 0x1c, 0x72, 0x44, 0x62, 0x80, 0x0d, 0x99, 0xa6, 0x15, 0xf0, 0xa8, 0x9a, 0x23, 0x88, 0x98,\
      0x49, 0xbd, 0xff, 0x1e, 0x84, 0x26, 0xc9, 0x82, 0x8b, 0x3e, 0x76, 0xec, 0x9b, 0x9e, 0x22, 0x9a,\
      0x3b, 0x4f, 0xca, 0x08, 0x65, 0x2a, 0x70, 0xc7, 0x03, 0x52,
    },
    {
      0x30, 0x81, 0x85, 0x02, 0x41, 0x00, 0x8c, 0x61, 0xed, 0xcc, 0xf7, 0xc0, 0x81, 0xb3, 0xe7, 0x32,\
      0x26, 0xf4, 0x0f, 0x3f, 0x8f, 0x2e, 0xcc, 0x8c, 0x91, 0xc9, 0xa0, 0x14, 0x10, 0xc2, 0xc0, 0x8b,\
      0xf7, 0xbe, 0xbf, 0x08, 0xff, 0x74, 0x1a, 0xc9, 0x58, 0xb2, 0xb0, 0x6d, 0x08, 0xe1, 0xc5, 0xe6,\
      0xa5, 0x7b, 0x41, 0x96, 0x05, 0xb5, 0x57, 0xcb, 0x0c, 0xea, 0x9b, 0xec, 0x0f, 0x7c, 0x42, 0x7b,\
      0x7f, 0x06, 0x9c, 0xbe, 0xd8, 0xa6, 0x02, 0x40, 0x1b, 0xcb, 0xf5, 0xcf, 0x3a, 0x27, 0x8e, 0xba,\
      0x25, 0x91, 0x92, 0x6b, 0x9d, 0x4a, 0xeb, 0x25, 0x87, 0x1b, 0x02, 0x4a, 0x0a, 0x56, 0xa1, 0x3f,\
      0xfd, 0x8d, 0x20, 0x88, 0x72, 0x1a, 0x32, 0x28, 0x4e, 0x7a, 0x5c, 0xae, 0xad, 0xa4, 0xcc, 0xfc,\
      0x29, 0x5f, 0x8e, 0xce, 0x52, 0x5c, 0xfd, 0x88, 0xe6, 0x3f, 0x26, 0x53, 0x33, 0x8f, 0xb4, 0xee,\
      0x02, 0x09, 0xe8, 0xfa, 0xf7, 0xc8, 0xc3, 0x25,
    },
    {
      0x30, 0x66, 0x02, 0x31, 0x00, 0xce, 0x13, 0xbd, 0x3f, 0x38, 0xbc, 0xb5, 0x4a, 0xe8, 0x93, 0x9a,\
      0x9e, 0xf9, 0x72, 0xf8, 0x8e, 0x07, 0xc2, 0x14, 0x11, 0xd2, 0x99, 0x37, 0x92, 0x84, 0x33, 0x88,\
      0xb3, 0xe0, 0x35, 0x76, 0x5d, 0xb7, 0x64, 0xd5, 0x12, 0xa4, 0x68, 0x55, 0x25, 0x26, 0x5b, 0x11,\
      0xa7, 0xd9, 0x54, 0x84, 0x2c, 0x02, 0x31, 0x00, 0x90, 0x0c, 0xfd, 0xa1, 0xcb, 0xa3, 0xd0, 0x6d,\
      0x28, 0x38, 0xbc, 0x2b, 0xec, 0x4d, 0xc2, 0x14, 0xa6, 0xb3, 0x04, 0x07, 0x3b, 0x46, 0xd4, 0xfa,\
      0x14, 0x18, 0xba, 0xac, 0xbc, 0x5f, 0xf1, 0x3d, 0x37, 0xbe, 0xed, 0x2f, 0x24, 0x5d, 0x7a, 0x2a,\
      0xda, 0xf3, 0x6d, 0x04, 0x35, 0x33, 0x8d, 0x99,
    },
    {
      0x30, 0x64, 0x02, 0x30, 0x3d, 0x8f, 0x47, 0xb5, 0xfb, 0x07, 0xf0, 0x81, 0x3d, 0x72, 0xec, 0xf6,\
      0xd5, 0xaa, 0xa9, 0x58, 0x70, 0x12, 0x8f, 0x0f, 0x71, 0x84, 0xbb, 0xd2, 0xc0, 0x71, 0xcf, 0x6a,\
      0x35, 0xd4, 0x8f, 0x65, 0xc9, 0x56, 0xb2, 0x15, 0xfc, 0x62, 0x9e, 0x12, 0xb1, 0xb0, 0xda, 0x5e,\
      0x22, 0xb9, 0x9f, 0x13, 0x02, 0x30, 0x65, 0x1d, 0x6a, 0x98, 0x48, 0x28, 0x4c, 0x59, 0xc3, 0x56,\
      0xb5, 0x48, 0x65, 0x65, 0x74, 0x48, 0x89, 0x89, 0x93, 0xc9, 0x16, 0x0f, 0xff, 0x36, 0xb6, 0x09,\
      0x4c, 0xdd, 0x70, 0x87, 0xef, 0x30, 0x7a, 0x8b, 0xdb, 0x10, 0x82, 0x25, 0xe0, 0x39, 0x28, 0x46,\
      0x1f, 0x4d, 0x2c, 0x39, 0x3b, 0x2e,
    },
    {
      0x30, 0x44, 0x02, 0x20, 0x32, 0x76, 0xbb, 0x41, 0xdd, 0x75, 0x76, 0x66, 0xcd, 0x5a, 0x0b, 0xbc,\
      0x92, 0x45, 0xb5, 0x1c, 0xca, 0x76, 0x33, 0x4e, 0x61, 0x45, 0xee, 0x82, 0xcc, 0xae, 0x22, 0x1a,\
      0xce, 0x32, 0x8e, 0x97, 0x02, 0x20, 0x49, 0x18, 0x30, 0x1b, 0x90, 0x32, 0x2f, 0x5c, 0x59, 0xa4,\
      0x3f, 0x51, 0x33, 0xaa, 0x98, 0xb2, 0x2b, 0x3c, 0x28, 0xa6, 0x82, 0x64, 0x1e, 0xac, 0xee, 0x45,\
      0xfc, 0x8b, 0x94, 0x8c, 0x37, 0x76,
    },
    {
      0x30, 0x46, 0x02, 0x21, 0x00, 0xbb, 0x2b, 0xb9, 0x02, 0x01, 0xd5, 0xb3, 0x84, 0x8b, 0x0d, 0x2f,\
      0x5d, 0xea, 0x3e, 0xf4, 0xb7, 0xa4, 0x91, 0xfd, 0xe9, 0x73, 0x6e, 0x15, 0x9c, 0x49, 0xb8, 0x2e,\
      0x12, 0xf9, 0xc2, 0x75, 0xdc, 0x02, 0x21, 0x00, 0xed, 0x1b, 0xc2, 0x3b, 0x6d, 0xd2, 0xe8, 0x60,\
      0xe6, 0x41, 0x4d, 0x9e, 0x0b, 0x32, 0xac, 0xe0, 0xb7, 0x84, 0x47, 0x78, 0xbb, 0x07, 0x05, 0x92,\
      0xa3, 0xab, 0x67, 0x8f, 0x3c, 0x00, 0x62, 0x8c,
    },
    {
      0x30, 0x44, 0x02, 0x20, 0x3a, 0x9e, 0x2e, 0x5c, 0x9f, 0x73, 0x62, 0x9d, 0xbc, 0xa6, 0xa9, 0xe4,\
      0x42, 0x86, 0x01, 0x1e, 0xdc, 0x0f, 0x3f, 0x6c, 0x55, 0xd2, 0x60, 0x1b, 0x65, 0xc0, 0xf6, 0xcb,\
      0x9c, 0xfa, 0x63, 0x49, 0x02, 0x20, 0x58, 0xd0, 0xba, 0x82, 0x3d, 0x55, 0xe5, 0x8a, 0x2c, 0x8b,\
      0x30, 0x3e, 0xca, 0xf5, 0x0d, 0xc9, 0xf2, 0x02, 0x13, 0x6f, 0xf8, 0xe0, 0x4f, 0xe9, 0x20, 0xf4,\
      0xb8, 0xc4, 0xfe, 0xf9, 0x96, 0x73,
    },
    {
      0x30, 0x3d, 0x02, 0x1d, 0x00, 0xa5, 0x56, 0x8b, 0x2b, 0x5e, 0x01, 0xbb, 0xb4, 0x5a, 0x92, 0x67,\
      0x8a, 0xd9, 0xbb, 0xc1, 0x74, 0xbb, 0x8a, 0xc6, 0x4b, 0x53, 0xff, 0xbc, 0xd0, 0x3e, 0x7b, 0x2d,\
      0x05, 0x02, 0x1c, 0x1d, 0x79, 0x4b, 0xaa, 0x3b, 0xe8, 0xf7, 0x5a, 0x44, 0x7a, 0xfc, 0x28, 0x04,\
      0x82, 0xcc, 0x69, 0xe6, 0xaf, 0xd1, 0x38, 0xec, 0x2a, 0x62, 0x80, 0x1d, 0xc6, 0x6a, 0x5c,
    },
    {
      0x30, 0x3e, 0x02, 0x1d, 0x00, 0xa1, 0x2f, 0x8f, 0xc9, 0xba, 0xfd, 0x88, 0x05, 0xe9, 0xc7, 0xa6,\
      0x53, 0x22, 0x2e, 0xea, 0x75, 0xe6, 0xd5, 0x9a, 0x26, 0x27, 0x82, 0x41, 0xe5, 0xa2, 0x52, 0x17,\
      0xb7, 0x02, 0x1d, 0x00, 0xf9, 0x0e, 0xa5, 0x7c, 0xa6, 0x2f, 0xd0, 0x60, 0xf4, 0xe6, 0xce, 0x99,\
      0xd0, 0xdc, 0xcc, 0x4e, 0x0d, 0x9a, 0x3d, 0x0c, 0x16, 0xc5, 0x14, 0xc7, 0xa3, 0x65, 0x0f, 0x50,
    },
    {
      0x30, 0x36, 0x02, 0x19, 0x00, 0xf3, 0xa2, 0x9f, 0x29, 0x63, 0x61, 0xda, 0xaf, 0x98, 0x3f, 0x78,\
      0x98, 0xeb, 0x2c, 0x5b, 0xb0, 0x3e, 0x8e, 0x68, 0x84, 0xb6, 0x21, 0x75, 0x29, 0x02, 0x19, 0x00,\
      0x95, 0x42, 0xdd, 0xd8, 0xa4, 0x6e, 0xd1, 0x9e, 0x66, 0xc6, 0x74, 0xa1, 0xaa, 0x9c, 0x7c, 0x26,\
      0x22, 0x2a, 0xd4, 0xcb, 0xed, 0x98, 0x4e, 0xea,
    },
    {
      0x30, 0x35, 0x02, 0x19, 0x00, 0x97, 0x61, 0xe0, 0x2b, 0x21, 0x3d, 0x49, 0x3d, 0xd0, 0x54, 0x76,\
      0x3d, 0xae, 0xbc, 0x58, 0x01, 0x94, 0x0f, 0xe0, 0xd5, 0xfc, 0xe3, 0xb6, 0x72, 0x02, 0x18, 0x74,\
      0x8f, 0x68, 0x14, 0x0d, 0x3e, 0x07, 0x7d, 0xee, 0x1c, 0xe4, 0xb1, 0xca, 0xd3, 0x3a, 0x45, 0xad,\
      0x9a, 0xaf, 0xde, 0x42, 0x20, 0x26, 0x6b,
    },
};
const unsigned char ecdsa_sig_len[11] = {138, 136, 104, 102, 70, 72, 70, 63, 64, 56, 55,};

static int mbedtls_ecdsa_self_test(int verbose)
{
    int ret = 1;
    int exit_code = MBEDTLS_EXIT_FAILURE;
    mbedtls_ecdsa_context ctx_sign, ctx_verify;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    unsigned char message[100];
    unsigned char hash[32];
    unsigned char sig[MBEDTLS_ECDSA_MAX_LEN];
    size_t sig_len;
    const char *pers = "ecdsa";
    unsigned int group_id, nb_curves = 0;
    const mbedtls_ecp_curve_info *ecp_supported_curves_temp = (const mbedtls_ecp_curve_info *)ecp_supported_curves;

    mbedtls_ecdsa_init( &ctx_sign );
    mbedtls_ecdsa_init( &ctx_verify );
    mbedtls_ctr_drbg_init( &ctr_drbg );
    mbedtls_entropy_init( &entropy );

NextCurve:
    if (nb_curves < 11) {
        exit_code = MBEDTLS_EXIT_FAILURE;
        mbedtls_ecdsa_free( &ctx_verify );
        mbedtls_ecdsa_free( &ctx_sign );
        mbedtls_ctr_drbg_free( &ctr_drbg );
        mbedtls_entropy_free( &entropy );

        mbedtls_ecdsa_init( &ctx_sign );
        mbedtls_ecdsa_init( &ctx_verify );
        mbedtls_ctr_drbg_init( &ctr_drbg );
        mbedtls_entropy_init( &entropy );
    } else {
        goto exit;
    }
    if( verbose != 0 )
        mbedtls_printf( "  ====== ECC Group[%d] %s ====== \r\n", nb_curves, ecp_supported_curves_temp[nb_curves].name);
    group_id = ecp_supported_curves_temp[nb_curves].grp_id;

    memset( sig, 0, sizeof( sig ) );
    memset( message, 0x25, sizeof( message ) );

    /*
     * Generate a key pair for signing
     */
    mbedtls_printf( "\n  . Seeding the random number generator..." );

    if( ( ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n  . Generating key pair..." );

#if 0
    if( ( ret = mbedtls_ecdsa_genkey( &ctx_sign, group_id,
                                mbedtls_ctr_drbg_random, &ctr_drbg ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ecdsa_genkey returned %d\n", ret );
        goto exit;
    }
#else
    if ( ( ret = mbedtls_ecp_group_load( &ctx_sign.grp, group_id )) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ecp_group_load returned %d\n", ret );
        goto exit;
    }
    mbedtls_mpi_read_binary(&ctx_sign.d,   ecdsa_d[nb_curves],  ecdsa_d_len[nb_curves]);
    mbedtls_mpi_read_binary(&ctx_sign.Q.X, ecdsa_qx[nb_curves], ecdsa_qx_len[nb_curves]);
    mbedtls_mpi_read_binary(&ctx_sign.Q.Y, ecdsa_qy[nb_curves], ecdsa_qy_len[nb_curves]);
    mbedtls_mpi_read_binary(&ctx_sign.Q.Z, ecdsa_qz, 4);
#endif

    mbedtls_printf( " ok (key size: %d bits)\n", (int) ctx_sign.grp.pbits );

    dump_pubkey( "  + Public key: ", &ctx_sign );

    /*
     * Compute message hash
     */
    mbedtls_printf( "  . Computing message hash..." );

    if( ( ret = mbedtls_sha256_ret( message, sizeof( message ), hash, 0 ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_sha256_ret returned %d\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );
    //dump_buf( "  + Hash: ", hash, sizeof( hash ) );

    /*
     * Sign message hash
     */
    mbedtls_printf( "  . Signing message hash..." );

    if( ( ret = mbedtls_ecdsa_write_signature( &ctx_sign, MBEDTLS_MD_SHA256,
                                       hash, sizeof( hash ),
                                       sig, &sig_len,
                                       mbedtls_ctr_drbg_random, &ctr_drbg ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ecdsa_genkey returned %d\n", ret );
        goto exit;
    }
    mbedtls_printf( " ok (signature length = %u)\n", (unsigned int) sig_len );

   //dump_buf( "  + Signature: ", sig, sig_len );

    /*
     * Transfer public information to verifying context
     *
     * We could use the same context for verification and signatures, but we
     * chose to use a new one in order to make it clear that the verifying
     * context only needs the public key (Q), and not the private key (d).
     */
    mbedtls_printf( "  . Preparing verification context..." );

    if( ( ret = mbedtls_ecp_group_copy( &ctx_verify.grp, &ctx_sign.grp ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ecp_group_copy returned %d\n", ret );
        goto exit;
    }

    if( ( ret = mbedtls_ecp_copy( &ctx_verify.Q, &ctx_sign.Q ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ecp_copy returned %d\n", ret );
        goto exit;
    }


    /*
     * Verify signature
     */
    mbedtls_printf( " ok\n  . Verifying signature..." );

    if( ( ret = mbedtls_ecdsa_read_signature( &ctx_verify,
                                      hash, sizeof( hash ),
                                      sig, sig_len ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ecdsa_read_signature returned %d\n", ret );
        goto exit;
    }

    if( (ecdsa_sig_len[nb_curves] != sig_len) ||
        (memcmp( sig, ecdsa_sig[nb_curves], ecdsa_sig_len[nb_curves] ) != 0) )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ecdsa_read_signature mismatch\n");
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    exit_code = MBEDTLS_EXIT_SUCCESS;

    nb_curves++;
    goto NextCurve;

exit:

    mbedtls_ecdsa_free( &ctx_verify );
    mbedtls_ecdsa_free( &ctx_sign );
    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );

    return( exit_code );
}
#endif /* MBEDTLS_ECDSA_C && MBEDTLS_ECDSA_C && CONFIG_HW_SECURITY_ENGINE */

void ssl_self_test(void *arg)
{
#if defined(MBEDTLS_SELF_TEST)

    int v, suites_tested = 0, suites_failed = 0;
#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C) && defined(MBEDTLS_SELF_TEST) && !defined(MBEDTLS_SELF_TEST_OUT)
    unsigned char buf[1000000];
#endif
    void *pointer;

    /*
     * The C standard doesn't guarantee that all-bits-0 is the representation
     * of a NULL pointer. We do however use that in our code for initializing
     * structures, which should work on every modern platform. Let's be sure.
     */
    memset( &pointer, 0, sizeof( void * ) );
    if( pointer != NULL )
    {
        mbedtls_printf( "all-bits-zero is not a NULL pointer\r\n" );
        //mbedtls_exit( MBEDTLS_EXIT_FAILURE );
    }

    {
        v = 1;
        mbedtls_printf( "\r\n" );
    }

#if defined(MBEDTLS_SELF_TEST)

#ifndef MBEDTLS_SELF_TEST_OUT
#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
    mbedtls_memory_buffer_alloc_init( buf, sizeof(buf) );
#endif
#endif

#if defined(MBEDTLS_MD2_C)
    if( mbedtls_md2_self_test( v )  != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if defined(MBEDTLS_MD4_C)
    if( mbedtls_md4_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if defined(MBEDTLS_MD5_C)
    if( mbedtls_md5_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if defined(MBEDTLS_RIPEMD160_C)
    if( mbedtls_ripemd160_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if defined(MBEDTLS_SHA1_C)
    if( mbedtls_sha1_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if defined(MBEDTLS_SHA256_C)
    if( mbedtls_sha256_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if defined(MBEDTLS_SHA512_C)
    if( mbedtls_sha512_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if defined(MBEDTLS_ARC4_C)
    if( mbedtls_arc4_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if defined(MBEDTLS_DES_C)
    if( mbedtls_des_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if defined(MBEDTLS_AES_C)
    if( mbedtls_aes_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if defined(MBEDTLS_GCM_C) && defined(MBEDTLS_AES_C)
    if( mbedtls_gcm_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if defined(MBEDTLS_CCM_C) && defined(MBEDTLS_AES_C)
    if( mbedtls_ccm_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if defined(MBEDTLS_CMAC_C)
    if( ( mbedtls_cmac_self_test( v ) ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if defined(MBEDTLS_BASE64_C)
    if( mbedtls_base64_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if defined(MBEDTLS_BIGNUM_C)
    if( mbedtls_mpi_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#ifdef CONFIG_HW_SECURITY_ENGINE
    mbedtls_mpi_exp_mod_self_test_512(1);
    mbedtls_mpi_exp_mod_self_test_1024(1);
#endif
#endif

#if defined(MBEDTLS_RSA_C)
    if( mbedtls_rsa_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if 0//defined(MBEDTLS_X509_USE_C)
    if( mbedtls_x509_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if defined(MBEDTLS_XTEA_C)
    if( mbedtls_xtea_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if defined(MBEDTLS_CAMELLIA_C)
    if( mbedtls_camellia_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if defined(MBEDTLS_CTR_DRBG_C)
    if( mbedtls_ctr_drbg_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if defined(MBEDTLS_HMAC_DRBG_C)
    if( mbedtls_hmac_drbg_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif


#if 1
#if defined(MBEDTLS_ECP_C)
    if( mbedtls_ecp_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif
#endif

#if defined(MBEDTLS_ECDSA_C)
#ifdef CONFIG_HW_SECURITY_ENGINE
    if( mbedtls_ecdsa_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif
#endif

#if defined(MBEDTLS_ECJPAKE_C)
//    if( mbedtls_ecjpake_self_test( v ) != 0 )
//    {
//        suites_failed++;
//    }
//    suites_tested++;
#endif

#if defined(MBEDTLS_DHM_C)
    if( mbedtls_dhm_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if defined(MBEDTLS_ENTROPY_C)

#if defined(MBEDTLS_ENTROPY_NV_SEED) && !defined(MBEDTLS_NO_PLATFORM_ENTROPY)
    create_entropy_seed_file();
#endif

    if( mbedtls_entropy_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

#if defined(MBEDTLS_PKCS5_C)
    if( mbedtls_pkcs5_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif

/* Slow tests last */

#if defined(MBEDTLS_TIMING_C)
/*
    if( mbedtls_timing_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
*/
#endif

    if( v != 0 )
    {
#ifndef MBEDTLS_SELF_TEST_OUT
#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C) && defined(MBEDTLS_MEMORY_DEBUG)
        mbedtls_memory_buffer_alloc_status();
#endif
#endif /* MBEDTLS_SELF_TEST_OUT */
    }

#ifndef MBEDTLS_SELF_TEST_OUT
#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
    mbedtls_memory_buffer_alloc_free();
    if( mbedtls_memory_buffer_alloc_self_test( v ) != 0 )
    {
        suites_failed++;
    }
    suites_tested++;
#endif
#endif /* MBEDTLS_SELF_TEST_OUT */

#else
    mbedtls_printf( " MBEDTLS_SELF_TEST not defined.\r\n" );
#endif

    if( v != 0 )
    {
        mbedtls_printf( "  Executed %d test suites\r\n", suites_tested );

        if( suites_failed > 0)
        {
            mbedtls_printf( "  [ %d tests FAIL ]\r\n", suites_failed );
        }
        else
        {
            mbedtls_printf( "  [ All tests PASS ]\r\n" );
        }
#if defined(_WIN32)
        mbedtls_printf( "  Press Enter to exit this program.\r\n" );
        fflush( stdout ); getchar();
#endif
    }

    //if( suites_failed > 0)
    //    mbedtls_exit( MBEDTLS_EXIT_FAILURE );

#endif//MBEDTLS_SELF_TEST

    sys_task_delete(NULL);
}

#endif

void cmd_ssl_selftest(int argc, char **argv)
{
    if (sys_task_create_dynamic((const uint8_t *)"ssl_selftest", SSL_CLIENT_TASK_STK_SIZE, SSL_CLIENT_TASK_PRIO,
        (task_func_t)ssl_self_test, NULL) == NULL) {
        mbedtls_printf("ERROR: Create ssl selftest task failed\r\n");
    }
}
