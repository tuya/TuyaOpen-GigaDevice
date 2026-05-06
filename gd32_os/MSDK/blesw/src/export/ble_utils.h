/*!
    \file    ble_utils.h
    \brief   Utility Functions and Definitions for BLE.

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

#ifndef _BLE_UTILS_H_
#define _BLE_UTILS_H_

#include <stdint.h>

#include "compiler.h"   // for INLINE

#ifdef __cplusplus
extern "C" {
#endif

/*!
    \brief      Macro to return value with one bit set
    \param[in]  pos: position of the bit to set
    \param[out] none
    \retval     The value with one bit set
*/
#ifndef BIT
#define BIT(pos)                (1UL << (pos))
#endif

/*!
    \brief      Macro to extract a field from a value containing several fields
    \param[in]  __r: bit field value
    \param[in]  __f: field name
    \param[out] none
    \retval     The value of the register masked and shifted
*/
#ifndef GETF
#define GETF(__r, __f)                                                           \
    (( (__r) & (__f##_MASK) ) >> (__f##_LSB))
#endif

/*!
    \brief      Macro to set a field value into a value containing several fields
    \param[in]  __r: bit field value
    \param[in]  __f: field name
    \param[in]  __v: value to put in field
    \param[out] none
    \retval     none
*/
#ifndef SETF
#define SETF(__r, __f, __v)                                                      \
    do {                                                                         \
        __r = (((__r) & ~(__f##_MASK)) | (__v) << (__f##_LSB));                  \
    } while (0)
#endif

/*!
    \brief      Macro to extract a bit from a value containing several fields
    \param[in]  __r: bit field value
    \param[in]  __b: bit field name
    \param[out] none
    \retval     The value of the register masked and shifted
*/
#ifndef GETB
#define GETB(__r, __b)                                                           \
    (( (__r) & (__b##_BIT) ) >> (__b##_POS))
#endif

/*!
    \brief      Macro to set a bit value into a value containing several fields
    \param[in]  __r: bit field value
    \param[in]  __b: bit field name
    \param[in]  __v: value to put in field
    \param[out] none
    \retval     none
*/
#ifndef SETB
#define SETB(__r, __b, __v)                                                      \
    do {                                                                         \
        __r = (((__r) & ~(__b##_BIT)) | (uint32_t)((__v ? 1 : 0) << (__b##_POS)));  \
    } while (0)
#endif

/*!
    \brief      Macro to toggle a bit in a value containing several bits
    \param[in]  __r: bit field value
    \param[in]  __b: bit field name
    \param[out] none
    \retval     none
*/
#ifndef TOGB
#define TOGB(__r, __b)                                                           \
    do {                                                                         \
        __r = ((__r) ^ (__b##_BIT));                                             \
    } while (0)
#endif

/* Get the number of elements within an array, give also number of rows in a 2-D array */
#ifndef ARRAY_LEN
#define ARRAY_LEN(array)   (sizeof((array)) / sizeof((array)[0]))
#endif

/*!
    \brief      Read an aligned 32 bits word
    \param[in]  ptr32: the address of the first byte of the 32 bits word
    \param[out] none
    \retval     uint32_t: the 32 bits value
*/
__INLINE uint32_t ble_read32(void const *ptr32)
{
    return *((uint32_t *)ptr32);
}

/*!
    \brief      Read an aligned 16 bits word
    \param[in]  ptr16: the address of the first byte of the 16 bits word
    \param[out] none
    \retval     uint16_t: the 16 bits value
*/
__INLINE uint16_t ble_read16(void const *ptr16)
{
    return *((uint16_t *)ptr16);
}

/*!
    \brief      Write an aligned 32 bits word
    \param[in]  ptr32: the address of the first byte of the 32 bits word
    \param[in]  value: the value to write
    \param[out] none
    \retval     none
*/
__INLINE void ble_write32(void const *ptr32, uint32_t value)
{
    *(uint32_t *)ptr32 = value;
}

/*!
    \brief      Write an aligned 16 bits word
    \param[in]  ptr16: the address of the first byte of the 16 bits word
    \param[in]  value: the value to write
    \param[out] none
    \retval     none
*/
__INLINE void ble_write16(void const *ptr16, uint32_t value)
{
    *(uint16_t *)ptr16 = value;
}

/*!
    \brief      Write a 8 bits word
    \param[in]  ptr8: the address of the first byte of the 8 bits word
    \param[in]  value: the value to write
    \param[out] none
    \retval     none
*/
__INLINE void ble_write8(void const *ptr8, uint32_t value)
{
    *(uint8_t *)ptr8 = value;
}

/*!
    \brief      Read a packed 16 bits word
    \param[in]  ptr16: the address of the first byte of the 16 bits word
    \param[out] none
    \retval     uint16_t: the 16 bits value
*/
__INLINE uint16_t ble_read16p(void const *ptr16)
{
    uint16_t value = ((uint8_t const volatile *)ptr16)[0] | ((uint8_t const volatile *)ptr16)[1] << 8;
    return value;
}

/*!
    \brief      Read a packed 24 bits word
    \param[in]  ptr24: the address of the first byte of the 24 bits word
    \param[out] none
    \retval     uint32_t: the 24 bits value
*/
__INLINE uint32_t ble_read24p(void const *ptr24)
{
    uint16_t addr_l, addr_h;
    addr_l = ble_read16p(ptr24);
    addr_h = *((uint8_t const volatile *)ptr24 + 2) & 0x00FF;
    return ((uint32_t)addr_l | (uint32_t)addr_h << 16);
}

/*!
    \brief      Write a packed 24 bits word
    \param[in]  ptr24: the address of the first byte of the 24 bits word
    \param[in]  value: the value to write
    \param[out] none
    \retval     none
*/
__INLINE void ble_write24p(void const *ptr24, uint32_t value)
{
    uint8_t volatile *ptr=(uint8_t*)ptr24;

    *ptr++ = (uint8_t)(value & 0xff);
    *ptr++ = (uint8_t)((value & 0xff00) >> 8);
    *ptr++ = (uint8_t)((value & 0xff0000) >> 16);
}

/*!
    \brief      Read a packed 32 bits word
    \param[in]  ptr32: the address of the first byte of the 32 bits word
    \param[out] none
    \retval     uint32_t: the 32 bits value
*/
__INLINE uint32_t ble_read32p(void const *ptr32)
{
    uint16_t addr_l, addr_h;
    addr_l = ble_read16p(ptr32);
    addr_h = ble_read16p((uint8_t *)ptr32 + 2);
    return ((uint32_t)addr_l | (uint32_t)addr_h << 16);
}

/*!
    \brief      Write a packed 32 bits word
    \param[in]  ptr32: the address of the first byte of the 32 bits word
    \param[in]  value: the value to write
    \param[out] none
    \retval     none
*/
__INLINE void ble_write32p(void const *ptr32, uint32_t value)
{
    uint8_t volatile *ptr=(uint8_t *)ptr32;

    *ptr++ = (uint8_t)(value & 0xff);
    *ptr++ = (uint8_t)((value & 0xff00) >> 8);
    *ptr++ = (uint8_t)((value & 0xff0000) >> 16);
    *ptr = (uint8_t)((value & 0xff000000) >> 24);
}

/*!
    \brief      Write a packed 16 bits word
    \param[in]  ptr16: the address of the first byte of the 16 bits word
    \param[in]  value: the value to write
    \param[out] none
    \retval     none
*/
__INLINE void ble_write16p(void const *ptr16, uint16_t value)
{
    uint8_t volatile *ptr=(uint8_t *)ptr16;

    *ptr++ = value & 0xff;
    *ptr = (value & 0xff00)>>8;
}

/*!
    \brief      Function to return the smallest of 2 unsigned 32 bits words
    \param[in]  a: unsigned 32 bits word to compare
    \param[in]  b: unsigned 32 bits word to compare
    \param[out] none
    \retval     uint32_t: the smallest value
*/
__INLINE uint32_t ble_min(uint32_t a, uint32_t b)
{
    return a < b ? a : b;
}

/*!
    \brief      Function to return the greatest of 2 unsigned 32 bits words
    \param[in]  a: unsigned 32 bits word to compare
    \param[in]  b: unsigned 32 bits word to compare
    \param[out] none
    \retval     uint32_t: the greatest value
*/
__INLINE uint32_t ble_max(uint32_t a, uint32_t b)
{
    return a > b ? a : b;
}

/*!
    \brief      Function to return the smallest of 2 signed 32 bits words
    \param[in]  a: signed 32 bits word to compare
    \param[in]  b: signed 32 bits word to compare
    \param[out] none
    \retval     int32_t: the smallest value
*/
__INLINE int32_t ble_min_s(int32_t a, int32_t b)
{
    return a < b ? a : b;
}

#ifdef __cplusplus
}
#endif

#endif /* _BLE_UITLS_H_ */
