/*!
    \file    cmd_mesh_rpr.c
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
#include "mesh_cfg.h"
#include "app_cfg.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ble_export.h"
#include "cmd_shell.h"
#include "api/mesh.h"
#include "dbg_print.h"
#include "app_mesh_rpr.h"
#include "cmd_mesh_rpr.h"

static struct bt_mesh_rpr_node rpr_node_srv;

void cmd_ble_mesh_rpr_set_srv(int argc, char **argv)
{
    rpr_node_srv.addr = (uint16_t)strtoul((const char *)argv[1], NULL, 0);
    rpr_node_srv.net_idx = (uint16_t)strtoul((const char *)argv[2], NULL, 0);
    rpr_node_srv.ttl = BT_MESH_TTL_DEFAULT;

}

void cmd_ble_mesh_rpr_scan(int argc, char **argv)
{
    uint8_t uuid[16] = {0};
    uint8_t timeout;

    timeout = (uint8_t)strtoul((const char *)argv[1], NULL, 0);

    if (argc > 2) {
        hex2bin(argv[2], strlen(argv[2]), uuid, 16);
    }

    app_mesh_rpr_scan(&rpr_node_srv, argc > 2 ? uuid : NULL, timeout);
}


void cmd_ble_mesh_rpr_scan_ext(int argc, char **argv)
{
    uint8_t ad_types[CONFIG_BT_MESH_RPR_AD_TYPES_MAX];
    uint8_t uuid[16] = {0};
    uint8_t timeout;
    int i;

    timeout = strtoul(argv[1], NULL, 0);

    hex2bin(argv[2], strlen(argv[2]), uuid, 16);

    for (i = 0; i < argc - 3; i++) {
        ad_types[i] = strtoul(argv[3 + i], NULL, 0);
    }

    app_mesh_rpr_scan_ext(&rpr_node_srv, timeout, uuid, ad_types, argc - 3);
}

void cmd_ble_mesh_rpr_scan_srv(int argc, char **argv)
{
    uint8_t ad_types[CONFIG_BT_MESH_RPR_AD_TYPES_MAX];
    uint8_t timeout;
    int i;

    timeout = strtoul(argv[1], NULL, 0);

    for (i = 0; i < argc - 2; i++) {
        ad_types[i] = strtoul(argv[2 + i], NULL, 0);
    }

    app_mesh_rpr_scan_ext(&rpr_node_srv, 0, NULL, ad_types, (argc - 2));
}

void cmd_ble_mesh_rpr_scan_caps(int argc, char **argv)
{
    app_mesh_rpr_scan_caps(&rpr_node_srv);
}

void cmd_ble_mesh_rpr_scan_get(int argc, char **argv)
{
    app_mesh_rpr_scan_get(&rpr_node_srv);
}

void cmd_ble_mesh_rpr_scan_stop(int argc, char **argv)
{
    app_mesh_rpr_scan_stop(&rpr_node_srv);
}

void cmd_ble_mesh_rpr_link_get(int argc, char **argv)
{
    app_mesh_rpr_link_get(&rpr_node_srv);
}

void cmd_ble_mesh_rpr_link_close(int argc, char **argv)
{
    app_mesh_rpr_link_close(&rpr_node_srv);
}

void cmd_ble_mesh_rpr_provision_remote(int argc, char **argv)
{
    uint8_t uuid[16];
    size_t len;
    uint16_t net_idx;
    uint16_t addr;

    len = hex2bin(argv[1], strlen(argv[1]), uuid, sizeof(uuid));
    (void)memset(uuid + len, 0, sizeof(uuid) - len);

    net_idx = strtoul(argv[2], NULL, 0);
    addr = strtoul(argv[3], NULL, 0);

    app_mesh_rpr_provision_remote(&rpr_node_srv, uuid, net_idx, addr);
}

void cmd_ble_mesh_rpr_reprovision_remote(int argc, char **argv)
{
    bool composition_changed;
    uint16_t addr;

    addr = strtoul(argv[1], NULL, 0);
    composition_changed = strtoul(argv[2], NULL, 0);

    app_mesh_rpr_reprovision_remote(&rpr_node_srv, addr, composition_changed);
}


