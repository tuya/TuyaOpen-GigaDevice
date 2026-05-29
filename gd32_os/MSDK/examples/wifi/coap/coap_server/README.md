# COAP server Example

Uses APIs from documents *AN185 GD32VW553 Network Application Development Guide.docx* and *AN158 GD32VW553 Wi-Fi Development Guide.docx* to make a simple COAP server in station mode.

# Using Examples

This example is inside the MSDK\examples\wifi\coap\coap_server directory and can not be moved to other directory unless the example project configuration will be modified.

Before building the example project, please modify some codes:

* Change macros SSID and PASSWORD to your Wi-Fi access point(must be connected to the internet) in coap_server_main.c. If connecting to an open Wi-Fi network, set the macro PASSWORD to NULL or "".

Building the example project can refer to document *AN154 GD32VW553 Quick Development Guide.docx*.

## Using GD32 Embedded Builder
* Start Embedded Builder.exe, select the directory MSDK\examples\wifi\coap as workspace.
* Import example project, select the directory MSDK\examples\wifi\coap\coap_server\Eclpise_project.
* Configure ToolChain and Build Tool.
* Build project, after compilation the image will be saved in the directory MSDK\examples\wifi\coap\coap_server\image.
* Use GDLINK/JLINK or dragging the image file into the USB disk to download image.

## Using Segger IDE
* Enter the directory MSDK\examples\wifi\coap\coap_server\Segger_project, click the coap_server.emProject to open project.
* Build project, after compilation the image will be saved in the directory MSDK\examples\wifi\coap\coap_server\image.
* Use GDLINK/JLINK or dragging the image file into the USB disk to download image.

# Use base commands in GD32VW55x SDK to make a COAP server

* Enable macro CONFIG_COAP in MSDK\app\app_cfg.h.
* Build SDK project refer to document *AN154 GD32VW553 Quick Development Guide.docx*.
* After the firmware is successfully downloaded, open the serial port tool, now can use base commands to make a COAP server.

* `wifi_scan`
* `wifi_connect GL_6019 12345678`
* `coap_server -v 7`
* `coap_server stop`
* `wifi_disconnect`
