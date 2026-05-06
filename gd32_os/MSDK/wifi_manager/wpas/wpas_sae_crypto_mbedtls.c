/*!
    \file    wpas_sae_crypto_mbedtls.c
    \brief   Wrapper functions for mbedtls.

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

#include "wpas_includes.h"
#include "mbedtls/version.h"

#if (MBEDTLS_VERSION_NUMBER == 0x02110000) //mbedtls v2.17.0
#include "rom_export_mbedtls.h"
#include "mbedtls/rom_config.h"
#else
#include "mbedtls/mbedtls_config.h"
#endif
#include "mbedtls/bignum.h"
#include "mbedtls/ecp.h"
#include "mbedtls/debug.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/base64.h"
#include "wpas_eap_tls_internal.h"
#include "wpas_sae_crypto.h"

/**
 * struct crypto_ec_key - Elliptic Curve Key pair
 *
 * Internal data structure for EC Key pair. The contents is specific to the used
 * crypto library.
 */
struct crypto_ec_key
{
    mbedtls_pk_context pk;
};

/**
 * struct crypto_ecdh - Elliptic Curve Diffie–Hellman context
 *
 * Internal data structure for ECDH. The contents is specific to the used
 * crypto library.
 */
struct crypto_ecdh
{
    struct crypto_ec_key *key;
    bool ephemeral_key;
};

struct crypto_bignum {
    mbedtls_mpi mpi;
};

static int mbedtls_rand(void *rng_state, unsigned char *output, size_t len )
{
    return sys_random_bytes_get(output, len);
}

/**
 * crypto_bignum_init - Allocate memory for bignum
 * Returns: Pointer to allocated bignum or %NULL on failure
 */
struct crypto_bignum *crypto_bignum_init(void)
{
    struct crypto_bignum *n;

    n = (struct crypto_bignum *)sys_zalloc(sizeof(*n));
    if (!n)
        return NULL;

    mbedtls_mpi_init(&n->mpi);

    return n;
}

/**
 * crypto_bignum_init_set - Allocate memory for bignum and set the value
 * @buf: Buffer with unsigned binary value
 * @len: Length of buf in octets
 * Returns: Pointer to allocated bignum or %NULL on failure
 */
struct crypto_bignum *crypto_bignum_init_set(const uint8_t *buf, size_t len)
{
    struct crypto_bignum *n = crypto_bignum_init();
    if (!n)
        return NULL;

    if (mbedtls_mpi_read_binary(&n->mpi, buf, len) != MP_OKAY) {
        crypto_bignum_deinit(n, 0);
        return NULL;
    }

    return n;
}

/**
 * crypto_bignum_init_set - Allocate memory for bignum and set the value (uint)
 * @val: Value to set
 * Returns: Pointer to allocated bignum or %NULL on failure
 */
struct crypto_bignum * crypto_bignum_init_uint(uint32_t val)
{
    struct crypto_bignum *n = crypto_bignum_init();
    if (!n)
        return NULL;

    // use mpi_mul_int as mpi_lset only take singed int as parameter
    if (mbedtls_mpi_lset(&n->mpi, 1) ||
            mbedtls_mpi_mul_int(&n->mpi, &n->mpi, val)) {
            crypto_bignum_deinit(n, 0);
        return NULL;
    }

    return n;
}

/**
 * crypto_bignum_deinit - Free bignum
 * @n: Bignum from crypto_bignum_init() or crypto_bignum_init_set()
 * @clear: Whether to clear the value from memory
 */
void crypto_bignum_deinit(struct crypto_bignum *n, int clear)
{
    // mbedtls always clear the memory
    mbedtls_mpi_free(&n->mpi);
    sys_mfree(n);
}

/**
 * crypto_bignum_to_bin - Set binary buffer to unsigned bignum
 * @a: Bignum
 * @buf: Buffer for the binary number
 * @len: Length of @buf in octets
 * @padlen: Length in octets to pad the result to or 0 to indicate no padding
 * Returns: Number of octets written on success, -1 on failure
 */
int crypto_bignum_to_bin(const struct crypto_bignum *a,
            uint8_t *buf, size_t buflen, size_t padlen)
{
    int res;
    size_t len = mbedtls_mpi_size(&a->mpi);

    if (len > buflen)
        return -1;

    // mbedtls will always do padding to the size of the buffer.
    if (padlen <= len)
        res = buflen = len;
    else
        res = buflen = padlen;

    if (mbedtls_mpi_write_binary(&a->mpi, buf, buflen))
        return -1;

    return res;
}

/**
 * crypto_bignum_rand - Create a random number in range of modulus
 * @r: Bignum; set to a random value
 * @m: Bignum; modulus
 * Returns: 0 on success, -1 on failure
 */
int crypto_bignum_rand(struct crypto_bignum *r, const struct crypto_bignum *m)
{
    int size = mbedtls_mpi_size(&m->mpi) + 1;
    void *buf = sys_malloc(size);
    int ret = -1;

    if (!buf)
        return -1;

    // As a first step takes the easy option, probably need something more
    // complete using mbedtls_mpi_fill_random
    if (sys_random_bytes_get(buf, size))
        goto end;

    if (mbedtls_mpi_read_binary(&r->mpi, buf, size))
        goto end;

    if (mbedtls_mpi_mod_mpi(&r->mpi, &r->mpi, &m->mpi))
        goto end;

    ret = 0;
end:
    sys_mfree(buf);
    return ret;
}

/**
 * crypto_bignum_add - c = a + b
 * @a: Bignum
 * @b: Bignum
 * @c: Bignum; used to store the result of a + b
 * Returns: 0 on success, -1 on failure
 */
int crypto_bignum_add(const struct crypto_bignum *a,
            const struct crypto_bignum *b,
            struct crypto_bignum *c)
{
    if (mbedtls_mpi_add_mpi(&c->mpi, &a->mpi, &b->mpi) != MP_OKAY)
        return -1;

    return 0;
}

/**
 * crypto_bignum_mod - c = a % b
 * @a: Bignum
 * @b: Bignum
 * @c: Bignum; used to store the result of a % b
 * Returns: 0 on success, -1 on failure
 */
int crypto_bignum_mod(const struct crypto_bignum *a,
            const struct crypto_bignum *b,
            struct crypto_bignum *c)
{
    if (mbedtls_mpi_mod_mpi(&c->mpi, &a->mpi, &b->mpi) != MP_OKAY)
        return -1;

    return 0;
}

/**
 * crypto_bignum_exptmod - Modular exponentiation: d = a^b (mod c)
 * @a: Bignum; base
 * @b: Bignum; exponent
 * @c: Bignum; modulus
 * @d: Bignum; used to store the result of a^b (mod c)
 * Returns: 0 on success, -1 on failure
 */
int crypto_bignum_exptmod(const struct crypto_bignum *a,
            const struct crypto_bignum *b,
            const struct crypto_bignum *c,
            struct crypto_bignum *d)
{
    // Only a odd modulus supported but should be ok as it is always called
    // with prime number as modulus
    if (mbedtls_mpi_exp_mod(&d->mpi, &a->mpi, &b->mpi, &c->mpi, NULL) != MP_OKAY)
        return -1;

    return 0;
}

/**
 * crypto_bignum_inverse - Inverse a bignum so that a * c = 1 (mod b)
 * @a: Bignum
 * @b: Bignum
 * @c: Bignum; used to store the result
 * Returns: 0 on success, -1 on failure
 */
int crypto_bignum_inverse(const struct crypto_bignum *a,
            const struct crypto_bignum *b,
            struct crypto_bignum *c)
{
    if (mbedtls_mpi_inv_mod(&c->mpi, &a->mpi, &b->mpi) != MP_OKAY)
        return -1;

    return 0;
}

/**
 * crypto_bignum_sub - c = a - b
 * @a: Bignum
 * @b: Bignum
 * @c: Bignum; used to store the result of a - b
 * Returns: 0 on success, -1 on failure
 */
int crypto_bignum_sub(const struct crypto_bignum *a,
            const struct crypto_bignum *b,
            struct crypto_bignum *c)
{
    if (mbedtls_mpi_sub_mpi(&c->mpi, &a->mpi, &b->mpi) != MP_OKAY)
        return -1;

    return 0;
}

/**
 * crypto_bignum_div - c = a / b
 * @a: Bignum
 * @b: Bignum
 * @c: Bignum; used to store the result of a / b
 * Returns: 0 on success, -1 on failure
 */
int crypto_bignum_div(const struct crypto_bignum *a,
            const struct crypto_bignum *b,
            struct crypto_bignum *c)
{
    if (mbedtls_mpi_div_mpi(&c->mpi, NULL, &a->mpi, &b->mpi) != MP_OKAY)
        return -1;

    return 0;
}

/**
 * crypto_bignum_addmod - d = a + b (mod c)
 * @a: Bignum
 * @b: Bignum
 * @c: Bignum
 * @d: Bignum; used to store the result of (a + b) % c
 * Returns: 0 on success, -1 on failure
 */
int crypto_bignum_addmod(const struct crypto_bignum *a,
            const struct crypto_bignum *b,
            const struct crypto_bignum *c,
            struct crypto_bignum *d)
{
    if (mbedtls_mpi_add_mpi(&d->mpi, &a->mpi, &b->mpi))
        return -1;

    if (mbedtls_mpi_mod_mpi(&d->mpi, &d->mpi, &c->mpi))
        return -1;

    return 0;
}

/**
 * crypto_bignum_mulmod - d = a * b (mod c)
 * @a: Bignum
 * @b: Bignum
 * @c: Bignum
 * @d: Bignum; used to store the result of (a * b) % c
 * Returns: 0 on success, -1 on failure
 */
int crypto_bignum_mulmod(const struct crypto_bignum *a,
                const struct crypto_bignum *b,
                const struct crypto_bignum *c,
                struct crypto_bignum *d)
{
#ifdef OLD_BIGNUM_ALGO
    int res;
    struct crypto_bignum t;

    mbedtls_mpi_init(&t.mpi);
    res = mbedtls_mpi_grow(&t.mpi, c->mpi.n);
    if (res != MP_OKAY) {
        return res;
    }

    res = mbedtls_mpi_mul_mpi(&t.mpi, &a->mpi, &b->mpi);
    if (res == MP_OKAY) {
        res = mbedtls_mpi_mod_mpi(&d->mpi, &t.mpi, &c->mpi);
    }

    mbedtls_mpi_free(&t.mpi);

    return res == MP_OKAY ? 0 : -1;
#else
    if (mbedtls_mpi_mul_mpi(&d->mpi, &a->mpi, &b->mpi))
        return -1;

    if (mbedtls_mpi_mod_mpi(&d->mpi, &d->mpi, &c->mpi))
        return -1;

    return 0;
#endif
}

/**
 * crypto_bignum_sqrmod - c = a^2 (mod b)
 * @a: Bignum
 * @b: Bignum
 * @c: Bignum; used to store the result of a^2 % b
 * Returns: 0 on success, -1 on failure
 */
int crypto_bignum_sqrmod(const struct crypto_bignum *a,
                const struct crypto_bignum *b,
                struct crypto_bignum *c)
{
    if (mbedtls_mpi_mul_mpi(&c->mpi, &a->mpi, &a->mpi))
        return -1;

    if (mbedtls_mpi_mod_mpi(&c->mpi, &c->mpi, &b->mpi))
        return -1;

    return 0;
}

/**
 * crypto_bignum_rshift - r = a >> n
 * @a: Bignum
 * @n: Number of bits
 * @r: Bignum; used to store the result of a >> n
 * Returns: 0 on success, -1 on failure
 */
int crypto_bignum_rshift(const struct crypto_bignum *a, int n,
                struct crypto_bignum *r)
{
    if ((a != r) &&
            mbedtls_mpi_copy(&r->mpi, &a->mpi) != MP_OKAY)
        return -1 ;

    if (mbedtls_mpi_shift_r(&r->mpi, n))
        return -1;

    return 0;
}

/**
 * crypto_bignum_cmp - Compare two bignums
 * @a: Bignum
 * @b: Bignum
 * Returns: -1 if a < b, 0 if a == b, or 1 if a > b
 */
int crypto_bignum_cmp(const struct crypto_bignum *a,
            const struct crypto_bignum *b)
{
    return mbedtls_mpi_cmp_mpi(&a->mpi, &b->mpi);
}

/**
 * crypto_bignum_bits - Get size of a bignum in bits
 * @a: Bignum
 * Returns: Number of bits in the bignum
 */
int crypto_bignum_bits(const struct crypto_bignum *a)
{
    return mbedtls_mpi_bitlen(&a->mpi);
}

/**
 * crypto_bignum_is_zero - Is the given bignum zero
 * @a: Bignum
 * Returns: 1 if @a is zero or 0 if not
 */
int crypto_bignum_is_zero(const struct crypto_bignum *a)
{
    if (mbedtls_mpi_cmp_int(&a->mpi, 0))
        return 0;
    return 1;
}

/**
 * crypto_bignum_is_one - Is the given bignum one
 * @a: Bignum
 * Returns: 1 if @a is one or 0 if not
 */
int crypto_bignum_is_one(const struct crypto_bignum *a)
{
    if (mbedtls_mpi_cmp_int(&a->mpi, 1))
        return 0;
    return 1;
}

#if (MBEDTLS_VERSION_NUMBER == 0x02110000) //mbedtls v2.17.0

/**
 * crypto_bignum_is_odd - Is the given bignum odd
 * @a: Bignum
 * Returns: 1 if @a is odd or 0 if not
 */
int crypto_bignum_is_odd(const mbedtls_mpi *a)
{
    if (a->p)
        return (a->p[0] & 0x1);
    return 0;
}
#else

/**
 * crypto_bignum_is_odd - Is the given bignum odd
 * @a: Bignum
 * Returns: 1 if @a is odd or 0 if not
 */
int crypto_bignum_is_odd(const struct crypto_bignum *a)
{
    if (a->mpi.p)
        return (a->mpi.p[0] & 0x1);
    return 0;
}
#endif

/**
 * crypto_bignum_legendre - Compute the Legendre symbol (a/p)
 * @a: Bignum
 * @p: Bignum
 * Returns: Legendre symbol -1,0,1 on success; -2 on calculation failure
 */
int crypto_bignum_legendre(const struct crypto_bignum *a,
                const struct crypto_bignum *p)
{
#ifdef OLD_BIGNUM_ALGO
    struct crypto_bignum t;
    int ret;
    int res = -2;

    mbedtls_mpi_init(&t);

    /* t = (p-1) / 2 */
    ret = mbedtls_mpi_sub_int(&t.mpi, &p->mpi, 1);
    if (ret == MP_OKAY)
        ret = mbedtls_mpi_shift_r(&t.mpi, 1);
    if (ret == MP_OKAY)
        ret = mbedtls_mpi_exp_mod(&t.mpi, &a->mpi, &t.mpi, &p->mpi, NULL);
    if (ret == MP_OKAY) {
        if (mp_isone(&t.mpi))
            res = 1;
        else if (mp_iszero(&t.mpi))
            res = 0;
        else
            res = -1;
    }

    crypto_bignum_deinit(&t);
    return res;
#else
    mbedtls_mpi exp, tmp;
    int res = -2;

    mbedtls_mpi_init(&exp);
    mbedtls_mpi_init(&tmp);

    // exp = (p-1) / 2
    if (mbedtls_mpi_sub_int(&exp, &p->mpi, 1) ||
            mbedtls_mpi_shift_r(&exp, 1))
        goto end;

    if (mbedtls_mpi_exp_mod(&tmp, &a->mpi, &exp, &p->mpi, NULL))
        goto end;

    if (mbedtls_mpi_cmp_int(&tmp, 1) == 0)
        res = 1;
    else if (mbedtls_mpi_cmp_int(&tmp, 0) == 0)
        res = 0;
    else
        res = -1;
end:
    mbedtls_mpi_free(&exp);
    mbedtls_mpi_free(&tmp);
    return res;
#endif
}

//#define MBEDTLS_MPI_CHK(f) do { if( ( ret = f ) != 0 ) goto cleanup; } while( 0 )
/*
 * Reduce a mbedtls_mpi mod p in-place, general case, to use after mbedtls_mpi_mul_mpi
 */
#define MOD_MUL( N )    do { MBEDTLS_MPI_CHK( ecp_modp( &N, grp ) ); } \
                        while( 0 )

/*
 * Reduce a mbedtls_mpi mod p in-place, to use after mbedtls_mpi_sub_mpi
 * N->s < 0 is a very fast test, which fails only if N is 0
 */
#define MOD_SUB( N )                                \
    while( N.s < 0 && mbedtls_mpi_cmp_int( &N, 0 ) != 0 )   \
        MBEDTLS_MPI_CHK( mbedtls_mpi_add_mpi( &N, &N, &grp->P ) )

/*
 * Reduce a mbedtls_mpi mod p in-place, to use after mbedtls_mpi_add_mpi and mbedtls_mpi_mul_int.
 * We known P, N and the result are positive, so sub_abs is correct, and
 * a bit faster.
 */
#define MOD_ADD( N )                                \
    while( mbedtls_mpi_cmp_mpi( &N, &grp->P ) >= 0 )        \
        MBEDTLS_MPI_CHK( mbedtls_mpi_sub_abs( &N, &N, &grp->P ) )

//#ifdef OLD_BIGNUM_ALGO
static int crypto_rand(void * p_rng, unsigned char *buf, size_t sz)
{
    return sys_random_bytes_get(buf, sz);
}
//#endif
#if 0
static void mpi_montg_init( mbedtls_mpi_uint *mm, const mbedtls_mpi *N )
{
    mbedtls_mpi_uint x, m0 = N->p[0];
    uint32_t i;

    x  = m0;
    x += ( ( m0 + 2 ) & 4 ) << 1;

    for( i = biL; i >= 8; i /= 2 )
        x *= ( 2 - ( m0 * x ) );

    *mm = ~x + 1;
}
#endif

#ifdef OLD_BIGNUM_ALGO
/* shift right by a certain bit count (store quotient in c, optional remainder in d) */
static int mp_div_2d (const mbedtls_mpi *a, int b, mbedtls_mpi *c)
{
    mbedtls_mpi_uint D, r, rr;
    int     x, res;
    mbedtls_mpi  t;
    int limb_bits = (sizeof(mbedtls_mpi_uint) << 3);

    /* if the shift count is <= 0 then we do no work */
    if (b <= 0) {
        res = mbedtls_mpi_copy(c, a);
        return res;
    }

    mbedtls_mpi_init(&t);

    /* copy */
    if ((res = mbedtls_mpi_copy(c, a)) != 0) {
        mbedtls_mpi_free(&t);
        return res;
    }

    /* shift by as many digits in the bit count */
    if (b >= limb_bits) {
        mbedtls_mpi_shift_r(c, b / limb_bits);
    }

    /* shift any bit count < DIGIT_BIT */
    D = (mbedtls_mpi_uint) (b & (limb_bits - 1));
    if (D != 0) {
        mbedtls_mpi_uint *tmpc, mask, shift;
        size_t used = (mbedtls_mpi_size(c) >> 2);

        /* mask */
        mask = (((mbedtls_mpi_uint)1) << D) - 1;

        /* shift for lsb */
        shift = limb_bits - D;

        /* alias */
        tmpc = c->p + (used - 1);

        /* carry */
        r = 0;
        for (x = used - 1; x >= 0; x--) {
            /* get the lower  bits of this word in a temp */
            rr = *tmpc & mask;

            /* shift the current word and mix in the carry bits from the previous word */
            *tmpc = (*tmpc >> D) | (r << shift);
            --tmpc;

            /* set the carry to the carry bits of the current word found above */
            r = rr;
        }
    }

    mbedtls_mpi_free(&t);
    return 0;
}

/* Y = sqrt(YY) mod P */
static int mp_sqrtmod_prime(mbedtls_mpi *Y, mbedtls_mpi *YY, mbedtls_mpi *P)
{
    int ret;
    mbedtls_mpi E;

    mbedtls_mpi_init(&E);

    ret = mbedtls_mpi_add_int(&E, P, 1);
    if (ret < 0)
        goto cleanup;

    ret = mp_div_2d(&E, 2, &E);
    if (ret < 0)
        goto cleanup;


    ret = mbedtls_mpi_exp_mod(Y, YY, &E, P, NULL);
    if (ret < 0)
        goto cleanup;

cleanup:
    mbedtls_mpi_free(&E);
    return ret;
}

/*
 * Wrapper around fast quasi-modp functions, with fall-back to mbedtls_mpi_mod_mpi.
 * See the documentation of struct mbedtls_ecp_group.
 *
 * This function is in the critial loop for mbedtls_ecp_mul, so pay attention to perf.
 */
static int ecp_modp(mbedtls_mpi *N, const mbedtls_ecp_group *grp)
{
    int ret;

    if (grp->modp == NULL)
        return (mbedtls_mpi_mod_mpi(N, N, &grp->P));

    /* N->s < 0 is a much faster test, which fails only if N is 0 */
    if ((N->s < 0 && mbedtls_mpi_cmp_int(N, 0) != 0) ||
        mbedtls_mpi_bitlen(N) > 2 * grp->pbits) {
        return( MBEDTLS_ERR_ECP_BAD_INPUT_DATA );
    }

    MBEDTLS_MPI_CHK(grp->modp(N));

    /* N->s < 0 is a much faster test, which fails only if N is 0 */
    while (N->s < 0 && mbedtls_mpi_cmp_int(N, 0) != 0)
        MBEDTLS_MPI_CHK(mbedtls_mpi_add_mpi(N, N, &grp->P));

    while (mbedtls_mpi_cmp_mpi(N, &grp->P) >= 0)
        /* we known P, N and the result are positive */
        MBEDTLS_MPI_CHK(mbedtls_mpi_sub_abs(N, N, &grp->P));

cleanup:
    return (ret);
}

/*
 * Point doubling R = 2 P, Jacobian coordinates
 *
 * Based on http://www.hyperelliptic.org/EFD/g1p/auto-shortw-jacobian.html#doubling-dbl-1998-cmo-2 .
 *
 * We follow the variable naming fairly closely. The formula variations that trade a MUL for a SQR
 * (plus a few ADDs) aren't useful as our bignum implementation doesn't distinguish squaring.
 *
 * Standard optimizations are applied when curve parameter A is one of { 0, -3 }.
 *
 * Cost: 1D := 3M + 4S          (A ==  0)
 *             4M + 4S          (A == -3)
 *             3M + 6S + 1a     otherwise
 */
static int ecp_double_jac(const mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
                           const mbedtls_ecp_point *P)
{
    int ret;
    mbedtls_mpi M, S, T, U;

#if defined(MBEDTLS_ECP_DOUBLE_JAC_ALT)
    if (mbedtls_internal_ecp_grp_capable(grp))
        return mbedtls_internal_ecp_double_jac(grp, R, P);
#endif /* MBEDTLS_ECP_DOUBLE_JAC_ALT */

    mbedtls_mpi_init(&M); mbedtls_mpi_init(&S); mbedtls_mpi_init(&T); mbedtls_mpi_init(&U);

    /* Special case for A = -3 */
    if (grp->A.p == NULL) {
        /* M = 3(X + Z^2)(X - Z^2) */
        MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&S, &P->Z, &P->Z)); MOD_MUL(S);
        MBEDTLS_MPI_CHK(mbedtls_mpi_add_mpi(&T, &P->X, &S)); MOD_ADD(T);
        MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(&U, &P->X, &S)); MOD_SUB(U);
        MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&S, &T, &U)); MOD_MUL(S);
        MBEDTLS_MPI_CHK(mbedtls_mpi_mul_int(&M, &S, 3)); MOD_ADD(M);
    } else {
        /* M = 3.X^2 */
        MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&S, &P->X, &P->X)); MOD_MUL(S);
        MBEDTLS_MPI_CHK(mbedtls_mpi_mul_int(&M, &S, 3)); MOD_ADD(M);

        /* Optimize away for "koblitz" curves with A = 0 */
        if (mbedtls_mpi_cmp_int(&grp->A, 0) != 0) {
            /* M += A.Z^4 */
            MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&S, &P->Z,&P->Z)); MOD_MUL(S);
            MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&T, &S, &S)); MOD_MUL(T);
            MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&S, &T, &grp->A)); MOD_MUL(S);
            MBEDTLS_MPI_CHK(mbedtls_mpi_add_mpi(&M, &M, &S)); MOD_ADD(M);
        }
    }

    /* S = 4.X.Y^2 */
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&T, &P->Y, &P->Y)); MOD_MUL(T);
    MBEDTLS_MPI_CHK(mbedtls_mpi_shift_l(&T, 1)); MOD_ADD(T);
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&S, &P->X, &T)); MOD_MUL(S);
    MBEDTLS_MPI_CHK(mbedtls_mpi_shift_l(&S, 1)); MOD_ADD(S);

    /* U = 8.Y^4 */
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&U, &T, &T)); MOD_MUL(U);
    MBEDTLS_MPI_CHK(mbedtls_mpi_shift_l(&U, 1)); MOD_ADD(U);

    /* T = M^2 - 2.S */
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&T, &M, &M)); MOD_MUL(T);
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(&T, &T, &S)); MOD_SUB(T);
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(&T, &T, &S)); MOD_SUB(T);

    /* S = M(S - T) - U */
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(&S, &S, &T)); MOD_SUB(S);
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&S, &S, &M)); MOD_MUL(S);
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(&S, &S, &U)); MOD_SUB(S);

    /* U = 2.Y.Z */
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&U, &P->Y, &P->Z)); MOD_MUL(U);
    MBEDTLS_MPI_CHK(mbedtls_mpi_shift_l(&U, 1)); MOD_ADD(U);

    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&R->X, &T));
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&R->Y, &S));
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&R->Z, &U));

cleanup:
    mbedtls_mpi_free(&M); mbedtls_mpi_free(&S); mbedtls_mpi_free(&T); mbedtls_mpi_free(&U);

    return (ret);
}

/*
 * Normalize jacobian coordinates so that Z == 0 || Z == 1  (GECC 3.2.1)
 * Cost: 1N := 1I + 3M + 1S
 */
static int ecp_normalize_jac(const mbedtls_ecp_group *grp, mbedtls_ecp_point *pt)
{
    int ret;
    mbedtls_mpi Zi, ZZi;

    if (mbedtls_mpi_cmp_int(&pt->Z, 0) == 0)
        return (0);

#if defined(MBEDTLS_ECP_NORMALIZE_JAC_ALT)
    if (mbedtls_internal_ecp_grp_capable(grp))
        return (mbedtls_internal_ecp_normalize_jac(grp, pt));
#endif /* MBEDTLS_ECP_NORMALIZE_JAC_ALT */

    mbedtls_mpi_init(&Zi); mbedtls_mpi_init(&ZZi);

    /*
     * X = X / Z^2  mod p
     */
    MBEDTLS_MPI_CHK(mbedtls_mpi_inv_mod(&Zi, &pt->Z, &grp->P));
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&ZZi, &Zi, &Zi)); MOD_MUL(ZZi);
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&pt->X, &pt->X, &ZZi)); MOD_MUL(pt->X);

    /*
     * Y = Y / Z^3  mod p
     */
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&pt->Y, &pt->Y, &ZZi)); MOD_MUL(pt->Y);
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&pt->Y, &pt->Y, &Zi)); MOD_MUL(pt->Y);

    /*
     * Z = 1
     */
    MBEDTLS_MPI_CHK(mbedtls_mpi_lset(&pt->Z, 1));

cleanup:
    mbedtls_mpi_free(&Zi); mbedtls_mpi_free(&ZZi);
    return (ret);
}
/*
 * Addition: R = P + Q, mixed affine-Jacobian coordinates (GECC 3.22)
 *
 * The coordinates of Q must be normalized (= affine),
 * but those of P don't need to. R is not normalized.
 *
 * Special cases: (1) P or Q is zero, (2) R is zero, (3) P == Q.
 * None of these cases can happen as intermediate step in ecp_mul_comb():
 * - at each step, P, Q and R are multiples of the base point, the factor
 *   being less than its order, so none of them is zero;
 * - Q is an odd multiple of the base point, P an even multiple,
 *   due to the choice of precomputed points in the modified comb method.
 * So branches for these cases do not leak secret information.
 *
 * We accept Q->Z being unset (saving memory in tables) as meaning 1.
 *
 * Cost: 1A := 8M + 3S
 */
static int ecp_add_mixed(const mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
                          const mbedtls_ecp_point *P, const mbedtls_ecp_point *Q)
{
    int ret;
    mbedtls_mpi T1, T2, T3, T4, X, Y, Z;

#if defined(MBEDTLS_SELF_TEST)
    add_count++;
#endif

#if defined(MBEDTLS_ECP_ADD_MIXED_ALT)
    if (mbedtls_internal_ecp_grp_capable(grp))
        return (mbedtls_internal_ecp_add_mixed(grp, R, P, Q));
#endif /* MBEDTLS_ECP_ADD_MIXED_ALT */

    /*
     * Trivial cases: P == 0 or Q == 0 (case 1)
     */
    if (mbedtls_mpi_cmp_int(&P->Z, 0) == 0)
        return (mbedtls_ecp_copy(R, Q));

    if (Q->Z.p != NULL && mbedtls_mpi_cmp_int(&Q->Z, 0) == 0)
        return (mbedtls_ecp_copy(R, P));

    /*
     * Make sure Q coordinates are normalized
     */
    if (Q->Z.p != NULL && mbedtls_mpi_cmp_int(&Q->Z, 1) != 0)
        return (MBEDTLS_ERR_ECP_BAD_INPUT_DATA);

    mbedtls_mpi_init(&T1); mbedtls_mpi_init(&T2); mbedtls_mpi_init(&T3); mbedtls_mpi_init(&T4);
    mbedtls_mpi_init(&X); mbedtls_mpi_init(&Y); mbedtls_mpi_init(&Z);

    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&T1, &P->Z, &P->Z)); MOD_MUL(T1);
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&T2, &T1, &P->Z)); MOD_MUL(T2);
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&T1, &T1, &Q->X)); MOD_MUL(T1);
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&T2, &T2, &Q->Y)); MOD_MUL(T2);
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(&T1, &T1, &P->X)); MOD_SUB(T1);
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(&T2, &T2, &P->Y)); MOD_SUB(T2);

    /* Special cases (2) and (3) */
    if (mbedtls_mpi_cmp_int(&T1, 0) == 0) {
        if (mbedtls_mpi_cmp_int( &T2, 0) == 0) {
            ret = ecp_double_jac(grp, R, P);
            goto cleanup;
        } else {
            ret = mbedtls_ecp_set_zero(R);
            goto cleanup;
        }
    }

    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&Z, &P->Z, &T1)); MOD_MUL(Z);
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&T3, &T1, &T1)); MOD_MUL(T3);
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&T4, &T3, &T1)); MOD_MUL(T4);
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&T3, &T3, &P->X)); MOD_MUL(T3);
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_int(&T1, &T3, 2)); MOD_ADD(T1);
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&X, &T2, &T2)); MOD_MUL(X);
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(&X, &X, &T1)); MOD_SUB(X);
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(&X, &X, &T4)); MOD_SUB(X);
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(&T3, &T3, &X)); MOD_SUB(T3);
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&T3, &T3, &T2)); MOD_MUL(T3);
    MBEDTLS_MPI_CHK(mbedtls_mpi_mul_mpi(&T4, &T4, &P->Y)); MOD_MUL(T4);
    MBEDTLS_MPI_CHK(mbedtls_mpi_sub_mpi(&Y, &T3, &T4)); MOD_SUB(Y);

    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&R->X, &X));
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&R->Y, &Y));
    MBEDTLS_MPI_CHK(mbedtls_mpi_copy(&R->Z, &Z));

cleanup:
    mbedtls_mpi_free(&T1); mbedtls_mpi_free(&T2); mbedtls_mpi_free(&T3); mbedtls_mpi_free(&T4);
    mbedtls_mpi_free(&X); mbedtls_mpi_free(&Y); mbedtls_mpi_free(&Z);

    return (ret);
}
#endif

/**
 * struct crypto_ec - Elliptic curve context
 *
 * Internal data structure for EC implementation. The contents is specific
 * to the used crypto library.
 */
struct crypto_ec {
    mbedtls_ecp_group group;
};

/* Map from IANA registry for IKE D-H groups to Mbed TLS group ID */
static mbedtls_ecp_group_id mbedtls_get_group_id(int group)
{
    switch (group) {
    case 19:
        return MBEDTLS_ECP_DP_SECP256R1;
    case 20:
        return MBEDTLS_ECP_DP_SECP384R1;
    case 21:
        return MBEDTLS_ECP_DP_SECP521R1;
    case 25:
        return MBEDTLS_ECP_DP_SECP192R1;
    case 26:
        // mbedtls support this curve (MBEDTLS_ECP_DP_SECP224R1) but since the prime
        // of this curve is not 3 congruent module 4 the square root algo used in
        // crypto_ec_point_solve_y_coord is not correct.
        return MBEDTLS_ECP_DP_NONE;
    case 28:
        return MBEDTLS_ECP_DP_BP256R1;
    case 29:
        return MBEDTLS_ECP_DP_BP384R1;
    case 30:
        return MBEDTLS_ECP_DP_BP512R1;
    default:
        return MBEDTLS_ECP_DP_NONE;
    }
}

/**
 * crypto_ec_init - Initialize elliptic curve context
 * @group: Identifying number for the ECC group (IANA "Group Description"
 *	attribute registrty for RFC 2409)
 * Returns: Pointer to EC context or %NULL on failure
 */
struct crypto_ec *crypto_ec_init(int group)
{
    struct crypto_ec *ec = NULL;
    mbedtls_ecp_group_id grp_id;

    grp_id = mbedtls_get_group_id(group);
    ec = (struct crypto_ec *)sys_zalloc(sizeof(*ec));
    if (ec == NULL)
        return NULL;

    mbedtls_ecp_group_init(&ec->group);
    if (mbedtls_ecp_group_load(&ec->group, grp_id)) {
        crypto_ec_deinit(ec);
        return NULL;
    }

    return ec;
}

/**
 * crypto_ec_deinit - Deinitialize elliptic curve context
 * @e: EC context from crypto_ec_init()
 */
void crypto_ec_deinit(struct crypto_ec *e)
{
    mbedtls_ecp_group_free(&e->group);
    sys_mfree(e);
}

/**
 * crypto_ec_prime_len - Get length of the prime in octets
 * @e: EC context from crypto_ec_init()
 * Returns: Length of the prime defining the group
 */
size_t crypto_ec_prime_len(struct crypto_ec *e)
{
    return mbedtls_mpi_size(&e->group.P);
}

/**
 * crypto_ec_prime_len_bits - Get length of the prime in bits
 * @e: EC context from crypto_ec_init()
 * Returns: Length of the prime defining the group in bits
 */
size_t crypto_ec_prime_len_bits(struct crypto_ec *e)
{
    return mbedtls_mpi_bitlen(&e->group.P);
}

/**
 * crypto_ec_order_len - Get length of the order in octets
 * @e: EC context from crypto_ec_init()
 * Returns: Length of the order defining the group
 */
size_t crypto_ec_order_len(struct crypto_ec *e)
{
    return mbedtls_mpi_size(&e->group.N);
}

/**
 * crypto_ec_get_prime - Get prime defining an EC group
 * @e: EC context from crypto_ec_init()
 * Returns: Prime (bignum) defining the group
 */
const struct crypto_bignum * crypto_ec_get_prime(struct crypto_ec *e)
{
    return (const struct crypto_bignum *)&e->group.P;
}

/**
 * crypto_ec_get_order - Get order of an EC group
 * @e: EC context from crypto_ec_init()
 * Returns: Order (bignum) of the group
 */
const struct crypto_bignum * crypto_ec_get_order(struct crypto_ec *e)
{
    return (const struct crypto_bignum *)&e->group.N;
}


/**
 * -3 in a bignum, to be used for NIST curve 'a' coefficient
 */
#if (MBEDTLS_VERSION_NUMBER == 0x02110000) //mbedtls v2.17.0
static mbedtls_mpi_uint minus_3_data[1] = {3};
static const struct crypto_bignum minus_3 = {
    .mpi = {
        .s = -1,
        .n = 1,
        .p = minus_3_data,
    },
};
#else
static mbedtls_mpi_uint minus_3_data[1] = {3};
static const struct crypto_bignum minus_3 = {
    .mpi = {
        .MBEDTLS_PRIVATE(p) = minus_3_data,
        .MBEDTLS_PRIVATE(s) = -1,
        .MBEDTLS_PRIVATE(n) = 1,
    }
};
#endif
/**
 * crypto_ec_get_a - Get 'a' value of an EC curve
 * @e: EC context from crypto_ec_init()
 * Returns: a (bignum) of the group
 */
const struct crypto_bignum * crypto_ec_get_a(struct crypto_ec *e)
{
    if (e->group.A.p)
        return (const struct crypto_bignum *)&e->group.A;
    else
        // For NIST curves mbedtls doesn't store the value
        // of 'a' in the group as it is always -3
        return (const struct crypto_bignum *)&minus_3;
}

/**
 * crypto_ec_get_b - Get 'b' value of an EC curve
 * @e: EC context from crypto_ec_init()
 * Returns: b (bignum) of the group
 */
const struct crypto_bignum * crypto_ec_get_b(struct crypto_ec *e)
{
    return (const struct crypto_bignum *)&e->group.B;
}

/**
 * crypto_ec_get_generator - Get generator point of the EC group's curve
 * @e: EC context from crypto_ec_init()
 * Returns: Pointer to Generator point
 */
const struct crypto_ec_point * crypto_ec_get_generator(struct crypto_ec *e)
{
    return (const struct crypto_ec_point *)&e->group.G;
}

/**
 * struct crypto_ec_point - Elliptic curve point
 *
 * Internal data structure for EC implementation to represent a point. The
 * contents is specific to the used crypto library.
 */
struct crypto_ec_point {
    mbedtls_ecp_point point;
};

/**
 * crypto_ec_point_init - Initialize data for an EC point
 * @e: EC context from crypto_ec_init()
 * Returns: Pointer to EC point data or %NULL on failure
 */
struct crypto_ec_point *crypto_ec_point_init(struct crypto_ec *e)
{
    struct crypto_ec_point *ecp;
    ecp = (struct crypto_ec_point *)sys_malloc(sizeof(*ecp));
    if (!ecp)
        return NULL;

    mbedtls_ecp_point_init(&ecp->point);
    return ecp;
}

/**
 * crypto_ec_point_deinit - Deinitialize EC point data
 * @p: EC point data from crypto_ec_point_init()
 * @clear: Whether to clear the EC point value from memory
 */
void crypto_ec_point_deinit(struct crypto_ec_point *p, int clear)
{
    // always clear memory
    mbedtls_ecp_point_free(&p->point);
    sys_mfree(p);
}

/**
 * crypto_ec_point_x - Copies the x-ordinate point into big number
 * @e: EC context from crypto_ec_init()
 * @p: EC point data
 * @x: Big number to set to the copy of x-ordinate
 * Returns: 0 on success, -1 on failure
 */
int crypto_ec_point_x(struct crypto_ec *e, const struct crypto_ec_point *p,
                struct crypto_bignum *x)
{
    if (mbedtls_mpi_copy(&x->mpi, &p->point.X))
        return -1;
    return 0;
}

/**
 * crypto_ec_point_to_bin - Write EC point value as binary data
 * @e: EC context from crypto_ec_init()
 * @p: EC point data from crypto_ec_point_init()
 * @x: Buffer for writing the binary data for x coordinate or %NULL if not used
 * @y: Buffer for writing the binary data for y coordinate or %NULL if not used
 * Returns: 0 on success, -1 on failure
 *
 * This function can be used to write an EC point as binary data in a format
 * that has the x and y coordinates in big endian byte order fields padded to
 * the length of the prime defining the group.
 */
int crypto_ec_point_to_bin(struct crypto_ec *e,
            const struct crypto_ec_point *p, uint8_t *x, uint8_t *y)
{
#ifdef OLD_BIGNUM_ALGO
    int len = mbedtls_mpi_size(&e->group.P);

    if (mbedtls_mpi_cmp_int(&p->point.Z, 1) != 0) {
        if (ecp_normalize_jac((const mbedtls_ecp_group *)&e->group, (mbedtls_ecp_point *)&p->point) < 0)
            return -1;
    }

    if (x) {
        if (crypto_bignum_to_bin(&p->point.X, x, len, len) <= 0)
            return -1;
    }

    if (y) {
        if (crypto_bignum_to_bin(&p->point.Y, y, len, len) <= 0)
            return -1;
    }
#else
    size_t p_len = crypto_ec_prime_len(e);

    if (x && mbedtls_mpi_write_binary(&p->point.X, x, p_len))
        return -1;

    if (y && mbedtls_mpi_write_binary(&p->point.Y, y, p_len))
        return -1;
#endif
    return 0;
}

/**
 * crypto_ec_point_from_bin - Create EC point from binary data
 * @e: EC context from crypto_ec_init()
 * @val: Binary data to read the EC point from
 * Returns: Pointer to EC point data or %NULL on failure
 *
 * This function readers x and y coordinates of the EC point from the provided
 * buffer assuming the values are in big endian byte order with fields padded to
 * the length of the prime defining the group.
 */
struct crypto_ec_point *crypto_ec_point_from_bin(struct crypto_ec *e,
                const uint8_t *val)
{
#ifdef OLD_BIGNUM_ALGO
    mbedtls_ecp_point *point = NULL;
    int len = mbedtls_mpi_size(&grp->P);
    int loaded = 0;

    point = crypto_ec_point_init(grp);
    if (!point)
        goto done;

    if (mbedtls_mpi_read_binary((mbedtls_mpi *)&point->X, val, len) < 0)
        goto done;
    val += len;
    if (mbedtls_mpi_read_binary((mbedtls_mpi *)&point->Y, val, len) < 0)
        goto done;
    mbedtls_mpi_lset((mbedtls_mpi *)&point->Z, 1);

    loaded = 1;
done:
    if (!loaded) {
        crypto_ec_point_deinit(point, 0);
        point = NULL;
    }
    return point;
#else
    size_t p_len = crypto_ec_prime_len(e);
    size_t tmp_len = 2 * p_len + 1;
    struct crypto_ec_point *ecp;
    uint8_t *tmp;

    ecp = crypto_ec_point_init(e);
    if (!ecp)
        return NULL;

    tmp = sys_zalloc(tmp_len);
    if (!tmp) {
        crypto_ec_point_deinit(ecp, 1);
        return NULL;
    }

    tmp[0] = 0x4; // UNCOMPRESSED
    memcpy(&tmp[1], val, 2 * p_len);

    if (mbedtls_ecp_point_read_binary(&e->group, &ecp->point, tmp, tmp_len)) {
        crypto_ec_point_deinit(ecp, 1);
        ecp = NULL;
    }

    sys_mfree(tmp);
    return ecp;
#endif
}

/**
 * crypto_bignum_add - c = a + b
 * @e: EC context from crypto_ec_init()
 * @a: Bignum
 * @b: Bignum
 * @c: Bignum; used to store the result of a + b
 * Returns: 0 on success, -1 on failure
 */
int crypto_ec_point_add(struct crypto_ec *e, const struct crypto_ec_point *a,
                const struct crypto_ec_point *b,
                struct crypto_ec_point *c)
{
#ifdef OLD_BIGNUM_ALGO
    int ret;

    if (mbedtls_mpi_cmp_int(&b->Z, 1) != 0) {
        ret = ecp_normalize_jac((const mbedtls_ecp_group *)grp, (mbedtls_ecp_point *)b);
        if (ret < 0)
            return -1;
    }

    ret = ecp_add_mixed(grp, c, a, b);
    if (ret < 0) {
        return -2;
    }

    if (mbedtls_mpi_cmp_int(&c->Z, 1) != 0) {
        ret = ecp_normalize_jac(grp, c);
        if (ret < 0)
            return -3;
    }

    return 0;
#else
    mbedtls_mpi one;
    int ret = -1;
    mbedtls_mpi_init(&one);

    if (mbedtls_mpi_lset(&one, 1))
        goto end;

    if (mbedtls_ecp_muladd(&e->group, &c->point, &one, &a->point,
                &one, &b->point))
        goto end;

    ret = 0;

end:
    mbedtls_mpi_free(&one);
    return ret;
#endif
}

/**
 * crypto_bignum_mul - res = b * p
 * @e: EC context from crypto_ec_init()
 * @p: EC point
 * @b: Bignum
 * @res: EC point; used to store the result of b * p
 * Returns: 0 on success, -1 on failure
 */
int crypto_ec_point_mul(struct crypto_ec *e, const struct crypto_ec_point *p,
            const struct crypto_bignum *b,
            struct crypto_ec_point *res)
{
#ifdef OLD_BIGNUM_ALGO
    return mbedtls_ecp_mul(grp, res, b, p, crypto_rand, NULL);
#else
    return mbedtls_ecp_mul(&e->group, &res->point, &b->mpi, &p->point,
            crypto_rand, NULL);
#endif
}


/**
 * crypto_ec_point_invert - Compute inverse of an EC point
 * @e: EC context from crypto_ec_init()
 * @p: EC point to invert (and result of the operation)
 * Returns: 0 on success, -1 on failure
 */
int crypto_ec_point_invert(struct crypto_ec *e, struct crypto_ec_point *p)
{
#ifdef OLD_BIGNUM_ALGO
    return mbedtls_mpi_sub_mpi(&p->Y, &grp->P, &p->Y);
#else
    mbedtls_mpi one;
    mbedtls_mpi minus_one;
    mbedtls_ecp_point zero;
    int ret = -1;

    mbedtls_mpi_init(&one);
    mbedtls_mpi_init(&minus_one);
    mbedtls_ecp_point_init(&zero);

    if (mbedtls_mpi_lset(&one, 1) ||
        mbedtls_mpi_lset(&minus_one, -1) ||
        mbedtls_ecp_set_zero(&zero))
        goto end;

    if (mbedtls_ecp_muladd(&e->group, &p->point, &one, &zero,
                   &minus_one, &p->point))
        goto end;

    ret = 0;

end:
    mbedtls_mpi_free(&one);
    mbedtls_mpi_free(&minus_one);
    mbedtls_ecp_point_free(&zero);
    return ret;
#endif
}

/**
 * crypto_ec_point_solve_y_coord - Solve y coordinate for an x coordinate
 * @e: EC context from crypto_ec_init()
 * @p: EC point to use for the returning the result
 * @x: x coordinate
 * @y_bit: y-bit (0 or 1) for selecting the y value to use
 * Returns: 0 on success, -1 on failure
 */
int crypto_ec_point_solve_y_coord(struct crypto_ec *e,
                    struct crypto_ec_point *p,
                    const struct crypto_bignum *x, int y_bit)
{
#ifdef OLD_BIGNUM_ALGO
    mbedtls_mpi *RHS;
    mbedtls_mpi Y;
    int pointType = y_bit ? ECC_POINT_COMP_ODD : ECC_POINT_COMP_EVEN;

    mbedtls_mpi_init(&Y);

    if (crypto_ec_point_x(grp, p, x) < 0)
        return -1;

    /* compute Y^2 = RHS = (X^3 + A X + B) */
    RHS = crypto_ec_point_compute_y_sqr(grp, x);
    if (RHS == NULL)
        goto cleanup;

    /* compute Y = sqrt(RHS) mod P */
    if (mp_sqrtmod_prime(&Y, RHS, &grp->P) < 0)
        goto cleanup;

    /* adjust Y */
    if ((mp_isodd(&Y) == 1 && pointType == ECC_POINT_COMP_ODD) ||
        (mp_isodd(&Y) == 0 && pointType == ECC_POINT_COMP_EVEN)) {
        if (mbedtls_mpi_mod_mpi(&p->Y, &Y, &grp->P) < 0)
            goto cleanup;
    } else {
        if (mbedtls_mpi_sub_mpi(&Y, &grp->P, &Y) < 0)
            goto cleanup;
        if (mbedtls_mpi_mod_mpi(&p->Y, &Y, &grp->P) < 0)
            goto cleanup;
    }

    mbedtls_mpi_lset(&(p->Z), 1);
    mbedtls_mpi_free(&Y);
    crypto_bignum_deinit(RHS, 1);

    return 0;

cleanup:
    mbedtls_mpi_free(&Y);
    if (RHS)
        crypto_bignum_deinit(RHS, 1);
    mbedtls_mpi_free(&(p->X));
    mbedtls_mpi_free(&(p->Y));

    return -1;
#else
    struct crypto_bignum *y_sqr;
    mbedtls_mpi exp;
    int ret = -1;

    mbedtls_mpi_init(&exp);

    y_sqr = crypto_ec_point_compute_y_sqr(e, x);
    if (!y_sqr)
        return -1;

    // If p = 3 % 4 (which is the case for all curves except the one for group 26)
    // then y = (y_sqr)^((p + 1) / 4)

    // exp = (p + 1) / 4
    if (mbedtls_mpi_add_int(&exp, &e->group.P, 1) ||
            mbedtls_mpi_shift_r(&exp, 2))
        goto end;

    if (mbedtls_mpi_exp_mod(&p->point.Y, &y_sqr->mpi, &exp, &e->group.P, NULL))
        goto end;

    if (((p->point.Y.p[0] & 0x1) != y_bit) &&
            mbedtls_mpi_sub_mpi(&p->point.Y, &e->group.P, &p->point.Y))
        goto end;

    if (mbedtls_mpi_copy(&p->point.X, &x->mpi) ||
            mbedtls_mpi_lset(&p->point.Z, 1))
        goto end;

    ret = 0;
end:
    mbedtls_mpi_free(&exp);
    crypto_bignum_deinit(y_sqr, 1);
    return ret;
#endif
}

/**
 * crypto_ec_point_compute_y_sqr - Compute y^2 = x^3 + ax + b
 * @e: EC context from crypto_ec_init()
 * @x: x coordinate
 * Returns: y^2 on success, %NULL failure
 */
struct crypto_bignum *
crypto_ec_point_compute_y_sqr(struct crypto_ec *e,
                    const struct crypto_bignum *x)
{
#ifdef OLD_BIGNUM_ALGO
    mbedtls_mpi *RHS;

    /*
     * Y^2 = RHS = X (X^2 + A) + B = X^3 + A X + B
     */
    RHS = crypto_bignum_init();

    /* RHS = X^2 */
    if (mbedtls_mpi_mul_mpi(RHS, x, x) < 0)
        goto cleanup;
    if (mbedtls_mpi_mod_mpi(RHS, RHS, &grp->P ) < 0)
        goto cleanup;

    /* Special case for A = -3 */
    /* RHS = (X^2 + A) */
    if (grp->A.p == NULL) {
        if (mbedtls_mpi_sub_int(RHS, RHS, 3) < 0)
            goto cleanup;
        while(RHS->s < 0 && mbedtls_mpi_cmp_int(RHS, 0) != 0 )
            if (mbedtls_mpi_add_mpi(RHS, RHS, &grp->P) != 0)
                goto cleanup;
    } else {
        if (mbedtls_mpi_add_mpi(RHS, RHS, &grp->A ) < 0)
            goto cleanup;
        while(mbedtls_mpi_cmp_mpi(RHS, &grp->P) >= 0)
            if (mbedtls_mpi_sub_abs(RHS, RHS, &grp->P) != 0)
                goto cleanup;
    }

    /* RHS = X (X^2 + A) */
    if (mbedtls_mpi_mul_mpi(RHS, RHS, x) < 0)
        goto cleanup;
    if (mbedtls_mpi_mod_mpi(RHS, RHS, &grp->P ) < 0)
        goto cleanup;
    /* RHS = X (X^2 + A) + B */
    if (mbedtls_mpi_add_mpi(RHS, RHS, &grp->B) < 0)
        goto cleanup;
    while(mbedtls_mpi_cmp_mpi(RHS, &grp->P) >= 0)
        if (mbedtls_mpi_sub_abs(RHS, RHS, &grp->P) != 0)
            goto cleanup;

    return RHS;
cleanup:
    crypto_bignum_deinit(RHS, 1);

    return NULL;
#else
    struct crypto_bignum *y_sqr = crypto_bignum_init();

    if (!y_sqr)
        return NULL;

    // x^2 (% p)
    if (mbedtls_mpi_mul_mpi(&y_sqr->mpi, &x->mpi, &x->mpi) ||
            mbedtls_mpi_mod_mpi(&y_sqr->mpi, &y_sqr->mpi, &e->group.P))
        goto error;

    // (X^2) + a (%p)
    if (e->group.A.p == NULL) {
    // For optimizations mbedtls doesn't store 'a' when it is -3 ...
    if (mbedtls_mpi_sub_int(&y_sqr->mpi, &y_sqr->mpi, 3) ||
            ((mbedtls_mpi_cmp_int(&y_sqr->mpi, 0) < 0) &&
            mbedtls_mpi_add_mpi(&y_sqr->mpi, &y_sqr->mpi, &e->group.P)))
        goto error;
    } else if (mbedtls_mpi_add_mpi(&y_sqr->mpi, &y_sqr->mpi, &e->group.A) ||
            ((mbedtls_mpi_cmp_mpi(&y_sqr->mpi, &e->group.P) >= 0) &&
            mbedtls_mpi_sub_abs(&y_sqr->mpi, &y_sqr->mpi, &e->group.P)))
        goto error;

    // (x^2 + a) * x (% p)
    if (mbedtls_mpi_mul_mpi(&y_sqr->mpi, &y_sqr->mpi, &x->mpi) ||
            mbedtls_mpi_mod_mpi(&y_sqr->mpi, &y_sqr->mpi, &e->group.P))
        goto error;

    // ((x^2 + a) * x) + b (%p)
    if (mbedtls_mpi_add_mpi(&y_sqr->mpi, &y_sqr->mpi, &e->group.B) ||
            ((mbedtls_mpi_cmp_mpi(&y_sqr->mpi, &e->group.P) >= 0) &&
            mbedtls_mpi_sub_abs(&y_sqr->mpi, &y_sqr->mpi, &e->group.P)))
        goto error;

    return y_sqr;

error:
    crypto_bignum_deinit(y_sqr, 1);
    return NULL;
#endif
}

/**
 * crypto_ec_point_is_at_infinity - Check whether EC point is neutral element
 * @e: EC context from crypto_ec_init()
 * @p: EC point
 * Returns: 1 if the specified EC point is the neutral element of the group or
 *	0 if not
 */
int crypto_ec_point_is_at_infinity(struct crypto_ec *e,
                const struct crypto_ec_point *p)
{
    return mbedtls_ecp_is_zero((mbedtls_ecp_point *)&p->point);
}

/**
 * crypto_ec_point_is_on_curve - Check whether EC point is on curve
 * @e: EC context from crypto_ec_init()
 * @p: EC point
 * Returns: 1 if the specified EC point is on the curve or 0 if not
 */
int crypto_ec_point_is_on_curve(struct crypto_ec *e,
                const struct crypto_ec_point *p)
{
    if (mbedtls_ecp_check_pubkey(&e->group, &p->point))
        return 0;
    return 1;
}

/**
 * crypto_ec_point_cmp - Compare two EC points
 * @e: EC context from crypto_ec_init()
 * @a: EC point
 * @b: EC point
 * Returns: 0 on equal, non-zero otherwise
 */
int crypto_ec_point_cmp(const struct crypto_ec *e,
                const struct crypto_ec_point *a,
                const struct crypto_ec_point *b)
{
    return mbedtls_ecp_point_cmp(&a->point, &b->point);
}

int crypto_mod_exp(const uint8_t *base, size_t base_len,
                const uint8_t *power, size_t power_len,
                const uint8_t *modulus, size_t modulus_len,
                uint8_t *result, size_t *result_len)
{
    mbedtls_mpi bn_base, bn_exp, bn_modulus, bn_result, bn_rinv;
    int ret = 0;

    mbedtls_mpi_init(&bn_base);
    mbedtls_mpi_init(&bn_exp);
    mbedtls_mpi_init(&bn_modulus);
    mbedtls_mpi_init(&bn_result);
    mbedtls_mpi_init(&bn_rinv);

    mbedtls_mpi_read_binary(&bn_base, base, base_len);
    mbedtls_mpi_read_binary(&bn_exp, power, power_len);
    mbedtls_mpi_read_binary(&bn_modulus, modulus, modulus_len);

    ret = mbedtls_mpi_exp_mod(&bn_result, &bn_base, &bn_exp, &bn_modulus, &bn_rinv);
    if (ret < 0) {
        mbedtls_mpi_free(&bn_base);
        mbedtls_mpi_free(&bn_exp);
        mbedtls_mpi_free(&bn_modulus);
        mbedtls_mpi_free(&bn_result);
        mbedtls_mpi_free(&bn_rinv);
        return ret;
    }

    ret = mbedtls_mpi_write_binary(&bn_result, result, *result_len);

    mbedtls_mpi_free(&bn_base);
    mbedtls_mpi_free(&bn_exp);
    mbedtls_mpi_free(&bn_modulus);
    mbedtls_mpi_free(&bn_result);
    mbedtls_mpi_free(&bn_rinv);

    return ret;
}

int crypto_dh_init(uint8_t generator, const uint8_t *prime, size_t prime_len, uint8_t *privkey, uint8_t *pubkey)
{
    size_t pubkey_len, pad;

    if (sys_random_bytes_get(privkey, prime_len) < 0) {
        return -1;
    }
    if (sys_memcmp(privkey, prime, prime_len) > 0) {
        /* Make sure private value is smaller than prime */
        privkey[0] = 0;
    }

    pubkey_len = prime_len;
    if (crypto_mod_exp(&generator, 1, privkey, prime_len, prime, prime_len,
                pubkey, &pubkey_len) < 0) {
        return -1;
    }
    if (pubkey_len < prime_len) {
        pad = prime_len - pubkey_len;
        memmove(pubkey + pad, pubkey, pubkey_len);
        sys_memset(pubkey, 0, pad);
    }

    return 0;
}

int crypto_dh_derive_secret(uint8_t generator, const uint8_t *prime, size_t prime_len,
                    const uint8_t *order, size_t order_len,
                    const uint8_t *privkey, size_t privkey_len,
                    const uint8_t *pubkey, size_t pubkey_len,
                    uint8_t *secret, size_t *len)
{
#ifdef CONFIG_WPS
    mbedtls_mpi pub;
    int res = -1;

    if (pubkey_len > prime_len ||
        (pubkey_len == prime_len &&
         sys_memcmp(pubkey, prime, prime_len) >= 0))
        return -1;

    mbedtls_mpi_init(&pub);
    res = mbedtls_mpi_read_binary(&pub, pubkey, pubkey_len);
    if (res || mbedtls_mpi_cmp_int(&pub, 1) <= 0)
        goto fail;

    if (order) {
        mbedtls_mpi p, q, tmp;
        int failed;

        /* verify: pubkey^q == 1 mod p */
        mbedtls_mpi_init(&p);
        res |= mbedtls_mpi_read_binary(&p, prime, prime_len);
        mbedtls_mpi_init(&q);
        res |= mbedtls_mpi_read_binary(&q, order, order_len);
        mbedtls_mpi_init(&tmp);
        failed = res ||
            mbedtls_mpi_exp_mod(&tmp, &pub, &q, &p, NULL) != 0 ||
            mbedtls_mpi_cmp_int(&tmp, 1) != 0;
        mbedtls_mpi_free(&p);
        mbedtls_mpi_free(&q);
        mbedtls_mpi_free(&tmp);
        if (failed)
            goto fail;
    }

    res = crypto_mod_exp(pubkey, pubkey_len, privkey, privkey_len,
                 prime, prime_len, secret, len);
fail:
    mbedtls_mpi_free(&pub);
    return res;
#else
    return 0;
#endif
}

#ifdef CONFIG_OWE
/**
 * crypto_ecdh_init - Initialize elliptic curve diffie–hellman context
 * @group: Identifying number for the ECC group (IANA "Group Description"
 *	attribute registrty for RFC 2409)
 * This function generates ephemeral key pair.
 * Returns: Pointer to ECDH context or %NULL on failure
 */
struct crypto_ecdh * crypto_ecdh_init(int group)
{
    struct crypto_ecdh *ecdh = NULL;
    mbedtls_ecp_group_id grp_id;
    mbedtls_ecp_keypair *key;

    grp_id = mbedtls_get_group_id(group);
    if (grp_id == MBEDTLS_ECP_DP_NONE)
        return NULL;

    ecdh = sys_zalloc(sizeof(struct crypto_ecdh) + sizeof(struct crypto_ec_key));
    if (!ecdh)
        return NULL;
    ecdh->key = (struct crypto_ec_key *)((uint8_t *)ecdh + sizeof(struct crypto_ecdh));

    ecdh->ephemeral_key = true;
    mbedtls_pk_init(&ecdh->key->pk);
    if (mbedtls_pk_setup(&ecdh->key->pk, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY_DH)))
        goto fail;

    key = mbedtls_pk_ec(ecdh->key->pk);

    if (mbedtls_ecp_group_load(&key->grp, grp_id) ||
        mbedtls_ecdh_gen_public(&key->grp, &key->d, &key->Q, mbedtls_rand, NULL))
        goto fail;

    return ecdh;

fail:
    crypto_ecdh_deinit(ecdh);
    return NULL;
}

/**
 * crypto_ecdh_deinit - Free ECDH context
 * @ecdh: ECDH context from crypto_ecdh_init() or crypto_ecdh_init2()
 */
void crypto_ecdh_deinit(struct crypto_ecdh *ecdh)
{
    if (!ecdh)
        return;

    if (ecdh->ephemeral_key)
        mbedtls_pk_free(&ecdh->key->pk);
    sys_mfree(ecdh);
}

/**
 * crypto_ecdh_prime_len - Get length of the prime in octets
 * @e: ECDH context from crypto_ecdh_init()
 * Returns: Length of the prime defining the group
 */
size_t crypto_ecdh_prime_len(struct crypto_ecdh *ecdh)
{
    mbedtls_ecp_keypair *key;
    key = mbedtls_pk_ec(ecdh->key->pk);
    return mbedtls_mpi_size(&key->grp.P);
}

/**
 * crypto_ecdh_get_pubkey - Retrieve Public from ECDH context
 * @ecdh: ECDH context from crypto_ecdh_init() or crypto_ecdh_init2()
 * @inc_y: Whether public key should include y coordinate (explicit form)
 * @pub_len: The pub length need to returned from func
 * or not (compressed form)
 * Returns: Binary data f the public key or %NULL on failure
 */
uint8_t *crypto_ecdh_get_pubkey(struct crypto_ecdh *ecdh, int inc_y, size_t *pub_len)
{
    uint8_t *pub;
    mbedtls_ecp_keypair *key;
    uint8_t *x = NULL, *y = NULL;
    size_t len = crypto_ecdh_prime_len(ecdh);
    int nb = 1;

    if (inc_y)
        nb = 2;

    pub = sys_zalloc(len * nb);
    if (!pub)
        return NULL;

    x = pub;
    if (inc_y)
        y = pub + len;

    key = mbedtls_pk_ec(ecdh->key->pk);

    if (crypto_ec_point_to_bin((struct crypto_ec *)&key->grp,
                                (struct crypto_ec_point *)&key->Q, x, y)) {
        sys_mfree(pub);
        pub = NULL;
    }
    *pub_len = len * nb;

    return pub;
}

/**
 * crypto_ecdh_set_peerkey - Compute ECDH secret
 * @ecdh: ECDH context from crypto_ecdh_init() or crypto_ecdh_init2()
 * @inc_y: Whether Peer's public key includes y coordinate (explicit form)
 * or not (compressed form)
 * @key: Binary data of the Peer's public key
 * @len: Length of the @key buffer
 * @srec_len: Length of the @key
 * Returns: Binary data with the EDCH secret or %NULL on failure
 */
uint8_t * crypto_ecdh_set_peerkey(struct crypto_ecdh *ecdh, int inc_y,
                                const uint8_t *key, size_t len, size_t *srec_len)
{
    mbedtls_ecp_point peer_pub;
    mbedtls_ecp_keypair *own_key;
    size_t prime_len, secret_len, secret_used = 0;
    mbedtls_mpi z;
    uint8_t *secret = NULL;

    own_key = mbedtls_pk_ec(ecdh->key->pk);
    prime_len = mbedtls_mpi_size(&own_key->grp.P);
    mbedtls_ecp_point_init(&peer_pub);
    mbedtls_mpi_init(&z);

    if (inc_y) {
        if (len != (2 * prime_len))
            return NULL;

        if (mbedtls_mpi_read_binary(&peer_pub.X, key, prime_len) ||
            mbedtls_mpi_read_binary(&peer_pub.Y, key + prime_len, prime_len) ||
            mbedtls_mpi_lset(&peer_pub.Z, 1))
            goto fail;
    } else {
        if (mbedtls_mpi_read_binary(&z, key, len) ||
            crypto_ec_point_solve_y_coord((struct crypto_ec *)&own_key->grp, (struct crypto_ec_point *)&peer_pub, (struct crypto_bignum *)&z, 0))
            goto fail;
        mbedtls_mpi_free(&z);
    }

    if (mbedtls_ecdh_compute_shared(&own_key->grp, &z, &peer_pub, &own_key->d,
                                    mbedtls_rand, NULL))
        goto fail;

    secret_len = mbedtls_mpi_size(&z);
    secret = sys_zalloc(prime_len);
    if (!secret)
        goto fail;

    // pad to prime len
    if (secret_len < prime_len) {
        // wpabuf_put(secret, prime_len - secret_len);
        secret_used = prime_len - secret_len;
    }

    if (mbedtls_mpi_write_binary(&z, (unsigned char *)(secret + secret_used), secret_len)) {
        sys_mfree(secret);
        secret = NULL;
    }
    *srec_len = prime_len;
fail:
    mbedtls_ecp_point_free(&peer_pub);
    mbedtls_mpi_free(&z);
    return secret;
}
#endif /* CONFIG_OWE */

#ifdef CONFIG_GDWIFI
int crypto_pkcs5_pbkdf2_hmac(int alg,
                              const unsigned char *password,
                              size_t plen, const unsigned char *salt, size_t slen,
                              unsigned int iteration_count,
                              uint32_t key_length, unsigned char *output)
{
    const mbedtls_md_info_t *md_info;
    mbedtls_md_type_t md_type;
    mbedtls_md_context_t md_ctx;

    if (password == NULL || plen == 0 || key_length == 0 || output == NULL)
        return -1;

    switch (alg) {
    case PBKDF2_DIG_ALG_MD5:
        md_type = MBEDTLS_MD_MD5;
        break;
    case PBKDF2_DIG_ALG_SHA1:
        md_type = MBEDTLS_MD_SHA1;
        break;
    case PBKDF2_DIG_ALG_SHA224:
        md_type = MBEDTLS_MD_SHA224;
        break;
    case PBKDF2_DIG_ALG_SHA256:
        md_type = MBEDTLS_MD_SHA256;
        break;
    case PBKDF2_DIG_ALG_SHA384:
        md_type = MBEDTLS_MD_SHA384;
        break;
    case PBKDF2_DIG_ALG_SHA512:
        md_type = MBEDTLS_MD_SHA512;
        break;
    default:
        md_type = MBEDTLS_MD_NONE;
        break;
    }

    mbedtls_md_init(&md_ctx);

    md_info = mbedtls_md_info_from_type(md_type);
    if(md_info == NULL)
        return -1;

    if (mbedtls_md_setup(&md_ctx, md_info, 1) != 0)
        return -1;
    if (mbedtls_pkcs5_pbkdf2_hmac(&md_ctx, password,
                                plen, salt, slen,
                                iteration_count, key_length, output) != 0) {
        return -1;
    }
    mbedtls_md_free(&md_ctx);

    return 0;
}

int crypto_base64_encode(unsigned char *dst, size_t dlen, size_t *olen,
                          const unsigned char *src, size_t slen)
{
    return mbedtls_base64_encode(dst, dlen, olen, src, slen);
}

int crypto_base64_decode(unsigned char *dst, size_t dlen, size_t *olen,
                          const unsigned char *src, size_t slen)
{
    return mbedtls_base64_decode(dst, dlen, olen, src,slen);
}
#endif /* CONFIG_GDWIFI */
