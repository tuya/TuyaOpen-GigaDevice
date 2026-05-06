#ifndef SAE_PK_GEN_H
#define SAE_PK_GEN_H

/**
 * sae_pk_key_gen() - Generate a new Private Key for SAE PK
 * @group: IKE group to use for the key.
 * @len: Pointer updated with the length of the key allocated.
 * Returns: Buffer containing DER encoding of ASN.1 ECPrivateKey (without
 * the public key). It is caller responsibility to free the buffer.
 */
uint8_t* sae_pk_key_gen(int group, int *len);

/**
 * sae_pk_password_gen() - Generate a SAE PK password for the giver Key/SSID
 * @der: DER encoding of ASN.1 ECPrivateKey
 * @der_len: Length, in byte, of der buffer
 * @modifier: Modifier value (in big endian)
 * @modifier_len: Size of the modifier value (should be SAE_PK_M_LEN)
 * @ssid: SSID
 * @ssid_len: Length, in bytes, of the ssid buffer
 * @sec: 'Sec' parameter for the password. Possible value are 3 and 5.
 * Invalid value are treated as 3.
 * @nb_part: Number of group of 4 character in the password. Minimum value is 3
 * (and lower value are treated as 3). Maximum value depend of the key curve.
 * @pw: Buffer where to write the password
 * @pw_len: Size of the pw buffer. Should be big enough to store nb_part (+
 * terminating null byte). If buffer is too small for the requested nb_part then
 * nb_part is automatically reduced to fit in the buffer (as long as it remains >= 3)
 * Returns 0 on success and !=0 otherwise
 */
int sae_pk_password_gen(const uint8_t *der, size_t der_len,
			uint8_t *modifier, size_t modifier_len,
			uint8_t *ssid, int ssid_len,
			int sec, int nb_part,
			char *pw, size_t pw_len);

/**
 * sae_pk_password_write() - Write config for sae pk (as AP) in a buffer
 * @password: SAE-PK password
 * @priv_key: DER encoding of ASN.1 ECPrivateKey
 * @priv_key_len: Length, in byte, of priv_key buffer
 * @modifier: Modifier value (in big endian, padded to SAE_PK_M_LEN)
 * @cfg_str: Buffer in which configuration must be written
 * @cfg_len: Length, in byte, of cfg_str buffer
 * Returns The size of bytes written in the buffer, and -1 in case of error
 * (including when the buffer is not big enough)
 */
int sae_pk_password_write(const char *password, const uint8_t *priv_key, size_t priv_key_len,
			  const uint8_t *modifier, char *cfg_str, size_t cfg_len);
#endif // SAE_PK_KEY_GEN

