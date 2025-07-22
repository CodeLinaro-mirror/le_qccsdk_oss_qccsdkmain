/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/* *
 *
 * sys_onbd_cfg.c
 *
 *  Created on: Dec 9, 2022
 *      Author: Shreyas
 */

#include "onbd_cfg_lib.h"
#include "sys_onbd_cfg.h"
#include "nt_logger_api.h"
#include "lfs.h"

uint8_t read_and_write_obd_cfg(onbd_config_mode cfg_mode)
{
    static Onbd_cfg_t onboard_cfg;
    if (cfg_mode == CONFIG_READ)
        return nt_app_read_onbd_config(&onboard_cfg);

    if (cfg_mode == CONFIG_WRITE)
        return nt_save_onb_cfg(&onboard_cfg);

    return 0;
}
