#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// adapt layer
#include "tkl_queue.h"
#include "tkl_semaphore.h"
#include "tkl_thread.h"
#include "tkl_memory.h"
#include "tkl_system.h"
#include "tal_log.h"
#include "wrapper_os.h"

// app layer
#include "mm_cmd_parse.h"
#include "mm_icc.h"
// #include "iperf_reply.h"
#ifdef ICC_USE_USB
#include "mm_usb.h"
#else
#include "mm_spi.h"
#endif

// static TKL_THREAD_HANDLE debug_task_handle = NULL;
#define DEBUG_LEN 4096
char *data_g_test = NULL;

static TKL_THREAD_HANDLE debug_task_handle1 = NULL;
static void ty_press_test_task(void *not_used)
{
    (void)not_used;
    int i = 0;
    char temp = 0;
    int j = 0;
    // tuya_os_adapt_system_sleep(5000);
    while (1) {
        temp = data_g_test[i + 1];
        data_g_test[i + 1] = 0;
        if (lp_proto_press_test(data_g_test) == 0) {
            data_g_test[i + 1] = temp;
            if (i < 3700 - 1) {
                i++;
            } else {
                i = 0;
                if (j++ == 5) {
                    break;
                }
            }
            tuya_os_adapt_system_sleep(5);
        } else {
            data_g_test[i + 1] = temp;
            tuya_os_adapt_system_sleep(100);
        }
    }
    debug_task_handle1 = NULL;
    tuya_os_adapt_thread_release(NULL);
}
// static void ty_print_tasks1(void* not_used)
// {
//     char pbuffer[2048] = {0};
//     while (1) {
//         tuya_os_adapt_system_sleep(10*1000);
//         vTaskGetRunTimeStats(pbuffer);
//         printf("----------------------------------------------\r\n");
//         printf("%s", pbuffer);
//         printf("----------------------------------------------\r\n");
//     }
//     debug_task_handle = NULL;
//     tuya_os_adapt_thread_release( NULL );

// }

void test_func(void)
{
    int ret = 0;
    if (data_g_test == NULL) {
        data_g_test = tkl_system_malloc(DEBUG_LEN);
        for (int i = 0; i < DEBUG_LEN; i++) {
            data_g_test[i] = (i % 90) + 0x21;
            if (i >= 3900) {
                data_g_test[i] = 0;
            }
        }
    }

    if (NULL == debug_task_handle1) {
        ret = tkl_thread_create(&debug_task_handle1, "debug_task1", 1024 * 4, 5, ty_press_test_task, NULL);
        if (OPRT_OK != ret) {
            PR_ERR("failed in test thread.");
        } else {
            PR_INFO("debug1 start");
        }
    }
}

/************************micro define**********************/
#define RX_QUEUE_SIZE         32
#define ICC_THREAD_STACK_SIZE (1024 * 4)
#define ICC_THREAD_PRIORITY   (OS_TASK_PRIORITY(2) + 2)
#define ICC_BLOCK_TIME        1000

TKL_QUEUE_HANDLE icc_rx_queue = NULL;

TKL_THREAD_HANDLE ty_icc_recv_thread = NULL;
int tx_count = 0;
int rx_count = 0;

static int ty_icc_rx_queue(void *buf, int size)
{
    void *msg = NULL;
    if (NULL == icc_rx_queue) {
        PR_ERR("icc rx queue not init.");
        return -1;
    }

    if (NULL == buf || 0 == size) {
        PR_ERR("\n%s:%d invalid args\n", __func__, __LINE__);
        return -1;
    }

    msg = tkl_system_malloc(size);

    if (NULL == msg) {
        PR_ERR("\n%s:%d failed in malloc\n", __func__, __LINE__);
        return -1;
    }
    tkl_system_memcpy(msg, buf, size);
#if 1
    struct ty_data_header *data_head = msg;
    unsigned int data_len = data_head->len;
    if (data_len != size - sizeof(struct ty_data_header)) {
        PR_ERR("recv error usb recv:[%d] parsed len:[%u]", size, data_len);
    }
#endif

    if (OPRT_OK != tkl_queue_post(icc_rx_queue, &msg, 0)) {
        PR_ERR("Post data in rx queue failed");
        tkl_system_free(msg);
        msg = NULL;
        return -1;
    }
    return 0;
}

int ty_icc_send_cmd(void *buf, int size)
{
#ifdef ICC_USE_USB
    return ty_usbd_send(buf, size, DATA_CMD);
#endif
    return cmd_to_spi(buf, size, 0);
}

int ty_icc_send_eth(void *buf, int size)
{
#ifdef ICC_USE_USB
    return ty_usbd_send(buf, size, DATA_ETH);
#endif
    return packet_to_spi(buf, size);
}

int ty_icc_recv(void *buf, int size)
{
    return ty_icc_rx_queue(buf, size);
}

int ty_icc_transfer_to_wifi(void *buf, int size)
{
    int ret = 0;
    char *data = (char *)buf;
    uint32_t len = (uint32_t)size;
    ret = send_to_wifi(data, len);
    return ret;
}

static void ty_icc_rx_process(void *param)
{
    (void)param;
    void *msg = NULL;
    struct ty_data_header *data_head = NULL;
    if (NULL == icc_rx_queue) {
        PR_ERR("queue not init.");
        ty_icc_recv_thread = NULL;
        tkl_thread_release(NULL);
        return;
    }
    while (1) {
        if (NULL == msg) {
            tkl_queue_fetch(icc_rx_queue, &msg, ICC_BLOCK_TIME);
        }
        if (NULL != msg) {
            data_head = msg;
            ty_lp_proto_cs_data_parse((unsigned char *)(data_head + 1), data_head->len, 0);
            tkl_system_free(msg);
            msg = NULL;
        } else {
            // PR_DEBUG("no msg received in %d ms", ICC_BLOCK_TIME);
#if defined(TUYA_SPI_RETRY) && (TUYA_SPI_RETRY == 1)
            ty_lp_proto_timeout_check();
#endif
        }
    }
    ty_icc_recv_thread = NULL;
    tkl_thread_release(NULL);
    return;
}

int ty_icc_init(void)
{
    int ret = 0;
    ret = tkl_queue_create_init(&icc_rx_queue, sizeof(void *), RX_QUEUE_SIZE);
    if (OPRT_OK != ret) {
        PR_ERR("failed in tkl_queue_create_init.");
        goto err;
    }

    ret = tkl_thread_create(&ty_icc_recv_thread, "ty_icc_recv", ICC_THREAD_STACK_SIZE, 20,
                            ty_icc_rx_process, NULL);
    if (OPRT_OK != ret) {
        PR_ERR("failed in tkl_thread_create.");
        goto err;
    }

#ifdef ICC_USE_USB
    ty_usbd_cdc_mccm_init();
#else
    mm_spi_init();
#endif

err:
    return ret;
}

int ty_icc_stop(void)
{
    // tuya_os_adapt_wifi_register_icc_callback(NULL);
#ifdef ICC_USE_USB
    // #if 0
    ty_usbd_set_sleep();
#else
    mm_spi_stop();
#endif
    return 0;
}

int ty_icc_start(void)
{
    /* wifi_trx_hook_init() 在 ty_icc_init 中已安装，此处无需再注册 icc_callback */
#ifdef ICC_USE_USB
    ty_usbd_wakeup();
#else
    mm_spi_start();
#endif
    return 0;
}

int ty_icc_ready(void)
{
#ifdef ICC_USE_USB
    ty_usbd_awake();
#else
#endif
    return 0;
}
