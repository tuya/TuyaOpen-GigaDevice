/**
 * @file tick_notify.h
 * @brief Device tick notify service (ALCS)
 * @copyright Copyright (c) Alibaba Cloud
 */
#ifndef __TICK_NOTIFY_H__
#define __TICK_NOTIFY_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize tick notify service
 * @return Context pointer on success, NULL on failure
 */
void *iotx_tick_notify_init(void);

/**
 * @brief Deinitialize tick notify service
 * @return 0 on success
 */
int iotx_tick_notify_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __TICK_NOTIFY_H__ */
