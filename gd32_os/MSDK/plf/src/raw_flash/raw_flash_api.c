/*!
    \file    raw_flash_api.c
    \brief   Flash RAW API for GD32VW55x SDK

    \version 2024-04-15, V1.0.0, firmware for GD32VW55x
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

#include "gd32vw55x.h"
#include "string.h"
#include "slist.h"
#include "raw_flash_api.h"
#include "wrapper_os.h"
#include "gd32vw55x_fmc.h"
#include "rom_export.h"
#include "ll.h"
#include "app_cfg.h"
#include "config_gdm32.h"

// Flash erase callback list
static struct list raw_erase_cb_list;

typedef struct
{
    struct list_hdr hdr;
    raw_flash_erase_handler_t callback;
} raw_erase_cb_list_item_t;

/*!
    \brief      flash initilization
    \param[in]  none
    \param[out] none
    \retval     none
*/
void raw_flash_init(void)
{
    list_init(&raw_erase_cb_list);
}

/*!
    \brief      get flash total size
    \param[in]  none
    \param[out] none
    \retval     flash total size
*/
uint32_t raw_flash_total_size(void)
{
    return FLASH_TOTAL_SIZE;
}

/*!
    \brief      get flash offset valid state
    \param[in]  offset: flash offset
    \param[out] none
    \retval     result of state(1: offset is valid, or 0: offset is invalid)
*/
int raw_flash_is_valid_offset(uint32_t offset)
{
    if (offset < raw_flash_total_size()) {
        return 1;
    }
    return 0;
}

/*!
    \brief      get flash addr valid state
    \param[in]  addr: flash address
    \param[out] none
    \retval     result of state(1: addr is valid, or 0: addr is not valid)
*/
int raw_flash_is_valid_addr(uint32_t addr)
{
    if ((addr >= FLASH_BASE) && (addr < (FLASH_BASE + raw_flash_total_size()))) {
        return 1;
    }
    return 0;
}

/*!
    \brief      configure no real time decrypt areas for flash
    \param[in]  nd_idx: no decrypt register index
    \param[in]  start_page: start page of no real time decrypt area
    \param[in]  end_page: end page of no real time decrypt area
    \param[out] none
    \retval     none
*/
void raw_flash_nodec_config(uint32_t nd_idx, uint32_t start_page, uint32_t end_page)
{
    /* unlock the flash program erase controller */
    fmc_unlock();
    /* unlock the option byte operation (include SECWM/HDP/WRP/NODEC/OFRG/OFVR) */
    ob_unlock();
    /* clear pending flags */
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR);

    /* set no OTFDEC region for flash */
    fmc_no_rtdec_config(start_page, end_page, nd_idx);

    /* lock the option byte operation */
    ob_lock();
    /* lock the flash program erase controller */
    fmc_lock();

}

/*!
    \brief      config flash offset region and value
    \param[in]  of_spage: start page of offset region, 0~0x1FFF
    \param[in]  of_epage: end page of offset region, 0~0x1FFF
    \param[in]  of_value: offset value, 0~0x1FFF
    \param[out] none
    \retval     none
*/
void raw_flash_offset_mapping(uint32_t of_spage, uint32_t of_epage, uint32_t of_value)
{
    fmc_unlock();
    ob_unlock();
    fmc_offset_region_config(of_spage, of_epage);
    fmc_offset_value_config(of_value);
    ob_lock();
    fmc_lock();
}

/*!
    \brief      read flash
    \param[in]  offset: flash offset
    \param[out] data: pointer to the buffer store flash read data
    \param[in]  len: length of data read from flash
    \retval     result of read flash(0: read ok, or -1: read error)
*/
int raw_flash_read(uint32_t offset, void *data, int len)
{
    int ret;
    uint32_t fmc_ofvr_temp = FMC_OFVR;

    if (!raw_flash_is_valid_offset(offset) || data == NULL
        || len <= 0 || !raw_flash_is_valid_offset(offset + len - 1)) {
        return -1;
    }


    if (fmc_ofvr_temp > 0) {                                                //working on image 1
        if (offset >= RE_IMG_0_OFFSET && offset < RE_IMG_1_OFFSET) {       //read flash from image 0
            __disable_irq();

            // reset FMC_OFVR to 0
            fmc_unlock();
            ob_unlock();
            fmc_offset_value_config(0);
            ob_lock();
            fmc_lock();

            ret = rom_flash_read(offset, data, len);

            // recovery FMC_OFVR value
            fmc_unlock();
            ob_unlock();
            fmc_offset_value_config(fmc_ofvr_temp);
            ob_lock();
            fmc_lock();
            __enable_irq();
        } else if (offset >= RE_IMG_1_OFFSET && offset < RE_IMG_1_END){                                                          //read flash from image 1
            ret = rom_flash_read(offset - (RE_IMG_1_OFFSET - RE_IMG_0_OFFSET), data, len);
        } else {
            ret = rom_flash_read(offset, data, len);
        }
    } else {                                                         //working on image 0
        if (offset >= RE_IMG_1_OFFSET && offset < RE_IMG_1_END) {        //read flash from image 1
            __disable_irq();

            fmc_unlock();
            ob_unlock();
            fmc_offset_region_config(RE_IMG_0_OFFSET >> 12, (RE_IMG_1_OFFSET >> 12) - 1);
            fmc_offset_value_config((RE_IMG_1_OFFSET - RE_IMG_0_OFFSET) >> 12);
            ob_lock();
            fmc_lock();

            ret = rom_flash_read(offset - (RE_IMG_1_OFFSET - RE_IMG_0_OFFSET), data, len);

            fmc_unlock();
            ob_unlock();
            fmc_offset_region_config(0x1fff, 0);
            fmc_offset_value_config(0);
            ob_lock();
            fmc_lock();

            __enable_irq();
        } else {                        //read flash from image 0 and other partition
            ret = rom_flash_read(offset, data, len);
        }
    }

    return ret;
}

#ifdef CONFIG_FLASH_NOT_BLOCK_UART_RX
static uint32_t* redirect_vector_sram(uint8_t enable)
{
    extern uint32_t _vetor_base[];
    extern void redirect_vector_table(uint32_t vector_new);
    uint32_t *vector_new = NULL;
    uint32_t vector_new_1;

    if (enable) {
        vector_new = (uint32_t *)sys_malloc(0x1d0 + 0x200);
        if (vector_new == NULL) {
            return NULL;
        }
        vector_new_1 = ((uint32_t)vector_new + 0x200) & (~(0x200 - 1));
        sys_memcpy((void *)vector_new_1, (void *)_vetor_base, 0x1d0);

        __disable_irq();
        redirect_vector_table((uint32_t)vector_new_1);
        __enable_irq();
        return vector_new;
    } else {
        __disable_irq();
        redirect_vector_table((uint32_t)_vetor_base);
        __enable_irq();
        return NULL;
    }
}

static void redirect_vector_free(uint32_t *vector_addr)
{
    if (vector_addr)
        sys_mfree(vector_addr);
}

#define VECTOR_SRAM_ENTER()                         \
    uint32_t *vector_addr = NULL;                   \
    vector_addr = redirect_vector_sram(1);          \
    if (vector_addr == NULL) {                      \
        return -1;                                  \
    }

#define VECTOR_SRAM_RESTORE()                       \
    redirect_vector_sram(0);                        \
    redirect_vector_free(vector_addr);
#else
#define VECTOR_SRAM_ENTER()
#define VECTOR_SRAM_RESTORE()
#endif
/*!
    \brief      write flash
    \param[in]  offset: flash offset
    \param[in]  data: pointer to the data write to flash
    \param[in]  len: length of data write to flash
    \param[out] none
    \retval     result of write flash(0: write ok, or -1: write error)
*/
int raw_flash_write(uint32_t offset, const void *data, int len)
{
    if (!raw_flash_is_valid_offset(offset) || data == NULL
        || len <= 0 || !raw_flash_is_valid_offset(offset + len - 1)) {
        return -1;
    }

    if (rom_flash_write(offset, data, len)) {
        return -1;
    }

    return 0;
}

static void raw_flash_erase_handler(raw_erase_type_t type)
{
    struct list_hdr *next;
    struct list_hdr *elt = list_pick(&raw_erase_cb_list);
    raw_erase_cb_list_item_t *p_item;

    while (elt != NULL) {
        p_item = (raw_erase_cb_list_item_t *)elt;
        next = list_next(elt);

        if (p_item->callback)
            p_item->callback(type);

        elt = next;
    }
}

int raw_flash_erase_handler_register(raw_flash_erase_handler_t callback)
{
    raw_erase_cb_list_item_t *p_item;
    struct list_hdr *elt = list_pick(&raw_erase_cb_list);

    while (elt != NULL) {
        p_item = (raw_erase_cb_list_item_t *)elt;

        if (p_item->callback == callback)
            return 0;

        elt = list_next(elt);
    }

    p_item = (raw_erase_cb_list_item_t *)sys_malloc(sizeof(raw_erase_cb_list_item_t));
    if (p_item == NULL)
        return -1;

    p_item->callback = callback;

    list_push_back(&raw_erase_cb_list, &(p_item->hdr));

    return 0;
}

void raw_flash_erase_handler_unregister(raw_flash_erase_handler_t callback)
{
    raw_erase_cb_list_item_t *p_item;
    struct list_hdr *prev_elt = NULL;
    struct list_hdr *elt = list_pick(&raw_erase_cb_list);

    while (elt != NULL) {
        p_item = (raw_erase_cb_list_item_t *)elt;

        if (p_item->callback && p_item->callback == callback) {
            list_remove(&raw_erase_cb_list, prev_elt, elt);
            sys_mfree(p_item);
        } else {
            prev_elt = elt;
        }
        elt = (prev_elt == NULL ? list_pick(&raw_erase_cb_list) : list_next(prev_elt));
    }

    return;
}

/*!
    \brief      erase flash
    \param[in]  offset: flash offset
    \param[in]  len: flash erase length
    \param[out] none
    \retval     result of erase flash(0: erase ok, or -1: erase error)
*/
int raw_flash_erase(uint32_t offset, int len)
{
    int ret;

    if (!raw_flash_is_valid_offset(offset)
        || len <= 0 || !raw_flash_is_valid_offset(offset + len - 1)) {
        return -1;
    }

#ifdef CFG_BLE_SUPPORT
    raw_flash_erase_handler(RAW_FLASH_ERASE_BLE_PRE_HANDLE);
#endif

    /* redirect vector table to sram */
    VECTOR_SRAM_ENTER();

    GLOBAL_INT_DISABLE();
    ret = rom_flash_erase(offset, len);
    GLOBAL_INT_RESTORE();

    /* restore vector table */
    VECTOR_SRAM_RESTORE();

#ifdef CFG_BLE_SUPPORT
    raw_flash_erase_handler(RAW_FLASH_ERASE_BLE_AFTER_HANDLE);
#endif

    return ret;
}

/*!
    \brief      write flash
    \param[in]  offset: flash offset
    \param[in]  data: pointer to the data write to flash
    \param[in]  len: length of data write to flash
    \param[out] none
    \retval     result of write flash(0: write ok, or -1: write error)
*/
int raw_flash_write_fast(uint32_t offset, const void *data, int len)
{
    int ret = 0;

    if (!raw_flash_is_valid_offset(offset) || data == NULL
        || len <= 0 || !raw_flash_is_valid_offset(offset + len - 1)) {
        return -1;
    }

    if (len <= 4)
        return raw_flash_write(offset, data, len);

    uint8_t r = 4 - (offset & 0x3);
    uint8_t rr = (offset + len) & 0x3;

    r = r == 4 ? 0 : r;
    if (r) {
        ret = raw_flash_write(offset, data, r);
        if (ret)
            return ret;
    }

    /* redirect vector table to sram */
    VECTOR_SRAM_ENTER();

    /* unlock the flash program erase controller */
    fmc_unlock();
    /* clear pending flags */
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR);

    /* prevent interrupt handler from reading flash, it will disrupt the flash continuous programming pipeline */
    GLOBAL_INT_DISABLE();
    ret = fmc_continuous_program(FLASH_BASE + offset + r, (uint32_t *)(data + r), len - r - rr);
    GLOBAL_INT_RESTORE();

    /* lock the flash program erase controller */
    fmc_lock();

    /* restore vector table */
    VECTOR_SRAM_RESTORE();

    if (rr) {
        ret = raw_flash_write(offset + len - rr, data + len - rr, rr);
        if (ret)
            return ret;
    }
    return ret;
}
