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
#include "cmd_mesh_rpr.h"
#include "cmd_mesh_health.h"
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
    size_t len;
    uint16_t net_idx, addr;
    uint32_t iv_index;
    uint8_t net_key[16] = {0};
    uint8_t dev_key[16] = {0};

    if (argc != 6) {
        goto usage;
    }

    net_idx = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    iv_index = (uint32_t)strtoul((const char *)argv[2], NULL, 0);
    addr = (uint16_t)strtoul((const char *)argv[3], NULL, 0);

    len = hex2bin(argv[4], strlen(argv[4]), net_key, sizeof(net_key));
    memset(net_key + len, 0, sizeof(net_key) - len);

    len = hex2bin(argv[5], strlen(argv[5]), dev_key, sizeof(dev_key));
    memset(dev_key + len, 0, sizeof(dev_key) - len);

    app_mesh_provision_local(net_idx, iv_index, addr, net_key, dev_key);
    return;

usage:
    app_print("Usage: ble_mesh_provision_local <net_idx> <iv_index> <addr> <netkey> <devkey>\r\n");
    app_print("\t<net_idx>: NetIdx that the node was provisioned to.\r\n");
    app_print("\t<iv_index>: IV Index.\r\n");
    app_print("\t<addr>: Address of the node's primary element.\r\n");
}

static void cmd_ble_mesh_cdb_create(int argc, char **argv)
{
    uint8_t net_key[16];
    size_t len;
    int err;

    if (argc < 2) {
        bt_rand(net_key, 16);
    } else {
        len = hex2bin(argv[1], strlen(argv[1]), net_key, sizeof(net_key));
        memset(net_key + len, 0, sizeof(net_key) - len);
    }

    debug_print_dump_data("net key", (char *)net_key, 16);

    err = bt_mesh_cdb_create(net_key);
    if (err < 0) {
        app_print("Failed to create CDB (err %d) \r\n", err);
    }
}

static void cmd_ble_mesh_cdb_node_add(int argc, char **argv)
{
    uint16_t net_idx, addr;
    uint8_t num_elem;
    uint8_t uuid[16] = {0}, dev_key[16] = {0};
    uint8_t *p_uuid = NULL, *p_dev_key = NULL;
    size_t len;

    if (argc < 4) {
        goto usage;
    }

    addr = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    num_elem = (uint8_t)strtoul((const char *)argv[2], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[3], NULL, 0);

    if (argc > 4) {
        len = hex2bin(argv[4], strlen(argv[4]), uuid, sizeof(uuid));
        if (len < 1) {
            goto usage;
        }
        p_uuid = uuid;
    }

    if (argc > 5) {
        len = hex2bin(argv[5], strlen(argv[5]), dev_key, sizeof(dev_key));
        if (len < 1) {
            goto usage;
        }
        p_dev_key = dev_key;
    }

    app_mesh_cdb_node_add(addr, num_elem, net_idx, p_uuid, p_dev_key);
    return;

usage:
    app_print("Usage: ble_mesh_cdb_node_add <addr> <num_elem> <net_idx> [UUID(1-16 hex)] [dev_key(1-16 hex)]\r\n");
    app_print("\t<addr>: Address of the node's primary element.\r\n");
    app_print("\t<num_elem>: Number of elements that the node has.\r\n");
    app_print("\t<net_idx>: NetIdx that the node was provisioned to.\r\n");
    app_print("\t[UUID(1-16 hex)]: UUID of the node.\r\n");
    app_print("\t[dev_key(1-16 hex)]: Device key value.\r\n");
}

static void cmd_ble_mesh_cdb_node_del(int argc, char **argv)
{
    char *endptr = NULL;
    struct bt_mesh_cdb_node *node;
    uint16_t addr;

    addr = (uint16_t)strtoul((const char *)argv[1], &endptr, 0);

    node = bt_mesh_cdb_node_get(addr);
    if (node == NULL) {
        app_print("No node with address 0x%04x\r\n", addr);
        return;
    }

    bt_mesh_cdb_node_del(node, true);

    app_print("Deleted node 0x%04x\r\n", addr);

    return;
}

static void cmd_ble_mesh_cdb_subnet_add(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t net_key[16];
    uint16_t net_idx;
    size_t len;

    net_idx = strtoul((const char *)argv[1], &endptr, 0);

    if (argc < 3) {
        bt_rand(net_key, 16);
    } else {
        len = hex2bin(argv[2], strlen(argv[2]), net_key, sizeof(net_key));
        memset(net_key + len, 0, sizeof(net_key) - len);
    }

    app_mesh_cdb_subnet_add(net_idx, net_key);
}

static void cmd_ble_mesh_cdb_subnet_del(int argc, char **argv)
{
    char *endptr = NULL;
    struct bt_mesh_cdb_subnet *sub;
    uint16_t net_idx;

    net_idx = strtoul((const char *)argv[1], &endptr, 0);

    sub = bt_mesh_cdb_subnet_get(net_idx);
    if (sub == NULL) {
        app_print("No subnet with NetIdx 0x%03x\r\n", net_idx);
        return;
    }

    bt_mesh_cdb_subnet_del(sub, true);

    app_print("Deleted subnet 0x%03x\r\n", net_idx);
}

static void cmd_ble_mesh_cdb_app_key_add(int argc, char **argv)
{
    char *endptr = NULL;
    uint16_t net_idx, app_idx;
    uint8_t app_key[16];
    size_t len;

    net_idx = strtoul((const char *)argv[1], &endptr, 0);
    app_idx = strtoul((const char *)argv[2], &endptr, 0);

    if (argc < 4) {
        bt_rand(app_key, 16);
    } else {
        len = hex2bin(argv[3], strlen(argv[3]), app_key, sizeof(app_key));
        memset(app_key + len, 0, sizeof(app_key) - len);
    }

    app_mesh_cdb_app_key_add(net_idx, app_idx, app_key);
}

static void cmd_ble_mesh_cdb_app_key_del(int argc, char **argv)
{
    char *endptr = NULL;
    struct bt_mesh_cdb_app_key *key;
    uint16_t app_idx;

    app_idx = strtoul((const char *)argv[1], &endptr, 0);

    key = bt_mesh_cdb_app_key_get(app_idx);
    if (key == NULL) {
        app_print("No AppKey 0x%03x\r\n", app_idx);
        return;
    }

    bt_mesh_cdb_app_key_del(key, true);

    app_print("Deleted AppKey 0x%03x\r\n", app_idx);
}

static void cmd_ble_mesh_cdb_clear(int argc, char **argv)
{
    bt_mesh_cdb_clear();

    app_print("Cleared CDB\r\n");
}


static void cmd_ble_mesh_cdb_show(int argc, char **argv)
{
    if (!atomic_test_bit(bt_mesh_cdb.flags, BT_MESH_CDB_VALID)) {
        app_print("No valid networks\r\n");
        return;
    }

    app_print("Mesh Network Information\r\n");
    app_print("========================\r\n");

    app_mesh_cdb_print_nodes();
    app_print("---\r\n");
    app_mesh_cdb_print_subnets();
    app_print("---\r\n");
    app_mesh_cdb_print_app_keys();
}

static void cmd_ble_mesh_remote_pub_key_set(int argc, char **argv)
{
    size_t len;
    uint8_t pub_key[64];
    int err = 0;

    len = hex2bin(argv[1], strlen(argv[1]), pub_key, sizeof(pub_key));
    if (len < 1) {
        app_print("Unable to parse input string argument");
        return;
    }

    err = bt_mesh_prov_remote_pub_key_set(pub_key);

    if (err) {
        app_print("Setting remote pub key failed (err %d)", err);
    }
}

static void cmd_ble_mesh_auth_method_set_input(int argc, char **argv)
{
    int err = 0;
    char *endptr = NULL;
    bt_mesh_input_action_t action = strtoul(argv[1], &endptr, 10);
    uint8_t size = strtoul(argv[2], &endptr, 10);

    err = bt_mesh_auth_method_set_input(action, size);
    if (err) {
        app_print("Setting input OOB authentication action failed (err %d)", err);
    }

    app_mesh_auth_method_set_done();
}


static void cmd_ble_mesh_auth_method_set_output(int argc, char **argv)
{
    int err = 0;
    char *endptr = NULL;
    bt_mesh_output_action_t action = strtoul(argv[1], &endptr, 10);
    uint8_t size = strtoul(argv[2], &endptr, 10);


    err = bt_mesh_auth_method_set_output(action, size);
    if (err) {
        app_print("Setting output OOB authentication action failed (err %d)", err);
    }

    app_mesh_auth_method_set_done();
}

static void cmd_ble_mesh_auth_method_set_static(int argc, char **argv)
{
    size_t len;
    uint8_t static_oob_auth[32];
    int err = 0;

    len = hex2bin(argv[1], strlen(argv[1]), static_oob_auth, sizeof(static_oob_auth));
    if (len < 1) {
        app_print("Unable to parse input string argument");
        return;
    }

    err = bt_mesh_auth_method_set_static(static_oob_auth, len);
    if (err) {
        app_print("Setting static OOB authentication failed (err %d)", err);
    }

    app_mesh_auth_method_set_done();
}

static void cmd_ble_mesh_auth_method_set_none(int argc, char **argv)
{
    bt_mesh_auth_method_set_none();

    app_mesh_auth_method_set_done();
}


static void cmd_ble_mesh_provision_adv(int argc, char **argv)
{
    uint8_t uuid[16];
    uint8_t attention_duration;
    uint16_t net_idx;
    uint16_t addr;
    size_t len;
    char *endptr = NULL;
    int err = 0;

    len = hex2bin(argv[1], strlen(argv[1]), uuid, sizeof(uuid));
    memset(uuid + len, 0, sizeof(uuid) - len);

    net_idx = strtoul(argv[2], &endptr, 0);
    addr = strtoul(argv[3], &endptr, 0);
    attention_duration = strtoul(argv[4], &endptr, 0);

    err = bt_mesh_provision_adv(uuid, net_idx, addr, attention_duration);
    if (err) {
        app_print("Provisioning failed (err %d)", err);
    }
}

static void cmd_ble_mesh_provision_gatt(int argc, char **argv)
{
    static uint8_t uuid[16];
    uint8_t attention_duration;
    uint16_t net_idx;
    uint16_t addr;
    size_t len;
    char *endptr = NULL;
    int err = 0;

    len = hex2bin(argv[1], strlen(argv[1]), uuid, sizeof(uuid));
    memset(uuid + len, 0, sizeof(uuid) - len);

    net_idx = strtoul(argv[2], &endptr, 0);
    addr = strtoul(argv[3], &endptr, 0);
    attention_duration = strtoul(argv[4], &endptr, 0);

    err = bt_mesh_provision_gatt(uuid, net_idx, addr, attention_duration);
    if (err) {
        app_print("Provisioning failed (err %d)\r\n", err);
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

static void cmd_ble_mesh_vnd_op(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t app_idx;
    uint16_t dst;
    uint8_t op;

    net_idx = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    app_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);
    dst = (uint16_t)strtoul((const char *)argv[3], NULL, 0);
    op = (uint8_t)strtoul((const char *)argv[4], NULL, 0);

    vnd_button_op(net_idx, app_idx, dst, op);
}

static void cmd_ble_mesh_vnd_op_va(int argc, char **argv)
{
    uint16_t net_idx;
    uint16_t app_idx;
    uint16_t dst;
    uint8_t op;
    uint16_t len;
    uint8_t label[16];

    net_idx = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    app_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);
    dst = (uint16_t)strtoul((const char *)argv[3], NULL, 0);
    len = hex2bin(argv[4], strlen(argv[4]), label, sizeof(label));
    memset(label + len, 0, sizeof(label) - len);
    op = (uint8_t)strtoul((const char *)argv[5], NULL, 0);

    vnd_button_op_va(net_idx, app_idx, dst, op, label);
}


#if (CONFIG_BT_MESH_PROXY_CLIENT)
static void cmd_ble_mesh_proxy_connect(int argc, char **argv)
{
    uint16_t net_idx;
    int err = 0;

    net_idx = (uint16_t)strtoul((const char *)argv[1], NULL, 0);

    err = bt_mesh_proxy_connect(net_idx);
    if (err) {
        app_print("Proxy connect failed (err %d)\r\n", err);
    }
}

static void cmd_ble_mesh_proxy_disconnect(int argc, char **argv)
{
    uint16_t net_idx;
    int err = 0;

    net_idx = (uint16_t)strtoul((const char *)argv[1], NULL, 0);

    err = bt_mesh_proxy_disconnect(net_idx);
    if (err) {
        app_print("Proxy disconnect failed (err %d)\r\n", err);
    }
}
#endif /* CONFIG_BT_MESH_PROXY_CLIENT */

#if (CONFIG_BT_MESH_PROXY_SOLICITATION)
static void cmd_ble_mesh_proxy_solicit(int argc, char **argv)
{
    uint16_t net_idx;
    int err = 0;

    net_idx = (uint16_t)strtoul((const char *)argv[1], NULL, 0);

    err = bt_mesh_proxy_solicit(net_idx);
    if (err) {
        app_print("Failed to advertise solicitation PDU (err %d)\r\n", err);
    }
}
#endif /* CONFIG_BT_MESH_PROXY_SOLICITATION */


#if (CONFIG_BT_MESH_OD_PRIV_PROXY_CLI)
static void cmd_ble_mesh_od_priv_gatt_proxy(int argc, char **argv)
{
    uint16_t net_idx, addr;
    uint8_t val = 0;
    uint8_t val_rsp;

    if (argc < 4) {
        goto usage;
    }

    addr = (uint16_t)strtoul((const char *)argv[2], NULL, 0);
    net_idx = (uint16_t)strtoul((const char *)argv[3], NULL, 0);

    if (!strcmp("get", argv[1])) {
        bt_mesh_od_priv_proxy_cli_get(net_idx, addr, &val_rsp);
    } else if (!strcmp("set", argv[1]) && argc == 5) {
        val = (uint8_t)strtoul((const char *)argv[4], NULL, 0);
        bt_mesh_od_priv_proxy_cli_set(net_idx, addr, val, &val_rsp);
    } else {
        goto usage;
    }

    app_print("mesh od_priv_gatt_proxy net_idx: %u, addr: %u, val_rsp: %u\r\n", net_idx, addr, val_rsp);
    return;

usage:
    app_print("Usage: mesh_od_priv_gatt_proxy <set or get> <addr> <net_idx> [val]\r\n");
    app_print("\t<set or get>: set or get handle.\r\n");
    app_print("\t<addr>: Address of the node's primary element.\r\n");
    app_print("\t<net_idx>: NetIdx that the node was provisioned to.\r\n");
    app_print("\t[val]: 1: enable; 0: disable.\r\n");
}
#endif

#if (CONFIG_BT_MESH_SOL_PDU_RPL_CLI)
static void cmd_ble_mesh_srpl_clear(int argc, char **argv)
{
    int status = 0;
    uint8_t len;
    uint16_t app_idx, addr, start_rsp, range_start;
    bool acked;
    uint8_t len_rsp;

    if (argc < 5) {
        goto usage;
    }

    addr = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    app_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);
    struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_APP(app_idx, addr);


    range_start = (uint16_t)strtoul((const char *)argv[3], NULL, 0);
    acked = (bool)strtoul((const char *)argv[4], NULL, 0);

    if (argc > 5) {
        len = (uint8_t)strtoul((const char *)argv[5], NULL, 0);
    } else {
        len = 0;
    }

    if (acked) {
        status = bt_mesh_sol_pdu_rpl_clear(&ctx, range_start, len, &start_rsp, &len_rsp);
        app_print("mesh srpl clear ack start_rsp: %u, len_rsp: %u, status: %u\r\n", start_rsp, len_rsp, status);
        return;
    }

    status = bt_mesh_sol_pdu_rpl_clear_unack(&ctx, range_start, len);
    app_print("mesh srpl clear unack status: %u\r\n", status);
    return;

usage:
    app_print("Usage: mesh_srpl_clear <addr> <app_idx> <range_start> <acked> [range_len]\r\n");
}
#endif

static void cmd_ble_mesh_large_comp_data_get(int argc, char **argv)
{
    NET_BUF_SIMPLE_DEFINE(comp, 64);
    struct bt_mesh_large_comp_data_rsp rsp = {
        .data = &comp,
    };
    int err = 0;

    uint16_t net_idx = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    uint16_t addr = (uint16_t)strtoul((const char *)argv[2], NULL, 0);
    uint8_t page = (uint8_t)strtoul((const char *)argv[3], NULL, 0);
    uint16_t offset = (uint16_t)strtoul((const char *)argv[4], NULL, 0);

    net_buf_simple_init(&comp, 0);

    err = bt_mesh_large_comp_data_get(net_idx, addr, page, offset, &rsp);
    if (err) {
        app_print("Large Composition Data get err: %d\r\n", err);
        return;
    }

    app_print("Large Composition Data get [0x%04x]: page: %u offset: %u total size: %u\r\n", addr, rsp.page, rsp.offset,
        rsp.total_size);
    debug_print_dump_data("Composition Data", (char *)rsp.data->data, rsp.data->len);
}

static void cmd_ble_mesh_models_metadata_get(int argc, char **argv)
{
    NET_BUF_SIMPLE_DEFINE(metadata, 64);
    struct bt_mesh_large_comp_data_rsp rsp = {
        .data = &metadata,
    };
    int err = 0;

    uint16_t net_idx = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    uint16_t addr = (uint16_t)strtoul((const char *)argv[2], NULL, 0);
    uint8_t page = (uint8_t)strtoul((const char *)argv[3], NULL, 0);
    uint16_t offset = (uint16_t)strtoul((const char *)argv[4], NULL, 0);

    net_buf_simple_init(&metadata, 0);

    err = bt_mesh_models_metadata_get(net_idx, addr, page, offset, &rsp);
    if (err) {
        app_print("models metadata get err: %d\r\n", err);
        return;
    }

    app_print("models metadata Data get [0x%04x]: page: %u offset: %u total size: %u\r\n", addr, rsp.page, rsp.offset,
        rsp.total_size);
    debug_print_dump_data("models metadata", (char *)rsp.data->data, rsp.data->len);
}

#if (CONFIG_BT_MESH_OP_AGG_CLI)
static void cmd_ble_mesh_seq_start(int argc, char **argv)
{
    int err = 0;
    uint16_t elem_addr = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    uint16_t net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);
    uint16_t app_idx = (uint16_t)strtoul((const char *)argv[3], NULL, 0);

    app_print("mesh dst set to 0x%04x\r\n", elem_addr);

    err = bt_mesh_op_agg_cli_seq_start(net_idx, app_idx, elem_addr, elem_addr);
    if (err) {
        app_print("Failed to configure Opcodes Aggregator Context (err %d)\r\n", err);
    }
}

static void cmd_ble_mesh_seq_send(int argc, char **argv)
{
    int err;

    err = bt_mesh_op_agg_cli_seq_send();
    if (err) {
        app_print("Failed to send Opcodes Aggregator Sequence message (err %d)\r\n", err);
    }
}

static void cmd_ble_mesh_seq_abort(int argc, char **argv)
{
    bt_mesh_op_agg_cli_seq_abort();
}
#endif

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

#if (CONFIG_BT_MESH_PRIV_BEACON_CLI)
static void cmd_ble_mesh_priv_beacon_get(int argc, char **argv)
{
    struct bt_mesh_priv_beacon val;
    int err;
    uint16_t net_idx = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    uint16_t addr = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    err = bt_mesh_priv_beacon_cli_get(net_idx, addr, &val);
    if (err) {
        app_print("Failed to send Private Beacon Get (err %d)\r\n", err);
        return;
    }

    app_print("Private Beacon state: %u, %u\r\n", val.enabled, val.rand_interval);
}

static void cmd_ble_mesh_priv_beacon_set(int argc, char **argv)
{
    struct bt_mesh_priv_beacon val;
    int err = 0;
    uint16_t net_idx = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    uint16_t addr = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    val.enabled = (uint8_t)strtoul((const char *)argv[3], NULL, 0);
    val.rand_interval = (uint8_t)strtoul((const char *)argv[4], NULL, 0);

    err = bt_mesh_priv_beacon_cli_set(net_idx, addr, &val, &val);
    if (err) {
        app_print("Failed to send Private Beacon Set (err %d)\r\n", err);
    }

    app_print("Private Beacon state: %u, %u\r\n", val.enabled, val.rand_interval);
}

static void cmd_ble_mesh_priv_gatt_proxy_get(int argc, char **argv)
{
    uint8_t state;
    int err;
    uint16_t net_idx = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    uint16_t addr = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    err = bt_mesh_priv_beacon_cli_gatt_proxy_get(net_idx, addr, &state);
    if (err) {
        app_print("Failed to send Private GATT Proxy Get (err %d)\r\n", err);
        return;
    }

    app_print("Private GATT Proxy state: %u\r\n", state);
}

static void cmd_ble_mesh_priv_gatt_proxy_set(int argc, char **argv)
{
    uint8_t state;
    int err = 0;
    uint16_t net_idx = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    uint16_t addr = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    state = (uint8_t)strtoul((const char *)argv[3], NULL, 0);

    err = bt_mesh_priv_beacon_cli_gatt_proxy_set(net_idx, addr, state, &state);
    if (err) {
        app_print("Failed to send Private GATT Proxy Set (err %d)\r\n", err);
    }

    app_print("Private GATT Proxy state: %u\r\n", state);
}

static void cmd_ble_mesh_priv_node_id_get(int argc, char **argv)
{
    struct bt_mesh_priv_node_id val;
    uint16_t key_net_idx;
    int err = 0;
    uint16_t net_idx = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    uint16_t addr = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    key_net_idx = (uint16_t)strtoul((const char *)argv[3], NULL, 0);

    err = bt_mesh_priv_beacon_cli_node_id_get(net_idx, addr, key_net_idx, &val);
    if (err) {
        app_print("Failed to send Private Node Identity Get (err %d)\r\n", err);
        return;
    }

    app_print("Private Node Identity state: (net_idx: %u, state: %u, status: %u)\r\n",
            val.net_idx, val.state, val.status);
}

static void cmd_ble_mesh_priv_node_id_set(int argc, char **argv)
{
    struct bt_mesh_priv_node_id val;
    int err = 0;
    uint16_t net_idx = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    uint16_t addr = (uint16_t)strtoul((const char *)argv[2], NULL, 0);

    val.net_idx = (uint16_t)strtoul((const char *)argv[3], NULL, 0);
    val.state = (uint8_t)strtoul((const char *)argv[4], NULL, 0);

    err = bt_mesh_priv_beacon_cli_node_id_set(net_idx, addr, &val, &val);
    if (err) {
        app_print("Failed to send Private Node Identity Set (err %d)\r\n", err);
    }

    app_print("Private Node Identity state: (net_idx: %u, state: %u, status: %u)\r\n",
            val.net_idx, val.state, val.status);
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
    {"mesh_cdb_create", cmd_ble_mesh_cdb_create},
    {"mesh_cdb_add_node", cmd_ble_mesh_cdb_node_add},
    {"mesh_cdb_del_node", cmd_ble_mesh_cdb_node_del},
    {"mesh_cdb_add_subnet", cmd_ble_mesh_cdb_subnet_add},
    {"mesh_cdb_del_subnet", cmd_ble_mesh_cdb_subnet_del},
    {"mesh_cdb_add_app_key", cmd_ble_mesh_cdb_app_key_add},
    {"mesh_cdb_del_app_key", cmd_ble_mesh_cdb_app_key_del},
    {"mesh_cdb_clear", cmd_ble_mesh_cdb_clear},
    {"mesh_cdb_show", cmd_ble_mesh_cdb_show},
    {"mesh_set_remote_pub_key", cmd_ble_mesh_remote_pub_key_set},
    {"mesh_auth_method_set_input", cmd_ble_mesh_auth_method_set_input},
    {"mesh_auth_method_set_output", cmd_ble_mesh_auth_method_set_output},
    {"mesh_auth_method_set_static", cmd_ble_mesh_auth_method_set_static},
    {"mesh_auth_method_set_none", cmd_ble_mesh_auth_method_set_none},
    {"mesh_provision_adv", cmd_ble_mesh_provision_adv},
    {"mesh_provision_gatt", cmd_ble_mesh_provision_gatt},
    {"mesh_input_num", cmd_ble_mesh_input_num},
    {"mesh_input_str", cmd_ble_mesh_input_str},
    {"mesh_change_comp", cmd_ble_mesh_comp_change},
    {"mesh_vnd_op", cmd_ble_mesh_vnd_op},
    {"mesh_vnd_op_va", cmd_ble_mesh_vnd_op_va},

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

#if (CONFIG_BT_MESH_RPR_CLI)
    {"mesh_rpr_set_srv", cmd_ble_mesh_rpr_set_srv},
    {"mesh_rpr_scan", cmd_ble_mesh_rpr_scan},
    {"mesh_rpr_scan_ext", cmd_ble_mesh_rpr_scan_ext},
    {"mesh_rpr_scan_srv", cmd_ble_mesh_rpr_scan_srv},
    {"mesh_rpr_scan_caps", cmd_ble_mesh_rpr_scan_caps},
    {"mesh_rpr_scan_get", cmd_ble_mesh_rpr_scan_get},
    {"mesh_rpr_scan_stop", cmd_ble_mesh_rpr_scan_stop},
    {"mesh_rpr_link_get", cmd_ble_mesh_rpr_link_get},
    {"mesh_rpr_link_close", cmd_ble_mesh_rpr_link_close},
    {"mesh_rpr_provision_remote", cmd_ble_mesh_rpr_provision_remote},
    {"mesh_rpr_reprovision_remote", cmd_ble_mesh_rpr_reprovision_remote},
#endif

#if (CONFIG_BT_MESH_PROXY_CLIENT)
    {"mesh_proxy_connect", cmd_ble_mesh_proxy_connect},
    {"mesh_proxy_disconnect", cmd_ble_mesh_proxy_disconnect},
#endif /* CONFIG_BT_MESH_PROXY_CLIENT */

#if (CONFIG_BT_MESH_PROXY_SOLICITATION)
    {"mesh_proxy_solicit", cmd_ble_mesh_proxy_solicit},
#endif /* CONFIG_BT_MESH_PROXY_SOLICITATION */

#if (CONFIG_BT_MESH_OD_PRIV_PROXY_CLI)
    {"mesh_od_pri_gatt_proxy", cmd_ble_mesh_od_priv_gatt_proxy},
#endif
#if (CONFIG_BT_MESH_SOL_PDU_RPL_CLI)
    {"mesh_srpl_clear", cmd_ble_mesh_srpl_clear},
#endif

#if (CONFIG_BT_MESH_HEALTH_CLI)
    {"mesh_fault_get", cmd_ble_mesh_fault_get},
    {"mesh_fault_clear", cmd_ble_mesh_fault_clear},
    {"mesh_fault_clear_unack", cmd_ble_mesh_fault_clear_unack},
    {"mesh_fault_test", cmd_ble_mesh_fault_test},
    {"mesh_fault_test_unack", cmd_ble_mesh_fault_test_unack},
    {"mesh_period_get", cmd_ble_mesh_period_get},
    {"mesh_period_set", cmd_ble_mesh_period_set},
    {"mesh_period_set_unack", cmd_ble_mesh_period_set_unack},
    {"mesh_attention_get", cmd_ble_mesh_attention_get},
    {"mesh_attention_set_unack", cmd_ble_mesh_attention_set_unack},
#endif

    {"mesh_add_fault", cmd_ble_mesh_add_fault},
    {"mesh_del_fault", cmd_ble_mesh_del_fault},

#if (CONFIG_BT_MESH_LARGE_COMP_DATA_CLI)
    {"mesh_large_comp_data_get", cmd_ble_mesh_large_comp_data_get},
    {"mesh_models_metadata_get", cmd_ble_mesh_models_metadata_get},
#endif

#if (CONFIG_BT_MESH_OP_AGG_CLI)
    {"mesh_seq_start", cmd_ble_mesh_seq_start},
    {"mesh_seq_send", cmd_ble_mesh_seq_send},
    {"mesh_seq_abort", cmd_ble_mesh_seq_abort},
#endif
#if (CONFIG_BT_MESH_SAR_CFG_CLI)
    {"mesh_tx_get", cmd_ble_mesh_tx_get},
    {"mesh_tx_set", cmd_ble_mesh_tx_set},
    {"mesh_rx_get", cmd_ble_mesh_rx_get},
    {"mesh_rx_set", cmd_ble_mesh_rx_set},
#endif
#if (CONFIG_BT_MESH_PRIV_BEACON_CLI)
    {"mesh_priv_beacon_get", cmd_ble_mesh_priv_beacon_get},
    {"mesh_priv_beacon_set", cmd_ble_mesh_priv_beacon_set},
    {"mesh_priv_gatt_proxy_get", cmd_ble_mesh_priv_gatt_proxy_get},
    {"mesh_priv_gatt_proxy_set", cmd_ble_mesh_priv_gatt_proxy_set},
    {"mesh_priv_node_id_get", cmd_ble_mesh_priv_node_id_get},
    {"mesh_priv_node_id_set", cmd_ble_mesh_priv_node_id_set},
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
}

