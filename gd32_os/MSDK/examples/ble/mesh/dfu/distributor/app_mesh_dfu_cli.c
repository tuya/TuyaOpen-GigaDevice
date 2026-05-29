/*!
    \file    app_mesh_dfu_cli.c
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
#include "api/settings.h"
#include "nvds_flash.h"
#include "app_mesh_dfu_cli.h"
#include "src/dfu_slot.h"
#include "config_gdm32.h"
#include "raw_flash_api.h"
#include "rom_export_mbedtls.h"
#include "rom_export.h"

#define APP_DFD_FWID          "GD_IMAGE0"
#define APP_META_DATA         "Local image"
#define READ_IMG_SIZE         512
#define IMAGE_SZIE            (RE_IMG_1_OFFSET - RE_IMG_0_OFFSET)
#define TRANS_MAX_IMAGE_SIZE  164352
//#define TRANS_MAX_IMAGE_SIZE  1024

static uint8_t checkdata[32] = {0};
static uint32_t dfu_backup_img_offset = RE_IMG_1_OFFSET;

static struct
{
    struct bt_mesh_dfu_target targets[32];
    struct bt_mesh_blob_target_pull pull[32];
    size_t target_cnt;
    struct bt_mesh_blob_cli_inputs inputs;
} app_dfu_tx;

static int app_dfd_srv_recv(struct bt_mesh_dfd_srv *srv,
                            const struct bt_mesh_dfu_slot *slot,
                            const struct bt_mesh_blob_io **io);

static void app_dfd_srv_del(struct bt_mesh_dfd_srv *srv,
                            const struct bt_mesh_dfu_slot *slot);


static int app_dfd_srv_send(struct bt_mesh_dfd_srv *srv,
                            const struct bt_mesh_dfu_slot *slot,
                            const struct bt_mesh_blob_io **io);

static int app_blob_io_open(const struct bt_mesh_blob_io *io,
                            const struct bt_mesh_blob_xfer *xfer,
                            enum bt_mesh_blob_io_mode mode);

static int app_blob_chunk_rd(const struct bt_mesh_blob_io *io,
                             const struct bt_mesh_blob_xfer *xfer,
                             const struct bt_mesh_blob_block *block,
                             const struct bt_mesh_blob_chunk *chunk);


static struct bt_mesh_dfd_srv_cb app_dfd_srv_cb = {
    .recv = app_dfd_srv_recv,
    .del = app_dfd_srv_del,
    .send = app_dfd_srv_send,
};

struct bt_mesh_dfd_srv app_dfd_srv = BT_MESH_DFD_SRV_INIT(&app_dfd_srv_cb);

static const struct bt_mesh_blob_io app_blob_io = {
    .open = app_blob_io_open,
    .rd = app_blob_chunk_rd,
};


/* DFD Model data*/
static int app_dfd_srv_recv(struct bt_mesh_dfd_srv *srv,
                            const struct bt_mesh_dfu_slot *slot,
                            const struct bt_mesh_blob_io **io)
{
    app_print("Uploading new firmware image to the distributor.\r\n");

    *io = &app_blob_io;

    return 0;
}

static void app_dfd_srv_del(struct bt_mesh_dfd_srv *srv,
                            const struct bt_mesh_dfu_slot *slot)
{
    app_print("Deleting the firmware image from the distributor.\r\n");
}

static int app_dfd_srv_send(struct bt_mesh_dfd_srv *srv,
                            const struct bt_mesh_dfu_slot *slot,
                            const struct bt_mesh_blob_io **io)
{
    app_print("Starting the firmware distribution.\r\n");

    *io = &app_blob_io;

    return 0;
}


static int app_blob_io_open(const struct bt_mesh_blob_io *io,
                            const struct bt_mesh_blob_xfer *xfer,
                            enum bt_mesh_blob_io_mode mode)
{
    app_print("app_blob_io_open mode %u.\r\n", mode);

    return 0;
}

static int app_blob_chunk_rd(const struct bt_mesh_blob_io *io,
                             const struct bt_mesh_blob_xfer *xfer,
                             const struct bt_mesh_blob_block *block,
                             const struct bt_mesh_blob_chunk *chunk)
{
    int err;
    uint32_t offset = dfu_backup_img_offset + block->offset + chunk->offset;
    uint32_t size = TRANS_MAX_IMAGE_SIZE > IMAGE_SZIE ? IMAGE_SZIE : TRANS_MAX_IMAGE_SIZE;

    app_print("chunk rd block->offset 0x%x, chunk->offset: 0x%x\r\n", block->offset, chunk->offset);

    if ((block->number % 100) == 0 && chunk->offset == 0) {
        app_print("chunk rd 0x%x, number: %u, chunk size: %u\r\n", offset, block->number, chunk->size);
    }

    if ((block->offset + chunk->offset) >= size) {
        uint32_t checkdata_offset = block->offset + chunk->offset - size ;
        uint32_t copy_size = (32 - checkdata_offset) > chunk->size ? chunk->size : 32;
        app_print("chunk rd number: %u, chunk size: %u, copy_size %u, checkdata_offset %u\r\n",
                  block->number, chunk->size, copy_size, checkdata_offset);
        memcpy(chunk->data, &checkdata[checkdata_offset], copy_size);
    } else if ((block->offset + chunk->offset + chunk->size) > size) {
        uint32_t copy_image_size = size  - block->offset - chunk->offset;
        uint32_t copy_checkdata_size = (chunk->size - copy_image_size) > 32 ? 32 :
                                       (chunk->size - copy_image_size);
        app_print("chunk rd number: %u, chunk size: %u, copy_image_size %u, copy_checkdata_size %u\r\n",
                  block->number, chunk->size, copy_image_size, copy_checkdata_size);
        err = raw_flash_read(offset, chunk->data, copy_image_size);
        if (err < 0) {
            app_print("flash read fail\r\n");
            return 0;
        }
        memcpy(&chunk->data[copy_image_size], checkdata, copy_checkdata_size);
    } else {
        err = raw_flash_read(offset, chunk->data, chunk->size);
        if (err < 0) {
            app_print("flash read fail\r\n");
            return 0;
        }
    }

    //debug_print_dump_data("chunk: ", (char*)chunk->data, chunk->size);
    return 0;
}

static void app_dfu_tx_prepare(void)
{
    sys_slist_init(&app_dfu_tx.inputs.targets);

    for (int i = 0; i < app_dfu_tx.target_cnt; i++) {
        /* Reset target context. */
        uint16_t addr = app_dfu_tx.targets[i].blob.addr;

        memset(&app_dfu_tx.targets[i].blob, 0,
               sizeof(struct bt_mesh_blob_target));
        memset(&app_dfu_tx.pull[i], 0,
               sizeof(struct bt_mesh_blob_target_pull));
        app_dfu_tx.targets[i].blob.addr = addr;
        app_dfu_tx.targets[i].blob.pull = &app_dfu_tx.pull[i];

        sys_slist_append(&app_dfu_tx.inputs.targets,
                         &app_dfu_tx.targets[i].blob.n);
    }
}

static void app_dfu_target(uint8_t img_idx, uint16_t addr)
{
    if (app_dfu_tx.target_cnt == ARRAY_SIZE(app_dfu_tx.targets)) {
        app_print("No room. \r\n");
        return;
    }

    for (int i = 0; i < app_dfu_tx.target_cnt; i++) {
        if (app_dfu_tx.targets[i].blob.addr == addr) {
            app_print("Target 0x%04x already exists\r\n", addr);
            return;
        }
    }

    app_dfu_tx.targets[app_dfu_tx.target_cnt].blob.addr = addr;
    app_dfu_tx.targets[app_dfu_tx.target_cnt].img_idx = img_idx;
    sys_slist_append(&app_dfu_tx.inputs.targets,
                     &app_dfu_tx.targets[app_dfu_tx.target_cnt].blob.n);
    app_dfu_tx.target_cnt++;

    app_print("Added target 0x%04x\r\n", addr);
}

void app_dfu_firmware_update_start(uint8_t slot_idx, uint8_t addr_cnt, uint16_t *addrs)
{
    struct bt_mesh_dfu_cli_xfer xfer;
    int err = 0;
    int i = 0;
    uint16_t appkey_idx;

    for (i = 0; i < app_dfd_srv.dfu.mod->keys_cnt; i++) {
        if (app_dfd_srv.dfu.mod->keys[i] != BT_MESH_KEY_UNUSED) {
            break;
        }
    }

    if (i == app_dfd_srv.dfu.mod->keys_cnt) {
        app_print("Model not bind with app key\r\n");
        return;
    } else {
        appkey_idx = app_dfd_srv.dfu.mod->keys[i];
    }

    xfer.mode = BT_MESH_BLOB_XFER_MODE_PUSH;
    xfer.blob_params = NULL;

    xfer.slot = bt_mesh_dfu_slot_at(slot_idx);
    if (!xfer.slot) {
        app_print("No image in slot %u \r\n", slot_idx);
        return;
    }

    for (i = 0; i < addr_cnt; i++) {
        app_dfu_target(slot_idx, addrs[i]);
    }

    app_dfu_tx_prepare();

    if (!app_dfu_tx.target_cnt) {
        app_print("No targets. \r\n");
        return;
    }

    if (addr_cnt > 1) {
        app_dfu_tx.inputs.group = BT_MESH_ADDR_UNASSIGNED;
    } else {
        app_dfu_tx.inputs.group = *addrs;
    }

    app_dfu_tx.inputs.app_idx = appkey_idx;
    app_dfu_tx.inputs.ttl = BT_MESH_TTL_DEFAULT;

    err = bt_mesh_dfu_cli_send(&app_dfd_srv.dfu, &app_dfu_tx.inputs, &app_blob_io, &xfer);

    if (err) {
        app_print("err %d\r\n", err);
        return;
    }

}

void app_dfu_firmware_update_apply(void)
{
    int err;

    err = bt_mesh_dfu_cli_apply(&app_dfd_srv.dfu);
    if (err) {
        app_print("err %d\r\n", err);
    }
}

void app_dfu_firmware_update_get(uint16_t net_idx, uint16_t addr)
{
    struct bt_mesh_msg_ctx ctx = {
        .net_idx = net_idx,
        .send_ttl = BT_MESH_TTL_DEFAULT,
    };
    struct bt_mesh_dfu_target_status rsp_data;
    int err, i = 0;
    uint16_t appkey_idx;

    for (i = 0; i < app_dfd_srv.dfu.mod->keys_cnt; i++) {
        if (app_dfd_srv.dfu.mod->keys[i] != BT_MESH_KEY_UNUSED) {
            break;
        }
    }

    if (i == app_dfd_srv.dfu.mod->keys_cnt) {
        app_print("Model not bind with app key\r\n");
        return;
    } else {
        appkey_idx = app_dfd_srv.dfu.mod->keys[i];
    }


    ctx.addr = addr;
    ctx.app_idx = appkey_idx;

    err = bt_mesh_dfu_cli_status_get(&app_dfd_srv.dfu, &ctx, &rsp_data);
    if (err) {
        app_print("err %d\r\n", err);
        return;
    }

    app_print("update get status %u, phase %u, effect %u, blob_id %u, image_idx %u\r\n",
              rsp_data.status, rsp_data.phase, rsp_data.effect, rsp_data.blob_id, rsp_data.img_idx);
}

void app_dfu_update_metadata_check(uint16_t net_idx, uint16_t addr, uint8_t img_idx,
                                   uint8_t slot_idx)
{
    const struct bt_mesh_dfu_slot *slot;
    struct bt_mesh_msg_ctx ctx = {
        .net_idx = net_idx,
        .send_ttl = BT_MESH_TTL_DEFAULT,
    };
    struct bt_mesh_dfu_metadata_status rsp_data;
    int err, i = 0;

    uint16_t appkey_idx;

    for (i = 0; i < app_dfd_srv.dfu.mod->keys_cnt; i++) {
        if (app_dfd_srv.dfu.mod->keys[i] != BT_MESH_KEY_UNUSED) {
            break;
        }
    }

    if (i == app_dfd_srv.dfu.mod->keys_cnt) {
        app_print("Model not bind with app key\r\n");
        return;
    } else {
        appkey_idx = app_dfd_srv.dfu.mod->keys[i];
    }

    ctx.addr = addr;
    ctx.app_idx = appkey_idx;
    slot = bt_mesh_dfu_slot_at(slot_idx);
    if (slot) {
        app_print("app_dfu_update_metadata_check can't find slot %d\r\n", slot_idx);
        return;
    }

    err = bt_mesh_dfu_cli_metadata_check(&app_dfd_srv.dfu, &ctx, img_idx, slot,
                                         &rsp_data);
    if (err) {
        app_print("app_dfu_update_metadata_check ERR %d\r\n", err);
    }

    app_print("app_dfu_update_metadata_check image idx %u, status %u, effect %u\r\n",
              rsp_data.idx, rsp_data.status, rsp_data.effect);
}


void app_dfu_firmware_update_cancel(uint16_t net_idx, uint16_t addr)
{
    struct bt_mesh_msg_ctx ctx = {
        .net_idx = net_idx,
        .send_ttl = BT_MESH_TTL_DEFAULT,
    };
    int err, i = 0;
    uint16_t appkey_idx;

    for (i = 0; i < app_dfd_srv.dfu.mod->keys_cnt; i++) {
        if (app_dfd_srv.dfu.mod->keys[i] != BT_MESH_KEY_UNUSED) {
            break;
        }
    }

    if (i == app_dfd_srv.dfu.mod->keys_cnt) {
        app_print("Model not bind with app key\r\n");
        return;
    } else {
        appkey_idx = app_dfd_srv.dfu.mod->keys[i];
    }

    ctx.addr = addr;
    ctx.app_idx = appkey_idx;

    err = bt_mesh_dfu_cli_cancel(&app_dfd_srv.dfu, &ctx);
    if (err) {
        app_print("app_dfu_firmware_update_cancel err %d\r\n", err);
    }
}


static enum bt_mesh_dfu_iter app_dfu_img_cb(struct bt_mesh_dfu_cli *cli,
                                            struct bt_mesh_msg_ctx *ctx,
                                            uint8_t idx, uint8_t total,
                                            const struct bt_mesh_dfu_img *img,
                                            void *cb_data)
{
    char fwid[2 * CONFIG_BT_MESH_DFU_FWID_MAXLEN + 1];
    size_t len;

    if (img->fwid_len <= sizeof(fwid)) {
        len = bin2hex(img->fwid, img->fwid_len, fwid, sizeof(fwid));
    } else {
        app_print("FWID is too big\r\n");
        return BT_MESH_DFU_ITER_STOP;
    }

    fwid[len] = '\0';

    app_print("Image %u:\r\n", idx);
    app_print("\tFWID: %s\r\n", fwid);
    if (img->uri) {
        app_print("\tURI:  %s\r\n", img->uri);
    }

    return BT_MESH_DFU_ITER_CONTINUE;
}

void app_dfu_info_get(uint16_t net_idx, uint16_t addr, uint8_t max_count)
{
    struct bt_mesh_msg_ctx ctx = {
        .net_idx = net_idx,
        .send_ttl = BT_MESH_TTL_DEFAULT,
    };
    int err = 0, i = 0;
    uint16_t appkey_idx;

    for (i = 0; i < app_dfd_srv.dfu.mod->keys_cnt; i++) {
        if (app_dfd_srv.dfu.mod->keys[i] != BT_MESH_KEY_UNUSED) {
            break;
        }
    }

    if (i == app_dfd_srv.dfu.mod->keys_cnt) {
        app_print("Model not bind with app key\r\n");
        return;
    } else {
        appkey_idx = app_dfd_srv.dfu.mod->keys[i];
    }

    ctx.addr = addr;
    ctx.app_idx = appkey_idx;

    err = bt_mesh_dfu_cli_imgs_get(&app_dfd_srv.dfu, &ctx, app_dfu_img_cb, NULL,
                                   max_count);
    if (err) {
        app_print("app_dfu_info_get err %d\r\n", err);
    }
}


void app_dfu_slot_add(size_t size, uint8_t *fwid, size_t fwid_len,
                      uint8_t *metadata, size_t metadata_len)
{
    struct bt_mesh_dfu_slot *slot;
    int err;

    slot = bt_mesh_dfu_slot_reserve();
    err = bt_mesh_dfu_slot_info_set(slot, size, metadata, metadata_len);
    if (err) {
        app_print("Failed to set slot info: %d \r\n", err);
        return;
    }

    err = bt_mesh_dfu_slot_fwid_set(slot, fwid, fwid_len);
    if (err) {
        app_print("Failed to set slot fwid: %d \r\n", err);
        return;
    }

    err = bt_mesh_dfu_slot_commit(slot);
    if (err) {
        app_print("Failed to commit slot: %d \r\n", err);
        return;
    }

    app_print("Slot added size %u.\r\n", size);
}


void app_mesh_dfu_cli_init(void)
{
    mbedtls_sha256_context sha256_context;
    uint8_t data[READ_IMG_SIZE] = {0};
    int i = 0;
    int err;
    uint8_t image_idx;
    uint32_t size = TRANS_MAX_IMAGE_SIZE > IMAGE_SZIE ? IMAGE_SZIE : TRANS_MAX_IMAGE_SIZE;
    uint32_t left_size = size % READ_IMG_SIZE;

    mbedtls_sha256_init(&sha256_context);
    mbedtls_sha256_starts(&sha256_context, 0);

    err = rom_sys_status_get(SYS_RUNNING_IMG, LEN_SYS_RUNNING_IMG, &image_idx);
    if (err != SYS_STATUS_FOUND_OK) {
        app_print("app_mesh_dfu_cli_init find running image fail\r\n");
    }

    if (image_idx == IMAGE_0) {
        dfu_backup_img_offset = RE_IMG_1_OFFSET;
    } else {
        dfu_backup_img_offset = RE_IMG_0_OFFSET;
    }

    // Before DFU start, load image to image0 or image1 regision firstly
    for (i = 0; i < (size / READ_IMG_SIZE); i++) {
        err = raw_flash_read(dfu_backup_img_offset + i * READ_IMG_SIZE, data, READ_IMG_SIZE);
        if (err < 0) {
            app_print("raw_flash_read fail\r\n");
        }

        mbedtls_sha256_update(&sha256_context, data, READ_IMG_SIZE);
        //debug_print_dump_data("chunk: ", (char*)data, READ_IMG_SIZE);
    }

    if (left_size) {
        err = raw_flash_read(dfu_backup_img_offset + size - left_size, data, left_size);
        if (err < 0) {
            app_print("raw_flash_read fail\r\n");
        }

        mbedtls_sha256_update(&sha256_context, data, left_size);
    }

    mbedtls_sha256_finish(&sha256_context, checkdata);

    app_print("checkdata: ");
    for (i = 0; i < 32; i++) {
        app_print("0x%x ", checkdata[i]);
    }
    app_print("\r\n");

    bt_mesh_dfu_slot_del_all();
    //Add local image0 to slot, add 32 bytes checkdata to the image tail
    app_dfu_slot_add(size + 32, (uint8_t *)APP_DFD_FWID, strlen(APP_DFD_FWID), (uint8_t *)APP_META_DATA,
                     strlen(APP_META_DATA));
}
