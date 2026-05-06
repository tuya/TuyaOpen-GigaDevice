/*!
    \file    qspi_nand_flash_api.c
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
#include "qspi_nand_flash_api.h"
#include "wrapper_os.h"
#include "ll.h"
#include "dbg_print.h"


#define    EXT_NANDFLASH_SIZE_INDX             30      // 2G bytes


#define    QSPI_POLLING_CYCLES                 0x10

#define    QSPI_NAND_READID              0x9F    //RDID (Read Identification)
#define    QSPI_NAND_GET_FEATURE         0x0F    //Get features
#define    QSPI_NAND_SET_FEATURE         0x1F    //Set features
#define    QSPI_NAND_PAGE_READ           0x13    //Array Read
#define    QSPI_NAND_PARAMETERPAGE_READ  0xEC    //Array Read
#define    QSPI_NAND_READ_CACHE          0x03    //Read From Cache
#define    QSPI_NAND_READ_CACHE2         0x3B    //Read From Cache*2
#define    QSPI_NAND_READ_CACHE4         0x6B    //Read From Cache*4
#define    QSPI_NAND_WREN                0x06    //Write Enable
#define    QSPI_NAND_WRDI                0x04    //Write Disable
#define    QSPI_NAND_PAGE_LOAD           0x02    //Page Program Load
#define    QSPI_NAND_PAGE_RAND_LOAD      0x84    //Page Program Random Input
#define    QSPI_NAND_PAGE_LOAD4          0x32    //Quad IO Page Program Load
#define    QSPI_NAND_PAGE_RAND_LOAD4     0x34    //Quad IO Page Program Random Input
#define    QSPI_NAND_PROGRAM_EXEC        0x10    //Program Execute
#define    QSPI_NAND_BLOCK_ERASE         0xD8    //BLock Erase
#define    QSPI_NAND_RESET               0xFF    //Nand Reset

/* SPI NAND memory parameters */
#define    QSPI_NAND_PAGE_SIZE           ((uint16_t)0x0800) /* 2 * 1024 bytes per page w/o Spare Area */
#define    QSPI_NAND_BLOCK_SIZE          ((uint16_t)0x0040) /* 64 pages per block */
#define    QSPI_NAND_SPARE_AREA_SIZE     ((uint16_t)0x0040) /* last 64 bytes as spare area */
#define    QSPI_NAND_PAGE_TOTAL_SIZE     (QSPI_NAND_PAGE_SIZE + QSPI_NAND_SPARE_AREA_SIZE)   /* the total sizze of page(page size + spare area size) */

#define    BI_OFFSET                    0       // the first byte in the blcok first page's spare area ,use for mark bad block
#define    PROTECTION                   0xA0    // nandflash protection register
#define    FEATURE1                     0xB0    // nandflash feature1 register
#define    STATUS                       0xC0    // nandflash status register
#define    FEATURE2                     0xD0    // nandflash feature2 register
#define    STATUS2                      0xF0    // nandflash status register

#define    NONE_VAL                     0x00

#define    OIP_BIT                      0x01     //Operation in progress bit
#define    WEL_VAL                      0x02
#define    WEL_BIT                      0x02     //Write enable latch bit

#define    E_FAIL_VAL                   0x04
#define    E_FAIL_BIT                   0x04     //Erase fail bit
#define    P_FAIL_BIT                   0x08     //Program fail bit

#define    ECCS0_BIT                    0x10     //ECCS0 bit
#define    ECCS1_VAl                    0x20
#define    ECCS1_BIT                    0x20     //ECCS1 bit


static void qspi_polling_match_status_flag(uint8_t status_reg, uint32_t match, uint32_t mask)
{
    qspi_polling_struct polling_cmd;
    qspi_command_struct qspi_cmd;

    polling_cmd.match            = match;
    polling_cmd.mask             = mask;
    polling_cmd.match_mode       = QSPI_MATCH_MODE_AND;
    polling_cmd.statusbytes_size = 1;
    polling_cmd.interval         = QSPI_POLLING_CYCLES;
    polling_cmd.polling_stop     = QSPI_POLLING_STOP_ENABLE;

    qspi_cmd.instruction         = QSPI_NAND_GET_FEATURE;
    qspi_cmd.data_mode           = QSPI_DATA_1_LINE;
    qspi_cmd.addr_mode           = QSPI_ADDR_NONE;
    qspi_cmd.instruction_mode    = QSPI_INSTRUCTION_1_LINE;
    qspi_cmd.addr                = 0;
    qspi_cmd.addr_size           = QSPI_ADDR_24_BITS;
    qspi_cmd.altebytes           = status_reg;
    qspi_cmd.altebytes_mode      = QSPI_ALTE_BYTES_1_LINE;
    qspi_cmd.altebytes_size      = QSPI_ALTE_BYTES_8_BITS;
    qspi_cmd.data_length         = 1;
    qspi_cmd.dummycycles         = 0;
    qspi_cmd.sioo_mode           = QSPI_SIOO_INST_EVERY_CMD;

    /* wait for the BUSY flag to be reset */
    while (RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_flag_clear(QSPI_FLAG_RPMF);
    qspi_polling_config(&qspi_cmd, &polling_cmd);

    /* wait for the match complete flag to be set */
    while (RESET == qspi_flag_get(QSPI_FLAG_RPMF)) {
    }
    qspi_flag_clear(QSPI_FLAG_RPMF);
}


static void qspi_nandflash_command(uint32_t instruction, uint32_t address, uint32_t dummy_cycles,
                                   uint32_t instruction_mode, uint32_t address_mode,
                                   uint32_t address_size, uint32_t data_mode, uint32_t altebytes_mode, uint32_t altebytes_size,
                                   uint32_t altebytes, uint32_t data_length)
{
    qspi_command_struct qspi_cmd;

    qspi_cmd.instruction      = instruction;
    qspi_cmd.instruction_mode = instruction_mode;
    qspi_cmd.addr             = address;
    qspi_cmd.addr_mode        = address_mode;
    qspi_cmd.addr_size        = address_size;
    qspi_cmd.altebytes        = altebytes;
    qspi_cmd.altebytes_mode   = altebytes_mode;
    qspi_cmd.altebytes_size   = altebytes_size;
    qspi_cmd.data_mode        = data_mode;
    qspi_cmd.data_length      = data_length;
    qspi_cmd.dummycycles      = dummy_cycles;
    qspi_cmd.sioo_mode        = QSPI_SIOO_INST_EVERY_CMD;
    qspi_command_config(&qspi_cmd);

}

static void qspi_nandflash_get_feature(uint8_t status_reg, uint8_t *status)
{
    qspi_command_struct qspi_cmd;
    /* wait for the BUSY flag to be reset */
    while (RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_cmd.instruction         = QSPI_NAND_GET_FEATURE;
    qspi_cmd.data_mode           = QSPI_DATA_1_LINE;
    qspi_cmd.addr_mode           = QSPI_ADDR_NONE;
    qspi_cmd.instruction_mode    = QSPI_INSTRUCTION_1_LINE;
    qspi_cmd.addr                = 0;
    qspi_cmd.addr_size           = QSPI_ADDR_24_BITS;
    qspi_cmd.altebytes           = status_reg;
    qspi_cmd.altebytes_mode      = QSPI_ALTE_BYTES_1_LINE;
    qspi_cmd.altebytes_size      = QSPI_ALTE_BYTES_8_BITS;
    qspi_cmd.data_length         = 1;
    qspi_cmd.dummycycles         = 0;
    qspi_cmd.sioo_mode           = QSPI_SIOO_INST_EVERY_CMD;

    qspi_command_config(&qspi_cmd);

    qspi_data_receive(status);

    /* wait for the BUSY flag to be reset */
    while (RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

}


static void qspi_nandflash_page_read(uint32_t page_no)
{
    /* wait for the BUSY flag to be reset */
    while (RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_nandflash_command(QSPI_NAND_PAGE_READ, page_no, 0, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_1_LINE,
                           QSPI_ADDR_24_BITS,
                           QSPI_DATA_NONE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, 0, 0);

    // sleep 1ms waiting for flash set OIP status flag
    // sys_ms_sleep(1);

    qspi_polling_match_status_flag(STATUS, NONE_VAL, OIP_BIT);

    /* wait for the BUSY flag to be reset */
    while (RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
}

static void qspi_nandflash_read_cache(uint8_t *buffer, uint16_t offset_in_page, uint32_t byte_cnt)
{
    uint32_t i = 0;
    /* wait for the BUSY flag to be reset */
    while (RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
    qspi_nandflash_command(QSPI_NAND_READ_CACHE, offset_in_page, 8, QSPI_INSTRUCTION_1_LINE,
                           QSPI_ADDR_1_LINE, QSPI_ADDR_16_BITS,
                           QSPI_DATA_1_LINE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, 0, byte_cnt);

    qspi_data_receive(buffer);

    while (RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
}

static void qspi_nandflash_set_register(uint8_t reg, uint8_t data)
{
    /* wait for the BUSY flag to be reset */
    while (RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    sys_enter_critical();
    qspi_nandflash_command(QSPI_NAND_SET_FEATURE, 0, 0, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_NONE,
                           QSPI_ADDR_24_BITS,
                           QSPI_DATA_1_LINE, QSPI_ALTE_BYTES_1_LINE, QSPI_ALTE_BYTES_8_BITS, reg, 1);

    qspi_data_transmit(&data);
    sys_exit_critical();

    /* wait for the data transmit completed */
    while (RESET == qspi_flag_get(QSPI_FLAG_TC)) {
    }
    /* clear the TC flag */
    qspi_flag_clear(QSPI_FLAG_TC);
}


static void qspi_nandflash_reset(void)
{
    /* wait for the BUSY flag */
    while (RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_nandflash_command(QSPI_NAND_RESET, 0, 0, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_NONE,
                           QSPI_ADDR_24_BITS,
                           QSPI_DATA_NONE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, 0, 0);

    qspi_polling_match_status_flag(STATUS, NONE_VAL, OIP_BIT);
}


static void qspi_nandflash_read_id(uint8_t *buff)
{
    /* wait for the BUSY flag */
    while (RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_nandflash_command(QSPI_NAND_READID, 0, 8, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_NONE,
                           QSPI_ADDR_24_BITS,
                           QSPI_DATA_1_LINE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, 0, 2);

    qspi_data_receive(buff);
    while (RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }
}

static void spi_nandflash_write_enable(void)
{
    /* wait for the BUSY flag */
    while (RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    qspi_nandflash_command(QSPI_NAND_WREN, 0, 0, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_NONE,
                           QSPI_ADDR_24_BITS,
                           QSPI_DATA_NONE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, 0, 0);

    qspi_polling_match_status_flag(STATUS, WEL_VAL, WEL_BIT);

}


/*!
    \brief      configure the QSPI peripheral
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void qspi_nandflash_init(void)
{
    qspi_init_struct qspi_init_para;

#if CONFIG_BOARD == PLATFORM_BOARD_32VW55X_START
    /* QSPI GPIO config:SCK/PA9, NSS/PA10, IO0/PA11, IO1/PA12 */
    gpio_af_set(GPIOA, GPIO_AF_4, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ,
                            GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);

    /* QSPI GPIO config: IO2/PB3, IO3/PB4 */
    gpio_af_set(GPIOB, GPIO_AF_3, GPIO_PIN_3 | GPIO_PIN_4);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3 | GPIO_PIN_4);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_3 | GPIO_PIN_4);
#elif (CONFIG_BOARD == PLATFORM_BOARD_32VW55X_EVAL || CONFIG_BOARD == PLATFORM_BOARD_32VW55X_SONIC)
    /* QSPI GPIO config:SCK/PA4, NSS/PA5, IO0/PA6, IO1/PA7 */
    gpio_af_set(GPIOA, GPIO_AF_3, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX,
                            GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
#endif
    /* initialize the init parameter structure */
    qspi_struct_para_init(&qspi_init_para);

#if 1
    qspi_init_para.clock_mode     = QSPI_CLOCK_MODE_3;
    qspi_init_para.fifo_threshold = 8;
    qspi_init_para.sample_shift   = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    qspi_init_para.cs_high_time   = QSPI_CS_HIGH_TIME_8_CYCLE;
    qspi_init_para.flash_size     = EXT_NANDFLASH_SIZE_INDX;
    qspi_init_para.prescaler      = 1;
#else
    // for ellisy capture
    qspi_init_para.clock_mode     = QSPI_CLOCK_MODE_0;
    qspi_init_para.fifo_threshold = 4;
    qspi_init_para.sample_shift   = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    qspi_init_para.cs_high_time   = QSPI_CS_HIGH_TIME_2_CYCLE;
    qspi_init_para.flash_size     = EXT_NANDFLASH_SIZE_INDX;
    qspi_init_para.prescaler      = 15;
#endif
    qspi_init(&qspi_init_para);

    qspi_enable();
}


flash_err_t qspi_nandflash_block_erase(uint16_t block_no)
{
    uint8_t status = 0;
    flash_err_t result = FLASH_ERR_NO_ERROR;
    uint8_t retry_cnt = 0;

    block_no *= QSPI_NAND_BLOCK_SIZE;        //block_no = block_no*64

    do {
        spi_nandflash_write_enable();

        qspi_nandflash_command(QSPI_NAND_BLOCK_ERASE, block_no, 0, QSPI_INSTRUCTION_1_LINE, QSPI_ADDR_1_LINE,
                               QSPI_ADDR_24_BITS,
                               QSPI_DATA_NONE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, 0, 0);

        qspi_polling_match_status_flag(STATUS, NONE_VAL, OIP_BIT);

        qspi_nandflash_get_feature(STATUS, &status);

        if ((status & E_FAIL_BIT) == E_FAIL_VAL) {
            result = FLASH_ERR_ERASE_FAIL;
        }
        else {
            result = FLASH_ERR_NO_ERROR;
        }

        retry_cnt++;
    } while (result && retry_cnt < 3);

    return result;
}


flash_err_t qspi_nandflash_read(uint8_t *buffer, uint16_t page_no, uint16_t offset_in_page,
                        uint32_t byte_cnt)
{
    uint8_t status = 0;
    uint8_t retrycnt = 0;
    flash_err_t result = FLASH_ERR_NO_ERROR;


    /* the capacity of page must be equal or greater than the taotal of offset_in_page and byte_cnt */
    if ((offset_in_page + byte_cnt) > QSPI_NAND_PAGE_TOTAL_SIZE) {
        return FLASH_ERR_OUT_OF_BOUND;
    }

    do {
        qspi_nandflash_page_read(page_no);

        qspi_nandflash_read_cache(buffer, offset_in_page, byte_cnt);

        qspi_nandflash_get_feature(STATUS, &status);
        if ((status & ECCS0_BIT) == 0 && (status & ECCS1_BIT) == ECCS1_VAl) {   //UECC
            result = FLASH_ERR_ECC_ERR;
        } else {
            result = FLASH_ERR_NO_ERROR;
            break;
        }

        retrycnt++;
    } while (retrycnt < 3);

    // printf("qspi_nandflash_read page_no %d, offset_in_page 0x%x, len %d end 0x%x\r\n", page_no, offset_in_page, byte_cnt, result);

    return result;
}

bool qspi_nandflash_check_is_badpage_by_read(uint16_t page_no)
{
    uint8_t status = 0;
    uint8_t retrycnt = 0;
    flash_err_t result = FLASH_ERR_NO_ERROR;


    do {
        qspi_nandflash_page_read(page_no);

        qspi_nandflash_get_feature(STATUS, &status);
        if ((status & ECCS0_BIT) == 0 && (status & ECCS1_BIT) == ECCS1_VAl) {   //UECC
            result = FLASH_ERR_ECC_ERR;
        } else {
            result = FLASH_ERR_NO_ERROR;
            break;
        }

        qspi_nandflash_reset();
        retrycnt++;
    } while (retrycnt < 3);

    return result == FLASH_ERR_ECC_ERR;
}


flash_err_t qspi_nandflash_write(uint8_t *buffer, uint16_t page_no, uint16_t offset_in_page,
                         uint32_t byte_cnt)
{
    // printf("qspi_nandflash_write page_no %d, offset 0x%x, len %d \r\n", page_no, offset_in_page, byte_cnt);

    /* the capacity of page must be equal or greater than the taotal of offset_in_page and byte_cnt */
    if ((offset_in_page + byte_cnt) > QSPI_NAND_PAGE_TOTAL_SIZE) {
        return FLASH_ERR_OUT_OF_BOUND;
    }

    /* wait for the BUSY flag to be reset */
    while (RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    /*sned the program load command,write data to cache*/
    qspi_nandflash_command(QSPI_NAND_PAGE_LOAD, offset_in_page, 0, QSPI_INSTRUCTION_1_LINE,
                           QSPI_ADDR_1_LINE, QSPI_ADDR_16_BITS,
                           QSPI_DATA_1_LINE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, 0, byte_cnt);

    qspi_data_transmit(buffer);

    spi_nandflash_write_enable();

    /*sned the program excute command*/
    qspi_nandflash_command(QSPI_NAND_PROGRAM_EXEC, page_no, 0, QSPI_INSTRUCTION_1_LINE,
                           QSPI_ADDR_1_LINE, QSPI_ADDR_24_BITS,
                           QSPI_DATA_NONE, QSPI_ALTE_BYTES_NONE, QSPI_ALTE_BYTES_8_BITS, 0, 0);

    qspi_polling_match_status_flag(STATUS, NONE_VAL, OIP_BIT);

    // FIX TODO polling P_FAIL

    /* wait for the BUSY flag to be reset */
    while (RESET != qspi_flag_get(QSPI_FLAG_BUSY)) {
    }

    return FLASH_ERR_NO_ERROR;
}


// This function only can be called when nand flash is first used and never operated before.
bool qspi_nandflash_is_badblock(uint16_t block_no)
{
    uint8_t uc_flag = 0, back_feature;
    bool res = false;

    qspi_nandflash_get_feature(FEATURE1, &back_feature);

    qspi_nandflash_set_register(FEATURE1, back_feature&0xEF);       //close ECC

    /* read the bad flag in spare area */
    qspi_nandflash_page_read(block_no * QSPI_NAND_BLOCK_SIZE);

    qspi_nandflash_read_cache(&uc_flag, QSPI_NAND_PAGE_SIZE + BI_OFFSET, 1);


    if (uc_flag != 0xFF){
        dbg_print(ERR, "check block page 0 IsBadBlock:0x%x\r\n", block_no);
        res = true;
    }

    /* read the bad flag in spare area(reserve area) in first page of block */
    qspi_nandflash_page_read(block_no * QSPI_NAND_BLOCK_SIZE + 1);

    qspi_nandflash_read_cache(&uc_flag, QSPI_NAND_PAGE_SIZE + BI_OFFSET, 1);

    if (uc_flag != 0xFF){
        dbg_print(ERR, "check block page 1 IsBadBlock:0x%x\r\n", block_no);
        res = true;
    }

    qspi_nandflash_set_register(FEATURE1, back_feature);

    return res;
}


bool qspi_nandflash_api_init(void)
{
    uint8_t id[2] = {0};

    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_QSPI);

    qspi_nandflash_init();

    /* reset NANDFLASH */
    qspi_nandflash_reset();

    /* configure the PROTECTION register*/
    qspi_nandflash_set_register(PROTECTION, 0x00);

    /* configure the FEATURE1 register*/
    qspi_nandflash_set_register(FEATURE1, 0x11);

    qspi_nandflash_read_id(id);

    dbg_print(DEBUG, "qspi_nandflash_api_init read unique id 0x%x, 0x%x\r\n", id[0], id[1]);
    return true;
}

uint8_t tx_nandbuffer[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };
uint8_t rx_nandbuffer[QSPI_NAND_PAGE_SIZE];

void nandflash_test(void)
{
    uint32_t i = 0;
    uint8_t data, page_no = 0;
    uint16_t block_no = 0;
    uint16_t offset_in_page = 0;
    int result = 0;
    flash_err_t res;

    printf("QSPI nandflash writing...\r\n");
    qspi_nandflash_api_init();

    res = qspi_nandflash_block_erase(0);
    printf("nandflash_test block 0 erase res %d \r\n", res);

    qspi_nandflash_write(tx_nandbuffer, 0, 0, sizeof(tx_nandbuffer));

    qspi_nandflash_page_read(0);

    qspi_nandflash_get_feature(STATUS, &data);
    qspi_nandflash_reset();

    printf("read status data %d \r\n", data);



    for (i = 0; i < 3; i++) {
        sys_random_bytes_get(&block_no, 2);
        block_no %= 2048;
        sys_random_bytes_get(&page_no, 1);
        page_no %= 64;
        sys_random_bytes_get(&offset_in_page, 2);
        offset_in_page %= (2048 - sizeof(tx_nandbuffer));

        qspi_nandflash_block_erase(block_no);

        qspi_nandflash_write(tx_nandbuffer, block_no * QSPI_NAND_BLOCK_SIZE + page_no, offset_in_page,
                             sizeof(tx_nandbuffer));

        sys_memset(rx_nandbuffer, 0, sizeof(QSPI_NAND_PAGE_SIZE));
        result = qspi_nandflash_read(rx_nandbuffer, block_no * QSPI_NAND_BLOCK_SIZE + page_no,
                                     offset_in_page, sizeof(tx_nandbuffer));
        if (result == 0) {
            if (sys_memcmp(tx_nandbuffer, rx_nandbuffer, sizeof(tx_nandbuffer))) {
                printf("!!!!!!!! QSPI nandflash block %d, page %d, offset_in_page %d write and read fail !!!!!!!!!\r\n",
                       block_no, page_no, offset_in_page);
            } else {
                printf("\r\n");
                printf("### QSPI nandflash block %d, page %d, offset_in_page %d write and read success ####\r\n",
                       block_no, page_no, offset_in_page);
                printf("\r\n");
            }
        } else {
            printf("!!!!!!!!! QSPI nandflash block %d, page %d, offset_in_page %d read fail(%d) !!!!!!!!!\r\n",
                   block_no, page_no, offset_in_page, result);
        }
    }

    printf("total nandflash test complete\r\n");
}



