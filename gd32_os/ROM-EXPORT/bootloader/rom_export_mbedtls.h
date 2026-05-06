/*!
    \file    rom_export_mbedtls.h
    \brief   Rom MbedTLS export file for GD32VW55x SDK

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

#ifndef __ROM_EXPORT_MBEDTLS_H__
#define __ROM_EXPORT_MBEDTLS_H__

#include "mbedtls/md5.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/md.h"
#include "mbedtls/cipher.h"
#include "mbedtls/dhm.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/ecp.h"
#include "mbedtls/pk.h"
#include "mbedtls/ecp.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/bignum.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/base64.h"
#include "mbedtls/platform.h"
#include "mbedtls/memory_buffer_alloc.h"

#ifndef EXTERN
#define EXTERN  extern
#endif

#define CONFIG_ASIC_CUT_AUTO  /* If undefined, please use rom_symbol.gcc instead of rom_symbol_m.gcc and only support >=bcut */

#ifdef CONFIG_ASIC_CUT_AUTO
EXTERN mbedtls_ecp_curve_info *ecp_supported_curves;
EXTERN void (*mbedtls_ecdsa_free_fn)( mbedtls_ecdsa_context *ctx );
EXTERN int (*mbedtls_ecdsa_from_keypair_fn)( mbedtls_ecdsa_context *ctx,
                                const mbedtls_ecp_keypair *key );
EXTERN int (*mbedtls_ecdsa_genkey_fn)( mbedtls_ecdsa_context *ctx, mbedtls_ecp_group_id gid,
                  int (*f_rng)(void *, unsigned char *, size_t), void *p_rng );
EXTERN void (*mbedtls_ecdsa_init_fn)( mbedtls_ecdsa_context *ctx );
EXTERN int (*mbedtls_ecdsa_read_signature_fn)( mbedtls_ecdsa_context *ctx,
                          const unsigned char *hash, size_t hlen,
                          const unsigned char *sig, size_t slen );
EXTERN int (*mbedtls_ecdsa_read_signature_restartable_fn)( mbedtls_ecdsa_context *ctx,
                          const unsigned char *hash, size_t hlen,
                          const unsigned char *sig, size_t slen,
                          mbedtls_ecdsa_restart_ctx *rs_ctx );
EXTERN int (*mbedtls_ecdsa_sign_fn)( mbedtls_ecp_group *grp, mbedtls_mpi *r, mbedtls_mpi *s,
                const mbedtls_mpi *d, const unsigned char *buf, size_t blen,
                int (*f_rng)(void *, unsigned char *, size_t), void *p_rng );
EXTERN int (*mbedtls_ecdsa_sign_det_fn)( mbedtls_ecp_group *grp, mbedtls_mpi *r,
                            mbedtls_mpi *s, const mbedtls_mpi *d,
                            const unsigned char *buf, size_t blen,
                            mbedtls_md_type_t md_alg );
EXTERN int (*mbedtls_ecdsa_verify_fn)( mbedtls_ecp_group *grp,
                          const unsigned char *buf, size_t blen,
                          const mbedtls_ecp_point *Q, const mbedtls_mpi *r,
                          const mbedtls_mpi *s);
EXTERN int (*mbedtls_ecdsa_write_signature_fn)( mbedtls_ecdsa_context *ctx,
                                   mbedtls_md_type_t md_alg,
                           const unsigned char *hash, size_t hlen,
                           unsigned char *sig, size_t *slen,
                           int (*f_rng)(void *, unsigned char *, size_t),
                           void *p_rng );
EXTERN int (*mbedtls_ecdsa_write_signature_det_fn)( mbedtls_ecdsa_context *ctx,
                               const unsigned char *hash, size_t hlen,
                               unsigned char *sig, size_t *slen,
                               mbedtls_md_type_t md_alg );
EXTERN int (*mbedtls_ecdsa_write_signature_restartable_fn)( mbedtls_ecdsa_context *ctx,
                           mbedtls_md_type_t md_alg,
                           const unsigned char *hash, size_t hlen,
                           unsigned char *sig, size_t *slen,
                           int (*f_rng)(void *, unsigned char *, size_t),
                           void *p_rng,
                           mbedtls_ecdsa_restart_ctx *rs_ctx );
EXTERN int (*mbedtls_ecp_check_privkey_fn)( const mbedtls_ecp_group *grp,
                               const mbedtls_mpi *d );
EXTERN int (*mbedtls_ecp_check_pub_priv_fn)( const mbedtls_ecp_keypair *pub,
                                const mbedtls_ecp_keypair *prv );
EXTERN int (*mbedtls_ecp_check_pubkey_fn)( const mbedtls_ecp_group *grp,
                              const mbedtls_ecp_point *pt );
EXTERN int (*mbedtls_ecp_copy_fn)( mbedtls_ecp_point *P, const mbedtls_ecp_point *Q );
EXTERN const mbedtls_ecp_curve_info * (*mbedtls_ecp_curve_info_from_grp_id_fn)( mbedtls_ecp_group_id grp_id );
EXTERN const mbedtls_ecp_curve_info * (*mbedtls_ecp_curve_info_from_name_fn)( const char *name );
EXTERN const mbedtls_ecp_curve_info * (*mbedtls_ecp_curve_info_from_tls_id_fn)( uint16_t tls_id );
EXTERN const mbedtls_ecp_curve_info * (*mbedtls_ecp_curve_list_fn)( void );
EXTERN int (*mbedtls_ecp_gen_key_fn)( mbedtls_ecp_group_id grp_id, mbedtls_ecp_keypair *key,
                         int (*f_rng)(void *, unsigned char *, size_t),
                         void *p_rng );
EXTERN int (*mbedtls_ecp_gen_keypair_fn)( mbedtls_ecp_group *grp, mbedtls_mpi *d,
                             mbedtls_ecp_point *Q,
                             int (*f_rng)(void *, unsigned char *, size_t),
                             void *p_rng );
EXTERN int (*mbedtls_ecp_gen_keypair_base_fn)( mbedtls_ecp_group *grp,
                                  const mbedtls_ecp_point *G,
                                  mbedtls_mpi *d, mbedtls_ecp_point *Q,
                                  int (*f_rng)(void *, unsigned char *, size_t),
                                  void *p_rng );
EXTERN int (*mbedtls_ecp_gen_privkey_fn)( const mbedtls_ecp_group *grp,
                     mbedtls_mpi *d,
                     int (*f_rng)(void *, unsigned char *, size_t),
                     void *p_rng );
EXTERN int (*mbedtls_ecp_group_copy_fn)( mbedtls_ecp_group *dst,
                            const mbedtls_ecp_group *src );
EXTERN void (*mbedtls_ecp_group_free_fn)( mbedtls_ecp_group *grp );
EXTERN void (*mbedtls_ecp_group_init_fn)( mbedtls_ecp_group *grp );
EXTERN const mbedtls_ecp_group_id * (*mbedtls_ecp_grp_id_list_fn)( void );
EXTERN int (*mbedtls_ecp_is_zero_fn)( mbedtls_ecp_point *pt );
EXTERN void (*mbedtls_ecp_keypair_free_fn)( mbedtls_ecp_keypair *key );
EXTERN void (*mbedtls_ecp_keypair_init_fn)( mbedtls_ecp_keypair *key );
EXTERN int (*mbedtls_ecp_mul_fn)( mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
             const mbedtls_mpi *m, const mbedtls_ecp_point *P,
             int (*f_rng)(void *, unsigned char *, size_t), void *p_rng );
EXTERN int (*mbedtls_ecp_mul_restartable_fn)( mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
             const mbedtls_mpi *m, const mbedtls_ecp_point *P,
             int (*f_rng)(void *, unsigned char *, size_t), void *p_rng,
             mbedtls_ecp_restart_ctx *rs_ctx );
EXTERN int (*mbedtls_ecp_muladd_fn)( mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
             const mbedtls_mpi *m, const mbedtls_ecp_point *P,
             const mbedtls_mpi *n, const mbedtls_ecp_point *Q );
EXTERN int (*mbedtls_ecp_muladd_restartable_fn)(
             mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
             const mbedtls_mpi *m, const mbedtls_ecp_point *P,
             const mbedtls_mpi *n, const mbedtls_ecp_point *Q,
             mbedtls_ecp_restart_ctx *rs_ctx );
EXTERN int (*mbedtls_ecp_point_cmp_fn)( const mbedtls_ecp_point *P,
                           const mbedtls_ecp_point *Q );
EXTERN void (*mbedtls_ecp_point_free_fn)( mbedtls_ecp_point *pt );
EXTERN void (*mbedtls_ecp_point_init_fn)( mbedtls_ecp_point *pt );
EXTERN int (*mbedtls_ecp_point_read_binary_fn)( const mbedtls_ecp_group *grp,
                                   mbedtls_ecp_point *P,
                                   const unsigned char *buf, size_t ilen );
EXTERN int (*mbedtls_ecp_point_read_string_fn)( mbedtls_ecp_point *P, int radix,
                           const char *x, const char *y );
EXTERN int (*mbedtls_ecp_point_write_binary_fn)( const mbedtls_ecp_group *grp, const mbedtls_ecp_point *P,
                            int format, size_t *olen,
                            unsigned char *buf, size_t buflen );
EXTERN int (*mbedtls_ecp_set_zero_fn)( mbedtls_ecp_point *pt );
EXTERN int (*mbedtls_ecp_tls_read_group_fn)( mbedtls_ecp_group *grp,
                                const unsigned char **buf, size_t len );
EXTERN int (*mbedtls_ecp_tls_read_group_id_fn)( mbedtls_ecp_group_id *grp,
                                   const unsigned char **buf,
                                   size_t len );
EXTERN int (*mbedtls_ecp_tls_read_point_fn)( const mbedtls_ecp_group *grp,
                                mbedtls_ecp_point *pt,
                                const unsigned char **buf, size_t len );
EXTERN int (*mbedtls_ecp_tls_write_group_fn)( const mbedtls_ecp_group *grp,
                                 size_t *olen,
                                 unsigned char *buf, size_t blen );
EXTERN int (*mbedtls_ecp_tls_write_point_fn)( const mbedtls_ecp_group *grp,
                                 const mbedtls_ecp_point *pt,
                                 int format, size_t *olen,
                                 unsigned char *buf, size_t blen );
EXTERN int (*mbedtls_internal_md5_process_fn)( mbedtls_md5_context *ctx,
                                  const unsigned char data[64] );
EXTERN int (*mbedtls_internal_sha256_process_fn)( mbedtls_sha256_context *ctx,
                                     const unsigned char data[64] );
EXTERN void (*mbedtls_md5_fn)( const unsigned char *input,
                                     size_t ilen,
                                     unsigned char output[16] );
EXTERN void (*mbedtls_md5_clone_fn)( mbedtls_md5_context *dst,
                        const mbedtls_md5_context *src );
EXTERN void (*mbedtls_md5_finish_fn)( mbedtls_md5_context *ctx,
                                            unsigned char output[16] );
EXTERN int (*mbedtls_md5_finish_ret_fn)( mbedtls_md5_context *ctx,
                            unsigned char output[16] );
EXTERN void (*mbedtls_md5_free_fn)( mbedtls_md5_context *ctx );
EXTERN void (*mbedtls_md5_init_fn)( mbedtls_md5_context *ctx );
EXTERN void (*mbedtls_md5_process_fn)( mbedtls_md5_context *ctx,
                                             const unsigned char data[64] );
EXTERN int (*mbedtls_md5_ret_fn)( const unsigned char *input,
                     size_t ilen,
                     unsigned char output[16] );
EXTERN void (*mbedtls_md5_starts_fn)( mbedtls_md5_context *ctx );
EXTERN int (*mbedtls_md5_starts_ret_fn)( mbedtls_md5_context *ctx );
EXTERN void (*mbedtls_md5_update_fn)( mbedtls_md5_context *ctx,
                                            const unsigned char *input,
                                            size_t ilen );
EXTERN int (*mbedtls_md5_update_ret_fn)( mbedtls_md5_context *ctx,
                            const unsigned char *input,
                            size_t ilen );
EXTERN int (*mbedtls_mpi_div_int_fn)( mbedtls_mpi *Q, mbedtls_mpi *R, const mbedtls_mpi *A,
                         mbedtls_mpi_sint b );
EXTERN int (*mbedtls_mpi_div_mpi_fn)( mbedtls_mpi *Q, mbedtls_mpi *R, const mbedtls_mpi *A,
                         const mbedtls_mpi *B );
EXTERN int (*mbedtls_mpi_exp_mod_fn)( mbedtls_mpi *X, const mbedtls_mpi *A,
                         const mbedtls_mpi *E, const mbedtls_mpi *N,
                         mbedtls_mpi *_RR );
EXTERN int (*mbedtls_mpi_exp_mod_sw_fn)( mbedtls_mpi *X, const mbedtls_mpi *A,
                         const mbedtls_mpi *E, const mbedtls_mpi *N,
                         mbedtls_mpi *_RR );
EXTERN int (*mbedtls_mpi_fill_random_fn)( mbedtls_mpi *X, size_t size,
                     int (*f_rng)(void *, unsigned char *, size_t),
                     void *p_rng );
EXTERN int (*mbedtls_mpi_gcd_fn)( mbedtls_mpi *G, const mbedtls_mpi *A,
                     const mbedtls_mpi *B );
EXTERN int (*mbedtls_mpi_gen_prime_fn)( mbedtls_mpi *X, size_t nbits, int flags,
                   int (*f_rng)(void *, unsigned char *, size_t),
                   void *p_rng );
EXTERN int (*mbedtls_mpi_inv_mod_fn)( mbedtls_mpi *X, const mbedtls_mpi *A,
                         const mbedtls_mpi *N );
EXTERN int (*mbedtls_mpi_is_prime_fn)( const mbedtls_mpi *X,
                          int (*f_rng)(void *, unsigned char *, size_t),
                          void *p_rng );
EXTERN int (*mbedtls_mpi_is_prime_ext_fn)( const mbedtls_mpi *X, int rounds,
                              int (*f_rng)(void *, unsigned char *, size_t),
                              void *p_rng );
EXTERN int (*mbedtls_mpi_mod_int_fn)( mbedtls_mpi_uint *r, const mbedtls_mpi *A,
                         mbedtls_mpi_sint b );
EXTERN int (*mbedtls_mpi_mod_mpi_fn)( mbedtls_mpi *R, const mbedtls_mpi *A,
                         const mbedtls_mpi *B );
EXTERN int (*mbedtls_mpi_mul_int_fn)( mbedtls_mpi *X, const mbedtls_mpi *A,
                         mbedtls_mpi_uint b );
EXTERN int (*mbedtls_mpi_mul_mpi_fn)( mbedtls_mpi *X, const mbedtls_mpi *A,
                         const mbedtls_mpi *B );
EXTERN int (*mbedtls_mpi_read_string_fn)( mbedtls_mpi *X, int radix, const char *s );
EXTERN int (*mbedtls_mpi_write_string_fn)( const mbedtls_mpi *X, int radix,
                              char *buf, size_t buflen, size_t *olen );
EXTERN int (*mbedtls_rsa_check_privkey_fn)( const mbedtls_rsa_context *ctx );
EXTERN int (*mbedtls_rsa_check_pub_priv_fn)( const mbedtls_rsa_context *pub,
                                const mbedtls_rsa_context *prv );
EXTERN int (*mbedtls_rsa_check_pubkey_fn)( const mbedtls_rsa_context *ctx );
EXTERN int (*mbedtls_rsa_complete_fn)( mbedtls_rsa_context *ctx );
EXTERN int (*mbedtls_rsa_copy_fn)( mbedtls_rsa_context *dst, const mbedtls_rsa_context *src );
EXTERN int (*mbedtls_rsa_export_fn)( const mbedtls_rsa_context *ctx,
                        mbedtls_mpi *N, mbedtls_mpi *P, mbedtls_mpi *Q,
                        mbedtls_mpi *D, mbedtls_mpi *E );
EXTERN int (*mbedtls_rsa_export_crt_fn)( const mbedtls_rsa_context *ctx,
                            mbedtls_mpi *DP, mbedtls_mpi *DQ, mbedtls_mpi *QP );
EXTERN int (*mbedtls_rsa_export_raw_fn)( const mbedtls_rsa_context *ctx,
                            unsigned char *N, size_t N_len,
                            unsigned char *P, size_t P_len,
                            unsigned char *Q, size_t Q_len,
                            unsigned char *D, size_t D_len,
                            unsigned char *E, size_t E_len );
EXTERN void (*mbedtls_rsa_free_fn)( mbedtls_rsa_context *ctx );
EXTERN int (*mbedtls_rsa_gen_key_fn)( mbedtls_rsa_context *ctx,
                         int (*f_rng)(void *, unsigned char *, size_t),
                         void *p_rng,
                         unsigned int nbits, int exponent );
EXTERN size_t (*mbedtls_rsa_get_len_fn)( const mbedtls_rsa_context *ctx );
EXTERN int (*mbedtls_rsa_import_fn)( mbedtls_rsa_context *ctx,
                        const mbedtls_mpi *N,
                        const mbedtls_mpi *P, const mbedtls_mpi *Q,
                        const mbedtls_mpi *D, const mbedtls_mpi *E );
EXTERN int (*mbedtls_rsa_import_raw_fn)( mbedtls_rsa_context *ctx,
                            unsigned char const *N, size_t N_len,
                            unsigned char const *P, size_t P_len,
                            unsigned char const *Q, size_t Q_len,
                            unsigned char const *D, size_t D_len,
                            unsigned char const *E, size_t E_len );
EXTERN void (*mbedtls_rsa_init_fn)( mbedtls_rsa_context *ctx,
                       int padding,
                       int hash_id );
EXTERN int (*mbedtls_rsa_pkcs1_decrypt_fn)( mbedtls_rsa_context *ctx,
                       int (*f_rng)(void *, unsigned char *, size_t),
                       void *p_rng,
                       int mode, size_t *olen,
                       const unsigned char *input,
                       unsigned char *output,
                       size_t output_max_len );
EXTERN int (*mbedtls_rsa_pkcs1_encrypt_fn)( mbedtls_rsa_context *ctx,
                       int (*f_rng)(void *, unsigned char *, size_t),
                       void *p_rng,
                       int mode, size_t ilen,
                       const unsigned char *input,
                       unsigned char *output );
EXTERN int (*mbedtls_rsa_pkcs1_sign_fn)( mbedtls_rsa_context *ctx,
                    int (*f_rng)(void *, unsigned char *, size_t),
                    void *p_rng,
                    int mode,
                    mbedtls_md_type_t md_alg,
                    unsigned int hashlen,
                    const unsigned char *hash,
                    unsigned char *sig );
EXTERN int (*mbedtls_rsa_pkcs1_verify_fn)( mbedtls_rsa_context *ctx,
                      int (*f_rng)(void *, unsigned char *, size_t),
                      void *p_rng,
                      int mode,
                      mbedtls_md_type_t md_alg,
                      unsigned int hashlen,
                      const unsigned char *hash,
                      const unsigned char *sig );
EXTERN int (*mbedtls_rsa_private_fn)( mbedtls_rsa_context *ctx,
                 int (*f_rng)(void *, unsigned char *, size_t),
                 void *p_rng,
                 const unsigned char *input,
                 unsigned char *output );
EXTERN int (*mbedtls_rsa_private_sw_fn)( mbedtls_rsa_context *ctx,
                 int (*f_rng)(void *, unsigned char *, size_t),
                 void *p_rng,
                 const unsigned char *input,
                 unsigned char *output );
EXTERN int (*mbedtls_rsa_public_fn)( mbedtls_rsa_context *ctx,
                const unsigned char *input,
                unsigned char *output );
EXTERN int (*mbedtls_rsa_rsaes_oaep_decrypt_fn)( mbedtls_rsa_context *ctx,
                            int (*f_rng)(void *, unsigned char *, size_t),
                            void *p_rng,
                            int mode,
                            const unsigned char *label, size_t label_len,
                            size_t *olen,
                            const unsigned char *input,
                            unsigned char *output,
                            size_t output_max_len );
EXTERN int (*mbedtls_rsa_rsaes_oaep_encrypt_fn)( mbedtls_rsa_context *ctx,
                            int (*f_rng)(void *, unsigned char *, size_t),
                            void *p_rng,
                            int mode,
                            const unsigned char *label, size_t label_len,
                            size_t ilen,
                            const unsigned char *input,
                            unsigned char *output );
EXTERN int (*mbedtls_rsa_rsaes_pkcs1_v15_decrypt_fn)( mbedtls_rsa_context *ctx,
                                 int (*f_rng)(void *, unsigned char *, size_t),
                                 void *p_rng,
                                 int mode, size_t *olen,
                                 const unsigned char *input,
                                 unsigned char *output,
                                 size_t output_max_len );
EXTERN int (*mbedtls_rsa_rsaes_pkcs1_v15_encrypt_fn)( mbedtls_rsa_context *ctx,
                                 int (*f_rng)(void *, unsigned char *, size_t),
                                 void *p_rng,
                                 int mode, size_t ilen,
                                 const unsigned char *input,
                                 unsigned char *output );
EXTERN int (*mbedtls_rsa_rsassa_pkcs1_v15_sign_fn)( mbedtls_rsa_context *ctx,
                               int (*f_rng)(void *, unsigned char *, size_t),
                               void *p_rng,
                               int mode,
                               mbedtls_md_type_t md_alg,
                               unsigned int hashlen,
                               const unsigned char *hash,
                               unsigned char *sig );
EXTERN int (*mbedtls_rsa_rsassa_pkcs1_v15_verify_fn)( mbedtls_rsa_context *ctx,
                                 int (*f_rng)(void *, unsigned char *, size_t),
                                 void *p_rng,
                                 int mode,
                                 mbedtls_md_type_t md_alg,
                                 unsigned int hashlen,
                                 const unsigned char *hash,
                                 const unsigned char *sig );
EXTERN int (*mbedtls_rsa_rsassa_pss_sign_fn)( mbedtls_rsa_context *ctx,
                         int (*f_rng)(void *, unsigned char *, size_t),
                         void *p_rng,
                         int mode,
                         mbedtls_md_type_t md_alg,
                         unsigned int hashlen,
                         const unsigned char *hash,
                         unsigned char *sig );
EXTERN int (*mbedtls_rsa_rsassa_pss_verify_fn)( mbedtls_rsa_context *ctx,
                           int (*f_rng)(void *, unsigned char *, size_t),
                           void *p_rng,
                           int mode,
                           mbedtls_md_type_t md_alg,
                           unsigned int hashlen,
                           const unsigned char *hash,
                           const unsigned char *sig );
EXTERN int (*mbedtls_rsa_rsassa_pss_verify_ext_fn)( mbedtls_rsa_context *ctx,
                               int (*f_rng)(void *, unsigned char *, size_t),
                               void *p_rng,
                               int mode,
                               mbedtls_md_type_t md_alg,
                               unsigned int hashlen,
                               const unsigned char *hash,
                               mbedtls_md_type_t mgf1_hash_id,
                               int expected_salt_len,
                               const unsigned char *sig );
EXTERN void (*mbedtls_rsa_set_padding_fn)( mbedtls_rsa_context *ctx, int padding,
                              int hash_id );
EXTERN void (*mbedtls_sha256_clone_fn)( mbedtls_sha256_context *dst,
                           const mbedtls_sha256_context *src );
EXTERN void (*mbedtls_sha256_finish_fn)( mbedtls_sha256_context *ctx,
                                               unsigned char output[32] );
EXTERN int (*mbedtls_sha256_finish_ret_fn)( mbedtls_sha256_context *ctx,
                               unsigned char output[32] );
EXTERN void (*mbedtls_sha256_free_fn)( mbedtls_sha256_context *ctx );
EXTERN void (*mbedtls_sha256_init_fn)( mbedtls_sha256_context *ctx );
EXTERN void (*mbedtls_sha256_starts_fn)( mbedtls_sha256_context *ctx,
                                               int is224 );
EXTERN int (*mbedtls_sha256_starts_ret_fn)( mbedtls_sha256_context *ctx, int is224 );
EXTERN void (*mbedtls_sha256_update_fn)( mbedtls_sha256_context *ctx,
                                               const unsigned char *input,
                                               size_t ilen );
EXTERN int (*mbedtls_sha256_update_ret_fn)( mbedtls_sha256_context *ctx,
                               const unsigned char *input,
                               size_t ilen );


#define mbedtls_ecdsa_free                          mbedtls_ecdsa_free_fn
#define mbedtls_ecdsa_from_keypair                  mbedtls_ecdsa_from_keypair_fn
#define mbedtls_ecdsa_genkey                        mbedtls_ecdsa_genkey_fn
#define mbedtls_ecdsa_init                          mbedtls_ecdsa_init_fn
#define mbedtls_ecdsa_read_signature                mbedtls_ecdsa_read_signature_fn
#define mbedtls_ecdsa_read_signature_restartable    mbedtls_ecdsa_read_signature_restartable_fn
#define mbedtls_ecdsa_sign                          mbedtls_ecdsa_sign_fn
#define mbedtls_ecdsa_sign_det                      mbedtls_ecdsa_sign_det_fn
#define mbedtls_ecdsa_verify                        mbedtls_ecdsa_verify_fn
#define mbedtls_ecdsa_write_signature               mbedtls_ecdsa_write_signature_fn
#define mbedtls_ecdsa_write_signature_det           mbedtls_ecdsa_write_signature_det_fn
#define mbedtls_ecdsa_write_signature_restartable   mbedtls_ecdsa_write_signature_restartable_fn
#define mbedtls_ecp_check_privkey                   mbedtls_ecp_check_privkey_fn
#define mbedtls_ecp_check_pub_priv                  mbedtls_ecp_check_pub_priv_fn
#define mbedtls_ecp_check_pubkey                    mbedtls_ecp_check_pubkey_fn
#define mbedtls_ecp_copy                            mbedtls_ecp_copy_fn
#define mbedtls_ecp_curve_info_from_grp_id          mbedtls_ecp_curve_info_from_grp_id_fn
#define mbedtls_ecp_curve_info_from_name            mbedtls_ecp_curve_info_from_name_fn
#define mbedtls_ecp_curve_info_from_tls_id          mbedtls_ecp_curve_info_from_tls_id_fn
#define mbedtls_ecp_curve_list                      mbedtls_ecp_curve_list_fn
#define mbedtls_ecp_gen_key                         mbedtls_ecp_gen_key_fn
#define mbedtls_ecp_gen_keypair                     mbedtls_ecp_gen_keypair_fn
#define mbedtls_ecp_gen_keypair_base                mbedtls_ecp_gen_keypair_base_fn
#define mbedtls_ecp_gen_privkey                     mbedtls_ecp_gen_privkey_fn
#define mbedtls_ecp_group_copy                      mbedtls_ecp_group_copy_fn
#define mbedtls_ecp_group_free                      mbedtls_ecp_group_free_fn
#define mbedtls_ecp_group_init                      mbedtls_ecp_group_init_fn
#define mbedtls_ecp_grp_id_list                     mbedtls_ecp_grp_id_list_fn
#define mbedtls_ecp_is_zero                         mbedtls_ecp_is_zero_fn
#define mbedtls_ecp_keypair_free                    mbedtls_ecp_keypair_free_fn
#define mbedtls_ecp_keypair_init                    mbedtls_ecp_keypair_init_fn
#define mbedtls_ecp_mul                             mbedtls_ecp_mul_fn
#define mbedtls_ecp_mul_restartable                 mbedtls_ecp_mul_restartable_fn
// #define mbedtls_ecp_mul_shortcuts                   mbedtls_ecp_mul_shortcuts_fn
#define mbedtls_ecp_muladd                          mbedtls_ecp_muladd_fn
#define mbedtls_ecp_muladd_restartable              mbedtls_ecp_muladd_restartable_fn
#define mbedtls_ecp_point_cmp                       mbedtls_ecp_point_cmp_fn
#define mbedtls_ecp_point_free                      mbedtls_ecp_point_free_fn
#define mbedtls_ecp_point_init                      mbedtls_ecp_point_init_fn
#define mbedtls_ecp_point_read_binary               mbedtls_ecp_point_read_binary_fn
#define mbedtls_ecp_point_read_string               mbedtls_ecp_point_read_string_fn
#define mbedtls_ecp_point_write_binary              mbedtls_ecp_point_write_binary_fn
#define mbedtls_ecp_set_zero                        mbedtls_ecp_set_zero_fn
#define mbedtls_ecp_tls_read_group                  mbedtls_ecp_tls_read_group_fn
#define mbedtls_ecp_tls_read_group_id               mbedtls_ecp_tls_read_group_id_fn
#define mbedtls_ecp_tls_read_point                  mbedtls_ecp_tls_read_point_fn
#define mbedtls_ecp_tls_write_group                 mbedtls_ecp_tls_write_group_fn
#define mbedtls_ecp_tls_write_point                 mbedtls_ecp_tls_write_point_fn
#define mbedtls_internal_md5_process                mbedtls_internal_md5_process_fn
#define mbedtls_internal_sha256_process             mbedtls_internal_sha256_process_fn
#define mbedtls_md5                                 mbedtls_md5_fn
#define mbedtls_md5_clone                           mbedtls_md5_clone_fn
#define mbedtls_md5_finish                          mbedtls_md5_finish_fn
#define mbedtls_md5_finish_ret                      mbedtls_md5_finish_ret_fn
#define mbedtls_md5_free                            mbedtls_md5_free_fn
#define mbedtls_md5_init                            mbedtls_md5_init_fn
#define mbedtls_md5_process                         mbedtls_md5_process_fn
#define mbedtls_md5_ret                             mbedtls_md5_ret_fn
#define mbedtls_md5_starts                          mbedtls_md5_starts_fn
#define mbedtls_md5_starts_ret                      mbedtls_md5_starts_ret_fn
#define mbedtls_md5_update                          mbedtls_md5_update_fn
#define mbedtls_md5_update_ret                      mbedtls_md5_update_ret_fn
#define mbedtls_mpi_div_int                         mbedtls_mpi_div_int_fn
#define mbedtls_mpi_div_mpi                         mbedtls_mpi_div_mpi_fn
#define mbedtls_mpi_exp_mod                         mbedtls_mpi_exp_mod_fn
#define mbedtls_mpi_exp_mod_sw                      mbedtls_mpi_exp_mod_sw_fn
#define mbedtls_mpi_fill_random                     mbedtls_mpi_fill_random_fn
#define mbedtls_mpi_gcd                             mbedtls_mpi_gcd_fn
#define mbedtls_mpi_gen_prime                       mbedtls_mpi_gen_prime_fn
#define mbedtls_mpi_inv_mod                         mbedtls_mpi_inv_mod_fn
#define mbedtls_mpi_is_prime                        mbedtls_mpi_is_prime_fn
#define mbedtls_mpi_is_prime_ext                    mbedtls_mpi_is_prime_ext_fn
#define mbedtls_mpi_mod_int                         mbedtls_mpi_mod_int_fn
#define mbedtls_mpi_mod_mpi                         mbedtls_mpi_mod_mpi_fn
#define mbedtls_mpi_mul_int                         mbedtls_mpi_mul_int_fn
#define mbedtls_mpi_mul_mpi                         mbedtls_mpi_mul_mpi_fn
#define mbedtls_mpi_read_string                     mbedtls_mpi_read_string_fn
#define mbedtls_mpi_write_string                    mbedtls_mpi_write_string_fn
#define mbedtls_rsa_check_privkey                   mbedtls_rsa_check_privkey_fn
#define mbedtls_rsa_check_pub_priv                  mbedtls_rsa_check_pub_priv_fn
#define mbedtls_rsa_check_pubkey                    mbedtls_rsa_check_pubkey_fn
#define mbedtls_rsa_complete                        mbedtls_rsa_complete_fn
#define mbedtls_rsa_copy                            mbedtls_rsa_copy_fn
#define mbedtls_rsa_export                          mbedtls_rsa_export_fn
#define mbedtls_rsa_export_crt                      mbedtls_rsa_export_crt_fn
#define mbedtls_rsa_export_raw                      mbedtls_rsa_export_raw_fn
#define mbedtls_rsa_free                            mbedtls_rsa_free_fn
#define mbedtls_rsa_gen_key                         mbedtls_rsa_gen_key_fn
#define mbedtls_rsa_get_len                         mbedtls_rsa_get_len_fn
#define mbedtls_rsa_import                          mbedtls_rsa_import_fn
#define mbedtls_rsa_import_raw                      mbedtls_rsa_import_raw_fn
#define mbedtls_rsa_init                            mbedtls_rsa_init_fn
#define mbedtls_rsa_pkcs1_decrypt                   mbedtls_rsa_pkcs1_decrypt_fn
#define mbedtls_rsa_pkcs1_encrypt                   mbedtls_rsa_pkcs1_encrypt_fn
#define mbedtls_rsa_pkcs1_sign                      mbedtls_rsa_pkcs1_sign_fn
#define mbedtls_rsa_pkcs1_verify                    mbedtls_rsa_pkcs1_verify_fn
#define mbedtls_rsa_private                         mbedtls_rsa_private_fn
#define mbedtls_rsa_private_sw                      mbedtls_rsa_private_sw_fn
#define mbedtls_rsa_public                          mbedtls_rsa_public_fn
#define mbedtls_rsa_rsaes_oaep_decrypt              mbedtls_rsa_rsaes_oaep_decrypt_fn
#define mbedtls_rsa_rsaes_oaep_encrypt              mbedtls_rsa_rsaes_oaep_encrypt_fn
#define mbedtls_rsa_rsaes_pkcs1_v15_decrypt         mbedtls_rsa_rsaes_pkcs1_v15_decrypt_fn
#define mbedtls_rsa_rsaes_pkcs1_v15_encrypt         mbedtls_rsa_rsaes_pkcs1_v15_encrypt_fn
#define mbedtls_rsa_rsassa_pkcs1_v15_sign           mbedtls_rsa_rsassa_pkcs1_v15_sign_fn
#define mbedtls_rsa_rsassa_pkcs1_v15_verify         mbedtls_rsa_rsassa_pkcs1_v15_verify_fn
#define mbedtls_rsa_rsassa_pss_sign                 mbedtls_rsa_rsassa_pss_sign_fn
#define mbedtls_rsa_rsassa_pss_verify               mbedtls_rsa_rsassa_pss_verify_fn
#define mbedtls_rsa_rsassa_pss_verify_ext           mbedtls_rsa_rsassa_pss_verify_ext_fn
#define mbedtls_rsa_set_padding                     mbedtls_rsa_set_padding_fn
#define mbedtls_sha256_clone                        mbedtls_sha256_clone_fn
#define mbedtls_sha256_finish                       mbedtls_sha256_finish_fn
#define mbedtls_sha256_finish_ret                   mbedtls_sha256_finish_ret_fn
#define mbedtls_sha256_free                         mbedtls_sha256_free_fn
#define mbedtls_sha256_init                         mbedtls_sha256_init_fn
#define mbedtls_sha256_starts                       mbedtls_sha256_starts_fn
#define mbedtls_sha256_starts_ret                   mbedtls_sha256_starts_ret_fn
#define mbedtls_sha256_update                       mbedtls_sha256_update_fn
#define mbedtls_sha256_update_ret                   mbedtls_sha256_update_ret_fn

#endif  /* CONFIG_ASIC_CUT_AUTO */

#endif  // __ROM_EXPORT_MBEDTLS_H__
