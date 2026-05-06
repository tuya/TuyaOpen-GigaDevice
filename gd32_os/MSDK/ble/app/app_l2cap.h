/*!
    \file    app_l2cap.h
    \brief   Header file of l2cap Application Module entry point.

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

#ifndef _APP_L2CAP_H_
#define _APP_L2CAP_H_

#include <stdint.h>

/*!
    \brief      Init application L2CAP module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_l2cap_mgr_init(void);

/*!
    \brief      Deinit application L2CAP module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_l2cap_mgr_deinit(void);

/*!
    \brief      Reset application L2CAP module
    \param[in]  none
    \param[out] none
    \retval     none
*/
void app_l2cap_reset(void);

/*!
    \brief      Set application L2CAP channel number
    \param[in]  nb_chan: channel number to set
    \param[out] none
    \retval     none
*/
void app_l2cap_set_nb_chan(uint8_t nb_chan);

/*!
    \brief      Add Simplified Protocol/Service Multiplexer
    \param[in]  spsm: Simplified Protocol/Service Multiplexer
    \param[in]  sec_lvl_bf: security level bit field @ref ble_l2cap_sec_lvl_bf
    \param[out] none
    \retval     none
*/
void app_l2cap_spsm_add(uint16_t spsm, uint8_t sec_lvl_bf);

/*!
    \brief      Remove Simplified Protocol/Service Multiplexer
    \param[in]  spsm: Simplified Protocol/Service Multiplexer
    \param[out] none
    \retval     none
*/
void app_l2cap_spsm_remove(uint16_t spsm);

/*!
    \brief      Enable enhanced L2CAP COC negotiations
    \param[in]  conidx: connection index
    \param[out] none
    \retval     none
*/
void app_l2cap_coc_enhanced_enable(uint8_t conidx);

/*!
    \brief      Create a L2CAP credit oriented connection
    \param[in]  conidx: connection index
    \param[in]  local_rx_mtu: local device reception maximum transmit unit size
    \param[in]  nb_chan: number of L2CAP channel
    \param[in]  spsm: Simplified Protocol/Service Multiplexer
    \param[in]  enhanced: true to use enhanced L2CAP COC negotiation, otherwise false
    \param[out] none
    \retval     none
*/
void app_l2cap_con_create(uint8_t conidx, uint16_t local_rx_mtu, uint8_t nb_chan, uint16_t spsm, uint8_t enhanced);

/*!
    \brief      Reconfig a L2CAP credit oriented connection parameter
    \param[in]  conidx: connection index
    \param[in]  nb_chan: number of L2CAP channels to reconfigure
    \param[in]  local_rx_mtu: local device rx mtu to reconfigure
    \param[in]  local_rx_mps: local device rx mps to reconfigure
    \param[out] none
    \retval     none
*/
void app_l2cap_con_reconfig(uint8_t conidx, uint8_t nb_chan, uint16_t local_rx_mtu, uint16_t local_rx_mps);

/*!
    \brief      Terminate a L2CAP credit oriented connection
    \param[in]  conidx: connection index
    \param[in]  chan_lid: L2CAP channel identifier
    \param[out] none
    \retval     none
*/
void app_l2cap_con_terminate(uint8_t conidx, uint8_t chan_lid);

/*!
    \brief      Transmit L2CAP segment packet
    \param[in]  conidx: connection index
    \param[in]  chan_lid: L2CAP channel identifier
    \param[in]  dbg_bf: debug bitfield
    \param[in]  length: transmit data length
    \param[out] none
    \retval     none
*/
void app_l2cap_sdu_send(uint8_t conidx, uint8_t chan_lid, uint8_t dbg_bf, uint16_t length);

#endif // _APP_L2CAP_H_
