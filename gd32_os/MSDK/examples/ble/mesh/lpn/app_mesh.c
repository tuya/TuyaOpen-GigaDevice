/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*!
    \file    app_mesh.c
    \brief   Implementation of BLE mesh application.

    \version 2024-05-24, V1.0.0, firmware for GD32VW55x
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
#include "app_cfg.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gd32vw55x.h"
#include "gd32vw55x_timer.h"

#include "mesh_cfg.h"
#include "api/mesh.h"
#include "app_mesh.h"
#include "bluetooth/bt_str.h"
#include "wrapper_os.h"
#include "mesh_kernel.h"
#include "dbg_print.h"
#include "app_mesh_cfg.h"
#include "api/settings.h"
#include "nvds_flash.h"

#define VND_MODULE_GPIO                  GPIOA
#define VND_MODULE_PIN                   GPIO_PIN_12
#define VND_MODULE_PIN2                  GPIO_PIN_5

#define MOD_LF 0x0000

#define BT_COMP_ID_LF           0x05f1 /**< The Linux Foundation */

#define OP_VENDOR_BUTTON_PRESSED    BT_MESH_MODEL_OP_3(0x00, BT_COMP_ID_LF)
#define OP_VENDOR_BUTTON_RELEASED   BT_MESH_MODEL_OP_3(0x01, BT_COMP_ID_LF)
#define OP_VENDOR_BUTTON_STATUS     BT_MESH_MODEL_OP_3(0x02, BT_COMP_ID_LF)

static int vnd_button_pressed(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
                              struct net_buf_simple *buf);
static int vnd_button_released(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
                              struct net_buf_simple *buf);
static int vnd_button_pub_update(const struct bt_mesh_model *mod);

static const char *const output_meth_string[] = {
    "Blink",
    "Beep",
    "Vibrate",
    "Display Number",
    "Display String",
};

static const char *const input_meth_string[] = {
    "Push",
    "Twist",
    "Enter Number",
    "Enter String",
};

static uint8_t dev_default_uuid[16] = { 0x4C, 0x50, 0x4E, 0x08, 0x10, 0x21, 0x0B, 0x0E, 0x0A, 0x0C, 0x00, 0x0B, 0x0E, 0x0A, 0x0C, 0x00 };

/* Default net, app & dev key values, unless otherwise specified */
const uint8_t app_mesh_default_net_key[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

const uint8_t app_mesh_default_dev_key[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

const uint8_t app_mesh_default_app_key[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};


#if (CONFIG_BT_MESH_SAR_CFG_CLI)
static struct bt_mesh_sar_cfg_cli sar_cfg_cli;
#endif

#if CONFIG_BT_MESH_LARGE_COMP_DATA_SRV
extern struct bt_mesh_models_metadata_entry health_srv_meta[];
#endif
extern struct bt_mesh_health_srv bt_mesh_shell_health_srv;
BT_MESH_HEALTH_PUB_DEFINE(health_pub, 4);

static const struct bt_mesh_model root_models[] = {
    BT_MESH_MODEL_CFG_SRV,
    BT_MESH_MODEL_CFG_CLI(&app_cfg_cli),
    BT_MESH_MODEL_HEALTH_SRV(&bt_mesh_shell_health_srv, &health_pub, health_srv_meta),
#if (CONFIG_BT_MESH_SAR_CFG_SRV)
    BT_MESH_MODEL_SAR_CFG_SRV,
#endif
#if (CONFIG_BT_MESH_SAR_CFG_CLI)
    BT_MESH_MODEL_SAR_CFG_CLI(&sar_cfg_cli),
#endif

#if (CONFIG_BT_MESH_OP_AGG_SRV)
    BT_MESH_MODEL_OP_AGG_SRV,
#endif
#if (CONFIG_BT_MESH_OP_AGG_CLI)
    BT_MESH_MODEL_OP_AGG_CLI,
#endif

#if (CONFIG_BT_MESH_LARGE_COMP_DATA_SRV)
    BT_MESH_MODEL_LARGE_COMP_DATA_SRV,
#endif

#if (CONFIG_BT_MESH_PRIV_BEACON_SRV)
    BT_MESH_MODEL_PRIV_BEACON_SRV,
#endif
};

static const struct bt_mesh_model models_alt[] = {
};


BT_MESH_MODEL_PUB_DEFINE(vnd_pub, vnd_button_pub_update, 5);


static const struct bt_mesh_model_op vnd_ops[] = {
    { OP_VENDOR_BUTTON_PRESSED, BT_MESH_LEN_EXACT(0), vnd_button_pressed },
    { OP_VENDOR_BUTTON_RELEASED, BT_MESH_LEN_EXACT(0), vnd_button_released },
    BT_MESH_MODEL_OP_END,
};


static const struct bt_mesh_model vnd_models[] = {
    BT_MESH_MODEL_VND_CB(BT_COMP_ID_LF, MOD_LF, vnd_ops, &vnd_pub, NULL, NULL),
};

static const struct bt_mesh_model vnd_models2[] = {
    BT_MESH_MODEL_VND_CB(BT_COMP_ID_LF, MOD_LF, vnd_ops, &vnd_pub, NULL, NULL),
};


static const struct bt_mesh_elem elements[] = {
    BT_MESH_ELEM(0, root_models, vnd_models),
    BT_MESH_ELEM(1, models_alt, vnd_models2),
};

static const struct bt_mesh_comp comp = {
    .cid = 0xFFFF,
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

#if (CONFIG_BT_MESH_COMP_PAGE_2)
static const uint8_t cmp2_elem_offset[1] = {0};

static const struct bt_mesh_comp2_record comp_rec = {
    .id = 0x1600,
    .version.x = 1,
    .version.y = 0,
    .version.z = 0,
    .elem_offset_cnt = 1,
    .elem_offset = cmp2_elem_offset,
    .data_len = 0
};

static const struct bt_mesh_comp2 comp_p2 = {.record_cnt = 1, .record = &comp_rec};
#endif

const char *bearer2str(bt_mesh_prov_bearer_t bearer)
{
    switch (bearer) {
    case BT_MESH_PROV_ADV:
        return "PB-ADV";
    case BT_MESH_PROV_GATT:
        return "PB-GATT";
    case BT_MESH_PROV_REMOTE:
        return "PB-REMOTE";
    default:
        return "unknown";
    }
}

static void config_light_gpio_2_normal(void)
{
    timer_deinit(TIMER1);

    gpio_mode_set(VND_MODULE_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, VND_MODULE_PIN);
    gpio_output_options_set(VND_MODULE_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, VND_MODULE_PIN);
}

static void config_vnd_pwm(uint32_t period)
{
    timer_oc_parameter_struct timer_ocintpara;
    timer_parameter_struct timer_initpara;

    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_12);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_12);
    gpio_af_set(GPIOA, GPIO_AF_9, GPIO_PIN_12);

    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);
    rcu_periph_clock_enable(RCU_TIMER1);

    timer_deinit(TIMER1);

    /* TIMER1 configuration */
    timer_initpara.prescaler         = 159;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = period - 1;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER1, &timer_initpara);

    /* CH1 configuration in PWM mode */
    timer_ocintpara.outputstate  = TIMER_CCX_ENABLE;
    timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
    timer_ocintpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
    timer_ocintpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
    timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;

    timer_channel_output_config(TIMER1,TIMER_CH_2, &timer_ocintpara);

    /* CH1 configuration in PWM mode0,duty cycle 50% */
    timer_channel_output_pulse_value_config(TIMER1,TIMER_CH_2,(period >> 1) -1);
    timer_channel_output_mode_config(TIMER1,TIMER_CH_2,TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER1,TIMER_CH_2,TIMER_OC_SHADOW_DISABLE);

    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER1);
    /* auto-reload preload enable */
    timer_enable(TIMER1);
}

static int vnd_button_pub_update(const struct bt_mesh_model *mod)
{
    struct bt_mesh_model_pub *pub = mod->pub;

    bt_mesh_model_msg_init(pub->msg, OP_VENDOR_BUTTON_STATUS);

    net_buf_simple_add_u8(pub->msg, 1);
    net_buf_simple_add_u8(pub->msg, 2);
    return 0;
}

static int vnd_button_pressed(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
                              struct net_buf_simple *buf)
{
    app_print("vnd_button_pressed src 0x%04x\r\n", ctx->addr);

    if (bt_mesh_model_elem(model)->rt->addr == bt_mesh_model_elem(vnd_models)->rt->addr) {
        gpio_bit_set(VND_MODULE_GPIO, VND_MODULE_PIN);
    }
    else {
        gpio_bit_set(VND_MODULE_GPIO, VND_MODULE_PIN2);
    }

    return 0;
}

static int vnd_button_released(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
                              struct net_buf_simple *buf)
{
    app_print("vnd_button_released src 0x%04x\r\n", ctx->addr);

    if (bt_mesh_model_elem(model)->rt->addr == bt_mesh_model_elem(vnd_models)->rt->addr) {
        gpio_bit_reset(VND_MODULE_GPIO, VND_MODULE_PIN);
    }
    else {
        gpio_bit_reset(VND_MODULE_GPIO, VND_MODULE_PIN2);
    }

    return 0;
}

static void app_mesh_heartbeat_recv(const struct bt_mesh_hb_sub *sub, uint8_t hops, uint16_t feat)
{
    app_print(
        "app_mesh_heartbeat_recv hops %d, feat 0x%04x:\r\n" \
        "\tSubscription period:            %us\r\n"            \
        "\tRemaining subscription time:    %us\r\n"            \
        "\tSource address:                 0x%04x\r\n"                \
        "\tDestination address:            0x%04x\r\n"                \
        "\tNumber Heartbeat messages:      %u\r\n"            \
        "\tMinimum hops:                   %u\r\n"                \
        "\tMaximum hops:                   %u\r\n",
        hops, feat, sub->period, sub->remaining, sub->src, sub->dst, sub->count,
        sub->min_hops, sub->max_hops);
}

static void app_mesh_heartbeat_sub_end(const struct bt_mesh_hb_sub *sub)
{
    app_print("app_mesh_heartbeat_sub_end src 0x%04x, dst 0x%04x\r\n", sub->src, sub->dst);
}

static void app_mesh_heartbeat_pub_sent(const struct bt_mesh_hb_pub *pub)
{
    app_print(
        "app_mesh_heartbeat_pub_sent:\r\n"                     \
        "\tDestination address:            0x%04x\r\n"         \
        "\tRemaining publish count:        %u\r\n"            \
        "\tTime To Live value:             %u\r\n"            \
        "\tFeatures:                       0x%04x\r\n"          \
        "\tNumber Heartbeat messages:      %u\r\n"              \
        "\tNetwork index:                  %u\r\n"              \
        "\tPublication period:             %us\r\n",
        pub->dst, pub->count, pub->ttl, pub->feat, pub->count, pub->net_idx, pub->period);
}

#if (CONFIG_MESH_CB_REGISTERED)
static struct bt_mesh_hb_cb heartbeat_cb = {
    .recv = app_mesh_heartbeat_recv,
    .sub_end = app_mesh_heartbeat_sub_end,
    .pub_sent = app_mesh_heartbeat_pub_sent,
    .next = NULL,
};
#else
BT_MESH_HB_CB_DEFINE(heartbeat_cb) = {
    .recv = app_mesh_heartbeat_recv,
    .sub_end = app_mesh_heartbeat_sub_end,
    .pub_sent = app_mesh_heartbeat_pub_sent,
};
#endif

#if (CONFIG_BT_MESH_LOW_POWER)
static void lpn_established(uint16_t net_idx, uint16_t friend_addr, uint8_t queue_size, uint8_t recv_win)
{
    app_print("Friendship (as LPN) established to Friend 0x%04x Queue Size %d Receive Window %d\r\n",
        friend_addr, queue_size, recv_win);
    config_light_gpio_2_normal();
    gpio_bit_set(VND_MODULE_GPIO, VND_MODULE_PIN2);
    sys_ms_sleep(1500);
    gpio_bit_reset(VND_MODULE_GPIO, VND_MODULE_PIN2);
}

static void lpn_terminated(uint16_t net_idx, uint16_t friend_addr)
{
    app_print("Friendship (as LPN) lost with Friend 0x%04x\r\n", friend_addr);

    gpio_bit_reset(VND_MODULE_GPIO, VND_MODULE_PIN);
    gpio_bit_reset(VND_MODULE_GPIO, VND_MODULE_PIN2);
    config_vnd_pwm(2000000);
}

#if (CONFIG_MESH_CB_REGISTERED)
static struct bt_mesh_lpn_cb lpn_cb = {
    .established = lpn_established,
    .terminated = lpn_terminated,
};

#else
BT_MESH_LPN_CB_DEFINE(lpn_cb) = {
    .established = lpn_established,
    .terminated = lpn_terminated,
};
#endif
#endif

static void app_mesh_prov_link_open(bt_mesh_prov_bearer_t bearer)
{
    app_print("Provisioning link opened on %s\r\n", bearer2str(bearer));
}

void app_mesh_prov_link_close(bt_mesh_prov_bearer_t bearer)
{
    app_print("Provisioning link closed on %s\r\n", bearer2str(bearer));
}

static void app_mesh_prov_complete(uint16_t net_idx, uint16_t addr)
{
    config_vnd_pwm(2000000);

    app_print("######## Provision complete net_idx %d, addr 0x%04x ######\r\n", net_idx, addr);
}

static int app_mesh_prov_output_number(bt_mesh_output_action_t action, uint32_t number)
{
    switch (action) {
    case BT_MESH_BLINK:
        app_print("OOB blink Number: %u\r\n", number);
        break;
    case BT_MESH_BEEP:
        app_print("OOB beep Number: %u\r\n", number);
        break;
    case BT_MESH_VIBRATE:
        app_print("OOB vibrate Number: %u\r\n", number);
        break;
    case BT_MESH_DISPLAY_NUMBER:
        app_print("OOB display Number: %u\r\n", number);
        break;
    default:
        app_print("Unknown Output action %u (number %u) requested!\r\n", action, number);
        return -EINVAL;
    }

    return 0;
}

static int app_mesh_prov_output_string(const char *str)
{
    app_print("OOB String: %s\r\n", str);
    return 0;
}

static int app_mesh_prov_input(bt_mesh_input_action_t act, uint8_t size)
{
    switch (act) {
    case BT_MESH_ENTER_NUMBER:
        app_print("Enter a number (max %u digits) with: Input-num <num>\r\n", size);
        break;
    case BT_MESH_ENTER_STRING:
        app_print("Enter a string (max %u chars) with: Input-str <str>\r\n", size);
        break;
    case BT_MESH_TWIST:
        app_print("\"Twist\" a number (max %u digits) with: Input-num <num>\r\n", size);
        break;
    case BT_MESH_PUSH:
        app_print("\"Push\" a number (max %u digits) with: Input-num <num>\r\n", size);
        break;
    default:
        app_print("Unknown Input action %u (size %u) requested!\r\n", act, size);
        return -EINVAL;
    }

    return 0;
}


static void app_mesh_prov_input_complete(void)
{
    app_print("Provison Input complete\r\n");
}


static const struct bt_mesh_prov prov = {
    .uuid = dev_default_uuid,
    .link_open = app_mesh_prov_link_open,
    .link_close = app_mesh_prov_link_close,
    .complete = app_mesh_prov_complete,

    .static_val = NULL,
    .static_val_len = 0,
    .output_size = 6,
    .output_actions = (BT_MESH_BLINK | BT_MESH_BEEP | BT_MESH_VIBRATE | BT_MESH_DISPLAY_NUMBER |
                       BT_MESH_DISPLAY_STRING),
    .input_size = 6,
    .input_actions = (BT_MESH_ENTER_NUMBER | BT_MESH_ENTER_STRING | BT_MESH_TWIST | BT_MESH_PUSH),

    .output_number = app_mesh_prov_output_number,
    .output_string = app_mesh_prov_output_string,
    .input = app_mesh_prov_input,
    .input_complete = app_mesh_prov_input_complete,
};

void app_mesh_provision_local(uint16_t net_idx, uint32_t iv_idx, uint16_t addr)
{
    int err = 0;

    err = bt_mesh_provision(app_mesh_default_net_key, net_idx, 0, iv_idx, addr,
                            app_mesh_default_dev_key);
    if (err) {
        app_print("provision local fail, err:%d\r\n", err);
        return;
    }

    app_print("provision local success, net_idx: %u, iv_idx: %u, addr: 0x%04x\r\n", net_idx, iv_idx,
              addr);
}

void app_mesh_set_dev_uuid_prop(uint8_t uuid[16])
{
    int err;
    uint8_t dev_uuid[16] = {0};
    uint32_t uuid_len = 16;
    err = nvds_data_get(NULL, MESH_NAME_SPACE, "DEV_UUID", dev_uuid, (uint32_t *)&uuid_len);

    if (err == NVDS_OK) {
        app_print("Get device uuid from storage %s\r\n", bt_hex(dev_uuid, uuid_len));
    }

    app_print("Set new device uuid to storage %s\r\n", bt_hex(uuid, uuid_len));
#if (CONFIG_BT_SETTINGS)
    err = settings_save_one("DEV_UUID", uuid, uuid_len);
#endif
    if (err != NVDS_OK) {
        app_print("Set new device uuid fail\r\n");
        return;
    }

    sys_memcpy(dev_default_uuid, uuid, uuid_len);
}

void app_mesh_init(void)
{
    int err;
    uint8_t dev_uuid[16] = {0};
    uint32_t uuid_len = 16;

    extern void ble_mesh_cli_init(void);
    ble_mesh_cli_init();

#if (CONFIG_BT_MESH_LOW_POWER && CONFIG_MESH_CB_REGISTERED)
    bt_mesh_lpn_cb_register(&lpn_cb);
#endif

    mesh_kernel_init();

    err = nvds_data_get(NULL, MESH_NAME_SPACE, "DEV_UUID", dev_uuid, (uint32_t *)&uuid_len);

    if (err == NVDS_OK) {
        app_print("Get device uuid from storage %s\r\n", bt_hex(dev_uuid, uuid_len));
        sys_memcpy(dev_default_uuid, dev_uuid, uuid_len);
    } else {
        sys_random_bytes_get(dev_default_uuid, uuid_len);
        app_print("First init mesh, get random device uuid %s\r\n", bt_hex(dev_default_uuid, uuid_len));
#if (CONFIG_BT_SETTINGS)
        settings_save_one("DEV_UUID", dev_default_uuid, uuid_len);
#endif
    }

    err = bt_mesh_init(&prov, &comp);

    if (err) {
        app_print("mesh init fail, err:%d\r\n", err);
        return;
    }

#if defined(CONFIG_BT_MESH_COMP_PAGE_2)
    bt_mesh_comp2_register(&comp_p2);
#endif

#if (CONFIG_MESH_CB_REGISTERED)
    bt_mesh_hearbeat_cb_register(&heartbeat_cb);
#endif

    rcu_periph_clock_enable(RCU_GPIOA);
    gpio_mode_set(VND_MODULE_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, VND_MODULE_PIN2);
    gpio_output_options_set(VND_MODULE_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, VND_MODULE_PIN2);

		if (!bt_mesh_is_provisioned()) {
        app_print("LPN device is not provisioned\r\n");
        err = bt_mesh_prov_enable(BT_MESH_PROV_ADV);
        if (err) {
            app_print("Failed to enable unporvision beacon (err %d)", err);
        }
        config_vnd_pwm(1000000);
		}

    app_print("mesh init success, uuid: %s\r\n", bt_hex(prov.uuid, 16));
}

