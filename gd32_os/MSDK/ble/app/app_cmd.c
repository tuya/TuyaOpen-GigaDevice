/*!
    \file    app_cmd.c
    \brief   Implementation of BLE related CLI commands.

    \version 2023-07-20, V1.0.0, firmware for GD32VW55x
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
#include "ble_app_config.h"

#if (BLE_APP_CMD_SUPPORT)
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ble_export.h"
#include "ble_adapter.h"
#include "ble_sample_srv.h"
#include "ble_sample_cli.h"

#include "app_adapter_mgr.h"
#include "app_sec_mgr.h"
#include "app_dev_mgr.h"
#include "app_scan_mgr.h"
#include "app_adv_mgr.h"
#include "app_per_sync_mgr.h"
#include "app_blue_courier_link.h"
#include "app_dfu_cli.h"
#include "app_datatrans_srv.h"

#include "cmd_shell.h"

#ifdef CONFIG_INTERNAL_DEBUG
#include "app_cmd_int.c"
#endif

#if (defined(CONFIG_INTERNAL_DEBUG) || defined(CONFIG_RF_TEST_SUPPORT) || defined(CONFIG_BLE_DTM_SUPPORT))
#include "app_cmd_rftest.h"
#endif

static void cmd_ble_help(int argc, char **argv);

static void cmd_ble_enable(int argc, char **argv)
{
    app_ble_enable();
}

static void cmd_ble_disable(int argc, char **argv)
{
    app_ble_disable();
}

static void cmd_ble_ps(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t mode;

    if (argc != 2) {
        goto usage;
    }

    mode = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
    if (mode > 1) {
        goto usage;
    }

    ble_sleep_mode_set(mode);
    app_print("ble_ps config complete. ps mode: %d\r\n", ble_sleep_mode_get());

    return;

usage:
    app_print("Current ps mode: %d\r\n", ble_sleep_mode_get());
    app_print("Usage: ble_ps <0, 1>\r\n");
    app_print("    0: ble not deep sleep\r\n");
    app_print("    1: ble deep sleep and support external wake-up\r\n");
}

static void cmd_addr_set(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t addr[6] = {0};
    uint8_t i = 0;

    if (argc != 7) {
        goto usage;
    }

    for (i = 0; i < 6; i++) {
        addr[i] = (uint8_t)strtoul((const char *)argv[1 + i], &endptr, 16);
    }

    if (ble_adp_public_addr_set(addr))
        app_print("ble addr set fail\r\n");
    else
        app_print("ble addr set success, please reboot to make it take effect\r\n");
    return;

usage:
    app_print("Usage: ble_addr_set <byte0> <byte1> <byte2> <byte3> <byte4> <byte5>\r\n");
    app_print("Example: ble_addr_set aa bb cc 11 22 33\r\n");

}

static void cmd_ble_courier_wifi(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t enable;
    ble_status_t ret;

    if (argc != 2) {
        app_print("Usage: ble_courier_wifi <0:disable; 1:enable>\r\n");
        return;
    }

    enable = (uint8_t)strtoul((const char *)argv[1], &endptr, 16);

    if (enable) {
        if (ble_work_status_get() != BLE_WORK_STATUS_ENABLE) {
            app_ble_enable();
        }
        ret = bcw_prf_enable(enable);
    } else {
        ret = bcw_prf_enable(enable);

        #ifndef CONFIG_BLE_ALWAYS_ENABLE
        app_ble_disable();
        #endif
    }

    app_print("ble_courier_wifi ret:%u\r\n", ret);
}

#if (BLE_CFG_ROLE & (BLE_CFG_ROLE_BROADCASTER | BLE_CFG_ROLE_PERIPHERAL))
#ifndef CONFIG_INTERNAL_DEBUG
static void cmd_advertise(int argc, char **argv)
{
    char *endptr = NULL;
    app_adv_param_t adv_param;
    ble_status_t ret;
    uint8_t set_num = 2;

    ble_adp_adv_sets_num_get(&set_num);

    if (argc != 2) {
        goto usage;
    }

    adv_param.type = (uint8_t)strtoul((const char *)argv[1], &endptr, 16);

    if (adv_param.type > 2) {
        goto usage;
    }

    if (adv_param.type == BLE_ADV_TYPE_LEGACY) {
        adv_param.prop = BLE_GAP_ADV_PROP_UNDIR_CONN;           // 0x0003,scannable connectable undirected
    } else if (adv_param.type == BLE_ADV_TYPE_EXTENDED) {
        adv_param.prop = BLE_GAP_EXT_ADV_PROP_CONN_UNDIRECT;    // 0x0001,connectable undirected
    } else {
        adv_param.prop = BLE_GAP_PER_ADV_PROP_UNDIRECT;         // 0x0000, undirected periodic adv
    }

    adv_param.adv_intv = APP_ADV_INT_MAX;
    adv_param.ch_map = BLE_GAP_ADV_CHANN_37 | BLE_GAP_ADV_CHANN_38 | BLE_GAP_ADV_CHANN_39;
    adv_param.max_data_len = 0x1F;
    adv_param.pri_phy = BLE_GAP_PHY_1MBPS;
    adv_param.sec_phy = BLE_GAP_PHY_1MBPS;
    adv_param.wl_enable = false;
    adv_param.own_addr_type = BLE_GAP_LOCAL_ADDR_STATIC;
    adv_param.disc_mode = BLE_GAP_ADV_MODE_GEN_DISC;

    ret = app_adv_create(&adv_param);

    if (ret != BLE_ERR_NO_ERROR) {
        app_print("create adv fail status 0x%x\r\n", ret);
    }

    return;

usage:
    app_print("Usage: ble_adv <adv type>\r\n");
    app_print("<adv type>: advertising type, value 0 ~ 2\r\n");
    app_print("\t0: legacy advertising, 1: extended advertising, 2: periodic advertising\r\n");
    app_print("\tsupport %u advertising sets at the same time\r\n", set_num);
}
#endif // #ifndef CONFIG_INTERNAL_DEBUG

static void cmd_advertise_stop(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t idx = 0;
    bool rmv_adv = true;
    ble_status_t ret;

    if (argc == 1 || argc > 3) {
        goto usage;
    }

    idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 16);

    if (argc == 3) {
        rmv_adv = (bool)strtoul((const char *)argv[2], &endptr, 16);
    }

    ret = app_adv_stop(idx, rmv_adv);
    if (ret != BLE_ERR_NO_ERROR) {
        app_print("stop adv fail status 0x%x\r\n", ret);
    }

    return;

usage:
    app_print("Usage: ble_adv_stop <adv idx> [remove]\r\n");
    app_print("<adv idx>: advertising index to stop\r\n");
    app_print("[remove]: remove advertising set after stopped, default is 1 if not set\r\n");
}

static void cmd_advertise_restart(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t idx = 0;
    ble_status_t ret;

    if (argc == 1) {
        goto usage;
    }

    idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 16);

    ret = app_adv_restart(idx);
    if (ret != BLE_ERR_NO_ERROR) {
        app_print("restart adv fail 0x%x\r\n", ret);
    }

    return;

usage:
    app_print("Usage: ble_adv_restart <adv idx>\r\n");
    app_print("<adv idx>: advertising index to restart\r\n");
}
#endif

#if (BLE_CFG_ROLE & (BLE_CFG_ROLE_OBSERVER | BLE_CFG_ROLE_CENTRAL))
#ifndef CONFIG_INTERNAL_DEBUG
static void cmd_scan(int argc, char **argv)
{
    app_scan_enable(false);
}
#endif // #ifndef CONFIG_INTERNAL_DEBUG

static void cmd_scan_stop(int argc, char **argv)
{
    app_scan_disable();
}

static void cmd_list_scan_devs(int argc, char **argv)
{
    scan_mgr_list_scanned_devices();
}

#if (BLE_APP_PER_ADV_SUPPORT)
#ifndef CONFIG_INTERNAL_DEBUG
static void cmd_sync(int argc, char **argv)
{
    char *endptr = NULL;
    dev_info_t *p_dev_info = NULL;
    ble_gap_per_sync_param_t param;
    uint8_t idx;
    ble_status_t ret;

    param.skip = 0;
    param.sync_tout = 1000;     // 10s
    param.type = BLE_GAP_PER_SYNC_TYPE_GENERAL;
    param.conn_idx = 0;
    param.report_en_bf = BLE_GAP_REPORT_ADV_EN_BIT | BLE_GAP_REPORT_DUPLICATE_FILTER_EN_BIT;
    param.adv_addr.adv_sid = 0;

    if (argc != 2) {
        goto usage;
    }

    idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
    p_dev_info = scan_mgr_find_dev_by_idx(idx);
    if (p_dev_info == NULL) {
        app_print("fail to find periodic advertising device\r\n");
        return;
    }

    param.adv_addr.addr_type = p_dev_info->peer_addr.addr_type;
    memcpy(param.adv_addr.addr, p_dev_info->peer_addr.addr, BLE_GAP_ADDR_LEN);
    param.adv_addr.adv_sid = p_dev_info->adv_sid;

    ret = ble_per_sync_start(BLE_GAP_LOCAL_ADDR_STATIC, &param);
    if (ret != BLE_ERR_NO_ERROR) {
        app_print("sync fail status 0x%x\r\n", ret);
    }

    return;

usage:
    app_print("Usage: ble_sync <dev idx>\r\n");
    app_print("<dev idx>: device index in scan list\r\n");
}
#endif  // #ifndef CONFIG_INTERNAL_DEBUG

static void cmd_sync_cancel(int argc, char **argv)
{
    app_per_sync_cancel();
}

static void cmd_sync_terminate(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t sync_actv_idx;

    if (argc != 2) {
        goto usage;
    }

    sync_actv_idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
    app_per_sync_terminate(sync_actv_idx);

    return;

usage:
    app_print("Usage: ble_sync_terminate <sync idx>\r\n");
    app_print("<sync idx>: periodic advertising sync index\r\n");
}

static void cmd_sync_ctrl(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t sync_actv_idx;
    uint8_t ctrl;
    ble_status_t ret;

    if (argc != 3) {
        goto usage;
    }

    sync_actv_idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
    ctrl = (uint8_t)strtoul((const char *)argv[2], &endptr, 16);

    ret = ble_per_sync_report_ctrl(sync_actv_idx, ctrl);
    if (ret != BLE_ERR_NO_ERROR) {
        app_print("ble sync ctrl fail status 0x%x \r\n", ret);
    }

    return;

usage:
    app_print("Usage: ble_sync_ctrl <sync idx> <report>\r\n");
    app_print("<sync idx>: periodic advertising sync index\r\n");
    app_print("<report>: control bitfield for periodic advertising report\r\n");
    app_print("\tbit 0: report periodic advertising event\r\n");
}
#endif // (BLE_APP_PER_ADV_SUPPORT)
#endif // (BLE_CFG_ROLE & (BLE_CFG_ROLE_OBSERVER | BLE_CFG_ROLE_CENTRAL))

#if (BLE_CFG_ROLE & (BLE_CFG_ROLE_PERIPHERAL | BLE_CFG_ROLE_CENTRAL))
#if (BLE_CFG_ROLE & BLE_CFG_ROLE_CENTRAL)
#ifndef CONFIG_INTERNAL_DEBUG
static void cmd_connect(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t idx = 0xFF;
    dev_info_t *p_dev_info = NULL;
    ble_status_t ret;

    if (argc != 2) {
        goto usage;
    }

    idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
    p_dev_info = scan_mgr_find_dev_by_idx(idx);
    if (p_dev_info == NULL) {
        app_print("fail to find device\r\n");
        return;
    }

    ret = ble_conn_connect(NULL, BLE_GAP_LOCAL_ADDR_STATIC, &p_dev_info->peer_addr, false);
    if ( ret != BLE_ERR_NO_ERROR) {
        app_print("connect fail status 0x%x\r\n", ret);
    }

    return;

usage:
    app_print("Usage: ble_conn <dev idx>\r\n");
    app_print("<dev idx>: dev index in scan list\r\n");
}

static void cmd_connect_by_addr(int argc, char **argv)
{
    char *endptr = NULL;
    ble_status_t ret;
    ble_gap_addr_t peer_addr;
    char *p_str;

    if (argc != 3) {
        goto usage;
    }

    peer_addr.addr_type = (uint8_t)strtoul((const char *)argv[1], &endptr, 16);

    p_str = argv[2];
    peer_addr.addr[5] = (uint8_t)strtoul((const char *)strtok(p_str, ":"), &endptr, 16);
    peer_addr.addr[4] = (uint8_t)strtoul((const char *)strtok(NULL, ":"), &endptr, 16);
    peer_addr.addr[3] = (uint8_t)strtoul((const char *)strtok(NULL, ":"), &endptr, 16);
    peer_addr.addr[2] = (uint8_t)strtoul((const char *)strtok(NULL, ":"), &endptr, 16);
    peer_addr.addr[1] = (uint8_t)strtoul((const char *)strtok(NULL, ":"), &endptr, 16);
    peer_addr.addr[0] = (uint8_t)strtoul((const char *)strtok(NULL, ":"), &endptr, 16);

    ret = ble_conn_connect(NULL, BLE_GAP_LOCAL_ADDR_STATIC, &peer_addr, false);
    if ( ret != BLE_ERR_NO_ERROR) {
        app_print("connect fail status 0x%x\r\n", ret);
    }

    app_print("set peer addr to 0x%02x:%02x:%02x:%02x:%02x:%02x\r\n", peer_addr.addr[5],
               peer_addr.addr[4], peer_addr.addr[3], peer_addr.addr[2], peer_addr.addr[1], peer_addr.addr[0]);

    return;

usage:
    app_print("Usage: ble_conn_by_addr <addr type> <addr>\r\n");
    app_print("<addr type>: address type\r\n");
    app_print("<addr>: bd address\r\n");
}

#endif // #ifndef CONFIG_INTERNAL_DEBUG

static void cmd_cancel_connect(int argc, char **argv)
{
    ble_status_t ret;

    ret = ble_conn_connect_cancel();

    if (ret != BLE_ERR_NO_ERROR) {
        app_print("cancel connect fail status 0x%x\r\n", ret);
    }
}
#endif

static void cmd_disconnect(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t idx = 0;
    ble_status_t ret;

    if (argc != 2) {
        goto usage;
    }

    idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);

    ret = ble_conn_disconnect(idx, BLE_ERROR_HL_TO_HCI(BLE_LL_ERR_REMOTE_USER_TERM_CON));
    if (ret != BLE_ERR_NO_ERROR) {
        app_print("disconnect connection fail status 0x%x\r\n", ret);
    }

    return;

usage:
    app_print("Usage: ble_disconn <conn idx>\r\n");
    app_print("<conn idx>: index of connection to disconnect\r\n");
}

static void cmd_remove_bond(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t idx = 0xFF;
    ble_device_t *p_dev = NULL;

    if (argc != 2) {
        goto usage;
    }

    idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
    p_dev = dm_find_dev_by_idx(idx);
    if (p_dev == NULL) {
        app_print("fail to find device\r\n");
        return;
    }

    if (!app_sec_remove_bond(p_dev->cur_addr)) {
        app_print("remove bond fail\r\n");
        return;
    }

    app_print("remove bond success\r\n");
    return;

usage:
    app_print("Usage: ble_remove_bond <dev idx>\r\n");
    app_print("<dev idx>: device index in bond list which can be get by ble_list_sec_devs command\r\n");
}

static void cmd_list_sec_devs(int argc, char **argv)
{
    dm_list_sec_devices(dm_list_sec_devices_cb);
}

#ifndef CONFIG_INTERNAL_DEBUG
static void cmd_set_auth(int argc, char **argv)
{
    char *endptr = NULL;
    bool bond;
    bool mitm;
    bool sc;
    uint8_t iocap;

    if (argc != 5) {
        goto usage;
    }

    bond  = (bool)strtoul((const char *)argv[1], &endptr, 0);
    mitm  = (bool)strtoul((const char *)argv[2], &endptr, 0);
    sc    = (bool)strtoul((const char *)argv[3], &endptr, 0);
    iocap = (uint8_t)strtoul((const char *)argv[4], &endptr, 0);

    if (iocap > 4) {
        goto usage;
    }

    app_sec_set_authen(bond, mitm, sc, iocap, false, false, 16);
    app_print("ble set auth success.\r\n");
    return;

usage:
    app_print("Usage: ble_set_auth <bond> <mitm> <sc> <iocap>\r\n");
    app_print("<bond>: bonding flag for authentication\r\n");
    app_print("\t0x00: no bonding\r\n");
    app_print("\t0x01: bonding\r\n");
    app_print("<mitm>: mitm flag for authentication\r\n");
    app_print("\t0x00: mitm protection not required\r\n");
    app_print("\t0x01: mitm protection required\r\n");
    app_print("<sc>: secure connections flag for authention\r\n");
    app_print("\t0x00: secure connections pairing is not supported\r\n");
    app_print("\t0x01: secure connections pairing is supported\r\n");
    app_print("<iocap>: io capability to set\r\n");
    app_print("\t0x00: display only\r\n");
    app_print("\t0x01: display yes no\r\n");
    app_print("\t0x02: keyboard only\r\n");
    app_print("\t0x03: no input no output\r\n");
    app_print("\t0x04: keyboard display\r\n");
}
#endif // #ifndef CONFIG_INTERNAL_DEBUG

static void cmd_pair(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t conidx = 0;
    ble_device_t *p_device;

    if (argc != 2) {
        goto usage;
    }

    conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 0);
    p_device = dm_find_dev_by_conidx(conidx);
    if (p_device == NULL) {
        app_print("fail to find device\r\n");
        return;
    }

    if (p_device->role == BLE_MASTER) {
        app_sec_send_bond_req(conidx);
    } else {
        app_sec_send_security_req(conidx);
    }

    return;

usage:
    app_print("Usage: ble_pair <conn idx>\r\n");
    app_print("<conn idx>: index of the connection to pair\r\n");
}

static void cmd_encrypt(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t conidx = 0;

    if (argc != 2) {
        goto usage;
    }

    conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 0);
    app_sec_send_encrypt_req(conidx);

    return;

usage:
    app_print("Usage: ble_encrypt <conn idx>\r\n");
    app_print("<conn idx>: index of the connection to start encryption\r\n");
}

static void cmd_passkey(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t conidx = 0;
    uint32_t passkey;

    if (argc != 3) {
        goto usage;
    }

    conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 0);
    passkey = (uint32_t)strtoul((const char *)argv[2], &endptr, 0);

    if (passkey > 999999) {
        goto usage;
    }

    app_sec_input_passkey(conidx, passkey);

    return;

usage:
    app_print("Usage: ble_passkey <conn idx> <passkey>\r\n");
    app_print("<conn idx>: index of connection to input passkey\r\n");
    app_print("<passkey>: passkey value to input, should be 6-digit value between 000000 and 999999\r\n");
}

static void cmd_compare(int argc, char **argv)
{
    char *endptr = NULL;
    bool value;
    uint8_t conidx = 0;

    if (argc != 3) {
        goto usage;
    }

    conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 0);
    value = (bool)strtoul((const char *)argv[2], &endptr, 0);
    app_sec_num_compare(conidx, value);

    return;

usage:
    app_print("Usage: ble_compare <conn idx> <result>\r\n");
    app_print("<conn idx> index of connection\r\n");
    app_print("<result>: numeric comparison result, 0 for fail and 1 for success\r\n");
}

static void cmd_ble_peer_feat(int argc, char **argv)
{
    uint8_t idx = 0;
    char *endptr = NULL;
    ble_status_t ret;

    if (argc != 2) {
        goto usage;
    }

    idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 0);
    ret = ble_conn_peer_feats_get(idx);
    if (ret != BLE_ERR_NO_ERROR) {
        app_print("get peer features fail status 0x%x\r\n", ret);
    }

    return;

usage:
    app_print("Usage: ble_peer_feat <conn idx>\r\n");
    app_print("<conn idx>: index of connection\r\n");
}

static void cmd_ble_peer_ver(int argc, char **argv)
{
    uint8_t idx = 0;
    char *endptr = NULL;
    ble_status_t ret;

    if (argc != 2) {
        goto usage;
    }

    idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 0);
    ret = ble_conn_peer_version_get(idx);
    if (ret != BLE_ERR_NO_ERROR) {
        app_print("get peer version fail status 0x%x\r\n", ret);
    }

    return;

usage:
    app_print("Usage: ble_peer_ver <conn idx>\r\n");
    app_print("<conn idx>: index of connection\r\n");
}

static void cmd_ble_param_update(int argc, char **argv)
{
    char *endptr = NULL;
    uint16_t interval;
    uint16_t latency;
    uint16_t supv_to;
    uint16_t ce_len;
    uint8_t idx;
    ble_status_t ret;

    if (argc != 6) {
        goto usage;
    }

    idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
    interval = (uint16_t)strtoul((const char *)argv[2], &endptr, 16);
    latency  = (uint16_t)strtoul((const char *)argv[3], &endptr, 16);
    supv_to  = (uint16_t)strtoul((const char *)argv[4], &endptr, 16);
    ce_len   = (uint16_t)strtoul((const char *)argv[5], &endptr, 16);

    ret = ble_conn_param_update_req(idx, interval, interval, latency, supv_to, ce_len, ce_len);
    if (ret != BLE_ERR_NO_ERROR) {
        app_print("update param fail status 0x%x\r\n", ret);
    }

    return;

usage:
    app_print("Usage: ble_param_update <conn idx> <interval> <latency> <supv tout> <ce len>\r\n");
    app_print("<conn idx>: index of connection\r\n");
    app_print("<interval>: connection interval in unit of 1.25ms, range from 0x0006 to 0x0C80 in hex value\r\n");
    app_print("<latency>: connection latency to update in hex value\r\n");
    app_print("<supv tout>: supervision timeout in unit of 10ms, range from 0x000A to 0x0C80 in hex value\r\n");
    app_print("<ce len>: connection event length in unit of 0.625 ms in hex value\r\n");
}

static void cmd_get_rssi(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t conidx;
    ble_status_t ret;

    if (argc != 2) {
        goto usage;
    }

    conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 0);
    ret = ble_conn_rssi_get(conidx);
    if (ret != BLE_ERR_NO_ERROR) {
        app_print("get rssi fail status 0x%x\r\n", ret);
    }

    return;

usage:
    app_print("Usage: ble_get_rssi <conn idx>\r\n");
    app_print("<conn idx>: index of connection\r\n");

}

static void cmd_set_dev_name(int argc, char **argv)
{
    //uint8_t i;
    if (argc != 2) {
        goto usage;
    }

    if (!app_adp_set_name(argv[1], strlen(argv[1]))) {
        app_print("set device name fail\r\n");
        return;
    }

    app_adv_data_update_all();
    app_print("set device name to %s\r\n", argv[1]);

    return;

usage:
    app_print("Usage: ble_set_dev_name <device name>\r\n");
    app_print("<device name>: ble device name\r\n");

}

static void cmd_get_dev_name(int argc, char **argv)
{
    uint8_t *p_dev_name = NULL;

    if (argc != 1) {
        goto usage;
    }

    app_adp_get_name(&p_dev_name);
    app_print("dev_name :%s\r\n",p_dev_name);

    return;

usage:
    app_print("Usage: ble_get_dev_name \r\n");
}

#if (BLE_APP_PHY_UPDATE_SUPPORT)
static void cmd_set_phy(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t tx_phy;
    uint8_t rx_phy;
    uint8_t phy_opt;
    uint8_t conidx;
    ble_status_t ret;

    if (argc != 5) {
        goto usage;
    }

    conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 0);
    tx_phy = (uint8_t)strtoul((const char *)argv[2], &endptr, 0);
    rx_phy = (uint8_t)strtoul((const char *)argv[3], &endptr, 0);
    phy_opt = (uint8_t)strtoul((const char *)argv[4], &endptr, 0);

    ret = ble_conn_phy_set(conidx, tx_phy, rx_phy, phy_opt);
    if (ret != BLE_ERR_NO_ERROR) {
        app_print("set phy fail status 0x%x\r\n", ret);
    }

    return;

usage:
    app_print("Usage: ble_set_phy <conn idx> <tx phy> <rx phy> <phy opt>\r\n");
    app_print("<conn idx>: index of connection\r\n");
    app_print("<tx phy>: transmit phy to set\r\n");
    app_print("\tbit 0: 1M phy, bit 1: 2M phy, bit 2: coded phy\r\n");
    app_print("<rx phy>: receive phy to set\r\n");
    app_print("\tbit 0: 1M phy, bit 1: 2M phy, bit 2: coded phy\r\n");
    app_print("<phy opt>: phy options for coded phy\r\n");
    app_print("\t0x00: no prefer coding\r\n");
    app_print("\t0x01: prefer S=2 coding be used\r\n");
    app_print("\t0x02: prefer S=8 coding be used\r\n");
}

static void cmd_get_phy(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t conidx;
    ble_status_t ret;

    if (argc != 2) {
        goto usage;
    }

    conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 0);

    ret = ble_conn_phy_get(conidx);
    if (ret != BLE_ERR_NO_ERROR) {
        app_print("get phy fail status 0x%x\r\n", ret);
    }

    return;

usage:
    app_print("Usage: ble_get_phy <conn idx>\r\n");
    app_print("<conn idx>: index of connection\r\n");
}
#endif // (BLE_APP_PHY_UPDATE_SUPPORT)

#if (BLE_APP_DATA_LEN_EXTEN_SUPPORT)
static void cmd_set_pkt_size(int argc, char **argv)
{
    char *endptr = NULL;
    uint16_t tx_oct;
    uint16_t tx_time;
    uint8_t conidx;
    ble_status_t ret;

    if (argc != 4) {
        goto usage;
    }

    conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 0);
    tx_oct = (uint16_t)strtoul((const char *)argv[2], &endptr, 10);
    tx_time = (uint16_t)strtoul((const char *)argv[3], &endptr, 10);

    ret = ble_conn_pkt_size_set(conidx, tx_oct, tx_time);
    if (ret != BLE_ERR_NO_ERROR) {
        app_print("set pkt size fail status 0x%x\r\n", ret);
    }

    return;

usage:
    app_print("Usage: ble_set_pkt_size <conn idx> <tx oct> <tx time>\r\n");
    app_print("<conn idx>: index of connection\r\n");
    app_print("<tx oct>: preferred maximum number of payload octets in a single data PDU, Range 27 to 251\r\n");
    app_print("<tx time>: preferred maximum number of microseconds used to transmit a single data PDU, Range 328 to 17040\r\n");
}
#endif // (BLE_APP_DATA_LEN_EXTEN_SUPPORT)
#endif // (BLE_CFG_ROLE & (BLE_CFG_ROLE_PERIPHERAL | BLE_CFG_ROLE_CENTRAL))

#if (BLE_PROFILE_SAMPLE_SERVER)
static void cmd_sample_srv_ntf(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t conn_idx = 0;
    uint16_t len = 0;
    uint16_t i = 0;
    uint8_t *p_data = NULL;

    if (argc != 3) {
        goto usage;
    }

    conn_idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
    len = (uint16_t)strtoul((const char *)argv[2], &endptr, 10);

    p_data = sys_malloc(len);
    for (i = 0; i < len; i++) {
        *(p_data + i) = i;
    }
    ble_sample_srv_ntf_send(conn_idx, len, p_data);
    sys_mfree(p_data);

    return;

usage:
    app_print("Usage: ble_sample_srv_ntf <conn idx> <len> \r\n");
    app_print("<conn idx>: index of connection\r\n");
    app_print("<len>: data length, Range 1 to mtu size\r\n");
}
#endif

#if (BLE_PROFILE_SAMPLE_CLIENT)
static void cmd_sample_cli_read_char(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t conidx = 0;

    if (argc != 2) {
        goto usage;
    }

    conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);

    ble_sample_cli_read_char(conidx);

    return;

usage:
    app_print("Usage: ble_sample_cli_read_char <conn idx> \r\n");
    app_print("<conn idx>: index of connection\r\n");
}

static void cmd_sample_cli_write_char(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t conidx = 0;
    uint16_t len = 0;
    uint16_t i = 0;
    uint8_t *p_data = NULL;

    if (argc != 3) {
        goto usage;
    }

    conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
    len = (uint16_t)strtoul((const char *)argv[2], &endptr, 10);

    p_data = sys_malloc(len);
    for (i = 0; i < len; i++) {
        *(p_data + i) = i;
    }
    ble_sample_cli_write_char(conidx, len, p_data);
    sys_mfree(p_data);

    return;

usage:
    app_print("Usage: ble_sample_cli_write_char <conn idx> <len> \r\n");
    app_print("<conn idx>: index of connection\r\n");
    app_print("<len>: data length, Range 1 to mtu size\r\n");
}

static void cmd_sample_cli_write_cccd(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t conidx = 0;

    if (argc != 2) {
        goto usage;
    }

    conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);

    ble_sample_cli_write_cccd(conidx);

    return;

usage:
    app_print("Usage: ble_sample_cli_write_cccd <conn idx> \r\n");
    app_print("<conn idx>: index of connection\r\n");
}
#endif //BLE_PROFILE_SAMPLE_CLIENT

void cmd_passth(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t conidx = 0;
    uint32_t baudrate = 0;

    if (argc != 3) {
        goto usage;
    }

    conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
    baudrate = (uint32_t)strtoul((const char *)argv[2], &endptr, 10);
    app_datatrans_start(conidx, baudrate);

    return;
usage:
    app_print("Usage: ble_passth <conn idx> \r\n");
    app_print("<conn idx>: index of connection\r\n");
    app_print("<baudrate>: uart baudrate\r\n");
}

#if (FEAT_SUPPORT_BLE_OTA)
static void cmd_ota_start(int argc, char **argv)
{
    char     *endptr = NULL;
    uint8_t  conidx = 0;
    uint32_t img_size = 0;

    if (argc != 3) {
        goto usage;
    }

    conidx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
    img_size = (uint32_t)strtoul((const char *)argv[2], &endptr, 10);

    app_ble_dfu_start(conidx, img_size);

    return;

usage:
    app_print("Usage: ble_ota_start <conidx> <img_size>\r\n");
    app_print("<conidx>: index of connection\r\n");
    app_print("<img_size>: size of ota image\r\n");
    app_print("for example:\r\n");
    app_print("    ble_ota_start 0 632916 \r\n");

}
#endif //(FEAT_SUPPORT_BLE_OTA)

const struct cmd_entry ble_cmd_table[] = {
    {"ble_help", cmd_ble_help},

#ifdef CONFIG_BASECMD
    {"ble_enable", cmd_ble_enable},
    {"ble_disable", cmd_ble_disable},
    {"ble_ps", cmd_ble_ps},
    {"ble_addr_set", cmd_addr_set},
#ifdef CFG_WLAN_SUPPORT
    {"ble_courier_wifi", cmd_ble_courier_wifi},
#endif

#if (BLE_CFG_ROLE & (BLE_CFG_ROLE_BROADCASTER | BLE_CFG_ROLE_PERIPHERAL))
#ifndef CONFIG_INTERNAL_DEBUG
    {"ble_adv",         cmd_advertise},
#endif // #ifndef CONFIG_INTERNAL_DEBUG
    {"ble_adv_stop",    cmd_advertise_stop},
    {"ble_adv_restart", cmd_advertise_restart},
#endif

#if (BLE_CFG_ROLE & (BLE_CFG_ROLE_OBSERVER | BLE_CFG_ROLE_CENTRAL))
#ifndef CONFIG_INTERNAL_DEBUG
    {"ble_scan", cmd_scan},
#endif // #ifndef CONFIG_INTERNAL_DEBUG
    {"ble_scan_stop", cmd_scan_stop},
    {"ble_list_scan_devs", cmd_list_scan_devs},

#if (BLE_APP_PER_ADV_SUPPORT)
#ifndef CONFIG_INTERNAL_DEBUG
    {"ble_sync", cmd_sync},
#endif // #ifndef CONFIG_INTERNAL_DEBUG
    {"ble_sync_cancel", cmd_sync_cancel},
    {"ble_sync_terminate", cmd_sync_terminate},
    {"ble_sync_ctrl", cmd_sync_ctrl},
#endif
#endif

#if (BLE_CFG_ROLE & (BLE_CFG_ROLE_PERIPHERAL | BLE_CFG_ROLE_CENTRAL))
#if (BLE_CFG_ROLE & BLE_CFG_ROLE_CENTRAL)
#ifndef CONFIG_INTERNAL_DEBUG
    {"ble_conn", cmd_connect},
    {"ble_conn_by_addr", cmd_connect_by_addr},
#endif // #ifndef CONFIG_INTERNAL_DEBUG
    {"ble_cancel_conn", cmd_cancel_connect},
#endif // (BLE_CFG_ROLE & BLE_CFG_ROLE_CENTRAL)

    {"ble_disconn", cmd_disconnect},
    {"ble_remove_bond", cmd_remove_bond},
    {"ble_list_sec_devs", cmd_list_sec_devs},

#ifndef CONFIG_INTERNAL_DEBUG
    {"ble_set_auth", cmd_set_auth},
#endif // #ifndef CONFIG_INTERNAL_DEBUG

    {"ble_pair", cmd_pair},
    {"ble_encrypt", cmd_encrypt},
    {"ble_passkey", cmd_passkey},
    {"ble_compare", cmd_compare},

    {"ble_peer_feat", cmd_ble_peer_feat},
    {"ble_peer_ver", cmd_ble_peer_ver},
    {"ble_param_update", cmd_ble_param_update},
    {"ble_get_rssi", cmd_get_rssi},
    {"ble_set_dev_name", cmd_set_dev_name},
    {"ble_get_dev_name", cmd_get_dev_name},

#if (BLE_APP_PHY_UPDATE_SUPPORT)
    {"ble_set_phy", cmd_set_phy},
    {"ble_get_phy", cmd_get_phy},
#endif

#if (BLE_APP_DATA_LEN_EXTEN_SUPPORT)
    {"ble_set_pkt_size", cmd_set_pkt_size},
#endif
#endif // (BLE_CFG_ROLE & (BLE_CFG_ROLE_PERIPHERAL | BLE_CFG_ROLE_CENTRAL))

#if (BLE_PROFILE_SAMPLE_SERVER)
    {"ble_sample_srv_ntf", cmd_sample_srv_ntf},
#endif

#if (BLE_PROFILE_SAMPLE_CLIENT)
    {"ble_sample_cli_read_char", cmd_sample_cli_read_char},
    {"ble_sample_cli_write_char", cmd_sample_cli_write_char},
    {"ble_sample_cli_write_cccd", cmd_sample_cli_write_cccd},
#endif
#if FEAT_SUPPORT_BLE_DATATRANS && (BLE_DATATRANS_MODE == PURE_DATA_TRANSMIT_MODE)
    {"ble_passth", cmd_passth},
#endif // FEAT_SUPPORT_BLE_DATATRANS || (BLE_DATATRANS_MODE == PURE_DATA_TRANSMIT_MODE)
#endif // CONFIG_BASECMD

#if (FEAT_SUPPORT_BLE_OTA & BLE_APP_GATT_CLIENT_SUPPORT)
    {"ble_ota_start", cmd_ota_start},
#endif //(FEAT_SUPPORT_BLE_OTA & BLE_APP_GATT_CLIENT_SUPPORT)

    {"", NULL}
};

const uint32_t ble_cmd_table_size = (sizeof(ble_cmd_table) / sizeof(ble_cmd_table[0]));

#if (!defined(CONFIG_RF_TEST_SUPPORT)) && defined(CONFIG_BASECMD)
void ble_base_cmd_help(void)
{
    int i;

    /* i is 1, not print 'ble_help' */
    for (i = 1; i < ble_cmd_table_size; i++) {
        if (ble_cmd_table[i].function) {
            app_print("\n\r    %s", ble_cmd_table[i].command);
        }
    }
}
#endif

static void cmd_ble_help(int argc, char **argv)
{
    app_print("BLE COMMAND LIST:");
    app_print("\n\r==============================");

#if (!defined(CONFIG_RF_TEST_SUPPORT)) && defined(CONFIG_BASECMD)
    ble_base_cmd_help();
#endif

#ifdef CONFIG_INTERNAL_DEBUG
    ble_int_cmd_help();
#endif

#if (defined(CONFIG_INTERNAL_DEBUG) || defined(CONFIG_RF_TEST_SUPPORT) || defined(CONFIG_BLE_DTM_SUPPORT))
    ble_rftest_cmd_help();
#endif

    app_print("\r\n");

    return;
}

void cmd_ble_help_cb(void)
{
    app_print("\tble_help\n");
}

uint8_t cmd_ble_get_handle_cb(void *data, void **cmd)
{
    struct cmd_entry *w_cmd;
    uint8_t ret = CLI_UNKWN_CMD;

    if (ble_work_status_get() != BLE_WORK_STATUS_ENABLE &&
        strcmp((char *)data, "ble_enable") && strcmp((char *)data, "ble_courier_wifi")) {
        app_print("ble is disabled, please \'ble_enable\' before\r\n");
        return CLI_ERROR;
    }

    w_cmd = (struct cmd_entry *)&ble_cmd_table[0];

    while (w_cmd->function) {
        if (!strcmp((char *)data, w_cmd->command)) {
            *cmd = w_cmd->function;
            ret = CLI_SUCCESS;
            break;
        }

        w_cmd++;
    }

#ifdef CONFIG_INTERNAL_DEBUG
    if (ret != CLI_SUCCESS) {
        ret = ble_int_get_handle_cb(data, cmd, &w_cmd);
    }
#endif

#if (defined(CONFIG_INTERNAL_DEBUG) || defined(CONFIG_RF_TEST_SUPPORT) ||defined(CONFIG_BLE_DTM_SUPPORT))
    if (ret != CLI_SUCCESS) {
        ret = ble_rftest_get_handle_cb(data, cmd, &w_cmd);
    }
#endif

    if (ret == CLI_SUCCESS) {
        ble_stack_task_resume(false);
    }

    return ret;
}

/*!
    \brief      Init BLE CLI module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_cli_init(void)
{
    cmd_module_reg(CMD_MODULE_BLE, "ble_", cmd_ble_get_handle_cb, cmd_ble_help_cb, NULL);
}

#endif // (BLE_APP_SUPPORT && BLE_APP_CMD_SUPPORT)
