/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mesh_cfg.h"
#include "mesh_kernel.h"
#include "sys/byteorder.h"
#include "net/buf.h"
#include "bluetooth/mesh_bluetooth.h"
#include "api/mesh.h"
#include "mesh_util.h"

#include "bluetooth/bt_str.h"

#include "ble_gap.h"
#include "ble_gatts.h"

#include "mesh.h"
#include "net.h"
#include "rpl.h"
#include "transport.h"
#include "prov.h"
#include "beacon.h"
#include "foundation.h"
#include "access.h"
#include "proxy.h"
#include "proxy_msg.h"
#include "crypto.h"
#include "scan.h"

#define LOG_LEVEL CONFIG_BT_MESH_PROXY_LOG_LEVEL
#include "api/mesh_log.h"

#if (CONFIG_BT_MESH_GATT_PROXY)
#define PROXY_SVC_INIT_TIMEOUT K_MSEC(10)
#define PROXY_SVC_REG_ATTEMPTS 5

/* Interval to update random value in (10 minutes).
 *
 * Defined in the Bluetooth Mesh Specification v1.1, Section 7.2.2.2.4.
 */
#define PROXY_RANDOM_UPDATE_INTERVAL (10 * 60 * MSEC_PER_SEC)

#define ADV_OPT_ADDR(private) (IS_ENABLED(CONFIG_BT_MESH_DEBUG_USE_ID_ADDR) ?                      \
                BLE_GAP_LOCAL_ADDR_STATIC : (private) ? BLE_GAP_LOCAL_ADDR_RESOLVABLE : BLE_GAP_LOCAL_ADDR_STATIC)

static void proxy_send_beacons(struct k_work *work);
static int proxy_send(uint8_t conn_idx, void *data, uint16_t len, bt_gatt_complete_func_t end, void *user_data);
static void bt_mesh_proxy_srv_conn_evt_handler(ble_conn_evt_t event, ble_conn_data_u *p_data);

static struct bt_mesh_proxy_client {
	struct bt_mesh_proxy_role *cli;
	uint16_t filter[CONFIG_BT_MESH_PROXY_FILTER_SIZE];
	enum __packed {
		TYPE_NONE,
		ACCEPT,
		REJECT,
	} filter_type;
	struct k_work send_beacons;
#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	bool privacy;
#endif
	uint16_t proxy_cccd;
} clients[CONFIG_BT_MAX_CONN] = {
	[0 ... (CONFIG_BT_MAX_CONN - 1)] = {
		.send_beacons = Z_WORK_INITIALIZER(proxy_send_beacons),
	},
};

static bool service_registered;

#if (CONFIG_MESH_CB_REGISTERED)
static struct bt_mesh_proxy_cb *p_proxy_list = NULL;
#else
extern uint32_t _proxy_cbs[];
extern uint32_t _eproxy_cbs[];
#endif

static struct bt_mesh_proxy_client *find_client(uint8_t conn_idx)
{
    for (int i = 0; i < ARRAY_SIZE(clients); i++) {
        if (clients[i].cli != NULL && clients[i].cli->conn_idx == conn_idx) {
            return &clients[i];
        }
    }

    return NULL;
}

static struct bt_mesh_proxy_client *alloc_client(void)
{
    for (int i = 0; i < ARRAY_SIZE(clients); i++) {
        if (clients[i].cli == NULL) {
            return &clients[i];
        }
    }

    return NULL;
}

static int gatt_recv(uint8_t conn_idx, const void *buf, uint16_t len)
{
	const uint8_t *data = buf;

	if (len < 1) {
		LOG_WRN("Too small Proxy PDU");
		return -EINVAL;
	}

	if (PDU_TYPE(data) == BT_MESH_PROXY_PROV) {
		LOG_WRN("Proxy PDU type doesn't match GATT service");
		return -EINVAL;
	}

	return bt_mesh_proxy_msg_recv(conn_idx, buf, len);
}

/* Next subnet in queue to be advertised */
static struct bt_mesh_subnet *beacon_sub;

static int filter_set(struct bt_mesh_proxy_client *client,
		      struct net_buf_simple *buf)
{
	uint8_t type;

	if (buf->len < 1) {
		LOG_WRN("Too short Filter Set message");
		return -EINVAL;
	}

	type = net_buf_simple_pull_u8(buf);
	LOG_DBG("type 0x%02x", type);

	switch (type) {
	case 0x00:
		(void)memset(client->filter, 0, sizeof(client->filter));
		client->filter_type = ACCEPT;
		break;
	case 0x01:
		(void)memset(client->filter, 0, sizeof(client->filter));
		client->filter_type = REJECT;
		break;
	default:
		LOG_WRN("Prohibited Filter Type 0x%02x", type);
		return -EINVAL;
	}

	return 0;
}

static void filter_add(struct bt_mesh_proxy_client *client, uint16_t addr)
{
	int i;

	LOG_DBG("addr 0x%04x", addr);

	if (addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
		if (client->filter[i] == addr) {
			return;
		}
	}

	for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
		if (client->filter[i] == BT_MESH_ADDR_UNASSIGNED) {
			client->filter[i] = addr;
			return;
		}
	}
}

static void filter_remove(struct bt_mesh_proxy_client *client, uint16_t addr)
{
	int i;

	LOG_DBG("addr 0x%04x", addr);

	if (addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
		if (client->filter[i] == addr) {
			client->filter[i] = BT_MESH_ADDR_UNASSIGNED;
			return;
		}
	}
}

static void send_filter_status(struct bt_mesh_proxy_client *client,
			       struct bt_mesh_net_rx *rx,
			       struct net_buf_simple *buf)
{
	struct bt_mesh_net_tx tx = {
		.sub = rx->sub,
		.ctx = &rx->ctx,
		.src = bt_mesh_primary_addr(),
	};
	uint16_t filter_size;
	int i, err;

	/* Configuration messages always have dst unassigned */
	tx.ctx->addr = BT_MESH_ADDR_UNASSIGNED;

	net_buf_simple_reset(buf);
	net_buf_simple_reserve(buf, 10);

	net_buf_simple_add_u8(buf, CFG_FILTER_STATUS);

	if (client->filter_type == ACCEPT) {
		net_buf_simple_add_u8(buf, 0x00);
	} else {
		net_buf_simple_add_u8(buf, 0x01);
	}

	for (filter_size = 0U, i = 0; i < ARRAY_SIZE(client->filter); i++) {
		if (client->filter[i] != BT_MESH_ADDR_UNASSIGNED) {
			filter_size++;
		}
	}

	net_buf_simple_add_be16(buf, filter_size);

	LOG_DBG("%u bytes: %s", buf->len, bt_hex(buf->data, buf->len));

	err = bt_mesh_net_encode(&tx, buf, BT_MESH_NONCE_PROXY);
	if (err) {
		LOG_ERR("Encoding Proxy cfg message failed (err %d)", err);
		return;
	}

	err = bt_mesh_proxy_msg_send(client->cli->conn_idx, BT_MESH_PROXY_CONFIG,
				     buf, NULL, NULL);
	if (err) {
		LOG_ERR("Failed to send proxy cfg message (err %d)", err);
	}
}

static void proxy_filter_recv(uint8_t conn_idx,
			      struct bt_mesh_net_rx *rx, struct net_buf_simple *buf)
{
	struct bt_mesh_proxy_client *client;
	uint8_t opcode;

	client = find_client(conn_idx);
	if (client == NULL) {
		LOG_ERR("find client fail. conn_idx %d", conn_idx);
		return;
	}

	opcode = net_buf_simple_pull_u8(buf);
	switch (opcode) {
	case CFG_FILTER_SET:
		filter_set(client, buf);
		send_filter_status(client, rx, buf);
		break;
	case CFG_FILTER_ADD:
		while (buf->len >= 2) {
			uint16_t addr;

			addr = net_buf_simple_pull_be16(buf);
			filter_add(client, addr);
		}
		send_filter_status(client, rx, buf);
		break;
	case CFG_FILTER_REMOVE:
		while (buf->len >= 2) {
			uint16_t addr;

			addr = net_buf_simple_pull_be16(buf);
			filter_remove(client, addr);
		}
		send_filter_status(client, rx, buf);
		break;
	default:
		LOG_WRN("Unhandled configuration OpCode 0x%02x", opcode);
		break;
	}
}

static void proxy_cfg(struct bt_mesh_proxy_role *role)
{
	NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_NET_MAX_PDU_LEN);
	struct bt_mesh_net_rx rx;
	int err;

	err = bt_mesh_net_decode(&role->buf, BT_MESH_NET_IF_PROXY_CFG,
				 &rx, &buf);
	if (err) {
		LOG_ERR("Failed to decode Proxy Configuration (err %d)", err);
		return;
	}

	rx.local_match = 1U;

	if (bt_mesh_rpl_check(&rx, NULL, false)) {
		LOG_WRN("Replay: src 0x%04x dst 0x%04x seq 0x%06x", rx.ctx.addr, rx.ctx.recv_dst,
			rx.seq);
		return;
	}

	/* Remove network headers */
	net_buf_simple_pull(&buf, BT_MESH_NET_HDR_LEN);

	LOG_DBG("%u bytes: %s", buf.len, bt_hex(buf.data, buf.len));

	if (buf.len < 1) {
		LOG_WRN("Too short proxy configuration PDU");
		return;
	}

	proxy_filter_recv(role->conn_idx, &rx, &buf);
}

static void proxy_msg_recv(struct bt_mesh_proxy_role *role)
{
	switch (role->msg_type) {
	case BT_MESH_PROXY_NET_PDU:
		LOG_DBG("Mesh Network PDU");
		bt_mesh_net_recv(&role->buf, 0, BT_MESH_NET_IF_PROXY);
		break;
	case BT_MESH_PROXY_BEACON:
		LOG_DBG("Mesh Beacon PDU");
		bt_mesh_beacon_recv(&role->buf);
		break;
	case BT_MESH_PROXY_CONFIG:
		LOG_DBG("Mesh Configuration PDU");
		proxy_cfg(role);
		break;
	default:
		LOG_WRN("Unhandled Message Type 0x%02x", role->msg_type);
		break;
	}
}

static int beacon_send(struct bt_mesh_proxy_client *client,
		       struct bt_mesh_subnet *sub)
{
	int err;

	NET_BUF_SIMPLE_DEFINE(buf, 28);

	net_buf_simple_reserve(&buf, 1);

#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	err = bt_mesh_beacon_create(sub, &buf, client->privacy);
#else
	err = bt_mesh_beacon_create(sub, &buf, false);
#endif
	if (err) {
		return err;
	}

	return bt_mesh_proxy_msg_send(client->cli->conn_idx, BT_MESH_PROXY_BEACON,
				      &buf, NULL, NULL);
}

static bool send_beacon_cb(struct bt_mesh_subnet *sub, void *cb_data)
{
	struct bt_mesh_proxy_client *client = cb_data;

	return beacon_send(client, sub) != 0;
}

static void proxy_send_beacons(struct k_work *work)
{
	struct bt_mesh_proxy_client *client;

	client = CONTAINER_OF(work, struct bt_mesh_proxy_client, send_beacons);

	(void)bt_mesh_subnet_find(send_beacon_cb, client);
}

void bt_mesh_proxy_beacon_send(struct bt_mesh_subnet *sub)
{
	int i;

	if (!sub) {
		/* NULL means we send on all subnets */
		bt_mesh_subnet_foreach(bt_mesh_proxy_beacon_send);
		return;
	}

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		if (clients[i].cli) {
			beacon_send(&clients[i], sub);
		}
	}
}

static void identity_enabled(struct bt_mesh_subnet *sub)
{
	sub->node_id = BT_MESH_NODE_IDENTITY_RUNNING;
	sub->node_id_start = k_uptime_get_32();

#if (CONFIG_MESH_CB_REGISTERED)
	struct bt_mesh_proxy_cb *p_cur_proxy_cb = p_proxy_list;
	while(p_cur_proxy_cb) {
		p_cur_proxy_cb->identity_enabled(sub->net_idx);
		p_cur_proxy_cb = p_cur_proxy_cb->next;
	}
#else
	struct bt_mesh_proxy_cb *p_cur_proxy_cb = (struct bt_mesh_proxy_cb *)_proxy_cbs;
	while(p_cur_proxy_cb != (struct bt_mesh_proxy_cb *)_eproxy_cbs) {
		p_cur_proxy_cb->identity_enabled(sub->net_idx);
		p_cur_proxy_cb++;
	}
#endif
}

static void node_id_start(struct bt_mesh_subnet *sub)
{
#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	sub->priv_beacon_ctx.node_id = false;
#endif

	identity_enabled(sub);
}

static void private_node_id_start(struct bt_mesh_subnet *sub)
{
#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	sub->priv_beacon_ctx.node_id = true;
#endif

	identity_enabled(sub);
}

void bt_mesh_proxy_identity_start(struct bt_mesh_subnet *sub, bool private)
{
	if (private) {
		private_node_id_start(sub);
	} else {
		node_id_start(sub);
	}

	/* Prioritize the recently enabled subnet */
	beacon_sub = sub;
}

void bt_mesh_proxy_identity_stop(struct bt_mesh_subnet *sub)
{
	sub->node_id = BT_MESH_NODE_IDENTITY_STOPPED;
	sub->node_id_start = 0U;

	/*
	STRUCT_SECTION_FOREACH(bt_mesh_proxy_cb, cb) {
		if (cb->identity_disabled) {
			cb->identity_disabled(sub->net_idx);
		}
	}
	*/
#if (CONFIG_MESH_CB_REGISTERED)
	struct bt_mesh_proxy_cb *p_cur_proxy_cb = p_proxy_list;
	while(p_cur_proxy_cb) {
		p_cur_proxy_cb->identity_disabled(sub->net_idx);
		p_cur_proxy_cb = p_cur_proxy_cb->next;
	}
#else
	struct bt_mesh_proxy_cb *p_cur_proxy_cb = (struct bt_mesh_proxy_cb *)_proxy_cbs;
	while(p_cur_proxy_cb != (struct bt_mesh_proxy_cb *)_eproxy_cbs) {
		p_cur_proxy_cb->identity_disabled(sub->net_idx);
		p_cur_proxy_cb++;
	}
#endif
}

int bt_mesh_proxy_identity_enable(void)
{
	LOG_DBG("");

	if (!bt_mesh_is_provisioned()) {
		return -EAGAIN;
	}

	if (bt_mesh_subnet_foreach(node_id_start)) {
		bt_mesh_adv_gatt_update();
	}

	return 0;
}

int bt_mesh_proxy_private_identity_enable(void)
{
	LOG_DBG("");

	if (!IS_ENABLED(CONFIG_BT_MESH_PRIV_BEACONS)) {
		return -ENOTSUP;
	}

	if (!bt_mesh_is_provisioned()) {
		return -EAGAIN;
	}

	if (bt_mesh_subnet_foreach(private_node_id_start)) {
		bt_mesh_adv_gatt_update();
	}

	return 0;
}

#if (CONFIG_MESH_CB_REGISTERED)
void bt_mesh_proxy_cb_register(struct bt_mesh_proxy_cb *p_proxy_cb)
{
	struct bt_mesh_proxy_cb  *p_cur_proxy_cb = NULL;
	if (p_proxy_cb) {
		if (p_proxy_list == NULL) {
			p_proxy_list = p_proxy_cb;
			p_proxy_cb->next = NULL;
		}
		else {
			p_cur_proxy_cb = p_proxy_list;
			while(p_cur_proxy_cb->next != NULL) {
				p_cur_proxy_cb = p_cur_proxy_cb->next;
			}
			p_cur_proxy_cb->next = p_proxy_cb;
			p_proxy_cb->next = NULL;
		}
	}
}
#endif

#define ENC_ID_LEN                      19
#define NET_ID_LEN                      11

#define NODE_ID_TIMEOUT                 (CONFIG_BT_MESH_NODE_ID_TIMEOUT * MSEC_PER_SEC)

static uint8_t proxy_svc_data[ENC_ID_LEN] = {
    BT_UUID_16_ENCODE(BT_UUID_MESH_PROXY_VAL),
};

static const struct bt_data enc_id_ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_MESH_PROXY_VAL)),
    BT_DATA(BT_DATA_SVC_DATA16, proxy_svc_data, ENC_ID_LEN),
};

static const struct bt_data net_id_ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_MESH_PROXY_VAL)),
    BT_DATA(BT_DATA_SVC_DATA16, proxy_svc_data, NET_ID_LEN),
};

static const struct bt_data sd[] = {
#if defined(CONFIG_BT_MESH_PROXY_USE_DEVICE_NAME)
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
#endif
};

static int enc_id_adv(struct bt_mesh_subnet *sub, uint8_t type, uint8_t hash[16], int32_t duration)
{
    struct ble_mesh_adv_param_t slow_adv_param = {
        .own_addr_type = ADV_OPT_ADDR(type == BT_MESH_ID_TYPE_PRIV_NET || type == BT_MESH_ID_TYPE_PRIV_NODE),
        .prop = BLE_GAP_ADV_PROP_UNDIR_CONN,
        .timeout = duration > 0 ? duration : 0,
        ADV_SLOW_INT,
    };
    struct ble_mesh_adv_param_t fast_adv_param = {
        .own_addr_type = ADV_OPT_ADDR(type == BT_MESH_ID_TYPE_PRIV_NET || type == BT_MESH_ID_TYPE_PRIV_NODE),
        .prop = BLE_GAP_ADV_PROP_UNDIR_CONN,
        .timeout = duration > 0 ? duration : 0,
        ADV_FAST_INT,
    };
    int err;

    err = bt_mesh_encrypt(&sub->keys[SUBNET_KEY_TX_IDX(sub)].identity, hash, hash);
    if (err) {
        return err;
    }

    proxy_svc_data[2] = type;
    memcpy(&proxy_svc_data[3], &hash[8], 8);

    err = bt_mesh_adv_gatt_start(type == BT_MESH_ID_TYPE_PRIV_NET ? &slow_adv_param : &fast_adv_param,
                                 enc_id_ad, ARRAY_SIZE(enc_id_ad), sd, ARRAY_SIZE(sd));
    if (err) {
        LOG_WRN("Failed to advertise using type 0x%02x (err %d)", type, err);
        return err;
    }

    return 0;
}

static int node_id_adv(struct bt_mesh_subnet *sub, int32_t duration)
{
	uint8_t *random = &proxy_svc_data[11];
	uint8_t tmp[16];
	int err;

	LOG_DBG("0x%03x", sub->net_idx);

	err = bt_rand(random, 8);
	if (err) {
		return err;
	}

	memset(&tmp[0], 0x00, 6);
	memcpy(&tmp[6], random, 8);
	sys_put_be16(bt_mesh_primary_addr(), &tmp[14]);

	return enc_id_adv(sub, BT_MESH_ID_TYPE_NODE, tmp, duration);
}

static int priv_node_id_adv(struct bt_mesh_subnet *sub, int32_t duration)
{
	uint8_t *random = &proxy_svc_data[11];
	uint8_t tmp[16];
	int err;

	LOG_DBG("0x%03x", sub->net_idx);

	err = bt_rand(random, 8);
	if (err) {
		return err;
	}

	memset(&tmp[0], 0x00, 5);
	tmp[5] = 0x03;
	memcpy(&tmp[6], random, 8);
	sys_put_be16(bt_mesh_primary_addr(), &tmp[14]);

	return enc_id_adv(sub, BT_MESH_ID_TYPE_PRIV_NODE, tmp, duration);
}

static int priv_net_id_adv(struct bt_mesh_subnet *sub, int32_t duration)
{
	uint8_t *random = &proxy_svc_data[11];
	uint8_t tmp[16];
	int err;

	LOG_DBG("0x%03x", sub->net_idx);

	err = bt_rand(random, 8);
	if (err) {
		return err;
	}

	memcpy(&tmp[0], sub->keys[SUBNET_KEY_TX_IDX(sub)].net_id, 8);
	memcpy(&tmp[8], random, 8);

	return enc_id_adv(sub, BT_MESH_ID_TYPE_PRIV_NET, tmp, duration);
}

static int net_id_adv(struct bt_mesh_subnet *sub, int32_t duration)
{
	struct ble_mesh_adv_param_t slow_adv_param = {
		.own_addr_type = ADV_OPT_ADDR(false),
		.prop = BLE_GAP_ADV_PROP_UNDIR_CONN,
		.timeout = duration > 0 ? duration : 0,
		ADV_SLOW_INT,
	};
	int err;

	proxy_svc_data[2] = BT_MESH_ID_TYPE_NET;

	LOG_DBG("Advertising with NetId %s", bt_hex(sub->keys[SUBNET_KEY_TX_IDX(sub)].net_id, 8));

	memcpy(proxy_svc_data + 3, sub->keys[SUBNET_KEY_TX_IDX(sub)].net_id, 8);

	err = bt_mesh_adv_gatt_start(&slow_adv_param, net_id_ad,
				     ARRAY_SIZE(net_id_ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_WRN("Failed to advertise using Network ID (err %d)", err);
		return err;
	}

	return 0;
}

static bool is_sub_proxy_active(struct bt_mesh_subnet *sub)
{
	if (sub->net_idx == BT_MESH_KEY_UNUSED) {
		return false;
	}

	return (sub->node_id == BT_MESH_NODE_IDENTITY_RUNNING ||
#if defined(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV)
		(bt_mesh_od_priv_proxy_get() > 0 && sub->solicited) ||
#endif
		bt_mesh_gatt_proxy_get() == BT_MESH_GATT_PROXY_ENABLED ||
		bt_mesh_priv_gatt_proxy_get() == BT_MESH_GATT_PROXY_ENABLED);
}

static bool active_proxy_sub_cnt_cb(struct bt_mesh_subnet *sub, void *cb_data)
{
	int *cnt = cb_data;

	if (is_sub_proxy_active(sub)) {
		(*cnt)++;
	}

	/* Don't stop until we've visited all subnets.
	 * We're only using the "find" variant of the subnet iteration to get a context parameter.
	 */
	return false;
}

static int active_proxy_sub_cnt_get(void)
{
	int cnt = 0;

	(void)bt_mesh_subnet_find(active_proxy_sub_cnt_cb, &cnt);

	return cnt;
}

static void proxy_adv_timeout_eval(struct bt_mesh_subnet *sub)
{
	int32_t time_passed;

	if (sub->node_id == BT_MESH_NODE_IDENTITY_RUNNING) {
		time_passed = k_uptime_get_32() - sub->node_id_start;
		if (time_passed > (NODE_ID_TIMEOUT - MSEC_PER_SEC)) {
			bt_mesh_proxy_identity_stop(sub);
			LOG_DBG("Node ID stopped for subnet %d after %dms", sub->net_idx,
				time_passed);
		}
	}

#if defined(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV)
	if (bt_mesh_od_priv_proxy_get() > 0 && sub->solicited && sub->priv_net_id_sent) {
		time_passed = k_uptime_get_32() - sub->priv_net_id_sent;
		if (time_passed > ((MSEC_PER_SEC * bt_mesh_od_priv_proxy_get()) - MSEC_PER_SEC)) {
			sub->priv_net_id_sent = 0;
			sub->solicited = false;
			LOG_DBG("Private Network ID stopped for subnet %d after %dms on "
				"solicitation",
				sub->net_idx, time_passed);
		}
	}
#endif
}

enum proxy_adv_evt {
	NET_ID,
	PRIV_NET_ID,
	NODE_ID,
	PRIV_NODE_ID,
	OD_PRIV_NET_ID,
};

struct proxy_adv_request {
	int32_t duration;
	enum proxy_adv_evt evt;
};

static bool proxy_adv_request_get(struct bt_mesh_subnet *sub, struct proxy_adv_request *request)
{
	if (!sub) {
		return false;
	}

	if (sub->net_idx == BT_MESH_KEY_UNUSED) {
		return false;
	}

	/** The priority for proxy adv is first solicitation, then Node Identity,
	 *  and lastly Network ID. Network ID is prioritized last since, in many
	 *  cases, another device can fulfill the same demand. Solicitation is
	 *  prioritized first since legacy devices are dependent on this to
	 *  connect to the network.
	 */

#if defined(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV)
	if (bt_mesh_od_priv_proxy_get() > 0 && sub->solicited) {
		int32_t timeout = MSEC_PER_SEC * (int32_t)bt_mesh_od_priv_proxy_get();

		request->evt = OD_PRIV_NET_ID;
		request->duration = !sub->priv_net_id_sent
					    ? timeout
					    : timeout - (k_uptime_get_32() - sub->priv_net_id_sent);
		return true;
	}
#endif

	if (sub->node_id == BT_MESH_NODE_IDENTITY_RUNNING) {
		request->duration = NODE_ID_TIMEOUT - (k_uptime_get_32() - sub->node_id_start);
		request->evt =
#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
			sub->priv_beacon_ctx.node_id ? PRIV_NODE_ID :
#endif
				NODE_ID;

		return true;
	}

	if (bt_mesh_priv_gatt_proxy_get() == BT_MESH_FEATURE_ENABLED) {
		request->evt = PRIV_NET_ID;
		request->duration = PROXY_RANDOM_UPDATE_INTERVAL;
		return true;
	}

	if (bt_mesh_gatt_proxy_get() == BT_MESH_FEATURE_ENABLED) {
		request->evt = NET_ID;
		request->duration = SYS_FOREVER_MS;
		return true;
	}

	return false;
}

static struct bt_mesh_subnet *adv_sub_get_next(struct bt_mesh_subnet *sub_start,
					       struct proxy_adv_request *request)
{
	struct bt_mesh_subnet *sub_temp = bt_mesh_subnet_next(sub_start);

	do {
		if (proxy_adv_request_get(sub_temp, request)) {
			return sub_temp;
		}

		sub_temp = bt_mesh_subnet_next(sub_temp);
	} while (sub_temp != sub_start);

	return NULL;
}

static struct {
	int32_t start;
	struct bt_mesh_subnet *sub;
	struct proxy_adv_request request;
} sub_adv;

static int gatt_proxy_advertise(void)
{
	int err;

	int32_t max_adv_duration = 0;
	int cnt;
	struct bt_mesh_subnet *sub;
	struct proxy_adv_request request;

	LOG_DBG("");

	/* Close proxy activity that has timed out on all subnets */
	bt_mesh_subnet_foreach(proxy_adv_timeout_eval);

	if (!bt_mesh_proxy_has_avail_conn()) {
		LOG_DBG("Connectable advertising deferred (max connections)");
		return SYS_FOREVER_MS;
	}

	cnt = active_proxy_sub_cnt_get();
	if (!cnt) {
		LOG_DBG("No subnets to advertise proxy on");
		return SYS_FOREVER_MS;
	} else if (cnt > 1) {
		/** There is more than one subnet that requires proxy adv,
		 *  and the adv resources must be shared.
		 */

		/* We use NODE_ID_TIMEOUT as a starting point since it may
		 * be less than 60 seconds. Divide this period into at least
		 * 6 slices, but make sure that a slice is more than one
		 * second long (to avoid excessive rotation).
		 */
		max_adv_duration = NODE_ID_TIMEOUT / MAX(cnt, 6);
		max_adv_duration = MAX(max_adv_duration, MSEC_PER_SEC + 20);

		/* Check if the previous subnet finished its allocated timeslot */
		if ((sub_adv.request.duration != SYS_FOREVER_MS) &&
		    proxy_adv_request_get(sub_adv.sub, &request) &&
		    (sub_adv.request.evt == request.evt)) {
			int32_t time_passed = k_uptime_get_32() - sub_adv.start;

			if (time_passed < sub_adv.request.duration &&
			    ((sub_adv.request.duration - time_passed) >= MSEC_PER_SEC)) {
				sub = sub_adv.sub;
				request.duration = sub_adv.request.duration - time_passed;
				goto end;
			}
		}
	}

	sub = adv_sub_get_next(sub_adv.sub, &request);
	if (!sub) {
		LOG_ERR("Could not find subnet to advertise");
		return SYS_FOREVER_MS;
	}
end:
	if (cnt > 1) {
		request.duration = (request.duration == SYS_FOREVER_MS)
					   ? max_adv_duration
					   : MIN(request.duration, max_adv_duration);
	}

	/* Save current state for next iteration */
	sub_adv.start = k_uptime_get_32();
	sub_adv.sub = sub;
	sub_adv.request = request;

	switch (request.evt) {
	case NET_ID:
		err = net_id_adv(sub, request.duration);
		break;
#if defined(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV)
	case OD_PRIV_NET_ID:
		if (!sub->priv_net_id_sent) {
			sub->priv_net_id_sent = k_uptime_get();
		}
		/* Fall through */
#endif
	case PRIV_NET_ID:
		err = priv_net_id_adv(sub, request.duration);
		break;
	case NODE_ID:
		err = node_id_adv(sub, request.duration);
		break;
	case PRIV_NODE_ID:
		err = priv_node_id_adv(sub, request.duration);
		break;
	default:
		LOG_ERR("Unexpected proxy adv evt: %d", request.evt);
		return SYS_FOREVER_MS;
	}

	if (err) {
		LOG_ERR("Advertising proxy failed (err: %d)", err);
		return SYS_FOREVER_MS;
	}

	LOG_DBG("Advertising %d ms for net_idx 0x%04x", request.duration, sub->net_idx);
	return request.duration;
}

static void subnet_evt(struct bt_mesh_subnet *sub, enum bt_mesh_key_evt evt)
{
	if (evt == BT_MESH_KEY_DELETED) {
		if (sub == beacon_sub) {
			beacon_sub = NULL;
		}
	} else {
		bt_mesh_proxy_beacon_send(sub);
		bt_mesh_adv_gatt_update();
	}
}

#if (CONFIG_MESH_CB_REGISTERED)
static struct bt_mesh_subnet_cb bt_mesh_subnet_cb_gatt_services = {
	.evt_handler = subnet_evt,
	.next = NULL,
};
#else
BT_MESH_SUBNET_CB_DEFINE(gatt_services) = {
	.evt_handler = subnet_evt,
};
#endif
static void proxy_ccc_write(uint8_t conn_idx, uint16_t value)
{
    struct bt_mesh_proxy_client *client;

    LOG_DBG("value: 0x%04x", value);

    if (value != BLE_GATT_CCCD_NTF_BIT) {
        LOG_WRN("Client wrote 0x%04x instead enabling notify", value);
        return;
    }

    client = find_client(conn_idx);
    if (client == NULL) {
        LOG_ERR("find client fail. conn_idx %d", conn_idx);
        return;
    }

    client->proxy_cccd = value;

    if (client->filter_type == TYPE_NONE) {
        client->filter_type = ACCEPT;
        k_work_submit(&client->send_beacons);
    }
}

static void proxy_srv_send_rsp(uint8_t conn_idx, uint16_t status)
{
    struct bt_mesh_proxy_client *client;

    client = find_client(conn_idx);
    if (client == NULL) {
        LOG_ERR("find client fail. conn_idx %d", conn_idx);
        return;
    }

    if (client->cli->end != NULL) {
        client->cli->end(conn_idx, status, client->cli->user_data);
        client->cli->end = NULL;
        client->cli->user_data = NULL;
    }
}

/* Blue courier wifi attribute index */
enum mesh_proxy_att_idx
{
    MESH_PROXY_IDX_PRIM_SVC,
    MESH_PROXY_IDX_CHAR_WRITE,
    MESH_PROXY_IDX_WRITE,
    MESH_PROXY_IDX_CHAR_NTF,
    MESH_PROXY_IDX_NTF,
    MESH_PROXY_IDX_NTF_CFG,

    MESH_PROXY_IDX_NUMBER,
};

/* Profile id. blue courier wifi profile identity */
uint8_t    mesh_proxy_prf_id;
const ble_gatt_attr_desc_t mesh_proxy_att_db[MESH_PROXY_IDX_NUMBER] = {
    [MESH_PROXY_IDX_PRIM_SVC]   = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_PRIMARY_SERVICE),     PROP(RD),            0                                 },
    [MESH_PROXY_IDX_CHAR_WRITE] = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),      PROP(RD),            0                                 },
    [MESH_PROXY_IDX_WRITE]      = {UUID_16BIT_TO_ARRAY(BLE_GATT_CHAR_MESH_PROXY_DATA_IN),  PROP(WC),            CONFIG_BT_MESH_PROXY_MSG_LEN      },
    [MESH_PROXY_IDX_CHAR_NTF]   = {UUID_16BIT_TO_ARRAY(BLE_GATT_DECL_CHARACTERISTIC),      PROP(RD),            0                                 },
    [MESH_PROXY_IDX_NTF]        = {UUID_16BIT_TO_ARRAY(BLE_GATT_CHAR_MESH_PROXY_DATA_OUT), PROP(NTF),           CONFIG_BT_MESH_PROXY_MSG_LEN      },
    [MESH_PROXY_IDX_NTF_CFG]    = {UUID_16BIT_TO_ARRAY(BLE_GATT_DESC_CLIENT_CHAR_CFG),     PROP(RD) | PROP(WR), OPT(NO_OFFSET) | sizeof(uint16_t) },
};

static ble_status_t bt_mesh_proxy_gatts_msg_cb(ble_gatts_msg_info_t *p_srv_msg_info)
{
    uint8_t  att_idx, conn_idx;
    uint8_t *data;
    uint16_t data_len;
    struct bt_mesh_proxy_client *client;

    if (p_srv_msg_info->srv_msg_type == BLE_SRV_EVT_GATT_OPERATION) {
        conn_idx = p_srv_msg_info->msg_data.gatts_op_info.conn_idx;
        client = find_client(conn_idx);
        if (client == NULL) {
            /* ignore */
            return BLE_ERR_NO_ERROR;
        }

        if (p_srv_msg_info->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_WRITE_REQ) {
            att_idx = p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.write_req.att_idx;
            data = p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.write_req.p_val;
            data_len = p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.write_req.val_len;

            switch (att_idx) {
            case MESH_PROXY_IDX_WRITE:
                gatt_recv(conn_idx, data, data_len);
                break;
            case MESH_PROXY_IDX_NTF_CFG:
                if (data_len != sizeof(uint16_t))
                    return BLE_ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
                ble_gatts_svc_attr_write_cfm(conn_idx, p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.write_req.token, BLE_ERR_NO_ERROR);
                p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.write_req.pending_cfm = true;
                proxy_ccc_write(conn_idx, *(uint16_t *)data);
                break;
            default:
                break;
            }
        } else if (p_srv_msg_info->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_NTF_IND_SEND_RSP) {
            proxy_srv_send_rsp(conn_idx, p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.ntf_ind_send_rsp.status);
        } else if (p_srv_msg_info->msg_data.gatts_op_info.gatts_op_sub_evt == BLE_SRV_EVT_READ_REQ) {
            ble_gatts_read_req_t *p_req = &p_srv_msg_info->msg_data.gatts_op_info.gatts_op_data.read_req;
            switch (p_req->att_idx) {
            case MESH_PROXY_IDX_NTF_CFG:
                p_req->val_len = sizeof(uint16_t);
                p_req->att_len = sizeof(uint16_t);
                memcpy(p_req->p_val, &client->proxy_cccd, p_req->val_len);
                break;
            default:
                break;
            }
        }
    }

    return BLE_ERR_NO_ERROR;
}

static void svc_reg_work_handler(struct k_work *work);
static struct k_work_delayable svc_reg_work = Z_WORK_DELAYABLE_INITIALIZER(svc_reg_work_handler);
static uint32_t svc_reg_attempts;

static void svc_reg_work_handler(struct k_work *work)
{
	ble_status_t err;
	uint8_t mesh_proxy_svc_uuid[16] = UUID_16BIT_TO_ARRAY(BLE_GATT_SVC_MESH_PROXY);

	err = ble_gatts_svc_add(&mesh_proxy_prf_id, mesh_proxy_svc_uuid, 0, 0, mesh_proxy_att_db, MESH_PROXY_IDX_NUMBER, bt_mesh_proxy_gatts_msg_cb);
	if ((err != BLE_ERR_NO_ERROR) && ((--svc_reg_attempts) > 0)) {
		/* settings_load() didn't finish yet. Try again. */
		(void)k_work_schedule(&svc_reg_work, PROXY_SVC_INIT_TIMEOUT);
		return;
	} else if (err) {
		LOG_ERR("Unable to register Mesh Proxy Service (err %d)", err);
		return;
	}

	service_registered = true;

	for (int i = 0; i < ARRAY_SIZE(clients); i++) {
		if (clients[i].cli) {
			clients[i].filter_type = ACCEPT;
		}
	}

	bt_mesh_adv_gatt_update();
}

int bt_mesh_proxy_gatt_enable(void)
{
	int err;

	LOG_DBG("");

	if (!bt_mesh_is_provisioned()) {
		return -ENOTSUP;
	}

	if (service_registered) {
		return -EBUSY;
	}

	svc_reg_attempts = PROXY_SVC_REG_ATTEMPTS;
	err = k_work_schedule(&svc_reg_work, PROXY_SVC_INIT_TIMEOUT);
	if (err < 0) {
		LOG_ERR("Enabling GATT proxy failed (err %d)", err);
		return err;
	}

	return 0;
}

void bt_mesh_proxy_gatt_disconnect(void)
{
	int i;

	LOG_DBG("");

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		struct bt_mesh_proxy_client *client = &clients[i];

		if (client->cli && (client->filter_type == ACCEPT ||
				     client->filter_type == REJECT)) {
			client->filter_type = NONE;
			ble_conn_disconnect(client->cli->conn_idx, BLE_ERROR_HL_TO_HCI(BLE_LL_ERR_REMOTE_USER_TERM_CON));
		}
	}
}

int bt_mesh_proxy_gatt_disable(void)
{
	LOG_DBG("");

	if (!service_registered) {
		return -EALREADY;
	}

	bt_mesh_proxy_gatt_disconnect();

	ble_gatts_svc_rmv(mesh_proxy_prf_id);
	service_registered = false;

	return 0;
}

void bt_mesh_proxy_addr_add(struct net_buf_simple *buf, uint16_t addr)
{
	struct bt_mesh_proxy_client *client;
	struct bt_mesh_proxy_role *cli =
		CONTAINER_OF(buf, struct bt_mesh_proxy_role, buf);

	client = find_client(cli->conn_idx);
	if (client == NULL) {
		LOG_ERR("find client fail. conn_idx %d", cli->conn_idx);
		return;
	}

	LOG_DBG("filter_type %u addr 0x%04x", client->filter_type, addr);

	if (client->filter_type == ACCEPT) {
		filter_add(client, addr);
	} else if (client->filter_type == REJECT) {
		filter_remove(client, addr);
	}
}

static bool client_filter_match(struct bt_mesh_proxy_client *client,
				uint16_t addr)
{
	int i;

	LOG_DBG("filter_type %u addr 0x%04x", client->filter_type, addr);

	if (client->filter_type == REJECT) {
		for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
			if (client->filter[i] == addr) {
				return false;
			}
		}

		return true;
	}

	if (addr == BT_MESH_ADDR_ALL_NODES) {
		return true;
	}

	if (client->filter_type == ACCEPT) {
		for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
			if (client->filter[i] == addr) {
				return true;
			}
		}
	}

	return false;
}

bool bt_mesh_proxy_relay(struct bt_mesh_adv *adv, uint16_t dst)
{
	bool relayed = false;
	int i;

	LOG_DBG("%u bytes to dst 0x%04x", adv->b.len, dst);

	for (i = 0; i < ARRAY_SIZE(clients); i++) {
		struct bt_mesh_proxy_client *client = &clients[i];

		if (!client->cli) {
			continue;
		}

		if (!client_filter_match(client, dst)) {
			continue;
		}

		if (bt_mesh_proxy_relay_send(client->cli->conn_idx, adv)) {
			continue;
		}

		relayed = true;
	}

	return relayed;
}

static void solicitation_reset(struct bt_mesh_subnet *sub)
{
#if defined(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV)
	sub->solicited = false;
	sub->priv_net_id_sent = 0;
#endif
}

static void gatt_connected(uint8_t conn_idx, uint8_t role)
{
	struct bt_mesh_proxy_client *client;

	if (role != BLE_SLAVE || !service_registered) {
		return;
	}

	LOG_INF("conn %d role %u", conn_idx, role);

	client = alloc_client();
	if (client == NULL) {
		LOG_ERR("find client fail. conidx %d", conn_idx);
		return;
	}

	client->filter_type = NONE;
	(void)memset(client->filter, 0, sizeof(client->filter));
	client->proxy_cccd = 0;
	client->cli = bt_mesh_proxy_role_setup(conn_idx, proxy_send, proxy_msg_recv);
	client->cli->role = role;

#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	/* Binding from MshPRTv1.1: 7.2.2.2.6. */
	enum bt_mesh_subnets_node_id_state cur_node_id = bt_mesh_subnets_node_id_state_get();

	if (bt_mesh_gatt_proxy_get() == BT_MESH_FEATURE_ENABLED ||
	    cur_node_id == BT_MESH_SUBNETS_NODE_ID_STATE_ENABLED) {
		client->privacy = false;
	} else {
		client->privacy = (bt_mesh_priv_gatt_proxy_get() == BT_MESH_FEATURE_ENABLED) ||
				  (cur_node_id == BT_MESH_SUBNETS_NODE_ID_STATE_ENABLED_PRIVATE);
	}

	LOG_DBG("privacy: %d", client->privacy);
#endif

	/* If connection was formed after Proxy Solicitation we need to stop future
	 * Private Network ID advertisements
	 */
	bt_mesh_subnet_foreach(solicitation_reset);

	/* Try to re-enable advertising in case it's possible */
	if (bt_mesh_proxy_has_avail_conn()) {
		bt_mesh_adv_gatt_update();
	}
}

static void gatt_disconnected(uint8_t conn_idx, uint8_t reason)
{
    struct bt_mesh_proxy_client *client;

    if (!service_registered && bt_mesh_is_provisioned()) {
        (void)bt_mesh_proxy_gatt_enable();
        return;
    }

    client = find_client(conn_idx);
    if (client == NULL) {
        return;
    }

    LOG_INF("Disconnected conn_idx:%u  reason:0x%x", conn_idx, reason);

    if (client->cli) {
        bt_mesh_proxy_role_cleanup(client->cli);
        client->cli = NULL;
    }
}

static void bt_mesh_proxy_srv_conn_evt_handler(ble_conn_evt_t event, ble_conn_data_u *p_data)
{
    switch (event) {
    case BLE_CONN_EVT_STATE_CHG:
        if (p_data->conn_state.state == BLE_CONN_STATE_DISCONNECTD) {
            gatt_disconnected(p_data->conn_state.info.discon_info.conn_idx, p_data->conn_state.info.discon_info.reason);
        } else if (p_data->conn_state.state == BLE_CONN_STATE_CONNECTED) {
            gatt_connected(p_data->conn_state.info.conn_info.conn_idx, p_data->conn_state.info.conn_info.role);
        }
        break;
    case BLE_CONN_EVT_PARAM_UPDATE_IND:
        ble_conn_param_update_cfm(p_data->conn_param_req_ind.conn_idx, true, 2, 4);
        break;
    default:
        break;
    }
}

static int proxy_send(uint8_t conn_idx, void *data, uint16_t len, bt_gatt_complete_func_t end, void *user_data)
{
    LOG_DBG("conn_idx %u   data %u bytes: %s", conn_idx, len, bt_hex(data, len));

    struct bt_mesh_proxy_client *client;

    client = find_client(conn_idx);
    if (client == NULL || client->proxy_cccd != BLE_GATT_CCCD_NTF_BIT) {
        LOG_ERR("find client fail. conn_idx %d", conn_idx);
        return -1;
    }

    client->cli->end = end;
    client->cli->user_data = user_data;

    ble_gatts_ntf_ind_send(conn_idx, mesh_proxy_prf_id, MESH_PROXY_IDX_NTF, data, len, BLE_GATT_NOTIFY);

    return 0;
}

int bt_mesh_proxy_adv_start(void)
{
	LOG_DBG("");

	if (!service_registered || !bt_mesh_is_provisioned()) {
		return SYS_FOREVER_MS;
	}

	return gatt_proxy_advertise();
}

uint8_t bt_mesh_proxy_srv_connected_cnt(void)
{
	uint8_t cnt = 0;

	for (int i = 0; i < ARRAY_SIZE(clients); i++) {
		if (clients[i].cli) {
			cnt++;
		}
	}

	return cnt;
}

#if (CONFIG_MESH_CB_REGISTERED)
void bt_mesh_proxy_srv_subnet_cb_init(void)
{
	bt_mesh_subnet_cb_register(&bt_mesh_subnet_cb_gatt_services);
}
#endif

void bt_mesh_proxy_srv_init(void)
{
	ble_conn_callback_register(bt_mesh_proxy_srv_conn_evt_handler);
}

#endif // CONFIG_BT_MESH_GATT_PROXY
