# BLE GATT Transfer + MQTTS + HTTPS Example

# Configure Example in main.c

/*==== Please check the following setting before build project =======*/
/*========================== USER CONFIGURATION ======================*/
/* Please choose BLE worked as Central or Periheral */
#define BLE_ROLE            BLE_CENTRAL
#define BLE_CENTRAL         1
#define BLE_PERIPHERAL      2

/* Please set the correct AP information */
#define SSID            "xxx"
#define PASSWORD        "xxxxxxx"

/* Please set the correct MQTT server information */
ip_addr_t MQTT_SERVER_IP = IPADDR4_INIT_BYTES(192, 168, 1, 12);
#define MQTT_SERVER_PORT     8883

/* Please set the correct HTTPS server information */
#define HTTPS_SERVER_NAME     "www.baidu.com"
#define HTTPS_SERVER_PORT     "443"

/* Please set round number */
#define TEST_ROUND            10000
/*========================= USER CONFIGURATION END ====================*/


# Build/download Example

This example can not be moved to other directory unless the example project configuration is modified.

Building the example project can refer to document *AN154 GD32VW553 Quick Development Guide.docx*.

* start GD32EclipseIDE, import example project, select the directory living.
* configure ToolChain and Build Tool.
* build project, after compilation the image will be saved in the directory MSDK\examples\combo\gatt_mqtts_https\image.bin
* use GDLINK/JLINK or dragging it into the USB disk to download image.


# Using Example

If BLE_ROLE is BLE_CENTRAL, after power up, start board ble will scan and try to connect to device which sending adv data with device name.

If BLE_ROLE is BLE_PERIPHERAL, after power up, start board will directly send advertising whose adv data include device name.

Both BLE_CENTRAL and BLE_PERIPHERAL, the WiFi will try to connect with the configured SSID. After WiFi connected, it will try to connect with MQTTS server and then HTTPS server.

Power up both sides, the ble connection will be establised automaticlly.
