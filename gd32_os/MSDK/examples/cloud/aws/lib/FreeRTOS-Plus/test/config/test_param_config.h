/*
 * FreeRTOS FreeRTOS LTS Qualification Tests preview
 * Copyright (C) 2022 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 * @file test_param_config.h
 * @brief This setup the test parameters for LTS qualification test.
 */

#ifndef TEST_PARAM_CONFIG_H
#define TEST_PARAM_CONFIG_H

#include "core_pkcs11_config.h"
/**
 * @brief Configuration that indicates if the device should generate a key pair.
 *
 * @note When FORCE_GENERATE_NEW_KEY_PAIR is set to 1, the device should generate
 * a new on-device key pair and output public key. When set to 0, the device
 * should keep existing key pair.
 *
 * #define FORCE_GENERATE_NEW_KEY_PAIR   0
 */
#define FORCE_GENERATE_NEW_KEY_PAIR   1

/**
 * @brief Endpoint of the MQTT broker to connect to in mqtt test.
 *
 * #define MQTT_SERVER_ENDPOINT   "PLACE_HOLDER"
 */
#define MQTT_SERVER_ENDPOINT   "PLACE_HOLDER"

/**
 * @brief Port of the MQTT broker to connect to in mqtt test.
 *
 * #define MQTT_SERVER_PORT       (8883)
 */
#define MQTT_SERVER_PORT       (8883)

/**
 * @brief The client identifier for MQTT test.
 *
 * #define MQTT_TEST_CLIENT_IDENTIFIER    "PLACE_HOLDER"
 */
#define MQTT_TEST_CLIENT_IDENTIFIER    "PLACE_HOLDER"

 /**
 * @brief Network buffer size specified in bytes. Must be large enough to hold the maximum
 * anticipated MQTT payload.
 *
 * #define MQTT_TEST_NETWORK_BUFFER_SIZE  ( 5000 )
 */
#define MQTT_TEST_NETWORK_BUFFER_SIZE  ( 5000 )

 /**
 * @brief Timeout for MQTT_ProcessLoop() function in milliseconds.
 * The timeout value is appropriately chosen for receiving an incoming
 * PUBLISH message and ack responses for QoS 1 and QoS 2 communications
 * with the broker.
 *
 * #define MQTT_TEST_PROCESS_LOOP_TIMEOUT_MS  ( 700 )
 */
#define MQTT_TEST_PROCESS_LOOP_TIMEOUT_MS  ( 700 )

/**
 * @brief Root certificate of the IoT Core.
 *
 * @note This certificate should be PEM-encoded.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 *
 * #define IOT_CORE_ROOT_CA NULL
 */
#define IOT_CORE_ROOT_CA NULL

/**
 * @brief Client certificate to connect to MQTT server.
 *
 * @note This certificate should be PEM-encoded.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 *
 * #define MQTT_CLIENT_CERTIFICATE NULL
 */
#define MQTT_CLIENT_CERTIFICATE NULL

/**
 * @brief Client private key to connect to MQTT server.
 *
 * @note This is should only be used for testing purpose.
 *
 * For qualification, the key should be generated on-device.
 *
 * #define MQTT_CLIENT_PRIVATE_KEY  NULL
 */
#define MQTT_CLIENT_PRIVATE_KEY  NULL

/**
 * @brief Endpoint of the echo server to connect to in transport interface test.
 *
 * #define ECHO_SERVER_ENDPOINT   "PLACE_HOLDER"
 */
#define ECHO_SERVER_ENDPOINT   "livingston"

/**
 * @brief Port of the echo server to connect to in transport interface test.
 *
 * #define ECHO_SERVER_PORT       (9000)
 */
#define ECHO_SERVER_PORT       (9000)

/**
 * @brief Root certificate of the echo server.
 *
 * @note This certificate should be PEM-encoded.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 *
 * #define ECHO_SERVER_ROOT_CA "PLACE_HOLDER"
 */
//#define ECHO_SERVER_ROOT_CA "echo-server-root-ca"

#define ECHO_SERVER_ROOT_CA "-----BEGIN CERTIFICATE-----\r\n"\
                            "MIID+zCCAuOgAwIBAgIUGZ8OWKccF/GWD4QugS+biOaQeG0wDQYJKoZIhvcNAQEL\r\n"\
                            "BQAwgYwxCzAJBgNVBAYTAlVTMQswCQYDVQQIDAJXQTEOMAwGA1UEBwwFUGxhY2Ux\r\n"\
                            "EzARBgNVBAoMCmdpZ2FkZXZpY2UxCzAJBgNVBAsMAklUMRMwEQYDVQQDDApsaXZp\r\n"\
                            "bmdzdG9uMSkwJwYJKoZIhvcNAQkBFhp0b25nemhvdS5kaUBnaWdhZGV2aWNlLmNv\r\n"\
                            "bTAeFw0yNDExMjEwODUwMTBaFw0yNTExMjEwODUwMTBaMIGMMQswCQYDVQQGEwJV\r\n"\
                            "UzELMAkGA1UECAwCV0ExDjAMBgNVBAcMBVBsYWNlMRMwEQYDVQQKDApnaWdhZGV2\r\n"\
                            "aWNlMQswCQYDVQQLDAJJVDETMBEGA1UEAwwKbGl2aW5nc3RvbjEpMCcGCSqGSIb3\r\n"\
                            "DQEJARYadG9uZ3pob3UuZGlAZ2lnYWRldmljZS5jb20wggEiMA0GCSqGSIb3DQEB\r\n"\
                            "AQUAA4IBDwAwggEKAoIBAQC1JTv53U7jzDXYgD/vQx/mWXCSYuQ8DWUsIKq+pD3Z\r\n"\
                            "mDpVhfBuSI4pGHKgVQvYRLLLBYdjmKUnt1BYCloGkXwTwDaZCGhJXz95Kr/qwpRb\r\n"\
                            "89ap5CmSWAeeXjFiLUschfovtYaPksdmfDctyhNmjXoh/a7GK6zzkcDBEwhfeCtx\r\n"\
                            "M21QQkNEMGqQu50b0ves99SxLuPvF68+rsCAOq/QYBIFZUxM30FretCSEQf861Dl\r\n"\
                            "xqs5Rkvy8E05662QZFvAJkFZfdTaWuctzgqTBzB1fcKDs+CJb2OY/4adzNNx1sGu\r\n"\
                            "iALM9ETBk0OhqJHk0/eQfbzu7Z7/9FFr9z/lw4egin5fAgMBAAGjUzBRMB0GA1Ud\r\n"\
                            "DgQWBBSec5SfhKxnHB9OEX6DBHG1aO+6ozAfBgNVHSMEGDAWgBSec5SfhKxnHB9O\r\n"\
                            "EX6DBHG1aO+6ozAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQAx\r\n"\
                            "egFO6n/inbd2egoT9JCNX+6lK3Urx3Dp703tBNReExPztB2VhOCp4P8BOYWE/Pz/\r\n"\
                            "eaCcxehTIgWSujqu4A1WlVzmMs51BF8Qnzg9gM4I2+nUQJNZEk/+88N6VpYr5QZF\r\n"\
                            "H30pLa/rC9pElLV37zcWO7CkCL+IMS9bd81kET0R1l7tHG7eEg+awIdqjp8j63H7\r\n"\
                            "K2bXDh6nLFEULOR1WuzMYOy6scKVT63Adu7VQie2OFZLSVn3PriQXxhF4zeGC+Eh\r\n"\
                            "m/aBW++9dXHpJ0XykxjLha+m7uT4RDHPGX97S1AMM4G/q+TApV/+s0+o2s05/qxy\r\n"\
                            "1loDENHLvkqdy527rlKH\r\n"\
                            "-----END CERTIFICATE-----\r\n"

/**
 * @brief Client certificate to connect to echo server.
 *
 * @note This certificate should be PEM-encoded.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 *
 * #define TRANSPORT_CLIENT_CERTIFICATE NULL
 */

#define TRANSPORT_CLIENT_CERTIFICATE    "-----BEGIN CERTIFICATE-----\r\n"\
                                        "MIID6jCCAtKgAwIBAgIUTMpxXzSKgu3xhC9464LvAVYEYWgwDQYJKoZIhvcNAQEL\r\n"\
                                        "BQAwgYwxCzAJBgNVBAYTAlVTMQswCQYDVQQIDAJXQTEOMAwGA1UEBwwFUGxhY2Ux\r\n"\
                                        "EzARBgNVBAoMCmdpZ2FkZXZpY2UxCzAJBgNVBAsMAklUMRMwEQYDVQQDDApsaXZp\r\n"\
                                        "bmdzdG9uMSkwJwYJKoZIhvcNAQkBFhp0b25nemhvdS5kaUBnaWdhZGV2aWNlLmNv\r\n"\
                                        "bTAeFw0yNDExMjEwODUwNTRaFw0yNTExMjEwODUwNTRaMIGMMQswCQYDVQQGEwJV\r\n"\
                                        "UzELMAkGA1UECAwCV0ExDjAMBgNVBAcMBVBsYWNlMRMwEQYDVQQKDApnaWdhZGV2\r\n"\
                                        "aWNlMQswCQYDVQQLDAJJVDETMBEGA1UEAwwKbGl2aW5nc3RvbjEpMCcGCSqGSIb3\r\n"\
                                        "DQEJARYadG9uZ3pob3UuZGlAZ2lnYWRldmljZS5jb20wggEiMA0GCSqGSIb3DQEB\r\n"\
                                        "AQUAA4IBDwAwggEKAoIBAQDyRztu6Y+y8PtNog9PzBOQ5YY2K7EkqAjgnoicrk6w\r\n"\
                                        "1xPR7nitt3ZsIC3cJ6FKPBr0HjtDL5bL2rWafa3QAKGj/kpdZwEFgJQermljnita\r\n"\
                                        "Z397JUcH8qchnlt86/sYV1fIDWuS0JqPghlWAE07ymK2yVQOmo3oU45zlsWKCGcQ\r\n"\
                                        "Pgc80SLVV6LCnkRgYnornsvqTLAQZ7Q1TffIsK1F8E99unK9Ei1UZyCJ+R5+rwn+\r\n"\
                                        "amTDcf8P3FgiWzCbEQXZK8R7a0CUtCbTs0VumeXQtDI7IGJh1sbzCiTm8GBN1J4/\r\n"\
                                        "RwvaZtaKU+POs0aIhKgBSJf3myoRtXIaQtht1fgW5Ft1AgMBAAGjQjBAMB0GA1Ud\r\n"\
                                        "DgQWBBSnfstfOGgVls9oXu15G4XWmxpF2jAfBgNVHSMEGDAWgBSec5SfhKxnHB9O\r\n"\
                                        "EX6DBHG1aO+6ozANBgkqhkiG9w0BAQsFAAOCAQEANDFm+UDzfH1+dyRI+C0smXF/\r\n"\
                                        "CTxD17YLCK/+eg3SVJfT+x4kqfFYe4EabOZt7PF7xHjXPp2TdU8JYRLIskvB5K8X\r\n"\
                                        "6AonboOvTs1ltcMB7UOnEchFN/PbnwwWbk2D9aSBwzcEGWuXjBPvfCbVB+LuqzaM\r\n"\
                                        "M0Tj+bynRcuq94pW8bAfmU7UPVbEAhoF6ExFr0Odis93FucjoOHVw3rC+CvW483C\r\n"\
                                        "p/XscHaKGSy4Hdb0raOPJTBSdnH1ahAlX3Z3QhGe2YcgYYRgjq8Qi2i7Tcz+A8Lg\r\n"\
                                        "FA36QMzOgiP4ew80HDyTvOKRDWvndqyXsnAz1nrYds+MBScHR2/mO2PIYzbPLg==\r\n"\
                                        "-----END CERTIFICATE-----\r\n"


/**
 * @brief Client private key to connect to echo server.
 *
 * @note This is should only be used for testing purpose.
 *
 * For qualification, the key should be generated on-device.
 *
 * #define TRANSPORT_CLIENT_PRIVATE_KEY  NULL
 */
 #define TRANSPORT_CLIENT_PRIVATE_KEY   "-----BEGIN PRIVATE KEY-----\r\n"\
                                        "MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDyRztu6Y+y8PtN\r\n"\
                                        "og9PzBOQ5YY2K7EkqAjgnoicrk6w1xPR7nitt3ZsIC3cJ6FKPBr0HjtDL5bL2rWa\r\n"\
                                        "fa3QAKGj/kpdZwEFgJQermljnitaZ397JUcH8qchnlt86/sYV1fIDWuS0JqPghlW\r\n"\
                                        "AE07ymK2yVQOmo3oU45zlsWKCGcQPgc80SLVV6LCnkRgYnornsvqTLAQZ7Q1TffI\r\n"\
                                        "sK1F8E99unK9Ei1UZyCJ+R5+rwn+amTDcf8P3FgiWzCbEQXZK8R7a0CUtCbTs0Vu\r\n"\
                                        "meXQtDI7IGJh1sbzCiTm8GBN1J4/RwvaZtaKU+POs0aIhKgBSJf3myoRtXIaQtht\r\n"\
                                        "1fgW5Ft1AgMBAAECggEACRTLcQJXLQ1kVgobidGr8BPyXmhv2OBliMnYdAsCjxVY\r\n"\
                                        "vFc0dcp5KV9haK2hmY88MWdKsF+Bxv1ZLkPEtpW3eVaoQe0A+ExUAPxnneDJXjef\r\n"\
                                        "6yVADte1FQkDA+EyhbyMYUbDf/rKuFNXkMYtHu8cmR5dxLhdVlZ/cey2VdCv0Ed4\r\n"\
                                        "QVVkGenH4gjPFK6SLsaiQlMRF5rqCRXnF8ShEY+BMWS6cZceWtiI0+bpaE/GPaVu\r\n"\
                                        "guJh483kogw66TsfH0AGzo2gbogFsbS8oqxbvaLTVNjhTr4nZOxHQyCL9N9St5jP\r\n"\
                                        "YvnaksDBcMjibBhHcPSW1BPVbE1MWPjfDFSBhid6EQKBgQD64klB4MeBPpGX4sGc\r\n"\
                                        "t4Ba9Ok9S8YDfyi5JdCXAMzPYKq0Jki7Dj9u9j6lnAmmMwIhHOoDE9GefD25FTx4\r\n"\
                                        "BCB4pOmuG15Coo7vSw636q6QfAGal5+KWW7rPN3ANUdhcF82FP7ZDjY2OsGzUJDV\r\n"\
                                        "hoBCAosuV3YrR//NVBswR5QkOQKBgQD3OAVbvS+ZOVxVNooBHElE7soAvaYNdhzj\r\n"\
                                        "lo4WkBzN61wmWi4snHSIa192NGHn/1byzNc1wFmsXiuKO63appoCq/55L46qBGO2\r\n"\
                                        "9l4rCIJ7liRxpRFXRUegewpU6hh1CiXlUjm23oLZV+IGlbkyzOZEe/MdvTG2nTHL\r\n"\
                                        "P7WwcwhJHQKBgQDc9Z9/nfEYK3sfQl8zH4q0kS87BRmIyt2a82sbE2FO49x0faNg\r\n"\
                                        "29OSbt4ODbeR4Srm7whLXZEo/FOARuvS6A+RMe19oi0KkxFAP+LeqJrGNfFTrmac\r\n"\
                                        "+tdt51WGi2Yqs0Wn5CXwFUw5xtYCj9p/tM4RVRwv3Gf/BpKMzJ6FjwUyQQKBgEod\r\n"\
                                        "wX1/eoW1bLkfYMB0eoLwFB+ku+PNRVv2ByM8kWYq6bWV08IMJePATR5jFfc31hl0\r\n"\
                                        "0BCNWlUS5nrK6ZRj3khuyBwM5fiS2FJCCnlcU8I5gTORCWHgo1i5ip9qj2qHYFUg\r\n"\
                                        "Ea2BiDkg3+KZgKx6QY7GmlyQHBdjiUomD5KW6iUBAoGBAL5y9C8gYNd8tI5+uWZ7\r\n"\
                                        "pfJik6FNk3i9LdSlX1UyY0D+FP/2FYE5b3AHU3fMh8zMsWtPotKfeAAUHGHC49nW\r\n"\
                                        "4wzxL2qC8nO+mgN/VYXIQQxkztDTZFNzriUTJbTvDsd/42M/qHGYYljlAsMHVbaX\r\n"\
                                        "NhTHOlTN0N+UINHTwJJQXCa1\r\n"\
                                        "-----END PRIVATE KEY-----\r\n"


/**
 * @brief The PKCS #11 supports RSA key function.
 *
 * Set to 1 if RSA private keys are supported by the platform. 0 if not.
 *
 * #define PKCS11_TEST_RSA_KEY_SUPPORT                     ( 1 )
 */
#define PKCS11_TEST_RSA_KEY_SUPPORT                     ( 1 )

/**
 * @brief The PKCS #11 supports EC key function.
 *
 * Set to 1 if elliptic curve private keys are supported by the platform. 0 if not.
 *
 * #define PKCS11_TEST_EC_KEY_SUPPORT                      ( 1 )
 */
#define PKCS11_TEST_EC_KEY_SUPPORT                      ( 1 )

/**
 * @brief The PKCS #11 supports import key method.
 *
 * Set to 1 if importing device private key via C_CreateObject is supported. 0 if not.
 *
 * #define PKCS11_TEST_IMPORT_PRIVATE_KEY_SUPPORT          ( 1 )
 */
#define PKCS11_TEST_IMPORT_PRIVATE_KEY_SUPPORT          ( 1 )

/**
 * @brief The PKCS #11 supports generate keypair method.
 *
 * Set to 1 if generating a device private-public key pair via C_GenerateKeyPair. 0 if not.
 *
 * #define PKCS11_TEST_GENERATE_KEYPAIR_SUPPORT            ( 1 )
 */
#define PKCS11_TEST_GENERATE_KEYPAIR_SUPPORT            ( 1 )

/**
 * @brief The PKCS #11 supports preprovisioning method.
 *
 * Set to 1 if preprovisioning is supported.
 *
 * #define PKCS11_TEST_PREPROVISIONED_SUPPORT              ( 0 )
 */

/**
 * @brief The PKCS #11 label for device private key for test.
 *
 * For devices with on-chip storage, this should match the non-test label.
 * For devices with secure elements or hardware limitations, this may be defined
 * to a different label to preserve AWS IoT credentials for other test suites.
 *
 * #define PKCS11_TEST_LABEL_DEVICE_PRIVATE_KEY_FOR_TLS    pkcs11configLABEL_DEVICE_PRIVATE_KEY_FOR_TLS
 */
#define PKCS11_TEST_LABEL_DEVICE_PRIVATE_KEY_FOR_TLS    pkcs11configLABEL_DEVICE_PRIVATE_KEY_FOR_TLS

/**
 * @brief The PKCS #11 label for device public key.
 *
 * For devices with on-chip storage, this should match the non-test label.
 * For devices with secure elements or hardware limitations, this may be defined
 * to a different label to preserve AWS IoT credentials for other test suites.
 *
 * #define PKCS11_TEST_LABEL_DEVICE_PUBLIC_KEY_FOR_TLS     pkcs11configLABEL_DEVICE_PUBLIC_KEY_FOR_TLS
 */
#define PKCS11_TEST_LABEL_DEVICE_PUBLIC_KEY_FOR_TLS     pkcs11configLABEL_DEVICE_PUBLIC_KEY_FOR_TLS

/**
 * @brief The PKCS #11 label for the device certificate.
 *
 * For devices with on-chip storage, this should match the non-test label.
 * For devices with secure elements or hardware limitations, this may be defined
 * to a different label to preserve AWS IoT credentials for other test suites.
 *
 * #define PKCS11_TEST_LABEL_DEVICE_CERTIFICATE_FOR_TLS    pkcs11configLABEL_DEVICE_CERTIFICATE_FOR_TLS
 */
#define PKCS11_TEST_LABEL_DEVICE_CERTIFICATE_FOR_TLS    pkcs11configLABEL_DEVICE_CERTIFICATE_FOR_TLS

/**
 * @brief The PKCS #11 supports storage for JITP.
 *
 * The PKCS11 test will verify the following labels with create/destroy objects.
 * PKCS11_TEST_LABEL_CODE_VERIFICATION_KEY
 * PKCS11_TEST_LABEL_JITP_CERTIFICATE
 * PKCS11_TEST_LABEL_ROOT_CERTIFICATE
 * Set to 1 if PKCS #11 supports storage for JITP certificate, code verify certificate,
 * and trusted server root certificate.
 *
 * #define PKCS11_TEST_JITP_CODEVERIFY_ROOT_CERT_SUPPORTED pkcs11configJITP_CODEVERIFY_ROOT_CERT_SUPPORTED
 */
#define PKCS11_TEST_JITP_CODEVERIFY_ROOT_CERT_SUPPORTED pkcs11configJITP_CODEVERIFY_ROOT_CERT_SUPPORTED

/**
 * @brief The PKCS #11 label for the object to be used for code verification.
 *
 * This label has to be defined if PKCS11_TEST_JITP_CODEVERIFY_ROOT_CERT_SUPPORTED is set to 1.
 *
 * #define PKCS11_TEST_LABEL_CODE_VERIFICATION_KEY    pkcs11configLABEL_CODE_VERIFICATION_KEY
 */
#define PKCS11_TEST_LABEL_CODE_VERIFICATION_KEY    pkcs11configLABEL_CODE_VERIFICATION_KEY

/**
 * @brief The PKCS #11 label for Just-In-Time-Provisioning.
 *
 * This label has to be defined if PKCS11_TEST_JITP_CODEVERIFY_ROOT_CERT_SUPPORTED is set to 1.
 *
 * #define PKCS11_TEST_LABEL_JITP_CERTIFICATE    pkcs11configLABEL_JITP_CERTIFICATE
 */
#define PKCS11_TEST_LABEL_JITP_CERTIFICATE    pkcs11configLABEL_JITP_CERTIFICATE

/**
 * @brief The PKCS #11 label for the AWS Trusted Root Certificate.
 *
 * This label has to be defined if PKCS11_TEST_JITP_CODEVERIFY_ROOT_CERT_SUPPORTED is set to 1.
 *
 * #define PKCS11_TEST_LABEL_ROOT_CERTIFICATE    pkcs11configLABEL_ROOT_CERTIFICATE
 */
#define PKCS11_TEST_LABEL_ROOT_CERTIFICATE    pkcs11configLABEL_ROOT_CERTIFICATE

#endif /* TEST_PARAM_CONFIG_H */
