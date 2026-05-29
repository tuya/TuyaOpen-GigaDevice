/*!
    \file    app_mesh_dfu_srv.c
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
#include "app_mesh_dfu_srv.h"
#include "config_gdm32.h"
#include "raw_flash_api.h"
#include "rom_export_mbedtls.h"
#include "rom_export.h"


#define APP_DFD_FWID          "GD_IMAGE0"
#define APP_META_DATA         "Local image"
#define READ_IMG_SIZE         512

static int app_dfu_meta_check(struct bt_mesh_dfu_srv *srv,
                              const struct bt_mesh_dfu_img *img,
                              struct net_buf_simple *metadata,
                              enum bt_mesh_dfu_effect *effect);

static int app_dfu_start(struct bt_mesh_dfu_srv *srv,
                         const struct bt_mesh_dfu_img *img,
                         struct net_buf_simple *metadata,
                         const struct bt_mesh_blob_io **io);

static void app_dfu_end(struct bt_mesh_dfu_srv *srv,
                        const struct bt_mesh_dfu_img *img, bool success);


static int app_dfu_apply(struct bt_mesh_dfu_srv *srv,
                         const struct bt_mesh_dfu_img *img);

static int app_dfu_srv_blob_io_open(const struct bt_mesh_blob_io *io,
                                    const struct bt_mesh_blob_xfer *xfer,
                                    enum bt_mesh_blob_io_mode mode);

static int app_dfu_srv_blob_chunk_wr(const struct bt_mesh_blob_io *io,
                                     const struct bt_mesh_blob_xfer *xfer,
                                     const struct bt_mesh_blob_block *block,
                                     const struct bt_mesh_blob_chunk *chunk);


static struct bt_mesh_dfu_img app_dfu_imgs[] = { {
        .fwid = APP_DFD_FWID,
        .fwid_len = strlen(APP_DFD_FWID),
    }
};

static const struct bt_mesh_dfu_srv_cb app_dfu_handlers = {
    .check = app_dfu_meta_check,
    .start = app_dfu_start,
    .end = app_dfu_end,
    .apply = app_dfu_apply,
};

struct bt_mesh_dfu_srv app_dfu_srv =
    BT_MESH_DFU_SRV_INIT(&app_dfu_handlers, app_dfu_imgs, ARRAY_SIZE(app_dfu_imgs));


static const struct bt_mesh_blob_io app_dfu_srv_blob_io = {
    .open = app_dfu_srv_blob_io_open,
    .wr = app_dfu_srv_blob_chunk_wr,
};

static uint32_t image_total_size = 0;
static uint8_t checkdata[32] = {0};
static uint32_t dfu_img_offset = RE_IMG_1_OFFSET;

static int app_dfu_srv_blob_io_open(const struct bt_mesh_blob_io *io,
                                    const struct bt_mesh_blob_xfer *xfer,
                                    enum bt_mesh_blob_io_mode mode)
{
    int err;
    uint8_t image_idx = 0;
    app_print("app_dfu_srv_blob_io_open mode %u, image size %u.\r\n", mode, xfer->size);
    image_total_size = xfer->size - 32;
    memset(checkdata, 0, 32);
    err = rom_sys_status_get(SYS_RUNNING_IMG, LEN_SYS_RUNNING_IMG, &image_idx);
    if (err != SYS_STATUS_FOUND_OK) {
        app_print("app_dfu_srv_blob_io_open find running image fail\r\n");
    }

    if (image_idx == IMAGE_0) {
        dfu_img_offset = RE_IMG_1_OFFSET;
    } else {
        dfu_img_offset = RE_IMG_0_OFFSET;
    }

    err = raw_flash_erase(dfu_img_offset, image_total_size);
    if (err < 0) {
        app_print("app_dfu_srv_blob_io_open raw_flash_erase fail\r\n");
    }
    return 0;
}

static int app_dfu_srv_blob_chunk_wr(const struct bt_mesh_blob_io *io,
                                     const struct bt_mesh_blob_xfer *xfer,
                                     const struct bt_mesh_blob_block *block,
                                     const struct bt_mesh_blob_chunk *chunk)
{
    int32_t ret = 0;
    uint32_t offset = dfu_img_offset + block->offset + chunk->offset;

    app_print("chunk wr block->offset 0x%x, chunk->offset: 0x%x\r\n", block->offset, chunk->offset);

    //debug_print_dump_data("chunk: ", (char*)chunk->data, chunk->size);

    if ((block->number % 100) == 0 && chunk->offset == 0) {
        app_print("app_dfu_srv_blob_chunk_wr block number %u, chunk offset %u.\r\n", block->number,
                  chunk->offset);
    }

    if ((block->offset + chunk->offset) >= image_total_size) {
        uint32_t checkdata_offset = block->offset + chunk->offset - image_total_size;
        uint32_t copy_size = (32 - checkdata_offset) > chunk->size ? chunk->size : 32;
        app_print("chunk wr number: %u, chunk size: %u, copy_size %u, checkdata_offset %u\r\n",
                  block->number, chunk->size, copy_size, checkdata_offset);
        memcpy(&checkdata[checkdata_offset], chunk->data, copy_size);
    } else if ((block->offset + chunk->offset + chunk->size) > image_total_size) {
        uint32_t copy_image_size = image_total_size - block->offset - chunk->offset;
        uint32_t copy_checkdata_size = (chunk->size - copy_image_size) > 32 ? 32 :
                                       (chunk->size - copy_image_size);
        app_print("chunk wr number: %u, chunk size: %u, copy_image_size %u, copy_checkdata_size %u\r\n",
                  block->number, chunk->size, copy_image_size, copy_checkdata_size);
        ret = raw_flash_write(offset, chunk->data, copy_image_size);
        if (ret < 0) {
            app_print("app_dfu_srv_blob_chunk_wr fail\r\n");
        }

        memcpy(checkdata, &chunk->data[copy_image_size], copy_checkdata_size);
    } else {
        ret = raw_flash_write(offset, chunk->data, chunk->size);
        if (ret < 0) {
            app_print("app_dfu_srv_blob_chunk_wr fail\r\n");
        }
    }
    return 0;
}

static int app_dfu_meta_check(struct bt_mesh_dfu_srv *srv,
                              const struct bt_mesh_dfu_img *img,
                              struct net_buf_simple *metadata,
                              enum bt_mesh_dfu_effect *effect)
{
    if (strlen(APP_META_DATA) != metadata->len ||
        memcmp(metadata->data, APP_META_DATA, strlen(APP_META_DATA))) {
        app_print("Wrong Firmware Metadata\r\n");
        return -EINVAL;
    }

    return 0;
}

static int app_dfu_start(struct bt_mesh_dfu_srv *srv,
                         const struct bt_mesh_dfu_img *img,
                         struct net_buf_simple *metadata,
                         const struct bt_mesh_blob_io **io)
{
    app_print("DFU setup\r\n");

    *io = &app_dfu_srv_blob_io;

    return 0;
}

static void app_dfu_end(struct bt_mesh_dfu_srv *srv,
                        const struct bt_mesh_dfu_img *img, bool success)
{
    mbedtls_sha256_context sha256_context;
    uint8_t data[READ_IMG_SIZE] = {0};
    uint8_t result_checkdata[32] = {0};
    uint32_t left_size = image_total_size % READ_IMG_SIZE;
    int i = 0;
    int err;

    if (!success) {
        app_print("DFU failed\r\n");
        return;
    }

    mbedtls_sha256_init(&sha256_context);
    mbedtls_sha256_starts(&sha256_context, 0);

    for (i = 0; i < (image_total_size / READ_IMG_SIZE); i++) {
        err = raw_flash_read(dfu_img_offset + i * READ_IMG_SIZE, data, READ_IMG_SIZE);
        if (err < 0) {
            app_print("raw_flash_read fail\r\n");
        }

        mbedtls_sha256_update(&sha256_context, data, READ_IMG_SIZE);
    }

    if (left_size > 0) {
        err = raw_flash_read(dfu_img_offset + image_total_size - left_size, data, left_size);
        if (err < 0) {
            app_print("raw_flash_read fail\r\n");
        }

        mbedtls_sha256_update(&sha256_context, data, left_size);
    }

    mbedtls_sha256_finish(&sha256_context, result_checkdata);

    if (memcmp(checkdata, result_checkdata, 32)) {
        app_print("checkdata wrong\r\n");
        app_print("result_checkdata: ");
        for (i = 0; i < 32; i++) {
            app_print("0x%x ", result_checkdata[i]);
        }
        app_print("\r\n");

        app_print("checkdata: ");
        for (i = 0; i < 32; i++) {
            app_print("0x%x ", checkdata[i]);
        }
        app_print("\r\n");
        bt_mesh_dfu_srv_rejected(srv);
        return;
    }
    app_print("checkdata success!\r\n");
    bt_mesh_dfu_srv_verified(srv);
}

static int app_dfu_apply(struct bt_mesh_dfu_srv *srv,
                         const struct bt_mesh_dfu_img *img)
{
    int err;
    uint8_t img_idx;

    app_print("Applying DFU transfer...\r\n");

    if (dfu_img_offset == RE_IMG_1_OFFSET) {
        img_idx = IMAGE_1;
        err = rom_sys_set_img_flag(IMAGE_0, (IMG_FLAG_IA_MASK | IMG_FLAG_NEWER_MASK),
                                   (IMG_FLAG_IA_OK | IMG_FLAG_OLDER));
    } else {
        img_idx = IMAGE_0;
        err = rom_sys_set_img_flag(IMAGE_1, (IMG_FLAG_IA_MASK | IMG_FLAG_NEWER_MASK),
                                   (IMG_FLAG_IA_OK | IMG_FLAG_OLDER));
    }

    app_print("Applying DFU transfer1... img_idx %d\r\n", img_idx);
    if (err) {
        app_print("Set img_idx %d unused fail!\r\n", img_idx);
        return -EIO;
    }

    err = rom_sys_set_img_flag(img_idx, (IMG_FLAG_IA_MASK | IMG_FLAG_VERIFY_MASK | IMG_FLAG_NEWER_MASK),
                               IMG_FLAG_NEWER);

    app_print("Applying DFU transfer2... img_idx %d\r\n", img_idx);

    if (err) {
        app_print("Set img_idx %d new fail!\r\n", img_idx);
        return -EIO;
    }

    app_print("Applying DFU new image success, please reset board manually!\r\n");
    return 0;
}

