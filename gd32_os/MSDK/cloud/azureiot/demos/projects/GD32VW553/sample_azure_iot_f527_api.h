/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @brief Defines an interface to be used by samples when interacting with sample_azure_iot_pnp.c module.
 *        This interface allows the module implementing the specific plug-and-play model to exchange data
 *        with the module responsible for communicating with Azure IoT Hub.
 */

#ifndef SAMPLE_AZURE_IOT_API_GD_H
#define SAMPLE_AZURE_IOT_API_GD_H

#include <stdbool.h>
#include <stdint.h>

#include "azure_iot_hub_client_properties.h"
#include "demo_config.h"
#include "azure_iot_adu_client.h"
#include "azure_iot_flash_platform_port.h"


#if 0
extern AzureIoTHubClient_t xAzureIoTHubClient;
extern AzureIoTADUClientDeviceProperties_t xADUDeviceProperties;
extern AzureIoTADUClient_t xAzureIoTADUClient;
extern AzureADUImage_t xImage;
#endif

/**
 * @brief Convenience macro to return if an operation failed.
 */
#define _aziot_RETURN_IF_FAILED(exp)       \
  do                                    \
  {                                     \
    AzureIoTResult_t const _az_result = (exp); \
    if (_az_result != eAzureIoTSuccess)   \
    {                                   \
      return _az_result;                \
    }                                   \
  } while (0)

typedef struct azure_at_message {
    uint8_t type;
    void *payload;
    uint32_t payload_len;
} azure_at_message_t;

typedef enum {
    AZURE_IOT_CONN_OK = 0,
    AZURE_IOT_CONN_CERT_ERR = 1,
    AZURE_IOT_CONN_SYMMKEY_ERR = 2,
    AZURE_IOT_CONN_PARAM_ERR = 3,
    AZURE_IOT_CONN_SUBSCRIBE_ERR = 4,
    AZURE_IOT_CONN_PUBLISH_ERR = 5,
    AZURE_IOT_CONN_WIFI_ERR = 6,
    AZURE_IOT_CONN_UNSPECIFIED_ERR = 0xFF,
} azure_iot_discon_reason_t;

enum azure_iot_at_message_type {
    AZURE_IOT_AT_CONNECT = 0,
    AZURE_IOT_AT_DISCONNECT,
    AZURE_IOT_AT_TELEMETRY,
    AZURE_IOT_AT_PROPERTY,
    AZURE_IOT_AT_CMD,
    AZURE_IOT_AT_OTA,
    AZURE_IOT_AT_EXIT = 0xFF,
};

typedef struct azure_iot_at_data {
    uint32_t topic_len;
    uint8_t *topic;

    uint32_t payload_len;
    uint8_t *payload;
} azure_iot_at_data_t;

typedef struct azure_iot_at_telemetry_data {
    uint32_t topic_len;
    uint8_t *topic;

    uint32_t payload_len;
    uint8_t *payload;
} azure_iot_at_telemetry_data_t;

typedef struct azure_iot_at_command_data {
    uint32_t topic_len;
    uint8_t *topic;

    uint32_t payload_len;
    uint8_t *payload;
} azure_iot_at_command_data_t;

typedef struct azure_iot_at_property_data {
    uint32_t topic_len;
    uint8_t *topic;

    uint32_t payload_len;
    uint8_t *payload;
} azure_iot_at_property_data_t;

typedef struct azure_iot_conn_cfg {
    uint8_t dps_disable; // 0: using dps, 1: direct to iothub
    uint8_t secure_mode; // 0: symmetric key, 1: x509 cert
} azure_iot_conn_cfg_t;

typedef struct azure_iot_comp
{
    uint8_t* ptr;
    int32_t size; // size must be >= 0
} azure_iot_comp_t;

int azure_iot_hub_local_init(void);
void azure_iot_hub_local_deinit(void);

AzureIoTResult_t azure_iot_hub_component_update(const uint8_t **component_str, uint32_t totallen);
AzureIoTResult_t azure_iot_hub_x509cert_update(const uint8_t *x509cert, uint32_t x509cert_len);
AzureIoTResult_t azure_iot_hub_symkey_update(const uint8_t *symkey, uint8_t symkey_len);
AzureIoTResult_t azure_iot_hub_endpoint_update(const uint8_t *endpoint, uint8_t endpoint_len);
AzureIoTResult_t azure_iot_hub_idscope_update(const uint8_t *idscope, uint8_t idscope_len);
AzureIoTResult_t azure_iot_hub_registrationid_update(const uint8_t *registrationid, uint8_t registrationid_len);
void azure_iot_hub_port_update(const uint32_t port);
AzureIoTResult_t azure_iot_hub_model_update(const uint8_t *model, uint8_t model_len);
AzureIoTResult_t azure_iot_hub_deviceid_update(const uint8_t *deviceid, uint8_t deviceid_len);
AzureIoTResult_t azure_iot_hub_hostname_update(const uint8_t *hostname, uint8_t hostname_len);
AzureIoTResult_t azure_iot_adu_manufacturer_update(const uint8_t *manuf, uint8_t manuf_len);
AzureIoTResult_t azure_iot_adu_model_update(const uint8_t *adumodel, uint8_t adumodel_len);
AzureIoTResult_t azure_iot_adu_provider_update(const uint8_t *provider, uint8_t provider_len);
AzureIoTResult_t azure_iot_adu_updatename_update(const uint8_t *name, uint8_t name_len);
AzureIoTResult_t azure_iot_adu_updatever_update(const uint8_t *version, uint8_t version_len);

azure_iot_at_data_t *azure_iot_at_data_construct(int topic_len, uint8_t *topic, int payload_len, uint8_t *payload);
azure_iot_at_data_t *azure_iot_at_data_nopayload_construct(int topic_len, uint8_t *topic, int payload_len, uint8_t *payload);
void azure_iot_at_data_free(azure_iot_at_data_t **data);
void prvDispatchPropertiesUpdate( AzureIoTHubClientPropertiesResponse_t * pxMessage );
void prvProcessADUPropertiesUpdate( AzureIoTHubClientPropertiesResponse_t * pxMessage );

uint8_t azure_iot_hub_conn_state_get(void);
void azure_iot_hub_conn_state_set(uint8_t state);
bool azure_iot_hub_azure_connected(void);


int azure_iot_hub_local_message_send(uint8_t type, uint8_t *payload, uint32_t len);

int azure_iot_hub_local_message_wait(uint32_t timeout);

void azure_iot_hub_local_message_flush(void);

#endif /* ifndef SAMPLE_AZURE_IOT_API_GD_H */
