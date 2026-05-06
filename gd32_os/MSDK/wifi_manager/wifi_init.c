/*!
    \file    wifi_init.c
    \brief   WiFi Initialization for GD32VW55x SDK.

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

#include "wrapper_os.h"
#include "wifi_management.h"
#include "wifi_export.h"
#include "wifi_init.h"
#include "dbg_print.h"
#include "wifi_netif.h"
#include "wifi_wpa.h"
#include "lwip/tcpip.h"
#include "gd32vw55x_platform.h"

// Semaphore signaled when all wifi tasks are ready
static os_sema_t wifi_ready_sem;

// Bitfield of task that are not yet fully initialized
static uint32_t wifi_pending_task;

// Semaphore signaled when all wifi tasks are terminated
static os_sema_t wifi_terminate_sem;

// Bitfield of task that are already terminated
static uint32_t wifi_terminated_task;

// Indicate WiFi is existed or not (CFG_WLAN_SUPPORT)
extern uint8_t wifi_exist_flag;

/*
 * FUNCTIONS
 ****************************************************************************************
 */
static void wifi_ready_cb(void)
{
#if !defined(CONFIG_INTERNAL_DEBUG) && !defined(CONFIG_RF_TEST_SUPPORT)
    static uint8_t ext_heap_initialized = 0;
    if (!ext_heap_initialized) {
        sys_add_heap_region(0x20048000, 0x8000);
        ext_heap_initialized = 1;
    }
#endif
}

/*!
    \brief      indicate the wifi task is ready
    \param[in]  task_id: index of wifi task
    \param[out] none
    \retval     none
*/
void wifi_task_ready(enum wifi_task_id task_id)
{
    sys_enter_critical();
    wifi_pending_task &= ~(CO_BIT(task_id));
    sys_exit_critical();


    dbg_print(DEBUG, "Task %d is now initialized\r\n", task_id);

    if (wifi_pending_task == 0 && wifi_ready_sem != NULL)
    {
        dbg_print(DEBUG, "All WIFI tasks are initialized\r\n");
        sys_sema_up(&wifi_ready_sem);
    }
}

/*!
    \brief      wait for wifi to be ready
    \param[in]  none
    \param[out] none
    \retval     0 on success and != 0 if error occured.
*/
int wifi_wait_ready(void)
{
    if (wifi_ready_sem == NULL)
        return -1;

    if (sys_sema_down(&wifi_ready_sem, 0))
        return -1;

    // always re-signal the semaphore in case it is called by several tasks.
    sys_sema_up(&wifi_ready_sem);

    wifi_ready_cb();

    return 0;
}

/*!
    \brief      terminate the wifi task
    \param[in]  task_id: index of wifi task
    \param[out] none
    \retval     none
*/
void wifi_task_terminated(enum wifi_task_id task_id)
{
    wifi_terminated_task |= CO_BIT(task_id);

    dbg_print(DEBUG, "Task %d is now terminated\r\n", task_id);

    sys_sema_up(&wifi_terminate_sem);
}

/*!
    \brief      wait for wifi task to be terminated
    \param[in]  task_id: index of wifi task
    \param[out] none
    \retval     0 on success and != 0 if error occured.
*/
int wifi_wait_terminated(enum wifi_task_id task_id)
{
    if (wifi_terminate_sem == NULL)
        return -1;

    if (sys_sema_down(&wifi_terminate_sem, 0))
        return -1;

    if (!(wifi_terminated_task & CO_BIT(task_id))) {
        dbg_print(ERR, "Task %d is not terminated!!!!!!\r\n", task_id);
    }

    return 0;
}

static void tcpip_init_done(void *arg)
{
    wifi_task_ready(IP_TASK);
}

/*!
    \brief      Initialize wifi module
    \param[in]  none
    \param[out] none
    \retval     0 on success and != 0 if error occured.
*/
int wifi_sw_init(void)
{
#ifdef CONFIG_RF_TEST_SUPPORT
    uint32_t level = 0, module = 0;
#endif
    bool init_mac = true;

    wifi_wakelock_acquire();

    wifi_pending_task |= (CO_BIT(MACIF_RX_TASK)     |
                          CO_BIT(MACIF_TX_TASK)     |
#ifdef CONFIG_WPA_SUPPLICANT
                          CO_BIT(MACIF_CONTROL_TASK)|
                          CO_BIT(SUPPLICANT_TASK)   |
#endif
                          CO_BIT(WIFI_CORE_TASK)    |
                          CO_BIT(WIFI_MGMT_TASK));
    wifi_terminated_task = 0;

    // Initialize the semaphores
    if (sys_sema_init_ext(&wifi_ready_sem, 1, 0)) {
        return -1;
    }
    if (sys_sema_init_ext(&wifi_terminate_sem, 1, 0)) {
        return -2;
    }

    // net l2 sema and mutex
    if (net_init())
    {
        dbg_print(ERR, "net init failed\r\n");
        return -3;
    }

    // WiFi core init
#ifdef CONFIG_WPA_SUPPLICANT
    if (wifi_core_init(init_mac, true))
#else  /* CONFIG_WPA_SUPPLICANT */
    if (wifi_core_init(init_mac, false))
#endif  /* CONFIG_WPA_SUPPLICANT */
    {
        dbg_print(ERR, "wifi core init failed\r\n");
        return -4;
    }

#ifdef CONFIG_WPA_SUPPLICANT
    // WPA supplicant environment
    if (wifi_wpa_init())
    {
        dbg_print(ERR, "wifi wpa init failed\r\n");
        return -5;
    }
#endif  /* CONFIG_WPA_SUPPLICANT */

    // Wifi management task
    if (wifi_management_init())
    {
        dbg_print(ERR, "wifi management init failed\r\n");
        return -6;
    }

#ifdef CONFIG_RF_TEST_SUPPORT
    // close DBG moudle.
    macif_dbg_filter_get(&level, &module);
    module = module & (~(0x01));
    macif_dbg_filter_set(level, module);
#endif

    return 0;
}

/*!
    \brief      Release wifi module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void wifi_sw_deinit(void)
{
    wifi_management_deinit();

#ifdef CONFIG_WPA_SUPPLICANT
    wifi_wpa_deinit();
#endif  /* CONFIG_WPA_SUPPLICANT */

#ifdef CONFIG_WPA_SUPPLICANT
    wifi_core_deinit(true);
#else
    wifi_core_deinit(false);
#endif

    wifi_vifs_deinit();

    net_deinit();

    sys_sema_free(&wifi_ready_sem);
    wifi_ready_sem = NULL;

    sys_sema_free(&wifi_terminate_sem);
    wifi_terminate_sem = NULL;

    wifi_wakelock_release();
}

/*!
    \brief      Initialize wifi power and wifi module
    \param[in]  none
    \param[out] none
    \retval     0 on success and != 0 if error occured.
*/
int wifi_init(void)
{
    int ret = 0;

    /* 1. Initialize the TCP/IP stack */
    wifi_pending_task = CO_BIT(IP_TASK);
    tcpip_init(tcpip_init_done, NULL);

    /* 2. wifi power on */
    wifi_exist_flag = 1;
    ret = wifi_power_on();
    if (ret) {
        dbg_print(ERR, "wifi power on failed\r\n");
        return ret;
    }
    /* 3. wifi enable IRQ */
    wifi_irq_enable();

    /* 4. WiFi sw init */
    ret = wifi_sw_init();
    if (ret) {
        dbg_print(ERR, "wifi sw init failed\r\n");
        return ret;
    }

    return 0;
}
