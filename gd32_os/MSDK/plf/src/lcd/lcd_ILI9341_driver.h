/*!
    \file  lcd_driver.h
    \brief the header file of lcd driver

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

#ifndef LCD_ILI9341_DRIVER_H
#define LCD_ILI9341_DRIVER_H

#include <stdlib.h>
#include "gd32vw55x.h"
#include <app_cfg.h>

#define H_VIEW

#ifdef H_VIEW
#define X_MAX_PIXEL         (uint16_t)320
#define Y_MAX_PIXEL         (uint16_t)240
#else
#define X_MAX_PIXEL         (uint16_t)240
#define Y_MAX_PIXEL         (uint16_t)320
#endif


#define USE_DMA_LCD        1

#define RED             (uint16_t)0xF800
#define GREEN           (uint16_t)0x07E0
#define BLUE            (uint16_t)0x001F
#define WHITE           (uint16_t)0xFFFF
#define BLACK           (uint16_t)0x0000
#define YELLOW          (uint16_t)0xFFE0

/* grays */
#define GRAY0           (uint16_t)0xEF7D
#define GRAY1           (uint16_t)0x8410
#define GRAY2           (uint16_t)0x4208

#if CONFIG_BOARD == PLATFORM_BOARD_32VW55X_SONIC
/* PA3 tft cs */
#define LCD_CS_SET      ((uint32_t)(GPIO_BOP(GPIOA) = GPIO_PIN_3))
#define LCD_CS_CLR      ((uint32_t)(GPIO_BC(GPIOA) = GPIO_PIN_3))

/* PC15 tft rs/dc */
#define LCD_RS_SET      ((uint32_t)(GPIO_BOP(GPIOC) = GPIO_PIN_15))
#define LCD_RS_CLR      ((uint32_t)(GPIO_BC(GPIOC) = GPIO_PIN_15))

/* PB0 tft rst */
#define LCD_RST_SET     ((uint32_t)(GPIO_BOP(GPIOB) = GPIO_PIN_0))
#define LCD_RST_CLR     ((uint32_t)(GPIO_BC(GPIOB) = GPIO_PIN_0))
#else
/* PA12 tft cs */
#define LCD_CS_SET      ((uint32_t)(GPIO_BOP(GPIOA) = GPIO_PIN_12))
#define LCD_CS_CLR      ((uint32_t)(GPIO_BC(GPIOA) = GPIO_PIN_12))

/* PB13 tft rs/dc */
#define LCD_RS_SET      ((uint32_t)(GPIO_BOP(GPIOB) = GPIO_PIN_13))
#define LCD_RS_CLR      ((uint32_t)(GPIO_BC(GPIOB) = GPIO_PIN_13))

/* PB12 tft rst */
#define LCD_RST_SET     ((uint32_t)(GPIO_BOP(GPIOB) = GPIO_PIN_12))
#define LCD_RST_CLR     ((uint32_t)(GPIO_BC(GPIOB) = GPIO_PIN_12))
#endif


#define SLEEP_OUT_CMD             0x11
#define GAMMA_SET_CMD             0x26
#define DISPLAY_ON_CMD            0x29
#define COLUMN_ADDR_SET_CMD       0x2A
#define PAGE_ADDR_SET_CMD         0x2B
#define MEMORY_WRITE_CMD          0x2C
#define MEM_ACC_CTRL_CMD          0x36
#define COL_MOD_SET_CMD           0x3A
#define FM_RATE_CTRL_CMD          0xB1
#define DIS_FUNC_CTRL_CMD         0xB6
#define POWER_CTRL_1_CMD          0xC0
#define POWER_CTRL_2_CMD          0xC1
#define VCOM_CTRL_1_CMD           0xC5
#define VCOM_CTRL_2_CMD           0xC7
#define ENABLE_3G_CMD             0xF2

#define POS_GAMMA_CORR_CMD        0xE0
#define NEG_GAMMA_CORR_CMD        0xE1
#define POWER_CTRL_A_CMD          0xCB
#define POWER_CTRL_B_CMD          0xCF
#define DRIVER_TIM_CTRL_A_CMD     0xE8
#define DRIVER_TIM_CTRL_B_CMD     0xEA
#define POWER_ON_SEQ_CTRL_CMD     0xED
#define PUMP_RATION_CTRL_CMD      0xF7

/* initialize the lcd */
void lcd_init(void);

void lcd_config(void);

/* set lcd display region */
void lcd_set_region(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);
/* set the start display point of lcd */
void lcd_set_xy(uint16_t x, uint16_t y);
/* draw a point on the lcd */
void gui_draw_point(uint16_t x, uint16_t y, uint16_t data);
/* clear the lcd */
void lcd_clear(uint16_t color);

/* send a halfword through the SPI interface and return a halfwold received from the SPI bus */
uint16_t spi_write_halfword(uint16_t halfword);

uint8_t lcd_spi_write_byte(uint8_t byte);

/* copy image data to LCD */
void lcd_copy(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t *src_data);
/* fill LCD region with specific color */
void lcd_fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

#endif /* LCD_DRIVER_H */
