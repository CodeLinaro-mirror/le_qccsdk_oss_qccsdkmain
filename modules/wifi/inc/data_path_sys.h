/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*
 * data_path_sys.h
 *
 *  Created on: 08-Sep-2020
 *      Author: Acer
 */

#ifndef CORE_WIFI_DPM_INC_DATA_PATH_SYS_H_
#define CORE_WIFI_DPM_INC_DATA_PATH_SYS_H_

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef NT_DEBUG
#define NT_DPM_CFG_INC_DBG_PRINT
#endif

#ifdef NT_DPM_CFG_INC_DBG_PRINT
void nt_dpm_sys_dbg_print(const char *s, const char *fn, const uint32_t ln, ...);
#define NT_DPM_DBG_PRINT(str, ...) nt_dpm_sys_dbg_print(str, __func__, __LINE__, ##__VA_ARGS__)
#else
#define NT_DPM_DBG_PRINT(str, ...)
#endif

#endif /* CORE_WIFI_DPM_INC_DATA_PATH_SYS_H_ */
