/*!
    \file    ble_glps.c
    \brief   Implementation of glucose profile sensor.

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

#include <string.h>

#include "ble_utils.h"
#include "ble_gatt.h"
#include "ble_gatts.h"
#include "ble_glps.h"
#include "wrapper_os.h"
#include "ble_conn.h"
#include "dbg_print.h"
#include "dlist.h"
#include "ble_sec.h"

/* GLS Attribute database handle list */
typedef enum
{
    BLE_GLS_HDL_SVC,

    BLE_GLS_HDL_MEAS_CHAR,
    BLE_GLS_HDL_MEAS_VAL,
    BLE_GLS_HDL_MEAS_CLI_CHR_CFG_DESC,

    BLE_GLS_HDL_MEAS_CTX_CHAR,
    BLE_GLS_HDL_MEAS_CTX_VAL,
    BLE_GLS_HDL_MEAS_CTX_CLI_CHR_CFG_DESC,

    BLE_GLS_HDL_FEATURE_CHAR,
    BLE_GLS_HDL_FEATURE_VAL,
    BLE_GLS_HDL_FEATURE_CLI_CHR_CFG_DESC,

    BLE_GLS_HDL_REC_ACCESS_CTRL_CHAR,
    BLE_GLS_HDL_REC_ACCESS_CTRL_VAL,
    BLE_GLS_HDL_REC_ACCESS_CTRL_CL_CHR_CFG_DESC,

    BLE_GLS_HDL_NB,
} ble_gls_attr_db_handle_t;

/* Type of operation */
typedef enum
{
    BLE_GLPS_OP_MEAS_SEND,              /*!< Send Measurement */
    BLE_GLPS_OP_MEAS_SEND_WITH_CTX,     /*!< Send Measurement - Context data following */
    BLE_GLPS_OP_MEAS_CTX_SEND,          /*!< Send Measurement Context */
    BLE_GLPS_OP_RACP_RSP_SEND,          /*!< Record Access Control Point Response Indication */
    BLE_GLPS_OP_FEAT_SEND               /*!< Features Indication */
} glps_op_type_t;

/* State Flag Bit field */
typedef enum
{
    BLE_GPLS_BOND_DATA_PRESENT_BIT = 0x01,      /*!< True: Bond data set by application */
    BLE_GPLS_BOND_DATA_PRESENT_POS = 0,
    BLE_GPLS_SENDING_MEAS_BIT      = 0x02,      /*!< True: Module is sending measurement data */
    BLE_GPLS_SENDING_MEAS_POS      = 1,
} glps_flag_bf_t;

/* Ongoing operation information */
typedef struct glps_data_meta
{
    dlist_t               list;
    uint16_t              att_idx;
    ble_gatt_evt_type_t   evt_type;
    uint8_t               operation;
    uint16_t              val_len;
    uint8_t               buf[0];
} glps_data_meta_t;

/* Glucose service device information */
typedef struct
{
    dlist_t             list;
    dlist_t             wait_queue;     /*!< Operation Event TX wait queue */
    uint8_t             conn_id;        /*!< Connection index */
    uint8_t             flags;          /*!< Glucose service processing flags, @ref glps_flag_bf_t */
    uint8_t             evt_cfg;        /*!< Event configuration (notification/indication) */
    uint8_t             racp_op_code;   /*!< Control point operation on-going, @ref ble_glp_racp_op_code_t */
    bool                op_ongoing;     /*!< Operation On-going */
    bool                in_exe_op;      /*!< Prevent recursion in execute_operation function */
} glps_dev_t;

/* Glucose service server environment variable */
typedef struct ble_glps_env
{
    uint8_t                 glps_id;
    uint16_t                features;           /*!< Glucose Feature, @ref ble_glp_srv_feature_flag */
    uint8_t                 meas_ctx_supported; /*!< Measurement context supported */
    dlist_t                 dev_list;           /*!< Environment variable list for each connections */
    ble_glps_callback_t     callbacks;
} ble_glps_env_t;

/* GLS Database Description */
const ble_gatt_attr_desc_t ble_gls_attr_db[BLE_GLS_HDL_NB] = {
    [BLE_GLS_HDL_SVC]                             = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_PRIMARY_SERVICE),    PROP(RD),                                   0                  },

    [BLE_GLS_HDL_MEAS_CHAR]                       = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),     PROP(RD),                                   0                  },
    [BLE_GLS_HDL_MEAS_VAL]                        = {UUID_16BIT_TO_ARRAY(BLE_GATT_CHAR_GLUCOSE_MEAS),       PROP(NTF),                                  OPT(NO_OFFSET)     },
    [BLE_GLS_HDL_MEAS_CLI_CHR_CFG_DESC]           = {UUID_16BIT_TO_ARRAY(BLE_GATT_DESC_CLIENT_CHAR_CFG),    PROP(RD) | PROP(WR),                        OPT(NO_OFFSET)     },

    [BLE_GLS_HDL_MEAS_CTX_CHAR]                   = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),     PROP(RD),                                   0                  },
    [BLE_GLS_HDL_MEAS_CTX_VAL]                    = {UUID_16BIT_TO_ARRAY(BLE_GATT_CHAR_GLUCOSE_MEAS_CTX),   PROP(NTF),                                  OPT(NO_OFFSET)     },
    [BLE_GLS_HDL_MEAS_CTX_CLI_CHR_CFG_DESC]       = {UUID_16BIT_TO_ARRAY(BLE_GATT_DESC_CLIENT_CHAR_CFG),    PROP(RD) | PROP(WR),                        OPT(NO_OFFSET)     },

    [BLE_GLS_HDL_FEATURE_CHAR]                    = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),     PROP(RD),                                   0                  },
    [BLE_GLS_HDL_FEATURE_VAL]                     = {UUID_16BIT_TO_ARRAY(BLE_GATT_CHAR_GLUCOSE_FEATURE),    PROP(RD) | PROP(IND),                       OPT(NO_OFFSET)     },
    [BLE_GLS_HDL_FEATURE_CLI_CHR_CFG_DESC]        = {UUID_16BIT_TO_ARRAY(BLE_GATT_DESC_CLIENT_CHAR_CFG),    PROP(RD) | PROP(WR),                        OPT(NO_OFFSET)     },

    [BLE_GLS_HDL_REC_ACCESS_CTRL_CHAR]            = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),     PROP(RD),                                   0                  },
    [BLE_GLS_HDL_REC_ACCESS_CTRL_VAL]             = {UUID_16BIT_TO_ARRAY(BLE_GATT_CHAR_REC_ACCESS_CTRL_PT), PROP(IND) | SEC_LVL(WP, AUTH) | PROP(WR),   OPT(NO_OFFSET) | BLE_GLP_REC_ACCESS_CTRL_MAX_LEN},
    [BLE_GLS_HDL_REC_ACCESS_CTRL_CL_CHR_CFG_DESC] = {UUID_16BIT_TO_ARRAY(BLE_GATT_DESC_CLIENT_CHAR_CFG),    PROP(RD) | PROP(WR),                          OPT(NO_OFFSET)     },
};

static ble_glps_env_t *p_glps_env = NULL;
static const uint8_t ble_gls_uuid[2] = UUID_16BIT_TO_ARRAY(BLE_GATT_SVC_GLUCOSE);

/*!
    \brief      Allocate glucose service device structor
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     glps_dev_t *: pointer to the allocated glucose service device structor
*/
static glps_dev_t *glps_alloc_dev_by_conn_id(uint8_t conn_id)
{
    glps_dev_t *p_device = NULL;

    p_device = (glps_dev_t *)sys_malloc(sizeof(glps_dev_t));

    if (p_device == NULL) {
        dbg_print(ERR, "hpss_alloc_dev_by_conn_id alloc device fail! \r\n");
        return NULL;
    }

    memset(p_device, 0, sizeof(glps_dev_t));

    INIT_DLIST_HEAD(&p_device->list);
    INIT_DLIST_HEAD(&p_device->wait_queue);
    p_device->conn_id = conn_id;

    list_add_tail(&p_device->list, &p_glps_env->dev_list);
    return p_device;
}

/*!
    \brief      Find glucose service device structor
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     glps_dev_t *: pointer to the found glucose service device structor
*/
static glps_dev_t *glps_find_dev_by_conn_id(uint8_t conn_id)
{
    dlist_t *pos, *n;
    glps_dev_t *p_device;

    if (list_empty(&p_glps_env->dev_list)) {
        return NULL;
    }

    list_for_each_safe(pos, n, &p_glps_env->dev_list) {
        p_device = list_entry(pos, glps_dev_t, list);
        if (p_device->conn_id == conn_id) {
            return p_device;
        }
    }

    return NULL;
}

/*!
    \brief      Find glucose service device structor, if no such device, allocate one
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     glps_dev_t *: pointer to the found or allocated glucose service device structor
*/
static glps_dev_t *glps_find_alloc_dev_by_conn_id(uint8_t conn_id)
{
    glps_dev_t *p_device = glps_find_dev_by_conn_id(conn_id);

    if (p_device == NULL) {
        p_device = glps_alloc_dev_by_conn_id(conn_id);
    }

    return p_device;
}

/*!
    \brief      Remove glucose service device structor from list
    \param[in]  conn_id: connection index
    \param[out] none
    \retval     none
*/
static void glps_remove_dev_by_conn_id(uint8_t conn_id)
{
    dlist_t *pos, *n;
    glps_dev_t *p_device = NULL;
    bool found = false;

    if (list_empty(&p_glps_env->dev_list)) {
        return;
    }

    list_for_each_safe(pos, n, &p_glps_env->dev_list) {
        p_device = list_entry(pos, glps_dev_t, list);
        if (p_device->conn_id == conn_id) {
            found = true;
            break;
        }
    }

    if (found) {
        while (!list_empty(&p_device->wait_queue)) {
            glps_data_meta_t *p_meta = (glps_data_meta_t *) list_first(&(p_device->wait_queue));
            list_del(&p_meta->list);
            sys_mfree(p_meta);
        }

        list_del(&p_device->list);
        sys_mfree(p_device);
    }
}

/*!
    \brief      Execute glucose service operation
    \param[in]  p_dev: pointer to the glucose service device structor
    \param[out] none
    \retval     none
*/
static void ble_glps_exe_operation(glps_dev_t *p_dev)
{
    if (!p_dev->in_exe_op) {
        p_dev->in_exe_op = true;

        while (!list_empty(&(p_dev->wait_queue)) && !(p_dev->op_ongoing)) {
            uint16_t status = BLE_ERR_NO_ERROR;
            glps_data_meta_t *p_meta = (glps_data_meta_t *) list_first(&(p_dev->wait_queue));
            uint8_t operation = p_meta->operation;

            // send GATT event
            status = ble_gatts_ntf_ind_send(p_dev->conn_id, p_glps_env->glps_id, p_meta->att_idx, p_meta->buf,
                                            p_meta->val_len, p_meta->evt_type);

            if (status == BLE_ERR_NO_ERROR) {
                p_dev->op_ongoing = true;
            }

            if (!p_dev->op_ongoing) {
                // Remove meta from list and free meta
                list_del(&p_meta->list);
                sys_mfree(p_meta);
                if (operation == BLE_GLPS_OP_RACP_RSP_SEND) {
                    // Inform application that control point response has been sent
                    if (p_dev->racp_op_code != BLE_GLP_REQ_RSP_CODE) {
                        p_glps_env->callbacks.racp_rsp_send_cmp_cb(p_dev->conn_id, status);
                    }

                    // consider control point operation done
                    p_dev->racp_op_code = BLE_GLP_REQ_RESERVED;
                } else if (operation != BLE_GLPS_OP_FEAT_SEND) {
                    SETB(p_dev->flags, BLE_GPLS_SENDING_MEAS, false);

                    // drop context data not yet send
                    if (operation == BLE_GLPS_OP_MEAS_SEND_WITH_CTX) {
                        p_meta = (glps_data_meta_t *) list_first(&(p_dev->wait_queue));
                        if (p_meta != NULL) {
                            list_del(&p_meta->list);
                            sys_mfree(p_meta);
                        }
                    }

                    // Inform application that event has been sent
                    p_glps_env->callbacks.meas_send_cmp_cb(p_dev->conn_id, status);
                }
            }
        }

        p_dev->in_exe_op = false;
    }
}

/*!
    \brief      Pack measurement data
    \param[in]  p_buf: pointer to output buffer
    \param[in]  seq_num: glucose measurement sequence number
    \param[in]  p_meas: pointer to measurement information
    \param[out] none
    \retval     uint16_t: total length of the packed data
*/
static uint16_t ble_glps_pack_meas(uint8_t *p_buf, uint16_t seq_num, const ble_glp_meas_t *p_meas)
{
    uint8_t meas_flags = p_meas->flags;
    uint8_t *pp = p_buf;

    // Flags
    LE_UINT8_TO_STREAM(pp, meas_flags);

    // Sequence Number
    LE_UINT16_TO_STREAM(pp, seq_num);

    // Base Time
    ble_prf_pack_date_time(pp, &(p_meas->base_time));
    pp += BLE_GLS_FILTER_USER_FACING_TIME_SIZE;

    // Time Offset
    if (GETB(meas_flags, BLE_GLP_MEAS_TIME_OFF_PRES)) {
        LE_UINT16_TO_STREAM(pp, p_meas->time_offset);
    }

    // Glucose Concentration, type and location
    if (GETB(meas_flags, BLE_GLP_MEAS_GL_CTR_TYPE_AND_SPL_LOC_PRES)) {
        LE_UINT16_TO_STREAM(pp, p_meas->concentration);
        // type and location are 2 nibble values
        LE_UINT8_TO_STREAM(pp, ((p_meas->location << 4) | (p_meas->type)));
    }

    // Sensor Status Annunciation
    if (GETB(meas_flags, BLE_GLP_MEAS_SENS_STAT_ANNUN_PRES)) {
        // Use a non-const value
        uint16_t sensor_status = p_meas->sensor_status;

        // If feature not supported, corresponding Flag in the Sensor Status Annunciation Field
        // shall be set to default of 0

        // Low Battery Detection During Measurement Support Bit
        if (!GETB(p_glps_env->features, BLE_GLP_FET_LOW_BAT_DET_DUR_MEAS_SUPP)) {
            SETB(sensor_status, BLE_GLP_MEAS_STATE_DEV_BAT_LOW, 0);
        }

        // Sensor Malfunction Detection Support Bit
        if (!GETB(p_glps_env->features, BLE_GLP_FET_SENS_MFNC_DET_SUPP)) {
            SETB(sensor_status, BLE_GLP_MEAS_STATE_SENS_MFNC_OR_FLTING, 0);
        }

        // Sensor Sample Size Support Bit
        if (!GETB(p_glps_env->features, BLE_GLP_FET_SENS_SPL_SIZE_SUPP)) {
            SETB(sensor_status, BLE_GLP_MEAS_STATE_SPL_SIZE_INSUFF, 0);
        }

        // Sensor Strip Insertion Error Detection Support Bit
        if (!GETB(p_glps_env->features, BLE_GLP_FET_SENS_STRIP_INSERT_ERR_DET_SUPP)) {
            SETB(sensor_status, BLE_GLP_MEAS_STATE_STRIP_INSERT_ERR, 0);
        }

        // Sensor Result High-Low Detection Support Bit
        if (!GETB(p_glps_env->features, BLE_GLP_FET_SENS_RES_HIGH_LOW_DET_SUPP)) {
            SETB(sensor_status, BLE_GLP_MEAS_STATE_SENS_RES_HIGHER, 0);
            SETB(sensor_status, BLE_GLP_MEAS_STATE_SENS_RES_LOWER, 0);
        }

        // Sensor Temperature High-Low Detection Support Bit
        if (!GETB(p_glps_env->features, BLE_GLP_FET_SENS_TEMP_HIGH_LOW_DET_SUPP)) {
            SETB(sensor_status, BLE_GLP_MEAS_STATE_SENS_TEMP_TOO_HIGH, 0);
            SETB(sensor_status, BLE_GLP_MEAS_STATE_SENS_TEMP_TOO_LOW, 0);
        }

        // Sensor Read Interrupt Detection Support Bit
        if (!GETB(p_glps_env->features, BLE_GLP_FET_SENS_RD_INT_DET_SUPP)) {
            SETB(sensor_status, BLE_GLP_MEAS_STATE_SENS_RD_INTED, 0);
        }

        // General Device Fault Support Bit
        if (!GETB(p_glps_env->features, BLE_GLP_FET_GEN_DEV_FLT_SUPP)) {
            SETB(sensor_status, BLE_GLP_MEAS_STATE_GEN_DEV_FLT, 0);
        }

        // Time Fault Support Bit
        if (!GETB(p_glps_env->features, BLE_GLP_FET_TIME_FLT_SUPP)) {
            SETB(sensor_status, BLE_GLP_MEAS_STATE_TIME_FLT, 0);
        }

        // Multiple Bond Support Bit
        if (!GETB(p_glps_env->features, BLE_GLP_FET_MUL_BOND_SUPP)) {
            // can determine that the Glucose Sensor supports only a single bond
        } else {
            // Collector can determine that the Glucose supports multiple bonds
        }

        LE_UINT16_TO_STREAM(pp, sensor_status);
    }

    return (pp - p_buf);
}

/*!
    \brief      Pack control point response
    \param[in]  p_buf: pointer to output buffer
    \param[in]  op_code: requested operation code, @ref ble_glp_racp_op_code_t
    \param[in]  racp_status: record access control point execution status, @ref ble_glp_racp_status
    \param[in]  num_of_record: number of record, meaningful for @ref BLE_GLP_REQ_REP_NUM_OF_STRD_RECS operation
    \param[out] none
    \retval     uint16_t: total length of the packed data
*/
static uint16_t ble_glps_pack_racp_rsp(uint8_t *p_buf, uint8_t op_code, uint8_t racp_status,
                                       uint16_t num_of_record)
{
    bool num_recs_rsp = ((op_code == BLE_GLP_REQ_REP_NUM_OF_STRD_RECS) &&
                         (racp_status == BLE_GLP_RSP_SUCCESS));
    uint8_t *pp = p_buf;

    // Set the Response Code
    if (num_recs_rsp) {
        LE_UINT8_TO_STREAM(pp, BLE_GLP_REQ_NUM_OF_STRD_RECS_RSP);
        // set operator (null)
        LE_UINT8_TO_STREAM(pp, 0);
        LE_UINT16_TO_STREAM(pp, num_of_record);
    } else {
        LE_UINT8_TO_STREAM(pp, BLE_GLP_REQ_RSP_CODE);
        // set operator (null)
        LE_UINT8_TO_STREAM(pp, 0);
        // requested opcode
        LE_UINT8_TO_STREAM(pp, op_code);
        // command status
        LE_UINT8_TO_STREAM(pp, racp_status);
    }

    return (pp - p_buf);
}

/*!
    \brief      Pack context data
    \param[in]  p_buf: pointer to output buffer
    \param[in]  seq_num: glucose measurement sequence number
    \param[in]  p_ctx: pointer to measurement context information
    \param[out] none
    \retval     uint16_t: total length of the packed data
*/
static uint16_t ble_glps_pack_meas_ctx(uint8_t *p_buf, uint16_t seq_num,
                                       const ble_glp_meas_ctx_t *p_ctx)
{
    uint8_t meas_flags = p_ctx->flags;
    uint8_t *pp = p_buf;

    // Flags
    LE_UINT8_TO_STREAM(pp, meas_flags);

    // Sequence Number
    LE_UINT16_TO_STREAM(pp, seq_num);

    // Extended Flags
    if (GETB(meas_flags, BLE_GLP_CTX_EXTD_F_PRES) != 0) {
        LE_UINT8_TO_STREAM(pp, p_ctx->ext_flags);
    }

    // Carbohydrate ID And Carbohydrate Present
    if (GETB(meas_flags, BLE_GLP_CTX_CRBH_ID_AND_CRBH_PRES) != 0) {
        // Carbohydrate ID
        LE_UINT8_TO_STREAM(pp, p_ctx->carbo_id);
        // Carbohydrate Present
        LE_UINT16_TO_STREAM(pp, p_ctx->carbo_val);
    }

    // Meal Present
    if (GETB(meas_flags, BLE_GLP_CTX_MEAL_PRES) != 0) {
        LE_UINT8_TO_STREAM(pp, p_ctx->meal);
    }

    // Tester-Health Present
    if (GETB(meas_flags, BLE_GLP_CTX_TESTER_HEALTH_PRES) != 0) {
        // Tester and Health are 2 nibble values
        LE_UINT8_TO_STREAM(pp, ((p_ctx->health << 4) | (p_ctx->tester)));
    }

    // Exercise Duration & Exercise Intensity Present
    if (GETB(meas_flags, BLE_GLP_CTX_EXE_DUR_AND_EXE_INTENS_PRES) != 0) {
        // Exercise Duration
        LE_UINT16_TO_STREAM(pp, p_ctx->exercise_dur);
        // Exercise Intensity
        LE_UINT8_TO_STREAM(pp, p_ctx->exercise_intens);
    }

    // Medication ID And Medication Present
    if (GETB(meas_flags, BLE_GLP_CTX_MEDIC_ID_AND_MEDIC_PRES) != 0) {
        // Medication ID
        LE_UINT8_TO_STREAM(pp, p_ctx->med_id);
        // Medication Present
        LE_UINT16_TO_STREAM(pp, p_ctx->med_val);
    }

    // HbA1c Present
    if (GETB(meas_flags, BLE_GLP_CTX_HBA1C_PRES) != 0) {
        // HbA1c
        LE_UINT16_TO_STREAM(pp, p_ctx->hba1c_val);
    }

    return (pp - p_buf);
}

/*!
    \brief      Unpack control point data and process it
    \param[in]  p_dev: pointer to the glucose service device structor
    \param[in]  p_buf: pointer to input data
    \param[in]  buf_len: buffer length
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_glps_unpack_racp_req(glps_dev_t *p_dev, uint8_t *p_buf, uint16_t buf_len)
{
    ble_status_t status = BLE_ERR_NO_ERROR;
    uint8_t racp_rsp_status = BLE_GLP_RSP_INVALID_OPERATOR;
    union ble_glp_filter filter;
    uint8_t op_code = 0;
    uint8_t func_operator = 0;
    uint8_t filter_type = 0;
    sys_memset(&filter, 0, sizeof(union ble_glp_filter));

    do {
        // verify that enough data present to load operation filter
        if (buf_len < 2) {
            status = BLE_ATT_ERR_UNLIKELY_ERR;
            break;
        }

        LE_STREAM_TO_UINT8(op_code, p_buf);
        LE_STREAM_TO_UINT8(func_operator, p_buf);
        buf_len -= 2;

        // Abort operation don't require any other parameter
        if (op_code == BLE_GLP_REQ_ABORT_OP) {
            if (p_dev->racp_op_code == BLE_GLP_REQ_RESERVED) {
                // do nothing since a procedure already in progress
                racp_rsp_status = BLE_GLP_RSP_ABORT_UNSUCCESSFUL;
            } else {
                // Handle abort, no need to extract other info
                racp_rsp_status = BLE_GLP_RSP_SUCCESS;
            }
            break;
        } else if (p_dev->racp_op_code != BLE_GLP_REQ_RESERVED) {
            // do nothing since a procedure already in progress
            status = BLE_GLP_ERR_PROC_ALREADY_IN_PROGRESS;
            break;
        }

        // check if opcode is supported
        if ((op_code < BLE_GLP_REQ_REP_STRD_RECS) || (op_code > BLE_GLP_REQ_REP_NUM_OF_STRD_RECS)) {
            racp_rsp_status = BLE_GLP_RSP_OP_CODE_NOT_SUP;
            break;
        }

        // check if operator is valid
        if (func_operator < BLE_GLP_OP_ALL_RECS) {
            racp_rsp_status = BLE_GLP_RSP_INVALID_OPERATOR;
            break;
        }
        // check if operator is supported
        else if (func_operator > BLE_GLP_OP_LAST_REC) {
            racp_rsp_status = BLE_GLP_RSP_OPERATOR_NOT_SUP;
            break;
        }

        // check if request requires operand (filter)
        if ((func_operator < BLE_GLP_OP_LT_OR_EQ) || (func_operator > BLE_GLP_OP_WITHIN_RANGE_OF)) {
            racp_rsp_status = BLE_GLP_RSP_SUCCESS;
            break;
        }

        if (buf_len == 0) {
            break;
        }

        LE_STREAM_TO_UINT8(filter_type, p_buf);
        buf_len--;

        // filter uses sequence number
        if (filter_type == BLE_GLP_FILTER_SEQ_NUMBER) {
            // retrieve minimum value
            if ((func_operator == BLE_GLP_OP_GT_OR_EQ) || (func_operator == BLE_GLP_OP_WITHIN_RANGE_OF)) {
                // check sufficient data available
                if (buf_len < 2) {
                    break;
                }

                // retrieve minimum value
                LE_STREAM_TO_UINT16(filter.seq_num.min, p_buf);
                buf_len -= 2;
            }

            // retrieve maximum value
            if ((func_operator == BLE_GLP_OP_LT_OR_EQ) || (func_operator == BLE_GLP_OP_WITHIN_RANGE_OF)) {
                // check sufficient data available
                if (buf_len < 2) {
                    break;
                }

                // retrieve maximum value
                LE_STREAM_TO_UINT16(filter.seq_num.max, p_buf);
                buf_len -= 2;
            }

            // check that range value is valid
            if ((func_operator == BLE_GLP_OP_WITHIN_RANGE_OF) && (filter.seq_num.min > filter.seq_num.max))  {
                break;
            }
        }
        // filter uses user facing time
        else if (filter_type == BLE_GLP_FILTER_USER_FACING_TIME) {
            // retrieve minimum value
            if ((func_operator == BLE_GLP_OP_GT_OR_EQ) || (func_operator == BLE_GLP_OP_WITHIN_RANGE_OF)) {
                // check sufficient data available
                if (buf_len < BLE_GLS_FILTER_USER_FACING_TIME_SIZE) {
                    break;
                }

                // retrieve minimum facing time
                ble_prf_unpack_date_time(p_buf, &(filter.time.facetime_min));
                p_buf += BLE_GLS_FILTER_USER_FACING_TIME_SIZE;
                buf_len -= BLE_GLS_FILTER_USER_FACING_TIME_SIZE;
            }

            // retrieve maximum value
            if ((func_operator == BLE_GLP_OP_LT_OR_EQ) || (func_operator == BLE_GLP_OP_WITHIN_RANGE_OF)) {
                if (buf_len < BLE_GLS_FILTER_USER_FACING_TIME_SIZE) {
                    break;
                }

                // retrieve maximum facing time
                ble_prf_unpack_date_time(p_buf, &(filter.time.facetime_max));
                p_buf += BLE_GLS_FILTER_USER_FACING_TIME_SIZE;
                buf_len -= BLE_GLS_FILTER_USER_FACING_TIME_SIZE;
            }
        } else {
            racp_rsp_status = BLE_GLP_RSP_OPERAND_NOT_SUP;
            break;
        }

        // consider that data extraction is a sucess
        racp_rsp_status = BLE_GLP_RSP_SUCCESS;
    } while (0);

    if (status == BLE_ERR_NO_ERROR) {
        // If no error raised, inform the application about the request
        if (racp_rsp_status == BLE_GLP_RSP_SUCCESS) {
            p_dev->racp_op_code  = op_code;

            // inform application about control point request
            p_glps_env->callbacks.racp_req_cb(p_dev->conn_id, op_code, func_operator, filter_type, &filter);
        } else {
            glps_data_meta_t *p_out_buf = sys_malloc(sizeof(glps_data_meta_t) + 4);

            if (p_out_buf) {
                INIT_DLIST_HEAD(&p_out_buf->list);

                p_dev->racp_op_code  = BLE_GLP_REQ_RSP_CODE;
                p_out_buf->buf[0] = BLE_GLP_REQ_RSP_CODE;
                p_out_buf->buf[1] = 0;
                p_out_buf->buf[2] = op_code;
                p_out_buf->buf[3] = racp_rsp_status;

                p_out_buf->operation = BLE_GLPS_OP_RACP_RSP_SEND;
                p_out_buf->evt_type  = BLE_GATT_INDICATE;
                p_out_buf->att_idx   = BLE_GLS_HDL_REC_ACCESS_CTRL_VAL;

                // put event on wait queue
                list_add_tail(&(p_dev->wait_queue), &(p_out_buf->list));
                // execute operation
                ble_glps_exe_operation(p_dev);
            } else {
                status = BLE_ATT_ERR_INSUFF_RESOURCE;
            }
        }
    }

    return (status);
}

/*!
    \brief      This function is called when GATT server has sent notification/indication or if an error occurs
    \param[in]  p_dev: pointer to the glucose service device structor
    \param[in]  opeation: operation code
    \param[in]  status: status of the procedure, @ref ble_status_t
    \param[out] none
    \retval     none
*/
static void ble_glps_cb_event_sent(glps_dev_t *p_dev, uint8_t opeation, uint16_t status)
{
    p_dev->op_ongoing = false;

    switch (opeation) {
    case BLE_GLPS_OP_MEAS_SEND_WITH_CTX: {
        if (status != BLE_ERR_NO_ERROR) {
            glps_data_meta_t *p_meta = (glps_data_meta_t *) list_first(&(p_dev->wait_queue));
            if (p_meta != NULL) {
                list_del(&p_meta->list);
                sys_mfree(p_meta);
            }
        } else {
            break;
        }
    }
    // no break
    case BLE_GLPS_OP_MEAS_SEND:
    case BLE_GLPS_OP_MEAS_CTX_SEND: {
        SETB(p_dev->flags, BLE_GPLS_SENDING_MEAS, false);
        p_glps_env->callbacks.meas_send_cmp_cb(p_dev->conn_id, status);
    }
    break;
    case BLE_GLPS_OP_RACP_RSP_SEND: {
        // Inform application that control point response has been sent
        if (p_dev->racp_op_code != BLE_GLP_REQ_RSP_CODE) {
            p_glps_env->callbacks.racp_rsp_send_cmp_cb(p_dev->conn_id, status);
        }

        p_dev->racp_op_code = BLE_GLP_REQ_RESERVED;
    }
    break;
    default: { /* Nothing to do */ } break;
    }

    // continue operation execution
    ble_glps_exe_operation(p_dev);
}

/*!
    \brief      Callback function to handle GATT server messages
    \param[in]  p_cb_data: pointer to GATT server message information
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
static ble_status_t ble_glps_rw_cb(ble_gatts_msg_info_t *p_cb_data)
{
    uint16_t attr_idx;
    uint8_t status = BLE_ERR_NO_ERROR;
    glps_dev_t *p_dev = NULL;

    if (p_cb_data->srv_msg_type == BLE_SRV_EVT_GATT_OPERATION) {
        p_dev = glps_find_alloc_dev_by_conn_id(p_cb_data->msg_data.gatts_op_info.conn_idx);

        if (p_dev == NULL) {
            dbg_print(ERR, "ble_glps_rw_cb can't find or alloc device \r\n");
            return BLE_ATT_ERR_VALUE_NOT_ALLOWED;
        }

        if (p_cb_data->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_READ_REQ) {
            uint16_t value = 0;
            ble_gatts_read_req_t *p_read_req = &(p_cb_data->msg_data.gatts_op_info.gatts_op_data.read_req);

            attr_idx = p_read_req->att_idx + BLE_GLS_HDL_SVC;

            switch (attr_idx) {
            case BLE_GLS_HDL_MEAS_CLI_CHR_CFG_DESC: {
                value = GETB(p_dev->evt_cfg, BLE_GLPS_MEAS_NTF_CFG)
                        ? BLE_PRF_CLI_START_NTF : BLE_PRF_CLI_STOP_NTFIND;
            }
            break;

            case BLE_GLS_HDL_MEAS_CTX_CLI_CHR_CFG_DESC: {
                value = GETB(p_dev->evt_cfg, BLE_GLPS_MEAS_CTX_NTF_CFG)
                        ? BLE_PRF_CLI_START_NTF : BLE_PRF_CLI_STOP_NTFIND;
            }
            break;

            case BLE_GLS_HDL_FEATURE_VAL: {
                value = p_glps_env->features;
            }
            break;

            case BLE_GLS_HDL_FEATURE_CLI_CHR_CFG_DESC: {
                value = GETB(p_dev->evt_cfg, BLE_GLPS_FEAT_IND_CFG)
                        ? BLE_PRF_CLI_START_IND : BLE_PRF_CLI_STOP_NTFIND;
            }
            break;

            case BLE_GLS_HDL_REC_ACCESS_CTRL_CL_CHR_CFG_DESC: {
                value = GETB(p_dev->evt_cfg, BLE_GLPS_RACP_IND_CFG)
                        ? BLE_PRF_CLI_START_IND : BLE_PRF_CLI_STOP_NTFIND;
            }
            break;

            default:
                return BLE_ATT_ERR_INVALID_HANDLE;
            }

            p_read_req->val_len = 2;
            p_read_req->att_len = 2;
            sys_memcpy(p_read_req->p_val, &value, p_read_req->val_len);
        } else if (p_cb_data->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_WRITE_REQ) {
            uint8_t cfg_upd_flag  = 0;
            uint16_t cfg_en_val = 0;
            ble_gatts_write_req_t *p_write_req = &(p_cb_data->msg_data.gatts_op_info.gatts_op_data.write_req);

            attr_idx = p_write_req->att_idx + BLE_GLS_HDL_SVC;

            switch (attr_idx) {
            case BLE_GLS_HDL_MEAS_CLI_CHR_CFG_DESC: {
                cfg_upd_flag = BLE_GLPS_MEAS_NTF_CFG_BIT;
            }
            break;

            case BLE_GLS_HDL_MEAS_CTX_CLI_CHR_CFG_DESC: {
                cfg_upd_flag = BLE_GLPS_MEAS_CTX_NTF_CFG_BIT;
            }
            break;

            case BLE_GLS_HDL_FEATURE_CLI_CHR_CFG_DESC: {
                cfg_upd_flag = BLE_GLPS_FEAT_IND_CFG_BIT;
            }
            break;

            case BLE_GLS_HDL_REC_ACCESS_CTRL_VAL: {
                // Check if sending of indications has been enabled
                if (!GETB(p_dev->evt_cfg, BLE_GLPS_RACP_IND_CFG)) {
                    // CPP improperly configured
                    status = BLE_GLP_ERR_IMPROPER_CLI_CHAR_CFG;
                } else {
                    // Unpack Control Point parameters
                    status = ble_glps_unpack_racp_req(p_dev, p_write_req->p_val, p_write_req->val_len);
                }
            }
            break;

            case BLE_GLS_HDL_REC_ACCESS_CTRL_CL_CHR_CFG_DESC: {
                cfg_upd_flag = BLE_GLPS_RACP_IND_CFG_BIT;
            }
            break;

            default:
                return BLE_ATT_ERR_INVALID_HANDLE;

            }

            if (cfg_upd_flag != 0) {
                if (p_write_req->val_len != sizeof(uint16_t)) {
                    status = BLE_PRF_CCCD_IMPR_CONFIGURED;
                } else {
                    cfg_en_val = *((uint16_t *)p_write_req->p_val);

                    if (cfg_en_val == BLE_PRF_CLI_STOP_NTFIND) {
                        p_dev->evt_cfg &= ~cfg_upd_flag;
                    } else {
                        p_dev->evt_cfg |= cfg_upd_flag;
                    }
                }
            }
        } else if (p_cb_data->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_NTF_IND_SEND_RSP) {
            ble_gatts_ntf_ind_send_rsp_t *p_ntf_ind = &
                                                      (p_cb_data->msg_data.gatts_op_info.gatts_op_data.ntf_ind_send_rsp);
            glps_data_meta_t *p_meta = (glps_data_meta_t *) list_first(&(p_dev->wait_queue));

            attr_idx = p_ntf_ind->att_idx + BLE_GLS_HDL_SVC;

            if (p_meta->att_idx == attr_idx) {
                list_del(&p_meta->list);
                ble_glps_cb_event_sent(p_dev, p_meta->operation, status);
                sys_mfree(p_meta);
            } else {
                status = BLE_ATT_ERR_INVALID_HANDLE;
            }
        }
    } else if (p_cb_data->srv_msg_type == BLE_SRV_EVT_CONN_STATE_CHANGE_IND) {
        if (p_cb_data->msg_data.conn_state_change_ind.conn_state == BLE_CONN_STATE_DISCONNECTD) {
            glps_remove_dev_by_conn_id(p_cb_data->msg_data.conn_state_change_ind.info.disconn_info.conn_idx);
        }
    }

    return status;
}

/*!
    \brief      Send glucose service feature indication
    \param[in]  p_dev: pointer to the glucose service device structor
    \param[out] none
    \retval     none
*/
static void ble_glps_feat_ind_send(glps_dev_t *p_dev)
{
    glps_data_meta_t *p_buf_feat = sys_malloc(sizeof(glps_data_meta_t) + 2);

    if (p_buf_feat) {
        p_buf_feat->operation = BLE_GLPS_OP_FEAT_SEND;
        p_buf_feat->evt_type  = BLE_GATT_INDICATE;
        p_buf_feat->att_idx   = BLE_GLS_HDL_FEATURE_VAL;
        p_buf_feat->val_len = 2;
        *((uint16_t *)p_buf_feat->buf) = p_glps_env->features;

        // put event(s) on wait queue
        INIT_DLIST_HEAD(&p_buf_feat->list);
        list_add_tail(&(p_dev->wait_queue), &(p_buf_feat->list));
        // execute operation
        ble_glps_exe_operation(p_dev);
    }
}

/*!
    \brief      Send glucose measurement information
    \param[in]  conn_id: connection index
    \param[in]  seq_num: glucose measurement sequence number
    \param[in]  p_meas: pointer to glucose measurement
    \param[in]  p_ctx: pointer to glucose measurement context
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_glps_meas_send(uint8_t conn_id, uint16_t seq_num, const ble_glp_meas_t *p_meas,
                                const ble_glp_meas_ctx_t *p_ctx)
{
    ble_status_t status = BLE_PRF_ERR_REQ_DISALLOWED;
    glps_dev_t *p_dev = glps_find_dev_by_conn_id(conn_id);

    if (p_dev == NULL) {
        return status;
    }

    if (p_meas == NULL) {
        status = BLE_GAP_ERR_INVALID_PARAM;
    } else {
        // Cannot send another measurement in parallel
        if (!GETB(p_dev->flags, BLE_GPLS_SENDING_MEAS)) {
            glps_data_meta_t *p_buf_meas = sys_malloc(sizeof(glps_data_meta_t) + BLE_GLP_MEAS_MAX_LEN);
            glps_data_meta_t *p_buf_meas_ctx = NULL;

            // check if context is supported
            if ((p_ctx != NULL) && !(p_glps_env->meas_ctx_supported)) {
                // Context not supported
                status = BLE_PRF_ERR_FEATURE_NOT_SUPPORTED;
            }
            // check if notifications enabled
            else if (!GETB(p_dev->evt_cfg, BLE_GLPS_MEAS_NTF_CFG)
                     || (!GETB(p_dev->evt_cfg, BLE_GLPS_MEAS_CTX_NTF_CFG) && (p_ctx != NULL))) {
                // Not allowed to send measurement if Notifications not enabled.
                status = (BLE_PRF_ERR_NTF_DISABLED);
            } else if (p_buf_meas) {
                p_buf_meas->operation = (p_ctx != NULL) ? BLE_GLPS_OP_MEAS_SEND_WITH_CTX : BLE_GLPS_OP_MEAS_SEND;
                p_buf_meas->evt_type  = BLE_GATT_NOTIFY;
                p_buf_meas->att_idx   = BLE_GLS_HDL_MEAS_VAL;

                // pack measurement
                p_buf_meas->val_len = ble_glps_pack_meas(p_buf_meas->buf, seq_num, p_meas);
                status = BLE_ERR_NO_ERROR;

                if (p_ctx != NULL) {
                    p_buf_meas_ctx = sys_malloc(sizeof(glps_data_meta_t) + BLE_GLP_MEAS_CTX_MAX_LEN);
                    if (p_buf_meas_ctx) {
                        p_buf_meas_ctx->operation = BLE_GLPS_OP_MEAS_CTX_SEND;
                        p_buf_meas_ctx->evt_type  = BLE_GATT_NOTIFY;
                        p_buf_meas_ctx->att_idx   = BLE_GLS_HDL_MEAS_CTX_VAL;

                        // pack measurement
                        p_buf_meas_ctx->val_len = ble_glps_pack_meas_ctx(p_buf_meas_ctx->buf, seq_num, p_ctx);
                    } else {
                        status = BLE_GAP_ERR_INSUFF_RESOURCES;
                        sys_mfree(p_buf_meas);
                    }
                }
            } else {
                status = BLE_GAP_ERR_INSUFF_RESOURCES;
            }

            if (status == BLE_ERR_NO_ERROR) {
                SETB(p_dev->flags, BLE_GPLS_SENDING_MEAS, true);

                // put event(s) on wait queue
                INIT_DLIST_HEAD(&p_buf_meas->list);
                list_add_tail(&(p_dev->wait_queue), &(p_buf_meas->list));
                if (p_ctx != NULL && p_buf_meas_ctx != NULL) {
                    INIT_DLIST_HEAD(&p_buf_meas_ctx->list);
                    list_add_tail(&(p_dev->wait_queue), &(p_buf_meas_ctx->list));
                }
                // execute operation
                ble_glps_exe_operation(p_dev);
            }
        }
    }

    return (status);
}

/*!
    \brief      Send glucose access control operation response
    \param[in]  conn_id: connection index
    \param[in]  op_code: glucose access control operation code, @ref ble_glp_racp_op_code_t
    \param[in]  racp_status: glucose access control operation status
    \param[in]  num_of_record: number of record
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_glps_racp_rsp_send(uint8_t conn_id, uint8_t op_code, uint8_t racp_status,
                                    uint16_t num_of_record)
{
    ble_status_t status = BLE_PRF_ERR_REQ_DISALLOWED;
    glps_dev_t *p_dev = glps_find_dev_by_conn_id(conn_id);

    if (p_dev == NULL) {
        return status;
    }

    do {
        glps_data_meta_t *p_buf_meta = NULL;

        // check if op code valid
        if ((op_code < BLE_GLP_REQ_REP_STRD_RECS) || (op_code > BLE_GLP_REQ_REP_NUM_OF_STRD_RECS)) {
            // Wrong op code
            status = BLE_PRF_ERR_INVALID_PARAM;
            break;
        }
        // check if RACP on going
        else if ((op_code != BLE_GLP_REQ_ABORT_OP) && (p_dev->racp_op_code != op_code)) {
            // Cannot send response since no RACP on going
            break;
        }

        // Check the current operation
        if (p_dev->racp_op_code == BLE_GLP_REQ_RESERVED) {
            // The confirmation has been sent without request indication, ignore
            break;
        }

        // Check if sending of indications has been enabled
        if (!GETB(p_dev->evt_cfg, BLE_GLPS_RACP_IND_CFG)) {
            // mark operation done
            p_dev->racp_op_code = BLE_GLP_REQ_RESERVED;
            // CPP improperly configured
            status = BLE_PRF_ERR_IND_DISABLED;
            break;
        }

        p_buf_meta = sys_malloc(sizeof(glps_data_meta_t) + BLE_GLP_REC_ACCESS_CTRL_MAX_LEN);
        if (p_buf_meta) {
            p_buf_meta->operation = BLE_GLPS_OP_RACP_RSP_SEND;
            p_buf_meta->att_idx   = BLE_GLS_HDL_REC_ACCESS_CTRL_VAL;
            p_buf_meta->evt_type  = BLE_GATT_INDICATE;

            // Pack structure
            p_buf_meta->val_len = ble_glps_pack_racp_rsp(p_buf_meta->buf, op_code, racp_status, num_of_record);
            // put event on wait queue
            list_add_tail(&(p_dev->wait_queue), &(p_buf_meta->list));
            // execute operation
            ble_glps_exe_operation(p_dev);
            status = BLE_ERR_NO_ERROR;
        } else {
            status = BLE_GAP_ERR_INSUFF_RESOURCES;
        }

    } while (0);

    return (status);
}

/*!
    \brief      Set glucose service feature
    \param[in]  features: glucose feature, @ref ble_glp_srv_feature_flag
    \param[out] none
    \retval     none
*/
void ble_glps_set_features(uint16_t features)
{
    dlist_t *pos, *n;
    glps_dev_t *p_device;

    if (p_glps_env->features != features) {
        p_glps_env->features = features;

        if (list_empty(&p_glps_env->dev_list)) {
            return;
        }

        list_for_each_safe(pos, n, &p_glps_env->dev_list) {
            p_device = list_entry(pos, glps_dev_t, list);
            if (GETB(p_device->evt_cfg, BLE_GLPS_FEAT_IND_CFG)) {
                ble_glps_feat_ind_send(p_device);
            }
        }
    }
}

/*!
    \brief      Init Glucose service profile
    \param[in]  callbacks: glucose sensor server callback
    \param[in]  params: parameters of the Glucose service database
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_glps_init(ble_glps_callback_t callbacks, struct ble_glps_db_cfg params)
{
    ble_status_t ret = BLE_ERR_NO_ERROR;

    p_glps_env = sys_malloc(sizeof(ble_glps_env_t));
    if (p_glps_env == NULL) {
        return BLE_ERR_NO_MEM_AVAIL;
    }

    sys_memset(p_glps_env, 0, sizeof(ble_glps_env_t));
    p_glps_env->callbacks = callbacks;

    ret = ble_gatts_svc_add(&p_glps_env->glps_id, ble_gls_uuid, 0, SVC_UUID(16), ble_gls_attr_db,
                            BLE_GLS_HDL_NB, ble_glps_rw_cb);

    if (ret != BLE_ERR_NO_ERROR) {
        sys_mfree(p_glps_env);
        return ret;
    }

    p_glps_env->features = params.features;
    p_glps_env->meas_ctx_supported = params.meas_ctx_supported;

    INIT_DLIST_HEAD(&p_glps_env->dev_list);

    return ret;
}

