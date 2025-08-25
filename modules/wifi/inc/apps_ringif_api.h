/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear

* @file apps_ringif_api.h
* @brief Application Ring Interface header
* ======================================================================*/
#ifndef APPS_RINGIF_API_H
#define APPS_RINGIF_API_H
/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <stdbool.h>
#include <com_dtypes.h>

#include "fwconfig_wlan.h"
#include "nt_flags.h"

#ifdef SUPPORT_RING_IF
#include "nt_common.h"
#include "nt_osal.h"
#include "wifi_fw_ring_api.h"

/*------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/
bool apps_ringif_f2a_ring_read(uint8_t num_args, uint32_t *args);
bool apps_ringif_a2f_ring_write(uint16_t num_args, uint32_t *args);
bool apps_ringif_start_stop_traffic_test(uint8_t test_mode, uint32_t durSec_sizeKB);

#endif /* SUPPORT_RING_IF */
#endif /* APPS_RINGIF_API_H */
