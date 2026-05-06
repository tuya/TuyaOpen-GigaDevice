/*!
    \file    virtual_hci.h
    \brief   Definitions of the Virtual Hci.

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

#ifndef _VIRTUAL_HCI_H_
#define _VIRTUAL_HCI_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "comm_hci.h"
#include "ble_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CFG_VIRTUAL_HCI_MODE
typedef struct hci_recv_callback
{
    void (*handle_event)(uint8_t *p_header, uint16_t payload_length);
    void (*handle_acl)(uint8_t *p_header, uint16_t payload_length);
    void (*handle_iso)(uint8_t *p_header, uint16_t payload_length);
    void (*handle_sco)(uint8_t *p_header, uint16_t payload_length);
} hci_recv_callback_t;

bool virtual_hci_send_command(uint16_t opcode, uint8_t length, uint8_t *p_payload);

bool virtual_hci_send_acl_data(uint16_t hdl_flags, uint16_t length, uint8_t *p_payload);

bool virtual_hci_send_iso_data(uint16_t hdl_flags, uint16_t length, uint8_t *p_payload);

bool virtual_hci_send_raw_data(uint8_t type, uint8_t length, uint8_t *p_payload);

bool virtual_hci_get_payload(uint8_t *p_buf, uint16_t payload_len);

bool virtual_hci_reset(void);

bool virtual_hci_init(hci_recv_callback_t callback, ble_uart_func_t **p_uart_func);

#endif /* CFG_VIRTUAL_HCI_MODE */

#ifdef __cplusplus
}
#endif

#endif // _VIRTUAL_HCI_H_
