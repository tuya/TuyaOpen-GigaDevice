/*!
    \file    mbl_image_validate.c
    \brief   MBL image validate file for GD32VW55x SDK

    \version 2023-07-20, V1.0.0, firmware for GD32VW55x
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

#include "mbl_includes.h"

/*!
    \brief      find boot image
    \param[in]  none
    \param[out] idx: pointer index of boot image
    \param[out] image_offset: pointer to offset of boot image
    \retval     result of find boot image
      \arg        0: find image success
      \arg        -1: offset of image is null
      \arg        -2: find image0 fail
      \arg        -3: find image1 fail
      \arg        -4: image1 has downloaded but image0 has not downloaded
      \arg        -5: other fail
      \arg        -6: read image address from system setting fail
*/
int boot_image_find(OUT uint32_t *idx, OUT uint32_t *image_offset)
{
    uint8_t img0_stat = 0, img1_stat = 0;
#if 0
    struct sys_setting_t setting;
#endif
    uint8_t img0_found = 0, img1_found = 0;
    int boot_idx = -1;
    int ret, result = 0;

    if (NULL == image_offset) {
        result = -1;
        goto Failed;
    }

    ret = rom_sys_status_get(SYS_IMAGE0_STATUS, LEN_SYS_IMAGE_STATUS, &img0_stat);
    if (ret <= SYS_STATUS_FOUND_ERR) {
        result = -2;
        goto Failed;
    } else if (ret == SYS_STATUS_FOUND_OK) {
        img0_found = 1;
    } else {  // SYS_STATUS_NOT_FOUND
        img0_found = 0;
    }
    ret = rom_sys_status_get(SYS_IMAGE1_STATUS, LEN_SYS_IMAGE_STATUS, &img1_stat);
    if (ret <= SYS_STATUS_FOUND_ERR) {
        result = -3;
        goto Failed;
    } else if (ret == SYS_STATUS_FOUND_OK) {
        img1_found = 1;
    } else {  // SYS_STATUS_NOT_FOUND
        img1_found = 0;
    }

    /* Image0 never downloaded, Image1 should not be downloaded too. */
    if (!img0_found && img1_found) {
        result = -4;
        goto Failed;
    }

    if (!img0_found && !img1_found) {
        /* ISP newly downloaded */
        rom_sys_set_img_flag(IMAGE_0, IMG_FLAG_NEWER_MASK, IMG_FLAG_NEWER);
        boot_idx = 0;
        goto ImgSelected;
    }

    if (img0_found && ((img0_stat & IMG_FLAG_NEWER_MASK) == IMG_FLAG_NEWER)) {
        if (((img0_stat & IMG_FLAG_VERIFY_MASK) != IMG_FLAG_VERIFY_FAIL)
            && ((img0_stat & IMG_FLAG_IA_MASK) != IMG_FLAG_IA_FAIL)) {
            boot_idx = 0;
            goto ImgSelected;
        }
    }

    if (img1_found && ((img1_stat & IMG_FLAG_NEWER_MASK) == IMG_FLAG_NEWER)) {
        if (((img1_stat & IMG_FLAG_VERIFY_MASK) != IMG_FLAG_VERIFY_FAIL)
            && ((img1_stat & IMG_FLAG_IA_MASK) != IMG_FLAG_IA_FAIL)) {
            boot_idx = 1;
            goto ImgSelected;
        }
    }

    if (img0_found && ((img0_stat & IMG_FLAG_VERIFY_MASK) == IMG_FLAG_VERIFY_OK)
        && ((img0_stat & IMG_FLAG_IA_MASK) == IMG_FLAG_IA_OK)) {
        boot_idx = 0;
        goto ImgSelected;
    }

    if (img1_found && ((img1_stat & IMG_FLAG_VERIFY_MASK) == IMG_FLAG_VERIFY_OK)
        && ((img1_stat & IMG_FLAG_IA_MASK) == IMG_FLAG_IA_OK)) {
        boot_idx = 1;
        goto ImgSelected;
    }

    if (boot_idx == -1) {
        result = -5;
        goto Failed;
    }

ImgSelected:
#if 0
    /* Read Image Address from system setting */
    ret = rom_sys_setting_get(&setting);
    if (ret != 0) {
        result = -6;
        goto Failed;
    }
    *idx = boot_idx;
    *image_offset = setting.img0_offset;
#else
    *idx = boot_idx;
    *image_offset = RE_IMG_0_OFFSET + IMG_OVERHEAD_LEN;
#endif

    return 0;

Failed:
    return result;
}

/*!
    \brief      validate image x
    \param[in]  img_offset: image offset
    \param[in]  pkhash: pointer to public key hash
    \param[in]  boot_opt: boot option
    \retval     result of validate nspe
*/
int image_x_validate(IN uint32_t img_offset,
                     IN uint8_t *pkhash,
                     IN uint8_t boot_opt)
{
    struct sw_info_t sw_info;
    int ret, result = 0;

    rom_trace_ex(ROM_DBG, "image_x_validate: image offset is 0x%x.\r\n", img_offset);

    if (boot_opt == IBL_VERIFY_NONE)
        return 0;

    /* Validate image cert and signature */
    if (boot_opt == IBL_VERIFY_CERT_IMG) {
        ret = rom_cert_img_validate(img_offset, IMG_TYPE_IMG, pkhash, &sw_info);
        if (ret != IMG_OK) {
            rom_trace_ex(ROM_ERR, "image_x_validate: cert or image verified failed(%d). \r\n", ret);
            result = -3;
            goto Failed;
        } else {
            rom_trace_ex(ROM_ALWAYS, "MBL: Image cert and sigature verified OK.\r\n");
        }
    } else if (boot_opt == IBL_VERIFY_IMG_ONLY) {
        ret = rom_img_validate(img_offset, IMG_TYPE_IMG, pkhash, &sw_info);
        if (ret != IMG_OK) {
            rom_trace_ex(ROM_ERR, "image_x_validate: Image verified failed (%d). \r\n", ret);
            result = -4;
            goto Failed;
        } else {
            rom_trace_ex(ROM_ALWAYS, "MBL: Image verified OK.\r\n");
        }
    }

    /* Update Main Image Version Counter */
    ret = rom_sys_set_fw_ver(IMG_TYPE_IMG, sw_info.version);
    if (ret < 0) {
        rom_trace_ex(ROM_ERR, "Update Main Image version counter failed(%d).\r\n", ret);
        result = -5;
        goto Failed;
    } else if (ret == 1){
        /* The new image version is equal to the local counter. Do nothing. */
    } else if (ret == 0){
        rom_trace_ex(ROM_ALWAYS, "MBL: Current image version is %s%x.%x.%x.%03x\r\n",
                    RE_CUSTOMER_NAME,
                    (RE_IMG_VERSION >> 28),
                    (RE_IMG_VERSION >> 20) & 0xFF,
                    (RE_IMG_VERSION >> 12) & 0xFF,
                    RE_IMG_VERSION & 0xFFF);
    }

    return 0;

Failed:
    return result;
}
