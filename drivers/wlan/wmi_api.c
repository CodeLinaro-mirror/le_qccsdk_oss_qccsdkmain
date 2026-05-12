/*
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "wlan_drv.h"
#include "wlan_qapi_helper.h"
#include "wmi_api.h"
#include "safeAPI.h"
#include "assert.h"
#include "wmi.h"
#ifdef CONFIG_WPS
#include "wps_def.h"
#endif
#include "wlan_power.h"
#ifdef CONFIG_WLAN_8021X
#include "wlan_8021x_cxt.h"
#include "supplicant_cxt.h"
#endif

typedef void (*wlan_evt_fn_table)(void *);

extern qurt_pipe_t msg_wfm_wmi_id;
extern ppm_common_t g_ppm_common_struct;
#ifdef CONFIG_WLAN_8021X
extern wlan_8021x_global_t *g_wl8021x_global;
#endif

static void wmi_enabled_event(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    dev_common_t *dev_common = (dev_common_t *)msg;
    devh_t *dev = dev_common->devp[NT_DEV_AP_ID];
    qapi_WLAN_Enable_Evt_t enable_evt = {0};

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    p_cxt->dev_common = dev_common;
    if (dev) {
        enable_evt.evt_hdr.status = QAPI_OK;
        memscpy(enable_evt.mac_addr, __QAPI_WLAN_MAC_LEN, dev->ic_myaddr, __QAPI_WLAN_MAC_LEN);
        enable_evt.num_networks = dev->numConn;
        enable_evt.cap_info = dev->ic_flags;
        enable_evt.cap_info2 = dev->ic_flags2;
        p_cxt->wlanEnabled = true;
    } else {
        enable_evt.evt_hdr.status = QAPI_WLAN_ERR_ENOENT;
        err_printf("QAPI_WLAN_ENABLE_CB_E status error\n");
    }
    info_printf("QAPI_WLAN_ENABLE_CB_E sent\n");
    set_wlan_qapi_error(enable_evt.evt_hdr.status);
    if (p_cxt->wlan_enable_block_mode) {
        qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_ENABLE_DONE);
    }
    if (p_cxt->qapi_event_handler) {
        p_cxt->qapi_event_handler(p_cxt->network_id, QAPI_WLAN_ENABLE_CB_E, p_cxt->event_application_Context,
                                  &enable_evt, sizeof(qapi_WLAN_Enable_Evt_t));
    }
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
}

static void wmi_disabled_event(__unused void *msg)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_WLAN_Disable_Evt_t disable_evt = {0};

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    disable_evt.evt_hdr.status = QAPI_OK;
    info_printf("QAPI_WLAN_DISABLE_CB_E sent\n");
    p_cxt->wlanEnabled = false;
    set_wlan_qapi_error(disable_evt.evt_hdr.status);
    if (p_cxt->wlan_disable_block_mode) {
        qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_DISABLE_DONE);
    }
    if (p_cxt->qapi_event_handler) {
        p_cxt->qapi_event_handler(p_cxt->network_id, QAPI_WLAN_DISABLE_CB_E, p_cxt->event_application_Context,
                                  &disable_evt, sizeof(qapi_WLAN_Enable_Evt_t));
    }
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
}

static void wmi_if_added_event(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_WLAN_If_Add_Comp_Evt_t if_comp_evt = {0};

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);

    p_cxt->network_id = ((WMI_NETIF_ADD_EVT *)msg)->net_id;
    if (0 == ((WMI_NETIF_ADD_EVT *)msg)->status) {
        if_comp_evt.evt_hdr.status = QAPI_OK;
    } else {
        if_comp_evt.evt_hdr.status = QAPI_WLAN_ERR_EINVAL;
        err_printf("QAPI_WLAN_IF_ADD_COMP_CB_E status error\n");
    }

    info_printf("QAPI_WLAN_IF_ADD_COMP_CB_E sent, network_id=%d\n", p_cxt->network_id);
    set_wlan_qapi_error(if_comp_evt.evt_hdr.status);
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    if (p_cxt->wlan_if_add_block_mode) {
        qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_IF_ADDED);
    }
    if (p_cxt->qapi_event_handler) {
        p_cxt->qapi_event_handler(p_cxt->network_id, QAPI_WLAN_IF_ADD_COMP_CB_E, p_cxt->event_application_Context,
                                  &if_comp_evt, sizeof(qapi_WLAN_If_Add_Comp_Evt_t));
    }
}

static void wmi_set_mode_event(void *msg)
{
    int8_t ret;
    qapi_Status_t err = QAPI_ERROR;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    PRINT_LOG_FUNC_LINE_ENTRY;
    ret = *(int8_t *)msg;
    if (ret == eWiFiSuccess)
        err = QAPI_OK;
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    set_wlan_qapi_error(err);
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);

    if (p_cxt->wlan_if_add_block_mode) {
        qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_MODE);
    }
}

static void wmi_scan_started_event(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    PRINT_LOG_FUNC_LINE_ENTRY;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_WLAN_Scan_Start_Evt_t scan_start_evt = {0};
    uint8_t scan_id = *((uint8_t *)msg);

    scan_start_evt.evt_hdr.status = QAPI_OK;
    scan_start_evt.scan_id = scan_id;

    info_printf("QAPI_WLAN_SCAN_START_CB_E sent, scan_id=%d\n", scan_start_evt.scan_id);
    if (p_cxt->wlan_scan_start_block_mode) {
        qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_STARTED_SCAN);
    }
    if (p_cxt->qapi_event_handler) {
        p_cxt->qapi_event_handler(p_cxt->network_id, QAPI_WLAN_SCAN_START_CB_E, p_cxt->event_application_Context,
                                  &scan_start_evt, sizeof(qapi_WLAN_Scan_Start_Evt_t));
    }
    PRINT_LOG_FUNC_LINE_EXIT;
}

extern dev_common_t *gpDevCommon;
uint8_t dc_freq_to_chindex(dev_common_t *pDevCmn, uint32_t frequency);

static void _wlan_fill_scan_info(qapi_WLAN_BSS_Scan_Info_t *dst, const ap_info *src)
{
    uint16_t channel = src->chan_freq;
    uint16_t rsn_Cipher, rsn_Auth, wpa_Cipher, wpa_Auth;
    extern int32_t wlan_freq_to_channel(uint16_t * channel);
    wlan_freq_to_channel(&channel);
    dst->channel = channel;
    memscpy(dst->bssid, __QAPI_WLAN_MAC_LEN, src->bssid, __QAPI_WLAN_MAC_LEN);
    dst->ssid_Length = src->ssid.ssid_len;
    if (dst->ssid_Length > __QAPI_WLAN_MAX_SSID_LEN) {
        dst->ssid_Length = __QAPI_WLAN_MAX_SSID_LEN;
    }
    memscpy(dst->ssid, dst->ssid_Length, src->ssid.ssid, dst->ssid_Length);
    dst->rssi = src->rssi;
    // src->wlan_mode  //wlan phy mode, WLAN_PHY_MODE, no map
    if (src->wpa_security_mode) {
        dst->security_Enabled = 1;
        wpa_Cipher = src->wpa_security_mode >> 16;
        wpa_Auth = (src->wpa_security_mode & 0xFFFF);
        if (wpa_Cipher & TKIP_CRYPT)
            dst->wpa_Cipher |= __QAPI_WLAN_CIPHER_TYPE_TKIP;

        if (wpa_Cipher & AES_CRYPT)
            dst->wpa_Cipher |= __QAPI_WLAN_CIPHER_TYPE_CCMP;

        if (wpa_Cipher & WEP_CRYPT)
            dst->wpa_Cipher |= __QAPI_WLAN_CIPHER_TYPE_WEP;

        if (wpa_Auth & WMI_WPA_AUTH)
            dst->wpa_Auth |= __QAPI_WLAN_SECURITY_AUTH_1X;

        if (wpa_Auth & WMI_WPA_PSK_AUTH)
            dst->wpa_Auth |= __QAPI_WLAN_SECURITY_AUTH_PSK;
    }

    if (src->rsn_security_mode) {
        dst->security_Enabled = 1;
        rsn_Auth = src->rsn_security_mode & 0xFFFF;
        rsn_Cipher = (src->rsn_security_mode) >> 16;
        if (rsn_Cipher & TKIP_CRYPT)
            dst->rsn_Cipher |= __QAPI_WLAN_CIPHER_TYPE_TKIP;

        if (rsn_Cipher & AES_CRYPT)
            dst->rsn_Cipher |= __QAPI_WLAN_CIPHER_TYPE_CCMP;

        if (rsn_Cipher & WEP_CRYPT)
            dst->rsn_Cipher |= __QAPI_WLAN_CIPHER_TYPE_WEP;

        if (rsn_Auth & WMI_WPA2_AUTH)
            dst->rsn_Auth |= __QAPI_WLAN_SECURITY_AUTH_1X;

#ifdef CONFIG_WLAN_8021X
        if (rsn_Auth & (WMI_WPA2_SHA256_AUTH | WMI_WPA3_ENTERPRISE_ONLY_AUTH))
            dst->rsn_Auth |= __QAPI_WLAN_SECURITY_AUTH_WPA3_1X;

        if (rsn_Auth & WMI_WPA3_ENTERPRISE_B_192_AUTH)
            dst->rsn_Auth |= __QAPI_WLAN_SECURITY_AUTH_WPA3_1X_B_192;
#endif

        if ((rsn_Auth & WMI_WPA2_PSK_AUTH))
            dst->rsn_Auth |= __QAPI_WLAN_SECURITY_AUTH_PSK;

        if (rsn_Auth & WMI_WPA3_SHA256_AUTH)
            dst->rsn_Auth |= __QAPI_WLAN_SECURITY_AUTH_SAE;
    }
}

static void wmi_scan_comp_event(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    PRINT_LOG_FUNC_LINE_ENTRY;

    SCAN_RESULT *p_scan_result = (SCAN_RESULT *)msg;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    uint8_t num_entries, last_idx;

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    if (!p_cxt->scan_in_progress) {
        qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
        // warn_printf("no pending scan, ignore\n");
        return;
    }

    qapi_WLAN_Scan_Comp_Evt_t *scan_comp_evt = (qapi_WLAN_Scan_Comp_Evt_t *)p_cxt->pScanOut;
    scan_comp_evt->evt_hdr.status = QAPI_OK;

    if (scan_comp_evt->total_bss < p_cxt->scanBssMaxCount) {

        last_idx = scan_comp_evt->num_bss_cur;
        int cur_idx = last_idx;
        for (int i = 0; i < p_scan_result->num_entries && cur_idx < p_cxt->scanBssMaxCount; ++i) {
            int duplicate = 0;
            for (int j = 0; j < last_idx; ++j) {
                if (memcmp(scan_comp_evt->scan_bss_info[j].bssid, p_scan_result->scan_bss_info[i].bssid,
                           IEEE80211_ADDR_LEN) == 0) {
                    duplicate = 1;
                    break;
                }
            }
            if (!duplicate) {
                _wlan_fill_scan_info(&scan_comp_evt->scan_bss_info[cur_idx],
                                     (ap_info *)((unsigned char *)p_scan_result + offsetof(SCAN_RESULT, scan_bss_info) +
                                                 sizeof(ap_info) * i));
                cur_idx++;
            }
        }
        scan_comp_evt->num_bss_cur = cur_idx;
    }
    scan_comp_evt->total_bss = scan_comp_evt->num_bss_cur;
    log_printf("scan found %d bss, our capacity %d\n", scan_comp_evt->total_bss, p_cxt->scanBssMaxCount);

    p_cxt->scan_in_progress = false;
    set_wlan_qapi_error(scan_comp_evt->evt_hdr.status);
    if (p_cxt->wait_scan_comp_evt) {
        p_cxt->wait_scan_comp_evt = false;
        log_printf("wakeup qapi_WLAN_Get_Scan_Results\n");
        qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
        qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SCAN_COMPLETED);
        qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    }
    if (p_cxt->qapi_event_handler) {
        info_printf("QAPI_WLAN_SCAN_COMPLETE_CB_E sent, scan_id=%d\n", scan_comp_evt->scan_id);
        uint32_t len =
            sizeof(qapi_WLAN_Scan_Comp_Evt_t) + sizeof(qapi_WLAN_BSS_Scan_Info_t) * scan_comp_evt->num_bss_cur;
        p_cxt->qapi_event_handler(p_cxt->network_id, QAPI_WLAN_SCAN_COMPLETE_CB_E, p_cxt->event_application_Context,
                                  p_cxt->pScanOut, len);
    }
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    PRINT_LOG_FUNC_LINE_EXIT;
}

static void wmi_scan_result_event(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    PRINT_LOG_FUNC_LINE_ENTRY;

    SCAN_RESULT *p_scan_result = (SCAN_RESULT *)msg;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    uint8_t num_entries, last_idx;

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    if (!p_cxt->scan_in_progress) {
        qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
        // warn_printf("no pending scan, ignore\n");
        return;
    }

    qapi_WLAN_Scan_Comp_Evt_t *scan_comp_evt = (qapi_WLAN_Scan_Comp_Evt_t *)p_cxt->pScanOut;
    if (scan_comp_evt->total_bss < p_cxt->scanBssMaxCount) {

        last_idx = scan_comp_evt->num_bss_cur;
        int cur_idx = last_idx;
        for (int i = 0; i < p_scan_result->num_entries && cur_idx < p_cxt->scanBssMaxCount; ++i) {
            int duplicate = 0;
            for (int j = 0; j < last_idx; ++j) {
                if (memcmp(scan_comp_evt->scan_bss_info[j].bssid, p_scan_result->scan_bss_info[i].bssid,
                           IEEE80211_ADDR_LEN) == 0) {
                    duplicate = 1;
                    break;
                }
            }
            if (!duplicate) {
                _wlan_fill_scan_info(&scan_comp_evt->scan_bss_info[cur_idx],
                                     (ap_info *)((unsigned char *)p_scan_result + offsetof(SCAN_RESULT, scan_bss_info) +
                                                 sizeof(ap_info) * i));
                cur_idx++;
            }
        }
        scan_comp_evt->num_bss_cur = cur_idx;
    }
    scan_comp_evt->total_bss = scan_comp_evt->num_bss_cur;

    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    PRINT_LOG_FUNC_LINE_EXIT;
}

extern void show_net_info_by_id(uint8_t id, uint8_t ip_ver);
static void wmi_ip_addr_ready_event(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    PRINT_LOG_FUNC_LINE_ENTRY;
#ifdef CONFIG_NET_SHELL
    WMI_IP_DDR_EVT *ip_ready_evt = (WMI_IP_DDR_EVT *)msg;

    show_net_info_by_id(ip_ready_evt->netif_id, ip_ready_evt->ip_ver);
#endif
}

static void _wlan_fill_join_event(qapi_WLAN_Join_Comp_Evt_t *dst, const WMI_JOIN_EVT *src)
{
    uint8_t mac_addr[__QAPI_WLAN_MAC_LEN];
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    wlan_get_mac_address(0, mac_addr);
    if (p_cxt->opmode == WHAL_M_AP && memcmp(src->bssid, mac_addr, __QAPI_WLAN_MAC_LEN) != 0)
        dst->bss_Connection_Status = 0;
    else
        dst->bss_Connection_Status = 1;

    /*The status of disconnection from command will be treated as success anyway in current version.*/
    if (src->status == NT_OK || src->reason_code == DISCONNECT_CMD) {
        dst->evt_hdr.status = QAPI_OK;
    } else {
        dst->evt_hdr.status = QAPI_WLAN_ERR_EPROTO;
    }

    if (p_cxt->opmode == WHAL_M_STA)
        memscpy(dst->passphrase, WMI_PASSPHRASE_LEN + 1, src->passphrase, WMI_PASSPHRASE_LEN + 1);

    memscpy(dst->bssid, __QAPI_WLAN_MAC_LEN, src->bssid, __QAPI_WLAN_MAC_LEN);
    dst->ssid_Length = src->ssid.ssid_len;
    memscpy(dst->ssid, dst->ssid_Length, src->ssid.ssid, dst->ssid_Length);
    dst->assoc_id = src->assoc_id;
    // dst->host_initiated = src->host_initiated; //host can judge this
    dst->reason_code = src->reason_code;
    dst->channel_frequency = src->channel_frequency;
}

static void wmi_join_comp_event(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    PRINT_LOG_FUNC_LINE_ENTRY;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_WLAN_Join_Comp_Evt_t *p_qapi_join_evt = &p_cxt->connect_result;

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);

    _wlan_fill_join_event(p_qapi_join_evt, (WMI_JOIN_EVT *)msg);
    set_wlan_qapi_error(p_qapi_join_evt->evt_hdr.status);

    if (p_qapi_join_evt->bss_Connection_Status == 0)
        goto done;

    if (p_qapi_join_evt->evt_hdr.status == QAPI_OK && (((WMI_JOIN_EVT *)msg)->reason_code != DISCONNECT_CMD)) {
        p_cxt->connected = true;
    } else {
        p_cxt->connected = false;
    }

    if (p_cxt->connect_in_progress) {
        if (p_cxt->wlan_connect_block_mode) {
            info_printf("wakeup qapi_WLAN_Commit about connect complete\n");
            qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
            if (p_cxt->connected) {
                qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_CONNECT_COMPLETED);

                stop_imps_cnx_wait_timer();
            } else {
                qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_DISCONNECTED);
                start_imps_cnx_wait_timer(g_ppm_common_struct.imps_struct_ctx.recnx_wait_time_ms);
            }
            qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
        }
        p_qapi_join_evt->host_initiated = true;
        p_cxt->connect_in_progress = false;
    }
    if (p_cxt->disconnect_in_progress) {
        if (p_cxt->qapi_event_handler) {
            p_cxt->qapi_event_handler(p_cxt->network_id, QAPI_WLAN_DISCONNECT_CB_E, p_cxt->event_application_Context,
                                      p_qapi_join_evt, sizeof(qapi_WLAN_Join_Comp_Evt_t));
        }
        if (p_cxt->wlan_disconnect_block_mode) {
            info_printf("wakeup qapi_WLAN_Disconnect about disconnect complete\n");
            qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
            qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_DISCONNECTED);
            qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
        }
        p_qapi_join_evt->host_initiated = true;
        p_cxt->disconnect_in_progress = false;

        qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
        goto exit;
    }

done:
    if (p_cxt->qapi_event_handler) {
        info_printf("QAPI_WLAN_CONNECT_CB_E sent, status=%d\n", p_qapi_join_evt->evt_hdr.status);
        p_cxt->qapi_event_handler(p_cxt->network_id, QAPI_WLAN_CONNECT_CB_E, p_cxt->event_application_Context,
                                  p_qapi_join_evt, sizeof(qapi_WLAN_Join_Comp_Evt_t));
    }

    if (p_cxt->opmode == WHAL_M_STA) {
        if (p_cxt->connected == false) {
            wlan_drv_roaming_start();
            start_imps_cnx_wait_timer(g_ppm_common_struct.imps_struct_ctx.recnx_wait_time_ms);
        } else {
            wlan_drv_roaming_stop();
            stop_imps_cnx_wait_timer();
        }
    }

    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
exit:
    PRINT_LOG_FUNC_LINE_EXIT;
}

static void wmi_disconnect_event(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    WMI_DISC_EVT *evt = (WMI_DISC_EVT *)msg;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_WLAN_Join_Comp_Evt_t *p_qapi_join_evt = &p_cxt->connect_result;

    info_printf("start_imps_cnx_wait_timer\r\n");
    start_imps_cnx_wait_timer(g_ppm_common_struct.imps_struct_ctx.recnx_wait_time_ms);

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    {
        p_qapi_join_evt->assoc_id = evt->assoc_id;
        p_qapi_join_evt->reason_code = evt->reason;
        p_qapi_join_evt->evt_hdr.status = QAPI_WLAN_ERR_EPROTO;
    }
    p_cxt->connected = false;

    if (p_cxt->disconnect_in_progress) {
        if (p_cxt->wlan_disconnect_block_mode) {
            info_printf("wakeup qapi_WLAN_Disconnect about disconnect complete\n");
            qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_DISCONNECTED);
        }
        p_qapi_join_evt->host_initiated = true;
        p_cxt->disconnect_in_progress = false;
    }

    if (p_cxt->qapi_event_handler) {
        info_printf("QAPI_WLAN_DISCONNECT_CB_E sent, status=%d\n", p_qapi_join_evt->evt_hdr.status);
        p_cxt->qapi_event_handler(p_cxt->network_id, QAPI_WLAN_CONNECT_CB_E, p_cxt->event_application_Context,
                                  p_qapi_join_evt, sizeof(qapi_WLAN_Join_Comp_Evt_t));
    }

    if (p_cxt->opmode == WHAL_M_STA)
        wlan_drv_roaming_start();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
}

static void wmi_set_param_event(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    SET_PDEV_PARAM_RESULT *buffer = (SET_PDEV_PARAM_RESULT *)msg;

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    if (p_cxt->wlan_set_param_block_mode) {
        uint8_t num = 0;
        while (buffer->param_id != p_cxt->param_id) {
            vTaskDelay(10 / portTICK_PERIOD_MS);
            num++;
            if (num >= 100) {
                err_printf("buffer param id:%d, pcxt paramid is %d\r\n", buffer->param_id, p_cxt->param_id);
                err_printf("param id can not match\n");
                break;
            }
        }
    }
    if (buffer->param_id == p_cxt->param_id && p_cxt->wlan_set_param_block_mode) {
        if (buffer->status == WIFI_STATUS_SUCCESS) {
            set_wlan_qapi_error(QAPI_OK);
        } else {
            set_wlan_qapi_error(QAPI_ERROR);
        }
        qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_PARAM);
    }
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
}

static void wmi_report_stat_event(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    wlan_cserv_stats_t *stat = (wlan_cserv_stats_t *)msg;

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    if (stat->status) {
        p_cxt->rssi = stat->cs_rssi;
        set_wlan_qapi_error(QAPI_OK);
    } else {
        set_wlan_qapi_error(QAPI_ERROR);
    }
    if (p_cxt->wlan_get_stat_block_mode) {
        qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_GET_STAT);
    }
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
}

static void wmi_regulatory_event(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    int i, num;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_WLAN_Reg_Evt_t *evt = &(p_cxt->reg_result);
    wlan_regulatory_t *result = (wlan_regulatory_t *)msg;

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    memscpy(evt->alpha, 3, result->alpha, 3);
    evt->num_2g_reg_rules = result->num_2g_reg_rules;
    evt->num_5g_reg_rules = result->num_5g_reg_rules;

    num = (result->num_2g_reg_rules) + (result->num_5g_reg_rules);
    if (num > QAPI_MAX_REG_RULES) {
        set_wlan_qapi_error(QAPI_ERROR);
        goto error;
    }
    for (i = 0; i < num; i++) {
        evt->reg_rules[i].start_freq = result->reg_rules[i].start_freq;
        evt->reg_rules[i].end_freq = result->reg_rules[i].end_freq;
        evt->reg_rules[i].reg_power = result->reg_rules[i].reg_power;
        evt->reg_rules[i].ant_gain = result->reg_rules[i].ant_gain;
        evt->reg_rules[i].flag_info = result->reg_rules[i].flag_info;
        evt->reg_rules[i].max_bw = result->reg_rules[i].max_bw;
    }
    set_wlan_qapi_error(QAPI_OK);
error:
    if (p_cxt->wlan_get_regulatory_block_mode) {
        qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_GET_REG);
    }
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
}

static void wmi_set_rate_event(void *msg)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    int ret = *(int *)msg;

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);

    set_wlan_qapi_error((ret == 1) ? QAPI_OK : QAPI_ERROR);
    if (p_cxt->wlan_disable_block_mode) {
        qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_RATE);
    }

    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
}

static void wmi_get_tx_power_event(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_WLAN_Get_Power_Evt_t *evt = &(p_cxt->get_tx_power_result);
    wlan_tx_power_t *result = (wlan_tx_power_t *)msg;

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    evt->ctl_power = result->ctl_power;
    evt->real_power = result->real_power;
    evt->reg_power = result->reg_power;
    evt->target_power = result->target_power;

    set_wlan_qapi_error(QAPI_OK);

    if (p_cxt->wlan_get_tx_power_block_mode) {
        qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_GET_TX_POWER);
    }
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
}

static void wmi_get_rate_event(void *msg)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);

    memscpy(&(gp_wlan_qapi_cxt->rate_param), sizeof(gp_wlan_qapi_cxt->rate_param), msg,
            sizeof(qapi_WLAN_Set_Rate_Params_t));

    if (p_cxt->wlan_disable_block_mode) {
        qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_GET_RATE);
    }

    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
}

static void wmi_get_bmps_stats_event(void *msg)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);

    if (p_cxt->wlan_disable_block_mode) {
        qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_GET_BMPS_STATS);
    }

    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
}

static void wmi_set_mgmt_filter_event(void *msg)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    int ret = *(int *)msg;

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);

    if (p_cxt->wlan_set_mgmt_filter_block_mode) {
        qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_MGMT_FILTER);
    }

    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
}

#ifdef CONFIG_WPS
static void wmi_stop_scan_event(void *msg)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    int ret = *(int *)msg;

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);

    if (p_cxt->wlan_scan_stop_block_mode) {
        qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_STOPPED_SCAN);
    }

    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
}

static void wmi_wps_fail_event(void *msg)
{
    qapi_WLAN_WPS_Fail_Evt_t qapi_wps_fail_evt;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    int reason = *(int *)msg;

    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    p_cxt->connected = false;

    if (p_cxt->qapi_event_handler) {
        p_cxt->qapi_event_handler(p_cxt->network_id, QAPI_WLAN_WPS_FAIL_CB_E, p_cxt->event_application_Context, &reason,
                                  sizeof(reason));
    }

    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
}
#endif

static void wmi_chan_switch_event(void *msg)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_WLAN_Chan_Switch_Evt_t qapi_chan_switch_evt;
    chan_switch_event *chan_switch_evt = (chan_switch_event *)msg;

    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    if (p_cxt->qapi_event_handler) {
        memset(&qapi_chan_switch_evt, 0, sizeof(qapi_chan_switch_evt));
        if (chan_switch_evt->status == 0) {
            qapi_chan_switch_evt.evt_hdr.status = QAPI_OK;
            qapi_chan_switch_evt.freq = chan_switch_evt->new_chan_freq;
        } else {
            qapi_chan_switch_evt.evt_hdr.status = QAPI_ERROR;
            qapi_chan_switch_evt.reason = chan_switch_evt->reason;
        }
        info_printf("QAPI_WLAN_CHANNEL_SWITCH_CB_E sent, status=%d\n", qapi_chan_switch_evt.evt_hdr.status);
        p_cxt->qapi_event_handler(p_cxt->network_id, QAPI_WLAN_CHANNEL_SWITCH_CB_E, p_cxt->event_application_Context,
                                  &qapi_chan_switch_evt, sizeof(qapi_chan_switch_evt));
    }
}

static void wmi_send_raw_event(void *msg)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    int ret = *(int *)msg;

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);

    set_wlan_qapi_error((ret == 1) ? QAPI_OK : QAPI_ERROR);
    if (p_cxt->wlan_disable_block_mode) {
        qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SEND_RAW);
    }

    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
}

#if CONFIG_ENABLE_P2P_MODE
void fill_p2p_event_info(qapi_WLAN_P2P_Event_Cb_Info_t *p2p_evt, void *pData)
{
    if (NULL == p2p_evt)
        return;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    if (p2p_evt->event_ID == WMI_P2P_GO_NEG_RESULT_EVENTID) {
        WMI_P2P_GO_NEG_RESULT_EVENT *p2p_go_result;
        qapi_WLAN_P2P_Go_Neg_Result_Event_t *q_p2p_go_result = &(p2p_evt->WLAN_P2P_Event_Info.go_Neg_Result_Event);

        p2p_evt->event_ID = __QAPI_WLAN_P2P_GO_NEG_RESULT_EVENTID;
        p2p_go_result = (WMI_P2P_GO_NEG_RESULT_EVENT *)pData;
        q_p2p_go_result->freq = p2p_go_result->freq;
        q_p2p_go_result->status = p2p_go_result->status;
        q_p2p_go_result->role_Go = p2p_go_result->role_go;
        q_p2p_go_result->ssid_Len = p2p_go_result->ssid_len;
        memscpy(q_p2p_go_result->ssid, q_p2p_go_result->ssid_Len, p2p_go_result->ssid, q_p2p_go_result->ssid_Len);
        memscpy(q_p2p_go_result->pass_Phrase, WMI_MAX_P2P_PASSPHRASE_STR_LEN, p2p_go_result->pass_phrase, WMI_MAX_P2P_PASSPHRASE_STR_LEN);
        memscpy(q_p2p_go_result->peer_Device_Addr, sizeof(q_p2p_go_result->peer_Device_Addr), p2p_go_result->peer_device_addr,
               sizeof(q_p2p_go_result->peer_Device_Addr));
        memscpy(q_p2p_go_result->peer_Interface_Addr, sizeof(q_p2p_go_result->peer_Interface_Addr), p2p_go_result->peer_interface_addr,
               sizeof(q_p2p_go_result->peer_Interface_Addr));
        q_p2p_go_result->wps_Method = p2p_go_result->wps_method;
        q_p2p_go_result->persistent_Grp = p2p_go_result->persistent_grp;
        q_p2p_go_result->passphrase_Len = p2p_go_result->passphrase_len;

        if (p_cxt->p2p_connect_in_progress) {
            if (p_cxt->wlan_p2p_block_mode) {
                qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_GO_NEG_RESULT);
            }
            p_cxt->p2p_connect_in_progress = false;
        }
    } else if (p2p_evt->event_ID == WMI_P2P_REQ_TO_AUTH_EVENTID) {
        WMI_P2P_REQ_TO_AUTH_EVENT *p2p_req_auth;
        qapi_WLAN_P2P_Req_To_Auth_Event_t *q_p2p_req_auth = &p2p_evt->WLAN_P2P_Event_Info.req_Auth_Event;

        p2p_evt->event_ID = __QAPI_WLAN_P2P_REQ_TO_AUTH_EVENTID;
        p2p_req_auth = (WMI_P2P_REQ_TO_AUTH_EVENT *)pData;
        memscpy(q_p2p_req_auth->sa, sizeof(q_p2p_req_auth->sa), p2p_req_auth->sa, sizeof(q_p2p_req_auth->sa));
        q_p2p_req_auth->dialog_Token = p2p_req_auth->dialog_token;
        q_p2p_req_auth->dev_Password_Id = p2p_req_auth->dev_password_id;
    } else if (p2p_evt->event_ID == WMI_P2P_PROV_DISC_REQ_EVENTID) {
        WMI_P2P_PROV_DISC_REQ_EVENT *p2p_prov_disc_req;
        qapi_WLAN_P2P_Prov_Disc_Req_Event_t *q_p2p_prov_disc_req = &p2p_evt->WLAN_P2P_Event_Info.prov_Disc_Req_Event;

        p2p_evt->event_ID = __QAPI_WLAN_P2P_PROV_DISC_REQ_EVENTID;
        p2p_prov_disc_req = (WMI_P2P_PROV_DISC_REQ_EVENT *)pData;
        memscpy(q_p2p_prov_disc_req->sa, sizeof(q_p2p_prov_disc_req->sa), p2p_prov_disc_req->sa, sizeof(q_p2p_prov_disc_req->sa));
        q_p2p_prov_disc_req->wps_Config_Method = p2p_prov_disc_req->wps_config_method;
        memscpy(q_p2p_prov_disc_req->dev_Addr, sizeof(q_p2p_prov_disc_req->dev_Addr), p2p_prov_disc_req->dev_addr, sizeof(q_p2p_prov_disc_req->dev_Addr));
        memscpy(q_p2p_prov_disc_req->pri_Dev_Type, sizeof(q_p2p_prov_disc_req->pri_Dev_Type), p2p_prov_disc_req->pri_dev_type,
               sizeof(q_p2p_prov_disc_req->pri_Dev_Type));
        memscpy(q_p2p_prov_disc_req->device_Name, sizeof(q_p2p_prov_disc_req->device_Name), p2p_prov_disc_req->device_name,
               sizeof(q_p2p_prov_disc_req->device_Name));
        q_p2p_prov_disc_req->dev_Name_Len = p2p_prov_disc_req->dev_name_len;
        q_p2p_prov_disc_req->dev_Config_Methods = p2p_prov_disc_req->dev_config_methods;
        q_p2p_prov_disc_req->device_Capab = p2p_prov_disc_req->device_capab;
        q_p2p_prov_disc_req->group_Capab = p2p_prov_disc_req->group_capab;
    } else if (p2p_evt->event_ID == WMI_P2P_SDPD_RX_EVENTID) {
        WMI_P2P_SDPD_RX_EVENT *p2p_service_disc_req;
        qapi_WLAN_P2P_Sdpd_Rx_Event_t *q_p2p_service_disc_req = &p2p_evt->WLAN_P2P_Event_Info.serv_Disc_Recv_Event;

        p2p_evt->event_ID = __QAPI_WLAN_P2P_SDPD_RX_EVENTID;
        p2p_service_disc_req = (WMI_P2P_SDPD_RX_EVENT *)pData;
        q_p2p_service_disc_req->type = p2p_service_disc_req->type;
        q_p2p_service_disc_req->transaction_Status = p2p_service_disc_req->transaction_status;
        q_p2p_service_disc_req->dialog_Token = p2p_service_disc_req->dialog_token;
        q_p2p_service_disc_req->frag_Id = p2p_service_disc_req->frag_id;
        memscpy(q_p2p_service_disc_req->peer_Addr, sizeof(q_p2p_service_disc_req->peer_Addr), p2p_service_disc_req->peer_addr,
               sizeof(q_p2p_service_disc_req->peer_Addr));
        q_p2p_service_disc_req->freq = p2p_service_disc_req->freq;
        q_p2p_service_disc_req->status_Code = p2p_service_disc_req->status_code;
        q_p2p_service_disc_req->comeback_Delay = p2p_service_disc_req->comeback_delay;
        q_p2p_service_disc_req->tlv_Length = p2p_service_disc_req->tlv_length;
        q_p2p_service_disc_req->update_Indic = p2p_service_disc_req->update_indic;
    } else if (p2p_evt->event_ID == WMI_P2P_INVITE_REQ_EVENTID) {
        WMI_P2P_FW_INVITE_REQ_EVENT *p2p_invite_req;
        qapi_WLAN_P2P_Invite_Req_Event_t *q_p2p_invite_req = &(p2p_evt->WLAN_P2P_Event_Info.invite_Req_Event);

        p2p_evt->event_ID = __QAPI_WLAN_P2P_INVITE_REQ_EVENTID;
        p2p_invite_req = (WMI_P2P_FW_INVITE_REQ_EVENT *)pData;
        memscpy(q_p2p_invite_req->sa, sizeof(q_p2p_invite_req->sa), p2p_invite_req->sa, sizeof(q_p2p_invite_req->sa));
        memscpy(q_p2p_invite_req->bssid, sizeof(q_p2p_invite_req->bssid), p2p_invite_req->bssid, sizeof(q_p2p_invite_req->bssid));
        memscpy(q_p2p_invite_req->go_Dev_Addr, sizeof(q_p2p_invite_req->go_Dev_Addr), p2p_invite_req->go_dev_addr, sizeof(q_p2p_invite_req->go_Dev_Addr));
        q_p2p_invite_req->ssid.ssid_Length = p2p_invite_req->ssid.ssidLength;
        memscpy(q_p2p_invite_req->ssid.ssid, q_p2p_invite_req->ssid.ssid_Length, p2p_invite_req->ssid.ssid, q_p2p_invite_req->ssid.ssid_Length);
        q_p2p_invite_req->is_Persistent = p2p_invite_req->is_persistent;
        q_p2p_invite_req->dialog_Token = p2p_invite_req->dialog_token;
    } else if (p2p_evt->event_ID == WMI_P2P_INVITE_RCVD_RESULT_EVENTID) {
        WMI_P2P_INVITE_RCVD_RESULT_EVENT *p2p_invite_rcvd_result;
        qapi_WLAN_P2P_Invite_Rcvd_Result_Event_t *q_p2p_invite_rcvd_result =
            &p2p_evt->WLAN_P2P_Event_Info.invite_Rcvd_Result_Event;

        p2p_evt->event_ID = __QAPI_WLAN_P2P_INVITE_RCVD_RESULT_EVENTID;
        p2p_invite_rcvd_result = (WMI_P2P_INVITE_RCVD_RESULT_EVENT *)pData;
        q_p2p_invite_rcvd_result->oper_Freq = p2p_invite_rcvd_result->oper_freq;
        memscpy(q_p2p_invite_rcvd_result->sa, sizeof(q_p2p_invite_rcvd_result->sa), p2p_invite_rcvd_result->sa, sizeof(q_p2p_invite_rcvd_result->sa));
        memscpy(q_p2p_invite_rcvd_result->bssid, sizeof(q_p2p_invite_rcvd_result->bssid), p2p_invite_rcvd_result->bssid, sizeof(q_p2p_invite_rcvd_result->bssid));
        q_p2p_invite_rcvd_result->is_Bssid_Valid = p2p_invite_rcvd_result->is_bssid_valid;
        memscpy(q_p2p_invite_rcvd_result->go_Dev_Addr, sizeof(q_p2p_invite_rcvd_result->go_Dev_Addr), p2p_invite_rcvd_result->go_dev_addr,
               sizeof(q_p2p_invite_rcvd_result->go_Dev_Addr));
        q_p2p_invite_rcvd_result->ssid.ssid_Length = p2p_invite_rcvd_result->ssid.ssidLength;
        memscpy(q_p2p_invite_rcvd_result->ssid.ssid, sizeof(q_p2p_invite_rcvd_result->ssid.ssid),
               p2p_invite_rcvd_result->ssid.ssid, sizeof(q_p2p_invite_rcvd_result->ssid.ssid));
        q_p2p_invite_rcvd_result->ssid.ssid_Length = p2p_invite_rcvd_result->ssid.ssidLength;
        q_p2p_invite_rcvd_result->status = p2p_invite_rcvd_result->status;
    } else if (p2p_evt->event_ID == WMI_P2P_INVITE_SENT_RESULT_EVENTID) {
        WMI_P2P_INVITE_SENT_RESULT_EVENT *p2p_invite_sent_result;
        qapi_WLAN_P2P_Invite_Sent_Result_Event_t *q_p2p_invite_sent_result =
            &p2p_evt->WLAN_P2P_Event_Info.invite_Sent_Result_Event;

        p2p_evt->event_ID = __QAPI_WLAN_P2P_INVITE_SENT_RESULT_EVENTID;
        p2p_invite_sent_result = (WMI_P2P_INVITE_SENT_RESULT_EVENT *)pData;
        q_p2p_invite_sent_result->status = p2p_invite_sent_result->status;
        memscpy(q_p2p_invite_sent_result->bssid, sizeof(q_p2p_invite_sent_result->bssid), p2p_invite_sent_result->bssid, sizeof(q_p2p_invite_sent_result->bssid));
        q_p2p_invite_sent_result->is_Bssid_Valid = p2p_invite_sent_result->is_bssid_valid;
    } else if (p2p_evt->event_ID == WMI_P2P_PROV_DISC_RESP_EVENTID) {
        WMI_P2P_PROV_DISC_RESP_EVENT *p2p_prov_disc_resp;
        qapi_WLAN_P2P_Prov_Disc_Resp_Event_t *q_p2p_prov_disc_resp = &p2p_evt->WLAN_P2P_Event_Info.prov_Disc_Resp_Event;

        p2p_evt->event_ID = __QAPI_WLAN_P2P_PROV_DISC_RESP_EVENTID;
        p2p_prov_disc_resp = (WMI_P2P_PROV_DISC_RESP_EVENT *)pData;
        q_p2p_prov_disc_resp->config_Methods = p2p_prov_disc_resp->config_methods;
        memscpy(q_p2p_prov_disc_resp->peer, sizeof(q_p2p_prov_disc_resp->peer), p2p_prov_disc_resp->peer, sizeof(p2p_prov_disc_resp->peer));
    }
}

static void wmi_p2p_event(event_t eventid, void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }
    qapi_WLAN_Callback_t wlan_cb = NULL;
    void *applicationContext = NULL;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_WLAN_P2P_Event_Cb_Info_t p2p_Event_Cb_Info = p_cxt->p2p_Event_Cb_Info;
    uint32_t len = sizeof(p2p_Event_Cb_Info);

    /* get the callback handler from the device context */
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    if(p_cxt->qapi_event_handler != NULL){
        /* call this later from outside the spinlock */
        wlan_cb = (qapi_WLAN_Callback_t )p_cxt->qapi_event_handler;
        applicationContext = p_cxt->event_application_Context;
    }

    /* call the callback function provided by application to
     * indicate last transmitted rate */
    if (wlan_cb != NULL) {
        /* Since event info is an union of multiple events, we can copy the
          payload onto any of the union members. We will always copy it to the
          first member of the union */
        memset(&p2p_Event_Cb_Info, 0, sizeof(p2p_Event_Cb_Info));
        p2p_Event_Cb_Info.event_ID = eventid;
        fill_p2p_event_info(&p2p_Event_Cb_Info, msg);
        p_cxt->qapi_event_handler(p_cxt->network_id, QAPI_WLAN_P2P_CB_E, applicationContext,
                                  &p2p_Event_Cb_Info, len);
    }
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
}

static void wmi_p2p_goneg_result_event(void *msg)
{
    WMI_P2P_GO_NEG_RESULT_EVENT *p2p_go_neg = (WMI_P2P_GO_NEG_RESULT_EVENT *)msg;

    /* This should be set by the firmware, for now setting it here */
    p2p_go_neg->passphrase_len = 8;

    wmi_p2p_event(WMI_P2P_GO_NEG_RESULT_EVENTID, msg);
}

static void wmi_p2p_node_list_event(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_WLAN_P2P_Node_List_Params_t *evt = &(p_cxt->get_p2p_nodelist);

    WMI_P2P_NODE_LIST_EVENT *handleP2PDev = (WMI_P2P_NODE_LIST_EVENT *)msg;
    if (!evt || !handleP2PDev || !evt->node_List_Buffer) {
        printf("Error: Null pointer input\n");
        return;
    }

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    if (!p_cxt) {
        printf("Invalid context or buffer\n");
        qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
        return;
    }
    // calculate the space
    uint32_t dev_count = handleP2PDev->num_p2p_dev;
    uint32_t dev_struct_size = sizeof(qapi_WLAN_P2P_Device_Lite_t);
    uint32_t required_size = dev_count * dev_struct_size + 1;

    if (evt->buffer_Length < required_size) {
        printf("Error: buffer too small for %zu devices\n", dev_count);
        qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
        return;
    }

    memset(evt->node_List_Buffer, 0, evt->buffer_Length);
    uint8_t *tmpBuf = evt->node_List_Buffer;

    *tmpBuf = handleP2PDev->num_p2p_dev;
    tmpBuf++;
    memcpy(tmpBuf, ((uint8_t *)(handleP2PDev->data)),
           (sizeof(qapi_WLAN_P2P_Device_Lite_t) * (handleP2PDev->num_p2p_dev)));

    if (!p_cxt->wlan_get_nodelist_block_mode) {
        qurt_signal_set(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_GET_NODELIST);
    }

    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
}

static void wmi_p2p_prov_disc_resp(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }
    wmi_p2p_event(WMI_P2P_PROV_DISC_RESP_EVENTID, msg);
}

static void wmi_p2p_req_auth_event(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    wmi_p2p_event(WMI_P2P_REQ_TO_AUTH_EVENTID, msg);
}

static void wmi_p2p_list_peristent_network(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    WMI_P2P_PERSISTENT_LIST_NETWORK_EVENT *ev = (WMI_P2P_PERSISTENT_LIST_NETWORK_EVENT *)msg;

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    if (p_cxt) {
        memset(p_cxt->pScanOut, 0, p_cxt->pScanOutSize);

        memcpy(p_cxt->pScanOut, (MAX_LIST_COUNT * sizeof(WMI_PERSISTENT_MAC_LIST)), ev->data);

        p_cxt->wlan_qapi_error = QAPI_ERROR;
    }

    qurt_signal_set(&p_cxt, WMI_P2P_LIST_PERSISTENT_NETWORK_EVENTID);
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
}

static void wmi_get_p2p_ctx(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    wmi_p2p_event(WMI_P2P_PROV_DISC_RESP_EVENTID, msg);
}

static void wmi_p2p_prov_disc_req(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    wmi_p2p_event(WMI_P2P_PROV_DISC_REQ_EVENTID, msg);
}

static void wmi_p2p_serv_disc_req(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    wmi_p2p_event(WMI_P2P_SDPD_RX_EVENTID, msg);
}

static void wmi_p2p_invite_req(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    wmi_p2p_event(WMI_P2P_INVITE_REQ_EVENTID, msg);
}

static void wmi_p2p_invite_rcvd_result(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    wmi_p2p_event(WMI_P2P_INVITE_RCVD_RESULT_EVENTID, msg);
}

static void wmi_p2p_invite_sent_result(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    wmi_p2p_event(WMI_P2P_INVITE_SENT_RESULT_EVENTID, msg);
}

static void wmi_p2p_sdpd_event(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }

    wmi_p2p_event(WMI_P2P_SDPD_RX_EVENTID, msg);
}

static void wmi_control_rx_p2p(event_t event_id, void *data)
{
    switch (event_id) {
    case WMI_P2P_LIST_PERSISTENT_NETWORK_EVENTID:
        wmi_p2p_list_peristent_network(data);
        break;
    case WMI_P2P_GO_NEG_RESULT_EVENTID:
        wmi_p2p_goneg_result_event(data);
        break;

    case WMI_P2P_NODE_LIST_EVENTID:
        wmi_p2p_node_list_event(data);
        break;

    case WMI_P2P_REQ_TO_AUTH_EVENTID:
        wmi_p2p_req_auth_event(data);
        break;

    case WMI_P2P_PROV_DISC_RESP_EVENTID:
        wmi_p2p_prov_disc_resp(data);
        break;

    case WMI_P2P_PROV_DISC_REQ_EVENTID:
        wmi_p2p_prov_disc_req(data);
        break;
    case WMI_P2P_INVITE_REQ_EVENTID:
        wmi_p2p_invite_req(data);
        break;

    case WMI_P2P_INVITE_RCVD_RESULT_EVENTID:
        wmi_p2p_invite_rcvd_result(data);
        break;

    case WMI_P2P_INVITE_SENT_RESULT_EVENTID:
        wmi_p2p_invite_sent_result(data);
        break;

    case WMI_P2P_SDPD_RX_EVENTID:
        wmi_p2p_sdpd_event(data);
        break;
    default:
        break;
    }
    return;
}
#endif

#ifdef CONFIG_WLAN_8021X
static void wmi_8021x_assoc_result_event(void *msg)
{
    if (!msg) {
        warn_printf("msg NULL\n");
        return;
    }
    PRINT_LOG_FUNC_LINE_ENTRY;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_WLAN_Connect_Cb_Info_t cxnInfo;
    int intf_id;
    
    memset(&cxnInfo, 0, sizeof(qapi_WLAN_Connect_Cb_Info_t));

    cxnInfo.value = true;
    memcpy(cxnInfo.mac_Addr, ((WMI_EAP_ASSOC_RESULT_MSG *)msg)->bssid, __QAPI_WLAN_MAC_LEN);

    wlan_8021x_event_cb(WLAN_SUPPLICANT_INTERFACE_ID, QAPI_WLAN_CONNECT_CB_E, g_wl8021x_global, (void *)&cxnInfo, sizeof(cxnInfo));
}
#endif

static void wmi_event_dispatch(event_t event_id, void *data)
{
    switch (event_id) {
    case WMI_DISCONNECT_EVTID:
        wmi_disconnect_event(data);
        break;
    case WMI_CONNECT_FAIL_EVTID:
    case WMI_CONNECT_SUCCESS_EVTID:
        wmi_join_comp_event(data);
        break;
    case WMI_WIFI_SET_MODE_EVTID:
        wmi_set_mode_event(data);
        break;
    case WMI_WIFI_EN_EVTID:
        wmi_enabled_event(data);
        break;
    case WMI_WIFI_DIS_EVTID:
        wmi_disabled_event(data);
        break;
    case WMI_NETIF_ADD_EVTID:
        wmi_if_added_event(data);
        break;
    case WMI_SCAN_COMP_EVTID:
        wmi_scan_comp_event(data);
        break;
    case WMI_SCAN_RESULT_EVTID:
        wmi_scan_result_event(data);
        break;
    case WMI_SCAN_START_EVTID:
        wmi_scan_started_event(data);
        break;
    case WMI_IP_DHCP_SUCCESS_EVTID:
        log_printf("DHCP operation successful\n");
        break;
    case WMI_IP_ADDR_READY_EVTID:
        wmi_ip_addr_ready_event(data);
        break;
    case WMI_SET_PARAM_EVENT_ID:
        wmi_set_param_event(data);
        break;
    case WMI_REPORT_STATISTICS_EVTID:
        wmi_report_stat_event(data);
        break;
    case WMI_REGULATORY_EVTID:
        wmi_regulatory_event(data);
        break;
    case WMI_SET_RATE_EVTID:
        wmi_set_rate_event(data);
        break;
    case WMI_GET_RATE_EVTID:
        wmi_get_rate_event(data);
        break;
    case WMI_CHAN_SWITCH_EVTID:
        wmi_chan_switch_event(data);
        break;
    case WMI_SEND_RAW_FRAME_EVTID:
        wmi_send_raw_event(data);
        break;
    case WMI_GET_TX_POWER_EVTID:
        wmi_get_tx_power_event(data);
        break;
    case WMI_MGMT_FRAME_FILTER_EVTID:
        wmi_set_mgmt_filter_event(data);
        break;
#ifdef CONFIG_WPS
    case WMI_SCAN_STOP_EVTID:
        wmi_stop_scan_event(data);
        break;
    case WMI_WPS_FAIL_EVTID:
        wmi_wps_fail_event(data);
        break;
#endif
    case WMI_BMPS_GET_STATS_EVENTID:
        wmi_get_bmps_stats_event(data);
        break;
#ifdef CONFIG_WLAN_8021X
    case WMI_8021X_ASSOC_RESULT_EVTID:
        wmi_8021x_assoc_result_event(data);
        break;
#endif

#if CONFIG_ENABLE_P2P_MODE
    case WMI_P2P_LIST_PERSISTENT_NETWORK_EVENTID:
    case WMI_P2P_GO_NEG_RESULT_EVENTID:
    case WMI_P2P_NODE_LIST_EVENTID:
    case WMI_P2P_REQ_TO_AUTH_EVENTID:
    case WMI_P2P_PROV_DISC_RESP_EVENTID:
    case WMI_P2P_PROV_DISC_REQ_EVENTID:
    case WMI_P2P_INVITE_REQ_EVENTID:
    case WMI_P2P_INVITE_RCVD_RESULT_EVENTID:
    case WMI_P2P_INVITE_SENT_RESULT_EVENTID:
    case WMI_P2P_SDPD_RX_EVENTID:
        wmi_control_rx_p2p(event_id, data);
        break;
#endif
    default:
        break;
    }

    return;
}

static void wmi_event_buf_free(void *data)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    wlan_evt_payload_t *event_payload;
    int16_t i, j;
    qbool_t is_match = FALSE;

    for (i = 0; i < EVT_PAYLOAD_MAX; i++) {
        event_payload = &(p_cxt->event_payload_buf[i]);
        for (j = 0; j < event_payload->buf_num; j++) {
            if (data == (event_payload->buf + j * event_payload->buf_length)) {
                is_match = TRUE;
                break;
            }
        }
        if (is_match) {
            break;
        }
    }

    if (is_match) {
        qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
        event_payload->buf_used--;
        assert(event_payload->buf_used >= 0);
        qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    }
}

static void wmi_cmd_result(void *msg)
{
    wmi_msg_struct_t *wmi_msg = (wmi_msg_struct_t *)msg;
    if (!wmi_msg) {
        PRINT_ERR_INVALID_PARAM;
        return;
    }

    void *data = ((wmi_msg_struct_t *)wmi_msg)->msg_struct.vo_data;
    event_t event_id = (event_t)(((wmi_msg_struct_t *)wmi_msg)->msg_struct.id);
    if (event_id == 0) {
        // default response, ignore
        qapi_Status_t ret;

        if (wmi_msg->msg_struct.return_status == eWiFiSuccess) //
        {
            ret = QAPI_OK;
        } else {
            ret = QAPI_ERROR;
        }

        set_wlan_qapi_error(ret);

        if (data != NULL && wmi_msg->trans_wmi_message_id == WMI_GET_RETURN_STATUS_CMDID) {
            wmi_event_buf_free(data);
        }

        return;
    }
    log_printf("msg WMI cmd_id=%d return_status=%d event_id=%d\n", wmi_msg->trans_wmi_message_id,
               wmi_msg->msg_struct.return_status, wmi_msg->msg_struct.id);
    wmi_event_dispatch(event_id, data);

    if (data != NULL && wmi_msg->trans_wmi_message_id == WMI_GET_RETURN_STATUS_CMDID) {
        wmi_event_buf_free(data);
    }

    return;
}

static void wmi_event_notify(WIFIReturnCode_t return_type, event_t event_id, void *data)
{
    log_printf("wlan_qapi_event: return_type=%d event_id=%d data=0x%x %d\n", return_type, event_id, data, *(int *)data);

    wmi_msg_struct_t wlan_result = {0};
    if (event_id >= invalid_app_event_id || event_id < aws_app_event_id) {
        PRINT_ERR_INVALID_PARAM1("event_id", event_id);
        return;
    }
    wlan_result.msg_struct.result_function = &wmi_cmd_result;
    wlan_result.msg_struct.return_status = return_type;
    wlan_result.msg_struct.id = event_id;
    wlan_result.msg_struct.vo_data = data;
    wlan_result.trans_wmi_message_id = WMI_GET_RETURN_STATUS_CMDID;
#if 0
    //this way is used by hostif, will call WMI_xxx->WMI_GET_RETURN_STATUS_CMDID's return(actual eid)->WMI_xxx's return(eid=0).
    //Has a mixed sequence, but can work on data allocated in stack
    wmi_qapi_response_handler((void*)&wlan_result);
#else
    // this way is used by nt_wfm, will call  WMI_xxx and its return(eid=0)->WMI_GET_RETURN_STATUS_CMDID and its
    // return(actual eid). THis has a better sequence, but cannot work on data allocated in stack The data stack
    // limitation now is fixed by relay on p_cxt->event_payload_buf
    qurt_pipe_send(msg_wfm_wmi_id, (void *)&wlan_result);
#endif
}

void wmi_event_relay(uint32_t if_id, event_t event_id, void *data, uint32_t data_length,
                     void __attribute__((__unused__)) * cxt)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    uint8_t *dst;
    wlan_evt_payload_t *event_payload;

    (void)if_id;
    if (data_length > p_cxt->event_payload_buf[EVT_LARGE_PAYLOAD].buf_length) {
        PRINT_ERR_INVALID_PARAM1("data_length", data_length);
        return;
    }
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);

    if (data_length > p_cxt->event_payload_buf[EVT_SMALL_PAYLOAD].buf_length) {
        event_payload = &(p_cxt->event_payload_buf[EVT_LARGE_PAYLOAD]);
    } else {
        event_payload = &(p_cxt->event_payload_buf[EVT_SMALL_PAYLOAD]);
    }
    if (event_payload->buf_used >= event_payload->buf_num) {
        err_printf("No free event payload buf");
        qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
        return;
    }

    dst = (event_payload->buf + event_payload->buf_write_pointer * event_payload->buf_length);
    memscpy(dst, data_length, data, data_length);
    event_payload->buf_write_pointer = ((event_payload->buf_write_pointer + 1) % event_payload->buf_num);
    event_payload->buf_used++;

    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    wmi_event_notify(eWiFiSuccess, event_id, dst);
}

qapi_Status_t wmi_cmd_send(WMI_COMMAND_ID cmd_id, void *p_data, uint32_t data_len)
{
    wmi_msg_struct_t wmi_msg = {0};

    wmi_msg.trans_wmi_message_id = cmd_id;
    wmi_msg.msg_struct.vo_data = p_data;
    wmi_msg.msg_struct.vo_data_len = data_len;
    wmi_msg.msg_struct.return_status = eWiFiNotSupported;
    wmi_msg.msg_struct.result_function = &wmi_cmd_result;
    wmi_msg.msg_struct.event_notify = &wmi_event_notify;
    if (cmd_id == WMI_WLAN_ON_CMDID || cmd_id == WMI_WLAN_OFF_CMDID) {
        wmi_msg.prot_flg = cmd_id;
    }
    log_printf("send WMI cmd=%d\n", wmi_msg.trans_wmi_message_id);
    qurt_pipe_send(msg_wfm_wmi_id, (void *)&wmi_msg);
    return QAPI_OK;
}

qapi_Status_t wmi_dev_cmd_send(WMI_COMMAND_ID cmd_id, uint8_t dev_id, void *p_data, uint32_t data_len)
{
    wmi_msg_struct_t wmi_msg = {0};

    wmi_msg.trans_wmi_message_id = cmd_id;
    wmi_msg.msg_struct.vo_data = p_data;
    wmi_msg.msg_struct.vo_data_len = data_len;
    wmi_msg.msg_struct.return_status = eWiFiNotSupported;
    wmi_msg.msg_struct.result_function = &wmi_cmd_result;
    wmi_msg.msg_struct.event_notify = &wmi_event_notify;
    if (cmd_id == WMI_WLAN_ON_CMDID || cmd_id == WMI_WLAN_OFF_CMDID) {
        wmi_msg.prot_flg = cmd_id;
    }
#if defined(SUPPORT_RING_IF) || defined(CONFIG_WMI_EVENT)
    wmi_msg.msg_struct.netif_id = dev_id;
#endif
    log_printf("send WMI cmd=%d\n", wmi_msg.trans_wmi_message_id);
    qurt_pipe_send(msg_wfm_wmi_id, (void *)&wmi_msg);
    return QAPI_OK;
}

qapi_Status_t wmi_on(void)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    wmi_cmd_send(WMI_WLAN_ON_CMDID, NULL, 0);
    if (p_cxt->wlan_enable_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_ENABLE_DONE, QURT_SIGNAL_ATTR_CLEAR_MASK);
        log_printf("block mode, WMI cmd done\n");
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    ret = get_wlan_qapi_error();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    return ret;
}

qapi_Status_t wmi_off(void)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    wmi_cmd_send(WMI_WLAN_OFF_CMDID, NULL, 0);
    if (p_cxt->wlan_disable_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_DISABLE_DONE, QURT_SIGNAL_ATTR_CLEAR_MASK);
        log_printf("block mode, WMI cmd done\n");
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    ret = get_wlan_qapi_error();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    if (p_cxt->opmode == WHAL_M_STA)
        wlan_drv_roaming_stop();

    /* when disable wlan, clear some setting */
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    p_cxt->qapi_event_handler = NULL;
    p_cxt->opmode = WHAL_M_AP;
    p_cxt->conc_mode = WHAL_M_NO_CONC;
    clr_wlan_qapi_error();
    wlan_clear_privacy();
    wlan_preset_specific_param();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    return ret;
}

qapi_Status_t wmi_add_device(uint8_t __attribute__((__unused__)) device_ID)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    // wmi_cmd_send(WMI_SET_MODE_CMDID, &p_cxt->connect_cmd, sizeof(WMI_IF_ADD_CMD));
    wmi_cmd_send(WMI_SET_MODE_CMDID, &p_cxt->connect_cmd, sizeof(WMI_CONNECT_CMD));
    if (p_cxt->wlan_if_add_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_IF_ADDED, QURT_SIGNAL_ATTR_CLEAR_MASK);
        log_printf("block mode, WMI cmd done\n");
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    ret = get_wlan_qapi_error();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    return ret;
}

qapi_Status_t wmi_start_scan(uint8_t __attribute__((__unused__)) device_ID,
                             const qapi_WLAN_Start_Scan_Params_t *scan_Params)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    PRINT_LOG_FUNC_LINE_ENTRY;
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    p_cxt->scan_in_progress = true;
    memset(p_cxt->pScanOut, 0, p_cxt->pScanOutSize);
    wlan_set_scan_param(&p_cxt->scan_cmd, scan_Params);
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);

    wmi_cmd_send(WMI_START_SCAN_CMDID, &p_cxt->scan_cmd, sizeof(WMI_START_SCAN_CMD));
    if (p_cxt->wlan_scan_start_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_STARTED_SCAN, QURT_SIGNAL_ATTR_CLEAR_MASK);
        log_printf("block mode, WMI cmd done\n");
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    ret = get_wlan_qapi_error();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    PRINT_LOG_FUNC_LINE_EXIT;
    return ret;
}

qapi_Status_t wlan_get_scan_results(uint8_t __attribute__((__unused__)) device_ID, qapi_WLAN_Scan_Comp_Evt_t *scan_Res,
                                    int16_t *num_Bss)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    PRINT_LOG_FUNC_LINE_ENTRY;
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    if (p_cxt->scan_in_progress) {
        p_cxt->wait_scan_comp_evt = true;
        qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
        log_printf("wait scan results\n");
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SCAN_COMPLETED, QURT_SIGNAL_ATTR_CLEAR_MASK);
        log_printf("got scan results\n");
        qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    }
    int16_t num_Bss_capacity = *num_Bss;
    qapi_WLAN_Scan_Comp_Evt_t *scan_comp_evt = (qapi_WLAN_Scan_Comp_Evt_t *)p_cxt->pScanOut;
    int16_t num_Bss_real = scan_comp_evt->num_bss_cur;
    if (num_Bss_real > num_Bss_capacity) {
        num_Bss_real = num_Bss_capacity;
    }
    log_printf("copy scan results\n");
    memscpy(scan_Res, sizeof(qapi_WLAN_Scan_Comp_Evt_t) + sizeof(qapi_WLAN_BSS_Scan_Info_t) * num_Bss_real,
            scan_comp_evt, sizeof(qapi_WLAN_Scan_Comp_Evt_t) + sizeof(qapi_WLAN_BSS_Scan_Info_t) * num_Bss_real);
    ret = get_wlan_qapi_error();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    *num_Bss = num_Bss_real;
    PRINT_LOG_FUNC_LINE_EXIT;
    return ret;
}

qapi_Status_t wmi_set_passphrase(void)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    if (p_cxt->connect_cmd.networkType == AP_NETWORK)
        wmi_cmd_send(WMI_CONFIG_AP_CMDID, &p_cxt->passphrase_cmd, sizeof(WMI_SET_PASSPHRASE_CMD));
    else
        wmi_cmd_send(WMI_SET_PASSPHRASE_CMDID, &p_cxt->passphrase_cmd, sizeof(WMI_SET_PASSPHRASE_CMD));
    log_printf("No specific event for WMI_SET_PASSPHRASE_CMDID, just go\n");
    return QAPI_OK;
}

qapi_Status_t wmi_connect(void)
{
    qapi_Status_t ret = QAPI_OK;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    WMI_COMMAND_ID cmd_id = WMI_AP_CONFIG_COMMIT_CMDID;
    if (p_cxt->connect_cmd.networkType != AP_NETWORK)
        cmd_id = WMI_CONNECT_CMDID;

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    memset(&p_cxt->connect_result, 0, sizeof(qapi_WLAN_Join_Comp_Evt_t));
    p_cxt->connect_in_progress = true;
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);

    wmi_cmd_send(cmd_id, &p_cxt->connect_cmd, sizeof(WMI_CONNECT_CMD));
    if (p_cxt->wlan_connect_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done,
                         WLAN_WMI_CMD_SIG_MASK_CONNECT_COMPLETED | WLAN_WMI_CMD_SIG_MASK_DISCONNECTED,
                         QURT_SIGNAL_ATTR_CLEAR_MASK | QURT_SIGNAL_ATTR_WAIT_ANY);
        log_printf("block mode, WMI cmd done\n");
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }

    if (p_cxt->wlan_connect_block_mode) {
        qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
        ret = get_wlan_qapi_error();
        qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    }
    return ret;
}

qapi_Status_t wmi_disconnect(void)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    p_cxt->disconnect_in_progress = true;
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);

    if (p_cxt->opmode == WHAL_M_AP) {
        qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
        p_cxt->discon_cmd.sta_id = -1;
        qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
        wmi_cmd_send(WMI_DISCONNECT_CMDID, &p_cxt->discon_cmd, sizeof(WLAN_WMI_DISCONN_t));
    } else
        wmi_cmd_send(WMI_DISCONNECT_CMDID, NULL, 0);
    if (p_cxt->wlan_disconnect_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_DISCONNECTED, QURT_SIGNAL_ATTR_CLEAR_MASK);
        log_printf("wmi_disconnect: block mode, WMI cmd done\n");
    } else {
        log_printf("wmi_disconnect: unblock mode, should check WMI cmd done in event cb\n");
    }

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    ret = get_wlan_qapi_error();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    if (p_cxt->opmode == WHAL_M_STA)
        wlan_drv_roaming_stop();
    return ret;
}

qapi_Status_t wmi_set_op_mode(void)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    wmi_cmd_send(WMI_SET_MODE_CMDID, &p_cxt->connect_cmd, sizeof(WMI_CONNECT_CMD));
    if (p_cxt->wlan_if_add_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_MODE, QURT_SIGNAL_ATTR_CLEAR_MASK);
        log_printf("block mode, WMI cmd done\n");
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    ret = get_wlan_qapi_error();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    return ret;
}

qapi_Status_t wmi_wlan_get_statistics(uint8_t device_ID)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    wmi_dev_cmd_send(WMI_GET_STATISTICS_CMDID, device_ID, NULL, 0);
    if (p_cxt->wlan_get_stat_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_GET_STAT, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }

    ret = get_wlan_qapi_error();
    return ret;
}

qapi_Status_t wmi_wlan_get_regulatory(void)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    wmi_cmd_send(WMI_GET_REGULATORY_CMDID, NULL, 0);
    if (p_cxt->wlan_get_regulatory_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_GET_REG, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }

    ret = get_wlan_qapi_error();
    return ret;
}

qapi_Status_t wmi_set_rate(void)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    wmi_cmd_send(WMI_SET_RATE, (void *)(&(p_cxt->rate_param)), sizeof(qapi_WLAN_Set_Rate_Params_t));
    if (p_cxt->wlan_set_rate_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_RATE, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }

    ret = get_wlan_qapi_error();
    return ret;
}

qapi_Status_t wmi_get_rate(void)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    wmi_cmd_send(WMI_GET_RATE, (void *)(&(p_cxt->rate_param)), sizeof(qapi_WLAN_Set_Rate_Params_t));
    if (p_cxt->wlan_set_rate_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_GET_RATE, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }

    ret = get_wlan_qapi_error();
    return ret;
}

qapi_Status_t  wmi_get_bmps_bwindow_wait_close_time_stats(void)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    wmi_cmd_send(WMI_BMPS_GET_BWINDOW_WAIT_CLOSE_TIME_CMDID, (void *)(p_cxt->cmd_bmps_stats.bwindow_wait_close_time), sizeof(pm_stats_active_sleep_time_record_buffer_t));
    if (p_cxt->wlan_get_bmps_stats_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_GET_BMPS_STATS, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }

    ret = get_wlan_qapi_error();
    return ret;
}

qapi_Status_t  wmi_get_bmps_soc_active_sleep_time_stats(void)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    wmi_cmd_send(WMI_BMPS_GET_SOC_ACTIVE_SLEEP_TIME_CMDID, (void *)(p_cxt->cmd_bmps_stats.soc_active_sleep_time), sizeof(pm_stats_active_sleep_time_record_buffer_t));
    if (p_cxt->wlan_get_bmps_stats_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_GET_BMPS_STATS, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }

    ret = get_wlan_qapi_error();
    return ret;
}

qapi_Status_t  wmi_get_bmps_tx_rx_counts(void)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    wmi_cmd_send(WMI_BMPS_GET_TXRX_COUNTS_CMDID, (void *)(p_cxt->cmd_bmps_stats.tx_rx_counts), sizeof(pm_stats_tx_rx_counts_record_buffer_t));
    if (p_cxt->wlan_get_bmps_stats_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_GET_BMPS_STATS, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }

    ret = get_wlan_qapi_error();
    return ret;
}

qapi_Status_t wmi_send_raw()
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    wmi_cmd_send(WMI_SEND_RAW, (void *)(&(p_cxt->raw_pkt_frame)), sizeof(SEND_RAW_FRAME));
    if (p_cxt->wlan_send_raw_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SEND_RAW, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }

    ret = get_wlan_qapi_error();
    return ret;
}

qapi_Status_t wmi_set_mgmt_filter(void)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t ret = QAPI_OK;

    wmi_cmd_send(WMI_SET_MGMT_FILTER_CMDID, (void *)(&(p_cxt->mgmt_filter)), sizeof(WMI_MGMT_FRAME_FILTER));
    if (p_cxt->wlan_set_mgmt_filter_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_MGMT_FILTER, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }

    ret = get_wlan_qapi_error();
    return ret;
}

qapi_Status_t wmi_get_tx_power(void)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    wmi_cmd_send(WMI_GET_TX_POWER_CMDID, (void *)(&(p_cxt->get_tx_power_result)), sizeof(qapi_WLAN_Get_Power_Evt_t));
    if (p_cxt->wlan_get_tx_power_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_GET_TX_POWER, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }

    ret = get_wlan_qapi_error();
    return ret;
}

#ifdef CONFIG_WPS
extern qapi_WLAN_WPS_Credentials_t gWpsCredentials;

qapi_Status_t wmi_start_wps_process(uint8_t __attribute__((__unused__)) device_ID,
                                    qapi_WLAN_WPS_Connect_Action_e connect_Action, qapi_WLAN_WPS_Mode_e mode,
                                    const char *pin, uint8_t auth_floor)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t ret = QAPI_OK;

    WMI_COMMAND_ID cmd_id = WMI_WPS_START_CMDID;

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    memset(&p_cxt->wps_param, 0, sizeof(WMI_WPS_START_CMD));
    p_cxt->wps_in_progress = true;
    p_cxt->wps_param.config_methods = WPS_EN_INT;
    p_cxt->wps_param.auth_floor = auth_floor;
    if (mode == QAPI_WLAN_WPS_PBC_MODE_E)
        p_cxt->wps_param.config_mode = WPS_PBC_MODE;
    else if (mode == QAPI_WLAN_WPS_PIN_MODE_E)
        p_cxt->wps_param.config_mode = WPS_PIN_MODE;
    else
        return QAPI_WLAN_ERROR;

    p_cxt->wps_param.ctl_flag = connect_Action;
    if (gWpsCredentials.ssid_Length == 0) {
        p_cxt->wps_param.ssid_info.ssid_len = 0;
    } else {
        memscpy(p_cxt->wps_param.ssid_info.ssid, sizeof(p_cxt->wps_param.ssid_info.ssid), gWpsCredentials.ssid,
                sizeof(p_cxt->wps_param.ssid_info.ssid));
        memscpy(p_cxt->wps_param.ssid_info.macaddress, sizeof(p_cxt->wps_param.ssid_info.macaddress),
                gWpsCredentials.mac_Addr, __QAPI_WLAN_MAC_LEN);
        p_cxt->wps_param.ssid_info.channel = gWpsCredentials.ap_Channel;
        p_cxt->wps_param.ssid_info.ssid_len = gWpsCredentials.ssid_Length;
    }

    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);

    wmi_cmd_send(cmd_id, &p_cxt->wps_param, sizeof(WMI_WPS_START_CMD));

    if (p_cxt->wlan_start_wps_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_STARTED_WPS_PROCESS, QURT_SIGNAL_ATTR_CLEAR_MASK);
        log_printf("block mode, WMI cmd done\n");
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }

    if (p_cxt->wlan_start_wps_block_mode) {
        qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
        ret = get_wlan_qapi_error();
        qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    }

    return ret;
}

qapi_Status_t wmi_stop_scan(void)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    p_cxt->stop_scan_in_progress = true;
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);

    if (p_cxt->opmode == WHAL_M_AP)
        return ret;
    else
        wmi_cmd_send(WMI_SCAN_STOP_CMDID, NULL, 0);
    if (p_cxt->wlan_scan_stop_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_STOPPED_SCAN, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("wmi_stop_scan: unblock mode, should check WMI cmd done in event cb\n");
    }
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    ret = get_wlan_qapi_error();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    return ret;
}
#endif

#ifdef CONFIG_ENABLE_P2P_MODE
qapi_Status_t wmi_p2p_get_node_list(void)
{
    qapi_Status_t error = QAPI_ERROR;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    /* Already a blocking P2P command is pending response. This has to wait till we get response for previous command */
    wmi_cmd_send(WMI_P2P_GET_NODE_LIST_CMDID, NULL, 0);

    if (!p_cxt->wlan_get_nodelist_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_GET_NODELIST, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    error = get_wlan_qapi_error();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);

    return error;
}
#endif
