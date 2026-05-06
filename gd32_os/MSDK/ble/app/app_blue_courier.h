/*!
    \file    app_blue_courier_prot.c
    \brief   Header file for Implemetions of blue courier wifi internal.

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

#ifndef _APP_BLUE_COURIER_H_
#define _APP_BLUE_COURIER_H_

#include <stdint.h>
#include "ble_adv.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Blue courier wifi value length */
#define BCW_VALUE_LEN                           512
/* Blue courier wifi message maximum fragment length */
#define BCW_FRAG_MAX_LEN                        256

/* Blue courier wifi gatt UUIDs */
#define BCW_GATT_SERVICE_UUID                   BLE_GATT_UUID_16_LSB(0xFFF0)
#define BCW_GATT_WRITE_UUID                     BLE_GATT_UUID_16_LSB(0xFFF1)
#define BCW_GATT_NTF_UUID                       BLE_GATT_UUID_16_LSB(0xFFF2)

/* bcwl_header_t flags */
#define BCWL_FLAG_BEGIN_MASK                    0x01
#define BCWL_FLAG_END_MASK                      0x02
#define BCWL_FLAG_REQ_ACK_MASK                  0x04

#define BCWL_FLAG_IS_BEGIN(flag)                ((flag) & BCWL_FLAG_BEGIN_MASK)
#define BCWL_FLAG_IS_END(flag)                  ((flag) & BCWL_FLAG_END_MASK)
#define BCWL_FLAG_IS_REQ_ACK(flag)              ((flag) & BCWL_FLAG_REQ_ACK_MASK)

/* Packet type mask and lsb */
#define BCWL_OPCODE_TYPE_MASK                   0xC0
#define BCWL_OPCODE_TYPE_LSB                    6
#define BCWL_OPCODE_SUBTYPE_MASK                0x3F
#define BCWL_OPCODE_SUBTYPE_LSB                 0

#define BCWL_OPCODE_GET_TYPE(opcode)            (((opcode) & BCWL_OPCODE_TYPE_MASK) >> BCWL_OPCODE_TYPE_LSB)
#define BCWL_OPCODE_GET_SUBTYPE(opcode)         (((opcode) & BCWL_OPCODE_SUBTYPE_MASK) >> BCWL_OPCODE_SUBTYPE_LSB)
#define BCWL_OPCODE_BUILD(type, subtype)        (((type << BCWL_OPCODE_TYPE_LSB) & BCWL_OPCODE_TYPE_MASK) | ((subtype) & BCWL_OPCODE_SUBTYPE_MASK))

/* Packet opcode type */
enum bcw_opcode_type
{
    BCWL_OPCODE_TYPE_MGMT = 0x00,
    BCWL_OPCODE_TYPE_DATA = 0x01,
};

/* Management packet opcode subtype */
enum bcw_opcode_mgmt_subtype
{
    BCWL_OPCODE_MGMT_SUBTYPE_HANDSHAKE         = 0x00,
    BCWL_OPCODE_MGMT_SUBTYPE_ACK               = 0x01,
    BCWL_OPCODE_MGMT_SUBTYPE_ERROR_REPORT      = 0x02,
};

/* Data packet opcode subtype */
enum bcw_opcode_data_subtype
{
    BCWL_OPCODE_DATA_SUBTYPE_CUSTOM_DATA        = 0x00,
    BCWL_OPCODE_DATA_SUBTYPE_GET_SCAN_LIST      = 0x01,
    BCWL_OPCODE_DATA_SUBTYPE_STAMODE_CONNECT    = 0x02,
    BCWL_OPCODE_DATA_SUBTYPE_STAMODE_DISCONNECT = 0x03,
    BCWL_OPCODE_DATA_SUBTYPE_SOFTAPMODE_START   = 0x04,
    BCWL_OPCODE_DATA_SUBTYPE_SOFTAPMODE_STOP    = 0x05,
    BCWL_OPCODE_DATA_SUBTYPE_STATUS_GET         = 0x06,
};

/* Blue courier wifi error code */
enum bcw_error_code
{
    BCWL_ERR_NEGOTIATE_FAIL,
    BCWL_ERR_PACKET_LEN_ERROR,
    BCWL_ERR_UNKNOWN_OPCODE,
    BCWL_ERR_SEND_NO_MEM,
    BCWL_ERR_RECV_NO_MEM,
    BCWL_ERR_ENCRYPT_FAIL,
    BCWL_ERR_DECRYPT_FAIL,
    BCWL_ERR_NO_HANDSHAKE,
    BCWL_ERR_SEQUENCE_ERROR,
    BCWL_ERR_CRC_CHECK,
    BCWL_ERR_FLAG_ERROR,
    BCWL_ERR_RECV_ERROR,
};

/* Blue courier wifi attribute index */
enum bcw_att_idx
{
    // Service blue courier wifi
    BCW_IDX_PRIM_SVC,
    BCW_IDX_CHAR_WRITE,
    BCW_IDX_WRITE,
    BCW_IDX_CHAR_NTF,
    BCW_IDX_NTF,
    BCW_IDX_NTF_CFG,

    BCW_IDX_NUMBER,
};

/* Blue courier wifi link environment struct */
typedef struct
{
    uint8_t         mode;               /*!< Blue courier wifi mode. 0: disable; 1: enable */
    uint8_t         conn_id;            /*!< Connection id. use to ble_srv_ntf_ind_send */
    uint8_t         adv_idx;            /*!< Advertising id. use to stop advertising */
    bool            remove_after_stop;  /*!< remove advertising after stop */
    uint16_t        ntf_cfg;            /*!< NTF CCCD value */
    ble_adv_state_t adv_state;          /*!< Advertising state */
    uint8_t         recv_seq;           /*!< Receive sequence number */
    uint8_t         send_seq;           /*!< Send sequence number */
    uint8_t        *recv_buf;           /*!< Receive current buffer */
    uint16_t        total_len;          /*!< Receive total len */
    uint16_t        offset;             /*!< Receive current buffer offset */
    uint8_t         frag_size;          /*!< Receive and send fragment size */
    uint16_t        peer_recv_size;     /*!< Peer device receive max size */
    bool            handshake_success;  /*!< Handshake status */
} bcwl_env_t;

/* Blue courier wifi link message header */
typedef struct
{
    uint8_t         flag;
    uint8_t         seq;
    uint8_t         opcode;
    uint8_t         data_len;
    uint8_t         data[0];
} bcwl_header_t;

/* Blue courier wifi link handshake message */
typedef struct
{
    uint16_t        mtu;
    uint16_t        recv_size;
} bcwl_mgmt_handshake_t;

extern bcwl_env_t bcwl_env;

/*!
    \brief      Blue courier wifi link send message to peer device
    \param[in]  opcode: opcode
    \param[in]  data: pointer to message data
    \param[in]  len: message length
    \param[out] none
    \retval     none
*/
void bcwl_send(uint8_t opcode, uint8_t *data, uint16_t len);

/*!
    \brief      Blue courier wifi protocol message handler
    \param[in]  subtype: message subtype, @ref bcw_opcode_data_subtype
    \param[in]  data: pointer to message data
    \param[in]  len: message length
    \param[out] none
    \retval     none
*/
void bcwp_msg_handler(uint8_t subtype, uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // _APP_BLUE_COURIER_H_
