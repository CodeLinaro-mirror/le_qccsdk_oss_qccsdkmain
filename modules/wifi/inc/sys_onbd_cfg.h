/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*
 *
 * sys_onbd_cfg.h
 *
 *  Created on: Dec 9, 2022
 *      Author: Shreyas
 */

//#include "onbd_cfg_lib.h"
#include "nt_osal.h"

#ifndef CORE_SYSTEM_INC_SYS_ONBD_CFG_H_
#define CORE_SYSTEM_INC_SYS_ONBD_CFG_H_

#define onbd_configMAX_SSID_LEN       (32)
#define onbd_configMAX_PASSPHRASE_LEN (64)

typedef enum onbd_config_mode {
    CONFIG_READ = 1,
    CONFIG_WRITE = 2,
} onbd_config_mode;

uint8_t read_and_write_obd_cfg(onbd_config_mode cfg_mode);

#endif /* CORE_SYSTEM_INC_SYS_ONBD_CFG_H_ */
