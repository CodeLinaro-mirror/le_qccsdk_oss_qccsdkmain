/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef _NT_TWT_AP_H_
#define _NT_TWT_AP_H_
#include "wifi_cmn.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "nt_osal.h"
#include "nt_timer.h"
#include "wlan_power.h"

#if defined(NT_FN_TWT) && defined(SUPPORT_TWT_AP)

#define NT_TWT_AP_SP_END_MARGIN_US (1000)

/**
 * @brief Timeout handler for sp start, This function is called when sp starts
 * @param thandle : Timer handle
 * @return This function does not reutrn anything
 */
void nt_twt_ap_sp_start_timeout_handler(nt_osal_timer_handle_t thandle);

/**
 * @brief Timeout handler for sp end(interrupt context), This function is called when sp ends
 * @param thandle : Timer handle
 * @return This function does not reutrn anything
 */
void nt_twt_ap_sp_end_timeout_handler(nt_osal_timer_handle_t thandle);

/**
 * @brief Timer handler for sp start in nt_wlan thread
 * @param thandle : Timer handle
 * @return This function does not reutrn anything
 */
void nt_twt_ap_sp_start_timeout(nt_osal_timer_handle_t thandle);

/**
 * @brief Timer handler for sp start in nt_wlan thread
 * @param thandle : Timer handle
 * @return This function does not reutrn anything
 */
void nt_twt_ap_sp_end_timeout(nt_osal_timer_handle_t thandle);

/**
 * @brief Deinitializes twt ap struct on wifi_off
 * @param thandle : pm structure
 * @return This function does not reutrn anything
 */
void nt_twt_ap_deinit(PM_STRUCT *pm);

/**
 * @brief Deletes the twt ap timers
 * @param thandle : pm structure
 * @return This function does not reutrn anything
 */
void nt_twt_delete_ap_timers(PM_STRUCT *pPmStruct);
#endif /* NT_FN_TWT */

#endif /* CORE_WIFI_SME_INC_NT_TWT_POWER_SAVE_H_ */
