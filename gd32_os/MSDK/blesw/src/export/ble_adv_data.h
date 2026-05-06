/*!
    \file    ble_adv_data.h
    \brief   Implementation of functions for BLE advertising data encoding and searching
             data of specific advertising types.

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

#ifndef _BLE_ADV_DATA_H_
#define _BLE_ADV_DATA_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "ble_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Advertising data contains 1 byte for length */
#define AD_LEN_SIZE                         1

/* Advertising data contains 1 byte for AD type */
#define AD_TYPE_SIZE                        1

/* Advertising data header size, 1 byte length and 1 byte type */
#define AD_DATA_HDR_SIZE                    (AD_LEN_SIZE + AD_TYPE_SIZE)

/* Data size in octets of AD type Appearance */
#define AD_TYPE_APPEARANCE_DATA_SIZE        2

/* Data size in octets of AD type Flags */
#define AD_TYPE_FLAGS_DATA_SIZE             1

/* Data size in octets of AD type TX Power Level */
#define AD_TYPE_TX_PWR_LVL_DATA_SIZE        1

/* Data size in octets of AD type Slave Connection Interval Range */
#define AD_TYPE_CONN_INT_DATA_SIZE          4

/* Size in octets of 16-bit UUID, which is a part of the Service Data AD type */
#define AD_TYPE_DATA_UUID_16_SIZE           2

/* Size in octets of 32-bit UUID, which is a part of the Service Data AD type */
#define AD_TYPE_DATA_UUID_32_SIZE           4

/* Size in octets of 128-bit UUID, which is a part of the Service Data AD type */
#define AD_TYPE_DATA_UUID_128_SIZE          16

/* Data size in octets of AD type Advertising Interval */
#define AD_TYPE_ADV_INT_DATA_SIZE           2

/* Data size in octets of AD type Public Target Address */
#define AD_TYPE_PUB_TGT_ADDR_DATA_SIZE      6

/* Data size in octets of AD type Random Target Address */
#define AD_TYPE_RND_TGT_ADDR_DATA_SIZE      6

/* Size in octets of the Company Identifier Code, which is a part of AD type Manufacturer Specific Data */
#define AD_TYPE_MANUF_SPEC_DATA_ID_SIZE     2

/* Advertising data name type */
typedef enum
{
    BLE_ADV_DATA_NO_NAME,               /*!< Include no device name in advertising data */
    BLE_ADV_DATA_SHORT_NAME,            /*!< Include short device name in advertising data */
    BLE_ADV_DATA_FULL_NAME              /*!< Include full device name in advertising data */
} ble_adv_data_name_type_t;

/* Structure of advertising data for type Name */
typedef struct
{
    ble_adv_data_name_type_t    type;       /*!< Name type */
    uint8_t                     name_len;   /*!< Name length */
    uint8_t                    *p_name;     /*!< Name value */
} ble_adv_data_name_t;

/* Structure of advertising data for type UUID List */
typedef struct
{
    uint16_t        uuid_cnt;       /*!< UUID count in the list */
    ble_uuid_t     *p_uuid;         /*!< UUID value list */
} ble_adv_data_uuid_list_t;

/* Structure of advertising data for type Connection Interval */
typedef struct
{
    uint16_t        min_conn_intv;      /*!< Min connection interval in units of 1.25 ms, range 6 to 3200 (7.5 ms to 4 s) */
    uint16_t        max_conn_intv;      /*!< Max connection interval in units of 1.25 ms, range 6 to 3200 (7.5 ms to 4 s). The value 0xFFFF indicates no specific maximum */
} ble_adv_data_conn_intv_t;

/* Structure of advertising data for type Manufacturer Specific Data */
typedef struct
{
    uint16_t        company_id;         /*!< Company identifier code */
    uint16_t        data_len;           /*!< Size of additional manufacturer specific data */
    uint8_t        *p_data;             /*!< Pointer to additional manufacturer specific data */
} ble_adv_data_manuf_data_t;

/* Service data structure */
typedef struct
{
    ble_uuid_t      uuid;               /*!< Service UUID */
    uint16_t        data_len;           /*!< Size of service data */
    uint8_t        *p_data;             /*!< Pointer to service data */
} ble_adv_data_srv_data_t;

/* Structure of advertising data for type Service Data List */
typedef struct
{
    uint8_t                     cnt;    /*!< Service data count in the list */
    ble_adv_data_srv_data_t    *p_data; /*!< Service data value list */
} ble_adv_data_srv_data_list_t;

/* Structure of advertising data for type URL */
typedef struct
{
    uint8_t        *p_url;      /*!< URL data */
    uint16_t        url_len;    /*!< URL length */
} ble_adv_data_url_t;

/* Structure of advertising data. APP can fill the structure and BLE ADV module will encode it into advertising data */
typedef struct
{
    ble_adv_data_name_t             local_name;             /*!< Local name */
    uint8_t                         flags;                  /*!< Flags, 0 means no AD type flags in advertising data */
    uint16_t                        appearance;             /*!< Appearance, 0 means no AD type appearance in advertising data */
    int8_t                         *p_tx_pwr;               /*!< Pointer to tx power, NULL means no AD type tx power in advertising data */
    ble_adv_data_uuid_list_t        uuid_more_available;    /*!< UUID list for AD type Incomplete List of Service UUIDs */
    ble_adv_data_uuid_list_t        uuid_complete;          /*!< UUID list for AD type Complete List of Service UUIDs */
    ble_adv_data_uuid_list_t        uuid_solicited;         /*!< UUID list for AD type Service Solicitation */
    ble_adv_data_conn_intv_t       *p_slave_conn_intv;      /*!< Slave Connection Interval Range, NULL means no sucn AD type in advertising data */
    ble_adv_data_manuf_data_t      *p_manuf_specific_data;  /*!< Manufacturer Specific Data, NULL means no sucn AD type in advertising data */
    ble_adv_data_srv_data_list_t    srv_data;               /*!< Service Data, count 0 means no such AD type in advertising data */
    uint16_t                        adv_intv;               /*!< Advertising Interval. 0 means no such AD type in advertising data */
    ble_adv_data_url_t              url;                    /*!< URL, length 0 means no such AD type in advertising data */
    uint8_t                        *p_pub_tgt_addr;         /*!< Public Target Address, NULL means no such AD type in advertising data */
    uint8_t                        *p_rand_tgt_addr;        /*!< Random Target Address, NULL means no such AD type in advertising data.*/
} ble_adv_data_t;

/*!
    \brief      Find specific AD type in the advertising data
    \param[in]  p_data: pointer to advertising data
    \param[in]  data_len: advertising data length
    \param[in]  ad_type: ad type to find
    \param[out] p_len: pointer to value length found in the advertising data
    \retval     uint8_t *: NULL if no such AD type in the advertising data, otherwise pointer to the value address
*/
uint8_t *ble_adv_find(uint8_t *p_data, uint16_t data_len, uint8_t ad_type, uint8_t *p_len);

/*!
    \brief      Find complete name in the advertising data
    \param[in]  p_data: pointer to advertising data
    \param[in]  data_len: advertising data length
    \param[in]  p_name: pointer to the complete name need to find
    \param[in]  name_len: length of the complete name
    \param[out] none
    \retval     bool: true if the complete name is found in the advertising data, otherwise false
*/
bool ble_adv_cmpl_name_find(uint8_t *p_data, uint16_t data_len, uint8_t *p_name, uint16_t name_len);

/*!
    \brief      Find short name in the advertising data
    \param[in]  p_data: pointer to advertising data
    \param[in]  data_len: advertising data length
    \param[in]  p_name: pointer to the short name need to find
    \param[in]  name_len_min: minimum length of the name that need to match
    \param[out] none
    \retval     bool: true if the short name with minimum length is found in the advertising data, otherwise false
*/
bool ble_adv_short_name_find(uint8_t *p_data, uint16_t data_len, uint8_t *p_name, uint16_t name_len_min);

/*!
    \brief      Find specific UUID in the advertising data
    \param[in]  p_data: pointer to advertising data
    \param[in]  data_len: advertising data length
    \param[in]  p_uuid: pointer to the UUID type and value to find
    \param[out] none
    \retval     bool: true if the specific UUID is found in the advertising data, otherwise false
*/
bool ble_adv_svc_uuid_find(uint8_t *p_data, uint16_t data_len, ble_uuid_t *p_uuid);

/*!
    \brief      Find appearace in the advertising data
    \param[in]  p_data: pointer to advertising data
    \param[in]  data_len: advertising data length
    \param[in]  appearance: appearance value
    \param[out] none
    \retval     bool: true if the appearace is found in the advertising data, otherwise false
*/
bool ble_adv_appearance_find(uint8_t *p_data, uint16_t data_len, uint16_t appearance);

#ifdef __cplusplus
}
#endif

#endif // _BLE_ADV_DATA_H_
