/*!
    \file    app_gatt_bqb.h
    \brief   GATT bqb Application Module entry point

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

#ifndef APP_GATT_BQB_H_
#define APP_GATT_BQB_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>          // Standard Integer Definition

#include "ble_gap.h"
#include "ble_gatt.h"

/*
 * DEFINES
 ****************************************************************************************
 */
#define APP_GATT_BQB_SUPPORT            0

#define VALUE_V2B_LEN                   22
#define VALUE_V2C_LEN                   512
#define VALUE_V2D_LEN                   43
#define VALUE_V5_CHAR_USER_DESC_LEN     26

/*
 * STRUCTURES DEFINITION
 ****************************************************************************************
 */
// GATT_Qualification_Test_Databases.xlsm Large Database 2
typedef struct app_gatt_bqb_srv_att_info
{
    uint16_t  value_v10;

    uint8_t   value_v2b[VALUE_V2B_LEN];
    uint8_t   value_v2c[VALUE_V2C_LEN];
    uint8_t   value_v2d[VALUE_V2D_LEN];

    uint16_t  value_v9;
    uint16_t  value_v9d2;

    uint8_t   value_v5;
    uint8_t   value_v5_char_user_desc[VALUE_V5_CHAR_USER_DESC_LEN];

    uint16_t  value_v1;
} app_gatt_bqb_srv_att_info_t;

struct app_gatt_bqb_env_tag
{
    // GATT user local identifier
    uint8_t         user_lid;
    // GAP service start handle
    uint16_t        gap_start_hdl;
    // GATT service start handle
    uint16_t        gatt_start_hdl;

    app_gatt_bqb_srv_att_info_t app_srv_att_info;
};

/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */
extern struct app_gatt_bqb_env_tag app_gatt_bqb_env;

/*
 * FUNCTIONS DECLARATION
 ****************************************************************************************
 */
void app_gatt_bqb_srv_db_svc_add_all(void);
void app_gatt_bqb_srv_db_svc_list_get(void);
void app_gatt_bqb_srv_db_svc_remove_all(void);
void app_gatt_bqb_init(void);
void app_gatt_bgb_srv_event_send(uint8_t conidx, uint8_t evt_type, uint16_t hdl);
void app_gatt_bqb_srv_event_reliable_send(uint8_t conidx, uint8_t evt_type, uint8_t nb_attr,
                                      ble_gatt_attr_t *p_attr);
void app_gatt_bqb_cli_discover_svc(uint8_t conidx, uint8_t disc_type, uint8_t full, uint16_t start_hdl,
                               uint16_t end_hdl, uint8_t uuid_type, uint8_t *uuid);
void app_gatt_bqb_cli_discover_inc_svc(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl);
void app_gatt_bqb_cli_discover_char(uint8_t conidx, uint8_t disc_type, uint16_t start_hdl,
                                uint16_t end_hdl, uint8_t uuid_type, uint8_t *uuid);
void app_gatt_bqb_cli_discover_desc(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl);
void app_gatt_bqb_cli_discover_cancel(uint8_t conidx);
void app_gatt_bqb_cli_read(uint8_t conidx, uint16_t hdl, uint16_t offset, uint16_t length);
void app_gatt_bqb_cli_read_by_uuid(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl,
                               uint8_t uuid_type, uint8_t *uuid);
void app_gatt_bqb_cli_read_multiple(uint8_t conidx, uint8_t nb_att, ble_gatt_attr_t *atts);
void app_gatt_bqb_cli_write_reliable(uint8_t conidx, uint8_t write_type, uint8_t write_mode,
                                 uint16_t hdl, uint16_t offset, uint16_t length);
void app_gatt_bqb_cli_write(uint8_t conidx, uint8_t write_type, uint16_t hdl, uint16_t value_length,
                        uint16_t *value);
void app_gatt_bqb_cli_write_exe(uint8_t conidx, uint8_t execute);
void app_gatt_bqb_cli_event_register(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl);
void app_gatt_bqb_cli_mtu_update(uint8_t conidx);
void app_gatt_bqb_eatt_estab(uint8_t conidx);

#endif /* APP_GATT_BQB_H_ */
