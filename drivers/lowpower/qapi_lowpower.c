
/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/

#include "qapi_lowpower.h"
#include "nt_socpm_sleep.h"
#include "lowpower_internal.h"


lpr_wmi_t g_lowpower_wmi;
extern void nt_watchdog_timer_freeze(void);


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

        nt_watchdog_timer_freeze();
        nt_enable_standby(sleep_time);
    } else if (wkup_src == 2) {
		nt_watchdog_timer_freeze();
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
   @param[in] sleep_time  Sleep time in ms, during deepsleep state;
   @param[in] recnx_wait  Re-connection timeout in ms. When wlan disconnect/connect_fail happens, this timer will start; if connect success happens then cancel the timer; if timeout, system will determine whether to enter into deepsleep;
   @param[in] wmi_wait    Wmi_wait time in ms. Upon recnx_wait timeout, check if there's any WMI cmd received during the wmi_wait duration, if no then goto deepsleep, if yes then start a timer with wmi_wait duration;
   @param[in] cnx_wait     Time in ms. Use for ENABLE_IMPS_TIMER_ON_BOOTUP feature, means starting this timer during bootup, if there's no wlan connection during this period, then system enters into deepsleep;

   @return
   - QAPI_OK                             --  IMPS cfg and enable/disable successfully.
*/
qapi_Status_t qapi_imps_cfg(uint8_t enable, uint32_t sleep_time, uint32_t recnx_wait, uint32_t wmi_wait, uint32_t cnx_wait)
{
    WMI_IMPS_CFG *pdata = (WMI_IMPS_CFG *)&g_lowpower_wmi;
    if (enable != 0 && enable != 1) {
        return QAPI_ERR_INVALID_PARAM;
    }
    memset(pdata, 0, sizeof(*pdata));
    pdata->enable = enable;
    pdata->slp_time = sleep_time;
    pdata->recnx_wait = recnx_wait;
    pdata->cmd_proc_wait = wmi_wait;
    pdata->cnx_wait = cnx_wait;
    wmi_cmd_send(WMI_IMPS_CFG_CMDID, pdata, sizeof(*pdata));
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

