/*!
    \file    mac_cfm.h
    \brief   MAC confirm related types definition.

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

#ifndef _MAC_CFM_H_
#define _MAC_CFM_H_

// Structure containing the parameters of the @ref DBG_GET_SYS_STAT_CFM message.
struct dbg_get_sys_stat_cfm
{
    // Time spent in CPU sleep since last reset of the system statistics
    //uint32_t cpu_sleep_time;
    // Time spent in DOZE since last reset of the system statistics
    uint32_t doze_time;
    // Total time spent since last reset of the system statistics
    uint32_t stats_time;
};

// Structure containing the parameters of the @ref MM_VERSION_CFM message.
struct mm_version_cfm
{
    // Version of the LMAC FW
    uint32_t version_lmac;
    // Version1 of the MAC HW (as encoded in version1Reg MAC HW register)
    uint32_t version_machw_1;
    // Version2 of the MAC HW (as encoded in version2Reg MAC HW register)
    uint32_t version_machw_2;
    // Version1 of the PHY (depends on actual PHY)
    uint32_t version_phy_1;
    // Version2 of the PHY (depends on actual PHY)
    uint32_t version_phy_2;
    // Supported Features
    uint32_t features;
    // Maximum number of supported stations
    uint16_t max_sta_nb;
    // Maximum number of supported virtual interfaces
    uint8_t max_vif_nb;
};

// Structure containing the parameters of the @ref MM_SET_CHANNEL_CFM message
struct mm_set_channel_cfm
{
    // Radio index to be used in policy table
    uint8_t radio_idx;
    // Actual TX power configured (in dBm)
    int8_t power;
};

// Structure containing the parameters of the @ref FTM_DONE_IND message.
struct ftm_done_ind
{
    // Index of the VIF for which the FTM is started
    uint8_t vif_idx;
    // Results
    struct mac_ftm_results results;
};

struct do_priv_cfm
{
    // result
    void* result;
};

#endif // _MAC_CFM_H_
