#ifndef _TY_LOW_POWER_INTV_PROTO_CS_H_
#define _TY_LOW_POWER_INTV_PROTO_CS_H_

#include "cJSON.h"
#if defined(MM_ENABLE_MQTT_KEEPALIVE)
#include "mm_keepalive.h"
#endif

#if defined(__cplusplus)
extern "C" {
#endif
#define LP_PROTO_HEAD_MARK 0x12345678 // 协议头部标识
#define LP_PROTO_VERSION   (0)

#pragma pack(1)
typedef struct {
    unsigned int mark;
    unsigned char version;
    unsigned char payLoadType;
    unsigned int payLoadLen;
} TY_LP_PROTO_CMD_HEAD_T;
#pragma pack()

typedef enum {
    TY_LP_PROTO_TYPE_JSON = 0,
    TY_LP_PROTO_TYPE_BIN,
    TY_LP_PROTO_TYPE_OTHER,
} TY_LP_PROTO_PAYLOAD_TYPE_E;

#pragma pack(1)
typedef struct {
    int cmd;
    int type;
    int errorcode;
    unsigned long long time;
} TY_LP_PROTO_JSON_HEAD_T;
#pragma pack()

/* Protocol ACK error codes */
typedef enum {
    MM_ERR_OK           =  0,   /*!< success                    */
    MM_ERR_GENERAL      = -1,   /*!< general / comm failure     */
    MM_ERR_INVALID_PARAM= -2,   /*!< missing or invalid param   */
    MM_ERR_MALLOC_FAIL  = -3,   /*!< memory allocation failure  */
    MM_ERR_TIMEOUT      = -4,   /*!< operation timeout          */
    MM_ERR_NOT_FOUND    = -5,   /*!< resource not found         */
} MM_ERR_CODE_E;

typedef enum {
    SPI_T_WIFI_SCAN                  = 0,  // S->W
    SPI_M_WIFI_SCAN_START            = 1,  // W->S start scan
    SPI_M_WIFI_SCAN                  = 2,  // W->S response wifi list
    SPI_M_WIFI_SCAN_END              = 3,
    SPI_T_WIFI_CONN                  = 4,
    SPI_T_WIFI_DISCONN               = 5,
    SPI_T_SET_MAC_ADDR               = 31, // param:{“mac_addr”:”010203040506”}
    SPI_T_SET_IP_ADDR                = 32, // param:{“ip_addr”:”192.168.8.1”,"mask":24}
    SPI_T_SET_ACTIVE_STAT            = 33, // T31发送注册激活状态 param:{"stat":0} 0:未激活 1:激活成功
    SPI_T_SET_DEEP_SLEEP             = 34, // T31控制WiFi模组断电自身进入深度休眠
    SPI_T_GET_WIFI_CALIBRATION_VALUE = 35, // T31查询WiFi模组TXpower校准值
    SPI_T_SET_WIFI_MODE              = 36, // T31设置WIFI模式
    SPI_T_GET_WIFI_MODE              = 37, // T31获取WIFI模式
    SPI_M_SET_WIFI_MODE              = 38, // WiFi模组设置WIFI模式(应答)
    SPI_T_SET_WIFI_CHANNEL           = 39, // T31设置WIFI信道
    SPI_T_GET_WIFI_CHANNEL           = 40, // T31获取WIFI信道
    SPI_M_SET_WIFI_CHANNEL           = 41, // WiFi模组设置WIFI信道(应答)
    SPI_T_GET_WIFI_RSSI              = 42, // LINUX获取WIFI信号强度
    SPI_M_SET_WIFI_RSSI              = 43, // WiFi模组设置WIFI信号强度(应答)
    SPI_T_GET_WIFI_CONN_STAT         = 44, // LINUX获取WIFI连接状态
    SPI_M_SET_WIFI_CONN_STAT         = 45, // WiFi模组设置WIFI连接状态(应答)
    SPI_T_WIFI_START_AP              = 46, // LINUX通知WiFi模组切换AP模式
    SPI_T_INTO_SLEEP_V2              = 48, // 单品休眠 param:{"ip_addr":"...","port":443,"domain":"...","dev_id":"...","local_key":"..."}
    SPI_M_GET_MAC_ADDR               = 59, // 从机->主机:{"mac_addr":"010203040506"}；或应答 SPI_T_GET_WIFI_MAC(type=1)
    SPI_M_WIFI_CON_FINISH            = 60, // WiFi模组->LINUX 连接完成上报 param:{"ip_addr":"192.168.1.112"}

    /* -----------------------------------------------------------------------
     * IPv6 扩展命令字区段 (61-63)
     * ----------------------------------------------------------------------- */
    SPI_T_SET_IPV6_ADDR              = 61, // 主机->从机 param:{"ipv6_addr":"fe80::1","prefix":64}
    SPI_T_GET_IPV6_ADDR              = 62, // 主机->从机 req param:{}
    SPI_M_SET_IPV6_ADDR              = 63, // 从机->主机 resp param:{"ipv6_addr":"fe80::1","prefix":64}

    SPI_T_WIFI_STOP_AP               = 64, /* 主机->从机 stop softap; slave calls real tkl_wifi_stop_ap(); ack same cmd type=1 */
    SPI_T_WIFI_INIT                  = 65, /* 主机->从机 wifi init; for tkl_wifi_init(); ack same cmd type=1 */

    // BLE 命令字
    /// BLE Stack 管理
    SPI_T_BLE_STACK_INIT             = 66, // 主机->从机 初始化BLE协议栈 param:{"role":0} 0=peripheral 1=central
    SPI_T_BLE_STACK_DEINIT           = 67, // 主机->从机 去初始化BLE协议栈 param:{"role":0}
    /// GAP: Advertising 广播管理
    SPI_T_BLE_GAP_ADV_START          = 68, // 主机->从机 启动BLE广播 param:{"adv_type":0,"adv_interval_min":160,"adv_interval_max":160}
    SPI_T_BLE_GAP_ADV_STOP           = 69, // 主机->从机 停止BLE广播
    SPI_T_BLE_GAP_ADV_DATA_SET       = 70, // 主机->从机 设置广播数据和扫描响应数据 param:{"adv_data":"hex...","scan_rsp":"hex..."}
    SPI_T_BLE_GAP_ADV_DATA_UPDATE    = 71, // 主机->从机 更新广播数据(广播运行中动态更新) param:{"adv_data":"hex...","scan_rsp":"hex..."}
    /// GAP: Address 地址管理
    SPI_T_BLE_GAP_ADDR_SET           = 72, // 主机->从机 设置BLE设备地址 param:{"addr_type":0,"addr":"AABBCCDDEEFF"}
    SPI_T_BLE_GAP_ADDR_GET           = 73, // 主机->从机 获取BLE设备地址
    /// GAP: Connection 连接管理
    SPI_T_BLE_GAP_DISCONNECT         = 74, // 主机->从机 主动断开BLE连接 param:{"conn_handle":1,"reason":19}
    SPI_T_BLE_GAP_CONN_PARAM_UPDATE  = 75, // 主机->从机 更新连接参数 param:{"conn_handle":1,"min_interval":6,...}
    SPI_M_BLE_GAP_CONNECT_EVT        = 76, // 从机->主机 连接建立事件上报
    SPI_M_BLE_GAP_DISCONNECT_EVT     = 77, // 从机->主机 连接断开事件上报
    /// GAP: Misc 其他GAP命令
    SPI_T_BLE_GAP_NAME_SET           = 78, // 主机->从机 设置BLE广播名称 param:{"name":"TuyaBLE"}
    SPI_T_BLE_GAP_TX_POWER_SET       = 79, // 主机->从机 设置发射功率 param:{"role":0,"tx_power":40}
    SPI_T_BLE_GAP_RSSI_GET           = 80, // 主机->从机 查询连接RSSI param:{"conn_handle":1}
    /// GATT Server 操作
    SPI_T_BLE_GATTS_SERVICE_ADD      = 81, // 主机->从机 添加GATT服务 param:{"svc_uuid":"FD50","char_num":2}
    SPI_T_BLE_GATTS_VALUE_SET        = 82, // 主机->从机 设置GATT特征值 param:{"conn_handle":1,"char_handle":3,"data":"hex"}
    SPI_T_BLE_GATTS_VALUE_GET        = 83, // 主机->从机 读取GATT特征值 param:{"conn_handle":1,"char_handle":3}
    SPI_T_BLE_GATTS_VALUE_NOTIFY     = 84, // 主机->从机 发送GATT通知(Notification) param:{"conn_handle":1,"char_handle":5,"data":"hex"}
    SPI_T_BLE_GATTS_VALUE_INDICATE   = 85, // 主机->从机 发送GATT指示(Indication) param:{"conn_handle":1,"char_handle":5,"data":"hex"}
    SPI_T_BLE_GATTS_MTU_REPLY        = 86, // 主机->从机 MTU交换响应(当前不支持) param:{"conn_handle":1,"mtu":247}
    /// GATT Server: 从机 -> 主机 异步事件上报
    SPI_M_BLE_GATTS_WRITE_EVT        = 87, // 从机->主机 GATT写入事件(手机写数据到从机)
    SPI_M_BLE_GATTS_MTU_EVT          = 88, // 从机->主机 MTU变化事件
    SPI_M_BLE_GATTS_READ_EVT         = 89, // 从机->主机 GATT读取事件(手机读从机数据)

    SPI_M_BLE_GAP_CONN_PARAM_EVT     = 90,
    SPI_M_BLE_GATTS_NOTIFY_COMPLETE  = 91,

    SPI_T_BLE_GAP_SCAN_START         = 92,
    SPI_T_BLE_GAP_SCAN_STOP          = 93,
    SPI_T_BLE_GAP_CONNECT            = 94,
    SPI_M_BLE_GAP_SCAN_RESULT        = 95,

    SPI_T_BLE_GATTC_SVC_DISCOVER     = 96,
    SPI_T_BLE_GATTC_CHR_DISCOVER     = 97,
    SPI_T_BLE_GATTC_DESC_DISCOVER    = 98,
    SPI_T_BLE_GATTC_READ             = 99,
    SPI_T_BLE_GATTC_WRITE            = 100,
    SPI_T_BLE_GATTC_WRITE_NO_RSP     = 101,
    SPI_T_BLE_GATTC_MTU_REQ          = 102,
    SPI_M_BLE_GATTC_PEER_DATA        = 103,

    SPI_T_BLE_GATTS_SERVICE_DEL      = 104,
    SPI_T_BLE_GATTS_CHAR_ADD         = 105,
    SPI_T_BLE_GATTS_CHAR_DEL         = 106,
    SPI_T_BLE_GATTS_INFO_GET         = 107,

    SPI_T_GET_WIFI_MAC               = 108, /* 主机->从机 WiFi: 主设备 tkl_wifi_get_mac -> 从机读 wlan0，应答 cmd=SPI_M_GET_MAC_ADDR type=1 */
    SPI_T_SET_WIFI_COUNTRY           = 109, /* 主机->从机 param:{"country":<int>} COUNTRY_CODE_E; ack same cmd type=1 */
    SPI_T_GET_WIFI_IP                = 110, /* 主机->从机 param:{"wf":<int>,"type":<int>} WF_IF_E, TY_AF_E (与上层/SDK 一致，透传) */
    SPI_M_GET_WIFI_IP                = 111, /* 从机->主机 type=1; v4: ip_addr,mask,gw[,dns]; v6: ipv6_addr,prefix */

} TY_LP_PROTO_JSON_COM_E;

typedef struct {
    char stat;
    char ip[16];
    char mask[16];
    char mac[18];
    char gw[16];
    char dns1[16];
    char dns2[16];
    char dns3[16];
} TY_LP_PROTO_NETINFO;

/***********************************************************
 *  Function: ty_lp_proto_cs_data_parse
 *  Desc:     parse recv data
 *  Input:    data: parse data buff
 *  Input:    len: parse data buff len
 *  Return:   0: success   others: fail
 ***********************************************************/
int ty_lp_proto_cs_data_parse(unsigned char *data, int len, unsigned int crc32);

/***********************************************************
 *  Function: ty_lp_proto_set_wifi_scan_cs
 *  Desc:     S->W,send wifi scan cmd
 *  Return:   0: success   others: fail
 ***********************************************************/
int ty_lp_proto_set_wifi_scan_cs(void);
/***********************************************************
 *  Function: ty_lp_proto_wifi_scan_begin_cs
 *  Desc:     W->S,send wifi scan start
 *  Return:   0: success   others: fail
 ***********************************************************/
int ty_lp_proto_wifi_scan_begin_cs(void);
/***********************************************************
 *  Function: ty_lp_proto_wifi_scan_end_cs
 *  Desc:     W->S,send wifi scan end
 *  Return:   0: success   others: fail
 ***********************************************************/
int ty_lp_proto_wifi_scan_end_cs(void);
/***********************************************************
 *  Function: ty_lp_proto_wifi_scan_result_cs
 *  Desc:     W->S,send wifi scan result(wifiList)
 *  Input:    data: buffer(json)
 *  Input:    len: buffer len
 *  Return:   0: success   others: fail
 ***********************************************************/
int ty_lp_proto_wifi_scan_result_cs(char *data, int len);
/***********************************************************
 *  Function: ty_lp_proto_set_mac_addr_cs
 *  Desc:     set wifi mac addr for factory
 *  Input:    addr
 *  Return:   0: success   others: fail
 ***********************************************************/
int ty_lp_proto_set_mac_addr_cs(char *addr);

#if defined(MM_ENABLE_MQTT_KEEPALIVE)
int ty_lp_proto_mqtt_protocal_register_cs(int dp_cmd, int p2p_cmd, int cloud_stream, int ipc_private);
int ty_lp_proto_mqtt_get_stat_cs(void);
int ty_lp_proto_mqtt_set_stat_cs(int stat);
int ty_lp_proto_mqtt_set_book_topic_cs(char *topic);
int ty_lp_proto_mqtt_send_data_cs(char *data, uint32_t data_len, uint16_t proto); // cur support json
int ty_lp_proto_mqtt_recv_data_cs(char *data, int data_len);                      // cur support json
#endif
int ty_lp_proto_set_net_info_cs(TY_LP_PROTO_NETINFO *netinfo);
int ty_lp_proto_wifi_conn_finish_cs(int error);

int ty_lp_proto_notify_callback_register(void);

/* SPI RETRY FUNCTIONS START*/
int __ty_list_mutex_init(void);
void ty_spi_cmd_send_thread(void *not_used);
void __ty_cmd_tx_sem_post(void);
int __ty_cmd_tx_sem_init(void);
int __ty_cmd_tx_sem_wait(void);
void tuya_empty_cmd_tx_list(void);
int __lp_proto_cs_send_ack(int cmd, unsigned long long frame_num, cJSON *param, int errorcode);
int ty_lp_proto_upload_debug(char *buf);
/* SPI RETRY FUNCTIONS END*/
int ty_lp_proto_sdk_bel_single_value(int signal_value, int ack_num);
void ty_lp_proto_timeout_check(void);
void rx_status_reset(void);
int ty_lp_proto_sdk_resp_version(void);
int ty_lp_proto_ble_gap_connect_evt_cs(uint16_t conn_handle, uint8_t role,
                                        const uint8_t *peer_addr, uint8_t peer_addr_type);
int ty_lp_proto_ble_gap_disconnect_evt_cs(uint16_t conn_handle, uint16_t reason);
int ty_lp_proto_notify_callback_register(void);
int ty_lp_proto_gatts_callback_register(void);
int ty_lp_proto_ble_gatts_write_evt_cs(uint16_t conn_handle, uint16_t char_handle,
                                        const uint8_t *data, uint16_t len);
int ty_lp_proto_ble_gatts_mtu_evt_cs(uint16_t conn_handle, uint16_t mtu);
int ty_lp_proto_ble_gatts_read_evt_cs(uint16_t conn_handle, uint16_t char_handle);

#if defined(__cplusplus)
}
#endif

#endif
