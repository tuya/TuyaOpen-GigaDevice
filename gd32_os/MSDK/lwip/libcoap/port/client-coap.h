/*
 * server-coap.h -- LwIP example
 *
 * Copyright (C) 2013-2016 Christian Amsüss <chrysn@fsfe.org>
 * Copyright (C) 2022-2024 Jon Shallow <supjps-libcoap@jpshallow.com>
 * Copyright (c) 2024, GigaDevice Semiconductor Inc.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * This file is part of the CoAP library libcoap. Please see README for terms
 * of use.
 */

#include "coap_config.h"
#include <coap3/coap.h>

typedef struct coap_client_config {
    coap_log_t log_level;
    coap_pdu_type_t pdu_type;
    coap_pdu_code_t pdu_code;
    char *use_uri;
    uint8_t *put_data;
    char *use_psk;
    char *use_id;
} coap_client_config_t;

/* Start up the CoAP Client */
void client_coap_init(struct coap_client_config* client_cfg);

/* Close down CoAP activity */

void client_coap_finished(void);

/*
 * Call this when you think that work needs to be done
 *
 * Returns: 1 if there is no more work to be done, else 0
 */
int client_coap_poll(void);
