/*!
    \file    spi.c
    \brief   SPI BSP for GD32VW55x SDK.

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

#include <ctype.h>
#include "gd32vw55x.h"
#include <stdio.h>
#include "spi.h"
#include "wrapper_os.h"
#include "wakelock.h"
#include "systime.h"

#if 1//defined(CONFIG_ATCMD_SPI) || defined(SPI_ROLE_MASTER)

void spi_dma_single_mode_config(uint32_t direction)
{
    dma_single_data_parameter_struct dma_init_struct;
    dma_channel_enum dma_chnlx;

    dma_single_data_para_struct_init(&dma_init_struct);
    dma_init_struct.direction = direction;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;

    if (direction == DMA_MEMORY_TO_PERIPH) {
        dma_init_struct.periph_addr = (uint32_t)&SPI_DATA;
        dma_init_struct.priority = DMA_PRIORITY_LOW;
    } else if (direction == DMA_PERIPH_TO_MEMORY) {
        dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
        dma_init_struct.periph_addr = (uint32_t)&SPI_DATA;
    } else {
        return;
    }

    if (direction == DMA_MEMORY_TO_PERIPH) {
        dma_chnlx = SPI_TX_DMA_CH;
    } else if (direction == DMA_PERIPH_TO_MEMORY) {
        dma_chnlx = SPI_RX_DMA_CH;
    }

    dma_deinit(dma_chnlx);
    dma_single_data_mode_init(dma_chnlx, &dma_init_struct);

    dma_circulation_disable(dma_chnlx);
    dma_channel_subperipheral_select(dma_chnlx, DMA_SUBPERI3);
    dma_flow_controller_config(dma_chnlx, DMA_FLOW_CONTROLLER_DMA);

    dma_interrupt_enable(dma_chnlx, DMA_INT_FTF);
}

uint8_t spi_dma_tx_dummy = 0x7e;//0x12;
uint8_t spi_dma_rx_dummy = 0x02;

void spi_dma_dummy_mode_config(uint32_t direction)
{
    dma_single_data_parameter_struct dma_init_struct;
    dma_channel_enum dma_chnlx = DMA_CH0;

    dma_single_data_para_struct_init(&dma_init_struct);
    dma_init_struct.direction = direction;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_DISABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;

    if (direction == DMA_MEMORY_TO_PERIPH) {
        dma_init_struct.periph_addr = (uint32_t)&SPI_DATA;
        dma_init_struct.priority = DMA_PRIORITY_LOW;
    } else if (direction == DMA_PERIPH_TO_MEMORY) {
        dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
        dma_init_struct.periph_addr = (uint32_t)&SPI_DATA;
    } else {
        return;
    }

    if (direction == DMA_MEMORY_TO_PERIPH) {
        dma_chnlx = SPI_TX_DMA_CH;
    } else if (direction == DMA_PERIPH_TO_MEMORY) {
        dma_chnlx = SPI_RX_DMA_CH;
    }

    dma_deinit(dma_chnlx);
    dma_single_data_mode_init(dma_chnlx, &dma_init_struct);

    dma_circulation_disable(dma_chnlx);
    dma_channel_subperipheral_select(dma_chnlx, DMA_SUBPERI3);
    dma_flow_controller_config(dma_chnlx, DMA_FLOW_CONTROLLER_DMA);

    dma_interrupt_enable(dma_chnlx, DMA_INT_FTF);
    if (direction == DMA_MEMORY_TO_PERIPH) {
        dma_memory_address_config(dma_chnlx, DMA_MEMORY_0, (uint32_t)&spi_dma_tx_dummy);
    } else {
        dma_memory_address_config(dma_chnlx, DMA_MEMORY_0, (uint32_t)&spi_dma_rx_dummy);
    }
}

static void spi_slave_pin_config(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_SPI);

#ifndef CONFIG_SPI_3_WIRED
    /* SPI GPIO config:MOSI/PA0, MISO/PA1, SCK/PA2, NSS/PA3 */
    gpio_af_set(SPI_SCK_GPIO, SPI_AF_NUM, SPI_MOSI_PIN |SPI_MISO_PIN |SPI_SCK_PIN | SPI_NSS_PIN);
    gpio_mode_set(SPI_SCK_GPIO, GPIO_MODE_AF, GPIO_PUPD_NONE, SPI_MOSI_PIN |SPI_MISO_PIN |SPI_SCK_PIN | SPI_NSS_PIN);
    gpio_output_options_set(SPI_SCK_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, SPI_MOSI_PIN |SPI_MISO_PIN |SPI_SCK_PIN | SPI_NSS_PIN);
#else
    /* SPI GPIO config:MOSI/PA0, MISO/PA1, SCK/PA2 */
    gpio_af_set(SPI_SCK_GPIO, SPI_AF_NUM, SPI_MOSI_PIN |SPI_MISO_PIN |SPI_SCK_PIN);
    gpio_mode_set(SPI_SCK_GPIO, GPIO_MODE_AF, GPIO_PUPD_NONE, SPI_MOSI_PIN |SPI_MISO_PIN |SPI_SCK_PIN);
    gpio_output_options_set(SPI_SCK_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, SPI_MOSI_PIN |SPI_MISO_PIN |SPI_SCK_PIN);
#endif
}

void spi_slave_init(void)
{
    spi_parameter_struct spi_init_struct;

    rcu_periph_clock_enable(RCU_DMA);
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_SPI);

    spi_slave_pin_config();
    spi_deinit();
    spi_struct_para_init(&spi_init_struct);

    /* SPI0 parameter config */
    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode          = SPI_SLAVE;
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
    spi_init_struct.prescale             = SPI_PSC_4;
    spi_init_struct.endian               = SPI_ENDIAN_MSB;

#ifndef CONFIG_SPI_3_WIRED
    spi_init_struct.nss         = SPI_NSS_HARD;
#else
    spi_init_struct.nss         = SPI_NSS_SOFT;
    spi_nss_internal_low();
#endif

    /* Enable Rx irq Only in duplex mode */
    eclic_irq_enable(SPI_RX_DMA_CH_IRQn, 0x9, 0);
    /* Enable Tx irq Only in duplex mode */
    eclic_irq_enable(SPI_TX_DMA_CH_IRQn, 0x9, 0);

    spi_init(&spi_init_struct);
    spi_enable();
}

void spi_dma_config(bool dma_rx, uint32_t rx_mem,
                    bool dma_tx, uint32_t tx_mem, uint32_t dma_num, bool from_isr)
{
    /* CRITICAL: Must disable DMA channels BEFORE clearing flags and reconfiguring */
    dma_channel_disable(SPI_RX_DMA_CH);
    dma_channel_disable(SPI_TX_DMA_CH);

    /* Disable SPI DMA to ensure clean state */
    spi_dma_disable(SPI_DMA_RECEIVE);
    spi_dma_disable(SPI_DMA_TRANSMIT);

    spi_crc_error_clear();
    dma_interrupt_flag_clear(SPI_RX_DMA_CH, DMA_INT_FLAG_FTF);
    dma_interrupt_flag_clear(SPI_TX_DMA_CH, DMA_INT_FLAG_FTF);

    if (dma_rx) {
        spi_dma_single_mode_config(DMA_PERIPH_TO_MEMORY);
        dma_memory_address_config(SPI_RX_DMA_CH, DMA_MEMORY_0, rx_mem);
    } else {
        spi_dma_dummy_mode_config(DMA_PERIPH_TO_MEMORY);
    }

    if (dma_tx) {
        spi_dma_single_mode_config(DMA_MEMORY_TO_PERIPH);
        dma_memory_address_config(SPI_TX_DMA_CH, DMA_MEMORY_0, tx_mem);
    } else {
        spi_dma_dummy_mode_config(DMA_MEMORY_TO_PERIPH);
    }

    dma_transfer_number_config(SPI_RX_DMA_CH, dma_num);
    dma_transfer_number_config(SPI_TX_DMA_CH, dma_num);

    /* CRITICAL: Enable DMA channels and SPI DMA in one atomic operation */
    if (from_isr == 0)
        sys_enter_critical();

    /* Enable DMA channels first */
    dma_channel_enable(SPI_RX_DMA_CH);
    dma_channel_enable(SPI_TX_DMA_CH);

    /* Then enable SPI DMA requests atomically to ensure sync */
    spi_dma_enable(SPI_DMA_RECEIVE);
    spi_dma_enable(SPI_DMA_TRANSMIT);

    if (from_isr == 0)
        sys_exit_critical();

    /* CRITICAL: Small delay to ensure DMA hardware is fully ready before Master starts sending
     * Without this, if Master responds too quickly after handshake, first clock cycle
     * may arrive before DMA captures it, causing rx:5/6 timeout */
    if (from_isr == 0) {
        sys_us_delay(5);
    }
}

FlagStatus spi_nss_status_get(void)
{
    return gpio_input_bit_get(SPI_NSS_GPIO, SPI_NSS_PIN);
}

void spi_tx_idle_wait(void)
{
    while (RESET == spi_flag_get(SPI_FLAG_TBE));
}

void spi_put_data(const uint8_t *d, int size)
{
    if (size == 0) {
        return;
    }

    while (1) {
        while (RESET == spi_flag_get(SPI_FLAG_TBE));
        spi_data_transmit(*d++);
        size--;
        if (size == 0) {
            return;
        }
    }
}

void spi_putc_noint(uint8_t c)
{
    uint16_t putc = c;

    while (RESET == spi_flag_get(SPI_FLAG_TBE));
    spi_data_transmit(putc);
}

void spi_rx_flush(void)
{
    while (RESET != spi_flag_get(SPI_FLAG_RBNE)) {
        spi_data_receive();
    }
}

void spi_receivec(char *ch)
{
    uint16_t c;

    while (RESET == spi_flag_get(SPI_FLAG_RBNE));
    c = spi_data_receive();

    *ch = (char)c;
}

int spi_receivec_with_timeout(char *ch, int timeout)
{
    uint16_t c;

    while (timeout-- > 0) {
        if (RESET != spi_flag_get(SPI_FLAG_RBNE)) {
            c = spi_data_receive();
            *ch = (char)c;
            return 0;
        }
    }

    return 1;
}

/*!
    \brief      configure the GPIO used when SPI slave send to master
    \param[in]  none
    \param[out] none
    \retval     none
*/
void spi_handshake_gpio_config(void)
{
    /* SPI trigger GPIO config:PA5 */
    gpio_mode_set(SPI_HANDSHAKE_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SPI_HANDSHAKE_PIN);
    gpio_output_options_set(SPI_HANDSHAKE_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, SPI_HANDSHAKE_PIN);
    gpio_bit_reset(SPI_HANDSHAKE_GPIO, SPI_HANDSHAKE_PIN);
}

#ifdef CONFIG_SPI_3_WIRED
void spi_nss_gpio_config(void)
{
    /* SPI trigger GPIO config:PA5 */
    gpio_mode_set(SPI_NSS_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SPI_NSS_PIN);
    gpio_output_options_set(SPI_NSS_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, SPI_NSS_PIN);
    gpio_bit_reset(SPI_NSS_GPIO, SPI_NSS_PIN);
}

void spi_nss_rising_trigger(bool from_isr)
{
    /* CRITICAL: Move printf OUTSIDE critical section to avoid blocking in ISR context */
    if (from_isr == 0) {
        sys_enter_critical();
    }
    gpio_bit_set(SPI_NSS_GPIO, SPI_NSS_PIN);
    sys_us_delay(30);
    gpio_bit_reset(SPI_NSS_GPIO, SPI_NSS_PIN);
    if (from_isr == 0)
        sys_exit_critical();
}
#endif

void spi_handshake_rising_trigger(void)
{
    sys_enter_critical();
    gpio_bit_set(SPI_HANDSHAKE_GPIO, SPI_HANDSHAKE_PIN);
    sys_us_delay(50);
    gpio_bit_reset(SPI_HANDSHAKE_GPIO, SPI_HANDSHAKE_PIN);
    sys_exit_critical();
}
#endif
