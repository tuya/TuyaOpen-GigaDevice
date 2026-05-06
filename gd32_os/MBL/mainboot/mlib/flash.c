#include "gd32vw55x.h"
#include "flash.h"

int is_valid_flash_offset(uint32_t offset)
{
    if (offset < FLASH_TOTAL_SIZE) {
        return 1;
    }
    return 0;
}

uint32_t flash_total_size(void)
{
    return FLASH_TOTAL_SIZE;
}

int flash_write(uint32_t offset, const void *data, int len)
{
    uint8_t *data_u8 = (uint8_t *)data;
    uint32_t base_addr = FLASH_BASE;
    uint32_t offset_align, val32;
    int vb, act_len, i;
    uint8_t val[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    int ret;

    if (!is_valid_flash_offset(offset) || data == NULL \
        || len <= 0 || !is_valid_flash_offset(offset + len - 1)) {
        return -1;
    }

    /* unlock the flash program erase controller */
    fmc_unlock();
    /* clear pending flags */
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR);

    offset_align = (offset & ~0x3);

    /* if offset is not 4-byte alignment */
    vb = (offset & 0x3);
    if (vb > 0) {
        act_len = ((4 - vb) > len) ? len : (4 - vb);
        for (i = 0; i < act_len; i++) {
            val[vb + i] = *(data_u8 + i);
        }
        ret = fmc_word_program((base_addr + offset_align), *(uint32_t *)val);
        if (ret)
            printf("fmc write fail, ret=%d\r\n", ret);
        offset_align += 4;
        data_u8 += act_len;
        len -= act_len;
    }

    /* word program */
    while (len >= 4) {
        ret = fmc_word_program((base_addr + offset_align), *(uint32_t *)data_u8);
        if (ret)
            printf("fmc write fail, ret=%d\r\n", ret);
        offset_align += 4;
        data_u8 += 4;
        len -= 4;
    }

    /* if len is not 4-byte alignment */
    val32 = 0xFFFFFFFF;
    if (len > 0) {
        while (len-- > 0) {
            val32 = (val32 << 8);
            val32 |= *(data_u8 + len);
        }
        fmc_word_program((base_addr + offset_align), val32);
    }

    /* lock the flash program erase controller */
    fmc_lock();
    return 0;
}

int flash_write_fast(uint32_t offset, const void *data, int len)
{
    int ret = 0;
    uint8_t r = 4 - (offset & 0x3);
    uint8_t rr = (offset + len) & 0x3;

    if (!is_valid_flash_offset(offset) || data == NULL
        || len <= 0 || !is_valid_flash_offset(offset + len - 1)) {
        return -1;
    }

    if (len <= 4)
        return flash_write(offset, data, len);

    r = r == 4 ? 0 : r;
    if (r) {
        ret = flash_write(offset, data, r);
        if (ret)
            return ret;
    }

    /* unlock the flash program erase controller */
    fmc_unlock();
    /* clear pending flags */
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR);

    /* prevent interrupt handler from reading flash, it will disrupt the flash continuous programming pipeline */
    //__disable_irq();
    ret = fmc_continuous_program(FLASH_BASE + offset + r, (uint32_t *)data + r, len - r - rr);
    //__enable_irq();
    if (ret)
        printf("fmc ontinuous_program fail, ret=%d\r\n", ret);

    /* lock the flash program erase controller */
    fmc_lock();

    if (rr) {
        ret = flash_write(offset + len - rr, data + len - rr, rr);
        if (ret)
            return ret;
    }
    return ret;
}


int flash_erase(uint32_t offset, int len)
{
    int ret;
    uint32_t erase_sz = FLASH_PAGE_SIZE;
    uint32_t page_start;

    if (!is_valid_flash_offset(offset)
        || len <= 0 || !is_valid_flash_offset(offset + len - 1)) {
        return -1;
    }

    /* Get page start */
    page_start = FLASH_BASE + (offset & (~(erase_sz - 1)));

    /* unlock the flash program erase controller */
    fmc_unlock();

    /* clear pending flags */
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR);

    while (len > 0) {
        /* erase page */
        ret = fmc_page_erase(page_start);
        if (ret != FMC_READY)
            return -2;
        page_start += erase_sz;
        len -= erase_sz;
    }
    /* lock the flash program erase controller */
    fmc_lock();

    return 0;
}

int flash_erase_chip(void)
{
    int ret;

    /* unlock the flash program erase controller */
    fmc_unlock();

    /* clear pending flags */
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR);

    ret = fmc_mass_erase();

    /* lock the flash program erase controller */
    fmc_lock();

    return ret;
}

void flash_nodec_config(uint32_t nd_idx, uint32_t start_page, uint32_t end_page)
{
    /* unlock the flash program erase controller */
    fmc_unlock();
    /* unlock the option byte operation (include SECWM/HDP/WRP/NODEC/OFRG/OFVR) */
    ob_unlock();
    /* clear pending flags */
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR);

    /* set NO-RTDEC region for flash */
    fmc_no_rtdec_config(start_page, end_page, nd_idx);

    /* lock the option byte operation */
    ob_lock();
    /* lock the flash program erase controller */
    fmc_lock();
}