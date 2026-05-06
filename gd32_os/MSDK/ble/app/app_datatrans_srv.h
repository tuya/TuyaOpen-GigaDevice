/*!
    \file    app_datatrans_srv.h
    \brief   Header file of Datatrans Service Server Application.

    \version 2024-07-2, V1.0.0, firmware for GD32VW55x
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

#ifndef _APP_DATATRANS_SRV_H_
#define _APP_DATATRANS_SRV_H_

#include <stdint.h>

/*!
    \brief      Start datatrans
    \param[in]  conidx: connection index
    \param[in]  baudrate: uart baudrate
    \param[out] none
    \retval     none
*/
void app_datatrans_start(uint8_t conidx, uint32_t baudrate);

/*!
    \brief      Init APP datatrans service server module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_datatrans_srv_init(void);

/*!
    \brief      Deinit APP datatrans service server module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_datatrans_srv_deinit(void);

/*!
    \brief      app datatrans uart rx dma irq handler
    \param[in]  dma_channel: dma channel
    \param[out] none
    \retval     none
*/
void app_datatrans_uart_rx_dma_irq_hdl(uint32_t dma_channel);
#endif // _APP_DATATRANS_SRV_H_
