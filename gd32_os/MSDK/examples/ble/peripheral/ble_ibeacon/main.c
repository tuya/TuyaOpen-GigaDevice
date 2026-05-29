/*!
    \file    main.c
    \brief   Main loop of BLE iBeacon example.

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

#include <stdint.h>

#include "gd32vw55x_platform.h"
#include "wrapper_os.h"

#include "ble_export.h"
#include "ble_gap.h"
#include "ble_adapter.h"
#include "ble_adv.h"
#include "dbg_print.h"

/* Advertising channel map */
#define ADV_CHANN_MAP               (0x07)

/* Minimum advertising interval - 100ms */
#define ADV_INTERVAL_MIN            (160)
/* Maximum advertising interval - 100ms */
#define ADV_INTERVAL_MAX            (160)

/* Company ID used in iBeacon advertising data - Apple */
#define COMPANY_ID                      0x004C

/* Total length of iBeacon data */
#define IBEACON_DATA_LENGTH             0x17

/* Data value used in iBeacon data, please refer to Appler <Proximity Beacon Specification> for the detail */
#define IBEACON_TYPE                    0x02, 0x15
#define IBEACON_UUID                    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, \
                                        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
#define IBEACON_MAJOR                   0x11, 0x11
#define IBEACON_MINOR                   0x22, 0x22
#define IBEACON_MEASURED_PWR            0xC3

/* Definitions of the different BLE task priorities */
enum
{
    BLE_STACK_TASK_PRIORITY = OS_TASK_PRIORITY(2),      /*!< Priority of the BLE stack task */
    BLE_APP_TASK_PRIORITY   = OS_TASK_PRIORITY(1),      /*!< Priority of the BLE APP task */
};

/* Definitions of the different BLE task stack size requirements */
enum
{
    BLE_STACK_TASK_STACK_SIZE = 768,        /*!< BLE stack task stack size */
    BLE_APP_TASK_STACK_SIZE   = 512,        /*!< BLE APP task stack size */
};

/* iBeacon data, please refer to Appler <Proximity Beacon Specification> for the detail */
static uint8_t ibeacon_data[IBEACON_DATA_LENGTH] =
{
    IBEACON_TYPE,
    IBEACON_UUID,
    IBEACON_MAJOR,
    IBEACON_MINOR,
    IBEACON_MEASURED_PWR
};

/* Advertising index */
static uint8_t adv_idx = 0xFF;

/* Advertising state */
static ble_adv_state_t adv_state = BLE_ADV_STATE_IDLE;

/*!
    \brief      Start iBeacon advertising set
    \param[in]  adv_idx: advertising index
    \param[out] none
    \retval     none
*/
static void ibeacon_adv_start(uint8_t adv_idx)
{
    ble_adv_data_set_t adv;
    ble_adv_data_t adv_data = {0};
    ble_adv_data_manuf_data_t manuf_data;

    manuf_data.company_id = COMPANY_ID;
    manuf_data.data_len = IBEACON_DATA_LENGTH;
    manuf_data.p_data = ibeacon_data;

    adv_data.flags = BLE_GAP_ADV_FLAG_LE_ONLY_GENERAL_DISC_MODE;
    adv_data.p_manuf_specific_data = &manuf_data;

    adv.data_force = false;
    adv.data.p_data_enc = &adv_data;

    ble_adv_start(adv_idx, &adv, NULL, NULL);
}

/*!
    \brief      Callback function to handle BLE advertising events
    \param[in]  adv_evt: BLE advertising event type
    \param[in]  p_data: pointer to BLE advertising event data
    \param[in]  p_context: context data used when create advertising
    \param[out] none
    \retval     none
*/
static void ble_adv_evt_hdlr(ble_adv_evt_t adv_evt, void *p_data, void *p_context)
{
    switch (adv_evt) {
    case BLE_ADV_EVT_STATE_CHG: {
        ble_adv_state_t old_state = adv_state;
        ble_adv_state_chg_t *p_chg = (ble_adv_state_chg_t *)p_data;

        adv_state = p_chg->state;

        dbg_print(NOTICE, "adv state change 0x%x ==> 0x%x, reason 0x%x\r\n",
                  old_state, p_chg->state, p_chg->reason);

        if ((p_chg->state == BLE_ADV_STATE_CREATE) && (old_state == BLE_ADV_STATE_CREATING)) {
            ibeacon_adv_start(p_chg->adv_idx);
        } else if ((p_chg->state == BLE_ADV_STATE_START) && (old_state == BLE_ADV_STATE_ADV_DATA_SET)) {
            app_print("iBeacon advertising started\r\n");
        }
    } break;

    default:
        break;
    }
}

/*!
    \brief      Create advertising set for iBeacon
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void ibeacon_adv_create(void)
{
    ble_adv_param_t adv_param = {0};

    adv_param.param.own_addr_type = BLE_GAP_LOCAL_ADDR_STATIC;
    adv_param.param.type = BLE_GAP_ADV_TYPE_LEGACY;
    adv_param.param.prop = BLE_GAP_ADV_PROP_NON_CONN_NON_SCAN;
    adv_param.param.filter_pol = BLE_GAP_ADV_ALLOW_SCAN_ANY_CON_ANY;

    adv_param.param.disc_mode = BLE_GAP_ADV_MODE_GEN_DISC;
    adv_param.param.primary_phy = BLE_GAP_PHY_1MBPS;

    adv_param.param.ch_map = ADV_CHANN_MAP;
    adv_param.param.adv_intv_min = ADV_INTERVAL_MIN;
    adv_param.param.adv_intv_max = ADV_INTERVAL_MAX;

    ble_adv_create(&adv_param, ble_adv_evt_hdlr, NULL);
}

/*!
    \brief      Callback function to handle BLE adapter events
    \param[in]  event: BLE adapter event type
    \param[in]  p_data: pointer to BLE advertising event data
    \param[out] none
    \retval     none
*/
static void ble_adp_evt_handler(ble_adp_evt_t event, ble_adp_data_u *p_data)
{
    switch (event) {
    case BLE_ADP_EVT_ENABLE_CMPL_INFO:
        ibeacon_adv_create();
        break;

    default:
        break;
    }
}

/*!
    \brief      Init BLE module
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void ble_init(void)
{
    ble_init_param_t param = {0};

    ble_os_api_t os_interface = {
      .os_malloc = sys_malloc,
      .os_calloc = sys_calloc,
      .os_mfree = sys_mfree,
      .os_memset = sys_memset,
      .os_memcpy = sys_memcpy,
      .os_memcmp = sys_memcmp,
      .os_task_create = sys_task_create,
      .os_task_init_notification = sys_task_init_notification,
      .os_task_wait_notification = sys_task_wait_notification,
      .os_task_notify = sys_task_notify,
      .os_task_delete = sys_task_delete,
      .os_ms_sleep = sys_ms_sleep,
      .os_current_task_handle_get = sys_current_task_handle_get,
      .os_queue_init = sys_queue_init,
      .os_queue_free = sys_queue_free,
      .os_queue_write = sys_queue_write,
      .os_queue_read = sys_queue_read,
      .os_random_bytes_get = sys_random_bytes_get,
    };

    ble_power_on();

    param.role = BLE_GAP_ROLE_PERIPHERAL;
    param.ble_task_stack_size = BLE_STACK_TASK_STACK_SIZE;
    param.ble_task_priority = BLE_STACK_TASK_PRIORITY;
    param.ble_app_task_stack_size = BLE_APP_TASK_STACK_SIZE;
    param.ble_app_task_priority = BLE_APP_TASK_PRIORITY;
    param.en_cfg = 0;
    param.p_os_api = &os_interface;
    ble_sw_init(&param);

    ble_adp_callback_register(ble_adp_evt_handler);
    /* The BLE interrupt must be enabled after ble_sw_init. */
    ble_irq_enable();
}

/*!
    \brief       Main entry point
                 This function is called right after the booting process has completed.
    \param[in]  none
    \param[out] none
    \retval     none
*/
int main(void)
{
    sys_os_init();
    platform_init();

    app_print("Starting Gigadevice BLE iBeacon example\r\n");

    ble_init();

    sys_os_start();

    for ( ; ; );
}
