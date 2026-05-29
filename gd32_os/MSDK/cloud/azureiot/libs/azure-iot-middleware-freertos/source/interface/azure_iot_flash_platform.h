/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @file azure_iot_flash_platform.h
 *
 * @brief Defines the flash platform interface for devices enabling ADU.
 */
#ifndef AZURE_IOT_FLASH_PLATFORM_H
#define AZURE_IOT_FLASH_PLATFORM_H

#include <stdint.h>

#include "azure_iot_result.h"

#include "azure_iot_flash_platform_port.h"

#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
#define ADU_IMAGE_HEADER_MAGIC  0xcafeface      //0x47494741   // "GIGA"
#define ADU_IMAGE_TYPE_VW553    1
#define ADU_IMAGE_TYPE_F527     2

typedef struct AzureAduImageHeader
{
    uint32_t ulMagic; // 0x47494741
    uint32_t ulVersion;  // package version + image version
    uint32_t ulImageLength;
    uint8_t ucImageType; //1:vw553, 2:F527
    uint8_t ucHeaderLength;
    uint16_t usPaddingLength;
} AzureAduImageHeader_t;
#endif


/**
 * @brief Initialize the flash platform.
 *
 * @param pxAduImage The #AzureADUImage_t to use for this operation.
 * @return AzureIoTResult_t
 */
AzureIoTResult_t AzureIoTPlatform_Init( AzureADUImage_t * const pxAduImage );

/**
 * @brief Get the size of a single boot bank.
 *
 * @return int64_t
 */
int64_t AzureIoTPlatform_GetSingleFlashBootBankSize();

#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
AzureIoTResult_t AzureIoTPlatform_ProcessBlock( AzureADUImage_t * const pxAduImage,
                                              uint32_t ulOffset,
                                              uint8_t * const pData,
                                              uint32_t ulBlockSize );
#else
/**
 * @brief Write a block of data to the image.
 *
 * @param pxAduImage The #AzureADUImage_t to use for this operation.
 * @param ulOffset The offset into the image from which to start writing.
 * @param pData The pointer to the data to write.
 * @param ulBlockSize The length of \p pData.
 * @return AzureIoTResult_t
 */
AzureIoTResult_t AzureIoTPlatform_WriteBlock( AzureADUImage_t * const pxAduImage,
                                              uint32_t ulOffset,
                                              uint8_t * const pData,
                                              uint32_t ulBlockSize );
#endif
/**
 * @brief Verify the bytes written to the image match a SHA256 hash.
 *
 * @param pxAduImage The #AzureADUImage_t to use for this operation.
 * @param pucSHA256Hash The pointer to the SHA256 hash.
 * @param ulSHA256HashLength The length of \p pucSHA256Hash.
 * @return AzureIoTResult_t
 */
AzureIoTResult_t AzureIoTPlatform_VerifyImage( AzureADUImage_t * const pxAduImage,
                                               uint8_t * pucSHA256Hash,
                                               uint32_t ulSHA256HashLength );

/**
 * @brief Enable the update image.
 *
 * @param pxAduImage The #AzureADUImage_t to use for this operation.
 * @return AzureIoTResult_t
 */
AzureIoTResult_t AzureIoTPlatform_EnableImage( AzureADUImage_t * const pxAduImage );

/**
 * @brief Reset the device.
 *
 * @param pxAduImage The #AzureADUImage_t to use for this operation.
 * @return AzureIoTResult_t
 */
AzureIoTResult_t AzureIoTPlatform_ResetDevice( AzureADUImage_t * const pxAduImage );

#endif /* AZURE_IOT_FLASH_PLATFORM_H */

