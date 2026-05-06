/*!
    \file    app_gatt_bqb.c
    \brief   GATT Application Module entry point.

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

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "ble_app_config.h"
#include "app_gatt_bqb.h"

#if (APP_GATT_BQB_SUPPORT)
#include <string.h>
#include "ble_gatts.h"
#if (BLE_APP_GATT_CLIENT_SUPPORT)
#include "ble_gattc.h"
#endif
#include "ble_storage.h"
#include "ble_utils.h"
#include "wrapper_os.h"
#include "dbg_print.h"

/*
 * DEFINES
 ****************************************************************************************
 */
// 16 bits attributes in a 128 bits array
#define UUID_PRIVATE_128(uuid) {0xEF,0xCD,0xAB,0x89,0x67,0x45,0x23,0x01,0x00,0x00,0x00,0x00, ((uuid) & 0xFF), ((uuid >> 8) & 0xFF),0,0}

#define VALUE_V2B_VALUE {2,2,2,2,2,3,3,3,3,3,4,4,4,4,4,5,5,5,5,5,6,6}
#define VALUE_V2D_VALUE {1,1,1,1,1,2,2,2,2,2,3,3,3,3,3,4,4,4,4,4,5,5,5,5,5,6,6,6,6,6,7,7,7,7,7,8,8,8,8,8,9,9,9}
#define VALUE_V5_CHAR_USER_DESC_VALUE   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

#define APP_GATT_CHAR_PRES_FMT_LEN      7

extern ble_status_t ble_bqb_cli_discover_svc(uint8_t conidx, uint8_t disc_type, uint8_t full, uint16_t start_hdl,
                               uint16_t end_hdl, uint8_t uuid_type, uint8_t *uuid);

extern ble_status_t ble_bqb_cli_discover_inc_svc(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl);

extern ble_status_t ble_bqb_cli_discover_char(uint8_t conidx, uint8_t disc_type, uint16_t start_hdl,
                                uint16_t end_hdl, uint8_t uuid_type, uint8_t *uuid);

extern ble_status_t ble_bqb_cli_discover_desc(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl);

extern ble_status_t ble_bqb_cli_discover_cancel(uint8_t conidx);

extern ble_status_t ble_bqb_cli_read(uint8_t conidx, uint16_t hdl, uint16_t offset, uint16_t length);

extern ble_status_t ble_bqb_cli_read_by_uuid(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl,
                               uint8_t uuid_type, uint8_t *uuid);

extern ble_status_t ble_bqb_cli_read_multiple(uint8_t conidx, uint8_t nb_att, ble_gatt_attr_t *atts);

extern ble_status_t ble_bqb_cli_write_reliable(uint8_t conidx, uint8_t write_type, uint8_t write_mode,
                                 uint16_t hdl, uint16_t offset, uint16_t length);

extern ble_status_t ble_bqb_cli_write(uint8_t conidx, uint8_t write_type, uint16_t hdl, uint16_t value_length,
                        uint8_t *value);

extern ble_status_t ble_bqb_cli_write_exe(uint8_t conidx, uint8_t execute);

extern ble_status_t ble_bqb_bearer_eatt_estab(uint8_t conidx);

extern ble_status_t ble_gatts_ntf_ind_reliable_send(uint8_t conn_idx, ble_gatt_attr_t *p_attr,
                             uint8_t attr_num, ble_gatt_evt_type_t evt_type);

extern ble_status_t ble_gatts_ntf_ind_send_by_handle(uint8_t conn_idx, uint16_t handle, uint8_t *p_val,
                             uint16_t len, ble_gatt_evt_type_t evt_type);

extern ble_status_t ble_gattc_event_register(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl);

extern ble_status_t ble_gattc_event_unregister(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl);

extern void ble_peer_data_bond_gatt_db_update(void);

uint8_t svc_c2_rw_cb(cb_data_t *cb_data);
uint8_t svc_c1_rw_cb(cb_data_t *cb_data);
uint8_t svc_b2_rw_cb(cb_data_t *cb_data);
uint8_t svc_b5_rw_cb(cb_data_t *cb_data);

// GATT_Qualification_Test_Databases.xlsm Large Database 2
enum
{
    // Service C.2
    SERVICE_C2_IDX_PRIM_SVC,

    SERVICE_C2_IDX_CHAR_V10,
    SERVICE_C2_IDX_V10,

    SERVICE_C2_IDX_CHAR_V2A,
    SERVICE_C2_IDX_V2A,
    SERVICE_C2_IDX_V2A_CFG,

    SERVICE_C2_IDX_CHAR_V2B,
    SERVICE_C2_IDX_V2B,

    SERVICE_C2_IDX_CHAR_V2C,
    SERVICE_C2_IDX_V2C,

    SERVICE_C2_IDX_CHAR_V2D,
    SERVICE_C2_IDX_V2D,

    SERVICE_C2_IDX_NUMBER,

    // Service C.1
    SERVICE_C1_IDX_PRIM_SVC = SERVICE_C2_IDX_NUMBER,

    SERVICE_C1_IDX_INC_SVC,

    SERVICE_C1_IDX_CHAR_V9,
    SERVICE_C1_IDX_V9,
    SERVICE_C1_IDX_DESC_V9D2,
    SERVICE_C1_IDX_DESC_V9D3,
    SERVICE_C1_IDX_CHAR_EXT_V9,

    SERVICE_C1_IDX_NUMBER,

    // Service D
    SERVICE_D_IDX_PRIM_SVC = SERVICE_C1_IDX_NUMBER,

    SERVICE_D_IDX_INC_SVC,

    SERVICE_D_IDX_CHAR_V11,
    SERVICE_D_IDX_V11,

    SERVICE_D_IDX_CHAR_V12,
    SERVICE_D_IDX_V12,

    SERVICE_D_IDX_NUMBER,

    // Service B.1
    SERVICE_B1_IDX_PRIM_SVC = SERVICE_D_IDX_NUMBER,

    SERVICE_B1_IDX_CHAR_V4,
    SERVICE_B1_IDX_V4,

    SERVICE_B1_IDX_CHAR_VE,
    SERVICE_B1_IDX_VE,

    SERVICE_B1_IDX_CHAR_VF,
    SERVICE_B1_IDX_VF,

    SERVICE_B1_IDX_NUMBER,

    // Service A
    SERVICE_A_IDX_PRIM_SVC = SERVICE_B1_IDX_NUMBER,

    SERVICE_A_IDX_INC_SVC_A00D,
    SERVICE_A_IDX_INC_SVC_C1,

    SERVICE_A_IDX_CHAR_V3,
    SERVICE_A_IDX_V3,

    SERVICE_A_IDX_NUMBER,

    // Service B.3
    SERVICE_B3_IDX_PRIM_SVC = SERVICE_A_IDX_NUMBER,

    SERVICE_B3_IDX_CHAR_V6,
    SERVICE_B3_IDX_V6,
    SERVICE_B3_IDX_V6_CFG,

    SERVICE_B3_IDX_NUMBER,

    // Service B.2
    SERVICE_B2_IDX_PRIM_SVC = SERVICE_B3_IDX_NUMBER,

    SERVICE_B2_IDX_CHAR_V5,
    SERVICE_B2_IDX_V5,
    SERVICE_B2_IDX_DESC_V5D4,
    SERVICE_B2_IDX_CHAR_EXT_V5,
    SERVICE_B2_IDX_CHAR_USER_V5,
    SERVICE_B2_IDX_CHAR_FORMAT_V5,

    SERVICE_B2_IDX_CHAR_V1,
    SERVICE_B2_IDX_V1,
    SERVICE_B2_IDX_CHAR_FORMAT_V1,

    SERVICE_B2_IDX_CHAR_V2,
    SERVICE_B2_IDX_V2,
    SERVICE_B2_IDX_CHAR_AGG_FORMAT_V2,

    SERVICE_B2_IDX_NUMBER,

    // Service B.5
    SERVICE_B5_IDX_PRIM_SVC = SERVICE_B2_IDX_NUMBER,

    SERVICE_B5_IDX_CHAR_V8,
    SERVICE_B5_IDX_V8,

    SERVICE_B5_IDX_CHAR_VE,
    SERVICE_B5_IDX_VE,
    SERVICE_B5_IDX_CHAR_FORMAT_VE,

    SERVICE_B5_IDX_CHAR_VF,
    SERVICE_B5_IDX_VF,
    SERVICE_B5_IDX_CHAR_FORMAT_VF,

    SERVICE_B5_IDX_CHAR_V6,
    SERVICE_B5_IDX_V6,
    SERVICE_B5_IDX_CHAR_FORMAT_V6,

    SERVICE_B5_IDX_CHAR_V7,
    SERVICE_B5_IDX_V7,
    SERVICE_B5_IDX_CHAR_FORMAT_V7,

    SERVICE_B5_IDX_CHAR_V10_1,
    SERVICE_B5_IDX_V10_1,
    SERVICE_B5_IDX_CHAR_AGG_FORMAT_V10_1,

    SERVICE_B5_IDX_CHAR_V11,
    SERVICE_B5_IDX_V11,

    SERVICE_B5_IDX_CHAR_V10_2,
    SERVICE_B5_IDX_V10_2,
    SERVICE_B5_IDX_CHAR_AGG_FORMAT_V10_2,

    SERVICE_B5_IDX_CHAR_V10_3,
    SERVICE_B5_IDX_V10_3,
    SERVICE_B5_IDX_CHAR_AGG_FORMAT_V10_3,

    SERVICE_B5_IDX_CHAR_V10_4,
    SERVICE_B5_IDX_V10_4,
    SERVICE_B5_IDX_CHAR_AGG_FORMAT_V10_4,

    SERVICE_B5_IDX_CHAR_V10_5,
    SERVICE_B5_IDX_V10_5,
    SERVICE_B5_IDX_CHAR_AGG_FORMAT_V10_5,

    SERVICE_B5_IDX_NUMBER,

    // Service E
    SERVICE_E_IDX_PRIM_SVC = SERVICE_B5_IDX_NUMBER,

    SERVICE_E_IDX_CHAR_V13,
    SERVICE_E_IDX_V13,

    SERVICE_E_IDX_NUMBER,

    // Maximum number of SERVICE_C2 attributes
    SERVICE_C2_NB_ATT = SERVICE_C2_IDX_NUMBER,
    // Maximum number of SERVICE_C1 attributes
    SERVICE_C1_NB_ATT = SERVICE_C1_IDX_NUMBER - SERVICE_C2_IDX_NUMBER,
    // Maximum number of SERVICE_D attributes
    SERVICE_D_NB_ATT = SERVICE_D_IDX_NUMBER - SERVICE_C1_IDX_NUMBER,
    // Maximum number of SERVICE_B1 attributes
    SERVICE_B1_NB_ATT = SERVICE_B1_IDX_NUMBER - SERVICE_D_IDX_NUMBER,
    // Maximum number of SERVICE_A attributes
    SERVICE_A_NB_ATT = SERVICE_A_IDX_NUMBER - SERVICE_B1_IDX_NUMBER,
    // Maximum number of SERVICE_B3 attributes
    SERVICE_B3_NB_ATT = SERVICE_B3_IDX_NUMBER - SERVICE_A_IDX_NUMBER,
    // Maximum number of SERVICE_B2 attributes
    SERVICE_B2_NB_ATT = SERVICE_B2_IDX_NUMBER - SERVICE_B3_IDX_NUMBER,
    // Maximum number of SERVICE_B5 attributes
    SERVICE_B5_NB_ATT = SERVICE_B5_IDX_NUMBER - SERVICE_B2_IDX_NUMBER,
    // Maximum number of SERVICE_E attributes
    SERVICE_E_NB_ATT = SERVICE_E_IDX_NUMBER - SERVICE_B5_IDX_NUMBER,
};

// GATT_Qualification_Test_Databases.xlsm Large Database 2
enum app_gatt_bqb_char
{
    APP_GATT_DECL_V1            = BLE_GATT_UUID_16_LSB(0xB001),     //!< Value: 0xB001
    APP_GATT_DECL_V2            = BLE_GATT_UUID_16_LSB(0xB002),     //!< Value: 0xB002
    APP_GATT_DECL_V3            = BLE_GATT_UUID_16_LSB(0xB003),     //!< Value: 0xB003
    APP_GATT_DECL_V4            = BLE_GATT_UUID_16_LSB(0xB004),     //!< Value: 0xB004
    APP_GATT_DECL_V5            = BLE_GATT_UUID_16_LSB(0xB005),     //!< Value: 0xB005
    APP_GATT_DECL_V5D4          = BLE_GATT_UUID_16_LSB(0xD5D4),     //!< Value: 0xD5D4
    APP_GATT_DECL_V6            = BLE_GATT_UUID_16_LSB(0xB006),     //!< Value: 0xB006
    APP_GATT_DECL_V7            = BLE_GATT_UUID_16_LSB(0xB007),     //!< Value: 0xB007
    APP_GATT_DECL_V8            = BLE_GATT_UUID_16_LSB(0xB008),     //!< Value: 0xB008
    APP_GATT_DECL_V9            = BLE_GATT_UUID_16_LSB(0xB009),     //!< Value: 0xB009
    APP_GATT_DECL_V9D2          = BLE_GATT_UUID_16_LSB(0xD9D2),     //!< Value: 0xD9D2
    APP_GATT_DECL_V9D3          = BLE_GATT_UUID_16_LSB(0xD9D3),     //!< Value: 0xD9D3
    APP_GATT_DECL_VA            = BLE_GATT_UUID_16_LSB(0xB00A),     //!< Value: 0xB00A
    APP_GATT_DECL_VB            = BLE_GATT_UUID_16_LSB(0xB00B),     //!< Value: 0xB00B
    APP_GATT_DECL_VC            = BLE_GATT_UUID_16_LSB(0xB00C),     //!< Value: 0xB00C
    APP_GATT_DECL_VD            = BLE_GATT_UUID_16_LSB(0xB00D),     //!< Value: 0xB00D
    APP_GATT_DECL_VE            = BLE_GATT_UUID_16_LSB(0xB00E),     //!< Value: 0xB00E
    APP_GATT_DECL_VF            = BLE_GATT_UUID_16_LSB(0xB00F),     //!< Value: 0xB00F
    APP_GATT_DECL_V10           = BLE_GATT_UUID_16_LSB(0xB010),     //!< Value: 0xB010
    APP_GATT_DECL_V11           = BLE_GATT_UUID_16_LSB(0xB011),     //!< Value: 0xB011

    APP_GATT_DECL_SERVICE_A     = BLE_GATT_UUID_16_LSB(0xA00A),     //!< Value: 0xA00A
    APP_GATT_DECL_SERVICE_B     = BLE_GATT_UUID_16_LSB(0xA00B),     //!< Value: 0xA00B
    APP_GATT_DECL_SERVICE_C     = BLE_GATT_UUID_16_LSB(0xA00C),     //!< Value: 0xA00C
    APP_GATT_DECL_SERVICE_D     = BLE_GATT_UUID_16_LSB(0xA00D),     //!< Value: 0xA00D
    APP_GATT_DECL_SERVICE_E     = BLE_GATT_UUID_16_LSB(0xA00E),     //!< Value: 0xA00E
};

typedef struct app_gatt_bqb_svc_info
{
    // Service UUID (LSB first)
    uint8_t   uuid[BLE_GATT_UUID_128_LEN];
    // service start index
    uint16_t  index;
    // service start handle
    uint16_t  start_hdl;
    // service num
    uint8_t   num;
    // Service Information bit field (see Table 18)
    uint8_t   info;
    // service idx
    uint8_t   svc_idx;
    // service write/read callback
    p_fun_svc_rw_cb rw_cb;
} app_gatt_bqb_svc_info_t;

typedef struct prf_char_pres_fmt
{
    // Unit (The Unit is a UUID)
    uint16_t unit;
    // Description
    uint16_t description;
    // Format
    uint8_t format;
    // Exponent
    uint8_t exponent;
    // Name space
    uint8_t name_space;
} prf_char_pres_fmt_t;

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
// APP Attribute database description
__STATIC ble_gatt_attr_16_desc_t app_svc_db[SERVICE_E_IDX_NUMBER] = {
    // ---------------------------------- SERVICE C.2 ---------------------------------------------------------------
    // Service C.2
    [SERVICE_C2_IDX_PRIM_SVC] = { BLE_GATT_DECL_PRIMARY_SERVICE, PROP(RD), 0 },

    [SERVICE_C2_IDX_CHAR_V10] = { BLE_GATT_DECL_CHARACTERISTIC,  PROP(RD), 0 },
    [SERVICE_C2_IDX_V10]      = { APP_GATT_DECL_VA,              PROP(RD) | PROP(WC) | PROP(WS), OPT(NO_OFFSET) | sizeof(uint16_t) },

    [SERVICE_C2_IDX_CHAR_V2A] = { BLE_GATT_DECL_CHARACTERISTIC,  PROP(RD), 0 },
    [SERVICE_C2_IDX_V2A]      = { APP_GATT_DECL_V2,              PROP(RD) | PROP(NTF),  OPT(NO_OFFSET) | OPT(NO_OFFSET) | sizeof(uint16_t) },
    [SERVICE_C2_IDX_V2A_CFG]  = { BLE_GATT_DESC_CLIENT_CHAR_CFG, PROP(RD) | PROP(WR), OPT(NO_OFFSET) | sizeof(uint16_t) },

    [SERVICE_C2_IDX_CHAR_V2B] = { BLE_GATT_DECL_CHARACTERISTIC,  PROP(RD), 0 },
    [SERVICE_C2_IDX_V2B]      = { APP_GATT_DECL_V2,              PROP(RD) | PROP(WR), OPT(NO_OFFSET) | VALUE_V2B_LEN },

    [SERVICE_C2_IDX_CHAR_V2C] = { BLE_GATT_DECL_CHARACTERISTIC,  PROP(RD), 0 },
    [SERVICE_C2_IDX_V2C]      = { APP_GATT_DECL_V2,              PROP(RD) | PROP(WR), VALUE_V2C_LEN },

    [SERVICE_C2_IDX_CHAR_V2D] = { BLE_GATT_DECL_CHARACTERISTIC,  PROP(RD), 0 },
    [SERVICE_C2_IDX_V2D]      = { APP_GATT_DECL_V2,              PROP(RD) | PROP(WR), VALUE_V2D_LEN },

    // ---------------------------------- SERVICE C.1 ---------------------------------------------------------------
    // Service C.1
    [SERVICE_C1_IDX_PRIM_SVC]    = { BLE_GATT_DECL_PRIMARY_SERVICE,     PROP(RD), 0 },

    [SERVICE_C1_IDX_INC_SVC]     = { BLE_GATT_DECL_INCLUDE,             PROP(RD), 0x0050 },

    [SERVICE_C1_IDX_CHAR_V9]     = { BLE_GATT_DECL_CHARACTERISTIC,      PROP(RD), 0 },
    [SERVICE_C1_IDX_V9]          = { APP_GATT_DECL_V9,                  PROP(RD) | PROP(WR) | PROP(EXT), OPT(NO_OFFSET) | sizeof(uint16_t) },
    [SERVICE_C1_IDX_DESC_V9D2]   = { APP_GATT_DECL_V9D2,                PROP(RD) | PROP(WR), OPT(NO_OFFSET) | sizeof(uint16_t) },
    [SERVICE_C1_IDX_DESC_V9D3]   = { APP_GATT_DECL_V9D3,                PROP(WR), OPT(NO_OFFSET) | sizeof(uint16_t) },
    [SERVICE_C1_IDX_CHAR_EXT_V9] = { BLE_GATT_DESC_CHAR_EXT_PROPERTIES, PROP(RD), 0x0001 },

    // ---------------------------------- SERVICE D ---------------------------------------------------------------
    // Service D
    [SERVICE_D_IDX_PRIM_SVC]     = { BLE_GATT_DECL_SECONDARY_SERVICE, PROP(RD), 0 },

    [SERVICE_D_IDX_INC_SVC]      = { BLE_GATT_DECL_INCLUDE,           PROP(RD), 0x0060 },

    [SERVICE_D_IDX_CHAR_V11]     = { BLE_GATT_DECL_CHARACTERISTIC,    PROP(RD), 0 },
    [SERVICE_D_IDX_V11]          = { APP_GATT_DECL_VB,                PROP(RD), OPT(NO_OFFSET) | sizeof(uint16_t) },
    [SERVICE_D_IDX_CHAR_V12]     = { BLE_GATT_DECL_CHARACTERISTIC,    PROP(RD), 0 },
    [SERVICE_D_IDX_V12]          = { APP_GATT_DECL_VC,                PROP(RD), OPT(NO_OFFSET) | sizeof(uint16_t) },

    // ---------------------------------- SERVICE B.1 ---------------------------------------------------------------
    // Service B.1
    [SERVICE_B1_IDX_PRIM_SVC]    = { BLE_GATT_DECL_PRIMARY_SERVICE, PROP(RD), 0 },

    [SERVICE_B1_IDX_CHAR_V4]     = { BLE_GATT_DECL_CHARACTERISTIC,  PROP(RD), 0 },
    [SERVICE_B1_IDX_V4]          = { APP_GATT_DECL_V4,              PROP(RD) | PROP(WR), OPT(NO_OFFSET) | sizeof(uint16_t) },

    [SERVICE_B1_IDX_CHAR_VE]     = { BLE_GATT_DECL_CHARACTERISTIC,  PROP(RD), 0 },
    [SERVICE_B1_IDX_VE]          = { APP_GATT_DECL_VE,              PROP(RD) | PROP(WR) | SEC_LVL(RP, UNAUTH), OPT(NO_OFFSET) | sizeof(uint16_t) },

    [SERVICE_B1_IDX_CHAR_VF]     = { BLE_GATT_DECL_CHARACTERISTIC,  PROP(RD), 0 },
    [SERVICE_B1_IDX_VF]          = { APP_GATT_DECL_VE,              PROP(RD) | PROP(WR) | SEC_LVL(RP, AUTH), OPT(NO_OFFSET) | sizeof(uint16_t) },

    // ---------------------------------- SERVICE A ---------------------------------------------------------------
    // Service A
    [SERVICE_A_IDX_PRIM_SVC]     = { BLE_GATT_DECL_PRIMARY_SERVICE, PROP(RD), 0 },

    [SERVICE_A_IDX_INC_SVC_A00D] = { BLE_GATT_DECL_INCLUDE,         PROP(RD), 0x0050 },
    [SERVICE_A_IDX_INC_SVC_C1]   = { BLE_GATT_DECL_INCLUDE,         PROP(RD), 0x0040 },

    [SERVICE_A_IDX_CHAR_V3]      = { BLE_GATT_DECL_CHARACTERISTIC,  PROP(RD), 0 },
    [SERVICE_A_IDX_V3]           = { APP_GATT_DECL_V3,              PROP(WR), OPT(NO_OFFSET) | sizeof(uint16_t) },

    // ---------------------------------- SERVICE B.3 ---------------------------------------------------------------
    // Service B.3
    [SERVICE_B3_IDX_PRIM_SVC]    = { BLE_GATT_DECL_PRIMARY_SERVICE, PROP(RD), 0 },

    [SERVICE_B3_IDX_CHAR_V6]     = { BLE_GATT_DECL_CHARACTERISTIC,  PROP(RD), 0 },
    [SERVICE_B3_IDX_V6]          = { APP_GATT_DECL_V6,              PROP(RD) | PROP(WR) | PROP(WC) | PROP(NTF) | PROP(IND), OPT(NO_OFFSET) | sizeof(uint16_t) },
    [SERVICE_B3_IDX_V6_CFG]      = { BLE_GATT_DESC_CLIENT_CHAR_CFG, PROP(RD) | PROP(WR), OPT(NO_OFFSET) | sizeof(uint16_t) },

    // ---------------------------------- SERVICE B.2 ---------------------------------------------------------------
    // Service B.2
    [SERVICE_B2_IDX_PRIM_SVC]       = { BLE_GATT_DECL_PRIMARY_SERVICE,          PROP(RD), 0 },

    [SERVICE_B2_IDX_CHAR_V5]        = { BLE_GATT_DECL_CHARACTERISTIC,           PROP(RD), 0 },
    [SERVICE_B2_IDX_V5]             = { APP_GATT_DECL_V5,                       PROP(RD) | PROP(WR) | PROP(EXT), OPT(NO_OFFSET) | sizeof(uint8_t) },
    [SERVICE_B2_IDX_DESC_V5D4]      = { APP_GATT_DECL_V5D4,                     PROP(RD), OPT(NO_OFFSET) | sizeof(uint8_t) },
    [SERVICE_B2_IDX_CHAR_EXT_V5]    = { BLE_GATT_DESC_CHAR_EXT_PROPERTIES,      PROP(RD), 0x0003 },
    [SERVICE_B2_IDX_CHAR_USER_V5]   = { BLE_GATT_DESC_CHAR_USER_DESCRIPTION,    PROP(RD) | PROP(WR), VALUE_V5_CHAR_USER_DESC_LEN },
    [SERVICE_B2_IDX_CHAR_FORMAT_V5] = { BLE_GATT_DESC_CHAR_PRES_FORMAT,         PROP(RD), 0 },

    [SERVICE_B2_IDX_CHAR_V1]        = { BLE_GATT_DECL_CHARACTERISTIC,           PROP(RD), 0 },
    [SERVICE_B2_IDX_V1]             = { APP_GATT_DECL_V1,                       PROP(RD) | PROP(WR),  OPT(NO_OFFSET) | sizeof(uint16_t) },
    [SERVICE_B2_IDX_CHAR_FORMAT_V1] = { BLE_GATT_DESC_CHAR_PRES_FORMAT,         PROP(RD), 0 },

    [SERVICE_B2_IDX_CHAR_V2]        = { BLE_GATT_DECL_CHARACTERISTIC,           PROP(RD), 0 },
    [SERVICE_B2_IDX_V2]             = { APP_GATT_DECL_V2,                       PROP(RD), 0 },
    [SERVICE_B2_IDX_CHAR_AGG_FORMAT_V2] = { BLE_GATT_DESC_CHAR_AGGREGATE_FORMAT, PROP(RD), 0 },

    // ---------------------------------- SERVICE B.5 ---------------------------------------------------------------
    // Service B.5
    [SERVICE_B5_IDX_PRIM_SVC]         = { BLE_GATT_DECL_PRIMARY_SERVICE,    PROP(RD), 0 },

    [SERVICE_B5_IDX_CHAR_V8]          = { BLE_GATT_DECL_CHARACTERISTIC,     PROP(RD), 0 },
    [SERVICE_B5_IDX_V8]               = { APP_GATT_DECL_V8,                 PROP(RD) | PROP(WR), OPT(NO_OFFSET) | sizeof(uint16_t) },

    [SERVICE_B5_IDX_CHAR_VE]          = { BLE_GATT_DECL_CHARACTERISTIC,     PROP(RD), 0 },
    [SERVICE_B5_IDX_VE]               = { APP_GATT_DECL_VE,                 PROP(RD) , OPT(NO_OFFSET) | sizeof(uint16_t) },
    [SERVICE_B5_IDX_CHAR_FORMAT_VE]   = { BLE_GATT_DESC_CHAR_PRES_FORMAT,   PROP(RD) , 0 },

    [SERVICE_B5_IDX_CHAR_VF]          = { BLE_GATT_DECL_CHARACTERISTIC,     PROP(RD), 0 },
    [SERVICE_B5_IDX_VF]               = { APP_GATT_DECL_VF,                 PROP(RD) , OPT(NO_OFFSET) | sizeof(uint16_t) },
    [SERVICE_B5_IDX_CHAR_FORMAT_VF]   = { BLE_GATT_DESC_CHAR_PRES_FORMAT,   PROP(RD) , 0 },

    [SERVICE_B5_IDX_CHAR_V6]          = { BLE_GATT_DECL_CHARACTERISTIC,     PROP(RD), 0 },
    [SERVICE_B5_IDX_V6]               = { APP_GATT_DECL_V6,                 PROP(RD) , OPT(NO_OFFSET) | sizeof(uint16_t) },
    [SERVICE_B5_IDX_CHAR_FORMAT_V6]   = { BLE_GATT_DESC_CHAR_PRES_FORMAT,   PROP(RD) , 0 },

    [SERVICE_B5_IDX_CHAR_V7]          = { BLE_GATT_DECL_CHARACTERISTIC,     PROP(RD), 0 },
    [SERVICE_B5_IDX_V7]               = { APP_GATT_DECL_V7,                 PROP(RD) , OPT(NO_OFFSET) | sizeof(uint16_t) },
    [SERVICE_B5_IDX_CHAR_FORMAT_V7]   = { BLE_GATT_DESC_CHAR_PRES_FORMAT,   PROP(RD) , 0 },

    [SERVICE_B5_IDX_CHAR_V10_1]            = { BLE_GATT_DECL_CHARACTERISTIC,            PROP(RD), 0 },
    [SERVICE_B5_IDX_V10_1]                 = { APP_GATT_DECL_V10,                       PROP(RD), 0 },
    [SERVICE_B5_IDX_CHAR_AGG_FORMAT_V10_1] = { BLE_GATT_DESC_CHAR_AGGREGATE_FORMAT,     PROP(RD), 0 },

    [SERVICE_B5_IDX_CHAR_V11]          = { BLE_GATT_DECL_CHARACTERISTIC,     PROP(RD), 0 },
    [SERVICE_B5_IDX_V11]               = { APP_GATT_DECL_VE,                 PROP(RD)| PROP(WR), OPT(NO_OFFSET) | sizeof(uint16_t) },

    [SERVICE_B5_IDX_CHAR_V10_2]            = { BLE_GATT_DECL_CHARACTERISTIC,            PROP(RD), 0 },
    [SERVICE_B5_IDX_V10_2]                 = { APP_GATT_DECL_V10,                       PROP(RD), 0 },
    [SERVICE_B5_IDX_CHAR_AGG_FORMAT_V10_2] = { BLE_GATT_DESC_CHAR_AGGREGATE_FORMAT,     PROP(RD), 0 },

    [SERVICE_B5_IDX_CHAR_V10_3]            = { BLE_GATT_DECL_CHARACTERISTIC,            PROP(RD), 0 },
    [SERVICE_B5_IDX_V10_3]                 = { APP_GATT_DECL_V10,                       PROP(RD), 0 },
    [SERVICE_B5_IDX_CHAR_AGG_FORMAT_V10_3] = { BLE_GATT_DESC_CHAR_AGGREGATE_FORMAT,     PROP(RD), 0 },

    [SERVICE_B5_IDX_CHAR_V10_4]            = { BLE_GATT_DECL_CHARACTERISTIC,            PROP(RD), 0 },
    [SERVICE_B5_IDX_V10_4]                 = { APP_GATT_DECL_V10,                       PROP(RD), 0 },
    [SERVICE_B5_IDX_CHAR_AGG_FORMAT_V10_4] = { BLE_GATT_DESC_CHAR_AGGREGATE_FORMAT,     PROP(RD), 0 },

    [SERVICE_B5_IDX_CHAR_V10_5]            = { BLE_GATT_DECL_CHARACTERISTIC,            PROP(RD), 0 },
    [SERVICE_B5_IDX_V10_5]                 = { APP_GATT_DECL_V10,                       PROP(RD), 0 },
    [SERVICE_B5_IDX_CHAR_AGG_FORMAT_V10_5] = { BLE_GATT_DESC_CHAR_AGGREGATE_FORMAT,     PROP(RD), 0 },


    // ---------------------------------- SERVICE E ---------------------------------------------------------------
    // Service E
    [SERVICE_E_IDX_PRIM_SVC]     = { BLE_GATT_DECL_PRIMARY_SERVICE, PROP(RD), 0 },

    [SERVICE_E_IDX_CHAR_V13]     = { BLE_GATT_DECL_CHARACTERISTIC,  PROP(RD), 0 },
    [SERVICE_E_IDX_V13]          = { APP_GATT_DECL_VD,              PROP(RD) | PROP(WR), OPT(NO_OFFSET) | sizeof(uint16_t) },
};

app_gatt_bqb_svc_info_t app_bqb_svc_infos[] = {
    {UUID_PRIVATE_128(APP_GATT_DECL_SERVICE_C),    SERVICE_C2_IDX_PRIM_SVC, 0x0030, SERVICE_C2_NB_ATT, SVC_UUID(128), 0, svc_c2_rw_cb},
    {UUID_PRIVATE_128(APP_GATT_DECL_SERVICE_C),    SERVICE_C1_IDX_PRIM_SVC, 0x0040, SERVICE_C1_NB_ATT, SVC_UUID(128), 0, svc_c1_rw_cb},
    {ATT_16_TO_128_ARRAY(APP_GATT_DECL_SERVICE_D), SERVICE_D_IDX_PRIM_SVC,  0x0050, SERVICE_D_NB_ATT,  0, 0, NULL},
    {ATT_16_TO_128_ARRAY(APP_GATT_DECL_SERVICE_B), SERVICE_B1_IDX_PRIM_SVC, 0x0060, SERVICE_B1_NB_ATT, BLE_GATT_SVC_EKS_BIT | SVC_SEC_LVL_VAL(BLE_GAP_SEC_UNAUTH), 0, NULL},
    {ATT_16_TO_128_ARRAY(APP_GATT_DECL_SERVICE_A), SERVICE_A_IDX_PRIM_SVC,  0x0070, SERVICE_A_NB_ATT,  0, 0, NULL},
    {ATT_16_TO_128_ARRAY(APP_GATT_DECL_SERVICE_B), SERVICE_B3_IDX_PRIM_SVC, 0x0080, SERVICE_B3_NB_ATT, 0, 0, NULL},
    {ATT_16_TO_128_ARRAY(APP_GATT_DECL_SERVICE_B), SERVICE_B2_IDX_PRIM_SVC, 0x0090, SERVICE_B2_NB_ATT, 0, 0, svc_b2_rw_cb},
    {ATT_16_TO_128_ARRAY(APP_GATT_DECL_SERVICE_B), SERVICE_B5_IDX_PRIM_SVC, 0x00a0, SERVICE_B5_NB_ATT, BLE_GATT_SVC_DIS_BIT, 0, svc_b5_rw_cb},
    {ATT_16_TO_128_ARRAY(APP_GATT_DECL_SERVICE_E), SERVICE_E_IDX_PRIM_SVC,  0xFFFD, SERVICE_E_NB_ATT,  SVC_SEC_LVL_VAL(BLE_GAP_SEC_AUTH), 0, NULL},
};

struct app_gatt_bqb_env_tag app_gatt_bqb_env;

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static void app_gatt_bqb_pack_char_pres_fmt(uint8_t *p_buf, prf_char_pres_fmt_t *char_pres_fmt)
{
    p_buf[0] = char_pres_fmt->format;
    p_buf[1] = char_pres_fmt->exponent;
    ble_write16p(&p_buf[2], char_pres_fmt->unit);
    p_buf[4] = char_pres_fmt->name_space;
    ble_write16p(&p_buf[5], char_pres_fmt->description);
}

uint8_t svc_c2_rw_cb(cb_data_t *cb_data)
{
    uint16_t  att_idx = cb_data->msg_data.index + SERVICE_C2_IDX_PRIM_SVC;
    uint16_t length  = 0;
    uint8_t status = BLE_ERR_NO_ERROR;
    uint16_t i  = 0;

    if (att_idx < SERVICE_E_IDX_NUMBER) {
        length = GETF(app_svc_db[att_idx].ext_info, BLE_GATT_ATTR_WRITE_MAX_SIZE);
    }

    if (cb_data->msg_type == SRV_WRITE_CHAR_VALUE_CALLBACK) {
        dbg_print(NOTICE, "svc_c2_rw_cb, write: att idx %d, handle 0x%x, offset %d, value_len %d, value: ",
               att_idx, cb_data->msg_data.handle, cb_data->msg_data.offset, cb_data->msg_data.param.val_len);

        for (i = 0; i < cb_data->msg_data.param.val_len; i++) {
            dbg_print(NOTICE, "%02x", cb_data->msg_data.param.p_val[i]);
        }
        dbg_print(NOTICE, "\r\n");
        switch (att_idx) {
        case SERVICE_C2_IDX_V10: {
            memcpy(&app_gatt_bqb_env.app_srv_att_info.value_v10, cb_data->msg_data.param.p_val,
                   sizeof(uint16_t));
        }
        break;

        case SERVICE_C2_IDX_V2B: {
            length = ble_min(length, cb_data->msg_data.param.val_len);
            memcpy(app_gatt_bqb_env.app_srv_att_info.value_v2b, cb_data->msg_data.param.p_val, length);
        }
        break;

        case SERVICE_C2_IDX_V2C: {
            length = ble_min(length, cb_data->msg_data.param.val_len);
            memcpy(app_gatt_bqb_env.app_srv_att_info.value_v2c, cb_data->msg_data.param.p_val, length);
        }
        break;

        case SERVICE_C2_IDX_V2D: {
            length = ble_min(length -  cb_data->msg_data.offset, cb_data->msg_data.param.val_len);
            memcpy(&app_gatt_bqb_env.app_srv_att_info.value_v2d[ cb_data->msg_data.offset],
                   cb_data->msg_data.param.p_val, length);
        }
        break;

        default:
            break;
        }
    } else if (cb_data->msg_type == SRV_READ_CHAR_VALUE_CALLBACK) {
        dbg_print(NOTICE, "svc_c2_rw_cb, read: att idx %d, handle %d, offset %d, value_len %d\r\n",
               att_idx, cb_data->msg_data.handle, cb_data->msg_data.offset, cb_data->msg_data.param.val_len);
        switch (att_idx) {
        case SERVICE_C2_IDX_V10: {
            ble_write16p(cb_data->msg_data.param.p_val, app_gatt_bqb_env.app_srv_att_info.value_v10);
        }
        break;

        case SERVICE_C2_IDX_V2B: {
            memcpy(cb_data->msg_data.param.p_val, app_gatt_bqb_env.app_srv_att_info.value_v2b, length);
        }
        break;

        case SERVICE_C2_IDX_V2C: {
            if (cb_data->msg_data.offset > length) {
                status = BLE_ATT_ERR_INVALID_OFFSET;
                break;
            }

            length = ble_min(length - cb_data->msg_data.offset, cb_data->msg_data.max_len);
            memcpy(cb_data->msg_data.param.p_val, app_gatt_bqb_env.app_srv_att_info.value_v2c, length);
        }
        break;

        case SERVICE_C2_IDX_V2D: {
            if (cb_data->msg_data.offset > length) {
                status = BLE_ATT_ERR_INVALID_OFFSET;
                break;
            }

            length = ble_min(length - cb_data->msg_data.offset, cb_data->msg_data.max_len);
            memcpy(cb_data->msg_data.param.p_val,
                   &app_gatt_bqb_env.app_srv_att_info.value_v2d[cb_data->msg_data.offset], length);
        }
        break;

        default:
            break;
        }
        cb_data->msg_data.param.val_len = length;
        cb_data->msg_data.param.attr_len = length;
    }
    return status;
}

uint8_t svc_c1_rw_cb(cb_data_t *cb_data)
{
    uint16_t  att_idx = cb_data->msg_data.index + SERVICE_C1_IDX_PRIM_SVC;
    uint16_t length  = 0;
    uint8_t status = BLE_ERR_NO_ERROR;
    uint16_t i  = 0;

    if (att_idx < SERVICE_E_IDX_NUMBER) {
        length = GETF(app_svc_db[att_idx].ext_info, BLE_GATT_ATTR_WRITE_MAX_SIZE);
    }

    if (cb_data->msg_type == SRV_WRITE_CHAR_VALUE_CALLBACK) {
        dbg_print(NOTICE, "svc_c1_rw_cb, write: att idx %d, handle %d, offset %d, value_len %d, value: ",
               att_idx, cb_data->msg_data.handle, cb_data->msg_data.offset, cb_data->msg_data.param.val_len);


        for (i = 0; i < cb_data->msg_data.param.val_len; i++) {
            dbg_print(NOTICE, "%02x", cb_data->msg_data.param.p_val[i]);
        }
        dbg_print(NOTICE, "\r\n");
        switch (att_idx) {
        case SERVICE_C1_IDX_V9: {
            memcpy(&app_gatt_bqb_env.app_srv_att_info.value_v9, cb_data->msg_data.param.p_val,
                   sizeof(uint16_t));
        }
        break;

        case SERVICE_C1_IDX_DESC_V9D2: {
            memcpy(&app_gatt_bqb_env.app_srv_att_info.value_v9d2, cb_data->msg_data.param.p_val,
                   sizeof(uint16_t));
        }
        break;


        default:
            break;
        }
    } else if (cb_data->msg_type == SRV_READ_CHAR_VALUE_CALLBACK) {
        dbg_print(NOTICE, "svc_c1_rw_cb, read: att idx %d, handle %d, offset %d, value_len %d\r\n",
               att_idx, cb_data->msg_data.handle, cb_data->msg_data.offset, cb_data->msg_data.param.val_len);
        switch (att_idx) {
        case SERVICE_C1_IDX_V9: {
            ble_write16p(cb_data->msg_data.param.p_val, app_gatt_bqb_env.app_srv_att_info.value_v9);
        }
        break;

        case SERVICE_C1_IDX_DESC_V9D2: {
            ble_write16p(cb_data->msg_data.param.p_val, app_gatt_bqb_env.app_srv_att_info.value_v9d2);
        }
        break;

        default:
            break;
        }
        cb_data->msg_data.param.val_len = length;
        cb_data->msg_data.param.attr_len = length;
    }
    return status;
}

uint8_t svc_b2_rw_cb(cb_data_t *cb_data)
{
    uint16_t  att_idx = cb_data->msg_data.index + SERVICE_B2_IDX_PRIM_SVC;
    uint16_t length  = 0;
    uint8_t status = BLE_ERR_NO_ERROR;
    uint16_t i  = 0;

    if (att_idx < SERVICE_E_IDX_NUMBER) {
        length = GETF(app_svc_db[att_idx].ext_info, BLE_GATT_ATTR_WRITE_MAX_SIZE);
    }

    if (cb_data->msg_type == SRV_WRITE_CHAR_VALUE_CALLBACK) {
        dbg_print(NOTICE, "svc_b2_rw_cb, write: att idx %d, handle %d, offset %d, value_len %d, value: ",
               att_idx, cb_data->msg_data.handle, cb_data->msg_data.offset, cb_data->msg_data.param.val_len);


        for (i = 0; i < cb_data->msg_data.param.val_len; i++) {
            dbg_print(NOTICE, "%02x", cb_data->msg_data.param.p_val[i]);
        }
        dbg_print(NOTICE, "\r\n");
        switch (att_idx) {
        case SERVICE_B2_IDX_V5: {
            memcpy(&app_gatt_bqb_env.app_srv_att_info.value_v5, cb_data->msg_data.param.p_val,
                   sizeof(uint16_t));
        }
        break;

        case SERVICE_B2_IDX_CHAR_USER_V5: {
            length = ble_min(length - cb_data->msg_data.offset, cb_data->msg_data.param.val_len);
            memcpy(&app_gatt_bqb_env.app_srv_att_info.value_v5_char_user_desc[cb_data->msg_data.offset],
                   cb_data->msg_data.param.p_val, length);
        }
        break;

        default:
            break;
        }
    } else if (cb_data->msg_type == SRV_READ_CHAR_VALUE_CALLBACK) {
        dbg_print(NOTICE, "svc_b2_rw_cb, read: att idx %d, handle %d, offset %d, value_len %d\r\n",
               att_idx, cb_data->msg_data.handle, cb_data->msg_data.offset, cb_data->msg_data.param.val_len);
        switch (att_idx) {
        case SERVICE_B2_IDX_V5: {
            *cb_data->msg_data.param.p_val = app_gatt_bqb_env.app_srv_att_info.value_v5;
        }
        break;

        case SERVICE_B2_IDX_CHAR_USER_V5: {
            if (cb_data->msg_data.offset > length) {
                status = BLE_ATT_ERR_INVALID_OFFSET;
                break;
            }

            length = ble_min(length - cb_data->msg_data.offset, cb_data->msg_data.max_len);
            memcpy(cb_data->msg_data.param.p_val,
                   &app_gatt_bqb_env.app_srv_att_info.value_v5_char_user_desc[cb_data->msg_data.offset], length);
        }
        break;

        case SERVICE_B2_IDX_CHAR_FORMAT_V5: {
            prf_char_pres_fmt_t value_v5_char_format;
            value_v5_char_format.unit = 0x3001;
            value_v5_char_format.description = 0x3111;
            value_v5_char_format.format = 0x04;
            value_v5_char_format.exponent = 0x00;
            value_v5_char_format.name_space = 0x01;
            app_gatt_bqb_pack_char_pres_fmt(cb_data->msg_data.param.p_val, &value_v5_char_format);
            length = APP_GATT_CHAR_PRES_FMT_LEN;
        }
        break;

        case SERVICE_B2_IDX_V1: {
            ble_write16p(cb_data->msg_data.param.p_val, app_gatt_bqb_env.app_srv_att_info.value_v1);
        }
        break;

        case SERVICE_B2_IDX_CHAR_FORMAT_V1: {
            prf_char_pres_fmt_t value_v1_char_format;
            value_v1_char_format.unit = 0x2710;
            value_v1_char_format.description = 0x0002;
            value_v1_char_format.format = 0x06;
            value_v1_char_format.exponent = 0x00;
            value_v1_char_format.name_space = 0x01;
            app_gatt_bqb_pack_char_pres_fmt(cb_data->msg_data.param.p_val, &value_v1_char_format);
            length = APP_GATT_CHAR_PRES_FMT_LEN;
        }
        break;

        case SERVICE_B2_IDX_V2: {
            *cb_data->msg_data.param.p_val = app_gatt_bqb_env.app_srv_att_info.value_v5;
            ble_write16p(cb_data->msg_data.param.p_val + 1, app_gatt_bqb_env.app_srv_att_info.value_v1);
            length = sizeof(uint8_t) + sizeof(uint16_t);
        }
        break;

        case SERVICE_B2_IDX_CHAR_AGG_FORMAT_V2: {
            ble_write16p(cb_data->msg_data.param.p_val,
                         cb_data->msg_data.handle - (att_idx - SERVICE_B2_IDX_CHAR_FORMAT_V5));
            ble_write16p(cb_data->msg_data.param.p_val + BLE_GATT_HANDLE_LEN,
                         cb_data->msg_data.handle - (att_idx - SERVICE_B2_IDX_CHAR_FORMAT_V1));
            length = BLE_GATT_HANDLE_LEN + BLE_GATT_HANDLE_LEN;
        }
        break;


        default:
            break;
        }
        cb_data->msg_data.param.val_len = length;
        cb_data->msg_data.param.attr_len = length;
    }
    return status;
}

uint8_t svc_b5_rw_cb(cb_data_t *cb_data)
{
    uint16_t  att_idx = cb_data->msg_data.index + SERVICE_B5_IDX_PRIM_SVC;
    uint16_t length  = 0;
    uint8_t status = BLE_ERR_NO_ERROR;
    uint16_t i  = 0;

    if (att_idx < SERVICE_E_IDX_NUMBER) {
        length = GETF(app_svc_db[att_idx].ext_info, BLE_GATT_ATTR_WRITE_MAX_SIZE);
    }

    if (cb_data->msg_type == SRV_WRITE_CHAR_VALUE_CALLBACK) {
        dbg_print(NOTICE, "svc_b5_rw_cb, write: att idx %d, handle %d, offset %d, value_len %d, value: ",
               att_idx, cb_data->msg_data.handle, cb_data->msg_data.offset, cb_data->msg_data.param.val_len);


        for (i = 0; i < cb_data->msg_data.param.val_len; i++) {
            dbg_print(NOTICE, "%02x", cb_data->msg_data.param.p_val[i]);
        }
        dbg_print(NOTICE, "\r\n");
        switch (att_idx) {
        default:
            break;
        }
    } else if (cb_data->msg_type == SRV_READ_CHAR_VALUE_CALLBACK) {
        dbg_print(NOTICE, "svc_b5_rw_cb, read: att idx %d, handle %d, offset %d, value_len %d\r\n",
               att_idx, cb_data->msg_data.handle, cb_data->msg_data.offset, cb_data->msg_data.param.val_len);
        switch (att_idx) {

        case SERVICE_B5_IDX_CHAR_FORMAT_VE: {
            prf_char_pres_fmt_t value_ve_char_format;
            value_ve_char_format.unit = 0x3000;
            value_ve_char_format.description =  0x0002;
            value_ve_char_format.format = 0x19;
            value_ve_char_format.exponent = 0x00;
            value_ve_char_format.name_space = 0x01;
            app_gatt_bqb_pack_char_pres_fmt(cb_data->msg_data.param.p_val, &value_ve_char_format);
            length = APP_GATT_CHAR_PRES_FMT_LEN;
        }
        break;

        case SERVICE_B5_IDX_CHAR_FORMAT_VF: {
            prf_char_pres_fmt_t value_vf_char_format;
            value_vf_char_format.unit = 0x2701;
            value_vf_char_format.description =  0x0002;
            value_vf_char_format.format = 0x04;
            value_vf_char_format.exponent = 0x00;
            value_vf_char_format.name_space = 0x01;
            app_gatt_bqb_pack_char_pres_fmt(cb_data->msg_data.param.p_val, &value_vf_char_format);
            length = APP_GATT_CHAR_PRES_FMT_LEN;
        }
        break;

        case SERVICE_B5_IDX_CHAR_FORMAT_V6: {
            prf_char_pres_fmt_t value_v6_char_format;
            value_v6_char_format.unit = 0x2710;
            value_v6_char_format.description =  0x0002;
            value_v6_char_format.format = 0x06;
            value_v6_char_format.exponent = 0x00;
            value_v6_char_format.name_space = 0x01;
            app_gatt_bqb_pack_char_pres_fmt(cb_data->msg_data.param.p_val, &value_v6_char_format);
            length = APP_GATT_CHAR_PRES_FMT_LEN;
        }
        break;

        case SERVICE_B5_IDX_CHAR_FORMAT_V7: {
            prf_char_pres_fmt_t value_v7_char_format;
            value_v7_char_format.unit = 0x2717;
            value_v7_char_format.description =  0x0002;
            value_v7_char_format.format = 0x08;
            value_v7_char_format.exponent = 0x00;
            value_v7_char_format.name_space = 0x01;
            app_gatt_bqb_pack_char_pres_fmt(cb_data->msg_data.param.p_val, &value_v7_char_format);
            length = APP_GATT_CHAR_PRES_FMT_LEN;
        }
        break;

        case SERVICE_B5_IDX_CHAR_AGG_FORMAT_V10_1: {
            ble_write16p(cb_data->msg_data.param.p_val,
                         cb_data->msg_data.handle - (att_idx - SERVICE_B5_IDX_CHAR_FORMAT_VF));
            ble_write16p(cb_data->msg_data.param.p_val + BLE_GATT_HANDLE_LEN,
                         cb_data->msg_data.handle - (att_idx - SERVICE_B5_IDX_CHAR_FORMAT_V6));
            ble_write16p(cb_data->msg_data.param.p_val + BLE_GATT_HANDLE_LEN + BLE_GATT_HANDLE_LEN,
                         cb_data->msg_data.handle - (att_idx - SERVICE_B5_IDX_CHAR_FORMAT_V7));
            length = BLE_GATT_HANDLE_LEN + BLE_GATT_HANDLE_LEN + BLE_GATT_HANDLE_LEN;
        }
        break;

        case SERVICE_B5_IDX_CHAR_AGG_FORMAT_V10_2: {
            ble_write16p(cb_data->msg_data.param.p_val,
                         cb_data->msg_data.handle - (att_idx - SERVICE_B5_IDX_CHAR_FORMAT_VF));
            ble_write16p(cb_data->msg_data.param.p_val + BLE_GATT_HANDLE_LEN,
                         cb_data->msg_data.handle - (att_idx - SERVICE_B5_IDX_CHAR_FORMAT_VF));
            length = BLE_GATT_HANDLE_LEN + BLE_GATT_HANDLE_LEN;
        }
        break;

        case SERVICE_B5_IDX_CHAR_AGG_FORMAT_V10_3: {
            ble_write16p(cb_data->msg_data.param.p_val,
                         cb_data->msg_data.handle - (att_idx - SERVICE_B5_IDX_CHAR_FORMAT_V7));
            ble_write16p(cb_data->msg_data.param.p_val + BLE_GATT_HANDLE_LEN,
                         cb_data->msg_data.handle - (att_idx - SERVICE_B5_IDX_CHAR_FORMAT_V6));
            length = BLE_GATT_HANDLE_LEN + BLE_GATT_HANDLE_LEN;
        }
        break;

        case SERVICE_B5_IDX_CHAR_AGG_FORMAT_V10_4: {
            ble_write16p(cb_data->msg_data.param.p_val,
                         cb_data->msg_data.handle - (att_idx - SERVICE_B5_IDX_CHAR_FORMAT_VF));
            ble_write16p(cb_data->msg_data.param.p_val + BLE_GATT_HANDLE_LEN,
                         cb_data->msg_data.handle - (att_idx - SERVICE_B5_IDX_CHAR_FORMAT_VF));
            length = BLE_GATT_HANDLE_LEN + BLE_GATT_HANDLE_LEN;
        }
        break;

        case SERVICE_B5_IDX_CHAR_AGG_FORMAT_V10_5: {
            ble_write16p(cb_data->msg_data.param.p_val,
                         cb_data->msg_data.handle - (att_idx - SERVICE_B5_IDX_CHAR_FORMAT_VF));
            ble_write16p(cb_data->msg_data.param.p_val + BLE_GATT_HANDLE_LEN,
                         cb_data->msg_data.handle - (att_idx - SERVICE_B5_IDX_CHAR_FORMAT_V6));
            length = BLE_GATT_HANDLE_LEN + BLE_GATT_HANDLE_LEN;
        }
        break;

        default:
            break;
        }
        cb_data->msg_data.param.val_len = length;
        cb_data->msg_data.param.attr_len = length;
    }
    return status;
}


static uint8_t app_svc_bqb_hdl_idx_get(uint16_t hdl)
{
    int cursor;
    uint16_t att_idx = SERVICE_E_IDX_NUMBER;

    for (cursor = ARRAY_LEN(app_bqb_svc_infos) - 1; cursor >= 0; cursor--) {
        app_gatt_bqb_svc_info_t app_bqb_svc_info = app_bqb_svc_infos[cursor];
        if (hdl > app_bqb_svc_info.start_hdl) {
            if (hdl - app_bqb_svc_info.start_hdl < app_bqb_svc_info.num) {
                att_idx = hdl - app_bqb_svc_info.start_hdl + app_bqb_svc_info.index;
            }
            break;
        }
    }

    return att_idx;
}

void app_gatt_bqb_srv_event_reliable_send(uint8_t conidx, uint8_t evt_type, uint8_t nb_attr,
                                          ble_gatt_attr_t *p_attr)
{
    ble_gatts_ntf_ind_reliable_send(conidx, p_attr, nb_attr, evt_type);
}

void app_gatt_bgb_srv_event_send(uint8_t conidx, uint8_t evt_type, uint16_t hdl)
{
    uint8_t  att_idx = app_svc_bqb_hdl_idx_get(hdl);
    uint16_t length  = 0;
    uint8_t *p_data = NULL;

    if (att_idx < SERVICE_E_IDX_NUMBER) {
        length = GETF(app_svc_db[att_idx].ext_info, BLE_GATT_ATTR_WRITE_MAX_SIZE);
    }

    if (length) {
        p_data = sys_malloc(length);

        if (p_data == NULL) {
            return;
        }
    }

    ble_gatts_ntf_ind_send_by_handle(conidx, hdl, p_data, length, evt_type);

    if (p_data) {
        sys_mfree(p_data);
    }
}

void app_gatt_bqb_init(void)
{
    uint8_t  value_v2b[VALUE_V2B_LEN] = VALUE_V2B_VALUE;
    uint8_t  value_v2d[VALUE_V2D_LEN] = VALUE_V2D_VALUE;
    uint8_t  value_v5_char_user_desc[VALUE_V5_CHAR_USER_DESC_LEN] = VALUE_V5_CHAR_USER_DESC_VALUE;

    memset(&app_gatt_bqb_env, 0, sizeof(struct app_gatt_bqb_env_tag));
    app_gatt_bqb_env.gap_start_hdl = 0x0020;
    app_gatt_bqb_env.gatt_start_hdl = 0x0010;

    memcpy(app_gatt_bqb_env.app_srv_att_info.value_v2b, value_v2b, VALUE_V2B_LEN);
    memcpy(app_gatt_bqb_env.app_srv_att_info.value_v2d, value_v2d, VALUE_V2D_LEN);
    memcpy(app_gatt_bqb_env.app_srv_att_info.value_v5_char_user_desc, value_v5_char_user_desc,
           VALUE_V5_CHAR_USER_DESC_LEN);
    app_gatt_bqb_env.app_srv_att_info.value_v5 = 0x05;
    app_gatt_bqb_env.app_srv_att_info.value_v1 = 0x1234;
}

void app_gatt_bqb_srv_db_svc_list_get(void)
{
    ble_gatts_svc_list_get();
}

void app_gatt_bqb_srv_db_svc_remove_all(void)
{
    uint8_t srv_num = sizeof(app_bqb_svc_infos) / sizeof(app_gatt_bqb_svc_info_t);
    uint8_t srv_idx = 0;

    for (srv_idx = 0; srv_idx < srv_num; srv_idx++) {
        ble_gatts_svc_rmv(app_bqb_svc_infos[srv_idx].svc_idx);
    }

    ble_peer_data_bond_gatt_db_update();
}

void app_gatt_bqb_srv_db_svc_add_all(void)
{
    uint16_t cursor;
    ble_gatt_attr_desc_t *p_atts_desc_all;
    app_gatt_bqb_svc_info_t *p_app_svc_info;

    p_atts_desc_all = (ble_gatt_attr_desc_t *) sys_malloc(sizeof(ble_gatt_attr_desc_t) *
                                                          SERVICE_E_IDX_NUMBER);
    if (p_atts_desc_all == NULL) {
        dbg_print(ERR, "error: app_gatt_bqb_srv_init insufficient resource");
        return;
    }

    for (cursor = 0 ; cursor < SERVICE_E_IDX_NUMBER ; cursor++) {
        ble_gatt_attr_desc_t *p_atts_desc = &p_atts_desc_all[cursor];
        ble_gatt_attr_16_desc_t *p_atts16_desc = &app_svc_db[cursor];
        // fill attribute description information
        p_atts_desc->info     = p_atts16_desc->info;
        p_atts_desc->ext_info = p_atts16_desc->ext_info;
        ble_write16p(p_atts_desc->uuid, p_atts16_desc->uuid16);

        switch (cursor) {
        case SERVICE_C1_IDX_DESC_V9D2:
        case SERVICE_C1_IDX_DESC_V9D3:
        case SERVICE_B2_IDX_DESC_V5D4: {
            uint8_t uuid[BLE_GATT_UUID_128_LEN] = UUID_PRIVATE_128(p_atts16_desc->uuid16);
            SETF(p_atts_desc->info, BLE_GATT_ATTR_UUID_TYPE, BLE_GATT_UUID_128);
            memcpy(p_atts_desc->uuid, uuid, BLE_GATT_UUID_128_LEN);
        }
        break;

        default:
            break;
        }
    }


    // Allocate service information
    for (cursor = 0 ; cursor < ARRAY_LEN(app_bqb_svc_infos); cursor++) {
        p_app_svc_info = &app_bqb_svc_infos[cursor];
        ble_gatts_svc_add(&p_app_svc_info->svc_idx, p_app_svc_info->uuid, p_app_svc_info->start_hdl, p_app_svc_info->info,
                        &p_atts_desc_all[p_app_svc_info->index], p_app_svc_info->num, p_app_svc_info->rw_cb);
    }

    // free attribute array
    if (p_atts_desc_all != NULL) {
        sys_mfree(p_atts_desc_all);
    }

    ble_peer_data_bond_gatt_db_update();
}

void app_gatt_bqb_cli_discover_svc(uint8_t conidx, uint8_t disc_type, uint8_t full, uint16_t start_hdl,
                               uint16_t end_hdl, uint8_t uuid_type, uint8_t *uuid)
{
    ble_bqb_cli_discover_svc(conidx, disc_type, full,
                          start_hdl, end_hdl, uuid_type, uuid);
}

void app_gatt_bqb_cli_discover_inc_svc(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl)
{
    ble_bqb_cli_discover_inc_svc(conidx, start_hdl, end_hdl);
}

void app_gatt_bqb_cli_discover_char(uint8_t conidx, uint8_t disc_type, uint16_t start_hdl,
                                uint16_t end_hdl, uint8_t uuid_type, uint8_t *uuid)
{
    ble_bqb_cli_discover_char(conidx, disc_type, start_hdl, end_hdl, uuid_type,
                           uuid);
}

void app_gatt_bqb_cli_discover_desc(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl)
{
    ble_bqb_cli_discover_desc(conidx, start_hdl, end_hdl);
}

void app_gatt_bqb_cli_discover_cancel(uint8_t conidx)
{
    ble_bqb_cli_discover_cancel(conidx);
}

void app_gatt_bqb_cli_read(uint8_t conidx, uint16_t hdl, uint16_t offset, uint16_t length)
{
    ble_bqb_cli_read(conidx, hdl, offset, length);
}

void app_gatt_bqb_cli_read_by_uuid(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl,
                               uint8_t uuid_type, uint8_t *uuid)
{
    ble_bqb_cli_read_by_uuid(conidx, start_hdl, end_hdl, uuid_type, uuid);
}

void app_gatt_bqb_cli_read_multiple(uint8_t conidx, uint8_t nb_att, ble_gatt_attr_t *atts)
{
    ble_bqb_cli_read_multiple(conidx, nb_att, atts);
}

void app_gatt_bqb_cli_write_reliable(uint8_t conidx, uint8_t write_type, uint8_t write_mode,
                                 uint16_t hdl, uint16_t offset, uint16_t length)
{
    ble_bqb_cli_write_reliable(conidx, write_type, write_mode, hdl, offset,
                                length);
}

void app_gatt_bqb_cli_write(uint8_t conidx, uint8_t write_type, uint16_t hdl, uint16_t value_length,
                        uint16_t *value)
{
    uint16_t i;
    uint8_t *p_data;

    p_data = sys_malloc(value_length);

    if (p_data == NULL) {
        return;
    }

    if (value_length <= 2 && value != NULL) {
        memcpy(p_data, value, value_length);
    } else {
        for (i = 0; i < value_length; i++) {
            p_data[i] = i;
        }
    }

    ble_bqb_cli_write(conidx, write_type, hdl, value_length, p_data);
}

void app_gatt_bqb_cli_write_exe(uint8_t conidx, uint8_t execute)
{
    ble_bqb_cli_write_exe(conidx, execute);
}

void app_gatt_bqb_cli_event_register(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl)
{
    ble_gattc_event_register(conidx, start_hdl, end_hdl);
}

void app_gatt_bqb_eatt_estab(uint8_t conidx)
{
    ble_bqb_bearer_eatt_estab(conidx);
}

#endif /* APP_GATT_BQB_SUPPORT */
