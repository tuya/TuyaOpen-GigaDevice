/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*!
    \file    app_mesh.c
    \brief   Implementation of BLE mesh application.

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

#include "mesh_cfg.h"
#include "api/mesh.h"
#include "app_mesh.h"
#include "bluetooth/bt_str.h"
#include "wrapper_os.h"
#include "mesh_kernel.h"
#include "dbg_print.h"
#include "app_mesh_cfg.h"
#include "api/settings.h"
#include "nvds_flash.h"
#include "app_mesh_dfu_cli.h"

static struct k_sem sem_prov_cap_cmd;

static const char *const output_meth_string[] = {
    "Blink",
    "Beep",
    "Vibrate",
    "Display Number",
    "Display String",
};

static const char *const input_meth_string[] = {
    "Push",
    "Twist",
    "Enter Number",
    "Enter String",
};

static uint8_t dev_default_uuid[16] = { 0x00, 0x1B, 0xDC, 0x08, 0x10, 0x21, 0x0B, 0x0E, 0x0A, 0x0C, 0x00, 0x0B, 0x0E, 0x0A, 0x0C, 0x00 };

/* Default net, app & dev key values, unless otherwise specified */
const uint8_t app_mesh_default_net_key[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

const uint8_t app_mesh_default_dev_key[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

const uint8_t app_mesh_default_app_key[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};


#if (CONFIG_BT_MESH_LARGE_COMP_DATA_CLI)
struct bt_mesh_large_comp_data_cli large_comp_data_cli;
#endif

#if (CONFIG_BT_MESH_SAR_CFG_CLI)
static struct bt_mesh_sar_cfg_cli sar_cfg_cli;
#endif

static const struct bt_mesh_model root_models[] = {
    BT_MESH_MODEL_CFG_SRV,
    BT_MESH_MODEL_CFG_CLI(&app_cfg_cli),

#if (CONFIG_BT_MESH_DFD_SRV)
    BT_MESH_MODEL_DFD_SRV(&app_dfd_srv),
#endif
#if (CONFIG_BT_MESH_SAR_CFG_SRV)
    BT_MESH_MODEL_SAR_CFG_SRV,
#endif
#if (CONFIG_BT_MESH_SAR_CFG_CLI)
    BT_MESH_MODEL_SAR_CFG_CLI(&sar_cfg_cli),
#endif

#if (CONFIG_BT_MESH_LARGE_COMP_DATA_SRV)
    BT_MESH_MODEL_LARGE_COMP_DATA_SRV,
#endif
#if (CONFIG_BT_MESH_LARGE_COMP_DATA_CLI)
    BT_MESH_MODEL_LARGE_COMP_DATA_CLI(&large_comp_data_cli),
#endif

};

static const struct bt_mesh_model vnd_models[] = {

};


static const struct bt_mesh_elem elements[] = {
    BT_MESH_ELEM(0, root_models, vnd_models),
};

static const struct bt_mesh_comp comp = {
    .cid = 0xFFFF,
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

#if (CONFIG_BT_MESH_COMP_PAGE_2)
static const uint8_t cmp2_elem_offset[1] = {0};

static const struct bt_mesh_comp2_record comp_rec = {
    .id = 0x1600,
    .version.x = 1,
    .version.y = 0,
    .version.z = 0,
    .elem_offset_cnt = 1,
    .elem_offset = cmp2_elem_offset,
    .data_len = 0
};

static const struct bt_mesh_comp2 comp_p2 = {.record_cnt = 1, .record = &comp_rec};
#endif

const char *bearer2str(bt_mesh_prov_bearer_t bearer)
{
    switch (bearer) {
    case BT_MESH_PROV_ADV:
        return "PB-ADV";
    case BT_MESH_PROV_GATT:
        return "PB-GATT";
    case BT_MESH_PROV_REMOTE:
        return "PB-REMOTE";
    default:
        return "unknown";
    }
}

static void app_mesh_heartbeat_recv(const struct bt_mesh_hb_sub *sub, uint8_t hops, uint16_t feat)
{
    app_print(
        "app_mesh_heartbeat_recv hops %d, feat 0x%04x:\r\n" \
        "\tSubscription period:            %us\r\n"            \
        "\tRemaining subscription time:    %us\r\n"            \
        "\tSource address:                 0x%04x\r\n"                \
        "\tDestination address:            0x%04x\r\n"                \
        "\tNumber Heartbeat messages:      %u\r\n"            \
        "\tMinimum hops:                   %u\r\n"                \
        "\tMaximum hops:                   %u\r\n",
        hops, feat, sub->period, sub->remaining, sub->src, sub->dst, sub->count,
        sub->min_hops, sub->max_hops);
}

static void app_mesh_heartbeat_sub_end(const struct bt_mesh_hb_sub *sub)
{
    app_print("app_mesh_heartbeat_sub_end src 0x%04x, dst 0x%04x\r\n", sub->src, sub->dst);
}

static void app_mesh_heartbeat_pub_sent(const struct bt_mesh_hb_pub *pub)
{
    app_print(
        "app_mesh_heartbeat_pub_sent:\r\n"                     \
        "\tDestination address:            0x%04x\r\n"         \
        "\tRemaining publish count:        %u\r\n"            \
        "\tTime To Live value:             %u\r\n"            \
        "\tFeatures:                       0x%04x\r\n"          \
        "\tNumber Heartbeat messages:      %u\r\n"              \
        "\tNetwork index:                  %u\r\n"              \
        "\tPublication period:             %us\r\n",
        pub->dst, pub->count, pub->ttl, pub->feat, pub->count, pub->net_idx, pub->period);
}

#if (CONFIG_MESH_CB_REGISTERED)
static struct bt_mesh_hb_cb heartbeat_cb = {
    .recv = app_mesh_heartbeat_recv,
    .sub_end = app_mesh_heartbeat_sub_end,
    .pub_sent = app_mesh_heartbeat_pub_sent,
    .next = NULL,
};
#else
BT_MESH_HB_CB_DEFINE(heartbeat_cb) = {
    .recv = app_mesh_heartbeat_recv,
    .sub_end = app_mesh_heartbeat_sub_end,
    .pub_sent = app_mesh_heartbeat_pub_sent,
};
#endif

static void app_mesh_prov_link_open(bt_mesh_prov_bearer_t bearer)
{
    app_print("Provisioning link opened on %s\r\n", bearer2str(bearer));
}

void app_mesh_prov_link_close(bt_mesh_prov_bearer_t bearer)
{
    app_print("Provisioning link closed on %s\r\n", bearer2str(bearer));
}

static void app_mesh_unprovisioned_beacon(uint8_t uuid[16], bt_mesh_prov_oob_info_t oob_info,
                                          uint32_t *uri_hash)
{
    char uuid_hex_str[32 + 1];

    bin2hex(uuid, 16, uuid_hex_str, sizeof(uuid_hex_str));

    app_print("uuid %s, oob_info %d\r\n", uuid_hex_str, oob_info);

    if (uri_hash) {
        app_print("uri_hash %s\r\n", uri_hash);
    }
}

static void app_mesh_unprovisioned_beacon_gatt(uint8_t uuid[16], bt_mesh_prov_oob_info_t oob_info)
{
    char uuid_hex_str[32 + 1];

    bin2hex(uuid, 16, uuid_hex_str, sizeof(uuid_hex_str));

    app_print("gatt uuid %s, oob_info %d\r\n", uuid_hex_str, oob_info);
}

static void app_mesh_node_added(uint16_t net_idx, uint8_t uuid[16], uint16_t addr, uint8_t num_elem)
{
    char uuid_hex_str[32 + 1];
    bin2hex(uuid, 16, uuid_hex_str, sizeof(uuid_hex_str));
    app_print("Node provisioned, net_idx 0x%04x address 0x%04x elements %d, uuid %s\r\n",
              net_idx, addr, num_elem, uuid_hex_str);
}

static void app_mesh_prov_complete(uint16_t net_idx, uint16_t addr)
{
    app_print("######## Provision complete net_idx %d, addr 0x%04x ######\r\n", net_idx, addr);
}

static void app_mesh_prov_capabilities(const struct bt_mesh_dev_capabilities *cap)
{
    int err;
    k_timeout_t timeout = K_SECONDS(10);        // 10s

    app_print("Provisionee capabilities:\r\n");
    app_print("\tStatic OOB is %ssupported\r\n", cap->oob_type & 1 ? "" : "not ");

    app_print("\tAvailable output actions (%d bytes max):%s\r\n", cap->output_size,
              cap->output_actions ? "" : "\n\t\tNone");
    for (int i = 0; i < ARRAY_SIZE(output_meth_string); i++) {
        if (cap->output_actions & BIT(i)) {
            app_print("\t\t%s\r\n", output_meth_string[i]);
        }
    }

    app_print("\tAvailable input actions (%d bytes max):%s\r\n", cap->input_size,
              cap->input_actions ? "" : "\n\t\tNone");
    for (int i = 0; i < ARRAY_SIZE(input_meth_string); i++) {
        if (cap->input_actions & BIT(i)) {
            app_print("\t\t%s\r\n", input_meth_string[i]);
        }
    }

    app_print("Please use ble_mesh_auth_method_set_input/ble_mesh_auth_method_set_output/ble_mesh_auth_method_set_static/ble_mesh_auth_method_set_none\r\n");
    app_print("Waiting to set auth method ...\r\n");

    err = k_sem_take(&sem_prov_cap_cmd, timeout);

    if (err != 0) {
        app_print("Timeout for set auth method\r\n");
    }
}

static int app_mesh_prov_output_number(bt_mesh_output_action_t action, uint32_t number)
{
    switch (action) {
    case BT_MESH_BLINK:
        app_print("OOB blink Number: %u\r\n", number);
        break;
    case BT_MESH_BEEP:
        app_print("OOB beep Number: %u\r\n", number);
        break;
    case BT_MESH_VIBRATE:
        app_print("OOB vibrate Number: %u\r\n", number);
        break;
    case BT_MESH_DISPLAY_NUMBER:
        app_print("OOB display Number: %u\r\n", number);
        break;
    default:
        app_print("Unknown Output action %u (number %u) requested!\r\n", action, number);
        return -EINVAL;
    }

    return 0;
}

static int app_mesh_prov_output_string(const char *str)
{
    app_print("OOB String: %s\r\n", str);
    return 0;
}

static int app_mesh_prov_input(bt_mesh_input_action_t act, uint8_t size)
{
    switch (act) {
    case BT_MESH_ENTER_NUMBER:
        app_print("Enter a number (max %u digits) with: Input-num <num>\r\n", size);
        break;
    case BT_MESH_ENTER_STRING:
        app_print("Enter a string (max %u chars) with: Input-str <str>\r\n", size);
        break;
    case BT_MESH_TWIST:
        app_print("\"Twist\" a number (max %u digits) with: Input-num <num>\r\n", size);
        break;
    case BT_MESH_PUSH:
        app_print("\"Push\" a number (max %u digits) with: Input-num <num>\r\n", size);
        break;
    default:
        app_print("Unknown Input action %u (size %u) requested!\r\n", act, size);
        return -EINVAL;
    }

    return 0;
}


static void app_mesh_prov_input_complete(void)
{
    app_print("Provison Input complete\r\n");
}


static const struct bt_mesh_prov prov = {
    .uuid = dev_default_uuid,
    .link_open = app_mesh_prov_link_open,
    .link_close = app_mesh_prov_link_close,
    .complete = app_mesh_prov_complete,

    .static_val = NULL,
    .static_val_len = 0,
    .output_size = 6,
    .output_actions = (BT_MESH_BLINK | BT_MESH_BEEP | BT_MESH_VIBRATE | BT_MESH_DISPLAY_NUMBER |
                       BT_MESH_DISPLAY_STRING),
    .input_size = 6,
    .input_actions = (BT_MESH_ENTER_NUMBER | BT_MESH_ENTER_STRING | BT_MESH_TWIST | BT_MESH_PUSH),
    .unprovisioned_beacon = app_mesh_unprovisioned_beacon,
    .unprovisioned_beacon_gatt = app_mesh_unprovisioned_beacon_gatt,
    .node_added = app_mesh_node_added,

    .capabilities = app_mesh_prov_capabilities,
    .output_number = app_mesh_prov_output_number,
    .output_string = app_mesh_prov_output_string,
    .input = app_mesh_prov_input,
    .input_complete = app_mesh_prov_input_complete,
};

void app_mesh_cdb_node_add(uint16_t addr, uint8_t num_elem, uint16_t net_idx, uint8_t uuid[16],
                           uint8_t dev_key[16])
{
    struct bt_mesh_cdb_node *node;
    int err = 0;

    node = bt_mesh_cdb_node_alloc(uuid == NULL ? dev_default_uuid : uuid, addr, num_elem, net_idx);
    if (node == NULL) {
        app_print("Failed to allocate node\r\n");
        return;
    }

    err = bt_mesh_cdb_node_key_import(node, dev_key == NULL ? app_mesh_default_dev_key : dev_key);
    if (err) {
        app_print("Unable to import device key into cdb, err:%d\r\n", err);
        return;
    }

    if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
        bt_mesh_cdb_node_store(node);
    }

    app_print("Added node addr: 0x%04x\r\n", node->addr);
}

void app_mesh_cdb_subnet_add(uint16_t net_idx, uint8_t net_key[16])
{
    struct bt_mesh_cdb_subnet *sub;

    sub = bt_mesh_cdb_subnet_alloc(net_idx);
    if (sub == NULL) {
        app_print("Could not add subnet\r\n");
        return ;
    }

    if (bt_mesh_cdb_subnet_key_import(sub, 0, net_key)) {
        app_print("Unable to import key for subnet 0x%03x\r\n", net_idx);
        return ;
    }

    if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
        bt_mesh_cdb_subnet_store(sub);
    }

    app_print("Added Subnet 0x%03x\r\n", net_idx);

    return;
}

void app_mesh_cdb_app_key_add(uint16_t net_idx, uint16_t app_idx, uint8_t app_key[16])
{
    struct bt_mesh_cdb_app_key *key;

    key = bt_mesh_cdb_app_key_alloc(net_idx, app_idx);
    if (key == NULL) {
        app_print("Could not add AppKey\r\n");
        return;
    }

    if (bt_mesh_cdb_app_key_import(key, 0, app_key)) {
        app_print("Unable to import app key 0x%03x\r\n", app_idx);
        return;
    }

    if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
        bt_mesh_cdb_app_key_store(key);
    }

    app_print("Added AppKey 0x%03x\r\n", app_idx);
}

void app_mesh_provision_local(uint16_t net_idx, uint32_t iv_idx, uint16_t addr, uint8_t *net_key,
                              uint8_t *dev_key)
{
    int err = 0;

    app_mesh_cdb_subnet_add(net_idx, net_key);

    err = bt_mesh_provision(net_key, net_idx, 0, iv_idx, addr, dev_key);
    if (err) {
        app_print("provision local fail, err:%d\r\n", err);
        return;
    }

    app_print("provision local success, net_idx: %u, iv_idx: %u, addr: 0x%04x\r\n", net_idx, iv_idx,
              addr);
}

void app_mesh_set_dev_uuid_prop(uint8_t uuid[16])
{
    int err;
    uint8_t dev_uuid[16] = {0};
    uint32_t uuid_len = 16;
    err = nvds_data_get(NULL, MESH_NAME_SPACE, "DEV_UUID", dev_uuid, (uint32_t *)&uuid_len);

    if (err == NVDS_OK) {
        app_print("Get device uuid from storage %s\r\n", bt_hex(dev_uuid, uuid_len));
    }

    app_print("Set new device uuid to storage %s\r\n", bt_hex(uuid, uuid_len));
#if (CONFIG_BT_SETTINGS)
    err = settings_save_one("DEV_UUID", uuid, uuid_len);
#endif
    if (err != NVDS_OK) {
        app_print("Set new device uuid fail\r\n");
        return;
    }

    sys_memcpy(dev_default_uuid, uuid, uuid_len);
}

void app_mesh_init(void)
{
    int err;
    uint8_t dev_uuid[16] = {0};
    uint32_t uuid_len = 16;

    extern void ble_mesh_cli_init(void);
    ble_mesh_cli_init();

    mesh_kernel_init();

    k_sem_init(&sem_prov_cap_cmd, 0, 1);

    err = nvds_data_get(NULL, MESH_NAME_SPACE, "DEV_UUID", dev_uuid, (uint32_t *)&uuid_len);

    if (err == NVDS_OK) {
        app_print("Get device uuid from storage %s\r\n", bt_hex(dev_uuid, uuid_len));
        sys_memcpy(dev_default_uuid, dev_uuid, uuid_len);
    } else {
        sys_random_bytes_get(dev_default_uuid, uuid_len);
        app_print("First init mesh, get random device uuid %s\r\n", bt_hex(dev_default_uuid, uuid_len));
#if (CONFIG_BT_SETTINGS)
        settings_save_one("DEV_UUID", dev_default_uuid, uuid_len);
#endif
    }

    err = bt_mesh_init(&prov, &comp);

    if (err) {
        app_print("mesh init fail, err:%d\r\n", err);
        return;
    }

#if defined(CONFIG_BT_MESH_COMP_PAGE_2)
    bt_mesh_comp2_register(&comp_p2);
#endif

#if (CONFIG_MESH_CB_REGISTERED)
    bt_mesh_hearbeat_cb_register(&heartbeat_cb);
#endif

    err = bt_mesh_cdb_create(app_mesh_default_net_key);
    if (err) {
        app_print("cdb create net key err %d\r\n", err);
    } else {
        err = bt_mesh_provision(app_mesh_default_net_key, 0, 0, 0, 1,
                                app_mesh_default_dev_key);
        if (err) {
            app_print("provision local fail, err:%d\r\n", err);
        }
    }

    app_mesh_dfu_cli_init();

    app_print("mesh init success, uuid: %s\r\n", bt_hex(prov.uuid, 16));
}


void app_mesh_cdb_print_nodes(void)
{
    char key_hex_str[32 + 1], uuid_hex_str[32 + 1];
    struct bt_mesh_cdb_node *node;
    int i, total = 0;
    bool configured;
    uint8_t dev_key[16];

    app_print("Address  Elements  Flags  %-32s  DevKey\r\n", "UUID");

    for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.nodes); ++i) {
        node = &bt_mesh_cdb.nodes[i];
        if (node->addr == BT_MESH_ADDR_UNASSIGNED) {
            continue;
        }

        configured = atomic_test_bit(node->flags, BT_MESH_CDB_NODE_CONFIGURED);

        total++;
        bin2hex(node->uuid, 16, uuid_hex_str, sizeof(uuid_hex_str));
        if (bt_mesh_cdb_node_key_export(node, dev_key)) {
            app_print("Unable to export key for node 0x%04x\r\n", node->addr);
            continue;
        }
        bin2hex(dev_key, 16, key_hex_str, sizeof(key_hex_str));
        app_print("0x%04x   %-8d  %-5s  %s  %s\r\n", node->addr,
                  node->num_elem, configured ? "C" : "-",
                  uuid_hex_str, key_hex_str);
    }

    app_print("> Total nodes: %d\r\n", total);
}

void app_mesh_cdb_print_subnets(void)
{
    struct bt_mesh_cdb_subnet *subnet;
    char key_hex_str[32 + 1];
    int i, total = 0;
    uint8_t net_key[16];

    app_print("NetIdx  NetKey\r\n");

    for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.subnets); ++i) {
        subnet = &bt_mesh_cdb.subnets[i];
        if (subnet->net_idx == BT_MESH_KEY_UNUSED) {
            continue;
        }

        if (bt_mesh_cdb_subnet_key_export(subnet, 0, net_key)) {
            app_print("Unable to export key for subnet 0x%03x\r\n", subnet->net_idx);
            continue;
        }

        total++;
        bin2hex(net_key, 16, key_hex_str, sizeof(key_hex_str));
        app_print("0x%03x   %s\r\n", subnet->net_idx, key_hex_str);
    }

    app_print("> Total subnets: %d\r\n", total);
}

void app_mesh_cdb_print_app_keys(void)
{
    struct bt_mesh_cdb_app_key *key;
    char key_hex_str[32 + 1];
    int i, total = 0;
    uint8_t app_key[16];

    app_print("NetIdx  AppIdx  AppKey\r\n");

    for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.app_keys); ++i) {
        key = &bt_mesh_cdb.app_keys[i];
        if (key->net_idx == BT_MESH_KEY_UNUSED) {
            continue;
        }

        if (bt_mesh_cdb_app_key_export(key, 0, app_key)) {
            app_print("Unable to export app key 0x%03x\r\n", key->app_idx);
            continue;
        }

        total++;
        bin2hex(app_key, 16, key_hex_str, sizeof(key_hex_str));
        app_print("0x%03x   0x%03x   %s\r\n", key->net_idx, key->app_idx, key_hex_str);
    }

    app_print("> Total app-keys: %d\r\n", total);
}


void app_mesh_auth_method_set_done(void)
{
    k_sem_give(&sem_prov_cap_cmd);
}

