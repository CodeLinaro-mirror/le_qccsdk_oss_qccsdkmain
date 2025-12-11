/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/

#include "qapi_lowpower.h"
#include "nt_socpm_sleep.h"
#include "nt_imps.h"
#include "lowpower_internal.h"
#include "wlan_power.h"
#include "wifi_fw_pwr_cb_infra.h"
#include "wmi.h"
#include "wmi_api.h"
#include "HALhwio.h"
#include <stdlib.h>
#include <string.h>

lpr_wmi_t g_lowpower_wmi;
extern ppm_common_t g_ppm_common_struct;
extern wlan_qapi_cxt_t *gp_wlan_qapi_cxt;

extern bool (*wakeup_cb_dtim)(uint16_t type, bool bm_cast,void* pbuf,uint16_t len);
extern bool (*wakeup_cb_net)(uint16_t type, bool bm_cast,void* pbuf,uint16_t len);

/**
   @brief Enable/Disable system power management.

   The API enable/disable system power management.

   @param[in] enable  Enable system power management. 1: To enable; 0: to disable;

   @return
   - QAPI_OK                             --  Enable/Disable system power management successfully.
*/
qapi_Status_t qapi_pm_enable(uint8_t enable)
{
    if (enable != 0 && enable != 1) {
        return QAPI_ERR_INVALID_PARAM;
    }
    nt_socpm_enable(enable);
    return QAPI_OK;
}

/**
   @brief Put system into deepsleep directly.

   The API put system into deepsleep directly.

   @param[in] wkup_src    Wakeup source: 1 for AON timer; 2 for external wakup source;
   @param[in] sleep_time  Sleep time in us, only valid when wkup_src is set 1;

   @return
   - QAPI_OK                             --  system entering into deepsleep successfully.
*/
qapi_Status_t qapi_deepsleep_enter(uint8_t wkup_src, uint64_t sleep_time)
{    
    if (wkup_src == 1) {
        if (sleep_time == 0)
            return QAPI_ERR_INVALID_PARAM;
        nt_enable_standby(sleep_time);
    } else if (wkup_src == 2) {
        nt_enable_indef_deepsleep(0);
    } else {
        return QAPI_ERR_INVALID_PARAM;
    }
    return QAPI_OK;
}

/**
   @brief Config and enable IMPS.

   The API config and enable IMPS (using deepsleep with AON timer as wkup source).

   @param[in] enable        1: Enable; 0: disable. Below parameters are valid only when enable is 1;
   @param[in] sleep_time  Sleep time in ms, during sleep state;
   @param[in] recnx_wait  Re-connection timeout in ms. When wlan disconnect/connect_fail happens, this timer will start; if connect success happens then cancel the timer; if timeout, system will determine whether to enter into sleep;
   @param[in] wmi_wait    Wmi_wait time in ms. Upon recnx_wait timeout, check if there's any WMI cmd received during the wmi_wait duration, if no then goto sleep, if yes then start a timer with wmi_wait duration;
   @param[in] cnx_wait     Time in ms. Use for ENABLE_IMPS_TIMER_ON_BOOTUP feature, means starting this timer during bootup, if there's no wlan connection during this period, then system enters into sleep;
   
   @return
   - QAPI_OK                             --  IMPS cfg and enable/disable successfully.
*/
qapi_Status_t qapi_imps_cfg(uint8_t enable, uint32_t sleep_time, uint32_t recnx_wait, uint32_t wmi_wait, uint32_t cnx_wait, qapi_sleep_mode sleep_mode)
{
    WMI_IMPS_CFG *pdata = (WMI_IMPS_CFG *)&g_lowpower_wmi;
    if (enable != 0 && enable != 1) {
        return QAPI_ERR_INVALID_PARAM;
    }

    if (sleep_mode != qapi_mcu_sleep && sleep_mode != qapi_standby) {
        return QAPI_ERR_INVALID_PARAM;
    }

    memset(pdata, 0, sizeof(*pdata));
    pdata->enable = enable;
    pdata->slp_time = sleep_time;
    pdata->recnx_wait = recnx_wait;
    pdata->cmd_proc_wait = wmi_wait;
    pdata->cnx_wait = cnx_wait;
    pdata->policy = sleep_mode;
    wmi_cmd_send(WMI_IMPS_CFG_CMDID, pdata, sizeof(*pdata));
    return QAPI_OK;
}

/**
   @brief Config and enable IMPS.

   The API config and enable IMPS (using deepsleep with AON timer as wkup source).

   @param[in] enable        1: Enable; 0: disable. Below parameters are valid only when enable is 1;
   @param[in] wait_time  Sleep time in ms, during sleep state;
   @param[in] sleep_time  Sleep time in ms, during sleep state;

   @return
   - QAPI_OK                             --  IMPS cfg and enable/disable successfully.
*/
qapi_Status_t qapi_imps_enter_sleep(uint8_t enable,uint32_t wait_time,uint32_t sleep_time)
{
    
    g_ppm_common_struct.imps_struct_ctx.imps_sleep_time = sleep_time;
    g_ppm_common_struct.imps_struct_ctx.imps_enabled = enable;
    g_ppm_common_struct.imps_struct_ctx.recnx_wait_time_ms = wait_time;
    g_ppm_common_struct.imps_struct_ctx.policy = qapi_mcu_sleep;
    start_imps_cnx_wait_timer(wait_time);
    return QAPI_OK;
}

qapi_Status_t qapi_imps_disable_sleep(void)
{
    if (g_ppm_common_struct.imps_struct_ctx.imps_cnx_timer_started) {
        nt_stop_timer(g_ppm_common_struct.imps_struct_ctx.imps_cnx_wait_timer);
    }
    g_ppm_common_struct.imps_struct_ctx.imps_enabled = FALSE;
    return QAPI_OK;
}

/**
   @brief Config and enable/disable BMPS.

   The API config and enable/disable BMPS.

   @param[in] enable          1: Enable; 0: disable;
   @param[in] idle_timeout  Idle timeout value in ms. When BMPS is enabled, system would start a timer with idle_timeout as timeout value, after timer expires, it'll check tx/rx cnt during this period, if meet condition then trigger system entering into BMPS, otherwise re-start the idle timer again;

   @return
   - QAPI_OK                             --  BMPS cfg and enable/disable successfully.
*/
qapi_Status_t qapi_bmps_cfg(uint8_t enable, uint32_t idle_timeout)
{
    if (enable != 0 && enable != 1 && enable != 2) {
        return QAPI_ERR_INVALID_PARAM;
    }
    if (idle_timeout) {
        WMI_BMPS_IDLE_TIME *pdata = (WMI_BMPS_IDLE_TIME *)&g_lowpower_wmi.bmps_cfg.bmps_idle_time;
        memset(pdata, 0, sizeof(*pdata));
        pdata->time = idle_timeout;
        wmi_cmd_send(WMI_STA_IDLE_TIMER_CMDID, pdata, sizeof(*pdata));
    }

    if (enable != 2) {
        WMI_BMPS_ENABLE *pbmps = (WMI_BMPS_ENABLE *)&g_lowpower_wmi.bmps_cfg.bmps_enable;
        memset(pbmps, 0, sizeof(*pbmps));
        pbmps->enable = enable;
        wmi_cmd_send(WMI_BMPS_ENABLE_CMDID, pbmps, sizeof(*pbmps));
    }
    return QAPI_OK;
}

qapi_Status_t qapi_bmps_log_enable(uint8_t enable)
{
    if (enable != 0 && enable != 1) {
        return QAPI_ERR_INVALID_PARAM;
    }

    WMI_BMPS_LOG_ENABLE *pdata = (WMI_BMPS_LOG_ENABLE *)&g_lowpower_wmi.bmps_cfg.bmps_log_enable;
    memset(pdata, 0, sizeof(*pdata));
    pdata->enable = enable;
    wmi_cmd_send(WMI_BMPS_LOG_ENABLE_CMDID, pdata, sizeof(*pdata));
}

qapi_Status_t qapi_bmps_power_optimization_enable(uint8_t enable)
{
    if (enable != 0 && enable != 1) {
        return QAPI_ERR_INVALID_PARAM;
    }

    WMI_BMPS_PWR_OPT_ENABLE *pdata = (WMI_BMPS_PWR_OPT_ENABLE *)&g_lowpower_wmi.bmps_cfg.bmps_pwr_opt_enable;
    memset(pdata, 0, sizeof(*pdata));
    pdata->enable = enable;
    wmi_cmd_send(WMI_BMPS_PWR_OPT_ENABLE_CMDID, pdata, sizeof(*pdata));
}

qapi_Status_t qapi_bmps_compress_qos_null_enable(uint8_t enable)
{
    if (enable != 0 && enable != 1) {
        return QAPI_ERR_INVALID_PARAM;
    }

    WMI_BMPS_CMPR_QOS_NULL_ENABLE *pdata = (WMI_BMPS_CMPR_QOS_NULL_ENABLE *)&g_lowpower_wmi.bmps_cfg.bmps_cmpr_qos_null_enable;
    memset(pdata, 0, sizeof(*pdata));
    pdata->enable = enable;
    wmi_cmd_send(WMI_BMPS_CMPR_QOS_NULL_ENABLE_CMDID, pdata, sizeof(*pdata));
}

qapi_Status_t qapi_bmps_rx_filter_enable(uint8_t enable)
{
    if (enable != 0 && enable != 1) {
        return QAPI_ERR_INVALID_PARAM;
    }
    WMI_BMPS_ENABLE *pbmps = (WMI_BMPS_ENABLE *)&g_lowpower_wmi.bmps_cfg.bmps_enable;
    memset(pbmps, 0, sizeof(*pbmps));
    pbmps->enable = enable;
    wmi_cmd_send(WMI_BMPS_RX_FILTER_ENABLE_CMDID, pbmps, sizeof(*pbmps));
    return QAPI_OK;
}

qapi_Status_t qapi_bmps_bcmc_rx_filter_cb_register(qapi_bmps_rx_filter_cb bmps_cb, qapi_bmps_rx_filter_cb net_cb)
{
    if(!bmps_cb)
    {
        return QAPI_ERR_INVALID_PARAM;
    }

    if(bmps_cb)
    {
        wakeup_cb_dtim = bmps_cb;
    }

    wakeup_cb_net = net_cb;
    

   return QAPI_OK;
}

qapi_Status_t qapi_bmps_sleep_wakeup_cb(ps_evt_cb_t cb, uint8_t flag)
{
    if(flag)
    {
        fpci_evt_cb_reg(cb, PWR_EVT_WMAC_PRE_SLEEP | PWR_EVT_WMAC_POST_AWAKE | PWR_EVT_WMAC_SLEEP_ABORT, 1, NULL);
    }
    else 
    {
        fpci_evt_cb_dereg(cb, PWR_EVT_WMAC_PRE_SLEEP | PWR_EVT_WMAC_POST_AWAKE | PWR_EVT_WMAC_SLEEP_ABORT);
    }
    return QAPI_OK;
}

qapi_Status_t qapi_bmps_get_exit_reason(uint8_t *reason)
{
    PM_STRUCT *pPmStruct;
    pPmStruct = (PM_STRUCT *) gdevp->pPmStruct;

    if(reason != NULL)
    {
        *reason = PM_GET_SLEEP_EXIT_REASON(pPmStruct);
        return QAPI_OK;
    }
    return QAPI_ERR_INVALID_PARAM;
}

qapi_Status_t qapi_bmps_set_period_to_record_for_stats(uint32_t period_to_record)
{
    WMI_BMPS_GET_STATS *pbmps = (WMI_BMPS_GET_STATS *)&g_lowpower_wmi.bmps_cfg.bmps_stats_record;
    memset(pbmps, 0, sizeof(*pbmps));
    pbmps->period_to_record = period_to_record;
    wmi_cmd_send(WMI_BMPS_SET_PERIOD_TO_RECORD_FOR_STATS_CMDID, pbmps, sizeof(uint32_t));
    return QAPI_OK;
}

qapi_Status_t qapi_bmps_get_bwindow_stats(pm_stats_active_sleep_time_record_buffer_t *bwindow_wait_close_time)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    memset(p_cxt->cmd_bmps_stats.bwindow_wait_close_time, 0, sizeof(pm_stats_active_sleep_time_record_buffer_t));

    memscpy(p_cxt->cmd_bmps_stats.bwindow_wait_close_time, \
                  sizeof(pm_stats_active_sleep_time_record_buffer_t), \
                  bwindow_wait_close_time, \
                  sizeof(pm_stats_active_sleep_time_record_buffer_t));

    WLAN_QAPI_LOCK();
    ret = wmi_get_bmps_bwindow_wait_close_time_stats();

    memscpy(bwindow_wait_close_time, \
                 sizeof(pm_stats_active_sleep_time_record_buffer_t),
                 gp_wlan_qapi_cxt->cmd_bmps_stats.bwindow_wait_close_time, \
                 sizeof(pm_stats_active_sleep_time_record_buffer_t));
    WLAN_QAPI_UNLOCK();
    
    return QAPI_OK;
}

qapi_Status_t qapi_bmps_get_soc_stats(pm_stats_active_sleep_time_record_buffer_t *soc_active_sleep_time)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;


    memset(p_cxt->cmd_bmps_stats.soc_active_sleep_time, 0, sizeof(pm_stats_active_sleep_time_record_buffer_t));

    memscpy(p_cxt->cmd_bmps_stats.soc_active_sleep_time, \
                  sizeof(pm_stats_active_sleep_time_record_buffer_t), \
                  soc_active_sleep_time, \
                  sizeof(pm_stats_active_sleep_time_record_buffer_t));

    WLAN_QAPI_LOCK();
    ret = wmi_get_bmps_soc_active_sleep_time_stats();

    memscpy(soc_active_sleep_time, \
                 sizeof(pm_stats_active_sleep_time_record_buffer_t),
                 gp_wlan_qapi_cxt->cmd_bmps_stats.soc_active_sleep_time, \
                 sizeof(pm_stats_active_sleep_time_record_buffer_t));
    WLAN_QAPI_UNLOCK();
    
    return QAPI_OK;
}

qapi_Status_t qapi_bmps_get_noise_status(WMI_GET_NOISE_STATUS *noise_status)
{

    if(noise_status == NULL)
    {
        return QAPI_ERROR;
    }

    noise_status->noise_floor = HWIO_INXF(SEQ_WCSS_AGC_OFFSET, AGC1RX_AGC_NF_CAL_STATUS, ANI_AVG_NF_M_VALUE);
    noise_status->pd_threshold = HWIO_INXF(SEQ_WCSS_AGC_OFFSET, AGC1RX_AGC_TH_CD20, TH);
    return QAPI_OK;
}

qapi_Status_t qapi_bmps_get_tx_rx_counts(pm_stats_tx_rx_counts_record_buffer_t *tx_rx_counts)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;


    memset(p_cxt->cmd_bmps_stats.tx_rx_counts, 0, sizeof(pm_stats_tx_rx_counts_record_buffer_t));

    memscpy(p_cxt->cmd_bmps_stats.tx_rx_counts, \
                  sizeof(pm_stats_tx_rx_counts_record_buffer_t), \
                  tx_rx_counts, \
                  sizeof(pm_stats_tx_rx_counts_record_buffer_t));

    WLAN_QAPI_LOCK();
    ret = wmi_get_bmps_tx_rx_counts();

    memscpy(tx_rx_counts, \
                 sizeof(pm_stats_tx_rx_counts_record_buffer_t),
                 gp_wlan_qapi_cxt->cmd_bmps_stats.tx_rx_counts, \
                 sizeof(pm_stats_tx_rx_counts_record_buffer_t));
    WLAN_QAPI_UNLOCK();

    return QAPI_OK;
}