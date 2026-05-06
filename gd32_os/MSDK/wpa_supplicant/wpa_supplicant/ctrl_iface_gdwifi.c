/*
 * WPA Supplicant / UDP socket -based control interface
 * Copyright (c) 2004-2016, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2023, GigaDevice Semiconductor Inc.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#include "common.h"
#include "eloop.h"
#include "config.h"
#include "wpa_supplicant_i.h"
#include "ctrl_iface.h"
#include "common/wpa_ctrl.h"

#ifndef CONFIG_NO_WPA_MSG
static void wpa_supplicant_ctrl_iface_msg_cb(void *ctx, int level,
					     enum wpa_msg_type type,
					     const char *txt, size_t len)
{
//	if (level >= wpa_debug_level)
//		wpa_printf(MSG_WARNING, "[WPA] %s\n", txt);
}
#endif


struct ctrl_iface_priv {
};

struct ctrl_iface_global_priv {
	int sock;
};

/* Per interface ctrl interface not used */
struct ctrl_iface_priv *
wpa_supplicant_ctrl_iface_init(struct wpa_supplicant *wpa_s)
{
	// cannot return NULL
	return (struct ctrl_iface_priv *)1;
}

void wpa_supplicant_ctrl_iface_deinit(struct wpa_supplicant *wpa_s,
				      struct ctrl_iface_priv *priv)
{
}

void wpa_supplicant_ctrl_iface_wait(struct ctrl_iface_priv *priv)
{
}

/* Global ctrl_iface */
static void wpa_supplicant_global_ctrl_iface_receive(int sock, void *eloop_ctx,
						     void *sock_ctx)
{
	struct ctrl_iface_global_priv *priv = sock_ctx;
	struct wpa_global *global = eloop_ctx;
	struct wifi_wpa_cmd cmd;
	struct wifi_wpa_resp resp;
	int res;
	struct sockaddr_in from;
	socklen_t fromlen = sizeof(from);
	char *reply = NULL;
	size_t reply_len;

	res = recvfrom(sock, &cmd, sizeof(cmd) - 1, 0,
		       (struct sockaddr *) &from, &fromlen);
	if (res < 0) {
		wpa_printf(MSG_ERROR, "Fail to receive command on WPA ctrl interface");
		return;
	}
	resp.len = cmd.resp_len;

	if (cmd.ifname[0] == 0)
	{
		resp.resp = wpa_supplicant_global_ctrl_iface_process(global, cmd.cmd,
								     &resp.len, cmd.resp);
	}
	else
	{
		struct wpa_supplicant *wpa_s;
		for (wpa_s = global->ifaces; wpa_s; wpa_s = wpa_s->next) {
			if (os_strcmp(cmd.ifname, wpa_s->ifname) == 0)
				break;
		}
		if (wpa_s != NULL) {
			resp.resp = wpa_supplicant_ctrl_iface_process(wpa_s, cmd.cmd,
								      &resp.len, cmd.resp);

		} else {
			resp.resp = NULL;
		}
	}

	if (resp.resp) {
		if (os_strncmp(resp.resp, "FAIL", 4) == 0)
			resp.status = WIFI_WPA_CMD_FAILED;
		else
			resp.status = WIFI_WPA_CMD_OK;
	} else {
		resp.status = WIFI_WPA_CMD_FAILED;
		resp.len = 0;
	}

	sendto(sock, &resp, sizeof(resp), 0, (struct sockaddr *)&from, fromlen);
}


struct ctrl_iface_global_priv *
wpa_supplicant_global_ctrl_iface_init(struct wpa_global *global)
{
	struct ctrl_iface_global_priv *priv;
	struct sockaddr_in addr;
	int port = WPA_GLOBAL_CTRL_IFACE_PORT;

	priv = os_zalloc(sizeof(*priv));
	if (priv == NULL)
		goto fail;

	priv->sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (priv->sock < 0)
		goto fail;

	os_memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl((127 << 24) | 1);

try_again:
	addr.sin_port = htons(port);
	if (bind(priv->sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		port++;
		if ((port - WPA_GLOBAL_CTRL_IFACE_PORT) <
		    WPA_GLOBAL_CTRL_IFACE_PORT_LIMIT)
			goto try_again;
		goto fail;
	}

	// save port (as dummy char *)
	global->params.ctrl_interface = (char *)port;

	eloop_register_read_sock(priv->sock,
				 wpa_supplicant_global_ctrl_iface_receive,
				 global, priv);
	wpa_msg_register_cb(wpa_supplicant_ctrl_iface_msg_cb);

	return priv;

fail:
	wpa_printf(MSG_ERROR, "Failed to start WPA ctrl interface");
	if (priv->sock >= 0)
		close(priv->sock);
	os_free(priv);
	return NULL;
}


void
wpa_supplicant_global_ctrl_iface_deinit(struct ctrl_iface_global_priv *priv)
{
	if (priv->sock >= 0) {
		eloop_unregister_read_sock(priv->sock);
		close(priv->sock);
	}
	os_free(priv);
}
