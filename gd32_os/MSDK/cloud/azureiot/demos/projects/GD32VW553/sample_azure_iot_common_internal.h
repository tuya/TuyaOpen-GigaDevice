/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @brief Defines an interface to be used by samples when interacting with sample_azure_iot_pnp.c module.
 *        This interface allows the module implementing the specific plug-and-play model to exchange data
 *        with the module responsible for communicating with Azure IoT Hub.
 */

#ifndef SAMPLE_AZURE_IOT_COMMON_INTERNAL_H
#define SAMPLE_AZURE_IOT_COMMON_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>

#include "app_cfg.h"

#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
#include "azure_iot_hub_client.h"
#include "azure_iot_adu_client.h"
#include "azure_iot_hub_client_properties.h"
#include "demo_config.h"
#include "sample_azure_iot_f527_api.h"

enum azure_iot_hub_state {
    AZURE_IOT_HUB_STATE_IDLE = 0,
    AZURE_IOT_HUB_STATE_WiFiConnected,
    AZURE_IOT_HUB_STATE_TLSConnected,
    AZURE_IOT_HUB_STATE_AzureConnected,
    AZURE_IOT_HUB_STATE_SubscribeOK,
    AZURE_IOT_HUB_STATE_Terminate = 0xFF,
};

enum azure_iot_conn_secure_mode {
    AZURE_IOT_CONN_USING_SYMMETRIC_KEY = 0,
    AZURE_IOT_CONN_USING_X509_CERT,
};

typedef struct azure_iot_local_info {
    uint8_t pucIotModelID[128];
    uint8_t pucIotModuleID[128];
    azure_iot_comp_t pnp_comp[sampleaduPNP_COMPONENTS_LIST_LENGTH];
    uint8_t pucENDPOINT[128];
    uint8_t pucIotID_SCOPE[32];
    uint8_t pucRegistration_ID[128];
    uint8_t pucDevice_ID[128];
    uint8_t pucIotHubHostname[128];
    uint8_t pucDeviceSymmetricKey[256];
    uint8_t *pucx509Cert;
    uint32_t ulx509CertLength;
    uint32_t ulIotPort; //default 8883

    uint8_t pucADUManufacturer[64];
    uint8_t pucADUDeviceModel[32];
    uint8_t pucADUUpdateProvider[64];
    uint8_t pucADUUpdateName[32];
    uint8_t pucADUUpdateVersion[32];
    uint8_t pucADUUpdateID[256];
    uint8_t ucIotHubConnState;
    uint8_t adu_supported;
    azure_iot_conn_cfg_t conn_cfg;
} azure_iot_hub_local_info_t;

extern azure_iot_hub_local_info_t azure_iot_hub_local_info;
#endif /* CONFIG_AZURE_F527_DEMO_SUPPORT */
#endif /* ifndef SAMPLE_AZURE_IOT_COMMON_INTERNAL_H */
