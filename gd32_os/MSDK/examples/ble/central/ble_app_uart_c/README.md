# BLE APP UART CLIENT Example

# Build/download Example

This example can not be moved to other directory unless the example project configuration is modified.

Building the example project can refer to document *AN154 GD32VW553 Quick Development Guide.docx*.

* start GD32EclipseIDE, import example project, select the directory living.
* configure ToolChain and Build Tool.
* build project, after compilation the image will be saved in the directory MSDK\examples\ble\central\ble_app_uart_c\eclipse\ble_app_uart_c_out
* use GDLINK/JLINK or dragging it into the USB disk to download image.

# Using Example

After powering up, start board will scan and try to connect to device which sending adv data with device name.

*  if using another start board for testing, ble_app_uart image should be downloaded to it. Power up both sides, the connection will be establised automaticlly. Both sides need tool Husky for testing. Please send or receive uart data by Husky.

The output of boot up:
```
=== RF initialization finished ===
=== Adapter enable success ===
hci_ver 0xc, hci_subver 0x71a1, lmp_ver 0xc, lmp_subver 0xcadb, manuf_name 0xc2b
adv_set_num 2, min_tx_pwr -30, max_tx_pwr 8, max_adv_data_len 989
sugg_max_tx_octets 251, sugg_max_tx_time 17040
loc irk: 2e 9b bf 5c 43 58 23 73 2b 1d cc 2a 90 1a 84 0a
identity addr AB:89:67:45:23:01
 === BLE Adapter enable complete ===
Ble Scan enabled status 0x0
```

The output of connection:
```
Ble Scan disabled  status 0x0
connect success. conn idx:0, conn_hdl:0x1
[ble_datatrans_cli_cb] conn_state_change_ind connected event, conn_idx = 0
conn_idx 0 pairing success, level 0x0 ltk_present 0 sc 0
[ble_datatrans_cli_cb] discovery result = 1, svc_instance_num = 1
```

send data:
```
datatrans cli send data:
aaaa
```

receive data:
```
datatrans cli receive data:
aaaa
```