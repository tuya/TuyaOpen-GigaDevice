/*!
    \file    dma.c
    \brief   DMA utility functions for GD32VW55x SDK.

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

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stddef.h>
#include <string.h>
#include "dma.h"
#include "gd32vw55x.h"
/*!
    \brief      configure DMA
    \param[in]  none
    \param[out] none
    \retval     none
*/
void dma_config(void)
{
    dma_multi_data_parameter_struct  dma_init_parameter;

    /* peripheral clock enable */
    rcu_periph_clock_enable(RCU_DMA);

    /* DMA peripheral configure */
    dma_deinit(DMA_CH0);

    dma_init_parameter.periph_width = DMA_PERIPH_WIDTH_32BIT;
    dma_init_parameter.periph_inc = DMA_PERIPH_INCREASE_ENABLE;
    dma_init_parameter.memory_width = DMA_MEMORY_WIDTH_32BIT;
    dma_init_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_parameter.memory_burst_width = DMA_MEMORY_BURST_SINGLE;
    dma_init_parameter.periph_burst_width = DMA_PERIPH_BURST_SINGLE;
    dma_init_parameter.critical_value = DMA_FIFO_4_WORD;
    dma_init_parameter.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    dma_init_parameter.direction = DMA_MEMORY_TO_MEMORY;
    dma_init_parameter.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_multi_data_mode_init(DMA_CH0, &dma_init_parameter);
}

/*!
    \brief      transfer DMA
    \param[in]  channel: specify which DMA channel to get flag
      \arg        DMA_CHx(x=0..7)
    \param[in]  periph_addr:periphel address
    \param[in]  memory0_addr:memory0 address
    \param[in]  number:data number want  to transfer
    \param[out] none
    \retval     none
*/
void dma_transfer(uint32_t periph_addr, uint32_t memory0_addr, uint32_t number)
{
    dma_flag_clear(DMA_CH0, (DMA_FLAG_FTF | DMA_FLAG_HTF));

    /* DMA peripheral configure */
    dma_periph_address_config(DMA_CH0, periph_addr);
    dma_memory_address_config(DMA_CH0, DMA_MEMORY_0, memory0_addr);
    dma_transfer_number_config(DMA_CH0, number);

    dma_channel_enable(DMA_CH0);
    while(!dma_flag_get(DMA_CH0, DMA_FLAG_FTF));
}
