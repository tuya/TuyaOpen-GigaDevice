/*!
    \file    nvds_flash.h
    \brief   Header file of nvds flash api for GD32VW55x SDK.

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

#ifndef _NVDS_FLASH_H_
#define _NVDS_FLASH_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>

/*
 * DEFINITIONS
 ****************************************************************************************
 */
// status code used in nvds_flash
enum nvds_ret {
    // NVDS status OK
    NVDS_OK = 0,
    // Not use flash, only used for not define NVDS_FLASH_SUPPORT
    NVDS_E_NOT_USE_FLASH,
    // generic NVDS status
    NVDS_E_FAIL,
    // NVDS invalid parameter
    NVDS_E_INVAL_PARAM,
    // flash read/write/erase api fail
    NVDS_E_FLASH_IO_FAIL,
    // NVDS data element not found
    NVDS_E_NOT_FOUND,
    // NVDS invalid length when get data
    NVDS_E_INVALID_LENGTH,
    // No space(flash/sram) for NVDS
    NVDS_E_NO_SPACE,
    // NVDS security config setting fail
    NVDS_E_SECUR_CFG_FAIL,
    // NVDS decryption failed while reading data
    NVDS_E_DECR_FAIL,
    // NVDS encryption failed while writing data
    NVDS_E_ENCR_FAIL,
};

#define NVDS_ERR(x)                     (x)

// define namespaces
#define NVDS_NS_BLE_PEER_DATA           "ble_peer_data"
#define NVDS_NS_BLE_LOCAL_DATA          "ble_local_data"
#define NVDS_NS_BLE_APP_DATA            "ble_app_data"

#define NVDS_NS_WIFI_INFO               "wifi_info"

typedef void (*found_keys_cb) (const char *namespace, const char *key, uint16_t val_len);
typedef void (*found_namespace_cb) (const char *namespace);

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

int nvds_find_keys_by_namespace(void *handle, const char *namespace, found_keys_cb cb);

int nvds_del_keys_by_namespace(void *handle, const char *namespace);

int nvds_find_all_namespace(void *handle, found_namespace_cb cb);

/**
 ****************************************************************************************
 * @brief Initialize nvds internal flash enviroment
 *
 * Internal flash storage base address is NVDS_FLASH_INTERNAL_ADDR
 * size is NVDS_FLASH_INTERNAL_SIZE
 *
 * @return status code
 *         NVDS_OK                  Initialize nvds internal flash ok
 *         NVDS_E_INVAL_PARAM       Internal address overlap with other nvds flash storage
 *                                  or internal flash size is illegal
 *         NVDS_E_NO_SPACE          Memory could not be allocated for the underlying structures
 ****************************************************************************************
 */
int nvds_flash_internal_init(void);

/**
 ****************************************************************************************
 * @brief Initialize nvds flash enviroment
 *
 * @param[in]  start_addr   Start flash address to keep nvds data
 * @param[in]  size         Size of nvds area in bytes
 *
 * @return If successful, handle of the nvds flash operation will be returned,
 *          otherwise return NULL.
 ****************************************************************************************
 */
void *nvds_flash_init(uint32_t start_addr, uint32_t size, const char *label);

/**
 ****************************************************************************************
 * @brief Deinitialize nvds flash enviroment
 *
 * @param[in]  handle       Handle of the nvds flash operation
 ****************************************************************************************
 */
void nvds_flash_deinit(void *handle);

/**
 ****************************************************************************************
 * @brief      Put element value with given key name and namespace if needed
 *
 * @param[in]  handle       Handle of the nvds flash operation, NULL indicate internal nvds flash
 * @param[in]  namespace    Set this value if looking for element with a specific namespace
 *                          NULL is for default namespace.
 *                          Maximum length is (KEY_NAME_MAX_SIZE - 1) characters.
 * @param[in]  key          Key name, shouldn't be empty.
 *                          Maximum length is (KEY_NAME_MAX_SIZE - 1) characters.
 * @param[in]  data         The value to set.
 * @param[in]  length       Length of value to set, in bytes;
 *
 * @return  NVDS_OK                 New data element correctly written to the nvds flash
 *          NVDS_E_FAIL             Generic nvds fail status
 *          NVDS_E_INVAL_PARAM      Parameter is invalid, such key, namespace has illegal length
 *          NVDS_E_FLASH_IO_FAIL    Flash api, such as flash read/write/erase operation fail
 *          NVDS_E_NO_SPACE         New element value can not fit in the available space in the NVDS
 ****************************************************************************************
 */
int nvds_data_put(void *handle, const char *namespace, const char *key, uint8_t *data, uint32_t length);

/**
 ****************************************************************************************
 * @brief      Get element value with given key name and namespace if needed,
 *              or get element length when data point is NULL
 *
 * If the length does not match, the real length of the given element will return by parameter,
 * in order for the caller to be able to check the actual length of the element.
 *
 * To get the size necessary to store the key value, call this function with zero pointer to data
 * and non-zero pointer to length. Variable pointed to by length argument will be
 * set to the required length.
 * When calling this function with non-zero pointer to data, length must be non-zero pointer,
 * and has to point to the length avilable in data.
 *
 * \code{c}
 * // Example (without error checking) of using nvds_data_get to get element value:
 * uint32_t required_size;
 * int ret = nvds_data_get(NULL, namespace, key, NULL, &required_size);
 * char *buffer = malloc(required_size);
 * int ret = nvds_data_get(NULL, namespace, key, buffer, &required_size);
 * \endcode
 *
 * @param[in]  handle       Handle of the nvds flash operation, NULL indicate internal nvds flash
 * @param[in]  namespace    Set this value if looking for element with a specific namespace
 *                          NULL is for default namespace.
 *                          Maximum length is (KEY_NAME_MAX_SIZE - 1) characters.
 * @param[in]  key          Key name, shouldn't be empty.
 *                          Maximum length is (KEY_NAME_MAX_SIZE - 1) characters.
 * @param[out] data         Pointer to key value, which should be already allocated
 * @param[inout] length     A non-zero pointer holding the length of data.
 *                          This function supports WinAPI-style length queries.
 *                          In case zero pointer to data, the actual length of the key value written
 *                          will set to length argument.
 *
 * @return  NVDS_OK                 The read operation was performed
 *          NVDS_E_FAIL             Generic nvds fail status
 *          NVDS_E_INVAL_PARAM      Parameter is invalid, such key, namespace has illegal length
 *          NVDS_E_FLASH_IO_FAIL    Flash api, such as flash read/write/erase operation fail
 *          NVDS_E_NOT_FOUND        The given element's namespace or key is not found
 *          NVDS_E_INVALID_LENGTH   The length passed in parameter is small than the element's
 ****************************************************************************************
 */
int nvds_data_get(void *handle, const char *namespace, const char *key, uint8_t *data, uint32_t *length);

/**
 ****************************************************************************************
 * @brief      Delete element with given key name and namespace if needed
 *
 * @param[in]  handle       Handle of the nvds flash operation, NULL indicate internal nvds flash
 * @param[in]  namespace    Set this value if looking for element with a specific namespace
 *                          NULL is for default namespace.
 *                          Maximum length is (KEY_NAME_MAX_SIZE - 1) characters.
 * @param[in]  key          Key name, shouldn't be empty.
 *                          Maximum length is (KEY_NAME_MAX_SIZE - 1) characters.
 *
 * @return  NVDS_OK                 The given element correctly found and deleted
 *          NVDS_E_FAIL             Generic nvds fail status
 *          NVDS_E_INVAL_PARAM      Parameter is invalid, such key, namespace has illegal length
 *          NVDS_E_FLASH_IO_FAIL    Flash api, such as flash read/write/erase operation fail
 *          NVDS_E_NOT_FOUND        The given element is not found
 ****************************************************************************************
 */
int nvds_data_del(void *handle, const char *namespace, const char *key);

/**
 ****************************************************************************************
 * @brief      Find element with given key name and namespace if needed
 *
 * @param[in]  handle       Handle of the nvds flash operation, NULL indicate internal nvds flash
 * @param[in]  namespace    Set this value if looking for element with a specific namespace
 *                          NULL is for default namespace.
 *                          Maximum length is (KEY_NAME_MAX_SIZE - 1) characters.
 * @param[in]  key          Key name, shouldn't be empty.
 *                          Maximum length is (KEY_NAME_MAX_SIZE - 1) characters.
 *
 * @return  NVDS_OK                 The given element correctly found
 *          NVDS_E_FAIL             Generic nvds fail status
 *          NVDS_E_INVAL_PARAM      Parameter is invalid, such key, namespace has illegal length
 *          NVDS_E_FLASH_IO_FAIL    Flash api, such as flash read/write/erase operation fail
 *          NVDS_E_NOT_FOUND        The given element is not found
 ****************************************************************************************
 */
int nvds_data_find(void *handle, const char *namespace, const char *key);

/**
 ****************************************************************************************
 * @brief      Erase nvds flash by label
 *
 * @param[in]  nvds_label   Select nvds by label to be erased

 * @return  NVDS_OK                 Erase ok
 *          NVDS_E_FAIL             Generic nvds fail status
 *          NVDS_E_FLASH_IO_FAIL    Flash erase operation fail
 *          NVDS_E_NOT_FOUND        The given nvds label is not found
 ****************************************************************************************
 */
int nvds_clean(const char *nvds_label);

/**
 ****************************************************************************************
 * @brief      Dump nvds flash data
 *
 * @param[in]  handle       Handle of the nvds flash operation, NULL indicate internal nvds flash
 * @param[in]  verbose      Dump all flash data when verbose = 1, else dump on-use data
 * @param[in]  namespace    Dump all key data in namespace when namespace is specified

 * @return  NVDS_OK                 Erase ok
 *          NVDS_E_FAIL             Generic nvds fail status
 *          NVDS_E_FLASH_IO_FAIL    Flash erase operation fail
 *          NVDS_E_NOT_FOUND        The given nvds label is not found
 ****************************************************************************************
 */
void nvds_dump(void *handle, uint8_t verbose, const char *namespace);

#endif /* _NVDS_FLASH_H_ */
