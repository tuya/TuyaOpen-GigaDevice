/*
 * AES key unwrap (RFC3394)
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
 * aes_unwrap - Unwrap key with AES Key Wrap Algorithm (RFC3394)
 * @kek: Key encryption key (KEK)
 * @kek_len: Length of KEK in octets
 * @n: Length of the plaintext key in 64-bit units; e.g., 2 = 128-bit = 16
 * bytes
 * @cipher: Wrapped key to be unwrapped, (n + 1) * 64 bits
 * @plain: Plaintext key, n * 64 bits
 * Returns: 0 on success, -1 on failure (e.g., integrity verification failed)
 */
int aes_unwrap(const u8 *kek, size_t kek_len, int n, const u8 *cipher,
	       u8 *plain)
{
	u8 a[8], *r, b[AES_BLOCK_SIZE];
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
	ctx = aes_decrypt_init(kek, kek_len);
	if (ctx == NULL)
		return -1;
#endif
/* GD modify end */
	/* 1) Initialize variables. */
	os_memcpy(a, cipher, 8);
	r = plain;
	os_memcpy(r, cipher + 8, 8 * n);


	/* 2) Compute intermediate values.
	 * For j = 5 to 0
	 *     For i = n to 1
	 *         B = AES-1(K, (A ^ t) | R[i]) where t = n*j+i
	 *         A = MSB(64, B)
	 *         R[i] = LSB(64, B)
	 */
	for (j = 5; j >= 0; j--) {
		r = plain + (n - 1) * 8;
		for (i = n; i >= 1; i--) {
			os_memcpy(b, a, 8);
			t = n * j + i;
			b[7] ^= t;
			b[6] ^= t >> 8;
			b[5] ^= t >> 16;
			b[4] ^= t >> 24;

			os_memcpy(b + 8, r, 8);
/* GD modify */
#ifdef WPA_HW_SECURITY_ENGINE_ENABLE
			{
				cau_parameter_struct cau_param;

				cau_deinit();
				cau_struct_para_init(&cau_param);
				cau_param.alg_dir = CAU_DECRYPT;
				cau_param.key = (u8 *)kek;
				cau_param.key_size = kek_len * 8;
				cau_param.input = b;
				cau_param.in_length = AES_BLOCK_SIZE;
				HW_ACC_ENGINE_LOCK();
				ret = cau_aes_ecb(&cau_param, res);
				HW_ACC_ENGINE_UNLOCK();
			}

			os_memcpy(a, res, 8);
			os_memcpy(r, res + 8, 8);
#else
/* GD modify end*/
			aes_decrypt(ctx, b, b);
			os_memcpy(a, b, 8);
			os_memcpy(r, b + 8, 8);
#endif /* GD modify */
			r -= 8;
		}
	}

#ifndef WPA_HW_SECURITY_ENGINE_ENABLE /* GD modify */
	aes_decrypt_deinit(ctx);
#endif /* GD modify */
	/* 3) Output results.
	 *
	 * These are already in @plain due to the location of temporary
	 * variables. Just verify that the IV matches with the expected value.
	 */
	for (i = 0; i < 8; i++) {
		if (a[i] != 0xa6)
			return -1;
	}
/* GD modify */
#ifdef WPA_HW_SECURITY_ENGINE_ENABLE
	return (ret == ERROR) ? -1 : 0;
#else
/* GD modify end */
	return 0;
#endif /* GD modify */
}
