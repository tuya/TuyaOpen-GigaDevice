/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <time.h>

#include "lwip/opt.h"
#include "lwip/apps/sntp.h"
#include "lwip/netif.h"
#include "wrapper_os.h"

extern volatile uint32_t    unix_time_base_g;

time_t unix_time_g;
uint8_t unixtime_get_flag = 0;

uint32_t os_system_time_get(void);

void sntp_set_system_time(u32_t sec)
{
    char buf[32];
    struct tm current_time_val;
    time_t current_time = (time_t)sec;

    unix_time_g = sec;
    unix_time_base_g = sec - os_system_time_get();
    printf("unix time base is %d,\r\n", unix_time_base_g);

//    //add 8h
//    unix_time_g += 28800;

#ifdef _MSC_VER
    localtime_s(&current_time_val, &current_time);
#else
    localtime_r(&current_time, &current_time_val);
#endif
  
    strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M:%S", &current_time_val);
    printf("SNTP time: %s\n", buf);
    unixtime_get_flag = 1;
}

void
sntp_example_init(void)
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "time.ustc.edu.cn");
    sntp_init();
}

/* get net time time_add 86400ms, between this time, can use time_add add it */
void time_add(void *p_arg)
{
    while(1){
        unix_time_g++;
        sys_ms_sleep(1000);
    }
}

void time_show(void *p_arg)
{
    char buf[32];
    struct tm current_time_val;
    time_t current_time;

    while(1){
        if(unixtime_get_flag) {

            current_time = (time_t)unix_time_g;
            localtime_r(&current_time, &current_time_val);

            strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M:%S", &current_time_val);
            printf("SNTP time: %s\n", buf);
            sys_ms_sleep(1000);
        } else {
            printf("Wait SNTP ready...\r\n");
            sys_ms_sleep(5000);
        }
    }
}

/* get the current system up time (second) */
uint32_t os_system_time_get(void)
{
    return (uint32_t)(sys_current_time_get() / 1000);
}

