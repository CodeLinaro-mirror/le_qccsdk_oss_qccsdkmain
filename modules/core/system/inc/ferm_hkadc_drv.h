/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __FERM_HKADC_DRV__
#define __FERM_HKADC_DRV__

#include "qapi_types.h"
#include "qapi_status.h"
#include "ferm_hkadc_hal.h"

#define CONFIG_HDADC_DRV_TEMP_TIMEOUT_COUNT 60000
#define CONFIG_HDADC_DRV_VBAT_TIMEOUT_COUNT 60000

/** Generic error. */
#define QAPI_HKADC_ERROR         ((qapi_Status_t)(__QAPI_ERROR(QAPI_MOD_HKADC, 1)))
#define QAPI_HKADC_DATA_TIME_OUT ((qapi_Status_t)(__QAPI_ERROR(QAPI_MOD_HKADC, 2)))

int32_t hkadc_single_temp_monitor_get_raw(void);
int32_t hkadc_single_vbat_monitor_get_raw(void);
void hkadc_drv_dump(const char *title);

#endif  //__FERM_HKADC_DRV__
