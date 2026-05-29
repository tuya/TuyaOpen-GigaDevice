/**
 * @file mm_spi.c
 * @brief spi communication
 *
 * @copyright Copyright(C),2018-2022, 涂鸦科技 www.tuya.com
 *
 */

#include "tuya_iot_config.h"
#if defined(SYSTEM_LINUX) && (OPERATING_SYSTEM != SYSTEM_LINUX)
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "tkl_queue.h"
#include "tkl_semaphore.h"
#include "tkl_thread.h"
#include "tkl_gpio.h"
#include "tkl_memory.h"
#include "tkl_system.h"
#include "mm_spi.h"
#include "tal_log.h"
#include "mm_icc.h"
// #include "lpmgr.h"
#include "mm_cmd_parse.h"

/* Forward declarations for platform-level DMA SPI functions */
extern void spi_dma_slave_init(void (*read_done_cb)(char *, int), void (*write_done_cb)(char *, int));
extern void spi_dma_slave_deinit(void);
extern void spi_dma_slave_read(char *buf, int size);
extern void spi_dma_slave_write(char *buf, int size);

/* Forward declarations for WiFi hook functions */
extern void wifi_trx_hook_init(void);
extern int send_to_wifi(char *buf, unsigned int len);

// 没有定义新的SPI 时默认使用这个分支
#ifndef TY_SPI_NEW_VER

#if 1
#define SLAVER_READY TUYA_GPIO_NUM_5 // PA5
#define MASTER_WRITE TUYA_GPIO_NUM_4 // PA4
#endif

/***********************************************************
*************************micro define***********************
***********************************************************/
#define TX_QUEUE_SIZE           32
#define SPI_THREAD_STACK_SIZE   (1024 * 2)  // 2K stack size.
#define CMD_THREAD_STACK_SIZE   (1024 * 6)  // 6K stack size.
#define SPI_THREAD_PRIORITY     4
#define TRX_BUF_SIZE            1600        // SPI DMA物理buffer大小，必须是32字节的倍数
#define TUYA_HEADER_LENGTH      sizeof(struct mm_data_header)
#define DATA_ETH                0xAA
#define DATA_CMD                0xBB
#define DATA_CONTROL            0xCC

/* SPI_PIECING_ENABLE 必须在 MAX_SPI_PAYLOAD_LEN 之前定义 */
#define SPI_PIECING_ENABLE

/* PIECING模式: spi_tx_buf = [msg1][msg2]...[control_msg(12B)][zeros]
 * 单条消息的有效 payload 上限 = TRX_BUF_SIZE - msg_header(12) - control_msg(12) = 1576
 * 非PIECING: 上限 = TRX_BUF_SIZE - msg_header(12) = 1588
 * WIFI_BRIDGE_MAX_ETH_LEN(1514) < MAX_SPI_PAYLOAD_LEN(1576)，标准以太网帧不会超限 */
#ifdef SPI_PIECING_ENABLE
#define MAX_SPI_PAYLOAD_LEN  (TRX_BUF_SIZE - 2U * TUYA_HEADER_LENGTH) /* 1576 */
#else
#define MAX_SPI_PAYLOAD_LEN  MAX_ICC_DATA_LEN /* 1588 */
#endif

struct mm_data_header {
    uint8_t cmd[4];
    uint32_t len;
    uint32_t crc32;
};

typedef enum {
    SPI_READING_E = 0,
    SPI_WRITING_E = 1,
} SPI_STATE_T;

typedef struct mm_spi_handle_ {
    uint8_t spi_rx_buf[TRX_BUF_SIZE] __attribute__((aligned(32)));
    uint8_t spi_tx_buf[TRX_BUF_SIZE] __attribute__((aligned(32)));
#ifdef SPI_PIECING_ENABLE
    int buffered_length;
#endif
    SPI_STATE_T state;

    struct mm_data_header *tx_msg;
    TKL_QUEUE_HANDLE spi_tx_queue;

    TKL_THREAD_HANDLE spi_thread;
    TKL_THREAD_HANDLE cmd_tx_theead;
    TKL_SEM_HANDLE spi_work_semaphore;
    TKL_SEM_HANDLE spi_isr_semaphore;
} mm_spi_handle_t;

static mm_spi_handle_t *mm_spi_handle = NULL;

/* ---- Interrupt-safe counters: incremented in ISR, read/logged in thread ---- */
static volatile uint32_t s_dma_done_cnt = 0;   /* DMA传输完成次数 */
static volatile uint32_t s_master_irq_cnt = 0; /* MASTER_WRITE GPIO中断触发次数 */
static volatile uint32_t s_tx_drop_cnt = 0;    /* msg_to_spi 入队失败（队列满/超时）计数 */

static int send_pack_to_wifi(unsigned char *buf, unsigned int len);
static int msg_to_spi(unsigned char *buf, unsigned int len, char type, unsigned int crc32);

static void master_write_handler(void *arg)
{
    (void)arg;
    OPERATE_RET ret = OPRT_OK;
    s_master_irq_cnt++; /* ISR: 仅计数，不打印，不影响时序 */
    TUYA_GPIO_LEVEL_E level;
    tkl_gpio_read(MASTER_WRITE, &level);
    if (level == TUYA_GPIO_LEVEL_HIGH) {
        ret = tkl_semaphore_post(mm_spi_handle->spi_work_semaphore);
    }
    return;
}

static void dma_done_interrupt_handler(char *buf, int len)
{
    (void)buf;
    (void)len;
    s_dma_done_cnt++; /* ISR: 仅计数，不打印，不影响时序 */
    // tkl_gpio_write(SLAVER_READY, TUYA_GPIO_LEVEL_LOW);
    tkl_semaphore_post(mm_spi_handle->spi_isr_semaphore);
    return;
}

int __is_master_writing(void)
{
    TUYA_GPIO_LEVEL_E level;
    tkl_gpio_read(MASTER_WRITE, &level);
    return (int)level;
}

void spi_master_write_init(TUYA_GPIO_IRQ_CB handler)
{
    TUYA_GPIO_IRQ_T irq_cfg = {
        .mode = TUYA_GPIO_IRQ_RISE_FALL,
        .cb = handler,
        .arg = NULL,
    };
    tkl_gpio_irq_init(MASTER_WRITE, &irq_cfg);
    tkl_gpio_irq_enable(MASTER_WRITE);
    return;
}

void spi_master_write_deinit(void)
{
    tkl_gpio_irq_disable(MASTER_WRITE);
    TUYA_GPIO_BASE_CFG_T cfg = {.mode = TUYA_GPIO_FLOATING, .direct = TUYA_GPIO_INPUT, .level = TUYA_GPIO_LEVEL_LOW};
    tkl_gpio_init(MASTER_WRITE, &cfg);
    return;
}

// ISR Out.
void set_slaver_ready(void)
{
    tkl_gpio_write(SLAVER_READY, TUYA_GPIO_LEVEL_HIGH);
    return;
}

void set_slaver_busy(void)
{
    tkl_gpio_write(SLAVER_READY, TUYA_GPIO_LEVEL_LOW);
    // tkl_system_sleep(1);
    return;
}

void spi_slaver_ready_init(void)
{
    TUYA_GPIO_BASE_CFG_T cfg = {.mode = TUYA_GPIO_PUSH_PULL, .direct = TUYA_GPIO_OUTPUT, .level = TUYA_GPIO_LEVEL_LOW};
    tkl_gpio_init(SLAVER_READY, &cfg);
    return;
}

void spi_slaver_ready_deinit(void)
{
    tkl_gpio_write(SLAVER_READY, TUYA_GPIO_LEVEL_LOW);
    return;
}

static void __stop_spi_thread_and_empty_buffer(void)
{
    if (mm_spi_handle->spi_thread == NULL || mm_spi_handle->spi_tx_queue == NULL) {
        PR_ERR("spi resource is already freed");
        return;
    }
    tkl_thread_release(mm_spi_handle->spi_thread);
    mm_spi_handle->spi_thread = NULL;
    /* 线程停止后释放手上持有的tx_msg（已出队但未发送） */
    if (mm_spi_handle->tx_msg != NULL) {
        tkl_system_free(mm_spi_handle->tx_msg);
        mm_spi_handle->tx_msg = NULL;
    }
    while (tkl_queue_fetch(mm_spi_handle->spi_tx_queue, &mm_spi_handle->tx_msg, 0) == OPRT_OK) {
        tkl_system_free(mm_spi_handle->tx_msg);
        mm_spi_handle->tx_msg = NULL;
    }
    tkl_semaphore_wait(mm_spi_handle->spi_work_semaphore, 0);
    tkl_semaphore_wait(mm_spi_handle->spi_isr_semaphore, 0);
#ifdef SPI_PIECING_ENABLE
    mm_spi_handle->buffered_length = 0;
#endif
    return;
}

static void spi_thread_handler(void *param)
{
    (void)param;

    if (NULL == mm_spi_handle->spi_tx_queue) {
        PR_ERR("queue not init.");
        mm_spi_handle->spi_thread = NULL;
        tkl_thread_release(NULL);
        return;
    }
    uint32_t seqno = 0;
    struct mm_data_header control_msg = {.cmd[0] = DATA_CONTROL, .crc32 = 0, .len = 0};
    while (1) {
        // Tx-thread block for no-data.
        if (tkl_semaphore_wait(mm_spi_handle->spi_work_semaphore, TKL_SEM_WAIT_FOREVER) != OPRT_OK) {
            PR_ERR("wait spi semaphore error!!!");
        }
        // PR_INFO("[SPI] wakeup: master_irq_cnt=%u dma_done_cnt=%u", s_master_irq_cnt, s_dma_done_cnt);

        if (__is_master_writing()) { // 记录SPI状态
            mm_spi_handle->state = SPI_READING_E;
            // PR_INFO("[SPI] direction=READING (Master is writing)");
        } else {
            mm_spi_handle->state = SPI_WRITING_E;
            // PR_INFO("[SPI] direction=WRITING (Slave has TX data, buffered_len=%d)", mm_spi_handle->buffered_length);
            if (NULL == mm_spi_handle->tx_msg) {
                if (tkl_queue_fetch(mm_spi_handle->spi_tx_queue, &mm_spi_handle->tx_msg, 0) != OPRT_OK) {
#ifdef SPI_PIECING_ENABLE
                    if (mm_spi_handle->buffered_length == 0)
#endif
                        continue; // 无数据收发重新等待信号量
                }
            }
        }
#ifdef SPI_PIECING_ENABLE
        do {
            if (NULL != mm_spi_handle->tx_msg) {
                if (mm_spi_handle->tx_msg->len + TUYA_HEADER_LENGTH >
                    sizeof(mm_spi_handle->spi_tx_buf) - TUYA_HEADER_LENGTH) {
                    PR_ERR("\n%s:%d len:%d\n", __func__, __LINE__, mm_spi_handle->tx_msg->len);
                    tkl_system_free(mm_spi_handle->tx_msg);
                    mm_spi_handle->tx_msg = NULL;
                } else {
                    if (mm_spi_handle->tx_msg->len + TUYA_HEADER_LENGTH + mm_spi_handle->buffered_length <=
                        sizeof(mm_spi_handle->spi_tx_buf) - TUYA_HEADER_LENGTH) {
                        tkl_system_memcpy(mm_spi_handle->spi_tx_buf + mm_spi_handle->buffered_length,
                                          mm_spi_handle->tx_msg, mm_spi_handle->tx_msg->len + TUYA_HEADER_LENGTH);
                        mm_spi_handle->buffered_length += mm_spi_handle->tx_msg->len + TUYA_HEADER_LENGTH;
                        tkl_system_free(mm_spi_handle->tx_msg);
                        mm_spi_handle->tx_msg = NULL;
                    } else {
                        control_msg.len = 1;
                        break;
                    }
                }
            }
            control_msg.len = 0;
            tkl_queue_fetch(mm_spi_handle->spi_tx_queue, &mm_spi_handle->tx_msg, 0);
        } while (mm_spi_handle->tx_msg != NULL);

        if (mm_spi_handle->buffered_length != 0) {
            control_msg.crc32 = seqno;
            tkl_system_memset(mm_spi_handle->spi_tx_buf + mm_spi_handle->buffered_length, 0,
                              sizeof(mm_spi_handle->spi_tx_buf) - mm_spi_handle->buffered_length);
            tkl_system_memcpy(mm_spi_handle->spi_tx_buf + mm_spi_handle->buffered_length, &control_msg,
                              sizeof(control_msg));
        } else {
            /* 全双工SPI DMA：即使无TX数据也必须配置TX DMA，否则RX DMA无法启动完成 */
            tkl_system_memset(mm_spi_handle->spi_tx_buf, 0, sizeof(mm_spi_handle->spi_tx_buf));
        }
        // PR_INFO("[SPI] DMA start: state=%s buffered_len=%d dma_done_cnt=%u",
        //         mm_spi_handle->state == SPI_READING_E ? "RD" : "WR",
        //         mm_spi_handle->buffered_length, s_dma_done_cnt);
        spi_dma_slave_write((unsigned char *)mm_spi_handle->spi_tx_buf, sizeof(mm_spi_handle->spi_tx_buf));

#else
        if (NULL != mm_spi_handle->tx_msg) {
            tkl_system_memset(mm_spi_handle->spi_tx_buf, 0, sizeof(mm_spi_handle->spi_tx_buf));
            if (mm_spi_handle->tx_msg->len + TUYA_HEADER_LENGTH > sizeof(mm_spi_handle->spi_tx_buf)) {
                PR_ERR("\n%s:%d len:%d\n", __func__, __LINE__, mm_spi_handle->tx_msg->len);
            }
            tkl_system_memcpy(mm_spi_handle->spi_tx_buf, mm_spi_handle->tx_msg,
                              mm_spi_handle->tx_msg->len + TUYA_HEADER_LENGTH);
        } else {
            /* 全双工SPI DMA：即使无TX数据也必须配置TX DMA，否则RX DMA无法启动完成 */
            tkl_system_memset(mm_spi_handle->spi_tx_buf, 0, sizeof(mm_spi_handle->spi_tx_buf));
        }
        spi_dma_slave_write(mm_spi_handle->spi_tx_buf, sizeof(mm_spi_handle->spi_tx_buf));
#endif

        tkl_system_memset(mm_spi_handle->spi_rx_buf, 0, sizeof(mm_spi_handle->spi_rx_buf));
        spi_dma_slave_read(mm_spi_handle->spi_rx_buf, sizeof(mm_spi_handle->spi_rx_buf));
        set_slaver_ready();
        if (tkl_semaphore_wait(mm_spi_handle->spi_isr_semaphore, 20) != OPRT_OK) {
            // dma中断超时20ms：大包分片场景下Master处理上一帧IP栈需数百ms才能再轮询SLAVER_READY
            set_slaver_busy();
            tkl_system_sleep(2);
            set_slaver_ready();
            if (tkl_semaphore_wait(mm_spi_handle->spi_isr_semaphore, 1000) != OPRT_OK) { // dma中断超时1000ms
                PR_ERR("[SPI] DMA sem timeout(1000ms), dma_done_cnt=%u", s_dma_done_cnt);
                set_slaver_busy();
                /* 超时：丢弃当前TX buffer，避免脏数据残留被意外重传 */
#ifdef SPI_PIECING_ENABLE
                mm_spi_handle->buffered_length = 0;
#else
                if (mm_spi_handle->tx_msg) {
                    tkl_system_free(mm_spi_handle->tx_msg);
                    mm_spi_handle->tx_msg = NULL;
                }
#endif
                tkl_semaphore_post(mm_spi_handle->spi_work_semaphore);
                continue; // 超时重试
            }
        }
        // tuya_os_adapt_semaphore_waittimeout(mm_spi_handle->spi_write_done_semaphore, 0);
        /* 使用 DMA 前记录的 state，而非重新读 GPIO：DMA 完成后 Master 已拉低 MASTER_WRITE，
         * 重读 GPIO 永远是 LOW，会将所有 READING_E 事务误判为"发送完成"路径。 */
        // if (__is_master_writing()) {
        if (mm_spi_handle->state == SPI_READING_E) {
            /* 全双工DMA已完成：TX数据（若有）已随本次传输发出，清零buffered_length避免下次重发 */
#ifdef SPI_PIECING_ENABLE
            if (mm_spi_handle->buffered_length != 0) {
                mm_spi_handle->buffered_length = 0;
                seqno++;
                if (seqno > 65535) {
                    seqno = 0;
                }
            }
#endif
            if (DATA_ETH == mm_spi_handle->spi_rx_buf[0]) {
                set_slaver_busy();
                struct mm_data_header *data_head = (struct mm_data_header *)mm_spi_handle->spi_rx_buf;
                SPI_DUMP("RX-ETH", mm_spi_handle->spi_rx_buf, data_head->len + TUYA_HEADER_LENGTH);
                int retry = 4;
                while (retry >= 0) {
                    if (send_pack_to_wifi((unsigned char *)mm_spi_handle->spi_rx_buf,
                                          sizeof(mm_spi_handle->spi_rx_buf)) == 0) {
                        break;
                    }
                    tkl_system_sleep(5);
                    retry--;
                }
                tkl_semaphore_post(mm_spi_handle->spi_work_semaphore);
            } else if (DATA_CMD == mm_spi_handle->spi_rx_buf[0]) {
                struct mm_data_header *data_head = (struct mm_data_header *)mm_spi_handle->spi_rx_buf;

                if (data_head->crc32) { // check sum
                    unsigned int i = 0;
                    unsigned int crc_cs = 0;
                    char *_data = (char *)(data_head + 1);
                    if (data_head->len + TUYA_HEADER_LENGTH <= MAX_ICC_SEND_LEN) {
                        for (i = 0; i < data_head->len; i++) {
                            crc_cs += (unsigned char)_data[i];
                        }
                    } else {
                        crc_cs = 0;
                        PR_ERR("error len:%d", data_head->len);
                    }
                    if (data_head->crc32 != crc_cs) {
                        set_slaver_busy();
                        SPI_DUMP("RX-CMD-CRCERR", mm_spi_handle->spi_rx_buf, data_head->len + TUYA_HEADER_LENGTH);
                        PR_ERR("[SPI] DATA_CMD CRC FAIL: expected=0x%08x got=0x%08x len=%u", data_head->crc32, crc_cs,
                               data_head->len);
                        PR_ERR("\n%s:%d crc check fail. data_head->crc32:%d crc_cs:%d\n", __func__, __LINE__,
                               data_head->crc32, crc_cs);
                        tkl_semaphore_post(mm_spi_handle->spi_work_semaphore);
                        continue;
                    }
                }
                if (data_head->len + TUYA_HEADER_LENGTH > MAX_ICC_SEND_LEN) {
                    PR_ERR("\n%s:%d data_head->len:%d\n", __func__, __LINE__, data_head->len);
                    set_slaver_busy();
                    tkl_semaphore_post(mm_spi_handle->spi_work_semaphore);
                    continue;
                }
                set_slaver_busy();
                // PR_INFO("[SPI] DATA_CMD received: len=%u crc32=0x%08x dma_done_cnt=%u", data_head->len,
                //         data_head->crc32, s_dma_done_cnt);
                // PR_INFO("[SPI] DATA_CMD CRC OK, dispatch ty_icc_recv: payload_len=%u total=%u", data_head->len,
                //         (unsigned)(data_head->len + TUYA_HEADER_LENGTH));
                // SPI_DUMP("RX-CMD", mm_spi_handle->spi_rx_buf, TRX_BUF_SIZE);
                ty_icc_recv(data_head, data_head->len + TUYA_HEADER_LENGTH);
                tkl_semaphore_post(mm_spi_handle->spi_work_semaphore);
            } else {
                set_slaver_busy();
                tkl_semaphore_post(mm_spi_handle->spi_work_semaphore);
                /* 0xFF = MOSI空闲电平，Master只打时钟取数据（不发），属正常现象，不触发重置 */
                if (mm_spi_handle->spi_rx_buf[0] != 0 && mm_spi_handle->spi_rx_buf[0] != 0xFF) {
                    PR_ERR("spi failed, reason: data header %02x restart spi", mm_spi_handle->spi_rx_buf[0]);
                    SPI_DUMP("RX-UNEXPECTED", mm_spi_handle->spi_rx_buf, TRX_BUF_SIZE);
                    spi_dma_slave_deinit();
                    spi_dma_slave_init(dma_done_interrupt_handler, NULL);
                }
                continue;
            }
        } else {
#ifdef SPI_PIECING_ENABLE
            if (mm_spi_handle->buffered_length != 0) {
                mm_spi_handle->buffered_length = 0;
                seqno++;
                if (seqno > 65535) {
                    seqno = 0;
                }
#else
            if (mm_spi_handle->tx_msg) {
                tkl_system_free(mm_spi_handle->tx_msg);
                mm_spi_handle->tx_msg = NULL;
#endif
            } else if (mm_spi_handle->state != SPI_READING_E) {
                PR_ERR("send done but buffer is empty!!!\n");
            }
            /* 全双工：Slave发送期间Master可能同时也发来了有效数据，应当处理而非丢弃 */
            if (mm_spi_handle->spi_rx_buf[0] == DATA_ETH) {
                set_slaver_busy();
                SPI_DUMP("RX-ETH(WR)", mm_spi_handle->spi_rx_buf, TRX_BUF_SIZE);
                int retry = 4;
                while (retry >= 0) {
                    if (send_pack_to_wifi((unsigned char *)mm_spi_handle->spi_rx_buf,
                                          sizeof(mm_spi_handle->spi_rx_buf)) == 0) {
                        break;
                    }
                    tkl_system_sleep(5);
                    retry--;
                }
            } else if (mm_spi_handle->spi_rx_buf[0] == DATA_CMD) {
                struct mm_data_header *data_head = (struct mm_data_header *)mm_spi_handle->spi_rx_buf;
                if (data_head->len + TUYA_HEADER_LENGTH <= MAX_ICC_SEND_LEN) {
                    // PR_INFO("[SPI] DATA_CMD(WR) received: len=%u", data_head->len);
                    SPI_DUMP("RX-CMD(WR)", mm_spi_handle->spi_rx_buf, TRX_BUF_SIZE);
                    ty_icc_recv(data_head, data_head->len + TUYA_HEADER_LENGTH);
                } else {
                    PR_ERR("DATA_CMD(WR) invalid len:%u", data_head->len);
                }
                set_slaver_busy();
            } else {
                /* 0xFF=MOSI空闲，其他非协议头 —— 仅对非常规内容输出警告 */
                if (mm_spi_handle->spi_rx_buf[0] != 0 && mm_spi_handle->spi_rx_buf[0] != 0xFF) {
                    PR_ERR("recv data in send process with head %02x\n", mm_spi_handle->spi_rx_buf[0]);
                }
                set_slaver_busy();
            }
            tkl_semaphore_post(mm_spi_handle->spi_work_semaphore);
        }
    }
    mm_spi_handle->spi_thread = NULL;
    tkl_thread_release(NULL);
    return;
}

void mm_spi_deinit(void)
{
    if (NULL == mm_spi_handle) {
        return;
    }

    if (NULL != mm_spi_handle->spi_thread) {
        tkl_thread_release(mm_spi_handle->spi_thread);
        mm_spi_handle->spi_thread = NULL;
    }

    /* 释放手上持有的tx_msg及队列中所有待发消息，避免内存泄漏 */
    if (mm_spi_handle->tx_msg != NULL) {
        tkl_system_free(mm_spi_handle->tx_msg);
        mm_spi_handle->tx_msg = NULL;
    }
    if (NULL != mm_spi_handle->spi_tx_queue) {
        struct mm_data_header *item = NULL;
        while (tkl_queue_fetch(mm_spi_handle->spi_tx_queue, &item, 0) == OPRT_OK) {
            tkl_system_free(item);
        }
        tkl_queue_free(mm_spi_handle->spi_tx_queue);
        mm_spi_handle->spi_tx_queue = NULL;
    }

    if (NULL != mm_spi_handle->spi_isr_semaphore) {
        tkl_semaphore_release(mm_spi_handle->spi_isr_semaphore);
        mm_spi_handle->spi_isr_semaphore = NULL;
    }

    if (NULL != mm_spi_handle->spi_work_semaphore) {
        tkl_semaphore_release(mm_spi_handle->spi_work_semaphore);
        mm_spi_handle->spi_work_semaphore = NULL;
    }

    tkl_system_free(mm_spi_handle);
    mm_spi_handle = NULL;

    return;
}

// not used.
int mm_spi_reset(void)
{
    mm_spi_stop();
    mm_spi_start();

    return 0;
}

int mm_spi_init(void)
{
    int ret = 0;
    if (mm_spi_handle != NULL) {
        /* Already initialised — skip to avoid leaking the old handle and
         * overwriting the global pointer while spi_thread is still running. */
        PR_NOTICE("mm_spi_init: already initialised, skip.");
        return OPRT_OK;
    }
    mm_spi_handle = tkl_system_malloc(sizeof(mm_spi_handle_t));
    if (NULL == mm_spi_handle) {
        PR_ERR("failed to malloc mm_spi_handle.");
        return -1;
    }
    tkl_system_memset(mm_spi_handle, 0, sizeof(mm_spi_handle_t));
    // init queue.
    ret = tkl_queue_create_init(&mm_spi_handle->spi_tx_queue, sizeof(void *), TX_QUEUE_SIZE);
    if (OPRT_OK != ret) {
        PR_ERR("failed in tkl_queue_create_init.");
        goto err;
    }

    ret = tkl_semaphore_create_init(&mm_spi_handle->spi_work_semaphore, 0, 1);
    if (OPRT_OK != ret) {
        PR_ERR("failed in tkl_semaphore_create_init.");
        goto err;
    }

    ret = tkl_semaphore_create_init(&mm_spi_handle->spi_isr_semaphore, 0, 1);
    if (OPRT_OK != ret) {
        PR_ERR("failed in tkl_semaphore_create_init.");
        goto err;
    }

#if defined(TUYA_SPI_RETRY) && (TUYA_SPI_RETRY == 1)
    __ty_cmd_tx_sem_init();
    __ty_list_mutex_init();
    ret = tkl_thread_create(&mm_spi_handle->cmd_tx_theead, "cmd_tx_thread", SPI_THREAD_STACK_SIZE, SPI_THREAD_PRIORITY,
                            ty_spi_cmd_send_thread, NULL);
    if (OPRT_OK != ret) {
        PR_ERR("failed in tkl_queue_create_init.");
        goto err;
    }
#endif
    return OPRT_OK;

err:
    /* 错误路径：清理已分配的资源，避免内存泄漏 */
    mm_spi_deinit();
    return ret;
}

int cmd_to_spi(unsigned char *buf, unsigned int len, unsigned int crc32)
{
    return msg_to_spi(buf, len, DATA_CMD, crc32);
}

int packet_to_spi(unsigned char *buf, unsigned int len)
{
    SPI_DUMP("TX-ETH", buf, len);
    // PR_INFO("[SPI] packet_to_spi: len=%u", len);
    return msg_to_spi(buf, len, DATA_ETH, 0);
}

static int msg_to_spi(unsigned char *buf, unsigned int len, char type, unsigned int crc32)
{
    char *msg = NULL;
    struct mm_data_header *data_head = NULL;

    if (NULL == mm_spi_handle->spi_tx_queue) {
        PR_ERR("\n%s:%d mm_spi_handle->spi_tx_queue not initialize.\n", __func__, __LINE__);
        goto exit;
    }

    /* MAX_SPI_PAYLOAD_LEN:
     *   PIECING模式 = 1576 (为control_msg保留尾部12字节)
     *   非PIECING   = 1588
     * 标准以太网帧最大 = WIFI_BRIDGE_MAX_ETH_LEN(1514) < 1576，正常情况下不会触发此检查 */
    if (NULL == buf || len == 0 || len > (unsigned)MAX_SPI_PAYLOAD_LEN) {
        PR_ERR("[SPI] msg_to_spi: invalid len=%u (max=%u) type=0x%02x, dropped",
               len, (unsigned)MAX_SPI_PAYLOAD_LEN, (unsigned char)type);
        goto exit;
    }

    msg = tkl_system_malloc(len + sizeof(struct mm_data_header));
    if (NULL == msg) {
        PR_ERR("\n%s:%d failed in malloc\n", __func__, __LINE__);
        goto exit;
    }

    tkl_system_memset(msg, 0, len + sizeof(struct mm_data_header));

    data_head = (struct mm_data_header *)msg;
    data_head->cmd[0] = type;
    data_head->len = len;
    data_head->crc32 = crc32;
    // copy data.
    tkl_system_memcpy(data_head + 1, buf, len);

    if (OPRT_OK == tkl_queue_post(mm_spi_handle->spi_tx_queue, &msg, 100)) {
        tkl_semaphore_post(mm_spi_handle->spi_work_semaphore);
        return 0;
    }
    s_tx_drop_cnt++;
    PR_ERR("[SPI] tx queue full or timeout, drop type=0x%02x len=%u, drop_cnt=%u",
           (unsigned char)type, len, s_tx_drop_cnt);
exit:
    tkl_system_free(msg);
    msg = NULL;

    return -1;
}

static int send_pack_to_wifi(unsigned char *buf, unsigned int len)
{
    (void)len;
    struct mm_data_header *data_head = (struct mm_data_header *)buf;
    /* 防御：data_head->len 不可超过 DMA buffer 中可用的有效数据区 */
    if ((int)data_head->len < 0 || data_head->len > MAX_ICC_DATA_LEN) {
        PR_ERR("[SPI] send_pack_to_wifi: ETH frame len=%u exceeds MAX_ICC_DATA_LEN=%u, dropped",
               data_head->len, (unsigned)MAX_ICC_DATA_LEN);
        return -1;
    }
    // PR_INFO("[SPI] send_pack_to_wifi: len=%u", data_head->len);
    return send_to_wifi((char *)(data_head + 1), data_head->len);
}

void mm_spi_start(void)
{
    PR_INFO("spi start!");
    // if (!lpmgr_is_registered(TY_LP_SPI)) {
    //     lpmgr_register(TY_LP_SPI);
    // }
    // init spi device.

    spi_dma_slave_init(dma_done_interrupt_handler, NULL);
    // init wifi hook
    wifi_trx_hook_init();

    // init spi interrupt.
    spi_slaver_ready_init();
    spi_master_write_init(master_write_handler);
#if defined(TUYA_SPI_RETRY) && (TUYA_SPI_RETRY == 1)
    // tuya_empty_cmd_tx_list();
    __ty_cmd_tx_sem_post();
    rx_status_reset();
#endif

    if (mm_spi_handle->spi_thread == NULL) {
        if (OPRT_OK != tkl_thread_create(&mm_spi_handle->spi_thread, "spi_thread", SPI_THREAD_STACK_SIZE,
                                         SPI_THREAD_PRIORITY, spi_thread_handler, NULL)) {
            PR_ERR("failed in tkl_thread_create. %s in LINE %d reboot!!!", __func__, __LINE__);
            tkl_system_sleep(1000);
            tkl_system_reset();
        }
    }

    return;
}

void mm_spi_stop(void)
{
    // deinit wifi hook
    // wifi_trx_hook_deinit();
    // init spi interrupt.
#if defined(TUYA_SPI_RETRY) && (TUYA_SPI_RETRY == 1)
    __ty_cmd_tx_sem_wait();
    tuya_empty_cmd_tx_list(); // TODO: replace with TKL equivalent
#endif

    __stop_spi_thread_and_empty_buffer();
    spi_slaver_ready_deinit();
    spi_master_write_deinit();

    // deinit spi device.
    spi_dma_slave_deinit();

    // if (lpmgr_is_registered(TY_LP_SPI)) {
    //     lpmgr_unregister(TY_LP_SPI);
    // }

    return;
}

#endif // end TY_SPI_NEW_VER

#endif /* #if defined(SYSTEM_LINUX) && (OPERATING_SYSTEM != SYSTEM_LINUX) */
