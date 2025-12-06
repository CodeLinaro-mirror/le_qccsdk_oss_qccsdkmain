/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
   @file lowpower_internal.h

   @brief Lowpower internal definitions

   @details This file provides lowpower internal definitions. 
*/

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include "wmi.h"

/**
Data structure used by the api layer to pass lowpower configurations to the driver.
*/
typedef union {
    WMI_IMPS_CFG imps_cfg;
    /**< IMPS cfg, used in qapi_imps_cfg. */
    struct {
        WMI_BMPS_IDLE_TIME bmps_idle_time;
        /**< The idle timeout in ms, used in qapi_bmps_cfg. */
        WMI_BMPS_ENABLE bmps_enable;
        /**< To enable/disable BMPS, used in qapi_bmps_cfg. */
        WMI_BMPS_LOG_ENABLE bmps_log_enable;
        /**< To enable/disable BMPS Log, used in qapi_bmps_log_enable. */
        WMI_BMPS_PWR_OPT_ENABLE bmps_pwr_opt_enable;
        /**< To enable/disable BMPS power optimization. */
        WMI_BMPS_CMPR_QOS_NULL_ENABLE bmps_cmpr_qos_null_enable;
        /**< To enable/disable compressing qos-null sending */
        WMI_BMPS_GET_STATS bmps_stats_record;
        /**< Get bwindow or soc active & sleep time */
    } bmps_cfg;
    /**< BMPS cfg, used in qapi_bmps_cfg. */
    WMI_BMPS_IGNORE_BCMC bmps_ignore_bcmc;
    /**< To config ignore group-cast traffic during BMPS. */
    WMI_BMPS_TIMING_CFG bmps_timing;
    /**< Internal timing parameters in BMPS. */
    WMI_SLP_CLK_CAL_CFG slp_clk_cal;
    /**< Enable/disable 32k clock calibration in sleep mode. */
    WMI_SLP_CLK_CAL_ACT slp_clk_cal_act;
    /**< Enable/disable 32k clock calibration in active mode. */
    uint32_t force_dtim;
    /**< Force dtim period */
} lpr_wmi_t;

