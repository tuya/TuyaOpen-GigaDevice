/*!
    \file    hal_rf.h
    \brief   Header file for WLAN HAL RF.

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
#ifndef __HAL_RF_H
#define __HAL_RF_H
#include <stdint.h>

#define write_rf_fields(reg, fields, value)                             write_rf_reg(reg, \
                                                                        ((read_rf_reg(reg) & (~fields##_MASK)) \
                                                                        | ((value << (fields##_SHIFT)) & (fields##_MASK))))

#define read_rf_fields(reg, fields)                                     ((read_rf_reg(reg) & (fields##_MASK)) >> (fields##_SHIFT))

#define clear_rf_bits(reg, bits)                                        write_rf_reg(reg, \
                                                                        (read_rf_reg(reg) & (~(bits))))

#define set_rf_bits(reg, bits)                                          write_rf_reg(reg, \
                                                                        (read_rf_reg(reg) | (bits)))
enum
{
    RFTX_CTRL_BY_BB = 0,
    RFTX_CTRL_MANUAL = 1,
};

// rf cut definition
enum rf_version {
    RF_103_A_CUT = 0,
    RF_103_B_CUT = 1,
    RF_103_C_CUT = 2,
    RF_103_CUT_MAX,
};

#define NUM_24G_CHANNELS                        (14)
#define NUM_5G_CHANNELS                         (8) //modify it if support more

#define CRYSTAL_TUNE_BASE                       0x40
#define CRYSTAL_TUNE_TYPE_BASE                  0
#define CRYSTAL_TUNE_TYPE_ACCT                  1  // Acceleration
#define XTAL_FREQ_TUNING_DEFAULT                0x40
#define XTAL_CAP_VALUE_MIN                      0
#define XTAL_CAP_VALUE_MAX                      0x7F

enum
{
    RF_BW_20 = 1,
    RF_BW_40 = 2,
};

/// RF driver context structure.
struct rf_env_tag
{
    /// Crystal frequency
    uint8_t crystal_freq;
};

/*
 * GLOBAL VARIABLES
 ****************************************************************************************
 */
extern struct rf_env_tag rf_env;

/* register operation api */
uint32_t read_rf_reg(uint32_t regaddr);
void write_rf_reg(uint32_t regaddr, uint32_t data);

uint8_t rf_mdll_div_num_getf(void);
void rf_mdll_div_num_setf(uint8_t div_num);

uint8_t rf_fix_xtal_tuning_getf(void);
void rf_fix_xtal_tuning_setf(uint8_t enable);

uint8_t rf_xtal_tuning_getf(void);
void rf_xtal_tuning_setf(uint8_t tuning);

uint8_t rf_get_version(void);

uint8_t rf_get_tx_state(void);

uint8_t rf_force_tx_gain_index_getf(void);
void rf_force_tx_gain_index_setf(uint8_t enable);

uint8_t rf_tx_gain_index_value_getf(void);
void rf_tx_gain_index_value_setf(uint8_t value);

uint32_t rf_force_tx_gain_value_get(void);
void rf_force_tx_gain_value_set(uint32_t value);

uint32_t rf_tx_calcomp_wifi_get(void);
void rf_tx_calcomp_wifi_set(uint32_t value);

uint8_t rf_tx_calcomp_wifi_n_getf(void);
void rf_tx_calcomp_wifi_n_setf(uint8_t n);

uint8_t rf_tx_calcomp_wifi_b_getf(void);
void rf_tx_calcomp_wifi_b_setf(uint8_t b);

uint8_t rf_tx_calcomp_ble_getf(void);
void rf_tx_calcomp_ble_setf(uint8_t ble);

/* */
void rf_load_freq_k(void);
uint32_t rf_get_thermal(uint32_t sensor_chs, uint32_t count, uint32_t *result);
void rf_set_freq(uint8_t crystal_freq, uint32_t freq, uint32_t *result);
void rf_set_plldig(uint8_t crystal_freq, uint32_t fpll, uint32_t *result);
void rf_tx_filter_coeff_by_chan_plan(uint8_t filter_idx, uint8_t cck_filter_idx);
void rf_enable_pll(uint8_t fxtal);
void rf_bandwidth_config(uint8_t bandwidth);
void rf_channel_config(uint8_t bandwidth, uint8_t above, uint8_t crystal_freq, uint8_t channel);

uint8_t hal_init_rf(void);

/* common api */
int32_t rf_get_crystal_cap(void);
void rf_set_crystal_cap(uint32_t tune_type, int32_t tune_value);

#endif /* __HAL_RF_H */
