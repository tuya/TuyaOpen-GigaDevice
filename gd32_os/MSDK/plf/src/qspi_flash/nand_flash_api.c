/*!
    \file    nand_flash_api.c
    \brief   Write and read nand flash

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

/*
optimize use bit0 to mark bad block
only record replace block
use spare data to record info
*/

#define PLATFORM_ASSERT_ENABLE

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "app_cfg.h"
#include "gd32vw55x.h"
#include "nand_flash_api.h"
#include "qspi_nand_flash_api.h"
#include "wrapper_os.h"
#include "ll.h"
#include "dbg_print.h"
#include "plf_assert.h"

#include "arch.h"

#define    USER_AREA_START              /*0*/     0
#define    USER_AREA_END                /*1899*/   (USER_AREA_START + BLOCK_NUM_FOR_USER - 1)
#define    ReplaceBlock_AREA_START      /*1900*/   (USER_AREA_END + 1)
#define    ReplaceBlock_AREA_END        /*1997*/   (ReplaceBlock_AREA_START + BLOCK_NUM_FOR_ReplaceBlock - 1)
#define    Table_AREA_START             /*1998*/   (ReplaceBlock_AREA_END + 1)
#define    Table_AREA_END               /*2047*/  (Table_AREA_START+BLOCK_NUM_FOR_Table - 1)


#define TYPE_BBT        0
#define TYPE_RBT        1

#define NAND_SPARE_DATA_OFFSET    4

#define BBT_SIZE    200
#define RBT_SIZE    BLOCK_NUM_FOR_USER

#define BBT_BAD_NUM_IDX           0
#define BBT_LAST_REP_VALID_IDX    1
#define BBT_LAST_TAB_VALID_IDX    2
#define BBT_START_BAD_IDX         3
#define BBT_MAX_REC_BB_NUM      (BBT_SIZE - BBT_START_BAD_IDX)

#define INVAL_PAGE_VAL            0xFF
#define PRINT_DEBUG_INFO          1

// index 0: bad block number; index 1: record last valid block; index 2: record last table valid block
static uint16_t BBT[BBT_SIZE] = {0};

static uint16_t RBT[RBT_SIZE] = {0};


#define    BBT_BLOCK_NUM                2
static uint16_t bbt_block[BBT_BLOCK_NUM] = {0};
static uint8_t  bbt_crt_block_idx = 0;   //0 or 1
static uint8_t  bbt_crt_page = INVAL_PAGE_VAL;

#define    RBT_BLOCK_NUM                2
static uint16_t rbt_block[RBT_BLOCK_NUM] = {0};
static uint8_t  rbt_crt_block_idx = 0;
static uint8_t  rbt_crt_page = INVAL_PAGE_VAL;

static uint16_t last_valid_rep_block = 0;
static uint16_t last_valid_tab_block = 0;
static uint16_t bad_block_num = 0;


static uint8_t *p_temp_buf = NULL;

static bool update_bbtrbt_2_nand(uint8_t type);

static bool check_is_in_bbt(uint16_t block_no)
{
    uint16_t  i;

    for (i = 0; i < bad_block_num; i++) {
        if (BBT[BBT_START_BAD_IDX + i] == block_no) {
            return true;
        }
    }
    return false;
}

static uint16_t get_replace_block_from_rbt(uint16_t block_no)
{
    return RBT[block_no];
}

static bool update_bb_2_bbtrbt(uint16_t user_block_no, uint16_t old_replace_block_no)
{
    bool result = false;

    if(user_block_no > USER_AREA_END){
        dbg_print(ERR, "update_bb_2_bbtrbt block_no(%d) is not user area\r\n", user_block_no);
        return false;
    }

    dbg_print(NOTICE, "update_bb_2_bbtrbt user_block_no %d old_replace_block_no %d\r\n", user_block_no, old_replace_block_no);

    // First find user area block is bad
    if (!check_is_in_bbt(user_block_no)) {
        PLF_ASSERT_ERR(user_block_no == old_replace_block_no);
        if(bad_block_num >= BBT_MAX_REC_BB_NUM){
            dbg_print(ERR, "update_bb_2_bbtrbt check user block %d is larger than %d\r\n", user_block_no, BBT_MAX_REC_BB_NUM);
            return false;
        }

        bad_block_num++;
        BBT[BBT_BAD_NUM_IDX] = bad_block_num;        //bad block number
        BBT[BBT_START_BAD_IDX + bad_block_num - 1] = user_block_no;
    }
    else {
        PLF_ASSERT_ERR(user_block_no != old_replace_block_no);
        PLF_ASSERT_ERR((ReplaceBlock_AREA_START <= old_replace_block_no && old_replace_block_no <= ReplaceBlock_AREA_END));
        if (!check_is_in_bbt(old_replace_block_no)) {
            if(bad_block_num >= BBT_MAX_REC_BB_NUM){
                dbg_print(ERR, "update_bb_2_bbtrbt check replace block %d is larger than %d\r\n", old_replace_block_no, BBT_MAX_REC_BB_NUM);
                PLF_ASSERT_ERR(false);
                return false;
            }

            bad_block_num++;
            BBT[BBT_BAD_NUM_IDX] = bad_block_num;        //bad block number
            BBT[BBT_START_BAD_IDX + bad_block_num - 1] = old_replace_block_no;
        }
        else {
            PLF_ASSERT_ERR(false);
        }
    }

    // Find valid block for replace bad user block
    if(last_valid_rep_block < ReplaceBlock_AREA_START){
        dbg_print(ERR, "update_bb_2_bbtrbt Replace Block are not enough %d\r\n", last_valid_rep_block);
        PLF_ASSERT_ERR(false);
        return false;
    }

    while (last_valid_rep_block >= ReplaceBlock_AREA_START) {
        if (!check_is_in_bbt(last_valid_rep_block)) {
            RBT[user_block_no] = last_valid_rep_block;
            dbg_print(NOTICE, "update_bb_2_bbtrbt replace user block %d to new block %d\r\n", user_block_no, last_valid_rep_block);
            last_valid_rep_block--;
            result = true;
            break;
        }

        last_valid_rep_block--;
    }

    BBT[BBT_LAST_REP_VALID_IDX] = last_valid_rep_block;     //record last valid block

    return result;
}

static uint16_t find_alloc_a_new_table_block(void)
{
    uint16_t block_no = last_valid_tab_block;

    for(; block_no > Table_AREA_START; block_no--){
        if(!check_is_in_bbt(block_no)) {      // good block
            break;
        }
    }

    // Can't find a new good table block
    if (block_no == Table_AREA_START) {
        last_valid_tab_block = block_no;
        BBT[BBT_LAST_TAB_VALID_IDX] = last_valid_tab_block;
        return 0xFFFF;
    }
    else {
        last_valid_tab_block = block_no - 1;
        BBT[BBT_LAST_TAB_VALID_IDX] = last_valid_tab_block;
        return block_no;
    }
}

static flash_err_t update_bbt_block_2_nand(bool *erase_bbt)
{
    uint16_t  block_no;
    uint16_t  page_no;

    flash_err_t result = FLASH_ERR_NO_ERROR;
    uint8_t *temp_buf = NULL;

    temp_buf = sys_malloc(NAND_PAGE_TOTAL_SIZE);

    if (temp_buf == NULL) {
        dbg_print(ERR, "update_bbt_block_2_nand malloc temp buf fail\r\n");
        return FLASH_ERR_MALLOC_FAIL;
    }

    if((bbt_crt_page >= (NAND_BLOCK_SIZE-1))&&(bbt_crt_page != INVAL_PAGE_VAL)){
        bbt_crt_block_idx = 1 - bbt_crt_block_idx;  //toggle block idx
        bbt_crt_page = 0;

        // First erase the pingpang block
        qspi_nandflash_block_erase(bbt_block[bbt_crt_block_idx]);
        if (erase_bbt) {
            *erase_bbt = true;
        }
    } else {
        if(bbt_crt_page == INVAL_PAGE_VAL){
            bbt_crt_page = 0;
        }
        else{
            bbt_crt_page++;
        }
    }

    block_no = bbt_block[bbt_crt_block_idx];

    sys_memset(temp_buf, 0xFF, NAND_PAGE_TOTAL_SIZE);
    sys_memcpy(temp_buf, BBT, BBT_SIZE*sizeof(BBT[0]));

    temp_buf[NAND_PAGE_SIZE+NAND_SPARE_DATA_OFFSET+0]= 'D';    //0x44
    temp_buf[NAND_PAGE_SIZE+NAND_SPARE_DATA_OFFSET+1]= 'B';    //0x42
    temp_buf[NAND_PAGE_SIZE+NAND_SPARE_DATA_OFFSET+2]= 'T';    //0x54
    temp_buf[NAND_PAGE_SIZE+NAND_SPARE_DATA_OFFSET+3]= '!';    //0x21
    temp_buf[NAND_PAGE_SIZE+NAND_SPARE_DATA_OFFSET+4]= (bbt_block[1 - bbt_crt_block_idx]&0xFF);         //record the pair block No
    temp_buf[NAND_PAGE_SIZE+NAND_SPARE_DATA_OFFSET+5]= (bbt_block[1 - bbt_crt_block_idx]&0xFF00)>>8;


    result = qspi_nandflash_write(temp_buf, block_no * NAND_BLOCK_SIZE + bbt_crt_page, 0, NAND_PAGE_TOTAL_SIZE);

    // FIX TODO
    // Write fail or ECC check is wrong
    if (result || qspi_nandflash_check_is_badpage_by_read(block_no * NAND_BLOCK_SIZE + bbt_crt_page)) {
        dbg_print(ERR, "update_bbt_block_2_nand write or ecc check fail(%d)\r\n", result);

        do {
            // If not in bad block table, add it!
            if (!check_is_in_bbt(block_no)) {
                if (bad_block_num >= BBT_MAX_REC_BB_NUM) {
                    dbg_print(ERR, "update_bbt_block_2_nand bad block is larger than %d\r\n", BBT_MAX_REC_BB_NUM);
                    PLF_ASSERT_ERR(false);
                    break;;
                }

                bad_block_num++;
                BBT[BBT_BAD_NUM_IDX] = bad_block_num;        //bad block number
                BBT[BBT_START_BAD_IDX + bad_block_num - 1] = block_no;
            }

            block_no = find_alloc_a_new_table_block();
            // alloc fail
            if (block_no == 0xFFFF) {
                dbg_print(ERR, "update_bbt_block_2_nand can't find a new table block\r\n");
                PLF_ASSERT_ERR(false);
                result = FLASH_ERR_NO_TAB_BLOCK;
                break;
            }

            sys_memcpy(temp_buf, BBT, BBT_SIZE*sizeof(BBT[0]));

            result = qspi_nandflash_block_erase(block_no);

            // Replace a new block, no need to move old data to new block, if erase fail start to find next table block
            if (FLASH_ERR_NO_ERROR == result) {
                result = qspi_nandflash_write(temp_buf, block_no * NAND_BLOCK_SIZE + 0, 0, NAND_PAGE_TOTAL_SIZE);
                // FIX TODO
                if (result || qspi_nandflash_check_is_badpage_by_read(block_no * NAND_BLOCK_SIZE + 0)) {
                    result = FLASH_ERR_ECC_ERR;
                    qspi_nandflash_block_erase(block_no);
                }
            }
        } while (result);
    }


    if(result != 0){
        dbg_print(ERR, "update_bbt_block_2_nand result is 0x%x\r\n", result);
        sys_mfree(temp_buf);
        return result;
    }

    //Find a new block, so set current page to 0 and try to erase old bbt block
    if (bbt_block[bbt_crt_block_idx] != block_no) {
        qspi_nandflash_block_erase(bbt_block[bbt_crt_block_idx]);
        bbt_block[bbt_crt_block_idx] = block_no;
        bbt_crt_page = 0;
    }
    sys_mfree(temp_buf);

    return result;
}

static flash_err_t update_rbt_block_2_nand(bool *erase_rbt)
{
    uint16_t  block_no;
    uint16_t  page_no, page_num;
    uint16_t  len = sizeof(RBT);
    uint16_t  idx = 0;
    uint8_t  crt_page = 0;
    bool need_sv_bbt = false;

    flash_err_t result = FLASH_ERR_NO_ERROR;
    uint8_t *temp_buf = NULL;


    temp_buf = sys_malloc(NAND_PAGE_TOTAL_SIZE);

    if (temp_buf == NULL) {
        dbg_print(ERR, "update_rbt_block_2_nand malloc temp buf fail\r\n");
        return FLASH_ERR_MALLOC_FAIL;
    }

    page_num = sizeof(RBT) / NAND_PAGE_SIZE;
    if ((sizeof(RBT) % NAND_PAGE_SIZE) != 0) {
        page_num++;
    }

    if(((rbt_crt_page + page_num)  > (NAND_BLOCK_SIZE - 1)) && (rbt_crt_page != INVAL_PAGE_VAL)){
        rbt_crt_block_idx = 1 - rbt_crt_block_idx;  //toggle block idx
        rbt_crt_page = 0;

        // First erase the pingpang block
        qspi_nandflash_block_erase(rbt_block[rbt_crt_block_idx]);
        if (erase_rbt) {
            *erase_rbt = true;
        }
    }
    else{
        if(rbt_crt_page == INVAL_PAGE_VAL){
            rbt_crt_page=0;
        }
        else{
            rbt_crt_page++;
        }
    }


    block_no = rbt_block[rbt_crt_block_idx];
    crt_page = rbt_crt_page;

    do {
        idx = 0;
        result = FLASH_ERR_NO_ERROR;
        uint8_t *p_buf = (uint8_t *)RBT;
        while(idx < len) {
            uint16_t copy_len = (len - idx) > NAND_PAGE_SIZE ? NAND_PAGE_SIZE : (len - idx);

            sys_memset(temp_buf, 0xFF, NAND_PAGE_TOTAL_SIZE);
            sys_memcpy(temp_buf, p_buf + idx, copy_len);

            temp_buf[NAND_PAGE_SIZE+NAND_SPARE_DATA_OFFSET+0]= 'R';    //0x52
            temp_buf[NAND_PAGE_SIZE+NAND_SPARE_DATA_OFFSET+1]= 'B';    //0x42
            temp_buf[NAND_PAGE_SIZE+NAND_SPARE_DATA_OFFSET+2]= 'T';    //0x54
            temp_buf[NAND_PAGE_SIZE+NAND_SPARE_DATA_OFFSET+3]= '!';    //0x21
            temp_buf[NAND_PAGE_SIZE+NAND_SPARE_DATA_OFFSET+4]= (rbt_block[1-rbt_crt_block_idx]&0xFF);         //record the pair block No
            temp_buf[NAND_PAGE_SIZE+NAND_SPARE_DATA_OFFSET+5]= (rbt_block[1-rbt_crt_block_idx]&0xFF00)>>8;

            result = qspi_nandflash_write(temp_buf, block_no * NAND_BLOCK_SIZE + crt_page, 0, NAND_PAGE_TOTAL_SIZE);

            // Write fail or ECC check fail
            if (result || qspi_nandflash_check_is_badpage_by_read(block_no * NAND_BLOCK_SIZE + crt_page)) {
                dbg_print(ERR, "update_rbt_block_2_nand write or ecc %d block fail 0x%x, \r\n", block_no, result);

                result = FLASH_ERR_ECC_ERR;
                break;
            }
            else {
                crt_page++;
                idx += copy_len;
            }
        }


        // Write fail or ECC check fail
        if (result) {
            crt_page = 0;

            // Try to find a new table block for rbt
            do {
                // If not in bad block table, add it!
                if (!check_is_in_bbt(block_no)) {
                    if (bad_block_num >= BBT_MAX_REC_BB_NUM) {
                        dbg_print(ERR, "update_rbt_block_2_nand bad block is larger than %d\r\n", BBT_MAX_REC_BB_NUM);
                        PLF_ASSERT_ERR(false);
                        result = FLASH_ERR_OUT_OF_BBT_BOUND;
                        break;;
                    }

                    bad_block_num++;
                    BBT[BBT_BAD_NUM_IDX] = bad_block_num;        //bad block number
                    BBT[BBT_START_BAD_IDX + bad_block_num - 1] = block_no;
                    need_sv_bbt = true;
                }

                // Try to erase new bad block
                if (block_no != rbt_block[rbt_crt_block_idx])
                    qspi_nandflash_block_erase(block_no);

                block_no = find_alloc_a_new_table_block();
                // alloc fail
                if (block_no == 0xFFFF) {
                    dbg_print(ERR, "update_rbt_block_2_nand can't find a new table block\r\n");
                    PLF_ASSERT_ERR(false);
                    result = FLASH_ERR_NO_TAB_BLOCK;
                    break;
                }

                result = qspi_nandflash_block_erase(block_no);
            } while (result);

            // Find a new table block and erase ok or can't find a new table block
            if (result == FLASH_ERR_NO_TAB_BLOCK) {
                break;
            }
            else {
                result = FLASH_ERR_ECC_ERR;  // set fake ecc error in order to restart program
            }
        }
    } while (result);

    if (need_sv_bbt) {
        update_bbtrbt_2_nand(TYPE_BBT);
    }

    if(result != 0){
        dbg_print(ERR, "update_rbt_block_2_nand result is 0x%x\r\n", result);
        sys_mfree(temp_buf);
        return result;
    }

    // find a new block to replace old bad block
    if (rbt_block[rbt_crt_block_idx] != block_no) {
        qspi_nandflash_block_erase(rbt_block[rbt_crt_block_idx]);
        rbt_block[rbt_crt_block_idx] = block_no;
    }
    rbt_crt_page = crt_page - 1;      // last used page number

    sys_mfree(temp_buf);

    return result;
}


static bool update_bbtrbt_2_nand(uint8_t type)
{
    uint16_t  block_no;
    uint16_t  page_no;
    bool   bbt_erase_flag = false, rbt_erase_flag = false;
    flash_err_t   result;

    /* update BBT table */
    if(type == TYPE_BBT)
    {
        result = update_bbt_block_2_nand(&bbt_erase_flag);
        if(result) {
            dbg_print(ERR, "update_bbtrbt_2_nand update bbt fail result is 0x%x\r\n", result);
            return false;
        }

        block_no = bbt_block[1- bbt_crt_block_idx]; // another bbt block

        if(bbt_erase_flag) {
            result = qspi_nandflash_block_erase(block_no);
            // Block may be bad, next time programing this block may be fail, it will trigger to find a new block replace of this bad block
            if (result) {
                dbg_print(ERR, "update_bbtrbt_2_nand erase bbt block %d result is 0x%x\r\n", block_no, result);
            }
        }
    }

    /* update RBT table */
    else if(type==TYPE_RBT)
    {
        result = update_rbt_block_2_nand(&rbt_erase_flag);

        if(result) {
            dbg_print(ERR, "update_bbtrbt_2_nand update rbt fail result is 0x%x\r\n", result);
            return false;
        }

        block_no = rbt_block[1- rbt_crt_block_idx];   //another rbt block

        if(rbt_erase_flag) {
            result = qspi_nandflash_block_erase(block_no);
            // Block may be bad, next time programing this block may be fail, it will trigger to find a new block replace of this bad block
            if (result) {
                dbg_print(ERR, "update_bbtrbt_2_nand erase rbt block %d result is 0x%x\r\n", block_no, result);
            }
        }
    }

    else {
        dbg_print(ERR, "update_bbtrbt_2_nand unkown type 0x%x\r\n", type);
        return false;
    }
    return true;
}

static void load_bbtrbt_from_nand(uint8_t type)
{
    flash_err_t result;
    /* load BBT table */
    if(type == TYPE_BBT){
        result = qspi_nandflash_read((uint8_t *)BBT, bbt_block[bbt_crt_block_idx] * NAND_BLOCK_SIZE + bbt_crt_page, 0,  BBT_SIZE*sizeof(BBT[0]));
        if (result) {
            dbg_print(ERR, "Load with reading bbt block(%d) page(%d) fail(%d)\r\n", bbt_block[bbt_crt_block_idx], bbt_crt_page, result);
            PLF_ASSERT_ERR(false);
        }
        dbg_print(NOTICE, "Load with reading bbt block(%d) page(%d) complete!\r\n", bbt_block[bbt_crt_block_idx], bbt_crt_page);
    }

    /* load RBT table */
    if(type == TYPE_RBT){
        uint16_t len = sizeof(RBT);
        uint16_t idx = 0;
        uint16_t total_page = len / NAND_PAGE_SIZE;
        uint8_t *p_buf = (uint8_t *)RBT;

        if ((len % NAND_PAGE_SIZE) != 0) {
            total_page++;
        }

        while(idx < len) {
            uint16_t copy_len = (len - idx) > NAND_PAGE_SIZE ? NAND_PAGE_SIZE : (len - idx);
            result = qspi_nandflash_read(p_buf + idx, rbt_block[rbt_crt_block_idx] * NAND_BLOCK_SIZE + rbt_crt_page - (total_page - 1), 0, copy_len);

            if (result) {
                dbg_print(ERR, "Load with reading rbt block(%d) page(%d, %d) fail(%d)\r\n", rbt_block[rbt_crt_block_idx], rbt_crt_page, total_page, result);
                PLF_ASSERT_ERR(false);
            }

            idx += copy_len;
            total_page--;
        }

        dbg_print(NOTICE, "Load with reading rbt block(%d) page(%d) idx(%d), len(%d) complete!\r\n", rbt_block[rbt_crt_block_idx], rbt_crt_page, idx, len);
#if 0
        for(uint16_t i = 0; i < RBT_BLOCK_NUM; i++) {
            if (((i + 1) % 16) == 0) {
                dbg_print(NOTICE, "0x%x\r\n", RBT[i]);
            }
            else {
                dbg_print(NOTICE, "0x%x ", RBT[i]);
            }
        }
#endif

    }


}


static bool alloc_bbtrbt_block_addr(void)
{
    uint16_t block_no = Table_AREA_END;
    uint8_t  idx;

    //alloc BBT block
    for(idx = 0;idx < BBT_BLOCK_NUM;idx++){
        for(; block_no > Table_AREA_START; block_no--){
            if(!check_is_in_bbt(block_no)){      // good block
                bbt_block[idx] = block_no;
                dbg_print(NOTICE, "BBT_Block[%x]=%x\r\n", idx, bbt_block[idx]);
                qspi_nandflash_block_erase(block_no);
                block_no--;
                break;
            }
        }
    }

    bbt_crt_block_idx = 0;
    bbt_crt_page = INVAL_PAGE_VAL;

     //alloc RBT block
    for(idx = 0;idx < RBT_BLOCK_NUM;idx++){
        for(; block_no > Table_AREA_START; block_no--){
            if(!check_is_in_bbt(block_no)){
                rbt_block[idx] = block_no;
                dbg_print(NOTICE, "RBT_Block[%x]=%x\r\n", idx, rbt_block[idx]);
                qspi_nandflash_block_erase(block_no);
                block_no--;
                break;
            }
        }
    }
    rbt_crt_block_idx = 0;
    rbt_crt_page = INVAL_PAGE_VAL;

    last_valid_tab_block = block_no;

    // Check bbt_block[] and rbt_block[] alloc success
    for(idx = 0;idx < BBT_BLOCK_NUM;idx++){
        if (bbt_block[idx] < Table_AREA_START) {
            dbg_print(ERR, "BBT_Block[%x]=%x alloc fail\r\n", idx, bbt_block[idx]);
            PLF_ASSERT_ERR(false);
            return false;
        }
    }

    for(idx = 0;idx < RBT_BLOCK_NUM;idx++){
        if (rbt_block[idx] < Table_AREA_START) {
            dbg_print(ERR, "RBT_Block[%x]=%x alloc fail\r\n", idx, rbt_block[idx]);
            PLF_ASSERT_ERR(false);
            return false;
        }
    }

    BBT[BBT_LAST_TAB_VALID_IDX] = last_valid_tab_block;

    return true;
}

static bool scan_all_2_build_bbtrbt(void)
{
    uint16_t block_no = 0;

    // Check user and replace area to record bad block
    for(block_no = 0; block_no <= Table_AREA_END; block_no++){
        // bad block
        if(qspi_nandflash_is_badblock(block_no)) {
            if (bad_block_num >= BBT_MAX_REC_BB_NUM) {
                dbg_print(ERR, "scan all flash bad block is larger than %d\r\n", BBT_MAX_REC_BB_NUM);
                PLF_ASSERT_ERR(false);
                continue;
            }

            bad_block_num++;
            BBT[BBT_START_BAD_IDX + bad_block_num - 1] = block_no;
        }
    }
    BBT[BBT_BAD_NUM_IDX] = bad_block_num;

    // Replace user bad block
    last_valid_rep_block = ReplaceBlock_AREA_END;
    for(block_no = 0;block_no <= USER_AREA_END;block_no++) {
        if(last_valid_rep_block >= ReplaceBlock_AREA_START && check_is_in_bbt(block_no)) {     // bad block
            while(check_is_in_bbt(last_valid_rep_block)){    // bad block
                last_valid_rep_block--;
                if(last_valid_rep_block < ReplaceBlock_AREA_START ){
                    dbg_print(ERR, "block_no %d flash not enough replace flash \r\n", block_no);
                    PLF_ASSERT_ERR(false);
                    return false;
                }
            }

            RBT[block_no] = last_valid_rep_block;
            last_valid_rep_block--;
            continue;
        }
        else{
            RBT[block_no] = block_no;
        }

        if (last_valid_rep_block < ReplaceBlock_AREA_START) {
            dbg_print(ERR, "scan all flash replace block is empty!! \r\n", block_no);
            PLF_ASSERT_ERR(false);
        }
    }

    BBT[BBT_LAST_REP_VALID_IDX] = last_valid_rep_block;

    return true;
}

static bool check_is_bad_block_by_read(uint16_t  block_no)
{
    uint16_t  page_no = 0;

    for(page_no = 0; page_no < NAND_BLOCK_SIZE; page_no++) {
        if (qspi_nandflash_check_is_badpage_by_read(block_no * NAND_BLOCK_SIZE + page_no)) {
            return true;
        }
    }

    return false;
}

static bool rebuild_bbtrbt(void)
{
    uint16_t  block_no = NAND_BLOCK_COUNT - 1;            //1023
    uint16_t  page_no = 0;                                //0~63
    bool      flag_bbt_exist = false;
    bool      flag_rbt_exist = false;
    uint16_t  pair_block_no;
    uint8_t   spare_data[6] = {0};
    flash_err_t result;

    //check BBT whether exist in nand
    for(block_no = Table_AREA_END; block_no >= Table_AREA_START; block_no--) {
        page_no = 0;

        result = qspi_nandflash_read(spare_data, block_no * NAND_BLOCK_SIZE + page_no, NAND_PAGE_SIZE + NAND_SPARE_DATA_OFFSET, 6);
        if (result == FLASH_ERR_NO_ERROR) {
            if((spare_data[0]=='D')&&(spare_data[1]=='B')&&(spare_data[2]=='T')&&(spare_data[3]=='!')){
                if (!check_is_bad_block_by_read(block_no)) {
                    flag_bbt_exist = true;
                    break;
                }
            }
        }
    }

    sys_memset(spare_data, 0, sizeof(spare_data));

    if(flag_bbt_exist){
        for(page_no = NAND_BLOCK_SIZE - 1; page_no < NAND_BLOCK_SIZE; page_no--) {    //0~63
            result = qspi_nandflash_read(spare_data, block_no * NAND_BLOCK_SIZE + page_no, NAND_PAGE_SIZE + NAND_SPARE_DATA_OFFSET, 6);
            // It shall not read fail
            if (result != FLASH_ERR_NO_ERROR) {
                PLF_ASSERT_ERR(false);
                // return false;
            }

            if((spare_data[0]=='D')&&(spare_data[1]=='B')&&(spare_data[2]=='T')&&(spare_data[3]=='!')){
                break;
            }
        }

        bbt_crt_block_idx = 0;
        pair_block_no = spare_data[4] + (spare_data[5]<<8);

        dbg_print(NOTICE, "rebuild bbt find block_no %d, pair_block_no %d, page_no %d \r\n", block_no, pair_block_no, page_no);

        sys_memset(spare_data, 0, sizeof(spare_data));

        result = qspi_nandflash_read(spare_data, pair_block_no * NAND_BLOCK_SIZE + 0, NAND_PAGE_SIZE + NAND_SPARE_DATA_OFFSET, 6);

        if (result == FLASH_ERR_NO_ERROR) {
            //If last page of the block has flag, the ping pang block may have the newest BBT info in page 0
            if(page_no == (NAND_BLOCK_SIZE - 1)){
                //If the page 0 of ping pang block has the flag, this means it was erased fail because of cutting off power suddently
                if((spare_data[0]=='D')&&(spare_data[1]=='B')&&(spare_data[2]=='T')&&(spare_data[3]=='!')){
                    page_no = 0;
                    bbt_crt_block_idx = 1;

                    //Earse the block which doesn't erase in time because of cutting off power
                    qspi_nandflash_block_erase(block_no);
                }
            }
        }
        else {
            dbg_print(ERR, "another bbt block %d, read fail \r\n", pair_block_no);
            qspi_nandflash_block_erase(pair_block_no);
        }

        bbt_block[0] = block_no;
        bbt_block[1] = pair_block_no;
        bbt_crt_page = page_no;
        load_bbtrbt_from_nand(TYPE_BBT);
        bad_block_num = BBT[BBT_BAD_NUM_IDX];        //bad block number
        last_valid_rep_block = BBT[BBT_LAST_REP_VALID_IDX];     //record last valid block
        last_valid_tab_block = BBT[BBT_LAST_TAB_VALID_IDX];
    }
    // If not find bbt, scan all the flash to build bbt and rbt
    else{
        scan_all_2_build_bbtrbt();
        alloc_bbtrbt_block_addr();
        update_bbtrbt_2_nand(TYPE_BBT);
        update_bbtrbt_2_nand(TYPE_RBT);
#if PRINT_DEBUG_INFO
        uint16_t idx = 0;
        dbg_print(NOTICE, "BBT table :\r\n");

        for(uint16_t i = 0; i < BBT_SIZE; i++) {
            if (((i + 1) % 16) == 0) {
                dbg_print(NOTICE, "0x%x\r\n", BBT[i]);
            }
            else {
                dbg_print(NOTICE, "0x%x ", BBT[i]);
            }
        }
        dbg_print(NOTICE, "\r\n");

        dbg_print(NOTICE, "RBT table :\r\n");
        for(uint16_t i = 0; i < RBT_SIZE; i++) {
            if (i != RBT[i]) {
                if (((idx + 1) % 16) == 0) {
                    dbg_print(NOTICE, "%d:0x%x \r\n", i, RBT[i]);
                }
                else {
                    dbg_print(NOTICE, "%d:0x%x ", i, RBT[i]);
                }

                idx++;
            }
        }
        dbg_print(NOTICE, "\r\n");
#endif
        return true;
    }

    sys_memset(spare_data, 0, sizeof(spare_data));

    //check RBT whether exist
    for(block_no = Table_AREA_END; block_no >= Table_AREA_START; block_no--){                  //1023,1022
        page_no = 0;
        result = qspi_nandflash_read(spare_data, block_no * NAND_BLOCK_SIZE + page_no, NAND_PAGE_SIZE + NAND_SPARE_DATA_OFFSET, 6);
        if (result == FLASH_ERR_NO_ERROR) {
            if((spare_data[0]=='R')&&(spare_data[1]=='B')&&(spare_data[2]=='T')&&(spare_data[3]=='!')){
                if (!check_is_bad_block_by_read(block_no)) {
                    flag_rbt_exist = true;
                    break;
                }
            }
        }
    }


    sys_memset(spare_data, 0, sizeof(spare_data));
    if(flag_rbt_exist){
        uint16_t page_num = sizeof(RBT) / NAND_PAGE_SIZE;
        uint16_t last_page_no = (NAND_BLOCK_SIZE / page_num) * page_num - 1;

        if ((sizeof(RBT) % NAND_PAGE_SIZE) != 0) {
            page_num++;
        }

        for(page_no = NAND_BLOCK_SIZE - 1; page_no < NAND_BLOCK_SIZE; page_no--){   //0~63
            result = qspi_nandflash_read(spare_data,block_no * NAND_BLOCK_SIZE+page_no, NAND_PAGE_SIZE + NAND_SPARE_DATA_OFFSET,6);
            if (result != FLASH_ERR_NO_ERROR) {
                PLF_ASSERT_ERR(false);
            }

            if((spare_data[0]=='R')&&(spare_data[1]=='B')&&(spare_data[2]=='T')&&(spare_data[3]=='!')){
                break;
            }
        }


        // partial page is saved, try to rebuild from last save pages
        if (((page_no + 1) % page_num) != 0) {
            PLF_ASSERT_ERR(false);
            dbg_print(ERR, "partial page saved %d, page num %d \r\n", page_no, page_num);

            // try to use last save pages
            page_no = ((page_no + 1) / page_num) * page_num - 1;
        }

        rbt_crt_block_idx = 0;
        pair_block_no = spare_data[4] + (spare_data[5]<<8);

        dbg_print(NOTICE, "rebuild rbt find block_no %d, pair_block_no %d, page_no %d \r\n", block_no, pair_block_no, page_no);

        result = qspi_nandflash_read(spare_data, pair_block_no * NAND_BLOCK_SIZE+0, NAND_PAGE_SIZE + NAND_SPARE_DATA_OFFSET, 6);
        if (result == FLASH_ERR_NO_ERROR) {
            //If last page of the block has flag, the ping pang block may have the newest BBT info in page 0
            if(page_no == last_page_no) {
                if((spare_data[0]=='R')&&(spare_data[1]=='B')&&(spare_data[2]=='T')&&(spare_data[3]=='!')){
                    page_no = page_num - 1;
                    rbt_crt_block_idx = 1;
                }
            }
        }
        else {
            dbg_print(ERR, "ping pang rbt block %d, read fail \r\n", pair_block_no);
            qspi_nandflash_block_erase(pair_block_no);
        }

        rbt_block[0] = block_no;
        rbt_block[1] = pair_block_no;
        rbt_crt_page = page_no;
        load_bbtrbt_from_nand(TYPE_RBT);

#if PRINT_DEBUG_INFO
        uint16_t idx = 0;
        dbg_print(NOTICE, "BBT table :\r\n");

        for(uint16_t i = 0; i < BBT_SIZE; i++) {
            if (((i + 1) % 16) == 0) {
                dbg_print(NOTICE, "0x%x\r\n", BBT[i]);
            }
            else {
                dbg_print(NOTICE, "0x%x ", BBT[i]);
            }
        }
        dbg_print(NOTICE, "\r\n");

        dbg_print(NOTICE, "RBT table :\r\n");
        for(uint16_t i = 0; i < RBT_SIZE; i++) {
            if (i != RBT[i]) {
                if (((idx + 1) % 16) == 0) {
                    dbg_print(NOTICE, "%d:0x%x \r\n", i, RBT[i]);
                }
                else {
                    dbg_print(NOTICE, "%d:0x%x ", i, RBT[i]);
                }

                idx++;
            }
        }
        dbg_print(NOTICE, "\r\n");
#endif

        return true;
    }
    else{
        //BBT exist but RBT not exist
        dbg_print(ERR, "BBT exist but RBT not exist \r\n");
        return false;
    }
}


static flash_err_t nandflash_move_page_data(uint16_t dest_block_no,uint16_t src_block_no,uint8_t page_no, uint8_t *p_buf)
{
    flash_err_t result;
    result = qspi_nandflash_read(p_buf, src_block_no * NAND_BLOCK_SIZE + page_no,0,(NAND_PAGE_SIZE + NAND_SPARE_AREA_SIZE));
    if(result){
        PLF_ASSERT_ERR(false);
        return FLASH_ERR_MV_READ_FAIL;
    }

    result = qspi_nandflash_write(p_buf,dest_block_no*NAND_BLOCK_SIZE+page_no,0,(NAND_PAGE_SIZE + NAND_SPARE_AREA_SIZE));

    if (result == FLASH_ERR_NO_ERROR) {
        if (qspi_nandflash_check_is_badpage_by_read(dest_block_no*NAND_BLOCK_SIZE+page_no)) {
            result = FLASH_ERR_ECC_ERR;
        }
    }

    return result;
}


int nandflash_page_read(uint8_t *buffer, uint16_t block_no, uint8_t page_no,
                        uint16_t offset_in_page, uint16_t buf_len)
{
    uint16_t replace_block;

    if ((block_no > USER_AREA_END) || (page_no >= NAND_BLOCK_SIZE) || (buf_len > NAND_PAGE_SIZE)) {
        return -1;
    }

    if (check_is_in_bbt(block_no)) {
        replace_block = get_replace_block_from_rbt(block_no);
    } else {
        replace_block = block_no;
    }

    if (qspi_nandflash_read(buffer, replace_block * NAND_BLOCK_SIZE + page_no, offset_in_page,
                            buf_len) == 0) {
        return 0;
    } else {
        return -2;
    }
}

int nandflash_block_erase(uint16_t block_no)
{
    flash_err_t result = 0;
    int res = 0;
    uint16_t replace_block;

    if(block_no > USER_AREA_END){
        return -1;
    }

    do {
        if(check_is_in_bbt(block_no)){
            replace_block = get_replace_block_from_rbt(block_no);
        }else{
            replace_block = block_no;
        }

        result = qspi_nandflash_block_erase(replace_block);
        if(result != FLASH_ERR_NO_ERROR){
            if (!update_bb_2_bbtrbt(block_no, replace_block)) {
                res = -2;
                break;
            }

            if (!update_bbtrbt_2_nand(TYPE_BBT)) {
                res = -3;
                break;
            }

            if (!update_bbtrbt_2_nand(TYPE_RBT)) {
                res = -4;
                break;
            }
        }
    } while(result != FLASH_ERR_NO_ERROR);

    return res;
}

int nandflash_page_program(uint8_t *buffer, uint16_t block_no, uint8_t page_no, uint16_t offset_in_page, uint16_t buf_len)
{
    flash_err_t result;
    int res = 0;
    uint8_t page_idx;
    uint16_t replace_block,new_replace_block;
    uint8_t *p_buf = NULL;

    if((block_no > USER_AREA_END)||(page_no>=NAND_BLOCK_SIZE)||((offset_in_page + buf_len)>NAND_PAGE_SIZE)){
        return -1;
    }

    do {
        if(check_is_in_bbt(block_no)){
            replace_block = get_replace_block_from_rbt(block_no);
        }else{
            replace_block = block_no;
        }

        result = qspi_nandflash_write(buffer, replace_block * NAND_BLOCK_SIZE + page_no, offset_in_page, buf_len);

        if (result == FLASH_ERR_OUT_OF_BOUND) {
            return -1;
        }

        if (result == FLASH_ERR_NO_ERROR && qspi_nandflash_check_is_badpage_by_read(replace_block * NAND_BLOCK_SIZE + page_no)) {
            result = FLASH_ERR_ECC_ERR;
        }

        new_replace_block = replace_block;

        if(result != FLASH_ERR_NO_ERROR){
            // find a new block to move data
            do {
                res = 0;

                if (!update_bb_2_bbtrbt(block_no, new_replace_block)) {
                    return -2;
                }

                new_replace_block = get_replace_block_from_rbt(block_no);
                //copy the old block content to new alloced block
                for(page_idx = 0; page_idx < page_no; page_idx++){
                    if (nandflash_move_page_data(new_replace_block, replace_block, page_idx, p_temp_buf) != FLASH_ERR_NO_ERROR) {
                        res = -2;
                        qspi_nandflash_block_erase(new_replace_block);
                        break;
                    }
                }

            } while(res);

            if (!update_bbtrbt_2_nand(TYPE_BBT)) {
                res = -3;
                break;
            }

            if (!update_bbtrbt_2_nand(TYPE_RBT)) {
                res = -4;
                break;
            }
        }
    } while (result != FLASH_ERR_NO_ERROR);

    return res;
}


void nandflash_erase_all(void)
{
    uint16_t block_no = 0;

    for(block_no = 0; block_no < NAND_BLOCK_COUNT; block_no++) {
        nandflash_block_erase(block_no);
    }
}


bool nandflash_api_init(void)
{

    p_temp_buf = sys_malloc(NAND_PAGE_SIZE + NAND_SPARE_AREA_SIZE);

    if (p_temp_buf == NULL) {
        dbg_print(ERR, "malloc temp buffer fail\r\n");
        return false;
    }

    if (!qspi_nandflash_api_init())
    {
        return false;
    }

    rebuild_bbtrbt();

    return true;
}


void nandflash_set_bad_block(uint16_t user_block_no, uint16_t replace_block_no)
{
    if (!update_bb_2_bbtrbt(user_block_no, replace_block_no)) {
        dbg_print(ERR, "update fail!\r\n");
        return;
    }


    if (!update_bbtrbt_2_nand(TYPE_BBT)) {
        dbg_print(ERR, "save bbt fail!\r\n");
        return;
    }

    if (!update_bbtrbt_2_nand(TYPE_RBT)) {
        dbg_print(ERR, "save rbt fail!\r\n");
        return;
    }

}



uint8_t tx_nandbuf[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };
uint8_t rx_nandbuf[NAND_PAGE_SIZE];

void nandflash_api_test(void)
{
    uint16_t i = 0;
    int res;
    uint16_t block_no = 0, page_no = 0;

    if (!nandflash_api_init()) {
        dbg_print(ERR, "api init fail!\r\n");
        return;
    }

    for(block_no = 0; block_no < 5; block_no++) {
        res = nandflash_block_erase(block_no);
        if (res) {
            dbg_print(ERR, "erase block_no %d fail(%d)\r\n", block_no, res);
            return;
        }

        for(page_no = 0; page_no < 64;  page_no++) {
            res = nandflash_page_program(tx_nandbuf, block_no, page_no, 0, sizeof(tx_nandbuf));
            if (res) {
                dbg_print(ERR, "write block_no %d page_no %d fail(%d)\r\n", block_no, page_no, res);
                return;
            }

            res = nandflash_page_read(rx_nandbuf, block_no, page_no, 0, sizeof(tx_nandbuf));

            if (res || sys_memcmp(rx_nandbuf, tx_nandbuf, sizeof(tx_nandbuf))) {
                dbg_print(ERR, "read block_no %d page_no %d fail(%d)\r\n", block_no, page_no, res);
                return;
            }
        }

        dbg_print(NOTICE, "write and read block_no %d page_no %d success!\r\n", block_no, page_no);
    }
}
