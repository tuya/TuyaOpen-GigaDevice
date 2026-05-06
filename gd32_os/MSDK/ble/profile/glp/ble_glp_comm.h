/*!
    \file    ble_glp_comm.h
    \brief   Header file of glucose profile common types.

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

#ifndef _BLE_GLP_COMM_H_
#define _BLE_GLP_COMM_H_

#include <stdint.h>
#include "co_math.h"
#include "ble_profile_utils.h"

/* Glucose measurement packet max length */
#define BLE_GLP_MEAS_MAX_LEN                (17)
/* Glucose measurement context packet max length */
#define BLE_GLP_MEAS_CTX_MAX_LEN            (17)
/* Record Access Control Point packet max length */
#define BLE_GLP_REC_ACCESS_CTRL_MAX_LEN     (21)

/* Glucose Service Error Code */
enum ble_glp_error_code
{
    BLE_GLP_ERR_PROC_ALREADY_IN_PROGRESS  = (0x80),     /*!< Procedure already in progress */
    BLE_GLP_ERR_IMPROPER_CLI_CHAR_CFG     = (0x81),     /*!< CCCD Improperly Configured */
};

/* Glucose Measurement Flags field bit values */
enum ble_glp_meas_flag_bf
{
    BLE_GLP_MEAS_TIME_OFF_PRES_POS                  = 0,    /*!< Time Offset Present */
    BLE_GLP_MEAS_TIME_OFF_PRES_BIT                  = CO_BIT(BLE_GLP_MEAS_TIME_OFF_PRES_POS),
    BLE_GLP_MEAS_GL_CTR_TYPE_AND_SPL_LOC_PRES_POS   = 1,    /*!< Glucose Concentration, Type and Sample Location Present */
    BLE_GLP_MEAS_GL_CTR_TYPE_AND_SPL_LOC_PRES_BIT   = CO_BIT(BLE_GLP_MEAS_GL_CTR_TYPE_AND_SPL_LOC_PRES_POS),
    BLE_GLP_MEAS_GL_CTR_UNITS_POS                   = 2,    /*!< Glucose Concentration Units, 0 for kg/L; 1 for mol/L */
    BLE_GLP_MEAS_GL_CTR_UNITS_BIT                   = CO_BIT(BLE_GLP_MEAS_GL_CTR_UNITS_POS),
    BLE_GLP_MEAS_SENS_STAT_ANNUN_PRES_POS           = 3,    /*!< Sensor Status Annunciation Present */
    BLE_GLP_MEAS_SENS_STAT_ANNUN_PRES_BIT           = CO_BIT(BLE_GLP_MEAS_SENS_STAT_ANNUN_PRES_POS),
    BLE_GLP_MEAS_CTX_INF_FOLW_POS                   = 4,    /*!< Context Information Follow */
    BLE_GLP_MEAS_CTX_INF_FOLW_BIT                   = CO_BIT(BLE_GLP_MEAS_CTX_INF_FOLW_POS),
};

/* Glucose Measurement Context Flags field bit values */
enum ble_glp_meas_ctx_flag_bf
{
    BLE_GLP_CTX_CRBH_ID_AND_CRBH_PRES_POS       = 0,    /*!< Carbohydrate ID And Carbohydrate Present */
    BLE_GLP_CTX_CRBH_ID_AND_CRBH_PRES_BIT       = CO_BIT(BLE_GLP_CTX_CRBH_ID_AND_CRBH_PRES_POS),
    BLE_GLP_CTX_MEAL_PRES_POS                   = 1,    /*!< Meal Present */
    BLE_GLP_CTX_MEAL_PRES_BIT                   = CO_BIT(BLE_GLP_CTX_MEAL_PRES_POS),
    BLE_GLP_CTX_TESTER_HEALTH_PRES_POS          = 2,    /*!< Tester-Health Present */
    BLE_GLP_CTX_TESTER_HEALTH_PRES_BIT          = CO_BIT(BLE_GLP_CTX_TESTER_HEALTH_PRES_POS),
    BLE_GLP_CTX_EXE_DUR_AND_EXE_INTENS_PRES_POS = 3,    /*!< Exercise Duration And Exercise Intensity Present */
    BLE_GLP_CTX_EXE_DUR_AND_EXE_INTENS_PRES_BIT = CO_BIT(BLE_GLP_CTX_EXE_DUR_AND_EXE_INTENS_PRES_POS),
    BLE_GLP_CTX_MEDIC_ID_AND_MEDIC_PRES_POS     = 4,    /*!< Medication ID And Medication Present */
    BLE_GLP_CTX_MEDIC_ID_AND_MEDIC_PRES_BIT     = CO_BIT(BLE_GLP_CTX_MEDIC_ID_AND_MEDIC_PRES_POS),
    BLE_GLP_CTX_MEDIC_VAL_UNITS_POS             = 5,    /*!< Medication Value Units, 0 for kilograms; 1 for liters */
    BLE_GLP_CTX_MEDIC_VAL_UNITS_BIT             = CO_BIT(BLE_GLP_CTX_MEDIC_VAL_UNITS_POS),
    BLE_GLP_CTX_HBA1C_PRES_POS                  = 6,    /*!< HbA1c Present */
    BLE_GLP_CTX_HBA1C_PRES_BIT                  = CO_BIT(BLE_GLP_CTX_HBA1C_PRES_POS),
    BLE_GLP_CTX_EXTD_F_PRES_POS                 = 7,    /*!< Extended Flags Present */
    BLE_GLP_CTX_EXTD_F_PRES_BIT                 = CO_BIT(BLE_GLP_CTX_EXTD_F_PRES_POS),
};


/* Glucose Service Feature Flags field bit values */
enum ble_glp_srv_feature_flag
{
    BLE_GLP_FET_LOW_BAT_DET_DUR_MEAS_SUPP_POS       = 0,    /*!< Low Battery Detection During Measurement Support */
    BLE_GLP_FET_LOW_BAT_DET_DUR_MEAS_SUPP_BIT       = CO_BIT(BLE_GLP_FET_LOW_BAT_DET_DUR_MEAS_SUPP_POS),
    BLE_GLP_FET_SENS_MFNC_DET_SUPP_POS              = 1,    /*!< Sensor Malfunction Detection Support */
    BLE_GLP_FET_SENS_MFNC_DET_SUPP_BIT              = CO_BIT(BLE_GLP_FET_SENS_MFNC_DET_SUPP_POS),
    BLE_GLP_FET_SENS_SPL_SIZE_SUPP_POS              = 2,    /*!< Sensor Sample Size Support */
    BLE_GLP_FET_SENS_SPL_SIZE_SUPP_BIT              = CO_BIT(BLE_GLP_FET_SENS_SPL_SIZE_SUPP_POS),
    BLE_GLP_FET_SENS_STRIP_INSERT_ERR_DET_SUPP_POS  = 3,    /*!< Sensor Strip Insertion Error Detection Support */
    BLE_GLP_FET_SENS_STRIP_INSERT_ERR_DET_SUPP_BIT  = CO_BIT(BLE_GLP_FET_SENS_STRIP_INSERT_ERR_DET_SUPP_POS),
    BLE_GLP_FET_SENS_STRIP_TYPE_ERR_DET_SUPP_POS    = 4,    /*!< Sensor Strip Type Error Detection Support */
    BLE_GLP_FET_SENS_STRIP_TYPE_ERR_DET_SUPP_BIT    = CO_BIT(BLE_GLP_FET_SENS_STRIP_TYPE_ERR_DET_SUPP_POS),
    BLE_GLP_FET_SENS_RES_HIGH_LOW_DET_SUPP_POS      = 5,    /*!< Sensor Result High-Low Detection Support */
    BLE_GLP_FET_SENS_RES_HIGH_LOW_DET_SUPP_BIT      = CO_BIT(BLE_GLP_FET_SENS_RES_HIGH_LOW_DET_SUPP_POS),
    BLE_GLP_FET_SENS_TEMP_HIGH_LOW_DET_SUPP_POS     = 6,    /*!< Sensor Temperature High-Low Detection Support */
    BLE_GLP_FET_SENS_TEMP_HIGH_LOW_DET_SUPP_BIT     = CO_BIT(BLE_GLP_FET_SENS_TEMP_HIGH_LOW_DET_SUPP_POS),
    BLE_GLP_FET_SENS_RD_INT_DET_SUPP_POS            = 7,    /*!< Sensor Read Interrupt Detection Support */
    BLE_GLP_FET_SENS_RD_INT_DET_SUPP_BIT            = CO_BIT(BLE_GLP_FET_SENS_RD_INT_DET_SUPP_POS),
    BLE_GLP_FET_GEN_DEV_FLT_SUPP_POS                = 8,    /*!< General Device Fault Support */
    BLE_GLP_FET_GEN_DEV_FLT_SUPP_BIT                = CO_BIT(BLE_GLP_FET_GEN_DEV_FLT_SUPP_POS),
    BLE_GLP_FET_TIME_FLT_SUPP_POS                   = 9,    /*!< Time Fault Support */
    BLE_GLP_FET_TIME_FLT_SUPP_BIT                   = CO_BIT(BLE_GLP_FET_TIME_FLT_SUPP_POS),
    BLE_GLP_FET_MUL_BOND_SUPP_POS                   = 10,   /*!< Multiple Bond Support */
    BLE_GLP_FET_MUL_BOND_SUPP_BIT                   = CO_BIT(BLE_GLP_FET_MUL_BOND_SUPP_POS),
};

/* Glucose measurement Sensor Status Annunciation */
enum ble_glp_meas_state_bf
{
    BLE_GLP_MEAS_STATE_DEV_BAT_LOW_POS               = 0,   /*!< Device battery low at time of measurement */
    BLE_GLP_MEAS_STATE_DEV_BAT_LOW_BIT               = CO_BIT(BLE_GLP_MEAS_STATE_DEV_BAT_LOW_POS),
    BLE_GLP_MEAS_STATE_SENS_MFNC_OR_FLTING_POS       = 1,   /*!< Sensor malfunction or faulting at time of measurement */
    BLE_GLP_MEAS_STATE_SENS_MFNC_OR_FLTING_BIT       = CO_BIT(BLE_GLP_MEAS_STATE_SENS_MFNC_OR_FLTING_POS),
    BLE_GLP_MEAS_STATE_SPL_SIZE_INSUFF_POS           = 2,   /*!< Sample size for blood or control solution insufficient at time of measurement */
    BLE_GLP_MEAS_STATE_SPL_SIZE_INSUFF_BIT           = CO_BIT(BLE_GLP_MEAS_STATE_SPL_SIZE_INSUFF_POS),
    BLE_GLP_MEAS_STATE_STRIP_INSERT_ERR_POS          = 3,   /*!< Strip insertion error */
    BLE_GLP_MEAS_STATE_STRIP_INSERT_ERR_BIT          = CO_BIT(BLE_GLP_MEAS_STATE_STRIP_INSERT_ERR_POS),
    BLE_GLP_MEAS_STATE_STRIP_TYPE_INCOR_FOR_DEV_POS  = 4,   /*!< Strip type incorrect for device */
    BLE_GLP_MEAS_STATE_STRIP_TYPE_INCOR_FOR_DEV_BIT  = CO_BIT(BLE_GLP_MEAS_STATE_STRIP_TYPE_INCOR_FOR_DEV_POS),
    BLE_GLP_MEAS_STATE_SENS_RES_HIGHER_POS           = 5,   /*!< Sensor result higher than the device can process */
    BLE_GLP_MEAS_STATE_SENS_RES_HIGHER_BIT           = CO_BIT(BLE_GLP_MEAS_STATE_SENS_RES_HIGHER_POS),
    BLE_GLP_MEAS_STATE_SENS_RES_LOWER_POS            = 6,   /*!< Sensor result lower than the device can process */
    BLE_GLP_MEAS_STATE_SENS_RES_LOWER_BIT            = CO_BIT(BLE_GLP_MEAS_STATE_SENS_RES_LOWER_POS),
    BLE_GLP_MEAS_STATE_SENS_TEMP_TOO_HIGH_POS        = 7,   /*!< Sensor temperature too high for valid test/result at time of measurement */
    BLE_GLP_MEAS_STATE_SENS_TEMP_TOO_HIGH_BIT        = CO_BIT(BLE_GLP_MEAS_STATE_SENS_TEMP_TOO_HIGH_POS),
    BLE_GLP_MEAS_STATE_SENS_TEMP_TOO_LOW_POS         = 8,   /*!< Sensor temperature too low for valid test/result at time of measurement */
    BLE_GLP_MEAS_STATE_SENS_TEMP_TOO_LOW_BIT         = CO_BIT(BLE_GLP_MEAS_STATE_SENS_TEMP_TOO_LOW_POS),
    BLE_GLP_MEAS_STATE_SENS_RD_INTED_POS             = 9,   /*!< Sensor read interrupted because strip was pulled too soon at time of measurement */
    BLE_GLP_MEAS_STATE_SENS_RD_INTED_BIT             = CO_BIT(BLE_GLP_MEAS_STATE_SENS_RD_INTED_POS),
    BLE_GLP_MEAS_STATE_GEN_DEV_FLT_POS               = 10,  /*!< General device fault has occurred in the sensor */
    BLE_GLP_MEAS_STATE_GEN_DEV_FLT_BIT               = CO_BIT(BLE_GLP_MEAS_STATE_GEN_DEV_FLT_POS),
    BLE_GLP_MEAS_STATE_TIME_FLT_POS                  = 11,  /*!< Time fault has occurred in the sensor and time may be inaccurate */
    BLE_GLP_MEAS_STATE_TIME_FLT_BIT                  = CO_BIT(BLE_GLP_MEAS_STATE_TIME_FLT_POS),
};

/* Glucose access control OP Code */
typedef enum
{
    BLE_GLP_REQ_RESERVED = (0),                 /*!< Reserved */
    BLE_GLP_REQ_REP_STRD_RECS = (1),            /*!< Report stored records */
    BLE_GLP_REQ_DEL_STRD_RECS = (2),            /*!< Delete stored records */
    BLE_GLP_REQ_ABORT_OP = (3),                 /*!< Abort operation */
    BLE_GLP_REQ_REP_NUM_OF_STRD_RECS = (4),     /*!< Report number of stored records */
    BLE_GLP_REQ_NUM_OF_STRD_RECS_RSP = (5),     /*!< Number of stored records response */
    BLE_GLP_REQ_RSP_CODE = (6),                 /*!< Response Code */
} ble_glp_racp_op_code_t;

/* Glucose access control Operator */
typedef enum
{
    BLE_GLP_OP_ALL_RECS = (1),          /*!< All records */
    BLE_GLP_OP_LT_OR_EQ = (2),          /*!< Less than or equal to */
    BLE_GLP_OP_GT_OR_EQ = (3),          /*!< Greater than or equal to */
    BLE_GLP_OP_WITHIN_RANGE_OF = (4),   /*!< Within range of (inclusive) */
    BLE_GLP_OP_FIRST_REC = (5),         /*!< First record(i.e. oldest record) */
    BLE_GLP_OP_LAST_REC = (6),          /*!< Last record (i.e. most recent record) */
} ble_glp_racp_operator_t;

/* Glucose access control response code */
enum ble_glp_racp_status
{
    BLE_GLP_RSP_SUCCESS = (1),                  /*!< Success */
    BLE_GLP_RSP_OP_CODE_NOT_SUP = (2),          /*!< Op Code not supported */
    BLE_GLP_RSP_INVALID_OPERATOR = (3),         /*!< Invalid Operator */
    BLE_GLP_RSP_OPERATOR_NOT_SUP = (4),         /*!< Operator not supported */
    BLE_GLP_RSP_INVALID_OPERAND = (5),          /*!< Invalid Operand */
    BLE_GLP_RSP_NO_RECS_FOUND = (6),            /*!< No records found */
    BLE_GLP_RSP_ABORT_UNSUCCESSFUL = (7),       /*!< Abort unsuccessful */
    BLE_GLP_RSP_PROCEDURE_NOT_COMPLETED = (8),  /*!< Procedure not completed */
    BLE_GLP_RSP_OPERAND_NOT_SUP = (9),          /*!< Operand not supported */
};

/* Record access control filter type */
typedef enum
{
    BLE_GLP_FILTER_SEQ_NUMBER = (1),        /*!< Filter using Sequence number */
    BLE_GLP_FILTER_USER_FACING_TIME = (2),  /*!< Filter using Facing time */
} ble_glp_racp_filter_t;

/* Record access control point operation filter union */
union ble_glp_filter {
    struct
    {
        uint16_t min;               /*!< Min sequence number */
        uint16_t max;               /*!< Max sequence number */
    } seq_num;      /*!< Sequence number filtering */

    struct
    {
        ble_prf_date_time_t facetime_min;   /*!< Min base time */
        ble_prf_date_time_t facetime_max;   /*!< Max base time */
    } time;         /*!< User facing time filtering */
};

/* Glucose measurement */
typedef struct ble_glp_meas
{
    ble_prf_date_time_t base_time;      /*!< Base Time */
    int16_t             time_offset;    /*!< Time Offset */
    ble_prf_sfloat      concentration;  /*!< Glucose Concentration, units of kg/L or mol/L */
    uint16_t            sensor_status;  /*!< Sensor Status Annunciation, @ref ble_glp_meas_state_bf */
    uint8_t             type;           /*!< Type */
    uint8_t             location;       /*!< Sample Location */
    uint8_t             flags;          /*!< Flags, @ref ble_glp_meas_flag_bf */
} ble_glp_meas_t;

/* Glucose measurement context */
typedef struct ble_glp_meas_ctx
{
    ble_prf_sfloat  carbo_val;      /*!< Carbohydrate, units of kilograms */
    uint16_t    exercise_dur;       /*!< Exercise Duration */
    ble_prf_sfloat  med_val;        /*!< Medication value, units of kilograms or liters */
    ble_prf_sfloat  hba1c_val;      /*!< HbA1c value */
    uint8_t     carbo_id;           /*!< Carbohydrate ID */
    uint8_t     meal;               /*!< Meal */
    uint8_t     tester;             /*!< Tester */
    uint8_t     health;             /*!< Health */
    uint8_t     exercise_intens;    /*!< Exercise Intensity */
    uint8_t     med_id;             /*!< Medication ID */
    uint8_t     flags;              /*!< Flag, @ref ble_glp_meas_ctx_flag_bf */
    uint8_t     ext_flags;          /*!< Extended Flags */
} ble_glp_meas_ctx_t;

#endif /* _BLE_GLP_COMM_H_ */
