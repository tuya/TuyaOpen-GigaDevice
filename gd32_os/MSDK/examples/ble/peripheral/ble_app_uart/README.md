# BLE APP UART Example

# Build/download Example

This example can not be moved to other directory unless the example project configuration is modified.

Building the example project can refer to document *AN154 GD32VW553 Quick Development Guide.docx*.

* start GD32EclipseIDE, import example project, select the directory MSDK\examples\ble\peripheral\ble_app_uart\eclipse.
* configure ToolChain and Build Tool.
* build project, after compilation the image will be saved in the directory MSDK\examples\ble\peripheral\ble_app_uart\eclipse\ble_app_uart_out.
* use GDLINK/JLINK or dragging it into the USB disk to download image.

# Using Example

After powering up, start board will directly send advertising whose adv data include device name.

*  if using lightblue for testing, connection should be establised manually, and tool Husky should bu used on start board side.
*  if using another start board for testing, ble_app_uart_c image should be downloaded to it. Power up both sides, the connection will be establised automaticlly. Both sides need tool Husky for testing. Please send or receive uart data by Husky.

The output of boot up:
```
=== RF initialization finished ===
[ble_datatrans_srv_cb], svc_add_rsp status = 0x0
=== Adapter enable success ===
hci_ver 0xc, hci_subver 0x4708, lmp_ver 0xc, lmp_subver 0x86fd, manuf_name 0xc2b
adv_set_num 1, min_tx_pwr -3, max_tx_pwr 8, max_adv_data_len 989
sugg_max_tx_octets 251, sugg_max_tx_time 17040
loc irk: 58 9e 25 0c d9 23 e1 6a f6 c1 d2 04 ce 94 ca 28
identity addr 41:41:41:41:41:41
 === BLE Adapter enable complete ===
app_adv_mgr_evt_hdlr state change 0x0 ==> 0x1, reason 0x0
app_adv_mgr_evt_hdlr state change 0x1 ==> 0x2, reason 0x0
app_adv_mgr_evt_hdlr state change 0x2 ==> 0x3, reason 0x0
app_adv_mgr_evt_hdlr state change 0x3 ==> 0x4, reason 0x0
app_adv_mgr_evt_hdlr state change 0x4 ==> 0x6, reason 0x0
```

The output of connection:
```
connect success. conn idx:0, conn_hdl:0x1
app_adv_mgr_evt_hdlr state change 0x6 ==> 0x2, reason 0x0
le pkt size info: conn idx 0, tx oct 251, tx time 2120, rx oct 251, rx time 2120
app_adv_mgr_evt_hdlr state change 0x2 ==> 0x0, reason 0x0
conn_idx 0 pairing success, level 0x0 ltk_present 0 sc 0
```

send data:
```
datatrans srv send data:
aaaa
```

receive data:
```
datatrans srv receive data:
aaaa
```