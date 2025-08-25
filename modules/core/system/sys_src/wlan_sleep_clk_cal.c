/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*----------------------------------------------------------------------------*
 * @file wlan_sleep_clk_cal.c
 * @brief Implementation of Active Mode Sleep Clock Calibration
 *
 *
 *---------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 * Include Files
 * --------------------------------------------------------------------------*/

#include "fwconfig_cmn.h"

#ifdef SLEEP_CLK_CAL_IN_ACTIVE_MODE

#include "nt_socpm_sleep.h"
#include "nt_common.h"
#include "nt_logger_api.h"
#include "phyCalUtils.h"
#include "hal_int_sys.h"
#include "nt_timer.h"
#include "wifi_fw_pwr_cb_infra.h"
#include "wmi.h"
#include "nt_wfm_wmi_interface.h"
#include "nt_devcfg.h"
#include "wifi_fw_pmu_ts_cfg.h"
#include "wlan_power.h"

/*-----------------------------------------------------------------------------
 * Externalized Varible/Function Definitions
 * ---------------------------------------------------------------------------*/
extern SOCPM_STRUCT g_socpm_struct;

/*-----------------------------------------------------------------------------
 * Function Declarations and Definitions
 *----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 * @function  : socpm_enable_slp_clk_cal_int
 * @brief     : Enable pmu_ccpu_slp_cal_done_intr Interuupt //Device Specific 65
 * @param     : None
 * @return    : None
 *-----------------------------------------------------------------------------
 */
void socpm_enable_slp_clk_cal_int(void)
{
#ifdef APPLY_SLEEP_CLK_CORRECTION
    uint32_t temp1;
    temp1 = HAL_REG_RD(NT_SOCPM_NVIC_ISER2);
    temp1 = temp1 | (0x1 << 1);
    HAL_REG_WR(NT_SOCPM_NVIC_ISER2, temp1);
#endif
    return;
}

/*-----------------------------------------------------------------------------
 * @function : pmu_ccpu_slp_cal_done_intr
 * @brief    : Interuppt handler for pmu_ccpu_slp_cal_done_intr
 * @param    : None
 * @return   : None
 *-----------------------------------------------------------------------------
 */
void pmu_ccpu_slp_cal_done_intr(void)
{
    /* For QCP5321, apply sleep time correction explicitly ,
    for QCP7321 correction will be applied to sleep time value in HW */
#ifdef APPLY_SLEEP_CLK_CORRECTION
    wmi_msg_struct_t slp_cal_event_msg;
    extern nt_osal_queue_handle_t msg_wfm_wmi_id;
    BaseType_t timeout = pdFALSE;
    memset(&slp_cal_event_msg, 0x0, sizeof(wmi_msg_struct_t));
    slp_cal_event_msg.trans_wmi_message_id = WMI_SLEEP_CLK_CAL_DONE_CMDID;
    slp_cal_event_msg.msg_struct.vo_data = NULL;
    slp_cal_event_msg.msg_struct.vo_data_len = 0;
    slp_cal_event_msg.msg_struct.result_function = NULL;
    HAL_REG_RD(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG);
    if (NT_QUEUE_FAIL == qurt_pipe_try_send(msg_wfm_wmi_id, (void *)&slp_cal_event_msg, &timeout)) {
        NT_LOG_PRINT(SOCPM, ERR, "Queue send failed");
        return;
    }
    return;
#else  /* APPLY_SLEEP_CLK_CORRECTION */
    return;
#endif /* APPLY_SLEEP_CLK_CORRECTION */
}

/*-----------------------------------------------------------------------------
 * @function : socpm_slp_clk_cal_trigger
 * @brief    : Trigger sleep clock cal and interuppt after cal is completed
 * @param    : None
 * @return   : nt_status_t
 *-----------------------------------------------------------------------------
 */
nt_status_t socpm_slp_clk_cal_trigger(void)
{
    volatile uint32_t rdata;

    socpm_enable_slp_clk_cal_int();

    // enable sleep cal done interrupt and active state sleep clock calibration
    rdata = HAL_REG_RD(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG);
    rdata = rdata | QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_SLP_CAL_ENABLE_ACTIVE_MASK |
            QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_CFG_SLP_CLK_CAL_DONE_INTR_EN_MASK;
    HAL_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, rdata);
    return NT_OK;
}

#ifdef APPLY_SLEEP_CLK_CORRECTION
/*-----------------------------------------------------------------------------
 * @function : socpm_slp_clk_cal_get_hbin
 * @brief    : Gets the right hbin after sleep clk cal done interuppt
                is received
 * @param    : None
 * @return   : nt_status_t
 *-----------------------------------------------------------------------------
 */
nt_status_t socpm_slp_clk_cal_get_hbin(void)
{
    uint32_t i = 0;
    uint8_t bin = 0;
    uint32_t hbin_data = 0;
    socpm_sleep_clk_cal_t *p_slp_clk_cal_params = &(g_socpm_struct.slp_clk_cal_params);

    p_slp_clk_cal_params->pmu_temp_sensor_data = pmu_ts_get_raw_data();

    if ((p_slp_clk_cal_params->pmu_temp_sensor_data < p_slp_clk_cal_params->hbin_range[0]) ||
        (p_slp_clk_cal_params->pmu_temp_sensor_data >= p_slp_clk_cal_params->hbin_range[NUM_HBIN_RANGE - 1])) {
        NT_LOG_PRINT(SOCPM, ERR, "Temperature is not live in any of bins");
        return NT_FAIL;
    }

    // Get the hbin number where cal data is available
    for (i = 0; i < NUM_HBIN_RANGE - 1; i++) {
        if (p_slp_clk_cal_params->pmu_temp_sensor_data >= p_slp_clk_cal_params->hbin_range[i]) {
            bin = i;
        } else {
            break;
        }
    }
    hbin_data = HAL_REG_RD(QWLAN_PMU_CFG_HBIN0_REG + (bin * 4));

    if (hbin_data & QWLAN_PMU_CFG_HBIN0_HBIN_ACTIVE_MASK) {
        p_slp_clk_cal_params->xocnt = (hbin_data & QWLAN_PMU_CFG_HBIN0_CALBIN_DATA_MASK);
        if (p_slp_clk_cal_params->xocnt > p_slp_clk_cal_params->refxocnt) {
            NT_LOG_PRINT(SOCPM, ERR,
                         "Xocnt %ld, Refxocnt %ld, RC running Slower ,Bin %d, TS data %ld, TS_data_valid %d",
                         p_slp_clk_cal_params->xocnt, p_slp_clk_cal_params->refxocnt, bin,
                         p_slp_clk_cal_params->pmu_temp_sensor_data, is_pmu_ts_data_valid());
        } else {
            NT_LOG_PRINT(SOCPM, ERR,
                         "Xocnt %ld, Refxocnt %ld, RC running Faster ,Bin %d, TS data %ld, TS_data_valid %d",
                         p_slp_clk_cal_params->xocnt, p_slp_clk_cal_params->refxocnt, bin,
                         p_slp_clk_cal_params->pmu_temp_sensor_data, is_pmu_ts_data_valid());
        }
    }
    /* When the HBIN corresponding to the temperature is not active, check the nearby bins(previous and next)
       if it has valid data - This condition can occur due to the oscillating temperatures */
    else {
        hbin_data = HAL_REG_RD(QWLAN_PMU_CFG_HBIN0_REG + ((bin - 1) * 4));
        if ((bin != 0) && (hbin_data & QWLAN_PMU_CFG_HBIN0_HBIN_ACTIVE_MASK)) {
            p_slp_clk_cal_params->xocnt = (hbin_data & QWLAN_PMU_CFG_HBIN0_CALBIN_DATA_MASK);
            NT_LOG_PRINT(SOCPM, ERR, "SOCPM Sleep Clk Cal - HBIN %d not active, fetching data from hbin %d", bin,
                         (bin - 1));
        } else if (bin != MAX_NUM_HBIN) {
            hbin_data = HAL_REG_RD(QWLAN_PMU_CFG_HBIN0_REG + ((bin + 1) * 4));
            if (hbin_data & QWLAN_PMU_CFG_HBIN0_HBIN_ACTIVE_MASK) {
                p_slp_clk_cal_params->xocnt = (hbin_data & QWLAN_PMU_CFG_HBIN0_CALBIN_DATA_MASK);
                NT_LOG_PRINT(SOCPM, ERR, "SOCPM Sleep Clk Cal - HBIN %d not active, fetching data from hbin %d", bin,
                             (bin + 1));
            } else {
                NT_LOG_PRINT(SOCPM, ERR, "SOCPM Sleep Clk Cal - HBIN %d not active, xocnt not updated", bin);
            }
        } else {
            NT_LOG_PRINT(SOCPM, ERR, "SOCPM Sleep Clk Cal - HBIN %d not active, xocnt not updated", bin);
        }
    }
    p_slp_clk_cal_params->prev_hbin = bin;
    p_slp_clk_cal_params->sleep_clk_cal_initialized = TRUE;
    return NT_OK;
}
#endif /* APPLY_SLEEP_CLK_CORRECTION */

/*-----------------------------------------------------------------------------
 * @function : socpm_sleep_clk_cal_timer_cb
 * @brief    : Sleep Clock Cal Poll Timer Callback Function to
                trigger the sleep clock calibration periodically
 * @param    : None
 * @return   : None
 *-----------------------------------------------------------------------------
 */
void socpm_sleep_clk_cal_timer_cb(void)
{
    socpm_sleep_clk_cal_t *p_slp_clk_cal_params = &(g_socpm_struct.slp_clk_cal_params);
    if (nt_pm_get_pre_slp_cb_complete_status() == TRUE) {
        return;
    }
    nt_timer_change_time_period(p_slp_clk_cal_params->slp_clk_cal_poll_timer,
                                p_slp_clk_cal_params->slp_clk_cal_poll_period);
    if (nt_start_timer(p_slp_clk_cal_params->slp_clk_cal_poll_timer) != NT_TIMER_SUCCESS) {
        NT_LOG_PRINT(SOCPM, ERR, "SOCPM Sleep Clock Cal Timer Cb - Start poll timer failed");
    }
#ifdef APPLY_SLEEP_CLK_CORRECTION
    // Get current temperature and check if hbin has changed
    p_slp_clk_cal_params->pmu_temp_sensor_data = pmu_ts_get_raw_data();

    if ((p_slp_clk_cal_params->pmu_temp_sensor_data >=
         p_slp_clk_cal_params->hbin_range[p_slp_clk_cal_params->prev_hbin] - TS_HYS_THRESH) &&
        (p_slp_clk_cal_params->pmu_temp_sensor_data <
         p_slp_clk_cal_params->hbin_range[(p_slp_clk_cal_params->prev_hbin) + 1] + TS_HYS_THRESH) &&
        (p_slp_clk_cal_params->sleep_clk_cal_initialized == TRUE)) {
        return;
    }

    NT_LOG_PRINT(SOCPM, INFO, "Trigger slp clk cal. %u %u %u %u %u", p_slp_clk_cal_params->pmu_temp_sensor_data,
                 p_slp_clk_cal_params->hbin_range[p_slp_clk_cal_params->prev_hbin],
                 p_slp_clk_cal_params->hbin_range[(p_slp_clk_cal_params->prev_hbin) + 1],
                 p_slp_clk_cal_params->prev_hbin, p_slp_clk_cal_params->sleep_clk_cal_initialized);
#endif /* APPLY_SLEEP_CLK_CORRECTION */

    socpm_slp_clk_cal_trigger();
    return;
}

/*-----------------------------------------------------------------------------
 * @function : socpm_slp_clk_cal_hw_init
 * @brief    : Initializes the HW registers needed for Sleep Clk Cal
 * @param    : None
 * @return   : nt_status_t
 *-----------------------------------------------------------------------------
 */
nt_status_t socpm_slp_clk_cal_hw_init(void)
{
    uint32_t rdata;
    socpm_sleep_clk_cal_t *p_slp_clk_cal_params = &(g_socpm_struct.slp_clk_cal_params);
    // Enable NVIC interuppt for pmu_ccpu_slp_cal_done_intr
    if (p_slp_clk_cal_params->slp_clk_cal_enabled_mode == ACTIVE_MODE) {
        socpm_enable_slp_clk_cal_int();
    }
    HAL_REG_WR(QWLAN_PMU_CFG_HBIN_LOW_OFFSET_REG, HBIN_TEMP_RANGE_START_OFFSET);
    HAL_REG_WR(QWLAN_PMU_CFG_HBIN_0_TO_5_RANGE_REG, HBIN_0_TO_5_RANGE); /* max range of 31 */
    HAL_REG_WR(QWLAN_PMU_CFG_HBIN_6_TO_11_RANGE_REG, HBIN_6_TO_11_RANGE);
    HAL_REG_WR(QWLAN_PMU_CFG_HBIN_12_TO_15_RANGE_REG, HBIN_12_TO_15_RANGE);

    HAL_REG_WR(QWLAN_PMU_CFG_REF_SLP_CLK_CNT_REG, REF_SLEEP_CLK_CNT);
    HAL_REG_WR(QWLAN_PMU_CFG_RS_VALUE_REG, RS_VALUE);

    // Unset the bit forcing sleep clock calibration state machine to IDLE state and set sleep clk cal enable bit
    rdata = (NT_REG_RD(QWLAN_PMU_CFG_SLP_CAL_CAL_EN_REG) &
             (~QWLAN_PMU_CFG_SLP_CAL_CAL_EN_CFG_SLP_CLK_CAL_SM_GO_IDLE_MASK)) |
            QWLAN_PMU_CFG_SLP_CAL_CAL_EN_CFG_SLP_CLK_CAL_EN_MASK;
    HAL_REG_WR(QWLAN_PMU_CFG_SLP_CAL_CAL_EN_REG, rdata);

    return NT_OK;
}

/*-----------------------------------------------------------------------------
 * @function : socpm_slp_clk_cal_init
 * @brief    : Initializes the Sleep Clk Cal struct parameters
 * @param    : None
 * @return   : nt_status_t
 *-----------------------------------------------------------------------------
 */
nt_status_t socpm_slp_clk_cal_init(void)
{
    socpm_sleep_clk_cal_t *p_slp_clk_cal_params = &(g_socpm_struct.slp_clk_cal_params);

    // Set Sleep Clock Calibration periodicity
    p_slp_clk_cal_params->slp_clk_cal_poll_period =
        *((uint32_t *)(nt_devcfg_get_config(NT_DEVCFG_SLP_CLK_CAL_POLL_TIMER_PERIOD_MS)));
#ifdef SLEEP_CLK_CAL_IN_SLEEP_MODE
    p_slp_clk_cal_params->sleep_mode_cal_enabled =
        *((bool *)(nt_devcfg_get_config(NT_DEVCFG_SLP_MODE_SLP_CLK_CAL_ENABLE)));
#endif /* SLEEP_CLK_CAL_IN_SLEEP_MODE */
    socpm_slp_clk_cal_hw_init();

#ifdef APPLY_SLEEP_CLK_CORRECTION
    uint32_t rdata1, rdata2, rdata3, ref_slp_cnt, tmp1;
    // Configure HBIN ranges, currently using default ranges
    p_slp_clk_cal_params->hbin_range[0] = HAL_REG_RD(QWLAN_PMU_CFG_HBIN_LOW_OFFSET_REG);
    rdata1 = HAL_REG_RD(QWLAN_PMU_CFG_HBIN_0_TO_5_RANGE_REG);
    rdata2 = HAL_REG_RD(QWLAN_PMU_CFG_HBIN_6_TO_11_RANGE_REG);
    rdata3 = HAL_REG_RD(QWLAN_PMU_CFG_HBIN_12_TO_15_RANGE_REG);
    for (uint16_t i = 0; i < (NUM_HBIN_RANGE - 1); i++) {
        if (i < HBIN_RANGE2_START) {
            p_slp_clk_cal_params->hbin_range[i + 1] =
                (p_slp_clk_cal_params->hbin_range[i] +
                 ((((QWLAN_PMU_CFG_HBIN_0_TO_5_RANGE_CFG_HBIN0_RANGE_MASK) << (HBIN_RANGE_BIT_OFFSET * i)) & rdata1) >>
                  (HBIN_RANGE_BIT_OFFSET * i)));
        } else if (i < HBIN_RANGE3_START) {
            p_slp_clk_cal_params->hbin_range[i + 1] =
                (p_slp_clk_cal_params->hbin_range[i] + ((((QWLAN_PMU_CFG_HBIN_6_TO_11_RANGE_CFG_HBIN6_RANGE_MASK)
                                                          << (HBIN_RANGE_BIT_OFFSET * (i - HBIN_RANGE2_START))) &
                                                         rdata2) >>
                                                        (HBIN_RANGE_BIT_OFFSET * (i - HBIN_RANGE2_START))));
        } else {
            p_slp_clk_cal_params->hbin_range[i + 1] =
                (p_slp_clk_cal_params->hbin_range[i] + ((((QWLAN_PMU_CFG_HBIN_12_TO_15_RANGE_CFG_HBIN12_RANGE_MASK)
                                                          << (HBIN_RANGE_BIT_OFFSET * (i - HBIN_RANGE3_START))) &
                                                         rdata3) >>
                                                        (HBIN_RANGE_BIT_OFFSET * (i - HBIN_RANGE3_START))));
        }
    }

    // Get the reference sleep clk count used to count xo clock , should be in order of power of 2
    ref_slp_cnt = HAL_REG_RD(QWLAN_PMU_CFG_REF_SLP_CLK_CNT_REG);
    switch (ref_slp_cnt) {
        case 16:
            tmp1 = 14;
            break;
        case 32:
            tmp1 = 28;
            break;
        case 64:
            tmp1 = 56;
            break;
        case 128:
            tmp1 = 112;
            break;
        case 512:
            tmp1 = 447;
            break;
        case 1024:
            tmp1 = 895;
            break;
        default:
            tmp1 = 28;
            break;
    }

    p_slp_clk_cal_params->refxocnt =
        (tmp1 * 9375) / 8;  //  (tmp1 * xo_clk_freq/rc_clk_freq) = (tmp1 * 38400000) / 32768;
    p_slp_clk_cal_params->xocnt = p_slp_clk_cal_params->refxocnt;
    p_slp_clk_cal_params->prev_hbin = 0xFF;
    ;
#endif /* APPLY_SLEEP_CLK_CORRECTION */

    // Create periodic NT timer for Active Mode Sleep Clock Calibration
    /*During init, configure poll timer periodicity for a smaller value(1 sec) and
      later in the timer callback it is set to the original poll value*/
    if (!(p_slp_clk_cal_params->slp_clk_cal_poll_timer)) {
        p_slp_clk_cal_params->slp_clk_cal_poll_timer =
            nt_create_timer(socpm_sleep_clk_cal_timer_cb, NULL, INIT_SLP_CAL_POLL_PERIOD_MS, FALSE);
    }

    if (!(p_slp_clk_cal_params->slp_clk_cal_poll_timer)) {
        NT_LOG_PRINT(SOCPM, ERR, "SOCPM Sleep Clock Cal Initialization -Create poll timer failed");
        return NT_FAIL;
    }

    if (nt_start_timer(p_slp_clk_cal_params->slp_clk_cal_poll_timer) != NT_TIMER_SUCCESS) {
        NT_LOG_PRINT(SOCPM, ERR, "SOCPM Sleep Clock Cal Initialization - Start poll timer failed");
        return NT_FAIL;
    }
    return NT_OK;
}

/*-----------------------------------------------------------------------------
 * @function : socpm_actv_slp_clk_cal_monitor_pause
 * @brief    : Pause Sleep Clock Calibration Poll Timer
 * @param    : None
 * @return   : None
 *-----------------------------------------------------------------------------
 */

void socpm_actv_slp_clk_cal_monitor_pause(void)
{
    socpm_sleep_clk_cal_t *p_slp_clk_cal_params = &(g_socpm_struct.slp_clk_cal_params);
    if (NULL != p_slp_clk_cal_params->slp_clk_cal_poll_timer) {
        if (nt_osal_is_timer_active(p_slp_clk_cal_params->slp_clk_cal_poll_timer)) {
            p_slp_clk_cal_params->sleep_clk_cal_timer_pending_ticks =
                nt_timer_get_expiry_time(p_slp_clk_cal_params->slp_clk_cal_poll_timer);
            if (nt_stop_timer(p_slp_clk_cal_params->slp_clk_cal_poll_timer) != NT_TIMER_SUCCESS) {
                NT_LOG_PRINT(SOCPM, ERR, "SOCPM Active Mode Sleep Clock Cal Pause- Stop poll timer failed");
            }
        } else {
            p_slp_clk_cal_params->sleep_clk_cal_timer_pending_ticks = 0;
            NT_LOG_PRINT(SOCPM, INFO,
                         "SOCPM Active Mode Sleep Clock Cal Pause -Poll timer not stopped as its not active");
        }
    }
    invalidate_pmu_ts_configuration();
    return;
}

/*-----------------------------------------------------------------------------
 * @function : socpm_actv_slp_clk_cal_monitor_resume
 * @brief    : Resume Sleep Clock Calibration Poll Timer
 * @param    : None
 * @return   : None
 *-----------------------------------------------------------------------------
 */
void socpm_actv_slp_clk_cal_monitor_resume(void)
{
    socpm_sleep_clk_cal_t *p_slp_clk_cal_params = &(g_socpm_struct.slp_clk_cal_params);
    if (NULL != p_slp_clk_cal_params->slp_clk_cal_poll_timer) {
        if (p_slp_clk_cal_params->sleep_clk_cal_timer_pending_ticks != 0) {
            if (nt_timer_change_time_period(p_slp_clk_cal_params->slp_clk_cal_poll_timer,
                                            p_slp_clk_cal_params->sleep_clk_cal_timer_pending_ticks) !=
                NT_TIMER_SUCCESS) {
                NT_LOG_PRINT(SOCPM, ERR, "SOCPM Sleep Clock Cal resume - Start poll timer failed");
            }
            p_slp_clk_cal_params->sleep_clk_cal_timer_pending_ticks = 0;
        } else {
            socpm_sleep_clk_cal_timer_cb();
        }
    }
    socpm_slp_clk_cal_hw_init();  // to do only for MCU sleep( can be skipped if registers are retained)
    if (is_pmu_ts_configured() != true) {
        pmu_ts_configure();
    }
    p_slp_clk_cal_params->slp_clk_cal_enabled_mode = ACTIVE_MODE;
    return;
}

/*-----------------------------------------------------------------------------
 * @function     : socpm_actv_slp_clk_cal_slp_cb
 * @brief        : power callback function during presleep/postawake events
                    for sleep clock calibration
 * @param evt    : Indicates the Sleep Event
 *        p_args : Arguments if any
 * @return       : None
 *-----------------------------------------------------------------------------
 */
void socpm_actv_slp_clk_cal_slp_cb(uint8_t evt, void *p_args)
{
    volatile uint32_t value;
    (void)p_args;
    if (evt == PWR_EVT_WMAC_PRE_SLEEP) {
        // Force sleep clock calibraiton state machine to IDLE state
        value =
            NT_REG_RD(QWLAN_PMU_CFG_SLP_CAL_CAL_EN_REG) | QWLAN_PMU_CFG_SLP_CAL_CAL_EN_CFG_SLP_CLK_CAL_SM_GO_IDLE_MASK;
        HAL_REG_WR(QWLAN_PMU_CFG_SLP_CAL_CAL_EN_REG, value);

        socpm_actv_slp_clk_cal_monitor_pause();
    }

    if ((evt == PWR_EVT_WMAC_POST_AWAKE) || (evt == PWR_EVT_WMAC_SLEEP_ABORT)) {
        socpm_actv_slp_clk_cal_monitor_resume();
    }
    return;
}

/*-----------------------------------------------------------------------------
 * @function   : socpm_slp_clk_cal_enable
 * @brief      : Function to enable/ disable Active Mode Sleep Clk Calibration
 * @param mode : 0- DISABLE, 1 - ACTIVE_MODE, 2 - SLEEP_MODE, 3 - HYBRID_MODE
 * @return     : nt_status_t
 *-----------------------------------------------------------------------------
 */
nt_status_t socpm_slp_clk_cal_enable(slp_clk_cal_mode_t mode)
{
    socpm_sleep_clk_cal_t *p_slp_clk_cal_params = &(g_socpm_struct.slp_clk_cal_params);
    uint32_t value;
    if (mode == ACTIVE_MODE) {
        NT_LOG_PRINT(SOCPM, ERR, "SOCPM slp clk cal enable prev_state %u,curr_state %u",
                     p_slp_clk_cal_params->slp_clk_cal_enabled_mode, mode);
        if (p_slp_clk_cal_params->slp_clk_cal_enabled_mode == ACTIVE_MODE) {
            return NT_OK;
        }
        p_slp_clk_cal_params->slp_clk_cal_enabled_mode = ACTIVE_MODE;
        socpm_slp_clk_cal_init();
        HAL_REG_WR(QWLAN_PMU_CFG_HBIN_RESET_REG, 0x1);
        fpci_evt_cb_reg((ps_evt_cb_t)&socpm_actv_slp_clk_cal_slp_cb,
                        PWR_EVT_WMAC_PRE_SLEEP | PWR_EVT_WMAC_POST_AWAKE | PWR_EVT_WMAC_SLEEP_ABORT,
                        PS_CB_SLEEP_CAL_PRIORITY, NULL);
        (void)value;
    } else if (mode == DISABLE) {
        NT_LOG_PRINT(SOCPM, ERR, "SOCPM slp clk cal enable prev_state %u,curr_state %u",
                     p_slp_clk_cal_params->slp_clk_cal_enabled_mode, mode);
        if (p_slp_clk_cal_params->slp_clk_cal_enabled_mode == DISABLE) {
            return NT_OK;
        }

        // Force sleep clock calibraiton state machine to IDLE state
        value = (NT_REG_RD(QWLAN_PMU_CFG_SLP_CAL_CAL_EN_REG) |
                 QWLAN_PMU_CFG_SLP_CAL_CAL_EN_CFG_SLP_CLK_CAL_SM_GO_IDLE_MASK) &
                (~QWLAN_PMU_CFG_SLP_CAL_CAL_EN_CFG_SLP_CLK_CAL_EN_MASK);
        HAL_REG_WR(QWLAN_PMU_CFG_SLP_CAL_CAL_EN_REG, value);

        HAL_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, 0x0);
        HAL_REG_WR(QWLAN_PMU_CFG_HBIN_RESET_REG, 0x1);
        if ((NULL != p_slp_clk_cal_params->slp_clk_cal_poll_timer) &&
            (nt_osal_is_timer_active(p_slp_clk_cal_params->slp_clk_cal_poll_timer))) {
            /* Stop the timer */
            if (nt_stop_timer(p_slp_clk_cal_params->slp_clk_cal_poll_timer) != NT_TIMER_SUCCESS) {
                NT_LOG_PRINT(SOCPM, ERR, "SOCPM slp clk cal disable - Poll timer stop failed");
            }
        } else {
            NT_LOG_PRINT(SOCPM, ERR, "SOCPM slp clk cal disable - Poll timer not active.Unable to stop");
        }
        if (NULL != p_slp_clk_cal_params->slp_clk_cal_poll_timer) {
            /* Delete the timer */
            if (nt_delete_timer(p_slp_clk_cal_params->slp_clk_cal_poll_timer) != NT_TIMER_SUCCESS) {
                NT_LOG_PRINT(SOCPM, ERR, "SOCPM slp clk cal disable - Poll timer delete failed");
                return NT_FAIL;
            }
        }
        fpci_evt_cb_dereg((ps_evt_cb_t)&socpm_actv_slp_clk_cal_slp_cb,
                          PWR_EVT_WMAC_PRE_SLEEP | PWR_EVT_WMAC_POST_AWAKE | PWR_EVT_WMAC_SLEEP_ABORT);
#ifdef APPLY_SLEEP_CLK_CORRECTION
        p_slp_clk_cal_params->xocnt = p_slp_clk_cal_params->refxocnt;
#endif /* APPLY_SLEEP_CLK_CORRECTION */
        p_slp_clk_cal_params->slp_clk_cal_poll_timer = NULL;
        p_slp_clk_cal_params->slp_clk_cal_enabled_mode = DISABLE;
        p_slp_clk_cal_params->sleep_clk_cal_initialized = FALSE;
    } else if (mode == SLEEP_MODE) {
#ifdef SLEEP_CLK_CAL_IN_SLEEP_MODE
        if (p_slp_clk_cal_params->sleep_mode_cal_enabled != true) {
            return NT_FAIL;
        }
        NT_LOG_PRINT(SOCPM, INFO, "SOCPM slp clk cal enable prev_state %u,curr_state %u",
                     p_slp_clk_cal_params->slp_clk_cal_enabled_mode, mode);
        p_slp_clk_cal_params->slp_clk_cal_enabled_mode = SLEEP_MODE;
        socpm_slp_clk_cal_hw_init();

#ifdef APPLY_SLEEP_CLK_CORRECTION
        // Setting xocnt to refxocnt to avoid reverse slp clk scaling in sleep mode calibration
        p_slp_clk_cal_params->xocnt = p_slp_clk_cal_params->refxocnt;
#endif /* APPLY_SLEEP_CLK_CORRECTION */

#else  /* SLEEP_CLK_CAL_IN_SLEEP_MODE */
        NT_LOG_PRINT(SOCPM, WARN, "SOCPM slp clk cal enable SLEEP MODE not supported");
#endif /* SLEEP_CLK_CAL_IN_SLEEP_MODE */
    } else if (mode >= MAX_SLEEP_CAL_MODE) {
        NT_LOG_PRINT(SOCPM, ERR, "SOCPM slp clk cal enable -INVALID MODE");
    }
    return NT_OK;
}

#ifdef SLEEP_CLK_CAL_IN_SLEEP_MODE

/*-----------------------------------------------------------------------------
 * @function   : socpm_slp_clk_cal_presleep_activites
 * @brief      : Enable sleep mode cal and pause applying sleep clock correction
 * @param      : None
 * @return     : None
 *-----------------------------------------------------------------------------
 */
void socpm_slp_clk_cal_presleep_activites(uint64_t remaining_slp_time)
{
    socpm_sleep_clk_cal_t *p_slp_clk_cal_params = &(g_socpm_struct.slp_clk_cal_params);
    slp_clk_cal_mode_t prev_cal_mode = p_slp_clk_cal_params->slp_clk_cal_enabled_mode;
    if (p_slp_clk_cal_params->sleep_mode_cal_enabled != true) {
        return;
    }
    socpm_slp_clk_cal_enable(SLEEP_MODE);
#ifdef APPLY_SLEEP_CLK_CORRECTION
    if (prev_cal_mode == ACTIVE_MODE) {
        nt_socpm_slp_tmr_set(remaining_slp_time);
    }
#else  /* APPLY_SLEEP_CLK_CORRECTION */
    (void)prev_cal_mode;
    (void)remaining_slp_time;
#endif /* APPLY_SLEEP_CLK_CORRECTION */
    return;
}

/*-----------------------------------------------------------------------------
 * @function   : socpm_slp_clk_cal_postawake_activities
 * @brief      : Function to reenable active mode calibration and resume
                    applying the sleep clock correction
 * @param      : None
 * @return     : None
 *-----------------------------------------------------------------------------
 */
void socpm_slp_clk_cal_postawake_activities(void)
{
    socpm_sleep_clk_cal_t *p_slp_clk_cal_params = &(g_socpm_struct.slp_clk_cal_params);
    if (p_slp_clk_cal_params->sleep_mode_cal_enabled != true) {
        return;
    }
#ifdef APPLY_SLEEP_CLK_CORRECTION
    uint32_t curr_hbin_data;
    /* After switching to ACTIVE MODE, restoring the xocnt value from the hbin, reinitialization/
       rerunning active mode cal not required */
    if (p_slp_clk_cal_params->slp_clk_cal_enabled_mode == SLEEP_MODE) {
        p_slp_clk_cal_params->slp_clk_cal_enabled_mode = ACTIVE_MODE;
        if (p_slp_clk_cal_params->prev_hbin <= MAX_NUM_HBIN) {
            curr_hbin_data = (HAL_REG_RD(QWLAN_PMU_CFG_HBIN0_REG + (p_slp_clk_cal_params->prev_hbin * 4)));
            if (curr_hbin_data & QWLAN_PMU_CFG_HBIN0_HBIN_ACTIVE_MASK) {
                p_slp_clk_cal_params->xocnt = (curr_hbin_data & QWLAN_PMU_CFG_HBIN0_CALBIN_DATA_MASK);
            } else {
                NT_LOG_PRINT(SOCPM, ERR, "SOCPM slp clk cal resume , HBIN not active, retriggering cal");
                socpm_slp_clk_cal_hw_init();
                socpm_slp_clk_cal_trigger();
            }
        } else {
            NT_LOG_PRINT(SOCPM, ERR, "SOCPM slp clk cal resume , bin value not valid, retriggering cal");
            socpm_slp_clk_cal_hw_init();
            socpm_slp_clk_cal_trigger();
        }
    }
#else  /* APPLY_SLEEP_CLK_CORRECTION */
    p_slp_clk_cal_params->slp_clk_cal_enabled_mode = ACTIVE_MODE;
    socpm_slp_clk_cal_hw_init();
#endif /* APPLY_SLEEP_CLK_CORRECTION */
    return;
}

/*-----------------------------------------------------------------------------
 * @function   : socpm_slp_clk_cal_dynamic_slp_mode_cal_enable
 * @brief      : Function to enable/ disable Sleep Mode Sleep Clk Calibration
 * @param en   : 0- DISABLE, 1 - Enable
 * @return     : None
 *-----------------------------------------------------------------------------
 */
void socpm_slp_clk_cal_dynamic_slp_mode_cal_enable(bool en)
{
    socpm_sleep_clk_cal_t *p_slp_clk_cal_params = &(g_socpm_struct.slp_clk_cal_params);
    p_slp_clk_cal_params->sleep_mode_cal_enabled = en;
    return;
}

#endif /*SLEEP_CLK_CAL_IN_SLEEP_MODE*/

#endif /* SLEEP_CLK_CAL_IN_ACTIVE_MODE */
