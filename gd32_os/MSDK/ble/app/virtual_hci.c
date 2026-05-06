/*!
    \file    virtual_hci.c
    \brief   Implementation of the Virtual Hci.

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

#ifdef CFG_VIRTUAL_HCI_MODE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "cyclic_buffer.h"
#include "wrapper_os.h"
#include "dbg_print.h"
#include "virtual_hci.h"
#include "ble_export.h"

typedef enum
{
    VIR_HCI_H2C_MSG_TYPE = 0,
    VIR_HCI_C2H_MSG_TYPE,
    VIR_HCI_CTL_READ_MSG_TYPE,
    VIR_HCI_RESET_MSG_TYPE,
} vir_hci_msg_type_t;

enum eif_status
{
    // EIF status OK
    EIF_STATUS_OK,
    // EIF status KO
    EIF_STATUS_ERROR,
};

typedef struct
{
    void (*read_callback)(void *, uint8_t);
    uint8_t  *p_read_buf;
    uint32_t  read_size;
    void     *p_read_dummy;
} hci_read_cb_t;

typedef struct
{
    vir_hci_msg_type_t    type;
    union {
        hci_read_cb_t   reab_cb;
    } info;
} vir_hci_msg_t;

typedef struct
{
    os_task_t    task_handle;
    cyclic_buf_t send_cyclic_buf;
    cyclic_buf_t recv_cyclic_buf;
    hci_recv_callback_t callback;

    uint8_t  *p_read_buf;
    uint32_t  read_size;
    void     *p_read_dummy;
} vir_hci_cb_t;


#define VIRTUAL_HCI_STACK_SIZE          768
#define STATIC_CYCLIC_BUFFER_LEN_MAX    2048
#define BLE_VIR_HCI_TASK_QUEUE_SIZE     128

static os_queue_t vir_hci_msg_queue;

static uint8_t received_packet_state = BT_PACKET_IDLE;
static uint32_t received_packet_bytes_need = 0;
static uint8_t recv_packet_current_type = 0;
static uint8_t received_resvered_header[16] = {0};
static uint16_t received_resvered_length = 0;

static const uint8_t hci_preamble_sizes[] = {
    HCI_CMD_HDR_LEN,
    HCI_ACL_HDR_LEN,
    HCI_SYNC_HDR_LEN,
    HCI_EVT_HDR_LEN,
    HCI_ISO_HDR_LEN
};

static void virtual_hci_read(uint8_t *bufptr, uint32_t size, void (*callback)(void *, uint8_t),
                             void *dummy);
static void virtual_hci_write(uint8_t *bufptr, uint32_t size, void (*callback)(void *, uint8_t),
                              void *dummy);
static bool virtual_hci_flow_off(void);
static void virtual_hci_flow_on(void);

static vir_hci_cb_t vir_hci_cb;
void (*read_wait_callback)(void *, uint8_t);

ble_uart_func_t vir_hci_api = {
    virtual_hci_read,
    virtual_hci_write,
    virtual_hci_flow_on,
    virtual_hci_flow_off,
};

static bool vir_hci_msg_send(vir_hci_msg_type_t type, void *p_data, uint16_t data_len)
{
    vir_hci_msg_t msg;

    msg.type = type;

    if (p_data) {
        sys_memcpy((uint8_t *)&msg.info, p_data, data_len);
    }

    if (sys_queue_write(&vir_hci_msg_queue, &msg, 0, false) == 0) {
        return true;
    } else {
        dbg_print(ERR, "ble app msg send fail! type %d \r\n", type);
        return false;
    }
}

static void virtual_hci_write(uint8_t *p_buf, uint32_t size, void (*callback)(void *, uint8_t),
                              void *dummy)
{
    if (!cyclic_buf_write(&(vir_hci_cb.recv_cyclic_buf), p_buf, size)) {
        callback(dummy, EIF_STATUS_ERROR);
    } else {
        callback(dummy, EIF_STATUS_OK);
        vir_hci_msg_send(VIR_HCI_C2H_MSG_TYPE, NULL, 0);
    }
}

static void virtual_hci_read_handler(hci_read_cb_t *p_reab_cb)
{
    read_wait_callback = p_reab_cb->read_callback;
    vir_hci_cb.p_read_buf = p_reab_cb->p_read_buf;
    vir_hci_cb.read_size = p_reab_cb->read_size;
    vir_hci_cb.p_read_dummy = p_reab_cb->p_read_dummy;
}

static void tx_start_handler()
{
    if (read_wait_callback) {
        if (cyclic_buf_read(&(vir_hci_cb.send_cyclic_buf), vir_hci_cb.p_read_buf, vir_hci_cb.read_size)) {
            read_wait_callback(vir_hci_cb.p_read_dummy, EIF_STATUS_OK);
            read_wait_callback = NULL;
            vir_hci_cb.p_read_buf = NULL;
            vir_hci_cb.p_read_dummy = NULL;
            vir_hci_cb.read_size = 0;
        } else {
            //just wait more data
        }
    }
}

static void virtual_hci_read(uint8_t *p_buf, uint32_t size, void (*callback)(void *, uint8_t),
                             void *dummy)
{
    hci_read_cb_t reab_cb;

    // same task just read
    if (vir_hci_cb.task_handle == sys_current_task_handle_get()) {
        if (cyclic_buf_read(&(vir_hci_cb.send_cyclic_buf), p_buf, size)) {
            callback(dummy, EIF_STATUS_OK);
            return;
        }
    }

    reab_cb.read_callback = callback;
    reab_cb.p_read_buf = p_buf;
    reab_cb.read_size = size;
    reab_cb.p_read_dummy = dummy;

    vir_hci_msg_send(VIR_HCI_CTL_READ_MSG_TYPE, &reab_cb, sizeof(hci_read_cb_t));
}

static bool virtual_hci_flow_off(void)
{
    return true;
}

static void virtual_hci_flow_on(void)
{
    return;
}


static void handle_event(uint8_t *p_header, uint16_t payload_length)
{
    if (vir_hci_cb.callback.handle_event) {
        vir_hci_cb.callback.handle_event(p_header, payload_length);
    }
    cyclic_buf_drop(&(vir_hci_cb.recv_cyclic_buf), payload_length);
}

static void handle_acl_data(uint8_t *p_header, uint16_t payload_length)
{
    if (vir_hci_cb.callback.handle_acl) {
        vir_hci_cb.callback.handle_acl(p_header, payload_length);
    }
    cyclic_buf_drop(&(vir_hci_cb.recv_cyclic_buf), payload_length);
}

static void handle_sco_data(uint8_t *p_header, uint16_t payload_length)
{
    if (vir_hci_cb.callback.handle_sco) {
        vir_hci_cb.callback.handle_sco(p_header, payload_length);
    }
    cyclic_buf_drop(&(vir_hci_cb.recv_cyclic_buf), payload_length);
}

static void handle_iso_data(uint8_t *p_header, uint16_t payload_length)
{
    if (vir_hci_cb.callback.handle_iso) {
        vir_hci_cb.callback.handle_iso(p_header, payload_length);
    }
    cyclic_buf_drop(&(vir_hci_cb.recv_cyclic_buf), payload_length);
}

static void rx_parse_handler()
{
    uint8_t type = 0;

    while (cyclic_buf_count(&(vir_hci_cb.recv_cyclic_buf))) {
        switch (received_packet_state) {
        case BT_PACKET_IDLE: {
            received_packet_bytes_need = 1;
            while (cyclic_buf_count(&(vir_hci_cb.recv_cyclic_buf))) {
                cyclic_buf_read(&(vir_hci_cb.recv_cyclic_buf), &type, 1);
                if (type < HCI_ACL_MSG_TYPE || type > HCI_ISO_MSG_TYPE) {
                    continue;
                }
                break;
            }
            recv_packet_current_type = type;
            received_packet_state = BT_PACKET_TYPE;
        }
        //no break

        case BT_PACKET_TYPE: {
            received_packet_bytes_need = hci_preamble_sizes[HCI_PACKET_TYPE_TO_INDEX(recv_packet_current_type)];
            received_resvered_length = 0;
            received_packet_state = BT_PACKET_HEADER;
        }
        //no break

        case BT_PACKET_HEADER: {
            if (cyclic_buf_read(&(vir_hci_cb.recv_cyclic_buf),
                                &received_resvered_header[received_resvered_length], received_packet_bytes_need)) {
                received_resvered_length += received_packet_bytes_need;
            } else {
                //not enough data, just return to wait
                return;
            }
            received_packet_state = BT_PACKET_CONTENT;

            if (recv_packet_current_type == HCI_ACL_MSG_TYPE) {
                received_packet_bytes_need = *(uint16_t *)&received_resvered_header[2];
            } else if (recv_packet_current_type == HCI_EVT_MSG_TYPE) {
                received_packet_bytes_need = received_resvered_header[1];
            } else if (recv_packet_current_type == HCI_SYNC_MSG_TYPE) {
                received_packet_bytes_need = received_resvered_header[2];
            } else {
                received_packet_bytes_need = (*(uint16_t *)&received_resvered_header[2]) &
                                             HCI_ISO_HDR_ISO_DATA_LOAD_LEN_MASK;
            }
        }
        //no break

        case BT_PACKET_CONTENT: {
            if (cyclic_buf_count(&(vir_hci_cb.recv_cyclic_buf)) < received_packet_bytes_need) {
                //not enough data, just return to wait
                return;
            }

            if (recv_packet_current_type == HCI_ACL_MSG_TYPE) {
                handle_acl_data(received_resvered_header, received_packet_bytes_need);
            } else if (recv_packet_current_type == HCI_EVT_MSG_TYPE) {
                handle_event(received_resvered_header, received_packet_bytes_need);
            } else if (recv_packet_current_type == HCI_SYNC_MSG_TYPE) {
                handle_sco_data(received_resvered_header, received_packet_bytes_need);
            } else {
                handle_iso_data(received_resvered_header, received_packet_bytes_need);
            }
            received_packet_state = BT_PACKET_END;
        }
        //no break

        case BT_PACKET_END:

            break;

        default:

            break;
        }

        received_packet_state = BT_PACKET_IDLE;
        received_packet_bytes_need = 0;
        recv_packet_current_type = 0;
        received_resvered_length = 0;

    }
}

static bool virtual_hci_reset_handler(void)
{
    received_packet_state = BT_PACKET_IDLE;
    received_packet_bytes_need = 0;
    recv_packet_current_type = 0;
    received_resvered_length = 0;

    read_wait_callback = NULL;
    vir_hci_cb.p_read_buf = NULL;
    vir_hci_cb.read_size = 0;
    vir_hci_cb.p_read_dummy = NULL;

    cyclic_buf_clear(&vir_hci_cb.recv_cyclic_buf);
    cyclic_buf_clear(&vir_hci_cb.send_cyclic_buf);

    return true;
}

static void vir_hci_task(void *param)
{
    vir_hci_msg_t msg;

    for (;;) {
        sys_queue_read(&vir_hci_msg_queue, &msg, -1, false);

        switch (msg.type) {
        case VIR_HCI_CTL_READ_MSG_TYPE:
            virtual_hci_read_handler(&(msg.info.reab_cb));
            tx_start_handler();
            break;

        case VIR_HCI_H2C_MSG_TYPE:
            ble_stack_task_resume(false);
            tx_start_handler();
            break;

        case VIR_HCI_C2H_MSG_TYPE:
            rx_parse_handler();
            break;

        case VIR_HCI_RESET_MSG_TYPE:
            virtual_hci_reset_handler();
            break;

        default:
            break;
        }
    }
}

bool virtual_hci_send_command(uint16_t opcode, uint8_t length, uint8_t *p_payload)
{
    uint8_t type = HCI_CMD_MSG_TYPE;

    if ((length > 0 && p_payload == NULL) ||
        cyclic_buf_room(&(vir_hci_cb.send_cyclic_buf)) < (HCI_CMD_HDR_LEN + length + 1)) {
        return false;
    }

    cyclic_buf_write(&(vir_hci_cb.send_cyclic_buf), &type, 1);
    cyclic_buf_write(&(vir_hci_cb.send_cyclic_buf), (uint8_t *)&opcode, 2);
    cyclic_buf_write(&(vir_hci_cb.send_cyclic_buf), &length, 1);
    if (length > 0) {
        cyclic_buf_write(&(vir_hci_cb.send_cyclic_buf), p_payload, length);
    }

    vir_hci_msg_send(VIR_HCI_H2C_MSG_TYPE, NULL, 0);
    return true;
}

bool virtual_hci_send_acl_data(uint16_t hdl_flags, uint16_t length, uint8_t *p_payload)
{
    uint8_t type = HCI_ACL_MSG_TYPE;

    if ((length > 0 && p_payload == NULL) ||
        cyclic_buf_room(&(vir_hci_cb.send_cyclic_buf)) < (HCI_ACL_HDR_LEN + length + 1)) {
        return false;
    }

    cyclic_buf_write(&(vir_hci_cb.send_cyclic_buf), &type, 1);
    cyclic_buf_write(&(vir_hci_cb.send_cyclic_buf), (uint8_t *)&hdl_flags, 2);
    cyclic_buf_write(&(vir_hci_cb.send_cyclic_buf), (uint8_t *)&length, 2);
    if (length > 0) {
        cyclic_buf_write(&(vir_hci_cb.send_cyclic_buf), p_payload, length);
    }

    vir_hci_msg_send(VIR_HCI_H2C_MSG_TYPE, NULL, 0);
    return true;
}

//send iso data with no ts
bool virtual_hci_send_iso_data(uint16_t hdl_flags, uint16_t length, uint8_t *p_payload)
{
    uint8_t type = HCI_ISO_MSG_TYPE;

    if ((length > 0 && p_payload == NULL) ||
        cyclic_buf_room(&(vir_hci_cb.send_cyclic_buf)) < (HCI_ISO_HDR_LEN + length + 1)) {
        return false;
    }
    hdl_flags |= 0x2000;      //pb_flag = complete ts = no
    cyclic_buf_write(&(vir_hci_cb.send_cyclic_buf), &type, 1);
    cyclic_buf_write(&(vir_hci_cb.send_cyclic_buf), (uint8_t *)&hdl_flags, 2);
    cyclic_buf_write(&(vir_hci_cb.send_cyclic_buf), (uint8_t *)&length, 2);
    if (length > 0) {
        cyclic_buf_write(&(vir_hci_cb.send_cyclic_buf), p_payload, length);
    }

    vir_hci_msg_send(VIR_HCI_H2C_MSG_TYPE, NULL, 0);
    return true;
}

bool virtual_hci_send_raw_data(uint8_t type, uint8_t length, uint8_t *p_payload)
{
    if (length == 0 || (length > 0 && p_payload == NULL) ||
        cyclic_buf_room(&(vir_hci_cb.send_cyclic_buf)) < (length + 1)) {
        return false;
    }

    cyclic_buf_write(&(vir_hci_cb.send_cyclic_buf), &type, 1);
    if (length > 0) {
        cyclic_buf_write(&(vir_hci_cb.send_cyclic_buf), p_payload, length);
    }

    vir_hci_msg_send(VIR_HCI_H2C_MSG_TYPE, NULL, 0);
    return true;
}

bool virtual_hci_get_payload(uint8_t *p_buf, uint16_t payload_len)
{
    if (payload_len > 0 && p_buf == NULL) {
        return false;
    }

    return cyclic_buf_peek(&(vir_hci_cb.recv_cyclic_buf), p_buf, payload_len);
}

bool virtual_hci_reset(void)
{
    return vir_hci_msg_send(VIR_HCI_RESET_MSG_TYPE, NULL, 0);
}

bool virtual_hci_init(hci_recv_callback_t callback, ble_uart_func_t **pp_uart_func)
{
    sys_memset(&vir_hci_cb, 0, sizeof(vir_hci_cb));

    if (!cyclic_buf_init(&vir_hci_cb.recv_cyclic_buf, STATIC_CYCLIC_BUFFER_LEN_MAX)) {
        dbg_print(ERR, "virtual_hci_init init receive cyclic buf fail");
        return false;
    }

    if (!cyclic_buf_init(&vir_hci_cb.send_cyclic_buf, STATIC_CYCLIC_BUFFER_LEN_MAX)) {
        dbg_print(ERR, "virtual_hci_init init send cyclic buf fail");
        cyclic_buf_free(&vir_hci_cb.recv_cyclic_buf);
        return false;
    }

    if (sys_queue_init(&vir_hci_msg_queue, BLE_VIR_HCI_TASK_QUEUE_SIZE, sizeof(vir_hci_msg_t))) {
        cyclic_buf_free(&vir_hci_cb.recv_cyclic_buf);
        cyclic_buf_free(&vir_hci_cb.send_cyclic_buf);
        return false;
    }

    vir_hci_cb.task_handle = sys_task_create_dynamic((const uint8_t *)"Virtual Hci task",
                                                     VIRTUAL_HCI_STACK_SIZE, OS_TASK_PRIORITY(3), vir_hci_task, NULL);
    if (vir_hci_cb.task_handle == NULL) {
        cyclic_buf_free(&vir_hci_cb.recv_cyclic_buf);
        cyclic_buf_free(&vir_hci_cb.send_cyclic_buf);
        sys_queue_free(&vir_hci_msg_queue);
        dbg_print(ERR, "create virtual hci task fail");
        return false;
    }

    received_packet_state = BT_PACKET_IDLE;
    received_packet_bytes_need = 0;
    recv_packet_current_type = 0;
    received_resvered_length = 0;
    vir_hci_cb.callback = callback;

    read_wait_callback = NULL;

    *pp_uart_func = &vir_hci_api;

    return true;
}

#endif /* CFG_VIRTUAL_HCI_MODE */
