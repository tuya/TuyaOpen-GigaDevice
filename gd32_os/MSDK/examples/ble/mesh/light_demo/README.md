# BLE Mesh Light Control Demo


## Overview

This demo shows how to control a light switch via BLE Mesh network. After powering on, the device automatically broadcasts and enters provisioning mode, with the red LED on.
You can use a mobile app to provision the device into the network; during provisioning, the blue LED will be on to distinguish it from unprovisioned devices.
After successful provisioning, the LED turns off. Once the App Key is bound, you can control the light switch via the GEN_ONOFF_SRV_MODEL. When the light is turned on, the green LED will be on.

**Note:** This demo must use the BLE_MAX LIB. Please ensure the BLE_MAX library is selected and linked during project configuration and compilation.

## Build/download Example

This example can not be moved to other directory unless the example project configuration is modified.

Building the example project can refer to document *AN154 GD32VW553 Quick Development Guide.docx*.

* start GD32EclipseIDE, import example project, select the directory living.
* configure ToolChain and Build Tool.
* build project, after compilation the image will be saved in the directory MSDK\examples\ble\mesh\light_demo\eclipse\ble_mesh_light_model
* use GDLINK/JLINK or dragging it into the USB disk to download image.

## Usage Instructions

1. Power on the device; it will enter unprovisioned state and automatically broadcast for provisioning. The red LED will be on.
2. Use a BLE Mesh mobile app to scan and provision the device. During provisioning, the blue LED will be on.
3. After successful provisioning, the LED turns off, indicating the device has joined the Mesh network.
4. After binding the App Key via GEN_ONOFF_SRV_MODEL, you can remotely control the light switch:
   - Turn on the light: green LED on.
   - Turn off the light: LED off.

## LED Status Description

## LED Status Description

| Status         | LED Color |
| -------------- | --------- |
| Unprovisioned  | Red On    |
| Provisioning   | Blue On   |
| Provisioned    | Off       |
| Light On       | Green On  |
| Light Off      | Off       |
