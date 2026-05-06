/*!
    \file    ble_gattc_co.h
    \brief   Definitions of gatt client common.

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

#ifndef _BLE_GATTC_CO_H_
#define _BLE_GATTC_CO_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include "ble_gatt.h"
#include "ble_conn.h"
#include "ble_types.h"

#ifdef __cplusplus
extern "C" {
#endif
/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
*/
typedef enum
{
    BLE_CLI_CO_EVT_DISC_SVC_INFO_IND,
    BLE_CLI_CO_EVT_DISC_SVC_RSP,
    BLE_CLI_CO_EVT_DISC_INC_SVC_INFO_IND,
    BLE_CLI_CO_EVT_DISC_INC_SVC_RSP,
    BLE_CLI_CO_EVT_DISC_CHAR_INFO_IND,
    BLE_CLI_CO_EVT_DISC_CHAR_RSP,
    BLE_CLI_CO_EVT_DISC_DESC_INFO_IND,
    BLE_CLI_CO_EVT_DISC_DESC_RSP,
    BLE_CLI_CO_EVT_READ_RSP,
    BLE_CLI_CO_EVT_WRITE_RSP,
    BLE_CLI_CO_EVT_WRITE_RELIABLE_RSP,
    BLE_CLI_CO_EVT_ATTR_VAL_GET_IND,
    BLE_CLI_CO_EVT_WRITE_EXE_RSP,
    BLE_CLI_CO_EVT_NTF_IND,
    BLE_CLI_CO_EVT_MTU_UPDATE_RSP,
    BLE_CLI_CO_EVT_MTU_INFO,
    BLE_CLI_CO_EVT_CONN_STATE_CHANGE_IND,
    BLE_CLI_CO_EVT_SVC_INFO,
} ble_gattc_co_evt_t;

typedef struct
{
    uint8_t              disc_info;     /*!< Discovery information, @ref ble_gatt_svc_disc_info */
    uint8_t              attr_num;      /*!< Number of attribute */
    ble_gatt_svc_attr_t *p_attr;        /*!< Attribute information present in a service */
} ble_gattc_co_svc_info_t;

typedef struct
{
    ble_uuid_t ble_uuid;
    uint16_t   start_hdl;
    uint16_t   end_hdl;
} ble_gattc_co_disc_svc_ind_t;

typedef struct
{
    ble_status_t    status;
    uint8_t         disc_type;
} ble_gattc_co_disc_svc_rsp_t;

typedef struct
{
    uint16_t    inc_svc_hdl;
    uint16_t    start_hdl;
    uint16_t    end_hdl;
    ble_uuid_t  ble_uuid;
} ble_gattc_co_disc_inc_svc_ind_t;

typedef struct
{
    ble_status_t    status;
} ble_gattc_co_disc_inc_svc_rsp_t;

typedef struct
{
    ble_uuid_t ble_uuid;
    uint8_t    prop;
    uint16_t   char_hdl;
    uint16_t   val_hdl;
} ble_gattc_co_disc_char_ind_t;

typedef struct
{
    ble_status_t    status;
} ble_gattc_co_disc_char_rsp_t;

typedef struct
{
    ble_uuid_t ble_uuid;
    uint16_t   desc_hdl;
} ble_gattc_co_disc_desc_ind_t;

typedef struct
{
    ble_status_t    status;
} ble_gattc_co_disc_desc_rsp_t;

typedef struct
{
    ble_status_t status;
    uint16_t     handle;
    uint16_t     length;
    uint8_t      *p_value;
} ble_gattc_co_read_rsp_t;

typedef struct
{
    ble_status_t status;
    uint16_t     handle;
    uint8_t      type;
} ble_gattc_co_write_rsp_t;

typedef struct
{
    ble_status_t status;
    uint16_t     handle;
    uint8_t      type;
} ble_gattc_co_write_reliable_rsp_t;

typedef struct
{
    uint16_t    token;          /*!< Token provided by GATT module that must be used in the confirm */
    uint16_t    hdl;            /*!< Attribute handle */
    uint16_t    offset;         /*!< Data offset */
    uint16_t    max_len;        /*!< Maximum value length to return */
} ble_gattc_co_attr_val_get_ind_t;

typedef struct
{
    ble_status_t status;
    bool         execute;
} ble_gattc_co_write_exe_rsp_t;

typedef struct
{
    uint16_t   handle;
    uint16_t   length;
    uint8_t    *p_value;
    bool       is_ntf;
} ble_gattc_co_ntf_ind_t;

typedef struct
{
    ble_status_t status;
} ble_gattc_co_mtu_update_rsp_t;

typedef struct
{
    uint16_t    mtu;
} ble_gattc_co_mtu_info_t;

typedef struct
{
    ble_gap_addr_t  peer_addr;
} ble_gattc_co_conn_info_t;

typedef struct
{
    uint16_t        reason;
} ble_gattc_co_disconn_info_t;

typedef struct
{
    ble_conn_state_t conn_state;
    union {
        ble_gattc_co_conn_info_t     conn_info;
        ble_gattc_co_disconn_info_t  disconn_info;
    } info;
} ble_gattc_co_conn_state_change_ind_t;

typedef struct
{
    ble_gattc_co_evt_t cli_cb_msg_type;
    uint8_t conn_idx;
    union
    {
        ble_gattc_co_disc_svc_ind_t             disc_svc_ind;
        ble_gattc_co_disc_svc_rsp_t             disc_svc_rsp;
        ble_gattc_co_disc_inc_svc_ind_t         disc_inc_svc_ind;
        ble_gattc_co_disc_inc_svc_rsp_t         disc_inc_svc_rsp;
        ble_gattc_co_disc_char_ind_t            disc_char_ind;
        ble_gattc_co_disc_char_rsp_t            disc_char_rsp;
        ble_gattc_co_disc_desc_ind_t            disc_desc_ind;
        ble_gattc_co_disc_desc_rsp_t            disc_desc_rsp;
        ble_gattc_co_read_rsp_t                 read_rsp;
        ble_gattc_co_write_rsp_t                write_rsp;
        ble_gattc_co_write_reliable_rsp_t       write_reliable_rsp;
        ble_gattc_co_attr_val_get_ind_t         attr_val_get_ind;
        ble_gattc_co_write_exe_rsp_t            write_exe_rsp;
        ble_gattc_co_ntf_ind_t                  ntf_ind;
        ble_gattc_co_mtu_update_rsp_t           mtu_update_rsp;
        ble_gattc_co_mtu_info_t                 mtu_info;
        ble_gattc_co_conn_state_change_ind_t    conn_state_chg_ind;
        ble_gattc_co_svc_info_t                 svc_info;
    } msg_data;
} ble_gattc_co_msg_info_t;

typedef ble_status_t (*p_fun_gattc_co_cb)(ble_gattc_co_msg_info_t *p_gattc_co_msg_info);
/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */
ble_status_t ble_gattc_co_disc_svc(uint8_t conn_idx, uint16_t start_hdl, uint16_t end_hdl);

ble_status_t ble_gattc_co_disc_svc_by_uuid(uint8_t conn_idx, uint16_t start_hdl,
                        uint16_t end_hdl, uint8_t uuid_type, uint8_t *p_uuid);

ble_status_t ble_gattc_co_disc_sec_svc(uint8_t conn_idx, uint16_t start_hdl, uint16_t end_hdl);

ble_status_t ble_gattc_co_disc_inc_svc(uint8_t conn_idx, uint16_t start_hdl, uint16_t end_hdl);

ble_status_t ble_gattc_co_disc_svc_all(uint8_t conn_idx, uint16_t start_hdl, uint16_t end_hdl);

ble_status_t ble_gattc_co_disc_sec_svc_all(uint8_t conn_idx, uint16_t start_hdl, uint16_t end_hdl);

ble_status_t ble_gattc_co_disc_char(uint8_t conn_idx, uint16_t start_hdl, uint16_t end_hdl);

ble_status_t ble_gattc_co_disc_char_by_uuid(uint8_t conn_idx, uint16_t start_hdl,
                            uint16_t end_hdl, uint8_t uuid_type, uint8_t *p_uuid);

ble_status_t ble_gattc_co_disc_desc(uint8_t conn_idx, uint16_t start_hdl, uint16_t end_hdl);

ble_status_t ble_gattc_co_cb_reg(p_fun_gattc_co_cb cb);

ble_status_t ble_gattc_co_cb_unreg(p_fun_gattc_co_cb cb);

ble_status_t ble_gattc_co_read(uint8_t conidx, uint16_t hdl, uint16_t offset, uint16_t length);

ble_status_t ble_gattc_co_write_req(uint8_t conidx, uint16_t hdl, uint16_t length, uint8_t *p_value);

ble_status_t ble_gattc_co_write_cmd(uint8_t conidx, uint16_t hdl, uint16_t length, uint8_t *p_value);

ble_status_t ble_gattc_co_write_signed(uint8_t conidx, uint16_t hdl, uint16_t length, uint8_t *p_value);

ble_status_t ble_gattc_co_mtu_update(uint8_t conidx, uint16_t mtu);

ble_status_t ble_gattc_co_write_prepare(uint8_t conn_idx, uint16_t hdl, uint16_t offset, uint16_t len);

ble_status_t ble_gattc_co_write_exe(uint8_t conn_idx, bool execute);

ble_status_t ble_gattc_co_write_auto(uint8_t conn_idx, uint16_t hdl, uint16_t offset, uint16_t len);

ble_status_t ble_gattc_co_attr_val_get_cfm(uint8_t conn_idx, uint16_t token,
                        uint16_t status, uint16_t val_len, uint8_t *p_val);

#ifdef __cplusplus
}
#endif

#endif // _BLE_GATTC_CO_H_
