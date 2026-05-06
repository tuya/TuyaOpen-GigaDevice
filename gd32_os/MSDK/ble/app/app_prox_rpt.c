/*!
    \file    app_prox_rpt.c
    \brief   Proximity Reporter Application Module entry point.

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

#include "ble_app_config.h"

#if (BLE_PROFILE_PROX_SERVER)

#include "wrapper_os.h"
#include "dbg_print.h"
#include "app_prox_rpt.h"
#include "gd32vw55x.h"

#define LED_LINK_LOSS         GPIO_PIN_11
#define LED_PATH_LOSS         GPIO_PIN_12

/* Proximity reporter application environment variable */
typedef struct
{
    // Only trace one device
    bool            traced;
    ble_gap_addr_t  peer_addr;
    os_timer_t      lls_timer;
    uint16_t        lls_cnt;
    proxm_alert_lvl_t lls_alert_level;
    os_timer_t      path_loss_timer;
    uint16_t        path_loss_cnt;
    uint16_t        total_time;
    proxm_alert_lvl_t path_loss_alert_level;
} app_proxr_env_t;

#define MILD_ALTER_MS       800
#define HIGH_ALTER_MS       200

static void app_lls_alert_update(ble_gap_addr_t peer_addr, proxm_alert_lvl_t alert_level);
static void app_path_loss_alert_update(ble_gap_addr_t peer_addr, proxm_alert_lvl_t alert_level);

static ble_proxr_callbacks_t proxr_callbacks = {
    .lls_alert_update = app_lls_alert_update,
    .path_loss_alert_update = app_path_loss_alert_update,
};

static app_proxr_env_t app_proxr_env;

/*!
    \brief      Configure LED setting for proximity reporter application
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void app_proxr_led_config(void)
{
    /* enable the LED GPIO clock */
    rcu_periph_clock_enable(RCU_GPIOB);
    /* configure LED GPIO pin */
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_LINK_LOSS);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, LED_LINK_LOSS);
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PATH_LOSS);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, LED_PATH_LOSS);
}

/*!
    \brief      Update lls alert level value
    \param[in]  peer_addr: peer device address
    \param[in]  alert_level: lls alert level value
    \param[out] none
    \retval     none
*/
static void app_lls_alert_update(ble_gap_addr_t peer_addr, proxm_alert_lvl_t alert_level)
{
    dbg_print(NOTICE, "app_lls_alert_update level %d\r\n", alert_level);
    if (!app_proxr_env.traced) {
        app_proxr_env.peer_addr = peer_addr;
        app_proxr_env.traced = true;
    } else if (sys_memcmp(&peer_addr, &app_proxr_env.peer_addr, sizeof(ble_gap_addr_t))) {
        dbg_print(WARNING, "app_lls_alert_update not support multiple device!\r\n");
        return;
    }

    if (alert_level != app_proxr_env.lls_alert_level) {
        if (alert_level == PROXM_ALERT_NONE) {
            sys_timer_stop(&app_proxr_env.lls_timer, false);
        } else if (alert_level == PROXM_ALERT_MILD) {
            // FIX TODO lls_cnt has race condition
            app_proxr_env.lls_cnt = app_proxr_env.total_time / MILD_ALTER_MS;
            sys_timer_start_ext(&app_proxr_env.lls_timer, MILD_ALTER_MS, false);
        } else {
            // FIX TODO lls_cnt has race condition
            app_proxr_env.lls_cnt = app_proxr_env.total_time / HIGH_ALTER_MS;
            sys_timer_start_ext(&app_proxr_env.lls_timer, HIGH_ALTER_MS, false);
        }
        app_proxr_env.lls_alert_level = alert_level;
    }

    if (app_proxr_env.lls_alert_level == PROXM_ALERT_NONE &&
        app_proxr_env.path_loss_alert_level == PROXM_ALERT_NONE) {
        app_proxr_env.traced = false;
    }
}

/*!
    \brief      Update path loss alert level value
    \param[in]  peer_addr: peer device address
    \param[in]  alert_level: path loss alert level value
    \param[out] none
    \retval     none
*/
static void app_path_loss_alert_update(ble_gap_addr_t peer_addr, proxm_alert_lvl_t alert_level)
{
    dbg_print(NOTICE, "app_path_loss_alert_update level %d\r\n", alert_level);
    if (!app_proxr_env.traced) {
        app_proxr_env.peer_addr = peer_addr;
        app_proxr_env.traced = true;
    } else if (sys_memcmp(&peer_addr, &app_proxr_env.peer_addr, sizeof(ble_gap_addr_t))) {
        dbg_print(WARNING, "app_path_loss_alert_update not support multiple device!\r\n");
        return;
    }

    if (alert_level != app_proxr_env.path_loss_alert_level) {
        if (alert_level == PROXM_ALERT_NONE) {
            sys_timer_stop(&app_proxr_env.path_loss_timer, false);
        } else if (alert_level == PROXM_ALERT_MILD) {
            // FIX TODO path_loss_cnt has race condition
            app_proxr_env.path_loss_cnt = app_proxr_env.total_time / MILD_ALTER_MS;
            sys_timer_start_ext(&app_proxr_env.path_loss_timer, MILD_ALTER_MS, false);
        } else {
            // FIX TODO path_loss_cnt has race condition
            app_proxr_env.path_loss_cnt = app_proxr_env.total_time / HIGH_ALTER_MS;
            sys_timer_start_ext(&app_proxr_env.path_loss_timer, HIGH_ALTER_MS, false);
        }
        app_proxr_env.path_loss_alert_level = alert_level;
    }

    if (app_proxr_env.lls_alert_level == PROXM_ALERT_NONE &&
        app_proxr_env.path_loss_alert_level == PROXM_ALERT_NONE) {
        app_proxr_env.traced = false;
    }
}

/*!
    \brief      lls timer callback function
    \param[in]  p_tmr: pointer to the timer
    \param[in]  p_arg: args pass back in the callback function
    \param[out] none
    \retval     none
*/
static void lls_timer_callback(void *p_tmr, void *p_arg)
{
    app_proxr_env.lls_cnt--;
    if (app_proxr_env.lls_cnt == 0) {
        sys_timer_stop(&app_proxr_env.lls_timer, false);
    }

    if ((app_proxr_env.lls_cnt % 2) == 0) {
        gpio_bit_reset(GPIOB, LED_LINK_LOSS);
    } else {
        gpio_bit_set(GPIOB, LED_LINK_LOSS);
    }
}

/*!
    \brief      Path loss timer callback function
    \param[in]  p_tmr: pointer to the timer
    \param[in]  p_arg: args pass back in the callback function
    \param[out] none
    \retval     none
*/
static void path_loss_timer_callback(void *p_tmr, void *p_arg)
{
    app_proxr_env.path_loss_cnt--;
    if (app_proxr_env.path_loss_cnt == 0) {
        sys_timer_stop(&app_proxr_env.path_loss_timer, false);
    }

    if ((app_proxr_env.path_loss_cnt % 2) == 0) {
        gpio_bit_reset(GPIOB, LED_PATH_LOSS);
    } else {
        gpio_bit_set(GPIOB, LED_PATH_LOSS);
    }
}

/*!
    \brief      Init proximity reporter application
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_prox_rpt_init(void)
{
    if (ble_proxr_init(proxr_callbacks) == BLE_ERR_NO_ERROR) {
        sys_memset(&app_proxr_env, 0, sizeof(app_proxr_env_t));
        app_proxr_env.total_time = 10000;    // 10s
        sys_timer_init(&app_proxr_env.lls_timer, (const uint8_t *)"link loss timer", MILD_ALTER_MS, 1,
                       lls_timer_callback, NULL);
        sys_timer_init(&app_proxr_env.path_loss_timer, (const uint8_t *)"path loss timer", MILD_ALTER_MS, 1,
                       path_loss_timer_callback, NULL);
        app_proxr_led_config();
    }
}
#endif // (BLE_PROFILE_PROX_SERVER)
