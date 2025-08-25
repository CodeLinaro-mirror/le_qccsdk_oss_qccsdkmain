/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @brief HAL API library control interface specification
 */

#ifndef _HAL_API_CTL_H_
#define _HAL_API_CTL_H_

#include <stdint.h>
#include "nt_common.h"
#include "hal_int_cfg.h"

#ifdef HAL_CFG_INC_TEST_THREAD
// TODO: temp test thread for HAL
#define NT_HAL_TEST_THREAD() nt_hal_test_thread()
uint32_t nt_hal_test_thread(void);
#else
#define NT_HAL_TEST_THREAD()
#endif

nt_status_t nt_hal_init(void);
#if (FERMION_CHIP_VERSION == 2)
nt_status_t nt_hal_start(uint32_t phy_rate_f_h_q);
#else
nt_status_t nt_hal_start(void);
#endif
nt_status_t nt_hal_stop(void);
nt_status_t nt_hal_deinit(void);

#endif  // _HAL_API_CTL_H_
