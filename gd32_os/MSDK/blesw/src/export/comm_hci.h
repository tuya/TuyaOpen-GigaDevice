/*!
    \file    comm_hci.h
    \brief   This file contains the HCI Bluetooth defines, enumerations and structures
             definitions for use by all modules in app.

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

#ifndef COMM_HCI_H_
#define COMM_HCI_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdbool.h>       // standard boolean definitions
#include <stddef.h>        // standard definitions
#include <stdint.h>        // standard integer definitions

/*
 * DEFINES
 ****************************************************************************************
 */
#define HCI_PACKET_TYPE_TO_INDEX(type) ((type) - 1)

/******************************************************************************************/
/* -------------------------   H4TL DEFINITIONS Part IV.A    -----------------------------*/
/******************************************************************************************/

enum {
    BT_PACKET_IDLE,
    BT_PACKET_TYPE,
    BT_PACKET_HEADER,
    BT_PACKET_CONTENT,
    BT_PACKET_END
};
enum hci_msg_type
{
    //UART header: command message type
    HCI_CMD_MSG_TYPE                           = 0x01,
    //UART header: ACL data message type
    HCI_ACL_MSG_TYPE                           = 0x02,
    //UART header: Synchronous data message type
    HCI_SYNC_MSG_TYPE                          = 0x03,
    //UART header: event message type
    HCI_EVT_MSG_TYPE                           = 0x04,
    //UART header: ISO data message type
    HCI_ISO_MSG_TYPE                           = 0x05,
};

/******************************************************************************************/
/* -------------------------   HCI DEFINITIONS Part 4.E     ------------------------------*/
/******************************************************************************************/

//HCI Command Opcode byte length
#define HCI_CMD_OPCODE_LEN         (0x02)

//HCI Event code byte length
#define HCI_EVT_CODE_LEN           (0x01)

//HCI Command/Event parameter length field byte length
#define HCI_CMDEVT_PARLEN_LEN      (0x01)

//HCI Command header length
#define HCI_CMD_HDR_LEN            (HCI_CMD_OPCODE_LEN + HCI_CMDEVT_PARLEN_LEN)

//HCI Event header length
#define HCI_EVT_HDR_LEN            (HCI_EVT_CODE_LEN + HCI_CMDEVT_PARLEN_LEN)

#define HCI_ACL_HDR_HDL_FLAGS_POS  (0)
#define HCI_ACL_HDR_HDL_FLAGS_LEN  (2)
// HCI ACL header: data length field length
#define HCI_ACL_HDR_DATA_LEN_POS   (HCI_ACL_HDR_HDL_FLAGS_LEN)
#define HCI_ACL_HDR_DATA_LEN_LEN   (2)

//HCI ACL data packet header length
#define HCI_ACL_HDR_LEN            (HCI_ACL_HDR_HDL_FLAGS_LEN + HCI_ACL_HDR_DATA_LEN_LEN)

#define HCI_SYNC_HDR_HDL_FLAGS_POS  (0)
#define HCI_SYNC_HDR_HDL_FLAGS_LEN  (2)

// HCI Synchronous header: data length field length
#define HCI_SYNC_HDR_DATA_LEN_POS   (HCI_SYNC_HDR_HDL_FLAGS_LEN)
#define HCI_SYNC_HDR_DATA_LEN_LEN   (1)
#define HCI_SYNC_MAX_DATA_SIZE      (255)

//HCI sync data packet header length
#define HCI_SYNC_HDR_LEN           (HCI_SYNC_HDR_HDL_FLAGS_LEN + HCI_SYNC_HDR_DATA_LEN_LEN)

//HCI Command Complete Event minimum parameter length: 1(nb_pk)+2(opcode)
#define HCI_CCEVT_HDR_PARLEN       (0x03)

//HCI Command Complete Event header length:1(code)+1(len)+1(pk)+2(opcode)
#define HCI_CCEVT_HDR_LEN          (HCI_EVT_HDR_LEN + HCI_CCEVT_HDR_PARLEN)

//HCI Basic Command Complete Event packet length
#define HCI_CCEVT_BASIC_LEN        (HCI_CCEVT_HDR_LEN + 1)

//HCI Command Status Event parameter length - constant
#define HCI_CSEVT_PARLEN           (0x04)

//HCI Command Status Event length:1(code)+1(len)+1(st)+1(pk)+2(opcode)
#define HCI_CSEVT_LEN              (HCI_EVT_HDR_LEN + HCI_CSEVT_PARLEN)

//HCI Reset Command parameter length
#define HCI_RESET_CMD_PARLEN       0

// Default return parameter length for HCI Command Complete Event
#define HCI_CCEVT_BASIC_RETPAR_LEN 1

// Max HCI commands param size
#define HCI_MAX_CMD_PARAM_SIZE    255

// Macro to extract OCF from OPCODE
#define HCI_OP2OCF(opcode)        ((opcode) & 0x03FF)

// Macro to extract OGF from OPCODE
#define HCI_OP2OGF(opcode)        ((opcode) >> 10 & 0x003F)

// Macro to create OPCODE from OGF and OCF
#define HCI_GET_OPCODE(ocf, ogf)      (((ogf) << 10) | ocf)

// Maximum length of HCI advertising data fragments
#define HCI_ADV_DATA_FRAG_MAX_LEN        251

// Maximum length of HCI periodic advertising data fragments
#define HCI_PER_ADV_DATA_FRAG_MAX_LEN    252

#define HCI_ISO_HDR_HDL_FLAGS_POS  (0)
#define HCI_ISO_HDR_HDL_FLAGS_LEN  (2)
// HCI ISO header: ISO_Data_Load field length
#define HCI_ISO_HDR_ISO_DATA_LOAD_LEN_POS   (HCI_ISO_HDR_HDL_FLAGS_LEN)
#define HCI_ISO_HDR_ISO_DATA_LOAD_LEN_LEN   (2)

//HCI ACL data packet header length
#define HCI_ISO_HDR_LEN            (HCI_ISO_HDR_HDL_FLAGS_LEN + HCI_ISO_HDR_ISO_DATA_LOAD_LEN_LEN)
#define HCI_ISO_HDR_ISO_DATA_LOAD_LEN_MASK (0x3FFF)

// Packet Boundary Flag   HCI:5.4.5
#define PB_FLAG_1ST_FRAG          0x00
#define PB_FLAG_CONT_FRAG         0x01
#define PB_FLAG_CMP_FRAG          0x02
#define PB_FLAG_LAST_FRAG         0x03

// HCI ISO header: handle and flags decoding
// Time_Stamp (32 bits)
#define HCI_ISO_DATA_LOAD_TIME_STAMP_POS      (0)
#define HCI_ISO_DATA_LOAD_TIME_STAMP_LSB      (0)
#define HCI_ISO_DATA_LOAD_TIME_STAMP_MASK     (0xFFFFFFFF)
// Packet_Sequence_Number (16 bits)
#define HCI_ISO_DATA_LOAD_PKT_SEQ_NB_POS      (4)
#define HCI_ISO_DATA_LOAD_PKT_SEQ_NB_LSB      (0)
#define HCI_ISO_DATA_LOAD_PKT_SEQ_NB_MASK     (0xFFFF)
// ISO_SDU_Length (12 bits)
#define HCI_ISO_DATA_LOAD_ISO_SDU_LEN_POS     (6)
#define HCI_ISO_DATA_LOAD_ISO_SDU_LEN_LSB     (0)
#define HCI_ISO_DATA_LOAD_ISO_SDU_LEN_MASK    (0x0FFF)
// RFU (2 bits)
#define HCI_ISO_DATA_LOAD_RFU_POS             (6)
#define HCI_ISO_DATA_LOAD_RFU_LSB             (12)
#define HCI_ISO_DATA_LOAD_RFU_MASK            (0x3000)
// Packet_Status_Flag (2 bits)
#define HCI_ISO_DATA_LOAD_PKT_STAT_FLAG_LSB   (14)
#define HCI_ISO_DATA_LOAD_PKT_STAT_FLAG_MASK  (0xC000)

// HCI ISO_Data_Load - Length of Time_Stamp field
#define HCI_ISO_DATA_LOAD_TIME_STAMP_LEN    (4)

// HCI ISO_Data_Load - Length of Packet Sequence Number field
#define HCI_ISO_DATA_LOAD_PKT_SEQ_NB_LEN    (2)

// HCI ISO_Data_Load - Length of ISO SDU Length and packet status flags field
#define HCI_ISO_DATA_LOAD_ISO_SDU_LEN_LEN   (2)

// HCI ISO_Data_Load - maximum header length
#define HCI_ISO_DATA_LOAD_HDR_LEN_MAX    (HCI_ISO_DATA_LOAD_TIME_STAMP_LEN + HCI_ISO_DATA_LOAD_PKT_SEQ_NB_LEN + HCI_ISO_DATA_LOAD_ISO_SDU_LEN_LEN)


/**************************************************************************************
 **************                       HCI COMMANDS                     ****************
 **************************************************************************************/

//HCI enumeration of possible Command OGF values.
enum
{
    //HCI Link Control Commands Group OGF code
    LK_CNTL_OGF = 0x01,
    //HCI Link Policy Commands Group OGF code
    LK_POL_OGF,
    //HCI Controller and Baseband Commands Group OGF code
    CNTLR_BB_OGF,
    //HCI Information Parameters Commands Group OGF code
    INFO_PAR_OGF,
    //HCI Status Commands Group OGF code
    STAT_PAR_OGF,
    //HCI Test Commands Group OGF code
    TEST_OGF,
    //HCI Low Energy Commands Group OGF code
    LE_CNTLR_OGF=0x08,
    //HCI Vendor Specific Group OGF code
    VS_OGF = 0x3F,
    MAX_OGF
};

//Commands Opcodes: OGF(6b) | OCF(10b)
/* Some Abbreviation used in names:
 *  - LK   = Link Key
 *  - RD   = Read
 *  - WR   = Write
 *  - REM  = Remote
 *  - STG  = Settings
 *  - CON  = Connection
 *  - CHG  = Change
 *  - DFT  = Default
 *  - PER  = Periodic
 */

//HCI enumeration of possible Command OP Codes.
/*@TRACE*/
enum hci_opcode
{
    HCI_NO_OPERATION_CMD_OPCODE               = 0x0000,

    //Link Control Commands
    HCI_INQ_CMD_OPCODE                        = 0x0401,
    HCI_INQ_CANCEL_CMD_OPCODE                 = 0x0402,
    HCI_PER_INQ_MODE_CMD_OPCODE               = 0x0403,
    HCI_EXIT_PER_INQ_MODE_CMD_OPCODE          = 0x0404,
    HCI_CREATE_CON_CMD_OPCODE                 = 0x0405,
    HCI_DISCONNECT_CMD_OPCODE                 = 0x0406,
    HCI_CREATE_CON_CANCEL_CMD_OPCODE          = 0x0408,
    HCI_ACCEPT_CON_REQ_CMD_OPCODE             = 0x0409,
    HCI_REJECT_CON_REQ_CMD_OPCODE             = 0x040A,
    HCI_LK_REQ_REPLY_CMD_OPCODE               = 0x040B,
    HCI_LK_REQ_NEG_REPLY_CMD_OPCODE           = 0x040C,
    HCI_PIN_CODE_REQ_REPLY_CMD_OPCODE         = 0x040D,
    HCI_PIN_CODE_REQ_NEG_REPLY_CMD_OPCODE     = 0x040E,
    HCI_CHG_CON_PKT_TYPE_CMD_OPCODE           = 0x040F,
    HCI_AUTH_REQ_CMD_OPCODE                   = 0x0411,
    HCI_SET_CON_ENC_CMD_OPCODE                = 0x0413,
    HCI_CHG_CON_LK_CMD_OPCODE                 = 0x0415,
    HCI_MASTER_LK_CMD_OPCODE                  = 0x0417,
    HCI_REM_NAME_REQ_CMD_OPCODE               = 0x0419,
    HCI_REM_NAME_REQ_CANCEL_CMD_OPCODE        = 0x041A,
    HCI_RD_REM_SUPP_FEATS_CMD_OPCODE          = 0x041B,
    HCI_RD_REM_EXT_FEATS_CMD_OPCODE           = 0x041C,
    HCI_RD_REM_VER_INFO_CMD_OPCODE            = 0x041D,
    HCI_RD_CLK_OFF_CMD_OPCODE                 = 0x041F,
    HCI_RD_LMP_HDL_CMD_OPCODE                 = 0x0420,
    HCI_SETUP_SYNC_CON_CMD_OPCODE             = 0x0428,
    HCI_ACCEPT_SYNC_CON_REQ_CMD_OPCODE        = 0x0429,
    HCI_REJECT_SYNC_CON_REQ_CMD_OPCODE        = 0x042A,
    HCI_IO_CAP_REQ_REPLY_CMD_OPCODE           = 0x042B,
    HCI_USER_CFM_REQ_REPLY_CMD_OPCODE         = 0x042C,
    HCI_USER_CFM_REQ_NEG_REPLY_CMD_OPCODE     = 0x042D,
    HCI_USER_PASSKEY_REQ_REPLY_CMD_OPCODE     = 0x042E,
    HCI_USER_PASSKEY_REQ_NEG_REPLY_CMD_OPCODE = 0x042F,
    HCI_REM_OOB_DATA_REQ_REPLY_CMD_OPCODE     = 0x0430,
    HCI_REM_OOB_DATA_REQ_NEG_REPLY_CMD_OPCODE = 0x0433,
    HCI_IO_CAP_REQ_NEG_REPLY_CMD_OPCODE       = 0x0434,
    HCI_ENH_SETUP_SYNC_CON_CMD_OPCODE         = 0x043D,
    HCI_ENH_ACCEPT_SYNC_CON_CMD_OPCODE        = 0x043E,
    HCI_TRUNC_PAGE_CMD_OPCODE                 = 0x043F,
    HCI_TRUNC_PAGE_CAN_CMD_OPCODE             = 0x0440,
    HCI_SET_CON_SLV_BCST_CMD_OPCODE           = 0x0441,
    HCI_SET_CON_SLV_BCST_REC_CMD_OPCODE       = 0x0442,
    HCI_START_SYNC_TRAIN_CMD_OPCODE           = 0x0443,
    HCI_REC_SYNC_TRAIN_CMD_OPCODE             = 0x0444,
    HCI_REM_OOB_EXT_DATA_REQ_REPLY_CMD_OPCODE = 0x0445,

    //Link Policy Commands
    HCI_HOLD_MODE_CMD_OPCODE                  = 0x0801,
    HCI_SNIFF_MODE_CMD_OPCODE                 = 0x0803,
    HCI_EXIT_SNIFF_MODE_CMD_OPCODE            = 0x0804,
    HCI_PARK_STATE_CMD_OPCODE                 = 0x0805,
    HCI_EXIT_PARK_STATE_CMD_OPCODE            = 0x0806,
    HCI_QOS_SETUP_CMD_OPCODE                  = 0x0807,
    HCI_ROLE_DISCOVERY_CMD_OPCODE             = 0x0809,
    HCI_SWITCH_ROLE_CMD_OPCODE                = 0x080B,
    HCI_RD_LINK_POL_STG_CMD_OPCODE            = 0x080C,
    HCI_WR_LINK_POL_STG_CMD_OPCODE            = 0x080D,
    HCI_RD_DFT_LINK_POL_STG_CMD_OPCODE        = 0x080E,
    HCI_WR_DFT_LINK_POL_STG_CMD_OPCODE        = 0x080F,
    HCI_FLOW_SPEC_CMD_OPCODE                  = 0x0810,
    HCI_SNIFF_SUB_CMD_OPCODE                  = 0x0811,

    //Controller and Baseband Commands
    HCI_SET_EVT_MASK_CMD_OPCODE               = 0x0C01,
    HCI_RESET_CMD_OPCODE                      = 0x0C03,
    HCI_SET_EVT_FILTER_CMD_OPCODE             = 0x0C05,
    HCI_FLUSH_CMD_OPCODE                      = 0x0C08,
    HCI_RD_PIN_TYPE_CMD_OPCODE                = 0x0C09,
    HCI_WR_PIN_TYPE_CMD_OPCODE                = 0x0C0A,
    HCI_CREATE_NEW_UNIT_KEY_CMD_OPCODE        = 0x0C0B,
    HCI_RD_STORED_LK_CMD_OPCODE               = 0x0C0D,
    HCI_WR_STORED_LK_CMD_OPCODE               = 0x0C11,
    HCI_DEL_STORED_LK_CMD_OPCODE              = 0x0C12,
    HCI_WR_LOCAL_NAME_CMD_OPCODE              = 0x0C13,
    HCI_RD_LOCAL_NAME_CMD_OPCODE              = 0x0C14,
    HCI_RD_CON_ACCEPT_TO_CMD_OPCODE           = 0x0C15,
    HCI_WR_CON_ACCEPT_TO_CMD_OPCODE           = 0x0C16,
    HCI_RD_PAGE_TO_CMD_OPCODE                 = 0x0C17,
    HCI_WR_PAGE_TO_CMD_OPCODE                 = 0x0C18,
    HCI_RD_SCAN_EN_CMD_OPCODE                 = 0x0C19,
    HCI_WR_SCAN_EN_CMD_OPCODE                 = 0x0C1A,
    HCI_RD_PAGE_SCAN_ACT_CMD_OPCODE           = 0x0C1B,
    HCI_WR_PAGE_SCAN_ACT_CMD_OPCODE           = 0x0C1C,
    HCI_RD_INQ_SCAN_ACT_CMD_OPCODE            = 0x0C1D,
    HCI_WR_INQ_SCAN_ACT_CMD_OPCODE            = 0x0C1E,
    HCI_RD_AUTH_EN_CMD_OPCODE                 = 0x0C1F,
    HCI_WR_AUTH_EN_CMD_OPCODE                 = 0x0C20,
    HCI_RD_CLASS_OF_DEV_CMD_OPCODE            = 0x0C23,
    HCI_WR_CLASS_OF_DEV_CMD_OPCODE            = 0x0C24,
    HCI_RD_VOICE_STG_CMD_OPCODE               = 0x0C25,
    HCI_WR_VOICE_STG_CMD_OPCODE               = 0x0C26,
    HCI_RD_AUTO_FLUSH_TO_CMD_OPCODE           = 0x0C27,
    HCI_WR_AUTO_FLUSH_TO_CMD_OPCODE           = 0x0C28,
    HCI_RD_NB_BDCST_RETX_CMD_OPCODE           = 0x0C29,
    HCI_WR_NB_BDCST_RETX_CMD_OPCODE           = 0x0C2A,
    HCI_RD_HOLD_MODE_ACTIVITY_CMD_OPCODE      = 0x0C2B,
    HCI_WR_HOLD_MODE_ACTIVITY_CMD_OPCODE      = 0x0C2C,
    HCI_RD_TX_PWR_LVL_CMD_OPCODE              = 0x0C2D,
    HCI_RD_SYNC_FLOW_CTRL_EN_CMD_OPCODE       = 0x0C2E,
    HCI_WR_SYNC_FLOW_CTRL_EN_CMD_OPCODE       = 0x0C2F,
    HCI_SET_CTRL_TO_HOST_FLOW_CTRL_CMD_OPCODE = 0x0C31,
    HCI_HOST_BUF_SIZE_CMD_OPCODE              = 0x0C33,
    HCI_HOST_NB_CMP_PKTS_CMD_OPCODE           = 0x0C35,
    HCI_RD_LINK_SUPV_TO_CMD_OPCODE            = 0x0C36,
    HCI_WR_LINK_SUPV_TO_CMD_OPCODE            = 0x0C37,
    HCI_RD_NB_SUPP_IAC_CMD_OPCODE             = 0x0C38,
    HCI_RD_CURR_IAC_LAP_CMD_OPCODE            = 0x0C39,
    HCI_WR_CURR_IAC_LAP_CMD_OPCODE            = 0x0C3A,
    HCI_SET_AFH_HOST_CH_CLASS_CMD_OPCODE      = 0x0C3F,
    HCI_RD_INQ_SCAN_TYPE_CMD_OPCODE           = 0x0C42,
    HCI_WR_INQ_SCAN_TYPE_CMD_OPCODE           = 0x0C43,
    HCI_RD_INQ_MODE_CMD_OPCODE                = 0x0C44,
    HCI_WR_INQ_MODE_CMD_OPCODE                = 0x0C45,
    HCI_RD_PAGE_SCAN_TYPE_CMD_OPCODE          = 0x0C46,
    HCI_WR_PAGE_SCAN_TYPE_CMD_OPCODE          = 0x0C47,
    HCI_RD_AFH_CH_ASSESS_MODE_CMD_OPCODE      = 0x0C48,
    HCI_WR_AFH_CH_ASSESS_MODE_CMD_OPCODE      = 0x0C49,
    HCI_RD_EXT_INQ_RSP_CMD_OPCODE             = 0x0C51,
    HCI_WR_EXT_INQ_RSP_CMD_OPCODE             = 0x0C52,
    HCI_REFRESH_ENC_KEY_CMD_OPCODE            = 0x0C53,
    HCI_RD_SP_MODE_CMD_OPCODE                 = 0x0C55,
    HCI_WR_SP_MODE_CMD_OPCODE                 = 0x0C56,
    HCI_RD_LOC_OOB_DATA_CMD_OPCODE            = 0x0C57,
    HCI_RD_INQ_RSP_TX_PWR_LVL_CMD_OPCODE      = 0x0C58,
    HCI_WR_INQ_TX_PWR_LVL_CMD_OPCODE          = 0x0C59,
    HCI_RD_DFT_ERR_DATA_REP_CMD_OPCODE        = 0x0C5A,
    HCI_WR_DFT_ERR_DATA_REP_CMD_OPCODE        = 0x0C5B,
    HCI_ENH_FLUSH_CMD_OPCODE                  = 0x0C5F,
    HCI_SEND_KEYPRESS_NOTIF_CMD_OPCODE        = 0x0C60,
    HCI_SET_EVT_MASK_PAGE_2_CMD_OPCODE        = 0x0C63,
    HCI_RD_FLOW_CNTL_MODE_CMD_OPCODE          = 0x0C66,
    HCI_WR_FLOW_CNTL_MODE_CMD_OPCODE          = 0x0C67,
    HCI_RD_ENH_TX_PWR_LVL_CMD_OPCODE          = 0x0C68,
    HCI_RD_LE_HOST_SUPP_CMD_OPCODE            = 0x0C6C,
    HCI_WR_LE_HOST_SUPP_CMD_OPCODE            = 0x0C6D,
    HCI_SET_MWS_CHANNEL_PARAMS_CMD_OPCODE     = 0x0C6E,
    HCI_SET_EXTERNAL_FRAME_CONFIG_CMD_OPCODE  = 0x0C6F,
    HCI_SET_MWS_SIGNALING_CMD_OPCODE          = 0x0C70,
    HCI_SET_MWS_TRANSPORT_LAYER_CMD_OPCODE    = 0x0C71,
    HCI_SET_MWS_SCAN_FREQ_TABLE_CMD_OPCODE    = 0x0C72,
    HCI_SET_MWS_PATTERN_CONFIG_CMD_OPCODE     = 0x0C73,
    HCI_SET_RES_LT_ADDR_CMD_OPCODE            = 0x0C74,
    HCI_DEL_RES_LT_ADDR_CMD_OPCODE            = 0x0C75,
    HCI_SET_CON_SLV_BCST_DATA_CMD_OPCODE      = 0x0C76,
    HCI_RD_SYNC_TRAIN_PARAM_CMD_OPCODE        = 0x0C77,
    HCI_WR_SYNC_TRAIN_PARAM_CMD_OPCODE        = 0x0C78,
    HCI_RD_SEC_CON_HOST_SUPP_CMD_OPCODE       = 0x0C79,
    HCI_WR_SEC_CON_HOST_SUPP_CMD_OPCODE       = 0x0C7A,
    HCI_RD_AUTH_PAYL_TO_CMD_OPCODE            = 0x0C7B,
    HCI_WR_AUTH_PAYL_TO_CMD_OPCODE            = 0x0C7C,
    HCI_RD_LOC_OOB_EXT_DATA_CMD_OPCODE        = 0x0C7D,
    HCI_RD_EXT_PAGE_TO_CMD_OPCODE             = 0x0C7E,
    HCI_WR_EXT_PAGE_TO_CMD_OPCODE             = 0x0C7F,
    HCI_RD_EXT_INQ_LEN_CMD_OPCODE             = 0x0C80,
    HCI_WR_EXT_INQ_LEN_CMD_OPCODE             = 0x0C81,
    HCI_SET_ECO_BASE_INTV_CMD_OPCODE          = 0x0C82,
    HCI_CONFIG_DATA_PATH_CMD_OPCODE           = 0x0C83,

    //Info Params
    HCI_RD_LOCAL_VER_INFO_CMD_OPCODE               = 0x1001,
    HCI_RD_LOCAL_SUPP_CMDS_CMD_OPCODE              = 0x1002,
    HCI_RD_LOCAL_SUPP_FEATS_CMD_OPCODE             = 0x1003,
    HCI_RD_LOCAL_EXT_FEATS_CMD_OPCODE              = 0x1004,
    HCI_RD_BUF_SIZE_CMD_OPCODE                     = 0x1005,
    HCI_RD_BD_ADDR_CMD_OPCODE                      = 0x1009,
    HCI_RD_DATA_BLOCK_SIZE_CMD_OPCODE              = 0x100A,
    HCI_RD_LOCAL_SUPP_CODECS_CMD_OPCODE            = 0x100B,
    HCI_RD_LOCAL_SP_OPT_CMD_OPCODE                 = 0x100C,
    HCI_RD_LOCAL_SUPP_CODECS_V2_CMD_OPCODE         = 0x100D,
    HCI_RD_LOCAL_SUPP_CODEC_CAP_CMD_OPCODE         = 0x100E,
    HCI_RD_LOCAL_SUPP_CTRL_DELAY_CMD_OPCODE        = 0x100F,

    //Status Params
    HCI_RD_FAIL_CONTACT_CNT_CMD_OPCODE             = 0x1401,
    HCI_RST_FAIL_CONTACT_CNT_CMD_OPCODE            = 0x1402,
    HCI_RD_LINK_QUAL_CMD_OPCODE                    = 0x1403,
    HCI_RD_RSSI_CMD_OPCODE                         = 0x1405,
    HCI_RD_AFH_CH_MAP_CMD_OPCODE                   = 0x1406,
    HCI_RD_CLK_CMD_OPCODE                          = 0x1407,
    HCI_RD_ENC_KEY_SIZE_CMD_OPCODE                 = 0x1408,
    HCI_GET_MWS_TRANSPORT_LAYER_CONFIG_CMD_OPCODE  = 0x140C,

    //Testing Commands
    HCI_RD_LOOPBACK_MODE_CMD_OPCODE                = 0x1801,
    HCI_WR_LOOPBACK_MODE_CMD_OPCODE                = 0x1802,
    HCI_EN_DUT_MODE_CMD_OPCODE                     = 0x1803,
    HCI_WR_SP_DBG_MODE_CMD_OPCODE                  = 0x1804,
    HCI_WR_SEC_CON_TEST_MODE_CMD_OPCODE            = 0x180A,

    // LE Commands Opcodes
    HCI_LE_SET_EVT_MASK_CMD_OPCODE                      = 0x2001,
    HCI_LE_RD_BUF_SIZE_CMD_OPCODE                       = 0x2002,
    HCI_LE_RD_LOCAL_SUPP_FEATS_CMD_OPCODE               = 0x2003,
    HCI_LE_SET_RAND_ADDR_CMD_OPCODE                     = 0x2005,
    HCI_LE_SET_ADV_PARAM_CMD_OPCODE                     = 0x2006,
    HCI_LE_RD_ADV_CHNL_TX_PW_CMD_OPCODE                 = 0x2007,
    HCI_LE_SET_ADV_DATA_CMD_OPCODE                      = 0x2008,
    HCI_LE_SET_SCAN_RSP_DATA_CMD_OPCODE                 = 0x2009,
    HCI_LE_SET_ADV_EN_CMD_OPCODE                        = 0x200A,
    HCI_LE_SET_SCAN_PARAM_CMD_OPCODE                    = 0x200B,
    HCI_LE_SET_SCAN_EN_CMD_OPCODE                       = 0x200C,
    HCI_LE_CREATE_CON_CMD_OPCODE                        = 0x200D,
    HCI_LE_CREATE_CON_CANCEL_CMD_OPCODE                 = 0x200E,
    HCI_LE_RD_WLST_SIZE_CMD_OPCODE                      = 0x200F,
    HCI_LE_CLEAR_WLST_CMD_OPCODE                        = 0x2010,
    HCI_LE_ADD_DEV_TO_WLST_CMD_OPCODE                   = 0x2011,
    HCI_LE_RMV_DEV_FROM_WLST_CMD_OPCODE                 = 0x2012,
    HCI_LE_CON_UPDATE_CMD_OPCODE                        = 0x2013,
    HCI_LE_SET_HOST_CH_CLASS_CMD_OPCODE                 = 0x2014,
    HCI_LE_RD_CH_MAP_CMD_OPCODE                         = 0x2015,
    HCI_LE_RD_REM_FEATS_CMD_OPCODE                      = 0x2016,
    HCI_LE_ENC_CMD_OPCODE                               = 0x2017,
    HCI_LE_RAND_CMD_OPCODE                              = 0x2018,
    HCI_LE_EN_ENC_CMD_OPCODE                            = 0x2019,
    HCI_LE_LTK_REQ_REPLY_CMD_OPCODE                     = 0x201A,
    HCI_LE_LTK_REQ_NEG_REPLY_CMD_OPCODE                 = 0x201B,
    HCI_LE_RD_SUPP_STATES_CMD_OPCODE                    = 0x201C,
    HCI_LE_RX_TEST_V1_CMD_OPCODE                        = 0x201D,
    HCI_LE_TX_TEST_V1_CMD_OPCODE                        = 0x201E,
    HCI_LE_TEST_END_CMD_OPCODE                          = 0x201F,
    HCI_LE_REM_CON_PARAM_REQ_REPLY_CMD_OPCODE           = 0x2020,
    HCI_LE_REM_CON_PARAM_REQ_NEG_REPLY_CMD_OPCODE       = 0x2021,
    HCI_LE_SET_DATA_LEN_CMD_OPCODE                      = 0x2022,
    HCI_LE_RD_SUGGTED_DFT_DATA_LEN_CMD_OPCODE           = 0x2023,
    HCI_LE_WR_SUGGTED_DFT_DATA_LEN_CMD_OPCODE           = 0x2024,
    HCI_LE_RD_LOC_P256_PUB_KEY_CMD_OPCODE               = 0x2025,
    HCI_LE_GEN_DHKEY_V1_CMD_OPCODE                      = 0x2026,
    HCI_LE_ADD_DEV_TO_RSLV_LIST_CMD_OPCODE              = 0x2027,
    HCI_LE_RMV_DEV_FROM_RSLV_LIST_CMD_OPCODE            = 0x2028,
    HCI_LE_CLEAR_RSLV_LIST_CMD_OPCODE                   = 0x2029,
    HCI_LE_RD_RSLV_LIST_SIZE_CMD_OPCODE                 = 0x202A,
    HCI_LE_RD_PEER_RSLV_ADDR_CMD_OPCODE                 = 0x202B,
    HCI_LE_RD_LOC_RSLV_ADDR_CMD_OPCODE                  = 0x202C,
    HCI_LE_SET_ADDR_RESOL_EN_CMD_OPCODE                 = 0x202D,
    HCI_LE_SET_RSLV_PRIV_ADDR_TO_CMD_OPCODE             = 0x202E,
    HCI_LE_RD_MAX_DATA_LEN_CMD_OPCODE                   = 0x202F,
    HCI_LE_RD_PHY_CMD_OPCODE                            = 0x2030,
    HCI_LE_SET_DFT_PHY_CMD_OPCODE                       = 0x2031,
    HCI_LE_SET_PHY_CMD_OPCODE                           = 0x2032,
    HCI_LE_RX_TEST_V2_CMD_OPCODE                        = 0x2033,
    HCI_LE_TX_TEST_V2_CMD_OPCODE                        = 0x2034,
    HCI_LE_SET_ADV_SET_RAND_ADDR_CMD_OPCODE             = 0x2035,
    HCI_LE_SET_EXT_ADV_PARAM_CMD_OPCODE                 = 0x2036,
    HCI_LE_SET_EXT_ADV_DATA_CMD_OPCODE                  = 0x2037,
    HCI_LE_SET_EXT_SCAN_RSP_DATA_CMD_OPCODE             = 0x2038,
    HCI_LE_SET_EXT_ADV_EN_CMD_OPCODE                    = 0x2039,
    HCI_LE_RD_MAX_ADV_DATA_LEN_CMD_OPCODE               = 0x203A,
    HCI_LE_RD_NB_SUPP_ADV_SETS_CMD_OPCODE               = 0x203B,
    HCI_LE_RMV_ADV_SET_CMD_OPCODE                       = 0x203C,
    HCI_LE_CLEAR_ADV_SETS_CMD_OPCODE                    = 0x203D,
    HCI_LE_SET_PER_ADV_PARAM_CMD_OPCODE                 = 0x203E,
    HCI_LE_SET_PER_ADV_DATA_CMD_OPCODE                  = 0x203F,
    HCI_LE_SET_PER_ADV_EN_CMD_OPCODE                    = 0x2040,
    HCI_LE_SET_EXT_SCAN_PARAM_CMD_OPCODE                = 0x2041,
    HCI_LE_SET_EXT_SCAN_EN_CMD_OPCODE                   = 0x2042,
    HCI_LE_EXT_CREATE_CON_CMD_OPCODE                    = 0x2043,
    HCI_LE_PER_ADV_CREATE_SYNC_CMD_OPCODE               = 0x2044,
    HCI_LE_PER_ADV_CREATE_SYNC_CANCEL_CMD_OPCODE        = 0x2045,
    HCI_LE_PER_ADV_TERM_SYNC_CMD_OPCODE                 = 0x2046,
    HCI_LE_ADD_DEV_TO_PER_ADV_LIST_CMD_OPCODE           = 0x2047,
    HCI_LE_RMV_DEV_FROM_PER_ADV_LIST_CMD_OPCODE         = 0x2048,
    HCI_LE_CLEAR_PER_ADV_LIST_CMD_OPCODE                = 0x2049,
    HCI_LE_RD_PER_ADV_LIST_SIZE_CMD_OPCODE              = 0x204A,
    HCI_LE_RD_TX_PWR_CMD_OPCODE                         = 0x204B,
    HCI_LE_RD_RF_PATH_COMP_CMD_OPCODE                   = 0x204C,
    HCI_LE_WR_RF_PATH_COMP_CMD_OPCODE                   = 0x204D,
    HCI_LE_SET_PRIV_MODE_CMD_OPCODE                     = 0x204E,
    HCI_LE_RX_TEST_V3_CMD_OPCODE                        = 0x204F,
    HCI_LE_TX_TEST_V3_CMD_OPCODE                        = 0x2050,
    HCI_LE_SET_CONLESS_CTE_TX_PARAM_CMD_OPCODE          = 0x2051,
    HCI_LE_SET_CONLESS_CTE_TX_EN_CMD_OPCODE             = 0x2052,
    HCI_LE_SET_CONLESS_IQ_SAMPL_EN_CMD_OPCODE           = 0x2053,
    HCI_LE_SET_CON_CTE_RX_PARAM_CMD_OPCODE              = 0x2054,
    HCI_LE_SET_CON_CTE_TX_PARAM_CMD_OPCODE              = 0x2055,
    HCI_LE_CON_CTE_REQ_EN_CMD_OPCODE                    = 0x2056,
    HCI_LE_CON_CTE_RSP_EN_CMD_OPCODE                    = 0x2057,
    HCI_LE_RD_ANTENNA_INF_CMD_OPCODE                    = 0x2058,
    HCI_LE_SET_PER_ADV_REC_EN_CMD_OPCODE                = 0x2059,
    HCI_LE_PER_ADV_SYNC_TRANSF_CMD_OPCODE               = 0x205A,
    HCI_LE_PER_ADV_SET_INFO_TRANSF_CMD_OPCODE           = 0x205B,
    HCI_LE_SET_PER_ADV_SYNC_TRANSF_PARAM_CMD_OPCODE     = 0x205C,
    HCI_LE_SET_DFT_PER_ADV_SYNC_TRANSF_PARAM_CMD_OPCODE = 0x205D,
    HCI_LE_GEN_DHKEY_V2_CMD_OPCODE                      = 0x205E,
    HCI_LE_MOD_SLEEP_CLK_ACC_CMD_OPCODE                 = 0x205F,
    HCI_LE_RD_BUF_SIZE_V2_CMD_OPCODE                    = 0x2060,
    HCI_LE_RD_ISO_TX_SYNC_CMD_OPCODE                    = 0x2061,
    HCI_LE_SET_CIG_PARAMS_CMD_OPCODE                    = 0x2062,
    HCI_LE_SET_CIG_PARAMS_TEST_CMD_OPCODE               = 0x2063,
    HCI_LE_CREATE_CIS_CMD_OPCODE                        = 0x2064,
    HCI_LE_REMOVE_CIG_CMD_OPCODE                        = 0x2065,
    HCI_LE_ACCEPT_CIS_REQ_CMD_OPCODE                    = 0x2066,
    HCI_LE_REJECT_CIS_REQ_CMD_OPCODE                    = 0x2067,
    HCI_LE_CREATE_BIG_CMD_OPCODE                        = 0x2068,
    HCI_LE_CREATE_BIG_TEST_CMD_OPCODE                   = 0x2069,
    HCI_LE_TERMINATE_BIG_CMD_OPCODE                     = 0x206A,
    HCI_LE_BIG_CREATE_SYNC_CMD_OPCODE                   = 0x206B,
    HCI_LE_BIG_TERMINATE_SYNC_CMD_OPCODE                = 0x206C,
    HCI_LE_REQ_PEER_SCA_CMD_OPCODE                      = 0x206D,
    HCI_LE_SETUP_ISO_DATA_PATH_CMD_OPCODE               = 0x206E,
    HCI_LE_REMOVE_ISO_DATA_PATH_CMD_OPCODE              = 0x206F,
    HCI_LE_ISO_TX_TEST_CMD_OPCODE                       = 0x2070,
    HCI_LE_ISO_RX_TEST_CMD_OPCODE                       = 0x2071,
    HCI_LE_ISO_READ_TEST_COUNTERS_CMD_OPCODE            = 0x2072,
    HCI_LE_ISO_TEST_END_CMD_OPCODE                      = 0x2073,
    HCI_LE_SET_HOST_FEATURE_CMD_OPCODE                  = 0x2074,
    HCI_LE_RD_ISO_LINK_QUALITY_CMD_OPCODE               = 0x2075,
    HCI_LE_ENH_RD_TX_PWR_LVL_CMD_OPCODE                 = 0x2076,
    HCI_LE_RD_REMOTE_TX_PWR_LVL_CMD_OPCODE              = 0x2077,
    HCI_LE_SET_PATH_LOSS_REP_PARAM_CMD_OPCODE           = 0x2078,
    HCI_LE_SET_PATH_LOSS_REP_EN_CMD_OPCODE              = 0x2079,
    HCI_LE_SET_TX_POWER_REP_EN_CMD_OPCODE               = 0x207A,
    HCI_LE_TX_TEST_V4_CMD_OPCODE                        = 0x207B
};

/**************************************************************************************
 **************                        HCI EVENTS                      ****************
 **************************************************************************************/

//Event Codes
/*@TRACE*/
enum hci_evt_code
{
    HCI_INQ_CMP_EVT_CODE                       = 0x01,
    HCI_INQ_RES_EVT_CODE                       = 0x02,
    HCI_CON_CMP_EVT_CODE                       = 0x03,
    HCI_CON_REQ_EVT_CODE                       = 0x04,
    HCI_DISC_CMP_EVT_CODE                      = 0x05,
    HCI_AUTH_CMP_EVT_CODE                      = 0x06,
    HCI_REM_NAME_REQ_CMP_EVT_CODE              = 0x07,
    HCI_ENC_CHG_EVT_CODE                       = 0x08,
    HCI_CHG_CON_LK_CMP_EVT_CODE                = 0x09,
    HCI_MASTER_LK_CMP_EVT_CODE                 = 0x0A,
    HCI_RD_REM_SUPP_FEATS_CMP_EVT_CODE         = 0x0B,
    HCI_RD_REM_VER_INFO_CMP_EVT_CODE           = 0x0C,
    HCI_QOS_SETUP_CMP_EVT_CODE                 = 0x0D,
    HCI_CMD_CMP_EVT_CODE                       = 0x0E,
    HCI_CMD_STATUS_EVT_CODE                    = 0x0F,
    HCI_HW_ERR_EVT_CODE                        = 0x10,
    HCI_FLUSH_OCCURRED_EVT_CODE                = 0x11,
    HCI_ROLE_CHG_EVT_CODE                      = 0x12,
    HCI_NB_CMP_PKTS_EVT_CODE                   = 0x13,
    HCI_MODE_CHG_EVT_CODE                      = 0x14,
    HCI_RETURN_LINK_KEYS_EVT_CODE              = 0x15,
    HCI_PIN_CODE_REQ_EVT_CODE                  = 0x16,
    HCI_LK_REQ_EVT_CODE                        = 0x17,
    HCI_LK_NOTIF_EVT_CODE                      = 0x18,
    HCI_DATA_BUF_OVFLW_EVT_CODE                = 0x1A,
    HCI_MAX_SLOT_CHG_EVT_CODE                  = 0x1B,
    HCI_RD_CLK_OFF_CMP_EVT_CODE                = 0x1C,
    HCI_CON_PKT_TYPE_CHG_EVT_CODE              = 0x1D,
    HCI_QOS_VIOL_EVT_CODE                      = 0x1E,
    HCI_PAGE_SCAN_REPET_MODE_CHG_EVT_CODE      = 0x20,
    HCI_FLOW_SPEC_CMP_EVT_CODE                 = 0x21,
    HCI_INQ_RES_WITH_RSSI_EVT_CODE             = 0x22,
    HCI_RD_REM_EXT_FEATS_CMP_EVT_CODE          = 0x23,
    HCI_SYNC_CON_CMP_EVT_CODE                  = 0x2C,
    HCI_SYNC_CON_CHG_EVT_CODE                  = 0x2D,
    HCI_SNIFF_SUB_EVT_CODE                     = 0x2E,
    HCI_EXT_INQ_RES_EVT_CODE                   = 0x2F,
    HCI_ENC_KEY_REFRESH_CMP_EVT_CODE           = 0x30,
    HCI_IO_CAP_REQ_EVT_CODE                    = 0x31,
    HCI_IO_CAP_RSP_EVT_CODE                    = 0x32,
    HCI_USER_CFM_REQ_EVT_CODE                  = 0x33,
    HCI_USER_PASSKEY_REQ_EVT_CODE              = 0x34,
    HCI_REM_OOB_DATA_REQ_EVT_CODE              = 0x35,
    HCI_SP_CMP_EVT_CODE                        = 0x36,
    HCI_LINK_SUPV_TO_CHG_EVT_CODE              = 0x38,
    HCI_ENH_FLUSH_CMP_EVT_CODE                 = 0x39,
    HCI_USER_PASSKEY_NOTIF_EVT_CODE            = 0x3B,
    HCI_KEYPRESS_NOTIF_EVT_CODE                = 0x3C,
    HCI_REM_HOST_SUPP_FEATS_NOTIF_EVT_CODE     = 0x3D,
    HCI_LE_META_EVT_CODE                       = 0x3E,
    HCI_MAX_EVT_MSK_PAGE_1_CODE                = 0x40,
    HCI_TRIGGERED_CLOCK_CAPTURE_CODE           = 0x4E,
    HCI_SYNC_TRAIN_CMP_EVT_CODE                = 0x4F,
    HCI_SYNC_TRAIN_REC_EVT_CODE                = 0x50,
    HCI_CON_SLV_BCST_REC_EVT_CODE              = 0x51,
    HCI_CON_SLV_BCST_TO_EVT_CODE               = 0x52,
    HCI_TRUNC_PAGE_CMP_EVT_CODE                = 0x53,
    HCI_SLV_PAGE_RSP_TO_EVT_CODE               = 0x54,
    HCI_CON_SLV_BCST_CH_MAP_CHG_EVT_CODE       = 0x55,
    HCI_AUTH_PAYL_TO_EXP_EVT_CODE              = 0x57,
    HCI_SAM_STATUS_CHANGE_EVT_CODE             = 0x58,
    HCI_MAX_EVT_MSK_PAGE_2_CODE                = 0x59,
    HCI_DBG_META_EVT_CODE                      = 0xFF,
};

/*@TRACE*/
enum hci_le_evt_subcode
{
    // LE Events Subcodes
    HCI_LE_CON_CMP_EVT_SUBCODE                 = 0x01,
    HCI_LE_ADV_REPORT_EVT_SUBCODE              = 0x02,
    HCI_LE_CON_UPDATE_CMP_EVT_SUBCODE          = 0x03,
    HCI_LE_RD_REM_FEATS_CMP_EVT_SUBCODE        = 0x04,
    HCI_LE_LTK_REQUEST_EVT_SUBCODE             = 0x05,
    HCI_LE_REM_CON_PARAM_REQ_EVT_SUBCODE       = 0x06,
    HCI_LE_DATA_LEN_CHG_EVT_SUBCODE            = 0x07,
    HCI_LE_RD_LOC_P256_PUB_KEY_CMP_EVT_SUBCODE = 0x08,
    HCI_LE_GEN_DHKEY_CMP_EVT_SUBCODE           = 0x09,
    HCI_LE_ENH_CON_CMP_EVT_SUBCODE             = 0x0A,
    HCI_LE_DIR_ADV_REP_EVT_SUBCODE             = 0x0B,
    HCI_LE_PHY_UPD_CMP_EVT_SUBCODE             = 0x0C,
    HCI_LE_EXT_ADV_REPORT_EVT_SUBCODE          = 0x0D,
    HCI_LE_PER_ADV_SYNC_EST_EVT_SUBCODE        = 0x0E,
    HCI_LE_PER_ADV_REPORT_EVT_SUBCODE          = 0x0F,
    HCI_LE_PER_ADV_SYNC_LOST_EVT_SUBCODE       = 0x10,
    HCI_LE_SCAN_TIMEOUT_EVT_SUBCODE            = 0x11,
    HCI_LE_ADV_SET_TERMINATED_EVT_SUBCODE      = 0x12,
    HCI_LE_SCAN_REQ_RCVD_EVT_SUBCODE           = 0x13,
    HCI_LE_CH_SEL_ALGO_EVT_SUBCODE             = 0x14,
    HCI_LE_CONLESS_IQ_REPORT_EVT_SUBCODE       = 0x15,
    HCI_LE_CON_IQ_REPORT_EVT_SUBCODE           = 0x16,
    HCI_LE_CTE_REQ_FAILED_EVT_SUBCODE          = 0x17,
    HCI_LE_PER_ADV_SYNC_TRANSF_REC_EVT_SUBCODE = 0x18,
    HCI_LE_CIS_ESTABLISHED_EVT_SUBCODE         = 0x19,
    HCI_LE_CIS_REQUEST_EVT_SUBCODE             = 0x1A,
    HCI_LE_CREATE_BIG_CMP_EVT_SUBCODE          = 0x1B,
    HCI_LE_TERMINATE_BIG_CMP_EVT_SUBCODE       = 0x1C,
    HCI_LE_BIG_SYNC_ESTABLISHED_EVT_SUBCODE    = 0x1D,
    HCI_LE_BIG_SYNC_LOST_EVT_SUBCODE           = 0x1E,
    HCI_LE_REQ_PEER_SCA_CMP_EVT_SUBCODE        = 0x1F,
    HCI_LE_PATH_LOSS_THRESHOLD_EVT_SUBCODE     = 0x20,
    HCI_LE_TX_POWER_REPORTING_EVT_SUBCODE      = 0x21,
    HCI_LE_BIG_INFO_ADV_REPORT_EVT_SUBCODE     = 0x22,
};

#endif // COMM_HCI_H_
