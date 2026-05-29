# Softap TCP Server Example

Uses APIs from documents *AN185 GD32VW553 Network Application Development Guide.docx* and *AN158 GD32VW553 Wi-Fi Development Guide.docx* to make a simple TCP server in softap mode. It can accept a request from the tcp client, then establish a connection with client and wait for some data to be received from the client. The received data will be printed and retransmitted back to the client.

# Using Examples

This example is inside the MSDK\examples\wifi directory and can not be moved to other directory unless the example project configuration will be modified.

Before building the example project, please modify some codes:

* change macros SSID and PASSWORD to your softap in softap_tcp_server_main.c. If starting an open softap, set the macro PASSWORD to NULL or "".

Building the example project can refer to document *AN154 GD32VW553 Quick Development Guide.docx*.

## Using GD32 Embedded Builder
* Start Embedded Builder.exe, select the directory MSDK\examples\wifi as workspace.
* import example project, select the directory MSDK\examples\wifi\softap_tcp_server\Eclipse_project.
* configure ToolChain and Build Tool.
* build project, after compilation the image will be saved in the directory MSDK\examples\wifi\softap_tcp_server\image.
* Use GDLINK/JLINK or dragging the image file into the USB disk to download image.

## Using Segger IDE
* Enter the directory MSDK\examples\wifi\softap_tcp_server\Segger_project, click the softap_tcp_server.emProject to open project.
* Build project, after compilation the image will be saved in the directory MSDK\examples\wifi\softap_tcp_server\image.
* Use GDLINK/JLINK or dragging the image file into the USB disk to download image.

# Use base commands in GD32VW55x SDK to make a softap TCP server

* Enable macro CONFIG_LWIP_SOCKETS_TEST in MSDK\app\app_cfg.h.
* Build SDK project refer to document *AN154 GD32VW553 Quick Development Guide.docx*.
* After the firmware is successfully downloaded, open the serial port tool, now can use base commands to make a softap TCP server.

* `wifi_ap test_ap 12345678 11 -a wpa2,wpa3` or `wifi_ap test_ap NULL 11`
* `socket_server 0 4065` // socket_server <0:TCP or 1:UDP> \<server port\>
* Wait for client to connect.
* `socket_close 0` // socket_close \<fd\>
* `wifi_stop_ap`
