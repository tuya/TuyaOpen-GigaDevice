/*!
    \file    crypto_mbedtls.c
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

#include "includes.h"
#include "rom_export_mbedtls.h"
#include "mbedtls/bignum.h"
#include "mbedtls/ecp.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/x509_csr.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/base64.h"

#include "common.h"
#include "crypto.h"
#include "random.h"
#include "tls/asn1.h"

struct crypto_bignum {
	mbedtls_mpi mpi;
};

/**
 * crypto_bignum_init - Allocate memory for bignum
 * Returns: Pointer to allocated bignum or %NULL on failure
 */
struct crypto_bignum *crypto_bignum_init(void)
{
	struct crypto_bignum *n;

	n = os_malloc(sizeof(*n));
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
struct crypto_bignum *crypto_bignum_init_set(const u8 *buf, size_t len)
{
	struct crypto_bignum *n = crypto_bignum_init();
	if (!n)
		return NULL;

	if (mbedtls_mpi_read_binary(&n->mpi, buf, len)) {
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
struct crypto_bignum * crypto_bignum_init_uint(unsigned int val)
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
	os_free(n);
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
			 u8 *buf, size_t buflen, size_t padlen)
{
	int res = buflen;
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
	void *buf = os_malloc(size);
	int ret = -1;

	if (!buf)
		return -1;

	// As a first step takes the easy option, probably need something more
	// complete using mbedtls_mpi_fill_random
	if (random_get_bytes(buf, size))
		goto end;

	if (mbedtls_mpi_read_binary(&r->mpi, buf, size))
		goto end;

	if (mbedtls_mpi_mod_mpi(&r->mpi, &r->mpi, &m->mpi))
		goto end;

	ret = 0;
end:
	os_free(buf);
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
	if (mbedtls_mpi_add_mpi(&c->mpi, &a->mpi, &b->mpi))
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
	if (mbedtls_mpi_mod_mpi(&c->mpi, &a->mpi, &b->mpi))
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
	if (mbedtls_mpi_exp_mod(&d->mpi, &a->mpi, &b->mpi, &c->mpi, NULL))
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
	if (mbedtls_mpi_inv_mod(&c->mpi, &a->mpi, &b->mpi))
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
	if (mbedtls_mpi_sub_mpi(&c->mpi, &a->mpi, &b->mpi))
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
	if (mbedtls_mpi_div_mpi(&c->mpi, NULL, &a->mpi, &b->mpi))
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
	if (mbedtls_mpi_mul_mpi(&d->mpi, &a->mpi, &b->mpi))
		return -1;

	if (mbedtls_mpi_mod_mpi(&d->mpi, &d->mpi, &c->mpi))
		return -1;

	return 0;
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
	    mbedtls_mpi_copy(&r->mpi, &a->mpi))
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

/**
 * crypto_bignum_legendre - Compute the Legendre symbol (a/p)
 * @a: Bignum
 * @p: Bignum
 * Returns: Legendre symbol -1,0,1 on success; -2 on calculation failure
 */
int crypto_bignum_legendre(const struct crypto_bignum *a,
			   const struct crypto_bignum *p)
{
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
}

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
	struct crypto_ec *ec;
	mbedtls_ecp_group_id grp_id;

	grp_id = mbedtls_get_group_id(group);
	if (grp_id == MBEDTLS_ECP_DP_NONE)
		return NULL;

	ec = os_malloc(sizeof(*ec));
	if (!ec)
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
	os_free(e);
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
static mbedtls_mpi_uint minus_3_data[1] = {3};
static const struct crypto_bignum minus_3 = {
	.mpi = {
		.s = -1,
		.n = 1,
		.p = minus_3_data,
	},
};

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
	ecp = os_malloc(sizeof(*ecp));
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
	os_free(p);
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
			   const struct crypto_ec_point *p, u8 *x, u8 *y)
{
	size_t p_len = crypto_ec_prime_len(e);

	if (x && mbedtls_mpi_write_binary(&p->point.X, x, p_len))
		return -1;

	if (y && mbedtls_mpi_write_binary(&p->point.Y, y, p_len))
		return -1;

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
						  const u8 *val)
{
	size_t p_len = crypto_ec_prime_len(e);
	size_t tmp_len = 2 * p_len + 1;
	struct crypto_ec_point *ecp;
	u8 *tmp;

	ecp = crypto_ec_point_init(e);
	if (!ecp)
		return NULL;

	tmp = os_malloc(tmp_len);
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

	os_free(tmp);
	return ecp;
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
	if (mbedtls_ecp_mul(&e->group, &res->point, &b->mpi, &p->point,
			    NULL, NULL))
		return -1;

	return 0;
}

/**
 * crypto_ec_point_invert - Compute inverse of an EC point
 * @e: EC context from crypto_ec_init()
 * @p: EC point to invert (and result of the operation)
 * Returns: 0 on success, -1 on failure
 */
int crypto_ec_point_invert(struct crypto_ec *e, struct crypto_ec_point *p)
{
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

/**
 * crypto_ec_point_debug_print - Dump EC point
 * @e: EC context from crypto_ec_init()
 * @p: EC point
 * @title: Name of the EC point in the trace
 */
#ifndef	CONFIG_NO_WPA_MSG
static char* write_hex(char *str, u8 val)
{
	u8 x[2];
	x[0] = (val >> 4) & 0x0f;
	x[1] = val & 0xf;

	for (int i = 0; i <2 ; i++)
	{
		if (x[i] < 10)
			*str++ = '0' + x[i];
		else
			*str++ = 'a' - 10 + x[i];
	}
	return str;
}
#endif

void crypto_ec_point_debug_print(const struct crypto_ec *e,
				 const struct crypto_ec_point *p,
				 const char *title)
{
#ifndef	CONFIG_NO_WPA_MSG
	size_t prime_len = crypto_ec_prime_len((struct crypto_ec *)e);
	u8 *bin = NULL;
	char *str = NULL, *pos;

	bin = os_malloc(prime_len);
	str = os_malloc(prime_len * 4 + 4);
	if (!bin || !str)
		goto fail;

	pos = str;
	*pos++ = '(';

	if (mbedtls_mpi_write_binary(&p->point.X, bin, prime_len))
		goto fail;

	for (int i=0; i <prime_len; i++) {
		pos = write_hex(pos, bin[i]);
	}

	*pos++ = ',';
	if (mbedtls_mpi_write_binary(&p->point.Y, bin, prime_len))
		goto fail;

	for (int i=0; i <prime_len; i++) {
		pos = write_hex(pos, bin[i]);
	}
	*pos++ = ')';
	*pos = 0;

	wpa_printf(MSG_DEBUG, "%s: %s", title, str);

fail:
	os_free(bin);
	os_free(str);
#endif
}

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

static int mbedtls_rand(void *rng_state, unsigned char *output, size_t len )
{
	return random_get_bytes(output, len);
}

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

	ecdh = os_zalloc(sizeof(struct crypto_ecdh) + sizeof(struct crypto_ec_key));
	if (!ecdh)
		return NULL;
	ecdh->key = (struct crypto_ec_key *)((u8 *)ecdh + sizeof(struct crypto_ecdh));

	ecdh->ephemeral_key = true;
	mbedtls_pk_init(&ecdh->key->pk);
	if (mbedtls_pk_setup(&ecdh->key->pk,
			     mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY_DH)))
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
 * crypto_ecdh_init2 - Initialize elliptic curve diffie–hellman context with
 * given EC key
 * @group: Identifying number for the ECC group (IANA "Group Description"
 *	attribute registrty for RFC 2409)
 * @own_key: Our own EC Key.
 * Returns: Pointer to ECDH context or %NULL on failure
 */
struct crypto_ecdh * crypto_ecdh_init2(int group, struct crypto_ec_key *own_key)
{
	struct crypto_ecdh *ecdh = NULL;

	ecdh = os_zalloc(sizeof(struct crypto_ecdh));
	if (!ecdh)
		return NULL;

	ecdh->key = own_key;
	return ecdh;
}

/**
 * crypto_ecdh_get_pubkey - Retrieve Public from ECDH context
 * @ecdh: ECDH context from crypto_ecdh_init() or crypto_ecdh_init2()
 * @inc_y: Whether public key should include y coordinate (explicit form)
 * or not (compressed form)
 * Returns: Binary data f the public key or %NULL on failure
 */
struct wpabuf * crypto_ecdh_get_pubkey(struct crypto_ecdh *ecdh, int inc_y)
{
	struct wpabuf *pub;
	mbedtls_ecp_keypair *key;
	u8 *x = NULL, *y = NULL;
	size_t len = crypto_ecdh_prime_len(ecdh);
	int nb = 1;

	if (inc_y)
		nb = 2;

	pub = wpabuf_alloc(len * nb);
	if (!pub)
		return NULL;

	x = wpabuf_put(pub, len);
	if (inc_y)
		y = wpabuf_put(pub, len);

	key = mbedtls_pk_ec(ecdh->key->pk);

	if (crypto_ec_point_to_bin((struct crypto_ec *)&key->grp,
				   (struct crypto_ec_point *)&key->Q, x, y)) {
		wpabuf_free(pub);
		pub = NULL;
	}

	return pub;
}

/**
 * crypto_ecdh_set_peerkey - Compute ECDH secret
 * @ecdh: ECDH context from crypto_ecdh_init() or crypto_ecdh_init2()
 * @inc_y: Whether Peer's public key includes y coordinate (explicit form)
 * or not (compressed form)
 * @key: Binary data of the Peer's public key
 * @len: Length of the @key buffer
 * Returns: Binary data with the EDCH secret or %NULL on failure
 */
struct wpabuf * crypto_ecdh_set_peerkey(struct crypto_ecdh *ecdh, int inc_y,
					const u8 *key, size_t len)
{
	mbedtls_ecp_point peer_pub;
	mbedtls_ecp_keypair *own_key;
	size_t prime_len, secret_len;
	mbedtls_mpi z;
	struct wpabuf *secret = NULL;

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
		    crypto_ec_point_solve_y_coord((struct crypto_ec *)&own_key->grp,
						  (struct crypto_ec_point *)&peer_pub,
						  (struct crypto_bignum *)&z, 0))
			goto fail;
		mbedtls_mpi_free(&z);
	}


	if (mbedtls_ecdh_compute_shared(&own_key->grp, &z, &peer_pub, &own_key->d,
					mbedtls_rand, NULL))
		goto fail;

	secret_len = mbedtls_mpi_size(&z);
	secret = wpabuf_alloc(prime_len);
	if (!secret)
		goto fail;

	// pad to prime len
	if (secret_len < prime_len)
		wpabuf_put(secret, prime_len - secret_len);

	if (mbedtls_mpi_write_binary(&z, wpabuf_put(secret, secret_len), secret_len)) {
		wpabuf_free(secret);
		secret = NULL;
	}

fail:
	mbedtls_ecp_point_free(&peer_pub);
	mbedtls_mpi_free(&z);
	return secret;
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
	os_free(ecdh);
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
 * crypto_ec_key_parse_priv - Initialize EC Key pair from ECPrivateKey ASN.1
 * @der: DER encoding of ASN.1 ECPrivateKey
 * @der_len: Length of @der buffer
 * Returns: EC key or %NULL on failure
 */
struct crypto_ec_key * crypto_ec_key_parse_priv(const u8 *der, size_t der_len)
{
	struct crypto_ec_key *key;

	key = os_zalloc(sizeof(struct crypto_ec_key));
	if (!key)
		return NULL;
	mbedtls_pk_init(&key->pk);

	if (mbedtls_pk_parse_key(&key->pk, der, der_len, NULL, 0)) {
		crypto_ec_key_deinit(key);
		return NULL;
	}

	return key;
}

/**
 * crypto_ec_key_parse_pub - Initialize EC Key pair from SubjectPublicKeyInfo ASN.1
 * @der: DER encoding of ASN.1 SubjectPublicKeyInfo
 * @der_len: Length of @der buffer
 * Returns: EC key or %NULL on failure
 */
static mbedtls_ecp_group_id mbedtls_oid_to_gr_id(const struct asn1_oid *oid, size_t *prime_len)
{
	if (asn1_oid_equal(oid, &asn1_prime256v1_oid)) {
		*prime_len = 32;
		return MBEDTLS_ECP_DP_SECP256R1;
	}
	if (asn1_oid_equal(oid, &asn1_secp384r1_oid)) {
		*prime_len = 48;
		return MBEDTLS_ECP_DP_SECP384R1;
	}
	if (asn1_oid_equal(oid, &asn1_secp521r1_oid)) {
		*prime_len = 66;
		return MBEDTLS_ECP_DP_SECP521R1;
	}
	if (asn1_oid_equal(oid, &asn1_brainpoolP256r1_oid)) {
		*prime_len = 32;
		return MBEDTLS_ECP_DP_BP256R1;
	}
	if (asn1_oid_equal(oid, &asn1_brainpoolP384r1_oid)) {
		*prime_len = 48;
		return MBEDTLS_ECP_DP_BP384R1;
	}
	if (asn1_oid_equal(oid, &asn1_brainpoolP512r1_oid)) {
		*prime_len = 64;
		return MBEDTLS_ECP_DP_BP512R1;
	}

	return MBEDTLS_ECP_DP_NONE;
}

struct crypto_ec_key * crypto_ec_key_parse_pub(const u8 *der, size_t der_len)
{
	struct crypto_ec_key *key;
	mbedtls_ecp_keypair *eckey;
	mbedtls_ecp_group_id grp_id;
	const u8 *pubkey, *algo, *end;
	size_t algo_len, pubkey_len, prime_len;
	struct asn1_oid oid;
	struct asn1_hdr asn1;

	// mbedtls_pk_parse_subpubkey, doesn't supported compressed form
	// so write our own parser...
	end = der + der_len;
	algo = der;
	algo_len = der_len;
	if (asn1_get_next(algo, algo_len, &asn1) ||
	    (asn1.tag != ASN1_TAG_SEQUENCE))
		return NULL;

	algo_len = end - asn1.payload;
	if (asn1_get_next(asn1.payload, der_len, &asn1) ||
	    (asn1.tag != ASN1_TAG_SEQUENCE))
		return NULL;

	pubkey = asn1.payload + asn1.length;
	pubkey_len = end - pubkey;

	algo_len = asn1.length;
	if (asn1_get_oid(asn1.payload, algo_len, &oid, &algo) ||
	    !asn1_oid_equal(&oid, &asn1_ec_public_key_oid))
		return NULL;

	algo_len = pubkey - algo;
	if (asn1_get_oid(algo, algo_len, &oid, &algo))
		return NULL;

	grp_id = mbedtls_oid_to_gr_id(&oid, &prime_len);
	if (grp_id == MBEDTLS_ECP_DP_NONE)
		return NULL;

	if (asn1_get_next(pubkey, pubkey_len, &asn1) ||
	    (asn1.tag != ASN1_TAG_BITSTRING))
		return NULL;

	pubkey = asn1.payload + 1; // skip unused bit count
	pubkey_len = asn1.length - 1;
	if ((pubkey[0] < 0x2) ||
	    (pubkey[0] > 0x4) ||
	    ((pubkey[0] == 0x4) && (asn1.length != 2 + 2 * prime_len)) ||
	    ((pubkey[0] != 0x4) && (asn1.length > 2 + prime_len)))
		return NULL;

	key = os_zalloc(sizeof(struct crypto_ec_key));
	if (!key)
		return NULL;

	mbedtls_pk_init(&key->pk);
	if (mbedtls_pk_setup(&key->pk,
			     mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)))
		goto fail;
	eckey = mbedtls_pk_ec(key->pk);
	if (mbedtls_ecp_group_load(&eckey->grp, grp_id))
		goto fail;

	if (pubkey[0] == 0x4) {
		// uncompressed form
		if (mbedtls_ecp_point_read_binary(&eckey->grp, &eckey->Q,
						  pubkey, pubkey_len))
			goto fail;

	} else {
		//compressed form
		mbedtls_mpi x;
		int y_bit = pubkey[0] & 0x1;

		mbedtls_mpi_init(&x);
		if (mbedtls_mpi_read_binary(&x, &pubkey[1], pubkey_len - 1) ||
		    crypto_ec_point_solve_y_coord((struct crypto_ec *)&eckey->grp,
						  (struct crypto_ec_point *)&eckey->Q,
						  (struct crypto_bignum *)&x, y_bit))
			goto fail;
	}

	if (mbedtls_ecp_check_pubkey(&eckey->grp, &eckey->Q))
		goto fail;

	return key;
fail:
	crypto_ec_key_deinit(key);
	return NULL;
}

/**
 * crypto_ec_key_set_pub - Initialize an EC Public Key from EC point coordinates
 * @group: Identifying number for the ECC group
 * @x: X coordinate of the Public key
 * @y: Y coordinate of the Public key
 * @len: Length of @x and @y buffer
 * Returns: EC key or %NULL on failure
 *
 * This function initialize an EC Key from public key coordinates, in big endian
 * byte order padded to the length of the prime defining the group.
 */
struct crypto_ec_key * crypto_ec_key_set_pub(int group, const u8 *x, const u8 *y, size_t len)
{
	struct crypto_ec_key *key;
	mbedtls_ecp_keypair *eckey;
	mbedtls_ecp_group_id grp_id;

	grp_id = mbedtls_get_group_id(group);
	if (grp_id == MBEDTLS_ECP_DP_NONE)
		return NULL;

	key = os_zalloc(sizeof(struct crypto_ec_key));
	if (!key)
		return NULL;
	mbedtls_pk_init(&key->pk);
	if (mbedtls_pk_setup(&key->pk,
			     mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)))
		goto fail;

	eckey = mbedtls_pk_ec(key->pk);

	if (mbedtls_ecp_group_load(&eckey->grp, grp_id) ||
	    mbedtls_mpi_read_binary(&eckey->Q.X, x, len) ||
	    mbedtls_mpi_read_binary(&eckey->Q.Y, y, len) ||
	    mbedtls_mpi_lset(&eckey->Q.Z, 1) ||
	    mbedtls_ecp_check_pubkey(&eckey->grp, &eckey->Q))
		goto fail;

	return key;
fail:
	crypto_ec_key_deinit(key);
	return NULL;
}

/**
 * crypto_ec_key_set_pub_point - Initialize an EC Public Key from EC point
 * @e: EC context from crypto_ec_init()
 * @pub: Public key point
 * Returns: EC key or %NULL on failure
 */
struct crypto_ec_key * crypto_ec_key_set_pub_point(struct crypto_ec *e,
						   const struct crypto_ec_point *pub)
{
	struct crypto_ec_key *key;
	mbedtls_ecp_keypair *eckey;

	key = os_zalloc(sizeof(struct crypto_ec_key));
	if (!key)
		return NULL;
	mbedtls_pk_init(&key->pk);
	if (mbedtls_pk_setup(&key->pk,
			     mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)))
		goto fail;

	eckey = mbedtls_pk_ec(key->pk);

	if (mbedtls_ecp_group_load(&eckey->grp, e->group.id) ||
	    mbedtls_ecp_copy(&eckey->Q, &pub->point) ||
	    mbedtls_ecp_check_pubkey(&eckey->grp, &eckey->Q))
		goto fail;

	return key;
fail:
	crypto_ec_key_deinit(key);
	return NULL;
}

/**
 * crypto_ec_key_gen - Generate EC Key pair
 * @group: Identifying number for the ECC group
 * Returns: EC key or %NULL on failure
 */
struct crypto_ec_key * crypto_ec_key_gen(int group)
{
	struct crypto_ec_key *key;
	mbedtls_ecp_keypair *eckey;
	mbedtls_ecp_group_id grp_id;

	grp_id = mbedtls_get_group_id(group);
	if (grp_id == MBEDTLS_ECP_DP_NONE)
		return NULL;

	key = os_zalloc(sizeof(struct crypto_ec_key));
	if (!key)
		return NULL;
	mbedtls_pk_init(&key->pk);
	if (mbedtls_pk_setup(&key->pk,
			     mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)))
		goto fail;

	eckey = mbedtls_pk_ec(key->pk);
	if (mbedtls_ecp_gen_key(grp_id, eckey, mbedtls_rand, NULL))
		goto fail;

	return key;
fail:
	crypto_ec_key_deinit(key);
	return NULL;
}

/**
 * crypto_ec_key_deinit - Free EC Key
 * @key: EC key from crypto_ec_key_parse_pub/priv() or crypto_ec_key_gen()
 */
void crypto_ec_key_deinit(struct crypto_ec_key *key)
{
	if (key) {
		mbedtls_pk_free(&key->pk);
		os_free(key);
	}
}

/**
 * crypto_ec_key_get_subject_public_key - Get SubjectPublicKeyInfo ASN.1 for a EC key
 * @key: EC key from crypto_ec_key_parse/set_pub/priv() or crypto_ec_key_gen()
 * Returns: Buffer with DER encoding of ASN.1 SubjectPublicKeyInfo or %NULL on failure
 */
static const struct asn1_oid * mbedtls_get_curve_oid(int grp_id, int *oid_len)
{
	switch (grp_id) {
	case MBEDTLS_ECP_DP_SECP256R1:
		*oid_len = 8;
		return  &asn1_prime256v1_oid;
	case MBEDTLS_ECP_DP_SECP384R1:
		*oid_len = 5;
		return &asn1_secp384r1_oid;
	case MBEDTLS_ECP_DP_SECP521R1:
		*oid_len = 5;
		return &asn1_secp521r1_oid;
	case MBEDTLS_ECP_DP_BP256R1:
		*oid_len = 9;
		return &asn1_brainpoolP256r1_oid;
	case MBEDTLS_ECP_DP_BP384R1:
		*oid_len = 9;
		return &asn1_brainpoolP384r1_oid;
	case MBEDTLS_ECP_DP_BP512R1:
		*oid_len = 9;
		return &asn1_brainpoolP512r1_oid;
	default:
		return NULL;
	}
}

struct wpabuf * crypto_ec_key_get_subject_public_key(struct crypto_ec_key *key)
{
	mbedtls_ecp_keypair *eckey = mbedtls_pk_ec(key->pk);
	const struct asn1_oid *oid_curve;
	struct wpabuf *der;
	int der_len, prime_len, algo_len, oid_len;

	// mbedtls_pk_write_pubkey_der always write key in uncompressed form and since we need
	// compressed form write our own DER writer ...
	prime_len = mbedtls_mpi_size(&eckey->grp.P);
	oid_curve = mbedtls_get_curve_oid(eckey->grp.id, &oid_len);
	if (!oid_curve)
		return NULL;

	algo_len = 2 + 7 /* OID ecPublicKey */ + 2 + oid_len;
	der_len =  (2  +                 // SubjectPublicKeyInfo
		    2  +                 // AlgorithmIdentifier
		    algo_len  +          // AlgorithmIdentifier.algorithm/parameters
		    2  + prime_len + 2); // subjectPublicKey
	der = wpabuf_alloc(der_len);
	if (!der)
		return NULL;

	asn1_put_hdr(der, ASN1_CLASS_UNIVERSAL, 1, ASN1_TAG_SEQUENCE, der_len - 2); // SubjectPublicKeyInfo
	asn1_put_hdr(der, ASN1_CLASS_UNIVERSAL, 1, ASN1_TAG_SEQUENCE, algo_len); // AlgorithmIdentifier
	asn1_put_oid(der, &asn1_ec_public_key_oid); // AlgorithmIdentifier.algorithm
	asn1_put_oid(der, oid_curve); // AlgorithmIdentifier.parameters
	asn1_put_hdr(der, ASN1_CLASS_UNIVERSAL, 0, ASN1_TAG_BITSTRING, prime_len + 2); // subjectPublicKey
	wpabuf_put_u8(der, 0); // number of used bit in the bitstring
	wpabuf_put_u8(der, 0x2 + mbedtls_mpi_get_bit(&eckey->Q.Y, 0)); // COMPRESSED form
	if (mbedtls_mpi_write_binary(&eckey->Q.X, wpabuf_put(der, prime_len), prime_len)) { // X coordinate
		wpabuf_free(der);
		return NULL;
	}

	return der;
}

/**
 * crypto_ec_key_get_ecprivate_key - Get ECPrivateKey ASN.1 for a EC key
 * @key: EC key from crypto_ec_key_parse_priv() or crypto_ec_key_gen()
 * @include_pub: Whether to include public key in the ASN.1 sequence
 * Returns: Buffer with DER encoding of ASN.1 ECPrivateKey or %NULL on failure
 */
struct wpabuf * crypto_ec_key_get_ecprivate_key(struct crypto_ec_key *key,
						bool include_pub)
{
	mbedtls_ecp_keypair *eckey = mbedtls_pk_ec(key->pk);
	const struct asn1_oid *oid_curve;
	struct wpabuf *der = NULL;
	int der_len, prime_len, oid_len, pub_len = 0, pub_len_len = 0, der_len_len = 2;

	if (eckey->d.n == 0)
		return NULL;

	// This time we want public key in uncompressed form but we don't always want
	// it and mbedtls_pk_write_key_der always include it. So again write our own
	// DER writer
	prime_len = mbedtls_mpi_size(&eckey->grp.P);
	oid_curve = mbedtls_get_curve_oid(eckey->grp.id, &oid_len);
	if (!oid_curve)
		return NULL;

	der_len = (der_len_len +      // ECPrivateKey
		   3 +                // version
		   2 + prime_len +    // privateKey
		   2 + 2 + oid_len);  // Context specific [0] ECParameters

	if (include_pub) {
		pub_len = 2 + prime_len * 2;
		if (pub_len > 127)
			pub_len_len = 1; // will require 1 more byte for len of public key
		else
			pub_len_len = 0;

		der_len += ( 2 + 2 + 2 * pub_len_len + pub_len); // Context specific [1] publicKey
		if (der_len - 2 > 127) {
			der_len_len++;     // size of ECPrivateKey will require 2 bytes
			der_len++;
		}
	}
	der = wpabuf_alloc(der_len);
	if (!der)
		return NULL;

	asn1_put_hdr(der, ASN1_CLASS_UNIVERSAL, 1, ASN1_TAG_SEQUENCE, der_len - der_len_len); // ECPrivateKey
	asn1_put_integer(der, 1); // version
	asn1_put_hdr(der, ASN1_CLASS_UNIVERSAL, 0, ASN1_TAG_OCTETSTRING, prime_len); // privateKey
	if (mbedtls_mpi_write_binary(&eckey->d, wpabuf_put(der, prime_len), prime_len))
		goto fail;
	asn1_put_hdr(der, ASN1_CLASS_CONTEXT_SPECIFIC, 1, 0, oid_len + 2); // Context specific 0
	asn1_put_oid(der, oid_curve); //ECParameters
	if (include_pub) {
		asn1_put_hdr(der, ASN1_CLASS_CONTEXT_SPECIFIC, 1, 1, 2 + pub_len_len + pub_len); // Context specific 1
		asn1_put_hdr(der, ASN1_CLASS_UNIVERSAL, 0, ASN1_TAG_BITSTRING, pub_len); // publicKey
		wpabuf_put_u8(der, 0); // number of used bit in the bitstring
		wpabuf_put_u8(der, 0x4); // UNCOMPRESSED form
		if (mbedtls_mpi_write_binary(&eckey->Q.X, wpabuf_put(der, prime_len), prime_len)) // X coordinate
			goto fail;
		if (mbedtls_mpi_write_binary(&eckey->Q.Y, wpabuf_put(der, prime_len), prime_len)) // Y coordinate
			goto fail;
	}
	return der;
fail:
	wpabuf_free(der);
	return NULL;
}

/**
 * crypto_ec_key_get_pubkey_point - Get Public Key Point coordinates
 * @key: EC key from crypto_ec_key_parse/set_pub() or crypto_ec_key_parse_priv()
 * @prefix: Whether output buffer should include the octect to indicate coordinate
 * form (as defined for SubjectPublicKeyInfo)
 * Returns: Buffer with coordinates of Public key in uncompressed form or %NULL on failure
 */
struct wpabuf * crypto_ec_key_get_pubkey_point(struct crypto_ec_key *key, int prefix)
{
	mbedtls_ecp_keypair *eckey = mbedtls_pk_ec(key->pk);
	size_t prime_len = mbedtls_mpi_size(&eckey->grp.P);
	struct wpabuf *pub;
	u8 *x, *y;

	pub = wpabuf_alloc(2 * prime_len + prefix);
	if (!pub)
		return NULL;

	if (prefix)
		wpabuf_put_u8(pub, 0x04);

	x = wpabuf_put(pub, prime_len);
	y = wpabuf_put(pub, prime_len);
	if (crypto_ec_point_to_bin((struct crypto_ec *)&eckey->grp,
				   (struct crypto_ec_point *)&eckey->Q, x, y)) {
		wpabuf_free(pub);
		return NULL;
	}

	return pub;
}

/**
 * crypto_ec_key_get_public_key - Get EC Public Key as an EC point
 * @key: EC key from crypto_ec_key_parse/set_pub() or crypto_ec_key_parse_priv()
 * Returns: Public key a an EC point and %NULL on failure
 */
struct crypto_ec_point *crypto_ec_key_get_public_key(struct crypto_ec_key *key)
{
	mbedtls_ecp_keypair *eckey = mbedtls_pk_ec(key->pk);
	return (struct crypto_ec_point *)&eckey->Q;
}

/**
 * crypto_ec_key_get_private_key - Get EC Private Key as a bignum
 * @key: EC key from crypto_ec_key_parse/set_pub() or crypto_ec_key_parse_priv()
 * Returns: private key as a bignum and %NULL on failure
 */
struct crypto_bignum *crypto_ec_key_get_private_key(struct crypto_ec_key *key)
{
	mbedtls_ecp_keypair *eckey = mbedtls_pk_ec(key->pk);
	return (struct crypto_bignum *)&eckey->d;
}

/**
 * crypto_ec_key_sign - Sign a buffer with an EC key
 * @key: EC key from crypto_ec_key_parse_priv() or crypto_ec_key_gen()
 * @data: Data to sign
 * @len: Length of @data buffer
 * Returns: Buffer with DER encoding of ASN.1 Ecdsa-Sig-Value or %NULL on failure
 */
struct wpabuf * crypto_ec_key_sign(struct crypto_ec_key *key, const u8 *data,
				   size_t len)
{
	mbedtls_ecp_keypair *eckey = mbedtls_pk_ec(key->pk);
	mbedtls_md_type_t md_alg;
	size_t prime_len = mbedtls_mpi_size(&eckey->grp.P);
	struct wpabuf *sig = NULL;
	size_t sig_len;

	if (prime_len == 32)
		md_alg = MBEDTLS_MD_SHA256;
	else if (prime_len == 48)
		md_alg = MBEDTLS_MD_SHA384;
	else
		md_alg = MBEDTLS_MD_SHA512;

	sig_len = ( 3 + // Ecdsa-Sig-Value (sequence may be > 127)
		    2 + prime_len + 1 + // r (may be prime_len +1 long as
		                        //    interger are signed in ASN.1)
		    2 + prime_len + 1); // s (same as r)
	sig = wpabuf_alloc(sig_len);
	if (!sig)
		return NULL;


	if (mbedtls_ecdsa_write_signature(eckey, md_alg, data, len,
					  wpabuf_mhead(sig), &sig_len,
					  mbedtls_rand, NULL)) {
		wpabuf_free(sig);
		return NULL;
	}

	wpabuf_put(sig, sig_len);
	return sig;
}

/**
 * crypto_ec_key_sign_r_s - Sign a buffer with an EC key
 * @key: EC key from crypto_ec_key_parse_priv() or crypto_ec_key_gen()
 * @data: Data to sign
 * @len: Length of @data buffer
 * Returns: Buffer with r and s value concatenated in a buffer. Each value
 * is in big endian byte order padded to the length of the prime defined the
 * group of the key.
 */
struct wpabuf * crypto_ec_key_sign_r_s(struct crypto_ec_key *key, const u8 *data,
				       size_t len)
{
	mbedtls_ecp_keypair *eckey = mbedtls_pk_ec(key->pk);
	size_t prime_len = mbedtls_mpi_size(&eckey->grp.P);
	mbedtls_mpi r, s;
	struct wpabuf *sig = NULL;

	mbedtls_mpi_init(&r);
	mbedtls_mpi_init(&s);

	if (mbedtls_ecdsa_sign(&eckey->grp, &r, &s, &eckey->d, data, len,
			       mbedtls_rand, NULL)) {
		goto fail;
	}

	sig = wpabuf_alloc(2 * prime_len);
	if (!sig)
		goto fail;

	if (mbedtls_mpi_write_binary(&r, wpabuf_put(sig, prime_len), prime_len) ||
	    mbedtls_mpi_write_binary(&s, wpabuf_put(sig, prime_len), prime_len)) {
		wpabuf_free(sig);
		sig = NULL;
	}

fail:
	mbedtls_mpi_free(&r);
	mbedtls_mpi_free(&s);
	return sig;
}

/**
 * crypto_ec_key_verify_signature - Verify signature
 * @key: EC key from crypto_ec_key_parse/set_pub() or crypto_ec_key_gen()
 * @data: Data to signed
 * @len: Length of @data buffer
 * @sig: DER encoding of ASN.1 Ecdsa-Sig-Value
 * @sig_len: Length of @sig buffer
 * Returns: 1 if signature is valid, 0 if signature is invalid and -1 on failure
 */
int crypto_ec_key_verify_signature(struct crypto_ec_key *key, const u8 *data,
				   size_t len, const u8 *sig, size_t sig_len)
{
	mbedtls_ecp_keypair *eckey = mbedtls_pk_ec(key->pk);
	int ret;

	ret = mbedtls_ecdsa_read_signature(eckey, data, len, sig, sig_len);
	if (ret == MBEDTLS_ERR_ECP_BAD_INPUT_DATA)
		return 0;
	else if (ret)
		return -1;

	return 1;
}

/**
 * crypto_ec_key_verify_signature_r_s - Verify signature
 * @key: EC key from crypto_ec_key_parse/set_pub() or crypto_ec_key_gen()
 * @data: Data to signed
 * @len: Length of @data buffer
 * @r: Binary data, in big endian byte order, of the 'r' field of the ECDSA signature.
 * @s: Binary data, in big endian byte order, of the 's' field of the ECDSA signature.
 * @r_len: Length of @r buffer
 * @s_len: Length of @s buffer
 * Returns: 1 if signature is valid, 0 if signature is invalid and -1 on failure
 */
int crypto_ec_key_verify_signature_r_s(struct crypto_ec_key *key, const u8 *data,
				       size_t len,  const u8 *r, size_t r_len,
				       const u8 *s, size_t s_len)
{
	mbedtls_ecp_keypair *eckey = mbedtls_pk_ec(key->pk);
	mbedtls_mpi mpi_r, mpi_s;
	int ret = -1;

	mbedtls_mpi_init(&mpi_r);
	mbedtls_mpi_init(&mpi_s);
	if (mbedtls_mpi_read_binary(&mpi_r, r, r_len) ||
	    mbedtls_mpi_read_binary(&mpi_s, s, s_len))
		goto fail;


	ret = mbedtls_ecdsa_verify(&eckey->grp, data, len,
				   &eckey->Q, &mpi_r, &mpi_s);
	if (ret == MBEDTLS_ERR_ECP_BAD_INPUT_DATA)
		ret = 0;
	else if (ret == 0)
		ret = 1;

fail:
	mbedtls_mpi_free(&mpi_r);
	mbedtls_mpi_free(&mpi_s);
	return ret;
}

/**
 * crypto_ec_key_group - Get IANA group identifier for an EC key
 * @key: EC key from crypto_ec_key_parse/set_pub/priv() or crypto_ec_key_gen()
 * Returns: IANA group identifier and -1 on failure
 */
int crypto_ec_key_group(struct crypto_ec_key *key)
{
	mbedtls_ecp_keypair *eckey = mbedtls_pk_ec(key->pk);

	switch (eckey->grp.id) {
	case MBEDTLS_ECP_DP_SECP256R1:
		return 19;
	case MBEDTLS_ECP_DP_SECP384R1:
		return 20;
	case MBEDTLS_ECP_DP_SECP521R1:
		return 21;
	case MBEDTLS_ECP_DP_SECP192R1:
		return 26;
	case MBEDTLS_ECP_DP_BP256R1:
		return 28;
	case MBEDTLS_ECP_DP_BP384R1:
		return 29;
	case MBEDTLS_ECP_DP_BP512R1:
		return 30;
	default:
		return -1;
	}
}

/**
 * crypto_ec_key_cmp - Compare 2 EC Public keys
 * @key1: Key 1
 * @key2: Key 2
 * Retruns: 0 if Public keys are identical, non-zero otherwise
 */
int crypto_ec_key_cmp(struct crypto_ec_key *key1, struct crypto_ec_key *key2)
{
	mbedtls_ecp_keypair *eckey1 = mbedtls_pk_ec(key1->pk);
	mbedtls_ecp_keypair *eckey2 = mbedtls_pk_ec(key2->pk);

	return mbedtls_ecp_point_cmp(&eckey1->Q, &eckey2->Q);
}

/**
 * crypto_ec_key_debug_print - Dump EC Key
 * @key:  EC key from crypto_ec_key_parse/set_pub/priv() or crypto_ec_key_gen()
 * @title: Name of the EC point in the trace
 */
void crypto_ec_key_debug_print(const struct crypto_ec_key *key,
			       const char *title)
{
#ifndef CONFIG_NO_WPA_MSG
	mbedtls_ecp_keypair *eckey = mbedtls_pk_ec(key->pk);
	size_t prime_len = mbedtls_mpi_size(&eckey->grp.P);

	wpa_printf(MSG_DEBUG, "%s", title);

	if (eckey->d.n) {
		u8 *bin = NULL;
		char *str = NULL, *pos;
		bin = os_malloc(prime_len);
		str = os_malloc(prime_len * 2 + 1);
		if (!bin || !str)
			goto fail;
		if (mbedtls_mpi_write_binary(&eckey->d, bin, prime_len))
			goto fail;
		pos = str;
		for (int i=0; i <prime_len; i++) {
			pos = write_hex(pos, bin[i]);
		}
		*pos = 0;

		wpa_printf(MSG_DEBUG, "- Private: %s", str);
	fail:
		os_free(bin);
		os_free(str);
	}

	crypto_ec_point_debug_print((struct crypto_ec *)&eckey->grp,
				    (struct crypto_ec_point *)&eckey->Q,
				    "- Public ");
#endif
}


/**
 * struct crypto_csr - Certification Signing Request
 *
 * Internal data structure for CSR. The contents is specific to the used
 * crypto library.
 * For now it is assumed that only an EC public key can be used
 */
struct crypto_csr
{
};

/**
 * crypto_csr_init - Initialize empty CSR
 * Returns: Pointer to CSR data or %NULL on failure
 */
struct crypto_csr * crypto_csr_init(void)
{
	return NULL;
}

/**
 * crypto_csr_verify - Initialize CSR from CertificationRequest
 * @req: DER encoding of ASN.1 CertificationRequest
 *
 * Returns: Pointer to CSR data or %NULL on failure or if signature is invalid
 */
struct crypto_csr * crypto_csr_verify(const struct wpabuf *req)
{
	return NULL;
}

/**
 * crypto_csr_deinit - Free CSR structure
 * @csr: CSR structure from @crypto_csr_init() or crypto_csr_verify()
 */
void crypto_csr_deinit(struct crypto_csr *csr)
{
}

/**
 * crypto_csr_set_ec_public_key - Set public Key in CSR
 * @csr: CSR structure from @crypto_csr_init()
 * @key: EC Public key to set as Public key in the CSR
 * Returns: 0 on success, -1 on failure
 */
int crypto_csr_set_ec_public_key(struct crypto_csr *csr, struct crypto_ec_key *key)
{
	return -1;
}

/**
 * crypto_csr_set_name - Set name entry in CSR SubjectName
 * @csr: CSR structure from @crypto_csr_init()
 * @type: Name type  to add into the CSR SubjectName
 * @name: UTF-8 string to write in the CSR SubjectName
 * Returns: 0 on success, -1 on failure
 */
int crypto_csr_set_name(struct crypto_csr *csr, enum crypto_csr_name type,
			const char *name)
{
	return -1;
}

/**
 * crypto_csr_set_attribute - Set attribute in CSR
 * @csr: CSR structure from @crypto_csr_init()
 * @attr: Atribute identifier
 * @attr_type: ASN.1 type of @value buffer
 * @value: Attribute value
 * @len: length of @value buffer
 * Returns: 0 on success, -1 on failure
 */
int crypto_csr_set_attribute(struct crypto_csr *csr, enum crypto_csr_attr attr,
			     int attr_type, const u8 *value, size_t len)
{
	return -1;
}

/**
 * crypto_csr_get_attribute - Get attribute from CSR
 * @csr: CSR structure from @crypto_csr_verify()
 * @attr: Updated with atribute identifier
 * @len: Updated with length of returned buffer
 * @type: ASN.1 type of the attribute buffer
 * Returns: Type, length and Pointer on atrtibute value or %NULL on failure
 */
const u8 *crypto_csr_get_attribute(struct crypto_csr *csr,
				   enum crypto_csr_attr attr,
				   size_t *len, int *type)
{
	return NULL;
}

/**
 * crypto_csr_sign - Sign CSR and return ASN.1 CertificationRequest
 * @csr: CSR structure from @crypto_csr_init()
 * @key: Private key to sign the CSR (for now ony EC key are supported)
 * @algo: Hash algorithm to use for the signature
 * Returns: DER encoding of ASN.1 CertificationRequest for the CSR or %NULL on failure
 */
struct wpabuf *crypto_csr_sign(struct crypto_csr *csr, struct crypto_ec_key *key,
			       enum crypto_hash_alg algo)
{
	return NULL;
}

struct wpabuf *pkcs7_get_certificates(const struct wpabuf *pkcs7)
{
	return NULL;
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

	ret = mbedtls_mpi_exp_mod(&bn_result, &bn_base, &bn_exp, &bn_modulus,
	 &bn_rinv);
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

int crypto_dh_init(u8 generator, const u8 *prime, size_t prime_len, u8 *privkey, u8 *pubkey)
{
	size_t pubkey_len, pad;

	if (os_get_random(privkey, prime_len) < 0) {
	return -1;
	}
	if (os_memcmp(privkey, prime, prime_len) > 0) {
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
	os_memmove(pubkey + pad, pubkey, pubkey_len);
	os_memset(pubkey, 0, pad);
	}

	return 0;
}

int crypto_dh_derive_secret(u8 generator, const u8 *prime, size_t prime_len,
			    const u8 *order, size_t order_len,
			    const u8 *privkey, size_t privkey_len,
			    const u8 *pubkey, size_t pubkey_len,
			    u8 *secret, size_t *len)
{
#ifdef CONFIG_WPS
	mbedtls_mpi pub;
	int res = -1;

	if (pubkey_len > prime_len ||
	    (pubkey_len == prime_len &&
	     os_memcmp(pubkey, prime, prime_len) >= 0))
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
