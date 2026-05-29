/*
 * FreeRTOS V202212.01
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#ifndef DEMO_CONFIG_H
#define DEMO_CONFIG_H

/* FreeRTOS config include. */
#include "FreeRTOSConfig.h"

/**************************************************/
/******* DO NOT CHANGE the following order ********/
/**************************************************/

/* Include logging header files and define logging macros in the following order:
 * 1. Include the header file "logging_levels.h".
 * 2. Define the LIBRARY_LOG_NAME and LIBRARY_LOG_LEVEL macros depending on
 * the logging configuration for DEMO.
 * 3. Include the header file "logging_stack.h", if logging is enabled for DEMO.
 */

#include "logging_levels.h"

/* Logging configuration for the Demo. */
#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME    "ShadowDemo"
#endif

#ifndef LIBRARY_LOG_LEVEL
    #define LIBRARY_LOG_LEVEL    LOG_INFO
#endif

/* Prototype for the function used to print to console on Windows simulator
 * of FreeRTOS.
 * The function prints to the console before the network is connected;
 * then a UDP port after the network has connected. */
extern void vLoggingPrintf( const char * pcFormatString,
                            ... );

/* Map the SdkLog macro to the logging function to enable logging
 * on Windows simulator. */
#ifndef SdkLog
    #define SdkLog( message )    vLoggingPrintf message
#endif

#include "logging_stack.h"

/************ End of logging configuration ****************/

/**
 * @brief The Thing resource registered on your AWS IoT account to use in the demo.
 * A Thing resource is required to communicate with the AWS IoT Device Shadow service.
 *
 * @note The Things associated with your AWS account can be found in the
 * AWS IoT console under Manage/Things, or using the ListThings REST API (that can
 * be called with the AWS CLI command line tool).
 *
 * #define democonfigTHING_NAME    "...insert here..."
 */

#ifndef democonfigCLIENT_IDENTIFIER

/**
 * @brief The MQTT client identifier used in this example.  Each client identifier
 * must be unique so edit as required to ensure no two clients connecting to the
 * same broker use the same client identifier.
 *
 * @note Appending __TIME__ to the client id string will reduce the possibility of a
 * client id collision in the broker. Note that the appended time is the compilation
 * time. This client id can cause collision, if more than one instance of the same
 * binary is used at the same time to connect to the broker.
 */
//    #define democonfigCLIENT_IDENTIFIER    "testClient"__TIME__
#define democonfigCLIENT_IDENTIFIER    "living_test"

#endif

/**
 * @brief The AWS IoT broker endpoint to connect to in the demo.
 *
 * @note Your AWS IoT Core endpoint can be found in the AWS IoT console under
 * Settings/Custom Endpoint, or using the DescribeEndpoint REST API (that can
 * be called with AWS CLI command line tool).
 *
 * #define democonfigMQTT_BROKER_ENDPOINT    "...insert here..."
 */
#define democonfigMQTT_BROKER_ENDPOINT    "a3083lmcq980qm-ats.iot.eu-north-1.amazonaws.com"

/**
 * @brief The port to use for the demo.
 *
 * In general, port 8883 is for secured MQTT connections.
 *
 * @note Port 443 requires use of the ALPN TLS extension with the ALPN protocol
 * name. Using ALPN with this demo would require additional changes, including
 * setting the `pAlpnProtos` member of the `NetworkCredentials_t` struct before
 * forming the TLS connection. When using port 8883, ALPN is not required.
 *
 * #define democonfigMQTT_BROKER_PORT    ( insert here. )
 */
#define democonfigMQTT_BROKER_PORT    8883

/**
 * @brief AWS root CA certificate.
 *
 * This certificate is used to identify the AWS IoT server and is publicly available.
 * Refer to the link below.
 * https://www.amazontrust.com/repository/AmazonRootCA1.pem
 *
 * @note This certificate should be PEM-encoded.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 *
 * #define democonfigROOT_CA_PEM    "...insert here..."
 */
#define democonfigROOT_CA_PEM   "-----BEGIN CERTIFICATE-----\r\n"\
                                "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\r\n"\
                                "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\r\n"\
                                "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\r\n"\
                                "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\r\n"\
                                "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\r\n"\
                                "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\r\n"\
                                "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\r\n"\
                                "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\r\n"\
                                "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\r\n"\
                                "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\r\n"\
                                "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\r\n"\
                                "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\r\n"\
                                "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\r\n"\
                                "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\r\n"\
                                "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\r\n"\
                                "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\r\n"\
                                "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\r\n"\
                                "rqXRfboQnoZsG4q5WTP468SQvvG5\r\n"\
                                "-----END CERTIFICATE-----\r\n"

/**
 * @brief Client certificate.
 *
 * Please refer to the AWS documentation below for details
 * regarding client authentication.
 * https://docs.aws.amazon.com/iot/latest/developerguide/client-authentication.html
 *
 * @note This certificate should be PEM-encoded.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 *
 * #define democonfigCLIENT_CERTIFICATE_PEM    "...insert here..."
 */

#define democonfigCLIENT_CERTIFICATE_PEM     "-----BEGIN CERTIFICATE-----\r\n"\
                                             "MIIDWjCCAkKgAwIBAgIVAKE2s4IjeKpsDkPEdfKVuryriTApMA0GCSqGSIb3DQEB\r\n"\
                                             "CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t\r\n"\
                                             "IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0yNDExMjYwOTE5\r\n"\
                                             "MjNaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh\r\n"\
                                             "dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC8fi2i2ff9Igt81xtP\r\n"\
                                             "hwfa9WjRqgLxGgdecdloAGx7VsDYZ+MrP1QBGe407GGyrOvFbeQiUKHoMS2h4vi+\r\n"\
                                             "ukSQ2+lf3KCrE/mYwtkWsnHT6x1FWA4OehA5+lWeGZWgnfP96Fr7yUd7Bp1RWxZU\r\n"\
                                             "5cA4WiBdnpB5V5b2yErikag8ueNm7hSPgDwfwwwYfqmHr/B8nZBIhx6G9+vh7rWQ\r\n"\
                                             "BE7IJUYDdhwx1nbC0NFW/LTYh8KyZjgYB+kHDxnzhdBjCArjGTh+LTjxb21CVNA6\r\n"\
                                             "Xka6Isuk0al1egeVGjahdNMogMdtdTOO+RzwAYHw250fDIign8sGvr4onppEmi0z\r\n"\
                                             "XgwRAgMBAAGjYDBeMB8GA1UdIwQYMBaAFOpQr7VSWEqtFDCq0gJaKIY22veGMB0G\r\n"\
                                             "A1UdDgQWBBQpmoAHaLI+2YMX1rNHP62yu93MSTAMBgNVHRMBAf8EAjAAMA4GA1Ud\r\n"\
                                             "DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAFvrrShykFiBAsaXCa2ijVWv1\r\n"\
                                             "/07klYHihHHP0FZJUhsq93eqAPFNFbAdX4VdiQBYf7t9PJB6jnN1JOI+LiniuwW2\r\n"\
                                             "PjNOhf6FO7RrLWTL3kivVxKqdF4fQVJFMWkdJ6MM1iTYTnJVzCNaKQiE1T867wMu\r\n"\
                                             "5KpNM78wugm/jkcqoYwAADtVTU0pyqZ+wlMVojYzet3zSnwqwkwwD1Bx3xAW2l/C\r\n"\
                                             "XGRvLOK8f+lh9rJnEvVlgcMgKKiBlZatvD4Ge87f8oS6vyYDBYvDTRCbZEEhF0zb\r\n"\
                                             "aX1ctPZT+RtOyGjM5Yj4t64ubcqHmqBIknsphxy9Rrc4/Y10BwxI7KUErIkFPw==\r\n"\
                                             "-----END CERTIFICATE-----\r\n"

/**
 * @brief Client's private key.
 *
 * Please refer to the AWS documentation below for details
 * regarding clientauthentication.
 * https://docs.aws.amazon.com/iot/latest/developerguide/client-authentication.html
 *
 * @note This private key should be PEM-encoded.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN RSA PRIVATE KEY-----\n"\
 * "...base64 data...\n"\
 * "-----END RSA PRIVATE KEY-----\n"
 *
 * #define democonfigCLIENT_PRIVATE_KEY_PEM    "...insert here..."
 */
#define democonfigCLIENT_PRIVATE_KEY_PEM    "-----BEGIN RSA PRIVATE KEY-----\r\n"\
                                            "MIIEpAIBAAKCAQEAvH4totn3/SILfNcbT4cH2vVo0aoC8RoHXnHZaABse1bA2Gfj\r\n"\
                                            "Kz9UARnuNOxhsqzrxW3kIlCh6DEtoeL4vrpEkNvpX9ygqxP5mMLZFrJx0+sdRVgO\r\n"\
                                            "DnoQOfpVnhmVoJ3z/eha+8lHewadUVsWVOXAOFogXZ6QeVeW9shK4pGoPLnjZu4U\r\n"\
                                            "j4A8H8MMGH6ph6/wfJ2QSIcehvfr4e61kAROyCVGA3YcMdZ2wtDRVvy02IfCsmY4\r\n"\
                                            "GAfpBw8Z84XQYwgK4xk4fi048W9tQlTQOl5GuiLLpNGpdXoHlRo2oXTTKIDHbXUz\r\n"\
                                            "jvkc8AGB8NudHwyIoJ/LBr6+KJ6aRJotM14MEQIDAQABAoIBAQC47S4GzMRBI6qT\r\n"\
                                            "k1KnBnpNdmGc+agjNP8vyJCcOXY015shdWLpZhsbbX5Hi/YZ3w79RmAAzozaCY2U\r\n"\
                                            "euLB9Gsal7Zxpzo2PcWt+tQTMYT9fXjdNJOt3lGXICPplX4381+SpujEh+fKKmYj\r\n"\
                                            "kDLrnfClGL8SOmPRuH5SpZTajj65O5LwgYFrjDSfGbAIusmFIbDCqOYQkgQR1rsw\r\n"\
                                            "Z3atXU38+vYQ4ySHVRMr0ZLa7ByMJnvt6rKn1X9rdumtIc6PqzwdqeUBEc8s5cXE\r\n"\
                                            "mYW7nbRbeOI8k49UBMKNuT8qqjvZMcymJ/86dG0czdCfFVdusf82CpvQGJudZtBz\r\n"\
                                            "ZH7Adz2FAoGBAObSt1XD+/k+6bX/+qTeee1bKqt7DWWxseJfByA85J3pbSM0Fmd9\r\n"\
                                            "36nWVyTuziZ/qJbRIJPwBUeJSJS6O4fLYUz8t3l1jYzQMth9PqnTEc++SpeKRXrp\r\n"\
                                            "n91tXbc4igASjqUxqqI7OfbId99yFdTMFlmPTyQVB47h4Q+7jzabgQSXAoGBANEN\r\n"\
                                            "eS1juUthsqKnodvVsx69upsnj6RH80DSdv1OjZZmUnvIInXeYXzEpCNA4mVF0OnL\r\n"\
                                            "BVJJPU0x1KBajMWxpcNhADE4TZ7D2KNlgUJ2+OJ/Asl+Gzv+W0JhgEUGbsYwGAnX\r\n"\
                                            "yezVKt7xqPKeqiwuNudlZcdWy/s3xq/y6WgJ9kGXAoGBALXK/9nsoWeMxUUydpWj\r\n"\
                                            "IyRtyQ2FvOd7LD692T7qtqQoVGyFeZ/I438/8SKFcpefs4gU7TOWtpHA7gk1I+2v\r\n"\
                                            "oNKht19R1koxL9qRyei29DBKDzWLEW3c0qvylvzW6tljxDUXCOz8+IoFGWRC1i7j\r\n"\
                                            "Djc7R3mDlpBSFo/tiL3Ze6PBAoGAbpXeADC8mC+6O2jJqmzFMAHVcLXgulqFR0kO\r\n"\
                                            "QLkJFQJTp78a25cjosiyuyNbn47rEIGg8wJjNy6g96JuzTVIJq5tV5wdE1sugmDz\r\n"\
                                            "m6Erz0S4yLW42meXBupk3B6nApf2X16TJoEtCHp1kMJ16qEX0hhFWsmDZgT/SZwJ\r\n"\
                                            "9EswxUkCgYB21ZiblPuzR6HgY1q3cqfxB8dSitTAOdZKsa5RsqaPNcTjg68JILOu\r\n"\
                                            "JfVv5vPDUJy0z4KN+JgwSkkcn4MzWfdwu25wZdEOy1H9cdVEudeiqBKnbQojry5p\r\n"\
                                            "UJkoAIpyHb7xeshAQSVUGfOO7xG6V0uVJvBvGMgbobrgPDYI7JH2DA==\r\n"\
                                            "-----END RSA PRIVATE KEY-----\r\n"

/**
 * @brief The username value for authenticating client to the MQTT broker when
 * username/password based client authentication is used.
 *
 * Please refer to the AWS IoT documentation below for
 * details regarding client authentication with a username and password.
 * https://docs.aws.amazon.com/iot/latest/developerguide/custom-authentication.html
 * An authorizer setup needs to be done, as mentioned in the above link, to use
 * username/password based client authentication.
 *
 * #define democonfigCLIENT_USERNAME    "...insert here..."
 */

/**
 * @brief The password value for authenticating client to the MQTT broker when
 * username/password based client authentication is used.
 *
 * Please refer to the AWS IoT documentation below for
 * details regarding client authentication with a username and password.
 * https://docs.aws.amazon.com/iot/latest/developerguide/custom-authentication.html
 * An authorizer setup needs to be done, as mentioned in the above link, to use
 * username/password based client authentication.
 *
 * #define democonfigCLIENT_PASSWORD    "...insert here..."
 */

/**
 * @brief The name of the operating system that the application is running on.
 * The current value is given as an example. Please update for your specific
 * operating system.
 */
#define democonfigOS_NAME                   "FreeRTOS"

/**
 * @brief The version of the operating system that the application is running
 * on. The current value is given as an example. Please update for your specific
 * operating system version.
 */
#define democonfigOS_VERSION                tskKERNEL_VERSION_NUMBER

/**
 * @brief The name of the hardware platform the application is running on. The
 * current value is given as an example. Please update for your specific
 * hardware platform.
 */
#define democonfigHARDWARE_PLATFORM_NAME    "WinSim"

/**
 * @brief The name of the MQTT library used and its version, following an "@"
 * symbol.
 */
#include "core_mqtt.h"     /* Include coreMQTT header for MQTT_LIBRARY_VERSION macro. */
#define democonfigMQTT_LIB               "core-mqtt@"MQTT_LIBRARY_VERSION

/**
 * @brief Set the stack size of the main demo task.
 *
 * In the Windows port, this stack only holds a structure. The actual
 * stack is created by an operating system thread.
 */
#define democonfigDEMO_STACKSIZE         configMINIMAL_STACK_SIZE

/**
 * @brief Size of the network buffer for MQTT packets.
 */
#define democonfigNETWORK_BUFFER_SIZE    ( 1024U )

/**
 * @brief Predefined shadow name.
 *
 * Defaults to unnamed "Classic" shadow. Change to a custom string to use a named shadow.
 */
#ifndef democonfigSHADOW_NAME
    #define democonfigSHADOW_NAME    SHADOW_NAME_CLASSIC
#endif

#endif /* DEMO_CONFIG_H */
