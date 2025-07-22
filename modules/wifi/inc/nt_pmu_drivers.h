/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CORE_SYSTEM_INC_NT_PMU_DRIVERS_H_
#define CORE_SYSTEM_INC_NT_PMU_DRIVERS_H_
#include "unpa.h"
#include "nt_hw.h"
#include "hal_int_sys.h"
#include "hal_int_powersave.h"
#ifdef NT_SOPCM_CHANGE
#define WIFI_MAX_STATE     0b111111
#define WIFI_SLEEP_DEFAULT 0b000000

// WIFI MASK FOR DIFFERENT REGISTERS
#define WIFI_AON_CONFIG_MASK
#define WIFI_WUR_CONFIG_MASK 0X1E
#define WIFI_OFF             1
//#define WIFI_SLEEP 6
//#define WUR_SLEEP 4
#define WIFI_WUR_OFF 0
typedef enum error_no {
    resource_creation_failed,
    resoource_not_accesible,
    resource_corrupted,
    user_data_invalid,
    resource_data_empty,
    resource_created,
    client_create_fail,
    client_created,
    pdc_init_success
} error_list;
// enum error_no error_list;
/**
 * <!-- wifi_pdc_init -->
 *
 * @brief :  Create UNPA resources and Clients for WiFi,WUR resources
 */
error_list wifi_pdc_init(void);
#endif
#endif /* CORE_SYSTEM_INC_NT_PMU_DRIVERS_H_ */
