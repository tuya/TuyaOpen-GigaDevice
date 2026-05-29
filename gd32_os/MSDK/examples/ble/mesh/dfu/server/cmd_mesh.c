/*!
    \file    cmd_mesh.c
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
#include "mesh_cfg.h"
#include "api/mesh.h"
#include "app_mesh.h"
#include "dbg_print.h"
#include "cmd_mesh_cfg.h"
#include "wrapper_os.h"

extern void mesh_log_set_dbg_level(uint16_t mask, uint8_t level);

static void cmd_ble_mesh_help(int argc, char **argv);

static void cmd_ble_mesh_set_log_property(int argc, char **argv)
{
    uint16_t mask;
    uint8_t dbg_level;

    if (argc < 3) {
        goto usage;
    }

    mask = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    dbg_level = (uint8_t)strtoul((const char *)argv[2], NULL, 0);

    mesh_log_set_dbg_level(mask, dbg_level);
    return;

usage:
    app_print("Usage: ble_mesh_set_log <mask> <level>\r\n");

}

static void cmd_ble_mesh_set_dev_uuid_prop(int argc, char **argv)
{
    size_t len;
    uint8_t uuid[16] = {0};

    if (argc < 2) {
        bt_rand(uuid, 16);
    } else {
        len = hex2bin(argv[1], strlen(argv[1]), uuid, sizeof(uuid));
        memset(uuid + len, 0, sizeof(uuid) - len);
    }

    app_mesh_set_dev_uuid_prop(uuid);
}


static void cmd_ble_mesh_reset(int argc, char **argv)
{
    bt_mesh_reset();
}

static void cmd_ble_mesh_prov_local(int argc, char **argv)
{
    uint16_t net_idx, addr;
    uint32_t iv_index;

    if (argc != 4) {
        goto usage;
    }

    net_idx = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    iv_index = (uint32_t)strtoul((const char *)argv[2], NULL, 0);
    addr = (uint16_t)strtoul((const char *)argv[3], NULL, 0);

    app_mesh_provision_local(net_idx, iv_index, addr);
    return;

usage:
    app_print("Usage: ble_mesh_provision_local <net_idx> <iv_index> <addr>\r\n");
    app_print("\t<net_idx>: NetIdx that the node was provisioned to.\r\n");
    app_print("\t<iv_index>: IV Index.\r\n");
    app_print("\t<addr>: Address of the node's primary element.\r\n");
}

static void cmd_ble_mesh_pb_adv(int argc, char **argv)
{
    int err = 0;
    bool onoff;
    char *endptr = NULL;

    if (argc < 2) {
        return;
    }

    onoff = strtoul(argv[1], &endptr, 0);

    if (onoff) {
        err = bt_mesh_prov_enable(BT_MESH_PROV_ADV);
        if (err) {
            app_print("Failed to enable %s (err %d)", bearer2str(BT_MESH_PROV_ADV), err);
        } else {
            app_print("%s enabled", bearer2str(BT_MESH_PROV_ADV));
        }
    } else {
        err = bt_mesh_prov_disable(BT_MESH_PROV_ADV);
        if (err) {
            app_print("Failed to disable %s (err %d)", bearer2str(BT_MESH_PROV_ADV), err);
        } else {
            app_print("%s disabled", bearer2str(BT_MESH_PROV_ADV));
        }
    }
}

static void cmd_ble_mesh_input_num(int argc, char **argv)
{
    int err = 0;
    char *endptr = NULL;
    uint32_t val;

    val = strtoul(argv[1], &endptr, 10);

    err = bt_mesh_input_number(val);
    if (err) {
        app_print("Numeric input failed (err %d)", err);
    }
}

static void cmd_ble_mesh_input_str(int argc, char **argv)
{
    int err = bt_mesh_input_string(argv[1]);

    if (err) {
        app_print("String input failed (err %d)", err);
    }
}

static void cmd_ble_mesh_comp_change(int argc, char **argv)
{
    bt_mesh_comp_change_prepare();
}


#if (CONFIG_BT_MESH_SAR_CFG_CLI)
static void cmd_ble_mesh_tx_get(int argc, char **argv)
{
    struct bt_mesh_sar_tx rsp;
    int err;
    uint16_t net_idx = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    uint16_t addr = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    err = bt_mesh_sar_cfg_cli_transmitter_get(net_idx, addr, &rsp);
    if (err) {
        app_print("Failed to send SAR Transmitter Get (err %d)\r\n", err);
        return;
    }

    app_print("Transmitter Get: %u %u %u %u %u %u %u\r\n",
              rsp.seg_int_step, rsp.unicast_retrans_count,
              rsp.unicast_retrans_without_prog_count,
              rsp.unicast_retrans_int_step, rsp.unicast_retrans_int_inc,
              rsp.multicast_retrans_count, rsp.multicast_retrans_int);
}

static void cmd_ble_mesh_tx_set(int argc, char **argv)
{
    struct bt_mesh_sar_tx set, rsp;
    int err = 0;
    uint16_t net_idx = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    uint16_t addr = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    set.seg_int_step = (uint8_t)strtoul((const char *)argv[3], NULL, 0);
    set.unicast_retrans_count = (uint8_t)strtoul((const char *)argv[4], NULL, 0);
    set.unicast_retrans_without_prog_count = (uint8_t)strtoul((const char *)argv[5], NULL, 0);
    set.unicast_retrans_int_step = (uint8_t)strtoul((const char *)argv[6], NULL, 0);
    set.unicast_retrans_int_inc = (uint8_t)strtoul((const char *)argv[7], NULL, 0);
    set.multicast_retrans_count = (uint8_t)strtoul((const char *)argv[8], NULL, 0);
    set.multicast_retrans_int = (uint8_t)strtoul((const char *)argv[9], NULL, 0);

    err = bt_mesh_sar_cfg_cli_transmitter_set(net_idx, addr, &set, &rsp);
    if (err) {
        app_print("Failed to send SAR Transmitter Set (err %d)\r\n", err);
        return;
    }

    app_print("Transmitter Set: %u %u %u %u %u %u %u\r\n",
              rsp.seg_int_step, rsp.unicast_retrans_count,
              rsp.unicast_retrans_without_prog_count,
              rsp.unicast_retrans_int_step, rsp.unicast_retrans_int_inc,
              rsp.multicast_retrans_count, rsp.multicast_retrans_int);
}

static void cmd_ble_mesh_rx_get(int argc, char **argv)
{
    struct bt_mesh_sar_rx rsp;
    int err;
    uint16_t net_idx = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    uint16_t addr = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    err = bt_mesh_sar_cfg_cli_receiver_get(net_idx, addr, &rsp);
    if (err) {
        app_print("Failed to send SAR Receiver Get (err %d)\r\n", err);
        return;
    }

    app_print("Receiver Get: %u %u %u %u %u\r\n", rsp.seg_thresh,
              rsp.ack_delay_inc, rsp.ack_retrans_count,
              rsp.discard_timeout, rsp.rx_seg_int_step);
}

static void cmd_ble_mesh_rx_set(int argc, char **argv)
{
    struct bt_mesh_sar_rx set, rsp;
    int err = 0;
    uint16_t net_idx = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    uint16_t addr = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    set.seg_thresh = (uint8_t)strtoul((const char *)argv[3], NULL, 0);
    set.ack_delay_inc = (uint8_t)strtoul((const char *)argv[4], NULL, 0);
    set.ack_retrans_count = (uint8_t)strtoul((const char *)argv[5], NULL, 0);
    set.discard_timeout = (uint8_t)strtoul((const char *)argv[6], NULL, 0);
    set.rx_seg_int_step = (uint8_t)strtoul((const char *)argv[7], NULL, 0);

    err = bt_mesh_sar_cfg_cli_receiver_set(net_idx, addr, &set, &rsp);
    if (err) {
        app_print("Failed to send SAR Receiver Set (err %d)\r\n", err);
        return;
    }

    app_print("Receiver Set: %u %u %u %u %u\r\n", rsp.seg_thresh,
              rsp.ack_delay_inc, rsp.ack_retrans_count,
              rsp.discard_timeout, rsp.rx_seg_int_step);
}
#endif

#if (CONFIG_BT_MESH_STATISTIC)
static void cmd_ble_mesh_stat_get(int argc, char **argv)
{
    struct bt_mesh_statistic st;

    bt_mesh_stat_get(&st);

    app_print("Received frames over:\r\n");
    app_print("adv:       %d\r\n", st.rx_adv);
    app_print("loopback:  %d\r\n", st.rx_loopback);
    app_print("proxy:     %d\r\n", st.rx_proxy);
    app_print("unknown:   %d\r\n", st.rx_uknown);

    app_print("Transmitted frames: <planned> - <succeeded>\r\n");
    app_print("relay adv:   %d - %d\r\n", st.tx_adv_relay_planned, st.tx_adv_relay_succeeded);
    app_print("local adv:   %d - %d\r\n", st.tx_local_planned, st.tx_local_succeeded);
    app_print("friend:      %d - %d\r\n", st.tx_friend_planned, st.tx_friend_succeeded);
}

static void cmd_ble_mesh_stat_clear(int argc, char **argv)
{
    bt_mesh_stat_reset();
}
#endif

const struct cmd_entry ble_mesh_cmd_table[] = {
    {"mesh_help", cmd_ble_mesh_help},
    {"mesh_set_log", cmd_ble_mesh_set_log_property},
    {"mesh_set_dev_uuid_prop", cmd_ble_mesh_set_dev_uuid_prop},
    {"mesh_reset", cmd_ble_mesh_reset},
    {"mesh_prov_local", cmd_ble_mesh_prov_local},
    {"mesh_pb_adv", cmd_ble_mesh_pb_adv},
    {"mesh_input_num", cmd_ble_mesh_input_num},
    {"mesh_input_str", cmd_ble_mesh_input_str},
    {"mesh_change_comp", cmd_ble_mesh_comp_change},

#if (CONFIG_BT_MESH_CFG_CLI)
    {"mesh_cfg_beacon", cmd_ble_mesh_cfg_beacon},
    {"mesh_cfg_get_comp", cmd_ble_mesh_cfg_get_comp},
    {"mesh_cfg_ttl", cmd_ble_mesh_cfg_ttl},
    {"mesh_cfg_gatt_proxy", cmd_ble_mesh_cfg_gatt_proxy},
    {"mesh_cfg_relay", cmd_ble_mesh_cfg_relay},
    {"mesh_cfg_pub", cmd_ble_mesh_cfg_mod_pub},
    {"mesh_cfg_sub_add", cmd_ble_mesh_cfg_mod_sub_add},
    {"mesh_cfg_sub_add_vnd", cmd_ble_mesh_cfg_mod_sub_add_vnd},
    {"mesh_cfg_sub_del", cmd_ble_mesh_cfg_mod_sub_del},
    {"mesh_cfg_sub_del_vnd", cmd_ble_mesh_cfg_mod_sub_del_vnd},
    {"mesh_cfg_sub_add_va", cmd_ble_mesh_cfg_mod_sub_add_va},
    {"mesh_cfg_sub_add_va_vnd", cmd_ble_mesh_cfg_mod_sub_add_va_vnd},
    {"mesh_cfg_sub_del_va", cmd_ble_mesh_cfg_mod_sub_del_va},
    {"mesh_cfg_sub_del_va_vnd", cmd_ble_mesh_cfg_mod_sub_del_va_vnd},
    {"mesh_cfg_sub_ow", cmd_ble_mesh_cfg_mod_sub_ow},
    {"mesh_cfg_sub_ow_vnd", cmd_ble_mesh_cfg_mod_sub_ow_vnd},
    {"mesh_cfg_sub_ow_va", cmd_ble_mesh_cfg_mod_sub_ow_va},
    {"mesh_cfg_sub_ow_va_vnd", cmd_ble_mesh_cfg_mod_sub_ow_va_vnd},
    {"mesh_cfg_sub_del_all", cmd_ble_mesh_cfg_mod_sub_del_all},
    {"mesh_cfg_sub_del_all_vnd", cmd_ble_mesh_cfg_mod_sub_del_all_vnd},
    {"mesh_cfg_sub_get", cmd_ble_mesh_cfg_mod_sub_get},
    {"mesh_cfg_sub_get_vnd", cmd_ble_mesh_cfg_mod_sub_get_vnd},
    {"mesh_cfg_reset_node", cmd_ble_mesh_cfg_node_reset},
    {"mesh_cfg_add_net_key", cmd_ble_mesh_cfg_net_key_add},
    {"mesh_cfg_update_net_key", cmd_ble_mesh_cfg_net_key_update},
    {"mesh_cfg_get_net_key", cmd_ble_mesh_cfg_net_key_get},
    {"mesh_cfg_add_app_key", cmd_ble_mesh_cfg_app_key_add},
    {"mesh_cfg_update_app_key", cmd_ble_mesh_cfg_app_key_upd},
    {"mesh_cfg_get_app_key", cmd_ble_mesh_cfg_app_key_get},
    {"mesh_cfg_del_app_key", cmd_ble_mesh_cfg_app_key_del},
    {"mesh_cfg_bind_mod_app", cmd_ble_mesh_cfg_mod_app_bind},
    {"mesh_cfg_unbind_mod_app", cmd_ble_mesh_cfg_mod_app_unbind},
    {"mesh_cfg_bind_mod_app_vnd", cmd_ble_mesh_cfg_mod_app_bind_vnd},
    {"mesh_cfg_unbind_mod_app_vnd", cmd_ble_mesh_cfg_mod_app_unbind_vnd},
    {"mesh_cfg_get_mod_app", cmd_ble_mesh_cfg_mod_app_get},
    {"mesh_cfg_get_mod_app_vnd", cmd_ble_mesh_cfg_mod_app_get_vnd},
    {"mesh_cfg_get_hb_pub", cmd_ble_mesh_cfg_hb_pub_get},
    {"mesh_cfg_set_hb_pub", cmd_ble_mesh_cfg_hb_pub_set},
    {"mesh_cfg_get_hb_sub", cmd_ble_mesh_cfg_hb_sub_get},
    {"mesh_cfg_set_hb_sub", cmd_ble_mesh_cfg_hb_sub_set},
    {"mesh_cfg_get_pollto", cmd_ble_mesh_cfg_pollto_get},
    {"mesh_cfg_net_transmit", cmd_ble_mesh_cfg_net_transmit},
#endif

#if (CONFIG_BT_MESH_SAR_CFG_CLI)
    {"mesh_tx_get", cmd_ble_mesh_tx_get},
    {"mesh_tx_set", cmd_ble_mesh_tx_set},
    {"mesh_rx_get", cmd_ble_mesh_rx_get},
    {"mesh_rx_set", cmd_ble_mesh_rx_set},
#endif

#if (CONFIG_BT_MESH_STATISTIC)
    {"mesh_stat_get", cmd_ble_mesh_stat_get},
    {"mesh_stat_clear", cmd_ble_mesh_stat_clear},
#endif

    {"", NULL}
};

const uint32_t ble_mesh_cmd_table_size = (sizeof(ble_mesh_cmd_table) / sizeof(
                                              ble_mesh_cmd_table[0]));

static void cmd_ble_mesh_help(int argc, char **argv)
{
    int i;

    app_print("mesh COMMAND LIST:");
    app_print("\n\r==============================");

    /* i is 1, not print 'ble_help' */
    for (i = 1; i < ble_mesh_cmd_table_size; i++) {
        if (ble_mesh_cmd_table[i].function) {
            app_print("\n\r    %s", ble_mesh_cmd_table[i].command);
        }
    }

    app_print("\r\n");
}

void cmd_ble_mesh_help_cb(void)
{
    app_print("\tmesh_help\n");
}

uint8_t cmd_ble_mesh_get_handle_cb(void *data, void **cmd)
{
    struct cmd_entry *w_cmd;
    uint8_t ret = CLI_UNKWN_CMD;

    if (ble_work_status_get() != BLE_WORK_STATUS_ENABLE) {
        app_print("ble is disabled, please \'ble_enable\' before\r\n");
        return CLI_ERROR;
    }

    w_cmd = (struct cmd_entry *)&ble_mesh_cmd_table[0];

    while (w_cmd->function) {
        if (!strcmp((char *)data, w_cmd->command)) {
            *cmd = w_cmd->function;
            ret = CLI_SUCCESS;
            break;
        }

        w_cmd++;
    }

    return ret;
}

void ble_mesh_cli_init(void)
{
    cmd_module_reg(CMD_MODULE_BLE_MESH, "mesh", cmd_ble_mesh_get_handle_cb, cmd_ble_mesh_help_cb,
                   NULL);

#if (CONFIG_BT_MESH_LOW_POWER && CONFIG_MESH_CB_REGISTERED)
    bt_mesh_lpn_cb_register(&lpn_cb);
#endif
}

