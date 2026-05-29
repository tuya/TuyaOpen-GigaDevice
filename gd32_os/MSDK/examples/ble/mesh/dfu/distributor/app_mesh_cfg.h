/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*!
    \file    app_mesh_cfg.h
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

#ifndef APP_MESH_CFG_H_
#define APP_MESH_CFG_H_

#define APP_CID_NVAL   0xffff
extern struct bt_mesh_cfg_cli app_cfg_cli;

void app_mesh_cfg_get_comp(uint16_t net_idx, uint16_t dst, uint8_t page);
int app_mesh_cfg_mod_pub_set(uint16_t net_idx, uint16_t dst, uint16_t addr, bool is_va,
                             uint16_t mod_id, uint16_t cid, char *argv[]);
int app_mesh_cfg_mod_pub_get(uint16_t net_idx, uint16_t dst, uint16_t addr, uint16_t mod_id,
                             uint16_t cid);
void app_mesh_cfg_net_key_add(uint16_t net_idx, uint16_t dst, uint16_t key_net_idx,
                              uint8_t key_val[16]);
void app_mesh_cfg_app_key_add(uint16_t net_idx, uint16_t dst, uint16_t key_net_idx,
                              uint16_t key_app_idx, uint8_t key_val[16]);

void app_mesh_cfg_hb_pub_get(uint16_t net_idx, uint16_t dst);
void app_mesh_cfg_hb_pub_set(uint16_t net_idx, uint16_t dst, struct bt_mesh_cfg_cli_hb_pub *pub);
void app_mesh_cfg_hb_sub_get(uint16_t net_idx, uint16_t dst);
void app_mesh_cfg_hb_sub_set(uint16_t net_idx, uint16_t dst, struct bt_mesh_cfg_cli_hb_sub *sub);
#endif // APP_MESH_CFG_H_