/*!
    \file    user_setting.h
    \brief   Header file of user setting for GD32VW55x SDK.

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
#ifndef _USER_SETTING_H_
#define _USER_SETTING_H_

// user setting parameter
typedef struct user_setting_param_t {
    /* Country code: 0-Global domain; 1-FCC; 2-CE; 3-TELEC; 4-SRRC; others-unsupported */
    uint8_t country_code_user;
    uint8_t country_code_user_enable;

    /* thermal_value: 0x1-0xff */
    uint8_t thermal_value_user;
    uint8_t thermal_value_user_enable;

    uint8_t pwrbyrate_tbl_user_enable;

    uint8_t ble_tgt_tx_pwr_user_enable;

    uint8_t ble_max_tx_pwr_user_enable;

    /* waiting for further expansion */
} user_setting_param_t;

#define RATE_CATEGORY                  4

extern user_setting_param_t user_setting;
extern int8_t pwr_by_rate_sw_tbl_user[RATE_CATEGORY][10];
extern const int8_t pwr_limit_tbl_fcc_user[RATE_CATEGORY][11];
extern const int8_t pwr_limit_tbl_etsi_user[RATE_CATEGORY][13];
extern const int8_t pwr_limit_tbl_telec_user[RATE_CATEGORY][14];
extern const int8_t pwr_limit_tbl_srrc_user[RATE_CATEGORY][13];
extern int8_t ble_max_pwr_tbl_user[5][4];
extern int8_t ble_tgt_pwr_tbl_user[5][4];

void user_setting_init(void);
void user_setting_pwrbyrate_tbl_check(uint8_t cck_pwr_idx_base, uint8_t ofdm_pwr_idx_base,
                uint8_t n20_pwr_idx_base, uint8_t ax20_pwr_idx_base, int8_t pwr_offset_min, uint8_t pwr_max);
void user_setting_tgt_pwr_table(uint8_t table_idx, uint8_t value_idx, int8_t value, uint8_t enable);
void user_setting_max_pwr_table(uint8_t table_idx, uint8_t value_idx, int8_t value, uint8_t enable);
#endif // _USER_SETTING_H_