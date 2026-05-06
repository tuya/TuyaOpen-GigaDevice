/*!
    \file    qspi_flash_api.c
    \brief   QSPI write and read external flash

    \version 2024-12-31
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

#include <stdio.h>
#include <stdbool.h>
#include "gd32vw55x.h"
#include "qspi_flash_api.h"
#include "wrapper_os.h"
#include "ll.h"
#include "dbg_print.h"

#define QSPI_FLASH_TEST                     0

#define QSPI_QUAD_EN                        0

// #define QSPI_DMA_READ

#define SIZE_32K                            32768
#define SIZE_64K                            65536
#define SIZE_128K                           131072

#if (QSPI_FLASH_MEM == 16)
#define QSPI_FLASH_TOTAL_SIZE               (0x1000000)
#define EXT_FLASH_SIZE_LOG_INDX             23      // 16M bytes
#elif (QSPI_FLASH_MEM == 32)
#define QSPI_FLASH_TOTAL_SIZE               (0x2000000)
#define EXT_FLASH_SIZE_LOG_INDX             24      // 32M bytes
#else
#define QSPI_FLASH_TOTAL_SIZE               (0x200000)
#define EXT_FLASH_SIZE_LOG_INDX             20      // 2M bytes
#endif

#define QSPI_FLASH_SECTOR_SIZE              0x1000

#define QSPI_MEMORY_MAP_BASE_ADDR           0x90000000


#define QSPI_POLLING_CYCLES                 0x10

#define READ_STATUS_REG                    (0x05)
#define READ_STATUS2_REG                   (0x35)
#if (QSPI_FLASH_MEM == 32)
#define READ_STATUS3_REG                   (0x15)
#endif


#define WRITE_STATUS_REG                    (0x01)
#if (QSPI_FLASH_MEM == 16 || QSPI_FLASH_MEM == 32)
#define WRITE_STATUS2_REG                   (0x31)
#endif
#define WRITE_ENABLE_CMD                    (0x06)
#define PAGE_PROG_CMD                       (0x02)
#define QUAD_PAGE_PROG_CMD                  (0x32)
#if (QSPI_FLASH_MEM == 32)
#define PAGE_4PROG_CMD                      (0x12)
#define QUAD_PAGE_4PROG_CMD                 (0x34)
#endif
#define READ_CMD                            (0x03)
#define QUAD_READ_CMD                       (0xEB)
#if (QSPI_FLASH_MEM == 32)
#define QUAD_4READ_CMD                      (0xEC)
#endif

#define READ_STATUS_REG1_CMD                (0x05)
#define READ_STATUS_REG2_CMD                (0x35)
#if (QSPI_FLASH_MEM == 16 || QSPI_FLASH_MEM == 32)
#define READ_STATUS_REG3_CMD                (0x15)
#endif

#define SECTOR_ERASE_CMD                    (0x20)
#if (QSPI_FLASH_MEM == 32)
#define SECTOR_4ERASE_CMD                   (0x21)
#endif
#define BLOCK_ERASE_32K_CMD                 (0x52)
#if (QSPI_FLASH_MEM == 32)
#define BLOCK_ERASE_4BE32K_CMD              (0x5C)
#endif
#define BLOCK_ERASE_64K_CMD                 (0xD8)
#if (QSPI_FLASH_MEM == 32)
#define BLOCK_ERASE_4BE64K_CMD              (0xDC)
#endif

#define CHIP_ERASE_CMD                      (0xC7)
#if (QSPI_FLASH_MEM == 2)
#define HIGH_PFM_EN_CMD                     (0xA3)
#endif
#define READ_UNI_ID_CMD                     (0x4B)
#define READ_REMS_CMD                       (0x90)
#if (QSPI_FLASH_MEM == 32)
#define ENTER_4ADS_CMD                      (0xB7)
#define EXIT_4ADS_CMD                       (0xE9)
#define FAST4_READ_CMD                      (0x0C)
#endif


#define STATUS_REG_WIP_VAL                  0x01
#define STATUS_REG_WIP_MSK                  0x01        //s0

#define STATUS_REG_WEL_VAL                  0x02
#define STATUS_REG_WEL_MSK                  0x02        //s1

#define STATUS_REG_QE_VAL                  0x02
#define STATUS_REG_QE_MSK                  0x02        //s9

#if (QSPI_FLASH_MEM == 2)
#define STATUS_REG_HPF_VAL                 0x20
#define STATUS_REG_HPF_MSK                 0x20        //s13
#endif

#if (QSPI_FLASH_MEM == 32)
#define STATUS_REG_ADS_VAL                  (0x01)
#define STATUS_REG_ADS_MSK                  (0x01)
#endif


#if QSPI_FLASH_TEST
#define BUFF_SECTOR   4096
uint8_t tx_buffer[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };
#define countof(a)                          (sizeof(a) / sizeof(*(a)))
#define BUF_SIZE                            (countof(tx_buffer))

uint16_t tx_buffer_sector[BUFF_SECTOR];
uint8_t rx_buffer[BUF_SIZE];
uint16_t rx_buffer_sector[BUFF_SECTOR];
#endif

static bool enabled = false;

#ifdef QSPI_DMA_READ
#define QSPI_DMA_READ_CHNL      DMA_CH1
static os_sema_t qspi_read_sema = NULL;
void DMA_Channel1_IRQHandler(void)
{
    if (RESET != dma_interrupt_flag_get(DMA_CH1, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(DMA_CH1, DMA_INT_FLAG_FTF);
        sys_sema_up_from_isr(&qspi_read_sema);
    }
}
#endif

static void qspi_flash_status_read(uint32_t instruction, uint8_t *data);

/*!
    \brief      QSPI write
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void qspi_write_enable(void)
{
    qspi_command_struct qspi_cmd;

    /* write enable */
    qspi_cmd.instruction_mode = QSPI_INSTRUCTION_1_LINE;
    qspi_cmd.instruction      = WRITE_ENABLE_CMD;
    qspi_cmd.addr_mode        = QSPI_ADDR_NONE;
    qspi_cmd.altebytes_mode   = QSPI_ALTE_BYTES_NONE;
    qspi_cmd.data_mode        = QSPI_DATA_NONE;
    qspi_cmd.dummycycles      = 0;
    qspi_cmd.sioo_mode        = QSPI_SIOO_INST_EVERY_CMD;


    qspi_cmd.addr                = 0;
    qspi_cmd.addr_size           = QSPI_ADDR_24_BITS;
    qspi_cmd.altebytes           = 0;
    qspi_cmd.altebytes_size      = QSPI_ALTE_BYTES_8_BITS;
    qspi_cmd.data_length         = 0;


    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_command_config(&qspi_cmd);
}

/*!
    \brief      QSPI polling status register with write enable latch
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void qspi_polling_match_wel(void)
{
    qspi_polling_struct polling_cmd;
    qspi_command_struct qspi_cmd;

    polling_cmd.match            = STATUS_REG_WEL_VAL;
    polling_cmd.mask             = STATUS_REG_WEL_MSK;
    polling_cmd.match_mode       = QSPI_MATCH_MODE_AND;
    polling_cmd.statusbytes_size = 1;
    polling_cmd.interval         = QSPI_POLLING_CYCLES;
    polling_cmd.polling_stop     = QSPI_POLLING_STOP_ENABLE;

    qspi_cmd.instruction         = READ_STATUS_REG1_CMD;
    qspi_cmd.data_mode           = QSPI_DATA_1_LINE;
    qspi_cmd.addr_mode           = QSPI_ADDR_NONE;
    qspi_cmd.instruction_mode    = QSPI_INSTRUCTION_1_LINE;
    qspi_cmd.addr                = 0;
    qspi_cmd.addr_size           = QSPI_ADDR_24_BITS;
    qspi_cmd.altebytes           = 0;
    qspi_cmd.altebytes_mode      = QSPI_ALTE_BYTES_NONE;
    qspi_cmd.altebytes_size      = QSPI_ALTE_BYTES_8_BITS;
    qspi_cmd.data_length         = 1;
    qspi_cmd.dummycycles         = 0;
    qspi_cmd.sioo_mode           = QSPI_SIOO_INST_EVERY_CMD;

    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_flag_clear(QSPI_FLAG_RPMF);
    qspi_polling_config(&qspi_cmd, &polling_cmd);

    /* wait for the match complete flag to be set */
    while(RESET == qspi_flag_get(QSPI_FLAG_RPMF)) {
    }
    qspi_flag_clear(QSPI_FLAG_RPMF);
}

/*!
    \brief      QSPI polling match write not in progress
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void qspi_polling_match_not_wip(void)
{
    qspi_polling_struct polling_cmd;
    qspi_command_struct qspi_cmd;

    qspi_cmd.instruction         = READ_STATUS_REG1_CMD;
    qspi_cmd.addr_mode           = QSPI_ADDR_NONE;
    qspi_cmd.data_mode           = QSPI_DATA_1_LINE;
    qspi_cmd.instruction_mode    = QSPI_INSTRUCTION_1_LINE;
    qspi_cmd.addr                = 0;
    qspi_cmd.addr_size           = QSPI_ADDR_8_BITS;
    qspi_cmd.altebytes           = 0;
    qspi_cmd.altebytes_mode      = QSPI_ALTE_BYTES_NONE;
    qspi_cmd.altebytes_size      = QSPI_ALTE_BYTES_8_BITS;
    qspi_cmd.data_length         = 1;
    qspi_cmd.dummycycles         = 0;
    qspi_cmd.sioo_mode           = QSPI_SIOO_INST_EVERY_CMD;

    polling_cmd.match            = 0x00;
    polling_cmd.mask             = STATUS_REG_WIP_MSK;
    polling_cmd.match_mode       = QSPI_MATCH_MODE_AND;
    polling_cmd.statusbytes_size = 1;
    polling_cmd.interval         = QSPI_POLLING_CYCLES;
    polling_cmd.polling_stop     = QSPI_POLLING_STOP_ENABLE;

    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_flag_clear(QSPI_FLAG_RPMF);
    qspi_polling_config(&qspi_cmd, &polling_cmd);
    while(RESET == qspi_flag_get(QSPI_FLAG_RPMF)) {
    }
    qspi_flag_clear(QSPI_FLAG_RPMF);
}

static void qspi_polling_match_qe(bool enable)
{
    qspi_polling_struct polling_cmd;
    qspi_command_struct qspi_cmd;

    polling_cmd.match            = enable ? STATUS_REG_QE_VAL : 0x00;
    polling_cmd.mask             = STATUS_REG_QE_MSK;
    polling_cmd.match_mode       = QSPI_MATCH_MODE_AND;
    polling_cmd.statusbytes_size = 1;
    polling_cmd.interval         = QSPI_POLLING_CYCLES;
    polling_cmd.polling_stop     = QSPI_POLLING_STOP_ENABLE;

    qspi_cmd.instruction         = READ_STATUS_REG2_CMD;
    qspi_cmd.data_mode           = QSPI_DATA_1_LINE;
    qspi_cmd.addr_mode           = QSPI_ADDR_NONE;
    qspi_cmd.instruction_mode    = QSPI_INSTRUCTION_1_LINE;
    qspi_cmd.addr                = 0;
    qspi_cmd.addr_size           = QSPI_ADDR_24_BITS;
    qspi_cmd.altebytes           = 0;
    qspi_cmd.altebytes_mode      = QSPI_ALTE_BYTES_NONE;
    qspi_cmd.altebytes_size      = QSPI_ALTE_BYTES_8_BITS;
    qspi_cmd.data_length         = 1;
    qspi_cmd.dummycycles         = 0;
    qspi_cmd.sioo_mode           = QSPI_SIOO_INST_EVERY_CMD;

    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_flag_clear(QSPI_FLAG_RPMF);
    qspi_polling_config(&qspi_cmd, &polling_cmd);
    while(RESET == qspi_flag_get(QSPI_FLAG_RPMF)) {
    }
    qspi_flag_clear(QSPI_FLAG_RPMF);
}

static void qspi_flash_quad_enable(void)
{
    qspi_command_struct qspi_cmd;

#if (QSPI_FLASH_MEM == 16 || QSPI_FLASH_MEM == 32)
    uint8_t write_status[1] ={0x02};
#else
    // FIX TODO status register may need to read value first
    uint8_t write_status[2] ={0x00,0x02};
#endif

    /* QSPI write enable */
    qspi_write_enable();

    /* configure read polling mode to wait for write enabling */
    qspi_polling_match_wel();

#if (QSPI_FLASH_MEM == 16 || QSPI_FLASH_MEM == 32)
    qspi_cmd.instruction = WRITE_STATUS2_REG; //write status register
#else
    qspi_cmd.instruction = WRITE_STATUS_REG; //write status register
#endif
    qspi_cmd.instruction_mode = QSPI_INSTRUCTION_1_LINE;
    qspi_cmd.addr = 0;
    qspi_cmd.addr_mode = QSPI_ADDR_NONE;
    qspi_cmd.addr_size = QSPI_ADDR_8_BITS;
    qspi_cmd.altebytes = 0;
    qspi_cmd.altebytes_mode = QSPI_ALTE_BYTES_NONE;
    qspi_cmd.altebytes_size = QSPI_ALTE_BYTES_8_BITS;
    qspi_cmd.data_mode = QSPI_DATA_1_LINE;
    qspi_cmd.data_length = sizeof(write_status);
    qspi_cmd.dummycycles = 0;
    qspi_cmd.sioo_mode = QSPI_SIOO_INST_EVERY_CMD;

    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    sys_enter_critical();
    qspi_command_config(&qspi_cmd);
    // QSPI_DTLEN = 1;
    qspi_data_transmit(write_status);
    sys_exit_critical();

    /* wait for the data transmit completed */
    while(RESET == qspi_flag_get(QSPI_FLAG_TC)) {
    }
    /* clear the TC flag */
    qspi_flag_clear(QSPI_FLAG_TC);

    qspi_polling_match_not_wip();

    qspi_polling_match_qe(true);
}

static void qspi_flash_quad_disable(void)
{
    qspi_command_struct qspi_cmd;

#if (QSPI_FLASH_MEM == 16 || QSPI_FLASH_MEM == 32)
    uint8_t write_status[1] ={0x00};
#else
    // FIX TODO status register may need to read value first
    uint8_t write_status[2] ={0x00,0x00};
#endif

#if (QSPI_FLASH_MEM == 16 || QSPI_FLASH_MEM == 32)
    qspi_flash_status_read(READ_STATUS_REG2_CMD, write_status);
    if (write_status[0] & STATUS_REG_QE_MSK) {
        write_status[0] &= ~STATUS_REG_QE_VAL;
    }
    else {
        return;
    }
#endif

    /* QSPI write enable */
    qspi_write_enable();
    /* configure read polling mode to wait for write enabling */
    qspi_polling_match_wel();

#if (QSPI_FLASH_MEM == 16 || QSPI_FLASH_MEM == 32)
    qspi_cmd.instruction = WRITE_STATUS2_REG; //write status register
#else
    qspi_cmd.instruction = WRITE_STATUS_REG; //write status register
#endif
    qspi_cmd.instruction_mode = QSPI_INSTRUCTION_1_LINE;
    qspi_cmd.addr = 0;
    qspi_cmd.addr_mode = QSPI_ADDR_NONE;
    qspi_cmd.addr_size = QSPI_ADDR_8_BITS;
    qspi_cmd.altebytes = 0;
    qspi_cmd.altebytes_mode = QSPI_ALTE_BYTES_NONE;
    qspi_cmd.altebytes_size = QSPI_ALTE_BYTES_8_BITS;
    qspi_cmd.data_mode = QSPI_DATA_1_LINE;
    qspi_cmd.data_length = sizeof(write_status);
    qspi_cmd.dummycycles = 0;
    qspi_cmd.sioo_mode = QSPI_SIOO_INST_EVERY_CMD;

    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_command_config(&qspi_cmd);

    // QSPI_DTLEN = 1;
    qspi_data_transmit(write_status);

    /* wait for the data transmit completed */
    while(RESET == qspi_flag_get(QSPI_FLAG_TC)) {
    }

    /* clear the TC flag */
    qspi_flag_clear(QSPI_FLAG_TC);

    qspi_polling_match_not_wip();

    qspi_polling_match_qe(false);
}

#if (QSPI_FLASH_MEM == 2)
static void qspi_polling_match_hpf(void)
{
    qspi_polling_struct polling_cmd;
    qspi_command_struct qspi_cmd;

    polling_cmd.match            = STATUS_REG_HPF_VAL;
    polling_cmd.mask             = STATUS_REG_HPF_MSK;
    polling_cmd.match_mode       = QSPI_MATCH_MODE_AND;
    polling_cmd.statusbytes_size = 1;
    polling_cmd.interval         = QSPI_POLLING_CYCLES;
    polling_cmd.polling_stop     = QSPI_POLLING_STOP_ENABLE;

    qspi_cmd.instruction         = READ_STATUS_REG2_CMD;
    qspi_cmd.data_mode           = QSPI_DATA_1_LINE;
    qspi_cmd.addr_mode           = QSPI_ADDR_NONE;
    qspi_cmd.instruction_mode    = QSPI_INSTRUCTION_1_LINE;
    qspi_cmd.addr                = 0;
    qspi_cmd.addr_size           = QSPI_ADDR_24_BITS;
    qspi_cmd.altebytes           = 0;
    qspi_cmd.altebytes_mode      = QSPI_ALTE_BYTES_NONE;
    qspi_cmd.altebytes_size      = QSPI_ALTE_BYTES_8_BITS;
    qspi_cmd.data_length         = 1;
    qspi_cmd.dummycycles         = 0;
    qspi_cmd.sioo_mode           = QSPI_SIOO_INST_EVERY_CMD;


    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_flag_clear(QSPI_FLAG_RPMF);
    qspi_polling_config(&qspi_cmd, &polling_cmd);
    while(RESET == qspi_flag_get(QSPI_FLAG_RPMF)) {
    }

    qspi_flag_clear(QSPI_FLAG_RPMF);
}

static void qspi_high_performance_enable(void)
{
    qspi_command_struct qspi_cmd;

    qspi_cmd.instruction = HIGH_PFM_EN_CMD;
    qspi_cmd.instruction_mode = QSPI_INSTRUCTION_1_LINE;
    qspi_cmd.addr = 0;
    qspi_cmd.addr_mode = QSPI_ADDR_NONE;
    qspi_cmd.addr_size = QSPI_ADDR_8_BITS;
    qspi_cmd.altebytes = 0;
    qspi_cmd.altebytes_mode = QSPI_ALTE_BYTES_NONE;
    qspi_cmd.altebytes_size = QSPI_ALTE_BYTES_8_BITS;
    qspi_cmd.data_mode = QSPI_DATA_1_LINE;
    qspi_cmd.data_length = 0;
    qspi_cmd.dummycycles = 26;
    qspi_cmd.sioo_mode = QSPI_SIOO_INST_EVERY_CMD;

    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
    qspi_command_config(&qspi_cmd);

    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    // qspi_polling_match_hpf();
}
#endif

#ifdef QSPI_DMA_READ
void qspi_dma_single_mode_config(uint32_t direction)
{
    dma_single_data_parameter_struct dma_init_struct;

    rcu_periph_clock_enable(RCU_DMA);
    if(QSPI_DMA_READ_CHNL == DMA_CH0){
        eclic_irq_enable(DMA_Channel0_IRQn, 9, 0);
    }
    else {
        eclic_irq_enable(DMA_Channel1_IRQn, 9, 0);
    }

    dma_single_data_para_struct_init(&dma_init_struct);

    dma_init_struct.direction = direction;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_addr = (uint32_t)&QSPI_DATA;
    dma_init_struct.circular_mode = DMA_CIRCULAR_MODE_DISABLE;

    if (direction == DMA_MEMORY_TO_PERIPH) {
        dma_init_struct.priority = DMA_PRIORITY_LOW;
    } else if (direction == DMA_PERIPH_TO_MEMORY) {
        dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    } else {
        return;
    }

    dma_deinit(QSPI_DMA_READ_CHNL);
    dma_single_data_mode_init(QSPI_DMA_READ_CHNL, &dma_init_struct);

    dma_circulation_disable(QSPI_DMA_READ_CHNL);
    dma_channel_subperipheral_select(QSPI_DMA_READ_CHNL, DMA_SUBPERI5);

    dma_interrupt_enable(QSPI_DMA_READ_CHNL, DMA_INT_FTF);
}
#endif


/*!
    \brief      configure the QSPI peripheral
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void qspi_flash_init(void)
{
    qspi_init_struct qspi_init_para;

#if CONFIG_BOARD == PLATFORM_BOARD_32VW55X_START
    /* QSPI GPIO config:SCK/PA9, NSS/PA10, IO0/PA11, IO1/PA12 */
    gpio_af_set(GPIOA, GPIO_AF_4, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);

    /* QSPI GPIO config: IO2/PB3, IO3/PB4 */
    gpio_af_set(GPIOB, GPIO_AF_3, GPIO_PIN_3 | GPIO_PIN_4);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3 | GPIO_PIN_4);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_3 | GPIO_PIN_4);
#elif (CONFIG_BOARD == PLATFORM_BOARD_32VW55X_EVAL || CONFIG_BOARD == PLATFORM_BOARD_32VW55X_SONIC)
    /* QSPI GPIO config:SCK/PA4, NSS/PA5, IO0/PA6, IO1/PA7 */
    gpio_af_set(GPIOA, GPIO_AF_3, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);

    // If i2s record is enabled, PB4 is used, so quad spi should not enabled for eval board
#ifndef I2S_RECORD
#if QSPI_QUAD_EN
    /* QSPI GPIO config:IO2/PB3, IO3/PB4 */
    gpio_af_set(GPIOB, GPIO_AF_3, GPIO_PIN_3 | GPIO_PIN_4);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3 | GPIO_PIN_4);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_3 | GPIO_PIN_4);
#endif
#endif
#endif
    /* initialize the init parameter structure */
    qspi_struct_para_init(&qspi_init_para);

#if 1
    qspi_init_para.clock_mode     = QSPI_CLOCK_MODE_3;
    qspi_init_para.fifo_threshold = 8;
    qspi_init_para.sample_shift   = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    qspi_init_para.cs_high_time   = QSPI_CS_HIGH_TIME_8_CYCLE;
    qspi_init_para.flash_size     = EXT_FLASH_SIZE_LOG_INDX;
    qspi_init_para.prescaler      = 1;
#else
    qspi_init_para.clock_mode     = QSPI_CLOCK_MODE_0;
    qspi_init_para.fifo_threshold = 4;
    qspi_init_para.sample_shift   = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    qspi_init_para.cs_high_time   = QSPI_CS_HIGH_TIME_2_CYCLE;
    qspi_init_para.flash_size     = EXT_FLASH_SIZE_LOG_INDX;
    qspi_init_para.prescaler      = 3;
#endif
    qspi_init(&qspi_init_para);

    qspi_enable();

#if QSPI_QUAD_EN
    qspi_flash_quad_enable();
#if (QSPI_FLASH_MEM == 2)
    qspi_high_performance_enable();
#endif
#else
    qspi_flash_quad_disable();
#endif
}

/*!
    \brief      send QSPI command
    \param[in]  instruction: QSPI instruction, reference flash commands description
    \param[in]  address: access address, 0-flash size
    \param[in]  dummy_cycles: dummy cycles, 0 - 31
    \param[in]  instruction_mode: instruction mode
      \arg        QSPI_INSTRUCTION_NONE
      \arg        QSPI_INSTRUCTION_1_LINE
      \arg        QSPI_INSTRUCTION_2_LINES
      \arg        QSPI_INSTRUCTION_4_LINES
    \param[in]  address_mode: address mode
      \arg        QSPI_ADDR_NONE
      \arg        QSPI_ADDR_1_LINE
      \arg        QSPI_ADDR_2_LINES
      \arg        QSPI_ADDR_4_LINES
    \param[in]  address_size: address size
      \arg        QSPI_ADDR_8_BITS
      \arg        QSPI_ADDR_16_BITS
      \arg        QSPI_ADDR_24_BITS
      \arg        QSPI_ADDR_32_BITS
    \param[in]  data_mode: data mode
      \arg        QSPI_DATA_NONE
      \arg        QSPI_DATA_1_LINE
      \arg        QSPI_DATA_2_LINES
      \arg        QSPI_DATA_4_LINES
    \param[out] none
    \retval     none
*/
static void qspi_send_command(uint32_t instruction, uint32_t address, uint32_t dummy_cycles, uint32_t instruction_mode,
                       uint32_t address_mode, uint32_t address_size, uint32_t data_mode, uint32_t altebytes_mode, uint32_t altebytes_size, uint32_t data_length)
{
    qspi_command_struct qspi_cmd;

    qspi_cmd.instruction      = instruction;
    qspi_cmd.instruction_mode = instruction_mode;
    qspi_cmd.addr             = address;
    qspi_cmd.addr_mode        = address_mode;
    qspi_cmd.addr_size        = address_size;
    qspi_cmd.altebytes        = 0;
    qspi_cmd.altebytes_mode   = altebytes_mode;
    qspi_cmd.altebytes_size   = altebytes_size;
    qspi_cmd.data_mode        = data_mode;
    qspi_cmd.data_length      = data_length;
    qspi_cmd.dummycycles      = dummy_cycles;
    qspi_cmd.sioo_mode        = QSPI_SIOO_INST_EVERY_CMD;
    qspi_command_config(&qspi_cmd);
}


/*!
    \brief      read spi flash by memory map mode
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void qspi_memory_map_read(void)
{
    qspi_command_struct qspi_cmd;

#if QSPI_QUAD_EN
#if (QSPI_FLASH_MEM == 32)
    qspi_cmd.instruction_mode = QSPI_INSTRUCTION_1_LINE;
    qspi_cmd.instruction      = QUAD_4READ_CMD;
    qspi_cmd.addr_mode        = QSPI_ADDR_4_LINES;
    qspi_cmd.addr_size        = QSPI_ADDR_32_BITS;
    qspi_cmd.altebytes_size   = QSPI_ALTE_BYTES_8_BITS;
    qspi_cmd.altebytes_mode   = QSPI_ALTE_BYTES_4_LINES;
    qspi_cmd.altebytes        = 0;
    qspi_cmd.data_mode        = QSPI_DATA_4_LINES;
    qspi_cmd.dummycycles      = 4;
    qspi_cmd.sioo_mode        = QSPI_SIOO_INST_EVERY_CMD;
#else
    qspi_cmd.instruction_mode = QSPI_INSTRUCTION_1_LINE;
    qspi_cmd.instruction      = QUAD_READ_CMD;
    qspi_cmd.addr_mode        = QSPI_ADDR_4_LINES;
    qspi_cmd.addr_size        = QSPI_ADDR_24_BITS;
    qspi_cmd.altebytes_size   = QSPI_ALTE_BYTES_8_BITS;
    qspi_cmd.altebytes_mode   = QSPI_ALTE_BYTES_4_LINES;
    qspi_cmd.altebytes        = 0;
    qspi_cmd.data_mode        = QSPI_DATA_4_LINES;
    qspi_cmd.dummycycles      = 4;
    qspi_cmd.sioo_mode        = QSPI_SIOO_INST_EVERY_CMD;
#endif
#else
#if (QSPI_FLASH_MEM == 32)
    // use 4 bytes address mode fast read
    qspi_cmd.instruction_mode = QSPI_INSTRUCTION_1_LINE;
    qspi_cmd.instruction      = FAST4_READ_CMD;
    qspi_cmd.addr_mode        = QSPI_ADDR_1_LINE;
    qspi_cmd.addr_size        = QSPI_ADDR_32_BITS;
    qspi_cmd.altebytes_mode   = QSPI_ALTE_BYTES_NONE;
    qspi_cmd.data_mode        = QSPI_DATA_1_LINE;
    qspi_cmd.dummycycles      = 8;
    qspi_cmd.altebytes        = 0;
    qspi_cmd.altebytes_size   = QSPI_ALTE_BYTES_8_BITS;
    qspi_cmd.sioo_mode        = QSPI_SIOO_INST_EVERY_CMD;
#else
    qspi_cmd.instruction_mode = QSPI_INSTRUCTION_1_LINE;
    qspi_cmd.instruction      = READ_CMD;
    qspi_cmd.addr_mode        = QSPI_ADDR_1_LINE;
    qspi_cmd.addr_size        = QSPI_ADDR_24_BITS;
    qspi_cmd.altebytes_mode   = QSPI_ALTE_BYTES_NONE;
    qspi_cmd.data_mode        = QSPI_DATA_1_LINE;
    qspi_cmd.dummycycles      = 0;
    qspi_cmd.altebytes        = 0;
    qspi_cmd.altebytes_size   = QSPI_ALTE_BYTES_8_BITS;
    qspi_cmd.sioo_mode        = QSPI_SIOO_INST_EVERY_CMD;
#endif
#endif

    qspi_cmd.addr                = 0;
    qspi_cmd.data_length         = 0;

    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
    qspi_memorymapped_config(&qspi_cmd, 0, QSPI_TMOUT_ENABLE);

    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
}

/*!
    \brief      read spi flash
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void qspi_flash_memory_read(uint32_t offset, uint8_t *data, uint32_t len)
{
    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
#if (QSPI_FLASH_MEM == 32)
#if QSPI_QUAD_EN
    qspi_send_command(QUAD_4READ_CMD, offset, 4, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_4_LINES, QSPI_ADDR_32_BITS, QSPI_DATA_4_LINES, QSPI_ALTE_BYTES_4_LINES, QSPI_ALTE_BYTES_8_BITS, len);
#else
    // use 4 bytes address mode fast read
    qspi_send_command(FAST4_READ_CMD, offset, 8, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_1_LINE, QSPI_ADDR_32_BITS, QSPI_DATA_1_LINE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, len);
#endif
#else
    qspi_send_command(READ_CMD, offset, 0, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_1_LINE, QSPI_ADDR_24_BITS, QSPI_DATA_1_LINE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, len);
#endif

#ifdef QSPI_DMA_READ
    if (qspi_read_sema) {
        dma_interrupt_flag_clear(QSPI_DMA_READ_CHNL, DMA_INT_FLAG_FTF);
        dma_memory_address_config(QSPI_DMA_READ_CHNL, DMA_MEMORY_0, (uint32_t)data);
        dma_transfer_number_config(QSPI_DMA_READ_CHNL, len);

        QSPI_TCFG = (QSPI_TCFG & ~QSPI_MEMORY_MAPPED) | QSPI_NORMAL_READ;
        QSPI_ADDR = offset;

        dma_channel_enable(QSPI_DMA_READ_CHNL);
        qspi_dma_enable();

        sys_sema_down(&qspi_read_sema, 0);
        qspi_dma_disable();
        return;
    }
#endif
    // QSPI_DTLEN = (uint32_t)(len - 1);
    qspi_data_receive((uint8_t *)data);
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
}

/*!
    \brief      qspi flash erase
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void qspi_flash_various_erase(uint32_t offset, uint32_t erase_cmd, uint32_t address_size)
{
    /* QSPI write enable */
    qspi_write_enable();
    /* configure read polling mode to wait for write enabling */
    qspi_polling_match_wel();

    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_send_command(erase_cmd, offset, 0, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_1_LINE,
                        address_size, QSPI_DATA_NONE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, 0);

    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_polling_match_not_wip();
}


/*!
    \brief      qspi flash program
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void qspi_flash_program(uint32_t offset, uint8_t *data, int len)
{
    /* QSPI write enable */
    qspi_write_enable();
    /* configure read polling mode to wait for write enabling */
    qspi_polling_match_wel();

    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
    /* clear the TC flag */
    qspi_flag_clear(QSPI_FLAG_TC);
    /* write data */
#if (QSPI_FLASH_MEM == 32)
#if QSPI_QUAD_EN
    qspi_send_command(QUAD_PAGE_4PROG_CMD, offset, 0, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_1_LINE,
                            QSPI_ADDR_32_BITS, QSPI_DATA_4_LINES, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, len);
#else
    qspi_send_command(PAGE_4PROG_CMD, offset, 0, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_1_LINE,
                            QSPI_ADDR_32_BITS, QSPI_DATA_1_LINE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, len);
#endif
#else
#if QSPI_QUAD_EN
    qspi_send_command(QUAD_PAGE_PROG_CMD, offset, 0, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_1_LINE,
                        QSPI_ADDR_24_BITS, QSPI_DATA_4_LINES, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, len);
#else
    qspi_send_command(PAGE_PROG_CMD, offset, 0, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_1_LINE,
                        QSPI_ADDR_24_BITS, QSPI_DATA_1_LINE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, len);
#endif
#endif
    // QSPI_DTLEN = (uint32_t)(len - 1);
    qspi_data_transmit((uint8_t *)data);
    /* wait for the data transmit completed */
    while(RESET == qspi_flag_get(QSPI_FLAG_TC)) {
    }
    /* clear the TC flag */
    qspi_flag_clear(QSPI_FLAG_TC);

    qspi_polling_match_not_wip();
}

static uint32_t qspi_flash_total_size(void)
{
    return QSPI_FLASH_TOTAL_SIZE;
}


static int qspi_flash_is_valid_offset(uint32_t offset)
{
    if (offset < qspi_flash_total_size()) {
        return 1;
    }
    return 0;
}


int qspi_flash_erase(uint32_t offset, uint32_t len)
{

    if (!enabled || !qspi_flash_is_valid_offset(offset)
        || len == 0 || !qspi_flash_is_valid_offset(offset + len - 1)) {
        return -1;
    }

    //printf("qspi_flash_erase offset 0x%x, len %d \r\n", offset, len);
#if 1
    do
    {
        if (len >= SIZE_64K && (offset & 0xFFFF) == 0) {
#if (QSPI_FLASH_MEM == 32)
            qspi_flash_various_erase(offset, BLOCK_ERASE_4BE64K_CMD, QSPI_ADDR_32_BITS);
#else
            qspi_flash_various_erase(offset, BLOCK_ERASE_64K_CMD, QSPI_ADDR_24_BITS);
#endif
            len -= SIZE_64K;
            offset += SIZE_64K;
        }
        else if (len >= SIZE_32K && (offset & 0x7FFF) == 0) {
#if (QSPI_FLASH_MEM == 32)
            qspi_flash_various_erase(offset, BLOCK_ERASE_4BE32K_CMD, QSPI_ADDR_32_BITS);
#else
            qspi_flash_various_erase(offset, BLOCK_ERASE_32K_CMD, QSPI_ADDR_24_BITS);
#endif
            len -= SIZE_32K;
            offset += SIZE_32K;
        }
        else {
            uint32_t erase_len;
#if (QSPI_FLASH_MEM == 32)
            qspi_flash_various_erase(offset, SECTOR_4ERASE_CMD, QSPI_ADDR_32_BITS);
#else
            qspi_flash_various_erase(offset, SECTOR_ERASE_CMD, QSPI_ADDR_24_BITS);
#endif

            if ((offset & 0x0FFF) == 0) {
                erase_len = QSPI_FLASH_SECTOR_SIZE;
            }
            else {
                erase_len = QSPI_FLASH_SECTOR_SIZE - (offset & 0x0FFF);
            }

            len = len > erase_len ? (len - erase_len) : 0;
            offset += erase_len;
        }
    } while(len > 0);
#else
    {
        uint16_t i, num_of_sector;

        num_of_sector = (len / QSPI_FLASH_SECTOR_SIZE);
        if ((len % QSPI_FLASH_SECTOR_SIZE) != 0)
            num_of_sector++;

        for (i = 0; i < num_of_sector; i++) {
#if (QSPI_FLASH_MEM == 32)
            qspi_flash_various_erase(offset + i * QSPI_FLASH_SECTOR_SIZE, SECTOR_4ERASE_CMD, QSPI_ADDR_32_BITS);
#else
            qspi_flash_various_erase(offset + i * QSPI_FLASH_SECTOR_SIZE, SECTOR_ERASE_CMD, QSPI_ADDR_24_BITS);
#endif
        }
    }
#endif
    //printf("qspi_flash_erase offset 0x%x, len %d end\r\n", offset, len);

    return 0;
}

int qspi_flash_write(uint32_t offset, uint8_t *data, uint32_t len)
{
    uint16_t num_of_page = 0;
    uint8_t num_of_single = 0;
    uint16_t page_left = 0;

    if (!enabled || !qspi_flash_is_valid_offset(offset) || data == NULL
        || len == 0 || !qspi_flash_is_valid_offset(offset + len - 1)) {
        return -1;
    }
    // printf("qspi_flash_write offset 0x%x, len %d \r\n", offset, len);

    page_left = NOR_FLASH_PAGE_SIZE - (offset % NOR_FLASH_PAGE_SIZE);

    if (page_left != NOR_FLASH_PAGE_SIZE && page_left < len) {
        qspi_flash_program(offset, data, page_left);
        offset += page_left;
        data += page_left;
        len -= page_left;
    }

    num_of_page = len / NOR_FLASH_PAGE_SIZE;
    num_of_single = len % NOR_FLASH_PAGE_SIZE;

    while(num_of_page) {
        qspi_flash_program(offset, data, NOR_FLASH_PAGE_SIZE);
        offset += NOR_FLASH_PAGE_SIZE;
        data += NOR_FLASH_PAGE_SIZE;
        num_of_page--;
    }

    if (num_of_single) {
        qspi_flash_program(offset, data, num_of_single);
    }

    //printf("qspi_flash_write offset 0x%x, len %d end\r\n", offset, len);
    return 0;
}


int qspi_flash_read(uint32_t offset, uint8_t *data, uint32_t len)
{
    int i;
    if (!enabled || !qspi_flash_is_valid_offset(offset) || data == NULL
        || len == 0 || !qspi_flash_is_valid_offset(offset + len - 1)) {
        return -1;
    }

#ifdef QSPI_DMA_READ
    qspi_flash_memory_read(offset, data, len);
#else
    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    //printf("qspi_flash_read offset 0x%x, len %d \r\n", offset, len);
    qspi_memory_map_read();

    for(i = 0; i < len; i++) {
        data[i] = *(uint8_t *)(QSPI_MEMORY_MAP_BASE_ADDR + offset + i);
    }


    if ((offset + len + 16) > QSPI_FLASH_TOTAL_SIZE) {
        qspi_transmission_abort();
    }

    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
#endif

    //printf("qspi_flash_read offset 0x%x, len %d end\r\n", offset, len);
    return 0;
}

static void qspi_flash_status_read(uint32_t instruction, uint8_t *data)
{
    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
    qspi_send_command(instruction, 0, 0, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_NONE, QSPI_ADDR_24_BITS, QSPI_DATA_1_LINE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, 1);
    qspi_data_receive((uint8_t *)data);
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
}

void qspi_flash_chip_erase(void)
{
    if (!enabled)
        return;

    /* QSPI write enable */
    qspi_write_enable();

    /* configure read polling mode to wait for write enabling */
    qspi_polling_match_wel();

    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_send_command(CHIP_ERASE_CMD, 0, 0, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_NONE,
                        QSPI_ADDR_24_BITS, QSPI_DATA_NONE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, 0);

    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_polling_match_not_wip();
}

// This should be called after os worked
void qspi_flash_dma_enable(void)
{
#ifdef QSPI_DMA_READ
    if (qspi_read_sema == NULL) {
        qspi_dma_single_mode_config(DMA_PERIPH_TO_MEMORY);
        if (sys_sema_init_ext(&qspi_read_sema, 1, 0)) {
            dbg_print(ERR, "qspi_flash_api_init read sema init fail \r\n");
            return;
        }
    }
    else {
        dbg_print(ERR, "qspi_flash_api_init read sema already init \r\n");
    }
#endif
}


bool qspi_flash_api_init(void)
{
    uint8_t id_data[16];

    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_QSPI);

    qspi_flash_init();
    enabled = false;

    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    sys_memset(id_data, 0, 16);

    qspi_send_command(READ_UNI_ID_CMD, 0, 8, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_1_LINE,
                        QSPI_ADDR_24_BITS, QSPI_DATA_1_LINE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, 16);

    qspi_data_receive(id_data);

    for(uint8_t i = 0; i < 16; i++) {
        if (id_data[i] != 0) {
            enabled = true;
            return true;
        }
    }


    dbg_print(ERR, "qspi_flash_api_init read unique id fail\r\n");

    return false;
}

#if QSPI_FLASH_TEST
void flash_test(void)
{
    uint32_t i = 0;
    uint8_t data;
#if (QSPI_FLASH_MEM == 16)
    printf("QSPI flash writing 16M flash...\r\n");
#elif (QSPI_FLASH_MEM == 32)
    printf("QSPI flash writing 32M flash...\r\n");
#else
    printf("QSPI flash writing 2M flash...\r\n");
#endif
    if (!qspi_flash_api_init()) {
        printf("QSPI flash init fail...\r\n");
        return;
    }

    qspi_flash_chip_erase();

#if (QSPI_FLASH_MEM == 16 || QSPI_FLASH_MEM == 32)
    /* wait for the BUSY flag to be reset */
    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_send_command(READ_STATUS_REG2_CMD, 0, 0, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_NONE,
                        QSPI_ADDR_24_BITS, QSPI_DATA_1_LINE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, 1);

    qspi_data_receive((uint8_t *)&data);

    printf("read status s15-s8: 0x%02x\r\n", data);


    while(RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
    qspi_send_command(READ_STATUS_REG3_CMD, 0, 0, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_NONE,
                        QSPI_ADDR_24_BITS, QSPI_DATA_1_LINE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, 1);

    qspi_data_receive((uint8_t *)&data);

    printf("read status s23-s16: 0x%02x\r\n", data);
#endif

    /* qspi flash sector erase */
    /* erase sector 0-0x1000 */
    qspi_flash_erase(0x10000, 0x1000);
    sys_ms_sleep(500);
    qspi_flash_erase(0x20000, 0x1000);
    printf("QSPI flash erase all complete...\r\n");


    /* qspi flash program */
    qspi_flash_write(0x20200, tx_buffer, BUF_SIZE);
    printf("QSPI flash reading...\r\n");
    qspi_flash_erase(0x10000, 0x2000);

    //qspi_flash_chip_erase();
    /* read data by memory map mode */
    qspi_flash_read(0x20200, rx_buffer, BUF_SIZE);

    if(!memcmp(rx_buffer, tx_buffer, BUF_SIZE)) {
        printf("SPI FLASH WRITE AND READ TEST SUCCESS!\r\n");
    } else {
        printf("read buf: ");
        for (int i = 0; i < BUF_SIZE; i++) {
            printf("0x%02x ", rx_buffer[i]);
        }
        printf("\r\n");
        printf("SPI FLASH WRITE AND READ TEST ERROR!\r\n");
    }

    memset(rx_buffer, 0, BUF_SIZE);
    // erase again
    qspi_flash_erase(0x20000, 0x2000);
    /* read data by memory map mode */
    qspi_flash_read(0x20200, rx_buffer, BUF_SIZE);

    if(!memcmp(rx_buffer, tx_buffer, BUF_SIZE)) {
        printf("erase before read ERROR!\r\n");
    } else {
        printf("erase before read SUCCESS!\r\n");
    }

    qspi_flash_write(0x20200, tx_buffer, BUF_SIZE);

    memset(rx_buffer, 0, BUF_SIZE);
    qspi_flash_read(0x20200, rx_buffer, BUF_SIZE);
    if(!memcmp(rx_buffer, tx_buffer, BUF_SIZE)) {
        printf("SPI FLASH WRITE AND READ TEST 2 SUCCESS!\r\n");
    } else {
        printf("SPI FLASH WRITE AND READ TEST 2 ERROR!\r\n");
    }

    memset(rx_buffer, 0, BUF_SIZE);
    /* qspi flash program */
    qspi_flash_write(0x20501, tx_buffer, BUF_SIZE);
    printf("QSPI flash reading...\r\n");
    /* read data by memory map mode */
    qspi_flash_read(0x20501, rx_buffer, BUF_SIZE);
    if(!memcmp(rx_buffer, tx_buffer, BUF_SIZE)) {
        printf("SPI FLASH WRITE AND READ TEST 3 SUCCESS!\r\n");
    } else {
        printf("SPI FLASH WRITE AND READ TEST 3 ERROR!\r\n");
    }

    qspi_flash_read(0x40000, (uint8_t *)rx_buffer_sector, BUFF_SECTOR);
    qspi_flash_read(0x42000, (uint8_t *)rx_buffer_sector, BUFF_SECTOR);
    printf("QSPI flash read complete...\r\n");
    qspi_flash_erase(0x7000, 0x1000);
    printf("QSPI flash erase complete4...\r\n");

    for(int i = 0; i < BUFF_SECTOR; i++) {
        tx_buffer_sector[i] = i + 1;
    }

    for(i = 0; i < 257; i++) {
        qspi_flash_erase(0x500, 0x4000);

        qspi_flash_write(0x500 + i, (uint8_t *)tx_buffer_sector, BUFF_SECTOR << 1);
        qspi_flash_read(0x500 + i, (uint8_t *)rx_buffer_sector, BUFF_SECTOR << 1);

        if(memcmp(tx_buffer_sector, rx_buffer_sector, BUFF_SECTOR << 1)) {
            printf("SPI FLASH WRITE AND READ TEST not align page, idx(%d)  ERROR!\r\n", i);
            break;
        }
    }

    if (i == 257) {
        printf("SPI FLASH WRITE AND READ page 0x500 offset TEST SUCCESS\r\n");
    }
    else {
        printf("SPI FLASH WRITE AND READ page 0x500 offset TEST FAIL !!!\r\n");
        return;
    }

    for(i = 0; i < 257; i++) {
        memset(rx_buffer, 0, BUF_SIZE);
        qspi_flash_erase(0x10500, 0x4000);

        qspi_flash_write(0x10500 + i, tx_buffer, BUF_SIZE);
        qspi_flash_read(0x10500 + i, rx_buffer, BUF_SIZE);

        if(memcmp(tx_buffer, rx_buffer, BUF_SIZE)) {
            printf("SPI FLASH WRITE AND READ TEST2 not align page, idx(%d)  ERROR!\r\n", i);
            break;
        }
    }

    if (i == 257) {
        printf("SPI FLASH WRITE AND READ page 0x10500 offset TEST2 SUCCESS\r\n");
    }
    else {
        printf("SPI FLASH WRITE AND READ page 0x10500 offset TEST2 FAIL !!!\r\n");
        return;
    }


#if (QSPI_FLASH_MEM == 16 || QSPI_FLASH_MEM == 32)
    // erase again
    qspi_flash_erase(0x300000, 0x2000);

    memset(rx_buffer, 0, BUF_SIZE);
    qspi_flash_write(0x300000, tx_buffer, BUF_SIZE);

    /* read data by memory map mode */
    qspi_flash_read(0x300000, rx_buffer, BUF_SIZE);

    if(!memcmp(rx_buffer, tx_buffer, BUF_SIZE)) {
        printf("SPI 16M FLASH WRITE AND READ TEST SUCCESS!\r\n");
    } else {
        printf("SPI 16M FLASH WRITE AND READ TEST ERROR!\r\n");
    }

#endif

#if (QSPI_FLASH_MEM == 32)
        // erase again
        qspi_flash_erase(0x1300000, 0x2000);

        memset(rx_buffer, 0, BUF_SIZE);
        qspi_flash_write(0x1300000, tx_buffer, BUF_SIZE);

        /* read data by memory map mode */
        qspi_flash_read(0x1300000, rx_buffer, BUF_SIZE);

        if(!memcmp(rx_buffer, tx_buffer, BUF_SIZE)) {
            printf("SPI 32M FLASH WRITE AND READ TEST SUCCESS!\r\n");
        } else {
            printf("SPI 32M FLASH WRITE AND READ TEST ERROR!\r\n");
        }
#endif


    printf("total flash test complete\r\n");
}
#else
void flash_test(void)
{

}
#endif
