/**
 * @file tuyaopen_license.c
 * @brief tuyaopen_license module is used to
 * @version 0.1
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 */

#include <string.h>
#include <stdio.h>
#include "tuyaopen_license.h"
#include "config_gdm32.h"
#include "rom_export.h"
#include "tkl_flash.h"
#include "tkl_memory.h"
#include "gd32vw55x_cau.h"
#include "gd32vw55x_rcu.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define TUYA_FLASH_LICENSE_SIZE (0x1000) // 4K
#define TUYA_FLASH_LICENSE_START (RE_NVDS_DATA_OFFSET - TUYA_FLASH_LICENSE_SIZE)

#define UUID_LENGTH      20
#define AUTHKEY_LENGTH   32
#define GCM_KEY_BITS     128
#define GCM_IV_LENGTH    16
#define GCM_TAG_LENGTH   16
#define GCM_BLOCK_LENGTH 16
#define GCM_DATA_LENGTH  32

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct {
    uint8_t uuid_cipher[GCM_DATA_LENGTH];
    uint8_t uuid_tag[GCM_TAG_LENGTH];
    uint8_t authkey_cipher[GCM_DATA_LENGTH];
    uint8_t authkey_tag[GCM_TAG_LENGTH];
    uint8_t aad[GCM_BLOCK_LENGTH];
    uint8_t iv[GCM_IV_LENGTH];
} authorize_flash_record_t;


/***********************************************************
********************function declaration********************
***********************************************************/


/***********************************************************
***********************variable define**********************
***********************************************************/


/***********************************************************
***********************function define**********************
***********************************************************/

/**
 * @brief Write license data
 * @param[in] data Pointer to license data
 * @param[in] data_len Length of license data
 * @return OPRT_NOT_SUPPORTED - operation not supported
 */
int tuyaopen_license_write(const char *data, const uint32_t data_len)
{
    return OPRT_NOT_SUPPORTED;
}

static char s_authorize_uuid[UUID_LENGTH + 1] = {0};
static char s_authorize_authkey[AUTHKEY_LENGTH + 1] = {0};

static OPERATE_RET __authorize_gcm_decrypt(const uint8_t *ciphertext, const uint8_t *aad, const uint8_t *iv,
                                           const uint8_t *tag, uint8_t *out, size_t out_len);

static int __authorize_tag_equal(const uint8_t *left, const uint8_t *right, size_t len)
{
    uint8_t diff = 0;
    size_t i;

    for (i = 0; i < len; i++) {
        diff |= left[i] ^ right[i];
    }

    return (0 == diff);
}

static OPERATE_RET __authorize_gcm_decrypt(const uint8_t *cipher, const uint8_t *aad, const uint8_t *iv,
                                           const uint8_t *tag, uint8_t *out, size_t out_len)
{
    cau_parameter_struct cau_gcm_parameter;
    uint8_t padded_plain[GCM_DATA_LENGTH] = {0};
    uint8_t calculated_tag[GCM_TAG_LENGTH] = {0};
    uint8_t gcm_key[AES_KEY_SZ] = {0};
    authorize_flash_record_t record = {0};

    if ((out_len > sizeof(padded_plain)) || (NULL == cipher) || (NULL == aad) || (NULL == iv) || (NULL == tag) ||
        (NULL == out)) {
        return OPRT_INVALID_PARM;
    }

    if (rom_do_symm_key_derive((uint8_t *)aad, GCM_BLOCK_LENGTH, gcm_key, AES_KEY_SZ)) {
        // printf("Authorization GCM key derive failed.\r\n");
        return OPRT_COM_ERROR;
    }

    cau_gcm_parameter.alg_dir = CAU_DECRYPT;
    cau_gcm_parameter.key = gcm_key;
    cau_gcm_parameter.key_size = GCM_KEY_BITS;
    cau_gcm_parameter.iv = (uint8_t *)iv;
    cau_gcm_parameter.iv_size = GCM_IV_LENGTH;
    cau_gcm_parameter.input = (uint8_t *)cipher;
    cau_gcm_parameter.in_length = GCM_DATA_LENGTH;
    cau_gcm_parameter.aad = (uint8_t *)aad;
    cau_gcm_parameter.aad_size = GCM_BLOCK_LENGTH;

    rcu_periph_clock_enable(RCU_CAU);
    cau_deinit();
    if (SUCCESS != cau_aes_gcm(&cau_gcm_parameter, padded_plain, calculated_tag)) {
        // printf("Authorization CAU GCM decrypt failed.\r\n");
        return OPRT_COM_ERROR;
    }

    if (!__authorize_tag_equal(tag, calculated_tag, GCM_TAG_LENGTH)) {
        // printf("Authorization CAU GCM tag verification failed.\r\n");
        return OPRT_COM_ERROR;
    }

    memcpy(out, padded_plain, out_len);
    return OPRT_OK;
}

#define LICENSE_JSON_FORMAT "{\"auzkey\":\"%s\",\"uuid\":\"%s\"}"

/**
 * @brief Read license data
 * @param[out] data Pointer to store license data address (allocated via tkl_system_malloc,
 *                  caller is responsible for freeing it with tkl_system_free), format:
 *                  {"auzkey":"xxx","uuid":"xxx"} JSON string, NOT including the terminating '\0' in data_len
 * @param[out] data_len Pointer to store license data length
 * @return OPRT_OK on success, others on error, please refer to tuya_error_code.h
 */
int tuyaopen_license_read(char **data, uint32_t *data_len)
{
    char *license_data = NULL;
    authorize_flash_record_t record = {0};
    int json_len = 0;

    if ((NULL == data) || (NULL == data_len)) {
        return OPRT_INVALID_PARM;
    }

    if ((OPRT_OK != tkl_flash_read(TUYA_FLASH_LICENSE_START, (uint8_t *)&record, sizeof(record))) ||
        (OPRT_OK != __authorize_gcm_decrypt(record.uuid_cipher, record.aad, record.iv, record.uuid_tag,
                             (uint8_t *)s_authorize_uuid, UUID_LENGTH)) ||
        (OPRT_OK != __authorize_gcm_decrypt(record.authkey_cipher, record.aad, record.iv, record.authkey_tag,
                             (uint8_t *)s_authorize_authkey, AUTHKEY_LENGTH))) {
        return OPRT_COM_ERROR;
    }

    s_authorize_uuid[UUID_LENGTH] = '\0';
    s_authorize_authkey[AUTHKEY_LENGTH] = '\0';

    json_len = snprintf(NULL, 0, LICENSE_JSON_FORMAT, s_authorize_authkey, s_authorize_uuid);
    if (json_len <= 0) {
        return OPRT_COM_ERROR;
    }

    license_data = (char *)tkl_system_malloc((size_t)json_len + 1);
    if (NULL == license_data) {
        return OPRT_MALLOC_FAILED;
    }

    snprintf(license_data, (size_t)json_len + 1, LICENSE_JSON_FORMAT, s_authorize_authkey, s_authorize_uuid);

    *data = license_data;
    *data_len = (uint32_t)json_len;

    return OPRT_OK;
}
