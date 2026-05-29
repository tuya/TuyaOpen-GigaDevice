/*!
    \file    ble_init.c
    \brief   Implementation of the BLE initialization.

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

#include "mesh_cfg.h"
#include "app_cfg.h"
#include "ble_init.h"
#include "ble_export.h"
#include "ble_types.h"

#include "ble_adapter.h"
#include "ble_adv.h"
#include "ble_scan.h"
#include "ble_conn.h"
#include "ble_l2cap_coc.h"
#include "ble_sec.h"
#include "ble_gap.h"

#include "ble_storage.h"
#include "wrapper_os.h"
#include "ble_gatts.h"
#include "ble_gattc.h"

#include "raw_flash_api.h"
#include "gd32vw55x_platform.h"
#include "dbg_print.h"

#include "ble_list.h"
#include "ble_utils.h"
#include "ble_init.h"
//#include "util.h"
#include "mesh_cfg.h"
#include "api/mesh.h"
#include "app_mesh.h"

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
    BLE_APP_TASK_STACK_SIZE   = 1024,        /*!< BLE APP task stack size */
};

/* BLE sleep mode when flash erase need to execute */
uint8_t flash_erase_sleep_mode = 0;

/* Semaphore signaled when all BLE tasks are ready */
static os_sema_t ble_ready_sem;

/*!
    \brief      Function to notify other modules that BLE is in ready state
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_task_ready(void)
{
    sys_sema_up(&ble_ready_sem);
}

/*!
    \brief      Function for other modules to wait BLE into ready state
    \param[in]  none
    \param[out] none
    \retval     int: 0 if successfully enter wait state, otherwise -1
*/
int ble_wait_ready(void)
{
    if (ble_ready_sem == NULL)
        return -1;

    if (sys_sema_down(&ble_ready_sem, 0))
        return -1;

    // always re-signal the semaphore in case it is called by several tasks.
    sys_sema_up(&ble_ready_sem);
    return 0;
}

/*!
    \brief      Function to handle before/after flash erase execute
    \param[in]  type: erase handle type
    \param[out] none
    \retval     none
*/
void ble_flash_erase_handler(raw_erase_type_t type)
{
    if (type == RAW_FLASH_ERASE_BLE_PRE_HANDLE) {
        flash_erase_sleep_mode = ble_sleep_mode_get();
        if (flash_erase_sleep_mode != 0) {
            ble_sleep_mode_set(0);
            ble_stack_task_resume(false);
            // wait ble pmu on, timeout 10ms
            ble_wait_sleep_exit(10);
        }
    } else if (type == RAW_FLASH_ERASE_BLE_AFTER_HANDLE) {
        if (flash_erase_sleep_mode != 0) {
            ble_sleep_mode_set(flash_erase_sleep_mode);
        }
    }

    return;
}

/*!
    \brief      Callback function to handle BLE adapter events
    \param[in]  event: BLE adapter event type
    \param[in]  p_data: pointer to BLE adapter event data
    \param[out] none
    \retval     none
*/
static void app_adp_evt_handler(ble_adp_evt_t event, ble_adp_data_u *p_data)
{
    uint8_t i = 0;

    if (event == BLE_ADP_EVT_ENABLE_CMPL_INFO) {
         if (p_data->adapter_info.status == BLE_ERR_NO_ERROR) {
            app_print("=== Adapter enable success ===\r\n");
            app_print( "hci_ver 0x%x, hci_subver 0x%x, lmp_ver 0x%x, lmp_subver 0x%x, manuf_name 0x%x\r\n",
                   p_data->adapter_info.version.hci_ver, p_data->adapter_info.version.hci_subver,
                   p_data->adapter_info.version.lmp_ver, p_data->adapter_info.version.lmp_subver,
                   p_data->adapter_info.version.manuf_name);

            app_print( "adv_set_num %u, min_tx_pwr %d, max_tx_pwr %d, max_adv_data_len %d \r\n",
                   p_data->adapter_info.adv_set_num, p_data->adapter_info.tx_pwr_range.min_tx_pwr,
                   p_data->adapter_info.tx_pwr_range.max_tx_pwr, p_data->adapter_info.max_adv_data_len);
            app_print( "sugg_max_tx_octets %u, sugg_max_tx_time %u \r\n",
                   p_data->adapter_info.sugg_dft_data.sugg_max_tx_octets,
                   p_data->adapter_info.sugg_dft_data.sugg_max_tx_time);

            app_print( "loc irk:");

            for (i = 0; i < BLE_GAP_KEY_LEN; i++) {
                app_print( " %02x", p_data->adapter_info.loc_irk_info.irk[i]);
            }

            app_print( "\r\n");
            app_print( "identity addr %02X:%02X:%02X:%02X:%02X:%02X \r\n ",
                   p_data->adapter_info.loc_irk_info.identity.addr[5],
                   p_data->adapter_info.loc_irk_info.identity.addr[4],
                   p_data->adapter_info.loc_irk_info.identity.addr[3],
                   p_data->adapter_info.loc_irk_info.identity.addr[2],
                   p_data->adapter_info.loc_irk_info.identity.addr[1],
                   p_data->adapter_info.loc_irk_info.identity.addr[0]);

            app_print("=== BLE Adapter enable complete ===\r\n");
            ble_task_ready();
            ble_adp_name_set((uint8_t *)CONFIG_BT_DEVICE_NAME, strlen(CONFIG_BT_DEVICE_NAME));
            app_mesh_init();
        } else {
            app_print("=== BLE Adapter enable fail ===\r\n");
        }

    }
}

/*!
    \brief      Init adapter application module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_adapter_init(void)
{
    ble_adp_callback_register(app_adp_evt_handler);
}

/*!
    \brief      Initialization of the BLE module
    \details    This function allocates all the resources needed by the different BLE sub-modules
                This function initialize the command processing
                This function will use the wrapper_os API to create tasks, semaphores, etc
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ble_init(void)
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

    sys_sema_init_ext(&ble_ready_sem, 1, 0);

    param.role = BLE_GAP_ROLE_PERIPHERAL | BLE_GAP_ROLE_CENTRAL;
    param.ble_task_stack_size = BLE_STACK_TASK_STACK_SIZE;
    param.ble_task_priority = BLE_STACK_TASK_PRIORITY;
    param.ble_app_task_stack_size = BLE_APP_TASK_STACK_SIZE;
    param.ble_app_task_priority = BLE_APP_TASK_PRIORITY;
    param.keys_user_mgr = false;
    param.pairing_mode = BLE_GAP_PAIRING_SECURE_CONNECTION | BLE_GAP_PAIRING_LEGACY;
    param.privacy_cfg = BLE_GAP_PRIV_CFG_PRIV_EN_BIT;
    param.name_perm = BLE_GAP_WRITE_NOT_ENC;
    param.appearance_perm = BLE_GAP_WRITE_NOT_ENC;
    param.en_cfg = 1;
    param.p_os_api = &os_interface;
    param.p_hci_uart_func = NULL;
    ble_sw_init(&param);

    app_adapter_init();

    /* ble need to close deep sleep before flash erase */
    raw_flash_erase_handler_register(ble_flash_erase_handler);
    /* The BLE interrupt must be enabled after ble_sw_init. */
    ble_irq_enable();
}
