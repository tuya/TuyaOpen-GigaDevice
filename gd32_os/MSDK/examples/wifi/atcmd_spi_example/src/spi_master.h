/*!
    \file    spi_master.h
    \brief   SPI master implementation.

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

#ifndef _SPI_MASTER_H_
#define _SPI_MASTER_H_

#include "app_cfg.h"
#define ATCMD_FIXED_LEN         128

// #define CONFIG_SPI_3_WIRED

//#define SPI_MASTER_DEBUG_PRINT

typedef struct {
    char cmd[64];
    size_t cmd_len;
    uint8_t *tx_buffer;
    size_t tx_buffer_len;
    uint16_t segment_len; // only valid in file transfer mode
    os_task_t task_handle;  // used to notify which task send this AT cmd
} at_cmd_send_info_t;

typedef struct {
    char ack[32];
    char *ack_buffer;
    uint8_t ack_size; // if not 0 means ack size is large than 16 bytes, the actual ack is in ack_buffer
    uint8_t *rx_buffer;
    size_t rx_buffer_len;
    int status;
} at_cmd_recv_info_t;

enum SPI_Master_stat_e {
    SPI_Master_Idle = 0,
    SPI_Master_AT_Sent,
    SPI_Master_AT_ACK,
    SPI_Master_Data_Sent,
    SPI_Master_Data_ACK,
    SPI_Master_Recv,
};

struct spi_manager_s {
    volatile uint8_t stat;
};
extern struct spi_manager_s spi_manager;

void spi_master_put_data(uint8_t *d, int size, uint8_t is_cmd);

int spi_hw_is_idle(void);
int spi_hw_is_in_atack(void);
int spi_manager_stat_get(void);
int at_spi_send_data_wait_rsp(uint8_t *data, int data_len, char *rsp, int rsp_len);
int at_spi_send_cmd_wait_rsp(char *cmd, int cmd_len, char *rsp, int rsp_len);
int at_spi_send_file_wait_rsp(uint8_t *data, int data_len, int segment_len, char *rsp, int rsp_len);
int at_spi_send_cmd_read_data(char *cmd, int cmd_len, at_cmd_recv_info_t *recv_info);
#endif // _SPI_MASTER_H_
