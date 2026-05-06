/*!
    \file    atcmd_azure.c
    \brief   AT command for Azure cloud

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
#include <app_cfg.h>

#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
#include "sample_azure_iot_f527_api.h"

static int at_uart_send_wait_rsp(char *cmd, int cmd_len,
                                          char *data, int data_len,
                                          char *rsp, int rsp_len)
{
    int count = 100;
    int ret = AT_RSP_TIMEOUT;
    char *p = rsp;
    char ch;

    if ((cmd == NULL && data == NULL)
        || rsp == NULL)
        return -1;

    sys_memset(rsp, 0, rsp_len);

    /* 1. wait uart rx process done */
    while (at_uart_rx_is_ongoing() && count-- > 0) {
        sys_ms_sleep(1);
    }

    /* 2. disable uart rx buffer not empty interrupt */
    usart_interrupt_disable(at_uart_conf.usart_periph, USART_INT_RBNE);

    /* 3. wait uart tx done */
    sys_sema_down(&at_hw_tx_sema, 0);

    /* 4. transmit atcmd */
    while (cmd_len > 0) {
        while (RESET == usart_flag_get(at_uart_conf.usart_periph, USART_FLAG_TBE));
        usart_data_transmit(at_uart_conf.usart_periph, *cmd++);
        cmd_len--;
    }

    /* 5. transmit data parameter */
    if (data_len) {
        while (data_len > 0) {
            while (RESET == usart_flag_get(at_uart_conf.usart_periph, USART_FLAG_TBE));
            usart_data_transmit(at_uart_conf.usart_periph, *data++);
            data_len--;
        }
    }

    /* 6. clear rx buf */
    uart_rx_flush(at_uart_conf.usart_periph);

    /* 7. transmit '\r' to finish command */
    while (RESET == usart_flag_get(at_uart_conf.usart_periph, USART_FLAG_TBE));
    usart_data_transmit(at_uart_conf.usart_periph, '\r');
    while (RESET == usart_flag_get(at_uart_conf.usart_periph, USART_FLAG_TC));

    /* 8. wait rx feedback, OK or ERROR */
    do {
        if (uart_getc_with_timeout(at_uart_conf.usart_periph, &ch, 85500000)){ //1500000
            break;
        }
        *p++ = ch;
        if (ch == '\r' || ch == '\n')
            break;
        if (((uint32_t)p - (uint32_t)rsp) > rsp_len)
            break;
    } while (1);

    /* 9. check response */
    AT_TRACE("rsp=%s\r\n", rsp);
    if (strstr(rsp, "OK")) {
        ret = AT_OK;
    } else if (strstr(rsp, "ERROR")) {
        ret = AT_RSP_ERR;
    }

    /* 10. release tx lock */
    sys_sema_up(&at_hw_tx_sema);

    /* 11. enable rx interrupt */
    usart_interrupt_enable(at_uart_conf.usart_periph, USART_INT_RBNE);
    //AT_TRACE("leave at_uart_send_wait_rsp\r\n");
    return ret;
}

static int at_str2hex(char *input_str, uint8_t *output_hex, uint32_t *output_len)
{
    uint32_t input_len;
    char iter_char = 0;
    int i;

    if (input_str == NULL) {
        return -1;
    }

    input_len = strlen(input_str);
    if (input_len <= 0 || input_len % 2 != 0 ||
        output_hex == NULL || *output_len < input_len / 2) {
        AT_TRACE("at_str2hex: input_len = %d *output_len = %d\r\n", input_len, *output_len);
        return -2;
    }
    sys_memset(output_hex, 0, *output_len);

    *output_len = input_len / 2;
    for (i = 0; i < input_len; i += 2) {
        if (input_str[i] >= '0' && input_str[i] <= '9') {
            iter_char = input_str[i] - '0';
        } else if (input_str[i] >= 'A' && input_str[i] <= 'F') {
            iter_char = input_str[i] - 'A' + 0x0A;
        } else if (input_str[i] >= 'a' && input_str[i] <= 'f') {
            iter_char = input_str[i] - 'a' + 0x0A;
        } else {
            return -3;
        }
        output_hex[i / 2] |= (iter_char << 4) & 0xF0;

        if (input_str[i + 1] >= '0' && input_str[i + 1] <= '9') {
            iter_char = input_str[i + 1] - '0';
        } else if (input_str[i + 1] >= 'A' && input_str[i + 1] <= 'F') {
            iter_char = input_str[i + 1] - 'A' + 0x0A;
        } else if (input_str[i + 1] >= 'a' && input_str[i + 1] <= 'f') {
            iter_char = input_str[i + 1] - 'a' + 0x0A;
        } else {
            return -4;
        }
        output_hex[i / 2] |= (iter_char) & 0x0F;
    }
    #if 0
    AT_TRACE("==== %s ====\r\n", __func__);
    for(int i = 0; i < *output_len; i++)
        AT_TRACE("%02x", output_hex[i]);
    AT_TRACE("\r\n");
    #endif
    return 0;
}
/*=============================================================================*/
/*========================= AT Azure command handlers =========================*/
/*=============================================================================*/
int aes_crypt_ecb(uint8_t *key, uint32_t key_len,
                    int mode,
                    const uint8_t input[16],
                    uint8_t output[16] )
{
    ErrStatus ret;
    cau_parameter_struct cau_aes_parameter;
    cau_aes_parameter.alg_dir = mode;
    cau_aes_parameter.key = key;
    cau_aes_parameter.key_size = key_len * 8;
    cau_aes_parameter.input = (uint8_t *)input;
    cau_aes_parameter.in_length = 16;

    rcu_periph_clock_enable(RCU_CAU);
    __disable_irq();
    ret = cau_aes_ecb(&cau_aes_parameter, output);
    __enable_irq();
    //rcu_periph_clock_disable(RCU_CAU);
    return (ret == ERROR) ? 1 : 0;
}

static int hash_sha256(uint8_t *input, uint32_t len, uint8_t *hash_result)
{
    ErrStatus ret;

    rcu_periph_clock_enable(RCU_HAU);
    __disable_irq();
    ret = hau_hash_sha_256(input, len, hash_result);
    __enable_irq();
    //rcu_periph_clock_disable(RCU_HAU);

    return (ret == ERROR) ? 1 : 0;
}

void hash_sha256_dma(uint8_t *input, uint32_t len, uint8_t *hash_result)
{
    dma_multi_data_parameter_struct dma_init_parameter;
    hau_init_parameter_struct init_para;

    uint32_t num_last_valid = 0U;
    uint32_t inputaddr    = (uint32_t)input;
    uint32_t offset = 0, actual_num;
    int remain = len;

    rcu_periph_clock_enable(RCU_HAU);
    rcu_periph_clock_enable(RCU_DMA);

    /* number of valid bits in last word */
    num_last_valid = 8U * (len & 3U);

    /* HAU peripheral initialization */
    hau_deinit();

    /* HAU configuration */
    init_para.algo = HAU_ALGO_SHA256;
    init_para.mode = HAU_MODE_HASH;
    init_para.datatype = HAU_SWAPPING_8BIT;
    hau_init(&init_para);

    /* configure the number of valid bits in last word of the message */
    hau_last_word_validbits_num_config(num_last_valid);

    /* DMA configuration */
    dma_multi_data_para_struct_init(&dma_init_parameter);
    dma_init_parameter.periph_addr = (uint32_t)(&HAU_DI);
    dma_init_parameter.periph_width = DMA_PERIPH_WIDTH_32BIT;
    dma_init_parameter.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_parameter.memory_width = DMA_MEMORY_WIDTH_32BIT;
    dma_init_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_parameter.memory_burst_width = DMA_MEMORY_BURST_4_BEAT;
    dma_init_parameter.periph_burst_width = DMA_MEMORY_BURST_4_BEAT;
    dma_init_parameter.critical_value = DMA_FIFO_4_WORD;
    dma_init_parameter.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    dma_init_parameter.direction = DMA_MEMORY_TO_PERIPH;
    dma_init_parameter.priority = DMA_PRIORITY_ULTRA_HIGH;

    hau_multiple_single_dma_config(MULTIPLE_DMA_NO_DIGEST);
    while (remain > 0) {
        if (remain > HAU_DMA_BLOCK_SIZE) {
            actual_num = (HAU_DMA_BLOCK_SIZE >> 2U);
            remain -= HAU_DMA_BLOCK_SIZE;
        } else {
            actual_num = (remain >> 2U) + ((remain & 3U) ? 1 : 0);
            remain = 0;
            hau_multiple_single_dma_config(SINGLE_DMA_AUTO_DIGEST);
        }
        dma_deinit(DMA_CH7);
        dma_init_parameter.memory0_addr = inputaddr + offset;
        dma_init_parameter.number = actual_num;
        dma_multi_data_mode_init(DMA_CH7, &dma_init_parameter);

        dma_channel_subperipheral_select(DMA_CH7, DMA_SUBPERI2);

        /* enable DMA channel */
        dma_channel_enable(DMA_CH7);

        hau_dma_enable();

        /* wait until the last transfer from OUT FIFO */
        while(!dma_flag_get(DMA_CH7, DMA_FLAG_FTF)) {
        }
        dma_flag_clear(DMA_CH7, DMA_FLAG_FTF);

        /* wait until the busy flag is reset */
        while(hau_flag_get(HAU_FLAG_BUSY)) {
        }
        offset += actual_num << 2;
    }
    /* read the message digest */
    hau_sha_md5_digest_read(HAU_ALGO_SHA256, hash_result);

    //rcu_periph_clock_disable(RCU_DMA);
    //rcu_periph_clock_disable(RCU_HAU);
    return ;
}

void azure_led_init()
{
    rcu_periph_clock_enable(RCU_GPIOA);

    gpio_mode_set(WIFI_CONNECTED_LED_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, WIFI_CONNECTED_LED_PIN);
    gpio_output_options_set(WIFI_CONNECTED_LED_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, WIFI_CONNECTED_LED_PIN);

    gpio_mode_set(AZURE_CONNECTED_LED_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, AZURE_CONNECTED_LED_PIN);
    gpio_output_options_set(AZURE_CONNECTED_LED_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, AZURE_CONNECTED_LED_PIN);
}

void wifi_connected_led(int state)
{
    if (state == RESET) {
        gpio_bit_reset(WIFI_CONNECTED_LED_GPIO, WIFI_CONNECTED_LED_PIN);
    } else {
        gpio_bit_set(WIFI_CONNECTED_LED_GPIO, WIFI_CONNECTED_LED_PIN);
    }
}

void azure_connected_led(int state)
{
    if (state == RESET) {
        gpio_bit_reset(AZURE_CONNECTED_LED_GPIO, AZURE_CONNECTED_LED_PIN);
    } else {
        gpio_bit_set(AZURE_CONNECTED_LED_GPIO, AZURE_CONNECTED_LED_PIN);
    }
}

static void cb_connect_fail(void *eloop_data, void *user_ctx)
{
    int vif_idx = WIFI_VIF_INDEX_DEFAULT;
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(vif_idx);
    int last_reason = wvif->sta.last_reason;
    enum wifi_conn_rsp_t result;

    switch (last_reason) {
    case WIFI_MGMT_CONN_UNSPECIFIED:
        result = WIFI_CONN_UNSPECIFIED;
        break;
    case WIFI_MGMT_CONN_NO_AP:
        result = WIFI_CONN_NO_AP;
        break;
    case WIFI_MGMT_CONN_AUTH_FAIL:
        result = WIFI_CONN_AUTH_FAIL;
        break;
    case WIFI_MGMT_CONN_ASSOC_FAIL:
        result = WIFI_CONN_ASSOC_FAIL;
        break;
    case WIFI_MGMT_CONN_HANDSHAKE_FAIL:
        result = WIFI_CONN_HANDSHAKE_FAIL;
        break;
    case WIFI_MGMT_CONN_DHCP_FAIL:
        result = WIFI_CONN_DHCP_FAIL;
        break;
    default:
        result = WIFI_CONN_UNSPECIFIED;
        break;
    }

    atcmd_wifi_conn_rsp(result);
    eloop_event_unregister(WIFI_MGMT_EVENT_CONNECT_FAIL, cb_connect_fail);
}

void at_azure_wifi_connect(int argc, char **argv)
{
    int vif_idx = WIFI_VIF_INDEX_DEFAULT;
    struct wifi_vif_tag *wvif = (struct wifi_vif_tag *)vif_idx_to_wvif(vif_idx);
    struct sta_cfg *cfg = &wvif->sta.cfg;

    AT_RSP_START(512);
    if (argc == 3) {
        char *ssid = at_string_parse(argv[1]);
        char *password = at_string_parse(argv[2]);
        if (ssid == NULL) {
            goto Error;
        }
        eloop_event_register(WIFI_MGMT_EVENT_CONNECT_FAIL, cb_connect_fail, NULL, NULL);
        if (wifi_management_connect(ssid, password, false)) {
            goto Error;
        }
    } else {
        goto Error;
    }

    AT_RSP_OK();
    return;

Error:
    AT_RSP_ERR();
    return;
}

static void at_azure_component_create(int argc, char **argv)
{
    char *comp[sampleaduPNP_COMPONENTS_LIST_LENGTH];
    uint32_t comp_num;
    int ret = 0;
    int i;

    if (argc < 3) {
        AT_TRACE("AT+AZCOMC: argc is less than 2.\r\n");
        goto Error;
    }

    comp_num = atoi(argv[1]);
    if (comp_num > sampleaduPNP_COMPONENTS_LIST_LENGTH) {
        AT_TRACE("AT+AZCOMC: comp_num is more than %d.\r\n", sampleaduPNP_COMPONENTS_LIST_LENGTH);
        goto Error;
    }

    if (comp_num != argc - 2) {
        AT_TRACE("AT+AZCOMC: comp_num is not equal to component number.\r\n");
        goto Error;
    }

    for (i = 0; i < comp_num; i++) {
        comp[i] = argv[i+2];
        AT_TRACE("COMPONENT[%d]: %s\r\n", i, comp[i]);
    }

    //call azure api to create components
    ret = azure_iot_hub_component_update((const uint8_t **)comp, comp_num);
    if (ret)
        goto Error;

    AT_RSP_DIRECT("OK\r\n", 4);
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
}

static void at_azure_cert(int argc, char **argv)
{
    uint8_t hash[32], hash_local[32];
    uint32_t hash_len;
    char *cert;
    uint32_t cert_len;
    int ret;

    if (argc != 4) {
        AT_TRACE("AT+AZCRT: argc is not 5.\r\n");
        goto Error;
    }

    hash_len = atoi(argv[1]);
    if (hash_len > 32) {
        AT_TRACE("AT+AZCRT: hash len > 32.\r\n");
        goto Error;
    }
    ret = at_str2hex(argv[2], hash, &hash_len);
    if (ret) {
        AT_TRACE("AT+AZCRT: hash parse failed.\r\n");
        goto Error;
    }
    cert_len = atoi(argv[3]);
    cert = sys_malloc(cert_len);
    if (NULL == cert) {
        AT_TRACE("AT+AZCRT: alloc cert failed.\r\n");
        goto Error;
    }

    AT_RSP_DIRECT("OK\r\n", 4);

    at_uart_dma_receive((uint32_t)cert, cert_len);

    ret = hash_sha256((uint8_t *)cert, cert_len, (uint8_t *)hash_local);
    if (ret) {
        AT_TRACE("AT+AZCRT: calculate hash failed.\r\n");
        goto End;
    }
    if (sys_memcmp(hash, hash_local, hash_len) == 0) {
        AT_RSP_DIRECT("OK\r\n", 4);
        AT_TRACE("AT+AZCRT: cert hash check pass.\r\n");
    } else {
        AT_RSP_DIRECT("ERROR\r\n", 7);
        AT_TRACE("AT+AZCRT: cert hash check failed.\r\n");
        AT_TRACE_DATA("hash:", (char*)hash, hash_len);
        AT_TRACE_DATA("hash_local:", (char*)hash_local, sizeof(hash_local));
    }

    // save to azure
    ret = azure_iot_hub_x509cert_update((const uint8_t *)cert, cert_len);
    if (ret) {
        AT_RSP_DIRECT("ERROR\r\n", 7);
    }
End:
    sys_mfree(cert);
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
}

static void at_azure_symkey(int argc, char **argv)
{
#if 1
    uint8_t *cipher = NULL;
    uint32_t cipher_len;
    uint8_t key[AES_KEY_SZ];
    char *symkey = NULL;
    uint8_t *input, *output;
    int ret, i, len;

    if (argc != 3) {
        AT_TRACE("AT+AZSYMKEY: argc is not 3.\r\n");
        goto Error;
    }

    for (i = 0; i < AES_KEY_SZ; i++) {
        key[i] = i;
    }

    cipher_len = atoi(argv[1]) + 1;
    cipher = sys_zalloc(cipher_len);
    symkey = sys_zalloc(cipher_len);
    if ((NULL == cipher) || (NULL == symkey)) {
        AT_TRACE("%s: Allocate cipher or symkey failed (cipher_len = %u)\r\n", __func__, cipher_len);
        goto Error;
    }

    ret = at_str2hex(argv[2], cipher, &cipher_len);
    if (ret) {
        AT_TRACE("AT+AZSYMKEY: at_str2hex return %d.\r\n", ret);
        goto Error;
    }
    if (cipher_len % AES_BLOCK_SZ) {
        AT_TRACE("AT+AZSYMKEY: the cipherkey len (%d) is not multiple of AES_BLOCK_SZ.\r\n", cipher_len);
        goto Error;
    }

    len = cipher_len;
    input = cipher;
    output = (uint8_t *)symkey;
    while (len > 0) {
        ret = aes_crypt_ecb(key, AES_KEY_SZ, CAU_DECRYPT, input, output);
        if (ret != 0)
            goto Error;
        input += AES_BLOCK_SZ;
        len -= AES_BLOCK_SZ;
        output += AES_BLOCK_SZ;
    }

    AT_TRACE("SYMKEY: %u, %s\r\n", strlen(symkey), symkey);
    //call azure api to save symkey
    ret = azure_iot_hub_symkey_update((const uint8_t *)symkey, strlen(symkey));
    if (ret)
        goto Error;

    AT_RSP_DIRECT("OK\r\n", 4);
    if (cipher)
        sys_mfree(cipher);
    if (symkey)
        sys_mfree(symkey);
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    if (cipher)
        sys_mfree(cipher);
    if (symkey)
        sys_mfree(symkey);
    return;
#else
    char *symkey;
    uint32_t symkey_len;
    int ret;

    if (argc != 3) {
        AT_TRACE("AT+AZSYMKEY: argc is not 3.\r\n");
        goto Error;
    }
    symkey_len = atoi(argv[1]);
    symkey = argv[2];

    AT_TRACE("SYMKEY: %u, %s\r\n", symkey_len, symkey);
    //call azure api to save symkey
    ret = azure_iot_hub_symkey_update((const uint8_t *)symkey, symkey_len);
    if (ret)
        goto Error;

    AT_RSP_DIRECT("OK\r\n", 4);
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
#endif
}

static void at_azure_endpoint(int argc, char **argv)
{
    char *ept;
    uint32_t ept_len;

    if (argc != 3) {
        AT_TRACE("AT+AZEPT: argc is not 3.\r\n");
        goto Error;
    }

    ept_len = atoi(argv[1]);
    ept = argv[2];

    AT_TRACE("ENDPOINT: %s\r\n", ept);
    //call azure api to save endpoint
    //(ept, ept_len)
    azure_iot_hub_endpoint_update((const uint8_t *)ept, ept_len);

    AT_RSP_DIRECT("OK\r\n", 4);
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
}

static void at_azure_idsp(int argc, char **argv)
{
    char *idsp;
    uint32_t idsp_len;

    if (argc != 3) {
        AT_TRACE("AT+AZIDSP: argc is not 3.\r\n");
        goto Error;
    }

    idsp_len = atoi(argv[1]);
    idsp = argv[2];

    AT_TRACE("IDSCOPE: %s\r\n", idsp);
    //call azure api to save id scope
    //(idsp, idsp_len)
    if (azure_iot_hub_idscope_update((const uint8_t *)idsp, idsp_len))
        goto Error;

    AT_RSP_DIRECT("OK\r\n", 4);
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
}

static void at_azure_regid(int argc, char **argv)
{
    char *dev_regid;
    uint32_t dev_regid_len;

    if (argc != 3) {
        AT_TRACE("AT+AZDEVREGID: argc is not 3.\r\n");
        goto Error;
    }

    dev_regid_len = atoi(argv[1]);
    dev_regid = argv[2];

    AT_TRACE("DEV_REGID: %s\r\n", dev_regid);
    //call azure api to save device register id
    //(dev_regid, dev_regid_len)
    if (azure_iot_hub_registrationid_update((const uint8_t *)dev_regid, dev_regid_len))
        goto Error;

    AT_RSP_DIRECT("OK\r\n", 4);
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
}

static void at_azure_port(int argc, char **argv)
{
    uint32_t port;

    if (argc != 2) {
        AT_TRACE("AT+AZPORT: argc is not 2.\r\n");
        goto Error;
    }

    port = atoi(argv[1]);

    AT_TRACE("HUB PORT: %u\r\n", port);
    //call azure api to save port
    azure_iot_hub_port_update(port);

    AT_RSP_DIRECT("OK\r\n", 4);
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
}

static void at_azure_pnp_modid(int argc, char **argv)
{
    char *pnp_modid;
    uint32_t pnp_modid_len;

    if (argc != 3) {
        AT_TRACE("AT+AZPNPMODID: argc is not 3.\r\n");
        goto Error;
    }

    pnp_modid_len = atoi(argv[1]);
    pnp_modid = argv[2];

    AT_TRACE("PNP_MODID: %s\r\n", pnp_modid);
    //call azure api to save pnp model id
    if (azure_iot_hub_model_update((const uint8_t *)pnp_modid, pnp_modid_len))
        goto Error;

    AT_RSP_DIRECT("OK\r\n", 4);
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
}

static void at_azure_devid(int argc, char **argv)
{
    char *devid;
    uint32_t devid_len;

    if (argc != 3) {
        AT_TRACE("AT+AZDEVID: argc is not 3.\r\n");
        goto Error;
    }

    devid_len = atoi(argv[1]);
    devid = argv[2];

    AT_TRACE("DEVID: %s\r\n", devid);
    //call azure api to save device id if dps disabled
    if (azure_iot_hub_deviceid_update((const uint8_t *)devid, devid_len))
        goto Error;

    AT_RSP_DIRECT("OK\r\n", 4);
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
}

static void at_azure_host_name(int argc, char **argv)
{
    char *host_name;
    uint32_t host_name_len;

    if (argc != 3) {
        AT_TRACE("AT+AZHOSTNM: argc is not 3.\r\n");
        goto Error;
    }

    host_name_len = atoi(argv[1]);
    host_name = argv[2];

    AT_TRACE("HOSTNAME: %s\r\n", host_name);
    //call azure api to save host name if dps disabled
    if (azure_iot_hub_hostname_update((const uint8_t *)host_name, host_name_len))
        goto Error;

    AT_RSP_DIRECT("OK\r\n", 4);
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
}

static void at_azure_adu_manufacturer(int argc, char **argv)
{
    char *manuf;
    uint32_t manuf_len;

    if (argc != 3) {
        AT_TRACE("AT+AZADUMANUF: argc is not 3.\r\n");
        goto Error;
    }

    manuf_len = atoi(argv[1]);
    manuf = argv[2];

    AT_TRACE("ADU Device MANUFACTURER: %d, %s\r\n", manuf_len, manuf);
    //call azure api to save adu manufacturer
    if (azure_iot_adu_manufacturer_update((const uint8_t *)manuf, manuf_len))
        goto Error;

    AT_RSP_DIRECT("OK\r\n", 4);

    AT_TRACE("RSP;OK\r\n");
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
}

static void at_azure_adu_model(int argc, char **argv)
{
    char *model;
    uint32_t model_len;

    if (argc != 3) {
        AT_TRACE("AT+AZADUMOD: argc is not 3.\r\n");
        goto Error;
    }

    model_len = atoi(argv[1]);
    model = argv[2];

    AT_TRACE("ADU Device Model: %s\r\n", model);
    //call azure api to save adu device model
    if (azure_iot_adu_model_update((const uint8_t *)model, model_len))
        goto Error;

    AT_RSP_DIRECT("OK\r\n", 4);
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
}

static void at_azure_adu_provider(int argc, char **argv)
{
    char *provider;
    uint32_t provider_len;

    if (argc != 3) {
        AT_TRACE("AT+AZADUPROV: argc is not 3.\r\n");
        goto Error;
    }

    provider_len = atoi(argv[1]);
    provider = argv[2];

    AT_TRACE("ADU Provider: %s\r\n", provider);
    //call azure api to save adu update provider
    if (azure_iot_adu_provider_update((const uint8_t *)provider, provider_len))
        goto Error;

    AT_RSP_DIRECT("OK\r\n", 4);
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
}


static void at_azure_adu_updatename(int argc, char **argv)
{
    char *name;
    uint32_t name_len;

    if (argc != 3) {
        AT_TRACE("AT+AZADUPNM: argc is not 3.\r\n");
        goto Error;
    }

    name_len = atoi(argv[1]);
    name = argv[2];

    AT_TRACE("ADU UpdateName: %s\r\n", name);
    //call azure api to save adu update name
    if (azure_iot_adu_updatename_update((const uint8_t *)name, name_len))
        goto Error;

    AT_RSP_DIRECT("OK\r\n", 4);
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
}

static void at_azure_adu_updatever(int argc, char **argv)
{
    char *ver;
    uint32_t ver_len;

    if (argc != 3) {
        AT_TRACE("AT+AZADUPVER: argc is not 3.\r\n");
        goto Error;
    }

    ver_len = atoi(argv[1]);
    ver = argv[2];

    AT_TRACE("ADU UpdateVer: %s\r\n", ver);
    //call azure api to save adu update version
    if (azure_iot_adu_updatever_update((const uint8_t *)ver, ver_len))
        goto Error;

    AT_RSP_DIRECT("OK\r\n", 4);
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
}

static void at_azure_connect(int argc, char **argv)
{
    azure_iot_conn_cfg_t *conn_cfg = NULL;
    int ret;

    if (argc != 3) {
        AT_TRACE("AT+AZCONN: argc is not 3.\r\n");
        goto Error;
    }
    conn_cfg = sys_zalloc(sizeof(azure_iot_conn_cfg_t));
    if (!conn_cfg) {
        AT_TRACE("AT+AZCONN: alloc conn_cfg failed\r\n");
        goto Error;
    }

    conn_cfg->dps_disable = atoi(argv[1]); //0: dps, 1: direct
    conn_cfg->secure_mode = atoi(argv[2]); //0: symkey, 1: x509

    AT_TRACE("Azure Connect: no_dps %d, cert %d\r\n", conn_cfg->dps_disable, conn_cfg->secure_mode);

    // send connect msg to do azure connect
    ret = azure_iot_hub_local_message_send(AZURE_IOT_AT_CONNECT, (uint8_t *)conn_cfg, sizeof(azure_iot_conn_cfg_t));
    if (ret) {
        AT_TRACE("AT+AZCONN: connect fail.\r\n");
        goto Error;
    }

    AT_RSP_DIRECT("OK\r\n", 4);
    return;
Error:
    if (conn_cfg)
        sys_mfree(conn_cfg);
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
}

static void at_azure_disconnect(int argc, char **argv)
{
    int ret;

    AT_TRACE("Azure Disconnect\r\n");
    //call azure api to do azure disconnect
    ret = azure_iot_hub_local_message_send(AZURE_IOT_AT_DISCONNECT, NULL, 0);
    if (ret) {
        AT_TRACE("AT+AZCONN: disconnect fail.\r\n");
        goto Error;
    }

    AT_RSP_DIRECT("OK\r\n", 4);
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
}

static void at_azure_telemetry_update(int argc, char **argv)
{
    char *topic;
    uint32_t topic_len;
    char *payload;
    uint32_t payload_len;
    int ret;
    azure_iot_at_data_t *telemetry = NULL;

    if (argc != 5) {
        AT_TRACE("AT+AZTELS: argc(%d) is not 5.\r\n", argc);
        goto Error;
    }
    topic_len = atoi(argv[1]);
    topic = argv[2];
    payload_len = atoi(argv[3]);
    payload = argv[4];

    AT_RSP_DIRECT("OK\r\n", 4);

    AT_TRACE("Telemetry: topic = %s\r\n", topic);
    AT_TRACE("Telemetry: payload = %s\r\n", payload);

    telemetry = azure_iot_at_data_construct(
                            topic_len, (uint8_t *)topic,
                            payload_len, (uint8_t *)payload);
    if (!telemetry) {
        AT_TRACE("telemetry construct fail\r\n");
        goto Error;
    }

    ret = azure_iot_hub_local_message_send(AZURE_IOT_AT_TELEMETRY, (uint8_t *)telemetry, sizeof(azure_iot_at_data_t));
    if (ret) {
        AT_TRACE("AT+AZTELS: fail.\r\n");
        goto Error;
    }

    return;

Error:
    azure_iot_at_data_free(&telemetry);
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
}

static void at_azure_property_update(int argc, char **argv)
{
    char *topic;
    uint32_t topic_len;
    char *payload;
    uint32_t payload_len;
    int ret;
    azure_iot_at_data_t *property = NULL;

    if (argc != 5) {
        AT_TRACE("AT+AZPROPS: argc(%d) is not 5.\r\n", argc);
        goto Error;
    }
    topic_len = atoi(argv[1]);
    topic = argv[2];
    payload_len = atoi(argv[3]);
    payload = argv[4];

    AT_RSP_DIRECT("OK\r\n", 4);

    AT_TRACE("Property: topic = %s\r\n", topic);
    AT_TRACE("Property: payload = %s\r\n", payload);

    if(strstr(topic, "twin/GET/?")) {
        /* property sync get request */
        property = azure_iot_at_data_nopayload_construct(
                                    topic_len, (uint8_t *)topic,
                                    payload_len, (uint8_t *)payload);
    } else {
        property = azure_iot_at_data_construct(
                                    topic_len, (uint8_t *)topic,
                                    payload_len, (uint8_t *)payload);
    }

    if (!property) {
        AT_TRACE("at construct property fail\r\n");
        goto Error;
    }

    ret = azure_iot_hub_local_message_send(AZURE_IOT_AT_PROPERTY, (uint8_t *)property, sizeof(azure_iot_at_data_t));
    if (ret) {
        AT_TRACE("AT+AZPROPS: fail.\r\n");
        goto Error;
    }

    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    azure_iot_at_data_free(&property);
    return;
}

static void at_azure_property_rsp(int argc, char **argv)
{
    char *topic;
    uint32_t topic_len;
    char *payload;
    uint32_t payload_len;
    int ret;
    azure_iot_at_data_t *proprsp = NULL;

    if (argc != 5) {
        AT_TRACE("AT+AZPROPRSP: argc(%d) is not 5.\r\n", argc);
        goto Error;
    }
    topic_len = atoi(argv[1]);
    topic = argv[2];
    payload_len = atoi(argv[3]);
    payload = argv[4];

    AT_TRACE("Property RSP: topic = %s\r\n", topic);
    AT_TRACE("Property RSP: payload = %s\r\n", payload);

    proprsp = azure_iot_at_data_construct(
                                topic_len, (uint8_t *)topic,
                                payload_len, (uint8_t *)payload);
    if (!proprsp) {
        AT_TRACE("prorsp data construct fail\r\n");
        goto Error;
    }

    ret = azure_iot_hub_local_message_send(AZURE_IOT_AT_PROPERTY, (uint8_t *)proprsp, sizeof(azure_iot_at_data_t));
    if (ret) {
        AT_TRACE("AT+AZPROPRSP: fail.\r\n");
        goto Error;
    }

    AT_RSP_DIRECT("OK\r\n", 4);
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    azure_iot_at_data_free(&proprsp);
    return;
}

static void at_azure_cmd_rsp(int argc, char **argv)
{
    char *topic;
    uint32_t topic_len;
    char *payload;
    uint32_t payload_len;
    int ret = 0;
    azure_iot_at_data_t *cmdrsp = NULL;

    if (argc != 5) {
        AT_TRACE("AT+AZCMDRSP: argc(%d) is not 5.\r\n", argc);
        goto Error;
    }
    topic_len = atoi(argv[1]);
    topic = argv[2];
    payload_len = atoi(argv[3]);
    payload = argv[4];

    AT_TRACE("CMD RSP: topic = %s\r\n", topic);
    AT_TRACE("CMD RSP: payload = %s\r\n", payload);

    cmdrsp = azure_iot_at_data_construct(
                                topic_len, (uint8_t *)topic,
                                payload_len, (uint8_t *)payload);

    if (!cmdrsp) {
        AT_TRACE("cmdrsp data construct fail\r\n");
        goto Error;
    }

    ret = azure_iot_hub_local_message_send(AZURE_IOT_AT_CMD, (uint8_t *)cmdrsp, sizeof(azure_iot_at_data_t));
    if (ret) {
        AT_TRACE("AT+AZCMDRSP: fail.\r\n");
        goto Error;
    }

    AT_RSP_DIRECT("OK\r\n", 4);
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    azure_iot_at_data_free(&cmdrsp);
    return;
}

static void at_azure_state_get(int argc, char **argv)
{
    enum azure_state_t state;

    bool wifi_connected = wifi_vif_is_sta_connected(WIFI_VIF_INDEX_DEFAULT);
    bool azure_connected = azure_iot_hub_azure_connected();

    AT_RSP_START(32);
    if (wifi_connected && azure_connected)
        state = AZURE_STATE_HUB_CONNECTED;
    else if (wifi_connected && !azure_connected)
        state = AZURE_STATE_WIFI_CONNECTED;
    else
        state = AZURE_STATE_IDLE;

    AT_TRACE("STATE = %d\r\n", state);

    AT_RSP("STATE=%d ", state);

    AT_RSP_OK();
}

static void at_azure_dev_update(int argc, char **argv)
{
    char *dev_type;
    char *dev_value;
    uint32_t int_val;
    double dbl_val;

    if (argc != 3) {
        AT_TRACE("AT+AZDEVUPT: argc is not 3.\r\n");
        goto Error;
    }

    dev_type = argv[1];
    dev_value = argv[2];

    if (strcmp(dev_type, "led1") == 0) {
        int_val = atoi(dev_value);
        //update the value of the LED to azure here

    } else if (strcmp(dev_type, "adv") == 0) {
        sscanf(dev_value, "%lf", &dbl_val);
        // update the value of adv to azure here

    } else {
        AT_TRACE("AT+AZDEVUPT: unknown dev type \"%s\"\r\n", dev_type);
        goto Error;
    }

    AT_RSP_DIRECT("OK\r\n", 4);
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
}

#if 0
static void at_azure_ota_rsp(int argc, char **argv)
{
    int result;

    if (argc != 2) {
        AT_TRACE("AT+AZOTARSP: argc is not 2.\r\n");
        goto Error;
    }

    result = atoi(argv[1]);

    AT_TRACE("OTA RESULT: %d\r\n", result);

    //indicate azure task ota result

    AT_RSP_DIRECT("OK\r\n", 4);
    return;
Error:
    AT_RSP_DIRECT("ERROR\r\n", 7);
    return;
}
#endif

/*=============================================================================*/
/*========================= AT Azure command sending APIs =====================*/
/*=============================================================================*/

int atcmd_wifi_conn_rsp(enum wifi_conn_rsp_t result)
{
    char atcmd[32], rsp[32];
    int ret, count = 1;

    if (result == WIFI_CONN_OK) {
        wifi_connected_led(SET);
    } else {
        wifi_connected_led(RESET);
    }

    co_snprintf(atcmd, sizeof(atcmd), "AT+AZCWRSP=%u", result);

Retry:
    ret = at_uart_send_wait_rsp(atcmd, strlen(atcmd),
                                NULL, 0, rsp, sizeof(rsp));
    if (ret != AT_OK) {
        if (--count > 0) {
            goto Retry;
        }
    }

    return ret;
}

int atcmd_azure_conn_rsp(enum azure_conn_rsp_t result)
{
    char atcmd[32], rsp[32];
    int ret, count = 1;

    if (result == AZURE_CONN_OK) {
        azure_connected_led(SET);
    } else {
        azure_connected_led(RESET);
    }

    co_snprintf(atcmd, sizeof(atcmd), "AT+AZCONNRSP=%u", result);

Retry:
    ret = at_uart_send_wait_rsp(atcmd, strlen(atcmd),
                                NULL, 0, rsp, sizeof(rsp));
    if (ret != AT_OK) {
        if (--count > 0) {
            goto Retry;
        }
    }

    return ret;
}

int atcmd_azure_dev_set(char *dev_type, char *dev_val)
{
    char atcmd[32], rsp[32];
    int ret, count = 1;

    co_snprintf(atcmd, sizeof(atcmd), "AT+AZDEVSET=%s,%s", dev_type, dev_val);

Retry:
    ret = at_uart_send_wait_rsp(atcmd, strlen(atcmd),
                                NULL, 0, rsp, sizeof(rsp));
    if (ret != AT_OK) {
        if (--count > 0) {
            goto Retry;
        }
    }

    return ret;
}

int atcmd_azure_prop_req(char *topic, uint32_t topic_len,
                                 char *payload, uint32_t payload_len)
{
    char *atcmd, rsp[32], *p;
    uint32_t atcmd_len, str_len;
    int ret;

    atcmd_len = topic_len + payload_len + 64;
    atcmd = sys_zalloc(atcmd_len);
    if (NULL == atcmd) {
        AT_TRACE("%s: alloc atcmd failed\r\n", __func__);
        return AT_ERR;
    }

    p = atcmd;
    str_len = co_snprintf(p, atcmd_len, "AT+AZPROPREQ=%u,\'", topic_len);
    p += str_len;
    sys_memcpy(p, topic, topic_len);
    p += topic_len;
    str_len = co_snprintf(p, atcmd_len, "\',%u,\'", payload_len);
    p += str_len;
    sys_memcpy(p, payload, payload_len);
    p += payload_len;
    co_snprintf(p, atcmd_len, "\'");

    AT_TRACE("prop req:%s, len=%d\r\n", atcmd, strlen(atcmd));

    ret = at_uart_send_wait_rsp(atcmd, strlen(atcmd),
                                NULL, 0, rsp, sizeof(rsp));
    if (ret != AT_OK) {
        AT_TRACE("%s: failed return %d\r\n", __func__, ret);
        return ret;
    }

    return AT_OK;
}

int atcmd_azure_cmd_req(char *topic, uint32_t topic_len,
                                 char *payload, uint32_t payload_len)
{
    char *atcmd, rsp[32], *p;
    uint32_t atcmd_len, str_len;
    int ret;

    atcmd_len = topic_len + payload_len + 64;
    atcmd = sys_zalloc(atcmd_len);
    if (NULL == atcmd) {
        AT_TRACE("%s: alloc atcmd failed\r\n", __func__);
        return AT_ERR;
    }

    p = atcmd;
    str_len = co_snprintf(p, atcmd_len, "AT+AZCMDREQ=%u,\'", topic_len);
    p += str_len;
    sys_memcpy(p, topic, topic_len);
    p += topic_len;
    str_len = co_snprintf(p, atcmd_len, "\',%u,\'", payload_len);
    p += str_len;
    sys_memcpy(p, payload, payload_len);
    p += payload_len;
    co_snprintf(p, atcmd_len, "\'");

    AT_TRACE("cmd req:%s, len=%d\r\n", atcmd, strlen(atcmd));
    ret = at_uart_send_wait_rsp(atcmd, strlen(atcmd),
                                NULL, 0, rsp, sizeof(rsp));
    if (ret != AT_OK) {
        AT_TRACE("%s: failed return %d\r\n", __func__, ret);
        return ret;
    }

    return AT_OK;
}

int atcmd_azure_c2dmsg_send(char *topic, uint32_t topic_len,
                                 char *payload, uint32_t payload_len)
{
    char *atcmd, rsp[32], *p;
    uint32_t atcmd_len, str_len;
    int ret;

    atcmd_len = topic_len + payload_len + 64;
    atcmd = sys_zalloc(atcmd_len);
    if (NULL == atcmd) {
        AT_TRACE("%s: alloc atcmd failed\r\n", __func__);
        return AT_ERR;
    }

    p = atcmd;
    str_len = co_snprintf(p, atcmd_len, "AT+AZC2DMSGS=%u,\'", topic_len);
    p += str_len;
    sys_memcpy(p, topic, topic_len);
    p += topic_len;
    str_len = co_snprintf(p, atcmd_len, "\',%u,\'", payload_len);
    p += str_len;
    sys_memcpy(p, payload, payload_len);
    p += payload_len;
    co_snprintf(p, atcmd_len, "\'");

    AT_TRACE("c2d:%s, len=%d\r\n", atcmd, strlen(atcmd));

    ret = at_uart_send_wait_rsp(atcmd, strlen(atcmd),
                                NULL, 0, rsp, sizeof(rsp));
    if (ret != AT_OK) {
        AT_TRACE("%s: failed return %d\r\n", __func__, ret);
        return ret;
    }

    return AT_OK;
}

int atcmd_azure_ota_ind_send(char *ver, uint32_t fw_len)
{
    char at_cmd[64];
    char *ok_str = "OTA AGREE";
    char rsp_buf[64];
    int retry_count = 1;
    int ret = AT_OK;

    co_snprintf(at_cmd, sizeof(at_cmd), "AT+AZOTAI=\'%s\',%u", ver, fw_len);
Retry:
    ret = at_uart_send_wait_rsp(at_cmd, strlen(at_cmd), NULL, 0, rsp_buf, sizeof(rsp_buf));
    if (ret) {
        AT_TRACE("AT+AZOTAI failed return %d\r\n", ret);
    } else {
        if (NULL == strstr(rsp_buf, ok_str)) {
            ret = AT_ERR;
            AT_TRACE("%s: no \"%s\" in rsp\r\n", __func__, ok_str);
        }
    }
    if ((ret != AT_OK) && (--retry_count > 0))
        goto Retry;

    return ret;
}

int atcmd_azure_ota_result_send(char *ver, uint32_t result)
{
    char at_cmd[64] = {0};
    char rsp[64];
    int ret = AT_OK;

    co_snprintf(at_cmd, sizeof(at_cmd), "AT+AZOTAR=\'%s\',%u", ver, result);
    AT_TRACE("ota result:%s, len=%d\r\n", at_cmd, strlen(at_cmd));
    ret = at_uart_send_wait_rsp(at_cmd, strlen(at_cmd),
                                NULL, 0, rsp, sizeof(rsp));
    if (ret != AT_OK) {
        AT_TRACE("%s: AT+AZOTAR: failed return %d\r\n", __func__, ret);
        return ret;
    }

    return ret;
}


int atcmd_azure_ota_block_send(char *buf, uint32_t len)
{
    char at_cmd[64];
    char rsp_buf[32];
    int retry_count = 1;
    int ret = AT_OK;

    co_snprintf(at_cmd, sizeof(at_cmd), "AT+AZOTAW=%u", len);
    AT_TRACE("ota block send: %d\r\n", len);
    AT_TRACE_DATA("OTA Write", buf, 64);

Retry:
    ret = at_uart_send_wait_rsp(at_cmd, strlen(at_cmd), NULL, 0, rsp_buf, sizeof(rsp_buf));
    if (ret) {
        AT_TRACE("AT+AZOTAW failed return %d\r\n", ret);
        if (--retry_count > 0)
            goto Retry;
    }

    ret = at_uart_send_wait_rsp(NULL, 0, buf, len, rsp_buf, sizeof(rsp_buf));
    if (ret) {
        AT_TRACE("OTA block data send failed return %d\r\n", ret);
    }

    return ret;
}

int atcmd_azure_ota_hash_send(char *buf, uint32_t len)
{
    char at_cmd[64];
    char rsp_buf[32];
    int retry_count = 1;
    int ret = AT_OK;

    co_snprintf(at_cmd, sizeof(at_cmd), "AT+AZOTAHASH=%u", len);
Retry:
    ret = at_uart_send_wait_rsp(at_cmd, strlen(at_cmd), NULL, 0, rsp_buf, sizeof(rsp_buf));
    if (ret) {
        AT_TRACE("AT+AZOTAHASH failed return %d\r\n", ret);
        if (--retry_count > 0)
            goto Retry;
    }

    ret = at_uart_send_wait_rsp(NULL, 0, buf, len, rsp_buf, sizeof(rsp_buf));
    if (ret) {
        AT_TRACE("OTA hash data send failed return %d\r\n", ret);
    }

    return ret;
}

int atcmd_azure_ota_hash_recv(char *hash, uint32_t hash_len)
{
    char at_cmd[64];
    char rsp_buf[32];
    int retry_count = 1;
    int ret = AT_OK;

    co_snprintf(at_cmd, sizeof(at_cmd), "AT+AZOTAHASHGET");
Retry:
    ret = at_uart_send_wait_rsp(at_cmd, strlen(at_cmd), NULL, 0, rsp_buf, sizeof(rsp_buf));
    if (ret) {
        AT_TRACE("AT+AZOTAHASH failed return %d\r\n", ret);
        if (--retry_count > 0)
            goto Retry;
        else
            return ret;
    }

    at_uart_dma_receive((uint32_t)hash, hash_len);
    printf("HASHGET=%2x %2x\r\n", hash[0], hash[31]);

    AT_RSP_DIRECT("OK\r\n", 4);

    return ret;
}
#endif
