/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * datapath_stats.h
 *
 * 		Created on: September 22, 2020
 *      Author:		mohammad
 */
#ifndef CORE_WIFI_DPM_INC_NT_DATAPATH_STATS_H_
#define CORE_WIFI_DPM_INC_NT_DATAPATH_STATS_H_

#include "wlan_dev.h"
#include "data_path.h"
#include "nt_wlan_task_manager.h"
#include "hal_int_modules.h"

struct nt_dp_cmbd_stats {
#if ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS) || \
     (defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS))
    uint32_t tx_mgmt_frms;
    struct nt_dp_stats dp_stats;
    hal_tpe_sta_stats_t tpe_stats;
    hal_dpu_stats_t dpu_stats;
    hal_rxp_stats_t rxp_stats;
    hal_pmi_stats_t pmi_stats;
#if (defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS)
    hal_mpi_stats_t mpi_stats;
    hal_dxestats_t dxe_stats;
    hal_txp_stats_t txp_stats;
#endif  // NT_FN_AP_HAL_DPH_DEBUG_STATS || NT_FN_STA_HAL_DPH_DEBUG_STATS
#if (defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS)
    hal_rpe_stats_t rpe_stats;
#endif  // (defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS)
#endif  //((defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS)||(defined
        //NT_FN_AP_HAL_DPH_PRODUCTION_STATS)||(defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS))
};

#if ((defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS) || \
     (defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
struct datapath_stats_action {
    char action_name[10];
    uint8_t dpm_hal_stats;
    struct nt_dp_cmbd_stats *statsinfo;
};

#if ((defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS))
NT_BOOL nt_collect_production_dpm_hw_stats_on_cmd(struct nt_dp_cmbd_stats *info, WMI_DPM_STATS_CMD *stats_operation);
#endif  // ((defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS)|| (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS))
#if ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
NT_BOOL nt_collect_debug_dpm_hw_stats_on_cmd(struct nt_dp_cmbd_stats *info, WMI_DPM_STATS_CMD *stats_operation);
#endif  // ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
#endif
#endif
