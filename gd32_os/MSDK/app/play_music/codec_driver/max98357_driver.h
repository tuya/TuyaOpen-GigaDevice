
#ifndef _MAX98357_DRIVER_H
#define _MAX98357_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "gd32vw55x.h"

void max98357_init(void);

void max98357_start(void);

void max98357_end(void);

#ifdef __cplusplus
}
#endif

#endif // _MAX98357_DRIVER_H