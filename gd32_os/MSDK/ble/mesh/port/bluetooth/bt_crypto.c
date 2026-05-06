/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mesh_cfg.h"
#include <string.h>
#include "mesh_errno.h"

#include "mesh_kernel.h"
#include "sys/byteorder.h"
#include "sys/slist.h"
#include "sys/mesh_atomic.h"

#include "bluetooth/mesh_bluetooth.h"

#include "bluetooth/bt_crypto.h"

#include <tinycrypt/constants.h>
#include <tinycrypt/hmac_prng.h>
#include <tinycrypt/aes.h>
#include <tinycrypt/utils.h>

#include "bluetooth/bt_str.h"
#include "wrapper_os.h"

#define LOG_LEVEL CONFIG_BT_MESH_CRYPTO_LOG_LEVEL
#include "api/mesh_log.h"

int bt_rand(void *buf, size_t len)
{
	return sys_random_bytes_get(buf, len);
}


int bt_encrypt_le(const uint8_t key[16], const uint8_t plaintext[16],
		  uint8_t enc_data[16])
{
	struct tc_aes_key_sched_struct s;
	uint8_t tmp[16];

	if(key == NULL || plaintext == NULL || enc_data == NULL) {
		return -EINVAL;
	}

	LOG_DBG("key %s", bt_hex(key, 16));
	LOG_DBG("plaintext %s", bt_hex(plaintext, 16));

	sys_memcpy_swap(tmp, key, 16);

	if (tc_aes128_set_encrypt_key(&s, tmp) == TC_CRYPTO_FAIL) {
		return -EINVAL;
	}

	sys_memcpy_swap(tmp, plaintext, 16);

	if (tc_aes_encrypt(enc_data, tmp, &s) == TC_CRYPTO_FAIL) {
		return -EINVAL;
	}

	sys_mem_swap(enc_data, 16);

	LOG_DBG("enc_data %s", bt_hex(enc_data, 16));

	return 0;
}

int bt_encrypt_be(const uint8_t key[16], const uint8_t plaintext[16],
		  uint8_t enc_data[16])
{
	struct tc_aes_key_sched_struct s;

	if(key == NULL || plaintext == NULL || enc_data == NULL) {
		return -EINVAL;
	}

	LOG_DBG("key %s", bt_hex(key, 16));
	LOG_DBG("plaintext %s", bt_hex(plaintext, 16));

	if (tc_aes128_set_encrypt_key(&s, key) == TC_CRYPTO_FAIL) {
		return -EINVAL;
	}

	if (tc_aes_encrypt(enc_data, plaintext, &s) == TC_CRYPTO_FAIL) {
		return -EINVAL;
	}

	LOG_DBG("enc_data %s", bt_hex(enc_data, 16));

	return 0;
}

