/*!
    \file    macif_priv.h
    \brief   Definition of private request and associated structures

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

#ifndef __MACIF_PRIV_H_
#define __MACIF_PRIV_H_

/**
 * @brief WiFi private request type
 */
typedef enum {
    WIFI_PRIV_READ_MAC_REG,
    WIFI_PRIV_WRITE_MAC_REG,
    WIFI_PRIV_DUMP_MAC,
    WIFI_PRIV_READ_PHY_REG,
    WIFI_PRIV_WRITE_PHY_REG,
    WIFI_PRIV_DUMP_PHY,
    WIFI_PRIV_READ_RF_REG,
    WIFI_PRIV_WRITE_RF_REG,
    WIFI_PRIV_DUMP_RF,
    WIFI_PRIV_READ_AADC,
    WIFI_PRIV_SET_TX_RATE,
    WIFI_PRIV_SET_TX_POWER,
    WIFI_PRIV_GET_TX_POWER,
    WIFI_PRIV_FORCE_TX_POWER,
    WIFI_PRIV_ADD_TX_POWER,
    WIFI_PRIV_SET_RTS_CTS,
    WIFI_PRIV_DAC_TX_TEST,
    WIFI_PRIV_ENABLE_PS,
    WIFI_PRIV_DISABLE_PS,
    WIFI_PRIV_TEST_TX,
    WIFI_PRIV_CONTINUOUS_TX,
    WIFI_PRIV_TX_STOP,
    WIFI_PRIV_TEST_RX,
    WIFI_PRIV_CONF_TRIG,
    WIFI_PRIV_DUMP_ADC,
    WIFI_PRIV_AD_SPI_READ,
    WIFI_PRIV_AD_SPI_WRITE,
    WIFI_PRIV_SET_LPBK_MODE,
    WIFI_PRIV_SET_RA_PARAM,
    WIFI_PRIV_MP_MODE,
    WIFI_PRIV_SET_MP_TAGETPWR,
    WIFI_PRIV_SET_MP_PWROFFSET,
    WIFI_PRIV_TX_IQK,
    WIFI_PRIV_TX_DCK,
    WIFI_PRIV_TX_LOK,
    WIFI_PRIV_RX_IQK,
    WIFI_PRIV_RX_DCK,
    WIFI_PRIV_RCK,
    WIFI_PRIV_LNAK,
    WIFI_PRIV_SET_CRYSTAL_CAP,
    WIFI_PRIV_GET_CRYSTAL_CAP,
    WIFI_PRIV_GET_BANDWIDTH,
    WIFI_PRIV_HAL_WDOG_EN,
    WIFI_PRIV_RESET_TRX_COUNTERS,
    WIFI_PRIV_GET_TX_COUNTERS,
    WIFI_PRIV_GET_RX_COUNTERS,
    WIFI_PRIV_RX_EVM,
    WIFI_PRIV_RX_PSD,
    WIFI_PRIV_RX_FILTER,
    WIFI_PRIV_DM,
    WIFI_PRIV_DM_OPTION,
    WIFI_POWER_BY_RATE,
    WIFI_COUNTRY_CODE,
    WIFI_PRIV_WRITE_EFUSE_CHECK,
    WIFI_PRIV_SET_RF_FREQ,
    WIFI_PRIV_SET_PLLDIG,
    WIFI_PRIV_BYPASS_TX,
    WIFI_PRIV_MEM_TX,
    WIFI_PRIV_GET_MACADDR,
    WIFI_PRIV_INIT_CHANNEL_FLAGS,
    WIFI_PRIV_LISTEN_INTERVAL,
    WIFI_PRIV_OMI,

    WIFI_PRIV_ME_START = 0x80,
    WIFI_PRIV_ME_RESET,
    WIFI_PRIV_ME_SET_ACTIVE,

    WIFI_PRIV_MAX
} WIFI_PRIV_REQ_E;

typedef enum {
    CHANNEL_WIDTH_20 = 0,
    CHANNEL_WIDTH_40 = 1,
    CHANNEL_WIDTH_80 = 2,
    CHANNEL_WIDTH_160 = 3,
    CHANNEL_WIDTH_80_80 = 4,
    CHANNEL_WIDTH_MAX = 5,
} CHANNEL_WIDTH;

typedef enum {
    THERMAL_CHANNEL_XTAL = 0x00,
    THERMAL_CHANNEL_VDD  = 0x02,
    THERMAL_CHANNEL_PA   = 0x03,
} THERMAL_CHANNEL;

typedef enum {
    TEST_THERMAL_NONE   ,
    TEST_THERMAL_XTAL   ,
    TEST_THERMAL_PA     ,
    TEST_THERMAL_VDD    ,
    TEST_THERMAL_PA_XTAL,
    TEST_THERMAL_ALL    ,
    TEST_THERMAL_MAX
} TEST_THERMAL_MODE;

typedef enum {
    MP_DISABLE = 0          ,
    MP_RF_K_TEST_MODE       ,
    MP_RF_NORMAL_TEST_MODE  ,
    MP_RF_TEMP_TEST_MODE    ,
    MP_MODE_MAX
} MP_MODE;

struct bypass_tx_params {
    uint32_t mode;
    uint32_t count;
    uint32_t length;
    uint32_t delay;
    uint32_t rate;
    uint32_t sgi;
    uint32_t tone1;
    uint32_t tone2;
    uint32_t payload_type;
};

struct conf_trig_params {
    uint32_t la_mode;
    uint32_t trigger_source;
    uint32_t debug_port;
    uint32_t trigger_mask;
    uint32_t trigger_value;
    uint32_t trigger_event;
    uint32_t trigger_point;
    uint32_t simpling_mask;
};

#endif /* __MACIF_PRIV_H_ */
