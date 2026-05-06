/*!
    \file    wpas_ft.h
    \brief   Header file for wpas fast transition.

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

#ifndef _WPAS_FT_H_
#define _WPAS_FT_H_
#include <stdint.h>

#ifdef CONFIG_IEEE80211R
/* IEEE 802.11r */
#define MOBILITY_DOMAIN_ID_LEN              2
#define FT_R0KH_ID_MAX_LEN                  48
#define FT_R1KH_ID_LEN                      6
#define WPA_PMK_NAME_LEN                    16

/* FTE - MIC Control - RSNXE Used */
#define FTE_MIC_CTRL_RSNXE_USED             BIT(0)
#define FTE_MIC_CTRL_MIC_LEN_MASK           (BIT(1) | BIT(2) | BIT(3))
#define FTE_MIC_CTRL_MIC_LEN_SHIFT          1

/* FTE - MIC Length subfield values */
enum ft_mic_len_subfield {
    FTE_MIC_LEN_16 = 0,
    FTE_MIC_LEN_24 = 1,
    FTE_MIC_LEN_32 = 2,
};

struct rsn_mdie {
    uint8_t mobility_domain[MOBILITY_DOMAIN_ID_LEN];
    uint8_t ft_capab;
} STRUCT_PACKED;

#define RSN_FT_CAPAB_FT_OVER_DS              BIT(0)
#define RSN_FT_CAPAB_FT_RESOURCE_REQ_SUPP    BIT(1)

struct rsn_ftie {
    uint8_t mic_control[2];
    uint8_t mic[16];
    uint8_t anonce[WPA_NONCE_LEN];
    uint8_t snonce[WPA_NONCE_LEN];
    /* followed by optional parameters */
} STRUCT_PACKED;

#define FTIE_SUBELEM_R1KH_ID            1
#define FTIE_SUBELEM_GTK                2
#define FTIE_SUBELEM_R0KH_ID            3
#define FTIE_SUBELEM_IGTK               4
#define FTIE_SUBELEM_OCI                5
#define FTIE_SUBELEM_BIGTK              6
#define FTIE_SUBELEM_MLO_GTK            8
#define FTIE_SUBELEM_MLO_IGTK           9
#define FTIE_SUBELEM_MLO_BIGTK          10

struct ft_params {

    uint8_t xxkey[PMK_LEN_MAX]; /* PSK or the second 256 bits of MSK, or the
                * first 384 bits of MSK */
    size_t xxkey_len;
    uint8_t pmk_r0[PMK_LEN_MAX];
    size_t pmk_r0_len;
    uint8_t pmk_r0_name[WPA_PMK_NAME_LEN];
    uint8_t pmk_r1[PMK_LEN_MAX];
    size_t pmk_r1_len;
    uint8_t pmk_r1_name[WPA_PMK_NAME_LEN];
    uint8_t mobility_domain[MOBILITY_DOMAIN_ID_LEN];
    uint8_t key_mobility_domain[MOBILITY_DOMAIN_ID_LEN];
    uint8_t r0kh_id[FT_R0KH_ID_MAX_LEN];
    size_t r0kh_id_len;
    uint8_t r1kh_id[FT_R1KH_ID_LEN];
    unsigned int ft_completed:1;
    unsigned int ft_reassoc_completed:1;
    unsigned int ft_protocol:1;
    int over_the_ds_in_progress;
    uint8_t target_ap[ETH_ALEN]; /* over-the-DS target AP */
    int set_ptk_after_assoc;
    uint8_t mdie_ft_capab; /* FT Capability and Policy from target AP MDIE */
    uint8_t *assoc_resp_ies; /* MDIE and FTIE from (Re)Association Response */
    size_t assoc_resp_ies_len;

    uint32_t keys_cleared_backup;
};

struct wpa_ft_ies {
    const uint8_t *mdie;
    size_t mdie_len;
    const uint8_t *ftie;
    size_t ftie_len;
    const uint8_t *r1kh_id;
    const uint8_t *gtk;
    size_t gtk_len;
    const uint8_t *r0kh_id;
    size_t r0kh_id_len;
    const uint8_t *fte_anonce;
    const uint8_t *fte_snonce;
    bool fte_rsnxe_used;
    unsigned int fte_elem_count;
    const uint8_t *fte_mic;
    size_t fte_mic_len;
    const uint8_t *rsn;
    size_t rsn_len;
    uint16_t rsn_capab;
    const uint8_t *rsn_pmkid;
    const uint8_t *tie;
    size_t tie_len;
    const uint8_t *igtk;
    size_t igtk_len;
    const uint8_t *bigtk;
    size_t bigtk_len;
    const uint8_t *ric;
    size_t ric_len;
    int key_mgmt;
    int pairwise_cipher;
    const uint8_t *rsnxe;
    size_t rsnxe_len;

    struct wpabuf *fte_buf;
};

int wpa_insert_pmkid(uint8_t *ies, size_t *ies_len, const uint8_t *pmkid, uint8_t replace);

int wpa_derive_ptk_ft(struct wpas_eapol *eapol, const unsigned char *src_addr,
                        const struct wpa_eapol_key *key, struct wpa_ptk *ptk);

int wpas_set_ft_params(struct wpas_eapol *eapol, const uint8_t *ies, size_t ies_len);

uint8_t * wpa_ft_gen_req_ies(struct wpas_eapol *eapol, size_t *len,
                   const uint8_t *anonce, const uint8_t *pmk_name,
                   const uint8_t *kck, size_t kck_len,
                   const uint8_t *target_ap,
                   const uint8_t *ric_ies, size_t ric_ies_len,
                   const uint8_t *ap_mdie, int omit_rsnxe);

int wpa_ft_prepare_auth_request(struct wpas_eapol *eapol, const uint8_t *mdie);

int wpa_validate_ie_ft(struct wpas_eapol *eapol,
                    const unsigned char *src_addr,
                    struct wpa_eapol_ie_parse *ie);

int wpa_ft_process_response(struct wpas_eapol *eapol, const uint8_t *ies, size_t ies_len,
                int ft_action, const uint8_t *target_ap,
                const uint8_t *ric_ies, size_t ric_ies_len);

int wpa_ft_validate_reassoc_resp(struct wpas_eapol *eapol, const uint8_t *ies,
                 size_t ies_len, const uint8_t *src_addr);

int wpa_ft_install_ptk(struct wpas_eapol *eapol, const uint8_t *bssid);

#endif /* CONFIG_IEEE80211R */

#endif /* _WPAS_FT_H_ */