/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*!
    \file    app_mesh_rpr.c
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
#include "app_mesh_rpr.h"

static void app_mesh_rpr_scan_report(struct bt_mesh_rpr_cli *cli,
                                     const struct bt_mesh_rpr_node *srv,
                                     struct bt_mesh_rpr_unprov *unprov,
                                     struct net_buf_simple *adv_data);

struct bt_mesh_rpr_cli app_rpr_cli = {
    .scan_report = app_mesh_rpr_scan_report,
};

static void app_mesh_rpr_scan_report(struct bt_mesh_rpr_cli *cli,
                                     const struct bt_mesh_rpr_node *srv,
                                     struct bt_mesh_rpr_unprov *unprov,
                                     struct net_buf_simple *adv_data)
{
    char uuid_hex_str[32 + 1];

    bin2hex(unprov->uuid, 16, uuid_hex_str, sizeof(uuid_hex_str));

    app_print(
        "Server 0x%04x:\r\n"
        "\tuuid:   %s\r\n"
        "\tOOB:    0x%04x\r\n",
        srv->addr, uuid_hex_str, unprov->oob);

    while (adv_data && adv_data->len > 2) {
        uint8_t len, type;
        uint8_t data[31];

        len = net_buf_simple_pull_u8(adv_data);
        if (len == 0) {
            /* No data in this AD Structure. */
            continue;
        }

        if (len > adv_data->len) {
            /* Malformed AD Structure. */
            break;
        }

        type = net_buf_simple_pull_u8(adv_data);
        if ((--len) > 0) {
            uint8_t dlen;

            /* Pull all length, but print only what fits into `data` array. */
            dlen = MIN(len, sizeof(data) - 1);
            memcpy(data, net_buf_simple_pull_mem(adv_data, len), dlen);
            len = dlen;
        }
        data[len] = '\0';

        if (type == BT_DATA_URI) {
            app_print("\tURI:    \"\\x%02x%s\"\r\n",
                      data[0], &data[1]);
        } else if (type == BT_DATA_NAME_COMPLETE) {
            app_print("\tName:   \"%s\"\r\n", data);
        } else {
            char string[64 + 1];

            bin2hex(data, len, string, sizeof(string));
            app_print("\t0x%02x:  %s\r\n", type, string);
        }
    }
}


int app_mesh_rpr_scan(struct bt_mesh_rpr_node *srv, uint8_t *uuid, uint8_t timeout)
{
    struct bt_mesh_rpr_scan_status rsp;
    int err = 0;

    err = bt_mesh_rpr_scan_start(&app_rpr_cli, srv, uuid, timeout, BT_MESH_RPR_SCAN_MAX_DEVS_ANY, &rsp);

    if (err) {
        app_print("rpr Scan start failed: %d \r\n", err);
        return err;
    }

    if (rsp.status == BT_MESH_RPR_SUCCESS) {
        app_print("rpr Scan started.\r\n");
    } else {
        app_print("rpr Scan start response: %d\r\n", rsp.status);
    }

    return 0;
}


int app_mesh_rpr_scan_ext(struct bt_mesh_rpr_node *srv, uint8_t timeout, uint8_t uuid[16],
                          uint8_t *ad_types, size_t ad_count)
{
    int err = 0;

    err = bt_mesh_rpr_scan_start_ext(&app_rpr_cli, srv, uuid, timeout, ad_types, ad_count);
    if (err) {
        app_print("rpr Scan start failed: %d\r\n", err);
        return err;
    }

    app_print("rpr Extended scan started.\r\n");

    return 0;
}

int app_mesh_rpr_scan_caps(struct bt_mesh_rpr_node *srv)
{
    struct bt_mesh_rpr_caps caps;
    int err;

    err = bt_mesh_rpr_scan_caps_get(&app_rpr_cli, srv, &caps);
    if (err) {
        app_print("rpr Scan capabilities get failed: %d\r\n", err);
        return err;
    }

    app_print("Remote Provisioning scan capabilities of 0x%04x:\r\n", srv->addr);
    app_print("\tMax devices:     %u\r\n", caps.max_devs);
    app_print("\tActive scanning: %s\r\n", caps.active_scan ? "true" : "false");
    return 0;
}

int app_mesh_rpr_scan_get(struct bt_mesh_rpr_node *srv)
{
    struct bt_mesh_rpr_scan_status rsp;
    int err;

    err = bt_mesh_rpr_scan_get(&app_rpr_cli, srv, &rsp);
    if (err) {
        app_print("Scan get failed: %d\r\n", err);
        return err;
    }

    app_print("Remote Provisioning scan on 0x%04x:\r\n", srv->addr);
    app_print("\tStatus:         %u\r\n", rsp.status);
    app_print("\tScan type:      %u\r\n", rsp.scan);
    app_print("\tMax devices:    %u\r\n", rsp.max_devs);
    app_print("\tRemaining time: %u\r\n", rsp.timeout);
    return 0;
}

int app_mesh_rpr_scan_stop(struct bt_mesh_rpr_node *srv)
{
    struct bt_mesh_rpr_scan_status rsp;
    int err;


    err = bt_mesh_rpr_scan_stop(&app_rpr_cli, srv, &rsp);
    if (err || rsp.status) {
        app_print("Scan stop failed: %d %u\r\n", err, rsp.status);
        return err;
    }

    app_print("Remote Provisioning scan on 0x%04x stopped.\r\n", srv->addr);
    return 0;
}

int app_mesh_rpr_link_get(struct bt_mesh_rpr_node *srv)
{
    struct bt_mesh_rpr_link rsp;
    int err;

    err = bt_mesh_rpr_link_get(&app_rpr_cli, srv, &rsp);
    if (err) {
        app_print("Link get failed: %d %u\r\n", err, rsp.status);
        return err;
    }

    app_print("Remote Provisioning Link on 0x%04x:\r\n", srv->addr);
    app_print("\tStatus: %u\r\n", rsp.status);
    app_print("\tState:  %u\r\n", rsp.state);
    return 0;
}

int app_mesh_rpr_link_close(struct bt_mesh_rpr_node *srv)
{
    struct bt_mesh_rpr_link rsp;
    int err;

    err = bt_mesh_rpr_link_close(&app_rpr_cli, srv, &rsp);
    if (err) {
        app_print("Link close failed: %d %u\r\n", err, rsp.status);
        return err;
    }

    app_print("Remote Provisioning Link on 0x%04x:\r\n", srv->addr);
    app_print("\tStatus: %u\r\n", rsp.status);
    app_print("\tState:  %u\r\n", rsp.state);
    return 0;
}

int app_mesh_rpr_provision_remote(struct bt_mesh_rpr_node *srv, uint8_t uuid[16], uint16_t net_idx,
                                  uint16_t addr)
{
    int err = 0;

    err = bt_mesh_provision_remote(&app_rpr_cli, srv, uuid, net_idx, addr);
    if (err) {
        app_print("Prov remote start failed: %d\r\n", err);
    }

    return err;
}

int app_mesh_rpr_reprovision_remote(struct bt_mesh_rpr_node *srv, uint16_t addr,
                                    bool composition_changed)
{
    int err = 0;

    if (!BT_MESH_ADDR_IS_UNICAST(addr)) {
        app_print("Must be a valid unicast address\r\n");
        return -EINVAL;
    }


    err = bt_mesh_reprovision_remote(&app_rpr_cli, srv, addr, composition_changed);
    if (err) {
        app_print("Reprovisioning failed: %d\r\n", err);
    }

    return 0;
}

