/*!
    \file    ble_export.h
    \brief   Declarations related to the BLE software api.

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

#ifndef BLE_EXPORT_H_
#define BLE_EXPORT_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "ble_gap.h"
#include "ble_error.h"

typedef enum  {
    EN_BTSNOOP_CAPTURE_CFG = 0x00000001,
} ble_cfg_func_t;

typedef void *os_task_t;
typedef void *os_queue_t;
typedef void (*task_func_t)(void *argv);

typedef struct ble_os_api
{
    void* (*os_malloc)(size_t size);

    void* (*os_calloc)(size_t count, size_t size);

    void (*os_mfree)(void *ptr);

    void (*os_memset)(void *s, uint8_t c, uint32_t count);

    void (*os_memcpy)(void *des, const void *src, uint32_t n);

    int32_t (*os_memcmp)(const void *buf1, const void *buf2, uint32_t count);

    void* (*os_task_create)(void *static_tcb, const uint8_t *name, uint32_t *stack_base, uint32_t stack_size,
                    uint32_t queue_size, uint32_t queue_item_size, uint32_t priority, task_func_t func, void *ctx);

    int (*os_task_init_notification)(void *task);

    int (*os_task_wait_notification)(int timeout);

    void (*os_task_notify)(void *task, bool isr);

    void (*os_task_delete)(void *task);

    void (*os_ms_sleep)(int ms);

    os_task_t (*os_current_task_handle_get)(void);

    int32_t (*os_queue_init)(os_queue_t *queue, int32_t queue_size, uint32_t item_size);

    void (*os_queue_free)(os_queue_t *queue);

    int (*os_queue_write)(os_queue_t *queue, void *message, int timeout, bool isr);

    int (*os_queue_read)(os_queue_t *queue, void *message, int timeout, bool isr);

    int32_t (*os_random_bytes_get)(void *dst, uint32_t size);
} ble_os_api_t;

/* Prototype of BLE APP message handler */
typedef bool (*ble_app_msg_hdl_t)(void *p_msg);

/* BLE work status */
enum ble_work_status_t {
    BLE_WORK_STATUS_ENABLE,         /*!< BLE enable, pmu on and task running */
    BLE_WORK_STATUS_DISABLE,        /*!< BLE disable, pmu off and task deleted */
};

/*!
    \brief      Prototype of function called when uart packet transmission/reception is finished
    \param[in]  dummy: dummy data pointer returned
    \param[in]  status: status returned, 0 for success
    \param[out] none
    \retval     none
*/
typedef void (*ble_uart_callback)(void *dummy, uint8_t status);

/* BLE uart related functions */
typedef struct ble_uart_func
{
    /*!
        \brief      Start data reception
        \param[in]  size: expected data size to receive
        \param[in]  callback: pointer to the function called when reception is finished
        \param[in]  dummy: dummy data pointer returned to callback when reception is finished
        \param[out] buf: pointer to the buffer to receive the data
        \retval     none
    */
    void (*read)(uint8_t *buf, uint32_t size, ble_uart_callback callback, void *dummy);

    /*!
        \brief      Start data transmission
        \param[in]  buf: pointer to the buffer of the transmission data
        \param[in]  size: data size to transmit
        \param[in]  callback: pointer to the function called when transmission is finished
        \param[in]  dummy: dummy data pointer returned to callback when transmission is finished
        \param[out] none
        \retval     none
    */
    void (*write)(uint8_t *buf, uint32_t size, ble_uart_callback callback, void *dummy);

    /*!
        \brief      Enable Interface flow
        \param[in]  none
        \param[out] none
        \retval     none
    */
    void (*flow_on)(void);

    /*!
        \brief      Disable Interface flow
        \param[in]  none
        \param[out] none
        \retval     bool: true if flow has been disabled, otherwise false
    */
    bool (*flow_off)(void);
} ble_uart_func_t;

/* BLE init parameters */
typedef struct ble_init_param
{
    uint8_t role;                       /*!< Device role, @ref ble_gap_role */
    uint8_t privacy_cfg;                /*!< Privacy configuration bit field, @ref ble_gap_privacy_cfg */
    uint8_t private_identity[6];        /*!< Private static identity address, meaningful if @ref BLE_GAP_PRIV_CFG_PRIV_ADDR_BIT set in privacy_cfg */

    bool    keys_user_mgr;              /*!< Application manager pairing keys */
    uint8_t pairing_mode;               /*!< Pairing mode authorized, @ref ble_gap_pairing_mode */

    ble_gap_write_att_perm_t name_perm;         /*!< Permission requirement to write name in GATT database */
    ble_gap_write_att_perm_t appearance_perm;   /*!< Permission requirement to write appearance  in GATT database  */

    uint32_t ble_task_stack_size;       /*!< BLE task's stack size in words (4 bytes) */
    uint32_t ble_task_priority;         /*!< BLE task's priority */

    uint32_t ble_app_task_stack_size;   /*!< BLE app task's stack size in words (4 bytes) */
    uint32_t ble_app_task_priority;     /*!< BLE app task's priority */

    ble_cfg_func_t   en_cfg;            /*!< BLE stack function configuration */
    ble_os_api_t    *p_os_api;          /*!< Pinter to OS related functions */

    ble_uart_func_t *p_hci_uart_func;   /*!< Pointer to HCI uart related functions */
} ble_init_param_t;

/*!
    \brief      Initialize the BLE software
    \param[in]  p_param: pointer to init parameters
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_sw_init(ble_init_param_t *p_param);

/*!
    \brief      Deinit BLE software module
    \param[in]  none
    \param[out] none
    \retval     ble_status_t: BLE_ERR_NO_ERROR on success, otherwise an error code
*/
ble_status_t ble_sw_deinit(void);

/*!
    \brief      Request the RTOS to resume the BLE stack task
    \details    This function first checks if the task was indeed suspended and then proceed to the
                resume. Note that currently this function is supposed to be called from interrupt
    \param[in]  isr: indicates if this function is called from ISR
    \param[out] none
    \retval     none
*/
void ble_stack_task_resume(bool isr);

/*!
    \brief      Send message to BLE APP task
    \param[in]  p_msg: pointer to message data
    \param[in]  msg_len: message length
    \param[out] none
    \retval     bool: true if message was sent successfully, otherwise false
*/
bool ble_local_app_msg_send(void *p_msg, uint16_t msg_len);

/*!
    \brief      Register handler function to handle messages sent by @ref ble_local_app_msg_send in BLE APP task
    \param[in]  p_hdl: message handler function
    \param[out] none
    \retval     none
*/
void ble_app_msg_hdl_reg(ble_app_msg_hdl_t p_hdl);

/*!
    \brief      Set BLE sleep mode
    \param[in]  mode: 1 for deep sleep mode and 0 for no deep sleep mode
    \param[out] none
    \retval     none
*/
void ble_sleep_mode_set(uint8_t mode);

/*!
    \brief      Get BLE sleep mode
    \param[in]  none
    \param[out] none
    \retval     uint8_t: 1 for deep sleep mode and 0 for no deep sleep mode
*/
uint8_t ble_sleep_mode_get(void);

/*!
    \brief      Determine if the BLE core is in deep sleep mode
    \param[in]  none
    \param[out] none
    \retval     bool: true if BLE is in deep sleep mode and false if BLE is not in deep sleep mode
*/
bool ble_core_is_deep_sleep(void);

/*!
    \brief      BLE modem config, need to be reconfigured after deep sleep wake
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_modem_config(void);

/*!
    \brief      BLE half slot interrupt handler
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_hslot_isr(void);

/*!
    \brief      BLE sleep wake up interrupt handler
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_slp_isr(void);

/*!
    \brief      BLE crypt complete interrupt handler
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_crypt_isr(void);

/*!
    \brief      BLE software interrupt handler
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_sw_isr(void);

/*!
    \brief      BLE fine timer interrupt handler
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_fine_tgt_isr(void);

/*!
    \brief      BLE timer 1 interrupt handler
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_ts_tgt1_isr(void);

/*!
    \brief      BLE timer 2 interrupt handler
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_ts_tgt2_isr(void);

/*!
    \brief      BLE timer 3 interrupt handler
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_ts_tgt3_isr(void);

/*!
    \brief      BLE frequency hop calculation complete interrupt handler
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_hop_isr(void);

/*!
    \brief      BLE err interrupt handler
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_error_isr(void);

/*!
    \brief      BLE fifo interrupt handler
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_fifo_isr(void);

/*!
    \brief      Get BLE work status
    \param[in]  none
    \param[out] none
    \retval     ble_work_status_t: @ref ble_work_status_t
*/
enum ble_work_status_t ble_work_status_get(void);

/*!
    \brief      Encode data using internal algorithms
    \param[in]  data: pointer to input data
    \param[in]  data: input data length
    \param[in]  rand: random make the same data encoded without repetition
    \param[out] data: pointer to output data
    \retval     none
*/
void ble_internal_encode(uint8_t *data, uint16_t len, uint8_t rand);

/*!
    \brief      Decode data using internal algorithms
    \param[in]  data: pointer to input data
    \param[in]  data: input data length
    \param[in]  rand: random make the same data encoded without repetition
    \param[out] data: pointer to output data
    \retval     none
*/
void ble_internal_decode(uint8_t *data, uint16_t len, uint8_t rand);

#endif // BLE_EXPORT_H_
