/*!
    \file    gd32vw55x_it.h
    \brief   the header file of the ISR

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

#ifndef GD32VW55X_IT_H
#define GD32VW55X_IT_H

#ifdef __cplusplus
 extern "C" {
#endif

void USART0_IRQHandler(void);
void UART1_IRQHandler(void);
void UART2_IRQHandler(void);
void WIFI_WKUP_IRQHandler(void);
void WIFI_INT_IRQHandler(void);
void WIFI_INTGEN_IRQHandler(void);
void WIFI_PROT_IRQHandler(void);
void WIFI_RX_IRQHandler(void);
void WIFI_TX_IRQHandler(void);
void I2C0_EV_IRQHandler(void);
void I2C0_ER_IRQHandler(void);
void I2C1_EV_IRQHandler(void);
void I2C1_ER_IRQHandler(void);

void BLE_POWER_STATUS_IRQHandler(void);
void BLE_WKUP_IRQHandler(void);
void BLE_HALF_SLOT_IRQHandler(void);
void BLE_SLEEP_MODE_IRQHandler(void);
void BLE_ENCRYPTION_ENGINE_IRQHandler(void);
void BLE_SW_TRIG_IRQHandler(void);
void BLE_FINE_TIMER_TARGET_IRQHandler(void);
void BLE_STAMP_TARGET1_IRQHandler(void);
void BLE_STAMP_TARGET2_IRQHandler(void);
void BLE_STAMP_TARGET3_IRQHandler(void);
void BLE_FREQ_SELECT_IRQHandler(void);
void BLE_ERROR_IRQHandler(void);
void BLE_FIFO_ACTIVITY_IRQHandler(void);

void DMA_Channel1_IRQHandler(void);
void RTC_WKUP_IRQHandler(void);
void EXTI5_9_IRQHandler(void);

void SPI_IRQHandler(void);
#ifdef __cplusplus
}
#endif

#endif /* GD32VW55X_IT_H */
