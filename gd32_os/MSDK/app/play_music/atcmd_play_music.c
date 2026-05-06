/*!
    \file    atcmd_play_music.c
    \brief   AT command play music part for GD32VW55x SDK

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


#include "mp3_play.h"
#ifdef ES8375_USED
#include "es8375_driver.h"
#endif
#ifdef MAX98357_USED
#include "max98357_driver.h"
#endif
#include "mbedtls/version.h"
#define MBEDTLS_VER_2_17_0       0x02110000
#if (!defined(MBEDTLS_VERSION_NUMBER) || MBEDTLS_VERSION_NUMBER != MBEDTLS_VER_2_17_0) //mbedtls v3.6.2
#include "mbedtls/md5.h"
#else
#include "rom_export_mbedtls.h"
#endif
#include "plf_assert.h"
#include "atcmd_play_music.h"

#ifdef PM_UART_DMA_RX
static uint16_t pm_cyclic_buf_count(void)
{
    uint32_t rx_dma_channel;

    if (at_uart_conf.usart_periph == USART0) {
        rx_dma_channel = DMA_CH2;
    } else if (at_uart_conf.usart_periph == UART1) {
        rx_dma_channel = DMA_CH0;
    } else if (at_uart_conf.usart_periph == UART2) {
        rx_dma_channel = DMA_CH5;
    } else {
        dbg_print(ERR, "pm_cyclic_buf_count usart_periph error!\r\n");
        return 0;
    }

    if (!pm_uart_cyc_buf.buf) {
        return 0;
    }

    pm_uart_cyc_buf.write_idx = (uint16_t)(pm_uart_cyc_buf.len - dma_transfer_number_get(
                                                rx_dma_channel));

    //dbg_print(ERR, "write_cycle %d, read_cycle %d, read_cycle %d, read_idx %d\r\n",
              // write_cycle, read_cycle, pm_uart_cyc_buf.write_idx, pm_uart_cyc_buf.read_idx);
    // FIX TODO there is a risk that write will recover index before read.
    if (pm_uart_cyc_buf.write_idx == pm_uart_cyc_buf.len) {
        pm_uart_cyc_buf.write_idx = 0;
    }

    if (read_cycle != write_cycle && pm_uart_cyc_buf.write_idx > pm_uart_cyc_buf.read_idx) {
        dbg_print(ERR, "write wrapper write_cycle %d, read_cycle %d, write_idx %d, read_idx %d\r\n",
                  write_cycle, read_cycle, pm_uart_cyc_buf.write_idx, pm_uart_cyc_buf.read_idx);
        PLF_ASSERT_ERR(true);
        read_cycle = write_cycle - 1;
        pm_uart_cyc_buf.read_idx = (pm_uart_cyc_buf.write_idx + 1) % pm_uart_cyc_buf.len;
    }

    return ((pm_uart_cyc_buf.write_idx + pm_uart_cyc_buf.len - pm_uart_cyc_buf.read_idx) %
            pm_uart_cyc_buf.len);
}

static bool pm_cyclic_buf_read(uint8_t *buf, uint16_t len)
{
    cyclic_buf_t *cyclic = &pm_uart_cyc_buf;
    uint16_t read_idx = pm_uart_cyc_buf.read_idx;

    if (!cyclic->buf) {
        return false;
    }

    if (len > cyclic_buf_count(cyclic)) {
        return false;
    }

    if (cyclic->read_idx + len <= cyclic->len) {
        sys_memcpy(buf, cyclic->buf + cyclic->read_idx, len);
        cyclic->read_idx += len;
    } else {
        uint16_t tlen = cyclic->len - cyclic->read_idx;

        sys_memcpy(buf, cyclic->buf + cyclic->read_idx, tlen);
        sys_memcpy(buf + tlen, cyclic->buf, len - tlen);
        cyclic->read_idx = len - tlen;
    }

    cyclic->read_idx %= cyclic->len;

    if (cyclic->read_idx < read_idx) {
        read_cycle++;
    }

    return true;
}

bool pm_cyclic_buf_drop(uint16_t len)
{
    cyclic_buf_t *cyclic = &pm_uart_cyc_buf;
    uint16_t read_idx = pm_uart_cyc_buf.read_idx;

    if (!cyclic->buf) {
        return false;
    }

    if (len > cyclic_buf_count(cyclic)) {
        return false;
    }

    if (cyclic->read_idx + len <= cyclic->len) {
        cyclic->read_idx += len;
    } else {
        uint16_t tlen = cyclic->len - cyclic->read_idx;

        cyclic->read_idx = len - tlen;
    }

    cyclic->read_idx %= cyclic->len;

    if (cyclic->read_idx < read_idx) {
        read_cycle++;
    }

    return true;
}

static uint16_t pm_cyclic_find_cmd_end(void)
{
    uint16_t read_idx;
    uint16_t total_len = 0;
    uint16_t read_len = 0;
    uint16_t found_r_idx = 0;

    if (!pm_uart_cyc_buf.buf) {
        return 0;
    }

    total_len = (pm_uart_cyc_buf.write_idx + pm_uart_cyc_buf.len - pm_uart_cyc_buf.read_idx) %
                pm_uart_cyc_buf.len;

    read_idx = pm_uart_cyc_buf.read_idx;

    while (total_len) {
        if (*(pm_uart_cyc_buf.buf + read_idx) == '\r') {
            if (read_len == 0) {
                pm_cyclic_buf_drop(1);
            } else {
                read_len++;
                found_r_idx = read_idx;
            }
        } else if (*(pm_uart_cyc_buf.buf + read_idx) == '\n') {
            if (read_len == 0) {
                pm_cyclic_buf_drop(1);
            } else {
                if (((read_idx + pm_uart_cyc_buf.len - found_r_idx) % pm_uart_cyc_buf.len) != 1) {
                    dbg_print(ERR, "Warnning CR and LF not match\r\n");
                }
                if (read_len > 0) {
                    *(pm_uart_cyc_buf.buf + found_r_idx) = '\0';
                    read_len++;
                    return read_len;
                }
            }
        } else {
            read_len++;
        }

        read_idx = (read_idx + 1) % pm_uart_cyc_buf.len;
        total_len--;
    }

    return 0;
}

static void pm_uart_rx_dma_irq_hdl(uint32_t dma_channel)
{
    if (RESET != dma_interrupt_flag_get(dma_channel, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(dma_channel, DMA_INT_FLAG_FTF);
        write_cycle++;
        sys_sema_up_from_isr(&rx_sema);
    } else if (RESET != dma_interrupt_flag_get(dma_channel, DMA_INT_FLAG_SDE)) {
        dma_interrupt_flag_clear(dma_channel, DMA_INT_FLAG_SDE);
        sys_sema_up_from_isr(&rx_sema);
    } else if (RESET != dma_interrupt_flag_get(dma_channel, DMA_INT_FLAG_TAE)) {
        dma_interrupt_flag_clear(dma_channel, DMA_INT_FLAG_TAE);
        sys_sema_up_from_isr(&rx_sema);
    }
}

#if (AT_CMD_UART == 0)
void DMA_Channel2_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */
    pm_uart_rx_dma_irq_hdl(DMA_CH2);
    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}
#elif (AT_CMD_UART == 1)
void DMA_Channel0_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */
    pm_uart_rx_dma_irq_hdl(DMA_CH0);
    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}
#else
void DMA_Channel5_IRQHandler(void)
{
    sys_int_enter();                            /* Tell the OS that we are starting an ISR            */
    pm_uart_rx_dma_irq_hdl(DMA_CH5);
    sys_int_exit();                             /* Tell the OS that we are leaving the ISR            */
}
#endif

static void at_install_music_bin(int argc, char **argv)
{
    uint8_t op, install_op;
    uint32_t data_len, buff_len;
    char *endptr = NULL;
    uint8_t *p_val = NULL;
    FIL *p_file;
    int res;
    uint32_t wr_len = 0;

    op = (uint8_t)strtoul((const char *)argv[1], &endptr, 0);

    dbg_print(NOTICE, "at_install_music_bin %d\r\n", op);

    p_file = (FIL *)sys_malloc(sizeof(FIL));

    if (NULL == p_file) {
        dbg_print(ERR, "malloc file fail\r\n");
        at_hw_send("+ERR\r\n", strlen("+ERR\r\n"));
        return;
    }

    switch (op) {
    case 0:
        qspi_flash_chip_erase();
        at_hw_send("+OK", strlen("+OK"));

        sys_ms_sleep(500);
        SysTimer_SoftwareReset();
        return;
    break;

    case 1:
    {
        mp3_file_info mp3_info;

        mp3_info.mp3_version[0] = (uint16_t)strtoul((const char *)argv[2], &endptr, 0);
        mp3_info.mp3_version[1] = (uint16_t)strtoul((const char *)argv[3], &endptr, 0);
        mp3_info.mp3_version[2] = (uint16_t)strtoul((const char *)argv[4], &endptr, 0);
        mp3_info.mp3_doc_num = (uint8_t)strtoul((const char *)argv[5], &endptr, 0);


        f_unlink(MP3_INFOS_FILE);

        res = f_open(p_file, MP3_INFOS_FILE, FA_CREATE_ALWAYS | FA_WRITE);
        if (res != FR_OK) {
            dbg_print(ERR, "open %s file fail %d\r\n", MP3_INFOS_FILE, res);
            at_hw_send("+ERR\r\n", strlen("+ERR\r\n"));
            goto op_done;
        }

        res = f_write(p_file, &mp3_info, sizeof(mp3_file_info), (UINT *)&wr_len);
        if (res != FR_OK) {
            dbg_print(ERR, "write mp3_info fail %d\r\n", res);
            at_hw_send("+ERR\r\n", strlen("+ERR\r\n"));
            f_close(p_file);
            goto op_done;
        }

        f_sync(p_file);
        f_close(p_file);
    }
    break;

    case 2:
    {
        bool need_mkdir = false;

        install_op = (uint8_t)strtoul((const char *)argv[2], &endptr, 0);
        uint8_t category = (uint8_t)strtoul((const char *)argv[3], &endptr, 0);

        // printf("install_op %d, category %d\r\n", install_op, category);

        p_file = (FIL *)sys_malloc(sizeof(FIL));

        if (NULL == p_file) {
            dbg_print(ERR, "malloc file fail\r\n");
            at_hw_send("+ERR\r\n", strlen("+ERR\r\n"));
            return;
        }

        if (category >= (sizeof(doc_path) / sizeof(doc_path[0]))) {
            dbg_print(ERR, "category %d wrong\r\n", category);
            at_hw_send("+ERR\r\n", strlen("+ERR\r\n"));
            goto op_done;
        }

        res = f_stat(doc_path[category], NULL);

        if (install_op != UPDATE_PARTIAL_OP) {
            if (res == FR_OK) {
                res = fatfs_delete(doc_path[category]);
                if (res != FR_OK) {
                    dbg_print(ERR, "Doc delete dir %s fail %d\r\n", doc_path[category], res);
                    at_hw_send("+ERR\r\n", strlen("+ERR\r\n"));
                    goto op_done;
                }
            }
            need_mkdir = true;
        }
        else {
            if (res != FR_OK) {
                need_mkdir = true;
            }
        }

        res = f_stat(MP3_PATH, NULL);
        if (res != FR_OK) {
            res = f_mkdir((const char *)MP3_PATH);
            if (res != FR_OK && res != FR_EXIST) {
                dbg_print(ERR, "Doc make dir %s fail %d\r\n", MP3_PATH, res);
                at_hw_send("+ERR\r\n", strlen("+ERR\r\n"));
                goto op_done;
            }
        }


        if (need_mkdir) {
            res = f_mkdir(doc_path[category]);
            if (res != FR_OK && res != FR_EXIST) {
                dbg_print(ERR, "Doc make dir %s fail %d\r\n", doc_path[category], res);
                at_hw_send("+ERR\r\n", strlen("+ERR\r\n"));
                goto op_done;
            }
        }
    }
    break;

    case 3:
    {
        data_len = (uint8_t)strtoul((const char *)argv[2], &endptr, 0);
        uint8_t *p_val = (uint8_t *)sys_malloc(data_len);

        if (NULL == p_val) {
            dbg_print(ERR, "malloc p_val fail\r\n");
            at_hw_send("+ERR\r\n", strlen("+ERR\r\n"));
            goto op_done;
        }

        uint16_t read_len = data_len;

        while ((buff_len = pm_cyclic_buf_count()) < read_len) {
            pm_cyclic_buf_read(&p_val[data_len - read_len], buff_len);
            read_len -= buff_len;
            if (sys_sema_down(&rx_sema, PM_DMA_RX_WAIT_MS) == OS_TIMEOUT) {
                break;
            }
        }

        buff_len = pm_cyclic_buf_count();

        if (buff_len < read_len) {
            pm_cyclic_buf_drop(buff_len);
            sys_mfree(p_val);
            at_hw_send("+ERR\r\n", strlen("+ERR\r\n"));
            goto op_done;
        } else {
            pm_cyclic_buf_read(&p_val[data_len - read_len], buff_len);
        }

        res = f_open(p_file, (char *)p_val, FA_CREATE_ALWAYS);
        if (res != FR_OK) {
            dbg_print(ERR,"open %s file fail %d\r\n", p_val, res);
            at_hw_send("+ERR\r\n", strlen("+ERR\r\n"));
            sys_mfree(p_val);
            goto op_done;;
        }

        sys_mfree(p_val);
        f_sync(p_file);
        f_close(p_file);
    }
    break;

    case 4:
    {
        uint8_t mp3_name_len = (uint8_t)strtoul((const char *)argv[2], &endptr, 0);
        data_len = (uint16_t)strtoul((const char *)argv[3], &endptr, 0);

        printf("mp3_name_len=%u  data_len=%u\r\n", mp3_name_len, data_len);

        mbedtls_md5_context  md5_context;
        uint8_t result[16] = {0};

        uint8_t *p_val = (uint8_t *)sys_malloc(data_len);

        if (NULL == p_val) {
            dbg_print(ERR, "malloc p_val fail\r\n");
            at_hw_send("+ERR\r\n", strlen("+ERR\r\n"));
            goto op_done;
        }

        uint16_t read_len = data_len;

        while ((buff_len = pm_cyclic_buf_count()) < read_len) {
            pm_cyclic_buf_read(&p_val[data_len - read_len], buff_len);
            read_len -= buff_len;
            if (sys_sema_down(&rx_sema, PM_DMA_RX_WAIT_MS) == OS_TIMEOUT) {
                break;
            }
        }

        buff_len = pm_cyclic_buf_count();

        if (buff_len < read_len) {
            pm_cyclic_buf_drop(buff_len);
            sys_mfree(p_val);
            at_hw_send("+ERR\r\n", strlen("+ERR\r\n"));
            goto op_done;
        } else {
            pm_cyclic_buf_read(&p_val[data_len - read_len], buff_len);
        }

        mbedtls_md5_init(&md5_context);
        mbedtls_md5_starts(&md5_context);

        mbedtls_md5_update(&md5_context, p_val + 16, data_len - 16);

        mbedtls_md5_finish(&md5_context, result);

        dbg_print(NOTICE, "read cyclic buf len %d\r\n", data_len);

        if (sys_memcmp(p_val, result, 16)) {
            at_hw_send("+ERR\r\n", strlen("+ERR\r\n"));
            sys_mfree(p_val);
            dbg_print(ERR, "md5 check fail\r\n");
            goto op_done;
        }
        else {
            char *mp3_file_path = (char *)(p_val + 16);

            res = f_open(p_file, mp3_file_path, FA_WRITE | FA_OPEN_APPEND);
            if (res != FR_OK) {
                dbg_print(ERR,"open %s file fail %d\r\n", mp3_file_path, res);
                at_hw_send("+ERR\r\n", strlen("+ERR\r\n"));
                sys_mfree(p_val);
                goto op_done;
            }

            res = f_write(p_file, p_val + 16 + mp3_name_len, data_len - 16 - mp3_name_len, (UINT *)&wr_len);
            if (res != FR_OK || wr_len == 0) {
                dbg_print(ERR, "write(%d) file fail %d.\r\n", wr_len, res);
                f_sync(p_file);
                f_close(p_file);
                sys_mfree(p_val);
                goto op_done;
            }
        }
        sys_mfree(p_val);
        f_sync(p_file);
        f_close(p_file);
    }

    break;

    default:
        break;
    }

    at_hw_send("+OK\r\n", strlen("+OK\r\n"));
op_done:
    sys_mfree(p_file);
}
#endif

static void at_getbaud(int argc, char **argv)
{
    uint32_t baudrate = BAUDRATE_115200;
    AT_RSP_START(40);
    AT_RSP("+OK=%u\r\n", baudrate);
    AT_RSP_RESULT();
}

static void at_play_music(int argc, char **argv)
{
    // AT_RSP_START(40);

    char *endptr = NULL;
    atcmd_play_music_err_t err = NO_ERR;
    uint16_t idx = 0;

#ifdef CONFIG_FATFS_SUPPORT
    if (fatfs_get_fs() == NULL) {
        err = INV_OPERATOR;
        goto err_exit;
    }
#else
    err = INV_OPERATOR;
    goto err_exit;
#endif

    if (argc == 2) {
        idx = (uint16_t)strtoul((const char *)argv[1], &endptr, 10);
    } else {
        err = INV_CMD_FORMAT_ERR;
        goto err_exit;
    }

    err = play_mp3(idx);
    if (err) {
        err = err == 4 ? FILE_NOT_FOUND : OPERATOR_NOT_PERMIT;
        goto err_exit;
    }

    // AT_RSP_DIRECT("+OK\r\n", 5);
    // AT_RSP_FREE();
    return;

err_exit:
    // AT_RSP("+ERR=-%d,\r\n", err);
    // AT_RSP_RESULT();
    // AT_RSP_DIRECT("+IND=STOP MUSIC\r\n", 17);
    return;
}

static void at_get_play_num(int argc, char **argv)
{
    AT_RSP_DIRECT("+ERR=-5\r\n", 9);
}

static void at_set_music_vol(int argc, char **argv)
{
    AT_RSP_START(40);

    char *endptr = NULL;
    atcmd_play_music_err_t err = NO_ERR;
    uint8_t vol_idx = 0;

#ifdef CONFIG_FATFS_SUPPORT
    if (fatfs_get_fs() == NULL) {
        err = INV_OPERATOR;
        goto err_exit;
    }
#else
    err = INV_OPERATOR;
    goto err_exit;
#endif

    if (argc == 2) {
#ifdef MAX98357_USED
        err = OPERATOR_NOT_PERMIT;
        goto err_exit;
#endif
        vol_idx = (uint8_t)strtoul((const char *)argv[1], &endptr, 10);
    } else {
        err = INV_CMD_FORMAT_ERR;
        goto err_exit;
    }

#ifdef ES8375_USED
    if (!es8375_volume_set(vol_idx)) {
        err = OPERATOR_NOT_PERMIT;
        goto err_exit;
    }
#endif

    AT_RSP_DIRECT("+OK\r\n", 5);
    AT_RSP_FREE();
    return;

err_exit:
    AT_RSP("+ERR=-%d,\r\n", err);
    AT_RSP_RESULT();
}

static void at_get_music_vol(int argc, char **argv)
{
#ifdef ES8375_USED
    AT_RSP_START(40);
    atcmd_play_music_err_t err = NO_ERR;
    uint8_t vol_idx = 0;

#ifdef CONFIG_FATFS_SUPPORT
    if (fatfs_get_fs() == NULL) {
        err = INV_OPERATOR;
        goto err_exit;
    }
#else
    err = INV_OPERATOR;
    goto err_exit;
#endif

    if (!es8375_volume_get(&vol_idx)) {
        err = OPERATOR_NOT_PERMIT;
        goto err_exit;
    }

    AT_RSP("+OK=%d\r\n", vol_idx);
    AT_RSP_RESULT();
    return;
err_exit:
    AT_RSP("+ERR=-%d,\r\n", err);
    AT_RSP_RESULT();
#endif

#ifdef MAX98357_USED
    AT_RSP_DIRECT("+ERR=-5\r\n", 9);
#endif
}

static void at_get_music_ver(int argc, char **argv)
{
    AT_RSP_START(60);
    const uint16_t *version = mp3_get_version();

    AT_RSP("+OK=SV:%d.%d.%d\r\n", version[0], version[1], version[2]);
    AT_RSP_RESULT();
}
