/*!
    \file    rom_ibl_state.h
    \brief   Rom IBL state file for GD32VW55x SDK

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

#ifndef __ROM_IBL_STATE_H__
#define __ROM_IBL_STATE_H__

#define IMPL_ID_MAX_SIZE        (32u)
#define IBL_STATE_MAGIC_CODE    0xBACEC0DE

enum reset_flag_t {
    RESET_BY_UNKNOWN = 0,
    RESET_BY_BOR,
    RESET_BY_PIN,
    RESET_BY_PWR_ON,
    RESET_BY_SW,
    RESET_BY_FWDG,
    RESET_BY_WWDG,
    RESET_BY_LOW_PWR,
};

enum boot_status_t {
    BOOT_FAIL_UNKNOWN = -0xFF,
    BOOT_FAIL_BAD_SYS_SETTING,
    BOOT_FAIL_BAD_SYS_STATUS,
    BOOT_FAIL_NOT_FOUND_MBL,
    BOOT_FAIL_BAD_OPT,
    BOOT_FAIL_BAD_CERT,
    BOOT_FAIL_BAD_MBL,
    BOOT_FAIL_BAD_ENTRY,
#if (SYS_STATUS_ENCRPTED == 0)
    BOOT_FAIL_SET_INITIAL_VER,
#endif
    BOOT_FAIL_SET_NV_CNTR,
    BOOT_FAIL_ENABLE_FWDG,

    BOOT_START = 0,

    BOOT_HW_INIT_OK,
    BOOT_SYS_CONFIG_OK,
    BOOT_VERIFY_MBL_OK,
    BOOT_OK,
};

enum ibl_option_t {
    IBL_VERIFY_NONE = 0,
    IBL_VERIFY_IMG_ONLY = 1,
    IBL_VERIFY_CERT_IMG = 3,
};

struct sw_info_t {
    uint32_t type;                          /* IMG_TYPE_XXX */
    uint32_t version;                       /* The version read from image header */
    uint8_t signer_id[PK_HASH_LEN];         /* The hash of Image public key */
    uint8_t digest[IMG_DIGEST_MAX_LEN];     /* The hash of Image digest (header + image self) */
};

struct ob_state_t {
    uint8_t spc_en ;                        /* 1: Enable security protection; 0: disable */
    uint8_t mbl_wp;                         /* 1: First 32KB for MBL are write protected. Set valid after system reset. */
    uint8_t rsvd[2];
};

/**
 * \struct ibl_state_t
 *
 * \brief Store the initial boot state for MBL
 */
struct ibl_state_t
{
    uint32_t magic;                         /* magic code: 0xBACEC0DE */
    uint32_t reset_flag;                    /* enum reset_flag_t */
    int boot_status;                        /* enum boot_status_t */
    uint32_t rom_ver;                       /* Indicate the ROM version to SW for future use */
    uint32_t ibl_opt;                       /* enum ibl_option_t */
    uint8_t rotpk_hash[PK_HASH_LEN];        /* ROTPK hash */
    struct ob_state_t obstat;               /* Option byte. Read from FMC. */
    struct sw_info_t mbl_info;              /* SW measurements: type, version, measurement */
};

void store_ibl_state(struct ibl_state_t *state);

#endif  /* __ROM_IBL_STATE_H__ */
