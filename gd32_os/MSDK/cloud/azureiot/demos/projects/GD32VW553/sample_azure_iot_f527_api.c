/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/* Standard includes. */

#include "app_cfg.h"

#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
#include <string.h>
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* GigaDevice os wrapper header */
#include "wrapper_os_config.h"

/* Azure Provisioning/IoT Hub library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_hub_client_properties.h"
#include "azure_iot_provisioning_client.h"
#include "azure_iot_adu_client.h"
#include "azure_iot_flash_platform.h"
#include "azure_iot_http.h"

/* Azure JSON includes */
#include "azure_iot_json_reader.h"
#include "azure_iot_json_writer.h"

/* Exponential backoff retry include. */
#include "backoff_algorithm.h"

/* Crypto helper header. */
#include "azure_sample_crypto.h"

/* Demo Specific configs. */
#include "demo_config.h"

/* Demo Specific Interface Functions. */
#include "azure_sample_connection.h"

/* Data Interface Definition */
#include "sample_azure_iot_pnp_data_if_gd.h"
#include "sample_azure_iot_f527_api.h"
#include "sample_azure_iot_common_internal.h"

#include "wrapper_os.h"

#include "azure_iot_mqtt.h"
#include "atcmd_azure.h"


#define azureiothubCOMMAND_EMPTY_RESPONSE              "{}"

os_task_t azure_task_tcb;

NetworkContext_t xNetworkContextGlobal = { 0 };
TlsTransportParams_t xTlsTransportParamsGlobal = { 0 };
AzureIoTTransportInterface_t xTransportGlobal = { 0 };
NetworkCredentials_t xNetworkCredentialsGloabl = { 0 };


extern void prvHandleCloudMessage(AzureIoTHubClientCloudToDeviceMessageRequest_t *pxMessage,
                                   void *pvContext);

extern void prvHandleCommand(AzureIoTHubClientCommandRequest_t *pxMessage,
                              void *pvContext);
extern void prvHandleProperties(AzureIoTHubClientPropertiesResponse_t *pxMessage,
                                 void *pvContext);

void azure_iot_hub_local_message_flush(void);

static void azure_iot_local_info_dump(void)
{
    LogInfo(("ModelID:%s,len:%d", azure_iot_hub_local_info.pucIotModelID, strlen((char *)azure_iot_hub_local_info.pucIotModelID)));
    LogInfo(("ModualID:%s,len=%d", azure_iot_hub_local_info.pucIotModuleID, strlen((char *)azure_iot_hub_local_info.pucIotModuleID)));
    LogInfo(("pucENDPOINT:%s,len=%d", azure_iot_hub_local_info.pucENDPOINT, strlen((char *)azure_iot_hub_local_info.pucENDPOINT)));
    LogInfo(("pucIotID_SCOPE:%s,len=%d", azure_iot_hub_local_info.pucIotID_SCOPE, strlen((char *)azure_iot_hub_local_info.pucIotID_SCOPE)));
    LogInfo(("pucRegistration_ID:%s,len=%d", azure_iot_hub_local_info.pucRegistration_ID, strlen((char *)azure_iot_hub_local_info.pucRegistration_ID)));
    LogInfo(("pucDevice_ID:%s,len=%d", azure_iot_hub_local_info.pucDevice_ID, strlen((char *)azure_iot_hub_local_info.pucDevice_ID)));
    LogInfo(("pucIotHubHostname:%s,len=%d", azure_iot_hub_local_info.pucIotHubHostname, strlen((char *)azure_iot_hub_local_info.pucIotHubHostname)));
    LogInfo(("pucDeviceSymmetricKey:%s,len=%d", azure_iot_hub_local_info.pucDeviceSymmetricKey, strlen((char *)azure_iot_hub_local_info.pucDeviceSymmetricKey)));
    LogInfo(("pucADUManufacturer:%s,len=%d", azure_iot_hub_local_info.pucADUManufacturer, strlen((char *)azure_iot_hub_local_info.pucADUManufacturer)));
    LogInfo(("pucADUDeviceModel:%s,len=%d", azure_iot_hub_local_info.pucADUDeviceModel, strlen((char *)azure_iot_hub_local_info.pucADUDeviceModel)));
    LogInfo(("pucADUUpdateProvider:%s,len=%d", azure_iot_hub_local_info.pucADUUpdateProvider, strlen((char *)azure_iot_hub_local_info.pucADUUpdateProvider)));
    LogInfo(("pucADUUpdateName:%s,len=%d", azure_iot_hub_local_info.pucADUUpdateName, strlen((char *)azure_iot_hub_local_info.pucADUUpdateName)));
    LogInfo(("pucADUUpdateVersion:%s,len=%d", azure_iot_hub_local_info.pucADUUpdateVersion, strlen((char *)azure_iot_hub_local_info.pucADUUpdateVersion)));
    LogInfo(("ulIotPort:%d", azure_iot_hub_local_info.ulIotPort));
    LogInfo(("ulIotPort:%d", azure_iot_hub_local_info.ulIotPort));
    LogInfo(("ucIotHubConnState:%d", azure_iot_hub_local_info.ucIotHubConnState));
    LogInfo(("disable_dps:%d, secure_mode:%d", azure_iot_hub_local_info.conn_cfg.dps_disable,
                azure_iot_hub_local_info.conn_cfg.secure_mode));
}
/*-----------------------------------------------------------*/

AzureIoTResult_t azure_iot_hub_model_update(const uint8_t *model, uint8_t model_len)
{

    if (!model || model_len >= sizeof(azure_iot_hub_local_info.pucIotModelID))
        return eAzureIoTErrorInvalidArgument;

    sys_memset(azure_iot_hub_local_info.pucIotModelID, 0, sizeof(azure_iot_hub_local_info.pucIotModelID));
    sys_memcpy(azure_iot_hub_local_info.pucIotModelID, model, model_len);

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

/* TODO */
static void azure_iot_hub_component_release(void)
{
    for (int i = 0; i < sampleaduPNP_COMPONENTS_LIST_LENGTH; i++) {
        if (azure_iot_hub_local_info.pnp_comp[i].ptr) {
            sys_mfree(azure_iot_hub_local_info.pnp_comp[i].ptr);
            azure_iot_hub_local_info.pnp_comp[i].ptr = NULL;
            azure_iot_hub_local_info.pnp_comp[i].size = 0;
        }
    }
}

AzureIoTResult_t azure_iot_hub_component_update(const uint8_t **component_str, uint32_t comp_num)
{
    LogInfo(("component updated."));

    uint32_t i = 0;
    uint8_t component_num;
    char **tmp = (char **)component_str;

    if (component_str == NULL || comp_num == 0 || comp_num > sampleaduPNP_COMPONENTS_LIST_LENGTH)
        return eAzureIoTErrorInvalidArgument;

    // release memory for previous configurations
    azure_iot_hub_component_release();

    for (i = 0; i < comp_num; i++) {
        azure_iot_hub_local_info.pnp_comp[i].ptr = (uint8_t *)sys_malloc(strlen(tmp[i]));
        if (!azure_iot_hub_local_info.pnp_comp[i].ptr)
            goto fail;

        sys_memcpy(azure_iot_hub_local_info.pnp_comp[i].ptr, tmp[i], strlen(tmp[i]));
        azure_iot_hub_local_info.pnp_comp[i].size = strlen(tmp[i]);
        pnp_components[i] = azureiothubCREATE_COMPONENT_GD(azure_iot_hub_local_info.pnp_comp[i].ptr,
                                                            azure_iot_hub_local_info.pnp_comp[i].size);
    }

    return eAzureIoTSuccess;
fail:
    azure_iot_hub_component_release();
    return eAzureIoTErrorOutOfMemory;
}
/*-----------------------------------------------------------*/

AzureIoTResult_t azure_iot_hub_endpoint_update(const uint8_t *endpoint, uint8_t endpoint_len)
{

    if (!endpoint || endpoint_len >= sizeof(azure_iot_hub_local_info.pucENDPOINT))
        return eAzureIoTErrorInvalidArgument;

    sys_memset(azure_iot_hub_local_info.pucENDPOINT, 0, sizeof(azure_iot_hub_local_info.pucENDPOINT));
    sys_memcpy(azure_iot_hub_local_info.pucENDPOINT, endpoint, endpoint_len);

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

AzureIoTResult_t azure_iot_hub_idscope_update(const uint8_t *idscope, uint8_t idscope_len)
{
    if (!idscope || idscope_len >= sizeof(azure_iot_hub_local_info.pucIotID_SCOPE))
        return eAzureIoTErrorInvalidArgument;

    sys_memset(azure_iot_hub_local_info.pucIotID_SCOPE, 0, sizeof(azure_iot_hub_local_info.pucIotID_SCOPE));

    sys_memcpy(azure_iot_hub_local_info.pucIotID_SCOPE, idscope, idscope_len);

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

AzureIoTResult_t azure_iot_hub_registrationid_update(const uint8_t *registrationid, uint8_t registrationid_len)
{

    if (!registrationid || registrationid_len >= sizeof(azure_iot_hub_local_info.pucRegistration_ID))
        return eAzureIoTErrorInvalidArgument;

    sys_memset(azure_iot_hub_local_info.pucRegistration_ID, 0, sizeof(azure_iot_hub_local_info.pucRegistration_ID));
    sys_memcpy(azure_iot_hub_local_info.pucRegistration_ID, registrationid, registrationid_len);

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

AzureIoTResult_t azure_iot_hub_deviceid_update(const uint8_t *deviceid, uint8_t deviceid_len)
{

    if (!deviceid || deviceid_len >= sizeof(azure_iot_hub_local_info.pucDevice_ID))
        return eAzureIoTErrorInvalidArgument;

    sys_memset(azure_iot_hub_local_info.pucDevice_ID, 0, sizeof(azure_iot_hub_local_info.pucDevice_ID));
    sys_memcpy(azure_iot_hub_local_info.pucDevice_ID, deviceid, deviceid_len);

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

AzureIoTResult_t azure_iot_hub_hostname_update(const uint8_t *hostname, uint8_t hostname_len)
{
    if (!hostname || hostname_len >= sizeof(azure_iot_hub_local_info.pucIotHubHostname))
        return eAzureIoTErrorInvalidArgument;

    sys_memset(azure_iot_hub_local_info.pucIotHubHostname, 0, sizeof(azure_iot_hub_local_info.pucIotHubHostname));
    sys_memcpy(azure_iot_hub_local_info.pucIotHubHostname, hostname, hostname_len);

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

AzureIoTResult_t azure_iot_hub_symkey_update(const uint8_t *symkey, uint8_t symkey_len)
{
    if ((!symkey) || (symkey_len >= sizeof(azure_iot_hub_local_info.pucDeviceSymmetricKey)))
        return eAzureIoTErrorInvalidArgument;

    sys_memset(azure_iot_hub_local_info.pucDeviceSymmetricKey, 0, sizeof(azure_iot_hub_local_info.pucDeviceSymmetricKey));
    sys_memcpy(azure_iot_hub_local_info.pucDeviceSymmetricKey, symkey, symkey_len);

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

AzureIoTResult_t azure_iot_hub_x509cert_update(const uint8_t *x509cert, uint32_t x509cert_len)
{
    if (azure_iot_hub_local_info.pucx509Cert != NULL) {
        sys_mfree(azure_iot_hub_local_info.pucx509Cert);
        azure_iot_hub_local_info.pucx509Cert = NULL;
    }

    azure_iot_hub_local_info.pucx509Cert = sys_malloc(x509cert_len);
    if (azure_iot_hub_local_info.pucx509Cert == NULL) {
        LogError(("Alloc memory for x509 cert fail."));
        return eAzureIoTErrorOutOfMemory;
    }

    sys_memcpy(azure_iot_hub_local_info.pucDeviceSymmetricKey, x509cert, x509cert_len);
    azure_iot_hub_local_info.ulx509CertLength = x509cert_len;

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

AzureIoTResult_t azure_iot_adu_manufacturer_update(const uint8_t *manuf, uint8_t manuf_len)
{
    if (!manuf || manuf_len >= sizeof(azure_iot_hub_local_info.pucADUManufacturer))
        return eAzureIoTErrorInvalidArgument;

    sys_memset(azure_iot_hub_local_info.pucADUManufacturer, 0, sizeof(azure_iot_hub_local_info.pucADUManufacturer));
    sys_memcpy(azure_iot_hub_local_info.pucADUManufacturer, manuf, manuf_len);

    xADUDeviceProperties.ucManufacturer = azure_iot_hub_local_info.pucADUManufacturer;
    xADUDeviceProperties.ulManufacturerLength = manuf_len;

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

AzureIoTResult_t azure_iot_adu_model_update(const uint8_t *adumodel, uint8_t adumodel_len)
{
    if (!adumodel || adumodel_len >= sizeof(azure_iot_hub_local_info.pucADUDeviceModel))
        return eAzureIoTErrorInvalidArgument;

    sys_memset(azure_iot_hub_local_info.pucADUDeviceModel, 0, sizeof(azure_iot_hub_local_info.pucADUDeviceModel));
    sys_memcpy(azure_iot_hub_local_info.pucADUDeviceModel, adumodel, adumodel_len);

    xADUDeviceProperties.ucModel= azure_iot_hub_local_info.pucADUDeviceModel;
    xADUDeviceProperties.ulModelLength = adumodel_len;

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

AzureIoTResult_t azure_iot_adu_provider_update(const uint8_t *provider, uint8_t provider_len)
{
    if (!provider || provider_len >= sizeof(azure_iot_hub_local_info.pucADUUpdateProvider))
        return eAzureIoTErrorInvalidArgument;

    sys_memset(azure_iot_hub_local_info.pucADUUpdateProvider, 0, sizeof(azure_iot_hub_local_info.pucADUUpdateProvider));
    sys_memcpy(azure_iot_hub_local_info.pucADUUpdateProvider, provider, provider_len);

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

AzureIoTResult_t azure_iot_adu_updatename_update(const uint8_t *name, uint8_t name_len)
{
    if (!name || name_len >= sizeof(azure_iot_hub_local_info.pucADUUpdateName))
        return eAzureIoTErrorInvalidArgument;

    sys_memset(azure_iot_hub_local_info.pucADUUpdateName, 0, sizeof(azure_iot_hub_local_info.pucADUUpdateName));
    sys_memcpy(azure_iot_hub_local_info.pucADUUpdateName, name, name_len);

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

AzureIoTResult_t azure_iot_adu_updatever_update(const uint8_t *version, uint8_t version_len)
{
    if (!version || version_len >= sizeof(azure_iot_hub_local_info.pucADUUpdateVersion))
        return eAzureIoTErrorInvalidArgument;

    sys_memset(azure_iot_hub_local_info.pucADUUpdateVersion, 0, sizeof(azure_iot_hub_local_info.pucADUUpdateVersion));
    sys_memcpy(azure_iot_hub_local_info.pucADUUpdateVersion, version, version_len);

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

AzureIoTResult_t azure_iot_adu_properties_init(void)
{
    AzureIoTResult_t res;
    res = snprintf((char *)azure_iot_hub_local_info.pucADUUpdateID,
                    sizeof(azure_iot_hub_local_info.pucADUUpdateID),
                    "{\"provider\":\"%s\",\"name\":\"%s\",\"version\":\"%s\"}",
                    azure_iot_hub_local_info.pucADUUpdateProvider,
                    azure_iot_hub_local_info.pucADUUpdateName,
                    azure_iot_hub_local_info.pucADUUpdateVersion);

    xADUDeviceProperties.ucCurrentUpdateId = azure_iot_hub_local_info.pucADUUpdateID;
    xADUDeviceProperties.ulCurrentUpdateIdLength = strlen((char *)xADUDeviceProperties.ucCurrentUpdateId);
    return eAzureIoTSuccess;
}

void azure_iot_hub_port_update(const uint32_t port)
{
    azure_iot_hub_local_info.ulIotPort = port;
}
/*-----------------------------------------------------------*/

uint8_t azure_iot_hub_conn_state_get(void)
{
    return azure_iot_hub_local_info.ucIotHubConnState;
}
/*-----------------------------------------------------------*/

bool azure_iot_hub_azure_connected(void)
{
    return (azure_iot_hub_local_info.ucIotHubConnState == AZURE_IOT_HUB_STATE_SubscribeOK);
}
/*-----------------------------------------------------------*/

int azure_iot_hub_local_message_send(uint8_t type, uint8_t *payload, uint32_t len)
{
    azure_at_message_t message = {0};
    int32_t res = 0;

    if (azure_iot_hub_local_info.ucIotHubConnState == AZURE_IOT_HUB_STATE_Terminate)
        return eAzureIoTErrorFailed;

    message.type = type;
    message.payload = payload;
    message.payload_len = len;
    res = sys_task_post(azure_task_tcb, &message, 0);

    return res;
}
/*-----------------------------------------------------------*/

/**
 * @brief Setup transport credentials.
 */
uint32_t prvSetupNetworkCredentials(NetworkCredentials_t * pxNetworkCredentials)
{
    pxNetworkCredentials->xDisableSni = pdFALSE;
    /* Set the credentials for establishing a TLS connection. */
    pxNetworkCredentials->pucRootCa = (const unsigned char * ) democonfigROOT_CA_PEM;
    pxNetworkCredentials->xRootCaSize = sizeof(democonfigROOT_CA_PEM);
    #ifdef democonfigCLIENT_CERTIFICATE_PEM
        pxNetworkCredentials->pucClientCert = ( const unsigned char * ) democonfigCLIENT_CERTIFICATE_PEM;
        pxNetworkCredentials->xClientCertSize = sizeof( democonfigCLIENT_CERTIFICATE_PEM );
        pxNetworkCredentials->pucPrivateKey = ( const unsigned char * ) democonfigCLIENT_PRIVATE_KEY_PEM;
        pxNetworkCredentials->xPrivateKeySize = sizeof( democonfigCLIENT_PRIVATE_KEY_PEM );
    #endif

    return 0;
}
/*-----------------------------------------------------------*/


#ifdef democonfigENABLE_DPS_SAMPLE

AzureIoTResult_t prvCreateProvisioningPayload(uint8_t *pucBuffer,
                                                      uint32_t ulBufferLength,
                                                      int32_t *plOutBufferLength)
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;

    if ((xResult = AzureIoTJSONWriter_Init(&xWriter,
                                             pucBuffer,
                                             ulBufferLength)) != eAzureIoTSuccess) {
        LogError(("Error initializing JSON writer: result 0x%08x", (uint16_t)xResult));
        return xResult;
    } else if((xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter ) ) != eAzureIoTSuccess) {
        LogError(("Error appending begin object: result 0x%08x", (uint16_t)xResult));
        return xResult;
    } else if((xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue(&xWriter,
                                                                           (uint8_t *)sampleazureiotMODEL_ID_STR,
                                                                           sizeof(sampleazureiotMODEL_ID_STR) - 1,
                                                                           AzureIoTADUModelID,
                                                                           AzureIoTADUModelIDLength)) != eAzureIoTSuccess) {
        LogError( ( "Error appending property name and string value: result 0x%08x", (uint16_t)xResult));
        return xResult;
    } else if((xResult = AzureIoTJSONWriter_AppendEndObject(&xWriter)) != eAzureIoTSuccess) {
        LogError(("Error appending end object: result 0x%08x", (uint16_t)xResult));
        return xResult;
    }

    *plOutBufferLength = AzureIoTJSONWriter_GetBytesUsed(&xWriter);

    return xResult;
}

/**
 * @brief Get IoT Hub endpoint and device Id info, when Provisioning service is used.
 *   This function will block for Provisioning service for result or return failure.
 */
uint32_t prvIoTHubInfoGet( NetworkCredentials_t *pXNetworkCredentials,
                                  uint8_t **ppucIothubHostname,
                                  uint32_t *pulIothubHostnameLength,
                                  uint8_t **ppucIothubDeviceId,
                                  uint32_t *pulIothubDeviceIdLength)
{
    NetworkContext_t xNetworkContext = {0};
    TlsTransportParams_t xTlsTransportParams = {0};
    AzureIoTResult_t xResult;
    AzureIoTTransportInterface_t xTransport;
    int32_t lOutProvisioningPayloadLength;
    uint32_t ucSamplepIothubHostnameLength = sizeof(ucSampleIotHubHostname);
    uint32_t ucSamplepIothubDeviceIdLength = sizeof(ucSampleIotHubDeviceId);
    uint32_t ulStatus;

    /* Set the pParams member of the network context with desired transport. */
    xNetworkContext.pParams = &xTlsTransportParams;

    ulStatus = prvConnectToServerWithBackoffRetries((char *)azure_iot_hub_local_info.pucENDPOINT,
                                                    azure_iot_hub_local_info.ulIotPort,
                                                    pXNetworkCredentials,
                                                    &xNetworkContext);
    if (ulStatus)
        return ulStatus;

    /* Fill in Transport Interface send and receive function pointers. */
    xTransport.pxNetworkContext = &xNetworkContext;
    xTransport.xSend = TLS_Socket_Send;
    xTransport.xRecv = TLS_Socket_Recv;

    xResult = AzureIoTProvisioningClient_Init(&xAzureIoTProvisioningClient,
                                               (const uint8_t *) azure_iot_hub_local_info.pucENDPOINT,
                                               strlen((char *)azure_iot_hub_local_info.pucENDPOINT),
                                               (const uint8_t *) azure_iot_hub_local_info.pucIotID_SCOPE,
                                               strlen((char *)azure_iot_hub_local_info.pucIotID_SCOPE),
                                               (const uint8_t *) azure_iot_hub_local_info.pucRegistration_ID,
                                               strlen((char *)azure_iot_hub_local_info.pucRegistration_ID),
                                               NULL, ucMQTTMessageBuffer, sizeof(ucMQTTMessageBuffer),
                                               ullGetUnixTime,
                                               &xTransport);
    if (xResult)
        return xResult;

    #ifdef democonfigDEVICE_SYMMETRIC_KEY
        xResult = AzureIoTProvisioningClient_SetSymmetricKey(&xAzureIoTProvisioningClient,
                                                              (const uint8_t *)azure_iot_hub_local_info.pucDeviceSymmetricKey,
                                                              strlen((char *)azure_iot_hub_local_info.pucDeviceSymmetricKey),
                                                              Crypto_HMAC);
        if (xResult)
            return xResult;
    #endif /* democonfigDEVICE_SYMMETRIC_KEY */

    xResult = prvCreateProvisioningPayload(ucScratchBuffer,
                                            sizeof( ucScratchBuffer),
                                            &lOutProvisioningPayloadLength);
    if (xResult)
        return xResult;

    xResult = AzureIoTProvisioningClient_SetRegistrationPayload(&xAzureIoTProvisioningClient,
                                                                 (const uint8_t *) ucScratchBuffer,
                                                                 (uint32_t)lOutProvisioningPayloadLength);
    if (xResult)
        return xResult;

    do {
        xResult = AzureIoTProvisioningClient_Register(&xAzureIoTProvisioningClient,
                                                       sampleazureiotProvisioning_Registration_TIMEOUT_MS);
    } while(xResult == eAzureIoTErrorPending);

    if (xResult == eAzureIoTSuccess) {
        LogInfo(("Successfully acquired IoT Hub name and Device ID"));
    } else {
        LogInfo( ( "Error getting IoT Hub name and Device ID: 0x%08x", (uint16_t)xResult ));
    }

    if (xResult)
        return xResult;

    xResult = AzureIoTProvisioningClient_GetDeviceAndHub(&xAzureIoTProvisioningClient,
                                                          ucSampleIotHubHostname, &ucSamplepIothubHostnameLength,
                                                          ucSampleIotHubDeviceId, &ucSamplepIothubDeviceIdLength);
    if (xResult)
        return xResult;

    AzureIoTProvisioningClient_Deinit(&xAzureIoTProvisioningClient);

    /* Close the network connection.  */
    TLS_Socket_Disconnect(&xNetworkContext);

    *ppucIothubHostname = ucSampleIotHubHostname;
    *pulIothubHostnameLength = ucSamplepIothubHostnameLength;
    *ppucIothubDeviceId = ucSampleIotHubDeviceId;
    *pulIothubDeviceIdLength = ucSamplepIothubDeviceIdLength;

    return 0;
}
#endif /* democonfigENABLE_DPS_SAMPLE */
/*-----------------------------------------------------------*/

/**
 * @brief Connect to endpoint with reconnection retries.
 *
 * If connection fails, retry is attempted after a timeout.
 * Timeout value will exponentially increase until maximum
 * timeout value is reached or the number of attempts are exhausted.
 *
 * @param pcHostName Hostname of the endpoint to connect to.
 * @param ulPort Endpoint port.
 * @param pxNetworkCredentials Pointer to Network credentials.
 * @param pxNetworkContext Point to Network context created.
 * @return uint32_t The status of the final connection attempt.
 */
uint32_t prvConnectToServerWithBackoffRetries( const char * pcHostName,
                                                      uint32_t port,
                                                      NetworkCredentials_t * pxNetworkCredentials,
                                                      NetworkContext_t * pxNetworkContext )
{
    TlsTransportStatus_t xNetworkStatus;
    BackoffAlgorithmStatus_t xBackoffAlgStatus = BackoffAlgorithmSuccess;
    BackoffAlgorithmContext_t xReconnectParams;
    uint16_t usNextRetryBackOff = 0U;

    /* Initialize reconnect attempts and interval. */
    BackoffAlgorithm_InitializeParams( &xReconnectParams,
                                       sampleazureiotRETRY_BACKOFF_BASE_MS,
                                       sampleazureiotRETRY_MAX_BACKOFF_DELAY_MS,
                                       sampleazureiotRETRY_MAX_ATTEMPTS );

    /* Attempt to connect to IoT Hub. If connection fails, retry after
     * a timeout. Timeout value will exponentially increase till maximum
     * attempts are reached.
     */
    do
    {
        LogInfo( ( "Creating a TLS connection to %s:%u.", pcHostName, ( uint16_t ) port ) );
        /* Attempt to create a mutually authenticated TLS connection. */
        xNetworkStatus = TLS_Socket_Connect( pxNetworkContext,
                                             pcHostName, port,
                                             pxNetworkCredentials,
                                             sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS,
                                             sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS );

        if( xNetworkStatus != eTLSTransportSuccess )
        {
            /* Generate a random number and calculate backoff value (in milliseconds) for
             * the next connection retry.
             * Note: It is recommended to seed the random number generator with a device-specific
             * entropy source so that possibility of multiple devices retrying failed network operations
             * at similar intervals can be avoided. */
            xBackoffAlgStatus = BackoffAlgorithm_GetNextBackoff( &xReconnectParams, configRAND32(), &usNextRetryBackOff );

            if( xBackoffAlgStatus == BackoffAlgorithmRetriesExhausted )
            {
                LogError( ( "Connection to the IoT Hub failed, all attempts exhausted." ) );
            }
            else if( xBackoffAlgStatus == BackoffAlgorithmSuccess )
            {
                LogWarn( ( "Connection to the IoT Hub failed [%d]. "
                           "Retrying connection with backoff and jitter [%d]ms.",
                           xNetworkStatus, usNextRetryBackOff ) );
                vTaskDelay( pdMS_TO_TICKS( usNextRetryBackOff ) );
            }
        }
    } while( ( xNetworkStatus != eTLSTransportSuccess ) && ( xBackoffAlgStatus == BackoffAlgorithmSuccess ) );

    return xNetworkStatus == eTLSTransportSuccess ? 0 : 1;
}
/*-----------------------------------------------------------*/

void prvDispatchPropertiesUpdate( AzureIoTHubClientPropertiesResponse_t * pxMessage )
{
    vHandleWritableProperties( pxMessage,
                               ucReportedPropertiesUpdate,
                               sizeof( ucReportedPropertiesUpdate ),
                               &ulReportedPropertiesUpdateLength );
}
/*-----------------------------------------------------------*/

#if 0
void prvProcessADUPropertiesUpdate( AzureIoTHubClientPropertiesResponse_t * pxMessage )
{
    vHandleADUProperty( pxMessage,
                               ucReportedPropertiesUpdate,
                               sizeof( ucReportedPropertiesUpdate ),
                               &ulReportedPropertiesUpdateLength );
}
/*-----------------------------------------------------------*/
#endif

AzureIoTResult_t azure_iot_hub_connect(azure_iot_hub_local_info_t *piot_hub_local_conn_info)
{
    uint32_t res;
    /* MQTT Connection */
    AzureIoTHubClientOptions_t xHubOptions = {0};
    AzureIoTADUClientOptions_t xADUOptions = { 0 };
    bool xSessionPresent;
    uint8_t *pucIotHubHostname = NULL;
    uint8_t *pucIotHubDeviceId = NULL;
    uint32_t pulIothubHostnameLength = 0;
    uint32_t pulIothubDeviceIdLength = 0;

    if (piot_hub_local_conn_info == NULL)
        return AZURE_IOT_CONN_UNSPECIFIED_ERR;

    //azure_iot_local_info_dump();

#ifndef democonfigENABLE_DPS_SAMPLE
    pucIotHubHostname = (uint8_t *)piot_hub_local_conn_info->pucIotHubHostname;
    pucIotHubDeviceId = (uint8_t *)piot_hub_local_conn_info->pucDevice_ID;
    pulIothubHostnameLength = strlen((char *)piot_hub_local_conn_info->pucIotHubHostname);
    pulIothubDeviceIdLength = strlen((char *)piot_hub_local_conn_info->pucDevice_ID);
#endif /* democonfigENABLE_DPS_SAMPLE */

    AzureIoT_Init();

    prvSetupNetworkCredentials(&xNetworkCredentialsGloabl);

 #ifdef democonfigENABLE_DPS_SAMPLE
    if (piot_hub_local_conn_info->conn_cfg.dps_disable == 0) {
        /* Run DPS.  */
        if ((res = prvIoTHubInfoGet(&xNetworkCredentialsGloabl, &pucIotHubHostname,
                                           &pulIothubHostnameLength, &pucIotHubDeviceId,
                                           &pulIothubDeviceIdLength)) != 0) {
            LogError(("Failed on sample_dps_entry!: error code = 0x%08x", res));
            return res;
        }
    }
#endif /* democonfigENABLE_DPS_SAMPLE */

    sys_memset(&xNetworkContextGlobal, 0, sizeof(xNetworkContextGlobal));
    xNetworkContextGlobal.pParams = &xTlsTransportParamsGlobal;

    /* Update Azure connect state */
    piot_hub_local_conn_info->ucIotHubConnState = AZURE_IOT_HUB_STATE_WiFiConnected;

    if (xAzureSample_IsConnectedToInternet()) {
        /* Attempt to establish TLS session with IoT Hub. If connection fails,
         * retry after a timeout. Timeout value will be exponentially increased
         * until  the maximum number of attempts are reached or the maximum timeout
         * value is reached. The function returns a failure status if the TCP
         * connection cannot be established to the IoT Hub after the configured
         * number of attempts. */
        res = prvConnectToServerWithBackoffRetries((const char *)pucIotHubHostname,
                                                         piot_hub_local_conn_info->ulIotPort,
                                                         &xNetworkCredentialsGloabl,
                                                         &xNetworkContextGlobal);
        if (res)
            return AZURE_IOT_CONN_UNSPECIFIED_ERR;

        piot_hub_local_conn_info->ucIotHubConnState = AZURE_IOT_HUB_STATE_TLSConnected;

        /* Fill in Transport Interface send and receive function pointers. */
        xTransportGlobal.pxNetworkContext = &xNetworkContextGlobal;
        xTransportGlobal.xSend = TLS_Socket_Send;
        xTransportGlobal.xRecv = TLS_Socket_Recv;

        /* Init IoT Hub option */
        AzureIoTHubClient_OptionsInit(&xHubOptions);

        xHubOptions.pucModuleID = (const uint8_t *)piot_hub_local_conn_info->pucIotModuleID; // TODO
        xHubOptions.ulModuleIDLength = strlen((char *)piot_hub_local_conn_info->pucIotModuleID);
        xHubOptions.pucModelID = (const uint8_t *)piot_hub_local_conn_info->pucIotModelID;
        xHubOptions.ulModelIDLength = strlen((char *)piot_hub_local_conn_info->pucIotModelID);

        #ifdef sampleaduPNP_COMPONENTS_LIST_LENGTH
            #if sampleaduPNP_COMPONENTS_LIST_LENGTH > 0
                xHubOptions.pxComponentList = sampleaduPNP_COMPONENTS_LIST; // TODO
                xHubOptions.ulComponentListLength = sampleaduPNP_COMPONENTS_LIST_LENGTH;
            #endif /* > 0 */
        #endif /* sampleaduPNP_COMPONENTS_LIST_LENGTH */

        res = AzureIoTHubClient_Init(&xAzureIoTHubClient,
                                          pucIotHubHostname, pulIothubHostnameLength,
                                          pucIotHubDeviceId, pulIothubDeviceIdLength,
                                          &xHubOptions,
                                          ucMQTTMessageBuffer, sizeof( ucMQTTMessageBuffer),
                                          ullGetUnixTime,
                                          &xTransportGlobal);
        if (res) {
            res = AZURE_IOT_CONN_PARAM_ERR;
            goto exit;
        }

        /*Init Azure IoT ADU Client Options */
        res = AzureIoTADUClient_OptionsInit( &xADUOptions );
        if (res) {
            res = AZURE_IOT_CONN_UNSPECIFIED_ERR;
            goto exit;
        }

        res = AzureIoTADUClient_Init( &xAzureIoTADUClient, &xADUOptions );
        if (res) {
            res = AZURE_IOT_CONN_UNSPECIFIED_ERR;
            goto exit;
        }

        res = azure_iot_adu_properties_init();
        if (res) {
            res = AZURE_IOT_CONN_UNSPECIFIED_ERR;
            goto exit;
        }

        #ifdef democonfigDEVICE_SYMMETRIC_KEY
        if (piot_hub_local_conn_info->conn_cfg.secure_mode == AZURE_IOT_CONN_USING_SYMMETRIC_KEY) {
            AzureIoTHubClient_SetSymmetricKey(&xAzureIoTHubClient,
                                                     (const uint8_t *)azure_iot_hub_local_info.pucDeviceSymmetricKey,
                                                     strlen((char *)azure_iot_hub_local_info.pucDeviceSymmetricKey),
                                                     Crypto_HMAC);
        }
        #endif /* democonfigDEVICE_SYMMETRIC_KEY */

        if (piot_hub_local_conn_info->conn_cfg.secure_mode != AZURE_IOT_CONN_USING_SYMMETRIC_KEY) {
            LogInfo(("TODO x509 cert is not implemented\r\n"));
            res = AZURE_IOT_CONN_PARAM_ERR;
            goto exit;
        }

        /* Sends an MQTT Connect packet over the already established TLS connection,
         * and waits for connection acknowledgment (CONNACK) packet. */
        LogInfo( ( "Creating an MQTT connection to %s.", pucIotHubHostname ) );
        res = AzureIoTHubClient_Connect(&xAzureIoTHubClient,
                                             false, &xSessionPresent,
                                             sampleazureiotCONNACK_RECV_TIMEOUT_MS);
        if (res) {
            if (piot_hub_local_conn_info->conn_cfg.secure_mode == AZURE_IOT_CONN_USING_SYMMETRIC_KEY)
                res = AZURE_IOT_CONN_SYMMKEY_ERR;
            else
                res = AZURE_IOT_CONN_CERT_ERR;
            goto exit;
        }
        piot_hub_local_conn_info->ucIotHubConnState = AZURE_IOT_HUB_STATE_AzureConnected;

        res = AzureIoTHubClient_SubscribeCommand( &xAzureIoTHubClient, prvHandleCommand,
                                                      &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT );

        res |= AzureIoTHubClient_SubscribeProperties( &xAzureIoTHubClient, prvHandleProperties,
                                                         &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT );

        res |= AzureIoTHubClient_SubscribeCloudToDeviceMessage( &xAzureIoTHubClient, prvHandleCloudMessage,
                                                         &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT );
        if (res) {
            res = AZURE_IOT_CONN_SUBSCRIBE_ERR;
            goto exit;
        }

        res = AzureIoTADUClient_SendAgentState( &xAzureIoTADUClient,
                                                &xAzureIoTHubClient,
                                                &xADUDeviceProperties,
                                                NULL,
                                                eAzureIoTADUAgentStateIdle,
                                                NULL,
                                                ucScratchBuffer,
                                                sizeof( ucScratchBuffer ),
                                                NULL );
        if (res) {
            LogError(("Send Agent State to idle fial"));
            res = AZURE_IOT_CONN_UNSPECIFIED_ERR;
            goto exit;
        }

        /* Get property document after initial connection */
        res = AzureIoTHubClient_RequestPropertiesAsync(&xAzureIoTHubClient);
        if (res) {
            res = AZURE_IOT_CONN_PUBLISH_ERR;
            goto exit;
        }
        piot_hub_local_conn_info->ucIotHubConnState = AZURE_IOT_HUB_STATE_SubscribeOK;
    }else {
        return AZURE_IOT_CONN_WIFI_ERR;
    }

    return res;

exit:
    if (piot_hub_local_conn_info->ucIotHubConnState >= AZURE_IOT_HUB_STATE_AzureConnected)
        AzureIoTHubClient_Disconnect(&xAzureIoTHubClient);
    /* Close the network connection. */
    if (piot_hub_local_conn_info->ucIotHubConnState >= AZURE_IOT_HUB_STATE_TLSConnected)
        TLS_Socket_Disconnect(&xNetworkContextGlobal);
    piot_hub_local_conn_info->ucIotHubConnState = AZURE_IOT_HUB_STATE_IDLE;
    return res;
}

AzureIoTResult_t azure_iot_hub_disconnect(azure_iot_hub_local_info_t *piot_hub_local_conn_info)
{
    AzureIoTResult_t xResult;

    if (piot_hub_local_conn_info->ucIotHubConnState == AZURE_IOT_HUB_STATE_Terminate || \
        piot_hub_local_conn_info->ucIotHubConnState == AZURE_IOT_HUB_STATE_IDLE)
        return eAzureIoTSuccess;

    if (piot_hub_local_conn_info->ucIotHubConnState == AZURE_IOT_HUB_STATE_SubscribeOK) {
        AzureIoTHubClient_UnsubscribeProperties(&xAzureIoTHubClient);
        AzureIoTHubClient_UnsubscribeCommand(&xAzureIoTHubClient);
        AzureIoTHubClient_UnsubscribeCloudToDeviceMessage(&xAzureIoTHubClient);
    }

    /* Send an MQTT Disconnect packet over the already connected TLS over
     * TCP connection. There is no corresponding response for the disconnect
     * packet. After sending disconnect, client must close the network
     * connection. */
    if (piot_hub_local_conn_info->ucIotHubConnState == AZURE_IOT_HUB_STATE_AzureConnected)
        AzureIoTHubClient_Disconnect(&xAzureIoTHubClient);

    /* Close the network connection. */
    if (piot_hub_local_conn_info->ucIotHubConnState == AZURE_IOT_HUB_STATE_TLSConnected)
        TLS_Socket_Disconnect(&xNetworkContextGlobal);

    piot_hub_local_conn_info->ucIotHubConnState = AZURE_IOT_HUB_STATE_IDLE;
    sys_memset(&xNetworkContextGlobal, 0, sizeof(xNetworkContextGlobal));

//    azure_iot_hub_local_message_flush();

    //TODO Release iot_hub_local_info
    // sys_memset(azure_iot_hub_local_info_t, 0, sizeof(*azure_iot_hub_local_info_t));
    return 0;
}

azure_iot_at_data_t *azure_iot_at_data_construct(int topic_len, uint8_t *topic, int payload_len, uint8_t *payload)
{
    azure_iot_at_data_t *tmp;

    tmp = sys_malloc(sizeof(azure_iot_at_data_t));
    if(!tmp) {
        return NULL;
    }

    tmp->topic = sys_zalloc(topic_len);
    if(!tmp->topic) {
        sys_mfree(tmp);
        return NULL;
    }

    tmp->payload = sys_zalloc(payload_len);
    if(!tmp->payload) {
        sys_mfree(tmp->topic);
        sys_mfree(tmp);
        return NULL;
    }

    sys_memcpy(tmp->topic, topic, topic_len);
    sys_memcpy(tmp->payload, payload, payload_len);
    tmp->topic_len = topic_len;
    tmp->payload_len = payload_len;
    return tmp;
}

azure_iot_at_data_t *azure_iot_at_data_nopayload_construct(int topic_len, uint8_t *topic, int payload_len, uint8_t *payload)
{
    azure_iot_at_data_t *tmp;

    tmp = sys_malloc(sizeof(azure_iot_at_data_t));
    if(!tmp) {
        return NULL;
    }

    tmp->topic = sys_zalloc(topic_len);
    if (!(tmp->topic)) {
        sys_mfree(tmp);
        return NULL;
    }

    sys_memcpy(tmp->topic, topic, topic_len);
    tmp->topic_len = topic_len;
    tmp->payload = NULL;
    tmp->payload_len = payload_len;

    return tmp;
}

void azure_iot_at_data_free(azure_iot_at_data_t **data)
{
    azure_iot_at_data_t *tmp;
    if (!data)
        return;

    tmp = *data;
    if (!tmp)
        return;

    if (tmp->payload)
        sys_mfree(tmp->payload);
    if (tmp->topic)
        sys_mfree(tmp->topic);
    sys_mfree(tmp);

    *data = NULL;
}

static AzureIoTResult_t azure_iot_SendRAWTelemetry( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                  azure_iot_at_data_t *telemetry,
                                                  uint16_t * pusTelemetryPacketID )
{
    AzureIoTMQTTResult_t xMQTTResult;
    AzureIoTResult_t xResult;
    AzureIoTMQTTPublishInfo_t xMQTTPublishInfo = { 0 };
    uint16_t usPublishPacketIdentifier = 0;

    if( pxAzureIoTHubClient == NULL || telemetry == NULL)
    {
        AZLogError( ( "AzureIoTHubClient_SendTelemetry failed: invalid argument" ) );
        xResult = eAzureIoTErrorInvalidArgument;
    } else if  (azure_iot_hub_conn_state_get() != AZURE_IOT_HUB_STATE_SubscribeOK)
    {
        AZLogError( ( "Azure not connected yet" ) );
        xResult = eAzureIoTErrorPublishFailed;
    }
    else
    {
        xMQTTPublishInfo.xQOS = eAzureIoTMQTTQoS1;
        xMQTTPublishInfo.pcTopicName = ( const void * ) telemetry->topic;
        xMQTTPublishInfo.usTopicNameLength = ( uint16_t ) telemetry->topic_len;
        xMQTTPublishInfo.pvPayload = ( const void * ) telemetry->payload;
        xMQTTPublishInfo.xPayloadLength = telemetry->payload_len;

        /* Get a unique packet id. Not used if QOS is 0 */

        usPublishPacketIdentifier = AzureIoTMQTT_GetPacketId( &( pxAzureIoTHubClient->_internal.xMQTTContext ) );

        /* Send PUBLISH packet. */
        if( ( xMQTTResult = AzureIoTMQTT_Publish( &( pxAzureIoTHubClient->_internal.xMQTTContext ),
                                                  &xMQTTPublishInfo, usPublishPacketIdentifier ) ) != eAzureIoTMQTTSuccess )
        {
            LogError( ( "Failed to publish telemetry: MQTT error=0x%08x", xMQTTResult ) );
            xResult = eAzureIoTErrorPublishFailed;
        }
        else
        {
            if( pusTelemetryPacketID != NULL )
            {
                *pusTelemetryPacketID = usPublishPacketIdentifier;
            }

            LogInfo( ( "Successfully sent telemetry message" ) );
            xResult = eAzureIoTSuccess;
        }
    }

    return xResult;
}

AzureIoTResult_t azure_iot_SendRAWCommandResponse( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                        azure_iot_at_data_t *command)
{
    AzureIoTMQTTResult_t xMQTTResult;
    AzureIoTResult_t xResult;
    AzureIoTMQTTPublishInfo_t xMQTTPublishInfo = { 0 };

    if( ( pxAzureIoTHubClient == NULL ) ||
        ( command == NULL ) )
    {
        LogError( ( "AzureIoTHubClient_SendCommandResponse failed: invalid argument" ) );
        xResult = eAzureIoTErrorInvalidArgument;
    } else if  (azure_iot_hub_conn_state_get() != AZURE_IOT_HUB_STATE_SubscribeOK)
    {
        AZLogError( ( "Azure not connected yet" ) );
        xResult = eAzureIoTErrorPublishFailed;
    }
    else
    {
        xMQTTPublishInfo.xQOS = eAzureIoTMQTTQoS0;
        xMQTTPublishInfo.pcTopicName = command->topic;
        xMQTTPublishInfo.usTopicNameLength = ( uint16_t ) command->topic_len;

        if( ( command->payload == NULL ) || ( command->payload_len == 0 ) )
        {
            xMQTTPublishInfo.pvPayload = ( const void * ) azureiothubCOMMAND_EMPTY_RESPONSE;
            xMQTTPublishInfo.xPayloadLength = sizeof( azureiothubCOMMAND_EMPTY_RESPONSE ) - 1;
        }
        else
        {
            xMQTTPublishInfo.pvPayload = ( const void * ) command->payload;
            xMQTTPublishInfo.xPayloadLength = command->payload_len;
        }

        if( ( xMQTTResult = AzureIoTMQTT_Publish( &( pxAzureIoTHubClient->_internal.xMQTTContext ),
                                                  &xMQTTPublishInfo, 0 ) ) != eAzureIoTMQTTSuccess )
        {
            LogError( ( "Failed to publish response: MQTT error=0x%08x", xMQTTResult ) );
            xResult = eAzureIoTErrorPublishFailed;
        }
        else
        {
            xResult = eAzureIoTSuccess;
        }
    }

    return xResult;
}

AzureIoTResult_t azure_iot_SendRAWProperties(AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                        azure_iot_at_data_t *property)
{
    AzureIoTMQTTResult_t xMQTTResult;
    AzureIoTResult_t xResult;
    AzureIoTMQTTPublishInfo_t xMQTTPublishInfo = { 0 };

    if( ( pxAzureIoTHubClient == NULL ) ||
        ( property == NULL ) )
    {
        LogError( ( "azure_iot_SendRAWProperties failed: invalid argument" ) );
        xResult = eAzureIoTErrorInvalidArgument;
    } else if  (azure_iot_hub_conn_state_get() != AZURE_IOT_HUB_STATE_SubscribeOK)
    {
        AZLogError( ( "Azure not connected yet" ) );
        xResult = eAzureIoTErrorPublishFailed;
    }
    else
    {
        xMQTTPublishInfo.xQOS = eAzureIoTMQTTQoS0;
        xMQTTPublishInfo.pcTopicName = property->topic;
        xMQTTPublishInfo.usTopicNameLength = property->topic_len;
        xMQTTPublishInfo.pvPayload = ( const void * ) property->payload;
        xMQTTPublishInfo.xPayloadLength = property->payload_len;

        if( ( xMQTTResult = AzureIoTMQTT_Publish( &( pxAzureIoTHubClient->_internal.xMQTTContext ),
                                                  &xMQTTPublishInfo, 0 ) ) != eAzureIoTMQTTSuccess )
        {
            LogError( ( "Failed to Publish properties reported message: MQTT error=0x%08x", xMQTTResult ) );
            xResult = eAzureIoTErrorPublishFailed;
        }
        else
        {
            xResult = eAzureIoTSuccess;
        }
    }

    return xResult;
}
/*-----------------------------------------------------------*/

static void azure_iot_notify_connect_result(AzureIoTResult_t xResult)
{
    uint8_t res;
    uint8_t conn_stage;

    conn_stage = azure_iot_hub_conn_state_get();
    if ((xResult == 0) && (conn_stage == AZURE_IOT_HUB_STATE_SubscribeOK))
        res = AZURE_CONN_OK;
    else if (xResult && (conn_stage == AZURE_IOT_HUB_STATE_IDLE))
        res = AZURE_CONN_INTERNET_FAIL;
    else if (xResult && (conn_stage == AZURE_IOT_HUB_STATE_WiFiConnected))
        res = AZURE_CONN_INTERNET_FAIL;
    else if (xResult && (conn_stage >= AZURE_IOT_HUB_STATE_TLSConnected))
        res = AZURE_CONN_OTHER_FAIL;
    else
        res = AZURE_CONN_OTHER_FAIL;
    atcmd_azure_conn_rsp(res);
}

static void azure_iot_hub_local_message_dispatch(azure_at_message_t msg)
{
    uint8_t type;
    azure_iot_conn_cfg_t *conn_cfg;
    AzureIoTResult_t xResult;
    azure_iot_at_data_t *azure_data = NULL;

    type = msg.type;
    switch (type) {
        case AZURE_IOT_AT_CONNECT:
            LogInfo(("AT Connect received"));

            conn_cfg = (azure_iot_conn_cfg_t *)msg.payload;
            if (conn_cfg) {
                azure_iot_hub_local_info.conn_cfg.dps_disable = conn_cfg->dps_disable;
                azure_iot_hub_local_info.conn_cfg.secure_mode = conn_cfg->secure_mode;
            }
            xResult = azure_iot_hub_connect(&azure_iot_hub_local_info);
            azure_iot_notify_connect_result(xResult);
            break;
        case AZURE_IOT_AT_DISCONNECT:
            LogInfo(("AT Disconnect received"));
            xResult = azure_iot_hub_disconnect(&azure_iot_hub_local_info);
            break;
        case AZURE_IOT_AT_TELEMETRY:
            LogInfo(("AT Telemetry received"));
            azure_data = (azure_iot_at_data_t *)msg.payload;
            xResult = azure_iot_SendRAWTelemetry(&xAzureIoTHubClient,
                                                       azure_data,
                                                       NULL);
            if (xResult != eAzureIoTSuccess) {
                LogError(("Send Telemetry Fail:%d", xResult));
            }
            break;
        case AZURE_IOT_AT_PROPERTY:
            LogInfo(("AT Property RSP&REPORT received"));
            azure_data = (azure_iot_at_data_t *)msg.payload;
            xResult = azure_iot_SendRAWProperties(&xAzureIoTHubClient,
                                                        azure_data);
            if (xResult != eAzureIoTSuccess) {
                LogError(("Send Property Fail:%d", xResult));
            }
            break;
        case AZURE_IOT_AT_CMD:
            LogInfo(("AT CMDRSP received"));
            azure_data = (azure_iot_at_data_t *)msg.payload;
            xResult = azure_iot_SendRAWCommandResponse(&xAzureIoTHubClient,
                                                        azure_data);
            if (xResult != eAzureIoTSuccess) {
                LogError(("Send Property Fail:%d", xResult));
            } else {
                LogInfo( ( "Successfully sent command response" ) );
            }
            break;
        case AZURE_IOT_AT_OTA:
            break;
        case AZURE_IOT_AT_EXIT: //For Test
            LogInfo(("AT TERMINATE received"));
            azure_iot_hub_local_info.ucIotHubConnState = AZURE_IOT_HUB_STATE_Terminate;
            break;
        default:
            break;
    }

    if (azure_data) {
        azure_iot_at_data_free(&azure_data);
    } else if (msg.payload) {
        sys_mfree(msg.payload);
    }
}
/*-----------------------------------------------------------*/

int azure_iot_hub_local_init(void)
{
    char *components[8] = {0};

    components[0] = "deviceUpdate";
    components[1] = "chargeSensor";

    sys_memset(&azure_iot_hub_local_info, 0, sizeof(azure_iot_hub_local_info));
    azure_iot_hub_local_info.ulIotPort = democonfigIOTHUB_PORT;
    azure_iot_hub_local_info.ucIotHubConnState = AZURE_IOT_HUB_STATE_IDLE;
    azure_iot_hub_local_info.conn_cfg.dps_disable = 0; //dps default;
    azure_iot_hub_local_info.conn_cfg.secure_mode = 0; //symmetric key


    sys_memcpy(azure_iot_hub_local_info.pucIotModuleID, democonfigMODULE_ID, sizeof( democonfigMODULE_ID ) - 1);
    azure_iot_hub_model_update((uint8_t *)AZ_IOT_CHARGE_CONTROLLER_CLIENT_AGENT_MODEL_ID, sizeof(AZ_IOT_CHARGE_CONTROLLER_CLIENT_AGENT_MODEL_ID) - 1);
    //azure_iot_hub_model_update((uint8_t *)AZ_IOT_ADU_CLIENT_AGENT_MODEL_ID, sizeof(AZ_IOT_ADU_CLIENT_AGENT_MODEL_ID) - 1);
    azure_iot_hub_component_update((const uint8_t **)&components, 2);
    azure_iot_hub_endpoint_update((uint8_t *)democonfigENDPOINT, sizeof(democonfigENDPOINT) - 1); //TODO
    azure_iot_hub_idscope_update((uint8_t *)democonfigID_SCOPE, sizeof( democonfigID_SCOPE ) - 1);
    azure_iot_hub_registrationid_update((uint8_t *)democonfigREGISTRATION_ID, sizeof( democonfigREGISTRATION_ID ) - 1);
    azure_iot_hub_deviceid_update((uint8_t *)democonfigDEVICE_ID, sizeof( democonfigDEVICE_ID ) - 1);
    azure_iot_hub_hostname_update(( uint8_t * ) democonfigHOSTNAME, sizeof( democonfigHOSTNAME ) - 1);
    azure_iot_hub_symkey_update(( const uint8_t * ) democonfigDEVICE_SYMMETRIC_KEY, sizeof( democonfigDEVICE_SYMMETRIC_KEY ) - 1);

    azure_iot_adu_manufacturer_update(( const uint8_t * ) democonfigADU_DEVICE_MANUFACTURER, sizeof( democonfigADU_DEVICE_MANUFACTURER ) - 1);
    azure_iot_adu_model_update(( const uint8_t * ) democonfigADU_DEVICE_MODEL, sizeof( democonfigADU_DEVICE_MODEL ) - 1);
    azure_iot_adu_provider_update(( const uint8_t * ) democonfigADU_UPDATE_PROVIDER, sizeof( democonfigADU_UPDATE_PROVIDER ) - 1);
    azure_iot_adu_updatename_update(( const uint8_t * ) democonfigADU_UPDATE_NAME, sizeof( democonfigADU_UPDATE_NAME ) - 1);
    azure_iot_adu_updatever_update(( const uint8_t * ) democonfigADU_UPDATE_VERSION, sizeof( democonfigADU_UPDATE_VERSION ) - 1);

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

void azure_iot_hub_local_deinit(void)
{
    if (azure_iot_hub_local_info.pucx509Cert)
        sys_mfree(azure_iot_hub_local_info.pucx509Cert);

    sys_memset(&azure_iot_hub_local_info, 0, sizeof(azure_iot_hub_local_info));
    sys_memset(&xNetworkContextGlobal, 0, sizeof(NetworkContext_t));
    sys_memset(&xTlsTransportParamsGlobal, 0, sizeof(TlsTransportParams_t));
    sys_memset(&xTransportGlobal, 0, sizeof(AzureIoTTransportInterface_t));
}
/*-----------------------------------------------------------*/

void azure_iot_hub_conn_state_set(uint8_t state)
{
    azure_iot_hub_local_info.ucIotHubConnState = state;
}
/*-----------------------------------------------------------*/

int azure_iot_hub_local_message_wait(uint32_t timeout)
{
    azure_at_message_t message;

    if (sys_task_wait(timeout, &message) == OS_OK) {
        azure_iot_hub_local_message_dispatch(message);
    }
    return 0;
}
/*-----------------------------------------------------------*/

void azure_iot_hub_local_message_flush(void)
{
    azure_at_message_t msg;

    while (sys_task_msg_num(azure_task_tcb, 0)) {
        sys_task_wait(1, &msg);
        if (msg.payload) {
            if (msg.type == AZURE_IOT_AT_TELEMETRY ||
                msg.type == AZURE_IOT_AT_PROPERTY ||
                msg.type == AZURE_IOT_AT_CMD) {
                azure_iot_at_data_t *azure_data = (azure_iot_at_data_t *)msg.payload;
                azure_iot_at_data_free(&azure_data);
            } else {
                sys_mfree(msg.payload);
            }
        }
    }
}
/*-----------------------------------------------------------*/

#endif /* CONFIG_AZURE_F527_DEMO_SUPPORT */