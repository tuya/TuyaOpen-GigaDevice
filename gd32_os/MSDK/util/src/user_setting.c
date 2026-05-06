/*!
    \file    user_setting.c
    \brief   User setting for GD32VW55x SDK.

    \version 2024-01-20, V1.0.0, firmware for GD32VW55x
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
#include <stdint.h>
#include "platform_def.h"
#include "user_setting.h"

user_setting_param_t user_setting;

/* power by rate table */
/* !!!don't change table name and size!!! */
/* if use, please set user_setting.pwrbyrate_tbl_user_enable = 1 in func user_setting_init, default 0 means use default table */
/* base power: CCK-17dbm, OFDM-15dbm, HT20-14dbm, HE20-14dbm */
/* step unit: 0.5db */
int8_t pwr_by_rate_sw_tbl_user[RATE_CATEGORY][10] = {
    {0, 0, 2, 2, -16, -16, -16, -16, -16, -16},  // CCK {11M, 5.5M, 2M, 1M}
    {0, 0, 2, 2, 4, 4, 6, 6, -16, -16},  // OFDM {54M, 48M, 36M, 24M, 18M, 12M, 9M, 6M}
    {0, 2, 2, 4, 4, 6, 6, 8, -16, -16},  // HT20 {MCS7, MCS6, MCS5, MCS4, MCS3, MCS2, MCS1, MCS0}
    {-4, -2, 0, 2, 2, 4, 4, 6, 6, 8},  // HE20&TB {MCS9, MCS8, MCS7, MCS6, MCS5, MCS4, MCS3, MCS2, MCS1, MCS0}
};

/* power limit table for FCC */
/* !!!don't change table name and size!!! */
/* base power: CCK-17dbm, OFDM-15dbm, HT20-14dbm, HE20-14dbm */
/* step unit: 1db */
const int8_t pwr_limit_tbl_fcc_user[RATE_CATEGORY][11] = {
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},                      // CCK
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},                    // OFDM
    {0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0},                    // HT20
    {0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0},                    // HE20
};

/* power limit table for CE/SRRC(old) */
/* !!!don't change table name and size!!! */
/* base power: CCK-17dbm, OFDM-15dbm, HT20-14dbm, HE20-14dbm */
/* step unit: 1db */
const int8_t pwr_limit_tbl_etsi_user[RATE_CATEGORY][13] = {
    {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2},   // CCK
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},                // OFDM
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},                // HT20
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},                // HE20
};

/* power limit table for TELEC */
/* !!!don't change table name and size!!! */
/* base power: CCK-17dbm, OFDM-15dbm, HT20-14dbm, HE20-14dbm */
/* step unit: 1db */
const int8_t pwr_limit_tbl_telec_user[RATE_CATEGORY][14] = {
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},  // CCK
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},                // OFDM
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},                // HT20
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},                // HE20
};

/* power limit table for SRRC(new) */
/* !!!don't change table name and size!!! */
/* base power: CCK-17dbm, OFDM-15dbm, HT20-14dbm, HE20-14dbm */
/* step unit: 1db */
const int8_t pwr_limit_tbl_srrc_user[RATE_CATEGORY][13] = {
    {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2},   // CCK
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -2},             // OFDM
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, -1},             // HT20
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, -3},             // HE20
};

/* BLE max transmit power(dbm) */
/* !!!don't change table name and size!!! */
/* valid value: -12, -6, 0, 4, 8, 12, 15 */
int8_t ble_max_pwr_tbl_user[5][4] = {
  /* 1M  2M  S8  S2 */
    { 8,  8,  8,  8},   // Global domain
    {15,  8, 15, 15},   // FCC
    { 4,  4,  4,  4},   // CE
    { 4,  4,  4,  4},   // TELEC
    { 4,  4,  4,  4},   // SRRC
};

/* BLE target tx power(dbm) */
/* !!!don't change table name and size!!! */
/* valid value: -24, -21, -18, -15, -12, -9, -6, -3, 0, 2, 4, 6, 8, 10, 12, 15 */
int8_t ble_tgt_pwr_tbl_user[5][4] = {
  /* 1M  2M  S8  S2 */
    { 0,  0,  0,  0},   // Global domain
    { 0,  0,  0,  0},   // FCC
    { 0,  0,  0,  0},   // CE
    { 0,  0,  0,  0},   // TELEC
    { 0,  0,  0,  0},   // SRRC
};

void user_setting_init(void)
{
#if CONFIG_PLATFORM == PLATFORM_ASIC_32103
    /* Country code: 0-Global domain; 1-FCC; 2-CE; 3-TELEC; 4-SRRC; others-unsupported */
    user_setting.country_code_user = 0;
    user_setting.country_code_user_enable = 0;

    /* thermal_value: 0x1-0xff */
    user_setting.thermal_value_user = 0x62;
    user_setting.thermal_value_user_enable = 0;

    user_setting.pwrbyrate_tbl_user_enable = 0;

    user_setting.ble_max_tx_pwr_user_enable = 0;

    user_setting.ble_tgt_tx_pwr_user_enable = 0;
#endif
}

void user_setting_pwrbyrate_tbl_check(uint8_t cck_pwr_idx_base, uint8_t ofdm_pwr_idx_base,
                uint8_t n20_pwr_idx_base, uint8_t ax20_pwr_idx_base, int8_t pwr_offset_min, uint8_t pwr_max)
{
    uint8_t i;

    if (user_setting.pwrbyrate_tbl_user_enable) {
        for (i = 0; i < 4; i++) {
            if (pwr_by_rate_sw_tbl_user[0][i] < pwr_offset_min)
                pwr_by_rate_sw_tbl_user[0][i] = pwr_offset_min;
            else if ((pwr_by_rate_sw_tbl_user[0][i] + cck_pwr_idx_base) > pwr_max)
                pwr_by_rate_sw_tbl_user[0][i] = pwr_max - cck_pwr_idx_base;
        }
        for (i = 0; i < 8; i++) {
            if (pwr_by_rate_sw_tbl_user[1][i] < pwr_offset_min)
                pwr_by_rate_sw_tbl_user[1][i] = pwr_offset_min;
            else if ((pwr_by_rate_sw_tbl_user[1][i] + ofdm_pwr_idx_base) > pwr_max)
                pwr_by_rate_sw_tbl_user[1][i] = pwr_max - ofdm_pwr_idx_base;
        }
        for (i = 0; i < 8; i++) {
            if (pwr_by_rate_sw_tbl_user[2][i] < pwr_offset_min)
                pwr_by_rate_sw_tbl_user[2][i] = pwr_offset_min;
            else if ((pwr_by_rate_sw_tbl_user[2][i] + n20_pwr_idx_base) > pwr_max)
                pwr_by_rate_sw_tbl_user[2][i] = pwr_max - n20_pwr_idx_base;
        }
        for (i = 0; i < 10; i++) {
            if (pwr_by_rate_sw_tbl_user[3][i] < pwr_offset_min)
                pwr_by_rate_sw_tbl_user[3][i] = pwr_offset_min;
            else if ((pwr_by_rate_sw_tbl_user[3][i] + ax20_pwr_idx_base) > pwr_max)
                pwr_by_rate_sw_tbl_user[3][i] = pwr_max - ax20_pwr_idx_base;
        }
    }
}

void user_setting_tgt_pwr_table(uint8_t table_idx, uint8_t value_idx, int8_t value, uint8_t enable)
{
    ble_tgt_pwr_tbl_user[table_idx][value_idx] = value;
    user_setting.ble_tgt_tx_pwr_user_enable = enable;
}

void user_setting_max_pwr_table(uint8_t table_idx, uint8_t value_idx, int8_t value, uint8_t enable)
{
    ble_max_pwr_tbl_user[table_idx][value_idx] = value;
    user_setting.ble_max_tx_pwr_user_enable = enable;
}

