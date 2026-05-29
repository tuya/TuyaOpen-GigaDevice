# HTTPS Client Example

Uses APIs from documents *AN185 GD32VW553 Network Application Development Guide.docx* and *AN158 GD32VW553 Wi-Fi Development Guide.docx* to make a simple HTTPS client in station mode over a secure connection, including verifying the server certificate.

# Using Examples

This example is inside the MSDK\examples\wifi directory and can not be moved to other directory unless the example project configuration will be modified.

Before building the example project, please modify some codes:

* Change macros SSID and PASSWORD to your Wi-Fi access point(must be connected to the internet) in https_client_main.c. If connecting to an open Wi-Fi network, set the macro PASSWORD to NULL or "".

Building the example project can refer to document *AN154 GD32VW553 Quick Development Guide.docx*.

## Using GD32 Embedded Builder
* Start Embedded Builder.exe, select the directory MSDK\examples\wifi as workspace.
* Import example project, select the directory MSDK\examples\wifi\https_client\Eclipse_project.
* Configure ToolChain and Build Tool.
* Build project, after compilation the image will be saved in the directory MSDK\examples\wifi\https_client\image.
* Use GDLINK/JLINK or dragging the image file into the USB disk to download image.

## Using Segger IDE
* Enter the directory MSDK\examples\wifi\https_client\Segger_project, click the https_client.emProject to open project.
* Build project, after compilation the image will be saved in the directory MSDK\examples\wifi\https_client\image.
* Use GDLINK/JLINK or dragging the image file into the USB disk to download image.

# Use base commands in GD32VW55x SDK to make a HTTPS client

* Enable macro CONFIG_SSL_TEST in MSDK\app\app_cfg.h.
* Build SDK project refer to document *AN154 GD32VW553 Quick Development Guide.docx*.
* After the firmware is successfully downloaded, open the serial port tool, now can use base commands to make a HTTPS client.

* `wifi_scan`
* `wifi_connect GL_6019 12345678`
* `ssl_client -h www.baidu.com -p 443 -cert baidu -method head`
* `wifi_disconnect`
