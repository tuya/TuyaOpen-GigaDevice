# WebSocket Client Example

Uses APIs from documents *AN185 GD32VW553 Network Application Development Guide.docx* and *AN158 GD32VW553 Wi-Fi Development Guide.docx* to make a simple WebSocket client in station mode over a secure connection, including verifying the server certificate.

# Using Examples

This example is inside the MSDK\examples\wifi directory and can not be moved to other directory unless the example project configuration will be modified.

Before building the example project, please modify some codes:

* change macros SSID and PASSWORD to your Wi-Fi access point(must be connected to the internet) in websocket_client_main.c.If connecting to an open Wi-Fi network, set the macro PASSWORD to NULL or "".

Building the example project can refer to document *AN154 GD32VW553 Quick Development Guide.docx*.

## Using GD32 Embedded Builder
* Start Embedded Builder.exe, select the directory MSDK\examples\wifi as workspace.
* import example project, select the directory MSDK\examples\wifi\websocket_client\Eclipse_project.
* configure ToolChain and Build Tool.
* build project, after compilation the image will be saved in the directory MSDK\examples\wifi\websocket_client\image.
* Use GDLINK/JLINK or dragging the image file into the USB disk to download image.
