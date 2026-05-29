/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#ifndef __AZURE_ENTRY_H__
#define __AZURE_ENTRY_H__

#include "app_cfg.h"
#ifdef CONFIG_AZURE_IOT_SUPPORT
#include <stdlib.h>
#include "co_int.h"
#include "co_math.h"

#define AZURE_PACKAGE_VER               "azure_package_ver"

void cmd_azure_iot(int argc, char **argv);
void cmd_azure_cli(int argc, char **argv);

int prvInitializeSNTP(void);
void prvStopSNTP(void);

void azure_task_start(void);

#ifdef CONFIG_AZURE_F527_DEMO_SUPPORT
int azure_package_version_set(uint16_t package_ver);
uint16_t azure_package_version_get(void);
#endif /* CONFIG_AZURE_F527_DEMO_SUPPORT */

#endif /* CONFIG_AZURE_IOT_SUPPORT */
#endif /* __AZURE_ENTRY_H__ */
