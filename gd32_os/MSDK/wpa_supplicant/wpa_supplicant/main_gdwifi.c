/*
 * WPA Supplicant / Example program entrypoint
 * Copyright (c) 2003-2005, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2025, GigaDevice Semiconductor Inc.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#include "common.h"
#include "wpa_supplicant_i.h"
#include "config.h"
#include "wifi_wpa.h"
#include "wifi_init.h"

void wpa_supplicant_main(void *env)
{
	int exitcode = 0;
	struct wpa_params params;
	struct wpa_global *global;

	memset(&params, 0, sizeof(params));
	params.wpa_debug_level = MSG_WARNING; //MSG_DEBUG

    params.wpa_debug_show_keys = 0;

	global = wpa_supplicant_init(&params);
	if (global == NULL)
		goto end;
	wifi_wpa_send_event(WIFI_WPA_STARTED, global->params.ctrl_interface, 0, WIFI_WPA_GLOBAL_VIF);
	global->params.ctrl_interface = NULL;

	exitcode = wpa_supplicant_run(global);

	wpa_supplicant_deinit(global);

    wifi_task_terminated(SUPPLICANT_TASK);
  end:
	wifi_wpa_send_event(WIFI_WPA_EXIT, (void *)exitcode, 0, WIFI_WPA_GLOBAL_VIF);
	sys_task_delete(NULL);
}

