/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mesh_cfg.h"
#include <string.h>
#include <stdlib.h>
#include "nvds_flash.h"
#include "api/mesh_log.h"

#define MESH_LOG_NAMESPACE        "BLE_MESH_LOG"
#define MESH_LOG_KEY_NAME         "LOG_LEVEL"
uint8_t mesh_log_mask[(CONFIG_BT_MESH_MAX_LOG_LEVEL + 1) >> 1] = {0};

void mesh_log_init(void)
{
    uint32_t len = sizeof(mesh_log_mask);
    int ret = nvds_data_get(NULL, MESH_LOG_NAMESPACE, MESH_LOG_KEY_NAME, mesh_log_mask, (uint32_t *)&len);

    if (ret != NVDS_OK) {
        LOG_ERR("mesh_log_init no log level property");
        memset(mesh_log_mask, 0x33, sizeof(mesh_log_mask));
    }
}

void mesh_log_set_dbg_level(uint16_t mask, uint8_t level)
{
    int ret;
    if (mask % 2) {
        level = (level << 4) & 0xF0;
        mesh_log_mask[mask >> 1] = (mesh_log_mask[mask >> 1] & 0x0F) | level;
    }
    else {
        level = level & 0x0F;
        mesh_log_mask[mask >> 1] = (mesh_log_mask[mask >> 1] & 0xF0) | level;
    }

    ret = nvds_data_put(NULL, MESH_LOG_NAMESPACE, MESH_LOG_KEY_NAME, mesh_log_mask, sizeof(mesh_log_mask));

    if (ret != NVDS_OK) {
        LOG_ERR("mesh_log_init set log level property fail");
    }
}
