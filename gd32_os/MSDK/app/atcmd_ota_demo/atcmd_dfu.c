/*!
    \file    atcmd_dfu.c
    \brief   Implementations of dfu

    \version 2024-07-31, V1.0.0, firmware for GD32VW55x
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

#include <string.h>
#include "app_cfg.h"
#include "atcmd_dfu.h"
#include "dbg_print.h"

#include "wrapper_os.h"

#include "config_gdm32.h"
#include "rom_export.h"
#include "raw_flash_api.h"
#include "gd32vw55x.h"
#include "atcmd.h"

#ifdef CONFIG_ATCMD_OTA_DEMO

#define AT_DFU_SEGMENT_LEN                 (4096)
#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define MAX(x, y) ((x) > (y) ? (x) : (y))


typedef struct at_dfu_context {
    at_dfu_mode_t mode;
    uint32_t image_size;
    uint32_t current_size;

    uint32_t start_address;
    uint32_t erase_address;
    uint8_t fw_running_idx;
} at_dfu_ctx_t;


static at_dfu_ctx_t *at_dfu_ctx = NULL;

int at_dfu_get_ready(at_dfu_mode_t mode, uint32_t image_size)
{
    int32_t err = 0;
    uint32_t image_maxlen = 0;

    if (at_dfu_ctx) {
        AT_TRACE("at_dfu_init at_dfu_ctx is not NULL\r\n");
        return -1;
    }

    at_dfu_ctx = sys_malloc(sizeof(at_dfu_ctx_t));
    if (!at_dfu_ctx) {
        AT_TRACE("at_dfu_init malloc at_dfu_ctx fail!\r\n");
        return -2;
    }

    sys_memset(at_dfu_ctx, 0, sizeof(at_dfu_ctx_t));
    at_dfu_ctx->mode = mode;
    at_dfu_ctx->image_size = image_size;

    err = rom_sys_status_get(SYS_RUNNING_IMG,
                    LEN_SYS_RUNNING_IMG,
                    &at_dfu_ctx->fw_running_idx);
    if (err < 0) {
        AT_TRACE("VW553 OTA get running idx failed! (res = %d)\r\n", err);
        goto ready_done;
    }

    if (at_dfu_ctx->fw_running_idx == IMAGE_0) {
        at_dfu_ctx->start_address = RE_IMG_1_OFFSET;
        image_maxlen = RE_IMG_1_END - RE_IMG_1_OFFSET;
    } else {
        at_dfu_ctx->start_address = RE_IMG_0_OFFSET;
        image_maxlen = RE_IMG_1_OFFSET - RE_IMG_0_OFFSET;
    }
    at_dfu_ctx->erase_address = at_dfu_ctx->start_address;

/*
    err = raw_flash_erase(at_dfu_ctx->start_address, image_maxlen);
    if (err < 0) {
        AT_TRACE("OTA flash erase failed (res = %d)\r\n", err);
        goto ready_done;
    }
*/
ready_done:
    if (err < 0) {
        sys_mfree(at_dfu_ctx);
        at_dfu_ctx = NULL;
    }

    return err;
}


void at_dfu_verify_image(unsigned char *output, mbedtls_md_type_t md_type)
{
    uint8_t *p_buf = NULL;
    int err;
    uint32_t offset, read_size;

    mbedtls_md_context_t ctx;

    if (!at_dfu_ctx) {
        return;
    }

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);

    p_buf = sys_malloc(AT_DFU_SEGMENT_LEN);

    if (p_buf == NULL) {
        AT_TRACE("malloc buf fail!\r\n");
        return;
    }

    for (offset = 0; offset < at_dfu_ctx->current_size; offset += AT_DFU_SEGMENT_LEN) {
        read_size = (at_dfu_ctx->current_size - offset) < AT_DFU_SEGMENT_LEN ? (at_dfu_ctx->current_size - offset) : AT_DFU_SEGMENT_LEN;
        err  = raw_flash_read(at_dfu_ctx->start_address + offset, (void *)p_buf, read_size);
        if (err) {
            AT_TRACE("Flash read failed %d\r\n", err);
        }
        mbedtls_md_update(&ctx, (const unsigned char *)p_buf, read_size);
    }

    mbedtls_md_finish(&ctx, output);
    mbedtls_md_free(&ctx);
    sys_mfree(p_buf);
}


int at_dfu_write_image(uint8_t *p_buf, uint32_t size)
{
    int len, erase_size = 0;

    if (at_dfu_ctx->start_address + at_dfu_ctx->current_size + size > at_dfu_ctx->erase_address) {
        erase_size = MAX(AT_DFU_SEGMENT_LEN, ROUND_UP(size, AT_DFU_SEGMENT_LEN));
        len = raw_flash_erase(at_dfu_ctx->erase_address, erase_size);
        if (len != 0) {
            AT_TRACE("dfu flash erase failed!\r\n");
            return len;
        } else {
            at_dfu_ctx->erase_address += erase_size;
        }
    }
    /*
    AT_TRACE("Write to 0x%x with len %d, erase_size 0x%x\r\n",
            at_dfu_ctx->start_address + at_dfu_ctx->current_size,
            size,
            at_dfu_ctx->erase_address);
    */
    /* Write to Flash */
    len = raw_flash_write_fast(at_dfu_ctx->start_address + at_dfu_ctx->current_size, p_buf, size);
    if (len < 0) {
        AT_TRACE("dfu flash write failed!\r\n");
    } else {
        at_dfu_ctx->current_size += size;
    }

    return len;
}


int at_dfu_finish(bool success)
{
    int ret = 0;

    if (!at_dfu_ctx) {
        return -1;
    }

    if (success) {
        /* Set image status */
        ret = rom_sys_set_img_flag(at_dfu_ctx->fw_running_idx,
                        (IMG_FLAG_IA_MASK | IMG_FLAG_NEWER_MASK),
                        (IMG_FLAG_IA_OK | IMG_FLAG_OLDER));
        ret |= rom_sys_set_img_flag(!(at_dfu_ctx->fw_running_idx), (IMG_FLAG_IA_MASK | IMG_FLAG_VERIFY_MASK | IMG_FLAG_NEWER_MASK), 0);
        ret |= rom_sys_set_img_flag(!(at_dfu_ctx->fw_running_idx), IMG_FLAG_NEWER_MASK, IMG_FLAG_NEWER);

        if (ret)
            AT_TRACE("AT dfu set image status failed! (%d)\r\n", ret);
        else
            AT_TRACE("AT dfu finish...\r\n");
    } else {
        AT_TRACE("AT dfu fail...\r\n");
    }

    sys_mfree(at_dfu_ctx);
    at_dfu_ctx = NULL;

    return ret;
}
#endif /* CONFIG_ATCMD_OTA_DEMO */
