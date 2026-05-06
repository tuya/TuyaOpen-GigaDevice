/*!
    \file    app_virtual_hci.c
    \brief   Implementation of BLE virtual hci.

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

#ifdef CFG_VIRTUAL_HCI_MODE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "app_virtual_hci.h"
#include "virtual_hci.h"
#include "dbg_print.h"
#include "wrapper_os.h"
#include "ble_error.h"
#include "ble_init.h"


typedef struct
{
    uint8_t     hci_ver;
    uint8_t     lmp_ver;
    uint16_t    hci_sub_ver;
    uint16_t    company_id;
    uint16_t    lmp_sub_ver;

    uint8_t     support_cmds[64];
    uint8_t     support_feats[8];
    uint8_t     support_states[8];

    uint16_t    le_acl_data_len;
    uint16_t    le_iso_data_len;
    uint8_t     total_num_acl_pkts;
    uint8_t     total_num_iso_pkts;

    uint8_t     wl_size;
    uint8_t     ral_size;

    uint16_t    max_tx_bytes;
    uint16_t    max_tx_time;
    uint16_t    max_rx_bytes;
    uint16_t    max_rx_time;

    uint16_t    cur_tx_bytes;
    uint16_t    cur_tx_time;

    uint16_t    max_adv_data_bytes;

    uint8_t     num_adv_sets;
    uint8_t     pal_size;
    uint8_t     local_bd_addr[6];
} le_cont_param_t;

static void app_handle_event(uint8_t *p_header, uint16_t payload_length);
static void app_handle_iso_data(uint8_t *p_header, uint16_t payload_length);
static void app_handle_acl_data(uint8_t *p_header, uint16_t payload_length);
static void app_handle_sco_data(uint8_t *p_header, uint16_t payload_length);

static le_cont_param_t le_ctl_param;


static hci_recv_callback_t recv_cb = {
    app_handle_event,
    app_handle_acl_data,
    app_handle_iso_data,
    app_handle_sco_data,
};

static uint8_t count_bits_1(uint32_t value)
{
    uint8_t count = 0;

    while (value) {
        if (value & 0x01) {
            count++;
        }
        value >>= 1;
    }
    return count;
}

static void app_virtual_hci_init_done()
{
    dbg_print(NOTICE, "#######################\r\n");
    dbg_print(NOTICE, "#      reset done      #\r\n");
    dbg_print(NOTICE, "#######################\r\n");
    dbg_print(NOTICE, "hci: 0x%02x/0x%04x, lmp: 0x%02x/0x%04x, company id: 0x%04x \r\n",
              le_ctl_param.hci_ver, le_ctl_param.hci_sub_ver,
              le_ctl_param.lmp_ver, le_ctl_param.lmp_sub_ver, le_ctl_param.company_id);

    dbg_print(NOTICE, "support cmd skip\r\n");

    dbg_print(NOTICE, "support features:  0x%02x/0x%02x/0x%02x/0x%02x/0x%02x/0x%02x/0x%02x/0x%02x\r\n",
              le_ctl_param.support_feats[0], le_ctl_param.support_feats[1],
              le_ctl_param.support_feats[2], le_ctl_param.support_feats[3],
              le_ctl_param.support_feats[4], le_ctl_param.support_feats[5],
              le_ctl_param.support_feats[6], le_ctl_param.support_feats[7]);

    dbg_print(NOTICE, "support states:  0x%02x/0x%02x/0x%02x/0x%02x/0x%02x/0x%02x/0x%02x/0x%02x\r\n",
              le_ctl_param.support_states[0], le_ctl_param.support_states[1],
              le_ctl_param.support_states[2], le_ctl_param.support_states[3],
              le_ctl_param.support_states[4], le_ctl_param.support_states[5],
              le_ctl_param.support_states[6], le_ctl_param.support_states[7]);

    dbg_print(NOTICE, "le_acl_data_len: %d, total_num_acl_packets: %d\r\n",
              le_ctl_param.le_acl_data_len, le_ctl_param.total_num_acl_pkts);

    dbg_print(NOTICE, "le_iso_data_len: %d, total_num_iso_pkts: %d\r\n",
              le_ctl_param.le_iso_data_len, le_ctl_param.total_num_iso_pkts);

    dbg_print(NOTICE, "white_list_size: %d, resolve_list_size: %d, periodic list size %d\r\n",
              le_ctl_param.wl_size, le_ctl_param.ral_size, le_ctl_param.pal_size);

    dbg_print(NOTICE, "max_tx_bytes: %d, max_tx_time: %dms\r\n",
              le_ctl_param.max_tx_bytes, le_ctl_param.max_tx_time);

    dbg_print(NOTICE, "max_rx_bytes: %d, max_rx_time: %dms\r\n",
              le_ctl_param.max_rx_bytes, le_ctl_param.max_rx_time);

    dbg_print(NOTICE, "cur_tx_bytes: %d, cur_tx_time: %dms\r\n",
              le_ctl_param.cur_tx_bytes, le_ctl_param.cur_tx_time);

    dbg_print(NOTICE, "num_adv_sets: %d, max_adv_data_bytes: %d\r\n",
              le_ctl_param.num_adv_sets, le_ctl_param.max_adv_data_bytes);

    dbg_print(NOTICE, "local address 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\r\n",
              le_ctl_param.local_bd_addr[0], le_ctl_param.local_bd_addr[1],
              le_ctl_param.local_bd_addr[2], le_ctl_param.local_bd_addr[3],
              le_ctl_param.local_bd_addr[4], le_ctl_param.local_bd_addr[5]);
    dbg_print(NOTICE, "#######################\r\n");

    ble_task_ready();
}

static void app_handle_cmd_cmplt_event(uint16_t cmd_opcode, uint8_t *pp)
{
    uint8_t status = BLE_ERR_NO_ERROR;
    uint16_t data_len = 0;

    if (pp == NULL) {
        return;
    }

    switch (cmd_opcode) {
    case HCI_RESET_CMD_OPCODE: {
        uint8_t evt_mask[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x00, 0x00, 0x00};
        virtual_hci_send_command(HCI_LE_SET_EVT_MASK_CMD_OPCODE, 8, evt_mask);
    }
    break;

    case HCI_LE_SET_EVT_MASK_CMD_OPCODE: {
        status = pp[data_len];
        data_len++;
        if (status == BLE_ERR_NO_ERROR) {
            uint8_t evt_mask[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0x3D};
            virtual_hci_send_command(HCI_SET_EVT_MASK_CMD_OPCODE, 8, evt_mask);
        }
    }
    break;

    case HCI_SET_EVT_MASK_CMD_OPCODE: {
        status = pp[data_len];
        data_len++;

        if (status == BLE_ERR_NO_ERROR) {
            virtual_hci_send_command(HCI_RD_LOCAL_VER_INFO_CMD_OPCODE, 0, NULL);
        }
    }
    break;
    /*
    case HCI_WR_LE_HOST_SUPP_CMD_OPCODE:            //not support
      {
          status = pp[data_len];
          data_len++;

          if(status == CO_ERROR_NO_ERROR) {
              virtual_hci_send_command(HCI_RD_LOCAL_VER_INFO_CMD_OPCODE, 0, NULL);
          }
      }
      break;
     */

    case HCI_RD_LOCAL_VER_INFO_CMD_OPCODE: {
        status = pp[data_len];
        data_len++;

        if (status == BLE_ERR_NO_ERROR) {
            le_ctl_param.hci_ver = pp[data_len];
            data_len++;

            le_ctl_param.hci_sub_ver = *((uint16_t *) &pp[data_len]);
            data_len += 2;

            le_ctl_param.lmp_ver = pp[data_len];
            data_len++;

            le_ctl_param.lmp_sub_ver = *((uint16_t *) &pp[data_len]);
            data_len += 2;

            virtual_hci_send_command(HCI_RD_LOCAL_SUPP_CMDS_CMD_OPCODE, 0, NULL);
        }
    }
    break;

    case HCI_RD_LOCAL_SUPP_CMDS_CMD_OPCODE: {
        status = pp[data_len];
        data_len++;

        if (status == BLE_ERR_NO_ERROR) {
            sys_memcpy(le_ctl_param.support_cmds, &pp[data_len], 64);
            virtual_hci_send_command(HCI_LE_RD_LOCAL_SUPP_FEATS_CMD_OPCODE, 0, NULL);
        }
    }
    break;

    case HCI_LE_RD_LOCAL_SUPP_FEATS_CMD_OPCODE: {
        status = pp[data_len];
        data_len++;

        if (status == BLE_ERR_NO_ERROR) {
            sys_memcpy(le_ctl_param.support_feats, &pp[data_len], 8);
            virtual_hci_send_command(HCI_LE_RD_SUPP_STATES_CMD_OPCODE, 0, NULL);
        }
    }
    break;

    case HCI_LE_RD_SUPP_STATES_CMD_OPCODE: {
        status = pp[data_len];
        data_len++;

        if (status == BLE_ERR_NO_ERROR) {
            sys_memcpy(le_ctl_param.support_states, &pp[data_len], 8);
            virtual_hci_send_command(HCI_LE_RD_BUF_SIZE_V2_CMD_OPCODE, 0, NULL);
        }
    }
    break;

    case HCI_LE_RD_BUF_SIZE_V2_CMD_OPCODE: {
        status = pp[data_len];
        data_len++;

        if (status == BLE_ERR_NO_ERROR) {
            le_ctl_param.le_acl_data_len = *((uint16_t *) &pp[data_len]);
            data_len += 2;

            le_ctl_param.total_num_acl_pkts = pp[data_len];
            data_len++;

            le_ctl_param.le_iso_data_len = *((uint16_t *) &pp[data_len]);
            data_len += 2;

            le_ctl_param.total_num_iso_pkts = pp[data_len];
            data_len++;

            virtual_hci_send_command(HCI_LE_RD_WLST_SIZE_CMD_OPCODE, 0, NULL);
        }
    }
    break;

    case HCI_LE_RD_WLST_SIZE_CMD_OPCODE: {
        status = pp[data_len];
        data_len++;

        if (status == BLE_ERR_NO_ERROR) {
            le_ctl_param.wl_size = pp[data_len];
            virtual_hci_send_command(HCI_LE_RD_RSLV_LIST_SIZE_CMD_OPCODE, 0, NULL);
        }
    }
    break;

    case HCI_LE_RD_RSLV_LIST_SIZE_CMD_OPCODE: {
        status = pp[data_len];
        data_len++;

        if (status == BLE_ERR_NO_ERROR) {
            le_ctl_param.ral_size = pp[data_len];
            virtual_hci_send_command(HCI_LE_RD_MAX_DATA_LEN_CMD_OPCODE, 0, NULL);
        }
    }
    break;

    case HCI_LE_RD_MAX_DATA_LEN_CMD_OPCODE: {
        status = pp[data_len];
        data_len++;

        if (status == BLE_ERR_NO_ERROR) {
            le_ctl_param.max_tx_bytes = *((uint16_t *) &pp[data_len]);
            data_len += 2;
            le_ctl_param.max_tx_time = *((uint16_t *) &pp[data_len]);
            data_len += 2;
            le_ctl_param.max_rx_bytes = *((uint16_t *) &pp[data_len]);
            data_len += 2;
            le_ctl_param.max_rx_time = *((uint16_t *) &pp[data_len]);
            data_len += 2;

            virtual_hci_send_command(HCI_LE_RD_SUGGTED_DFT_DATA_LEN_CMD_OPCODE, 0, NULL);
        }
    }
    break;

    case HCI_LE_RD_SUGGTED_DFT_DATA_LEN_CMD_OPCODE: {
        status = pp[data_len];
        data_len++;

        if (status == BLE_ERR_NO_ERROR) {
            le_ctl_param.cur_tx_bytes = *((uint16_t *) &pp[data_len]);
            data_len += 2;
            le_ctl_param.cur_tx_time = *((uint16_t *) &pp[data_len]);
            data_len += 2;

            virtual_hci_send_command(HCI_LE_RD_MAX_ADV_DATA_LEN_CMD_OPCODE, 0, NULL);
        }
    }
    break;

    case HCI_LE_RD_MAX_ADV_DATA_LEN_CMD_OPCODE: {
        status = pp[data_len];
        data_len++;

        if (status == BLE_ERR_NO_ERROR) {
            le_ctl_param.max_adv_data_bytes = *((uint16_t *) &pp[data_len]);
            data_len += 2;
            virtual_hci_send_command(HCI_LE_RD_NB_SUPP_ADV_SETS_CMD_OPCODE, 0, NULL);
        }
    }
    break;

    case HCI_LE_RD_NB_SUPP_ADV_SETS_CMD_OPCODE: {
        status = pp[data_len];
        data_len++;

        if (status == BLE_ERR_NO_ERROR) {
            le_ctl_param.num_adv_sets = pp[data_len];
            virtual_hci_send_command(HCI_LE_RD_PER_ADV_LIST_SIZE_CMD_OPCODE, 0, NULL);
        }
    }
    break;

    case HCI_LE_RD_PER_ADV_LIST_SIZE_CMD_OPCODE: {
        status = pp[data_len];
        data_len++;

        if (status == BLE_ERR_NO_ERROR) {
            uint8_t data[2] = {0x20, 0x01};

            le_ctl_param.pal_size = pp[data_len];
            virtual_hci_send_command(HCI_LE_SET_HOST_FEATURE_CMD_OPCODE, 2, data);
        }
    }
    break;

    case HCI_LE_SET_HOST_FEATURE_CMD_OPCODE: {
        status = pp[data_len];
        data_len++;

        if (status == BLE_ERR_NO_ERROR) {
            virtual_hci_send_command(HCI_RD_BD_ADDR_CMD_OPCODE, 0, NULL);
        }
    }
    break;

    case HCI_RD_BD_ADDR_CMD_OPCODE: {
        status = pp[data_len];
        data_len++;

        if (status == BLE_ERR_NO_ERROR) {
            sys_memcpy(le_ctl_param.local_bd_addr, &pp[data_len], 6);
            app_virtual_hci_init_done();
        }
    }
    break;

    default:
        break;

    }

    if (status != BLE_ERR_NO_ERROR) {
        dbg_print(NOTICE, "cmd complete error opcode 0x%x, errno 0x%x \n\r", cmd_opcode, status);
    }

}


static void handle_le_event(uint8_t *pp)
{
    uint8_t sub_event = 0;
    uint8_t num_report;
    uint8_t i;
    uint16_t data_len = 0;

    if (pp == NULL) {
        return;
    }

    sub_event = pp[data_len];
    data_len++;

    switch (sub_event) {
    case HCI_LE_ADV_REPORT_EVT_SUBCODE: {
        num_report = pp[data_len];
        data_len++;
        for (i = 0; i < num_report; i++) {
            uint8_t adv_data_len = pp[data_len + 8];
            uint8_t rssi = pp[data_len + 9 + adv_data_len];
            dbg_print(NOTICE,
                      "adv_type %d, addr_type %d, address: 0x%02x : 0x%02x : 0x%02x : 0x%02x : 0x%02x : 0x%02x  rssi %d\r\n",
                      pp[data_len], pp[data_len + 1], pp[data_len + 2], pp[data_len + 3], pp[data_len + 4],
                      pp[data_len + 5], pp[data_len + 6], pp[data_len + 7],
                      rssi);

            data_len += (10 + adv_data_len);
        }
    }
    break;
    case HCI_LE_EXT_ADV_REPORT_EVT_SUBCODE: {
        num_report = pp[data_len];
        data_len++;

        for (i = 0; i < num_report; i++) {
            uint8_t comm_data[24] = {0};
            sys_memcpy(comm_data, &pp[data_len], 24);
            data_len += 24;

            dbg_print(NOTICE,
                      "adv_type 0x%02x, addr_type %d, address: 0x%02x : 0x%02x : 0x%02x : 0x%02x : 0x%02x : 0x%02x  rssi %d\r\n",
                      comm_data[0] | comm_data[1] << 8, comm_data[2], comm_data[3], comm_data[4], comm_data[5],
                      comm_data[6], comm_data[7], comm_data[8], comm_data[13]);

            data_len += (24 + comm_data[23]);
        }
    }
    break;

    }
}

static void app_handle_event(uint8_t *p_header, uint16_t payload_length)
{
    uint8_t *pp = sys_malloc(payload_length);

    if (pp == NULL || !virtual_hci_get_payload(pp, payload_length)) {
        goto done;
    }

    switch (p_header[0]) {
    case HCI_CMD_CMP_EVT_CODE: {
        uint16_t cmd_opcode;
        cmd_opcode = *((uint16_t *)(pp + 1));
        payload_length -= 3;
        app_handle_cmd_cmplt_event(cmd_opcode, (pp + 3));
    }
    break;
    case HCI_LE_META_EVT_CODE: {
        handle_le_event(pp);
    }
    break;
    }

done:
    if (pp) {
        sys_mfree(pp);
    }
}

static void app_handle_acl_data(uint8_t *p_header, uint16_t payload_length)
{

}

static void app_handle_iso_data(uint8_t *p_header, uint16_t payload_length)
{

}

static void app_handle_sco_data(uint8_t *p_header, uint16_t payload_length)
{
}

void virtual_hci_reset_cmd(int argc, char **argv)
{
    virtual_hci_send_command(HCI_RESET_CMD_OPCODE, 0, NULL);
}

void virtual_hci_set_ext_scan_param(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t cmd_data[13] = {0};
    uint16_t scan_interval = 0x0190;  //scan interval 250ms
    uint16_t scan_window = 0x00C8;       //scan window 125ms
    uint8_t count = 0;

    cmd_data[0] = 0x00;     //own public address
    cmd_data[1] = 0x00;     //scan policy acceept all
    cmd_data[2] = 0x01;     //scan phy 1M
    cmd_data[3] = 0x01;   //scan active

    cmd_data[4] = scan_interval & 0xFF;
    cmd_data[5] = (scan_interval >> 8) & 0xFF;
    cmd_data[6] = scan_window & 0xFF;
    cmd_data[7] = (scan_window >> 8) & 0xFF;


    if (argc > 1) {
        cmd_data[0] = (uint8_t)strtoul((const char *)argv[1], &endptr, 0);
    }

    if (argc > 2) {
        cmd_data[1] = (uint8_t)strtoul((const char *)argv[2], &endptr, 0);
    }

    if (argc > 3) {
        cmd_data[2] = (uint8_t)strtoul((const char *)argv[3], &endptr, 0);
    }

    if (argc > 4) {
        cmd_data[3] = (uint8_t)strtoul((const char *)argv[4], &endptr, 0);
    }

    if (argc > 5) {
        scan_interval = (uint16_t)strtoul((const char *)argv[5], &endptr, 0);
        cmd_data[4] = scan_interval & 0xFF;
        cmd_data[5] = (scan_interval >> 8) & 0xFF;
    }

    if (argc > 6) {
        scan_window = (uint16_t)strtoul((const char *)argv[6], &endptr, 0);
        cmd_data[6] = scan_window & 0xFF;
        cmd_data[7] = (scan_window >> 8) & 0xFF;
    }

    count = count_bits_1(cmd_data[2]);
    if (count == 2) {
        memcpy(&cmd_data[8], &cmd_data[3], 5);
    } else if (count < 1 || count > 2) {
        dbg_print(ERR, "cmd_set_ext_scan param error count %d \r\n", count);
        return;
    }

    virtual_hci_send_command(HCI_LE_SET_EXT_SCAN_PARAM_CMD_OPCODE, count == 2 ? 13 : 8, cmd_data);
}

void virtual_hci_set_ext_scan_enable(int argc, char **argv)
{
    char *endptr = NULL;
    uint8_t cmd_data[6];

    cmd_data[0] = 0x01;     //scan enable
    cmd_data[1] = 0x00;     //duplicated disabled
    cmd_data[2] = 0x00;
    cmd_data[3] = 0x00;   //duration continuously until explicity disable
    cmd_data[4] = 0x00;
    cmd_data[5] = 0x00;   //period scan continuously

    if (argc > 1) {
        cmd_data[0] = (uint8_t)strtoul((const char *)argv[1], &endptr, 0);
    }

    if (argc > 2) {
        cmd_data[1] = (uint8_t)strtoul((const char *)argv[2], &endptr, 0);
    }

    if (argc > 3) {
        uint16_t scan_duration = (uint16_t)strtoul((const char *)argv[3], &endptr, 0);
        cmd_data[2] = scan_duration & 0xFF;
        cmd_data[3] = (scan_duration >> 8) & 0xFF;
    }

    if (argc > 4) {
        uint16_t scan_period = (uint16_t)strtoul((const char *)argv[4], &endptr, 0);
        cmd_data[4] = scan_period & 0xFF;
        cmd_data[5] = (scan_period >> 8) & 0xFF;
    }

    virtual_hci_send_command(HCI_LE_SET_EXT_SCAN_EN_CMD_OPCODE, 6, cmd_data);
}


void app_virtual_hci_enable(void)
{
    virtual_hci_send_command(HCI_RESET_CMD_OPCODE, 0, NULL);
}

void app_virtual_hci_init(ble_uart_func_t **pp_uart_func)
{
    virtual_hci_init(recv_cb, pp_uart_func);
}

#endif /* CFG_VIRTUAL_HCI_MODE */
