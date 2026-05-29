#include "tuya_iot_config.h"
#if defined(SYSTEM_LINUX) && (OPERATING_SYSTEM != SYSTEM_LINUX)
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "tuya_log.h"
#include "mm_cmd_parse.h"
#include "tal_time_service.h"
#if defined(MM_ENABLE_MQTT_KEEPALIVE)
#include "mm_mqtt.h"
// #include "uni_time.h"
#include "mm_keepalive.h"
#endif
#include "mm_spi.h"
#include "tal_sw_timer.h"
#include "mm_icc.h"
#include "tkl_system.h"
#include "tkl_thread.h"
#include "tkl_wifi.h"
#include "app_adapter_mgr.h"
#include "app_adv_mgr.h"
#include "tkl_bluetooth.h"
#include "ble_conn.h"
#include "tuya_ble_hal.h"

#define MAX_PID_LEN (32 + 1)
#define MAX_UUID_LEN (25 + 1)
#define MAX_AUTHKEY_LEN (32 + 1)
#define MAX_CONTRYCD_LEN (8 + 1)
#define MAX_CMD_PAYLOAD_LEN (MAX_ICC_DATA_LEN - sizeof(TY_LP_PROTO_CMD_HEAD_T))
/*MQTT Reserve 350 bytes for other fields and json overhead */
#define MQTT_DATA_SLICE_LEN (MAX_CMD_PAYLOAD_LEN - 350)
#if defined ICC_USE_USB
#undef TUYA_SPI_RETRY
#endif
#define MQTT_DATA_MAX_LEN       (1024 * 8)
#if defined(TUYA_SPI_RETRY) && (TUYA_SPI_RETRY == 1)
#include "tkl_mutex.h"
#include "tkl_semaphore.h"

#define TY_MAX_TX_NODE 30
#define TY_MAX_RX_NODE 15
#define TY_NOT_USED 0
#define TY_USED 1
#define TY_SEND 2
#define TY_SPI_RETRY_TIME_OUT 1000

typedef struct
{
    int stat;
    int cmd;
    int frame_num;
    int data_len;
    unsigned int crc;
    char *data;
} TY_LP_PROTO_RX_NODE_T;

typedef struct
{
    int stat;               // 0 not data make sure data == NULL; 1 have data ;2 have data and send
    unsigned int ticks;     // tick for send
    unsigned int org_ticks; // tick for send
    int cmd;
    int data_len;
    int frame_num;
    char *data;
} TY_LP_PROTO_TX_NODE_T;

TY_LP_PROTO_TX_NODE_T g_spi_tx_list[TY_MAX_TX_NODE] = {0};
TY_LP_PROTO_RX_NODE_T g_spi_rx_list[TY_MAX_RX_NODE] = {0};
static TKL_MUTEX_HANDLE s_rx_list_mutex;
static TKL_MUTEX_HANDLE s_tx_list_mutex;
static TKL_SEM_HANDLE s_cmd_tx_sem;
static int s_frame_num = 0;
static int g_max_proc_fn = 0;             // 已处理的最大序号
static int g_max_fn_in_buffer = 0;        // 缓存的最大序号
static unsigned int g_last_proc_time = 0; // 上次命令处理时间
static int __rx_list_get_node(TY_LP_PROTO_RX_NODE_T *node, int frame_num);
static int __rx_list_insert(char *data, int data_len, int cmd, int frame_num, unsigned int crc);
static int __wait_list_insert(char *data, int data_len, int cmd, int frame_num);
static int __wait_list_delete(int cmd, int frame_num);
static int __wait_list_get_need_send(TY_LP_PROTO_TX_NODE_T *node);
static int ty_lp_proto_handle_cmd_in_buffer(void);
static int __ty_tx_list_lock(void);
static int __ty_tx_list_unlock(void);
#endif

static int ty_lp_proto_cmd_handle(int cmd, cJSON *pRoot);

void rx_status_reset(void);

void __cmd_parse_print_debug(cJSON *param)
{
    return;
    char *tmp_str = cJSON_PrintUnformatted(param);
    if (tmp_str)
    {
        cJSON_free(tmp_str);
    }
    return;
}

static const char *__cmd_name(int cmd)
{
    switch (cmd) {
    case SPI_T_WIFI_SCAN:                  return "WIFI_SCAN";
    case SPI_M_WIFI_SCAN_START:            return "WIFI_SCAN_START";
    case SPI_M_WIFI_SCAN:                  return "WIFI_SCAN";
    case SPI_M_WIFI_SCAN_END:              return "WIFI_SCAN_END";
    case SPI_T_WIFI_CONN:                  return "WIFI_CONN";
    case SPI_T_WIFI_DISCONN:               return "WIFI_DISCONN";
    case SPI_T_SET_MAC_ADDR:               return "SET_MAC_ADDR";
    case SPI_T_SET_IP_ADDR:                return "SET_IP_ADDR";
    case SPI_T_SET_ACTIVE_STAT:            return "SET_ACTIVE_STAT";
    case SPI_T_SET_DEEP_SLEEP:             return "SET_DEEP_SLEEP";
    case SPI_T_GET_WIFI_CALIBRATION_VALUE: return "GET_WIFI_CALIBRATION_VALUE";
    case SPI_T_SET_WIFI_MODE:              return "SET_WIFI_MODE";
    case SPI_T_GET_WIFI_MODE:              return "GET_WIFI_MODE";
    case SPI_M_SET_WIFI_MODE:              return "SET_WIFI_MODE";
    case SPI_T_SET_WIFI_CHANNEL:           return "SET_WIFI_CHANNEL";
    case SPI_T_GET_WIFI_CHANNEL:           return "GET_WIFI_CHANNEL";
    case SPI_M_SET_WIFI_CHANNEL:           return "SET_WIFI_CHANNEL";
    case SPI_T_GET_WIFI_RSSI:              return "GET_WIFI_RSSI";
    case SPI_M_SET_WIFI_RSSI:              return "SET_WIFI_RSSI";
    case SPI_T_GET_WIFI_CONN_STAT:         return "GET_WIFI_CONN_STAT";
    case SPI_M_SET_WIFI_CONN_STAT:         return "SET_WIFI_CONN_STAT";
    case SPI_T_WIFI_START_AP:              return "WIFI_START_AP";
    case SPI_T_INTO_SLEEP_V2:              return "INTO_SLEEP_V2";
    case SPI_M_GET_MAC_ADDR:               return "GET_MAC_ADDR";
    case SPI_T_GET_WIFI_MAC:               return "GET_WIFI_MAC";
    case SPI_T_SET_WIFI_COUNTRY:           return "SET_WIFI_COUNTRY";
    case SPI_T_GET_WIFI_IP:                return "GET_WIFI_IP";
    case SPI_M_GET_WIFI_IP:                return "GET_WIFI_IP_RSP";
    case SPI_T_WIFI_INIT:                  return "WIFI_INIT";
    case SPI_M_WIFI_CON_FINISH:            return "WIFI_CON_FINISH";
    case SPI_T_BLE_STACK_INIT:             return "BLE_STACK_INIT";
    case SPI_T_BLE_STACK_DEINIT:           return "BLE_STACK_DEINIT";
    case SPI_T_BLE_GAP_ADV_START:          return "BLE_GAP_ADV_START";
    case SPI_T_BLE_GAP_ADV_STOP:           return "BLE_GAP_ADV_STOP";
    case SPI_T_BLE_GAP_ADV_DATA_SET:       return "BLE_GAP_ADV_DATA_SET";
    case SPI_T_BLE_GAP_ADV_DATA_UPDATE:    return "BLE_GAP_ADV_DATA_UPDATE";
    case SPI_T_BLE_GAP_ADDR_SET:           return "BLE_GAP_ADDR_SET";
    case SPI_T_BLE_GAP_ADDR_GET:           return "BLE_GAP_ADDR_GET";
    case SPI_T_BLE_GAP_DISCONNECT:         return "BLE_GAP_DISCONNECT";
    case SPI_T_BLE_GAP_CONN_PARAM_UPDATE:  return "BLE_GAP_CONN_PARAM_UPDATE";
    case SPI_M_BLE_GAP_CONNECT_EVT:        return "BLE_GAP_CONNECT_EVT";
    case SPI_M_BLE_GAP_DISCONNECT_EVT:     return "BLE_GAP_DISCONNECT_EVT";
    case SPI_T_BLE_GAP_NAME_SET:           return "BLE_GAP_NAME_SET";
    case SPI_T_BLE_GAP_TX_POWER_SET:       return "BLE_GAP_TX_POWER_SET";
    case SPI_T_BLE_GAP_RSSI_GET:           return "BLE_GAP_RSSI_GET";
    case SPI_T_BLE_GATTS_SERVICE_ADD:      return "BLE_GATTS_SERVICE_ADD";
    case SPI_T_BLE_GATTS_VALUE_SET:        return "BLE_GATTS_VALUE_SET";
    case SPI_T_BLE_GATTS_VALUE_GET:        return "BLE_GATTS_VALUE_GET";
    case SPI_T_BLE_GATTS_VALUE_NOTIFY:     return "BLE_GATTS_VALUE_NOTIFY";
    case SPI_T_BLE_GATTS_VALUE_INDICATE:   return "BLE_GATTS_VALUE_INDICATE";
    case SPI_T_BLE_GATTS_MTU_REPLY:        return "BLE_GATTS_MTU_REPLY";
    case SPI_M_BLE_GATTS_WRITE_EVT:        return "BLE_GATTS_WRITE_EVT";
    case SPI_M_BLE_GATTS_MTU_EVT:          return "BLE_GATTS_MTU_EVT";
    case SPI_M_BLE_GATTS_READ_EVT:         return "BLE_GATTS_READ_EVT";
    case SPI_M_BLE_GAP_CONN_PARAM_EVT:     return "BLE_GAP_CONN_PARAM_EVT";
    case SPI_M_BLE_GATTS_NOTIFY_COMPLETE:  return "BLE_GATTS_NOTIFY_COMPLETE";
    case SPI_T_BLE_GAP_SCAN_START:         return "BLE_GAP_SCAN_START";
    case SPI_T_BLE_GAP_SCAN_STOP:          return "BLE_GAP_SCAN_STOP";
    case SPI_T_BLE_GAP_CONNECT:            return "BLE_GAP_CONNECT";
    case SPI_T_BLE_GATTC_SVC_DISCOVER:     return "BLE_GATTC_SVC_DISCOVER";
    case SPI_T_BLE_GATTC_CHR_DISCOVER:     return "BLE_GATTC_CHR_DISCOVER";
    case SPI_T_BLE_GATTC_DESC_DISCOVER:    return "BLE_GATTC_DESC_DISCOVER";
    case SPI_T_BLE_GATTC_READ:             return "BLE_GATTC_READ";
    case SPI_T_BLE_GATTC_WRITE:            return "BLE_GATTC_WRITE";
    case SPI_T_BLE_GATTC_WRITE_NO_RSP:     return "BLE_GATTC_WRITE_NO_RSP";
    case SPI_T_BLE_GATTC_MTU_REQ:          return "BLE_GATTC_MTU_REQ";
    default: return "UNKNOWN";
    }
}

int __lp_proto_json_pack(cJSON *root, TY_LP_PROTO_JSON_HEAD_T *pHead)
{
    cJSON_AddItemToObject(root, "cmd", cJSON_CreateNumber(pHead->cmd));
    cJSON_AddItemToObject(root, "type", cJSON_CreateNumber(pHead->type));
    cJSON_AddItemToObject(root, "errorCode", cJSON_CreateNumber(pHead->errorcode));
    cJSON_AddItemToObject(root, "t", cJSON_CreateNumber(pHead->time));
    return 0;
}

int __lp_proto_data_pack_and_send(TY_LP_PROTO_CMD_HEAD_T *pHead, char *data, int len)
{
    char *sendBuff = NULL;
    int sendLen = 0;
    sendBuff = (char *)tkl_system_malloc(sizeof(TY_LP_PROTO_CMD_HEAD_T) + len + 1);
    if (NULL == sendBuff)
    {
        PR_ERR("Malloc failed");
        return MM_ERR_MALLOC_FAIL;
    }
    pHead->payLoadLen = (unsigned int)len;  /* fix: must fill payLoadLen before sending */
    memcpy(sendBuff, pHead, sizeof(TY_LP_PROTO_CMD_HEAD_T));
    memcpy(sendBuff + sizeof(TY_LP_PROTO_CMD_HEAD_T), data, len);
    sendLen = sizeof(TY_LP_PROTO_CMD_HEAD_T) + len;
    sendBuff[sendLen] = '\0';
    // todo send data
    {
        int i = 0;
        unsigned int crc_cs = 0;
        for (i = 0; i < sendLen; i++)
        {
            crc_cs += ((unsigned char *)sendBuff)[i];
        }
        cmd_to_spi((unsigned char *)sendBuff, sendLen, crc_cs);
    }
    tkl_system_free(sendBuff);
    return 0;
}

int __lp_proto_json_com_pack(TY_LP_PROTO_JSON_COM_E cmd, cJSON *param)
{
    int ret = 0;

    cJSON *root = cJSON_CreateObject();
    if (NULL == root)
    {
        PR_ERR("cJSON_CreateObject failed");
        return MM_ERR_MALLOC_FAIL;
    }
    TY_LP_PROTO_JSON_HEAD_T strJsonHead = {0};
    strJsonHead.cmd = cmd;
    strJsonHead.type = 0;
    strJsonHead.errorcode = 0;
#if defined(TUYA_SPI_RETRY) && (TUYA_SPI_RETRY == 1)
    if (1
#if defined(MM_ENABLE_MQTT_KEEPALIVE)
        && cmd != LP_PROTO_O_MQTT_RECV_DATA_ASYNC && cmd != LP_PROTO_O_MQTT_SEND_DATA_ASYNC
#endif
    )
    {
        __ty_tx_list_lock();
        strJsonHead.time = (unsigned long long)++s_frame_num;
    }
#endif
    __lp_proto_json_pack(root, &strJsonHead);
    if (param)
    {
        cJSON_AddItemToObject(root, "param", param);
    }

    char *jsonTmp = cJSON_PrintUnformatted(root);
    if (NULL == jsonTmp)
    {
        PR_ERR("cJSON_Print failed");
        cJSON_Delete(root);
#if defined(TUYA_SPI_RETRY) && (TUYA_SPI_RETRY == 1)
        __ty_tx_list_unlock();
#endif
        return MM_ERR_MALLOC_FAIL;
    }
    size_t cmd_length = strnlen(jsonTmp, 1600);
#if defined(TUYA_SPI_RETRY) && (TUYA_SPI_RETRY == 1)
    if (1
#if defined(MM_ENABLE_MQTT_KEEPALIVE)
        && cmd != LP_PROTO_O_MQTT_RECV_DATA_ASYNC && cmd != LP_PROTO_O_MQTT_SEND_DATA_ASYNC
#endif
    )
    {
        __wait_list_insert(jsonTmp, cmd_length, cmd, (int)strJsonHead.time);
        __ty_tx_list_unlock();
    }
    else
    {
#endif
        TY_LP_PROTO_CMD_HEAD_T strHead = {0};
        strHead.mark = LP_PROTO_HEAD_MARK;
        strHead.version = LP_PROTO_VERSION;
        strHead.payLoadType = TY_LP_PROTO_TYPE_JSON;
        ret = __lp_proto_data_pack_and_send(&strHead, jsonTmp, strnlen(jsonTmp, 1600));
        if (ret < 0)
        {
            PR_ERR("pack failed ret = %d", ret);
        }
#if defined(TUYA_SPI_RETRY) && (TUYA_SPI_RETRY == 1)
    }
#endif
    cJSON_free(jsonTmp);
    cJSON_Delete(root);
    return ret;
}

int __lp_proto_cs_send_ack(int cmd, unsigned long long frame_num, cJSON *param, int errorcode)
{
    int ret = 0;
    TY_LP_PROTO_CMD_HEAD_T strHead = {0};

    cJSON *root = cJSON_CreateObject();
    if (NULL == root)
    {
        PR_ERR("cJSON_CreateObject failed");
        return MM_ERR_MALLOC_FAIL;
    }
    TY_LP_PROTO_JSON_HEAD_T strJsonHead = {0};
    strJsonHead.cmd = cmd;
    strJsonHead.type = 1;
    strJsonHead.errorcode = errorcode;
    strJsonHead.time = frame_num;
    __lp_proto_json_pack(root, &strJsonHead);
    if (param)
    {
        cJSON_AddItemToObject(root, "param", param);
    }

    char *jsonTmp = cJSON_PrintUnformatted(root);
    if (NULL == jsonTmp)
    {
        PR_ERR("cJSON_Print failed");
        cJSON_Delete(root);
        return MM_ERR_MALLOC_FAIL;
    }
    strHead.mark = LP_PROTO_HEAD_MARK;
    strHead.version = LP_PROTO_VERSION;
    strHead.payLoadType = TY_LP_PROTO_TYPE_JSON;
    ret = __lp_proto_data_pack_and_send(&strHead, jsonTmp, strnlen(jsonTmp, 1600));
    if (ret < 0)
    {
        PR_ERR("pack failed ret = %d", ret);
    }
    cJSON_free(jsonTmp);
    cJSON_Delete(root);
    return ret;
}

#if defined(MM_ENABLE_MQTT_KEEPALIVE)
void pack_mqtt_data_web(int proto, char *data, int slice, char *topic)
{
    static char *mqtt_data = NULL;
    static int copy_len = 0;

    if (NULL == mqtt_data)
    {
        mqtt_data = (char *)tkl_system_malloc(MQTT_DATA_MAX_LEN);
        if (NULL != mqtt_data)
        {
            memset(mqtt_data, 0, MQTT_DATA_MAX_LEN);
        }
        else
        {
            PR_ERR("%s:%d failed in malloc ", __func__, __LINE__);
            return;
        }
    }
    strncpy(mqtt_data + copy_len, data, MQTT_DATA_MAX_LEN - copy_len - 1);
    copy_len = strnlen(mqtt_data, MQTT_DATA_MAX_LEN - 1);

    if (0 == slice || 3 == slice || copy_len >= MQTT_DATA_MAX_LEN - 1)
    {
        // send data to mqtt.
        if (0 == slice || 3 == slice)
        {
            // send data to mqtt.
            mm_mqtt_post_data(mqtt_data, proto, topic);
        }
        else
        {
            PR_ERR("%s:%d pack fail copy_len%d", __func__, __LINE__, copy_len);
        }

        tkl_system_free(mqtt_data);
        mqtt_data = NULL;
        copy_len = 0;
    }

    return;
}

void pack_mqtt_data_app(int proto, char *data, int slice, char *topic)
{
    static char *mqtt_data = NULL;
    static int copy_len = 0;

    if (NULL == mqtt_data)
    {
        mqtt_data = (char *)tkl_system_malloc(MQTT_DATA_MAX_LEN);
        if (NULL != mqtt_data)
        {
            memset(mqtt_data, 0, MQTT_DATA_MAX_LEN);
        }
        else
        {
            PR_ERR("%s:%d failed in malloc ", __func__, __LINE__);
            return;
        }
    }
    strncpy(mqtt_data + copy_len, data, MQTT_DATA_MAX_LEN - copy_len - 1);
    copy_len = strnlen(mqtt_data, MQTT_DATA_MAX_LEN - 1);
    if (0 == slice || 3 == slice || copy_len >= MQTT_DATA_MAX_LEN - 1)
    {
        // send data to mqtt.
        if (0 == slice || 3 == slice)
        {
            // send data to mqtt.
            mm_mqtt_post_data(mqtt_data, proto, topic);
        }
        else
        {
            PR_ERR("%s:%d pack fail copy_len%d", __func__, __LINE__, copy_len);
        }
        tkl_system_free(mqtt_data);
        mqtt_data = NULL;
        copy_len = 0;
    }

    return;
}
#endif

int __lp_proto_cs_json_parse(unsigned char *data, int len)
{
    cJSON *pRoot = NULL;
    cJSON *pCmd = NULL;
    cJSON *pType = NULL;
    cJSON *pError = NULL;
    cJSON *pTime = NULL;
    TY_LP_PROTO_JSON_HEAD_T strHead = {0};
    int ret = 0;
    pRoot = cJSON_Parse((char *)data);
    if (NULL == pRoot)
    {
        PR_ERR("error");
        return MM_ERR_GENERAL;
    }
    pCmd = cJSON_GetObjectItem(pRoot, "cmd");
    if (pCmd == NULL)
    {
        PR_ERR("parse pCmd error");
        goto exit;
    }
    pType = cJSON_GetObjectItem(pRoot, "type");
    if (pType == NULL)
    {
        PR_ERR("parse pType error");
        goto exit;
    }
    if (pType->valueint == 1)
    {
        pError = cJSON_GetObjectItem(pRoot, "errorCode");
        if (pError == NULL)
        {
            PR_ERR("parse pError error");
            goto exit;
        }
    }
    pTime = cJSON_GetObjectItem(pRoot, "t");
    if (pTime == NULL)
    {
        PR_ERR("parse pTime error");
        goto exit;
    }
    strHead.cmd = pCmd->valueint;
    if (pError)
    {
        strHead.errorcode = pError->valueint;
    }
    else
    {
        strHead.errorcode = 0;
    }
    strHead.type = pType->valueint;
    strHead.time = pTime->valuedouble;
    if (strHead.type == 0)
    {
        PR_INFO("recv cmd [%d(%s) err=%d]", strHead.cmd, __cmd_name(strHead.cmd), strHead.errorcode);
    }
    else
    {
        PR_INFO("recv ack [%d(%s) err=%d]", strHead.cmd, __cmd_name(strHead.cmd), strHead.errorcode);
    }
#if defined(TUYA_SPI_RETRY) && (TUYA_SPI_RETRY == 1)
    if (strHead.type == 1)
    {
        if (1
#if defined(MM_ENABLE_MQTT_KEEPALIVE)
            && strHead.cmd != LP_PROTO_O_MQTT_SEND_DATA_ASYNC && strHead.cmd != LP_PROTO_O_MQTT_RECV_DATA_ASYNC
#endif
        )
        {
            __wait_list_delete(strHead.cmd, (int)strHead.time);
        }
        cJSON_Delete(pRoot);
        return 0;
    }
    if (0
#if defined(MM_ENABLE_MQTT_KEEPALIVE)
        || strHead.cmd == LP_PROTO_O_MQTT_SEND_DATA_ASYNC || strHead.cmd == LP_PROTO_O_MQTT_RECV_DATA_ASYNC
#endif

    )
    {
        ret = ty_lp_proto_cmd_handle(strHead.cmd, pRoot);
        cJSON_Delete(pRoot);
        return ret;
    }
    __lp_proto_cs_send_ack(strHead.cmd, strHead.time, NULL, 0);
    if (strHead.cmd == SPI_T_SET_MAC_ADDR || strHead.cmd == LP_PROTO_S_GET_AUTHINFO)
    { // 收到下一条指令发送给应用
        tkl_system_sleep(1);
        g_last_proc_time = tkl_system_get_millisecond(); // 记录上次发送时间
        PR_INFO("parse cmd [%d-%d-%llu]", strHead.cmd, strHead.errorcode, strHead.time);
        ret = ty_lp_proto_cmd_handle(strHead.cmd, pRoot);
        cJSON_Delete(pRoot);
        return 0;
    }
    int current_frame_num = (int)strHead.time;

    if (g_max_proc_fn + 1 == current_frame_num)
    {                                                    // 收到下一条指令发送给应用
        g_last_proc_time = tkl_system_get_millisecond(); // 记录上次发送时间
        PR_INFO("parse cmd [%d-%d-%llu]", strHead.cmd, strHead.errorcode, strHead.time);
        ret = ty_lp_proto_cmd_handle(strHead.cmd, pRoot);
        cJSON_Delete(pRoot);
        g_max_proc_fn++;
        if (g_max_fn_in_buffer >= g_max_proc_fn + 1)
        {                                       // 若缓存中最大序号大于等于下一个序号，查询缓存中是否有下一个序号
            ty_lp_proto_handle_cmd_in_buffer(); // 遍历缓存，解析序号
        }
    }
    else if (g_max_proc_fn >= current_frame_num)
    { // 序号小于当前序号--重传指令已发送直接忽略
        cJSON_Delete(pRoot);
        return 0;
    }
    else
    { // 如果收到的命令序号比下一个序号大，放入缓存
        if (0 == __rx_list_insert((char *)data, len, pCmd->valueint, current_frame_num, 0))
        {
            if (g_max_fn_in_buffer <= g_max_proc_fn)
            {
                g_last_proc_time = tkl_system_get_millisecond(); // 记录最新超时缓存时间
            }
            g_max_fn_in_buffer = g_max_fn_in_buffer < current_frame_num ? current_frame_num : g_max_fn_in_buffer; // 记录缓存中最大序号
        }
        cJSON_Delete(pRoot);
    }
#else
    /* In non-retry mode: silently drop all ACKs (type==1); only dispatch requests (type==0). */
    if (strHead.type == 1 &&
        (strHead.cmd == SPI_M_BLE_GAP_CONNECT_EVT ||
        strHead.cmd == SPI_M_BLE_GAP_DISCONNECT_EVT ||
        strHead.cmd == SPI_M_BLE_GATTS_WRITE_EVT ||
        strHead.cmd == SPI_M_BLE_GATTS_MTU_EVT ||
        strHead.cmd == SPI_M_BLE_GATTS_READ_EVT ||
        strHead.cmd == SPI_M_BLE_GAP_CONN_PARAM_EVT ||
        strHead.cmd == SPI_M_BLE_GATTS_NOTIFY_COMPLETE ||
        strHead.cmd == SPI_M_BLE_GAP_SCAN_RESULT ||
        strHead.cmd == SPI_M_BLE_GATTC_PEER_DATA)) {
        cJSON_Delete(pRoot);
        return 0;
    }
    ty_lp_proto_cmd_handle(strHead.cmd, pRoot);
    cJSON_Delete(pRoot);
#endif
    return 0;
exit:
    cJSON_Delete(pRoot);
    return MM_ERR_GENERAL;
}

/**
 * @brief Convert even-length hex string into bytes
 * @param[in]  hex     hex string ("A1B2...")
 * @param[out] out     output buffer
 * @param[in]  out_sz  output buffer capacity in bytes
 * @return number of bytes written, -1 on invalid
 */
static int __hex_to_bin(const char *hex, unsigned char *out, size_t out_sz)
{
    if (hex == NULL || out == NULL) {
        return -1;
    }
    size_t n = strlen(hex);
    if (n == 0U || (n % 2U) != 0U) {
        return -1;
    }
    size_t blen = n / 2U;
    if (blen > out_sz) {
        blen = out_sz;
    }
    for (size_t i = 0; i < blen; i++) {
        unsigned int v = 0;
        if (sscanf(hex + i * 2U, "%2x", &v) != 1) {
            return -1;
        }
        out[i] = (unsigned char)v;
    }
    return (int)blen;
}

/**
 * @brief Parse BLE address from JSON object {type:int, addr:string(12-hex)}
 * @return 0 success, -1 failure
 */
static int __addr_from_json(cJSON *jo, TKL_BLE_GAP_ADDR_T *addr)
{
    if (jo == NULL || addr == NULL) { return -1; }
    memset(addr, 0, sizeof(*addr));
    cJSON *jt = cJSON_GetObjectItem(jo, "type");
    if (jt == NULL) { jt = cJSON_GetObjectItem(jo, "addr_type"); } /* compat */
    cJSON *ja = cJSON_GetObjectItem(jo, "addr");
    if (ja == NULL || !cJSON_IsString(ja) || ja->valuestring == NULL) { return -1; }
    addr->type = (uint8_t)(jt ? jt->valueint : 0);
    if (strlen(ja->valuestring) != 12U) { return -1; }
    if (__hex_to_bin(ja->valuestring, addr->addr, 6) != 6) { return -1; }
    return 0;
}

/**
 * @brief Parse BLE scan params from JSON object {extended,active,scan_phys,interval,window,timeout,scan_channel_map}
 * @return 0 success, -1 failure
 */
static int __scan_params_from_json(cJSON *jo, TKL_BLE_GAP_SCAN_PARAMS_T *sp)
{
    if (jo == NULL || sp == NULL) { return -1; }
    memset(sp, 0, sizeof(*sp));
    cJSON *jext = cJSON_GetObjectItem(jo, "extended");
    cJSON *jact = cJSON_GetObjectItem(jo, "active");
    cJSON *jphy = cJSON_GetObjectItem(jo, "scan_phys");
    cJSON *ji   = cJSON_GetObjectItem(jo, "interval");
    cJSON *jw   = cJSON_GetObjectItem(jo, "window");
    cJSON *jto  = cJSON_GetObjectItem(jo, "timeout");
    cJSON *jcm  = cJSON_GetObjectItem(jo, "scan_channel_map");
    sp->extended         = jext ? (uint8_t)jext->valueint  : 0;
    sp->active           = jact ? (uint8_t)jact->valueint  : 1;
    sp->scan_phys        = jphy ? (uint8_t)jphy->valueint  : TKL_BLE_GAP_PHY_1MBPS;
    sp->interval         = ji   ? (uint16_t)ji->valueint   : 100;
    sp->window           = jw   ? (uint16_t)jw->valueint   : 50;
    sp->timeout          = jto  ? (uint16_t)jto->valueint  : 0;
    sp->scan_channel_map = jcm  ? (uint8_t)jcm->valueint   : 0x07;
    return 0;
}

/**
 * @brief Parse BLE connection params from JSON object
 * @return 0 success, -1 failure
 */
static int __conn_params_from_json(cJSON *jo, TKL_BLE_GAP_CONN_PARAMS_T *cp)
{
    if (jo == NULL || cp == NULL) { return -1; }
    memset(cp, 0, sizeof(*cp));
    cJSON *jmin = cJSON_GetObjectItem(jo, "conn_interval_min");
    cJSON *jmax = cJSON_GetObjectItem(jo, "conn_interval_max");
    cJSON *jlat = cJSON_GetObjectItem(jo, "conn_latency");
    cJSON *jsup = cJSON_GetObjectItem(jo, "conn_sup_timeout");
    cp->conn_interval_min = jmin ? (uint16_t)jmin->valueint : 6;
    cp->conn_interval_max = jmax ? (uint16_t)jmax->valueint : 12;
    cp->conn_latency      = jlat ? (uint16_t)jlat->valueint : 0;
    cp->conn_sup_timeout  = jsup ? (uint16_t)jsup->valueint : 500;
    return 0;
}

static int ty_lp_proto_cmd_handle(int cmd, cJSON *pRoot)
{
    cJSON *pParam = NULL;
    switch (cmd)
    {

    case SPI_T_WIFI_SCAN:
    {
        AP_IF_S *ap_ary = NULL;
        uint32_t num = 0;
        uint32_t i = 0;
        cJSON *param = NULL;
        cJSON *wifiList = NULL;
        cJSON *ap_item = NULL;
        char *json_str = NULL;
        char bssid_str[18] = {0};

        int scan_ret = OPRT_OK;
        if ((scan_ret = tal_wifi_all_ap_scan(&ap_ary, &num)) != OPRT_OK) {
            PR_ERR("wifi scan failed");
            __lp_proto_cs_send_ack(SPI_T_WIFI_SCAN, 0, NULL, scan_ret);
            ty_lp_proto_wifi_scan_end_cs();
            break;
        } else {
            __lp_proto_cs_send_ack(SPI_T_WIFI_SCAN, 0, NULL, 0);
        }

        ty_lp_proto_wifi_scan_begin_cs();

        /* 分批发送扫描结果 (每批 ≤8 个 AP)，避免单帧超出 MAX_SPI_PAYLOAD_LEN(1576) */
        uint32_t batch_start = 0, batch_end = 0;
        do {
            batch_end = batch_start + 8;
            if (batch_end > num) batch_end = num;

            param = cJSON_CreateObject();
            if (NULL == param) {
                PR_ERR("create scan param failed");
                break;
            }
            wifiList = cJSON_CreateArray();
            if (NULL == wifiList) {
                PR_ERR("create wifiList failed");
                cJSON_Delete(param);
                break;
            }

            for (i = batch_start; i < batch_end; i++) {
                ap_item = cJSON_CreateObject();
                if (NULL == ap_item) {
                    break;
                }
                snprintf(bssid_str, sizeof(bssid_str), "%02X%02X%02X%02X%02X%02X",
                         ap_ary[i].bssid[0], ap_ary[i].bssid[1], ap_ary[i].bssid[2],
                         ap_ary[i].bssid[3], ap_ary[i].bssid[4], ap_ary[i].bssid[5]);
                cJSON_AddStringToObject(ap_item, "bssid", bssid_str);
                cJSON_AddStringToObject(ap_item, "essid", (char *)ap_ary[i].ssid);
                cJSON_AddNumberToObject(ap_item, "rssi", ap_ary[i].rssi);
                cJSON_AddNumberToObject(ap_item, "channel", ap_ary[i].channel);
                cJSON_AddItemToArray(wifiList, ap_item);
            }

            cJSON_AddItemToObject(param, "wifiList", wifiList);
            json_str = cJSON_PrintUnformatted(param);
            cJSON_Delete(param);

            if (json_str) {
                ty_lp_proto_wifi_scan_result_cs(json_str, strlen(json_str));
                cJSON_free(json_str);
            }

            batch_start = batch_end;
        } while (batch_start < num);

        tkl_wifi_release_ap(ap_ary);
        ty_lp_proto_wifi_scan_end_cs();
        break;
    }
    case SPI_M_WIFI_SCAN_START:
    {
        PR_DEBUG("recv wifi scan begin");
        break;
    }
    case SPI_M_WIFI_SCAN:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam)
        {
            PR_ERR("wifilist param parse failed");
        }
        else
        {
            PR_DEBUG("recv wifi scan result");
            __cmd_parse_print_debug(pParam);
        }
    }
    break;
    case SPI_M_WIFI_SCAN_END:
    {
        PR_DEBUG("recv wifi scan end");
        break;
    }

    case SPI_T_WIFI_CONN:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam) {
            PR_ERR("wifi conn param parse failed");
            __lp_proto_cs_send_ack(SPI_T_WIFI_CONN, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        cJSON *ssid_j = cJSON_GetObjectItem(pParam, "ssid");
        cJSON *passwd_j = cJSON_GetObjectItem(pParam, "passwd");
        if (ssid_j && ssid_j->valuestring) {
            const char *passwd_str = (passwd_j && passwd_j->valuestring) ? passwd_j->valuestring : "";
            PR_DEBUG("wifi conn ssid[%s]", ssid_j->valuestring);
            /* Per protocol: send ACK first, then start connect; result reported via SPI_M_WIFI_CON_FINISH */
            __lp_proto_cs_send_ack(SPI_T_WIFI_CONN, 0, NULL, MM_ERR_OK);
            // wifi_bridge_self_mode();
            int conn_ret = tkl_wifi_station_connect((int8_t *)ssid_j->valuestring, (int8_t *)passwd_str);
            // wifi_bridge_exit_sleep_mode();
            ty_lp_proto_wifi_conn_finish_cs(conn_ret);
        } else {
            PR_ERR("wifi conn: missing ssid");
            __lp_proto_cs_send_ack(SPI_T_WIFI_CONN, 0, NULL, MM_ERR_INVALID_PARAM);
        }
        break;
    }
    case SPI_T_WIFI_DISCONN:
    {
        PR_DEBUG("wifi disconn cmd");
        int disconn_ret = tkl_wifi_station_disconnect();
        __lp_proto_cs_send_ack(SPI_T_WIFI_DISCONN, 0, NULL, disconn_ret);
        break;
    }
    case SPI_T_SET_MAC_ADDR:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam) {
            PR_ERR("set mac addr param parse failed");
            __lp_proto_cs_send_ack(SPI_T_SET_MAC_ADDR, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        cJSON *mac_addr_j = cJSON_GetObjectItem(pParam, "mac_addr");
        if (NULL == mac_addr_j) {
            __lp_proto_cs_send_ack(SPI_T_SET_MAC_ADDR, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        NW_MAC_S nw_mac = {0};
        char *mac_str = mac_addr_j->valuestring;
        int mi;
        for (mi = 0; mi < 6; mi++) {
            unsigned int byte_val = 0;
            sscanf(mac_str + mi * 2, "%02x", &byte_val);
            nw_mac.mac[mi] = (uint8_t)byte_val;
        }
        PR_DEBUG("set mac: %02x:%02x:%02x:%02x:%02x:%02x",
                 nw_mac.mac[0], nw_mac.mac[1], nw_mac.mac[2],
                 nw_mac.mac[3], nw_mac.mac[4], nw_mac.mac[5]);
        int mac_ret = tkl_wifi_set_mac(WF_STATION, &nw_mac);
        __lp_proto_cs_send_ack(SPI_T_SET_MAC_ADDR, 0, NULL, mac_ret);
        break;
    }
    case SPI_T_SET_IP_ADDR:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam) {
            PR_ERR("set ip addr param parse failed");
            __lp_proto_cs_send_ack(SPI_T_SET_IP_ADDR, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        cJSON *ip_addr_j = cJSON_GetObjectItem(pParam, "ip_addr");
        cJSON *mask_j    = cJSON_GetObjectItem(pParam, "mask");
        if (ip_addr_j && mask_j) {
            NW_IP_S nw_ip = {0};
            uint32_t cidr = (uint32_t)mask_j->valueint;
            uint32_t mask_val = cidr ? (~0u << (32 - cidr)) : 0;
            unsigned int o1 = 0, o2 = 0, o3 = 0, o4 = 0;
            sscanf(ip_addr_j->valuestring, "%u.%u.%u.%u", &o1, &o2, &o3, &o4);
            snprintf(nw_ip.ip,   sizeof(nw_ip.ip),   "%s", ip_addr_j->valuestring);
            snprintf(nw_ip.mask, sizeof(nw_ip.mask),  "%u.%u.%u.%u",
                     (mask_val >> 24) & 0xFF, (mask_val >> 16) & 0xFF,
                     (mask_val >> 8)  & 0xFF,  mask_val & 0xFF);
            snprintf(nw_ip.gw,   sizeof(nw_ip.gw),   "%u.%u.%u.1", o1, o2, o3);
            PR_DEBUG("set ip: %s mask: %s gw: %s", nw_ip.ip, nw_ip.mask, nw_ip.gw);
            int ip_ret = tkl_wifi_set_ip(WF_STATION, &nw_ip);
            __lp_proto_cs_send_ack(SPI_T_SET_IP_ADDR, 0, NULL, ip_ret);
        } else {
            PR_ERR("set ip addr: missing ip_addr or mask");
            __lp_proto_cs_send_ack(SPI_T_SET_IP_ADDR, 0, NULL, MM_ERR_INVALID_PARAM);
        }
        break;
    }
    case SPI_T_SET_ACTIVE_STAT:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (pParam) {
            cJSON *stat_j = cJSON_GetObjectItem(pParam, "stat");
            if (stat_j) {
                PR_DEBUG("active stat: %d", stat_j->valueint);
            }
        }
        __lp_proto_cs_send_ack(SPI_T_SET_ACTIVE_STAT, 0, NULL, 0);
        break;
    }
    case SPI_T_SET_DEEP_SLEEP:
    {
        PR_DEBUG("recv deep sleep cmd");
        __lp_proto_cs_send_ack(SPI_T_SET_DEEP_SLEEP, 0, NULL, 0);
        // tkl_wifi_set_lp_mode(TRUE, 0);
        break;
    }
    case SPI_T_GET_WIFI_CALIBRATION_VALUE:
    {
        PR_DEBUG("recv get wifi calibration value cmd");
        __lp_proto_cs_send_ack(SPI_T_GET_WIFI_CALIBRATION_VALUE, 0, NULL, 0);
        break;
    }
    case SPI_T_SET_WIFI_MODE:
    {
        int mode_ret = 0;
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (pParam) {
            cJSON *mode_j = cJSON_GetObjectItem(pParam, "mode");
            if (mode_j) {
                /* Protocol: 0=STA, 1=AP */
                WF_WK_MD_E wf_mode;
                if (mode_j->valueint == 0) {
                    wf_mode = WWM_STATION;
                } else if (mode_j->valueint == 1) {
                    wf_mode = WWM_SOFTAP;
                } else {
                    wf_mode = WWM_UNKNOWN;
                }
                mode_ret = tkl_wifi_set_work_mode(wf_mode);
            }
        }
        __lp_proto_cs_send_ack(SPI_T_SET_WIFI_MODE, 0, NULL, mode_ret);
        break;
    }
    case SPI_T_GET_WIFI_MODE:
    {
        WF_WK_MD_E mode = WWM_UNKNOWN;
        int mode_ret = tkl_wifi_get_work_mode(&mode);
        cJSON *param = NULL;
        if (mode_ret == OPRT_OK) {
            param = cJSON_CreateObject();
            if (param) {
                /* Protocol: 0=STA, 1=AP */
                int proto_mode;
                if (mode == WWM_STATION) {
                    proto_mode = 0;
                } else if (mode == WWM_SOFTAP) {
                    proto_mode = 1;
                } else {
                    proto_mode = -1;
                }
                cJSON_AddNumberToObject(param, "mode", proto_mode);
                PR_DEBUG("report wifi mode: %d (wf_mode=%d)", proto_mode, (int)mode);
            }
        }
        __lp_proto_cs_send_ack(SPI_M_SET_WIFI_MODE, 0, param, mode_ret);
        break;
    }
    case SPI_T_SET_WIFI_CHANNEL:
    {
        int chan_ret = 0;
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (pParam) {
            cJSON *chan_j = cJSON_GetObjectItem(pParam, "channel");
            if (chan_j) {
                chan_ret = tkl_wifi_set_cur_channel((uint8_t)chan_j->valueint);
            }
        }
        __lp_proto_cs_send_ack(SPI_T_SET_WIFI_CHANNEL, 0, NULL, chan_ret);
        break;
    }
    case SPI_T_GET_WIFI_CHANNEL:
    {
        uint8_t channel = 0;
        int chan_ret = tkl_wifi_get_cur_channel(&channel);
        cJSON *param = NULL;
        if (chan_ret == OPRT_OK) {
            param = cJSON_CreateObject();
            if (param) {
                cJSON_AddNumberToObject(param, "channel", (int)channel);
                PR_DEBUG("report wifi channel: %d", (int)channel);
            }
        }
        __lp_proto_cs_send_ack(SPI_M_SET_WIFI_CHANNEL, 0, param, chan_ret);
        break;
    }
    case SPI_T_GET_WIFI_RSSI:
    {
        int8_t rssi = 0;
        int rssi_ret = tkl_wifi_station_get_conn_ap_rssi(&rssi);
        cJSON *param = NULL;
        if (rssi_ret == OPRT_OK) {
            param = cJSON_CreateObject();
            if (param) {
                cJSON_AddNumberToObject(param, "rssi", (int)rssi);
                PR_DEBUG("report wifi rssi: %d", (int)rssi);
            }
        }
        __lp_proto_cs_send_ack(SPI_M_SET_WIFI_RSSI, 0, param, rssi_ret);
        break;
    }
    case SPI_M_SET_WIFI_RSSI:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (pParam) {
            cJSON *rssi_j = cJSON_GetObjectItem(pParam, "rssi");
            if (rssi_j) {
                PR_DEBUG("recv wifi rssi: %d", rssi_j->valueint);
            }
        }
        break;
    }
    case SPI_T_GET_WIFI_CONN_STAT:
    {
        WF_STATION_STAT_E stat = WSS_IDLE;
        int stat_ret = tkl_wifi_station_get_status(&stat);
        cJSON *param = NULL;
        if (stat_ret == OPRT_OK) {
            param = cJSON_CreateObject();
            if (param) {
                /* Protocol: report WF_STATION_STAT_E enum value directly */
                cJSON_AddNumberToObject(param, "stat", (int)stat);
                PR_DEBUG("report wifi conn stat: %d", (int)stat);
            }
        }
        __lp_proto_cs_send_ack(SPI_M_SET_WIFI_CONN_STAT, 0, param, stat_ret);
        break;
    }
    case SPI_M_SET_WIFI_CONN_STAT:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (pParam) {
            cJSON *stat_j = cJSON_GetObjectItem(pParam, "stat");
            if (stat_j) {
                PR_DEBUG("recv wifi conn stat: %d", stat_j->valueint);
            }
        }
        break;
    }
    case SPI_T_WIFI_START_AP:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam) {
            PR_ERR("start ap: missing param");
            __lp_proto_cs_send_ack(SPI_T_WIFI_START_AP, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        /* Protocol fields: ssid, passwd, chan, md, ssid_hidden, max_conn, ms_interval, s_len, p_len */
        cJSON *ap_ssid_j        = cJSON_GetObjectItem(pParam, "ssid");
        cJSON *ap_passwd_j      = cJSON_GetObjectItem(pParam, "passwd");
        cJSON *ap_chan_j        = cJSON_GetObjectItem(pParam, "chan");   /* protocol key is 'chan' */
        cJSON *ap_md_j         = cJSON_GetObjectItem(pParam, "md");
        cJSON *ap_hidden_j     = cJSON_GetObjectItem(pParam, "ssid_hidden");
        cJSON *ap_maxconn_j    = cJSON_GetObjectItem(pParam, "max_conn");
        cJSON *ap_interval_j   = cJSON_GetObjectItem(pParam, "ms_interval");
        cJSON *ap_slen_j       = cJSON_GetObjectItem(pParam, "s_len");
        cJSON *ap_plen_j       = cJSON_GetObjectItem(pParam, "p_len");
        if (NULL == ap_ssid_j) {
            PR_ERR("start ap: missing ssid");
            __lp_proto_cs_send_ack(SPI_T_WIFI_START_AP, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        WF_AP_CFG_IF_S ap_cfg;
        memset(&ap_cfg, 0, sizeof(ap_cfg));
        strncpy((char *)ap_cfg.ssid, ap_ssid_j->valuestring, WIFI_SSID_LEN);
        ap_cfg.s_len = ap_slen_j ? (uint8_t)ap_slen_j->valueint
                                 : (uint8_t)strlen(ap_ssid_j->valuestring);
        if (ap_passwd_j && ap_passwd_j->valuestring && ap_passwd_j->valuestring[0]) {
            strncpy((char *)ap_cfg.passwd, ap_passwd_j->valuestring, WIFI_PASSWD_LEN);
            ap_cfg.p_len = ap_plen_j ? (uint8_t)ap_plen_j->valueint
                                     : (uint8_t)strlen(ap_passwd_j->valuestring);
            ap_cfg.md    = ap_md_j ? (WF_AP_AUTH_MODE_E)ap_md_j->valueint : WAAM_WPA2_PSK;
        } else {
            ap_cfg.md = WAAM_OPEN;
        }
        ap_cfg.chan        = (uint8_t)(ap_chan_j ? ap_chan_j->valueint : 6);
        ap_cfg.ssid_hidden = (uint8_t)(ap_hidden_j ? ap_hidden_j->valueint : 0);
        ap_cfg.max_conn    = (uint8_t)(ap_maxconn_j ? ap_maxconn_j->valueint : 4);
        ap_cfg.ms_interval = (uint16_t)(ap_interval_j ? ap_interval_j->valueint : 100);
        PR_DEBUG("start ap: ssid=%s chan=%d auth=%d hidden=%d max_conn=%d",
                 ap_cfg.ssid, ap_cfg.chan, ap_cfg.md, ap_cfg.ssid_hidden, ap_cfg.max_conn);
        int ap_ret = tkl_wifi_start_ap(&ap_cfg);
        if (ap_ret != OPRT_OK) {
            PR_ERR("start ap failed");
        } else {
            // extern void wifi_softap_client_add_event_register(void);
            // wifi_softap_client_add_event_register();
        }
        __lp_proto_cs_send_ack(SPI_T_WIFI_START_AP, 0, NULL, ap_ret);
        break;
    }
    case SPI_T_WIFI_STOP_AP:
    {
        int ap_stop_ret = tkl_wifi_stop_ap();
        {
            // extern void wifi_softap_client_add_event_unregister(void);
            // wifi_softap_client_add_event_unregister();
        }
        __lp_proto_cs_send_ack(SPI_T_WIFI_STOP_AP, 0, NULL, ap_stop_ret);
        break;
    }
    case SPI_T_INTO_SLEEP_V2:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (pParam) {
            cJSON *ip_j      = cJSON_GetObjectItem(pParam, "ip_addr");
            cJSON *port_j    = cJSON_GetObjectItem(pParam, "port");
            cJSON *domain_j  = cJSON_GetObjectItem(pParam, "domain");
            cJSON *devid_j   = cJSON_GetObjectItem(pParam, "dev_id");
            cJSON *lkey_j    = cJSON_GetObjectItem(pParam, "local_key");
            PR_DEBUG("into sleep v2: ip=%s port=%d domain=%s dev_id=%s",
                     ip_j     ? ip_j->valuestring     : "",
                     port_j   ? port_j->valueint       : 0,
                     domain_j ? domain_j->valuestring  : "",
                     devid_j  ? devid_j->valuestring   : "");
            (void)lkey_j;
        }
        __lp_proto_cs_send_ack(SPI_T_INTO_SLEEP_V2, 0, NULL, 0);
        tkl_wifi_set_work_mode(WWM_POWERDOWN);
        break;
    }
    case SPI_T_GET_WIFI_MAC:
    {
        /* Request: no param. Response: SPI_M_GET_MAC_ADDR type=1, param:{"mac_addr":"AABBCCDDEEFF"} */
        NW_MAC_S mac = {0};
        pParam = cJSON_GetObjectItem(pRoot, "param");
        /* wf interface: default WF_STATION; param may carry {"wf":<int>} */
        WF_IF_E wf_if = WF_STATION;
        if (pParam) {
            cJSON *wf_j = cJSON_GetObjectItem(pParam, "wf");
            if (wf_j) wf_if = (WF_IF_E)wf_j->valueint;
        }
        int mac_ret = tkl_wifi_get_mac(wf_if, &mac);
        cJSON *mac_param = NULL;
        if (mac_ret == OPRT_OK) {
            char mac_str[13] = {0};
            snprintf(mac_str, sizeof(mac_str), "%02X%02X%02X%02X%02X%02X",
                     mac.mac[0], mac.mac[1], mac.mac[2],
                     mac.mac[3], mac.mac[4], mac.mac[5]);
            mac_param = cJSON_CreateObject();
            if (mac_param) {
                cJSON_AddStringToObject(mac_param, "mac_addr", mac_str);
            }
            PR_DEBUG("get wifi mac: %s", mac_str);
        } else {
            PR_ERR("get wifi mac failed ret=%d", mac_ret);
        }
        __lp_proto_cs_send_ack(SPI_M_GET_MAC_ADDR, 0, mac_param, mac_ret);
        break;
    }
    case SPI_T_SET_WIFI_COUNTRY:
    {
        /* Request param: {"country": <int>}  COUNTRY_CODE_E */
        pParam = cJSON_GetObjectItem(pRoot, "param");
        int country_ret = MM_ERR_INVALID_PARAM;
        if (pParam) {
            cJSON *country_j = cJSON_GetObjectItem(pParam, "country");
            if (country_j) {
                COUNTRY_CODE_E country = (COUNTRY_CODE_E)country_j->valueint;
                PR_DEBUG("set wifi country: %d", (int)country);
                country_ret = tkl_wifi_set_country_code(country);
            }
        }
        __lp_proto_cs_send_ack(SPI_T_SET_WIFI_COUNTRY, 0, NULL, country_ret);
        break;
    }
    case SPI_T_GET_WIFI_IP:
    {
        /* Request param: {"wf": <WF_IF_E>, "type": <TY_AF_E>}
         * TY_AF_INET=2 (IPv4), TY_AF_INET6=10 (IPv6) */
        pParam = cJSON_GetObjectItem(pRoot, "param");
        WF_IF_E wf_if = WF_STATION;
        int addr_type = -1;
        if (pParam) {
            cJSON *wf_j   = cJSON_GetObjectItem(pParam, "wf");
            cJSON *type_j = cJSON_GetObjectItem(pParam, "type");
            if (wf_j)   wf_if     = (WF_IF_E)wf_j->valueint;
            if (type_j) addr_type = type_j->valueint;
        }
        /* Validate addr_type: must be TY_AF_INET or TY_AF_INET6 */
        if (addr_type != (int)TY_AF_INET && addr_type != (int)TY_AF_INET6) {
            PR_ERR("get wifi ip: invalid type=%d", addr_type);
            __lp_proto_cs_send_ack(SPI_M_GET_WIFI_IP, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        int rsp_ipv4 = (addr_type == (int)TY_AF_INET);
        cJSON *ip_param = cJSON_CreateObject();
        if (ip_param == NULL) {
            __lp_proto_cs_send_ack(SPI_M_GET_WIFI_IP, 0, NULL, MM_ERR_MALLOC_FAIL);
            break;
        }
        if (rsp_ipv4) {
            NW_IP_S ip_info = {0};
            int ip_ret = tkl_wifi_get_ip(wf_if, &ip_info);
            if (ip_ret == OPRT_OK) {
                cJSON_AddStringToObject(ip_param, "ip_addr", ip_info.ip);
                cJSON_AddStringToObject(ip_param, "mask",    ip_info.mask);
                cJSON_AddStringToObject(ip_param, "gw",      ip_info.gw);
                PR_DEBUG("get wifi ip wf=%d ip=%s mask=%s gw=%s",
                         (int)wf_if, ip_info.ip, ip_info.mask, ip_info.gw);
            } else {
                PR_ERR("get wifi ip failed ret=%d", ip_ret);
                cJSON_Delete(ip_param);
                ip_param = NULL;
                __lp_proto_cs_send_ack(SPI_M_GET_WIFI_IP, 0, NULL, ip_ret);
                break;
            }
        } else {
            /* IPv6 not supported on this platform, return empty */
            cJSON_AddStringToObject(ip_param, "ipv6_addr", "");
            cJSON_AddNumberToObject(ip_param, "prefix", 64);
        }
        __lp_proto_cs_send_ack(SPI_M_GET_WIFI_IP, 0, ip_param, MM_ERR_OK);
        break;
    }
    case SPI_T_WIFI_INIT:
    {
        /* Protocol §4.1: simply ACK, no param */
        PR_DEBUG("wifi init cmd, ack only");
        __lp_proto_cs_send_ack(SPI_T_WIFI_INIT, 0, NULL, MM_ERR_OK);
        break;
    }
    case SPI_T_BLE_STACK_INIT:
    {
        /* Protocol: role 0=peripheral, 1=central; TKL: SERVER=0x01, CLIENT=0x02 */
        uint8_t tkl_role = TKL_BLE_ROLE_SERVER; /* default: peripheral */
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (pParam) {
            cJSON *role_j = cJSON_GetObjectItem(pParam, "role");
            if (role_j) {
                if (role_j->valueint == 1) {
                    tkl_role = TKL_BLE_ROLE_CLIENT;
                } else {
                    tkl_role = TKL_BLE_ROLE_SERVER;
                }
            }
            PR_DEBUG("ble stack init, proto_role=%d tkl_role=0x%02x",
                     role_j ? role_j->valueint : 0, tkl_role);
        }

        // ty_lp_proto_notify_callback_register();
        ty_lp_proto_gatts_callback_register();
        ty_lp_proto_gap_callback_register();

        OPERATE_RET init_ret = tuya_hal_init(tkl_role);
        __lp_proto_cs_send_ack(SPI_T_BLE_STACK_INIT, 0, NULL, (int)init_ret);
        break;
    }
    case SPI_T_BLE_STACK_DEINIT:
    {
        /* Protocol: role 0=peripheral, 1=central; TKL: SERVER=0x01, CLIENT=0x02 */
        uint8_t tkl_role = TKL_BLE_ROLE_SERVER; /* default: peripheral */
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (pParam) {
            cJSON *role_j = cJSON_GetObjectItem(pParam, "role");
            if (role_j) {
                if (role_j->valueint == 1) {
                    tkl_role = TKL_BLE_ROLE_CLIENT;
                } else {
                    tkl_role = TKL_BLE_ROLE_SERVER;
                }
            }
            PR_DEBUG("ble stack deinit, proto_role=%d tkl_role=0x%02x",
                     role_j ? role_j->valueint : 0, tkl_role);
        }
        app_ble_disable();
        __lp_proto_cs_send_ack(SPI_T_BLE_STACK_DEINIT, 0, NULL, 0);
        break;
    }
    case SPI_T_BLE_GAP_ADV_START:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        TKL_BLE_GAP_ADV_PARAMS_T adv_param;
        memset(&adv_param, 0, sizeof(adv_param));
        adv_param.adv_type         = TKL_BLE_GAP_ADV_TYPE_CONN_SCANNABLE_UNDIRECTED;
        adv_param.adv_interval_min = 160; /* 100ms, unit: 0.625ms */
        adv_param.adv_interval_max = 160;
        adv_param.adv_channel_map  = 0x07; /* channels 37, 38, 39 */
        if (pParam) {
            cJSON *adv_type_j = cJSON_GetObjectItem(pParam, "adv_type");
            cJSON *intv_min_j = cJSON_GetObjectItem(pParam, "adv_interval_min");
            cJSON *intv_max_j = cJSON_GetObjectItem(pParam, "adv_interval_max");
            cJSON *ch_map_j   = cJSON_GetObjectItem(pParam, "adv_channel_map");
            cJSON *direct_j   = cJSON_GetObjectItem(pParam, "direct_addr");
            if (adv_type_j) adv_param.adv_type         = (uint8_t)adv_type_j->valueint;
            if (intv_min_j) adv_param.adv_interval_min = (uint16_t)intv_min_j->valueint;
            if (intv_max_j) adv_param.adv_interval_max = (uint16_t)intv_max_j->valueint;
            if (ch_map_j)   adv_param.adv_channel_map  = (uint8_t)ch_map_j->valueint;
            if (direct_j)   (void)__addr_from_json(direct_j, &adv_param.direct_addr);
            PR_DEBUG("ble adv start: type=%d intv_min=%u intv_max=%u ch_map=0x%02x",
                     adv_param.adv_type, adv_param.adv_interval_min,
                     adv_param.adv_interval_max, adv_param.adv_channel_map);
        }
        OPERATE_RET adv_ret = tkl_ble_gap_adv_start(&adv_param);
        __lp_proto_cs_send_ack(SPI_T_BLE_GAP_ADV_START, 0, NULL, (int)adv_ret);
        break;
    }
    case SPI_T_BLE_GAP_ADV_STOP:
    {
        PR_DEBUG("ble adv stop");
        ble_status_t stop_ret = tkl_ble_gap_adv_stop();
        __lp_proto_cs_send_ack(SPI_T_BLE_GAP_ADV_STOP, 0, NULL, (int)stop_ret);
        break;
    }
    case SPI_T_BLE_GAP_ADV_DATA_SET:
    case SPI_T_BLE_GAP_ADV_DATA_UPDATE:
    {
        int data_cmd = cmd;
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam) {
            PR_ERR("adv data set: missing param");
            __lp_proto_cs_send_ack(data_cmd, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        cJSON *adv_data_j = cJSON_GetObjectItem(pParam, "adv_data");
        cJSON *scan_rsp_j = cJSON_GetObjectItem(pParam, "scan_rsp");
        uint8_t adv_buf[255] = {0};
        uint8_t rsp_buf[255] = {0};
        TKL_BLE_DATA_T adv_data = {0, NULL};
        TKL_BLE_DATA_T rsp_data = {0, NULL};
        if (adv_data_j && adv_data_j->valuestring && adv_data_j->valuestring[0]) {
            int adv_len = __hex_to_bin(adv_data_j->valuestring, adv_buf, sizeof(adv_buf));
            if (adv_len < 0) {
                PR_ERR("adv data set: invalid adv_data hex");
                __lp_proto_cs_send_ack(data_cmd, 0, NULL, MM_ERR_INVALID_PARAM);
                break;
            }
            adv_data.p_data = adv_buf;
            adv_data.length = (uint16_t)adv_len;
            PR_DEBUG("adv_data len=%d hex=\"%.64s%s\"", adv_len,
                     adv_data_j->valuestring,
                     (strlen(adv_data_j->valuestring) > 64U) ? "..." : "");
        }
        if (scan_rsp_j && scan_rsp_j->valuestring && scan_rsp_j->valuestring[0]) {
            int rsp_len = __hex_to_bin(scan_rsp_j->valuestring, rsp_buf, sizeof(rsp_buf));
            if (rsp_len < 0) {
                PR_ERR("adv data set: invalid scan_rsp hex");
                __lp_proto_cs_send_ack(data_cmd, 0, NULL, MM_ERR_INVALID_PARAM);
                break;
            }
            rsp_data.p_data = rsp_buf;
            rsp_data.length = (uint16_t)rsp_len;
            PR_DEBUG("scan_rsp len=%d hex=\"%.64s%s\"", rsp_len,
                     scan_rsp_j->valuestring,
                     (strlen(scan_rsp_j->valuestring) > 64U) ? "..." : "");
        }
        OPERATE_RET ds_ret;
        if (data_cmd == SPI_T_BLE_GAP_ADV_DATA_UPDATE) {
            ds_ret = tkl_ble_gap_adv_rsp_data_update(
                adv_data.p_data ? &adv_data : NULL,
                rsp_data.p_data ? &rsp_data : NULL);
        } else {
            ds_ret = tkl_ble_gap_adv_rsp_data_set(
                adv_data.p_data ? &adv_data : NULL,
                rsp_data.p_data ? &rsp_data : NULL);
        }
        PR_DEBUG("ble adv_rsp_data_%s adv_len=%u rsp_len=%u ret=%d",
                 data_cmd == SPI_T_BLE_GAP_ADV_DATA_UPDATE ? "update" : "set",
                 (unsigned)adv_data.length, (unsigned)rsp_data.length, (int)ds_ret);
        __lp_proto_cs_send_ack(data_cmd, 0, NULL, (int)ds_ret);
        break;
    }
    case SPI_T_BLE_GAP_ADDR_SET:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam) {
            PR_ERR("ble addr set: missing param");
            __lp_proto_cs_send_ack(SPI_T_BLE_GAP_ADDR_SET, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        cJSON *addr_type_j = cJSON_GetObjectItem(pParam, "addr_type");
        cJSON *addr_j      = cJSON_GetObjectItem(pParam, "addr");
        if (NULL == addr_j || NULL == addr_j->valuestring) {
            PR_ERR("ble addr set: missing addr");
            __lp_proto_cs_send_ack(SPI_T_BLE_GAP_ADDR_SET, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        TKL_BLE_GAP_ADDR_T addr_info;
        memset(&addr_info, 0, sizeof(addr_info));
        addr_info.type = (uint8_t)(addr_type_j ? addr_type_j->valueint : TKL_BLE_GAP_ADDR_TYPE_PUBLIC);
        const char *hex = addr_j->valuestring;
        for (int bi = 0; bi < 6; bi++) {
            unsigned int bv = 0;
            sscanf(hex + bi * 2, "%02x", &bv);
            addr_info.addr[bi] = (uint8_t)bv;
        }
        OPERATE_RET as_ret = tkl_ble_gap_addr_set(&addr_info);
        PR_DEBUG("ble addr set type=%d addr=%02x%02x%02x%02x%02x%02x ret=%d",
                 addr_info.type,
                 addr_info.addr[0], addr_info.addr[1], addr_info.addr[2],
                 addr_info.addr[3], addr_info.addr[4], addr_info.addr[5], (int)as_ret);
        __lp_proto_cs_send_ack(SPI_T_BLE_GAP_ADDR_SET, 0, NULL, (int)as_ret);
        break;
    }
    case SPI_T_BLE_GAP_ADDR_GET:
    {
        TKL_BLE_GAP_ADDR_T id_addr;
        memset(&id_addr, 0, sizeof(id_addr));
        OPERATE_RET ag_ret = tkl_ble_gap_address_get(&id_addr);
        if (ag_ret == OPRT_OK) {
            char addr_hex[13];
            snprintf(addr_hex, sizeof(addr_hex), "%02X%02X%02X%02X%02X%02X",
                     id_addr.addr[0], id_addr.addr[1], id_addr.addr[2],
                     id_addr.addr[3], id_addr.addr[4], id_addr.addr[5]);
            cJSON *resp = cJSON_CreateObject();
            if (resp) {
                cJSON_AddItemToObject(resp, "addr_type", cJSON_CreateNumber(id_addr.type));
                cJSON_AddItemToObject(resp, "addr",      cJSON_CreateString(addr_hex));
                __lp_proto_cs_send_ack(SPI_T_BLE_GAP_ADDR_GET, 0, resp, 0);
            } else {
                __lp_proto_cs_send_ack(SPI_T_BLE_GAP_ADDR_GET, 0, NULL, MM_ERR_MALLOC_FAIL);
            }
        } else {
            PR_ERR("ble addr get failed ret=%d", (int)ag_ret);
            __lp_proto_cs_send_ack(SPI_T_BLE_GAP_ADDR_GET, 0, NULL, (int)ag_ret);
        }
        break;
    }
    case SPI_T_BLE_GAP_DISCONNECT:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam) {
            PR_ERR("ble disconnect: missing param");
            __lp_proto_cs_send_ack(SPI_T_BLE_GAP_DISCONNECT, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        cJSON *conn_handle_j = cJSON_GetObjectItem(pParam, "conn_handle");
        cJSON *reason_j      = cJSON_GetObjectItem(pParam, "reason");
        uint16_t conn_handle = conn_handle_j ? (uint16_t)conn_handle_j->valueint : 0;
        uint8_t  reason      = reason_j      ? (uint8_t)reason_j->valueint : 0x13;
        OPERATE_RET dc_ret = tkl_ble_gap_disconnect(conn_handle, reason);
        PR_DEBUG("ble disconnect conn=%u reason=0x%02x ret=%d", conn_handle, reason, (int)dc_ret);
        __lp_proto_cs_send_ack(SPI_T_BLE_GAP_DISCONNECT, 0, NULL, (int)dc_ret);
        break;
    }
    case SPI_T_BLE_GAP_CONN_PARAM_UPDATE:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam) {
            PR_ERR("ble conn param update: missing param");
            __lp_proto_cs_send_ack(SPI_T_BLE_GAP_CONN_PARAM_UPDATE, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        cJSON *conn_handle_j = cJSON_GetObjectItem(pParam, "conn_handle");
        /* Protocol: conn_params nested object with fields conn_interval_min/max/latency/sup_timeout */
        cJSON *conn_params_j = cJSON_GetObjectItem(pParam, "conn_params");
        uint16_t conn_handle = conn_handle_j ? (uint16_t)conn_handle_j->valueint : 0;
        TKL_BLE_GAP_CONN_PARAMS_T cp;
        memset(&cp, 0, sizeof(cp));
        if (conn_params_j) {
            cJSON *min_j = cJSON_GetObjectItem(conn_params_j, "conn_interval_min");
            cJSON *max_j = cJSON_GetObjectItem(conn_params_j, "conn_interval_max");
            cJSON *lat_j = cJSON_GetObjectItem(conn_params_j, "conn_latency");
            cJSON *sup_j = cJSON_GetObjectItem(conn_params_j, "conn_sup_timeout");
            cp.conn_interval_min = min_j ? (uint16_t)min_j->valueint : 6;
            cp.conn_interval_max = max_j ? (uint16_t)max_j->valueint : 12;
            cp.conn_latency      = lat_j ? (uint16_t)lat_j->valueint : 0;
            cp.conn_sup_timeout  = sup_j ? (uint16_t)sup_j->valueint : 500;
        } else {
            cp.conn_interval_min = 6;
            cp.conn_interval_max = 12;
            cp.conn_latency      = 0;
            cp.conn_sup_timeout  = 500;
        }
        OPERATE_RET cp_ret = tkl_ble_gap_conn_param_update(conn_handle, &cp);
        PR_DEBUG("ble conn param update conn=%u min=%u max=%u lat=%u to=%u ret=%d",
                 conn_handle, cp.conn_interval_min, cp.conn_interval_max,
                 cp.conn_latency, cp.conn_sup_timeout, (int)cp_ret);
        __lp_proto_cs_send_ack(SPI_T_BLE_GAP_CONN_PARAM_UPDATE, 0, NULL, (int)cp_ret);
        break;
    }
    case SPI_T_BLE_GAP_NAME_SET:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam) {
            PR_ERR("ble name set: missing param");
            __lp_proto_cs_send_ack(SPI_T_BLE_GAP_NAME_SET, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        cJSON *name_j = cJSON_GetObjectItem(pParam, "name");
        if (name_j == NULL || name_j->valuestring == NULL) {
            PR_ERR("ble name set: missing name");
            __lp_proto_cs_send_ack(SPI_T_BLE_GAP_NAME_SET, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        OPERATE_RET ns_ret = tkl_ble_gap_name_set(name_j->valuestring);
        PR_DEBUG("ble name set name=%s ret=%d", name_j->valuestring, (int)ns_ret);
        __lp_proto_cs_send_ack(SPI_T_BLE_GAP_NAME_SET, 0, NULL, (int)ns_ret);
        break;
    }
    case SPI_T_BLE_GAP_TX_POWER_SET:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam) {
            PR_ERR("ble tx power set: missing param");
            __lp_proto_cs_send_ack(SPI_T_BLE_GAP_TX_POWER_SET, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        cJSON *role_j     = cJSON_GetObjectItem(pParam, "role");
        cJSON *txpwr_j    = cJSON_GetObjectItem(pParam, "tx_power");
        uint8_t tx_role   = role_j  ? (uint8_t)role_j->valueint  : 0;
        int     tx_power  = txpwr_j ? txpwr_j->valueint           : 0;
        OPERATE_RET tp_ret = tkl_ble_gap_tx_power_set(tx_role, tx_power);
        PR_DEBUG("ble tx power set role=%u power=%d ret=%d", tx_role, tx_power, (int)tp_ret);
        __lp_proto_cs_send_ack(SPI_T_BLE_GAP_TX_POWER_SET, 0, NULL, (int)tp_ret);
        break;
    }
    case SPI_T_BLE_GAP_RSSI_GET:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam) {
            PR_ERR("ble rssi get: missing param");
            __lp_proto_cs_send_ack(SPI_T_BLE_GAP_RSSI_GET, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        cJSON *conn_handle_j = cJSON_GetObjectItem(pParam, "conn_handle");
        uint16_t conn_handle = conn_handle_j ? (uint16_t)conn_handle_j->valueint : 0;
        OPERATE_RET rr_ret = tkl_ble_gap_rssi_get(conn_handle);
        PR_DEBUG("ble rssi get conn=%u ret=%d", conn_handle, (int)rr_ret);
        if (rr_ret != OPRT_OK) {
            __lp_proto_cs_send_ack(SPI_T_BLE_GAP_RSSI_GET, 0, NULL, (int)rr_ret);
        }
        /* ACK with rssi value sent asynchronously via BLE_CONN_EVT_RSSI_GET_RSP */
        break;
    }
    case SPI_T_BLE_GATTS_SERVICE_ADD:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam) {
            PR_ERR("gatts svc add: missing param");
            __lp_proto_cs_send_ack(SPI_T_BLE_GATTS_SERVICE_ADD, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        /* Protocol: full format with svc_num + services array */
        cJSON *svc_num_j = cJSON_GetObjectItem(pParam, "svc_num");
        cJSON *services_j = cJSON_GetObjectItem(pParam, "services");
        if (services_j == NULL || !cJSON_IsArray(services_j)) {
            /* Legacy fallback: simple svc_uuid + char_num format */
            cJSON *svc_uuid_j = cJSON_GetObjectItem(pParam, "svc_uuid");
            cJSON *char_num_j = cJSON_GetObjectItem(pParam, "char_num");
            if (svc_uuid_j == NULL || svc_uuid_j->valuestring == NULL) {
                PR_ERR("gatts svc add: missing svc_uuid or services array");
                __lp_proto_cs_send_ack(SPI_T_BLE_GATTS_SERVICE_ADD, 0, NULL, MM_ERR_INVALID_PARAM);
                break;
            }
            uint16_t svc_uuid16 = (uint16_t)strtoul(svc_uuid_j->valuestring, NULL, 16);
            uint8_t  char_num   = char_num_j ? (uint8_t)char_num_j->valueint : 0;
            if (char_num > 8) char_num = 8;
            TKL_BLE_CHAR_PARAMS_T chars[8];
            memset(chars, 0, sizeof(chars));
            for (uint8_t ci = 0; ci < char_num; ci++) {
                chars[ci].char_uuid.uuid_type   = TKL_BLE_UUID_TYPE_16;
                chars[ci].char_uuid.uuid.uuid16 = svc_uuid16 + 1 + ci;
                chars[ci].property  = TKL_BLE_GATT_CHAR_PROP_READ |
                                       TKL_BLE_GATT_CHAR_PROP_WRITE_NO_RSP |
                                       TKL_BLE_GATT_CHAR_PROP_WRITE |
                                       TKL_BLE_GATT_CHAR_PROP_NOTIFY;
                chars[ci].permission = TKL_BLE_GATT_PERM_READ | TKL_BLE_GATT_PERM_WRITE;
                chars[ci].value_len  = 20;
            }
            TKL_BLE_SERVICE_PARAMS_T svc;
            memset(&svc, 0, sizeof(svc));
            svc.svc_uuid.uuid_type   = TKL_BLE_UUID_TYPE_16;
            svc.svc_uuid.uuid.uuid16 = svc_uuid16;
            svc.type     = TKL_BLE_UUID_SERVICE_PRIMARY;
            svc.char_num = char_num;
            svc.p_char   = char_num > 0 ? chars : NULL;
            TKL_BLE_GATTS_PARAMS_T gatts_params;
            memset(&gatts_params, 0, sizeof(gatts_params));
            gatts_params.svc_num   = 1;
            gatts_params.p_service = &svc;
            OPERATE_RET sa_ret = tkl_ble_gatts_service_add(&gatts_params);
            PR_DEBUG("gatts svc add (legacy) uuid=0x%04x char_num=%u ret=%d",
                     svc_uuid16, char_num, (int)sa_ret);
            __lp_proto_cs_send_ack(SPI_T_BLE_GATTS_SERVICE_ADD, 0, NULL, (int)sa_ret);
            break;
        }
        /* Full format: parse services array */
        int svc_num = svc_num_j ? svc_num_j->valueint : cJSON_GetArraySize(services_j);
        if (svc_num <= 0 || svc_num > TKL_BLE_GATT_SERVICE_MAX_NUM) {
            PR_ERR("gatts svc add: invalid svc_num=%d", svc_num);
            __lp_proto_cs_send_ack(SPI_T_BLE_GATTS_SERVICE_ADD, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        TKL_BLE_SERVICE_PARAMS_T *svcs = (TKL_BLE_SERVICE_PARAMS_T *)tkl_system_malloc(
                                            (size_t)svc_num * sizeof(TKL_BLE_SERVICE_PARAMS_T));
        if (svcs == NULL) {
            __lp_proto_cs_send_ack(SPI_T_BLE_GATTS_SERVICE_ADD, 0, NULL, MM_ERR_MALLOC_FAIL);
            break;
        }
        memset(svcs, 0, (size_t)svc_num * sizeof(TKL_BLE_SERVICE_PARAMS_T));
        int parse_ok = 1;
        for (int si = 0; si < svc_num; si++) {
            cJSON *jsvc = cJSON_GetArrayItem(services_j, si);
            if (jsvc == NULL) { parse_ok = 0; break; }
            cJSON *jtype     = cJSON_GetObjectItem(jsvc, "svc_type");
            cJSON *jsvc_uuid = cJSON_GetObjectItem(jsvc, "svc_uuid"); /* object or string */
            cJSON *jchar_num = cJSON_GetObjectItem(jsvc, "char_num");
            cJSON *jchars    = cJSON_GetObjectItem(jsvc, "chars");
            int char_num = jchar_num ? jchar_num->valueint : 0;
            if (char_num < 0 || char_num > TKL_BLE_GATT_CHAR_MAX_NUM) { parse_ok = 0; break; }
            svcs[si].type = jtype ? (TKL_BLE_SERVICE_TYPE_E)jtype->valueint
                                  : TKL_BLE_UUID_SERVICE_PRIMARY;
                {
                    char *__jcuuid_str = jsvc_uuid ? cJSON_PrintUnformatted(jsvc_uuid) : NULL;
                    PR_DEBUG("jcuuid[si=%d]: %s", si, __jcuuid_str ? __jcuuid_str : "(null)");
                    if (__jcuuid_str) { cJSON_free(__jcuuid_str); }
                }
            /* svc_uuid: support object {uuid_type, uuid} or plain hex string */
            if (jsvc_uuid && cJSON_IsObject(jsvc_uuid)) {
                cJSON *jut = cJSON_GetObjectItem(jsvc_uuid, "uuid_type");
                cJSON *juv = cJSON_GetObjectItem(jsvc_uuid, "uuid");
                svcs[si].svc_uuid.uuid_type = jut ? (TKL_BLE_UUID_TYPE_E)jut->valueint
                                                  : TKL_BLE_UUID_TYPE_16;
                if (juv && cJSON_IsString(juv) && juv->valuestring) {
                    if (svcs[si].svc_uuid.uuid_type == TKL_BLE_UUID_TYPE_128) {
                        __hex_to_bin(juv->valuestring, svcs[si].svc_uuid.uuid.uuid128, 16);
                    } else if (svcs[si].svc_uuid.uuid_type == TKL_BLE_UUID_TYPE_32) {
                        svcs[si].svc_uuid.uuid.uuid32 =
                            (uint32_t)strtoul(juv->valuestring, NULL, 16);
                    } else {
                        svcs[si].svc_uuid.uuid.uuid16 =
                            (uint16_t)strtoul(juv->valuestring, NULL, 16);
                    }
                }
            } else if (jsvc_uuid && cJSON_IsString(jsvc_uuid) && jsvc_uuid->valuestring) {
                svcs[si].svc_uuid.uuid_type   = TKL_BLE_UUID_TYPE_16;
                svcs[si].svc_uuid.uuid.uuid16 =
                    (uint16_t)strtoul(jsvc_uuid->valuestring, NULL, 16);
            }
            svcs[si].char_num = (uint8_t)char_num;
            TKL_BLE_CHAR_PARAMS_T *cps = NULL;
            if (char_num > 0) {
                cps = (TKL_BLE_CHAR_PARAMS_T *)tkl_system_malloc(
                                                (size_t)char_num * sizeof(TKL_BLE_CHAR_PARAMS_T));
                if (cps == NULL) { parse_ok = 0; break; }
                memset(cps, 0, (size_t)char_num * sizeof(TKL_BLE_CHAR_PARAMS_T));
                for (int ci = 0; ci < char_num; ci++) {
                    cJSON *jch = jchars ? cJSON_GetArrayItem(jchars, ci) : NULL;
                    if (jch == NULL) { parse_ok = 0; break; }
                    cJSON *jprop  = cJSON_GetObjectItem(jch, "property");
                    cJSON *jperm  = cJSON_GetObjectItem(jch, "permission");
                    cJSON *jvlen  = cJSON_GetObjectItem(jch, "value_len");
                    cJSON *jcuuid = cJSON_GetObjectItem(jch, "char_uuid");
                    cps[ci].property   = jprop ? (uint8_t)jprop->valueint
                                               : (TKL_BLE_GATT_CHAR_PROP_READ |
                                                  TKL_BLE_GATT_CHAR_PROP_WRITE_NO_RSP |
                                                  TKL_BLE_GATT_CHAR_PROP_WRITE |
                                                  TKL_BLE_GATT_CHAR_PROP_NOTIFY);
                    cps[ci].permission = jperm ? (uint8_t)jperm->valueint
                                               : (TKL_BLE_GATT_PERM_READ | TKL_BLE_GATT_PERM_WRITE);
                    cps[ci].value_len  = jvlen ? (uint8_t)jvlen->valueint : 20;

                    if (jcuuid && cJSON_IsObject(jcuuid)) {
                        cJSON *jut = cJSON_GetObjectItem(jcuuid, "uuid_type");
                        cJSON *juv = cJSON_GetObjectItem(jcuuid, "uuid");
                        cps[ci].char_uuid.uuid_type = jut ? (TKL_BLE_UUID_TYPE_E)jut->valueint
                                                          : TKL_BLE_UUID_TYPE_16;
                        if (juv && cJSON_IsString(juv) && juv->valuestring) {
                            if (cps[ci].char_uuid.uuid_type == TKL_BLE_UUID_TYPE_128) {
                                __hex_to_bin(juv->valuestring, cps[ci].char_uuid.uuid.uuid128, 16);
                            } else if (cps[ci].char_uuid.uuid_type == TKL_BLE_UUID_TYPE_32) {
                                cps[ci].char_uuid.uuid.uuid32 =
                                    (uint32_t)strtoul(juv->valuestring, NULL, 16);
                            } else {
                                cps[ci].char_uuid.uuid.uuid16 =
                                    (uint16_t)strtoul(juv->valuestring, NULL, 16);
                            }
                        }
                    }
                }
                if (!parse_ok) { tkl_system_free(cps); break; }
            }
            svcs[si].p_char = cps;
        }
        if (!parse_ok) {
            for (int si = 0; si < svc_num; si++) {
                if (svcs[si].p_char) { tkl_system_free(svcs[si].p_char); }
            }
            tkl_system_free(svcs);
            PR_ERR("gatts svc add: parse services array failed");
            __lp_proto_cs_send_ack(SPI_T_BLE_GATTS_SERVICE_ADD, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        TKL_BLE_GATTS_PARAMS_T gatts_params;
        memset(&gatts_params, 0, sizeof(gatts_params));
        gatts_params.svc_num   = (uint8_t)svc_num;
        gatts_params.p_service = svcs;
        OPERATE_RET sa_ret = tkl_ble_gatts_service_add(&gatts_params);
        PR_DEBUG("gatts svc add svc_num=%d ret=%d", svc_num, (int)sa_ret);
        /* Build handle response before freeing svcs */
        cJSON *sa_rsp = cJSON_CreateObject();
        if (sa_rsp) {
            cJSON_AddItemToObject(sa_rsp, "svc_num", cJSON_CreateNumber(svc_num));
            cJSON *sa_svcs = cJSON_CreateArray();
            if (sa_svcs) {
                cJSON_AddItemToObject(sa_rsp, "services", sa_svcs);
                for (int si = 0; si < svc_num; si++) {
                    cJSON *os = cJSON_CreateObject();
                    if (!os) { continue; }
                    cJSON_AddItemToArray(sa_svcs, os);
                    cJSON_AddItemToObject(os, "handle", cJSON_CreateNumber(svcs[si].handle));
                    cJSON *oc = cJSON_CreateArray();
                    if (oc) {
                        cJSON_AddItemToObject(os, "chars", oc);
                        for (int ci = 0; ci < (int)svcs[si].char_num && svcs[si].p_char; ci++) {
                            cJSON *och = cJSON_CreateObject();
                            if (!och) { continue; }
                            cJSON_AddItemToArray(oc, och);
                            cJSON_AddItemToObject(och, "handle",
                                cJSON_CreateNumber(svcs[si].p_char[ci].handle));
                        }
                    }
                }
            }
        }
        for (int si = 0; si < svc_num; si++) {
            if (svcs[si].p_char) { tkl_system_free(svcs[si].p_char); }
        }
        tkl_system_free(svcs);
        __lp_proto_cs_send_ack(SPI_T_BLE_GATTS_SERVICE_ADD, 0, sa_rsp, (int)sa_ret);
        break;
    }
    case SPI_T_BLE_GATTS_VALUE_SET:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam) {
            PR_ERR("gatts value set: missing param");
            __lp_proto_cs_send_ack(SPI_T_BLE_GATTS_VALUE_SET, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        cJSON *conn_handle_j = cJSON_GetObjectItem(pParam, "conn_handle");
        cJSON *char_handle_j = cJSON_GetObjectItem(pParam, "char_handle");
        cJSON *data_j        = cJSON_GetObjectItem(pParam, "data");
        if (data_j == NULL || data_j->valuestring == NULL) {
            PR_ERR("gatts value set: missing data");
            __lp_proto_cs_send_ack(SPI_T_BLE_GATTS_VALUE_SET, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        uint16_t vs_conn   = conn_handle_j ? (uint16_t)conn_handle_j->valueint : 0;
        uint16_t vs_char   = char_handle_j ? (uint16_t)char_handle_j->valueint : 0;
        const char *vs_hex = data_j->valuestring;
        uint8_t vs_buf[512];
        int vs_len = __hex_to_bin(vs_hex, vs_buf, sizeof(vs_buf));
        if (vs_len < 0) {
            PR_ERR("gatts value set: invalid hex data");
            __lp_proto_cs_send_ack(SPI_T_BLE_GATTS_VALUE_SET, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        OPERATE_RET vs_ret = tkl_ble_gatts_value_set(vs_conn, vs_char, vs_buf, (uint16_t)vs_len);
        PR_DEBUG("gatts value set conn=%u char=%u len=%d ret=%d", vs_conn, vs_char, vs_len, (int)vs_ret);
        __lp_proto_cs_send_ack(SPI_T_BLE_GATTS_VALUE_SET, 0, NULL, (int)vs_ret);
        break;
    }
    case SPI_T_BLE_GATTS_VALUE_GET:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam) {
            PR_ERR("gatts value get: missing param");
            __lp_proto_cs_send_ack(SPI_T_BLE_GATTS_VALUE_GET, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        cJSON *conn_handle_j = cJSON_GetObjectItem(pParam, "conn_handle");
        cJSON *char_handle_j = cJSON_GetObjectItem(pParam, "char_handle");
        uint16_t vg_conn = conn_handle_j ? (uint16_t)conn_handle_j->valueint : 0;
        uint16_t vg_char = char_handle_j ? (uint16_t)char_handle_j->valueint : 0;
        uint8_t  vg_buf[512] = {0};
        OPERATE_RET vg_ret = tkl_ble_gatts_value_get(vg_conn, vg_char, vg_buf, sizeof(vg_buf));
        PR_DEBUG("gatts value get conn=%u char=%u ret=%d", vg_conn, vg_char, (int)vg_ret);
        if (vg_ret == OPRT_OK) {
            /* Build hex string from returned buffer */
            char vg_hex[512 * 2 + 1];
            uint16_t vg_len = (uint16_t)sizeof(vg_buf);
            for (uint16_t gi = 0; gi < vg_len; gi++) {
                snprintf(vg_hex + gi * 2, 3, "%02X", vg_buf[gi]);
            }
            vg_hex[vg_len * 2] = '\0';
            cJSON *vg_param = cJSON_CreateObject();
            if (vg_param) {
                cJSON_AddStringToObject(vg_param, "data", vg_hex);
            }
            __lp_proto_cs_send_ack(SPI_T_BLE_GATTS_VALUE_GET, 0, vg_param, 0);
        } else {
            __lp_proto_cs_send_ack(SPI_T_BLE_GATTS_VALUE_GET, 0, NULL, (int)vg_ret);
        }
        break;
    }
    case SPI_T_BLE_GATTS_VALUE_NOTIFY:
    case SPI_T_BLE_GATTS_VALUE_INDICATE:
    {
        int ni_cmd = cmd;
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam) {
            PR_ERR("gatts %s: missing param", ni_cmd == SPI_T_BLE_GATTS_VALUE_NOTIFY ? "notify" : "indicate");
            __lp_proto_cs_send_ack(ni_cmd, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        cJSON *conn_handle_j = cJSON_GetObjectItem(pParam, "conn_handle");
        cJSON *char_handle_j = cJSON_GetObjectItem(pParam, "char_handle");
        cJSON *data_j        = cJSON_GetObjectItem(pParam, "data");
        if (data_j == NULL || data_j->valuestring == NULL) {
            PR_ERR("gatts %s: missing data", ni_cmd == SPI_T_BLE_GATTS_VALUE_NOTIFY ? "notify" : "indicate");
            __lp_proto_cs_send_ack(ni_cmd, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        uint16_t ni_conn    = conn_handle_j ? (uint16_t)conn_handle_j->valueint : 0;
        uint16_t ni_char    = char_handle_j ? (uint16_t)char_handle_j->valueint : 0;
        const char *ni_hex  = data_j->valuestring;
        uint8_t ni_buf[512];
        int ni_len = __hex_to_bin(ni_hex, ni_buf, sizeof(ni_buf));
        if (ni_len < 0) {
            PR_ERR("gatts %s: invalid hex data", ni_cmd == SPI_T_BLE_GATTS_VALUE_NOTIFY ? "notify" : "indicate");
            __lp_proto_cs_send_ack(ni_cmd, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        OPERATE_RET ni_ret;
        if (ni_cmd == SPI_T_BLE_GATTS_VALUE_NOTIFY) {
            ni_ret = tkl_ble_gatts_value_notify(ni_conn, ni_char, ni_buf, (uint16_t)ni_len);
        } else {
            ni_ret = tkl_ble_gatts_value_indicate(ni_conn, ni_char, ni_buf, (uint16_t)ni_len);
        }
        PR_DEBUG("gatts %s conn=%u char=%u len=%d ret=%d",
                 ni_cmd == SPI_T_BLE_GATTS_VALUE_NOTIFY ? "notify" : "indicate",
                 ni_conn, ni_char, ni_len, (int)ni_ret);
        __lp_proto_cs_send_ack(ni_cmd, 0, NULL, (int)ni_ret);
        break;
    }
    case SPI_T_BLE_GATTS_MTU_REPLY:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam) {
            PR_ERR("gatts mtu reply: missing param");
            __lp_proto_cs_send_ack(SPI_T_BLE_GATTS_MTU_REPLY, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        cJSON *conn_handle_j = cJSON_GetObjectItem(pParam, "conn_handle");
        cJSON *mtu_j         = cJSON_GetObjectItem(pParam, "mtu");
        uint16_t mr_conn = conn_handle_j ? (uint16_t)conn_handle_j->valueint : 0;
        uint16_t mr_mtu  = mtu_j         ? (uint16_t)mtu_j->valueint         : 247;
        OPERATE_RET mr_ret = tkl_ble_gatts_exchange_mtu_reply(mr_conn, mr_mtu);
        PR_DEBUG("gatts mtu reply conn=%u mtu=%u ret=%d", mr_conn, mr_mtu, (int)mr_ret);
        __lp_proto_cs_send_ack(SPI_T_BLE_GATTS_MTU_REPLY, 0, NULL, (int)mr_ret);
        break;
    }
    case SPI_T_BLE_GAP_SCAN_START:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        TKL_BLE_GAP_SCAN_PARAMS_T scan_sp;
        memset(&scan_sp, 0, sizeof(scan_sp));
        cJSON *jsp = pParam ? cJSON_GetObjectItem(pParam, "scan_params") : NULL;
        if (jsp) {
            (void)__scan_params_from_json(jsp, &scan_sp);
        } else {
            scan_sp.extended         = 0;
            scan_sp.active           = 1;
            scan_sp.scan_phys        = TKL_BLE_GAP_PHY_1MBPS;
            scan_sp.interval         = 100;
            scan_sp.window           = 50;
            scan_sp.timeout          = 0;
            scan_sp.scan_channel_map = 0x07;
        }
        PR_DEBUG("ble scan start interval=%u window=%u active=%u extended=%u phys=%u",
                 scan_sp.interval, scan_sp.window, scan_sp.active,
                 scan_sp.extended, scan_sp.scan_phys);
        OPERATE_RET ss_ret = tkl_ble_gap_scan_start(&scan_sp);
        PR_DEBUG("ble scan start ret=%d", (int)ss_ret);
        __lp_proto_cs_send_ack(SPI_T_BLE_GAP_SCAN_START, 0, NULL, (int)ss_ret);
        break;
    }
    case SPI_T_BLE_GAP_SCAN_STOP:
    {
        PR_DEBUG("ble scan stop");
        OPERATE_RET sto_ret = tkl_ble_gap_scan_stop();
        PR_DEBUG("ble scan stop ret=%d", (int)sto_ret);
        __lp_proto_cs_send_ack(SPI_T_BLE_GAP_SCAN_STOP, 0, NULL, (int)sto_ret);
        break;
    }
    case SPI_T_BLE_GAP_CONNECT:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        cJSON *jpa = pParam ? cJSON_GetObjectItem(pParam, "peer_addr")  : NULL;
        cJSON *jsc = pParam ? cJSON_GetObjectItem(pParam, "scan_params") : NULL;
        cJSON *jcp = pParam ? cJSON_GetObjectItem(pParam, "conn_params") : NULL;
        TKL_BLE_GAP_ADDR_T peer_addr;
        memset(&peer_addr, 0, sizeof(peer_addr));
        if (__addr_from_json(jpa, &peer_addr) != 0) {
            PR_ERR("ble connect: invalid peer_addr");
            __lp_proto_cs_send_ack(SPI_T_BLE_GAP_CONNECT, 0, NULL, MM_ERR_INVALID_PARAM);
            break;
        }
        TKL_BLE_GAP_SCAN_PARAMS_T conn_sp;
        memset(&conn_sp, 0, sizeof(conn_sp));
        if (jsc) { (void)__scan_params_from_json(jsc, &conn_sp); }
        TKL_BLE_GAP_CONN_PARAMS_T conn_cp;
        memset(&conn_cp, 0, sizeof(conn_cp));
        if (jcp) { (void)__conn_params_from_json(jcp, &conn_cp); }
        PR_DEBUG("ble connect peer_type=%u", (unsigned)peer_addr.type);
        OPERATE_RET gc_ret = tkl_ble_gap_connect(&peer_addr,
                                                  jsc ? &conn_sp : NULL,
                                                  jcp ? &conn_cp : NULL);
        PR_DEBUG("ble connect ret=%d", (int)gc_ret);
        __lp_proto_cs_send_ack(SPI_T_BLE_GAP_CONNECT, 0, NULL, (int)gc_ret);
        break;
    }
    case SPI_T_BLE_GATTC_SVC_DISCOVER:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        cJSON *gsd_h = pParam ? cJSON_GetObjectItem(pParam, "conn_handle") : NULL;
        uint16_t gsd_conn = gsd_h ? (uint16_t)gsd_h->valueint : 0;
        PR_DEBUG("ble gattc all svc discover conn=%u", gsd_conn);
        OPERATE_RET gsd_ret = tkl_ble_gattc_all_service_discovery(gsd_conn);
        PR_DEBUG("ble gattc all svc discover ret=%d", (int)gsd_ret);
        __lp_proto_cs_send_ack(SPI_T_BLE_GATTC_SVC_DISCOVER, 0, NULL, (int)gsd_ret);
        break;
    }
    case SPI_T_BLE_GATTC_CHR_DISCOVER:
    case SPI_T_BLE_GATTC_DESC_DISCOVER:
    {
        int gcd_cmd = cmd;
        pParam = cJSON_GetObjectItem(pRoot, "param");
        cJSON *gcd_hj = pParam ? cJSON_GetObjectItem(pParam, "conn_handle")  : NULL;
        cJSON *gcd_sj = pParam ? cJSON_GetObjectItem(pParam, "start_handle") : NULL;
        cJSON *gcd_ej = pParam ? cJSON_GetObjectItem(pParam, "end_handle")   : NULL;
        uint16_t gcd_conn  = gcd_hj ? (uint16_t)gcd_hj->valueint : 0;
        uint16_t gcd_start = gcd_sj ? (uint16_t)gcd_sj->valueint : 1;
        uint16_t gcd_end   = gcd_ej ? (uint16_t)gcd_ej->valueint : 0xFFFF;
        OPERATE_RET gcd_ret;
        if (gcd_cmd == SPI_T_BLE_GATTC_CHR_DISCOVER) {
            PR_DEBUG("ble gattc all char discover conn=%u range=[%u,%u]", gcd_conn, gcd_start, gcd_end);
            gcd_ret = tkl_ble_gattc_all_char_discovery(gcd_conn, gcd_start, gcd_end);
        } else {
            PR_DEBUG("ble gattc char desc discover conn=%u range=[%u,%u]", gcd_conn, gcd_start, gcd_end);
            gcd_ret = tkl_ble_gattc_char_desc_discovery(gcd_conn, gcd_start, gcd_end);
        }
        PR_DEBUG("ble gattc discover ret=%d", (int)gcd_ret);
        __lp_proto_cs_send_ack(gcd_cmd, 0, NULL, (int)gcd_ret);
        break;
    }
    case SPI_T_BLE_GATTC_READ:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        cJSON *gr_hj = pParam ? cJSON_GetObjectItem(pParam, "conn_handle") : NULL;
        cJSON *gr_cj = pParam ? cJSON_GetObjectItem(pParam, "char_handle") : NULL;
        uint16_t gr_conn = gr_hj ? (uint16_t)gr_hj->valueint : 0;
        uint16_t gr_char = gr_cj ? (uint16_t)gr_cj->valueint : 0;
        PR_DEBUG("ble gattc read conn=%u char=%u", gr_conn, gr_char);
        OPERATE_RET gr_ret = tkl_ble_gattc_read(gr_conn, gr_char);
        PR_DEBUG("ble gattc read ret=%d", (int)gr_ret);
        __lp_proto_cs_send_ack(SPI_T_BLE_GATTC_READ, 0, NULL, (int)gr_ret);
        break;
    }
    case SPI_T_BLE_GATTC_WRITE:
    case SPI_T_BLE_GATTC_WRITE_NO_RSP:
    {
        int gw_cmd = cmd;
        pParam = cJSON_GetObjectItem(pRoot, "param");
        cJSON *gw_hj = pParam ? cJSON_GetObjectItem(pParam, "conn_handle") : NULL;
        cJSON *gw_cj = pParam ? cJSON_GetObjectItem(pParam, "char_handle") : NULL;
        cJSON *gw_dj = pParam ? cJSON_GetObjectItem(pParam, "data")        : NULL;
        uint16_t gw_conn = gw_hj ? (uint16_t)gw_hj->valueint : 0;
        uint16_t gw_char = gw_cj ? (uint16_t)gw_cj->valueint : 0;
        uint8_t gw_buf[512];
        int gw_len = 0;
        if (gw_dj && cJSON_IsString(gw_dj) && gw_dj->valuestring && gw_dj->valuestring[0]) {
            gw_len = __hex_to_bin(gw_dj->valuestring, gw_buf, sizeof(gw_buf));
            if (gw_len < 0) {
                PR_ERR("ble gattc write: invalid data hex");
                __lp_proto_cs_send_ack(gw_cmd, 0, NULL, MM_ERR_INVALID_PARAM);
                break;
            }
        }
        OPERATE_RET gw_ret;
        if (gw_cmd == SPI_T_BLE_GATTC_WRITE_NO_RSP) {
            PR_DEBUG("ble gattc write_no_rsp conn=%u char=%u len=%d", gw_conn, gw_char, gw_len);
            gw_ret = tkl_ble_gattc_write_without_rsp(gw_conn, gw_char,
                         (gw_len > 0) ? gw_buf : NULL, (uint16_t)gw_len);
        } else {
            PR_DEBUG("ble gattc write conn=%u char=%u len=%d", gw_conn, gw_char, gw_len);
            gw_ret = tkl_ble_gattc_write(gw_conn, gw_char,
                         (gw_len > 0) ? gw_buf : NULL, (uint16_t)gw_len);
        }
        PR_DEBUG("ble gattc write ret=%d", (int)gw_ret);
        __lp_proto_cs_send_ack(gw_cmd, 0, NULL, (int)gw_ret);
        break;
    }
    case SPI_T_BLE_GATTC_MTU_REQ:
    {
        pParam = cJSON_GetObjectItem(pRoot, "param");
        cJSON *gm_hj = pParam ? cJSON_GetObjectItem(pParam, "conn_handle") : NULL;
        cJSON *gm_mj = pParam ? cJSON_GetObjectItem(pParam, "mtu")         : NULL;
        uint16_t gm_conn = gm_hj ? (uint16_t)gm_hj->valueint : 0;
        uint16_t gm_mtu  = gm_mj ? (uint16_t)gm_mj->valueint : 247;
        PR_DEBUG("ble gattc mtu request conn=%u mtu=%u", gm_conn, gm_mtu);
        OPERATE_RET gm_ret = tkl_ble_gattc_exchange_mtu_request(gm_conn, gm_mtu);
        PR_DEBUG("ble gattc mtu request ret=%d", (int)gm_ret);
        __lp_proto_cs_send_ack(SPI_T_BLE_GATTC_MTU_REQ, 0, NULL, (int)gm_ret);
        break;
    }

#if defined(MM_ENABLE_MQTT_KEEPALIVE)
    case LP_PROTO_O_SDK_GET_TIME:
    {
        PR_DEBUG("receive get time cmd");
        ty_system_set_time_async();
        break;
    }
    case LP_PROTO_O_SDK_SET_TIME:
    {
        PR_DEBUG("get time");
        cJSON *start_utc;
        cJSON *end_utc;
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam)
        {
            PR_DEBUG("no time info");
            return MM_ERR_INVALID_PARAM;
        }
        else
        {
            __cmd_parse_print_debug(pParam);
        }
        cJSON *time_utc_s = cJSON_GetObjectItem(pParam, "time_utc_s");
        cJSON *time_zone = cJSON_GetObjectItem(pParam, "time_zone");
        cJSON *dls_list = cJSON_GetObjectItem(pParam, "dls_list");
        if (NULL == dls_list)
        {
            PR_DEBUG("no dls_list info");
            return MM_ERR_INVALID_PARAM;
        }
        int dls_list_num = cJSON_GetArraySize(dls_list);

        for (int i = 0; i < 1; i++)
        {
            cJSON *pSub = cJSON_GetArrayItem(dls_list, i);
            if (NULL != pSub)
            {
                start_utc = cJSON_GetObjectItem(pSub, "start_utc");
                end_utc = cJSON_GetObjectItem(pSub, "end_utc");
            }
        }
        if (time_utc_s && time_zone && start_utc && end_utc)
        {
            PR_DEBUG("time info time_utc_s[%d] time_zone[%d] start_utc[%d],\
                    end_utc[%d], dls_list_num[%d]",
                     time_utc_s->valueint,
                     time_zone->valueint, start_utc->valueint, end_utc->valueint,
                     dls_list_num);
        }
        else
        {
            return MM_ERR_INVALID_PARAM;
        }
        break;
    }
    case LP_PROTO_O_SDK_GET_URL:
    {
        PR_DEBUG("receive get url cmd");
        break;
    }
    case LP_PROTO_O_SDK_SET_URL:
    {
        PR_DEBUG("get url");
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam)
        {
            PR_DEBUG("no net info");
            return MM_ERR_INVALID_PARAM;
        }
        else
        {
            __cmd_parse_print_debug(pParam);
        }
        cJSON *httpUrl = cJSON_GetObjectItem(pParam, "httpUrl");
        cJSON *mqttUrl = cJSON_GetObjectItem(pParam, "mqttUrl");
        cJSON *httpsSelfUrl = cJSON_GetObjectItem(pParam, "httpsSelfUrl");
        cJSON *mqttsSelfUrl = cJSON_GetObjectItem(pParam, "mqttsSelfUrl");
        if (httpUrl && mqttUrl && httpsSelfUrl && mqttsSelfUrl)
        {
            PR_DEBUG("tuya url: httpurl[%s] mqtturl[%s] httpsSelfUrl[%s] mqttSelfUrl[%s]", httpUrl->valuestring, mqttUrl->valuestring,
                     httpsSelfUrl->valuestring, mqttsSelfUrl->valuestring);
        }
        else
        {
            return MM_ERR_INVALID_PARAM;
        }
        break;
    }
    case LP_PROTO_O_SDK_GET_UPGRADE_URL:
    {
        PR_DEBUG("receive get upgrade url cmd");
        break;
    }
    case LP_PROTO_O_SDK_SET_UPGRADE_URL:
    {
        PR_DEBUG("set upgrade url");
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam)
        {
            PR_DEBUG("no upgrade url info");
            return MM_ERR_INVALID_PARAM;
        }
        else
        {
            __cmd_parse_print_debug(pParam);
        }
        cJSON *url = cJSON_GetObjectItem(pParam, "url");
        if (url)
        {
            PR_DEBUG("upgrade url info [%s]", url->valuestring);
        }
        else
        {
            return MM_ERR_INVALID_PARAM;
        }
        break;
    }
    case LP_PROTO_O_SDK_SET_QRCODE_RESULT:
    {
        PR_DEBUG("receive set qrcode result cmd");
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam)
        {
            PR_DEBUG("no qrcode result info");
            return MM_ERR_INVALID_PARAM;
        }
        else
        {
            __cmd_parse_print_debug(pParam);
        }
        cJSON *ssid = cJSON_GetObjectItem(pParam, "ssid");
        cJSON *passwd = cJSON_GetObjectItem(pParam, "passwd");
        cJSON *token = cJSON_GetObjectItem(pParam, "token");
        if (ssid && passwd && token)
        {
            save_gw_info(ssid->valuestring, passwd->valuestring, token->valuestring);
            driver_notify_netcfg_complete(ssid->valuestring, passwd->valuestring, token->valuestring);
        }
        else
        {
            return MM_ERR_INVALID_PARAM;
        }
        break;
    }
    // for mqtt case
    case LP_PROTO_O_MQTT_SET_BOOK_TOPIC:
    {
        PR_DEBUG("receive set qrcode result cmd");
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam)
        {
            PR_DEBUG("no book topic param info");
            return MM_ERR_INVALID_PARAM;
        }
        else
        {
            /*__cmd_parse_print_debug(pParam);*/
        }
        cJSON *add_topic1 = cJSON_GetObjectItem(pParam, "add_topic1");
        if (add_topic1)
        {
            PR_DEBUG("add_topic info add_topic1[%s]", add_topic1->valuestring);
        }
        else
        {
            return MM_ERR_INVALID_PARAM;
        }
        break;
    }
    case LP_PROTO_O_MQTT_PROTOCOL_REGISTER:
    {
        PR_DEBUG("receive mqtt protocal register cmd");
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam)
        {
            PR_DEBUG("no book topic param info");
            return MM_ERR_INVALID_PARAM;
        }
        else
        {
            /*__cmd_parse_print_debug(pParam);*/
        }
        cJSON *dp_cmd = cJSON_GetObjectItem(pParam, "dp_cmd");
        cJSON *p2p_cmd = cJSON_GetObjectItem(pParam, "p2p_cmd");
        cJSON *cloud_stream = cJSON_GetObjectItem(pParam, "cloud_stream");
        cJSON *ipc_private = cJSON_GetObjectItem(pParam, "ipc_private");
        if (dp_cmd && p2p_cmd && cloud_stream && ipc_private)
        {
            PR_DEBUG("mqtt protocal register info dp_cmd [%d],p2p_cmd[%d],\
                    cloud_stream[%d],ipc_private[%d]",
                     dp_cmd->valueint,
                     p2p_cmd->valueint, cloud_stream->valueint, ipc_private->valueint);
        }
        else
        {
            return MM_ERR_INVALID_PARAM;
        }
        break;
    }
    case LP_PROTO_O_MQTT_RECV_DATA:
    case LP_PROTO_O_MQTT_RECV_DATA_ASYNC:
    {
        PR_DEBUG("recv mqtt data cmd");
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam)
        {
            PR_DEBUG("no recv data info");
        }
        else
        {
            __cmd_parse_print_debug(pParam);
        }
        break;
    }
    case LP_PROTO_O_MQTT_SEND_DATA:
    case LP_PROTO_O_MQTT_SEND_DATA_ASYNC:
    {
        PR_DEBUG("recv send mqtt data cmd ");
        pParam = cJSON_GetObjectItem(pRoot, "param");
        if (NULL == pParam)
        {
            PR_DEBUG("no recv data info");
        }
        else
        {
            /*__cmd_parse_print_debug(pParam);*/
            /*goto exit;*/
            cJSON *proto = cJSON_GetObjectItem(pParam, "proto");
            cJSON *data = cJSON_GetObjectItem(pParam, "data");
            cJSON *slice = cJSON_GetObjectItem(pParam, "slice");
            cJSON *topic = cJSON_GetObjectItem(pParam, "topic");
            if (NULL == proto || NULL == data || NULL == slice || NULL == topic)
            {
                PR_ERR("noprotoordata");
                return MM_ERR_INVALID_PARAM;
            }
            if (NULL == topic->valuestring || NULL == data->valuestring)
            {
                PR_ERR("error data or topic");
                return MM_ERR_INVALID_PARAM;
            }
            if (strncmp(topic->valuestring, "smart/device", sizeof("smart/device") - 1) == 0 ||
                strncmp(topic->valuestring, "log/log_seq", sizeof("log/log_seq") - 1) == 0)
            {
                pack_mqtt_data_app(proto->valueint, data->valuestring, slice->valueint, topic->valuestring);
            }
            else if (strncmp(topic->valuestring, "/av/moto/", sizeof("/av/moto/") - 1) == 0)
            {
                pack_mqtt_data_web(proto->valueint, data->valuestring, slice->valueint, topic->valuestring);
            }
            else
            {
                PR_ERR("unknown topic");
            }
        }
        break;
    }
#endif /* MM_ENABLE_MQTT_KEEPALIVE */

    default:
    {
        PR_ERR("cur not support [%d]", cmd);
        break;
    }
    }
    return 0;
}

#if defined(TUYA_SPI_RETRY) && (TUYA_SPI_RETRY == 1)
void ty_lp_proto_timeout_check(void)
{
    if (g_max_fn_in_buffer > g_max_proc_fn && g_last_proc_time + 3000 < tkl_system_get_millisecond())
    {
        PR_ERR(" SPI time out frame %d lost, parse next", ++g_max_proc_fn);
        ty_lp_proto_handle_cmd_in_buffer();
    }
}

static int ty_lp_proto_handle_cmd_in_buffer(void)
{
    while (1)
    {
        TY_LP_PROTO_RX_NODE_T cur_data_node = {0};
        int ret = 0;
        ret = __rx_list_get_node(&cur_data_node, g_max_proc_fn + 1);
        if (ret == 0)
        { // 找到了下一个序号的数据就上报到应用
            cJSON *pRoot = cJSON_Parse(cur_data_node.data);
            PR_INFO("parse cmd [%d-%d-%d]", cur_data_node.cmd, 0, cur_data_node.frame_num);
            ret = ty_lp_proto_cmd_handle(cur_data_node.cmd, pRoot);
            cJSON_Delete(pRoot);
            g_max_proc_fn++; // 序号加一
            tkl_system_free(cur_data_node.data);
        }
        else
        { // 未找到合适的命令退出
            break;
        }
    }
    return 0;
}
#endif

int ty_lp_proto_cs_data_parse(unsigned char *data, int len, unsigned int crc32)
{
    if (0 != crc32) {
#if 0
//verify checksum
        unsigned int crc_cs = 0;
        unsigned int i = 0;

        for (i = 0; i < len; i++) {
            crc_cs += data[i];
        }

        if (crc_cs != crc32) {
            PR_ERR("\ndata parse %d crc fail c32:%d cs:%d l:%d ", __LINE__, crc32, crc_cs, len);
        }
        else {
            // PR_INFO("\ncmd c ok");
        }
#endif
    }
    TY_LP_PROTO_CMD_HEAD_T *pHead = NULL;

    if (NULL == data || len < (int)sizeof(TY_LP_PROTO_CMD_HEAD_T))
    {
        PR_ERR("input error [%x]", len);
        return MM_ERR_INVALID_PARAM;
    }
    pHead = (TY_LP_PROTO_CMD_HEAD_T *)data;
    if (LP_PROTO_HEAD_MARK != pHead->mark)
    {
        PR_ERR("mark error [%x]", pHead->mark);
        return MM_ERR_GENERAL;
    }

    switch (pHead->payLoadType)
    {
    case TY_LP_PROTO_TYPE_JSON:
    {
        if (0 != __lp_proto_cs_json_parse(data + sizeof(TY_LP_PROTO_CMD_HEAD_T), len - sizeof(TY_LP_PROTO_CMD_HEAD_T)))
        {
            PR_ERR("json_parse failed");
            return MM_ERR_GENERAL;
        }
        break;
    }
    case TY_LP_PROTO_TYPE_BIN:
    {
        break;
    }
    default:
    {
        PR_ERR("cur not support [%d]", pHead->payLoadType);
        break;
    }
    }
    return 0;
}

int ty_lp_proto_set_wifi_scan_cs(void)
{
    PR_DEBUG("send wifi scan cmd");
    return __lp_proto_json_com_pack(SPI_T_WIFI_SCAN, NULL);
}

int ty_lp_proto_wifi_scan_begin_cs(void)
{
    PR_DEBUG("send wifi scan begin");
    return __lp_proto_json_com_pack(SPI_M_WIFI_SCAN_START, NULL);
}

int ty_lp_proto_wifi_scan_end_cs(void)
{
    PR_DEBUG("send wifi scan end");
    return __lp_proto_json_com_pack(SPI_M_WIFI_SCAN_END, NULL);
}

int ty_lp_proto_wifi_scan_result_cs(char *data, int len)
{
    cJSON *param = cJSON_Parse(data);
    if (NULL == param) {
        PR_ERR("wifi scan result param parse failed");
        return MM_ERR_MALLOC_FAIL;
    }

    return __lp_proto_json_com_pack(SPI_M_WIFI_SCAN, param);
}

int ty_lp_proto_wifi_conn_finish_cs(int error)
{
    cJSON *param = NULL;
    if (error == OPRT_OK) {
        NW_IP_S ip_info = {0};
        if (tal_wifi_get_ip(WF_STATION, &ip_info) == OPRT_OK && ip_info.ip[0] != '\0') {
            param = cJSON_CreateObject();
            if (param) {
                cJSON_AddStringToObject(param, "ip_addr", ip_info.ip);
            }
            PR_INFO("wifi conn finish, ip=%s", ip_info.ip);
        } else {
            PR_ERR("wifi_conn_finish: get ip failed");
            error = MM_ERR_GENERAL;
        }
    } else {
        PR_ERR("wifi_conn_finish: conn failed, error=%d", error);
    }
    return __lp_proto_cs_send_ack(SPI_M_WIFI_CON_FINISH, 0, param, error);
}

int ty_lp_proto_netcfg_start(void)
{
    PR_INFO("%s:%d", __func__, __LINE__);
    return 0;
}

int ty_lp_proto_netcfg_complete(void)
{
    PR_INFO("%s:%d", __func__, __LINE__);
    return 0;
}

#if defined(MM_ENABLE_MQTT_KEEPALIVE)
int ty_lp_proto_mqtt_get_stat_cs(void)
{
    return __lp_proto_json_com_pack(LP_PROTO_O_MQTT_GET_STAT, NULL);
}

int ty_lp_proto_mqtt_set_stat_cs(int stat)
{
    PR_DEBUG("set mqtt stat ");
    cJSON *param = cJSON_CreateObject();
    if (NULL == param)
    {
        PR_ERR("un like error");
        return MM_ERR_MALLOC_FAIL;
    }
    cJSON_AddItemToObject(param, "stat", cJSON_CreateNumber(stat));

    PR_INFO("%s:%d stat:%d ***************", __func__, __LINE__, stat);
    return __lp_proto_json_com_pack(LP_PROTO_O_MQTT_SET_STAT, param);
}

int ty_lp_proto_mqtt_set_book_topic_cs(char *topic)
{
    PR_DEBUG("set mqtt add book topic ");
    cJSON *param = cJSON_CreateObject();
    if (NULL == param)
    {
        PR_ERR("un like error");
        return MM_ERR_MALLOC_FAIL;
    }
    cJSON_AddItemToObject(param, "add_topic1", cJSON_CreateString(topic));
    return __lp_proto_json_com_pack(LP_PROTO_O_MQTT_SET_BOOK_TOPIC, param);
}

int ty_lp_proto_mqtt_protocal_register_cs(int dp_cmd, int p2p_cmd, int cloud_stream, int ipc_private)
{
    PR_DEBUG("set protocal register ");
    cJSON *param = cJSON_CreateObject();
    if (NULL == param)
    {
        PR_ERR("un like error");
        return MM_ERR_MALLOC_FAIL;
    }
    cJSON_AddItemToObject(param, "dp_cmd", cJSON_CreateNumber(dp_cmd));
    cJSON_AddItemToObject(param, "p2p_cmd", cJSON_CreateNumber(p2p_cmd));
    cJSON_AddItemToObject(param, "cloud_stream", cJSON_CreateNumber(cloud_stream));
    cJSON_AddItemToObject(param, "ipc_private", cJSON_CreateNumber(ipc_private));
    return __lp_proto_json_com_pack(LP_PROTO_O_MQTT_PROTOCOL_REGISTER, param);
}
#endif /* MM_ENABLE_MQTT_KEEPALIVE */

/**
 * @brief Convert bytes to upper hex string
 * @param[in]  bin     bytes
 * @param[in]  bin_len bytes length
 * @param[out] out     output string buffer
 * @param[in]  out_sz  output buffer size
 * @return 0 success, -1 failure
 */
static int __bin_to_hex(const unsigned char *bin, size_t bin_len, char *out, size_t out_sz)
{
    static const char *hex = "0123456789ABCDEF";
    if (bin == NULL || out == NULL || out_sz == 0U) {
        return -1;
    }
    if (out_sz < (bin_len * 2U + 1U)) {
        return -1;
    }
    for (size_t i = 0; i < bin_len; i++) {
        out[i * 2U] = hex[(bin[i] >> 4) & 0x0F];
        out[i * 2U + 1U] = hex[bin[i] & 0x0F];
    }
    out[bin_len * 2U] = '\0';
    return 0;
}

/**
 * @brief Encode BLE UUID to JSON object
 * @param[in] uuid UUID
 * @return cJSON object or NULL
 */
static cJSON *__uuid_to_json(const TKL_BLE_UUID_T *uuid)
{
    if (uuid == NULL) {
        return NULL;
    }
    cJSON *obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }
    cJSON_AddItemToObject(obj, "uuid_type", cJSON_CreateNumber((int)uuid->uuid_type));
    if (uuid->uuid_type == TKL_BLE_UUID_TYPE_16) {
        cJSON_AddItemToObject(obj, "uuid16", cJSON_CreateNumber((int)uuid->uuid.uuid16));
    } else if (uuid->uuid_type == TKL_BLE_UUID_TYPE_32) {
        cJSON_AddItemToObject(obj, "uuid32", cJSON_CreateNumber((double)uuid->uuid.uuid32));
    } else if (uuid->uuid_type == TKL_BLE_UUID_TYPE_128) {
        char uhex[33];
        memset(uhex, 0, sizeof(uhex));
        (void)__bin_to_hex(uuid->uuid.uuid128, 16, uhex, sizeof(uhex));
        cJSON_AddItemToObject(obj, "uuid128_hex", cJSON_CreateString(uhex));
    }
    return obj;
}

/**
 * @brief Encode BLE data report to JSON
 * @param[in] data BLE data
 * @param[in] max_len clamp length
 * @return cJSON object or NULL
 */
static cJSON *__ble_data_to_json(const TKL_BLE_DATA_T *data, size_t max_len)
{
    cJSON *obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }
    if (data == NULL || data->p_data == NULL || data->length == 0) {
        cJSON_AddItemToObject(obj, "len", cJSON_CreateNumber(0));
        cJSON_AddItemToObject(obj, "data_hex", cJSON_CreateString(""));
        return obj;
    }
    size_t dl = data->length;
    if (dl > max_len) {
        dl = max_len;
    }
    char *hex = (char *)malloc(dl * 2U + 1U);
    if (hex == NULL) {
        cJSON_Delete(obj);
        return NULL;
    }
    if (__bin_to_hex(data->p_data, dl, hex, dl * 2U + 1U) == 0) {
        cJSON_AddItemToObject(obj, "len", cJSON_CreateNumber((int)dl));
        cJSON_AddItemToObject(obj, "data_hex", cJSON_CreateString(hex));
        cJSON_AddItemToObject(obj, "origin_len", cJSON_CreateNumber((int)data->length));
    }
    tkl_system_free(hex);
    return obj;
}

/**
 * @brief Convert GATT event struct to JSON (type-dependent union fully preserved)
 * @param[in] evt GATT event
 * @return cJSON object or NULL
 */
static cJSON *__gatt_evt_to_json(const TKL_BLE_GATT_PARAMS_EVT_T *evt)
{
    if (evt == NULL) {
        return NULL;
    }
    cJSON *obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }
    cJSON_AddItemToObject(obj, "type", cJSON_CreateNumber((int)evt->type));
    cJSON_AddItemToObject(obj, "conn_handle", cJSON_CreateNumber((int)evt->conn_handle));
    cJSON_AddItemToObject(obj, "result", cJSON_CreateNumber((int)evt->result));

    if (evt->type == TKL_BLE_GATT_EVT_MTU_REQUEST || evt->type == TKL_BLE_GATT_EVT_MTU_RSP) {
        cJSON_AddItemToObject(obj, "exchange_mtu", cJSON_CreateNumber((int)evt->gatt_event.exchange_mtu));
    } else if (evt->type == TKL_BLE_GATT_EVT_PRIM_SEV_DISCOVERY) {
        cJSON *sd = cJSON_CreateObject();
        if (sd) {
            cJSON_AddItemToObject(sd, "svc_num", cJSON_CreateNumber((int)evt->gatt_event.svc_disc.svc_num));
            cJSON *arr = cJSON_CreateArray();
            if (arr) {
                for (int i = 0; i < (int)evt->gatt_event.svc_disc.svc_num && i < TKL_BLE_GATT_SERVICE_MAX_NUM; i++) {
                    cJSON *it = cJSON_CreateObject();
                    if (it) {
                        cJSON *u = __uuid_to_json(&evt->gatt_event.svc_disc.services[i].uuid);
                        if (u) {
                            cJSON_AddItemToObject(it, "uuid", u);
                        }
                        cJSON_AddItemToObject(it, "start_handle", cJSON_CreateNumber((int)evt->gatt_event.svc_disc.services[i].start_handle));
                        cJSON_AddItemToObject(it, "end_handle", cJSON_CreateNumber((int)evt->gatt_event.svc_disc.services[i].end_handle));
                        cJSON_AddItemToArray(arr, it);
                    }
                }
                cJSON_AddItemToObject(sd, "services", arr);
            }
            cJSON_AddItemToObject(obj, "svc_disc", sd);
        }
    } else if (evt->type == TKL_BLE_GATT_EVT_CHAR_DISCOVERY) {
        cJSON *cd = cJSON_CreateObject();
        if (cd) {
            cJSON_AddItemToObject(cd, "char_num", cJSON_CreateNumber((int)evt->gatt_event.char_disc.char_num));
            cJSON *arr = cJSON_CreateArray();
            if (arr) {
                for (int i = 0; i < (int)evt->gatt_event.char_disc.char_num && i < TKL_BLE_GATT_CHAR_MAX_NUM; i++) {
                    cJSON *it = cJSON_CreateObject();
                    if (it) {
                        cJSON *u = __uuid_to_json(&evt->gatt_event.char_disc.characteristics[i].uuid);
                        if (u) {
                            cJSON_AddItemToObject(it, "uuid", u);
                        }
                        cJSON_AddItemToObject(it, "handle", cJSON_CreateNumber((int)evt->gatt_event.char_disc.characteristics[i].handle));
                        cJSON_AddItemToArray(arr, it);
                    }
                }
                cJSON_AddItemToObject(cd, "characteristics", arr);
            }
            cJSON_AddItemToObject(obj, "char_disc", cd);
        }
    } else if (evt->type == TKL_BLE_GATT_EVT_CHAR_DESC_DISCOVERY) {
        cJSON_AddItemToObject(obj, "cccd_handle", cJSON_CreateNumber((int)evt->gatt_event.desc_disc.cccd_handle));
    } else if (evt->type == TKL_BLE_GATT_EVT_NOTIFY_TX) {
        cJSON *nr = cJSON_CreateObject();
        if (nr) {
            cJSON_AddItemToObject(nr, "char_handle", cJSON_CreateNumber((int)evt->gatt_event.notify_result.char_handle));
            cJSON_AddItemToObject(nr, "result", cJSON_CreateNumber((int)evt->gatt_event.notify_result.result));
            cJSON_AddItemToObject(obj, "notify_result", nr);
        }
    } else if (evt->type == TKL_BLE_GATT_EVT_WRITE_REQ) {
        cJSON *wr = cJSON_CreateObject();
        if (wr) {
            cJSON_AddItemToObject(wr, "char_handle", cJSON_CreateNumber((int)evt->gatt_event.write_report.char_handle));
            cJSON *dr = __ble_data_to_json(&evt->gatt_event.write_report.report, 512);
            if (dr) {
                cJSON_AddItemToObject(wr, "report", dr);
            }
            cJSON_AddItemToObject(obj, "write_report", wr);
        }
    } else if (evt->type == TKL_BLE_GATT_EVT_NOTIFY_INDICATE_RX || evt->type == TKL_BLE_GATT_EVT_READ_RX) {
        cJSON *drp = cJSON_CreateObject();
        if (drp) {
            cJSON_AddItemToObject(drp, "char_handle", cJSON_CreateNumber((int)evt->gatt_event.data_report.char_handle));
            cJSON *dr = __ble_data_to_json(&evt->gatt_event.data_report.report, 512);
            if (dr) {
                cJSON_AddItemToObject(drp, "report", dr);
            }
            cJSON_AddItemToObject(obj, "data_report", drp);
        }
    } else if (evt->type == TKL_BLE_GATT_EVT_SUBSCRIBE) {
        cJSON *sb = cJSON_CreateObject();
        if (sb) {
            cJSON_AddItemToObject(sb, "char_handle", cJSON_CreateNumber((int)evt->gatt_event.subscribe.char_handle));
            cJSON_AddItemToObject(sb, "reason", cJSON_CreateNumber((int)evt->gatt_event.subscribe.reason));
            cJSON_AddItemToObject(sb, "prev_notify", cJSON_CreateNumber((int)evt->gatt_event.subscribe.prev_notify));
            cJSON_AddItemToObject(sb, "cur_notify", cJSON_CreateNumber((int)evt->gatt_event.subscribe.cur_notify));
            cJSON_AddItemToObject(sb, "prev_indicate", cJSON_CreateNumber((int)evt->gatt_event.subscribe.prev_indicate));
            cJSON_AddItemToObject(sb, "cur_indicate", cJSON_CreateNumber((int)evt->gatt_event.subscribe.cur_indicate));
            cJSON_AddItemToObject(obj, "subscribe", sb);
        }
    } else if (evt->type == TKL_BLE_GATT_EVT_READ_CHAR_VALUE) {
        cJSON *cr = cJSON_CreateObject();
        if (cr) {
            cJSON_AddItemToObject(cr, "char_handle", cJSON_CreateNumber((int)evt->gatt_event.char_read.char_handle));
            cJSON_AddItemToObject(cr, "offset", cJSON_CreateNumber((int)evt->gatt_event.char_read.offset));
            cJSON_AddItemToObject(obj, "char_read", cr);
        }
    }

    return obj;
}

static void mm_ble_gatt_evt_handler(TKL_BLE_GATT_PARAMS_EVT_T *p_event)
{
    if (p_event == NULL) {
        return;
    }

    PR_DEBUG("GATT_EVT type=%d conn=%u result=%d",
             (int)p_event->type, (unsigned)p_event->conn_handle, (int)p_event->result);

    int cmd = -1;
    if (p_event->type == TKL_BLE_GATT_EVT_WRITE_REQ) {
        cmd = SPI_M_BLE_GATTS_WRITE_EVT;
    } else if (p_event->type == TKL_BLE_GATT_EVT_READ_CHAR_VALUE) {
        cmd = SPI_M_BLE_GATTS_READ_EVT;
    } else if (p_event->type == TKL_BLE_GATT_EVT_MTU_REQUEST || p_event->type == TKL_BLE_GATT_EVT_MTU_RSP) {
        cmd = SPI_M_BLE_GATTS_MTU_EVT;
    } else if (p_event->type == TKL_BLE_GATT_EVT_NOTIFY_TX) {
        cmd = SPI_M_BLE_GATTS_NOTIFY_COMPLETE;
    } else if (p_event->type == TKL_BLE_GATT_EVT_NOTIFY_INDICATE_RX || p_event->type == TKL_BLE_GATT_EVT_READ_RX) {
        cmd = SPI_M_BLE_GATTC_PEER_DATA;
    } else if (p_event->type == TKL_BLE_GATT_EVT_PRIM_SEV_DISCOVERY ||
               p_event->type == TKL_BLE_GATT_EVT_CHAR_DISCOVERY ||
               p_event->type == TKL_BLE_GATT_EVT_CHAR_DESC_DISCOVERY ||
               p_event->type == TKL_BLE_GATT_EVT_SUBSCRIBE) {
        cmd = SPI_M_BLE_GATTC_PEER_DATA;
    }

    if (cmd >= 0) {
        cJSON *p = cJSON_CreateObject();
        if (p) {
            cJSON *ge = __gatt_evt_to_json(p_event);
            if (ge) {
                cJSON_AddItemToObject(p, "gatt_evt", ge);
            }
            PR_DEBUG("GATT_EVT->JSON cmd=%d(%s)", cmd, __cmd_name(cmd));
            __lp_proto_json_com_pack((TY_LP_PROTO_JSON_COM_E)cmd, p);
        }
    }
}

/**
 * @brief Report GAP async event to master via SPI_M_BLE_*
 * @param[in] cmd  async cmd id
 * @param[in] param json param (may be NULL)
 * @return none
 */
static void __ble_report_async(int cmd, cJSON *param)
{
    __lp_proto_json_com_pack((TY_LP_PROTO_JSON_COM_E)cmd, param);
}

/**
 * @brief Encode BLE address to JSON object
 * @param[in] addr BLE address
 * @return cJSON object or NULL
 */
static cJSON *__addr_to_json(const TKL_BLE_GAP_ADDR_T *addr)
{
    if (addr == NULL) {
        return NULL;
    }
    cJSON *obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }
    char addr_hex[13];
    memset(addr_hex, 0, sizeof(addr_hex));
    (void)__bin_to_hex(addr->addr, 6, addr_hex, sizeof(addr_hex));
    cJSON_AddItemToObject(obj, "type", cJSON_CreateNumber((int)addr->type));
    cJSON_AddItemToObject(obj, "addr_hex", cJSON_CreateString(addr_hex));
    return obj;
}

/**
 * @brief Encode GAP conn params to JSON
 * @param[in] cp conn params
 * @return cJSON object or NULL
 */
static cJSON *__conn_params_to_json(const TKL_BLE_GAP_CONN_PARAMS_T *cp)
{
    if (cp == NULL) {
        return NULL;
    }
    cJSON *obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }
    cJSON_AddItemToObject(obj, "conn_interval_min", cJSON_CreateNumber((int)cp->conn_interval_min));
    cJSON_AddItemToObject(obj, "conn_interval_max", cJSON_CreateNumber((int)cp->conn_interval_max));
    cJSON_AddItemToObject(obj, "conn_latency", cJSON_CreateNumber((int)cp->conn_latency));
    cJSON_AddItemToObject(obj, "conn_sup_timeout", cJSON_CreateNumber((int)cp->conn_sup_timeout));
    cJSON_AddItemToObject(obj, "connection_timeout", cJSON_CreateNumber((int)cp->connection_timeout));
    return obj;
}

/**
 * @brief Convert GAP event struct to JSON (type-dependent union fully preserved)
 * @param[in] evt GAP event
 * @return cJSON object or NULL
 */
static cJSON *__gap_evt_to_json(const TKL_BLE_GAP_PARAMS_EVT_T *evt)
{
    if (evt == NULL) {
        return NULL;
    }
    cJSON *obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }
    cJSON_AddItemToObject(obj, "type", cJSON_CreateNumber((int)evt->type));
    cJSON_AddItemToObject(obj, "conn_handle", cJSON_CreateNumber((int)evt->conn_handle));
    cJSON_AddItemToObject(obj, "result", cJSON_CreateNumber((int)evt->result));

    if (evt->type == TKL_BLE_GAP_EVT_CONNECT) {
        cJSON *co = cJSON_CreateObject();
        if (co) {
            cJSON_AddItemToObject(co, "role", cJSON_CreateNumber((int)evt->gap_event.connect.role));
            cJSON *pa = __addr_to_json(&evt->gap_event.connect.peer_addr);
            if (pa) {
                cJSON_AddItemToObject(co, "peer_addr", pa);
            }
            cJSON *cp = __conn_params_to_json(&evt->gap_event.connect.conn_params);
            if (cp) {
                cJSON_AddItemToObject(co, "conn_params", cp);
            }
            cJSON_AddItemToObject(obj, "connect", co);
        }
    } else if (evt->type == TKL_BLE_GAP_EVT_DISCONNECT) {
        cJSON *dc = cJSON_CreateObject();
        if (dc) {
            cJSON_AddItemToObject(dc, "role", cJSON_CreateNumber((int)evt->gap_event.disconnect.role));
            cJSON_AddItemToObject(dc, "reason", cJSON_CreateNumber((int)evt->gap_event.disconnect.reason));
            cJSON_AddItemToObject(obj, "disconnect", dc);
        }
    } else if (evt->type == TKL_BLE_GAP_EVT_ADV_REPORT) {
        cJSON *ar = cJSON_CreateObject();
        if (ar) {
            cJSON_AddItemToObject(ar, "adv_type", cJSON_CreateNumber((int)evt->gap_event.adv_report.adv_type));
            cJSON_AddItemToObject(ar, "rssi", cJSON_CreateNumber((int)evt->gap_event.adv_report.rssi));
            cJSON_AddItemToObject(ar, "channel_index", cJSON_CreateNumber((int)evt->gap_event.adv_report.channel_index));
            cJSON *pa = __addr_to_json(&evt->gap_event.adv_report.peer_addr);
            if (pa) {
                cJSON_AddItemToObject(ar, "peer_addr", pa);
            }
            cJSON *dr = __ble_data_to_json(&evt->gap_event.adv_report.data, 512);
            if (dr) {
                cJSON_AddItemToObject(ar, "data", dr);
            }
            cJSON_AddItemToObject(obj, "adv_report", ar);
        }
    } else if (evt->type == TKL_BLE_GAP_EVT_CONN_PARAM_REQ || evt->type == TKL_BLE_GAP_EVT_CONN_PARAM_UPDATE) {
        cJSON *cp = __conn_params_to_json(&evt->gap_event.conn_param);
        if (cp) {
            cJSON_AddItemToObject(obj, "conn_param", cp);
        }
    } else if (evt->type == TKL_BLE_GAP_EVT_CONN_RSSI) {
        cJSON_AddItemToObject(obj, "link_rssi", cJSON_CreateNumber((int)evt->gap_event.link_rssi));
    }

    return obj;
}

/**
 * @brief BLE GAP event callback (slave-side real stack)
 * @param[in] p_event event params
 * @return none
 */
static VOID mm_ble_gap_evt_handler(TKL_BLE_GAP_PARAMS_EVT_T *p_event)
{
    if (p_event == NULL) {
        return;
    }

    PR_DEBUG("GAP_EVT type=%d conn=%u result=%d",
           (int)p_event->type, (unsigned)p_event->conn_handle, (int)p_event->result);

    int cmd = -1;
    if (p_event->type == TKL_BLE_GAP_EVT_CONNECT) {
        cmd = SPI_M_BLE_GAP_CONNECT_EVT;
    } else if (p_event->type == TKL_BLE_GAP_EVT_DISCONNECT) {
        cmd = SPI_M_BLE_GAP_DISCONNECT_EVT;
    } else if (p_event->type == TKL_BLE_GAP_EVT_ADV_REPORT) {
        cmd = SPI_M_BLE_GAP_SCAN_RESULT;
    } else if (p_event->type == TKL_BLE_GAP_EVT_CONN_PARAM_REQ || p_event->type == TKL_BLE_GAP_EVT_CONN_PARAM_UPDATE) {
        cmd = SPI_M_BLE_GAP_CONN_PARAM_EVT;
    } else if (p_event->type == TKL_BLE_GAP_EVT_CONN_RSSI) {
        cmd = SPI_T_BLE_GAP_RSSI_GET;
    }

    if (cmd >= 0) {
        cJSON *p = cJSON_CreateObject();
        if (p) {
            cJSON *ge = __gap_evt_to_json(p_event);
            if (ge) {
                cJSON_AddItemToObject(p, "gap_evt", ge);
            }
            char *js = cJSON_PrintUnformatted(p);
            if (js) {
                PR_DEBUG("GAP_EVT->JSON cmd=%d(%s) | %s", cmd, __cmd_name(cmd), js);
                cJSON_free(js);
            }
            __ble_report_async(cmd, p);
        }
    }
}

/**
 * @brief BLE GATT event callback (slave-side real stack)
 * @param[in] p_event event params
 * @return none
 */
int ty_lp_proto_gatts_callback_register(void)
{
    tkl_ble_gatt_callback_register(mm_ble_gatt_evt_handler);
    return 0;
}

int ty_lp_proto_gap_callback_register(void)
{
    tkl_ble_gap_callback_register(mm_ble_gap_evt_handler);
    return 0;
}

int ty_lp_proto_ble_gatts_write_evt_cs(uint16_t conn_handle, uint16_t char_handle,
                                        const uint8_t *data, uint16_t len)
{
    cJSON *param = cJSON_CreateObject();
    if (NULL == param) {
        PR_ERR("alloc failed");
        return MM_ERR_MALLOC_FAIL;
    }
    /* convert binary data to upper-case hex string */
    char *hex = NULL;
    char hex_empty[] = "";
    if (len > 0) {
        hex = (char *)tkl_system_malloc(len * 2 + 1);
    }
    if (hex) {
        for (uint16_t i = 0; i < len; i++) {
            snprintf(hex + i * 2, 3, "%02X", data[i]);
        }
        hex[len * 2] = '\0';
    } else {
        hex = hex_empty;
    }
    cJSON_AddItemToObject(param, "conn_handle", cJSON_CreateNumber(conn_handle));
    cJSON_AddItemToObject(param, "char_handle", cJSON_CreateNumber(char_handle));
    cJSON_AddItemToObject(param, "data",        cJSON_CreateString(hex));
    cJSON_AddItemToObject(param, "len",         cJSON_CreateNumber(len));
    if (hex != hex_empty) {
        tkl_system_free(hex);
    }
    PR_INFO("gatts write evt conn=%u char=%u len=%u", conn_handle, char_handle, len);
    return __lp_proto_json_com_pack(SPI_M_BLE_GATTS_WRITE_EVT, param);
}

int ty_lp_proto_ble_gatts_mtu_evt_cs(uint16_t conn_handle, uint16_t mtu)
{
    cJSON *param = cJSON_CreateObject();
    if (NULL == param) {
        PR_ERR("alloc failed");
        return MM_ERR_MALLOC_FAIL;
    }
    cJSON_AddItemToObject(param, "conn_handle", cJSON_CreateNumber(conn_handle));
    cJSON_AddItemToObject(param, "mtu",         cJSON_CreateNumber(mtu));
    PR_INFO("gatts mtu evt conn=%u mtu=%u", conn_handle, mtu);
    return __lp_proto_json_com_pack(SPI_M_BLE_GATTS_MTU_EVT, param);
}

int ty_lp_proto_ble_gatts_read_evt_cs(uint16_t conn_handle, uint16_t char_handle)
{
    cJSON *param = cJSON_CreateObject();
    if (NULL == param) {
        PR_ERR("alloc failed");
        return MM_ERR_MALLOC_FAIL;
    }
    cJSON_AddItemToObject(param, "conn_handle", cJSON_CreateNumber(conn_handle));
    cJSON_AddItemToObject(param, "char_handle", cJSON_CreateNumber(char_handle));
    PR_INFO("gatts read evt conn=%u char=%u", conn_handle, char_handle);
    return __lp_proto_json_com_pack(SPI_M_BLE_GATTS_READ_EVT, param);
}

static void mm_ble_conn_state_handler(ble_conn_evt_t event, ble_conn_data_u *p_data)
{
    if (p_data == NULL) {
        return;
    }
    if (event == BLE_CONN_EVT_STATE_CHG) {
        if (p_data->conn_state.state == BLE_CONN_STATE_CONNECTED) {
            const ble_gap_conn_info_t *ci = &p_data->conn_state.info.conn_info;
            /* role: MSDK 0=Central→SPI 2, MSDK 1=Peripheral→SPI 1 */
            uint8_t spi_role = (ci->role == 0) ? 2 : 1;
            ty_lp_proto_ble_gap_connect_evt_cs(ci->conn_hdl, spi_role,
                                                ci->peer_addr.addr, ci->peer_addr.addr_type);
        } else if (p_data->conn_state.state == BLE_CONN_STATE_DISCONNECTD) {
            const ble_gap_disconn_info_t *di = &p_data->conn_state.info.discon_info;
            ty_lp_proto_ble_gap_disconnect_evt_cs(di->conn_hdl, di->reason);
        }
    } else if (event == BLE_CONN_EVT_RSSI_GET_RSP) {
        cJSON *param = cJSON_CreateObject();
        if (param) {
            cJSON_AddItemToObject(param, "rssi", cJSON_CreateNumber(p_data->rssi_ind.rssi));
        }
        PR_INFO("ble rssi rsp rssi=%d", (int)p_data->rssi_ind.rssi);
        __lp_proto_cs_send_ack(SPI_T_BLE_GAP_RSSI_GET, 0, param, 0);
    }
}

int ty_lp_proto_notify_callback_register(void)
{
    ble_conn_callback_register(mm_ble_conn_state_handler);
    return 0;
}

int ty_lp_proto_ble_gap_connect_evt_cs(uint16_t conn_handle, uint8_t role,
                                        const uint8_t *peer_addr, uint8_t peer_addr_type)
{
    cJSON *param = cJSON_CreateObject();
    if (NULL == param) {
        PR_ERR("alloc failed");
        return MM_ERR_MALLOC_FAIL;
    }
    char addr_hex[13];
    snprintf(addr_hex, sizeof(addr_hex), "%02X%02X%02X%02X%02X%02X",
             peer_addr[0], peer_addr[1], peer_addr[2],
             peer_addr[3], peer_addr[4], peer_addr[5]);
    cJSON_AddItemToObject(param, "conn_handle",    cJSON_CreateNumber(conn_handle));
    cJSON_AddItemToObject(param, "role",           cJSON_CreateNumber(role));
    cJSON_AddItemToObject(param, "peer_addr",      cJSON_CreateString(addr_hex));
    cJSON_AddItemToObject(param, "peer_addr_type", cJSON_CreateNumber(peer_addr_type));
    PR_INFO("ble connect evt conn=%u role=%u addr=%s type=%u",
            conn_handle, role, addr_hex, peer_addr_type);
    return __lp_proto_json_com_pack(SPI_M_BLE_GAP_CONNECT_EVT, param);
}

int ty_lp_proto_ble_gap_disconnect_evt_cs(uint16_t conn_handle, uint16_t reason)
{
    cJSON *param = cJSON_CreateObject();
    if (NULL == param) {
        PR_ERR("alloc failed");
        return MM_ERR_MALLOC_FAIL;
    }
    cJSON_AddItemToObject(param, "conn_handle", cJSON_CreateNumber(conn_handle));
    cJSON_AddItemToObject(param, "reason",      cJSON_CreateNumber(reason));
    PR_INFO("ble disconnect evt conn=%u reason=0x%04x", conn_handle, reason);
    return __lp_proto_json_com_pack(SPI_M_BLE_GAP_DISCONNECT_EVT, param);
}

int ty_lp_proto_set_mac_addr_cs(char *addr)
{
    PR_DEBUG("set mac addr [%s]", addr);
    cJSON *param = cJSON_CreateObject();
    if (NULL == param)
    {
        PR_ERR("un like error");
        return MM_ERR_MALLOC_FAIL;
    }
    cJSON_AddItemToObject(param, "mac_addr", cJSON_CreateString(addr));
    return __lp_proto_json_com_pack(SPI_T_SET_MAC_ADDR, param);
}

#if defined(MM_ENABLE_MQTT_KEEPALIVE)
int ty_lp_proto_mqtt_send_data_cs(char *data, int data_len, uint16_t proto)
{
    PR_DEBUG("send mqtt data");
    uint32_t send_len = 0;

    while (send_len < data_len)
    {
        cJSON *param = cJSON_CreateObject();
        char slice_data[MQTT_DATA_SLICE_LEN + 1];
        char slice_type = 0;
        if (NULL == param)
        {
            PR_ERR("un like error");
            return MM_ERR_MALLOC_FAIL;
        }
        if (data_len <= MQTT_DATA_SLICE_LEN)
        {
            // no slice.
            slice_type = 0;
        }
        else if (0 == send_len)
        {
            // slice start.
            slice_type = 1;
        }
        else if (data_len - send_len > MQTT_DATA_SLICE_LEN)
        {
            // slice.
            slice_type = 2;
        }
        else if (data_len - send_len <= MQTT_DATA_SLICE_LEN)
        {
            // slice end.
            slice_type = 3;
        }
        memset(slice_data, 0, sizeof(slice_data));
        strncpy(slice_data, data + send_len, MQTT_DATA_SLICE_LEN);
        send_len += strnlen(slice_data, sizeof(slice_data));
        cJSON_AddItemToObject(param, "proto", cJSON_CreateNumber(proto));
        cJSON_AddItemToObject(param, "data", cJSON_CreateString(slice_data));
        cJSON_AddItemToObject(param, "slice", cJSON_CreateNumber(slice_type));
        if (proto != 302)
        {
            __lp_proto_json_com_pack(LP_PROTO_O_MQTT_SEND_DATA, param);
        }
        else
        {
            __lp_proto_json_com_pack(LP_PROTO_O_MQTT_SEND_DATA_ASYNC, param);
        }
    }

    return 0;
}
#endif /* MM_ENABLE_MQTT_KEEPALIVE */

#if defined(TUYA_SPI_RETRY) && (TUYA_SPI_RETRY == 1)
void ty_spi_cmd_send_thread(void *not_used)
{

    /* To avoid gcc warnings */
    (void)not_used;
    int ret = 0;
    while (1)
    {
        __ty_cmd_tx_sem_wait();
        TY_LP_PROTO_TX_NODE_T cur_tx_node = {0};
        if (0 != __wait_list_get_need_send(&cur_tx_node) || cur_tx_node.data == NULL)
        {
            __ty_cmd_tx_sem_post();
            tkl_system_sleep(10);
            continue;
        }
        TY_LP_PROTO_CMD_HEAD_T strHead = {0};
        strHead.mark = LP_PROTO_HEAD_MARK;
        strHead.version = LP_PROTO_VERSION;
        strHead.payLoadType = TY_LP_PROTO_TYPE_JSON;
        ret = __lp_proto_data_pack_and_send(&strHead, cur_tx_node.data, cur_tx_node.data_len);
        if (ret < 0)
        {
            PR_ERR("pack failed ret = %d", ret);
        }
        tkl_system_free(cur_tx_node.data);
        __ty_cmd_tx_sem_post();
    }

    tkl_thread_release(NULL);
}

int __ty_list_mutex_init(void)
{
    int ret = 0;

    memset(&s_rx_list_mutex, 0, sizeof(s_rx_list_mutex));
    memset(&s_tx_list_mutex, 0, sizeof(s_tx_list_mutex));
    ret = tkl_mutex_create_init(&s_rx_list_mutex);
    if (0 != ret)
    {
        PR_ERR("Failed in tkl_mutex_create_init rx ret:%d", ret);
    }
    ret = tkl_mutex_create_init(&s_tx_list_mutex);
    if (0 != ret)
    {
        PR_ERR("Failed in tkl_mutex_create_init tx ret:%d", ret);
    }
    else
    {
        PR_ERR("xxxxxxxxxxxxxxxxxxxxxx ret %d %p", ret, s_tx_list_mutex);
    }
    return ret;
}
static int __ty_tx_list_lock(void)
{
    int ret = tkl_mutex_lock(s_tx_list_mutex);
    if (ret != 0)
    {
        PR_ERR("xxxxxxxxxxxxxxxxxxxxxx ret %d", ret);
    }
    return ret;
}
static int __ty_tx_list_unlock(void)
{
    int ret = tkl_mutex_unlock(s_tx_list_mutex);
    if (ret != 0)
    {
        PR_ERR("xxxxxxxxxxxxxxxxxxxxxx ret %d %p", ret, s_tx_list_mutex);
    }
    return ret;
}

static int __ty_rx_list_lock(void)
{
    int ret = tkl_mutex_lock(s_rx_list_mutex);
    if (ret != 0)
    {
        PR_ERR("xxxxxxxxxxxxxxxxxxxxxx ret %d %p", ret, s_rx_list_mutex);
    }
    return ret;
}

static int __ty_rx_list_unlock(void)
{
    int ret = tkl_mutex_unlock(s_rx_list_mutex);
    if (ret != 0)
    {
        PR_ERR("xxxxxxxxxxxxxxxxxxxxxx ret %d %p", ret, s_rx_list_mutex);
    }
    return ret;
}

int __ty_cmd_tx_sem_init(void)
{
    int ret = 0;

    memset(&s_cmd_tx_sem, 0, sizeof(s_cmd_tx_sem));
    ret = tkl_semaphore_create_init(&s_cmd_tx_sem, 1, 1);
    if (0 != ret)
    {
        PR_ERR("Failed in tkl_semaphore_create_init ret:%d", ret);
    }
    return ret;
}

int __ty_cmd_tx_sem_wait(void)
{
    return tkl_semaphore_wait(s_cmd_tx_sem, 0xFFFFFFFF);
}

void __ty_cmd_tx_sem_post(void)
{
    if (0 != tkl_semaphore_post(s_cmd_tx_sem))
    {
        PR_ERR("xxxxxxxxxxxxxxxx");
    }
    return;
}

static int __wait_list_insert(char *data, int data_len, int cmd, int frame_num)
{
    int i = 0;
    // TODO
    for (i = 0; i < TY_MAX_TX_NODE; i++)
    {
        if (g_spi_tx_list[i].stat == TY_NOT_USED)
        {
            if (g_spi_tx_list[i].data != NULL)
            {
                PR_ERR(" ram not released before malloc ");
            }
            g_spi_tx_list[i].data = tkl_system_malloc(data_len);
            if (NULL == g_spi_tx_list[i].data)
            {
                PR_ERR("malloc failed:%d", data_len);
                break;
            }
            memcpy(g_spi_tx_list[i].data, data, data_len);
            g_spi_tx_list[i].stat = TY_USED;
            g_spi_tx_list[i].data_len = data_len;
            g_spi_tx_list[i].cmd = cmd;
            g_spi_tx_list[i].frame_num = frame_num;
            g_spi_tx_list[i].ticks = tkl_system_get_millisecond();
            g_spi_tx_list[i].org_ticks = g_spi_tx_list[i].ticks;
            return 0;
        }
    }
    PR_ERR("tx list full, cmd: %d cant be sent", cmd);
    return MM_ERR_GENERAL;
}

static int __wait_list_delete(int cmd, int frame_num)
{
    int i = 0;
    int ret = 0;
    // unsigned int min_timestamp = 0xFFFFFFFF;
    int index_to_delete = -1;
    __ty_tx_list_lock();
    for (i = 0; i < TY_MAX_TX_NODE; i++)
    {
        if (g_spi_tx_list[i].stat == TY_SEND &&
            g_spi_tx_list[i].cmd == cmd &&
            g_spi_tx_list[i].frame_num == frame_num)
        {
            index_to_delete = i;
            break;
        }
    }
    if (index_to_delete >= 0 && index_to_delete < TY_MAX_TX_NODE)
    {
        PR_INFO("remove cmd:%d, frame_num:%d, cost time:%u", cmd, frame_num, tkl_system_get_millisecond() - g_spi_tx_list[index_to_delete].org_ticks);
        if (g_spi_tx_list[index_to_delete].data)
        {
            tkl_system_free(g_spi_tx_list[index_to_delete].data);
            g_spi_tx_list[index_to_delete].data = NULL;
        }
        g_spi_tx_list[index_to_delete].stat = TY_NOT_USED;
        g_spi_tx_list[index_to_delete].data_len = 0;
        g_spi_tx_list[index_to_delete].frame_num = 0;
        g_spi_tx_list[index_to_delete].ticks = 0;
        g_spi_tx_list[index_to_delete].org_ticks = 0;
    }
    else
    {
        PR_ERR("cmd:%d not found", cmd);
        ret = MM_ERR_NOT_FOUND;
    }
    __ty_tx_list_unlock();
    return ret;
}

static int __wait_list_get_need_send(TY_LP_PROTO_TX_NODE_T *node)
{
    int i = 0;
    int ret = 0;
    unsigned int current_time = tkl_system_get_millisecond();
    __ty_tx_list_lock();
    // unsigned int min_timestamp = 0xFFFFFFFF;
    int index_to_send = -1;
    for (i = 0; i < TY_MAX_TX_NODE; i++)
    {
        if (g_spi_tx_list[i].stat == TY_USED)
        {
            g_spi_tx_list[i].stat = TY_SEND;
            index_to_send = i;
            break;
        }
        else if (g_spi_tx_list[i].stat == TY_SEND)
        {
            if (current_time >= g_spi_tx_list[i].ticks + TY_SPI_RETRY_TIME_OUT)
            {
                g_spi_tx_list[i].ticks = current_time;
                index_to_send = i;
                PR_ERR("----retry----cmd:%d", g_spi_tx_list[i].cmd);
                break;
            }
        }
    }
    if (index_to_send >= 0 && index_to_send < TY_MAX_TX_NODE)
    {
        node->data = tkl_system_malloc(g_spi_tx_list[index_to_send].data_len);
        if (NULL == node->data)
        {
            PR_ERR("malloc failed:%d", g_spi_tx_list[index_to_send].data_len);
            __ty_tx_list_unlock();
            return MM_ERR_MALLOC_FAIL;
        }
        memset(node->data, 0, g_spi_tx_list[index_to_send].data_len);
        memcpy(node->data, g_spi_tx_list[index_to_send].data, g_spi_tx_list[index_to_send].data_len);
        node->data_len = g_spi_tx_list[index_to_send].data_len;
        node->cmd = g_spi_tx_list[index_to_send].cmd;
        node->ticks = g_spi_tx_list[index_to_send].ticks;
        node->org_ticks = g_spi_tx_list[index_to_send].org_ticks;
        node->frame_num = g_spi_tx_list[index_to_send].frame_num;
        node->stat = TY_SEND;
    }
    else
    {
        ret = MM_ERR_NOT_FOUND;
    }
    __ty_tx_list_unlock();
    return ret;
}

void tuya_empty_cmd_tx_list(void)
{
    int i = 0;
    __ty_tx_list_lock();
    s_frame_num = 0;
    for (i = 0; i < TY_MAX_TX_NODE; i++)
    {
        if (g_spi_tx_list[i].stat != TY_NOT_USED)
        {
            if (g_spi_tx_list[i].data)
            {
                tkl_system_free(g_spi_tx_list[i].data);
                g_spi_tx_list[i].data = NULL;
            }
            g_spi_tx_list[i].stat = TY_NOT_USED;
            g_spi_tx_list[i].data_len = 0;
            g_spi_tx_list[i].ticks = 0;
            g_spi_tx_list[i].org_ticks = 0;
            g_spi_tx_list[i].frame_num = 0;
        }
    }
    __ty_tx_list_unlock();
}

static int __rx_list_insert(char *data, int data_len, int cmd, int frame_num, unsigned int crc)
{
    int i = 0;
    __ty_rx_list_lock();
    for (i = 0; i < TY_MAX_RX_NODE; i++)
    {
        if (g_spi_rx_list[i].stat == TY_NOT_USED)
        {
            if (g_spi_rx_list[i].data != NULL)
            {
                PR_ERR(" ram not released before malloc ");
            }
            g_spi_rx_list[i].data = tkl_system_malloc(data_len);
            if (NULL == g_spi_rx_list[i].data)
            {
                PR_ERR("malloc failed:%d", data_len);
                break;
            }
            memcpy(g_spi_rx_list[i].data, data, data_len);
            g_spi_rx_list[i].stat = TY_USED;
            g_spi_rx_list[i].data_len = data_len;
            g_spi_rx_list[i].frame_num = frame_num;
            g_spi_rx_list[i].crc = crc;
            g_spi_rx_list[i].cmd = cmd;
            __ty_rx_list_unlock();
            return 0;
        }
    }
    __ty_rx_list_unlock();
    PR_ERR("rx list full, cmd: %d cant be sent", cmd);
    return MM_ERR_GENERAL;
}

static int __rx_list_get_node(TY_LP_PROTO_RX_NODE_T *node, int frame_num)
{
    int i = 0;
    __ty_rx_list_lock();
    for (i = 0; i < TY_MAX_RX_NODE; i++)
    {
        if (g_spi_rx_list[i].stat == TY_USED && g_spi_rx_list[i].frame_num == frame_num)
        {
            if (g_spi_rx_list[i].data_len <= 0)
            {
                PR_ERR("invaild data len:%d", g_spi_rx_list[i].data_len);
                __ty_rx_list_unlock();
                return MM_ERR_INVALID_PARAM;
            }
            node->data = tkl_system_malloc(g_spi_rx_list[i].data_len);
            if (NULL == node->data)
            {
                PR_ERR("malloc failed:%d", g_spi_rx_list[i].data_len);
                __ty_rx_list_unlock();
                return MM_ERR_MALLOC_FAIL;
            }
            memset(node->data, 0, g_spi_rx_list[i].data_len);
            memcpy(node->data, g_spi_rx_list[i].data, g_spi_rx_list[i].data_len);
            node->frame_num = g_spi_rx_list[i].frame_num;
            node->data_len = g_spi_rx_list[i].data_len;
            node->stat = g_spi_rx_list[i].stat;
            node->crc = g_spi_rx_list[i].crc;
            node->cmd = g_spi_rx_list[i].cmd;
            if (g_spi_rx_list[i].data != NULL)
            {
                tkl_system_free(g_spi_rx_list[i].data);
                g_spi_rx_list[i].data = NULL;
            }
            g_spi_rx_list[i].frame_num = 0;
            g_spi_rx_list[i].stat = TY_NOT_USED;
            g_spi_rx_list[i].data_len = 0;
            g_spi_rx_list[i].crc = 0;
            g_spi_rx_list[i].cmd = 0;
            __ty_rx_list_unlock();
            return 0;
        }
        else if (g_spi_rx_list[i].stat == TY_USED && g_spi_rx_list[i].frame_num < frame_num)
        {
            if (g_spi_rx_list[i].data != NULL)
            {
                tkl_system_free(g_spi_rx_list[i].data);
                g_spi_rx_list[i].data = NULL;
            }
            g_spi_rx_list[i].frame_num = 0;
            g_spi_rx_list[i].stat = TY_NOT_USED;
            g_spi_rx_list[i].data_len = 0;
            g_spi_rx_list[i].crc = 0;
        }
    }
    __ty_rx_list_unlock();
    return MM_ERR_NOT_FOUND;
}
void rx_status_reset(void)
{
    __ty_rx_list_lock();
    g_max_proc_fn = 0;
    g_max_fn_in_buffer = 0;
    g_last_proc_time = 0;
    int i = 0;
    for (i = 0; i < TY_MAX_RX_NODE; i++)
    {
        if (g_spi_rx_list[i].data != NULL)
        {
            tkl_system_free(g_spi_rx_list[i].data);
            g_spi_rx_list[i].data = NULL;
        }
        g_spi_rx_list[i].frame_num = 0;
        g_spi_rx_list[i].stat = TY_NOT_USED;
        g_spi_rx_list[i].data_len = 0;
        g_spi_rx_list[i].crc = 0;
        g_spi_rx_list[i].cmd = 0;
    }
    __ty_rx_list_unlock();
}

#endif

#endif /* #if defined(SYSTEM_LINUX) && (OPERATING_SYSTEM != SYSTEM_LINUX) */
