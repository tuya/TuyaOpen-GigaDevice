/*!
    \file    trace_ext.c
    \brief   Trace external for GD32VW55x SDK.

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

/**
 ******************************************************************************
 * @defgroup TRACE TRACE
 * @ingroup DEBUG
 * @brief Trace buffer debug module.
 *
 * Trace module is enabled when fw is compiled with TRACE=on option.\n
 *
 * Trace module is using a buffer located in shared ram, to store trace event
 * form fw. A trace entry is optimized to only contains IDs and parameters,
 * instead of complete string. This IDs and parameters can then be decoded
 * using dictionary generated at compilation time.
 *
 * A trace entry in memory looks like :
 * @verbatim
 *  15       8 7        0
 * +---------------------+ --+
 * | NB PARAM |  ID MSB  |   |
 * +---------------------+   |
 * |       ID LSB        |   |
 * +---------------------+   +--> header (fixed size)
 * | TIMESTAMP  MSB      |   |
 * +---------------------+   |
 * | TIMESTAMP  LSB      |   |
 * +---------------------+ --+
 * | PARAM(1)            |   |
 * +---------------------+   |
 *    ....                   +--> parameters (variable size)
 * +---------------------+   |
 * | PARAM(x)            |   |
 * +---------------------+ --+
 * @endverbatim

 * @verbatim
 *  15       8 7        0
 * +---------------------+ --+
 * |     OFFSET ID LSB   |   |
 * +---------------------+   |
 * |     OFFSET ID MSB   |   |
 * +---------------------+   +
 * | Reserve | MODULE ID |   |
 * +---------------------+   +
 * |NB PARAM | TRACELEVEL|   |
 * +---------------------+   |
 * | TIMESTAMP  MSB      |   |
 * +---------------------+   |
 * | TIMESTAMP  LSB      |   |
 * +---------------------+ --+
 * | PARAM(1)            |   |
 * +---------------------+   |
 *    ....                   +--> parameters (variable size)
 * +---------------------+   |
 * | PARAM(x)            |   |
 * +---------------------+ --+
 * @endverbatim
 *
 * Trace buffer is defined by @ref _trace_start and @ref _trace_end whose value
 * is defined in linker script in order to use all the remaining shared ram.
 *
 * @{
 ******************************************************************************
 */

#include "trace_ext.h"
#include "systime.h"
#include "ll.h"

#ifdef CFG_GD_TRACE_EXT
#include "trace_uart.h"
#include "gd32vw55x.h"

#if (defined(TRACE_UART_DMA) && defined(CFG_GD_TRACE_DYNAMIC_PRI_SCH))
#error "TRACE print mode define error!"
#endif

#ifdef CFG_GD_TRACE_DYNAMIC_PRI_SCH
#include "wrapper_os.h"
#define TRACE_PRIORITY_MAX    2
static os_task_t priority_adjust_task_handle = NULL;
static os_task_t trace_task_handle = NULL;
#endif

#define TRACE_ADDR_NO_ALIGN      1

#if (defined(CFG_GD_TRACE_DYNAMIC_PRI_SCH))
static uint32_t sched_low_level;
static uint32_t sched_up_level;

#define SCHED_LOW_LEVEL_FACTOR    5
#define SCHED_UP_LEVEL_FACTOR     2
#endif
/*
 * DEFINES
 ******************************************************************************
 */
// Maximum number of parameter in one trace entry
#define TRACE_MAX_PARAM  0xff
// Maximum ID of a trace entry
#define TRACE_MAX_ID     0xffffff

#define TRACE_SYNC_WORD       0x7E

#define COMPLT_FLAG               0x00
#define START_FLAG                0x01
#define CONTINUE_FLAG             0x02
#define END_FLAG                  0x03


// Size of trace header
#ifdef NEW_TRACE_USED
#define TRACE_HEADER_BYTES 12     // | Log Offset ID[4 Bytes]| Module ID[1 Byte] | reserv[1 Byte] | Trace Level[1 Byte] | NB_PARAMS[1 Byte] | TimeStampMSB[2 Bytes] | TimeStampLSB[2 Bytes] |
#else
#define TRACE_HEADER_BYTES 8
#endif

#if TRACE_ADDR_NO_ALIGN
#define LOG_HEADER_BYTES                0x06       // | Sync Word[1 Byte]| SeqNo[1 Byte] | Payload(10 bits)-Flag(2 bits)-Type(4 bits)[2 Bytes] | Extra Flag[1 Byte] | CheckSum[1 Byte]
#define MAX_PAYLOAD_LEN                 0x3FE
#else
#define LOG_HEADER_BYTES                0x08       // | Sync Word[1 Byte]| SeqNo[1 Byte] | Payload(10 bits)-Flag(2 bits)-Type(4 bits)[2 Bytes] | Extra Flag[1 Byte] | CheckSum[1 Byte] | Align Reserv(2 Bytes)
#define MAX_PAYLOAD_LEN                 0x3FC

#define RESV_BYTES_FOR_ALIGN            0x01
#endif
#define BTSNOOP_HEADER_BYTES            0x08      //1(hci_type) + 4(timestamp) + 3(reserv)

#define TOTAL_TRACE_HEADER_BYTES        ((LOG_HEADER_BYTES) + (TRACE_HEADER_BYTES))
#define TOTAL_BTSNOOP_HEADER_BYTES      ((LOG_HEADER_BYTES) + (BTSNOOP_HEADER_BYTES))


#define PAYLOAD_LEN_MASK          0x03FF


/*
 * GLOBAL VARIABLES
 ******************************************************************************
 */
// Global variable indicating if trace buffer has been initialized.
static bool trace_initialized = false;
// Global variable indicating if trace buffer must be used as a circular buffer
static bool trace_loop = false;

//#define TRACE_SIZE_MAX      0x4000

extern uint32_t _trace[];
extern uint32_t _etrace[];
static uint8_t *trace_start = (uint8_t *)_trace;
static uint32_t TRACE_SIZE_MAX = 0x4000;

struct trace_env_tag {
    // Index (in 16bits word) of the first trace within the trace buffer
    volatile uint32_t trace_start;
    // Index (in 16bits word) of the last trace within the trace buffer
    volatile uint32_t trace_end;
    //sequence number
    volatile uint8_t seqno;
    volatile uint8_t btsnoop_seqno;
    volatile uint8_t wifi_seqno;

#ifdef CFG_GD_TRACE_DYNAMIC_PRI_SCH
    volatile uint8_t trace_priority;
    volatile bool task_sleep;
#endif

#ifdef TRACE_UART_DMA
    volatile uint16_t dma_send_bytes;
#endif
};
static struct trace_env_tag trace_env;

static uint8_t ble_trace_mask[(MODULE_NUM + 1)/2] = {0};

/*
 * FUNCTION DEFINITIONS
 ******************************************************************************
 */
#ifdef CFG_GD_TRACE_DYNAMIC_PRI_SCH
static void task_notify_with_isr_check(os_task_t task_handler)
{
    // In interrupt
    if(__get_CONTROL() == 1)
    {
        sys_task_notify((void *)task_handler, true);
    }
    else if(__get_CONTROL() == 0)
    {
        sys_task_notify((void *)task_handler, false);
    }
}

static void priority_adjust_task(void *argv)
{
    uint32_t room_left;

    priority_adjust_task_handle = (os_task_t)xTaskGetCurrentTaskHandle();
    for (;;)
    {
        room_left = (trace_env.trace_start + TRACE_SIZE_MAX - (trace_env.trace_end + 1)) % TRACE_SIZE_MAX;
        if(room_left < sched_low_level && trace_env.trace_priority == 0)
        {
            trace_env.trace_priority = TRACE_PRIORITY_MAX;
            sys_priority_set(trace_task_handle, OS_TASK_PRIORITY(TRACE_PRIORITY_MAX));
        }
        else if(room_left > sched_up_level && trace_env.trace_priority != 0)
        {
            trace_env.trace_priority = 0;
            sys_priority_set(trace_task_handle, OS_TASK_PRIORITY(0));
        }
        /* wait for notification */
        sys_task_wait_notification(-1);
    }
}

static void trace_print_task(void *argv)
{
    uint32_t room_left;

    for (;;) {
        trace_env.task_sleep = false;
        if(trace_count() != 0)
        {
            trace_print(300);
            room_left = (trace_env.trace_start + TRACE_SIZE_MAX - (trace_env.trace_end + 1)) % TRACE_SIZE_MAX;
            if(trace_env.trace_priority != 0 && room_left > sched_up_level)
            {
                sys_task_notify((void *)priority_adjust_task_handle, false);
            }
        }
        else
        {
            trace_env.task_sleep = true;
            /* wait for notification */
            sys_task_wait_notification(-1);
        }
    }
}
#endif

static uint32_t free_space_check_hdl()
{
    uint32_t room_left = (trace_env.trace_start + TRACE_SIZE_MAX - (trace_env.trace_end + 1)) % TRACE_SIZE_MAX;
#if (defined(CFG_GD_TRACE_DYNAMIC_PRI_SCH))
    if(trace_env.trace_priority == 0 && room_left < sched_low_level)
    {
        task_notify_with_isr_check(priority_adjust_task_handle);
    }
#endif
    return room_left;
}

#if (TRACE_ADDR_NO_ALIGN == 0)
static void trace_buf_Align()
{
    trace_env.trace_end = (((trace_env.trace_end + 3) & 0xFFFFFFFFC)) % TRACE_SIZE_MAX;
}
#endif

static void trace_buf_write(uint8_t *buf, uint16_t len)
{
    if (trace_env.trace_end + len <= TRACE_SIZE_MAX)
    {
        memcpy(trace_start + trace_env.trace_end, buf, len);
        trace_env.trace_end += len;
    }
    else
    {
        uint16_t tlen = TRACE_SIZE_MAX - trace_env.trace_end;

        memcpy(trace_start + trace_env.trace_end, buf, tlen);
        memcpy(trace_start, buf + tlen, len - tlen);
        trace_env.trace_end = len - tlen;
    }

    trace_env.trace_end %= TRACE_SIZE_MAX;
}


static bool free_used_space(uint32_t need_bytes)
{
    do {
        uint32_t data_len;
        uint8_t *p_start = trace_start + trace_env.trace_start;
        //FIX TODO we may remove continue payload and checksum
        data_len = p_start[2] | (p_start[3] << 8);
        data_len &= PAYLOAD_LEN_MASK;
        data_len += LOG_HEADER_BYTES;      //add header
        need_bytes = data_len < need_bytes ? (need_bytes - data_len) : 0;
        trace_env.trace_start = (data_len + trace_env.trace_start) % TRACE_SIZE_MAX;
    }while(need_bytes > 0);

    return true;
}

void trace_console(uint16_t len, uint8_t *p_buf)
{
    uint8_t header[LOG_HEADER_BYTES] = {0};
    uint16_t length;
    uint8_t flag = 0;
    uint8_t block_num;
    uint16_t left_bytes;
    uint32_t total_bytes;
    uint32_t room_left;
#if (TRACE_ADDR_NO_ALIGN == 0)
    uint32_t temp_total_bytes = 0;
#endif

    if(!trace_initialized || len == 0 || p_buf == NULL)
    {
        return;
    }

    block_num = len / MAX_PAYLOAD_LEN;
    left_bytes = len % MAX_PAYLOAD_LEN;
    total_bytes = block_num * (LOG_HEADER_BYTES + MAX_PAYLOAD_LEN);
    if(left_bytes > 0)
    {
        total_bytes += (LOG_HEADER_BYTES + left_bytes);
    }

#if (TRACE_ADDR_NO_ALIGN == 0)
    temp_total_bytes = (total_bytes + 3) & 0xFFFFFFFC;
#endif

    /* Needed to be able to trace within IT handler */
    GLOBAL_INT_DISABLE();

    room_left = free_space_check_hdl();

#if TRACE_ADDR_NO_ALIGN
    if(room_left < total_bytes)
    {
        if(!trace_loop)
        {
            goto end;     //drop the current log
        }

        if(!free_used_space(total_bytes - room_left))
        {
            goto end;
        }
    }
#else
    if(room_left < temp_total_bytes)
    {
        goto end;     //drop the current log
    }
#endif

    //reuse total_bytes to payload total length
    //remove sync header
    total_bytes -= LOG_HEADER_BYTES * block_num;
    if(left_bytes > 0) {
        total_bytes -= LOG_HEADER_BYTES;
    }

    header[0] = TRACE_SYNC_WORD;
    header[1] = 0xFF;
    length = block_num > 0 ? MAX_PAYLOAD_LEN : left_bytes;
    flag = COMPLT_FLAG;
    header[2] = length & 0xFF;
    header[3] = ((length >> 8) & 0x03) | (TRACE_TYPE_CONSOLE << 4) | (flag << 2);
#if TRACE_ADDR_NO_ALIGN
    header[4] = 0;         //extra flag
#else
    header[4] = RESV_BYTES_FOR_ALIGN;
#endif

    header[5] = header[0] ^ header[1] ^ header[2] ^ header[3] ^ header[4];
#if (TRACE_ADDR_NO_ALIGN == 0)
    header[6] = 0xFF;
    header[7] = 0xFF;
#endif

    //reuse left_bytes to first packet left bytes
    left_bytes = MAX_PAYLOAD_LEN - LOG_HEADER_BYTES;
    trace_buf_write(header, LOG_HEADER_BYTES);

    //first payload segment
    if(total_bytes <= left_bytes)
    {
        trace_buf_write(p_buf, total_bytes);
#if TRACE_ADDR_NO_ALIGN
        goto end;
#else
        goto align_buffer;
#endif

    }
    else
    {
        trace_buf_write(p_buf, left_bytes);
        p_buf += left_bytes;
        total_bytes -= left_bytes;
    }

    //left bytes always complete packet
    header[0] = TRACE_SYNC_WORD;
    header[1] = 0xFF;
    flag = COMPLT_FLAG;
    do {
        if(total_bytes <= MAX_PAYLOAD_LEN)
        {
            length = total_bytes;
            total_bytes = 0;
        }
        else
        {
            length = MAX_PAYLOAD_LEN;
            total_bytes -= MAX_PAYLOAD_LEN;
        }
        header[2] = length & 0xFF;
        header[3] = ((length >> 8) & 0x03) | (TRACE_TYPE_CONSOLE << 4) | (flag << 2);
#if TRACE_ADDR_NO_ALIGN
        header[4] = 0;         //extra flag
#else
        header[4] = RESV_BYTES_FOR_ALIGN;
#endif

        header[5] = header[0] ^ header[1] ^ header[2] ^ header[3] ^ header[4];
#if (TRACE_ADDR_NO_ALIGN == 0)
        header[6] = 0xFF;
        header[7] = 0xFF;
#endif

        trace_buf_write(header, LOG_HEADER_BYTES);
        trace_buf_write(p_buf, length);
        p_buf += length;
    }while(total_bytes > 0);

#if (TRACE_ADDR_NO_ALIGN == 0)
align_buffer:
    trace_buf_Align();
#endif

end:
#ifdef CFG_GD_TRACE_DYNAMIC_PRI_SCH
    if(trace_env.task_sleep)
    {
        task_notify_with_isr_check(trace_task_handle);
    }
#endif
    GLOBAL_INT_RESTORE();
}

void trace_ext_init(bool force, bool loop)
{
    uint8_t init_data[] = {0x60, 0x55, 0x60, 0x55, 0x60};

    if (trace_initialized && !force)
        return;

    TRACE_SIZE_MAX = (uint8_t *)_etrace - trace_start;

    // If trace size too small, just return
    if(TRACE_SIZE_MAX < (TOTAL_TRACE_HEADER_BYTES + 10))
    {
        return;
    }

#if (defined(CFG_GD_TRACE_DYNAMIC_PRI_SCH))
    sched_low_level = TRACE_SIZE_MAX / SCHED_LOW_LEVEL_FACTOR;
    sched_up_level = TRACE_SIZE_MAX / SCHED_UP_LEVEL_FACTOR;
#endif

#ifdef CFG_GD_TRACE_DYNAMIC_PRI_SCH
    trace_task_handle = (os_task_t)sys_task_create_dynamic((const uint8_t *)"trace task", 256, OS_TASK_PRIORITY(0), trace_print_task, NULL);
    if (trace_task_handle == NULL)
        return;

    if (sys_task_create_dynamic((const uint8_t *)"priority adjust task", 256, configMAX_PRIORITIES, priority_adjust_task, NULL) == NULL)
    {
        sys_task_delete(trace_task_handle);
        return;
    }

    trace_env.trace_priority = 0;
    trace_env.task_sleep = false;
#endif

#ifdef TRACE_UART_DMA
    trace_env.dma_send_bytes = 0;
#endif

    memset(trace_start, 0, TRACE_SIZE_MAX);

    trace_env.trace_start = 0;
    trace_env.trace_end = 0;

    trace_loop = loop;
    trace_initialized = true;
    trace_env.seqno = 0;
    trace_env.wifi_seqno = 0;
    trace_env.btsnoop_seqno = 0;
    memset(ble_trace_mask, 0xFF, sizeof(ble_trace_mask));
    trace_console(5, init_data);
}

bool trace_ble_filter_set(uint8_t module, uint8_t trace_mask_set)
{
    uint8_t block, offset;
    if(!trace_initialized || module >= MODULE_NUM || trace_mask_set > 15)
    {
        return false;
    }
    block = module >> 1;
    offset = (module * 4) % 8;
    ble_trace_mask[block] = (ble_trace_mask[block] & ((uint8_t)0xF0 >> offset)) | (trace_mask_set << offset);
    return true;
}

bool trace_wifi_filter_set()
{
    return false;
}

#ifdef NEW_TRACE_USED
void trace_ble_log(uint32_t id, uint16_t nb_param, uint16_t *param, uint8_t type, uint8_t module, uint8_t trace_level)
{
    uint8_t *ptr;
    uint64_t ts;
    uint8_t header[TOTAL_TRACE_HEADER_BYTES] = {0};
    uint16_t length;
    uint8_t flag = 0;
    uint8_t block, offset;
    uint8_t block_num;
    uint16_t left_bytes;
    uint32_t total_bytes;
    uint32_t room_left;
#if (TRACE_ADDR_NO_ALIGN == 0)
    uint32_t temp_total_bytes = 0;
#endif

    if(!trace_initialized || module >= MODULE_NUM || trace_level >= LEVEL_NUM)
    {
        return;
    }

    if(TRACE_TYPE_BLE == type)
    {
        block = module >> 1;
        offset = (module & 0x01) << 2;

        if(trace_level != LEVEL_ERROR && ((ble_trace_mask[block] >> offset) & (1 << (trace_level - 1))) == 0)
        {
            return;
        }
    }
    else
    {
        return;
    }

    /* Ensure parameters provided are within expected range */
    if (nb_param > TRACE_MAX_PARAM)
        nb_param = TRACE_MAX_PARAM;
    id &= TRACE_MAX_ID;

    block_num = (TRACE_HEADER_BYTES + (nb_param << 1)) / MAX_PAYLOAD_LEN;
    left_bytes = (TRACE_HEADER_BYTES + (nb_param << 1)) % MAX_PAYLOAD_LEN;
    total_bytes = block_num * (LOG_HEADER_BYTES + MAX_PAYLOAD_LEN);
    if(left_bytes > 0)
    {
        total_bytes += (LOG_HEADER_BYTES + left_bytes);
    }

#if (TRACE_ADDR_NO_ALIGN == 0)
    temp_total_bytes = (total_bytes + 3) & 0xFFFFFFFC;
#endif

    /* Needed to be able to trace within IT handler */
    GLOBAL_INT_DISABLE();
    ts = get_sys_local_time_us();

    room_left = free_space_check_hdl();

#if TRACE_ADDR_NO_ALIGN
    if(room_left < total_bytes)
    {
        if(!trace_loop || !free_used_space(total_bytes - room_left))
        {
            trace_env.seqno++;
            goto end;     //drop the current log
        }
    }
#else
    if(room_left < temp_total_bytes)
    {
        trace_env.seqno++;
        goto end;     //drop the current log
    }
#endif

    //reuse total_bytes to payload total length
    //remove sync header
    total_bytes -= LOG_HEADER_BYTES * block_num;
    if(left_bytes > 0)
    {
        total_bytes -= LOG_HEADER_BYTES;
    }

    //Mask new type
    type |= TRACE_TYPE_NEW;
    /*log header 5 bytes*/
    header[0] = TRACE_SYNC_WORD;
    header[1] = trace_env.seqno++;
    length = block_num > 0 ? MAX_PAYLOAD_LEN : left_bytes;
    flag = block_num > 0 ? START_FLAG: COMPLT_FLAG;
    header[2] = length & 0xFF;
    header[3] = ((length >> 8) & 0x03) | (type << 4) | (flag << 2);
#if TRACE_ADDR_NO_ALIGN
    header[4] = 0;         //extra flag
#else
    header[4] = RESV_BYTES_FOR_ALIGN;
#endif
    header[5] = header[0] ^ header[1] ^ header[2] ^ header[3] ^ header[4];

#if TRACE_ADDR_NO_ALIGN
    /* write trace header 8 bytes*/
    header[6] = id & 0xFF;
    header[7] = (id >> 8) & 0xFF;
    header[8] = (id >> 16) & 0xFF;
    header[9] = (id >> 24) & 0xFF;
    header[10] = module;
    header[11] = 0xFF;         //reserv byte
    header[12] = trace_level;
    header[13] = nb_param & 0xFF;
    //MSB timestamp
    header[14] =  (ts >> 16) & 0xFF;
    header[15] = (ts >> 24) & 0xFF;
    //LSB timestamp
    header[16] = ts & 0xFF;
    header[17] = (ts >> 8) & 0xFF;
#else
    //Align Reserv
    header[6] = 0xFF;
    header[7] = 0xFF;

    /* write trace header 8 bytes*/
    header[8] = id & 0xFF;
    header[9] = (id >> 8) & 0xFF;
    header[10] = (id >> 16) & 0xFF;
    header[11] = (id >> 24) & 0xFF;
    header[12] = module;
    header[13] = 0xFF;         //reserv byte
    header[14] = trace_level;
    header[15] = nb_param & 0xFF;
    //MSB timestamp
    header[16] =  (ts >> 16) & 0xFF;
    header[17] = (ts >> 24) & 0xFF;
    //LSB timestamp
    header[18] = ts & 0xFF;
    header[19] = (ts >> 8) & 0xFF;
#endif

    //reuse left_bytes to first packet left bytes
    left_bytes = MAX_PAYLOAD_LEN - TRACE_HEADER_BYTES;
    total_bytes -= TRACE_HEADER_BYTES;
    trace_buf_write(header, TOTAL_TRACE_HEADER_BYTES);

    //first payload segment
    ptr = (uint8_t *)param;
    if(total_bytes <= left_bytes)
    {
        trace_buf_write(ptr, total_bytes);
#if TRACE_ADDR_NO_ALIGN
        goto end;
#else
        goto align_buffer;
#endif
    }
    else
    {
        trace_buf_write(ptr, left_bytes);
        ptr += left_bytes;
        total_bytes -= left_bytes;
    }

    //continue/end segment
    header[0] = TRACE_SYNC_WORD;
    do {
        header[1] = trace_env.seqno - 1;
        //end segment
        if(total_bytes <= MAX_PAYLOAD_LEN)
        {
            flag = END_FLAG;
            length = total_bytes;
            total_bytes = 0;
        }
        else
        {
            //continue segment
            flag = CONTINUE_FLAG;
            length = MAX_PAYLOAD_LEN;
            total_bytes -= MAX_PAYLOAD_LEN;
        }
        header[2] = length & 0xFF;
        header[3] = ((length >> 8) & 0x03) | (type << 4) | (flag << 2);
#if TRACE_ADDR_NO_ALIGN
        header[4] = 0;         //extra flag
#else
        header[4] = RESV_BYTES_FOR_ALIGN;
#endif
        header[5] = header[0] ^ header[1] ^ header[2] ^ header[3] ^ header[4];

#if (TRACE_ADDR_NO_ALIGN == 0)
        header[6] = 0xFF;
        header[7] = 0xFF;
#endif
        trace_buf_write(header, LOG_HEADER_BYTES);
        trace_buf_write(ptr, length);
        ptr += length;
    }while(total_bytes > 0);

#if (TRACE_ADDR_NO_ALIGN == 0)
align_buffer:
    trace_buf_Align();
#endif

end:
#ifdef CFG_GD_TRACE_DYNAMIC_PRI_SCH
    if(trace_env.task_sleep)
    {
        task_notify_with_isr_check(trace_task_handle);
    }
#endif
    GLOBAL_INT_RESTORE();
}


void trace_ble(uint32_t id, uint16_t nb_param, uint16_t *param, bool trace_buf)
{
    uint8_t *ptr;
    uint64_t ts;
    uint8_t header[TOTAL_TRACE_HEADER_BYTES] = {0};
    uint16_t length;
    uint8_t flag = 0;
    uint32_t room_left;
    uint8_t block_num;
    uint16_t left_bytes;
    uint32_t total_bytes;
#if (TRACE_ADDR_NO_ALIGN == 0)
    uint32_t temp_total_bytes = 0;
#endif

    if(!trace_initialized)
    {
        return;
    }

    /* Ensure parameters provided are within expected range */
    if (nb_param > TRACE_MAX_PARAM)
        nb_param = TRACE_MAX_PARAM;
    id &= TRACE_MAX_ID;

    block_num = (TRACE_HEADER_BYTES + (nb_param << 1)) / MAX_PAYLOAD_LEN;
    left_bytes = (TRACE_HEADER_BYTES + (nb_param << 1)) % MAX_PAYLOAD_LEN;
    total_bytes = block_num * (LOG_HEADER_BYTES + MAX_PAYLOAD_LEN);
    if(left_bytes > 0)
    {
        total_bytes += (LOG_HEADER_BYTES + left_bytes);
    }

#if (TRACE_ADDR_NO_ALIGN == 0)
    temp_total_bytes = (total_bytes + 3) & 0xFFFFFFFC;
#endif

    /* Needed to be able to trace within IT handler */
    GLOBAL_INT_DISABLE();
    ts = get_sys_local_time_us();

    room_left = free_space_check_hdl();

#if TRACE_ADDR_NO_ALIGN
    if(room_left < total_bytes)
    {
        if(!trace_loop || !free_used_space(total_bytes - room_left))
        {
            trace_env.seqno++;
            goto end;     //drop the current log
        }
    }
#else
    if(room_left < temp_total_bytes)
    {
        trace_env.seqno++;
        goto end;     //drop the current log
    }
#endif

    //reuse total_bytes to payload total length
    //remove sync header
    total_bytes -= LOG_HEADER_BYTES * block_num;
    if(left_bytes > 0)
    {
        total_bytes -= LOG_HEADER_BYTES;
    }

    /*log header 5 bytes*/
    header[0] = TRACE_SYNC_WORD;
    header[1] = trace_env.seqno++;
    length = block_num > 0 ? MAX_PAYLOAD_LEN : left_bytes;
    flag = block_num > 0 ? START_FLAG: COMPLT_FLAG;
    header[2] = length & 0xFF;
    header[3] = ((length >> 8) & 0x03) | ((TRACE_TYPE_NEW | TRACE_TYPE_BLE) << 4) | (flag << 2);
#if TRACE_ADDR_NO_ALIGN
    header[4] = 0;         //extra flag
#else
    header[4] = RESV_BYTES_FOR_ALIGN;
#endif
    header[5] = header[0] ^ header[1] ^ header[2] ^ header[3] ^ header[4];

#if TRACE_ADDR_NO_ALIGN
    /* write trace header 8 bytes*/
    header[6] = id & 0xFF;
    header[7] = (id >> 8) & 0xFF;
    header[8] = (id >> 16) & 0xFF;
    header[9] = (id >> 24) & 0xFF;
    header[10] = 0xFF;
    header[11] = 0xFF;        //reserv byte
    header[12] = 0xFF;
    header[13] = nb_param & 0xFF;
    //MSB timestamp
    header[14] =  (ts >> 16) & 0xFF;
    header[15] = (ts >> 24) & 0xFF;
    //LSB timestamp
    header[16] = ts & 0xFF;
    header[17] = (ts >> 8) & 0xFF;
#else
    //Align Reserv
    header[6] = 0xFF;
    header[7] = 0xFF;

    /* write trace header 8 bytes*/
    header[8] = id & 0xFF;
    header[9] = (id >> 8) & 0xFF;
    header[10] = (id >> 16) & 0xFF;
    header[11] = (id >> 24) & 0xFF;
    header[12] = 0xFF;
    header[13] = 0xFF;        //reserv byte
    header[14] = 0xFF;
    header[15] = nb_param & 0xFF;
    //MSB timestamp
    header[16] =  (ts >> 16) & 0xFF;
    header[17] = (ts >> 24) & 0xFF;
    //LSB timestamp
    header[18] = ts & 0xFF;
    header[19] = (ts >> 8) & 0xFF;
#endif

    //reuse left_bytes to first packet left bytes
    left_bytes = MAX_PAYLOAD_LEN - TRACE_HEADER_BYTES;
    total_bytes -= TRACE_HEADER_BYTES;
    trace_buf_write(header, TOTAL_TRACE_HEADER_BYTES);

    if (trace_buf)
    {
        uint32_t buf;
        buf = (param[1]) | (param[2] << 16);
        if (param[0] > ((TRACE_MAX_PARAM - 1) * 2))
        {
            //set bit 10 to indicate incomplete buffer
            param[0] = ((TRACE_MAX_PARAM - 1) * 2) + (1 << 10);
        }
        header[0] = param[0];
        header[1] = param[0] >> 8;
        param = _PTR_ALIGN(buf);
        #ifndef CFG_RWTL
        // set bit 9 to indicate unaligned buffer
        header[1] |= ((buf & 0x1) << 1);
        #endif
        total_bytes -= 2;
        left_bytes -= 2;
        trace_buf_write(header, 2);
    }

    //first payload segment
    ptr = (uint8_t *)param;
    if(total_bytes <= left_bytes)
    {
        trace_buf_write(ptr, total_bytes);
#if TRACE_ADDR_NO_ALIGN
        goto end;
#else
        goto align_buffer;
#endif
    }
    else
    {
        trace_buf_write(ptr, left_bytes);
        ptr += left_bytes;
        total_bytes -= left_bytes;
    }

    //continue segment
    header[0] = TRACE_SYNC_WORD;
    do {
        header[1] = trace_env.seqno - 1;
        //end segment
        if(total_bytes <= MAX_PAYLOAD_LEN)
        {
            flag = END_FLAG;
            length = total_bytes;
            total_bytes = 0;
        }
        else
        {
            //continue segment
            flag = CONTINUE_FLAG;
            length = MAX_PAYLOAD_LEN;
            total_bytes -= MAX_PAYLOAD_LEN;
        }
        header[2] = length & 0xFF;
        header[3] = ((length >> 8) & 0x03) | (TRACE_TYPE_BLE << 4) | (flag << 2);
#if TRACE_ADDR_NO_ALIGN
        header[4] = 0;         //extra flag
#else
        header[4] = RESV_BYTES_FOR_ALIGN;
#endif
        header[5] = header[0] ^ header[1] ^ header[2] ^ header[3] ^ header[4];

#if (TRACE_ADDR_NO_ALIGN == 0)
        header[6] = 0xFF;
        header[7] = 0xFF;
#endif

        trace_buf_write(header, LOG_HEADER_BYTES);
        trace_buf_write(ptr, length);
        ptr += length;
    }while(total_bytes > 0);

#if (TRACE_ADDR_NO_ALIGN == 0)
align_buffer:
    trace_buf_Align();
#endif

end:
#ifdef CFG_GD_TRACE_DYNAMIC_PRI_SCH
    if(trace_env.task_sleep)
    {
        task_notify_with_isr_check(trace_task_handle);
    }
#endif
    GLOBAL_INT_RESTORE();
}


//Fix TODO wifi need to use new way?
void trace_wifi(uint32_t id, uint16_t nb_param, uint16_t *param, bool trace_buf)
{
    uint8_t *ptr;
    uint64_t ts;
    uint8_t header[TOTAL_TRACE_HEADER_BYTES] = {0};
    uint16_t length;
    uint8_t flag = 0;
    uint32_t room_left;
    uint8_t block_num;
    uint16_t left_bytes;
    uint32_t total_bytes;
#if (TRACE_ADDR_NO_ALIGN == 0)
    uint32_t temp_total_bytes = 0;
#endif

    if(!trace_initialized)
    {
        return;
    }

    /* Ensure parameters provided are within expected range */
    if (nb_param > TRACE_MAX_PARAM)
        nb_param = TRACE_MAX_PARAM;
    id &= TRACE_MAX_ID;

    block_num = (TRACE_HEADER_BYTES + (nb_param << 1)) / MAX_PAYLOAD_LEN;
    left_bytes = (TRACE_HEADER_BYTES + (nb_param << 1)) % MAX_PAYLOAD_LEN;
    total_bytes = block_num * (LOG_HEADER_BYTES + MAX_PAYLOAD_LEN);
    if(left_bytes > 0)
    {
        total_bytes += (LOG_HEADER_BYTES + left_bytes);
    }

#if (TRACE_ADDR_NO_ALIGN == 0)
    temp_total_bytes = (total_bytes + 3) & 0xFFFFFFFC;
#endif

    /* Needed to be able to trace within IT handler */
    GLOBAL_INT_DISABLE();
    ts = get_sys_local_time_us();   //need to change

    room_left = free_space_check_hdl();

#if TRACE_ADDR_NO_ALIGN
    if(room_left < total_bytes)
    {
        if(!trace_loop || !free_used_space(total_bytes - room_left))
        {
            trace_env.wifi_seqno++;
            goto end;     //drop the current log
        }
    }

#else
    if(room_left < temp_total_bytes)
    {
        trace_env.wifi_seqno++;
        goto end;     //drop the current log
    }
#endif

    //reuse total_bytes to payload total length
    //remove sync header
    total_bytes -= LOG_HEADER_BYTES * block_num;
    if(left_bytes > 0)
    {
        total_bytes -= LOG_HEADER_BYTES;
    }

    /*log header 5 bytes*/
    header[0] = TRACE_SYNC_WORD;
    header[1] = trace_env.wifi_seqno++;
    length = block_num > 0 ? MAX_PAYLOAD_LEN : left_bytes;
    flag = block_num > 0 ? START_FLAG: COMPLT_FLAG;
    header[2] = length & 0xFF;
    header[3] = ((length >> 8) & 0x03) | (TRACE_TYPE_WIFI << 4) | (flag << 2);
#if TRACE_ADDR_NO_ALIGN
    header[4] = 0;         //extra flag
#else
    header[4] = RESV_BYTES_FOR_ALIGN;
#endif
    header[5] = header[0] ^ header[1] ^ header[2] ^ header[3] ^ header[4];


#if TRACE_ADDR_NO_ALIGN
    /* write trace header 8 bytes*/
    header[6] = (id >> 16) & 0xFF;
    header[7] = nb_param & 0xFF;
    header[8] = id & 0xFF;
    header[9] = (id >> 8) & 0xFF;
    //MSB timestamp
    header[10] =  (ts >> 16) & 0xFF;
    header[11] = (ts >> 24) & 0xFF;
    //LSB timestamp
    header[12] = ts & 0xFF;
    header[13] = (ts >> 8) & 0xFF;
#else
    //Align Reserv
    header[6] = 0xFF;
    header[7] = 0xFF;

    /* write trace header 8 bytes*/
    header[8] = (id >> 16) & 0xFF;
    header[9] = nb_param & 0xFF;
    header[10] = id & 0xFF;
    header[11] = (id >> 8) & 0xFF;
    //MSB timestamp
    header[12] =  (ts >> 16) & 0xFF;
    header[13] = (ts >> 24) & 0xFF;
    //LSB timestamp
    header[14] = ts & 0xFF;
    header[15] = (ts >> 8) & 0xFF;
#endif

    //reuse left_bytes to first packet left bytes
    left_bytes = MAX_PAYLOAD_LEN - TRACE_HEADER_BYTES - 2;
    total_bytes -= TRACE_HEADER_BYTES - 2;
    trace_buf_write(header, TOTAL_TRACE_HEADER_BYTES);

    if (trace_buf)
    {
        uint32_t buf;
        buf = (param[1]) | (param[2] << 16);
        if (param[0] > ((TRACE_MAX_PARAM - 1) * 2))
        {
            //set bit 10 to indicate incomplete buffer
            param[0] = ((TRACE_MAX_PARAM - 1) * 2) + (1 << 10);

        }
        header[0] = param[0];
        header[1] = param[0] >> 8;
        param = _PTR_ALIGN(buf);
        #ifndef CFG_RWTL
        // set bit 9 to indicate unaligned buffer
        header[1] |= ((buf & 0x1) << 1);
        #endif
        total_bytes -= 2;
        left_bytes -= 2;
        trace_buf_write(header, 2);
    }

    //first payload segment
    ptr = (uint8_t *)param;
    if(total_bytes <= left_bytes)
    {
        trace_buf_write(ptr, total_bytes);
#if TRACE_ADDR_NO_ALIGN
        goto end;
#else
        goto align_buffer;
#endif
    }
    else
    {
        trace_buf_write(ptr, left_bytes);
        ptr += left_bytes;
        total_bytes -= left_bytes;
    }

    //continue segment
    header[0] = TRACE_SYNC_WORD;
    do {
        header[1] = trace_env.wifi_seqno - 1;
        //end segment
        if(total_bytes <= MAX_PAYLOAD_LEN)
        {
            flag = END_FLAG;
            length = total_bytes;
            total_bytes = 0;
        }
        else
        {
            //continue segment
            flag = CONTINUE_FLAG;
            length = MAX_PAYLOAD_LEN;
            total_bytes -= MAX_PAYLOAD_LEN;
        }
        header[2] = length & 0xFF;
        header[3] = ((length >> 8) & 0x03) | (TRACE_TYPE_BLE << 4) | (flag << 2);
#if TRACE_ADDR_NO_ALIGN
        header[4] = 0;         //extra flag
#else
        header[4] = RESV_BYTES_FOR_ALIGN;
#endif
        header[5] = header[0] ^ header[1] ^ header[2] ^ header[3] ^ header[4];

#if (TRACE_ADDR_NO_ALIGN == 0)
        header[6] = 0xFF;
        header[7] = 0xFF;
#endif

        trace_buf_write(header, LOG_HEADER_BYTES);
        trace_buf_write(ptr, length);
        ptr += length;
    }while(total_bytes > 0);

#if (TRACE_ADDR_NO_ALIGN == 0)
align_buffer:
    trace_buf_Align();
#endif

end:
#ifdef CFG_GD_TRACE_DYNAMIC_PRI_SCH
    if(trace_env.task_sleep)
    {
        task_notify_with_isr_check(trace_task_handle);
    }
#endif
    GLOBAL_INT_RESTORE();
}

#else
void trace_ble_log(uint32_t id, uint16_t nb_param, uint16_t *param, uint8_t type, uint8_t module, uint8_t trace_level)
{
    uint8_t *ptr;
    uint64_t ts;
    uint8_t header[TOTAL_TRACE_HEADER_BYTES] = {0};
    uint16_t length;
    uint8_t flag = 0;
    uint8_t block, offset;
    uint8_t block_num;
    uint16_t left_bytes;
    uint32_t total_bytes;
    uint32_t room_left;
#if (TRACE_ADDR_NO_ALIGN == 0)
    uint32_t temp_total_bytes = 0;
#endif

    if(!trace_initialized || module >= MODULE_NUM || trace_level >= LEVEL_NUM)
    {
        return;
    }

    if(TRACE_TYPE_BLE == type)
    {
        block = module >> 1;
        offset = (module & 0x01) << 2;

        if(trace_level != LEVEL_ERROR && ((ble_trace_mask[block] >> offset) & (1 << (trace_level - 1))) == 0)
        {
            return;
        }
    }
    else
    {
        return;
    }

    /* Ensure parameters provided are within expected range */
    if (nb_param > TRACE_MAX_PARAM)
        nb_param = TRACE_MAX_PARAM;
    id &= TRACE_MAX_ID;

    block_num = (TRACE_HEADER_BYTES + (nb_param << 1)) / MAX_PAYLOAD_LEN;
    left_bytes = (TRACE_HEADER_BYTES + (nb_param << 1)) % MAX_PAYLOAD_LEN;
    total_bytes = block_num * (LOG_HEADER_BYTES + MAX_PAYLOAD_LEN);
    if(left_bytes > 0)
    {
        total_bytes += (LOG_HEADER_BYTES + left_bytes);
    }

#if (TRACE_ADDR_NO_ALIGN == 0)
    temp_total_bytes = (total_bytes + 3) & 0xFFFFFFFC;
#endif

    /* Needed to be able to trace within IT handler */
    GLOBAL_INT_DISABLE();
    ts = get_sys_local_time_us();

    room_left = free_space_check_hdl();

#if TRACE_ADDR_NO_ALIGN
    if(room_left < total_bytes)
    {
        if(!trace_loop || !free_used_space(total_bytes - room_left))
        {
            trace_env.seqno++;
            goto end;     //drop the current log
        }
    }
#else
    if(room_left < temp_total_bytes)
    {
        trace_env.seqno++;
        goto end;     //drop the current log
    }
#endif
    //reuse total_bytes to payload total length
    //remove sync header
    total_bytes -= LOG_HEADER_BYTES * block_num;
    if(left_bytes > 0)
    {
        total_bytes -= LOG_HEADER_BYTES;
    }

    /*log header 5 bytes*/
    header[0] = TRACE_SYNC_WORD;
    header[1] = trace_env.seqno++;
    length = block_num > 0 ? MAX_PAYLOAD_LEN : left_bytes;
    flag = block_num > 0 ? START_FLAG: COMPLT_FLAG;
    header[2] = length & 0xFF;
    header[3] = ((length >> 8) & 0x03) | (type << 4) | (flag << 2);
#if TRACE_ADDR_NO_ALIGN
    header[4] = 0;         //extra flag
#else
    header[4] = RESV_BYTES_FOR_ALIGN
#endif
    header[5] = header[0] ^ header[1] ^ header[2] ^ header[3] ^ header[4];

#if TRACE_ADDR_NO_ALIGN
    /* write trace header 8 bytes*/
    header[6] = (id >> 16) & 0xFF;
    header[7] = nb_param & 0xFF;
    header[8] = id & 0xFF;
    header[9] = (id >> 8) & 0xFF;
    //MSB timestamp
    header[10] =  (ts >> 16) & 0xFF;
    header[11] = (ts >> 24) & 0xFF;
    //LSB timestamp
    header[12] = ts & 0xFF;
    header[13] = (ts >> 8) & 0xFF;
#else
    //Align Reserv
    header[6] = 0xFF;
    header[7] = 0xFF;

    /* write trace header 8 bytes*/
    header[8] = (id >> 16) & 0xFF;
    header[9] = nb_param & 0xFF;
    header[10] = id & 0xFF;
    header[11] = (id >> 8) & 0xFF;
    //MSB timestamp
    header[12] =  (ts >> 16) & 0xFF;
    header[13] = (ts >> 24) & 0xFF;
    //LSB timestamp
    header[14] = ts & 0xFF;
    header[15] = (ts >> 8) & 0xFF;
#endif

    //reuse left_bytes to first packet left bytes
    left_bytes = MAX_PAYLOAD_LEN - TRACE_HEADER_BYTES;
    total_bytes -= TRACE_HEADER_BYTES;
    trace_buf_write(header, TOTAL_TRACE_HEADER_BYTES);

    //first payload segment
    ptr = (uint8_t *)param;
    if(total_bytes <= left_bytes)
    {
        trace_buf_write(ptr, total_bytes);
#if TRACE_ADDR_NO_ALIGN
        goto end;
#else
        goto align_buffer;
#endif
    }
    else
    {
        trace_buf_write(ptr, left_bytes);
        ptr += left_bytes;
        total_bytes -= left_bytes;
    }

    //continue/end segment
    header[0] = TRACE_SYNC_WORD;
    do {
        header[1] = trace_env.seqno - 1;
        //end segment
        if(total_bytes <= MAX_PAYLOAD_LEN)
        {
            flag = END_FLAG;
            length = total_bytes;
            total_bytes = 0;
        }
        else
        {
            //continue segment
            flag = CONTINUE_FLAG;
            length = MAX_PAYLOAD_LEN;
            total_bytes -= MAX_PAYLOAD_LEN;
        }
        header[2] = length & 0xFF;
        header[3] = ((length >> 8) & 0x03) | (type << 4) | (flag << 2);
#if TRACE_ADDR_NO_ALIGN
        header[4] = 0;         //extra flag
#else
        header[4] = RESV_BYTES_FOR_ALIGN;
#endif

        header[5] = header[0] ^ header[1] ^ header[2] ^ header[3] ^ header[4];
#if (TRACE_ADDR_NO_ALIGN == 0)
        header[6] = 0xFF;
        header[7] = 0xFF;
#endif

        trace_buf_write(header, LOG_HEADER_BYTES);
        trace_buf_write(ptr, length);
        ptr += length;
    }while(total_bytes > 0);

#if (TRACE_ADDR_NO_ALIGN == 0)
align_buffer:
    trace_buf_Align();
#endif

end:
#ifdef CFG_GD_TRACE_DYNAMIC_PRI_SCH
    if(trace_env.task_sleep)
    {
        task_notify_with_isr_check(trace_task_handle);
    }
#endif
    GLOBAL_INT_RESTORE();
}


void trace_ble(uint32_t id, uint16_t nb_param, uint16_t *param, bool trace_buf)
{
    uint8_t *ptr;
    uint64_t ts;
    uint8_t header[TOTAL_TRACE_HEADER_BYTES] = {0};
    uint16_t length;
    uint8_t flag = 0;
    uint32_t room_left;
    uint8_t block_num;
    uint16_t left_bytes;
    uint32_t total_bytes;
#if (TRACE_ADDR_NO_ALIGN == 0)
    uint32_t temp_total_bytes = 0;
#endif

    if(!trace_initialized)
    {
        return;
    }

    /* Ensure parameters provided are within expected range */
    if (nb_param > TRACE_MAX_PARAM)
        nb_param = TRACE_MAX_PARAM;
    id &= TRACE_MAX_ID;

    block_num = (TRACE_HEADER_BYTES + (nb_param << 1)) / MAX_PAYLOAD_LEN;
    left_bytes = (TRACE_HEADER_BYTES + (nb_param << 1)) % MAX_PAYLOAD_LEN;
    total_bytes = block_num * (LOG_HEADER_BYTES + MAX_PAYLOAD_LEN);
    if(left_bytes > 0)
    {
        total_bytes += (LOG_HEADER_BYTES + left_bytes);
    }

#if (TRACE_ADDR_NO_ALIGN == 0)
    temp_total_bytes = (total_bytes + 3) & 0xFFFFFFFC;
#endif

    /* Needed to be able to trace within IT handler */
    GLOBAL_INT_DISABLE();
    ts = get_sys_local_time_us();

    room_left = free_space_check_hdl();

#if TRACE_ADDR_NO_ALIGN
    if(room_left < total_bytes)
    {
        if(!trace_loop || !free_used_space(total_bytes - room_left))
        {
            trace_env.seqno++;
            goto end;     //drop the current log
        }
    }
#else
    if(room_left < temp_total_bytes)
    {
        trace_env.seqno++;
        goto end;     //drop the current log
    }
#endif
    //reuse total_bytes to payload total length
    //remove sync header
    total_bytes -= LOG_HEADER_BYTES * block_num;
    if(left_bytes > 0)
    {
        total_bytes -= LOG_HEADER_BYTES;
    }

    /*log header 5 bytes*/
    header[0] = TRACE_SYNC_WORD;
    header[1] = trace_env.seqno++;
    length = block_num > 0 ? MAX_PAYLOAD_LEN : left_bytes;
    flag = block_num > 0 ? START_FLAG: COMPLT_FLAG;
    header[2] = length & 0xFF;
    header[3] = ((length >> 8) & 0x03) | (TRACE_TYPE_BLE << 4) | (flag << 2);
#if TRACE_ADDR_NO_ALIGN
    header[4] = 0;         //extra flag
#else
    header[4] = RESV_BYTES_FOR_ALIGN
#endif

    header[5] = header[0] ^ header[1] ^ header[2] ^ header[3] ^ header[4];

#if TRACE_ADDR_NO_ALIGN
    /* write trace header 8 bytes*/
    header[6] = (id >> 16) & 0xFF;
    header[7] = nb_param & 0xFF;
    header[8] = id & 0xFF;
    header[9] = (id >> 8) & 0xFF;
    //MSB timestamp
    header[10] =  (ts >> 16) & 0xFF;
    header[11] = (ts >> 24) & 0xFF;
    //LSB timestamp
    header[12] = ts & 0xFF;
    header[13] = (ts >> 8) & 0xFF;
#else
    //Align Reserv
    header[6] = 0xFF;
    header[7] = 0xFF;
    /* write trace header 8 bytes*/
    header[8] = (id >> 16) & 0xFF;
    header[9] = nb_param & 0xFF;
    header[10] = id & 0xFF;
    header[11] = (id >> 8) & 0xFF;
    //MSB timestamp
    header[12] =  (ts >> 16) & 0xFF;
    header[13] = (ts >> 24) & 0xFF;
    //LSB timestamp
    header[14] = ts & 0xFF;
    header[15] = (ts >> 8) & 0xFF;
#endif

    //reuse left_bytes to first packet left bytes
    left_bytes = MAX_PAYLOAD_LEN - TRACE_HEADER_BYTES;
    total_bytes -= TRACE_HEADER_BYTES;
    trace_buf_write(header, TOTAL_TRACE_HEADER_BYTES);

    if (trace_buf)
    {
        uint32_t buf;
        buf = (param[1]) | (param[2] << 16);
        if (param[0] > ((TRACE_MAX_PARAM - 1) * 2))
        {
            //set bit 10 to indicate incomplete buffer
            param[0] = ((TRACE_MAX_PARAM - 1) * 2) + (1 << 10);

        }
        header[0] = param[0];
        header[1] = param[0] >> 8;
        param = _PTR_ALIGN(buf);
        #ifndef CFG_RWTL
        // set bit 9 to indicate unaligned buffer
        header[1] |= ((buf & 0x1) << 1);
        #endif
        total_bytes -= 2;
        left_bytes -= 2;
        trace_buf_write(header, 2);
    }

    //first payload segment
    ptr = (uint8_t *)param;
    if(total_bytes <= left_bytes)
    {
        trace_buf_write(ptr, total_bytes);
#if TRACE_ADDR_NO_ALIGN
        goto end;
#else
        goto align_buffer;
#endif
    }
    else
    {
        trace_buf_write(ptr, left_bytes);
        ptr += left_bytes;
        total_bytes -= left_bytes;
    }

    //continue segment
    header[0] = TRACE_SYNC_WORD;
    do {
        header[1] = trace_env.seqno - 1;
        //end segment
        if(total_bytes <= MAX_PAYLOAD_LEN)
        {
            flag = END_FLAG;
            length = total_bytes;
            total_bytes = 0;
        }
        else
        {
            //continue segment
            flag = CONTINUE_FLAG;
            length = MAX_PAYLOAD_LEN;
            total_bytes -= MAX_PAYLOAD_LEN;
        }
        header[2] = length & 0xFF;
        header[3] = ((length >> 8) & 0x03) | (TRACE_TYPE_BLE << 4) | (flag << 2);
#if TRACE_ADDR_NO_ALIGN
        header[4] = 0;         //extra flag
#else
        header[4] = RESV_BYTES_FOR_ALIGN;
#endif
        header[5] = header[0] ^ header[1] ^ header[2] ^ header[3] ^ header[4];

#if (TRACE_ADDR_NO_ALIGN == 0)
        header[6] = 0xFF;
        header[7] = 0xFF;
#endif

        trace_buf_write(header, LOG_HEADER_BYTES);
        trace_buf_write(ptr, length);
        ptr += length;
    }while(total_bytes > 0);

#if (TRACE_ADDR_NO_ALIGN == 0)
align_buffer:
    trace_buf_Align();
#endif

end:
#ifdef CFG_GD_TRACE_DYNAMIC_PRI_SCH
    if(trace_env.task_sleep)
    {
        task_notify_with_isr_check(trace_task_handle);
    }
#endif
    GLOBAL_INT_RESTORE();
}

void trace_wifi(uint32_t id, uint16_t nb_param, uint16_t *param, bool trace_buf)
{
    uint8_t *ptr;
    uint64_t ts;
    uint8_t header[TOTAL_TRACE_HEADER_BYTES] = {0};
    uint16_t length;
    uint8_t flag = 0;
    uint32_t room_left;
    uint8_t block_num;
    uint16_t left_bytes;
    uint32_t total_bytes;

#if (TRACE_ADDR_NO_ALIGN == 0)
    uint32_t temp_total_bytes = 0;
#endif

    if(!trace_initialized)
    {
        return;
    }

    /* Ensure parameters provided are within expected range */
    if (nb_param > TRACE_MAX_PARAM)
        nb_param = TRACE_MAX_PARAM;
    id &= TRACE_MAX_ID;

    block_num = (TRACE_HEADER_BYTES + (nb_param << 1)) / MAX_PAYLOAD_LEN;
    left_bytes = (TRACE_HEADER_BYTES + (nb_param << 1)) % MAX_PAYLOAD_LEN;
    total_bytes = block_num * (LOG_HEADER_BYTES + MAX_PAYLOAD_LEN);
    if(left_bytes > 0)
    {
        total_bytes += (LOG_HEADER_BYTES + left_bytes);
    }

#if (TRACE_ADDR_NO_ALIGN == 0)
    temp_total_bytes = (total_bytes + 3) & 0xFFFFFFFC;
#endif

    /* Needed to be able to trace within IT handler */
    GLOBAL_INT_DISABLE();
    ts = get_sys_local_time_us();   //need to change

    room_left = free_space_check_hdl();

#if TRACE_ADDR_NO_ALIGN
    if(room_left < total_bytes)
    {
        if(!trace_loop || !free_used_space(total_bytes - room_left))
        {
            trace_env.wifi_seqno++;
            goto end;     //drop the current log
        }
    }
#else
    if(room_left < temp_total_bytes)
    {
        trace_env.wifi_seqno++;
        goto end;     //drop the current log
    }
#endif
    //reuse total_bytes to payload total length
    //remove sync header
    total_bytes -= LOG_HEADER_BYTES * block_num;
    if(left_bytes > 0)
    {
        total_bytes -= LOG_HEADER_BYTES;
    }

    /*log header 5 bytes*/
    header[0] = TRACE_SYNC_WORD;
    header[1] = trace_env.wifi_seqno++;
    length = block_num > 0 ? MAX_PAYLOAD_LEN : left_bytes;
    flag = block_num > 0 ? START_FLAG: COMPLT_FLAG;
    header[2] = length & 0xFF;
    header[3] = ((length >> 8) & 0x03) | (TRACE_TYPE_WIFI << 4) | (flag << 2);
#if TRACE_ADDR_NO_ALIGN
    header[4] = 0;         //extra flag
#else
    header[4] = RESV_BYTES_FOR_ALIGN
#endif

    header[5] = header[0] ^ header[1] ^ header[2] ^ header[3] ^ header[4];

#if TRACE_ADDR_NO_ALIGN
    /* write trace header 8 bytes*/
    header[6] = (id >> 16) & 0xFF;
    header[7] = nb_param & 0xFF;
    header[8] = id & 0xFF;
    header[9] = (id >> 8) & 0xFF;
    //MSB timestamp
    header[10] =  (ts >> 16) & 0xFF;
    header[11] = (ts >> 24) & 0xFF;
    //LSB timestamp
    header[12] = ts & 0xFF;
    header[13] = (ts >> 8) & 0xFF;
#else
    //Align Reserv
    header[6] = 0xFF;
    header[7] = 0xFF;
    /* write trace header 8 bytes*/
    header[8] = (id >> 16) & 0xFF;
    header[9] = nb_param & 0xFF;
    header[10] = id & 0xFF;
    header[11] = (id >> 8) & 0xFF;
    //MSB timestamp
    header[12] =  (ts >> 16) & 0xFF;
    header[13] = (ts >> 24) & 0xFF;
    //LSB timestamp
    header[14] = ts & 0xFF;
    header[15] = (ts >> 8) & 0xFF;
#endif

    //reuse left_bytes to first packet left bytes
    left_bytes = MAX_PAYLOAD_LEN - TRACE_HEADER_BYTES;
    total_bytes -= TRACE_HEADER_BYTES;
    trace_buf_write(header, TOTAL_TRACE_HEADER_BYTES);

    if (trace_buf)
    {
        uint32_t buf;
        buf = (param[1]) | (param[2] << 16);
        if (param[0] > ((TRACE_MAX_PARAM - 1) * 2))
        {
            //set bit 10 to indicate incomplete buffer
            param[0] = ((TRACE_MAX_PARAM - 1) * 2) + (1 << 10);
        }
        header[0] = param[0];
        header[1] = param[0] >> 8;
        param = _PTR_ALIGN(buf);
        #ifndef CFG_RWTL
        // set bit 9 to indicate unaligned buffer
        header[1] |= ((buf & 0x1) << 1);
        #endif
        total_bytes -= 2;
        left_bytes -= 2;
        trace_buf_write(header, 2);
    }

    //first payload segment
    ptr = (uint8_t *)param;
    if(total_bytes <= left_bytes)
    {
        trace_buf_write(ptr, total_bytes);
#if TRACE_ADDR_NO_ALIGN
        goto end;
#else
        goto align_buffer;
#endif
    }
    else
    {
        trace_buf_write(ptr, left_bytes);
        ptr += left_bytes;
        total_bytes -= left_bytes;
    }

    //continue segment
    header[0] = TRACE_SYNC_WORD;
    do {
        header[1] = trace_env.wifi_seqno - 1;
        //end segment
        if(total_bytes <= MAX_PAYLOAD_LEN)
        {
            flag = END_FLAG;
            length = total_bytes;
            total_bytes = 0;
        }
        else
        {
            //continue segment
            flag = CONTINUE_FLAG;
            length = MAX_PAYLOAD_LEN;
            total_bytes -= MAX_PAYLOAD_LEN;
        }
        header[2] = length & 0xFF;
        header[3] = ((length >> 8) & 0x03) | (TRACE_TYPE_BLE << 4) | (flag << 2);
#if TRACE_ADDR_NO_ALIGN
        header[4] = 0;         //extra flag
#else
        header[4] = RESV_BYTES_FOR_ALIGN;
#endif
        header[5] = header[0] ^ header[1] ^ header[2] ^ header[3] ^ header[4];

#if (TRACE_ADDR_NO_ALIGN == 0)
        header[6] = 0xFF;
        header[7] = 0xFF;
#endif

        trace_buf_write(header, LOG_HEADER_BYTES);
        trace_buf_write(ptr, length);
        ptr += length;
    }while(total_bytes > 0);

#if (TRACE_ADDR_NO_ALIGN == 0)
align_buffer:
    trace_buf_Align();
#endif

end:
#ifdef CFG_GD_TRACE_DYNAMIC_PRI_SCH
    if(trace_env.task_sleep)
    {
        task_notify_with_isr_check(trace_task_handle);
    }
#endif
    GLOBAL_INT_RESTORE();
}
#endif

void trace_btsnoop(uint8_t *p_buf, uint16_t len, uint8_t direction, uint8_t hci_type)
{
    uint64_t ts;
    uint8_t header[TOTAL_BTSNOOP_HEADER_BYTES] = {0};
    uint16_t length;
    uint8_t flag = 0;
    uint32_t room_left;
#if (TRACE_ADDR_NO_ALIGN == 0)
    uint32_t temp_total_bytes = 0;
#endif

    uint8_t block_num = (len + BTSNOOP_HEADER_BYTES) / MAX_PAYLOAD_LEN;      //1-hci_type, 4-timestamp, 3-reserv
    uint16_t left_bytes = (len + BTSNOOP_HEADER_BYTES) % MAX_PAYLOAD_LEN;    //1-hci_type, 4-timestamp, 3-reserv
    uint32_t total_bytes = block_num * (LOG_HEADER_BYTES + MAX_PAYLOAD_LEN);

    if(!trace_initialized)
    {
        return;
    }

    if(left_bytes > 0)
    {
        total_bytes += (LOG_HEADER_BYTES + left_bytes);
    }

#if (TRACE_ADDR_NO_ALIGN == 0)
    temp_total_bytes = (total_bytes + 3) & 0xFFFFFFFC;
#endif

    /* Needed to be able to trace within IT handler */
    GLOBAL_INT_DISABLE();
    ts = get_sys_local_time_us();

    room_left = free_space_check_hdl();

#if TRACE_ADDR_NO_ALIGN
    if(room_left < total_bytes)
    {
        if(!trace_loop || !free_used_space(total_bytes - room_left))
        {
            trace_env.btsnoop_seqno++;
            goto end;     //drop the current log
        }
    }
#else
    if(room_left < temp_total_bytes)
    {
        trace_env.btsnoop_seqno++;
        goto end;     //drop the current log
    }
#endif

    //reuse total_bytes to payload total length
    //remove sync header
    total_bytes -= LOG_HEADER_BYTES * block_num;
    if(left_bytes > 0)
    {
        total_bytes -= LOG_HEADER_BYTES;
    }

    header[0] = TRACE_SYNC_WORD;
    header[1] = trace_env.btsnoop_seqno++;
    length = block_num > 0 ? MAX_PAYLOAD_LEN : left_bytes;
    flag = block_num > 0 ? START_FLAG: COMPLT_FLAG;
    header[2] = length & 0xFF;
    header[3] = ((length >> 8) & 0x03) | (TRACE_TYPE_BTSNOOP << 4) | (flag << 2);
#if TRACE_ADDR_NO_ALIGN
    header[4] = 0;         //extra flag
#else
    header[4] = RESV_BYTES_FOR_ALIGN;
#endif
    header[5] = header[0] ^ header[1] ^ header[2] ^ header[3] ^ header[4];

#if TRACE_ADDR_NO_ALIGN
    /* write btsnoop header */
    header[6] = (hci_type & 0x7F) | ((direction & 0x01) << 7);
    header[7] = ts & 0xFF;
    header[8] = (ts >> 8) & 0xFF;
    header[9] = (ts >> 16) & 0xFF;
    header[10] = (ts >> 24) & 0xFF;
    header[11] = 0xFF;
    header[12] = 0xFF;
    header[13] = 0xFF;
#else
    header[6] = 0xFF;
    header[7] = 0xFF;
    /* write btsnoop header */
    header[8] = (hci_type & 0x7F) | ((direction & 0x01) << 7);
    header[9] = ts & 0xFF;
    header[10] = (ts >> 8) & 0xFF;
    header[11] = (ts >> 16) & 0xFF;
    header[12] = (ts >> 24) & 0xFF;
    header[13] = 0xFF;
    header[14] = 0xFF;
    header[15] = 0xFF;
#endif

    //set left_bytes to first packet left bytes
    left_bytes = MAX_PAYLOAD_LEN - BTSNOOP_HEADER_BYTES;
    total_bytes -= BTSNOOP_HEADER_BYTES;
    trace_buf_write(header, TOTAL_BTSNOOP_HEADER_BYTES);

    //first payload segment, it may complete packet or start packet
    if(total_bytes <= left_bytes)
    {
        trace_buf_write(p_buf, total_bytes);
#if TRACE_ADDR_NO_ALIGN
        //complete packet go end
        goto end;
#else
        goto align_buffer;
#endif
    }
    else
    {
        trace_buf_write(p_buf, left_bytes);
        p_buf += left_bytes;
        total_bytes -= left_bytes;
    }

    //continue/end segment
    header[0] = TRACE_SYNC_WORD;
    do {
        header[1] = trace_env.btsnoop_seqno - 1;
        //end segment
        if(total_bytes <= MAX_PAYLOAD_LEN)
        {
            flag = END_FLAG;
            length = total_bytes;
            total_bytes = 0;
        }
        else
        {
            //continue segment
            flag = CONTINUE_FLAG;
            length = MAX_PAYLOAD_LEN;
            total_bytes -= MAX_PAYLOAD_LEN;
        }
        header[2] = length & 0xFF;
        header[3] = ((length >> 8) & 0x03) | (TRACE_TYPE_BTSNOOP << 4) | (flag << 2);
#if TRACE_ADDR_NO_ALIGN
        header[4] = 0;         //extra flag
#else
        header[4] = RESV_BYTES_FOR_ALIGN;
#endif
        header[5] = header[0] ^ header[1] ^ header[2] ^ header[3] ^ header[4];
#if (TRACE_ADDR_NO_ALIGN == 0)
        header[6] = 0xFF;
        header[7] = 0xFF;
#endif

        trace_buf_write(header, LOG_HEADER_BYTES);
        trace_buf_write(p_buf, length);
        p_buf += length;
    }while(total_bytes > 0);

#if (TRACE_ADDR_NO_ALIGN == 0)
align_buffer:
    trace_buf_Align();
#endif

end:
#ifdef CFG_GD_TRACE_DYNAMIC_PRI_SCH
    if(trace_env.task_sleep)
    {
        task_notify_with_isr_check(trace_task_handle);
    }
#endif
    GLOBAL_INT_RESTORE();
}

void trace_btsnoop_payload(uint8_t *p_buf, uint16_t len, uint8_t direction, uint8_t hci_type, uint8_t *p_payload, uint16_t payload_len)
{
    uint64_t ts;
    uint8_t header[TOTAL_BTSNOOP_HEADER_BYTES] = {0};
    uint16_t length;
    uint8_t flag = 0;
    uint32_t room_left;
#if (TRACE_ADDR_NO_ALIGN == 0)
    uint32_t temp_total_bytes = 0;
#endif

    uint8_t block_num = (len + payload_len + BTSNOOP_HEADER_BYTES) / MAX_PAYLOAD_LEN;    //1-hci_type, 4-timestamp, 3-reserv
    uint16_t left_bytes = (len + payload_len + BTSNOOP_HEADER_BYTES) % MAX_PAYLOAD_LEN;  //1-hci_type, 4-timestamp, 3-reserv
    uint32_t total_bytes = block_num * (LOG_HEADER_BYTES + MAX_PAYLOAD_LEN);

    if(!trace_initialized)
    {
        return;
    }

    if(left_bytes > 0)
    {
        total_bytes += (LOG_HEADER_BYTES + left_bytes);
    }

#if (TRACE_ADDR_NO_ALIGN == 0)
    temp_total_bytes = (total_bytes + 3) & 0xFFFFFFFC;
#endif

    /* Needed to be able to trace within IT handler */
    GLOBAL_INT_DISABLE();
    ts = get_sys_local_time_us();

    room_left = free_space_check_hdl();

#if TRACE_ADDR_NO_ALIGN
    if(room_left < total_bytes)
    {
        if(!trace_loop || !free_used_space(total_bytes - room_left))
        {
            trace_env.btsnoop_seqno++;
            goto end;     //drop the current log
        }
    }
#else
    if(room_left < temp_total_bytes)
    {
        trace_env.btsnoop_seqno++;
        goto end;     //drop the current log
    }
#endif
    //reuse total_bytes to payload total length
    //remove sync header
    total_bytes -= LOG_HEADER_BYTES * block_num;
    if(left_bytes > 0)
    {
        total_bytes -= LOG_HEADER_BYTES;
    }

    header[0] = TRACE_SYNC_WORD;
    header[1] = trace_env.btsnoop_seqno++;
    length = block_num > 0 ? MAX_PAYLOAD_LEN : left_bytes;
    flag = block_num > 0 ? START_FLAG: COMPLT_FLAG;
    header[2] = length & 0xFF;
    header[3] = ((length >> 8) & 0x03) | (TRACE_TYPE_BTSNOOP << 4) | (flag << 2);
#if TRACE_ADDR_NO_ALIGN
    header[4] = 0;         //extra flag
#else
    header[4] = RESV_BYTES_FOR_ALIGN;
#endif
    header[5] = header[0] ^ header[1] ^ header[2] ^ header[3] ^ header[4];

#if TRACE_ADDR_NO_ALIGN
    /* write btsnoop header */
    header[6] = (hci_type & 0x7F) | ((direction & 0x01) << 7);
    header[7] = ts & 0xFF;
    header[8] = (ts >> 8) & 0xFF;
    header[9] = (ts >> 16) & 0xFF;
    header[10] = (ts >> 24) & 0xFF;
    header[11] = 0xFF;
    header[12] = 0xFF;
    header[13] = 0xFF;
#else
    //Align Reserv
    header[6] = 0xFF;
    header[7] = 0xFF;
    /* write btsnoop header */
    header[8] = (hci_type & 0x7F) | ((direction & 0x01) << 7);
    header[9] = ts & 0xFF;
    header[10] = (ts >> 8) & 0xFF;
    header[11] = (ts >> 16) & 0xFF;
    header[12] = (ts >> 24) & 0xFF;
    header[13] = 0xFF;
    header[14] = 0xFF;
    header[15] = 0xFF;
#endif

    //set left_bytes to first packet left bytes
    left_bytes = MAX_PAYLOAD_LEN - BTSNOOP_HEADER_BYTES;
    total_bytes -= BTSNOOP_HEADER_BYTES;
    trace_buf_write(header, TOTAL_BTSNOOP_HEADER_BYTES);

    //first payload segment, it may complete packet or start packet
    if(total_bytes <= left_bytes)
    {
        trace_buf_write(p_buf, len);
        if(p_payload != NULL)
        {
            trace_buf_write(p_payload, payload_len);
        }
#if TRACE_ADDR_NO_ALIGN
        //complete packet go end
        goto end;
#else
        goto align_buffer;
#endif
    }
    else
    {
        if(len > left_bytes)
        {
            trace_buf_write(p_buf, left_bytes);
            len -= left_bytes;
            p_buf += left_bytes;
        }
        else
        {
            trace_buf_write(p_buf, len);
            trace_buf_write(p_payload, left_bytes- len);
            payload_len -= (left_bytes - len);
            p_payload += (left_bytes - len);
            len = 0;
        }
        total_bytes -= left_bytes;
    }

    //continue/end segment
    header[0] = TRACE_SYNC_WORD;
    do {
        header[1] = trace_env.btsnoop_seqno - 1;
        //end segment
        if(total_bytes <= MAX_PAYLOAD_LEN)
        {
            flag = END_FLAG;
            length = total_bytes;
            total_bytes = 0;
        }
        else
        {
            //continue segment
            flag = CONTINUE_FLAG;
            length = MAX_PAYLOAD_LEN;
            total_bytes -= MAX_PAYLOAD_LEN;
        }
        header[2] = length & 0xFF;
        header[3] = ((length >> 8) & 0x03) | (TRACE_TYPE_BTSNOOP << 4) | (flag << 2);
#if TRACE_ADDR_NO_ALIGN
        header[4] = 0;         //extra flag
#else
        header[4] = RESV_BYTES_FOR_ALIGN;
#endif
        header[5] = header[0] ^ header[1] ^ header[2] ^ header[3] ^ header[4];
#if (TRACE_ADDR_NO_ALIGN == 0)
        header[6] = 0xFF;
        header[7] = 0xFF;
#endif

        trace_buf_write(header, LOG_HEADER_BYTES);
        if(total_bytes == 0)
        {
            if(len > 0)
            {
                trace_buf_write(p_buf, len);
            }
            if(payload_len > 0)
            {
                trace_buf_write(p_payload, payload_len);
            }
        }
        else
        {
            if(len > 0)
            {
                if(len > length)
                {
                    trace_buf_write(p_buf, length);
                    p_buf += length;
                    len -= length;
                    length = 0;
                }
                else
                {
                    trace_buf_write(p_buf, len);
                    len = 0;
                    length -= len;
                }
            }

            if(length > 0 && payload_len > 0)
            {
                if(payload_len > length)
                {
                    trace_buf_write(p_payload, length);
                    payload_len -= length;
                    p_payload += length;
                }
                else
                {
                    trace_buf_write(p_payload, payload_len);
                    payload_len = 0;
                }
            }
        }
    }while(total_bytes > 0);

#if (TRACE_ADDR_NO_ALIGN == 0)
align_buffer:
    trace_buf_Align();
#endif

end:
#ifdef CFG_GD_TRACE_DYNAMIC_PRI_SCH
    if(trace_env.task_sleep)
    {
        task_notify_with_isr_check(trace_task_handle);
    }
#endif
    GLOBAL_INT_RESTORE();
}

uint16_t trace_count()
{
    return trace_initialized ? (trace_env.trace_end + TRACE_SIZE_MAX - trace_env.trace_start) % TRACE_SIZE_MAX : 0;
}

uint16_t trace_print(uint16_t max_bytes)
{
    uint16_t send_bytes = 0;

    if(!trace_initialized || trace_count() == 0)
    {
        return send_bytes;
    }

    send_bytes = (trace_env.trace_end + TRACE_SIZE_MAX - trace_env.trace_start) % TRACE_SIZE_MAX;

    if(send_bytes > max_bytes)
    {
        send_bytes = max_bytes;
    }

    if(send_bytes > 0)
    {
        if (trace_env.trace_start + send_bytes <= TRACE_SIZE_MAX)
        {
            uart_transfer_trace_data(trace_start + trace_env.trace_start, send_bytes);
            trace_env.trace_start += send_bytes;
        }
        else
        {
            uint16_t tlen = TRACE_SIZE_MAX - trace_env.trace_start;

            uart_transfer_trace_data(trace_start + trace_env.trace_start, tlen);
            uart_transfer_trace_data(trace_start, send_bytes - tlen);
            trace_env.trace_start = send_bytes - tlen;
        }

        trace_env.trace_start %= TRACE_SIZE_MAX;
    }
    return send_bytes;
}

void trace_dma_print()
{
#ifdef TRACE_UART_DMA
    uint16_t send_bytes = 0;
    if(!trace_initialized || trace_count() == 0)
    {
        return;
    }

    // DMA is not sending bytes, arrange dma transfer
    if(trace_env.dma_send_bytes == 0)
    {
        send_bytes = (trace_env.trace_end + TRACE_SIZE_MAX - trace_env.trace_start) % TRACE_SIZE_MAX;
        if(send_bytes > 0)
        {
            if (trace_env.trace_start + send_bytes <= TRACE_SIZE_MAX)
            {
                trace_env.dma_send_bytes = send_bytes;
            }
            else
            {
                trace_env.dma_send_bytes = TRACE_SIZE_MAX - trace_env.trace_start;
            }

            trace_uart_dma_transfer((uint32_t)(trace_start + trace_env.trace_start), trace_env.dma_send_bytes);
        }
    }

#endif
}

#ifdef TRACE_UART_DMA
void trace_dma_transfer_cmplt()
{
    uint16_t send_bytes = 0;

    if(!trace_initialized)
    {
        return;
    }

    GLOBAL_INT_DISABLE();
    trace_env.trace_start = (trace_env.trace_start + trace_env.dma_send_bytes) % TRACE_SIZE_MAX;

    // Check left bytes need to transfer
    send_bytes = (trace_env.trace_end + TRACE_SIZE_MAX - trace_env.trace_start) % TRACE_SIZE_MAX;
    if(send_bytes > 0)
    {
        if (trace_env.trace_start + send_bytes <= TRACE_SIZE_MAX)
        {
            trace_env.dma_send_bytes = send_bytes;
        }
        else
        {
            trace_env.dma_send_bytes = TRACE_SIZE_MAX - trace_env.trace_start;
        }

        trace_uart_dma_transfer((uint32_t)(trace_start + trace_env.trace_start), trace_env.dma_send_bytes);
    }
    else
        trace_env.dma_send_bytes = 0;

    GLOBAL_INT_RESTORE();
}
#endif

#else /* CFG_GD_TRACE_EXT */
void trace_ext_init(bool force, bool loop)
{
    return;
}

void trace_console(uint16_t len, uint8_t *p_buf)
{

}

uint16_t trace_count()
{
    return 0;
}

uint16_t trace_print(uint16_t max_bytes)
{
    return 0;
}

void trace_dma_print()
{

}

void trace_btsnoop(uint8_t *p_buf, uint16_t len, uint8_t direction, uint8_t hci_type)
{

}

void trace_btsnoop_payload(uint8_t *p_buf, uint16_t len, uint8_t direction, uint8_t hci_type, uint8_t *p_payload, uint16_t payload_len)
{

}

#endif /* CFG_GD_TRACE_EXT */
