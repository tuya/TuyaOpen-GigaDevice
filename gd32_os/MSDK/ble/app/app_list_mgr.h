/*!
    \file    app_list_mgr.h
    \brief   Definitions of BLE application list operation manager to add device to fal/ral/pal.

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

#ifndef APP_LIST_MGR_H_
#define APP_LIST_MGR_H_

#include "ble_gap.h"

/* RAL information structure */
typedef struct ral_info
{
    uint8_t idx;    /*!< Device index */
    uint8_t mode;   /*!< Privacy Mode, @ref ble_gap_privacy_mode */
} ral_info_t;

/*!
    \brief      Set filter accept list
    \param[in]  num: number of provided device index
    \param[in]  p_array: pointer to array of device index used to set in filter accept list
    \param[out] none
    \retval     none
*/
void app_wl_set(uint8_t num, uint8_t *p_array);

/*!
    \brief      Add devices to filter accept list
    \param[in]  num: number of provided device index
    \param[in]  p_array: pointer to array of device index used to add in filter accept list
    \param[out] none
    \retval     none
*/
void app_wl_add(uint8_t num, uint8_t *p_array);

/*!
    \brief      Remove devices from filter accept list
    \param[in]  num: number of provided device index
    \param[in]  p_array: pointer to array of device index used to remove from filter accept list
    \param[out] none
    \retval     none
*/
void app_wl_rmv(uint8_t num, uint8_t *p_array);

/*!
    \brief      Add a device to filter accept list
    \param[in]  addr_type: address type
    \param[in]  p_addr: pointer to address used to add in filter accept list
    \param[out] none
    \retval     none
*/
void app_wl_add_addr(uint8_t addr_type, uint8_t *p_addr);

/*!
    \brief      Remove a device from filter accept list
    \param[in]  addr_type: address type
    \param[in]  p_addr: pointer to address used to remove from filter accept list
    \param[out] none
    \retval     none
*/
void app_wl_rmv_addr(uint8_t addr_type, uint8_t *p_addr);

/*!
    \brief      Clear filter accept list
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_wl_clear(void);

/*!
    \brief      Set periodic advertiser list
    \param[in]  num: number of provided device index
    \param[in]  p_array: pointer to array of device index used to set in periodic advertiser list
    \param[out] none
    \retval     none
*/
void app_pal_set(uint8_t num, uint8_t *p_array);

/*!
    \brief      Add devices to periodic advertiser list
    \param[in]  num: number of provided device index
    \param[in]  p_array: pointer to array of device index used to add in periodic advertiser list
    \param[out] none
    \retval     none
*/
void app_pal_add(uint8_t num, uint8_t *p_array);

/*!
    \brief      Remove device from periodic advertiser list
    \param[in]  num: number of provided device index
    \param[in]  p_array: pointer to array of device index used to remove from periodic advertiser list
    \param[out] none
    \retval     none
*/
void app_pal_rmv(uint8_t num, uint8_t *p_array);

/*!
    \brief      Clear periodic advertiser list
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_pal_clear(void);

/*!
    \brief      Set resolving list
    \param[in]  num: number of provided device index
    \param[in]  p_array: pointer to array of device index used to set in resolving list
    \param[out] none
    \retval     none
*/
void app_ral_set(uint8_t num, ral_info_t *p_array);

/*!
    \brief      Add devices to resolving list
    \param[in]  num: number of provided device index
    \param[in]  p_array: pointer to array of device index used to add in resolving list
    \param[out] none
    \retval     none
*/
void app_ral_add(uint8_t num, ral_info_t *p_array);

/*!
    \brief      Remove device from resolving list
    \param[in]  num: number of provided device index
    \param[in]  p_array: pointer to array of device index used to remove from resolving list
    \param[out] none
    \retval     none
*/
void app_ral_rmv(uint8_t num, ral_info_t *p_array);

/*!
    \brief      Clear resolving list
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_ral_clear(void);

/*!
    \brief      Init application list manager
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_list_mgr_init(void);

/*!
    \brief      Deinit application list manager
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_list_mgr_deinit(void);

/*!
    \brief      Reset application list manager
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_list_mgr_reset(void);

#endif // APP_LIST_MGR_H_
