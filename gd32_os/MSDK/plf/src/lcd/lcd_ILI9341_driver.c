/*!
    \file  lcd_driver.c
    \brief lcd driver functions

    \version 2025-02-19, V1.4.0, demo for GD32W51x
*/

/*
    Copyright (c) 2025, GigaDevice Semiconductor Inc.

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

#include "gd32vw55x.h"
#include "lcd_ILI9341_driver.h"
#include "wrapper_os.h"
#include "ll.h"
#include "dbg_print.h"


static void lcd_spi_init(void);
static void lcd_write_index(uint8_t index);
static void lcd_write_data(uint8_t data);
static void lcd_write_data_16bit(uint8_t datah, uint8_t datal);
static void lcd_reset(void);

/*!
    \brief      send a byte through the SPI interface and return a byte received from the SPI bus
    \param[in]  byte: data to be send
    \param[out] none
    \retval     the value of the received byte
*/
uint8_t lcd_spi_write_byte(uint8_t byte)
{
    while (RESET == (SPI_STAT & SPI_FLAG_TBE));
    SPI_DATA = byte;

    while (RESET == (SPI_STAT & SPI_FLAG_RBNE));
    return (SPI_DATA);
}
/*!
    \brief      send a halfword through the SPI interface and return a halfwold received from the SPI bus
    \param[in]  spi_periph: SPIx(x=0,1)
    \param[in]  halfword: data to be send
    \param[out] none
    \retval     the value of the received halfword
*/
uint16_t spi_write_halfword(uint16_t halfword)
{
    while (RESET == (SPI_STAT & SPI_FLAG_TBE));
    SPI_DATA = halfword;

    while (RESET == (SPI_STAT & SPI_FLAG_RBNE));
    return (SPI_DATA);
}

#if (USE_DMA_LCD==1)
uint16_t rx_array[1];
/*!
    \brief      configure LCD DMA
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void lcd_dma_config(void)
{
    dma_multi_data_parameter_struct dma_init_parameter;

    /* peripheral clock enable */
    rcu_periph_clock_enable(RCU_DMA);

    /* DMA peripheral configure */
    dma_deinit(DMA_CH3);
    dma_multi_data_para_struct_init(&dma_init_parameter);
    dma_init_parameter.memory0_addr         = 0U;
    dma_init_parameter.memory_width         = DMA_MEMORY_WIDTH_16BIT;
    dma_init_parameter.memory_inc           = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_parameter.memory_burst_width   = DMA_MEMORY_BURST_4_BEAT;
    dma_init_parameter.periph_addr          = (uint32_t)&SPI_DATA;
    dma_init_parameter.periph_width         = DMA_PERIPH_WIDTH_16BIT;
    dma_init_parameter.periph_inc           = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_parameter.periph_burst_width   = DMA_PERIPH_BURST_SINGLE;
    dma_init_parameter.direction            = DMA_MEMORY_TO_PERIPH;
    dma_init_parameter.critical_value       = DMA_FIFO_2_WORD;
    dma_init_parameter.circular_mode        = DMA_CIRCULAR_MODE_DISABLE;
    dma_init_parameter.number               = 0;
    dma_init_parameter.priority             = DMA_PRIORITY_HIGH;
    dma_multi_data_mode_init(DMA_CH3, &dma_init_parameter);
    /* connect DMA0_CH4 to SPI TX */
    dma_channel_subperipheral_select(DMA_CH3, DMA_SUBPERI3);
    /* interrupt configure */
    dma_interrupt_enable(DMA_CH3, DMA_INT_FTF);
    eclic_irq_enable(DMA_Channel3_IRQn, 8, 0);

    /* DMA peripheral configure */
    dma_deinit(DMA_CH2);
    dma_multi_data_para_struct_init(&dma_init_parameter);
    dma_init_parameter.memory0_addr         = (uint32_t)rx_array;
    dma_init_parameter.memory_width         = DMA_MEMORY_WIDTH_16BIT;
    dma_init_parameter.memory_inc           = DMA_MEMORY_INCREASE_DISABLE;
    dma_init_parameter.memory_burst_width   = DMA_MEMORY_BURST_SINGLE;
    dma_init_parameter.periph_addr          = (uint32_t)&SPI_DATA;
    dma_init_parameter.periph_width         = DMA_PERIPH_WIDTH_16BIT;
    dma_init_parameter.periph_inc           = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_parameter.periph_burst_width   = DMA_PERIPH_BURST_SINGLE;
    dma_init_parameter.direction            = DMA_PERIPH_TO_MEMORY;
    dma_init_parameter.critical_value       = DMA_FIFO_1_WORD;
    dma_init_parameter.circular_mode        = DMA_CIRCULAR_MODE_DISABLE;
    dma_init_parameter.number               = 0U;
    dma_init_parameter.priority             = DMA_PRIORITY_LOW;
    dma_multi_data_mode_init(DMA_CH2, &dma_init_parameter);
    /* connect DMA0_CH4 to SPI RX */
    dma_channel_subperipheral_select(DMA_CH2, DMA_SUBPERI3);
    /* interrupt configure */
    dma_interrupt_enable(DMA_CH2, DMA_INT_FTF);
    eclic_irq_enable(DMA_Channel2_IRQn, 8, 0);
}
#endif

/*!
    \brief      initialize SPI0
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void lcd_spi_init(void)
{
    spi_parameter_struct spi_init_struct;
    rcu_periph_clock_enable(RCU_SPI);
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);

#if CONFIG_BOARD == PLATFORM_BOARD_32VW55X_SONIC
    /* configure SPI GPIO: SCK/PA2, MISO/PA1, MOSI/PA0 */
    gpio_af_set(GPIOA, GPIO_AF_5, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX,
                            GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);

    /* configure GPIOB REST/PB0*/
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_0);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_0);

    /* configure GPIOC DC/RS PC15*/
    gpio_mode_set(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_15);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, GPIO_PIN_15); // Only use 2MHZ for PC13 ~ PC15

    /* configure GPIOA NSS/PA3*/
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_3);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_3);

#else
    /* configure SPI GPIO: SCK/PA11, MISO/PA10, MOSI/PA9 */
    gpio_af_set(GPIOA, GPIO_AF_0, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ,
                            GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11);

    /* configure GPIOB */
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_12 | GPIO_PIN_13);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_12 | GPIO_PIN_13);

    /* configure GPIOA */
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_12);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_12);
#endif

    /* configure SPI parameter */
    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode          = SPI_MASTER;
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
    spi_init_struct.nss                  = SPI_NSS_SOFT;
    spi_init_struct.prescale             = SPI_PSC_4;
    spi_init_struct.endian               = SPI_ENDIAN_MSB;
    spi_init(&spi_init_struct);


#if (USE_DMA_LCD==1)
    lcd_dma_config();
    spi_dma_enable(SPI_DMA_TRANSMIT);
    spi_dma_enable(SPI_DMA_RECEIVE);
#endif
    /* set crc polynomial */
    spi_crc_polynomial_set(7);
    spi_enable();
}

/*!
    \brief      write the register address
    \param[in]  index: the value of register address to be written
    \param[out] none
    \retval     none
*/
static void lcd_write_index(uint8_t index)
{
    LCD_RS_CLR;
    lcd_spi_write_byte(index);
}

/*!
    \brief      write the register data
    \param[in]  data: the value of register data to be written
    \param[out] none
    \retval     none
*/
static void lcd_write_data(uint8_t data)
{
    LCD_RS_SET;
    lcd_spi_write_byte(data);
}

/*!
    \brief      write the register data(an unsigned 16-bit data)
    \param[in]  datah: the high 8bit of register data to be written
    \param[in]  datal: the low 8bit of register data to be written
    \param[out] none
    \retval     none
*/
static void lcd_write_data_16bit(uint8_t datah, uint8_t datal)
{
    lcd_write_data(datah);
    lcd_write_data(datal);
}


/*!
    \brief      reset the lcd
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void lcd_reset(void)
{
    LCD_RST_CLR;
    sys_ms_sleep(100);
    LCD_RST_SET;
    sys_ms_sleep(50);
}

void lcd_config(void)
{
    LCD_CS_CLR;
    /* write the register address 0xCB(Power control A)*/
    lcd_write_index(POWER_CTRL_A_CMD);
    lcd_write_data(0x39);
    lcd_write_data(0x2C);
    lcd_write_data(0x00);
    lcd_write_data(0x34);
    lcd_write_data(0x02);

    /* write the register address 0xCF(Power control B)*/
    lcd_write_index(POWER_CTRL_B_CMD);
    lcd_write_data(0x00);
    lcd_write_data(0XC1);
    lcd_write_data(0X30);

    /* write the register address 0xE8(Driver timing control A)*/
    lcd_write_index(DRIVER_TIM_CTRL_A_CMD);
    lcd_write_data(0x85);
    lcd_write_data(0x00);
    lcd_write_data(0x78);

    /* write the register address 0xEA(Driver timing control B)*/
    lcd_write_index(DRIVER_TIM_CTRL_B_CMD);
    lcd_write_data(0x00);
    lcd_write_data(0x00);

    /* write the register address 0xED(Power on sequence control)*/
    lcd_write_index(POWER_ON_SEQ_CTRL_CMD);
    lcd_write_data(0x64);
    lcd_write_data(0x03);
    lcd_write_data(0X12);
    lcd_write_data(0X81);

    /* write the register address 0xF7(Pump ratio control)*/
    lcd_write_index(PUMP_RATION_CTRL_CMD);
    lcd_write_data(0x20);

    /* power control VRH[5:0] */
    lcd_write_index(POWER_CTRL_1_CMD);
    lcd_write_data(0x23);

    /* power control SAP[2:0];BT[3:0] */
    lcd_write_index(POWER_CTRL_2_CMD);
    lcd_write_data(0x10);

    /* vcom control */
    lcd_write_index(VCOM_CTRL_1_CMD);
    lcd_write_data(0x3e);
    lcd_write_data(0x28);

    /* vcom control2 */
    lcd_write_index(VCOM_CTRL_2_CMD);
    lcd_write_data(0x86);

    lcd_write_index(MEM_ACC_CTRL_CMD);
#ifdef H_VIEW
    lcd_write_data(0xE8);
#else
    lcd_write_data(0x48);
#endif

    /* write the register address 0x3A(COLMOD: Pixel Format Set)*/
    lcd_write_index(COL_MOD_SET_CMD);
    lcd_write_data(0x55);

    /* write the register address 0xB1(Frame Rate Control (In Normal Mode/Full Colors) )*/
    lcd_write_index(FM_RATE_CTRL_CMD);
    lcd_write_data(0x00);
    lcd_write_data(0x18);

    /* display function control */
    lcd_write_index(DIS_FUNC_CTRL_CMD);
    lcd_write_data(0x08);
    lcd_write_data(0x82);
    lcd_write_data(0x27);

    /* 3gamma function disable */
    lcd_write_index(ENABLE_3G_CMD);
    lcd_write_data(0x00);

    /* gamma curve selected  */
    lcd_write_index(GAMMA_SET_CMD);
    lcd_write_data(0x01);

    /* set gamma  */
    lcd_write_index(POS_GAMMA_CORR_CMD);
    lcd_write_data(0x0F);
    lcd_write_data(0x31);
    lcd_write_data(0x2B);
    lcd_write_data(0x0C);
    lcd_write_data(0x0E);
    lcd_write_data(0x08);
    lcd_write_data(0x4E);
    lcd_write_data(0xF1);
    lcd_write_data(0x37);
    lcd_write_data(0x07);
    lcd_write_data(0x10);
    lcd_write_data(0x03);
    lcd_write_data(0x0E);
    lcd_write_data(0x09);
    lcd_write_data(0x00);

    /* set gamma  */
    lcd_write_index(NEG_GAMMA_CORR_CMD);
    lcd_write_data(0x00);
    lcd_write_data(0x0E);
    lcd_write_data(0x14);
    lcd_write_data(0x03);
    lcd_write_data(0x11);
    lcd_write_data(0x07);
    lcd_write_data(0x31);
    lcd_write_data(0xC1);
    lcd_write_data(0x48);
    lcd_write_data(0x08);
    lcd_write_data(0x0F);
    lcd_write_data(0x0C);
    lcd_write_data(0x31);
    lcd_write_data(0x36);
    lcd_write_data(0x0F);

    /* exit sleep */
    lcd_write_index(SLEEP_OUT_CMD);
    sys_ms_sleep(120);

    /* display on */
    lcd_write_index(DISPLAY_ON_CMD);
    LCD_CS_SET;
}

/*!
    \brief      initialize the lcd
    \param[in]  none
    \param[out] none
    \retval     none
*/
void lcd_init(void)
{
    lcd_spi_init();

    LCD_CS_CLR;
    lcd_reset();

    LCD_CS_SET;

}

/*!
    \brief      set lcd display region
    \param[in]  x_start: the x position of the start point
    \param[in]  y_start: the y position of the start point
    \param[in]  x_end: the x position of the end point
    \param[in]  y_end: the y position of the end point
    \param[out] none
    \retval     none
*/
void lcd_set_region(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{
    LCD_CS_CLR;

    /* write the register address 0x2A*/
    lcd_write_index(COLUMN_ADDR_SET_CMD);
    lcd_write_data_16bit(x_start >> 8, x_start);
    lcd_write_data_16bit(x_end >> 8, x_end);

    /* write the register address 0x2B*/
    lcd_write_index(PAGE_ADDR_SET_CMD);
    lcd_write_data_16bit(y_start >> 8, y_start);
    lcd_write_data_16bit(y_end >> 8, y_end);

    /* write the register address 0x2C*/
    lcd_write_index(MEMORY_WRITE_CMD);
    LCD_CS_SET;
}

/*!
    \brief      set the start display point of lcd
    \param[in]  x: the x position of the start point
    \param[in]  y: the y position of the start point
    \param[out] none
    \retval     none
*/
void lcd_set_xy(uint16_t x, uint16_t y)
{
    /* write the register address 0x2A*/
    lcd_write_index(0x2A);
    lcd_write_data_16bit(x >> 8, x);

    /* write the register address 0x2B*/
    lcd_write_index(0x2B);
    lcd_write_data_16bit(y >> 8, y);

    /* write the register address 0x2C*/
    lcd_write_index(0x2C);
}

/*!
    \brief      draw a point on the lcd
    \param[in]  x: the x position of the point
    \param[in]  y: the y position of the point
    \param[in]  data: the register data to be written
    \param[out] none
    \retval     none
*/
void gui_draw_point(uint16_t x, uint16_t y, uint16_t data)
{
    lcd_set_xy(x, y);
    lcd_write_data(data >> 8);
    lcd_write_data(data);
}

/*!
    \brief      clear the lcd
    \param[in]  color: lcd display color
    \param[out] none
    \retval     none
*/
void lcd_clear(uint16_t color)
{
    unsigned int i, m;
    /* set lcd display region */
    lcd_set_region(0, 0, X_MAX_PIXEL - 1, Y_MAX_PIXEL - 1);
    LCD_RS_SET;

    LCD_CS_CLR;
    for (i = 0; i < Y_MAX_PIXEL; i ++) {
        for (m = 0; m < X_MAX_PIXEL; m ++) {
            lcd_spi_write_byte(color >> 8);
            lcd_spi_write_byte(color);
        }
    }
    LCD_CS_SET;
}
