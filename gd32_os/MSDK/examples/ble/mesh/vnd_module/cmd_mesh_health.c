/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*!
    \file    cmd_mesh_health.c
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
#include "app_mesh.h"
#include "cmd_mesh_health.h"

#define BT_COMP_ID_LF           0x05f1 /**< The Linux Foundation */

/** Maximum number of faults the health server can have. */
#define BT_MESH_SHELL_CUR_FAULTS_MAX        4

static uint8_t cur_faults[BT_MESH_SHELL_CUR_FAULTS_MAX];
static uint8_t reg_faults[BT_MESH_SHELL_CUR_FAULTS_MAX * 2];

static const struct bt_mesh_model *mod;
extern const struct bt_mesh_comp *bt_mesh_comp_get(void);

bool bt_mesh_shell_mdl_first_get(uint16_t id, const struct bt_mesh_model **mod)
{
    const struct bt_mesh_comp *comp = bt_mesh_comp_get();

    for (int i = 0; i < comp->elem_count; i++) {
        *mod = bt_mesh_model_find(&comp->elem[i], id);
        if (*mod) {
            return true;
        }
    }

    return false;
}

static const struct bt_mesh_elem *primary_element(void)
{
    const struct bt_mesh_comp *comp = bt_mesh_comp_get();

    if (comp) {
        return &comp->elem[0];
    }

    return NULL;
}

static void get_faults(uint8_t *faults, uint8_t faults_size, uint8_t *dst, uint8_t *count)
{
    uint8_t i, limit = *count;

    for (i = 0U, *count = 0U; i < faults_size && *count < limit; i++) {
        if (faults[i]) {
            *dst++ = faults[i];
            (*count)++;
        }
    }
}

static int fault_get_cur(const struct bt_mesh_model *model, uint8_t *test_id,
             uint16_t *company_id, uint8_t *faults, uint8_t *fault_count)
{
    app_print("Sending current faults\r\n");

    *test_id = 0x00;
    *company_id = BT_COMP_ID_LF;

    get_faults(cur_faults, sizeof(cur_faults), faults, fault_count);

    return 0;
}

static int fault_get_reg(const struct bt_mesh_model *model, uint16_t cid,
             uint8_t *test_id, uint8_t *faults, uint8_t *fault_count)
{
    if (cid != BT_COMP_ID_LF) {
        app_print("Faults requested for unknown Company ID 0x%04x\r\n", cid);
        return -EINVAL;
    }

    app_print("Sending registered faults\r\n");

    *test_id = 0x00;

    get_faults(reg_faults, sizeof(reg_faults), faults, fault_count);

    return 0;
}

static int fault_clear(const struct bt_mesh_model *model, uint16_t cid)
{
    if (cid != BT_COMP_ID_LF) {
        return -EINVAL;
    }

    (void)memset(reg_faults, 0, sizeof(reg_faults));

    return 0;
}

static int fault_test(const struct bt_mesh_model *model, uint8_t test_id,
              uint16_t cid)
{
    if (cid != BT_COMP_ID_LF) {
        return -EINVAL;
    }

    if (test_id != 0x00) {
        return -EINVAL;
    }

    return 0;
}

static void attention_on(const struct bt_mesh_model *model)
{
    app_print("Attention On\r\n");
}

static void attention_off(const struct bt_mesh_model *model)
{
    app_print("Attention Off\r\n");
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
    .fault_get_cur = fault_get_cur,
    .fault_get_reg = fault_get_reg,
    .fault_clear = fault_clear,
    .fault_test = fault_test,
    .attn_on = attention_on,
    .attn_off = attention_off,
};

#if CONFIG_BT_MESH_LARGE_COMP_DATA_SRV
static uint8_t health_tests[] = {
    BT_MESH_HEALTH_TEST_INFO(BT_COMP_ID_LF, 6, 0x01, 0x02, 0x03, 0x04, 0x34,
                             0x15),
};

static const uint8_t zero_metadata[100];

struct bt_mesh_models_metadata_entry health_srv_meta[] = {
    BT_MESH_HEALTH_TEST_INFO_METADATA(health_tests),
    {
        .len = ARRAY_SIZE(zero_metadata),
        .id = 0xABCD,
        .data = zero_metadata,
    },
    BT_MESH_MODELS_METADATA_END,
};
#endif

struct bt_mesh_health_srv bt_mesh_shell_health_srv = {
    .cb = &health_srv_cb,
};

void cmd_ble_mesh_add_fault(int argc, char **argv)
{
    uint8_t i;
    const struct bt_mesh_elem *elem;
    int err = 0;
    uint8_t fault_id;

    if (argc != 2) {
        goto usage;
    }

    fault_id = (uint8_t)strtoul((const char *)argv[1], NULL, 0);

    elem = primary_element();
    if (elem == NULL) {
        app_print("Element not found!\r\n");
        return;
    }

    if (!fault_id) {
        app_print("The Fault ID must be non-zero!\r\n");
        return;
    }

    for (i = 0U; i < sizeof(cur_faults); i++) {
        if (!cur_faults[i]) {
            cur_faults[i] = fault_id;
            break;
        }
    }

    if (i == sizeof(cur_faults)) {
        app_print("Fault array is full. Use \"del-fault\" to clear it\r\n");
        return;
    }

    for (i = 0U; i < sizeof(reg_faults); i++) {
        if (!reg_faults[i]) {
            reg_faults[i] = fault_id;
            break;
        }
    }

    if (i == sizeof(reg_faults)) {
        app_print("No space to store more registered faults\r\n");
    }

    bt_mesh_health_srv_fault_update(elem);
    return;
usage:
    app_print("Usage: mesh_add_fault <fault_id>\r\n");
}

void cmd_ble_mesh_del_fault(int argc, char **argv)
{
    uint8_t fault_id;
    uint8_t all = true;
    uint8_t i;
    const struct bt_mesh_elem *elem;
    int err = 0;

    if (argc > 2) {
        goto usage;
    }

    if (argc == 2) {
        fault_id = (uint8_t)strtoul((const char *)argv[1], NULL, 0);
        all = false;
    }

    elem = primary_element();
    if (elem == NULL) {
        app_print("Element not found!\r\n");
        return;
    }

    if (all) {
        (void)memset(cur_faults, 0, sizeof(cur_faults));
        app_print("All current faults cleared\r\n");
        bt_mesh_health_srv_fault_update(elem);
        return;
    }

    if (!fault_id) {
        app_print("The Fault ID must be non-zero!\r\n");
        return;
    }

    for (i = 0U; i < sizeof(cur_faults); i++) {
        if (cur_faults[i] == fault_id) {
            cur_faults[i] = 0U;
            app_print("Fault cleared\r\n");
        }
    }

    bt_mesh_health_srv_fault_update(elem);
    return;
usage:
    app_print("Usage: mesh_add_fault [fault_id]\r\n");
}
