/*
 * corePKCS11 v3.5.0
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file core_pkcs11_pal.c
 * @brief Linux file save and read implementation
 * for PKCS #11 based on mbedTLS with for software keys. This
 * file deviates from the FreeRTOS style standard for some function names and
 * data types in order to maintain compliance with the PKCS #11 standard.
 */
/*-----------------------------------------------------------*/

/* PKCS 11 includes. */
#include "core_pkcs11_config.h"
#include "core_pkcs11_config_defaults.h"
#include "core_pkcs11.h"
#include "core_pkcs11_pal_utils.h"

/* C runtime includes. */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "nvds_flash.h"

#define NVDS_NS_PKCS11_DATA            "pkcs11_data"

/*-----------------------------------------------------------*/
uint8_t temp_buf[2048];    //may be able to reduce

CK_RV PKCS11_PAL_Initialize( void )
{
    return CKR_OK;
}

bool PKCS11_PAL_flash_load(const char *key, uint8_t *p_data, uint32_t *p_length)
{
    uint32_t str_len = 0;

    if (!nvds_data_get(NULL, NVDS_NS_PKCS11_DATA, key, NULL, &str_len)) {
        if(!nvds_data_get(NULL, NVDS_NS_PKCS11_DATA, key, p_data, &str_len)) {
            *p_length = str_len;
            return true;
        }
    }

    return false;
}

bool PKCS11_PAL_flash_find(const char *key)
{
    if (!nvds_data_find(NULL, NVDS_NS_PKCS11_DATA, key))
        return true;

    return false;
}

bool PKCS11_PAL_flash_save(const char *key, uint8_t *p_data, uint32_t length)
{
    uint32_t str_len = 0;

    if (!nvds_data_put(NULL, NVDS_NS_PKCS11_DATA, key, p_data, length)) {
        return true;
    }

    return false;
}

bool PKCS11_PAL_flash_delete(const char *key)
{
    if (!nvds_data_del(NULL, NVDS_NS_PKCS11_DATA, key)) {
        return true;
    }

    return false;
}

CK_OBJECT_HANDLE PKCS11_PAL_SaveObject( CK_ATTRIBUTE_PTR pxLabel,
                                        CK_BYTE_PTR pucData,
                                        CK_ULONG ulDataSize )
{
    const char * pcFileName = NULL;
    CK_OBJECT_HANDLE xHandle = ( CK_OBJECT_HANDLE ) eInvalidHandle;
    char key[16] = {0};

    if( ( pxLabel != NULL ) && ( pucData != NULL ) )
    {
        /* Converts a label to its respective filename and handle. */
        PAL_UTILS_LabelToFilenameHandle( pxLabel->pValue,
                                         &pcFileName,
                                         &xHandle );
    }
    else
    {
        LogError( ( "Could not save object. Received invalid parameters." ) );
    }

    if( pcFileName != NULL && xHandle!= 0)
    {
        key[0] = xHandle;
        if (PKCS11_PAL_flash_save(key, pucData, ulDataSize)) {
            LogDebug( ( "Successfully wrote to %s", pcFileName ) );
        } else {
            LogError( ( "PKCS #11 PAL was unable to save object to file. ") );
            xHandle = ( CK_OBJECT_HANDLE ) eInvalidHandle;
        }
    }
    else
    {
        LogError( ( "Could not save object. Unable to find the correct file." ) );
    }

    return xHandle;
}

CK_RV PKCS11_PAL_DestroyObject( CK_OBJECT_HANDLE xHandle )
{
    const char * pcFileName = NULL;
    CK_BBOOL xIsPrivate = CK_TRUE;
    CK_RV xResult = CKR_OBJECT_HANDLE_INVALID;
    int ret = 0;
    char key[16] = {0};

    xResult = PAL_UTILS_HandleToFilename( xHandle,
                                          &pcFileName,
                                          &xIsPrivate );

    if(xResult == CKR_OK && xHandle!= 0)
    {
        key[0] = xHandle;
        PKCS11_PAL_flash_delete(key);

        if( ret != 0 )
        {
            xResult = CKR_FUNCTION_FAILED;
        }
    }

    return xResult;
}

CK_OBJECT_HANDLE PKCS11_PAL_FindObject( CK_BYTE_PTR pxLabel,
                                        CK_ULONG usLength )
{
    const char * pcFileName = NULL;
    CK_OBJECT_HANDLE xHandle = ( CK_OBJECT_HANDLE ) eInvalidHandle;
    char key[16] = {0};

    ( void ) usLength;

    if( pxLabel != NULL )
    {
        PAL_UTILS_LabelToFilenameHandle( ( const char * ) pxLabel,
                                         &pcFileName,
                                         &xHandle );
        if (xHandle != 0) {
            key[0] = xHandle;
            if (!PKCS11_PAL_flash_find(key)) {
                xHandle = ( CK_OBJECT_HANDLE ) eInvalidHandle;
            }
        }
    }
    else
    {
        LogError( ( "Could not find object. Received a NULL label." ) );
    }

    return xHandle;
}

CK_RV PKCS11_PAL_GetObjectValue( CK_OBJECT_HANDLE xHandle,
                                 CK_BYTE_PTR * ppucData,
                                 CK_ULONG_PTR pulDataSize,
                                 CK_BBOOL * pIsPrivate )
{
    CK_RV xReturn = CKR_OK;
    const char * pcFileName = NULL;
    uint32_t str_len = 0;
    char key[16] = {0};
    uint32_t data_length = 0;

    if( ( ppucData == NULL ) || ( pulDataSize == NULL ) || ( pIsPrivate == NULL ) )
    {
        xReturn = CKR_ARGUMENTS_BAD;
        LogError( ( "Could not get object value. Received a NULL argument." ) );
    }
    else
    {
        xReturn = PAL_UTILS_HandleToFilename( xHandle, &pcFileName, pIsPrivate );
    }

    if( xReturn == CKR_OK && xHandle != 0)
    {
        key[0] = xHandle;
        if (!PKCS11_PAL_flash_load(key, temp_buf, pulDataSize)){
            xReturn = CKR_GENERAL_ERROR;
        } else {
            *ppucData = temp_buf;
        }
    }

    return xReturn;
}

void PKCS11_PAL_GetObjectValueCleanup( CK_BYTE_PTR pucData,
                                       CK_ULONG ulDataSize )
{
    /* Unused parameters. */
    ( void ) ulDataSize;

    if( NULL != pucData )
    {
        free( pucData );
    }
}

/*-----------------------------------------------------------*/
