/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @file azure_iot_flash_platform.c
 *
 * @brief Port file for the GigaDevice flash abstraction.
 *
 */

#include <string.h>
#include "azure_iot_flash_platform.h"
#include "azure_iot_flash_platform_port.h"

#include "config_gdm32.h"
#include "rom_export.h"
#include "raw_flash_api.h"
#include "wrapper_os.h"
#include "gd32vw55x.h"

/* Logging */
#include "azure_iot.h"
#include "azure/core/az_base64.h"
#include "mbedtls/md.h"
#include "rom_export_mbedtls.h"
#include "mbedtls/sha256.h"

#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
#include "atcmd_azure.h"
#endif

#define azureiotflashSHA_256_SIZE             32

static uint8_t ucPartitionReadBuffer[ 32 ];
static uint8_t ucDecodedManifestHash[ azureiotflashSHA_256_SIZE ];
static uint8_t ucCalculatedHash[ azureiotflashSHA_256_SIZE ];

#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
static uint32_t ucCurrentImageType;
static uint32_t ulCurrentImageLength;
static uint32_t ulCurrentImageOffset;
static uint32_t ulNextImagePosition;
static uint32_t ulImageVw553Size;

static uint32_t ulImageF527Version;
static uint8_t ucCalculatedF527Hash[ azureiotflashSHA_256_SIZE ];

mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
static mbedtls_md_context_t ctx;        // for package from Azure
static mbedtls_md_context_t f527_ctx;   // for f527 image

#endif

static AzureIoTResult_t prvBase64Decode( uint8_t * base64Encoded,
                                         size_t ulBase64EncodedLength,
                                         uint8_t * pucOutputBuffer,
                                         size_t bufferLen,
                                         size_t * outputSize )
{
    az_result xCoreResult;

    az_span encodedSpan = az_span_create( base64Encoded, ulBase64EncodedLength );

    az_span outputSpan = az_span_create( pucOutputBuffer, bufferLen );

    if( az_result_failed( xCoreResult = az_base64_decode( outputSpan, encodedSpan, ( int32_t * ) outputSize ) ) )
    {
        AZLogError( ( "az_base64_decode failed: core error=0x%08x", xCoreResult ) );
        return eAzureIoTErrorFailed;
    }

    AZLogInfo( ( "Unencoded the base64 encoding\r\n" ) );

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_Init( AzureADUImage_t * const pxAduImage )
{
    int32_t xResult = eAzureIoTSuccess;
    uint32_t ulImageMaxLen = 0;

    pxAduImage->xUpdatePartition = 0;
    pxAduImage->ulImageFileSize = 0;
    pxAduImage->ulCurrentOffset = 0;
    xResult = rom_sys_status_get( SYS_RUNNING_IMG, LEN_SYS_RUNNING_IMG, &pxAduImage->xRunningIdx );
    if( xResult < 0 )
    {
        AZLogError( (  "OTA get running idx failed! (res = %d)\r\n", xResult ) );
        return xResult;
    }

    if( pxAduImage->xRunningIdx == IMAGE_0 )
    {
        pxAduImage->xUpdatePartition = RE_IMG_1_OFFSET;
        ulImageMaxLen = RE_IMG_1_END - RE_IMG_1_OFFSET;
    }
    else
    {
        pxAduImage->xUpdatePartition = RE_IMG_0_OFFSET;
        ulImageMaxLen = RE_IMG_1_OFFSET - RE_IMG_0_OFFSET;
    }

    xResult = raw_flash_erase( pxAduImage->xUpdatePartition, ulImageMaxLen );
    if( xResult < 0 )
    {
        AZLogError( ( "OTA flash erase failed (res = %d)\r\n", xResult ) );
    }

#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
    ucCurrentImageType = 0;
    ulCurrentImageLength = 0;
    ulCurrentImageOffset = 0;
    ulNextImagePosition = 0;
    ulImageVw553Size = 0;
    ulImageF527Version = 0;

    mbedtls_md_init( &ctx );
    mbedtls_md_setup( &ctx, mbedtls_md_info_from_type( md_type ), 0 );
    mbedtls_md_starts( &ctx );

    mbedtls_md_init( &f527_ctx );
    mbedtls_md_setup( &f527_ctx, mbedtls_md_info_from_type( md_type ), 0 );
    mbedtls_md_starts( &f527_ctx );
#endif
    return xResult;
}

int64_t AzureIoTPlatform_GetSingleFlashBootBankSize()
{
    return MIN( RE_IMG_1_END - RE_IMG_1_OFFSET, RE_IMG_1_OFFSET - RE_IMG_0_OFFSET );
}

#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
static AzureIoTResult_t AzureIoTPlatform_WriteBlock( uint8_t ucImageType,
                                              uint32_t pucNextWriteAddr,
                                              uint8_t * const pData,
                                              uint32_t ulBlockSize )
{
    AzureIoTResult_t xResult = eAzureIoTSuccess;

    if (ucImageType == ADU_IMAGE_TYPE_VW553) {
        xResult = raw_flash_write( pucNextWriteAddr, ( uint8_t * )pData, ulBlockSize );
        if( xResult < 0 )
        {
            AZLogError( ( "OTA flash write failed!\r\n" ) );
            xResult = eAzureIoTErrorFailed;
        }
    } else {
        mbedtls_md_update( &f527_ctx, pData, ulBlockSize );
        if (atcmd_azure_ota_block_send(( char * )pData, ulBlockSize)) {
            AZLogError( ( "OTA block send failed\r\n" ) );
            xResult = eAzureIoTErrorFailed;
        }
    }
    return xResult;
}

AzureIoTResult_t AzureIoTPlatform_ProcessBlock( AzureADUImage_t * const pxAduImage,
                                              uint32_t ulOffset,
                                              uint8_t * const pData,
                                              uint32_t ulBlockSize )
{

    uint32_t pucNextWriteAddr, ulWriteSize;
    uint8_t * pBlock = (uint8_t *)pData;
    AzureIoTResult_t xResult = eAzureIoTSuccess;
    AzureAduImageHeader_t *xHeader;
    uint32_t ulLocalVersion;
    char pVersion[32];
    int lReturn;

    mbedtls_md_update( &ctx, ( const unsigned char * ) pData, ulBlockSize);

    if (ulOffset == 0)
    {
        xHeader = (AzureAduImageHeader_t *) pBlock;
        // check image magic
        if (xHeader->ulMagic != ADU_IMAGE_HEADER_MAGIC) {
            AZLogError( ( "OTA image header magic error (%x != %x)\r\n", xHeader->ulMagic, ADU_IMAGE_HEADER_MAGIC ) );
            goto Error;
        }
        // check image type
        if ((xHeader->ucImageType != ADU_IMAGE_TYPE_F527) && (xHeader->ucImageType != ADU_IMAGE_TYPE_VW553) ) {
            AZLogError( ( "OTA image header type error (%x)\r\n", xHeader->ucImageType) );
            goto Error;
        }
        ucCurrentImageType = xHeader->ucImageType;

        // check version
        if (ucCurrentImageType == ADU_IMAGE_TYPE_F527) {
            sprintf(pVersion, "F527 version 0x%x", (xHeader->ulVersion));
            ulImageF527Version = xHeader->ulVersion;
            if (atcmd_azure_ota_ind_send(pVersion, xHeader->ulImageLength)) {
                AZLogError( ( "OTA refused\r\n" ) );
                goto Error;
            }
        } else {
            //Major(4bits) + Minor(4bits) + Rev(8bits)
            //Major(8bits) + Minor(8bits) + Rev(16bits) RE_IMG_VERSION
            ulLocalVersion = ((RE_IMG_VERSION & 0x0F000000) >> 12 | (RE_IMG_VERSION & 0x0F0000) >> 8 | (RE_IMG_VERSION & 0xFF));
            AZLogInfo( ( "Upgrade VW553 version vs Remote (0x%x vs 0x%x)\r\n" , ulLocalVersion, (xHeader->ulVersion & 0xFFFF) ));

            if ((xHeader->ulVersion & 0xFFFF) < ulLocalVersion) {
                AZLogError( ( "Upgrade version is less than current version (%x vs %x)\r\n" ,
                        xHeader->ulVersion, ulLocalVersion) );
                goto Error;
            }
            ulImageVw553Size = xHeader->ulImageLength;
        }

        // store length
        ulCurrentImageLength = xHeader->ulImageLength;
        ulCurrentImageOffset = 0;
        ulNextImagePosition = xHeader->ucHeaderLength + xHeader->ulImageLength + xHeader->usPaddingLength;
        pBlock += xHeader->ucHeaderLength;
        ulBlockSize -= xHeader->ucHeaderLength;
    }

    if (ulOffset + ulBlockSize <= ulNextImagePosition) {
        if (ulCurrentImageOffset < ulCurrentImageLength) {
            pucNextWriteAddr = pxAduImage->xUpdatePartition + ulCurrentImageOffset;
            ulWriteSize = ulBlockSize;
            if (ulCurrentImageOffset + ulBlockSize > ulCurrentImageLength)
                ulWriteSize = ulCurrentImageLength - ulCurrentImageOffset;
            xResult = AzureIoTPlatform_WriteBlock(ucCurrentImageType, pucNextWriteAddr,
                                                ( uint8_t * )pBlock, ulWriteSize );
            if( xResult < 0 )
            {
                AZLogError( ( "OTA flash write failed!\r\n" ) );
                goto Error;
            }
            ulCurrentImageOffset += ulWriteSize;
        }
    } else if ((ulOffset + ulBlockSize > ulNextImagePosition)
                && (ulOffset <= ulNextImagePosition))  {
        if (ulCurrentImageOffset < ulCurrentImageLength) {
            pucNextWriteAddr = pxAduImage->xUpdatePartition + ulCurrentImageOffset;
            ulWriteSize = ulCurrentImageLength - ulCurrentImageOffset;
            xResult = AzureIoTPlatform_WriteBlock(ucCurrentImageType, pucNextWriteAddr,
                                                ( uint8_t * )pBlock, ulWriteSize );
            if( xResult < 0 )
            {
                AZLogError( ( "OTA flash write failed!\r\n" ) );
                goto Error;
            }
        } else {
            ulWriteSize = 0;
        }

        /* padding Length */
        pBlock += (ulNextImagePosition - ulOffset);
        ulBlockSize -= (ulNextImagePosition - ulOffset);

        /* ==========================Next Image ================================*/
        xHeader = (AzureAduImageHeader_t *) pBlock;
        if (xHeader->ulMagic != ADU_IMAGE_HEADER_MAGIC) {
            AZLogError( ( "OTA image header magic error (%x != %x)\r\n", xHeader->ulMagic, ADU_IMAGE_HEADER_MAGIC ) );
            goto Error;

        }
        ucCurrentImageType = xHeader->ucImageType;
        if ((xHeader->ucImageType != ADU_IMAGE_TYPE_F527) && (xHeader->ucImageType != ADU_IMAGE_TYPE_VW553) ) {
            AZLogError( ( "OTA image header type error (%x)\r\n", xHeader->ucImageType ) );
            goto Error;
        }

        // check version
        if (ucCurrentImageType == ADU_IMAGE_TYPE_F527) {
            //ignore package version first
            AZLogInfo( ( "Upgrade F527 version is(0x%x)\r\n" , xHeader->ulVersion) );
            ulImageF527Version = xHeader->ulVersion;
            sprintf(pVersion, "0x%x", (xHeader->ulVersion));
            if (atcmd_azure_ota_ind_send(pVersion, xHeader->ulImageLength)) {
                AZLogError( ( "OTA refused\r\n" ) );
                goto Error;
            }
        } else {

            //Major(4bits) + Minor(4bits) + Rev(8bits)
            //Major(8bits) + Minor(8bits) + Rev(16bits) RE_IMG_VERSION
            ulLocalVersion = ((RE_IMG_VERSION & 0x0F000000) >> 12 | (RE_IMG_VERSION & 0x0F0000) >> 8 | (RE_IMG_VERSION & 0xFF));
            AZLogInfo( ( "Upgrade VW553 version vs Remote (0x%x vs 0x%x)\r\n" , ulLocalVersion, (xHeader->ulVersion & 0xFFFF) ));

            if ((xHeader->ulVersion & 0xFFFF) < ulLocalVersion) {
                AZLogError( ( "Upgrade version is less than current version (%x vs %x)\r\n" ,
                        xHeader->ulVersion, ulLocalVersion) );
                goto Error;
            }
            ulImageVw553Size = xHeader->ulImageLength;
        }

        ulCurrentImageLength = xHeader->ulImageLength;
        ulCurrentImageOffset = 0;
        pBlock += xHeader->ucHeaderLength;
        ulBlockSize -= xHeader->ucHeaderLength;

        pucNextWriteAddr = pxAduImage->xUpdatePartition + ulCurrentImageOffset;
        xResult = AzureIoTPlatform_WriteBlock(ucCurrentImageType, pucNextWriteAddr,
                                            pBlock, ulBlockSize );
        if( xResult < 0 )
        {
            AZLogError( ( "OTA flash write failed!\r\n" ) );
            goto Error;
        }
        ulCurrentImageOffset += ulBlockSize;
    } else { // ulOffset > ulNextImagePosition
        if (ulCurrentImageOffset < ulCurrentImageLength) {
            if (ulCurrentImageOffset + ulBlockSize > ulCurrentImageLength)
                ulBlockSize = ulCurrentImageLength - ulCurrentImageOffset;
            pucNextWriteAddr = pxAduImage->xUpdatePartition + ulCurrentImageOffset;
            xResult = AzureIoTPlatform_WriteBlock(ucCurrentImageType, pucNextWriteAddr,
                                                pBlock, ulBlockSize );
            if( xResult < 0 )
            {
                AZLogError( ( "OTA flash write failed!\r\n" ) );
                goto Error;
            }
        }
    }

    return xResult;

Error:

    mbedtls_md_finish( &ctx, ucCalculatedHash );
    mbedtls_md_free( &ctx );

    mbedtls_md_finish( &f527_ctx, ucCalculatedF527Hash );
    mbedtls_md_free( &f527_ctx );

    return xResult;
}

AzureIoTResult_t AzureIoTPlatform_VerifyImage( AzureADUImage_t * const pxAduImage,
                                               uint8_t * pucSHA256Hash,
                                               uint32_t ulSHA256HashLength )
{
    int xResult;
    uint32_t ulOutputSize;
    uint32_t ulReadSize;
    uint32_t ulRxHashSize = 32;
    uint8_t ucRxHash[32] = {0};

    mbedtls_md_finish( &ctx, ucCalculatedHash );
    mbedtls_md_free( &ctx );

    mbedtls_md_finish( &f527_ctx, ucCalculatedF527Hash );
    mbedtls_md_free( &f527_ctx );

    AZLogInfo( ( "Base64 Encoded Hash from ADU: %.*s", ulSHA256HashLength, pucSHA256Hash ) );
    xResult = prvBase64Decode( pucSHA256Hash, ulSHA256HashLength, ucDecodedManifestHash, azureiotflashSHA_256_SIZE, ( size_t * ) &ulOutputSize );
    if( xResult != eAzureIoTSuccess )
    {
        AZLogError( ( "Unable to decode base64 SHA256\r\n" ) );
        return eAzureIoTErrorFailed;
    }

    atcmd_azure_ota_hash_recv((char *)ucRxHash, ulRxHashSize);
#if 0
    AZLogInfo( ( "SHAs from 527, len=%d\r\n", ulRxHashSize) );
    for( int i = 0; i < azureiotflashSHA_256_SIZE; ++i )
    {
        AZLogInfo( ( "%x", ucRxHash[ i ] ) );
    }

    AZLogInfo( ( "SHAs for 527 local, len=%d\r\n", ulRxHashSize) );
    for( int i = 0; i < azureiotflashSHA_256_SIZE; ++i )
    {
            AZLogInfo( ( "%x", ucCalculatedF527Hash[ i ] ) );
    }
#endif

    if( memcmp( ucRxHash, ucCalculatedF527Hash, azureiotflashSHA_256_SIZE ) == 0 )
    {
            AZLogInfo( ( "F527 SHAs match\r\n" ) );
            xResult = eAzureIoTSuccess;
    } else {
        AZLogError( ( "F527 SHAs do not match\r\n" ) );
        return eAzureIoTErrorFailed;
    }

    AZLogInfo( ( "mbedtls calculation completed\r\n" ) );

    if( memcmp( ucDecodedManifestHash, ucCalculatedHash, azureiotflashSHA_256_SIZE ) == 0 )
    {
        AZLogInfo( ( "SHAs match\r\n" ) );
        xResult = eAzureIoTSuccess;
    }
    else
    {
        AZLogError( ( "SHAs do not match\r\n" ) );
        AZLogInfo( ( "Wanted: " ) );

        for( int i = 0; i < azureiotflashSHA_256_SIZE; ++i )
        {
            AZLogInfo( ( "%x", ucDecodedManifestHash[ i ] ) );
        }

        AZLogInfo( ( "\r\n" ) );
        AZLogInfo( ( "Calculated: " ) );

        for( int i = 0; i < azureiotflashSHA_256_SIZE; ++i )
        {
            AZLogInfo( ( "%x", ucCalculatedHash[ i ] ) );
        }

        AZLogInfo( ( "\r\n" ) );

        xResult = eAzureIoTErrorFailed;
    }

    return xResult;
}
#else
AzureIoTResult_t AzureIoTPlatform_WriteBlock( AzureADUImage_t * const pxAduImage,
                                              uint32_t ulOffset,
                                              uint8_t * const pData,
                                              uint32_t ulBlockSize )
{

    uint32_t pucNextWriteAddr = pxAduImage->xUpdatePartition + ulOffset;
    AzureIoTResult_t xResult = eAzureIoTSuccess;

    xResult = raw_flash_write( pucNextWriteAddr, ( uint8_t * )pData, ulBlockSize );
    if( xResult < 0 )
    {
        AZLogError( ( "OTA flash write failed!\r\n" ) );
        xResult = eAzureIoTErrorFailed;
    }

    return xResult;
}

AzureIoTResult_t AzureIoTPlatform_VerifyImage( AzureADUImage_t * const pxAduImage,
                                               uint8_t * pucSHA256Hash,
                                               uint32_t ulSHA256HashLength )
{
    int xResult;
    uint32_t ulOutputSize;
    uint32_t ulReadSize;

    AZLogInfo( ( "Base64 Encoded Hash from ADU: %.*s", ulSHA256HashLength, pucSHA256Hash ) );
    xResult = prvBase64Decode( pucSHA256Hash, ulSHA256HashLength, ucDecodedManifestHash, azureiotflashSHA_256_SIZE, ( size_t * ) &ulOutputSize );

    if( xResult != eAzureIoTSuccess )
    {
        AZLogError( ( "Unable to decode base64 SHA256\r\n" ) );
        return eAzureIoTErrorFailed;
    }

    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    mbedtls_md_init( &ctx );
    mbedtls_md_setup( &ctx, mbedtls_md_info_from_type( md_type ), 0 );
    mbedtls_md_starts( &ctx );

    AZLogInfo( ( "Starting the mbedtls calculation: image size %d\r\n", pxAduImage->ulImageFileSize ) );

    for( size_t ulOffset = 0; ulOffset < pxAduImage->ulImageFileSize; ulOffset += sizeof( ucPartitionReadBuffer ) )
    {
        ulReadSize = pxAduImage->ulImageFileSize - ulOffset < sizeof( ucPartitionReadBuffer ) ? pxAduImage->ulImageFileSize - ulOffset : sizeof( ucPartitionReadBuffer );

        xResult = raw_flash_read( pxAduImage->xUpdatePartition + ulOffset, ( void * )ucPartitionReadBuffer, sizeof( ucPartitionReadBuffer ) );
        if( xResult )
        {
            AZLogError( ( "Flash read failed %d\r\n", xResult ) );
        }
        mbedtls_md_update( &ctx, ( const unsigned char * ) ucPartitionReadBuffer, ulReadSize );
    }

    AZLogInfo( ( "mbedtls calculation completed\r\n" ) );

    mbedtls_md_finish( &ctx, ucCalculatedHash );
    mbedtls_md_free( &ctx );

    if( memcmp( ucDecodedManifestHash, ucCalculatedHash, azureiotflashSHA_256_SIZE ) == 0 )
    {
        AZLogInfo( ( "SHAs match\r\n" ) );
        xResult = eAzureIoTSuccess;
    }
    else
    {
        AZLogError( ( "SHAs do not match\r\n" ) );
        AZLogInfo( ( "Wanted: " ) );

        for( int i = 0; i < azureiotflashSHA_256_SIZE; ++i )
        {
            AZLogInfo( ( "%x", ucDecodedManifestHash[ i ] ) );
        }

        AZLogInfo( ( "\r\n" ) );
        AZLogInfo( ( "Calculated: " ) );

        for( int i = 0; i < azureiotflashSHA_256_SIZE; ++i )
        {
            AZLogInfo( ( "%x", ucCalculatedHash[ i ] ) );
        }

        AZLogInfo( ( "\r\n" ) );

        xResult = eAzureIoTErrorFailed;
    }

    return xResult;
}
#endif

AzureIoTResult_t AzureIoTPlatform_EnableImage( AzureADUImage_t * const pxAduImage )
{
    AzureIoTResult_t xResult = eAzureIoTSuccess;
#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
    char pVersion[32];
#endif

    /* Set image status */
    xResult = rom_sys_set_img_flag( pxAduImage->xRunningIdx, ( IMG_FLAG_IA_MASK | IMG_FLAG_NEWER_MASK ), ( IMG_FLAG_IA_OK | IMG_FLAG_OLDER ) );
    xResult |= rom_sys_set_img_flag( !( pxAduImage->xRunningIdx ), ( IMG_FLAG_IA_MASK | IMG_FLAG_VERIFY_MASK | IMG_FLAG_NEWER_MASK ), 0 );
    xResult |= rom_sys_set_img_flag( !( pxAduImage->xRunningIdx ), IMG_FLAG_NEWER_MASK, IMG_FLAG_NEWER );

    if( xResult != eAzureIoTSuccess )
    {
        AZLogError( ( "OTA set image status failed! (xResult = %d)\r\n", xResult ) );
    }
#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
    sprintf(pVersion, "0x%x", ulImageF527Version);
    atcmd_azure_ota_result_send(pVersion, true);
#endif
    AZLogInfo( ( "OTA finish... Please reboot now.\r\n" ) );

    return xResult;
}

AzureIoTResult_t AzureIoTPlatform_ResetDevice( AzureADUImage_t * const pxAduImage )
{
    SysTimer_SoftwareReset();
    return eAzureIoTSuccess;
}
