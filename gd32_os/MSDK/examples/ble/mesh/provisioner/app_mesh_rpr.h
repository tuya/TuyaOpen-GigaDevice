/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*!
    \file    app_mesh_rpr.h
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

#ifndef APP_MESH_RPR_H_
#define APP_MESH_RPR_H_

extern struct bt_mesh_rpr_cli app_rpr_cli;

int app_mesh_rpr_scan(struct bt_mesh_rpr_node *srv, uint8_t *uuid, uint8_t timeout);
int app_mesh_rpr_scan_ext(struct bt_mesh_rpr_node *srv, uint8_t timeout, uint8_t uuid[16],
                          uint8_t *ad_types, size_t ad_count);
int app_mesh_rpr_scan_caps(struct bt_mesh_rpr_node *srv);
int app_mesh_rpr_scan_get(struct bt_mesh_rpr_node *srv);
int app_mesh_rpr_scan_stop(struct bt_mesh_rpr_node *srv);
int app_mesh_rpr_link_get(struct bt_mesh_rpr_node *srv);
int app_mesh_rpr_link_close(struct bt_mesh_rpr_node *srv);
int app_mesh_rpr_provision_remote(struct bt_mesh_rpr_node *srv, uint8_t uuid[16], uint16_t net_idx,
                                  uint16_t addr);
int app_mesh_rpr_reprovision_remote(struct bt_mesh_rpr_node *srv, uint16_t addr,
                                    bool composition_changed);
#endif // APP_MESH_RPR_H_