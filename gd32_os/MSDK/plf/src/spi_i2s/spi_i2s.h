/*!
    \file    spi_i2s.h
    \brief   SPI I2S for GD32VW55x SDK.

    \version 2024-09-29, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2024, GigaDevice Semiconductor Inc.

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

#ifndef _SPI_I2S_H
#define _SPI_I2S_H

#ifdef __cplusplus
extern "C" {
#endif

#include "wrapper_os.h"

typedef struct {
    uint32_t pcm_addr;
}  pcm_buf_info_t;

void spi_i2s_start_send(os_queue_t queue, uint32_t dma_addr0, uint32_t dma_addr1, uint32_t len);
void spi_i2s_stop_send(void);
void spi_i2s_init_config(void);
uint8_t spi_i2s_init_sample_rate(uint16_t sample_rate);

void spi_i2s_dma_irqhandler(void);
#ifdef I2S_RECORD
void spi_i2s_record_start(os_queue_t queue, uint32_t dma_addr0, uint32_t dma_addr1, uint32_t len);
#endif

#ifdef __cplusplus
}
#endif

#endif // _SPI_I2S_H