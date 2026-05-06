/*
 * Simultaneous authentication of equals
 * Copyright (c) 2012-2013, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

/*!
    \file    wpas_sae.h
    \brief   Header file for wpas SAE.

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

#ifndef _WPAS_SAE_H_
#define _WPAS_SAE_H_

#include "wpas_sae_crypto.h"
// #define CONFIG_FFC_GROUP_SUPPORT
//struct crypto_bignum;

#define SAE_KCK_LEN                     32
#define SAE_PMK_LEN                     32
#define SAE_PMKID_LEN                   16
#define SAE_PMK_LEN_MAX                 64
#define SAE_KEYSEED_KEY_LEN             32
#define SAE_MAX_FFC_PRIME_LEN           512
#define SAE_MAX_ECC_PRIME_LEN           48  // 32  // 66
#ifdef CONFIG_FFC_GROUP_SUPPORT
#define SAE_MAX_PRIME_LEN               SAE_MAX_FFC_PRIME_LEN
#else
#define SAE_MAX_PRIME_LEN               SAE_MAX_ECC_PRIME_LEN
#endif
#define SAE_COMMIT_MAX_LEN              (2 + 3 * SAE_MAX_PRIME_LEN)
#define SAE_CONFIRM_MAX_LEN             (2 + SAE_MAX_PRIME_LEN)

#define DEFAULT_SAE_GROUP               19
#define SHA256_MAC_LEN                  32

/* Special value returned by sae_parse_commit() */
#define SAE_SILENTLY_DISCARD            65535

#define SAE_SYNC_MAX                    5

#define DRAGONFLY_MAX_ECC_PRIME_LEN     66

#define SAE_REAUTH_TO                   (500)   //ms
#define SAE_REJECT_REAUTH_TO            (2000)  //TODO
#define AUTH_SAE_REAUTH_TO              (1000)  //ms
#define AUTH_SAE_PROCESS_TO             (10)    //ms

#define OWE_DH_GROUP                    19

#if 0
// AKM string used by wpa_supplicant
extern const char * const wpa_akm_str[];

// Cipher suites string used by wpa_supplicant
extern const char * const wpa_cipher_str[];
#endif

enum sae_transaction {
    SAE_TRANSACTION_COMMIT = 1,
    SAE_TRANSACTION_CONFIRM,
};

enum sae_state {
    SAE_NOTHING, SAE_COMMITTED, SAE_CONFIRMED, SAE_ACCEPTED
};
/*
struct crypto_ec {
    struct crypto_ec *group;
    struct crypto_bignum a;
    struct crypto_bignum prime;
    struct crypto_bignum order;
    uint32_t mont_b;
    struct crypto_bignum b;
};
*/
struct sae_temporary_data {
    uint8_t kck[SAE_KCK_LEN];
    struct crypto_bignum *own_commit_scalar;
    struct crypto_bignum *own_commit_element_ffc;
    struct crypto_ec_point *own_commit_element_ecc;
    struct crypto_bignum *peer_commit_element_ffc;
    struct crypto_ec_point *peer_commit_element_ecc;
    struct crypto_ec_point *pwe_ecc;
    struct crypto_bignum *pwe_ffc;
    struct crypto_bignum *sae_rand;
    struct crypto_ec *ec;
    int prime_len;
    int order_len;
#ifdef CONFIG_FFC_GROUP_SUPPORT
    const struct dh_group *dh;
#endif
    const struct crypto_bignum *prime;
    const struct crypto_bignum *order;
    struct crypto_bignum *prime_buf;
    struct crypto_bignum *order_buf;
    // struct wpabuf *anti_clogging_token;
    // uint8_t *pw_id;
    int vlan_id;
    uint8_t bssid[WIFI_ALEN];
};

struct sae_data {
    enum sae_state state;
    uint16_t send_confirm;
    uint8_t pmk[SAE_PMK_LEN];
    uint8_t pmkid[SAE_PMKID_LEN];
    struct crypto_bignum *peer_commit_scalar;
    struct crypto_bignum *peer_commit_scalar_accepted;
    int group;
    uint32_t sync; /* protocol instance variable: Sync */
    uint16_t rc; /* protocol instance variable: Rc (received send-confirm) */
    struct sae_temporary_data *tmp;
};

struct sae_commit_queue_t {
    struct list_head list;
    int rssi;
    size_t len;
    uint8_t msg[];
};

struct sa_query_data {
    int sa_query_count; /* number of pending SA Query requests;
                            0 = no SA Query in progress */
    int sa_query_timed_out;
    uint8_t *sa_query_trans_id; /* buffer of WLAN_SA_QUERY_TR_ID_LEN *
                * sa_query_count octets of pending
                * SA Query transaction identifiers */
    struct os_reltime sa_query_start;
    struct os_reltime last_unprot_disconnect;
};

struct wpas_sae
{
    struct sae_data sae;
    uint8_t *token;
    uint16_t token_len;
    uint8_t sae_group;
    uint16_t peer_seq_num;
};

struct ext_auth_rsp
{
    struct mac_addr addr;
    uint16_t status;
    const uint8_t *pmkid;
};

int dragonfly_suitable_group(int group, int ecc_only);
uint32_t dragonfly_min_pwe_loop_iter(int group);
int dragonfly_get_random_qr_qnr(const struct crypto_bignum *prime,
                    struct crypto_bignum **qr,
                    struct crypto_bignum **qnr);
int dragonfly_is_quadratic_residue_blind(struct crypto_ec *e,
                    const uint8_t *qr, const uint8_t *qnr,
                    const struct crypto_bignum *val);
int dragonfly_generate_scalar(const struct crypto_bignum *order,
                    struct crypto_bignum *_rand,
                    struct crypto_bignum *_mask,
                    struct crypto_bignum *scalar);

int sae_set_group(struct sae_data *sae, int group);
void sae_clear_temp_data(struct sae_data *sae);
void sae_clear_data(struct sae_data *sae);

int sae_prepare_commit(const uint8_t *addr1, const uint8_t *addr2,
                    const char *password, size_t password_len,
                    const uint8_t *identifier, struct sae_data *sae);
int sae_process_commit(struct sae_data *sae);
uint32_t sae_write_commit(struct sae_data *sae, uint8_t *pframe,
                    const uint8_t *token, uint32_t token_len, const uint8_t *identifier);
uint16_t sae_parse_commit(struct sae_data *sae, const uint8_t *data, size_t len,
                    const uint8_t **token, size_t *token_len, int *allowed_groups);
uint32_t sae_write_confirm(struct sae_data *sae, uint8_t *pframe);
int sae_check_confirm(struct sae_data *sae, const uint8_t *data, size_t len);
uint16_t sae_group_allowed(struct sae_data *sae, int *allowed_groups, uint16_t group);
const char *sae_state_txt(enum sae_state state);

/**
 * const_time_fill_msb - Fill all bits with MSB value
 * @val: Input value
 * Returns: Value with all the bits set to the MSB of the input val
 */
static inline uint32_t const_time_fill_msb(uint32_t val)
{
    /* Move the MSB to LSB and multiple by -1 to fill in all bits. */
    return (val >> (sizeof(val) * 8 - 1)) * ~0U;
}

/* Returns: -1 if val is zero; 0 if val is not zero */
static inline uint32_t const_time_is_zero(uint32_t val)
{
    /* Set MSB to 1 for 0 and fill rest of bits with the MSB value */
    return const_time_fill_msb(~val & (val - 1));
}

/* Returns: -1 if a == b; 0 if a != b */
static inline uint32_t const_time_eq(uint32_t a, uint32_t b)
{
    return const_time_is_zero(a ^ b);
}

/* Returns: -1 if a == b; 0 if a != b */
static inline uint8_t const_time_eq_u8(uint32_t a, uint32_t b)
{
    return (uint8_t) const_time_eq(a, b);
}

/**
 * const_time_eq_bin - Constant time memory comparison
 * @a: First buffer to compare
 * @b: Second buffer to compare
 * @len: Number of octets to compare
 * Returns: -1 if buffers are equal, 0 if not
 *
 * This function is meant for comparing passwords or hash values where
 * difference in execution time or memory access pattern could provide external
 * observer information about the location of the difference in the memory
 * buffers. The return value does not behave like memcmp(), i.e.,
 * const_time_eq_bin() cannot be used to sort items into a defined order. Unlike
 * memcmp(), the execution time of const_time_eq_bin() does not depend on the
 * contents of the compared memory buffers, but only on the total compared
 * length.
 */
static inline uint32_t const_time_eq_bin(const void *a, const void *b,
                         size_t len)
{
    const uint8_t *aa = (const uint8_t *)a;
    const uint8_t *bb = (const uint8_t *)b;
    size_t i;
    uint8_t res = 0;

    for (i = 0; i < len; i++)
        res |= aa[i] ^ bb[i];

    return const_time_is_zero(res);
}

/**
 * const_time_select - Constant time uint32_t selection
 * @mask: 0 (false) or -1 (true) to identify which value to select
 * @true_val: Value to select for the true case
 * @false_val: Value to select for the false case
 * Returns: true_val if mask == -1, false_val if mask == 0
 */
static inline uint32_t const_time_select(uint32_t mask,
                         uint32_t true_val,
                         uint32_t false_val)
{
    return (mask & true_val) | (~mask & false_val);
}


/**
 * const_time_select_int - Constant time int selection
 * @mask: 0 (false) or -1 (true) to identify which value to select
 * @true_val: Value to select for the true case
 * @false_val: Value to select for the false case
 * Returns: true_val if mask == -1, false_val if mask == 0
 */
static inline int const_time_select_int(uint32_t mask, int true_val,
                    int false_val)
{
    return (int) const_time_select(mask, (uint32_t) true_val,
                       (uint32_t) false_val);
}


/**
 * const_time_select_u8 - Constant time uint8_t selection
 * @mask: 0 (false) or -1 (true) to identify which value to select
 * @true_val: Value to select for the true case
 * @false_val: Value to select for the false case
 * Returns: true_val if mask == -1, false_val if mask == 0
 */
static inline uint8_t const_time_select_u8(uint8_t mask, uint8_t true_val, uint8_t false_val)
{
    return (uint8_t) const_time_select(mask, true_val, false_val);
}


/**
 * const_time_select_s8 - Constant time int8_t selection
 * @mask: 0 (false) or -1 (true) to identify which value to select
 * @true_val: Value to select for the true case
 * @false_val: Value to select for the false case
 * Returns: true_val if mask == -1, false_val if mask == 0
 */
static inline int8_t const_time_select_s8(uint8_t mask, int8_t true_val, int8_t false_val)
{
    return (int8_t) const_time_select(mask, (uint32_t) true_val,
                      (uint32_t) false_val);
}


/**
 * const_time_select_bin - Constant time binary buffer selection copy
 * @mask: 0 (false) or -1 (true) to identify which value to copy
 * @true_val: Buffer to copy for the true case
 * @false_val: Buffer to copy for the false case
 * @len: Number of octets to copy
 * @dst: Destination buffer for the copy
 *
 * This function copies the specified buffer into the destination buffer using
 * operations with identical memory access pattern regardless of which buffer
 * is being copied.
 */
static inline void const_time_select_bin(uint8_t mask, const uint8_t *true_val,
                     const uint8_t *false_val, size_t len,
                     uint8_t *dst)
{
    size_t i;

    for (i = 0; i < len; i++)
        dst[i] = const_time_select_u8(mask, true_val[i], false_val[i]);
}


static inline int const_time_memcmp(const void *a, const void *b, size_t len)
{
    const uint8_t *aa = (const uint8_t *)a;
    const uint8_t *bb = (const uint8_t *)b;
    int diff, res = 0;
    uint32_t mask;

    if (len == 0)
        return 0;
    do {
        len--;
        diff = (int) aa[len] - (int) bb[len];
        mask = const_time_is_zero((uint32_t) diff);
        res = const_time_select_int(mask, res, diff);
    } while (len);

    return res;
}

static inline void buf_shift_right(uint8_t *buf, size_t len, size_t bits)
{
    size_t i;

    for (i = len - 1; i > 0; i--)
        buf[i] = (buf[i - 1] << (8 - bits)) | (buf[i] >> bits);
    buf[0] >>= bits;
}

/*******************************************************************************************/
/*                                   FOR STA ONLY                                          */
/*******************************************************************************************/
enum sae_state sae_get_state(struct sae_data *sae);
int wpas_sae_start(struct wpas_sae *w_sae);
int wpas_sae_stop(struct wpas_sae *w_sae);
int wpas_sae_frame_recved(struct wpas_sae *w_sae, uint8_t *pframe, uint32_t frm_len);


/*******************************************************************************************/
/*                                   FOR SoftAP ONLY                                          */
/*******************************************************************************************/
struct wpas_ap;
struct ap_cli;
struct wpa_cli_sm;
int auth_sae_queued_addr(struct wpas_ap *ap, const uint8_t *addr);
void auth_sae_queue(struct wpas_ap *ap,
               const struct ieee80211_mgmt *mgmt, size_t len,
               int rssi);
void handle_auth_sae(struct wpas_ap *ap, struct ap_cli *cli,
                const struct ieee80211_mgmt *mgmt, size_t len,
                uint16_t auth_transaction, uint16_t status_code);
bool auth_sae_is_connected_cb(void *ctx, void *ctx_ex);
void auth_sae_retrans_timer_clear(struct wpas_ap *ap, struct ap_cli *cli);

#endif /* _WPAS_SAE_H_ */
