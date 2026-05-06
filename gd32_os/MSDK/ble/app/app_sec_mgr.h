/*!
    \file    app_sec_mgr.c
    \brief   Definitions of BLE application security manager.

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

#ifndef APP_SEC_MGR_H_
#define APP_SEC_MGR_H_

#include <stdint.h>
#include "ble_gap.h"

typedef struct
{
    void (*authen_cmpl)(uint8_t conn_idx, uint8_t result);

    void (*input_key_req)(uint8_t conn_idx);

    void (*key_cfm_req)(uint8_t conn_idx, uint32_t key);
} app_sec_callbacks;


/*!
    \brief      Reset application security module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_sec_mgr_reset(void);

/*!
    \brief      Init application security module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_sec_mgr_init(void);

/*!
    \brief      Deinit application security module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_sec_mgr_deinit(void);

/*!
    \brief      Check if bond is needed
    \param[in]  none
    \param[out] none
    \retval     bool: true if bond is needed, otherwise false
*/
bool app_sec_need_authen_bond(void);

/*!
    \brief      Check if a device is the one under pairing
    \param[in]  address: peer device address
    \param[out] none
    \retval     bool: true if the device is the pairing one, otherwise false
*/
bool app_sec_is_pairing_device(ble_gap_addr_t address);

/*!
    \brief      Send security request
    \param[in]  conidx: connection index
    \param[out] none
    \retval     none
*/
void app_sec_send_security_req(uint8_t conidx);

/*!
    \brief      Send pairing request
    \param[in]  conidx: connection index
    \param[out] none
    \retval     none
*/
void app_sec_send_bond_req(uint8_t conidx);

/*!
    \brief      Send encryption request
    \param[in]  conidx: connection index
    \param[out] none
    \retval     none
*/
void app_sec_send_encrypt_req(uint8_t conidx);

/*!
    \brief      Input passkey for pairing
    \param[in]  conidx: connection index
    \param[in]  passkey: passkey value
    \param[out] none
    \retval     none
*/
void app_sec_input_passkey(uint8_t conidx, uint32_t passkey);

/*!
    \brief      Input OOB key for TK
    \param[in]  conidx: connection index
    \param[in]  p_oob: pointer to the OOB key data
    \param[out] none
    \retval     none
*/
void app_sec_input_oob(uint8_t conidx, uint8_t *p_oob);

/*!
    \brief      Set numeric comparison result
    \param[in]  conidx: connection index
    \param[in]  accept: true to accept the numeric comparison, otherwisw false
    \param[out] none
    \retval     none
*/
void app_sec_num_compare(uint8_t conidx, bool accept);

/*!
    \brief      Set OOB data
    \param[in]  p_conf: pointer to confirm value
    \param[in]  p_rand: pointer to random number
    \param[out] none
    \retval     none
*/
void app_set_oob_data(uint8_t *p_conf, uint8_t *p_rand);

/*!
    \brief      Generate OOB data
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_gen_oob_data(void);

/*!
    \brief      Set authentication parameters
    \param[in]  bond: true to support bond authentication, otherwise false
    \param[in]  mitm: true to support man-in-the-middle protection, otherwise false
    \param[in]  sc: true to support secure connection, otherwise false
    \param[in]  iocap: IO capabilities. @ref ble_gap_io_cap_t
    \param[in]  oob: true to support OOB, otherwise false
    \param[in]  sc_only: true to support secure connection only, otherwise false
    \param[in]  key_size: encrytion key size
    \param[out] none
    \retval     none
*/
void app_sec_set_authen(bool bond, bool mitm, bool sc, uint8_t iocap, bool oob, bool sc_only, uint8_t key_size);

/*!
    \brief      Initiate pairing procedure
    \param[in]  address: peer device address
    \param[out] none
    \retval     bool: true if pairing is initiated successfully, otherwise false
*/
bool app_sec_create_bond(ble_gap_addr_t address);

/*!
    \brief      Remove bond information
    \param[in]  address: peer device address
    \param[out] none
    \retval     bool: true if bond information can be removed, otherwise false
*/
bool app_sec_remove_bond(ble_gap_addr_t address);

/*!
    \brief      Cancel the ongoing pairing procedure
    \param[in]  none
    \param[out] none
    \retval     bool: true if pairing can be cancelled, otherwise false
*/
bool app_sec_cancel_bonding(void);

/*!
    \brief      Get if security keys are managered by application
    \param[in]  none
    \param[out] none
    \retval     bool: true if security keys are managered by application, otherwise false
*/
bool app_sec_user_key_mgr_get(void);

bool app_sec_pin_code_set(uint32_t pin_code);

uint32_t app_sec_pin_code_get(void);

bool app_sec_callbacks_set(app_sec_callbacks cb);

#endif // APP_SEC_MGR_H_
