/*
 * wpa_supplicant - WPA2/RSN PMKSA cache functions
 * Copyright (c) 2003-2009, 2011-2012, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

/*!
    \file    wpas_pmksa_cache.h
    \brief   Header file for wpas pmksa cache.

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

#ifndef _WPAS_PMKSA_CACHE_H_
#define _WPAS_PMKSA_CACHE_H_

#define RSN_PMK_LIFE_TIME            43200  /* 12 hours */

#define PMKSA_CACHE_MAX_ENTRIES     8

#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC2STR_U16(a) ((a)[0] & 0xFF), ((a)[0] >> 8), ((a)[1] & 0xFF), ((a)[1] >> 8), ((a)[2] & 0xFF), ((a)[2] >> 8)
#endif

/**
 * struct rsn_pmksa_cache_entry - PMKSA cache entry
 */
struct rsn_pmksa_cache_entry {
    struct rsn_pmksa_cache_entry *next;
    uint8_t pmkid[SAE_PMKID_LEN];
    uint8_t pmk[SAE_PMK_LEN_MAX];
    size_t pmk_len;
    os_time_t expiration;
    int akmp; /* WPA_KEY_MGMT_* */
    uint8_t sa[WIFI_ALEN];
};

enum pmksa_free_reason {
    PMKSA_FREE,
    PMKSA_REPLACE,
    PMKSA_EXPIRE,
};

struct rsn_pmksa_cache {
    struct rsn_pmksa_cache_entry *pmksa; /* PMKSA cache */
    int16_t pmksa_count; /* number of entries in PMKSA cache */

    void (*free_cb)(struct rsn_pmksa_cache_entry *entry, void *ctx,
            enum pmksa_free_reason reason);
    bool (*is_current_cb)(void *ctx, void *ctx_ex);
    void *ctx;
};

static inline int wpa_snprintf_hex(char *buf, size_t buf_size, const uint8_t *data,
                    size_t len)
{
    size_t i;
    char *pos = buf, *end = buf + buf_size;
    int ret;

    if (buf_size == 0)
        return 0;
    for (i = 0; i < len; i++) {
        ret = snprintf(pos, end - pos, "%02x", data[i]);
        if (ret < 0 || (unsigned int) ret >= end - pos) {
            end[-1] = '\0';
            return pos - buf;
        }
        pos += ret;
    }
    end[-1] = '\0';
    return pos - buf;
}

int
pmksa_cache_init(int vif_idx, struct rsn_pmksa_cache *pmksa,
                        void (*free_cb)(struct rsn_pmksa_cache_entry *entry,
                                        void *ctx, enum pmksa_free_reason reason),
                        bool (*is_current_cb)(void *ctx, void *ctx_ex),
                        void *ctx);
void pmksa_cache_deinit(struct rsn_pmksa_cache *pmksa);
struct rsn_pmksa_cache_entry *
    pmksa_cache_get(struct rsn_pmksa_cache *pmksa,
                        const uint8_t *sa, const uint8_t *pmkid,
                        int akmp);
int pmksa_cache_list(struct rsn_pmksa_cache *pmksa, char *buf, size_t len);
struct rsn_pmksa_cache_entry *
    pmksa_cache_head(struct rsn_pmksa_cache *pmksa);
struct rsn_pmksa_cache_entry *
pmksa_cache_add(struct rsn_pmksa_cache *pmksa, const uint8_t *pmk, size_t pmk_len,
        const uint8_t *pmkid, const uint8_t *kck, size_t kck_len,
        const uint8_t *sa, int akmp);
struct rsn_pmksa_cache_entry *
    pmksa_cache_add_entry(struct rsn_pmksa_cache *pmksa,
              struct rsn_pmksa_cache_entry *entry);
void pmksa_cache_flush(struct rsn_pmksa_cache *pmksa,
               const uint8_t *pmk, size_t pmk_len);
void pmksa_cache_reconfig(struct rsn_pmksa_cache *pmksa);

void pmksa_cache_expire(struct rsn_pmksa_cache *pmksa);

void pmksa_cache_flush_all(struct rsn_pmksa_cache *pmksa);

#endif /* _WPAS_PMKSA_CACHE_H_ */
