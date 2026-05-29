# Alicloud

The example include two parts:
* alicloud lib to build libalicloud.a using alicloud SDK.
* alicloud_example provide an example of how to use the libalicloud.a to configure the network and then connect to the cloud.

## Build alicloud Library

Before using the example, user should build the alicloud library as below.
### Using GD32 Embedded Builder
* Start Embedded Builder.exe, select the directory MSDK\examples\cloud\alicloud\alibcloud_lib as workspace.
* Import example project, select the directory MSDK\examples\cloud\alicloud\alicloud_lib\Eclipse_project.
* Configure ToolChain and Build Tool.
* Build project, after compilation the libalicloud.a will be saved in the directory MSDK\examples\cloud\alicloud\alicloud_lib\Eclipse_project\lib.

## Build alicloud Example

This example is inside the MSDK\examples\cloud\alicloud\alicloud_example directory and can not be moved to other directory unless the example project configuration will be modified.

Before building the example project, please modify some codes:

* Change macros PRODUCT_KEY, PRODUCT_SECRET, DEVICE_NAME, DEVICE_SECRET(must be valid) in living_platform_ut.h to user's registered value on alicloud website.
Building the example project can refer to document *AN154 GD32VW553 Quick Development Guide.docx*.

### Using GD32 Embedded Builder
* Start Embedded Builder.exe, select the directory MSDK\examples\cloud\alicloud\alibcloud_example as workspace.
* Import example project, select the directory MSDK\examples\cloud\alicloud\alicloud_example\Eclipse_project.
* Configure ToolChain and Build Tool.
* Build project, after compilation the image: image-all.bin and image-ota.bin will be saved in the directory MSDK\examples\cloud\alicloud\alicloud_example\Eclipse_project\image.
* Use GDLINK/JLINK or dragging the image-all.bin file into the USB disk to download image.

## Use alicloud Example

When the image is running on VW553 device, operate the "云智能" app on mobilephone to configure the device and register it to the Alibaba Cloud.
