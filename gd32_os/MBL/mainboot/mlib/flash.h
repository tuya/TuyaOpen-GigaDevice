#ifndef _FLASH_H_
#define _FLASH_H_

#define FLASH_TOTAL_SIZE        (0x400000)
#define FLASH_PAGE_SIZE         (0x1000)

uint32_t flash_total_size(void);
int flash_write(uint32_t offset, const void *data, int len);
int flash_write_fast(uint32_t offset, const void *data, int len);
int flash_erase(uint32_t offset, int len);
int flash_erase_chip(void);
void flash_nodec_config(uint32_t nd_idx, uint32_t start_page, uint32_t end_page);

#endif /* _FLASH_H_ */
