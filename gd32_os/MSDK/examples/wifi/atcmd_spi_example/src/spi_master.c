/*!
    \file    spi_master.c
    \brief   implementation of spi master to issue the AT command

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

#include "version.h"
#include "app_cfg.h"
#include "wlan_config.h"
#include "build_config.h"
#include "gd32vw55x.h"
#include "dbg_print.h"
#include "gd32vw55x_dma.h"
#include "wrapper_os.h"

#include "spi.h"
#include "spi_master.h"


#if (SPI_ROLE == SPI_ROLE_MASTER)
#define AT_NULL_SYMBOL                      0x16
#define AT_PADDING_SYMBOL                   '\0'
#define AT_TRX_TIMEOUT                      30000

#define SPI_IRQ_RX_PRESCALE                 SPI_PSC_16  //160M/256

#define SPI_MIN_SEGMENT_LEN                 1460

#define HANDSHAKE_GPIO                      GPIOA
#define HANDSHAKE_PIN                       GPIO_PIN_12

#define SPI_NSS_GPIO                        GPIOA
#define SPI_NSS_PIN                         GPIO_PIN_4

#define CRC_CHECK_ACK                       "ACK"
#define CRC_CHECK_NAK                       "NAK"

#define SPI_RECVDATA_PREFIX                 "+CIPRECVDATA:"

extern os_sema_t spi_slave_ready_sema;
extern os_mutex_t spi_master_send_mutex;

#ifndef MIN
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#endif

#ifndef CONFIG_SPI_3_WIRED
#define SET_AT_SPI_NSS_HIGH()   do {                                    \
                                    gpio_bit_set(GPIOA, GPIO_PIN_4);    \
                                    spi_disable();                      \
                                } while(0)

#define SET_AT_SPI_NSS_LOW()    do {                                    \
                                    gpio_bit_reset(GPIOA, GPIO_PIN_4);  \
                                    spi_enable();                       \
                                } while(0)
#else
#define SET_AT_SPI_NSS_HIGH()
#define SET_AT_SPI_NSS_LOW()
#endif


#define AT_TRACE_DATA(title, data, len)                         \
    do {                                                        \
        int i;                                                  \
        app_print("======== %s: %u ========", title, len);       \
        for (i = 0; i < len; i++) {                             \
            if (i % 16 == 0) {                                  \
                app_print("\r\n\t");                             \
            }                                                   \
            app_print("%02x ", *(data + i));                     \
        }                                                       \
        app_print("\r\n");                                       \
    } while(0)

/*===========================================*/

int spi_hw_is_idle(void)
{
    return (spi_manager.stat == SPI_Master_Idle);
}

int spi_hw_is_in_atack(void)
{
    return ((spi_manager.stat == SPI_Master_AT_ACK) ||
                (spi_manager.stat == SPI_Master_Data_ACK) ||
                (spi_manager.stat == SPI_Master_Data_Sent));
}

int spi_manager_stat_get(void)
{
    return spi_manager.stat;
}

void wait_handshake_pin_idle(void)
{
    while (gpio_input_bit_get(HANDSHAKE_GPIO, HANDSHAKE_PIN) == SET);
    return;
}

static int32_t spi_hw_lock(void)
{
    return sys_mutex_try_get(&spi_master_send_mutex, -1);
}

static void spi_hw_unlock(void)
{
    return sys_mutex_put(&spi_master_send_mutex);
}

/*!
    \brief      Initialize SPI Master, including PIN, SPI init.
    \param[in]  none
    \param[out] none
    \retval     none
*/
static int spi_master_init(void)
{
    spi_parameter_struct spi_init_struct;

    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_SPI);

#ifndef CONFIG_SPI_3_WIRED
    /* SPI GPIO config:MOSI/PA0, MISO/PA1, SCK/PA2*/
    gpio_af_set(SPI_SCK_GPIO, SPI_AF_NUM, SPI_MOSI_PIN |SPI_MISO_PIN |SPI_SCK_PIN);
    gpio_mode_set(SPI_SCK_GPIO, GPIO_MODE_AF, GPIO_PUPD_NONE, SPI_MOSI_PIN |SPI_MISO_PIN |SPI_SCK_PIN);
    gpio_output_options_set(SPI_SCK_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, SPI_MOSI_PIN |SPI_MISO_PIN |SPI_SCK_PIN);

    /* PA4 as NSS */
    gpio_mode_set(SPI_NSS_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SPI_NSS_PIN);
    gpio_output_options_set(SPI_NSS_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, SPI_NSS_PIN);
#else
    /* SPI GPIO config:MOSI/PA0, MISO/PA1, SCK/PA2 */
    gpio_af_set(SPI_SCK_GPIO, SPI_AF_NUM, SPI_MOSI_PIN |SPI_MISO_PIN |SPI_SCK_PIN);
    gpio_mode_set(SPI_SCK_GPIO, GPIO_MODE_AF, GPIO_PUPD_NONE, SPI_MOSI_PIN |SPI_MISO_PIN |SPI_SCK_PIN);
    gpio_output_options_set(SPI_SCK_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, SPI_MOSI_PIN |SPI_MISO_PIN |SPI_SCK_PIN);
#endif

    spi_deinit();
    spi_struct_para_init(&spi_init_struct);
    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode          = SPI_MASTER;
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
    spi_init_struct.prescale             = SPI_IRQ_RX_PRESCALE;//prescale;
    spi_init_struct.endian               = SPI_ENDIAN_MSB;
    spi_init_struct.nss                  = SPI_NSS_SOFT;

    spi_nss_internal_high();

#ifdef CONFIG_SPI_3_WIRED
    spi_nss_internal_high();
#endif

    spi_init(&spi_init_struct);

    return 0;
}

/*!
    \brief      Configure SPI Master
                Re-initialize SPI struct and configure DMA according to input parameters
    \param[in]  dma_rx  0: irq rx, 1: dma rx
    \param[in]  dma_tx  0: cpu tx, 1: dma tx
    \param[out] none
    \retval     none
*/
void spi_master_trx_dma_init(void)
{
    dma_single_data_parameter_struct dma_init_struct;
    dma_single_data_para_struct_init(&dma_init_struct);

    rcu_periph_clock_enable(RCU_DMA);

    /* configure SPI transmit dma */
    dma_deinit(SPI_TX_DMA_CH);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA;
    dma_init_struct.memory0_addr        = 0;
    dma_init_struct.direction           = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority            = DMA_PRIORITY_LOW;
    dma_init_struct.number              = 0;
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_DISABLE;
    dma_single_data_mode_init(SPI_TX_DMA_CH, &dma_init_struct);
    dma_channel_subperipheral_select(SPI_TX_DMA_CH, DMA_SUBPERI3);

    // dma_interrupt_disable(SPI_TX_DMA_CH, DMA_INT_FTF);

    /* configure SPI receive dma */
    dma_deinit(SPI_RX_DMA_CH);
    dma_init_struct.periph_addr  = (uint32_t)&SPI_DATA;
    dma_init_struct.memory0_addr = 0;
    dma_init_struct.direction    = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.priority     = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(SPI_RX_DMA_CH, &dma_init_struct);
    dma_channel_subperipheral_select(SPI_RX_DMA_CH, DMA_SUBPERI3);
}

/*!
    \brief      Configure HANDSHAKE GPIO PIN used when SPI Slave tx data to Master
                The GPIO is used as an INPUT EXTI PIN.
    \param[in]  none
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void spi_master_handshake_pin_config(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_SYSCFG);

    gpio_mode_set(HANDSHAKE_GPIO, GPIO_MODE_INPUT, GPIO_PUPD_NONE, HANDSHAKE_PIN);
    eclic_irq_enable(EXTI10_15_IRQn, 9, 0);
    syscfg_exti_line_config(EXTI_SOURCE_GPIOA, (uint8_t) HANDSHAKE_PIN);
    exti_init(EXTI_12, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    exti_interrupt_flag_clear(EXTI_12);

#ifdef CONFIG_SPI_3_WIRED
    gpio_mode_set(SPI_NSS_GPIO, GPIO_MODE_INPUT, GPIO_PUPD_NONE, SPI_NSS_PIN);
    eclic_irq_enable(EXTI4_IRQn, 9, 0);
    syscfg_exti_line_config(EXTI_SOURCE_GPIOA, (uint8_t) SPI_NSS_PIN);
    exti_init(EXTI_12, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    exti_interrupt_flag_clear(EXTI_4);
#endif
}

/*!
    \brief      Initialize SPI Master, including PIN, SPI init and HANDSHAKE GPIO PIN.
    \param[in]  none
    \param[out] none
    \retval     none
*/
void spi_master_demo_init(void)
{
    spi_master_handshake_pin_config();

    spi_master_init();
    spi_master_trx_dma_init();

    SET_AT_SPI_NSS_HIGH();

    app_print("SPI Master Initialized\r\n");
}

/*!
    \brief      SPI Master send data to Slave using DMA CH3.
    \param[in]  data data to be sent
    \param[in]  data_len data length to be sent
    \param[out] none
    \retval     0 by default
*/
static int spi_master_dma_send(uint8_t *data, uint32_t data_len)
{
    uint32_t recv_null_pointer = 0;

    if (data == NULL || data_len == 0)
        return -1;

    SET_AT_SPI_NSS_HIGH();

// tx dma config
    dma_memory_address_config(SPI_TX_DMA_CH, DMA_MEMORY_0, (uint32_t)data);
    dma_transfer_number_config(SPI_TX_DMA_CH, data_len);
    dma_memory_address_generation_config(SPI_TX_DMA_CH, DMA_MEMORY_INCREASE_ENABLE);

// rx dma config
    dma_memory_address_config(SPI_RX_DMA_CH, DMA_MEMORY_0, (uint32_t)&recv_null_pointer);
    dma_transfer_number_config(SPI_RX_DMA_CH, data_len);
    dma_memory_address_generation_config(SPI_RX_DMA_CH, DMA_MEMORY_INCREASE_DISABLE);

    dma_channel_enable(SPI_RX_DMA_CH);
    dma_channel_enable(SPI_TX_DMA_CH);
    SET_AT_SPI_NSS_LOW();

    spi_dma_enable(SPI_DMA_TRANSMIT);
    spi_dma_enable(SPI_DMA_RECEIVE);

    while (!dma_flag_get(SPI_TX_DMA_CH, DMA_INTF_FTFIF));
    dma_flag_clear(SPI_TX_DMA_CH, DMA_INTF_FTFIF);

    while (!dma_flag_get(SPI_RX_DMA_CH, DMA_INTF_FTFIF));
    dma_flag_clear(SPI_RX_DMA_CH, DMA_INTF_FTFIF);

    while (RESET == spi_flag_get(SPI_FLAG_TBE));

    spi_dma_disable(SPI_DMA_TRANSMIT);
    spi_dma_disable(SPI_DMA_RECEIVE);
    dma_channel_disable(SPI_TX_DMA_CH);
    dma_channel_disable(SPI_RX_DMA_CH);

    return 0;
}

void spi_master_put_data(uint8_t *d, int size, uint8_t is_cmd)
{
    SET_AT_SPI_NSS_LOW();
    uint8_t cmd[ATCMD_FIXED_LEN] = {0};

    if (is_cmd) {
        sys_memcpy(cmd, d, size);
        spi_master_dma_send(cmd, ATCMD_FIXED_LEN);
    } else {
        spi_master_dma_send(d, size);
    }

    while (RESET == spi_flag_get(SPI_FLAG_TBE));
}

/*!
    \brief      SPI Master recv data from Slave using DMA CH2.
    \param[in]  data data to be store
    \param[in]  data_len data length to be recieved
    \param[out] none
    \retval     0 by default
*/
static int spi_master_dma_recv(uint8_t *data, uint32_t data_len)
{
    char null_data = '\0';

    if (data == NULL || data_len == 0)
        return -1;

    SET_AT_SPI_NSS_HIGH();

// tx dma config
    dma_memory_address_config(SPI_TX_DMA_CH, DMA_MEMORY_0, (uint32_t)&null_data);
    dma_transfer_number_config(SPI_TX_DMA_CH, data_len);
    dma_memory_address_generation_config(SPI_TX_DMA_CH, DMA_MEMORY_INCREASE_DISABLE);

// rx dma config
    dma_memory_address_config(SPI_RX_DMA_CH, DMA_MEMORY_0, (uint32_t)data);
    dma_transfer_number_config(SPI_RX_DMA_CH, data_len);
    dma_memory_address_generation_config(SPI_RX_DMA_CH, DMA_MEMORY_INCREASE_ENABLE);

    dma_channel_enable(SPI_RX_DMA_CH);
    dma_channel_enable(SPI_TX_DMA_CH);
    SET_AT_SPI_NSS_LOW();

    spi_dma_enable(SPI_DMA_RECEIVE);
    spi_dma_enable(SPI_DMA_TRANSMIT);

    while (!dma_flag_get(SPI_TX_DMA_CH, DMA_INTF_FTFIF));
    dma_flag_clear(SPI_TX_DMA_CH, DMA_INTF_FTFIF);

    while (!dma_flag_get(SPI_RX_DMA_CH, DMA_INTF_FTFIF));
    dma_flag_clear(SPI_RX_DMA_CH, DMA_INTF_FTFIF);

    while (RESET == spi_flag_get(SPI_FLAG_TBE));

    spi_dma_disable(SPI_DMA_TRANSMIT);
    spi_dma_disable(SPI_DMA_RECEIVE);
    dma_channel_disable(SPI_TX_DMA_CH);
    dma_channel_disable(SPI_RX_DMA_CH);

    return 0;
}

static int spi_master_dma_recv_len(void)
{
    char lenstr[50] = {0};
    char *p = NULL;
    int cnt = 0, ret = -1;

    spi_master_dma_recv((uint8_t *)lenstr, 5);

    /* 5. rcv rsp header, 4 bytes */
    p = &lenstr[0];
    do {
        if (*p == ',') {
            *p = '\0';
            break;
        } else if (*p < '0' || *p > '9') {
            app_print("rx len error, %s\r\n", lenstr);
            goto error;
        }

        p++;
        cnt++;
        if (cnt > 5) {
            app_print("rx len cnt error, %s\r\n", lenstr);
            goto error;
        }
    } while (1);

    ret = atoi(lenstr);
    return ret;

error:
    AT_TRACE_DATA("Error rx 1", lenstr, 6);
    spi_master_dma_recv((uint8_t *)lenstr, 50);
    AT_TRACE_DATA("Error rx", lenstr, 50);
    return ret;
}

/*!
    \brief      SPI Master send data to Slave in normal mode(CIPMODE = 0).
                "SEND OK\r\n" or "ERROR\r\n" is responsed by SPI Slave.
    \param[in]  data      data to be sent
    \param[in]  data_len  data length to be sent
    \param[out]  rsp      data response by SPI Slave
    \param[in]  rsp       max rsp length
    \retval     0 if send ok, negtive value if send fail
*/
int at_spi_send_data_wait_rsp(uint8_t *data, int data_len, char *rsp, int rsp_len)
{
    int cnt = 0, len = 0, ret = 0;

    if (!data || data_len == 0)
        return -1;

#ifdef SPI_MASTER_DEBUG_PRINT
    app_print("send data---start\r\n");
#endif

    spi_manager.stat = SPI_Master_Data_Sent;

    /* 0. Aquire SPI HW lock */
    spi_hw_lock();
    /* 1. config spi master using polling */
    wait_handshake_pin_idle();
    SET_AT_SPI_NSS_LOW();

    /* 2. send data over spi */
    // wait spi is ready to recieve data
    if (sys_sema_down(&spi_slave_ready_sema, AT_TRX_TIMEOUT) == OS_TIMEOUT) { // wait 30s
        app_print("data ack rcv timeout\r\n");
        ret = -2;
        goto exit;
    }
//    spi_manager.stat = SPI_Master_Data_ACK;
sys_enter_critical();
    spi_master_put_data(data, data_len, 0);
    spi_manager.stat = SPI_Master_Data_ACK;
sys_exit_critical();


    /* 3. wait data ack is ready */
    if (sys_sema_down(&spi_slave_ready_sema, AT_TRX_TIMEOUT) == OS_TIMEOUT) { // wait 30s
        app_print("data ack rcv timeout\r\n");
        ret = -3;
        goto exit;
    }

    /* 4. Receive lenght prefix */
    len = spi_master_dma_recv_len();
    if (len > rsp_len || len == -1) {
        app_print("send data wait rsp len error\r\n");
        ret = -4;
        goto exit;
    }

    /* 5. rcv rsp */
    sys_memset(rsp, 0, rsp_len);
    spi_master_dma_recv((uint8_t *)rsp, len);

    /* 6. parse response */
#ifdef SPI_MASTER_DEBUG_PRINT
    app_print("data rsp=%s\r\n", rsp);
#endif
    if (strstr(rsp, "OK")) {
        ret = 0;
    } else {
        app_print("error data rsp=%s\r\n", rsp);
        ret = -5;
    }

exit:
    spi_hw_unlock();
    SET_AT_SPI_NSS_HIGH();
    return ret;
}

/*!
    \brief      SPI Master send ATCMD to Slave
                "OK\r\n" or "ERROR\r\n" is responsed by SPI Slave.
    \param[in]  cmd       ATCMD to be sent
    \param[in]  cmd_len   ATCMD length to be sent
    \param[out] rsp       ATCMD response by SPI Slave
    \param[in]  rsp_len   max rsp length
    \retval     0 if send ok, negtive value if send fail
*/
int at_spi_send_cmd_wait_rsp(char *cmd, int cmd_len, char *rsp, int rsp_len)
{
    char lenstr[6] = {0};
    int cnt = 0, len = 0, ret = 0;

    if (!cmd || cmd_len == 0)
        return -1;

#ifdef SPI_MASTER_DEBUG_PRINT
    app_print("---start send cmd %s\r\n", cmd);
#endif

    /* 1. update SPI manager state */
    spi_manager.stat = SPI_Master_AT_Sent;

    /* 2. get spi bus mutex */
    spi_hw_lock();


    /* 3. enable NSS */
    wait_handshake_pin_idle();
    SET_AT_SPI_NSS_LOW();
//    spi_manager.stat = SPI_Master_AT_ACK;
sys_enter_critical();

    /* 4. send ATCMD over spi using padding */
    spi_master_put_data((uint8_t *)cmd, cmd_len, 1);

#ifdef SPI_MASTER_DEBUG_PRINT
    app_print("send cmd %s over---\r\n", cmd);
#endif
    spi_manager.stat = SPI_Master_AT_ACK;
sys_exit_critical();

    /* 5. waiting handshake gpio pulls high as ATCMD ACK is ready */
    if (sys_sema_down(&spi_slave_ready_sema, AT_TRX_TIMEOUT) == OS_TIMEOUT) { // wait 30s
        app_print("at response rcv timeout\r\n");
        ret = -2;
        goto exit;
    }

    /* 6. rcv rsp length prefix */
    len = spi_master_dma_recv_len();

    if (len > rsp_len || len == -1) {
        app_print("atcmd=%s cmd lenstr=%s, cmdrsp_len=%d\r\n", cmd, lenstr, len);
        ret = -3;
        goto exit;
    }

    /* 5. rcv rsp */
    sys_memset(rsp, 0, rsp_len);
    spi_master_dma_recv((uint8_t *)rsp, len);

#ifdef SPI_MASTER_DEBUG_PRINT
    app_print("atcmd rsp=%s\r\n", rsp);
#endif

    /* 9. parse response */
    if (strstr(rsp, "OK") || strstr(rsp, ">")) {
        ret = 0;
    } else if (strstr(rsp, "ERROR")) {
        ret = -4;
    } else {
        app_print("error rsp:%s\r\n", rsp);
        ret = -5;
    }

#ifdef SPI_MASTER_DEBUG_PRINT
    app_print("rcv cmd %s rsp %s---\r\n", cmd, rsp);
#endif

exit:
    spi_hw_unlock();
    if (strstr(cmd, "AT+CIPSDFILE")) {
        return ret;
    }
    SET_AT_SPI_NSS_HIGH();
    return ret;
}

/*!
    \brief      SPI Master send ATCMD to read data from tcp server
                "OK\r\n" or "ERROR\r\n" is responsed by SPI Slave.
    \param[in]  cmd       ATCMD to be sent
    \param[in]  cmd_len   ATCMD length to be sent
    \param[out] rsp       ATCMD response by SPI Slave
    \param[in]  rsp_len   max rsp length
    \retval     0 if send ok, negtive value if send fail
*/
int at_spi_send_cmd_read_data(char *cmd, int cmd_len, at_cmd_recv_info_t *recv_info)
{
    int len = 0, ret = 0;

    if (!cmd || cmd_len == 0)
        return -1;

#ifdef SPI_MASTER_DEBUG_PRINT
    //app_print("---start send cmd to read data %s\r\n", cmd);
#endif

    /* 1. Update SPI manager stat */
    spi_manager.stat = SPI_Master_AT_Sent;

    /* 2. get spi bus mutex */
    spi_hw_lock();

    /* 3. config spi master using polling */
    wait_handshake_pin_idle();
    SET_AT_SPI_NSS_LOW();


sys_enter_critical();
    /* 4. send ATCMD over spi using padding */
    spi_master_put_data((uint8_t *)cmd, cmd_len, 1);
#ifdef SPI_MASTER_DEBUG_PRINT
    //app_print("send cmd %s over---\r\n", cmd);
#endif
    spi_manager.stat = SPI_Master_AT_ACK;
sys_exit_critical();

    /* 5. waiting handshake gpio pulls high as ATCMD ACK is ready */
    if (sys_sema_down(&spi_slave_ready_sema, AT_TRX_TIMEOUT) == OS_TIMEOUT) { // wait 30s
        app_print("at response rcv timeout----\r\n");
        ret = -2;
        goto exit;
    }

    len = spi_master_dma_recv_len();
    if (len > 8205 || len == -1) {
        app_print("read data atcmd=%s\r\n", cmd);
        ret = -3;
        goto exit;
    }

    if (recv_info->rx_buffer == NULL)
        recv_info->rx_buffer = sys_malloc(len);
    if (recv_info->rx_buffer == NULL) {
        app_print("recv info payload malloc failed\r\n");
        ret = -4;
        goto exit;
    }

    recv_info->rx_buffer_len = len;

    /* 5. rcv data */
    spi_master_dma_recv((uint8_t *)recv_info->rx_buffer, len);

#ifdef SPI_MASTER_DEBUG_PRINT
    app_print("recv tcp data=%s\r\n", (char *)recv_info->rx_buffer);
#endif

    ret = 0;

#ifdef SPI_MASTER_DEBUG_PRINT
    app_print("read data OK---\r\n");
#endif

exit:
    spi_hw_unlock();
    SET_AT_SPI_NSS_HIGH();
    return ret;
}

/*!
    \brief      SPI Master send large file to Slave using DMA CH3.
                "ACK" or "NAK" is responsed by SPI Slave.
    \param[in]  data        data to be sent
    \param[in]  data_len    data length to be sent
    \param[in]  segment_len segment length to be split to transmit to the network
    \param[out] rsp         response by SPI Slave
    \param[in]  rsp         max rsp length
    \retval     0 if send ok, negtive value if send fail
*/
int at_spi_send_file_wait_rsp(uint8_t *data, int data_len, int segment_len, char *rsp, int rsp_len)
{
    char ch = 0;
    char *p = rsp;
    int ret = 0, len = 0, cnt = 0, loop = 0;
    char *data_crc = NULL;
    uint32_t checksum = 0, remain = 0, len_align = 0, remain_len = data_len;
    __IO uint32_t read_data;
    uint32_t t1, t2;

    if (data_len == 0 || rsp == NULL || segment_len < SPI_MIN_SEGMENT_LEN || rsp_len < 3)
        return -1;

#ifdef SPI_MASTER_DEBUG_PRINT
    app_print("sendfile: data_len=%d, segment_len=%d, rsp_len=%d\r\n",
            data_len, segment_len, rsp_len);
#endif
    data_crc = (char *)sys_zalloc(segment_len + 4);
    if (data_crc == NULL) {
        ret = -2;
        goto Exit;
    }

    t1 = sys_current_time_get();

    spi_manager.stat = SPI_Master_Data_Sent;
    rcu_periph_clock_enable(RCU_CRC);

    /* 0. Aquire SPI HW lock */
    spi_hw_lock();

    /* 1. config spi master using polling */
    wait_handshake_pin_idle();

    /* 2. send file segment */
    while (remain_len > 0) {
        /* 2.1. simulate send data */
        uint32_t real_len = MIN(remain_len, segment_len);
        sys_memset(data_crc, 'a', 1000);
        sys_memset(data_crc + 1000, '6', 460);

        crc_data_register_reset();
        checksum = crc_block_data_calculate((uint32_t *)data_crc, real_len/4);
        remain = real_len & 0x03;
        len_align = real_len - remain;
        if (remain) {
            read_data = *(uint32_t *)(data_crc + len_align);
            read_data = ((read_data  << (8 * (4 - remain))) >> (8 * (4 - remain)));
            checksum = crc_single_data_calculate(read_data);
        }
        data_crc[real_len + 3] = (checksum & 0xFF000000) >> 24;
        data_crc[real_len + 2] = (checksum & 0x00FF0000) >> 16;
        data_crc[real_len + 1] = (checksum & 0x0000FF00) >> 8;
        data_crc[real_len + 0] = (checksum & 0x000000FF);
        sys_memset(rsp, 0, rsp_len);

retry:
        /* 2.2 send data over spi */
        // wait spi is ready to recieve data
        if (sys_sema_down(&spi_slave_ready_sema, AT_TRX_TIMEOUT) == OS_TIMEOUT) { // wait 30s
            app_print("waiting slave ready timeout\r\n");
            ret = -2;
            goto retry;
        }

        spi_master_put_data((uint8_t *)data_crc, real_len + 4, 0);

        /* 2.3. waiting data ack */
        spi_manager.stat = SPI_Master_Data_ACK;

        /* 2.4  wait data ack is ready */
        if (sys_sema_down(&spi_slave_ready_sema, AT_TRX_TIMEOUT) == OS_TIMEOUT) { // wait 30s
            app_print("waiting data ack timeout\r\n");
            ret = -3;
            goto retry;
        }

        /* 2.5 recv rsp length */
        len = spi_master_dma_recv_len();
        if (len > rsp_len || len == -1) {
            ret = -4;
            goto retry;
        }

        /* 2.6 rcv rsp */
        sys_memset(rsp, 0, rsp_len);
        spi_master_dma_recv((uint8_t *)rsp, len);

        /* 2.7 parse response */
#ifdef SPI_MASTER_DEBUG_PRINT
        app_print("file rsp=%s\r\n", rsp);
#endif

        /* 8. parse the ack to retry or not */
        if (strstr(rsp, "N")) {
            ret = -4;
            goto retry;
        }

        loop++;
        remain_len = remain_len - real_len;
    }

    t2 = sys_current_time_get();
#ifdef SPI_MASTER_DEBUG_PRINT
    app_print("File Transfer Done ----- costing time=%d(ms)\r\n", t2 - t1);
#endif
    while (RESET == spi_flag_get(SPI_FLAG_TBE));
    spi_hw_unlock();

Exit:
    SET_AT_SPI_NSS_HIGH();
    if (data_crc)
        sys_mfree(data_crc);

    rcu_periph_clock_disable(RCU_CRC);
    return ret;
}

#endif
