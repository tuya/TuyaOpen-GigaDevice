/*
 * AES Key Wrap Algorithm (RFC3394)
 *
 * Copyright (c) 2003-2007, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2025, GigaDevice Semiconductor Inc.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "gd32vw55x_cau.h" /* GD modify */
#include "includes.h"

#include "common.h"
#include "aes.h"
#include "aes_wrap.h"

/**
 * aes_wrap - Wrap keys with AES Key Wrap Algorithm (RFC3394)
 * @kek: Key encryption key (KEK)
 * @kek_len: Length of KEK in octets
 * @n: Length of the plaintext key in 64-bit units; e.g., 2 = 128-bit = 16
 * bytes
 * @plain: Plaintext key to be wrapped, n * 64 bits
 * @cipher: Wrapped key, (n + 1) * 64 bits
 * Returns: 0 on success, -1 on failure
 */
int aes_wrap(const u8 *kek, size_t kek_len, int n, const u8 *plain, u8 *cipher)
{
	u8 *a, *r, b[AES_BLOCK_SIZE];
	int i, j;
/* GD modify */
	unsigned int t;
#ifdef WPA_HW_SECURITY_ENGINE_ENABLE
	u8 res[AES_BLOCK_SIZE]; // for output
	int ret = ERROR;
#else
/* GD modify end */
	void *ctx;
/* GD modify */
	ctx = aes_encrypt_init(kek, kek_len);
	if (ctx == NULL)
		return -1;
#endif
/* GD modify end*/
	a = cipher;
	r = cipher + 8;

	/* 1) Initialize variables. */
	os_memset(a, 0xa6, 8);
	os_memcpy(r, plain, 8 * n);

	/* 2) Calculate intermediate values.
	 * For j = 0 to 5
	 *     For i=1 to n
	 *         B = AES(K, A | R[i])
	 *         A = MSB(64, B) ^ t where t = (n*j)+i
	 *         R[i] = LSB(64, B)
	 */
	for (j = 0; j <= 5; j++) {
		r = cipher + 8;
		for (i = 1; i <= n; i++) {
			os_memcpy(b, a, 8);
			os_memcpy(b + 8, r, 8);
/* GD modify */
#ifdef WPA_HW_SECURITY_ENGINE_ENABLE
			{
				cau_parameter_struct cau_param;

				cau_deinit();
				cau_struct_para_init(&cau_param);
				cau_param.key = (u8 *)kek;
				cau_param.key_size = kek_len * 8;
				cau_param.input = b;
				cau_param.in_length = AES_BLOCK_SIZE;
				HW_ACC_ENGINE_LOCK();
				ret = cau_aes_ecb(&cau_param, res);
				HW_ACC_ENGINE_UNLOCK();
			}

			os_memcpy(a, res, 8);
			t = n * j + i;
			a[7] ^= t;
			a[6] ^= t >> 8;
			a[5] ^= t >> 16;
			a[4] ^= t >> 24;
			os_memcpy(r, res + 8, 8);
#else
/* GD modify end */
			aes_encrypt(ctx, b, b);
			os_memcpy(a, b, 8);
			t = n * j + i;
			a[7] ^= t;
			a[6] ^= t >> 8;
			a[5] ^= t >> 16;
			a[4] ^= t >> 24;
			os_memcpy(r, b + 8, 8);
#endif /* GD modify */
			r += 8;
		}
	}
/* GD modify */
#ifdef WPA_HW_SECURITY_ENGINE_ENABLE
	return (ret == ERROR) ? -1 : 0;
#else
/* GD modify end */
	aes_encrypt_deinit(ctx);

	/* 3) Output the results.
	 *
	 * These are already in @cipher due to the location of temporary
	 * variables.
	 */

	return 0;
#endif /* GD modify */
}
