# MQTT Client Example

Uses APIs from documents *AN185 GD32VW553 Network Application Development Guide.docx* and *AN158 GD32VW553 Wi-Fi Development Guide.docx* to make a simple mqtt client in station mode. It can establish a connection to mqtt server with multiple authentication methods and then subscribe or publish topic.
The SDK supports MQTT5.0 and MQTT3.1.1.

# Using Examples

This example is inside the MSDK\examples\wifi directory and can not be moved to other directory unless the example project configuration will be modified.

Before building the example project, please modify some codes:

* Change macros SSID and PASSWORD to your Wi-Fi access point in mqtt_client_main.c. If connecting to an open Wi-Fi network, set the macro PASSWORD to NULL or "".
* Change SERVER_PORT and sever_ip_addr in mqtt_client_main.c to your test mqtt server listener.
* If necessary, client_id and current_mqtt_mode can be modified. Now the default protocol is MQTT5.0 and if the server does not support it, it will automatically switch to MQTT3.1.1.

According to the different authentication methods, the following modifications are required.
Before test, please ensure that the following authentication methods are supported by the mqtt server.
Authentication methods 1 and 2 are unsafe and not recommended for use.

1. authentication: anonymous logon; encryption: NULL;
   1. set client_user and client_password be NULL in client_info in mqtt_client_main.c.
   2. set tls_auth_mode be TLS_AUTH_MODE_NONE in mqtt_client_main.c.

2. authentication: username and password; encryption: NULL;
   1. set client_user and client_password in client_info in mqtt_client_main.c.
   2. set tls_auth_mode be TLS_AUTH_MODE_NONE in mqtt_client_main.c.

3. authentication: username and password; encryption: TLS without pre-shared key and certificate;
   1. set client_user and client_password in client_info in mqtt_client_main.c.
   2. set tls_auth_mode be TLS_AUTH_MODE_KEY_SHARE in mqtt_client_main.c.

4. authentication: username and password; encryption: TLS with pre-shared key;
   1. set client_user and client_password in client_info in mqtt_client_main.c.
   2. set tls_auth_mode be TLS_AUTH_MODE_PSK in mqtt_client_main.c.
   3. change psk and psk_identity in mqtt_ssl_config.c.

5. authentication: username and password; encryption: TLS with one-way certificate;
   1. set client_user and client_password in client_info in mqtt_client_main.c.
   2. set tls_auth_mode be TLS_AUTH_MODE_CERT_1WAY in mqtt_client_main.c.
   3. change certificate ca in mqtt_ssl_config.c.

6. authentication: client certificate; encryption: TLS with two-way certificate;
   1. set client_user and client_password be NULL in client_info mqtt_client_main.c.
   2. set tls_auth_mode be TLS_AUTH_MODE_CERT_2WAY mqtt_client_main.c.
   3. change certificates ca/client_crt and key client_private_key in mqtt_ssl_config.c.

Both the username and password, as well as the certificates and key, need to be obtained from the MQTT server.

Building the example project can refer to document *AN154 GD32VW553 Quick Development Guide.docx*.

## Using GD32 Embedded Builder
* Start Embedded Builder.exe, select the directory MSDK\examples\wifi as workspace.
* Import example project, select the directory MSDK\examples\wifi\mqtt_client\Eclipse_project.
* Configure ToolChain and Build Tool.
* Build project, after compilation the image will be saved in the directory MSDK\examples\wifi\mqtt_client\image.
* Use GDLINK/JLINK or dragging the image file into the USB disk to download image.

## Using Segger IDE
* Enter the directory MSDK\examples\wifi\mqtt_client\Segger_project, click the mqtt_client.emProject to open project.
* Build project, after compilation the image will be saved in the directory MSDK\examples\wifi\mqtt_client\image.
* Use GDLINK/JLINK or dragging the image file into the USB disk to download image.

* before test, please start mqtt server using some MQTT brokers, such as mosquitto. Mosquitto is a lightweight and open source MQTT broker. It implements the MQTT protocol versions 5.0, 3.1.1 and 3.1 and is convenient for test.
