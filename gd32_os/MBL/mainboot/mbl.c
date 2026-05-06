/*!
    \file    mbl.c
    \brief   Main boot loader for GD32VW55x SDK

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

#include "platform_def.h"
#include "mbl_includes.h"
#include "mbedtls/entropy_poll.h"
#include "rom_export_mbedtls.h"
#include "rom_flash.h"
#include "init_rom.h"
#include <stdio.h>

// #define CONFIG_BYPASS_MBL
#define CONFIG_XMODEM
#if CONFIG_BOARD == PLATFORM_BOARD_32VW55X_EVAL
#define LOG_UART        UART1
#else
#define LOG_UART        UART2
#endif

#ifdef CONFIG_XMODEM
extern void xmodem_start(uint32_t timeout);
#endif

static uint8_t alloc_buf[MBL_BUF_SIZE];
struct rom_api_t *p_rom_api = (struct rom_api_t *)ROM_API_ARRAY_BASE;

#if (RE_MBL_OFFSET == 0)
uint32_t compat_prefix[0x400] = {
    0x0000106F, 0xFFFFFFFF, 0x47443332, 0x00000000, 0x00001000, RE_SYS_STATUS_OFFSET, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
 };
#endif

#if defined(__GNUC__)
int _write(int fd, char *str, int len)
{
    (void)fd;
    int32_t i = 0;

    /* Send string and return the number of characters written */
    while (i != len) {
        while(RESET == usart_flag_get(LOG_UART, USART_FLAG_TBE));
        usart_data_transmit(LOG_UART, *str);
        str++;
        i++;
    }

    while(RESET == usart_flag_get(LOG_UART, USART_FLAG_TC));

    return i;
}
#endif

/*!
    \brief      initialize usart
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void log_uart_init(void)
{
    uint32_t usart_periph = LOG_UART;

    if (usart_periph == UART2) {
        rcu_periph_clock_enable(RCU_UART2);
        rcu_periph_clock_enable(RCU_GPIOA);
        gpio_af_set(GPIOA, GPIO_AF_10, GPIO_PIN_6);  // UART2 TX
        gpio_af_set(GPIOA, GPIO_AF_8, GPIO_PIN_7);   // UART2 RX
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_6);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_6);
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_7);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_7);
    } else if (usart_periph == UART1) {
        rcu_periph_clock_enable(RCU_GPIOA);
        rcu_periph_clock_enable(RCU_GPIOB);
        rcu_periph_clock_enable(RCU_UART1);
        gpio_af_set(GPIOB, GPIO_AF_7, GPIO_PIN_15);  // UART1 TX
        gpio_af_set(GPIOA, GPIO_AF_3, GPIO_PIN_8);   // UART1 RX
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_8);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_8);
        gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_15);
        gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_15);
    } else if (usart_periph == USART0) {
        rcu_periph_clock_enable(RCU_USART0);
        rcu_periph_clock_enable(RCU_GPIOA);
        rcu_periph_clock_enable(RCU_GPIOB);
        gpio_af_set(GPIOA, GPIO_AF_2, GPIO_PIN_8);   // UART0 TX
        gpio_af_set(GPIOB, GPIO_AF_8, GPIO_PIN_15);  // UART0 RX
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_8);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_8);
        gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_15);
        gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO_PIN_15);
    } else {
        return;
    }

    usart_deinit(usart_periph);
    usart_baudrate_set(usart_periph, 921600U);
    usart_receive_config(usart_periph, USART_RECEIVE_ENABLE);
    usart_transmit_config(usart_periph, USART_TRANSMIT_ENABLE);
    usart_interrupt_enable(usart_periph, USART_INT_RBNE);

    usart_enable(usart_periph);
}

/*!
    \brief      wait usart transmit complete
    \param[in]  none
    \param[out] none
    \retval     none
*/
void log_uart_idle_wait(void)
{
    while (RESET == usart_flag_get(LOG_UART, USART_FLAG_TC));
}

/*!
    \brief      mapping flash offset
    \param[in]  none
    \param[out] none
    \retval     none
*/
void flash_offset_mapping(void)
{
    fmc_unlock();
    ob_unlock();
    fmc_offset_region_config(RE_IMG_0_OFFSET >> 12, (RE_IMG_1_OFFSET >> 12) - 1);
    fmc_offset_value_config((RE_IMG_1_OFFSET - RE_IMG_0_OFFSET) >> 12);
    ob_lock();
    fmc_lock();
}

/*!
    \brief      deinitialize mbedtls
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void mbedtls_deinit(void)
{
    mbedtls_platform_set_calloc_free(NULL, NULL);
    mbedtls_platform_set_snprintf(NULL);
    mbedtls_platform_set_printf(NULL);
}

static void jump_to_main_image(uint32_t start_addr)
{
    __asm volatile("jr %0":: "r"((uint8_t *)(start_addr)));
}

/*!
    \brief      main function
    \param[in]  none
    \param[out] none
    \retval     none
*/
int main(void)
{
    uint32_t start_addr;
#ifndef CONFIG_BYPASS_MBL
    struct ibl_state_t ibl_state;
    uint32_t boot_idx = 0, image_offset = 0;
    int secure_boot = 1;
    uint8_t val;
    int ret;
    uint8_t xmodem_flag = 0;

    /* Read Initial boot state from shared SRAM */
    memcpy(&ibl_state, (void *)IBL_SHARED_DATA_START, sizeof(struct ibl_state_t));
    if (ibl_state.magic != IBL_STATE_MAGIC_CODE) { /* Not boot from ROM */
        secure_boot = 0;
        memset((void *)IBL_DATA_START, 0, IBL_DATA_SIZE);
        memset(&ibl_state, 0, sizeof(struct ibl_state_t));
    }

    /* Initialize ROM symbols */
    rom_symbol_init();

    /* Used for image x validation */
    mbedtls_memory_buffer_alloc_init(alloc_buf, sizeof(alloc_buf));

    /* Not boot from ROM */
    if (secure_boot == 0) {
        rcu_periph_clock_enable(RCU_PKCAU);
        rcu_periph_clock_enable(RCU_CAU);
        rcu_periph_clock_enable(RCU_HAU);

        rom_sys_status_check();
        rom_log_uart_set(LOG_UART);
        ret = rom_sys_status_get(SYS_TRACE_LEVEL, LEN_SYS_TRACE_LEVEL, &val);
        if (ret == SYS_STATUS_OK) {
            rom_sys_set_trace_level(val);
        } else {
            rom_sys_set_trace_level(ROM_INFO);
        }

        mbedtls_platform_set_hardware_poll(rom_hardware_poll);

        /* Others */
        mbedtls_ecp_curve_val_init();
    }

#ifdef CONFIG_XMODEM
    /* XMODEM: auto-detect UART for 100ms(looks for SYNCREQ 0x75 at 115200).
     * On hit: reply SYNCACK 0x79, then handshake (baud/frame/erase).*/
    ret = rom_sys_status_get(SYS_XMODEM_FLAG, 1, &xmodem_flag);
    if (ret == SYS_STATUS_FOUND_OK) {
        if (xmodem_flag) {
            xmodem_flag = 0;
            rom_sys_status_set(SYS_XMODEM_FLAG, 1, &xmodem_flag);
            xmodem_start(0xffffffff);
        } else {
            xmodem_start(100000);
        }
    } else {
        xmodem_start(100000);
    }
#endif

    /* Reinitialize uart since system clock source changed from IRC16M to HXTAL40M */
    log_uart_init();

    rom_trace_ex(ROM_ALWAYS, "MBL: First print.\r\n");

    /* Find the correct image to boot, Image 0 or Image 1 */
    ret = boot_image_find(&boot_idx, &image_offset);
    if (ret < 0) {
        rom_trace_ex(ROM_ERR, "No image to boot (ret = %d).\r\n", ret);
        goto NoImageBoot;
    } else {
        rom_trace_ex(ROM_ALWAYS, "MBL: Boot from Image %d.\r\n", boot_idx);
    }

    /* If boot from Image 1, config offset mapping */
    if (boot_idx == IMAGE_1) {
        flash_offset_mapping();
    }

    /* Use HAU DMA to accelerate digest calculation */
    rcu_periph_clock_enable(RCU_DMA);
    rom_digest_haudma_en(1);

    /* Verify Image */
    ret = image_x_validate((image_offset - sizeof(struct image_header)), ibl_state.rotpk_hash, ibl_state.ibl_opt);
    if (ret < 0) {
        rom_trace_ex(ROM_ERR, "Validate Image %d failed (ret = %d).\r\n", boot_idx, ret);
        rom_sys_set_img_flag(boot_idx, IMG_FLAG_VERIFY_MASK, IMG_FLAG_VERIFY_FAIL);
        goto BootFailed;
    } else {
        rom_trace_ex(ROM_ALWAYS, "MBL: Validate Image %d OK.\r\n", boot_idx);
    }

    /* Update Image status and Running Image flag */
    rom_sys_set_img_flag(boot_idx, IMG_FLAG_VERIFY_MASK, IMG_FLAG_VERIFY_OK);
    rom_sys_set_running_img(boot_idx);

    mbedtls_deinit();

    start_addr = FLASH_BASE + image_offset;
    if (boot_idx == IMAGE_1) {
        rom_trace_ex(ROM_ALWAYS, "MBL: Jump to Main Image (0x%08x).\r\n", FLASH_BASE + RE_IMG_1_OFFSET);
    } else {
        rom_trace_ex(ROM_ALWAYS, "MBL: Jump to Main Image (0x%08x).\r\n", start_addr);
    }
    log_uart_idle_wait();

    /* Jump to main image */
    jump_to_main_image(start_addr);

BootFailed:
    SysTimer_SoftwareReset();

NoImageBoot:
    while(1);

#else /* defined CONFIG_BYPASS_MBL */
    log_uart_init();
    printf("=== GD32VW55x ===\r\n");

    start_addr = FLASH_BASE + RE_IMG_0_OFFSET;

    /* Jump to main image */
    jump_to_main_image(start_addr);

#endif  /* CONFIG_BYPASS_MBL */
}
