/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "app_cfg.h"
#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
/* Data Interface Definition */
#include "sample_azure_iot_common_internal.h"
#include "azure_entry.h"
#include "atcmd_azure.h"

/*-----------------------------------------------------------*/

/**
 * @brief Time in ticks to wait between each cycle of the demo implemented
 * by prvMQTTDemoTask().
 */
#define sampleazureiotDELAY_BETWEEN_WIFI_CONNECTING           (pdMS_TO_TICKS(1000U))


/* Store the local settings from ATCMD */
azure_iot_hub_local_info_t azure_iot_hub_local_info = {0};

static void prvConnectHTTP( AzureIoTTransportInterface_t * pxHTTPTransport,
                            const char * pucURL )
{
    SocketTransportStatus_t xStatus;
    TickType_t xRecvTimeout = sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS;
    TickType_t xSendTimeout = sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS;

    LogInfo( ( "Connecting socket to %s", pucURL ) );
    xStatus = Azure_Socket_Connect( pxHTTPTransport->pxNetworkContext, pucURL, 80, xRecvTimeout, xSendTimeout );

    LogInfo( ( " xStatus: %i", xStatus ) );

    configASSERT( xStatus == eSocketTransportSuccess );
}

/**
 * @brief Parses the full ADU file URL into a host (FQDN) and its path.
 *
 * @param xFileUrl ADU file Url to be parsed.
 * @param pucBuffer Buffer to be used for pxHost and pxPath.
 * @param ulBufferSize Size of pucBuffer
 * @param pucHost Where the host part of the url is stored, including a null terminator.
 *               The size of this span is equal to the length of the host address plus the
 *               size of the null-terminator char.
 * @param pulHostLength The length of the content pointed by pucHost.
 * @param pucPath Where the path part of the url is stored.
 * @param pulPathLength The length of the content pointed by pucPath.
 */
static void prvParseAduFileUrl( AzureIoTADUUpdateManifestFileUrl_t xFileUrl,
                                uint8_t * pucBuffer,
                                uint32_t ulBufferSize,
                                uint8_t ** pucHost,
                                uint32_t * pulHostLength,
                                uint8_t ** pucPath,
                                uint32_t * pulPathLength )
{
    configASSERT( ulBufferSize >= xFileUrl.ulUrlLength );

    /* Skipping the http protocol prefix. */
    uint8_t * pucUrl = xFileUrl.pucUrl + sizeof( "http://" ) - 1;
    char * pcPathStart = strstr( ( const char * ) pucUrl, "/" );
    configASSERT( pcPathStart != NULL );

    *pucHost = pucBuffer;
    /* Adding an extra space for the null-terminator. */
    *pulHostLength = pcPathStart - ( char * ) pucUrl + 1;
    *pucPath = pucBuffer + *pulHostLength;

    /* Discouting the size of host and http prefix from ulUrlLength */
    *pulPathLength = xFileUrl.ulUrlLength - ( *pulHostLength - 1 ) - ( sizeof( "http://" ) - 1 );

    ( void ) memcpy( *pucHost, pucUrl, *pulHostLength - 1 );
    ( void ) memset( *pucHost + *pulHostLength - 1, 0, 1 );
    ( void ) memcpy( *pucPath, pcPathStart, *pulPathLength );
}

static AzureIoTResult_t prvDownloadUpdateImageIntoFlash( int32_t ullTimeoutInSec )
{
    AzureIoTResult_t xResult;
    AzureIoTHTTPResult_t xHttpResult;
    AzureIoTHTTP_t xHTTP;
    char * pucOutDataPtr;
    uint32_t ulOutHttpDataBufferLength;
    uint8_t * pucFileUrlHost;
    uint32_t ulFileUrlHostLength;
    uint8_t * pucFileUrlPath;
    uint32_t ulFileUrlPathLength;
    uint64_t ullPreviousTimeout;
    uint64_t ullCurrentTime;

    /*HTTP Connection */
    AzureIoTTransportInterface_t xHTTPTransport;
    NetworkContext_t xHTTPNetworkContext = { 0 };
    SocketTransportParams_t xHTTPSocketTransportParams = { 0 };

    /* Fill in Transport Interface send and receive function pointers. */
    xHTTPTransport.pxNetworkContext = &xHTTPNetworkContext;
    xHTTPTransport.xSend = Azure_Socket_Send;
    xHTTPTransport.xRecv = Azure_Socket_Recv;

    xHTTPNetworkContext.pParams = &xHTTPSocketTransportParams;

    xResult = AzureIoTPlatform_Init( &xImage );

    if( xResult != eAzureIoTSuccess )
    {
        LogError( ( "[ADU] Error initializing platform." ) );
        return xResult;
    }

    LogInfo( ( "[ADU] Step: eAzureIoTADUUpdateStepFirmwareDownloadStarted" ) );

    LogInfo( ( "[ADU] Send property update." ) );

    xResult = AzureIoTADUClient_SendAgentState( &xAzureIoTADUClient,
                                                &xAzureIoTHubClient,
                                                &xADUDeviceProperties,
                                                &xAzureIoTAduUpdateRequest,
                                                eAzureIoTADUAgentStateDeploymentInProgress,
                                                NULL,
                                                ucScratchBuffer,
                                                sizeof( ucScratchBuffer ),
                                                NULL );

    LogInfo( ( "[ADU] Invoke HTTP Connect Callback." ) );

    prvParseAduFileUrl(
        xAzureIoTAduUpdateRequest.pxFileUrls[ 0 ],
        ucScratchBuffer, sizeof( ucScratchBuffer ),
        &pucFileUrlHost, &ulFileUrlHostLength,
        &pucFileUrlPath, &ulFileUrlPathLength );

    prvConnectHTTP( &xHTTPTransport, ( const char * ) pucFileUrlHost );

    /* Range Check */
    xHttpResult = AzureIoTHTTP_RequestSizeInit( &xHTTP, &xHTTPTransport,
                                                ( const char * ) pucFileUrlHost,
                                                ulFileUrlHostLength - 1, /* minus the null-terminator. */
                                                ( const char * ) pucFileUrlPath,
                                                ulFileUrlPathLength,
                                                ( char * ) ucAduDownloadHeaderBuffer,
                                                sizeof( ucAduDownloadHeaderBuffer ) );

    if( xHttpResult != eAzureIoTHTTPSuccess )
    {
        return eAzureIoTErrorFailed;
    }

    if( ( xImage.ulImageFileSize = AzureIoTHTTP_RequestSize( &xHTTP, ( char * ) ucAduDownloadBuffer,
                                                             sizeof( ucAduDownloadBuffer ) ) ) != -1 )
    {
        LogInfo( ( "[ADU] HTTP Range Request was successful: size %u bytes", ( uint16_t ) xImage.ulImageFileSize ) );
    }
    else
    {
        LogError( ( "[ADU] Error getting the headers. " ) );
        return eAzureIoTErrorFailed;
    }

    LogInfo( ( "[ADU] Send HTTP request, ulImageFileSize=0x%x", xImage.ulImageFileSize ) );

    ullPreviousTimeout = ullGetUnixTime();

    while( xImage.ulCurrentOffset < xImage.ulImageFileSize )
    {
        ullCurrentTime = ullGetUnixTime();

        if( ullCurrentTime - ullPreviousTimeout > ullTimeoutInSec )
        {
            LogInfo( ( "%u second timeout. Taking a break from downloading image.", ( uint16_t ) ullTimeoutInSec ) );
            LogInfo( ( "Receiving messages from IoT Hub." ) );
            xResult = AzureIoTHubClient_ProcessLoop( &xAzureIoTHubClient,
                                                     sampleazureiotPROCESS_LOOP_TIMEOUT_MS );

            ullPreviousTimeout = ullGetUnixTime();

            if( xAzureIoTAduUpdateRequest.xWorkflow.xAction == eAzureIoTADUActionCancel )
            {
                LogInfo( ( "Deployment was cancelled" ) );
                break;
            }
        }

        AzureIoTHTTP_Init( &xHTTP, &xHTTPTransport,
                           ( const char * ) pucFileUrlHost,
                           ulFileUrlHostLength - 1, /* minus the null-terminator. */
                           ( const char * ) pucFileUrlPath,
                           ulFileUrlPathLength,
                           ( char * ) ucAduDownloadHeaderBuffer,
                           sizeof( ucAduDownloadHeaderBuffer ) );

        if( ( xHttpResult = AzureIoTHTTP_Request( &xHTTP, xImage.ulCurrentOffset,
                                                  xImage.ulCurrentOffset + democonfigCHUNK_DOWNLOAD_SIZE - 1,
                                                  ( char * ) ucAduDownloadBuffer,
                                                  sizeof( ucAduDownloadBuffer ),
                                                  &pucOutDataPtr,
                                                  &ulOutHttpDataBufferLength ) ) == eAzureIoTHTTPSuccess )
        {
#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
            xResult = AzureIoTPlatform_ProcessBlock( &xImage,
                                                   ( uint32_t ) xImage.ulCurrentOffset,
                                                   ( uint8_t * ) pucOutDataPtr,
                                                   ulOutHttpDataBufferLength );
            if( xResult != eAzureIoTSuccess )
            {
                LogError( ( "[ADU] Error processing block." ) );
                return eAzureIoTErrorFailed;
            }
#else

            /* Write bytes to the flash */
            xResult = AzureIoTPlatform_WriteBlock( &xImage,
                                                   ( uint32_t ) xImage.ulCurrentOffset,
                                                   ( uint8_t * ) pucOutDataPtr,
                                                   ulOutHttpDataBufferLength );

            if( xResult != eAzureIoTSuccess )
            {
                LogError( ( "[ADU] Error writing to flash." ) );
                return eAzureIoTErrorFailed;
            }
#endif
            /* Advance the offset */
            xImage.ulCurrentOffset += ( int32_t ) ulOutHttpDataBufferLength;
            LogInfo( ( "[ADU] ulCurrentOffset=0x%x", xImage.ulCurrentOffset ) );

        }
        else if( xHttpResult == eAzureIoTHTTPNoResponse )
        {
            LogInfo( ( "[ADU] Reconnecting..." ) );
            LogInfo( ( "[ADU] Invoke HTTP Connect Callback." ) );
            prvConnectHTTP( &xHTTPTransport, ( const char * ) pucFileUrlHost );

            if( xResult != eAzureIoTSuccess )
            {
                LogError( ( "[ADU] Failed to reconnect to HTTP server!" ) );
                return eAzureIoTErrorFailed;
            }
        }
        else
        {
            break;
        }
    }

    AzureIoTHTTP_Deinit( &xHTTP );

    return eAzureIoTSuccess;
}

static AzureIoTResult_t prvEnableImageAndResetDevice()
{
    AzureIoTResult_t xResult;
    AzureIoTADUClientInstallResult_t xUpdateResults;

    /* Call into platform specific image verification */
    LogInfo( ( "[ADU] Image validated against hash from ADU" ) );

    if( AzureIoTPlatform_VerifyImage(
            &xImage,
            xAzureIoTAduUpdateRequest.xUpdateManifest.pxFiles[ 0 ].pxHashes[ 0 ].pucHash,
            xAzureIoTAduUpdateRequest.xUpdateManifest.pxFiles[ 0 ].pxHashes[ 0 ].ulHashLength
            ) != eAzureIoTSuccess )
    {
        LogError( ( "[ADU] File hash from ADU did not match calculated hash" ) );
        return eAzureIoTErrorFailed;
    }

    LogInfo( ( "[ADU] Enable the update image" ) );

    if( AzureIoTPlatform_EnableImage( &xImage ) != eAzureIoTSuccess )
    {
        LogError( ( "[ADU] Image could not be enabled" ) );
        return eAzureIoTErrorFailed;
    }

    /*
     * In a production implementation the application would fill the final lResultCode
     * (and optionally lExtendedResultCode) at the end of the update, and the results
     * of each step as they are processed by the application.
     * This result is then reported to the Azure Device Update service, allowing it
     * to assess if the update succeeded.
     * Optional details of the steps and overall installation results can be provided
     * through pucResultDetails.
     */
    xUpdateResults.lResultCode = 0;
    xUpdateResults.lExtendedResultCode = sampleaduSAMPLE_EXTENDED_RESULT_CODE;
    xUpdateResults.pucResultDetails = sampleaduDEFAULT_RESULT_DETAILS;
    xUpdateResults.ulResultDetailsLength = sizeof( sampleaduDEFAULT_RESULT_DETAILS ) - 1;
    xUpdateResults.ulStepResultsCount =
        xAzureIoTAduUpdateRequest.xUpdateManifest.xInstructions.ulStepsCount;

    /*
     * The order of the step results must match order of the steps
     * in the the update manifest instructions.
     */
    for( int32_t ulStepIndex = 0; ulStepIndex < xUpdateResults.ulStepResultsCount; ulStepIndex++ )
    {
        xUpdateResults.pxStepResults[ ulStepIndex ].ulResultCode = 0;
        xUpdateResults.pxStepResults[ ulStepIndex ].ulExtendedResultCode = sampleaduSAMPLE_EXTENDED_RESULT_CODE;
        xUpdateResults.pxStepResults[ ulStepIndex ].pucResultDetails = sampleaduDEFAULT_RESULT_DETAILS;
        xUpdateResults.pxStepResults[ ulStepIndex ].ulResultDetailsLength = sizeof( sampleaduDEFAULT_RESULT_DETAILS ) - 1;
    }

    LogInfo( ( "[ADU] Send property update." ) );

    xResult = AzureIoTADUClient_SendAgentState( &xAzureIoTADUClient,
                                                &xAzureIoTHubClient,
                                                &xADUDeviceProperties,
                                                &xAzureIoTAduUpdateRequest,
                                                eAzureIoTADUAgentStateDeploymentInProgress,
                                                &xUpdateResults,
                                                ucScratchBuffer,
                                                sizeof( ucScratchBuffer ),
                                                NULL );

    if( xResult != eAzureIoTSuccess )
    {
        LogError( ( "[ADU] Failed sending agent state." ) );
        return xResult;
    }

    LogInfo( ( "[ADU] Reset the device" ) );

    if( AzureIoTPlatform_ResetDevice( &xImage ) != eAzureIoTSuccess )
    {
        LogError( ( "[ADU] Failed resetting the device." ) );
        return eAzureIoTErrorFailed;
    }

    /* If a device resets, it will not get here. */
    /* For linux devices, this will mark the device as updated and we will change the version as if */
    /* it did update. */
    LogInfo( ( "[ADU] DEVICE HAS UPDATED" ) );
    xDidDeviceUpdate = true;

    return eAzureIoTSuccess;
}

/* This code is only run on the simulator. Devices will not reach this code since they reboot. */
static AzureIoTResult_t prvSpoofNewVersion( void )
{
    #ifdef democonfigADU_UPDATE_NEW_VERSION
        xADUDeviceProperties.ucCurrentUpdateId = ( const uint8_t * ) democonfigADU_SPOOFED_UPDATE_ID;
        xADUDeviceProperties.ulCurrentUpdateIdLength = strlen( democonfigADU_SPOOFED_UPDATE_ID );
    #else
        LogError( ( "[ADU] New ADU update version for simulator not given." ) );
    #endif
    LogInfo( ( "[ADU] Device Version %.*s",
               ( int16_t ) xADUDeviceProperties.ulCurrentUpdateIdLength, xADUDeviceProperties.ucCurrentUpdateId ) );
    return AzureIoTADUClient_SendAgentState( &xAzureIoTADUClient,
                                             &xAzureIoTHubClient,
                                             &xADUDeviceProperties,
                                             NULL,
                                             eAzureIoTADUAgentStateIdle,
                                             NULL,
                                             ucScratchBuffer,
                                             sizeof( ucScratchBuffer ),
                                             NULL );
}
/*-----------------------------------------------------------*/

/**
 * @brief Cloud message callback handler
 */
void prvHandleCloudMessage(AzureIoTHubClientCloudToDeviceMessageRequest_t *pxMessage,
                                   void *pvContext)
{
    (void)pvContext;

    LogDebug(("Cloud message payload : %.*s \r\n",
               (int)pxMessage->ulPayloadLength,
               (const char *)pxMessage->pvMessagePayload));

    /* Send to L527 by uart <topic_len, topic, payload_len, payload> */
    atcmd_azure_c2dmsg_send((char *)pxMessage->pcTopicName,
                                pxMessage->usTopicNameLength,
                                (char *)pxMessage->pvMessagePayload,
                                pxMessage->ulPayloadLength);
}
/*-----------------------------------------------------------*/

/**
 * @brief Internal function for handling Command requests.
 *
 * @remark This function is required for the interface with samples to work properly.
 */
void prvHandleCommand(AzureIoTHubClientCommandRequest_t *pxMessage,
                              void *pvContext)
{
    AzureIoTHubClient_t *pxHandle = (AzureIoTHubClient_t *)pvContext;
    uint32_t ulResponseStatus = 0;
    AzureIoTResult_t xResult;

    LogDebug(("Command payload : %.*s \r\n",
               (int16_t) pxMessage->ulPayloadLength,
               (const char *) pxMessage->pvMessagePayload));

    /* Send to L527 by uart <topic_len, topic, payload_len, payload> */
#if 1
    atcmd_azure_cmd_req((char *)pxMessage->pcTopicName,
                            pxMessage->usTopicNameLength,
                            (char *)pxMessage->pvMessagePayload,
                            pxMessage->ulPayloadLength);
    return;
#else
    uint32_t ulCommandResponsePayloadLength = ulHandleCommand( pxMessage,
                                                               &ulResponseStatus,
                                                               ucCommandResponsePayloadBuffer,
                                                               sizeof( ucCommandResponsePayloadBuffer ) );

    if( ( xResult = AzureIoTHubClient_SendCommandResponse( pxHandle, pxMessage, ulResponseStatus,
                                                           ucCommandResponsePayloadBuffer,
                                                           ulCommandResponsePayloadLength ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Error sending command response: result 0x%08x", ( uint16_t ) xResult ) );
    }
    else
    {
        LogInfo( ( "Successfully sent command response %u", ( uint16_t ) ulResponseStatus ) );
    }
#endif
}
/*-----------------------------------------------------------*/


/**
 * @brief Private property message callback handler.
 */
void prvHandleProperties(AzureIoTHubClientPropertiesResponse_t *pxMessage,
                                 void *pvContext)
{
    (void)pvContext;

    LogDebug(("Property document payload : %.*s ",
                (int16_t) pxMessage->ulPayloadLength,
                (const char*) pxMessage->pvMessagePayload));

    /* Forward to L527 by ATCMD */
    switch( pxMessage->xMessageType )
    {
        case eAzureIoTHubPropertiesRequestedMessage:
            LogInfo( ( "Device property document GET received" ) );
            #if 1
            atcmd_azure_prop_req((char *)pxMessage->pcTopicName,
                        pxMessage->usTopicNameLength,
                        (char *)pxMessage->pvMessagePayload,
                        pxMessage->ulPayloadLength);
            prvDispatchPropertiesUpdate( pxMessage );
            /* Send to L527 by uart <topic_len, topic, payload_len, payload> */
            #else
            prvDispatchPropertiesUpdate( pxMessage );
            #endif

            break;

        case eAzureIoTHubPropertiesWritablePropertyMessage:
            LogInfo( ( "Device writeable property received" ) );
            #if 1
            /* Send to L527 by uart <topic_len, topic, payload_len, payload> */
            atcmd_azure_prop_req((char *)pxMessage->pcTopicName,
                        pxMessage->usTopicNameLength,
                        (char *)pxMessage->pvMessagePayload,
                        pxMessage->ulPayloadLength);
            prvDispatchPropertiesUpdate( pxMessage );
            #else
            prvDispatchPropertiesUpdate( pxMessage );
            #endif
            break;
        case eAzureIoTHubPropertiesReportedResponseMessage:
            LogInfo( ( "Device reported property response received" ) );
            break;
        default:
            LogError( ( "Unknown property message: 0x%08x", pxMessage->xMessageType ) );
            return;
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Azure IoT demo task that gets started in the platform specific project.
 *  In this demo task, middleware API's are used to connect to Azure IoT Hub and
 *  function to adhere to the Plug and Play device convention.
 */
static void prvAzureDemoTask(void * pvParameters)
{
    AzureIoTResult_t xResult;
    uint32_t ulScratchBufferLength = 0U;
    AzureIoTMessageProperties_t xPropertyBag = { 0 };
    LogInfo(("------------------------------------------------------------------------------"));
    LogInfo(("GigaDevice Azure IoT PnP SAMPLE"));
    LogInfo(("------------------------------------------------------------------------------"));

    azure_iot_hub_local_init();

#if 0
    /* Create a bag of properties for the telemetry */
    xResult = AzureIoTMessage_PropertiesInit( &xPropertyBag, ucPropertyBuffer, 0, sizeof( ucPropertyBuffer ) );
    configASSERT( xResult == eAzureIoTSuccess );

    /* Append a component Property. */
     xResult = AzureIoTMessage_PropertiesAppend( &xPropertyBag,
                                               ( uint8_t * ) AZ_IOT_MESSAGE_COMPONENT_NAME, sizeof( AZ_IOT_MESSAGE_COMPONENT_NAME ) - 1,
                                               ( uint8_t * ) AZ_IOT_CHARGE_SENSOR_PROPERTIES_COMPONENT_NAME, sizeof( AZ_IOT_CHARGE_SENSOR_PROPERTIES_COMPONENT_NAME ) - 1 );
    configASSERT( xResult == eAzureIoTSuccess );
#endif
    for ( ; azure_iot_hub_conn_state_get() != AZURE_IOT_HUB_STATE_Terminate; ) {
        if (xAzureSample_IsConnectedToInternet()) {
            xResult = prvInitializeSNTP();
            if (xResult)
                continue;
            //azure_iot_hub_conn_state_set(AZURE_IOT_HUB_STATE_WiFiConnected);

            for ( ;(azure_iot_hub_conn_state_get() != AZURE_IOT_HUB_STATE_Terminate) && xAzureSample_IsConnectedToInternet(); ) {
                xResult = azure_iot_hub_local_message_wait(sampleazureiotPROCESS_LOOP_TIMEOUT_MS);
                if (xResult != 0) {
                    LogError(("Azure IoT Send Error:%d, Exit.", xResult));
                    goto iteration;
                }

                if (azure_iot_hub_conn_state_get() == AZURE_IOT_HUB_STATE_SubscribeOK) {
                    LogDebug(("Attempt to receive publish message from IoT Hub."));
                    xResult = AzureIoTHubClient_ProcessLoop(&xAzureIoTHubClient,
                                                         sampleazureiotPROCESS_LOOP_TIMEOUT_MS);
                    if (xResult != eAzureIoTSuccess) {
                        LogError(("Azure IoT ProcessLoop Error:%d, Exit.", xResult));
                        goto iteration;

                    }

#if 0
                    vTaskDelay(sampleazureiotDELAY_BETWEEN_DEMO_ITERATIONS_TICKS);
                /* Hook for sending Telemetry */
                if( ( ulCreateTelemetry( ucScratchBuffer, sizeof( ucScratchBuffer ), &ulScratchBufferLength ) == 0 ) &&
                    ( ulScratchBufferLength > 0 ) )
                {
                    xResult = AzureIoTHubClient_SendTelemetry( &xAzureIoTHubClient,
                                                               ucScratchBuffer, ulScratchBufferLength,
                                                               &xPropertyBag, eAzureIoTHubMessageQoS1, NULL );
                    configASSERT( xResult == eAzureIoTSuccess );
                }

                /* Hook for sending update to reported properties */
                ulReportedPropertiesUpdateLength = ulCreateReportedPropertiesUpdate( ucReportedPropertiesUpdate, sizeof( ucReportedPropertiesUpdate ) );

                if( ulReportedPropertiesUpdateLength > 0 )
                {
                    xResult = AzureIoTHubClient_SendPropertiesReported( &xAzureIoTHubClient, ucReportedPropertiesUpdate, ulReportedPropertiesUpdateLength, NULL );
                    configASSERT( xResult == eAzureIoTSuccess );
                }
#endif
                    if( xProcessUpdateRequest && !xDidDeviceUpdate )
                    {
                        if( xAzureIoTAduUpdateRequest.xWorkflow.xAction == eAzureIoTADUActionCancel )
                        {
                            xResult = AzureIoTADUClient_SendAgentState( &xAzureIoTADUClient,
                                                                        &xAzureIoTHubClient,
                                                                        &xADUDeviceProperties,
                                                                        &xAzureIoTAduUpdateRequest,
                                                                        eAzureIoTADUAgentStateIdle,
                                                                        NULL,
                                                                        ucScratchBuffer,
                                                                        sizeof( ucScratchBuffer ),
                                                                        NULL );

                            xProcessUpdateRequest = false;
                        }
                        else if( xAzureIoTAduUpdateRequest.xWorkflow.xAction == eAzureIoTADUActionApplyDownload )
                        {
                            xResult = prvDownloadUpdateImageIntoFlash( sampleazureiotADU_DOWNLOAD_TIMEOUT_SEC );
                            if (xResult) {
                                xProcessUpdateRequest = false;
                                //xAzureIoTAduUpdateRequest.xWorkflow.xAction == eAzureIoTADUAction
                            }

                            LogInfo( ( "Checking for ADU twin updates one more time before committing to update." ) );
                            xResult = AzureIoTHubClient_ProcessLoop( &xAzureIoTHubClient,
                                                                     sampleazureiotPROCESS_LOOP_TIMEOUT_MS );

                            /* In prvDownloadUpdateImageIntoFlash, we make a call to the _ProcessLoop() function */
                            /* which could bring in a new or cancelled update. */
                            /* Check xProcessUpdateRequest and the action again in case a new version came in that was invalid. */
                            if( xProcessUpdateRequest && ( xAzureIoTAduUpdateRequest.xWorkflow.xAction == eAzureIoTADUActionApplyDownload ) )
                            {
                                xResult = prvEnableImageAndResetDevice();
                                if (xResult) {
                                    xProcessUpdateRequest = false;
                                }

                                xResult = prvSpoofNewVersion();
                                if (xResult) {
                                    xProcessUpdateRequest = false;
                                }
                            }
                            else
                            {
                                xResult = AzureIoTADUClient_SendAgentState( &xAzureIoTADUClient,
                                                                            &xAzureIoTHubClient,
                                                                            &xADUDeviceProperties,
                                                                            &xAzureIoTAduUpdateRequest,
                                                                            eAzureIoTADUAgentStateIdle,
                                                                            NULL,
                                                                            ucScratchBuffer,
                                                                            sizeof( ucScratchBuffer ),
                                                                            NULL );

                                xProcessUpdateRequest = false;
                            }
                        }
                        else
                        {
                            LogInfo( ( "Unknown action received: %i", xAzureIoTAduUpdateRequest.xWorkflow.xAction ) );
                        }
                    }
                }
            }
        } else {
            vTaskDelay(sampleazureiotDELAY_BETWEEN_DEMO_ITERATIONS_TICKS);
        }

iteration:
        /* Wait for some time between two iterations to ensure that we do not
         * bombard the IoT Hub. */
        prvStopSNTP();
        azure_iot_hub_disconnect(&azure_iot_hub_local_info);
        LogInfo(("WiFi Disconnected, Short delay before starting the next iteration.... "));
        vTaskDelay(sampleazureiotDELAY_BETWEEN_DEMO_ITERATIONS_TICKS);
    }
    LogInfo(("GigaDevice Azure IoT PnP SAMPLE Exit."));
    azure_iot_hub_local_message_flush();
    azure_iot_hub_local_deinit();
    sys_task_delete(NULL);
}
/*-----------------------------------------------------------*/

/*
 * @brief Create the task that demonstrates the AzureIoTHub demo
 */
void vStartDemoTask( void )
{
    azure_led_init();
    azure_task_tcb = (os_task_t)sys_task_create(NULL,
                (const uint8_t *)"AzureDemoTask", NULL,
                 democonfigDEMO_STACKSIZE,
                 democonfigDEMO_QUEUESIZE, democonfigDEMO_ITEMSIZE,
                 OS_TASK_PRIORITY( democonfigDEMO_TASKPRIO ),
                 (task_func_t)prvAzureDemoTask, NULL);
    if (azure_task_tcb == NULL) {
        LogError(("Create Azure Demo task failed."));
    }
}
/*-----------------------------------------------------------*/

void azure_task_start(void)
{
    return vStartDemoTask();
}
/*-----------------------------------------------------------*/
#endif /* CONFIG_AZURE_F527_DEMO_SUPPORT */
