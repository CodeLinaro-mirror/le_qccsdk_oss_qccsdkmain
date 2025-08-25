/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * test_dpm.h
 *
 *  Created on: 24-Jun-2022
 *      Author: Acer
 */

#ifndef CORE_WIFI_DPM_INC_TEST_DPM_H_
#define CORE_WIFI_DPM_INC_TEST_DPM_H_

#include "fwconfig_wlan.h"
#include "nt_flags.h"

#ifdef NT_RMF_TEST
/*
 * To append BD to Frame used for STA Attack scenario.
 * Parameters:
 * Param1:frame:      frame pointer.
 * Param2:length:     length of the frame.
 * Param3:ret_frame:  return frame pointer
 * Param4:testvar:    testvar used for frame manipulation
 * */
uint32_t nt_append_bd_to_frame_test(void *frame, uint32_t length, void **ret_frame, uint8_t testvar);
#endif

#endif /* CORE_WIFI_DPM_INC_TEST_DPM_H_ */
