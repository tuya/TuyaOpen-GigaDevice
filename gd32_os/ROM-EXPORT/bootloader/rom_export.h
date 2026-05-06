/*!
    \file    rom_export.h
    \brief   Rom export file for GD32VW55x SDK

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

#ifndef __ROM_EXPORT_H
#define __ROM_EXPORT_H

#define V_1_0                0x100
#define V_1_1                0x101
#define V_1_2                0x102
#define V_2_1                0x201

#include "stdint.h"
#include "stddef.h"
#include "rom_trace.h"
#include "rom_region.h"
#include "rom_image.h"
#include "rom_sys.h"
#include "rom_ibl_state.h"
#include "rom_api.h"

extern struct rom_api_t *p_rom_api;

#define rom_printf                              p_rom_api->printf
#define rom_trace_ex                            p_rom_api->trace_ex
#define rom_rand                                p_rom_api->rand
#define rom_hardware_poll                       p_rom_api->hardware_poll

#define rom_cal_checksum                        p_rom_api->cal_checksum
#define rom_img_verify_sign                     p_rom_api->img_verify_sign
#define rom_img_verify_hash                     p_rom_api->img_verify_digest
#define rom_img_verify_hdr                      p_rom_api->img_verify_hdr
#define rom_img_verify_pkhash                   p_rom_api->img_verify_pkhash
#define rom_img_validate                        p_rom_api->img_validate
#define rom_cert_img_validate                   p_rom_api->cert_img_validate

#define rom_sys_setting_get                     p_rom_api->sys_setting_get
#define rom_sys_status_set                      p_rom_api->sys_status_set
#define rom_sys_status_get                      p_rom_api->sys_status_get
#define rom_sys_set_trace_level                 p_rom_api->sys_set_trace_level
#define rom_sys_set_err_process                 p_rom_api->sys_set_err_process
#define rom_sys_set_img_flag                    p_rom_api->sys_set_img_flag
#define rom_sys_reset_img_flag                  p_rom_api->sys_reset_img_flag
#define rom_sys_set_running_img                 p_rom_api->sys_set_running_img
#define rom_sys_set_fw_ver                      p_rom_api->sys_set_fw_version
#define rom_sys_set_pk_ver                      p_rom_api->sys_set_pk_version

#define rom_flash_read                          p_rom_api->flash_read
#define rom_flash_write                         p_rom_api->flash_write
#define rom_flash_erase                         p_rom_api->flash_erase

#define rom_efuse_get_rotpkh                    p_rom_api->efuse_get_rotpkh

#define rom_do_symm_key_derive                  p_rom_api->do_symm_key_derive

#define rom_sys_status_check                    p_rom_api->sys_status_check
#define rom_log_uart_set                        p_rom_api->log_uart_set

#define rom_digest_haudma_en                    p_rom_api->digest_haudma_en

#endif  // __ROM_EXPORT_H
