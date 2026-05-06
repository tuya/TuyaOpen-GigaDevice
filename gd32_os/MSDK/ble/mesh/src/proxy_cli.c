/*
 * Copyright (c) 2021 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mesh_cfg.h"
#include "mesh_kernel.h"
#include "sys/byteorder.h"

#include "net/buf.h"
#include "bluetooth/mesh_bluetooth.h"
#include "bluetooth/mesh_uuid.h"
#include "api/mesh.h"

#include "ble_gap.h"
#include "ble_gatt.h"
#include "ble_conn.h"
#include "bluetooth/bt_str.h"

#include "mesh.h"
#include "net.h"
#include "rpl.h"
#include "transport.h"
#include "prov.h"
#include "beacon.h"
#include "foundation.h"
#include "access.h"
#include "proxy.h"
#include "gatt_cli.h"
#include "proxy_msg.h"
#include "crypto.h"

#define LOG_LEVEL CONFIG_BT_MESH_PROXY_LOG_LEVEL
#include "api/mesh_log.h"

#if (CONFIG_BT_MESH_PROXY_CLIENT)

#define PROXY_OP_TIMEOUT K_SECONDS(10)

static struct bt_mesh_proxy_server {
	struct bt_mesh_proxy_role *role;
	bool link_opened;
	uint16_t net_idx;
	uint16_t id_addr;
	uint8_t pending_op;
	struct k_sem  status_sem;       /**< Sync semaphore. */
	int *status;
	struct k_work_delayable op_timer;
} servers[CONFIG_BT_MAX_CONN] = {
	[0 ... (CONFIG_BT_MAX_CONN - 1)] = {
		.net_idx = BT_MESH_KEY_UNUSED,
		.pending_op = 0xFF,
	},
};

static bool allow_all_subnet;

static struct bt_mesh_proxy_server *find_proxy_srv(uint16_t net_idx, bool conn, bool disconn)
{
	for (int i = 0; i < ARRAY_SIZE(servers); i++) {
		if (!servers[i].role) {
			if (!disconn) {
				continue;
			}
		} else if (!conn) {
			continue;
		}

		if (servers[i].net_idx == net_idx) {
			return &servers[i];
		}
	}

	return NULL;
}

static struct bt_mesh_proxy_server *find_proxy_srv_by_conn(uint8_t conn_idx)
{
	for (int i = 0; i < ARRAY_SIZE(servers); i++) {
		if (!servers[i].role ||
			servers[i].role->conn_idx != conn_idx) {
			continue;
		}

		return &servers[i];
	}

	return NULL;
}

bool bt_mesh_proxy_cli_relay(struct bt_mesh_adv *adv)
{
	bool relayed = false;
	int i;

	for (i = 0; i < ARRAY_SIZE(servers); i++) {
		struct bt_mesh_proxy_server *server = &servers[i];

		if (!server->link_opened) {
			continue;
		}

		if (bt_mesh_proxy_relay_send(server->role->conn_idx, adv)) {
			continue;
		}

		relayed = true;
	}

	return relayed;
}

static void bt_mesh_gatt_complete_cb(uint8_t conn_idx, uint16_t status, void *user_data)
{
    struct bt_mesh_proxy_server *srv = (struct bt_mesh_proxy_server *)user_data;

    if (status != BLE_ERR_NO_ERROR) {
        srv->pending_op = 0xFF;
        k_work_cancel_delayable(&srv->op_timer);
    }
}


static void proxy_filter_status_recv(uint8_t conn_idx, struct bt_mesh_net_rx *rx,
                              struct net_buf_simple *buf)
{
    struct bt_mesh_proxy_server *srv = find_proxy_srv_by_conn(conn_idx);
    uint8_t opcode;

    if (!srv) {
        return;
    }

    opcode = net_buf_simple_pull_u8(buf);

    if (opcode == CFG_FILTER_STATUS) {
        srv->pending_op = 0xFF;
        k_work_cancel_delayable(&srv->op_timer);
        if (srv->status) {
          *srv->status = 0;
          srv->status = NULL;
          k_sem_give(&srv->status_sem);
        }
    }
}

static void proxy_cli_cfg_recv(struct bt_mesh_proxy_role *role)
{
    NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_NET_MAX_PDU_LEN);
    struct bt_mesh_net_rx rx;
    int err;

    err = bt_mesh_net_decode(&role->buf, BT_MESH_NET_IF_PROXY_CFG, &rx, &buf);
    if (err) {
        LOG_ERR("Failed to decode Proxy Configuration (err %d)", err);
        return;
    }

    rx.local_match = 1U;

    if (bt_mesh_rpl_check(&rx, NULL, false)) {
        LOG_WRN("Replay: src 0x%04x dst 0x%04x seq 0x%06x", rx.ctx.addr, rx.ctx.recv_dst, rx.seq);
        return;
    }

    /* Remove network headers */
    net_buf_simple_pull(&buf, BT_MESH_NET_HDR_LEN);

    LOG_DBG("%u bytes: %s", buf.len, bt_hex(buf.data, buf.len));

    if (buf.len < 1) {
        LOG_WRN("Too short proxy configuration PDU");
        return;
    }

    proxy_filter_status_recv(role->conn_idx, &rx, &buf);
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
		proxy_cli_cfg_recv(role);
		break;
	default:
		LOG_WRN("Unhandled Message Type 0x%02x", role->msg_type);
		break;
	}
}

static void proxy_op_timeout(struct k_work *work)
{
    struct bt_mesh_proxy_server *srv;
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);

    LOG_WRN("Proxy operation timeout");

    srv = CONTAINER_OF(dwork, struct bt_mesh_proxy_server, op_timer);
    if (srv->status) {
        *srv->status = -ETIMEDOUT;
        srv->status = NULL;
        k_sem_give(&srv->status_sem);
    }
    if (srv->pending_op != 0xFF) {
        ble_conn_disconnect(srv->role->conn_idx, BLE_ERROR_HL_TO_HCI(BLE_LL_ERR_REMOTE_USER_TERM_CON));
    }
}

static void proxy_connected(uint8_t conn_idx, void *user_data)
{
	struct bt_mesh_proxy_server *srv = user_data;

	srv->role = bt_mesh_proxy_role_setup(conn_idx, bt_mesh_gatt_send, proxy_msg_recv);
	srv->role->role = BLE_MASTER;
	k_work_init_delayable(&srv->op_timer, proxy_op_timeout);
	if (!srv->status_sem.sem)
		k_sem_init(&srv->status_sem, 0, 1);
	srv->status = NULL;
}

static void proxy_link_open(uint8_t conn_idx)
{
	struct bt_mesh_proxy_server *srv = find_proxy_srv_by_conn(conn_idx);

	srv->link_opened = true;
}

static void proxy_disconnected(uint8_t conn_idx)
{
	struct bt_mesh_proxy_server *srv = find_proxy_srv_by_conn(conn_idx);

	bt_mesh_proxy_role_cleanup(srv->role);

	srv->role = NULL;
	srv->link_opened = false;
	srv->pending_op = 0xFF;
	k_work_cancel_delayable(&srv->op_timer);
	if (srv->status) {
		*srv->status = -ETIMEDOUT;
		srv->status = NULL;
		k_sem_give(&srv->status_sem);
	}
}

static const struct bt_mesh_gatt_cli proxy = {
	.srv_uuid		= BLE_UUID_INIT_16(BLE_GATT_SVC_MESH_PROXY),
	.data_in_uuid		= BLE_UUID_INIT_16(BLE_GATT_CHAR_MESH_PROXY_DATA_IN),
	.data_out_uuid		= BLE_UUID_INIT_16(BLE_GATT_CHAR_MESH_PROXY_DATA_OUT),
	.data_out_cccd_uuid	= BLE_UUID_INIT_16(BLE_GATT_DESC_CLIENT_CHAR_CFG),

	.connected		= proxy_connected,
	.link_open		= proxy_link_open,
	.disconnected		= proxy_disconnected
};

static bool proxy_srv_check_and_get(struct bt_mesh_subnet *sub, const uint8_t *net_id,
				    struct bt_mesh_proxy_server **p_srv)
{
	struct bt_mesh_proxy_server *srv;

	srv = find_proxy_srv(sub->net_idx, true, true);
	if (srv) {
		if (srv->role) {
			return true;
		}
	} else if (!allow_all_subnet) {
		return false;
	}

	if (!srv) {
		srv = find_proxy_srv(BT_MESH_KEY_UNUSED, false, true);
		if (!srv) {
			return true;
		}
	}

	/* If net_id is NULL we already know that the networks match */
	if (!net_id || !memcmp(sub->keys[0].net_id, net_id, 8) ||
	    (bt_mesh_subnet_has_new_key(sub) && !memcmp(sub->keys[1].net_id, net_id, 8))) {

		*p_srv = srv;
		return true;
	}

	return false;
}

struct find_net_id {
	uint8_t type;

	union {
		const uint8_t *net_id;
		struct {
			const uint8_t *hash;
			const uint8_t *rand;
		} priv;
	} data;

	struct bt_mesh_proxy_server *srv;
};

static bool is_hash_equal(struct bt_mesh_subnet *sub, struct find_net_id *res, uint8_t idx)
{
	int err;
	uint8_t in[16], out[16];

	memcpy(&in[0], sub->keys[idx].net_id, 8);
	memcpy(&in[8], res->data.priv.rand, 8);
	err = bt_mesh_encrypt(&sub->keys[idx].identity, in, out);
	if (err) {
		LOG_ERR("Failed to generate hash (err: %d)", err);
		return false;
	}

	if (memcmp(&out[8], res->data.priv.hash, 8)) {
		return false;
	}

	return true;
}

static bool has_net_id(struct bt_mesh_subnet *sub, void *user_data)
{
	struct find_net_id *res = user_data;
	uint8_t *net_id = NULL;

	if (res->type == BT_MESH_ID_TYPE_NET) {
		net_id = (uint8_t *)res->data.net_id;
		goto end;
	}

	/* Additional handling for BT_MESH_ID_TYPE_PRIV_NET msg type */
	if (!(is_hash_equal(sub, res, 0) ||
	      (bt_mesh_subnet_has_new_key(sub) && is_hash_equal(sub, res, 1)))) {
		return false;
	}
end:
	return proxy_srv_check_and_get(sub, net_id, &res->srv);
}

static void handle_net_id(uint8_t type, const struct bt_le_scan_recv_info *info,
			  struct net_buf_simple *buf)
{
	int err;
	struct find_net_id res;
	struct bt_mesh_subnet *sub;

	res.type = type;
	res.srv = NULL;

	if (type == BT_MESH_ID_TYPE_NET) {
		if (buf->len != 8) {
			return;
		}
		res.data.net_id = net_buf_simple_pull_mem(buf, 8);

	} else {
		if (buf->len != 16) {
			return;
		}

		res.data.priv.hash = net_buf_simple_pull_mem(buf, 8);
		res.data.priv.rand = net_buf_simple_pull_mem(buf, 8);
	}

	sub = bt_mesh_subnet_find(has_net_id, (void *)&res);
	if (sub && res.srv) {
		err = bt_mesh_gatt_cli_connect(info->addr, &proxy, res.srv);
		if (err) {
			LOG_DBG("Failed to connect over GATT (err:%d)", err);
		}
	}
}

static bool is_node_hash_equal(struct bt_mesh_subnet *sub, struct find_net_id *res, uint8_t idx)
{
    int err;
    uint8_t in[16], out[16];
    struct bt_mesh_proxy_server *srv;

    srv = find_proxy_srv(sub->net_idx, false, true);

    if (!srv) {
        return false;
    }

    memset(in, 0, 16);

    memcpy(&in[6], res->data.priv.rand, 8);
    sys_put_be16(srv->id_addr, &in[14]);

    if (res->type == BT_MESH_ID_TYPE_PRIV_NODE) {
        in[5] = 0x03;
    }

    err = bt_mesh_encrypt(&sub->keys[idx].identity, in, out);
    if (err) {
        LOG_ERR("Failed to generate hash (err: %d)", err);
        return false;
    }

    if (memcmp(&out[8], res->data.priv.hash, 8)) {
        return false;
    }

    res->srv = srv;
    return true;
}

static bool has_node_identity_addr(struct bt_mesh_subnet *sub, void *user_data)
{
    struct find_net_id *res = user_data;

    if (!(is_node_hash_equal(sub, res, 0) ||
          (bt_mesh_subnet_has_new_key(sub) && is_node_hash_equal(sub, res, 1)))) {
        return false;
    }

    return true;
}


static void handle_node_identity_addr(uint8_t type, const struct bt_le_scan_recv_info *info,
                                        struct net_buf_simple *buf)
{
    int err;
    struct find_net_id res;
    struct bt_mesh_subnet *sub;

    res.type = type;
    res.srv = NULL;

    if (buf->len != 16) {
        return;
    }

    res.data.priv.hash = net_buf_simple_pull_mem(buf, 8);
    res.data.priv.rand = net_buf_simple_pull_mem(buf, 8);

    sub = bt_mesh_subnet_find(has_node_identity_addr, (void *)&res);
    if (sub) {
        err = bt_mesh_gatt_cli_connect(info->addr, &proxy, res.srv);
        if (err) {
            LOG_DBG("Failed to connect over GATT (err:%d)", err);
        }
    }
}


void bt_mesh_proxy_cli_adv_recv(const struct bt_le_scan_recv_info *info,
				struct net_buf_simple *buf)
{
	uint8_t type;

	type = net_buf_simple_pull_u8(buf);
	switch (type) {
	case BT_MESH_ID_TYPE_NET:
		/* Fallthrough */
	case BT_MESH_ID_TYPE_PRIV_NET: {
		handle_net_id(type, info, buf);
		break;
	}
	case BT_MESH_ID_TYPE_NODE:
		/* Fallthrough */
	case BT_MESH_ID_TYPE_PRIV_NODE: {
		handle_node_identity_addr(type, info, buf);
		break;
	}
	default:
		return;
	}
}

int bt_mesh_proxy_connect(uint16_t net_idx)
{
	struct bt_mesh_proxy_server *srv;

	if (net_idx == BT_MESH_KEY_ANY) {
		if (allow_all_subnet) {
			return -EALREADY;
		}

		allow_all_subnet = true;

		return 0;
	}

	srv = find_proxy_srv(net_idx, true, true);
	if (srv) {
		return -EALREADY;
	}

	srv = find_proxy_srv(BT_MESH_KEY_UNUSED, false, true);
	if (!srv) {
		return -ENOMEM;
	}

	srv->net_idx = net_idx;

	return 0;
}

int bt_mesh_proxy_connect_node_id(uint16_t net_idx, uint16_t addr)
{
	struct bt_mesh_proxy_server *srv;

	if (!BT_MESH_ADDR_IS_UNICAST(addr)) {
		return -EINVAL;
	}

	srv = find_proxy_srv(net_idx, true, true);
	if (srv) {
		return -EALREADY;
	}

	srv = find_proxy_srv(BT_MESH_KEY_UNUSED, false, true);
	if (!srv) {
		return -ENOMEM;
	}

	srv->net_idx = net_idx;
	srv->id_addr = addr;

	return 0;
}


int bt_mesh_proxy_disconnect(uint16_t net_idx)
{
	int err;
	struct bt_mesh_proxy_server *srv;

	if (net_idx != BT_MESH_KEY_ANY) {
		srv = find_proxy_srv(net_idx, true, true);
		if (!srv) {
			return -EALREADY;
		}

		srv->net_idx = BT_MESH_KEY_UNUSED;
		srv->id_addr = BT_MESH_ADDR_UNASSIGNED;

		if (!srv->role) {
			return 0;
		}

		return ble_conn_disconnect(srv->role->conn_idx, BLE_ERROR_HL_TO_HCI(BLE_LL_ERR_REMOTE_USER_TERM_CON));
	}

	if (!allow_all_subnet) {
		return -EALREADY;
	}

	allow_all_subnet = false;

	for (int i = 0; i < ARRAY_SIZE(servers); i++) {
		servers[i].net_idx = BT_MESH_KEY_UNUSED;

		if (!servers[i].role) {
			continue;
		}

		err = ble_conn_disconnect(servers[i].role->conn_idx, BLE_ERROR_HL_TO_HCI(BLE_LL_ERR_REMOTE_USER_TERM_CON));
		if (err) {
			return err;
		}
	}

	return 0;
}

static void subnet_evt(struct bt_mesh_subnet *sub, enum bt_mesh_key_evt evt)
{
	switch (evt) {
	case BT_MESH_KEY_DELETED:
		(void)bt_mesh_proxy_disconnect(sub->net_idx);
		break;

	default:
		break;
	}
}

#if (CONFIG_MESH_CB_REGISTERED)
static struct bt_mesh_subnet_cb bt_mesh_subnet_cb_proxy_cli = {
	.evt_handler = subnet_evt,
	.next = NULL,
};
#else
BT_MESH_SUBNET_CB_DEFINE(proxy_cli) = {
	.evt_handler = subnet_evt,
};
#endif

bool bt_mesh_proxy_cli_is_connected(uint16_t net_idx)
{
	if (find_proxy_srv(net_idx, true, false)) {
		return true;
	}

	return false;
}

int bt_mesh_proxy_filter_set(uint16_t net_idx, uint8_t type, int *status)
{
    NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_NET_MAX_PDU_LEN);
    struct bt_mesh_msg_ctx ctx;
    int i, err = 0;
    struct bt_mesh_proxy_server *srv;
    struct bt_mesh_net_tx tx = {
        .ctx = &ctx,
    };

    srv = find_proxy_srv(net_idx, true, false);
    if (!srv) {
        return -EINVAL;
    }

    if (srv->pending_op != 0xFF) {
        LOG_ERR("Already pending an operation(%d)!", srv->pending_op);
        return -EALREADY;
    }

    tx.sub = bt_mesh_subnet_get(net_idx);
    if (tx.sub == NULL) {
        return -EINVAL;
    }

    tx.src = bt_mesh_primary_addr();
    tx.ctx->net_idx = net_idx;
    tx.ctx->app_idx = BT_MESH_KEY_UNUSED;
    /* Configuration messages always have dst unassigned */
    tx.ctx->addr = BT_MESH_ADDR_UNASSIGNED;
    tx.ctx->send_ttl = 0;

    net_buf_simple_reset(&buf);
    net_buf_simple_reserve(&buf, 10);

    net_buf_simple_add_u8(&buf, CFG_FILTER_SET);
    net_buf_simple_add_u8(&buf, type);

    LOG_DBG("%u bytes: %s", buf.len, bt_hex(buf.data, buf.len));

    err = bt_mesh_net_encode(&tx, &buf, BT_MESH_NONCE_PROXY);
    if (err) {
        LOG_ERR("Encoding Proxy cfg message failed (err %d)", err);
        return err;
    }

    err = bt_mesh_proxy_msg_send(srv->role->conn_idx, BT_MESH_PROXY_CONFIG,
                                 &buf, bt_mesh_gatt_complete_cb, srv);
    if (err) {
        LOG_ERR("Failed to send proxy cfg message (err %d)", err);
        return err;
    }

    srv->status = status;
    srv->pending_op = CFG_FILTER_SET;
    k_work_schedule(&srv->op_timer, PROXY_OP_TIMEOUT);

    if (status) {
        err = k_sem_take(&srv->status_sem, K_FOREVER);
    }

    return err;
}

int bt_mesh_proxy_filter_addr_op(uint16_t net_idx, uint16_t *addrs, uint8_t addr_size, bool add,
                                 int *status)
{
    NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_NET_MAX_PDU_LEN);
    struct bt_mesh_msg_ctx ctx;
    int i, err = 0;
    struct bt_mesh_proxy_server *srv;
    struct bt_mesh_net_tx tx = {
        .ctx = &ctx,
    };

    srv = find_proxy_srv(net_idx, true, false);
    if (!srv || addr_size > 5 || !addrs) {
        return -EINVAL;
    }

    if (srv->pending_op != 0xFF) {
        LOG_ERR("Already pending an operation(%d)!", srv->pending_op);
        return -EALREADY;
    }

    tx.sub = bt_mesh_subnet_get(net_idx);
    if (tx.sub == NULL) {
        return -EINVAL;
    }

    tx.src = bt_mesh_primary_addr();
    tx.ctx->net_idx = net_idx;
    tx.ctx->app_idx = BT_MESH_KEY_UNUSED;
    /* Configuration messages always have dst unassigned */
    tx.ctx->addr = BT_MESH_ADDR_UNASSIGNED;
    tx.ctx->send_ttl = 0;

    net_buf_simple_reset(&buf);
    net_buf_simple_reserve(&buf, 10);

    if (add) {
        net_buf_simple_add_u8(&buf, CFG_FILTER_ADD);
        srv->pending_op = CFG_FILTER_ADD;
    } else {
        net_buf_simple_add_u8(&buf, CFG_FILTER_REMOVE);
        srv->pending_op = CFG_FILTER_REMOVE;
    }

    for (i = 0; i < addr_size; i++) {
        net_buf_simple_add_be16(&buf, addrs[i]);
    }

    LOG_DBG("%u bytes: %s", buf.len, bt_hex(buf.data, buf.len));

    err = bt_mesh_net_encode(&tx, &buf, BT_MESH_NONCE_PROXY);
    if (err) {
        LOG_ERR("Encoding Proxy cfg message failed (err %d)", err);
        return err;
    }


    err = bt_mesh_proxy_msg_send(srv->role->conn_idx, BT_MESH_PROXY_CONFIG,
                                 &buf, bt_mesh_gatt_complete_cb, srv);
    if (err) {
        LOG_ERR("Failed to send proxy cfg message (err %d)", err);
        return err;
    }

    srv->status = status;
    k_work_schedule(&srv->op_timer, PROXY_OP_TIMEOUT);

    if (status) {
        err = k_sem_take(&srv->status_sem, K_FOREVER);
    }
    return err;
}

#if (CONFIG_MESH_CB_REGISTERED)
void bt_mesh_proxy_cli_subnet_cb_init(void)
{
	bt_mesh_subnet_cb_register(&bt_mesh_subnet_cb_proxy_cli);
}
#endif
#endif // CONFIG_BT_MESH_PROXY_CLIENT
