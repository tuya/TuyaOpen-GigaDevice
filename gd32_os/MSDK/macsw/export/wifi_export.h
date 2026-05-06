/*!
    \file    wifi_export.h
    \brief   Definition of WiFi export API.

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

#ifndef _WIFI_EXPORT_H_
#define _WIFI_EXPORT_H_

#include "co_bool.h"
#include "mac_types.h"

#define VIF_RSSI_HYSTERESIS      (5)
#define VIF_RSSI_THRESHOLD       (-70)
#define VIF_DHCP_TIMEOUT         (10000)//(5000)   //ms

enum {
    // wifi tx stop
    STOPMOD_WIFI_TX = 0,
    // wifi tx mode
    NORMALMOD_WIFI_PKT_TX,
    BYPASSMOD_DUTY_CYCLE_TX,
    BYPASSMOD_CONTINUOUS_TX,
    // dac tx, include single tone tx, dual tone and mem tx mode
    BYPASSMOD_SINGLE_TONE_TX,
    BYPASSMOD_DUAL_TONE_TX,
    BYPASSMOD_MEM_TX
};

// Maximum Number of CCK rates
#define N_CCK       8
// Maximum Number of OFDM rates
#define N_OFDM      8
// First OFDM rate index
#define FIRST_OFDM  N_CCK
// First HT rate index
#define FIRST_HT    FIRST_OFDM + N_OFDM
// First VHT rate index
#define FIRST_VHT   FIRST_HT + (8 * 2 * 2 * 4)
// First HE SU rate index
#define FIRST_HE_SU FIRST_VHT + (10 * 4 * 2 * 8)
// First HE MU rate index
#define FIRST_HE_MU FIRST_HE_SU + (12 * 4 * 3 * 8)
// First HE ER rate index
#define FIRST_HE_ER FIRST_HE_MU + (12 * 6 * 3 * 8)
// Total Number of rates
#define TOT_RATES   FIRST_HE_ER + (3 * 3 + 3)

#define GD32VW55X_WIFI_MUL_INTS

// Definitions of WLAN PTI frame types
enum wlan_pti_frame_type
{
    WLAN_PTI_ACK   = 0,
    WLAN_PTI_CNTL,
    WLAN_PTI_MGMT,
    WLAN_PTI_VO_DATA,
    WLAN_PTI_VI_DATA,
    WLAN_PTI_BE_DATA,
    WLAN_PTI_BK_DATA,
    WLAN_PTI_BCN,
    WLAN_PTI_TYPE_MAX
};

static inline int wifi_freq_to_channel(uint16_t freq)
{
    if ((freq >= 2412) && (freq <= 2484)) {
        if (freq == 2484)
            return 14;
        else
            return (freq - 2407) / 5;
    }
    return 0;
}

static inline uint16_t wifi_channel_to_freq(int channel)
{
    if ((channel >= 1) && (channel <= 14)) {
        if (channel == 14)
            return 2484;
        else
            return 2407 + channel * 5;
    }
    return 0;
}

/**
 ****************************************************************************************
 * @brief Init WiFi core and create wifi core tasks.
 *
 * @param[in] init_mac Init MAC or not
 * @param[in] use_wpa_supplicant use wpa_supplicant or not
 ****************************************************************************************
 */
int wifi_core_init(bool init_mac, bool use_wpa_supplicant);

/**
 ****************************************************************************************
 * @brief Deinit WiFi core and terminate wifi core tasks.
 *
 * @param[in] use_wpa_supplicant use wpa_supplicant or not
 ****************************************************************************************
 */
void wifi_core_deinit(bool use_wpa_supplicant);

/**
 ****************************************************************************************
 * @brief Request the RTOS to resume the WiFi task.
 * This function first checks if the task was indeed suspended and then proceed to the
 * resume. Note that currently this function is supposed to be called from interrupt.
 *
 * @param[in] isr Indicates if called from ISR
 ****************************************************************************************
 */
void wifi_core_task_resume(bool isr);

/**
 ****************************************************************************************
 * @brief This function performs the wake up from DOZE mode.
 *
 ****************************************************************************************
 */
void wifi_wakeup(int from_isr);

/**
 ****************************************************************************************
 * @brief Set WLAN PTI value to register.
 * @param[in] frame_type wlan frame type
 * @param[in] pti pti to set
 *
 * @return none
 ****************************************************************************************
 */
void coex_set_wlan_pti(enum wlan_pti_frame_type frame_type, uint32_t pti);

/**
 ****************************************************************************************
 * @brief Get WLAN PTI value from register.
 * @param[in] frame_type wlan frame type
 *
 * @return pti of the frame type
 ****************************************************************************************
 */
uint8_t coex_get_wlan_pti(enum wlan_pti_frame_type frame_type);

#ifdef CFG_WIFI_MESH_SMART
void macif_set_user_vendor_oui(uint8_t *vendor_id);
void macif_get_user_vendor_oui(uint8_t *vendor_id);
#endif

extern bool wifi_in_doze(void);
extern void wifi_wakeup_isr(void);

extern int wifi_rc_print_rate(char *buf, int size, uint32_t rate_config,
                        uint8_t ru_size, int *r_idx);

extern int wifi_rc_config(uint32_t config_type, int32_t param1, int32_t param2);

//extern bool phy_vht_supported(void);
extern bool phy_he_supported(void);
extern uint8_t phy_get_bw(void);
extern void intc_init(void);
extern void intc_deinit(void);
extern void intc_irq(void);
extern void hal_machw_gen_handler(void);
extern void txl_prot_trigger(void);
extern void txl_transmit_trigger(void);
extern void rxl_mpdu_isr(void);
extern void phy_mdm_isr(void);
extern void phy_rc_isr(void);
extern void hal_la_isr(void);
extern void mac_bypass_single_tone_tx(float freq);
extern void mac_bypass_tx_finish(bool restore_power);
extern uint32_t hal_calc_agc_table_crc(void);
extern void sysctrl_init(void);
#endif // _WIFI_EXPORT_H_
