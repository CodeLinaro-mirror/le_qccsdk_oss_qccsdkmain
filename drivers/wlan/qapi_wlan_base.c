
#include "wlan_drv.h"
#include "wmi_api.h"
#include "wlan_qapi_helper.h"

qapi_Status_t qapi_WLAN_Error (void)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    qapi_Status_t wlan_error = get_wlan_qapi_error();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    return wlan_error;
}

qapi_Status_t qapi_WLAN_Enabled (qapi_WLAN_Enable_e *enable)
{
    if (!enable) {
        PRINT_ERR_INVALID_PARAM;
        return QAPI_WLAN_ERR_EINVAL;
    }

    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    *enable = (qapi_WLAN_Enable_e)p_cxt->wlanEnabled;
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    return QAPI_OK;
}

qapi_Status_t qapi_WLAN_Enable (qapi_WLAN_Enable_e enable)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    WLAN_QAPI_LOCK();
    PRINT_LOG_FUNC_LINE_ENTRY;
    if (QAPI_WLAN_ENABLE_E==enable) {
        ret = wmi_on();
    } else if (QAPI_WLAN_DISABLE_E==enable) {
        ret = wmi_off();
    } else {
        PRINT_ERR_INVALID_PARAM;
        ret = QAPI_WLAN_ERR_EINVAL;
        goto exit;
    }
exit:
    PRINT_LOG_FUNC_LINE_EXIT;
    WLAN_QAPI_UNLOCK();
    return ret;
}

qapi_Status_t qapi_WLAN_Add_Device (uint8_t device_ID)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    WLAN_QAPI_LOCK();
    ret = wmi_add_device(device_ID);
    WLAN_QAPI_UNLOCK();
    return ret;
}

qapi_Status_t qapi_WLAN_Set_Callback (qapi_WLAN_Callback_t callback, void *application_Context)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    WLAN_QAPI_LOCK();
    ret = wlan_drv_set_cb(callback, application_Context);
    WLAN_QAPI_UNLOCK();
    return ret;
}

qapi_Status_t qapi_WLAN_Start_Scan(uint8_t device_ID, const qapi_WLAN_Start_Scan_Params_t *scan_Params)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    WLAN_QAPI_LOCK();
    ret = wmi_start_scan(device_ID, scan_Params);
    WLAN_QAPI_UNLOCK();
    return ret;
}

qapi_Status_t qapi_WLAN_Get_Scan_Results (uint8_t __attribute__((__unused__)) device_ID, qapi_WLAN_Scan_Comp_Evt_t *scan_Res, int16_t *num_Bss)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    if (!scan_Res || !num_Bss) {
        PRINT_ERR_INVALID_PARAM;
        return  QAPI_WLAN_ERR_EINVAL;
    }

    WLAN_QAPI_LOCK();
    ret = wlan_get_scan_results(device_ID, scan_Res, num_Bss);
    WLAN_QAPI_UNLOCK();
    return ret;
}

qapi_Status_t qapi_WLAN_Disconnect (uint8_t __attribute__((__unused__)) device_ID)
{
    qapi_Status_t ret = QAPI_OK;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    WLAN_QAPI_LOCK();
    if (p_cxt->connected==true) {
        ret = wmi_disconnect();
    }

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    memset(&p_cxt->connect_cmd, 0, sizeof(WMI_CONNECT_CMD));
    memset(&p_cxt->passphrase_cmd, 0, sizeof(WMI_SET_PASSPHRASE_CMD));
    wlan_clear_privacy();
    wlan_preset_specific_param();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);

    WLAN_QAPI_UNLOCK();
    return ret;
}

qapi_Status_t qapi_WLAN_Commit (uint8_t  __attribute__((__unused__)) device_ID)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    uint8_t authMode = p_cxt->connect_cmd.authMode;

    WLAN_QAPI_LOCK();
    if ((authMode==WMI_WPA_PSK_AUTH) || (authMode==WMI_WPA2_PSK_AUTH)) {
        wmi_set_passphrase();
    }
    ret = wmi_connect();
    WLAN_QAPI_UNLOCK();
    return ret;
}

qapi_Status_t qapi_WLAN_Set_11d(uint32_t  __attribute__((__unused__)) enable)
{
    WLAN_QAPI_LOCK();
    PRINT_ERR_NOT_SUPPORTED;
    WLAN_QAPI_UNLOCK();
    return QAPI_WLAN_ERROR;
}

qapi_Status_t qapi_WLAN_Get_11d(uint32_t  __attribute__((__unused__)) *result)
{
    WLAN_QAPI_LOCK();
    PRINT_ERR_NOT_SUPPORTED;
    WLAN_QAPI_UNLOCK();
    return QAPI_WLAN_ERROR;
}

qapi_Status_t qapi_WLAN_Get_Country_Code(char  __attribute__((__unused__)) *country_code)
{
    WLAN_QAPI_LOCK();
    PRINT_ERR_NOT_SUPPORTED;
    WLAN_QAPI_UNLOCK();
    return QAPI_WLAN_ERROR;
}

qapi_Status_t qapi_WLAN_Get_Regulatory_Info(qapi_WLAN_Reg_Evt_t *reg)
{
	return wlan_sta_get_reg_info(reg);
}

qapi_Status_t qapi_WLAN_Set_Rate (qapi_WLAN_Set_Rate_Params_t *prate_para)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    memcpy(&(p_cxt->rate_param), prate_para, sizeof(qapi_WLAN_Set_Rate_Params_t));

    WLAN_QAPI_LOCK();
    ret = wmi_set_rate();
    WLAN_QAPI_UNLOCK();
    return ret;
}

qapi_Status_t qapi_WLAN_Get_Rate (qapi_WLAN_Set_Rate_Params_t *prate_para)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    memcpy(&(p_cxt->rate_param), \
                  prate_para, \
                  sizeof(qapi_WLAN_Set_Rate_Params_t));

    WLAN_QAPI_LOCK();
    ret = wmi_get_rate();

    memcpy(prate_para, \
                 &(gp_wlan_qapi_cxt->rate_param), \
                 sizeof(qapi_WLAN_Set_Rate_Params_t));
    WLAN_QAPI_UNLOCK();
    return ret;
}

