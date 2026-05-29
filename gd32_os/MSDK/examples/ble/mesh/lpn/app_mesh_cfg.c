/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*!
    \file    app_mesh_cfg.c
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
#include "mesh_cfg.h"
#include "app_cfg.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api/mesh.h"
#include "app_mesh.h"
#include "bluetooth/bt_str.h"
#include "wrapper_os.h"
#include "mesh_kernel.h"
#include "dbg_print.h"
#include "app_mesh_cfg.h"
#include "errno.h"

static void app_cfg_cli_comp_data(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t page,
                                  struct net_buf_simple *buf);
static void app_cfg_cli_mod_pub_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                       uint16_t elem_addr, uint16_t mod_id, uint16_t cid,
                                       struct bt_mesh_cfg_cli_mod_pub *pub);
static void app_cfg_cli_mod_sub_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                       uint16_t elem_addr,
                                       uint16_t sub_addr, uint32_t mod_id);
static void app_cfg_cli_mod_sub_list(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                     uint16_t elem_addr, uint16_t mod_id, uint16_t cid,
                                     struct net_buf_simple *buf);
static void app_cfg_cli_node_reset_status(struct bt_mesh_cfg_cli *cli, uint16_t addr);
static void app_cfg_cli_beacon_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status);
static void app_cfg_cli_ttl_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status);
static void app_cfg_cli_friend_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status);
static void app_cfg_cli_gatt_proxy_status(struct bt_mesh_cfg_cli *cli, uint16_t addr,
                                          uint8_t status);
static void app_cfg_cli_network_transmit_status(struct bt_mesh_cfg_cli *cli, uint16_t addr,
                                                uint8_t status);
static void app_cfg_cli_relay_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                     uint8_t transmit);
static void app_cfg_cli_net_key_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                       uint16_t net_idx);
static void app_cfg_cli_net_key_list(struct bt_mesh_cfg_cli *cli, uint16_t addr,
                                     struct net_buf_simple *buf);
static void app_cfg_cli_app_key_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                       uint16_t net_idx, uint16_t app_idx);
static void app_cfg_cli_app_key_list(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                     uint16_t net_idx, struct net_buf_simple *buf);
static void app_cfg_cli_mod_app_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                       uint16_t elem_addr,
                                       uint16_t app_idx, uint32_t mod_id);
static void app_cfg_cli_mod_app_list(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                     uint16_t elem_addr, uint16_t mod_id, uint16_t cid,
                                     struct net_buf_simple *buf);
static void app_cfg_cli_node_identity_status(struct bt_mesh_cfg_cli *cli, uint16_t addr,
                                             uint8_t status, uint16_t net_idx, uint8_t identity);
static void app_cfg_cli_lpn_timeout_status(struct bt_mesh_cfg_cli *cli, uint16_t addr,
                                           uint16_t elem_addr, uint32_t timeout);
static void app_cfg_cli_krp_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                   uint16_t net_idx, uint8_t phase);
static void app_cfg_cli_hb_pub_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                      struct bt_mesh_cfg_cli_hb_pub *pub);
static void app_cfg_cli_hb_sub_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                      struct bt_mesh_cfg_cli_hb_sub *sub);

static struct bt_mesh_cfg_cli_cb app_cfg_cli_cb = {
    .comp_data = app_cfg_cli_comp_data,
    .mod_pub_status = app_cfg_cli_mod_pub_status,
    .mod_sub_status = app_cfg_cli_mod_sub_status,
    .mod_sub_list = app_cfg_cli_mod_sub_list,
    .node_reset_status = app_cfg_cli_node_reset_status,
    .beacon_status = app_cfg_cli_beacon_status,
    .ttl_status = app_cfg_cli_ttl_status,
    .friend_status = app_cfg_cli_friend_status,
    .gatt_proxy_status = app_cfg_cli_gatt_proxy_status,
    .network_transmit_status = app_cfg_cli_network_transmit_status,
    .relay_status = app_cfg_cli_relay_status,
    .net_key_status = app_cfg_cli_net_key_status,
    .net_key_list = app_cfg_cli_net_key_list,
    .app_key_status = app_cfg_cli_app_key_status,
    .app_key_list = app_cfg_cli_app_key_list,
    .mod_app_status = app_cfg_cli_mod_app_status,
    .mod_app_list = app_cfg_cli_mod_app_list,
    .node_identity_status = app_cfg_cli_node_identity_status,
    .lpn_timeout_status = app_cfg_cli_lpn_timeout_status,
    .krp_status = app_cfg_cli_krp_status,
    .hb_pub_status = app_cfg_cli_hb_pub_status,
    .hb_sub_status = app_cfg_cli_hb_sub_status,
};

struct bt_mesh_cfg_cli app_cfg_cli = {
    .cb = &app_cfg_cli_cb,
};

void app_mesh_cfg_get_comp(uint16_t net_idx, uint16_t dst, uint8_t page)
{
    NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_RX_SDU_MAX);
    struct bt_mesh_comp_p0_elem elem;
    struct bt_mesh_comp_p0 comp;
    int err = 0;

    err = bt_mesh_cfg_cli_comp_data_get(net_idx, dst, page, &page, &buf);
    if (err) {
        app_print("Getting composition failed (err %d)\r\n", err);
        return;
    }

    if (page != 0 && page != 128 &&
        ((page != 1 && page != 129) || !IS_ENABLED(CONFIG_BT_MESH_COMP_PAGE_1)) &&
        ((page != 2 && page != 130) || !IS_ENABLED(CONFIG_BT_MESH_COMP_PAGE_2))) {
        app_print("Got page %d. No parser available.\r\n", page);
        return;
    }

    if (page == 0 || page == 128) {
        err = bt_mesh_comp_p0_get(&comp, &buf);

        if (err) {
            app_print("Couldn't parse Composition data (err %d)\r\n", err);
            return;
        }

        app_print("Got Composition Data for 0x%04x, page: %d:\r\n", dst, page);
        app_print("\tCID      0x%04x\r\n", comp.cid);
        app_print("\tPID      0x%04x\r\n", comp.pid);
        app_print("\tVID      0x%04x\r\n", comp.vid);
        app_print("\tCRPL     0x%04x\r\n", comp.crpl);
        app_print("\tFeatures 0x%04x\r\n", comp.feat);

        while (bt_mesh_comp_p0_elem_pull(&comp, &elem)) {
            int i;

            app_print("\tElement @ 0x%04x:\r\n", elem.loc);

            if (elem.nsig) {
                app_print("\t\tSIG Models:\r\n");
            } else {
                app_print("\t\tNo SIG Models\r\n");
            }

            for (i = 0; i < elem.nsig; i++) {
                uint16_t mod_id = bt_mesh_comp_p0_elem_mod(&elem, i);

                app_print("\t\t\t0x%04x\r\n", mod_id);
            }

            if (elem.nvnd) {
                app_print("\t\tVendor Models:\r\n");
            } else {
                app_print("\t\tNo Vendor Models\r\n");
            }

            for (i = 0; i < elem.nvnd; i++) {
                struct bt_mesh_mod_id_vnd mod =
                    bt_mesh_comp_p0_elem_mod_vnd(&elem, i);

                app_print("\t\t\tCompany 0x%04x: 0x%04x\r\n", mod.company, mod.id);
            }
        }
    }

    if (IS_ENABLED(CONFIG_BT_MESH_COMP_PAGE_1) && (page == 1 || page == 129)) {
        /* size of 32 is chosen arbitrary, as sufficient for testing purposes */
        NET_BUF_SIMPLE_DEFINE(p1_buf, 32);
        NET_BUF_SIMPLE_DEFINE(p1_item_buf, 32);
        struct bt_mesh_comp_p1_elem p1_elem = { ._buf = &p1_buf };
        struct bt_mesh_comp_p1_model_item mod_item = { ._buf = &p1_item_buf };
        struct bt_mesh_comp_p1_ext_item ext_item = { 0 };
        int mod_idx = 1;

        if (!buf.len) {
            app_print("Composition data empty\r\n");
            return;
        }
        app_print("Got Composition Data for 0x%04x, page: %d: \r\n", dst, page);

        while (bt_mesh_comp_p1_elem_pull(&buf, &p1_elem)) {
            int i, j;

            app_print("\tElement #%d description\r\n", mod_idx);

            for (i = 0; i < p1_elem.nsig; i++) {
                if (bt_mesh_comp_p1_item_pull(&p1_elem, &mod_item)) {
                    app_print("\t\tSIG Model Item #%d:\r\n", i + 1);
                    if (mod_item.cor_present) {
                        app_print("\t\t\tWith Corresponding ID %u\r\n", mod_item.cor_id);
                    } else {
                        app_print("\t\t\tWithout Corresponding ID\r\n");
                    }
                    app_print("\t\t\tWith %u Extended Model Item(s)\r\n", mod_item.ext_item_cnt);
                }
                for (j = 0; j < mod_item.ext_item_cnt; j++) {
                    bt_mesh_comp_p1_pull_ext_item(&mod_item,
                                                  &ext_item);
                    app_print("\t\t\t\tExtended Item #%d:\r\n", j + 1);
                    if (ext_item.type == SHORT) {
                        app_print("\t\t\t\t\toffset: %u\r\n", ext_item.short_item.elem_offset);
                        app_print("\t\t\t\t\tindex: %u\r\n", ext_item.short_item.mod_item_idx);
                    } else {
                        app_print("\t\t\t\t\toffset: %u\r\n", ext_item.long_item.elem_offset);
                        app_print("\t\t\t\t\tindex: %u\r\n", ext_item.long_item.mod_item_idx);
                    }
                }
            }
            for (i = 0; i < p1_elem.nvnd; i++) {
                if (bt_mesh_comp_p1_item_pull(&p1_elem, &mod_item)) {
                    app_print("\t\tVendor Model Item #%d:\r\n", i + 1);
                    if (mod_item.cor_present) {
                        app_print("\t\t\tWith Corresponding ID %u\r\n", mod_item.cor_id);
                    } else {
                        app_print("\t\t\tWithout Corresponding ID\r\n");
                    }
                    app_print("\t\t\tWith %u Extended Model Item(s)\r\n", mod_item.ext_item_cnt);
                }
                for (j = 0; j < mod_item.ext_item_cnt; j++) {
                    bt_mesh_comp_p1_pull_ext_item(&mod_item, &ext_item);
                    app_print("\t\t\t\tExtended Item #%d:\r\n", j + 1);

                    if (ext_item.type == SHORT) {
                        app_print("\t\t\t\t\toffset: %u\r\n", ext_item.short_item.elem_offset);
                        app_print("\t\t\t\t\tindex: %u\r\n", ext_item.short_item.mod_item_idx);
                    } else {
                        app_print("\t\t\t\t\toffset: %u\r\n", ext_item.long_item.elem_offset);
                        app_print("\t\t\t\t\tindex: %u\r\n", ext_item.long_item.mod_item_idx);
                    }
                }
            }
            mod_idx++;
        }
    }

    if (IS_ENABLED(CONFIG_BT_MESH_COMP_PAGE_2) && (page == 2 || page == 130)) {
        /* size of 32 is chosen arbitrary, as sufficient for testing purposes */
        NET_BUF_SIMPLE_DEFINE(p2_elem_offset_buf, 32);
        NET_BUF_SIMPLE_DEFINE(p2_data_buf, 32);
        struct bt_mesh_comp_p2_record p2_elem = {
            .elem_buf = &p2_elem_offset_buf,
            .data_buf = &p2_data_buf
        };

        if (!buf.len) {
            app_print("Composition data empty\r\n");
            return;
        }
        app_print("Got Composition Data for 0x%04x, page: %d:\r\n", dst, page);

        while (bt_mesh_comp_p2_record_pull(&buf, &p2_elem)) {

            app_print("\tMesh Profile id: %04x \r\n", p2_elem.id);
            app_print("\t\tVersion: %d.%d.%d \r\n", p2_elem.version.x, p2_elem.version.y, p2_elem.version.z);
            app_print("\t\tElement offsets:\r\n");

            while (p2_elem.elem_buf->len) {
                app_print("\t\t\t%d \r\n", net_buf_simple_pull_u8(p2_elem.elem_buf));
            }

            if (p2_elem.data_buf->len) {
                app_print("\t\t%d bytes of additional data is available\r\n", p2_elem.data_buf->len);
            }
        }
    }

    if (buf.len) {
        app_print("\t\t...truncated data!\r\n");
    }
}

int app_mesh_cfg_mod_pub_set(uint16_t net_idx, uint16_t dst, uint16_t addr, bool is_va,
                             uint16_t mod_id,
                             uint16_t cid, char *argv[])
{
    struct bt_mesh_cfg_cli_mod_pub pub;
    uint8_t status, count, res_step, steps;
    uint16_t interval;
    uint8_t uuid[16];
    uint8_t len;
    int err = 0;

    if (!is_va) {
        pub.addr = strtoul(argv[0], NULL, 0);
        pub.uuid = NULL;
    } else {
        len = hex2bin(argv[0], strlen(argv[0]), uuid, sizeof(uuid));
        memset(uuid + len, 0, sizeof(uuid) - len);
        pub.uuid = (const uint8_t *)&uuid;
    }

    pub.app_idx = strtoul(argv[1], NULL, 0);
    pub.cred_flag = strtoul(argv[2], NULL, 0);
    pub.ttl = strtoul(argv[3], NULL, 0);
    res_step = strtoul(argv[4], NULL, 0);
    steps = strtoul(argv[5], NULL, 0);
    if ((res_step > 3) || (steps > 0x3F)) {
        app_print("Invalid period\r\n");
        return -EINVAL;
    }

    pub.period = (steps << 2) + res_step;
    count = strtoul(argv[6], NULL, 0);
    if (count > 7) {
        app_print("Invalid retransmit count\r\n");
        return -EINVAL;
    }

    interval = strtoul(argv[7], NULL, 0);
    if (err) {
        app_print("Unable to parse input string argument\r\n");
        return err;
    }

    if (interval > (31 * 50) || (interval % 50)) {
        app_print("Invalid retransmit interval %u\r\n", interval);
        return -EINVAL;
    }

    pub.transmit = BT_MESH_PUB_TRANSMIT(count, interval);

    app_print("Mod pub set addr 0x%x, app_idx %d, cred_flag %d, ttl %d, period 0x%x, transmit 0x%x\r\n",
              pub.addr, pub.app_idx, pub.cred_flag, pub.ttl, pub.period, pub.transmit);

    if (cid == APP_CID_NVAL) {
        err = bt_mesh_cfg_cli_mod_pub_set(net_idx, dst, addr, mod_id, &pub, &status);
    } else {
        err = bt_mesh_cfg_cli_mod_pub_set_vnd(net_idx, dst, addr, mod_id, cid, &pub, &status);
    }

    if (err) {
        app_print("Model Publication Set failed (err %d)\r\n", err);
        return 0;
    }

    if (status) {
        app_print("Model Publication Set failed (status 0x%02x)\r\n", status);
    } else {
        app_print("Model Publication successfully set\r\n");
    }

    return 0;
}


int app_mesh_cfg_mod_pub_get(uint16_t net_idx, uint16_t dst, uint16_t addr, uint16_t mod_id,
                             uint16_t cid)
{
    struct bt_mesh_cfg_cli_mod_pub pub;
    uint8_t status;
    int err;

    if (cid == APP_CID_NVAL) {
        err = bt_mesh_cfg_cli_mod_pub_get(net_idx, dst, addr, mod_id, &pub, &status);
    } else {
        err = bt_mesh_cfg_cli_mod_pub_get_vnd(net_idx, dst, addr, mod_id, cid, &pub, &status);
    }

    if (status) {
        app_print("Model Publication Get failed (status 0x%02x)\r\n", status);
        return 0;
    }

#if 0
    app_print(
        "Model Publication for Element 0x%04x, Model 0x%04x:\r\n"
        "\tPublish Address:                0x%04x\r\n"
        "\tAppKeyIndex:                    0x%04x\r\n"
        "\tCredential Flag:                %u\r\n"
        "\tPublishTTL:                     %u\r\n"
        "\tPublishPeriod:                  0x%02x\r\n"
        "\tPublishRetransmitCount:         %u\r\n"
        "\tPublishRetransmitInterval:      %ums",
        addr, mod_id, pub.addr, pub.app_idx, pub.cred_flag, pub.ttl, pub.period,
        BT_MESH_PUB_TRANSMIT_COUNT(pub.transmit),
        BT_MESH_PUB_TRANSMIT_INT(pub.transmit));
#endif

    return 0;
}

void app_mesh_cfg_net_key_add(uint16_t net_idx, uint16_t dst, uint16_t key_net_idx,
                              uint8_t key_val[16])
{
    uint8_t status;
    int err = 0;

    if (IS_ENABLED(CONFIG_BT_MESH_CDB)) {
        struct bt_mesh_cdb_subnet *subnet;

        subnet = bt_mesh_cdb_subnet_get(key_net_idx);
        if (subnet) {
            if (memcmp(subnet->keys[0].net_key.key, key_val, 16)) {
                app_print("Subnet 0x%03x already has a value\r\n", key_net_idx);
                return;
            }
        } else {
            subnet = bt_mesh_cdb_subnet_alloc(key_net_idx);
            if (!subnet) {
                app_print("No space for subnet in cdb\r\n");
                return;
            }

            if (bt_mesh_cdb_subnet_key_import(subnet, 0, key_val)) {
                app_print("Unable to import subnet key into cdb 0x%03x\r\n", key_net_idx);
                return;
            }
            bt_mesh_cdb_subnet_store(subnet);
        }
    }

    err = bt_mesh_cfg_cli_net_key_add(net_idx, dst, key_net_idx, key_val, &status);
    if (err) {
        app_print("Unable to send NetKey Add (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("NetKeyAdd failed with status 0x%02x\r\n", status);
    } else {
        app_print("NetKey added with NetKey Index 0x%03x\r\n", key_net_idx);
    }
}


void app_mesh_cfg_app_key_add(uint16_t net_idx, uint16_t dst, uint16_t key_net_idx,
                              uint16_t key_app_idx, uint8_t key_val[16])
{
    uint8_t status;
    int err = 0;

    if (IS_ENABLED(CONFIG_BT_MESH_CDB)) {
        struct bt_mesh_cdb_app_key *app_key;

        app_key = bt_mesh_cdb_app_key_get(key_app_idx);
        if (app_key) {
            if (memcmp(app_key->keys[0].app_key.key, key_val, 16)) {
                app_print("App key 0x%03x already has a value\r\n", key_app_idx);
                return;
            }
        } else {
            app_key = bt_mesh_cdb_app_key_alloc(key_net_idx, key_app_idx);
            if (!app_key) {
                app_print("No space for app key in cdb\r\n");
                return;
            }

            if (bt_mesh_cdb_app_key_import(app_key, 0, key_val)) {
                app_print("Unable to import app key 0x%03x into cdb\r\n", key_app_idx);
                return;
            }
            bt_mesh_cdb_app_key_store(app_key);
        }
    }

    err = bt_mesh_cfg_cli_app_key_add(net_idx, dst, key_net_idx, key_app_idx, key_val, &status);
    if (err) {
        app_print("Unable to send App Key Add (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("AppKeyAdd failed with status 0x%02x\r\n", status);
    } else {
        app_print("AppKey added, NetKeyIndex 0x%04x AppKeyIndex 0x%04x\r\n", key_net_idx, key_app_idx);
    }
}


void app_mesh_cfg_hb_pub_get(uint16_t net_idx, uint16_t dst)
{
    struct bt_mesh_cfg_cli_hb_pub pub;
    uint8_t status;
    int err;

    err = bt_mesh_cfg_cli_hb_pub_get(net_idx, dst, &pub, &status);

    if (err) {
        app_print("Heartbeat Publication Get failed (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Heartbeat Publication Get failed (status 0x%02x)\r\n", status);
        return;
    }

    app_print("Heartbeat publication:\r\n");
    app_print("\tdst 0x%04x count 0x%02x period 0x%02x\r\n", pub.dst, pub.count, pub.period);
    app_print("\tttl 0x%02x feat 0x%04x net_idx 0x%04x\r\n", pub.ttl, pub.feat, pub.net_idx);
}

void app_mesh_cfg_hb_pub_set(uint16_t net_idx, uint16_t dst, struct bt_mesh_cfg_cli_hb_pub *pub)
{
    uint8_t status;
    int err;

    err = bt_mesh_cfg_cli_hb_pub_set(net_idx, dst, pub, &status);
    if (err) {
        app_print("Heartbeat Publication Set failed (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Heartbeat Publication Set failed (status 0x%02x)\r\n", status);
    } else {
        app_print("Heartbeat publication successfully set\r\n");
    }
}

static void app_mesh_cfg_hb_sub_print(struct bt_mesh_cfg_cli_hb_sub *sub)
{
    app_print(\
              "Heartbeat Subscription:\r\n"       \
              "\tSource:      0x%04x\r\n"         \
              "\tDestination: 0x%04x\r\n"         \
              "\tPeriodLog:   0x%02x\r\n"         \
              "\tCountLog:    0x%02x\r\n"         \
              "\tMinHops:     %u\r\n"             \
              "\tMaxHops:     %u\r\n",
              sub->src, sub->dst, sub->period, sub->count, sub->min, sub->max);
}

void app_mesh_cfg_hb_sub_get(uint16_t net_idx, uint16_t dst)
{
    struct bt_mesh_cfg_cli_hb_sub sub;
    uint8_t status;
    int err;

    err = bt_mesh_cfg_cli_hb_sub_get(net_idx, dst, &sub, &status);
    if (err) {
        app_print("Heartbeat Subscription Get failed (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Heartbeat Subscription Get failed (status 0x%02x)\r\n", status);
    } else {
        app_mesh_cfg_hb_sub_print(&sub);
    }
}

void app_mesh_cfg_hb_sub_set(uint16_t net_idx, uint16_t dst, struct bt_mesh_cfg_cli_hb_sub *sub)
{
    uint8_t status;
    int err = 0;

    err = bt_mesh_cfg_cli_hb_sub_set(net_idx, dst, sub, &status);
    if (err) {
        app_print("Heartbeat Subscription Set failed (err %d)\r\n", err);
        return;
    }

    if (status) {
        app_print("Heartbeat Subscription Set failed (status 0x%02x)\r\n", status);
    } else {
        app_mesh_cfg_hb_sub_print(sub);
    }
}


static void app_cfg_cli_comp_data(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t page,
                                  struct net_buf_simple *buf)
{
    app_print("app_cfg_cli_comp_data addr 0x%x, page %d, length %d\r\n", addr, page, buf->len);
}


static void app_cfg_cli_mod_pub_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                       uint16_t elem_addr, uint16_t mod_id, uint16_t cid,
                                       struct bt_mesh_cfg_cli_mod_pub *pub)
{
    app_print(
        "Model Publication for Element 0x%04x, Model 0x%04x:\r\n" \
        "\tPublish Address:                0x%04x\r\n"            \
        "\tAppKeyIndex:                    0x%04x\r\n"            \
        "\tCredential Flag:                %u\r\n"                \
        "\tPublishTTL:                     %u\r\n"                \
        "\tPublishPeriod:                  0x%02x\r\n"            \
        "\tPublishRetransmitCount:         %u\r\n"                \
        "\tPublishRetransmitInterval:      %ums\r\n",
        addr, mod_id, pub->addr, pub->app_idx, pub->cred_flag, pub->ttl, pub->period,
        BT_MESH_PUB_TRANSMIT_COUNT(pub->transmit),
        BT_MESH_PUB_TRANSMIT_INT(pub->transmit));
}

static void app_cfg_cli_mod_sub_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                       uint16_t elem_addr,
                                       uint16_t sub_addr, uint32_t mod_id)
{
    app_print("app_cfg_cli_mod_sub_status addr 0x%x, status %d elem_addr 0x%x, sub_addr 0x%x, mod_id 0x%x\r\n",
              addr, status, elem_addr, sub_addr, mod_id);

}

static void app_cfg_cli_mod_sub_list(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                     uint16_t elem_addr, uint16_t mod_id, uint16_t cid,
                                     struct net_buf_simple *buf)
{
    if (status) {
        app_print("Model Subscription Get failed with status 0x%02x\r\n", status);
    } else {
        app_print("Model Subscriptions for Addr 0x%04x, Element 0x%04x, Model 0x%04x 0x%x:", addr,
                  elem_addr, mod_id, cid);

    }

    if (buf->len == 0) {
        app_print("\tNone.\r\n");
    }

    while (buf->len) {
        app_print("\t0x%04x", net_buf_simple_pull_le16(buf));
    }
}

static void app_cfg_cli_node_reset_status(struct bt_mesh_cfg_cli *cli, uint16_t addr)
{
}

static void app_cfg_cli_beacon_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status)
{
    app_print("app_cfg_cli_beacon_status addr 0x%x, status %d\r\n", addr, status);
}

static void app_cfg_cli_ttl_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status)
{
    app_print("app_cfg_cli_ttl_status addr 0x%x, Default TTL %d\r\n", addr, status);
}

static void app_cfg_cli_friend_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status)
{
    app_print("app_cfg_cli_friend_status addr 0x%x, status %d\r\n", addr, status);
}

static void app_cfg_cli_gatt_proxy_status(struct bt_mesh_cfg_cli *cli, uint16_t addr,
                                          uint8_t status)
{
    app_print("app_cfg_cli_gatt_proxy_status addr 0x%x, proxy %d\r\n", addr, status);
}

static void app_cfg_cli_network_transmit_status(struct bt_mesh_cfg_cli *cli, uint16_t addr,
                                                uint8_t status)
{
    app_print("app_cfg_cli_network_transmit_status addr 0x%x\r\n", addr);
    app_print("Transmit 0x%02x (count %u interval %ums)\r\n", status,
              BT_MESH_TRANSMIT_COUNT(status), BT_MESH_TRANSMIT_INT(status));
}

static void app_cfg_cli_relay_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                     uint8_t transmit)
{
    app_print("app_cfg_cli_relay_status addr 0x%x, Relay is 0x%02x, Transmit 0x%02x (count %u interval %ums)\r\n",
              addr, transmit, BT_MESH_TRANSMIT_COUNT(transmit), BT_MESH_TRANSMIT_INT(transmit));
}

static void app_cfg_cli_net_key_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                       uint16_t net_idx)
{
    app_print("app_cfg_cli_net_key_status addr 0x%x, status is 0x%02x, net_idx 0x%02x \r\n", addr,
              status, net_idx);
}

static void app_cfg_cli_net_key_list(struct bt_mesh_cfg_cli *cli, uint16_t addr,
                                     struct net_buf_simple *buf)
{
    uint16_t keys[16];
    size_t cnt = 16;
    int err, i;
    err = bt_mesh_key_idx_unpack_list(buf, keys, &cnt);

    if (err) {
        app_print("The message size for the application opcode is incorrect.\r\n");
        return;
    }

    app_print("NetKeys known by 0x%04x:\r\n", addr);
    for (i = 0; i < cnt; i++) {
        app_print("\t0x%03x\r\n", keys[i]);
    }
}

static void app_cfg_cli_app_key_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                       uint16_t net_idx, uint16_t app_idx)
{
    app_print("app_cfg_cli_app_key_status addr 0x%04x:\r\n", addr);
    if (status) {
        app_print("AppKeyAdd failed with status 0x%02x\r\n", status);
    } else {
        app_print("AppKey added, NetKeyIndex 0x%04x AppKeyIndex 0x%04x\r\n", net_idx, app_idx);
    }
}

static void app_cfg_cli_app_key_list(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                     uint16_t net_idx, struct net_buf_simple *buf)
{
    uint16_t keys[16];
    size_t cnt = 16;
    int err, i;
    err = bt_mesh_key_idx_unpack_list(buf, keys, &cnt);

    if (err) {
        app_print("The message size for the application opcode is incorrect.\r\n");
        return;
    }

    app_print("AppKeys for NetKey 0x%03x known by 0x%04x: \r\n", net_idx, addr);
    for (i = 0; i < cnt; i++) {
        app_print("\t0x%03x\r\n", keys[i]);
    }

}

static void app_cfg_cli_mod_app_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                       uint16_t elem_addr,
                                       uint16_t app_idx, uint32_t mod_id)
{
}

static void app_cfg_cli_mod_app_list(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                     uint16_t elem_addr, uint16_t mod_id, uint16_t cid,
                                     struct net_buf_simple *buf)
{
}

static void app_cfg_cli_node_identity_status(struct bt_mesh_cfg_cli *cli, uint16_t addr,
                                             uint8_t status, uint16_t net_idx, uint8_t identity)
{
}

static void app_cfg_cli_lpn_timeout_status(struct bt_mesh_cfg_cli *cli, uint16_t addr,
                                           uint16_t elem_addr, uint32_t timeout)
{
}

static void app_cfg_cli_krp_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                   uint16_t net_idx, uint8_t phase)
{
}

static void app_cfg_cli_hb_pub_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                      struct bt_mesh_cfg_cli_hb_pub *pub)
{
    app_print("app_cfg_cli_hb_pub_status addr 0x%x:\r\n", addr);
    app_print("\tdst 0x%04x count 0x%02x period 0x%02x\r\n", pub->dst, pub->count, pub->period);
    app_print("\tttl 0x%02x feat 0x%04x net_idx 0x%04x\r\n", pub->ttl, pub->feat, pub->net_idx);

}

static void app_cfg_cli_hb_sub_status(struct bt_mesh_cfg_cli *cli, uint16_t addr, uint8_t status,
                                      struct bt_mesh_cfg_cli_hb_sub *sub)
{
    app_print("app_cfg_cli_hb_sub_status addr 0x%x status 0x%x:\r\n", addr, status);
    app_mesh_cfg_hb_sub_print(sub);
}


