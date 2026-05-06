/*!
    \file    ble_cscs.h
    \brief   Cycling Speed and Cadence Service common definitions for server and client.

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

#ifndef _BLE_CSCS_H_
#define _BLE_CSCS_H_

#include <stdint.h>

/* CSCS specific error code */
#define BLE_CSCS_ERROR_PROC_IN_PROGRESS     (0x80)  /*!< Procedure Already in Progress */
#define BLE_CSCS_ERROR_CCCD_IMPROPER_CFG    (0x81)  /*!< CCCD improperly configured */

/* CSC Measurement value length limit */
#define BLE_CSCS_CSC_MEAS_MIN_LEN           (1)     /*!< CSC Measurement value min length */
#define BLE_CSCS_CSC_MEAS_MAX_LEN           (11)    /*!< CSC Measurement value max length */

/* SC Control Point Request value length limit */
#define BLE_CSCS_SC_CTRL_PT_REQ_MIN_LEN     (1)     /*!< SC Control Point Request value min length */
#define BLE_CSCS_SC_CTRL_PT_REQ_MAX_LEN     (5)     /*!< SC Control Point Request value max length */

/* SC Control Point Response value length limit */
#define BLE_CSCS_SC_CTRL_PT_RSP_MIN_LEN     (3)     /*!< SC Control Point Response value min length */
#define BLE_CSCS_SC_CTRL_PT_RSP_MAX_LEN     (BLE_CSCS_SC_CTRL_PT_RSP_MIN_LEN + BLE_CSCS_SENSOR_LOC_MAX)    /*!< SC Control Point Response value max length */

/* CSC Measurement Flag bit field */
typedef enum
{
    BLE_CSCS_MEAS_WHEEL_REV_DATA_PRESENT = (0x01 << 0), /*!< Wheel Revolution Data Present */
    BLE_CSCS_MEAS_CRANK_REV_DATA_PRESENT = (0x01 << 1), /*!< Crank Revolution Data Present */
} ble_cscs_meas_flag_bf_t;

/* Cycling Speed and Cadence Service feature bits */
typedef enum
{
    BLE_CSCS_FEAT_WHEEL_REV_DATA_BIT  = (0x01 << 0),     /*!< Wheel Revolution Data Supported bit */
    BLE_CSCS_FEAT_CRANK_REV_DATA_BIT  = (0x01 << 1),     /*!< Crank Revolution Data Supported bit */
    BLE_CSCS_FEAT_MULT_SENSOR_LOC_BIT = (0x01 << 2),     /*!< Multiple Sensor Locations Supported bit */
} ble_cscs_feat_bf_t;

/* CSCP Sensor Locations */
typedef enum
{
    BLE_CSCS_SENSOR_LOC_OTHER        = 0 ,  /*!< Other        */
    BLE_CSCS_SENSOR_LOC_TOP_OF_SHOE  = 1 ,  /*!< Top of shoe  */
    BLE_CSCS_SENSOR_LOC_IN_SHOE      = 2 ,  /*!< In shoe      */
    BLE_CSCS_SENSOR_LOC_HIP          = 3 ,  /*!< Hip          */
    BLE_CSCS_SENSOR_LOC_FRONT_WHEEL  = 4 ,  /*!< Front Wheel  */
    BLE_CSCS_SENSOR_LOC_LEFT_CRANK   = 5 ,  /*!< Left Crank   */
    BLE_CSCS_SENSOR_LOC_RIGHT_CRANK  = 6 ,  /*!< Right Crank  */
    BLE_CSCS_SENSOR_LOC_LEFT_PEDAL   = 7 ,  /*!< Left Pedal   */
    BLE_CSCS_SENSOR_LOC_RIGHT_PEDAL  = 8 ,  /*!< Right Pedal  */
    BLE_CSCS_SENSOR_LOC_FRONT_HUB    = 9 ,  /*!< Front Hub    */
    BLE_CSCS_SENSOR_LOC_REAR_DROPOUT = 10,  /*!< Rear Dropout */
    BLE_CSCS_SENSOR_LOC_CHAINSTAY    = 11,  /*!< Chainstay    */
    BLE_CSCS_SENSOR_LOC_REAR_WHEEL   = 12,  /*!< Rear Wheel   */
    BLE_CSCS_SENSOR_LOC_REAR_HUB     = 13,  /*!< Rear Hub     */

    BLE_CSCS_SENSOR_LOC_MAX,
} ble_cscs_sensor_loc_t;

/* Control Point Operation Code Keys */
typedef enum
{
    BLE_CSCS_CTRL_PT_OP_RESERVED        = 0,    /*!< Reserved value */

    BLE_CSCS_CTRL_PT_OP_SET_CUMUL_VAL,          /*!< Set Cumulative Value */
    BLE_CSCS_CTRL_PT_OP_START_CAL,              /*!< Start Sensor Calibration */
    BLE_CSCS_CTRL_PT_OP_UPDATE_LOC,             /*!< Update Sensor Location */
    BLE_CSCS_CTRL_PT_OP_REQ_SUPP_LOC,           /*!< Request Supported Sensor Location */

    BLE_CSCS_CTRL_PT_RSP_CODE           = 16,   /*!< Response Code */
} ble_cscs_ctrl_pt_op_code;

/* Control Point Response Value */
typedef enum
{
    BLE_CSCS_CTRL_PT_RSP_RESERVED       = 0,    /*!< Reserved value */

    BLE_CSCS_CTRL_PT_RSP_SUCCESS,               /*!< Success */
    BLE_CSCS_CTRL_PT_RSP_NOT_SUPP,              /*!< Operation Code Not Supported */
    BLE_CSCS_CTRL_PT_RSP_INVALID_PARAM,         /*!< Invalid Parameter */
    BLE_CSCS_CTRL_PT_RSP_FAILED,                /*!< Operation Failed */
} ble_cscs_ctrl_pt_rsp_val;

/* CSC Measurement */
typedef struct
{
    uint8_t     flags;                 /*!< Flags */
    uint32_t    cumul_wheel_rev;       /*!< Cumulative Wheel Revolution */
    uint16_t    last_wheel_evt_time;   /*!< Last Wheel Event Time */
    uint16_t    cumul_crank_rev;       /*!< Cumulative Crank Revolution */
    uint16_t    last_crank_evt_time;   /*!< Last Crank Event Time */
} ble_cscs_csc_meas_t;

#endif //(_BLE_CSCS_H_)
