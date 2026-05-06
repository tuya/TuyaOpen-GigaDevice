/*
 * SAE-PK password/modifier generator
 * Copyright (c) 2020, The Linux Foundation
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "utils/includes.h"

#include "utils/common.h"
#include "utils/base64.h"
#include "crypto/crypto.h"
#include "common/sae.h"

#ifdef CONFIG_SAE_PK
uint8_t* sae_pk_key_gen(int group, int *len)
{
	struct crypto_ec_key *key = NULL;
	struct wpabuf *priv = NULL;
	uint8_t *key_der = NULL;

	*len = 0;
	key = crypto_ec_key_gen(group);
	if (!key)
		return NULL;

	priv = crypto_ec_key_get_ecprivate_key(key, false);
	if (!priv)
		goto end;

	key_der = os_malloc(wpabuf_len(priv));
	if (!key_der)
		goto end;

	*len = wpabuf_len(priv);
	os_memcpy(key_der, wpabuf_head(priv), wpabuf_len(priv));
end:
	wpabuf_free(priv);
	crypto_ec_key_deinit(key);
	if (!key_der)
		wpa_printf(MSG_ERROR, "Failed to generate key");
	return key_der;
}

int sae_pk_password_gen(const uint8_t *der, size_t der_len,
			uint8_t *modifier, size_t modifier_len,
			uint8_t *ssid, int ssid_len,
			int sec, int nb_part,
			char *password, size_t password_len)
{
	struct crypto_ec_key *key = NULL;
	struct wpabuf *pub = NULL;
	uint8_t *data = NULL, *m;
	size_t data_len;
	int j;
	int ret = -1;
	uint8_t hash[SAE_MAX_HASH_LEN];
	char hash_hex[2 * SAE_MAX_HASH_LEN + 1];
	uint8_t pw_base_bin[SAE_MAX_HASH_LEN];
	uint8_t *dst;
	int group;
	size_t hash_len, max_part;
	unsigned long long i, expected;
	u32 sec_1b;
	char *pw = NULL;

	if (modifier_len < SAE_PK_M_LEN)
		return -1;

	// only possible value for sec are 3 or 5
	if (sec != 5)
		sec = 3;
	sec_1b = sec == 3;

	expected = 1;
	for (j = 0; j < sec; j++)
		expected *= 256;

	key = crypto_ec_key_parse_priv((const uint8_t *)der, der_len);
	if (!key)
		goto fail;
	pub = crypto_ec_key_get_subject_public_key(key);
	if (!pub)
		goto fail;

	group = crypto_ec_key_group(key);
	switch (group) {
 	case 19:
		hash_len = 32;
		break;
	case 20:
#ifndef CONFIG_SHA384
		wpa_printf(MSG_ERROR, "Missing CONFIG_SHA384 option for group 20");
#endif
		hash_len = 48;
		break;
	case 21:
#ifndef CONFIG_SHA512
		wpa_printf(MSG_ERROR, "Missing CONFIG_SHA512 option for group 21");
#endif
		hash_len = 64;
		break;
	default:
		goto fail;
	}
	max_part = ((hash_len - sec) * 8 + 5) / 19;
	if (nb_part < 3)
		nb_part = 3;
	else if (nb_part > max_part)
		nb_part = max_part;

	// limit number of part in the password to fit in provided buffer
	while (password_len < (5 * nb_part))
	{
		nb_part--;
		if (nb_part < 3)
			goto fail;
	}

	data_len = ssid_len + SAE_PK_M_LEN + wpabuf_len(pub);
	data = os_malloc(data_len);
	if (!data)
		goto fail;

	os_memcpy(data, ssid, ssid_len);
	m = data + ssid_len;

	// re-use provider modifier value unless it is only 0
	for (i = 0; i < SAE_PK_M_LEN; i++) {
		if (modifier[i])
			break;
	}
	if (i < SAE_PK_M_LEN)
		os_memcpy(m, modifier, SAE_PK_M_LEN);
	else if (os_get_random(m, SAE_PK_M_LEN) < 0)
		goto fail;
	os_memcpy(m + SAE_PK_M_LEN, wpabuf_head(pub), wpabuf_len(pub));

	for (i = 0;; i++) {
		if (sae_hash(hash_len, data, data_len, hash) < 0)
			goto fail;

		for (j = 0; j < sec; j++) {
			if (hash[j])
				break;
		}
		if (j == sec)
			break;
		inc_byte_array(m, SAE_PK_M_LEN);
	}
	os_memcpy(modifier, m, SAE_PK_M_LEN);

	/* Skip 8*Sec bits and add Sec_1b as the every 20th bit starting with
	 * one. */
	os_memset(pw_base_bin, 0, sizeof(pw_base_bin));
	dst = pw_base_bin;
	for (j = 0; j < 8 * (int) hash_len / 20; j++) {
		u32 val20;
		val20 = sae_pk_get_be19(hash + sec);
		val20 |= sec_1b << 19;
		sae_pk_buf_shift_left_19(hash + sec, hash_len - sec);

		if (j & 1) {
			*dst |= (val20 >> 16) & 0x0f;
			dst++;
			*dst++ = (val20 >> 8) & 0xff;
			*dst++ = val20 & 0xff;
		} else {
			*dst++ = (val20 >> 12) & 0xff;
			*dst++ = (val20 >> 4) & 0xff;
			*dst = (val20 << 4) & 0xf0;
		}
	}

	pw = sae_pk_base32_encode(pw_base_bin, 20 * nb_part - 5);
	if (!pw)
		goto fail;
	if (os_strlen(pw) >= password_len)
		goto fail;
	os_memcpy(password, pw, os_strlen(pw) + 1);

	ret = 0;
fail:
	wpabuf_free(pub);
	crypto_ec_key_deinit(key);
	os_free(data);
	os_free(pw);

	if (ret)
		wpa_printf(MSG_ERROR, "Failed to generate the password");

	return ret;
}

int sae_pk_password_write(const char *password, const uint8_t *priv_key, size_t priv_key_len,
			  const uint8_t *modifier, char *cfg_str, size_t cfg_len)
{
	char *priv_key_b64 = NULL;
	char m_hex[2 * SAE_PK_M_LEN + 1];
	int res;

	priv_key_b64 = base64_encode_no_lf(priv_key, priv_key_len, NULL);
	if (!priv_key_b64)
		return -1;

	wpa_snprintf_hex(m_hex, sizeof(m_hex), modifier, SAE_PK_M_LEN);
	res = os_snprintf(cfg_str, cfg_len, "sae_password \"%s\";sae_password_pk %s:%s;",
			  password, m_hex, priv_key_b64);

	os_free(priv_key_b64);

	if (res >= cfg_len)
		return -1;
	return res;
}
#endif /* CONFIG_SAE_PK */
