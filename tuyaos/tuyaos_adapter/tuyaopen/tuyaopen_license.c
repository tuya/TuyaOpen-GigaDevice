/**
 * @file tuyaopen_license.c
 * @brief tuyaopen_license module is used to
 * @version 0.1
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 */

#include <string.h>
#include <stdio.h>
#include "gd32vw55x.h"
#include "tuyaopen_license.h"
#include "config_gdm32.h"
#include "rom_export.h"
#include "tkl_flash.h"
#include "tkl_memory.h"
#include "gd32vw55x_cau.h"
#include "gd32vw55x_rcu.h"
#include "gd32vw55x_fmc.h"

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

#define LICENSE_JSON_FORMAT "{\"auzkey\":\"%s\",\"uuid\":\"%s\"}"
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
static const uint8_t s_authorize_aad[] = "tuya-uuidauthkey";
static const uint8_t s_authorize_gcm_iv[GCM_IV_LENGTH] = { // "TuyaOpenFixedIV1"
    0x54, 0x75, 0x79, 0x61, 0x4f, 0x70, 0x65, 0x6e, 0x46, 0x69, 0x78, 0x65, 0x64, 0x49, 0x56, 0x31,
};
static char s_authorize_uuid[UUID_LENGTH + 1] = {0};
static char s_authorize_authkey[AUTHKEY_LENGTH + 1] = {0};

/***********************************************************
***********************function define**********************
***********************************************************/
static int __authorize_gcm_encrypt(const uint8_t *plain, size_t plain_len, const uint8_t *aad,
    const uint8_t *iv, uint8_t *ciphertext, uint8_t *tag)
{
    cau_parameter_struct cau_gcm_parameter;
    uint8_t padded_plain[GCM_DATA_LENGTH] = {0};
    uint8_t gcm_key[AES_KEY_SZ] = {0};

    if ((plain_len > sizeof(padded_plain)) || (NULL == aad) || (NULL == iv) ||
        (NULL == ciphertext) || (NULL == tag)) {
        return -1;
    }

    if (rom_do_symm_key_derive((uint8_t *)aad, GCM_BLOCK_LENGTH, gcm_key, AES_KEY_SZ)) {
        printf("Authorization GCM key derive failed.\r\n");
        return -2;
    }

    sys_memcpy(padded_plain, plain, plain_len);

    cau_gcm_parameter.alg_dir = CAU_ENCRYPT;
    cau_gcm_parameter.key = gcm_key;
    cau_gcm_parameter.key_size = GCM_KEY_BITS;
    cau_gcm_parameter.iv = (uint8_t *)iv;
    cau_gcm_parameter.iv_size = GCM_IV_LENGTH;
    cau_gcm_parameter.input = padded_plain;
    cau_gcm_parameter.in_length = GCM_DATA_LENGTH;
    cau_gcm_parameter.aad = (uint8_t *)aad;
    cau_gcm_parameter.aad_size = GCM_BLOCK_LENGTH;

    rcu_periph_clock_enable(RCU_CAU);
    cau_deinit();
    if (SUCCESS != cau_aes_gcm(&cau_gcm_parameter, ciphertext, tag)) {
        // printf("Authorization CAU GCM encrypt failed.\r\n");
        return -3;
    }

    return 0;
}

/**
 * @brief Extract a string value of the given key from a (non null-terminated safe) JSON buffer
 * @param[in] json JSON buffer
 * @param[in] json_len JSON buffer length
 * @param[in] key Key name to search, e.g. "uuid"
 * @param[out] out Output buffer, always null-terminated on success
 * @param[in] out_size Size of the output buffer (including the terminating '\0')
 * @return value length on success, -1 on failure
 */
static int __license_json_get_string(const char *json, uint32_t json_len, const char *key, char *out,
                                     size_t out_size)
{
    size_t key_len = 0;
    uint32_t i = 0;
    uint32_t start = 0;

    if ((NULL == json) || (NULL == key) || (NULL == out) || (out_size < 2)) {
        return -1;
    }

    key_len = strlen(key);
    if ((0 == key_len) || (json_len < key_len + 2)) {
        return -1;
    }

    /* locate "key" */
    for (i = 0; i + key_len + 2 <= json_len; i++) {
        if (('"' == json[i]) && ('"' == json[i + key_len + 1]) && (0 == memcmp(&json[i + 1], key, key_len))) {
            break;
        }
    }
    if (i + key_len + 2 > json_len) {
        return -1;
    }

    i += key_len + 2;

    /* skip spaces before ':' */
    while ((i < json_len) && (' ' == json[i])) {
        i++;
    }
    if ((i >= json_len) || (':' != json[i])) {
        return -1;
    }
    i++;

    /* skip spaces before the opening quote */
    while ((i < json_len) && (' ' == json[i])) {
        i++;
    }
    if ((i >= json_len) || ('"' != json[i])) {
        return -1;
    }
    i++;

    start = i;
    while ((i < json_len) && ('"' != json[i])) {
        i++;
    }
    if (i >= json_len) {
        return -1;
    }

    if ((size_t)(i - start) >= out_size) {
        return -1;
    }

    memcpy(out, &json[start], i - start);
    out[i - start] = '\0';

    return (int)(i - start);
}

/**
 * @brief Write license data
 * @param[in] data Pointer to license data
 * @param[in] data_len Length of license data
 * @return OPRT_NOT_SUPPORTED - operation not supported
 */
int tuyaopen_license_write(const char *data, const uint32_t data_len)
{
    int ret = 0;
    int uuid_len = 0;
    int authkey_len = 0;
    authorize_flash_record_t record = {0};
    char uuid[UUID_LENGTH + 1] = {0};
    char authkey[AUTHKEY_LENGTH + 1] = {0};
    char *read_data = NULL;
    uint32_t read_data_len = 0;
    char read_uuid[UUID_LENGTH + 1] = {0};
    char read_authkey[AUTHKEY_LENGTH + 1] = {0};

    if ((NULL == data) || (0 == data_len)) {
        return OPRT_INVALID_PARM;
    }

    if (tuya_license_protection_status_get() == 1) {
        // Maybe the license page is locked, auth information may be already written
        return OPRT_COM_ERROR;
    }

    uuid_len = __license_json_get_string(data, data_len, "uuid", uuid, sizeof(uuid));
    authkey_len = __license_json_get_string(data, data_len, "auzkey", authkey, sizeof(authkey));
    if ((uuid_len <= 0) || (AUTHKEY_LENGTH != authkey_len)) {
        // printf("Authorization license JSON missing uuid/auzkey.\r\n");
        return OPRT_INVALID_PARM;
    }

    if (sizeof(record) > TUYA_FLASH_LICENSE_SIZE) {
        // printf("License protect flash size not enough.\r\n");
        return OPRT_COM_ERROR;
    }

    sys_memcpy(record.aad, s_authorize_aad, sizeof(record.aad));
    sys_memcpy(record.iv, s_authorize_gcm_iv, sizeof(record.iv));

    ret = __authorize_gcm_encrypt((const uint8_t *)uuid, (size_t)uuid_len, record.aad, record.iv, record.uuid_cipher,
        record.uuid_tag);
    if (0 != ret) {
        // printf("Authorization uuid encrypt failure.\r\n");
        goto fail;
    }

    ret = __authorize_gcm_encrypt((const uint8_t *)authkey, (size_t)authkey_len, record.aad, record.iv,
        record.authkey_cipher, record.authkey_tag);
    if (0 != ret) {
        // printf("Authorization authkey encrypt failure.\r\n");
        goto fail;
    }

    ret = raw_flash_erase(TUYA_FLASH_LICENSE_START, 0x1000); // erase one page
    if (0 != ret) {
        // printf("Authorization flash erase failed.\r\n");
        goto fail;
    }

    if (0 != raw_flash_write(TUYA_FLASH_LICENSE_START, (const uint8_t *)&record, sizeof(record))) {
        // printf("Authorization write failure.\r\n");
        goto fail;
    }

    ret = tuyaopen_license_read(&read_data, &read_data_len);
    if (OPRT_OK != ret) {
        // printf("Authorization write verification by read failed, ret = %d.\r\n", ret);
        goto fail;
    }

    if ((__license_json_get_string(read_data, read_data_len, "uuid", read_uuid, sizeof(read_uuid)) != uuid_len) ||
        (__license_json_get_string(read_data, read_data_len, "auzkey", read_authkey, sizeof(read_authkey)) !=
         authkey_len) ||
        (0 != memcmp(read_uuid, uuid, (size_t)uuid_len)) ||
        (0 != memcmp(read_authkey, authkey, (size_t)authkey_len))) {
        // printf("Authorization write readback plain-text comparison failed.\r\n");
        tkl_system_free(read_data);
        goto fail;
    }

    tkl_system_free(read_data);

    if (OPRT_OK != tuya_license_lock()) {
        // printf("Authorization license lock failed.\r\n");
        goto fail;
    }

    return OPRT_OK;

fail:
    return OPRT_COM_ERROR;
}

int tuyaopen_license_erase(void)
{
    if (tuya_license_protection_status_get() == 1) {
        tuya_license_unlock(); // unlock first
    }

    if (0 != raw_flash_erase(TUYA_FLASH_LICENSE_START, 0x1000)) {
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

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

/**
 * @brief Lock the license page by enabling write protection.
 *
 * @return OPRT_OK on success, OPRT_COM_ERROR on failure.
 */
int tuya_license_lock(void)
{
    fmc_state_enum state;
    uint32_t license_page = TUYA_FLASH_LICENSE_START / 0x1000;
    char uuid[UUID_LENGTH + 1] = {0};
    char authkey[AUTHKEY_LENGTH + 1] = {0};
    int protection_status;

    protection_status = tuya_license_protection_status_get();
    if (protection_status < 0) {
        return protection_status;
    }
    if (protection_status > 0) {
        return OPRT_OK; /* already locked */
    }

    fmc_unlock();
    ob_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR);
    /* lock one page */
    state = ob_write_protection_config(license_page, license_page, OBWRP_INDEX1);

    ob_lock();
    fmc_lock();

    if (state != FMC_READY) {
        // printf("lock failed, state = %d.\r\n", state);
        return OPRT_COM_ERROR;
    }

    if (1 != tuya_license_protection_status_get()) {
        // printf("lock verify failed.\r\n");
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief Get the protection status of the license page.
 *
 * @return 1 if the license page is locked, 0 if unlocked, OPRT_COM_ERROR on error.
 */
int tuya_license_protection_status_get(void)
{
    uint32_t license_page = TUYA_FLASH_LICENSE_START / 0x1000;
    uint32_t obwrp1 = FMC_OBWRP1;
    uint32_t protected_start = obwrp1 & FMC_OBWRP1_WRP1_SPAGE;
    uint32_t protected_end = (obwrp1 & FMC_OBWRP1_WRP1_EPAGE) >> 16;

    if (protected_start > protected_end) {
        return 0; /* invalid/empty region: protection disabled -> unlocked */
    }

    /* protection enabled on [protected_start, protected_end] */
    if (protected_start == license_page && protected_end == license_page) {
        return 1; /* license page locked */
    }

    if (license_page >= protected_start && license_page <= protected_end) {
        // printf("tuya_license_protection_status_get: license page is covered by a larger WRP1 region [%lu, %lu].\r\n",
        //        (unsigned long)protected_start, (unsigned long)protected_end);
        return OPRT_COM_ERROR;
    }

    /* WRP1 protects an unrelated region, license page is not locked */
    // printf("tuya_license_protection_status_get: WRP1 protects unrelated region [%lu, %lu].\r\n",
    //        (unsigned long)protected_start, (unsigned long)protected_end);
    return OPRT_COM_ERROR;
}

/**
 * @brief Unlock the license page by disabling write protection.
 *
 * @return OPRT_OK on success, OPRT_COM_ERROR on failure.
 */
int tuya_license_unlock(void)
{
    fmc_state_enum state;
    int protection_status = tuya_license_protection_status_get();

    if (protection_status < 0) {
        return protection_status;
    }
    if (0 == protection_status) {
        return OPRT_OK; /* already unlocked */
    }

    fmc_unlock();
    ob_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR);
    /* disable write protection: start page > end page means empty region */
    state = ob_write_protection_config(0x3ffU, 0U, OBWRP_INDEX1);
    ob_lock();
    fmc_lock();

    if (state != FMC_READY) {
        // printf("unlock failed, state = %d.\r\n", state);
        return OPRT_COM_ERROR;
    }

    if (0 != tuya_license_protection_status_get()) {
        // printf("unlock verify failed.\r\n");
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}
