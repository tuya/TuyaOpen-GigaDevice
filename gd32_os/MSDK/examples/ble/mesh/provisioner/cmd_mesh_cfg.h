/*!
    \file    cmd_mesh_cfg.h
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

#ifndef CMD_MESH_CFG_H_
#define CMD_MESH_CFG_H_

void cmd_ble_mesh_cfg_set_srv(int argc, char **argv);
void cmd_ble_mesh_cfg_beacon(int argc, char **argv);
void cmd_ble_mesh_cfg_get_comp(int argc, char **argv);
void cmd_ble_mesh_cfg_ttl(int argc, char **argv);
void cmd_ble_mesh_cfg_gatt_proxy(int argc, char **argv);
void cmd_ble_mesh_cfg_relay(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_pub(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_sub_add(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_sub_add_vnd(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_sub_del(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_sub_del_vnd(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_sub_add_va(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_sub_add_va_vnd(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_sub_del_va(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_sub_del_va_vnd(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_sub_ow(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_sub_ow_vnd(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_sub_ow_va(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_sub_ow_va_vnd(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_sub_del_all(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_sub_del_all_vnd(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_sub_get(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_sub_get_vnd(int argc, char **argv);
void cmd_ble_mesh_cfg_net_key_add(int argc, char **argv);
void cmd_ble_mesh_cfg_net_key_update(int argc, char **argv);
void cmd_ble_mesh_cfg_net_key_get(int argc, char **argv);
void cmd_ble_mesh_cfg_app_key_add(int argc, char **argv);
void cmd_ble_mesh_cfg_app_key_upd(int argc, char **argv);
void cmd_ble_mesh_cfg_app_key_get(int argc, char **argv);
void cmd_ble_mesh_cfg_app_key_del(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_app_bind(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_app_unbind(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_app_bind_vnd(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_app_unbind_vnd(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_app_get(int argc, char **argv);
void cmd_ble_mesh_cfg_mod_app_get_vnd(int argc, char **argv);
void cmd_ble_mesh_cfg_node_reset(int argc, char **argv);

void cmd_ble_mesh_cfg_hb_pub_get(int argc, char **argv);
void cmd_ble_mesh_cfg_hb_pub_set(int argc, char **argv);
void cmd_ble_mesh_cfg_hb_sub_get(int argc, char **argv);
void cmd_ble_mesh_cfg_hb_sub_set(int argc, char **argv);
void cmd_ble_mesh_cfg_pollto_get(int argc, char **argv);
void cmd_ble_mesh_cfg_net_transmit(int argc, char **argv);
#endif // CMD_MESH_CFG_H_