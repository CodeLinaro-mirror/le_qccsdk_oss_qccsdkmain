
#ifndef __WLAN_DRV_H__
#define __WLAN_DRV_H__

#include <stdio.h>

#include "qapi_wlan.h"
#include "wmi.h"
#include "nt_wfm_wmi_interface.h"
#include "wlan_dev.h"

#include "qurt_internal.h"
#include "qurt_mutex.h"
#include "nt_osal.h"

#define QAPI_WLAN_DEBUG 1

#if QAPI_WLAN_DEBUG
#define QAPI_WLAN_ERR   1
#define QAPI_WLAN_WARN  1
#define QAPI_WLAN_INFO  1
#define QAPI_WLAN_LOG   1
#else
#define QAPI_WLAN_ERR   0
#define QAPI_WLAN_WARN  0
#define QAPI_WLAN_INFO  0
#define QAPI_WLAN_LOG   0
#endif

#define MODULE_PRINT_PREFIX     "[qapi_wlan] "

#define ERR_PREFIX  "[ERR] "
#define WARN_PREFIX  "[WARN] "
#define INFO_PREFIX  "[INFO] "
#define LOG_PREFIX  "[LOG] "

#if QAPI_WLAN_ERR
#define err_printf(msg,...)     printf(MODULE_PRINT_PREFIX ERR_PREFIX msg, ##__VA_ARGS__)
#else
#define err_printf(msg...)     do { } while (0)
#endif

#if QAPI_WLAN_WARN
#define warn_printf(msg,...)     printf(MODULE_PRINT_PREFIX WARN_PREFIX msg, ##__VA_ARGS__)
#else
#define warn_printf(msg...)     do { } while (0)
#endif

#if QAPI_WLAN_INFO
#define info_printf(msg,...)     printf(MODULE_PRINT_PREFIX INFO_PREFIX msg, ##__VA_ARGS__)
#else
#define info_printf(msg...)     do { } while (0)
#endif

#if QAPI_WLAN_LOG
#define log_printf(msg,...)     printf(MODULE_PRINT_PREFIX LOG_PREFIX msg, ##__VA_ARGS__)
#else
#define log_printf(msg...)     do { } while (0)
#endif

#define PRINT_ERR_NOT_SUPPORTED         err_printf("Not supported yet\n")
#define PRINT_ERR_INVALID_PARAM         err_printf("Invalid paramter\n")
#define PRINT_ERR_INVALID_PARAM1(msg, argx)         err_printf("Invalid paramter: " msg "=0x%x\n", argx)
#define PRINT_ERR_ALREADY_EXIST         err_printf("Already exist\n")
#define PRINT_ERR_WMI_CMD_SEND_FAILED   err_printf("WMI command send failed\n")
#define PRINT_ERR_NO_RESOURCE           err_printf("No resource\n")

#define PRINT_WARN_SKIP                 warn_printf("Skip\n")

#define PRINT_LOG_FUNC_LINE             log_printf("%s %d\n", __FUNCTION__, __LINE__)
#define PRINT_LOG_FUNC_LINE_ENTRY       log_printf("%s %d entry\n", __FUNCTION__, __LINE__)
#define PRINT_LOG_FUNC_LINE_EXIT        log_printf("%s %d exit\n", __FUNCTION__, __LINE__)

#define QAPI_EVENT_PAYLOAD_LENGTH_MAX           1000

#ifndef DEF_AP_COUNTRY_CODE
#define DEF_AP_COUNTRY_CODE		"US "
#endif
typedef struct wlan_qapi_cxt_s {
    qbool_t                 wlanEnabled;
    qapi_WLAN_Callback_t    qapi_event_handler;
    void                   *event_application_Context;
    qurt_mutex_t            wlan_qapi_cxt_mutex;    /* protect wlan_qapi_cxt_t safe in multi-thread */
    qurt_signal_t           wlan_cmd_done; /* will be ready if wlan cmd is done */
    qurt_mutex_t            wlan_qapi_block_mutex;
    uint32_t                wlan_enable_block_mode:1;
    uint32_t                wlan_disable_block_mode:1;
    uint32_t                wlan_if_add_block_mode:1;
    uint32_t                wlan_scan_start_block_mode:1;
    uint32_t                wlan_connect_block_mode:1;
    uint32_t                wlan_disconnect_block_mode:1;
    uint32_t                wlan_get_stat_block_mode:1;
    uint32_t                wlan_set_param_block_mode:1;
    uint32_t                wlan_get_regulatory_block_mode:1;
    uint32_t                wlan_roaming_started:1;
    uint32_t                wlan_set_rate_block_mode:1;
    uint32_t                wlan_get_rate_block_mode:1;
    qapi_Status_t           wlan_qapi_error;
    dev_common_t           *dev_common;
    uint8_t                *event_payload_buf;
    uint32_t                event_payload_buf_length;
    uint32_t                event_payload_actual_length;
    WMI_CONNECT_CMD         connect_cmd;
    WMI_START_SCAN_CMD      scan_cmd;
    WMI_SET_PASSPHRASE_CMD  passphrase_cmd;
    WLAN_WMI_DISCONN_t      discon_cmd;
    qapi_WLAN_Join_Comp_Evt_t connect_result;
    qapi_WLAN_Reg_Evt_t	    reg_result;
	uint8_t					param_id;
    qbool_t                 connect_in_progress;
    qbool_t                 disconnect_in_progress;
    qbool_t                 scan_in_progress;
    qbool_t                 wait_scan_comp_evt;
    qbool_t                 connected;
    uint8_t                *pScanOut;       /* callers buffer to hold results. */
    uint16_t                pScanOutSize;
    uint16_t                scanBssMaxCount;
    uint8_t					conc_mode;
    uint8_t					opmode;
    uint8_t					rssi;
    uint8_t                 network_id;
    uint32_t                roaming_time_out;
    TimerHandle_t           roaming_timer;
    char					country_code[3];
    qapi_WLAN_Set_Rate_Params_t rate_param;
} wlan_qapi_cxt_t;

extern wlan_qapi_cxt_t *gp_wlan_qapi_cxt;

#define set_wlan_qapi_error(x)   (gp_wlan_qapi_cxt->wlan_qapi_error = (x))
#define get_wlan_qapi_error()    (gp_wlan_qapi_cxt->wlan_qapi_error)
#define clr_wlan_qapi_error()    set_wlan_qapi_error(QAPI_OK)

#define CONFIG_WLAN_QAPI_BLOCK_ENABLE  1

#if CONFIG_WLAN_QAPI_BLOCK_ENABLE
#define WLAN_QAPI_LOCK()          qurt_mutex_lock(&gp_wlan_qapi_cxt->wlan_qapi_block_mutex)
#define WLAN_QAPI_UNLOCK()        qurt_mutex_unlock(&gp_wlan_qapi_cxt->wlan_qapi_block_mutex)
#else
#define WLAN_QAPI_LOCK()
#define WLAN_QAPI_UNLOCK()
#endif

qapi_Status_t wlan_drv_set_cb (qapi_WLAN_Callback_t callback, void *application_Context);
qapi_Status_t wlan_drv_roaming_start(void);
qapi_Status_t wlan_drv_roaming_stop(void);

#endif //__WLAN_DRV_H__

