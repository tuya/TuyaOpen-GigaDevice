/*!
    \file    app_flash.c
    \brief   Implementation of BLE flash operation.

    \version 2024-07-24, V1.0.0, firmware for GD32VW55x
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

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "ble_app_config.h"

#if (BLE_APP_SUPPORT)
#include <string.h>
#include "ble_types.h"
#include "nvds_flash.h"
#include "app_flash.h"

/*!
    \brief      save app data from flash
    \param[in]  key: key name, shouldn't be empty. Maximum length is (KEY_NAME_MAX_SIZE - 1) characters.
    \param[in]  p_data: pointer to key value, which should be already allocated
    \param[in]  p_length: a non-zero pointer holding the length of data
    \param[in]  max_length: data max length
    \param[out] none
    \retval     bool: true if load successfully, otherwise false
*/
bool app_flash_load(const char *key, uint8_t *p_data, uint32_t *p_length, uint32_t max_length)
{
    uint32_t str_len = 0;

    if (!nvds_data_get(NULL, NVDS_NS_BLE_APP_DATA, key, NULL, &str_len) && (str_len <= max_length)) {       //for taking data length first
        if(!nvds_data_get(NULL, NVDS_NS_BLE_APP_DATA, key, p_data, &str_len)) {
            *p_length = str_len;
            return true;
        }
    }

    return false;
}

/*!
    \brief      save app data to flash
    \param[in]  key: key name, shouldn't be empty.Maximum length is (KEY_NAME_MAX_SIZE - 1) characters.
    \param[in]  p_data: pointer to the value to save.
    \param[in]  length: length of value to set, in bytes;.
    \param[out] none
    \retval     bool: true if save successfully, otherwise false
*/
bool app_flash_save(const char *key, uint8_t *p_data, uint32_t length)
{
    uint32_t str_len = 0;

    if (!nvds_data_put(NULL, NVDS_NS_BLE_APP_DATA, key, p_data, length)) {
        return true;
    }

    return false;
}
#endif // (BLE_APP_SUPPORT)
