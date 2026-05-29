# Azure (Microsoft IoT Hub)

The example include two parts:
* azure_lib to build libazureiot.a using MS Azure SDK.
* azure_example provide an example of how to use the libazureiot.a to connect and register to the cloud.

## Build Azure Library

Before using the example, user should build the azure library as below.

### Using GD32 Embedded Builder
* Start Embedded Builder.exe, select the directory MSDK\examples\cloud\azure\azure_lib as workspace.
* Import example project, select the directory MSDK\examples\cloud\azure\azure_lib\Eclipse_project.
* Configure ToolChain and Build Tool.
* Build project, after compilation the libazureiot.a will be saved in the directory MSDK\examples\cloud\azure\azure_lib\Eclipse_project\lib.

## Build Azure Example

This example is inside the MSDK\examples\cloud\azure\azure_example directory and can not be moved to other directory unless the example project configuration will be modified.

Before building the example project, please modify some codes:

* Change macros democonfigDEVICE_ID, democonfigHOSTNAME, democonfigDEVICE_SYMMETRIC_KEY(democonfigID_SCOPE and democonfigREGISTRATION_ID if democonfigENABLE_DPS_SAMPLE)
in demo_configu.h to user's registered value on Microsoft website.
* Change macros SSID and PASSWORD to your Wi-Fi access point(must be connected to the internet) in azure_example_main.c. If connecting to an open Wi-Fi network, set the macro PASSWORD to NULL or "".
* Building the example project can refer to document *AN154 GD32VW553 Quick Development Guide.docx*.

### Using GD32 Embedded Builder
* Start Embedded Builder.exe, select the directory MSDK\examples\cloud\azure\azure_example as workspace.
* Import example project, select the directory MSDK\examples\cloud\azure\azure_example\Eclipse_project.
* Build project, after compilation the image: image-all.bin and image-ota.bin will be saved in the directory MSDK\examples\cloud\azure\azure_example\Eclipse_project\image.
* Use GDLINK/JLINK or dragging the image-all.bin file into the USB disk to download image.

## Use Azure F527 Example

This Example is for VW553 device, but should cooperate will F527 board, to connect the Azure IoT Hub. VW553 is responsible for connect to the internet, while
F527 acts as an controller to notify the VW553 to connect the internet, send packets to Azure etc. UART interface and ATCMD is used between VW553 and F527.

## Prepare the board
Download F512 firmware to F527, and then connect PA8/PB15(USART0) with PC6/PC7 PIN.

## Build Azure f527 Example

This example is inside the MSDK\examples\cloud\azure\azure_example directory and can not be moved to other directory unless the example project configuration will be modified.

Before building the example project, please modify some codes:

* Change macros democonfigDEVICE_ID, democonfigHOSTNAME, democonfigDEVICE_SYMMETRIC_KEY(democonfigID_SCOPE and democonfigREGISTRATION_ID if democonfigENABLE_DPS_SAMPLE)
in demo_configu.h to user's registered value on Microsoft website.
* Change macros SSID and PASSWORD to your Wi-Fi access point(must be connected to the internet) in azure_example_main.c. If connecting to an open Wi-Fi network, set the macro PASSWORD to NULL or "".
* define CONFIG_AZURE_F527_DEMO_SUPPORT in GD32 Embedded Builder.
* deine macro CONFIG_BOARD to PLATFORM_BOARD_32VW55X_F527.
* Building the example project can refer to document *AN154 GD32VW553 Quick Development Guide.docx*.

### Using GD32 Embedded Builder
* Start Embedded Builder.exe, select the directory MSDK\examples\cloud\azure\azure_example as workspace.
* Import example project, select the directory MSDK\examples\cloud\azure\azure_example\Eclipse_project.
* Configure ToolChain and Build Tool.
* Build project, after compilation the image: image-all.bin and image-ota.bin will be saved in the directory MSDK\examples\cloud\azure\azure_example\Eclipse_project\image.
* Use GDLINK/JLINK or dragging the image-all.bin file into the USB disk to download image.

## Use Azure F527 Example
* When the image is running on VW553 device, reset the VW553 demo board.
* Power on F527. Then the device will connect and register itself to Azure IoT Hub.
