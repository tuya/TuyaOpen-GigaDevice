/*!
    \file    app_mesh_dfu_cli.h
    \brief   Header file for BLE mesh application.

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

#ifndef APP_MESH_DFU_CLI_H_
#define APP_MESH_DFU_CLI_H_

extern struct bt_mesh_dfd_srv app_dfd_srv;

void app_mesh_dfu_cli_init(void);

void app_dfu_firmware_update_start(uint8_t slot_idx, uint8_t addr_cnt, uint16_t *addrs);

void app_dfu_firmware_update_apply(void);

void app_dfu_firmware_update_get(uint16_t net_idx, uint16_t addr);

void app_dfu_update_metadata_check(uint16_t net_idx, uint16_t addr, uint8_t img_idx,
                                   uint8_t slot_idx);

void app_dfu_firmware_update_cancel(uint16_t net_idx, uint16_t addr);

void app_dfu_info_get(uint16_t net_idx, uint16_t addr, uint8_t max_count);
#endif // APP_MESH_DFU_CLI_H_