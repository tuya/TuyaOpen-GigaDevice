/**
 * @file gd32_peri_busy.h
 * @brief "this peripheral needs its clock" vote. Platform internal, not a TKL interface.
 *
 * TUYA_CPU_SLEEP maps to the chip's stop mode, which takes the whole 1.2V clock tree down - a
 * dma transfer still in flight is cut in half, a pwm output freezes at whatever level it was
 * on, a hardware timer stops counting. None of that is true of shallow sleep, which stops the
 * core alone and leaves every peripheral running.
 *
 * So this does not block sleep, it caps its depth: hold it and idle picks shallow sleep instead
 * of stop mode. Cost is roughly the difference between the two, so hold it only while the
 * hardware really is doing something, and drop it as soon as it is not.
 *
 * Refcounted, so nesting and several drivers at once are safe, and cheap enough to sit on a
 * per-transfer path - one atomic add, plus one wakelock write on the 0<->1 edge.
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __GD32_PERI_BUSY_H__
#define __GD32_PERI_BUSY_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief this peripheral's clock must keep running - hold idle to shallow sleep
 */
void gd32_peri_busy_inc(void);

/**
 * @brief done with the clock - let idle reach stop mode again
 */
void gd32_peri_busy_dec(void);

#ifdef __cplusplus
}
#endif

#endif /* __GD32_PERI_BUSY_H__ */
