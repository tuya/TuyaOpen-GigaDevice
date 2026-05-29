#ifndef __TY_KEEPALIVE_NEW_H__
#define __TY_KEEPALIVE_NEW_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

typedef enum
{
    KEEPALIVE_NONE,
    KEEPALIVE_TO_GW,
    KEEPALIVE_TO_CLOUD,
}KEEPALIVE_TYPE_E;

typedef enum
{
    POWER_ON,
    POWER_OFF=1,
    POWER_OFF_RTC=2,
}POWER_STATE_TYPE_E;

typedef enum
{
    WAKEUP_REASON_MIN = 11,
    WAKEUP_PIR = 12,
    WAKEUP_BUTTON_SHORT,
    WAKEUP_BUTTON_LONG,
    WAKEUP_CHARGE_IN,
    WAKEUP_CHARGE_OUT,
    WAKEUP_LOW_POWER,
    WAKEUP_GW,
    WAKEUP_SLEEP_FAIL,   
    WAKEUP_DOORBELL,
    WAKEUP_HALL_SENSOR,
    WAKEUP_POWER_ON = 22,
    WAKEUP_REASON_MAX,
}WAKEUP_REASON_E;


typedef enum
{
    RET_OK,
    WAKEUP_NOT_INITED = -1,
    WAKEUP_INVAILD_REASON = -2,
    WAKEUP_SOFTPOWEROFF_LIMIT = -3,
    WAKEUP_PIR_COOLDOWN = -4,
    KEEPALIVE_NOT_INITED = -5,
    KEEPALIVE_TYPE_ERROR = -6,
    KEEPALIVE_INVALID_PARAMETER = -7,
    KEEPALIVE_ALREADY_SLEPT = -8,
    KEEPALIVE_ALREADY_AWAKE = -9,
    KEEPALIVE_EARLY_WAKEUP = -10,
    KEEPALIVE_NOTIFY_FAILED = -11,
}RESULT_E;

typedef enum
{
    HOST_SLEPT  = 0,
    HOST_WAKING = 1,
    HOST_READY  = 2,
}HOST_STATE_E;

int tuya_device_sleep_establish_keepalive_to_gw(unsigned int server, int port, char *send_data, char *recv_data, uint8_t* pkey, int version);

int tuya_device_sleep_establish_keepalive_to_cloud(char* domain, uint32_t ip, uint32_t port, char* p_dev_id, char* p_local_key, uint64_t timestamp);

int tuya_device_sleep_keepalive_with_mqtt(void);

int tuya_keepalive_init(KEEPALIVE_TYPE_E type);

void tuya_keepalive_uninit(void);

void tuya_keepalive_poweroff(POWER_STATE_TYPE_E type);

void tuya_keepalive_set_pir_wakeup_interval(int interval);

int tuya_wakeup_reason_handler(WAKEUP_REASON_E reason);

void tuya_keepalive_set_host_ready(void);

int tuya_keepalive_is_host_sleep(void);

int tuya_keepalive_get_host_state(void);

POWER_STATE_TYPE_E tuya_keepalive_get_power_state(void);

int tuya_keepalive_poweron(void);

#if defined(__cplusplus)
}
#endif

#endif 


