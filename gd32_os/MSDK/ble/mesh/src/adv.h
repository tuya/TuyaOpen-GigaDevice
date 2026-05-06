/*!
    \file    adv.h
    \brief   Implementation of BLE mesh advertising adapter include header.

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

#ifndef MESH_SUBSYS_BLUETOOTH_MESH_ADV_H_
#define MESH_SUBSYS_BLUETOOTH_MESH_ADV_H_

/* Maximum advertising data payload for a single data type */
#define BT_MESH_ADV_DATA_SIZE 29

enum bt_mesh_adv_type {
    BT_MESH_ADV_PROV,
    BT_MESH_ADV_DATA,
    BT_MESH_ADV_BEACON,
    BT_MESH_ADV_URI,

    BT_MESH_ADV_TYPES,
};

enum bt_mesh_adv_tag {
    BT_MESH_ADV_TAG_LOCAL,
    BT_MESH_ADV_TAG_RELAY,
    BT_MESH_ADV_TAG_PROXY,
    BT_MESH_ADV_TAG_FRIEND,
    BT_MESH_ADV_TAG_PROV,
};

enum bt_mesh_adv_tag_bit {
    BT_MESH_ADV_TAG_BIT_LOCAL   = BIT(BT_MESH_ADV_TAG_LOCAL),
    BT_MESH_ADV_TAG_BIT_RELAY   = BIT(BT_MESH_ADV_TAG_RELAY),
    BT_MESH_ADV_TAG_BIT_PROXY   = BIT(BT_MESH_ADV_TAG_PROXY),
    BT_MESH_ADV_TAG_BIT_FRIEND  = BIT(BT_MESH_ADV_TAG_FRIEND),
    BT_MESH_ADV_TAG_BIT_PROV    = BIT(BT_MESH_ADV_TAG_PROV),
};

struct bt_mesh_adv_ctx {
    const struct bt_mesh_send_cb *cb;
    void *cb_data;

    uint8_t type:2,
            started:1,
            busy:1,
            tag:4;

    uint8_t priv:1;
    uint8_t xmit;
};

struct bt_mesh_adv {
    sys_snode_t node;

    struct bt_mesh_adv_ctx ctx;

    struct net_buf_simple b;

    uint8_t __ref;

    uint8_t __bufs[BT_MESH_ADV_DATA_SIZE];
};

struct ble_mesh_adv_param_t
{
    /** Own address type, @ref ble_gap_local_addr_type_t */
    uint8_t  own_addr_type;

    /** Advertising properties. @ref ble_gap_legacy_adv_prop_t for legacy advertising */
    uint16_t prop;

    /** Minimum Advertising Interval (N * 0.625) */
    uint16_t interval_min;

    /** Maximum Advertising Interval (N * 0.625) */
    uint16_t interval_max;

    /** Number of advertising events. Set to zero for no limit. */
    uint8_t  max_adv_evt;

    /** Number of advertising events. Set to zero for no limit. unit: ms*/
    uint16_t timeout;
};

struct bt_mesh_adv *bt_mesh_adv_ref(struct bt_mesh_adv *adv);

void bt_mesh_adv_unref(struct bt_mesh_adv *adv);

/* xmit_count: Number of retransmissions, i.e. 0 == 1 transmission */
struct bt_mesh_adv *bt_mesh_adv_create(enum bt_mesh_adv_type type, enum bt_mesh_adv_tag tag, uint8_t xmit,
                                            k_timeout_t timeout);

/** @brief Send an advertising message.
 *
 * Send an advertising message out into the mesh adv bearer layer.
 *
 * @param adv The advertising context.
 * @param cb Callback function.
 * @param cb_data Callback data.
 */
void bt_mesh_adv_send(struct bt_mesh_adv *adv, const struct bt_mesh_send_cb *cb, void *cb_data);

/** @brief After updating the parameters of the connectable advertising, call this function to restart.
 */
void bt_mesh_adv_gatt_update(void);

int bt_mesh_adv_init(void);

/** @brief Enable the adv bearer layer.
 *
 * @return 0 on success or negative error code on failure.
 */
int bt_mesh_adv_enable(void);

/** @brief Disable the adv bearer layer.
 *
 * @return 0 on success or negative error code on failure.
 */
int bt_mesh_adv_disable(void);

/** @brief Terminate the parameter advertising while it is playing.
 *
 * @param adv The advertising context.
 *
 * @return 0 on success or negative error code on failure.
 */
int bt_mesh_adv_terminate(struct bt_mesh_adv *adv);

/** @brief This function is used by proxy_srv and pb-gatt_srv to send connectable advertising.
 *
 * @return 0 on success or negative error code on failure.
 */
int bt_mesh_adv_gatt_start(struct ble_mesh_adv_param_t *param, const struct bt_data *ad, size_t ad_len,
                                 const struct bt_data *sd, size_t sd_len);

/** @brief Send an advertising message start callback.
 */
void bt_mesh_adv_send_start(uint16_t duration, int err, struct bt_mesh_adv_ctx *ctx);

/** @brief This function is used to send special advertising, such as solicitation.
 *
 * @return 0 on success or negative error code on failure.
 */
int bt_mesh_adv_bt_data_send(uint8_t num_events, uint16_t adv_interval, const struct bt_data *ad, size_t ad_len);

#endif /* MESH_SUBSYS_BLUETOOTH_MESH_ADV_H_ */

