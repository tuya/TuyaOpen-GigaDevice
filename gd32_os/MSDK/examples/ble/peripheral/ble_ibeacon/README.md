# BLE iBeacon Example

Use APIs from *AN152 GD32VW553 BLE Development Guide.docx* to send iBeacon advertising data.

**Note: Before using iBeacon technology in devices, please visit https://developer.apple.com/ibeacon/ to obtain a license.**

# Using Example

This example is inside the MSDK\examples\ble\peripheral\ble_ibeacon directory and can not be moved to other directory unless the example project configuration is modified.

Building the example project can refer to document *AN154 GD32VW553 Quick Development Guide.docx*.

* start GD32EclipseIDE, import example project, select the directory MSDK\examples\ble\peripheral\ble_ibeacon\eclipse.
* configure ToolChain and Build Tool.
* build project, after compilation the image will be saved in the directory MSDK\examples\ble\peripheral\ble_ibeacon\eclipse\ibeacon.
* use GDLINK/JLINK or dragging it into the USB disk to download image.

## Example Output

The example will broadcast iBeacon data after initializing. Any BLE develop tool can be used to observe and view the advertising data.

The output from the uart will be:
```
Starting Gigadevice BLE iBeacon example
adv state change 0x0 ==> 0x1, reason 0x0
adv state change 0x1 ==> 0x2, reason 0x0
adv state change 0x2 ==> 0x3, reason 0x0
adv state change 0x3 ==> 0x6, reason 0x0
iBeacon advertising started
```