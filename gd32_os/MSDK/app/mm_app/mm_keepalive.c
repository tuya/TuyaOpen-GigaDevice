#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if defined(MM_ENABLE_MQTT_KEEPALIVE)
#include "aes_inf.h"
#include "mm_keepalive.h"
#include "tkl_mutex.h"
#include "tkl_thread.h"
#include "tkl_system.h"
#include "tkl_sleep.h"
#include "tkl_wifi.h"
#include "tal_sw_timer.h"
#include "tuya_error_code.h"
#include "mm_cmd_parse.h"
#include "tuya_log.h"
#include "lpmgr.h"
#include "mm_system.h"
#include "mm_gpio.h"
#include "mm_spi.h"
#include "mm_mqtt.h"
#include "netmgr_api.h"
// #include "simple_flash_app.h"
#include "kv_storge.h"
#include "tuya_iot_com_api.h"

#include <lwip/sockets.h>
#include <lwip/netif.h>
#include <lwip/tcpip.h>
#include <lwip/netdb.h>
#include <lwip/opt.h>
#include <wifi/wifi_conf.h>

#define TUYA_ALIVE_TASK_NAME       "TUYA_ALIVE_TASK"
#define POWEROFF_FLAG              "POWEROFF_FLAG"
#define TUYA_ALIVE_TASK_STACK_SIZE (1536 * 4)
#define TUYA_ALIVE_TASK_PRIO       3
#define MAC_ADDR_LENGTH            12
#define HEAD_LENGTH                10
#define ENCRYPTED_DATA_LENGTH      32
#define HB_PACK_LENGTH             (HEAD_LENGTH + ENCRYPTED_DATA_LENGTH)
#define RESERVED_LENGTH            10
#define RECV_PACK_HEAD_LENGTH      4
#define RECV_PACK_ENCRYPTED_LENGTH 32
#define RECV_PACK_LENGTH           (RECV_PACK_HEAD_LENGTH + RECV_PACK_ENCRYPTED_LENGTH)
#define CONFIG_WIFI_BACKOFF_SECS   {60, 180, 600, 1800}

typedef enum {
    WAKEUP_PKG,      // wakeup package
    HB_RESPONSE_PKG, // heartbeat from cloud
    RTC_SET_PKG,     // RTC package from gateway
    GW_NO_RESPONSE,  // recv error but could be missreported
    SOCKET_ERROR,    // recv error, wifi need to restart
    INVAILD_PKG,     // recv succeed but content invaild
    PKG_TYPE_MAX,
} KEEPALIVE_RECV_PKG_TYPE_E;

typedef struct {
    int port;
    uint32_t server_ip;
    uint8_t aes_key[16];
    uint8_t send_data[12];
    uint8_t recv_data[12];
    uint8_t version;
} KEEPALIVE_INFO_GW_T;

// typedef u32_t (*KEEPALIVE_GET_IP_CB)(int reconncet_count);

// typedef int (*KEEPALIVE_CONNECT_LPS_CB)(u32_t ip);

// typedef int (*KEEPALIVE_SEND_PACKAGE_CB)(int type);

// typedef KEEPALIVE_RECV_PKG_TYPE_E (*KEEPALIVE_RECV_PACKAGE_CB)(void);

typedef struct {
    int soft_poweroff;          // 软关机标�?    int keepalive_sock_fd;          //保活连接socket
    int keepalive_pkg_interval; // 保活包发送间�?    int inited; //线程是否被初始化（线程是否在运行中）
    int host_state; // 主控当前状�?    int first_loop; //标志是休眠开始的第一个循环，用于判断是否直接关wifi
    int timer_func; // 定时器功能（开始休眠时，默认为0：时间到断开wifi�?运行完后切至1：时间到开启wifi�?    int
                    // reconn_time;                //重连时间
    int wakeup_reason;
    unsigned int pir_wakeup_interval; // pir唤醒间隔
    unsigned int sleep_time; // 上次休眠下去的时�?    TKL_MUTEX_HANDLE mutex;               //互斥�? TKL_THREAD_HANDLE
                             // keep_alive_task_handle;
    TKL_THREAD_HANDLE reconn_thread_handle;
    TIMER_ID reconn_timer;              // 重连定时器handle
    KEEPALIVE_TYPE_E keepalive_type;    // 保活对象类型
    KEEPALIVE_INFO_GW_T keepalive_info; // 保活相关信息
} KEEPALIVE_CTL_T;

static KEEPALIVE_CTL_T g_keepalive_ctl;
static void __start_reconnect(void);

/**
 *@brief config the pir trigger interval for wakeup.
 *@param[in] interval: interval in s.
 *@return None.
 */
void tuya_keepalive_set_pir_wakeup_interval(int interval)
{
    PR_INFO("set pirwakeup interval: %ds\n", interval);
    g_keepalive_ctl.pir_wakeup_interval = interval;
    return;
}

/**
 *@brief wakeup T31 and turn wifi.
 *@param None.
 *@return None
 */
static void tuya_device_wakeup_from_keepalive(void)
{
    if (!lpmgr_is_registered(TY_LP_SPI)) {
        lpmgr_register(TY_LP_SPI);
    }
    tkl_mutex_lock(g_keepalive_ctl.mutex);
    // wifi_rf_on();
    // netmgr_connect();
    if (g_keepalive_ctl.host_state != HOST_SLEPT) {
        PR_DEBUG("already wakeup");
        tkl_mutex_unlock(g_keepalive_ctl.mutex);
        return;
    }
    g_keepalive_ctl.host_state = HOST_WAKING;
    mm_spi_start();
#if defined(TUYA_SPI_RETRY) && (TUYA_SPI_RETRY == 1)
    tuya_empty_cmd_tx_list();
    __ty_cmd_tx_sem_post();
    rx_status_reset();
#endif
    mm_power_on();
    // ty_system_clean_reset();
    tkl_mutex_unlock(g_keepalive_ctl.mutex);
    if (ty_system_restore_host_wdt() != OPRT_OK) {
        PR_ERR("restart watchdog time failed, REBOOT!!!");
        tkl_system_reset();
    }
}

/**
 *@brief handle the wakeup event.
 *@param[in] reason: event source.
 *@return result
 */
int tuya_wakeup_reason_handler(WAKEUP_REASON_E reason)
{
    if (g_keepalive_ctl.inited == 0) {
        PR_ERR("keepalive thread not initialized yet");
        return WAKEUP_NOT_INITED;
    }
    if (g_keepalive_ctl.host_state != HOST_SLEPT) {
        PR_INFO("already wakeup");
        return KEEPALIVE_ALREADY_AWAKE;
    }
    unsigned int time_now = ty_system_get_time();
    if (time_now - g_keepalive_ctl.sleep_time < 500) {
        PR_INFO("cant wakeup, 500ms not reached");
        return KEEPALIVE_EARLY_WAKEUP;
    }
    if (reason >= WAKEUP_REASON_MAX || reason <= WAKEUP_REASON_MIN) {
        PR_ERR("invailid reason:[%d]", reason);
        return WAKEUP_INVAILD_REASON;
    }
    if (reason == WAKEUP_PIR && time_now - g_keepalive_ctl.sleep_time < g_keepalive_ctl.pir_wakeup_interval * 1000) {
        PR_INFO("PIR wakeup in cooldown");
        return WAKEUP_PIR_COOLDOWN;
    }
    g_keepalive_ctl.wakeup_reason = reason;

    PR_INFO("wake up by [%d]", g_keepalive_ctl.wakeup_reason);
    if (ty_system_get_device_mode() == SINGLE_MODE) {
        if (true == ty_system_device_is_actived()) {
            PR_INFO("\n%s:%d.............type:%d wifi:%d mqtt:%d \n", __func__, __LINE__, reason,
                    netmgr_network_available(), mm_get_mqtt_stat());
            if (!netmgr_network_available() || (!mm_get_mqtt_stat() && !mm_get_mf_test_stat())) {
                PR_INFO("\n%s:%d.............\n", __func__, __LINE__);
                netmgr_reconnect_clear();
                netmgr_fast_reconnect();
            }
        } else if (reason != WAKEUP_POWER_ON) {
            PR_INFO("not actived reboot");
            tkl_system_reset();
        }
    } else {
        if (!netmgr_network_available()) {
            __start_reconnect();
        }
    }
    tuya_device_wakeup_from_keepalive();
    ty_system_report_time(); // get time async and send to T31.
    if (ty_lp_proto_set_wakeup_reason_cs(g_keepalive_ctl.wakeup_reason) == RET_OK) {
        return RET_OK;
    } else {
        return KEEPALIVE_NOTIFY_FAILED;
    }
}

/**
 *@brief turnoff T31 enter sleep mode.
 *@param None.
 *@return None.
 */
static void tuya_set_devive_sleep(void)
{
    tkl_mutex_lock(g_keepalive_ctl.mutex);
    if (g_keepalive_ctl.host_state == HOST_SLEPT) {
        PR_INFO("already slept");
        tkl_mutex_unlock(g_keepalive_ctl.mutex);
        return;
    }
    g_keepalive_ctl.sleep_time = ty_system_get_time();
    mm_spi_stop();
    mm_power_off();
    g_keepalive_ctl.host_state = HOST_SLEPT;
    tkl_mutex_unlock(g_keepalive_ctl.mutex);
    if (ty_system_stop_host_wdt() != OPRT_OK) {
        PR_ERR("stop watchdog time failed, REBOOT!!!");
        tkl_system_reset();
    }
    if (lpmgr_is_registered(TY_LP_SPI)) {
        lpmgr_unregister(TY_LP_SPI);
    }
}

/**
 *@brief calculate the next time to turnoff wifi in s.
 *@param[in] @param[out] count: previous time to wait.
 *@return None.
 */
static void __get_next_sleep_count(int *count)
{
    int backoff_secs[] = CONFIG_WIFI_BACKOFF_SECS;
    int last_count = *count;
    int idx = 0;
    int arr_len = sizeof(backoff_secs) / sizeof(backoff_secs[0]);
    if (last_count <= 0) {
        idx = 0;
    } else if (last_count == backoff_secs[arr_len - 1]) {
        idx = arr_len - 1;
    } else {
        int i = 0;
        for (; i < arr_len; i++) {
            if (backoff_secs[i] == last_count) {
                idx = i + 1;
                break;
            }
        }
    }
    *count = backoff_secs[idx];
}

static void auto_reconnect_proc(void *not_used)
{
    int i = 5;
    while (i > 0 && !netmgr_network_available()) {
        netmgr_connect();
        tkl_system_sleep(10 * 1000);
        i--;
    }
    if (i == 0) {
        netmgr_connect();
        tkl_system_sleep(10 * 1000);
    }
    g_keepalive_ctl.reconn_thread_handle = NULL;
    tkl_thread_release(NULL);
}
static void __start_reconnect(void)
{
    if (true != netmgr_is_ap_distribution()) {
        PR_ERR("never connect to AP, reconnect failed");
        if (lpmgr_is_registered(TY_LP_NETCFG)) {
            lpmgr_unregister(TY_LP_NETCFG);
        }
        return;
    }
    if (g_keepalive_ctl.reconn_thread_handle == NULL) {
        int ret = 0;
        ret = tkl_thread_create(&g_keepalive_ctl.reconn_thread_handle, "auto_reconnect_proc_thread", 1024, 4,
                                auto_reconnect_proc, NULL);
        if (OPRT_OK != ret) {
            PR_ERR("failed in creating autoconnect thread.");
        } else {
            PR_INFO("wifi reconnect");
        }
    }
    return;
}
static void reconn_timer_callback(TIMER_ID timer_id, void *arg)
{
    (void)timer_id;
    (void)arg;
    PR_INFO("time up!\n");
    if (g_keepalive_ctl.host_state == HOST_SLEPT) {
        if (g_keepalive_ctl.timer_func ==
            0) { // 断开wifi，加载重连时�?            __get_next_sleep_count(&g_keepalive_ctl.reconn_time);
            g_keepalive_ctl.timer_func = 1;
            PR_INFO("turn off wifi, for %ds", g_keepalive_ctl.reconn_time);
            PR_DEBUG(
                "change period result:%d\n",
                tal_sw_timer_start(g_keepalive_ctl.reconn_timer, g_keepalive_ctl.reconn_time * 1000, TAL_TIMER_ONCE));
            // wifi_rf_on();
            netmgr_disconnect();
            if (lpmgr_is_registered(TY_LP_NETCFG)) {
                lpmgr_unregister(TY_LP_NETCFG);
            }
        } else { // 开启wifi，连�?分钟
            g_keepalive_ctl.timer_func = 0;
            PR_INFO("turn on wifi");
            PR_DEBUG("change period result:%d\n",
                     tal_sw_timer_start(g_keepalive_ctl.reconn_timer, 60 * 1000, TAL_TIMER_ONCE));
            if (!lpmgr_is_registered(TY_LP_NETCFG)) {
                lpmgr_register(TY_LP_NETCFG);
            }
            __start_reconnect();
            // wifi_rf_off();
        }
    } else {
        PR_INFO("not in sleep, timer stop!\n");
    }
}
/**
 *@brief reconnect router by turnon/-off wifi.
 *@param None.
 *@return None.
 */
static void tuya_reconnect_router(void)
{
    netmgr_disconnect();
    // wifi_rf_off();
    tkl_system_sleep(100);
    // wifi_rf_on();
    __start_reconnect();
}

/**
 *@brief connect the gateway low power server.
 *@param[in] ip: the ip addr to connect.
 *@return -1: fail to connect, other value: connected succeed, as the socket number.
 */
static int tuya_connect_gw(u32_t ip)
{
    if (g_keepalive_ctl.keepalive_type != KEEPALIVE_TO_GW) {
        PR_ERR("invalid keepalive type");
        return -1;
    }
    int ret = -1;
    int socket = -1;
    struct sockaddr_in addr;
    NW_IP_S ip_info;
    u32_t ip_gw = ip;
    unsigned char buf[4] = {0};
    tkl_wifi_get_ip(WF_STATION, &ip_info);
    sscanf(ip_info.gw, "%d.%d.%d.%d", buf, buf + 1, buf + 2, buf + 3);
    ip_gw = buf[3] + (buf[2] << 8) + (buf[1] << 16) + (buf[0] << 24);
    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = lwip_htons(g_keepalive_ctl.keepalive_info.port);
    addr.sin_addr.s_addr = lwip_htonl(ip_gw);
    socket = socket(AF_INET, SOCK_STREAM, 0);
    if (socket < 0) {
        PR_INFO("TCP client create failed");
        return -1;
    }
    ret = connect(socket, (struct sockaddr *)&addr, sizeof(addr));
    PR_INFO("TCP client connect to [%u:%s], port:%d result[%d]", addr.sin_addr.s_addr, ip_info.gw, addr.sin_port, ret);
    if (ret < 0) {
        lwip_close(socket);
        PR_INFO("TCP client connect failed, ret :%d", ret);
        return -1;
    }
    return socket;
}

/**
 *@brief convert char'0'-'f' to hex 0x0-0xf.
 *@param[in] ch: input char '0'-'f'.
 *@param[out] hex: out put hex 0x0-0xf.
 *@return -1: invaild character not a number, 0:succeed.
 */
static int chartohex(uint8_t ch, uint8_t *hex)
{
    if (ch >= 'a') {
        *hex = ch - 'a' + 0x0a;
    } else if (ch >= 'A') {
        *hex = ch - 'A' + 0x0a;
    } else if (ch >= '0') {
        *hex = ch - '0';
    } else {
        *hex = 0xff;
    }
    if (*hex > 0x0f) {
        return -1;
    } else {
        return 0;
    }
}

/**
 *@brief convert mac from string to hex.
 *@param[in] mac_string: input mac string.
 *@param[in] mac_length: mac length.
 *@param[out] mac_hex: out put  mac hex.
 *@return -1: invaild mac content, 0:succeed.
 */
static int convert_mac_string2hex(uint8_t *mac_string, uint8_t *mac_hex, int mac_length)
{
    int i;
    if ((mac_length % 2)) {
        return -1;
    }
    for (i = 0; i < mac_length / 2; i++) {
        uint8_t mac_high = 0;
        uint8_t mac_low = 0;
        if (chartohex(mac_string[2 * i], &mac_high) == -1) {
            return -1;
        }
        if (chartohex(mac_string[2 * i + 1], &mac_low) == -1) {
            return -1;
        }
        mac_hex[i] = mac_low + (mac_high << 4);
    }
    return 0;
}

/**
 *@brief send heartbeat package to gateway low power server.
 *@param[in] type: 0: not in doorbell event, 1: in doorbell event.
 *@return -1: send failed or not receive heartbet from cloud, 1: send succeed.
 */
static int send_hb_package_to_gw(int type)
{
    if (g_keepalive_ctl.keepalive_type != KEEPALIVE_TO_GW) {
        PR_ERR("invalid keepalive type");
        return -1;
    }
    if (g_keepalive_ctl.host_state != HOST_SLEPT) {
        PR_ERR("not in sleep mode");
        return -1;
    }
    if (g_keepalive_ctl.keepalive_sock_fd == -1) {
        PR_ERR("socket closed cant send hb");
        return -1;
    }
    int i;
    int ret;
    int offset = 0;
    unsigned int check_sum = 0;
    uint8_t encrypted_buffer[ENCRYPTED_DATA_LENGTH] = {0};
    uint8_t send_data[HB_PACK_LENGTH] = {'t', 'y'};
    offset += 2;
    uint8_t iv[16] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'A', 'B', 'C', 'D', 'E', 'F'};
    unsigned int time_stamp = 0;
    time_stamp = ty_system_get_rtc_time_s();
    send_data[offset++] = g_keepalive_ctl.keepalive_info.version;
    ret = convert_mac_string2hex(g_keepalive_ctl.keepalive_info.send_data, send_data + offset, MAC_ADDR_LENGTH);
    if (ret == -1) {
        PR_ERR("invaild mac addr, disconnect wifi and delete ap info");
        netmgr_disconnect();
        netmgr_ap_info_delete();
        return -1;
    }
    offset += MAC_ADDR_LENGTH / 2;
    send_data[offset++] = 0;
    memcpy(send_data + offset, g_keepalive_ctl.keepalive_info.send_data, MAC_ADDR_LENGTH); // mac地址 12byte
    offset += MAC_ADDR_LENGTH;
    memcpy(send_data + offset, &time_stamp, sizeof(time_stamp)); // 时间�?4byte
    offset += sizeof(time_stamp);
    send_data[offset++] = '0'; // 门铃事件:type=0：无 type=1:�?    send_data[offset++]= type ? '1' : '0';
    send_data[offset++] = '0';
    send_data[offset++] = type ? '1' : '0';
    memset(send_data + offset, 0, RESERVED_LENGTH); // 保留 10 byte
    offset += RESERVED_LENGTH;
    memset(send_data + offset, HB_PACK_LENGTH - offset, HB_PACK_LENGTH - offset); // 填充 2字节填充pkcs7u
    aes128_cbc_encode_raw(send_data + HEAD_LENGTH, ENCRYPTED_DATA_LENGTH, g_keepalive_ctl.keepalive_info.aes_key, iv,
                          encrypted_buffer);
    memcpy(send_data + HEAD_LENGTH, encrypted_buffer, ENCRYPTED_DATA_LENGTH);
    for (i = 0; i < HB_PACK_LENGTH; i++) {
        check_sum += send_data[i];
    }
    send_data[HEAD_LENGTH - 1] = check_sum % 256;
    PR_INFO("TCP client write start");
    ret = send(g_keepalive_ctl.keepalive_sock_fd, send_data, HB_PACK_LENGTH, 0);
    if (HB_PACK_LENGTH != ret) {
        PR_INFO("send heart beat data error: %d\n", ret);
        return -1;
    } else {
        PR_INFO("TCP client write:ret = %d", HB_PACK_LENGTH);
        // lpmgr_show_power_mode();
    }
    return 0;
}

/**
 *@brief receive the package from the gateway, when there are sth selected.
 *@param None.
 *@return package type: see KEEPALIVE_RECV_PKG_TYPE_E.
 */
static KEEPALIVE_RECV_PKG_TYPE_E tuya_recv_from_gw_and_parse(void)
{
    if (g_keepalive_ctl.keepalive_type != KEEPALIVE_TO_GW) {
        PR_ERR("invalid keepalive type");
        return SOCKET_ERROR;
    }

    int rcv_len = 0;
    char rcv_buf[128];
    int size = sizeof(rcv_buf);
    rcv_len = recv(g_keepalive_ctl.keepalive_sock_fd, rcv_buf, size, 0);
    printf("-----------------------recv data length:%d--------------------------\n", rcv_len);
    if (rcv_len < 0) {
        PR_ERR("-----------------error!%d:  %s", errno, strerror(errno));
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
            return GW_NO_RESPONSE;
        } else {
            printf("connect status :%d", netmgr_network_available());
            if (!lpmgr_is_registered(TY_LP_NETCFG)) {
                lpmgr_register(TY_LP_NETCFG);
            }
            __start_reconnect();
            tkl_system_sleep(1000);
            return SOCKET_ERROR;
        }
    } else if (rcv_len == 0) {
        printf("socket closed\n");
        tkl_system_sleep(1000);
        return SOCKET_ERROR;
    }
    PR_INFO("-----------------------raw data-------------------------\n");
    int i, check_sum;
    for (i = 0; i < rcv_len; i++) {
        printf("%02x", rcv_buf[i]);
    }
    printf("\n");
    if (rcv_len < RECV_PACK_LENGTH) {
        printf("invaild data length\n");
    } else if (rcv_buf[0] != 't' || rcv_buf[1] != 'y') {
        printf("invaild data head\n");
    } else {
        check_sum = 0;
        for (i = 0; i < RECV_PACK_LENGTH; i++) {
            check_sum += rcv_buf[i];
        }
        check_sum -= rcv_buf[RECV_PACK_HEAD_LENGTH - 1];
        if (rcv_buf[RECV_PACK_HEAD_LENGTH - 1] != (check_sum % 256)) {
            printf("checksum error\n");
        } else {
            uint8_t decrypted_data[RECV_PACK_ENCRYPTED_LENGTH] = {0};
            uint8_t to_decrypted_data[RECV_PACK_ENCRYPTED_LENGTH] = {0};
            uint8_t gw_iv[16] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'A', 'B', 'C', 'D', 'E', 'F'};
            memcpy(to_decrypted_data, rcv_buf + RECV_PACK_HEAD_LENGTH, RECV_PACK_ENCRYPTED_LENGTH);
            aes128_cbc_decode_raw(to_decrypted_data, RECV_PACK_ENCRYPTED_LENGTH, g_keepalive_ctl.keepalive_info.aes_key,
                                  gw_iv, decrypted_data);
            if (decrypted_data[0] == '0' && decrypted_data[1] == '3') {
                return WAKEUP_PKG;
            } else if (decrypted_data[0] == '0' && decrypted_data[1] == '2') {
                unsigned long long recv_time = 0;
                unsigned int rtc_set_time = 0;
                memcpy(&recv_time, decrypted_data + 2, 8);
                rtc_set_time = (unsigned int)(recv_time / ((unsigned long long)(1000)));
                ty_system_set_rtc_time_s(rtc_set_time);
                return RTC_SET_PKG;
            } else {
                printf("unknow type\n");
            }
        }
    }
    printf("invaild package\n");
    return INVAILD_PKG;
}

/**
 *@brief send a heartbeat package with doorbell event, this is a callback for doorbell event.
 *@param[in] type: 2: doorbell pressed, 3: doorbell released.
 *@param[in] duration: not used here.
 *@return None.
 */
static int tuya_keepalive_doorbell_cb(int type, int duration)
{
    PR_INFO("state :%d", g_keepalive_ctl.host_state);
    if (g_keepalive_ctl.inited && g_keepalive_ctl.host_state == HOST_SLEPT) {
        if (type == BTN_PRESSED && g_keepalive_ctl.keepalive_type == KEEPALIVE_TO_GW) {
            PR_INFO("send hb with doorbell");
            send_hb_package_to_gw(1);
        }
        if (type == BTN_RELEASED) {
            PR_TRACE("Doorbell relased after:%d", duration);
        }
    }
    return 0;
}

/**
 *@brief enter sleep mode, register callbacks and restore keepalive related infomation.
 *@param[in] server, port, etc.: keepalive related infomation.
 *@return KEEPALIVE_NOT_INITED: fail to enter sleep mode, RET_OK: enter sleep mode succeed.
 */
int tuya_device_sleep_establish_keepalive_to_gw(unsigned int server, int port, char *send_data, char *recv_data,
                                                uint8_t *pkey, int version)
{
    if (g_keepalive_ctl.inited == 0) {
        PR_ERR("keepalive not inited");
        return KEEPALIVE_NOT_INITED;
    }
    if (send_data == NULL || recv_data == NULL || pkey == NULL) {
        PR_ERR("invalid input");
        return KEEPALIVE_INVALID_PARAMETER;
    }
    tkl_mutex_lock(g_keepalive_ctl.mutex);
    g_keepalive_ctl.keepalive_type = KEEPALIVE_TO_GW;
    g_keepalive_ctl.keepalive_pkg_interval = 60;
    g_keepalive_ctl.keepalive_info.server_ip = server;
    g_keepalive_ctl.keepalive_info.port = port;
    memcpy(g_keepalive_ctl.keepalive_info.send_data, send_data, 12);
    memcpy(g_keepalive_ctl.keepalive_info.recv_data, recv_data, 12);
    memcpy(g_keepalive_ctl.keepalive_info.aes_key, pkey, 16);
    g_keepalive_ctl.keepalive_info.version = version & 0xFF;
    // ty_system_keep_alive_doorbell_cb_register(tuya_keepalive_doorbell_cb);
    g_keepalive_ctl.first_loop = 1;
    g_keepalive_ctl.timer_func = 0; // 默认下次断开wifi
    g_keepalive_ctl.reconn_time = 0;
    printf("change period result:%d\n", tal_sw_timer_start(g_keepalive_ctl.reconn_timer, 60 * 1000, TAL_TIMER_ONCE));
    tkl_mutex_unlock(g_keepalive_ctl.mutex);
    tuya_set_devive_sleep();
    if (!lpmgr_is_registered(TY_LP_NETCFG)) {
        lpmgr_register(TY_LP_NETCFG);
    }
    return RET_OK;
}

/**
 *@brief enter sleep mode.
 *@param[in] server, port, etc.: keepalive related infomation.
 *@return KEEPALIVE_NOT_INITED: fail to enter sleep mode, RET_OK: enter sleep mode succeed.
 */
int tuya_device_sleep_keepalive_with_mqtt(void)
{
    if (ty_system_get_device_mode() != 2) {
        return KEEPALIVE_TYPE_ERROR;
    }

    tkl_mutex_lock(g_keepalive_ctl.mutex);
    if (g_keepalive_ctl.host_state == HOST_SLEPT) {
        tkl_mutex_unlock(g_keepalive_ctl.mutex);
        return KEEPALIVE_ALREADY_SLEPT;
    }
    g_keepalive_ctl.keepalive_type = KEEPALIVE_TO_CLOUD;
    tkl_mutex_unlock(g_keepalive_ctl.mutex);
    tuya_set_devive_sleep();
    if (true != ty_system_device_is_actived()) {
        netmgr_reconnect_ctrl(false);
        netmgr_disconnect();
    }
    return RET_OK;
}

/**
 *@brief the keepalive task function, used for connect router/ low power server, send heartbeat package, receive and
 * parse package.
 *@param[in] not_used: not used.
 *@return None.
 */
static void tuya_keepalive_task(void *not_used)
{
    /* To avoid gcc warnings */
    (void)not_used;
    g_keepalive_ctl.first_loop =
        1; // 判断是否休眠后首次进入保�?避免第一次因为T31重连1分钟进保�?682再重�?分钟，表现重�?分钟的BUG
    int connect_lps_count = 0; // 连接lowpower server计次
    int timer_state = 0;
    // device offline management
    while (g_keepalive_ctl.inited) {
        if (g_keepalive_ctl.host_state != HOST_SLEPT) {
            tkl_system_sleep(200);
            continue;
        }
        if (g_keepalive_ctl.keepalive_sock_fd >= 0) {
            close(g_keepalive_ctl.keepalive_sock_fd);
            g_keepalive_ctl.keepalive_sock_fd = -1;
        }

        timer_state = tal_sw_timer_is_running(g_keepalive_ctl.reconn_timer) ? 1 : 0;
        if (timer_state == 0) {
            printf("start reconnect timer\n");
            g_keepalive_ctl.timer_func = 0; // 恢复默认配置
            if (tal_sw_timer_start(g_keepalive_ctl.reconn_timer, 60 * 1000, TAL_TIMER_ONCE) != OPRT_OK) {
                PR_ERR("reconn_timer xTimerStart fail\n");
            }
        }
        if (ty_system_check_wifi_connected() ==
            1) { // 连接上了WIFI，尝试连接低功耗服务器，连上后收发保活�?            g_keepalive_ctl.first_loop = 0;
            u32_t ip;

            ip = g_keepalive_ctl.keepalive_info.server_ip;
            printf("connecting to lowpower server\n");
            g_keepalive_ctl.keepalive_sock_fd = tuya_connect_gw(ip);
            if (g_keepalive_ctl.keepalive_sock_fd == -1) {
                printf("connect failed");
                connect_lps_count++;
                if (connect_lps_count >= 5) {
                    connect_lps_count = 0;
                    printf("failed 4 times, reconnect wifi\n");
                    tuya_reconnect_router();
                }
                tkl_system_sleep(1000);
                continue;
            } else { // 连上低功耗服务器，重置重连时间，关闭定时�?                connect_lps_count = 0;
                if (OPRT_OK != tal_sw_timer_stop(g_keepalive_ctl.reconn_timer)) {
                    PR_ERR("reconn_timer xTimerStop fail\n");
                }
                if (lpmgr_is_registered(TY_LP_NETCFG)) {
                    lpmgr_unregister(TY_LP_NETCFG);
                }
                g_keepalive_ctl.reconn_time = 0;
            }
        } else {                              // 没连上wifi，尝试连接wifi（按照重连时间）
            if (g_keepalive_ctl.first_loop) { // first_loop�?，直接关闭wifi
                g_keepalive_ctl.first_loop = 0;
                g_keepalive_ctl.timer_func =
                    1; // 关闭了wifi，相当于跳过一次重�? __get_next_sleep_count(&g_keepalive_ctl.reconn_time);
                netmgr_disconnect();
                // wifi_rf_off();
                if (lpmgr_is_registered(TY_LP_NETCFG)) {
                    lpmgr_unregister(TY_LP_NETCFG);
                }
                printf("turn off wifi, for %ds\n", 60);
            }
            tkl_system_sleep(1000);
            continue;
        }
        // 连上低功耗服务器后发送保活包
        if (OPRT_OK != send_hb_package_to_gw(0)) {
            continue;
        }
        while (g_keepalive_ctl.host_state ==
               HOST_SLEPT) { // 保活中收发保活包，若wifi或者低功耗服务器断开，则break后重�?            fd_set rfds;
            struct timeval tv;
            int retval, maxfd = -1;
            FD_ZERO(&rfds);
            FD_SET(g_keepalive_ctl.keepalive_sock_fd, &rfds);
            maxfd = g_keepalive_ctl.keepalive_sock_fd;
            tv.tv_sec = g_keepalive_ctl.keepalive_pkg_interval;
            tv.tv_usec = 0;

            retval = select(maxfd + 1, &rfds, NULL, NULL,
                            &tv); // select实现发送间隔控�?            printf("select result:%d\n", retval);
            if (retval < 0) {     // select 异常，退出重连低功耗服务器
                printf("Will exit and the select is error! %s", strerror(errno));
                printf("connect status :%d", netmgr_network_available());
                if (!lpmgr_is_registered(TY_LP_NETCFG)) {
                    lpmgr_register(TY_LP_NETCFG);
                }
                __start_reconnect();
                break;
            } else if (retval == 0) { // select超时，发送保活包
                if (OPRT_OK != send_hb_package_to_gw(0)) {
                    break;
                }
            } else { // select�?，收到数据或者socket异常
                PR_DEBUG("-----------------select result:  %s", strerror(errno));
                if (FD_ISSET(g_keepalive_ctl.keepalive_sock_fd, &rfds)) {
                    if (g_keepalive_ctl.host_state != HOST_SLEPT) { // 已经从保活中唤醒
                        break;
                    }
                    printf("============recv data==============\n");

                    retval = tuya_recv_from_gw_and_parse(); // 接收解析数据
                    // printf("return form recv retval:%d\n", retval);
                    // vTaskDelay(10000 / portTICK_PERIOD_MS);
                    if (retval == WAKEUP_PKG) { // 拉流唤醒
                        tuya_wakeup_reason_handler(WAKEUP_GW);
                        PR_ERR("wake up by user");
                        break;
                    } else if (retval == HB_RESPONSE_PKG) { // 收到云端发回的保活包, 清空计数，计数超�?重连服务�?
                                                            // printf("recv heart beat data\n");
                    } else if (retval == RTC_SET_PKG) {
                        printf("recv and set utc time success\n");
                    } else if (retval == INVAILD_PKG) {
                        printf("invalid package\n");
                    } else if (retval == GW_NO_RESPONSE) {
                        printf("no response\n");
                        tkl_system_sleep(500);
                    } else if (retval == SOCKET_ERROR) {
                        printf("socket error package, reconnect wifi\n");
                        break;
                    } else {
                        printf("unknow result, reconnect wifi\n");
                        break;
                    }
                } else { // socket不再select的范围中，重连服务器，重新select
                    printf("unknown socket(%d)\n", g_keepalive_ctl.keepalive_sock_fd);
                    break;
                }
                tkl_system_sleep(500);
            }
        }
        if (g_keepalive_ctl.keepalive_type == KEEPALIVE_TO_GW &&
            g_keepalive_ctl.keepalive_sock_fd >= 0) { // 和基站保活需要在唤醒时关闭socket，和云端不用（否则会导致设备下线�?
                                                      // close(g_keepalive_ctl.keepalive_sock_fd);
            g_keepalive_ctl.keepalive_sock_fd = -1;
            printf("close socket\n");
        }
    }

    g_keepalive_ctl.keep_alive_task_handle = NULL;
    tkl_thread_release(NULL);
}

/**
 *@brief start keepalive task.
 *@param[in] type: the object to keepalive, cloud or gateway lowpower server.
 *@return -1: failed, 0: succeed.
 */
int tuya_keepalive_init(KEEPALIVE_TYPE_E type)
{
    int ret = 0;
    if (type != KEEPALIVE_TO_GW && type != KEEPALIVE_TO_CLOUD) {
        PR_ERR("invaild keepalive type");
        return -1;
    }

    if (g_keepalive_ctl.inited == 0) {
        memset(&g_keepalive_ctl, 0, sizeof(g_keepalive_ctl));
        g_keepalive_ctl.inited = 1;
        g_keepalive_ctl.keepalive_sock_fd = -1;

        memset(&g_keepalive_ctl.mutex, 0, sizeof(g_keepalive_ctl.mutex));
        tkl_mutex_create_init(&g_keepalive_ctl.mutex);
        ret = tal_sw_timer_create(reconn_timer_callback, NULL, &g_keepalive_ctl.reconn_timer);
        if (OPRT_OK != ret) {
            PR_ERR("failed in creat pir timer.");
            return ret;
        }
        g_keepalive_ctl.host_state = HOST_SLEPT;
        if (type != KEEPALIVE_TO_CLOUD) {
            tkl_thread_create(&g_keepalive_ctl.keep_alive_task_handle, "keep_alive_task", TUYA_ALIVE_TASK_STACK_SIZE,
                              TUYA_ALIVE_TASK_PRIO, tuya_keepalive_task, NULL);
            ty_system_doorbell_keepalive_cb_register(tuya_keepalive_doorbell_cb);
            netmgr_reconnect_ctrl(false);
        }
    } else {
        PR_ERR("keepalive already initialized!");
        return -1;
    }
    return 0;
}

/**
 *@brief delete keepalive task.
 *@param None.
 *@return None.
 */
void tuya_keepalive_uninit(void)
{
    g_keepalive_ctl.inited = 0;
    return;
}

/**
 *@param  poweroff type 0:poweron 1: button poweroff(wakeup by button) 2:lowpower poweroff(wakeup by charge pin).
 *@brief enter soft poweroff mode.
 *@return None.
 */
void tuya_keepalive_poweroff(POWER_STATE_TYPE_E type)
{
    unsigned char val = (unsigned char)type;
    int ret = 0;
    PR_INFO("receive set Deep sleep cmd [%d]\n", type);
    mm_power_off();
    if (kvs_write(POWEROFF_FLAG, &val, 1) != OPRT_OK) {
        PR_ERR("kvs_write err %d", ret);
    }
    tkl_cpu_sleep_mode_set(TRUE, TUYA_CPU_DEEP_SLEEP);
    return;
}

POWER_STATE_TYPE_E tuya_keepalive_get_power_state(void)
{
    int ret = 0;
    POWER_STATE_TYPE_E power_state = 0;
    unsigned char *data = NULL;
    int len = 0;
    if (OPRT_OK != kvs_read(POWEROFF_FLAG, &data, &len)) {
        PR_ERR("kvs_read err");
        return 0;
    } else {
        power_state = *data;
        PR_INFO("kvs_read power state = %d", power_state);
        free(data);
    }
    return power_state;
}

int tuya_keepalive_poweron(void)
{
    unsigned char val = 0;
    int ret = 0;
    PR_INFO("poweron from poweroff\n");
    ret = kvs_write(POWEROFF_FLAG, &val, 1);
    if (ret != OPRT_OK) {
        PR_ERR("kvs_write err %d", ret);
    }
    return ret;
}

int tuya_keepalive_get_host_state(void)
{
    return g_keepalive_ctl.host_state;
}

int tuya_keepalive_is_host_sleep(void)
{
    int ret = FALSE;
    if (g_keepalive_ctl.host_state == HOST_SLEPT) {
        ret = TRUE;
    }
    return ret;
}

void tuya_keepalive_set_host_ready(void)
{
    tkl_mutex_lock(g_keepalive_ctl.mutex);
    if (g_keepalive_ctl.host_state != HOST_WAKING) {
        PR_ERR("error host state: %d", g_keepalive_ctl.host_state);
    }
    g_keepalive_ctl.host_state = HOST_READY;
    tkl_mutex_unlock(g_keepalive_ctl.mutex);
    return;
}

#endif /* #if defined(MM_ENABLE_MQTT_KEEPALIVE) */
