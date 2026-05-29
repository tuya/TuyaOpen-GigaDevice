/*!
    \file    cmd_mesh_cfg.c
    \brief   Implementation of BLE mesh CLI commands.

    \version 2024-05-24, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2023, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/
#include "app_cfg.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ble_export.h"
#include "cmd_shell.h"
#include "api/mesh.h"
#include "dbg_print.h"
#include "app_mesh_cfg.h"
#include "mesh_util.h"

void cmd_ble_mesh_cfg_beacon(int argc, char **argv)
{
    uint16_t net_idx, addr;
    uint8_t val = 0;
    uint8_t status;

    if (argc < 4) {
        goto usage;
    }

    addr = (uint16_t)strtoul((const char *)argv[2], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[3], NULL, 0);

    if (!strcmp("get", argv[1])) {
        bt_mesh_cfg_cli_beacon_get(net_idx, addr, &status);
    } else if (!strcmp("set", argv[1]) && argc == 5) {
        val = (uint8_t)strtoul((const char *)argv[4], NULL, 0);
        bt_mesh_cfg_cli_beacon_set(net_idx, addr, val, &status);
    } else {
        goto usage;
    }

    app_print("mesh beacon net_idx: %u, addr: %u, beacon status: %u\r\n", net_idx, addr, status);
    return;

usage:
    app_print("Usage: ble_mesh_beacon <set or get> <addr> <net_idx> [val]\r\n");
    app_print("\t<set or get>: set or get handle.\r\n");
    app_print("\t<addr>: Address of the node's primary element.\r\n");
    app_print("\t<net_idx>: NetIdx that the node was provisioned to.\r\n");
    app_print("\t[val]: 1: enable; 0: disable.\r\n");
}

void cmd_ble_mesh_cfg_get_comp(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint8_t page;

    net_idx = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    dst = (uint16_t)strtoul((const char *)argv[2], NULL, 0);
    page = (uint8_t)strtoul((const char *)argv[3], NULL, 0);

    app_mesh_cfg_get_comp(net_idx, dst, page);
}

void cmd_ble_mesh_cfg_ttl(int argc, char **argv)
{
    uint8_t ttl;
    uint16_t net_idx;
    uint16_t dst;

    dst = (uint16_t)strtoul((const char *)argv[2], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[3], NULL, 0);

    if (!strcmp("get", argv[1])) {
        bt_mesh_cfg_cli_ttl_get(net_idx, dst, &ttl);
    } else if (!strcmp("set", argv[1])) {
        uint8_t val = (uint8_t)strtoul((const char *)argv[4], NULL, 0);
        bt_mesh_cfg_cli_ttl_set(net_idx, dst, val, &ttl);
    } else {
        goto usage;
    }

    app_print("Default TTL is 0x%02x\r\n", ttl);
    return;
usage:
    app_print("Usage: ble_mesh_ttl <set or get> <addr> <net_idx> [val]\r\n");
    app_print("\t<set or get>: set or get handle.\r\n");
    app_print("\t<addr>: Address of the node's primary element.\r\n");
    app_print("\t<net_idx>: NetIdx that the node was provisioned to.\r\n");
    app_print("\t[val]: ttl value.\r\n");

}

void cmd_ble_mesh_cfg_gatt_proxy(int argc, char **argv)
{
    uint8_t proxy;
    uint16_t net_idx;
    uint16_t dst;

    dst = (uint16_t)strtoul((const char *)argv[2], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[3], NULL, 0);

    if (!strcmp("get", argv[1])) {
        bt_mesh_cfg_cli_gatt_proxy_get(net_idx, dst, &proxy);
    } else if (!strcmp("set", argv[1])) {
        uint8_t val = (uint8_t)strtoul((const char *)argv[4], NULL, 0);
        bt_mesh_cfg_cli_gatt_proxy_set(net_idx, dst, val, &proxy);
    } else {
        goto usage;
    }

    app_print("GATT Proxy is set to 0x%02x\r\n", proxy);
    return;
usage:
    app_print("Usage: ble_mesh_gatt_proxy <set or get> <addr> <net_idx> [val]\r\n");
    app_print("\t<set or get>: set or get handle.\r\n");
    app_print("\t<addr>: Address of the node's primary element.\r\n");
    app_print("\t<net_idx>: NetIdx that the node was provisioned to.\r\n");
    app_print("\t[val]: proxy state.\r\n");

}

void cmd_ble_mesh_cfg_relay(int argc, char **argv)
{
    uint8_t relay, transmit;
    uint16_t net_idx;
    uint16_t dst;

    dst = (uint16_t)strtoul((const char *)argv[2], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[3], NULL, 0);

    if (!strcmp("get", argv[1])) {
        bt_mesh_cfg_cli_relay_get(net_idx, dst, &relay, &transmit);
    } else if (!strcmp("set", argv[1])) {
        uint8_t count, interval, new_transmit = 0;
        uint8_t val = strtoul(argv[4], NULL, 0);
        if (val) {
            count = (uint8_t)strtoul(argv[5], NULL, 0);
            interval = (uint8_t)strtoul(argv[6], NULL, 0);
            new_transmit = BT_MESH_TRANSMIT(count, interval);
        }
        bt_mesh_cfg_cli_relay_set(net_idx, dst, val, new_transmit, &relay, &transmit);
    } else {
        goto usage;
    }

    app_print("Relay is 0x%02x, Transmit 0x%02x (count %u interval %ums)\r\n", relay,
              transmit, BT_MESH_TRANSMIT_COUNT(transmit), BT_MESH_TRANSMIT_INT(transmit));

    return;
usage:
    app_print("Usage: ble_mesh_relay <set or get> <addr> <net_idx> [relay] [count] [interval]\r\n");
    app_print("\t<set or get>: set or get handle.\r\n");
    app_print("\t<addr>: Address of the node's primary element.\r\n");
    app_print("\t<net_idx>: NetIdx that the node was provisioned to.\r\n");
    app_print("\t[relay]: Relay state.\r\n");
    app_print("\t[count]: Relay retransmit count.\r\n");
    app_print("\t[count]: Relay retransmit interval steps.\r\n");
}

void cmd_ble_mesh_cfg_mod_pub(int argc, char **argv)
{
    uint16_t elem_addr, mod_id, cid;
    uint16_t net_idx;
    uint16_t dst;
    char *p_op = argv[1];

    dst = (uint16_t)strtoul((const char *)argv[2], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[3], NULL, 0);

    elem_addr = strtoul(argv[4], NULL, 0);
    mod_id = strtoul(argv[5], NULL, 0);

    argc -= 6;
    argv += 6;

    if (argc > 0) {
        cid = strtoul(argv[0], NULL, 0);
        argc--;
        argv++;
    } else {
        cid = APP_CID_NVAL;
    }

    if (!strcmp("set", p_op)) {
        if (argc < 8) {
            goto usage;
        }
        app_mesh_cfg_mod_pub_set(net_idx, dst, elem_addr, false, mod_id, cid, argv);
    } else if (!strcmp("get", p_op)) {
        app_mesh_cfg_mod_pub_get(net_idx, dst, elem_addr, mod_id, cid);
    } else {
        goto usage;
    }

    return;

usage:
    app_print("Usage: ble_mesh_mod_pub <set or get> <addr> <net_idx> <elem_addr> <mod_id> [cid] [pub_addr] [app_idx] [cred_flag] [ttl] [step resolution] [step num] [retransmit count] [retransmit interval step]\r\n");
}

void cmd_ble_mesh_cfg_mod_sub_add(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, sub_addr, mod_id;
    uint8_t status;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    elem_addr = strtoul(argv[3], NULL, 0);
    sub_addr = strtoul(argv[4], NULL, 0);
    mod_id = strtoul(argv[5], NULL, 0);

    err = bt_mesh_cfg_cli_mod_sub_add(net_idx, dst, elem_addr, sub_addr, mod_id, &status);

    if (err) {
        app_print("Unable to send Model Subscription Add (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Model Subscription Add failed with status 0x%02x\r\n", status);
    } else {
        app_print("Model subscription was successful\r\n");
    }
}

void cmd_ble_mesh_cfg_mod_sub_add_vnd(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, sub_addr, mod_id, cid;
    uint8_t status;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    elem_addr = strtoul(argv[3], NULL, 0);
    sub_addr = strtoul(argv[4], NULL, 0);
    mod_id = strtoul(argv[5], NULL, 0);
    cid = strtoul(argv[6], NULL, 0);

    err = bt_mesh_cfg_cli_mod_sub_add_vnd(net_idx, dst, elem_addr, sub_addr, mod_id, cid, &status);

    if (err) {
        app_print("Unable to send Model Subscription Add (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Model Subscription Add failed with status 0x%02x\r\n", status);
    } else {
        app_print("Model subscription was successful\r\n");
    }
}

void cmd_ble_mesh_cfg_mod_sub_del(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, sub_addr, mod_id;
    uint8_t status;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    elem_addr = strtoul(argv[3], NULL, 0);
    sub_addr = strtoul(argv[4], NULL, 0);
    mod_id = strtoul(argv[5], NULL, 0);

    err = bt_mesh_cfg_cli_mod_sub_del(net_idx, dst, elem_addr, sub_addr, mod_id, &status);

    if (err) {
        app_print("Unable to send Model Subscription Delete (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Model Subscription Delete failed with status 0x%02x\r\n", status);
    } else {
        app_print("Model subscription deltion was successful\r\n");
    }
}

void cmd_ble_mesh_cfg_mod_sub_del_vnd(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, sub_addr, mod_id, cid;
    uint8_t status;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    elem_addr = strtoul(argv[3], NULL, 0);
    sub_addr = strtoul(argv[4], NULL, 0);
    mod_id = strtoul(argv[5], NULL, 0);
    cid = strtoul(argv[6], NULL, 0);

    err = bt_mesh_cfg_cli_mod_sub_del_vnd(net_idx, dst, elem_addr, sub_addr, mod_id, cid, &status);

    if (err) {
        app_print("Unable to send Model Subscription Delete (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Model Subscription Delete failed with status 0x%02x\r\n", status);
    } else {
        app_print("Model subscription deltion was successful\r\n");
    }
}

void cmd_ble_mesh_cfg_mod_sub_add_va(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, sub_addr, mod_id;
    uint8_t label[16];
    uint8_t status;
    size_t len;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    elem_addr = strtoul(argv[3], NULL, 0);

    len = hex2bin(argv[4], strlen(argv[4]), label, sizeof(label));
    (void)memset(label + len, 0, sizeof(label) - len);

    mod_id = strtoul(argv[5], NULL, 0);

    err = bt_mesh_cfg_cli_mod_sub_va_add(net_idx, dst, elem_addr, label, mod_id, &sub_addr, &status);

    if (err) {
        app_print("Unable to send Mod Sub VA Add (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Mod Sub VA Add failed with status 0x%02x\r\n", status);
    } else {
        app_print("0x%04x subscribed to Label UUID %s (va 0x%04x)", elem_addr, argv[4], sub_addr);
    }
}

void cmd_ble_mesh_cfg_mod_sub_add_va_vnd(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, sub_addr, mod_id, cid;
    uint8_t label[16];
    uint8_t status;
    size_t len;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    elem_addr = strtoul(argv[3], NULL, 0);

    len = hex2bin(argv[4], strlen(argv[4]), label, sizeof(label));
    (void)memset(label + len, 0, sizeof(label) - len);

    mod_id = strtoul(argv[5], NULL, 0);
    cid = strtoul(argv[6], NULL, 0);

    err = bt_mesh_cfg_cli_mod_sub_va_add_vnd(net_idx, dst, elem_addr, label, mod_id, cid, &sub_addr,
                                             &status);

    if (err) {
        app_print("Unable to send Mod Sub VA Add (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Mod Sub VA Add failed with status 0x%02x\r\n", status);
    } else {
        app_print("0x%04x subscribed to Label UUID %s (va 0x%04x)\r\n", elem_addr, argv[4], sub_addr);
    }
}

void cmd_ble_mesh_cfg_mod_sub_del_va(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, sub_addr, mod_id;
    uint8_t label[16];
    uint8_t status;
    size_t len;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    elem_addr = strtoul(argv[3], NULL, 0);

    len = hex2bin(argv[4], strlen(argv[4]), label, sizeof(label));
    (void)memset(label + len, 0, sizeof(label) - len);

    mod_id = strtoul(argv[5], NULL, 0);

    err = bt_mesh_cfg_cli_mod_sub_va_del(net_idx, dst, elem_addr, label, mod_id, &sub_addr, &status);

    if (err) {
        app_print("Unable to send Model Subscription Delete (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Model Subscription Delete failed with status 0x%02x\r\n", status);
    } else {
        app_print("0x%04x unsubscribed from Label UUID %s (va 0x%04x)\r\n", elem_addr, argv[4], sub_addr);
    }
}

void cmd_ble_mesh_cfg_mod_sub_del_va_vnd(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, sub_addr, mod_id, cid;
    uint8_t label[16];
    uint8_t status;
    size_t len;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    elem_addr = strtoul(argv[3], NULL, 0);

    len = hex2bin(argv[4], strlen(argv[4]), label, sizeof(label));
    (void)memset(label + len, 0, sizeof(label) - len);

    mod_id = strtoul(argv[5], NULL, 0);
    cid = strtoul(argv[6], NULL, 0);

    err = bt_mesh_cfg_cli_mod_sub_va_del_vnd(net_idx, dst, elem_addr, label, mod_id, cid, &sub_addr,
                                             &status);

    if (err) {
        app_print("Unable to send Model Subscription Delete (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Model Subscription Delete failed with status 0x%02x\r\n", status);
    } else {
        app_print("0x%04x unsubscribed from Label UUID %s (va 0x%04x)\r\n", elem_addr, argv[4], sub_addr);
    }
}

void cmd_ble_mesh_cfg_mod_sub_ow(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, sub_addr, mod_id;
    uint8_t status;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    elem_addr = strtoul(argv[3], NULL, 0);
    sub_addr = strtoul(argv[4], NULL, 0);
    mod_id = strtoul(argv[5], NULL, 0);

    err = bt_mesh_cfg_cli_mod_sub_overwrite(net_idx, dst, elem_addr, sub_addr, mod_id, &status);

    if (err) {
        app_print("Unable to send Model Subscription Overwrite (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Model Subscription Overwrite failed with status 0x%02x\r\n", status);
    } else {
        app_print("Model subscription overwrite was successful\r\n");
    }
}

void cmd_ble_mesh_cfg_mod_sub_ow_vnd(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, sub_addr, mod_id, cid;
    uint8_t status;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    elem_addr = strtoul(argv[3], NULL, 0);
    sub_addr = strtoul(argv[4], NULL, 0);
    mod_id = strtoul(argv[5], NULL, 0);
    cid = strtoul(argv[6], NULL, 0);

    err = bt_mesh_cfg_cli_mod_sub_overwrite_vnd(net_idx, dst, elem_addr, sub_addr, mod_id, cid,
                                                &status);

    if (err) {
        app_print("Unable to send Model Subscription Overwrite (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Model Subscription Overwrite failed with status 0x%02x\r\n", status);
    } else {
        app_print("Model subscription overwrite was successful\r\n");
    }
}

void cmd_ble_mesh_cfg_mod_sub_ow_va(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, sub_addr, mod_id;
    uint8_t label[16];
    uint8_t status;
    size_t len;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    elem_addr = strtoul(argv[3], NULL, 0);

    len = hex2bin(argv[4], strlen(argv[4]), label, sizeof(label));
    (void)memset(label + len, 0, sizeof(label) - len);

    mod_id = strtoul(argv[5], NULL, 0);

    err = bt_mesh_cfg_cli_mod_sub_va_overwrite(net_idx, dst, elem_addr, label, mod_id, &sub_addr,
                                               &status);

    if (err) {
        app_print("Unable to send Mod Sub VA Overwrite (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Mod Sub VA Overwrite failed with status 0x%02x\r\n", status);
    } else {
        app_print("0x%04x overwrite to Label UUID %s (va 0x%04x)\r\n", elem_addr, argv[4], sub_addr);
    }
}

void cmd_ble_mesh_cfg_mod_sub_ow_va_vnd(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, sub_addr, mod_id, cid;
    uint8_t label[16];
    uint8_t status;
    size_t len;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    elem_addr = strtoul(argv[3], NULL, 0);

    len = hex2bin(argv[4], strlen(argv[4]), label, sizeof(label));
    (void)memset(label + len, 0, sizeof(label) - len);

    mod_id = strtoul(argv[5], NULL, 0);
    cid = strtoul(argv[6], NULL, 0);

    err = bt_mesh_cfg_cli_mod_sub_va_overwrite_vnd(net_idx, dst, elem_addr, label, mod_id, cid,
                                                   &sub_addr, &status);

    if (err) {
        app_print("Unable to send Mod Sub VA Overwrite (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Mod Sub VA Overwrite failed with status 0x%02x\r\n", status);
    } else {
        app_print("0x%04x overwrite to Label UUID %s (va 0x%04x)\r\n", elem_addr, argv[4], sub_addr);
    }
}

void cmd_ble_mesh_cfg_mod_sub_del_all(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, mod_id;
    uint8_t status;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    elem_addr = strtoul(argv[3], NULL, 0);
    mod_id = strtoul(argv[4], NULL, 0);

    err = bt_mesh_cfg_cli_mod_sub_del_all(net_idx, dst, elem_addr, mod_id, &status);

    if (err) {
        app_print("Unable to send Model Subscription Delete All (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Model Subscription Delete All failed with status 0x%02x\r\n", status);
    } else {
        app_print("Model subscription deltion all was successful\r\n");
    }
}

void cmd_ble_mesh_cfg_mod_sub_del_all_vnd(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, mod_id, cid;
    uint8_t status;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    elem_addr = strtoul(argv[3], NULL, 0);
    mod_id = strtoul(argv[4], NULL, 0);
    cid = strtoul(argv[5], NULL, 0);

    err = bt_mesh_cfg_cli_mod_sub_del_all_vnd(net_idx, dst, elem_addr, mod_id, cid, &status);

    if (err) {
        app_print("Unable to send Model Subscription Delete All (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Model Subscription Delete All failed with status 0x%02x\r\n", status);
    } else {
        app_print("Model subscription deltion all was successful\r\n");
    }
}

void cmd_ble_mesh_cfg_mod_sub_get(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, mod_id;
    uint16_t subs[16];
    uint8_t status;
    size_t cnt;
    int err = 0;
    int i;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    cnt = ARRAY_SIZE(subs);
    elem_addr = strtoul(argv[3], NULL, 0);
    mod_id = strtoul(argv[4], NULL, 0);

    err = bt_mesh_cfg_cli_mod_sub_get(net_idx, dst, elem_addr, mod_id, &status, subs, &cnt);

    if (err) {
        app_print("Unable to send Model Subscription Get (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Model Subscription Get failed with status 0x%02x\r\n", status);
    } else {
        app_print("Model Subscriptions for Element 0x%04x, Model 0x%04x %s:\r\n", elem_addr, mod_id,
                  "(SIG)");

        if (!cnt) {
            app_print("\tNone.\r\n");
        }

        for (i = 0; i < cnt; i++) {
            app_print("\t0x%04x\r\n", subs[i]);
        }
    }
}


void cmd_ble_mesh_cfg_mod_sub_get_vnd(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, mod_id, cid;
    uint16_t subs[16];
    uint8_t status;
    size_t cnt;
    int err = 0;
    int i;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    cnt = ARRAY_SIZE(subs);
    elem_addr = strtoul(argv[3], NULL, 0);
    mod_id = strtoul(argv[4], NULL, 0);
    cid = strtoul(argv[5], NULL, 0);

    err = bt_mesh_cfg_cli_mod_sub_get_vnd(net_idx, dst, elem_addr, mod_id, cid, &status, subs, &cnt);

    if (err) {
        app_print("Unable to send Model Subscription Get (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Model Subscription Get failed with status 0x%02x\r\n", status);
    } else {
        app_print("Model Subscriptions for Element 0x%04x, Model 0x%04x 0x%x:\r\n", elem_addr, mod_id, cid);

        if (!cnt) {
            app_print("\tNone.\r\n");
        }

        for (i = 0; i < cnt; i++) {
            app_print("\t0x%04x\r\n", subs[i]);
        }
    }
}


void cmd_ble_mesh_cfg_net_key_add(int argc, char **argv)
{
    uint8_t key_val[16];
    uint16_t key_net_idx;
    uint16_t net_idx;
    uint16_t dst;
    size_t len;

    if (argc < 5) {
        goto usage;
    }

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    key_net_idx = strtoul(argv[3], NULL, 0);

    len = hex2bin(argv[4], strlen(argv[4]), key_val, sizeof(key_val));
    (void)memset(key_val + len, 0, sizeof(key_val) - len);

    app_mesh_cfg_net_key_add(net_idx, dst, key_net_idx, key_val);

    return;

usage:
    app_print("Usage: ble_mesh_net_key_add <addr> <net_idx> <key_net_idx> <key>\r\n");

}

void cmd_ble_mesh_cfg_net_key_update(int argc, char **argv)
{
    uint8_t key_val[16];
    uint16_t key_net_idx;
    uint8_t status;
    uint16_t net_idx;
    uint16_t dst;
    size_t len;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    key_net_idx = strtoul(argv[3], NULL, 0);

    len = hex2bin(argv[4], strlen(argv[4]), key_val, sizeof(key_val));
    (void)memset(key_val + len, 0, sizeof(key_val) - len);


    err = bt_mesh_cfg_cli_net_key_update(net_idx, dst, key_net_idx, key_val, &status);
    if (err) {
        app_print("Unable to send NetKey Update (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("NetKeyUpdate failed with status 0x%02x\r\n", status);
    } else {
        app_print("NetKey updated with NetKey Index 0x%03x\r\n", key_net_idx);
    }
}

void cmd_ble_mesh_cfg_net_key_get(int argc, char **argv)
{
    uint16_t keys[16];
    size_t cnt;
    uint16_t net_idx;
    uint16_t dst;
    int err, i;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    cnt = ARRAY_SIZE(keys);

    err = bt_mesh_cfg_cli_net_key_get(net_idx, dst, keys, &cnt);
    if (err) {
        app_print("Unable to send NetKeyGet (err %d)\r\n", err);
        return;
    }

    app_print("NetKeys known by 0x%04x:\r\n", dst);
    for (i = 0; i < cnt; i++) {
        app_print("\t0x%03x\r\n", keys[i]);
    }
}

void cmd_ble_mesh_cfg_net_key_del(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t key_net_idx;
    uint8_t status;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    key_net_idx = strtoul(argv[3], NULL, 0);

    err = bt_mesh_cfg_cli_net_key_del(net_idx, dst, key_net_idx, &status);
    if (err) {
        app_print("Unable to send NetKeyDel (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("NetKeyDel failed with status 0x%02x\r\n", status);
    } else {
        app_print("NetKey 0x%03x deleted\r\n", key_net_idx);
    }
}

void cmd_ble_mesh_cfg_app_key_add(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint8_t key_val[16];
    uint16_t key_net_idx, key_app_idx;
    size_t len;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    key_net_idx = strtoul(argv[3], NULL, 0);
    key_app_idx = strtoul(argv[4], NULL, 0);

    len = hex2bin(argv[5], strlen(argv[5]), key_val, sizeof(key_val));
    (void)memset(key_val + len, 0, sizeof(key_val) - len);

    app_mesh_cfg_app_key_add(net_idx, dst, key_net_idx, key_app_idx, key_val);
}

void cmd_ble_mesh_cfg_app_key_upd(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint8_t key_val[16];
    uint16_t key_net_idx, key_app_idx;
    size_t len;
    uint8_t status;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    key_net_idx = strtoul(argv[3], NULL, 0);
    key_app_idx = strtoul(argv[4], NULL, 0);

    len = hex2bin(argv[5], strlen(argv[5]), key_val, sizeof(key_val));
    (void)memset(key_val + len, 0, sizeof(key_val) - len);


    err = bt_mesh_cfg_cli_app_key_update(net_idx, dst, key_net_idx, key_app_idx, key_val, &status);

    if (err) {
        app_print("Unable to send App Key Update (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("AppKey update failed with status 0x%02x\r\n", status);
    } else {
        app_print("AppKey updated, NetKeyIndex 0x%04x AppKeyIndex 0x%04x\r\n",
                  key_net_idx, key_app_idx);
    }
}

void cmd_ble_mesh_cfg_app_key_get(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t key_net_idx;
    uint16_t keys[16];
    size_t cnt;
    uint8_t status;
    int err = 0;
    int i;

    cnt = ARRAY_SIZE(keys);
    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);
    key_net_idx = (uint16_t)strtoul((const char *)argv[3], NULL, 0);


    err = bt_mesh_cfg_cli_app_key_get(net_idx, dst, key_net_idx, &status, keys, &cnt);
    if (err) {
        app_print("Unable to send AppKeyGet (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("AppKeyGet failed with status 0x%02x\r\n", status);
    } else {
        app_print("AppKeys for NetKey 0x%03x known by 0x%04x:\r\n", key_net_idx, dst);
        for (i = 0; i < cnt; i++) {
            app_print("\t0x%03x\r\n", keys[i]);
        }
    }
}

void cmd_ble_mesh_cfg_app_key_del(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t key_net_idx, key_app_idx;
    uint8_t status;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    key_net_idx = strtoul(argv[3], NULL, 0);
    key_app_idx = strtoul(argv[4], NULL, 0);


    err = bt_mesh_cfg_cli_app_key_del(net_idx, dst, key_net_idx, key_app_idx, &status);
    if (err) {
        app_print("Unable to send App Key del(err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("AppKeyDel failed with status 0x%02x\r\n", status);
    } else {
        app_print("AppKey deleted, NetKeyIndex 0x%04x AppKeyIndex 0x%04x\r\n",
                  key_net_idx, key_app_idx);
    }
}

void cmd_ble_mesh_cfg_mod_app_bind(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, mod_app_idx, mod_id;
    uint8_t status;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    elem_addr = strtoul(argv[3], NULL, 0);
    mod_app_idx = strtoul(argv[4], NULL, 0);
    mod_id = strtoul(argv[5], NULL, 0);

    err = bt_mesh_cfg_cli_mod_app_bind(net_idx, dst, elem_addr, mod_app_idx, mod_id, &status);

    if (err) {
        app_print("Unable to send Model App Bind (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Model App Bind failed with status 0x%02x\r\n", status);
    } else {
        app_print("AppKey successfully bound\r\n");
    }
}


void cmd_ble_mesh_cfg_mod_app_bind_vnd(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, mod_app_idx, mod_id, cid;
    uint8_t status;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    elem_addr = strtoul(argv[3], NULL, 0);
    mod_app_idx = strtoul(argv[4], NULL, 0);
    mod_id = strtoul(argv[5], NULL, 0);
    cid = strtoul(argv[6], NULL, 0);

    err = bt_mesh_cfg_cli_mod_app_bind_vnd(net_idx, dst, elem_addr, mod_app_idx, mod_id, cid, &status);

    if (err) {
        app_print("Unable to send Model App Bind (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Model App Bind failed with status 0x%02x\r\n", status);
    } else {
        app_print("AppKey successfully bound\r\n");
    }
}


void cmd_ble_mesh_cfg_mod_app_unbind(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, mod_app_idx, mod_id;
    uint8_t status;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    elem_addr = strtoul(argv[3], NULL, 0);
    mod_app_idx = strtoul(argv[4], NULL, 0);
    mod_id = strtoul(argv[5], NULL, 0);

    err = bt_mesh_cfg_cli_mod_app_unbind(net_idx, dst, elem_addr, mod_app_idx, mod_id, &status);

    if (err) {
        app_print("Unable to send Model App Unbind (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Model App Unbind failed with status 0x%02x\r\n", status);
    } else {
        app_print("AppKey successfully unbound\r\n");
    }
}

void cmd_ble_mesh_cfg_mod_app_unbind_vnd(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, mod_app_idx, mod_id, cid;
    uint8_t status;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    elem_addr = strtoul(argv[3], NULL, 0);
    mod_app_idx = strtoul(argv[4], NULL, 0);
    mod_id = strtoul(argv[5], NULL, 0);
    cid = strtoul(argv[6], NULL, 0);

    err = bt_mesh_cfg_cli_mod_app_unbind_vnd(net_idx, dst, elem_addr, mod_app_idx, mod_id, cid,
                                             &status);

    if (err) {
        app_print("Unable to send Model App Unbind (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Model App Unbind failed with status 0x%02x\r\n", status);
    } else {
        app_print("AppKey successfully unbound\r\n");
    }
}

void cmd_ble_mesh_cfg_mod_app_get(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, mod_id;
    uint16_t apps[16];
    uint8_t status;
    size_t cnt;
    int err = 0;
    int i;

    cnt = ARRAY_SIZE(apps);
    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);
    elem_addr = strtoul(argv[3], NULL, 0);
    mod_id = strtoul(argv[4], NULL, 0);

    err = bt_mesh_cfg_cli_mod_app_get(net_idx, dst, elem_addr, mod_id, &status, apps, &cnt);

    if (err) {
        app_print("Unable to send Model App Get (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Model App Get failed with status 0x%02x\r\n", status);
    } else {
        app_print("Apps bound to Element 0x%04x, Model 0x%04x %s:\r\n", elem_addr, mod_id, "(SIG)");

        if (!cnt) {
            app_print("\tNone.\r\n");
        }

        for (i = 0; i < cnt; i++) {
            app_print("\t0x%04x\r\n", apps[i]);
        }
    }
}

void cmd_ble_mesh_cfg_mod_app_get_vnd(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    uint16_t elem_addr, mod_id, cid;
    uint16_t apps[16];
    uint8_t status;
    size_t cnt;
    int err = 0;
    int i;

    cnt = ARRAY_SIZE(apps);
    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);
    elem_addr = strtoul(argv[3], NULL, 0);
    mod_id = strtoul(argv[4], NULL, 0);

    cid = strtoul(argv[5], NULL, 0);

    err = bt_mesh_cfg_cli_mod_app_get_vnd(net_idx, dst, elem_addr, mod_id, cid, &status, apps, &cnt);

    if (err) {
        app_print("Unable to send Model App Get (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Model App Get failed with status 0x%02x\r\n", status);
    } else {
        app_print("Apps bound to Element 0x%04x, Model 0x%04x 0x%04x:\r\n", elem_addr, mod_id, cid);

        if (!cnt) {
            app_print("\tNone.\r\n");
        }

        for (i = 0; i < cnt; i++) {
            app_print("\t0x%04x\r\n", apps[i]);
        }
    }
}


void cmd_ble_mesh_cfg_hb_pub_get(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    app_mesh_cfg_hb_pub_get(net_idx, dst);
}

void cmd_ble_mesh_cfg_hb_pub_set(int argc, char **argv)
{
    struct bt_mesh_cfg_cli_hb_pub pub;
    uint16_t net_idx;
    uint16_t dst;

    if (argc < 9) {
        goto usage;
    }

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    pub.dst = strtoul(argv[3], NULL, 0);
    pub.count = strtoul(argv[4], NULL, 0);
    pub.period = strtoul(argv[5], NULL, 0);
    pub.ttl = strtoul(argv[6], NULL, 0);
    pub.feat = strtoul(argv[7], NULL, 0);
    pub.net_idx = strtoul(argv[8], NULL, 0);

    app_mesh_cfg_hb_pub_set(net_idx, dst, &pub);

    return;
usage:
    app_print("Usage: ble_mesh_hb_pub_set <dst> <net_idx> <pub_dst> <count> <period> <ttl> <feat> <pub net_idx>\r\n");
}


void cmd_ble_mesh_cfg_hb_sub_get(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    app_mesh_cfg_hb_sub_get(net_idx, dst);
}

void cmd_ble_mesh_cfg_hb_sub_set(int argc, char **argv)
{
    struct bt_mesh_cfg_cli_hb_sub sub;
    uint16_t net_idx;
    uint16_t dst;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    sub.src = strtoul(argv[3], NULL, 0);
    sub.dst = strtoul(argv[4], NULL, 0);
    sub.period = strtoul(argv[5], NULL, 0);

    app_mesh_cfg_hb_sub_set(net_idx, dst, &sub);
}

void cmd_ble_mesh_cfg_pollto_get(int argc, char **argv)
{
    uint16_t lpn_address;
    int32_t poll_timeout;
    uint16_t net_idx;
    uint16_t dst;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    lpn_address = strtoul(argv[3], NULL, 0);

    err = bt_mesh_cfg_cli_lpn_timeout_get(net_idx, dst, lpn_address, &poll_timeout);
    if (err) {
        app_print("Unable to send LPN PollTimeout Get (err %d)\r\n", err);
        return;
    }

    app_print("PollTimeout value %d\r\n", poll_timeout);
}


void cmd_ble_mesh_cfg_net_transmit(int argc, char **argv)
{
    uint8_t transmit;
    uint8_t count, interval, new_transmit;
    uint16_t net_idx;
    uint16_t dst;
    int err = 0;

    dst = (uint16_t)strtoul((const char *)argv[2], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[3], NULL, 0);

    if (!strcmp("get", argv[1])) {
        err = bt_mesh_cfg_cli_net_transmit_get(net_idx, dst, &transmit);
    } else if (!strcmp("set", argv[1])) {
        count = strtoul(argv[1], NULL, 0);
        interval = strtoul(argv[2], NULL, 0);
        new_transmit = BT_MESH_TRANSMIT(count, interval);

        err = bt_mesh_cfg_cli_net_transmit_set(net_idx, dst, new_transmit, &transmit);
    }

    if (err) {
        app_print("Unable to send network transmit Get/Set (err %d)\r\n", err);
        return;
    }

    app_print("Transmit 0x%02x (count %u interval %ums)\r\n", transmit,
              BT_MESH_TRANSMIT_COUNT(transmit), BT_MESH_TRANSMIT_INT(transmit));
}

void cmd_ble_mesh_cfg_node_reset(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t dst;
    int err;
    bool reset = false;

    dst = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    err = bt_mesh_cfg_cli_node_reset(net_idx, dst, &reset);
    if (err) {
        app_print("Unable to send Remote Node Reset (err %d)\r\n", err);
        return;
    }

    if (IS_ENABLED(CONFIG_BT_MESH_CDB)) {
        struct bt_mesh_cdb_node *node = bt_mesh_cdb_node_get(dst);

        if (node) {
            bt_mesh_cdb_node_del(node, true);
        }
    }

    app_print("Remote node reset complete\r\n");
}

