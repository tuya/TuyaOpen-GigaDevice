/*!
    \file    ble_profile_utils.h
    \brief   Header file of Profile Utils.

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

#ifndef _BLE_PROFILE_UTILS_H_
#define _BLE_PROFILE_UTILS_H_

#include <stdint.h>

/* Little Endian stream to uint8_t */
#define LE_STREAM_TO_UINT8(u8, s)   {               \
              u8 = (uint8_t)(*(s));                   \
              (s) += 1;                               \
        }

/* Little Endian stream to uint16_t */
#define LE_STREAM_TO_UINT16(u16, s)   {             \
              u16 = ((uint16_t)(*(s))) +              \
                    ((uint16_t)(*((s) + 1)) << 8);    \
              (s) += 2;                               \
      }

/* Little Endian uint8_t to stream */
#define LE_UINT8_TO_STREAM(s, u8)   {               \
              *(s)++ = (uint8_t)(u8);                 \
      }

/* Little Endian uint16_t to stream */
#define LE_UINT16_TO_STREAM(s, u16)   {             \
              *(s)++ = (uint8_t)(u16);                \
              *(s)++ = (uint8_t)((u16) >> 8);         \
      }

/* Possible values for setting client configuration characteristics */
enum ble_prf_cli_conf
{
    BLE_PRF_CLI_STOP_NTFIND = 0x0000,   /*!< Stop notification/indication */
    BLE_PRF_CLI_START_NTF = 0x0001,     /*!< Start notification */
    BLE_PRF_CLI_START_IND = 0x0002,     /*!< Start indication */
};

/* Time profile information */
typedef struct ble_prf_date_time
{
    uint16_t year;          /*!< Year element */
    uint8_t month;          /*!< Month element */
    uint8_t day;            /*!< Day element */
    uint8_t hour;           /*!< Hour element */
    uint8_t min;            /*!< Minute element */
    uint8_t sec;            /*!< Second element */
} ble_prf_date_time_t;

/* Date profile information */
typedef struct ble_prf_date
{
    uint16_t year;          /*!< Year element */
    uint8_t month;          /*!< Month element */
    uint8_t day;            /*!< Day element */
} ble_prf_date_t;

/**
 *  SFLOAT: Short Floating Point Type
 *
 * @verbatim
 *        +----------+----------+---------+
 *        | Exponent | Mantissa |  Total  |
 * +------+----------+----------+---------+
 * | size |  4 bits  | 12 bits  | 16 bits |
 * +------+----------+----------+---------+
 * @endverbatim
 */
typedef uint16_t ble_prf_sfloat;

/* utf8 string */
typedef struct ble_prf_utf_8
{
    uint16_t length;        /*!< Value length */
    uint8_t str[0];         /*!< Value string in UTF8 format */
} ble_prf_utf_8_t;

/*!
    \brief      Pack date and time information into little endian stream data buffer
    \param[in]  p_buf: pointer to buffer to fill the packed data
    \param[in]  p_date_time: pointer to date and time information
    \param[out] p_buf: pointer to the next byte in the buffer after packed data
    \retval     none
*/
void ble_prf_pack_date_time(uint8_t* p_buf, const ble_prf_date_time_t* p_date_time);

/*!
    \brief      Pack date information into little endian stream data buffer
    \param[in]  p_buf: pointer to buffer to fill the packed data
    \param[in]  p_date: pointer to date information
    \param[out] p_buf: pointer to the next byte in the buffer after packed data
    \retval     none
*/
void ble_prf_pack_date(uint8_t* p_buf, const ble_prf_date_t* p_date);

/*!
    \brief      Unpack date and time information from little endian stream data
    \param[in]  p_buf: pointer to buffer contain the packed data
    \param[in]  p_date_time: pointer to date and time information structure to set the unpacked value
    \param[out] p_buf: pointer to the next byte in the buffer after packed data
    \retval     none
*/
void ble_prf_unpack_date_time(uint8_t* p_buf, ble_prf_date_time_t* p_date_time);

/*!
    \brief      Unpack date information from little endian stream data
    \param[in]  p_buf: pointer to buffer contain the packed data
    \param[in]  p_date: pointer to date information structure to set the unpacked value
    \param[out] p_buf: pointer to the next byte in the buffer after packed data
    \retval     none
*/
void ble_prf_unpack_date(uint8_t* p_buf, ble_prf_date_t* p_date);

#endif /* _BLE_PROFILE_UTILS_H_ */
