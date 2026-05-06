/*!
    \file    wpas_crypto.h
    \brief   Header file for wpas crypto.

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

#ifndef _WPAS_CRYPTO_H_
#define _WPAS_CRYPTO_H_

#define ETHER_ADDRLEN               6
#define PMK_EXPANSION_CONST         "Pairwise key expansion"
#define PMK_EXPANSION_CONST_SIZE    22
#define PMKID_NAME_CONST            "PMK Name"
#define PMKID_NAME_CONST_SIZE       8
#define GMK_EXPANSION_CONST         "Group key expansion"
#define GMK_EXPANSION_CONST_SIZE    19
#define RANDOM_EXPANSION_CONST      "Init Counter"
#define RANDOM_EXPANSION_CONST_SIZE 12
#define PTK_LEN_CCMP                48

#define SHA256_BLOCK_LEN            64
#define SHA512_MAC_LEN              64
#define MD5_MAC_LEN                 16
#define SHA384_MAC_LEN              48
#define SHA1_MAC_LEN                20

#define LargeIntegerOverflow(x)     ((x.field.HighPart == 0xffffffff) && \
                                    (x.field.LowPart == 0xffffffff))
#define LargeIntegerZero(x)         sys_memset(&x.charData, 0, 8);

#define Octet16IntegerOverflow(x)   (LargeIntegerOverflow(x.field.HighPart) && \
                                    LargeIntegerOverflow(x.field.LowPart))
#define Octet16IntegerZero(x)       sys_memset(&x.charData, 0, 16);

//#define SetNonce(ocDst, oc32Counter) set_eapol_key_iv(ocDst, oc32Counter)

#define MP_OKAY 0

#ifdef CONFIG_WPA3_SAE
/* Ecp point compression type */
#define ECC_POINT_COMP_EVEN         0x02
#define ECC_POINT_COMP_ODD          0x03
#define ECC_POINT_UNCOMP            0x04
/* ---> Basic Manipulations <--- */
#define mp_iszero(a) ((mbedtls_mpi_size(a) == 0) ? 1 : 0)
#define mp_isone(a)  ((((mbedtls_mpi_size(a) == 1)) && ((a)->p[0] == 1u)) ? 1 : 0)
#define mp_iseven(a) ((mbedtls_mpi_size(a) > 0 && (((a)->p[0] & 1u) == 0u)) ? 1 : 0)
#define mp_isodd(a)  ((mbedtls_mpi_size(a) > 0 && (((a)->p[0] & 1u) == 1u)) ? 1 : 0)
#endif

/***************  SW  *********************/
#ifdef CONFIG_OWE
struct crypto_ecdh * crypto_ecdh_init(int group);
void crypto_ecdh_deinit(struct crypto_ecdh *ecdh);
uint8_t *crypto_ecdh_get_pubkey(struct crypto_ecdh *ecdh, int inc_y, size_t *pub_len);
uint8_t * crypto_ecdh_set_peerkey(struct crypto_ecdh *ecdh, int inc_y,
                                const uint8_t *key, size_t len, size_t *srec_len);
#endif

enum crypto_cipher_alg {
    CRYPTO_CIPHER_NULL = 0, CRYPTO_CIPHER_ALG_AES, CRYPTO_CIPHER_ALG_3DES,
    CRYPTO_CIPHER_ALG_DES, CRYPTO_CIPHER_ALG_RC2, CRYPTO_CIPHER_ALG_RC4
};

enum crypto_hash_alg {
    CRYPTO_HASH_ALG_MD5, CRYPTO_HASH_ALG_SHA1,
    CRYPTO_HASH_ALG_HMAC_MD5, CRYPTO_HASH_ALG_HMAC_SHA1,
    CRYPTO_HASH_ALG_SHA256, CRYPTO_HASH_ALG_HMAC_SHA256,
    CRYPTO_HASH_ALG_SHA384, CRYPTO_HASH_ALG_SHA512
};

struct sha256_state {
    uint64_t length;
    uint32_t state[8], curlen;
    uint8_t buf[SHA256_BLOCK_LEN];
};

void sha256_init(struct sha256_state *md);
int sha256_process(struct sha256_state *md, const unsigned char *in,
            unsigned long inlen);
int sha256_done(struct sha256_state *md, unsigned char *out);
int tls_prf_sha256(const uint8_t *secret, size_t secret_len, const char *label,
                    const uint8_t *seed, size_t seed_len, uint8_t *out, size_t outlen);

struct SHA1Context {
    uint32_t state[5];
    uint32_t count[2];
    unsigned char buffer[64];
};

typedef struct SHA1Context SHA1_CTX;

void SHA1Init(SHA1_CTX* context);
void SHA1Update(SHA1_CTX* context, const void *_data, uint32_t len);
void SHA1Final(unsigned char digest[20], SHA1_CTX* context);
void SHA1Transform(uint32_t state[5], const unsigned char buffer[64]);
struct MD5Context {
    uint32_t buf[4];
    uint32_t bits[2];
    uint8_t in[64];
};

void MD5Init(struct MD5Context *ctx);
void MD5Update(struct MD5Context *ctx, unsigned char const *buf, unsigned len);
void MD5Final(unsigned char digest[16], struct MD5Context *ctx);
void * __hide_aliasing_typecast(void *foo);

#define aliasing_hide_typecast(a,t) (t *) __hide_aliasing_typecast((a))
int tls_prf_sha1_md5(const uint8_t *secret, size_t secret_len, const char *label,
                    const uint8_t *seed, size_t seed_len, uint8_t *out, size_t outlen);
int md5_vector(size_t num_elem, const uint8_t *addr[], const size_t *len, uint8_t *mac);
int sha1_vector(size_t num_elem, const uint8_t *addr[], const size_t *len, uint8_t *mac);

#define SHA512_BLOCK_SIZE 128

struct sha512_state {
    uint64_t length, state[8];
    uint32_t curlen;
    uint8_t  buf[SHA512_BLOCK_SIZE];
};

void sha512_init(struct sha512_state *md);
int sha512_process(struct sha512_state *md, const unsigned char *in,
                    unsigned long inlen);
int sha512_done(struct sha512_state *md, unsigned char *out);
int sha512_vector(size_t num_elem, const uint8_t *addr[], const size_t *len, uint8_t *mac);

#define SHA384_BLOCK_SIZE SHA512_BLOCK_SIZE

#define sha384_state sha512_state

int sha384_vector(size_t num_elem, const uint8_t *addr[], const size_t *len, uint8_t *mac);
int hmac_sha384(const uint8_t *key, size_t key_len, const uint8_t *data,
                size_t data_len, uint8_t *mac);
int sha384_prf(const uint8_t *key, size_t key_len, const char *label,
                const uint8_t *data, size_t data_len, uint8_t *buf, size_t buf_len);
int hmac_sha384_kdf(const uint8_t *secret, size_t secret_len,
                    const char *label, const uint8_t *seed, size_t seed_len,
                    uint8_t *out, size_t outlen);

struct des3_key_s {
    uint32_t ek[3][32];
    uint32_t dk[3][32];
};

void des_block_encrypt(const uint8_t *plain, const uint32_t *ek, uint8_t *crypt);
void des_block_decrypt(const uint8_t *crypt, const uint32_t *dk, uint8_t *plain);
void des_key_setup(const uint8_t *key, uint32_t *ek, uint32_t *dk);

void des3_decrypt(const uint8_t *crypt, const struct des3_key_s *key, uint8_t *plain);
void des3_encrypt(const uint8_t *plain, const struct des3_key_s *key, uint8_t *crypt);
void des3_key_setup(const uint8_t *key, struct des3_key_s *dkey);

struct crypto_hash * crypto_hash_init(enum crypto_hash_alg alg, const uint8_t *key, size_t key_len);
void crypto_hash_update(struct crypto_hash *ctx, const uint8_t *data, size_t len);
int crypto_hash_finish(struct crypto_hash *ctx, uint8_t *mac, size_t *len);

/*****************         HW  ************/

int hmac_sha1(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len,
            uint8_t *mac);
int hmac_sha1_vector(const uint8_t *key, size_t key_len, size_t num_elem,
                    const uint8_t *addr[], const size_t *len, uint8_t *mac);
int sha1_prf(const uint8_t *key, size_t key_len, const char *label,
            const uint8_t *data, size_t data_len, uint8_t *buf, size_t buf_len);
int pbkdf2_sha1(const char *passphrase, const uint8_t *ssid, size_t ssid_len,
        int iterations, uint8_t *buf, size_t buflen);

int hmac_md5(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len,
            uint8_t *mac);
int hmac_md5_vector(const uint8_t *key, size_t key_len, size_t num_elem,
                    const uint8_t *addr[], const size_t *len, uint8_t *mac);

int hmac_sha256(const uint8_t *key, size_t key_len, const uint8_t *data,
                size_t data_len, uint8_t *mac);
int hmac_sha256_vector(const uint8_t *key, size_t key_len, size_t num_elem,
                        const uint8_t *addr[], const size_t *len, uint8_t *mac);
int hmac_sha256_kdf(const uint8_t *secret, size_t secret_len,
                    const char *label, const uint8_t *seed, size_t seed_len,
                    uint8_t *out, size_t outlen);
int sha256_prf(const uint8_t *key, size_t key_len, const char *label,
                const uint8_t *data, size_t data_len, uint8_t *buf, size_t buf_len);
int sha256_vector(size_t num_elem, const uint8_t *addr[], const size_t *len, uint8_t *mac);
int sha256_prf_bits(const uint8_t *key, size_t key_len, const char *label,
            const uint8_t *data, size_t data_len, uint8_t *buf,
            size_t buf_len_bits);

int rc4_skip(const uint8_t *key, size_t keylen, size_t skip,
         uint8_t *data, size_t data_len);

#endif /* _WPAS_CRYPTO_H_ */
