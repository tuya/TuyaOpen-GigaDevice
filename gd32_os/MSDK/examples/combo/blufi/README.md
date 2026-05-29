# Blufi Example

Use APIs from *AN152 GD32VW553 BLE Development Guide.docx* to support blufi function.

# Using Example

This example is inside the MSDK\examples\combo\blufi directory and can not be moved to other directory unless the example project configuration is modified.

Building the example project can refer to document *AN154 GD32VW553 Quick Development Guide.docx*.

* start GD32EclipseIDE, import example project, select the directory MSDK\examples\combo\blufi\Eclipse_project.
* configure ToolChain and Build Tool.
* build project, after compilation the image will be saved in the directory MSDK\examples\combo\blufi.
* use GDLINK/JLINK or dragging it into the USB disk to download image.

## Example Output

The example will advertising blufi data after initializing. The espblufi tool can be used for Bluetooth provisioning.

The output from the uart will be:
```
Starting advertising blufi data example
blufi_adv_mgr_evt_hdlr state change 0x0 ==> 0x1, reason 0x0
blufi_adv_mgr_evt_hdlr state change 0x1 ==> 0x2, reason 0x0
blufi_adv_mgr_evt_hdlr state change 0x2 ==> 0x3, reason 0x0
blufi_adv_mgr_evt_hdlr state change 0x3 ==> 0x4, reason 0x0
blufi_adv_mgr_evt_hdlr state change 0x4 ==> 0x6, reason 0x0
blufi advertising started
```