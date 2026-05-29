/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2024, GigaDevice Semiconductor Inc.
 */

#include <stdint.h>

#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "wrapper_os.h"
#include "dbg_print.h"
#include "blufi_int.h"
#include "blufi_adapter.h"
#include "crc.h"

tBLUFI_ENV blufi_env;

// static functions declare
static void btc_blufi_send_ack(uint8_t seq);

uint8_t btc_blufi_profile_init(void)
{
    sys_memset(&blufi_env, 0x0, sizeof(blufi_env));
    blufi_env.frag_size = BLUFI_FRAG_DATA_DEFAULT_LEN;
    blufi_env.enabled = true;
    return 0;
}

static uint8_t btc_blufi_profile_deinit(void)
{
    if (!blufi_env.enabled) {
        dbg_print(ERR, "BLUFI already de-initialized");
        return -1;
    }
    return 0;
}

void btc_blufi_recv_handler(uint8_t *data, int len)
{
    struct blufi_hdr *hdr = (struct blufi_hdr *)data;
    uint16_t checksum, checksum_pkt;
    int ret;

    if (hdr->seq != blufi_env.recv_seq) {
        dbg_print(ERR, "%s seq %d is not expect %d\n", __func__, hdr->seq, blufi_env.recv_seq + 1);
        return;
    }

    blufi_env.recv_seq++;

    // second step, check sum
    if (BLUFI_FC_IS_CHECK(hdr->fc)) {
        checksum = crc16(&hdr->seq, hdr->data_len + 2, hdr->seq);
        checksum_pkt = hdr->data[hdr->data_len] | (((uint16_t) hdr->data[hdr->data_len + 1]) << 8);
        if (checksum != checksum_pkt) {
            dbg_print(ERR, "%s checksum error %04x, pkt %04x\n", __func__, checksum, checksum_pkt);
            btc_blufi_send_error_info(ESP_BLUFI_CHECKSUM_ERROR);
            return;
        }
    }

    if (BLUFI_FC_IS_REQ_ACK(hdr->fc)) {
        btc_blufi_send_ack(hdr->seq);
    }

    if (BLUFI_FC_IS_FRAG(hdr->fc)) {
        if (blufi_env.offset == 0) {
            blufi_env.total_len = hdr->data[0] | (((uint16_t) hdr->data[1]) << 8);
            blufi_env.aggr_buf = sys_malloc(blufi_env.total_len);
            if (blufi_env.aggr_buf == NULL) {
                dbg_print(ERR, "%s no mem, len %d\n", __func__, blufi_env.total_len);
                return;
            }
        }
        if (blufi_env.offset + hdr->data_len  - 2 <= blufi_env.total_len){
            memcpy(blufi_env.aggr_buf + blufi_env.offset, hdr->data + 2, hdr->data_len  - 2);
            blufi_env.offset += (hdr->data_len - 2);
        } else {
            dbg_print(ERR, "%s payload is longer than packet length, len %d \n", __func__, blufi_env.total_len);
            return;
        }

    } else {
        if (blufi_env.offset > 0) {   /* if previous pkt is frag */
            memcpy(blufi_env.aggr_buf + blufi_env.offset, hdr->data, hdr->data_len);

            btc_blufi_protocol_handler(hdr->type, blufi_env.aggr_buf, blufi_env.total_len);
            blufi_env.offset = 0;
            sys_mfree(blufi_env.aggr_buf);
            blufi_env.aggr_buf = NULL;
        } else {
            btc_blufi_protocol_handler(hdr->type, hdr->data, hdr->data_len);
            blufi_env.offset = 0;
        }
    }
}

void btc_blufi_send_notify(void *arg)
{
    struct blufi_hdr *hdr = (struct blufi_hdr *)arg;
    blufi_ntf_event_send((uint8_t *)hdr, ((hdr->fc & BLUFI_FC_CHECK) ?
                           hdr->data_len + sizeof(struct blufi_hdr) + 2 :
                           hdr->data_len + sizeof(struct blufi_hdr)));
}

void btc_blufi_send_encap(uint8_t type, uint8_t *data, int total_data_len)
{
    struct blufi_hdr *hdr = NULL;
    int remain_len = total_data_len;
    uint16_t checksum;
    int ret;

    while (remain_len > 0) {
        if (remain_len > blufi_env.frag_size) {
            hdr = sys_malloc(sizeof(struct blufi_hdr) + 2 + blufi_env.frag_size + 2);
            if (hdr == NULL) {
                dbg_print(ERR, "%s no mem\n", __func__);
                return;
            }
            hdr->fc = 0x0;
            hdr->data_len = blufi_env.frag_size + 2;
            hdr->data[0] = remain_len & 0xff;
            hdr->data[1] = (remain_len >> 8) & 0xff;
            memcpy(hdr->data + 2, &data[total_data_len - remain_len], blufi_env.frag_size); //copy first, easy for check sum
            hdr->fc |= BLUFI_FC_FRAG;
        } else {
            hdr = sys_malloc(sizeof(struct blufi_hdr) + remain_len + 2);
            if (hdr == NULL) {
                dbg_print(ERR, "%s no mem\n", __func__);
                return;
            }
            hdr->fc = 0x0;
            hdr->data_len = remain_len;
            memcpy(hdr->data, &data[total_data_len - remain_len], hdr->data_len); //copy first, easy for check sum
        }

        hdr->type = type;
        hdr->fc |= BLUFI_FC_DIR_E2P;
        hdr->seq = blufi_env.send_seq++;

        if (BLUFI_TYPE_IS_CTRL(hdr->type)) {
            if ((blufi_env.sec_mode & BLUFI_CTRL_SEC_MODE_CHECK_MASK)) {
                hdr->fc |= BLUFI_FC_CHECK;
                checksum = crc16(&hdr->seq, hdr->data_len + 2, hdr->seq);
                memcpy(&hdr->data[hdr->data_len], &checksum, 2);
            }
        } else if (!BLUFI_TYPE_IS_DATA_NEG(hdr->type) && !BLUFI_TYPE_IS_DATA_ERROR_INFO(hdr->type)) {
            if ((blufi_env.sec_mode & BLUFI_DATA_SEC_MODE_CHECK_MASK)) {
                hdr->fc |= BLUFI_FC_CHECK;
                checksum = crc16(&hdr->seq, hdr->data_len + 2, hdr->seq);
                memcpy(&hdr->data[hdr->data_len], &checksum, 2);
            }
        }

        if (hdr->fc & BLUFI_FC_FRAG) {
            remain_len -= (hdr->data_len - 2);
        } else {
            remain_len -= hdr->data_len;
        }

        btc_blufi_send_notify(hdr);

        sys_mfree(hdr);
        hdr =  NULL;
    }
}

void btc_blufi_wifi_conn_report(uint8_t opmode, uint8_t sta_conn_state, uint8_t softap_conn_num, esp_blufi_extra_info_t *info, int info_len)
{
    uint8_t type;
    uint8_t *data;
    int data_len;
    uint8_t *p;

    data_len = info_len + 3;
    p = data = sys_malloc(256);
    if (data == NULL) {
        return;
    }

    type = BLUFI_BUILD_TYPE(BLUFI_TYPE_DATA, BLUFI_TYPE_DATA_SUBTYPE_WIFI_REP);
    *p++ = opmode;
    *p++ = sta_conn_state;
    *p++ = softap_conn_num;

    if (info) {
        if (info->sta_bssid_set) {
            *p++ = BLUFI_TYPE_DATA_SUBTYPE_STA_BSSID;
            *p++ = 6;
            memcpy(p, info->sta_bssid, 6);
            p += 6;
        }
        if (info->sta_ssid) {
            *p++ = BLUFI_TYPE_DATA_SUBTYPE_STA_SSID;
            *p++ = info->sta_ssid_len;
            memcpy(p, info->sta_ssid, info->sta_ssid_len);
            p += info->sta_ssid_len;
        }
        if (info->sta_passwd) {
            *p++ = BLUFI_TYPE_DATA_SUBTYPE_STA_PASSWD;
            *p++ = info->sta_passwd_len;
            memcpy(p, info->sta_passwd, info->sta_passwd_len);
            p += info->sta_passwd_len;
        }
        if (info->softap_ssid) {
            *p++ = BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_SSID;
            *p++ = info->softap_ssid_len;
            memcpy(p, info->softap_ssid, info->softap_ssid_len);
            p += info->softap_ssid_len;
        }
        if (info->softap_passwd) {
            *p++ = BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_PASSWD;
            *p++ = info->softap_passwd_len;
            memcpy(p, info->softap_passwd, info->softap_passwd_len);
            p += info->softap_passwd_len;
        }
        if (info->softap_authmode_set) {
            *p++ = BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_AUTH_MODE;
            *p++ = 1;
            *p++ = info->softap_authmode;
        }
        if (info->softap_max_conn_num_set) {
            *p++ = BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_MAX_CONN_NUM;
            *p++ = 1;
            *p++ = info->softap_max_conn_num;
        }
        if (info->softap_channel_set) {
            *p++ = BLUFI_TYPE_DATA_SUBTYPE_SOFTAP_CHANNEL;
            *p++ = 1;
            *p++ = info->softap_channel;
        }
        if (info->sta_max_conn_retry_set) {
            *p++ = BLUFI_TYPE_DATA_SUBTYPE_STA_MAX_CONN_RETRY;
            *p++ = 1;
            *p++ = info->sta_max_conn_retry;
        }
        if (info->sta_conn_end_reason_set) {
            *p++ = BLUFI_TYPE_DATA_SUBTYPE_STA_CONN_END_REASON;
            *p++ = 1;
            *p++ = info->sta_conn_end_reason;
        }
        if (info->sta_conn_rssi_set) {
            *p++ = BLUFI_TYPE_DATA_SUBTYPE_STA_CONN_RSSI;
            *p++ = 1;
            *p++ = info->sta_conn_rssi;
        }
    }
    if (p - data > 256) {
        dbg_print(ERR, "%s len error %d %d\n", __func__, (int)(p - data), data_len);
    }

    btc_blufi_send_encap(type, data, (int)(p - data));
    sys_mfree(data);
}

void btc_blufi_send_wifi_list(uint16_t apCount, esp_blufi_ap_record_t *list)
{
    uint8_t type;
    uint8_t *data;
    int data_len;
    uint8_t *p;
    // malloc size: (len + RSSI + ssid buffer) * apCount;
    uint16_t malloc_size = (1 + 1 + sizeof(list->ssid)) * apCount;
    p = data = sys_malloc(malloc_size);
    if (data == NULL) {
        dbg_print(ERR, "%s malloc error\n", __func__);
        return;
    }
    type = BLUFI_BUILD_TYPE(BLUFI_TYPE_DATA, BLUFI_TYPE_DATA_SUBTYPE_WIFI_LIST);
    for (int i = 0; i < apCount; ++i)
    {
        uint8_t len = strlen((const char *)list[i].ssid);
        data_len = (p - data);
        //current_len + ssid + rssi + total_len_value
        if((data_len + len + 1 + 1) >  malloc_size) {
            dbg_print(ERR, "%s len error", __func__);
            sys_mfree(data);
            return;
        }
        *p++ = len + 1; // length of ssid + rssi
        *p++ = list[i].rssi;
        memcpy(p, list[i].ssid, len);
        p = p + len;
    }
    data_len = (p - data);
    btc_blufi_send_encap(type, data, data_len);
    sys_mfree(data);
}

static void btc_blufi_send_ack(uint8_t seq)
{
    uint8_t type;
    uint8_t data;

    type = BLUFI_BUILD_TYPE(BLUFI_TYPE_CTRL, BLUFI_TYPE_CTRL_SUBTYPE_ACK);
    data = seq;

    btc_blufi_send_encap(type, &data, 1);
}

void btc_blufi_send_error_info(uint8_t state)
{
    uint8_t type;
    uint8_t *data;
    int data_len;
    uint8_t *p;

    data_len = 1;
    p = data = sys_malloc(data_len);
    if (data == NULL) {
        return;
    }

    type = BLUFI_BUILD_TYPE(BLUFI_TYPE_DATA, BLUFI_TYPE_DATA_SUBTYPE_ERROR_INFO);
    *p++ = state;
    if (p - data > data_len) {
        dbg_print(ERR, "%s len error %d %d\n", __func__, (int)(p - data), data_len);
    }

    btc_blufi_send_encap(type, data, data_len);
    sys_mfree(data);
}

void btc_blufi_send_custom_data(uint8_t *value, uint32_t value_len)
{
    if(value == NULL || value_len == 0) {
        dbg_print(ERR, "%s value or value len error", __func__);
        return;
    }
    uint8_t *data = sys_malloc(value_len);
    if (data == NULL) {
        dbg_print(ERR, "%s mem malloc error", __func__);
        return;
    }
    uint8_t type = BLUFI_BUILD_TYPE(BLUFI_TYPE_DATA, BLUFI_TYPE_DATA_SUBTYPE_CUSTOM_DATA);
    memcpy(data, value, value_len);
    btc_blufi_send_encap(type, data, value_len);
    sys_mfree(data);
}

uint16_t btc_blufi_get_version(void)
{
    return BTC_BLUFI_VERSION;
}
