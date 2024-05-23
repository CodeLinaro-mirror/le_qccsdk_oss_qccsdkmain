/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "wlan_drv.h"
#include "wlan_qapi_helper.h"
#include "wmi_api.h"
#include "safeAPI.h"


#ifdef SUPPORT_5GHZ
/*11 for 2G and 33 for 5G*/
#define SCAN_LIST_NUM_CHANNELS 44
#else
/*11 for 2G*/
#define SCAN_LIST_NUM_CHANNELS 11
#endif

/* Should be called under protection of p_cxt->wlan_qapi_cxt_mutex */
void wlan_clear_privacy(void)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    WMI_SET_PASSPHRASE_CMD *p_passphrase_cmd = &p_cxt->passphrase_cmd;
    WMI_CONNECT_CMD *p_connect_cmd = &p_cxt->connect_cmd;

    p_connect_cmd->dot11AuthMode = OPEN_AUTH;
    p_connect_cmd->authMode = WMI_NONE_AUTH;
    p_connect_cmd->pairwiseCryptoType = NONE_CRYPT;
    p_connect_cmd->groupCryptoType = NONE_CRYPT;
    p_connect_cmd->pairwiseCryptoLen = 0;
    p_connect_cmd->groupCryptoLen = 0;

    memset(p_passphrase_cmd->passphrase, 0, WMI_PASSPHRASE_LEN+1);
    p_passphrase_cmd->passphrase_len = 0;
}

/* Should be called under protection of p_cxt->wlan_qapi_cxt_mutex */
void wlan_set_connect_ssid (const unsigned char *ssid, uint8_t ssidLength)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    WMI_SET_PASSPHRASE_CMD *p_passphrase_cmd = &p_cxt->passphrase_cmd;
    WMI_CONNECT_CMD *p_connect_cmd = &p_cxt->connect_cmd;

    if (!ssid || !ssidLength) {
        info_printf("clear WMI_CONNECT_CMD ssid\n");
        memset(p_connect_cmd->ssid, 0, WMI_MAX_SSID_LEN + 1);
        p_connect_cmd->ssidLength = 0;
        memset(p_passphrase_cmd->ssid, 0, WMI_MAX_SSID_LEN + 1);
        p_passphrase_cmd->ssid_len = 0;
    } else if (ssidLength <= WMI_MAX_SSID_LEN) {
        memscpy(p_connect_cmd->ssid, ssidLength, ssid, ssidLength);
        p_connect_cmd->ssidLength = ssidLength;
        info_printf("set WMI_CONNECT_CMD ssid=%s\n", p_connect_cmd->ssid);
        memscpy(p_passphrase_cmd->ssid, ssidLength, ssid, ssidLength);
        p_passphrase_cmd->ssid_len = ssidLength;
    }
}

/* Should be called under protection of p_cxt->wlan_qapi_cxt_mutex */
void wlan_set_connect_bssid (const uint8_t *bssid, uint8_t bssid_length)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    WMI_CONNECT_CMD *p_cmd = &p_cxt->connect_cmd;

    if (!bssid || !bssid_length) {
        info_printf("clear WMI_CONNECT_CMD bssid\n");
        memset(p_cmd->bssid, 0, __QAPI_WLAN_MAC_LEN);
    } else if (bssid_length == IEEE80211_ADDR_LEN) {
        memscpy(p_cmd->bssid, bssid_length, bssid, bssid_length);
        info_printf("set WMI_CONNECT_CMD bssid=%02x:%02x:%02x:%02x:%02x:%02x\n",
            p_cmd->bssid[0], p_cmd->bssid[1], p_cmd->bssid[2], p_cmd->bssid[3], p_cmd->bssid[4], p_cmd->bssid[5]);
    }
}

/* Should be called under protection of p_cxt->wlan_qapi_cxt_mutex */
void wlan_set_passphrase (const uint8_t *passphrase, uint8_t passphrase_len)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    WMI_SET_PASSPHRASE_CMD *p_passphrase_cmd = &p_cxt->passphrase_cmd;
    WMI_CONNECT_CMD *p_connect_cmd = &p_cxt->connect_cmd;

    if (!passphrase || !passphrase_len) {
        wlan_clear_privacy();
        info_printf("clear passphrase\n");
    } else if (passphrase_len <= __QAPI_WLAN_PASSPHRASE_LEN) {
        info_printf("set passphrase=%s\n", passphrase);
        memscpy(p_passphrase_cmd->passphrase, passphrase_len, passphrase, passphrase_len);
        p_passphrase_cmd->passphrase_len = passphrase_len;
        p_connect_cmd->pairwiseCryptoLen = passphrase_len;
        p_connect_cmd->groupCryptoLen = passphrase_len;
    }
}

/* Should be called under protection of p_cxt->wlan_qapi_cxt_mutex */
void wlan_set_scan_param (WMI_START_SCAN_CMD *p_cmd, const qapi_WLAN_Start_Scan_Params_t *scan_Params)
{
    memset(p_cmd, 0, sizeof(WMI_START_SCAN_CMD));
    if (!scan_Params) {
        p_cmd->scan_type = any_profile;
        p_cmd->cnt_prof = 0;
    } else {
        p_cmd->scan_type = specific_ssid;
        p_cmd->cnt_prof = 1;
        p_cmd->ssid[0].ssid_len = scan_Params->ssid_Length;
        memscpy(p_cmd->ssid[0].ssid, scan_Params->ssid_Length, scan_Params->ssid, scan_Params->ssid_Length);
    }
    p_cmd->auth_mode = WMI_NONE_AUTH;
    p_cmd->crypto_type = NONE_CRYPT;
    p_cmd->probe_type = active_probe;
	p_cmd->num_channels = SCAN_LIST_NUM_CHANNELS;
    int i;
    for (i=0; i<p_cmd->num_channels; i++) {
        p_cmd->channel_list[i] = i;
    }
    p_cmd->scan_only = true;
}

//ToDo: should be set but not hard code
void wlan_preset_specific_param (void)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    WMI_CONNECT_CMD *p_connect_cmd = &p_cxt->connect_cmd;

	if(p_cxt->opmode == WHAL_M_AP)
		p_connect_cmd->networkType = AP_NETWORK;
	else
		p_connect_cmd->networkType = INFRA_NETWORK;
    p_connect_cmd->num_channels = 0;
    p_connect_cmd->wlan_mode = MODE_11ABGN_HT20;
}

int32_t wlan_channel_to_freq(uint16_t *channel)
{
    if (NULL == channel)
    {
        return -1;
    }
    if(*channel < 1 || *channel > 173)
    {
      return -1;
    }
    if (*channel < 27) {
		if(*channel == 14)
			*channel = __QAPI_WLAN_CHAN_FREQ_14;
		else
			*channel = __QAPI_WLAN_CHAN_FREQ_1 + ((*channel - 1) * 5);
    } else {
        *channel = (5000 + (*channel * 5));
    }
    return 0;
}

int32_t wlan_freq_to_channel(uint16_t *channel)
{
    if (NULL == channel)
    {
        return -1;
    }
    if(*channel < 3000)
    {
        *channel -= __QAPI_WLAN_CHAN_FREQ_1;
        if((*channel / 5) == 14)
        {
            *channel = 14;
        }
        else
        {
            *channel = (*channel / 5) + 1;
        }
    }
    else
    {
        *channel -= __QAPI_WLAN_CHAN_FREQ_36;
        *channel = 36 + (*channel / 5); // since in 11a channel 36 is the starting number
    }
    return 0;
}

qapi_Status_t wlan_set_channel(uint8_t device_id, uint16_t channel)
{
	qapi_Status_t error = QAPI_OK;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
	WMI_SET_PDEV_PARAM_CMD *cmd = &p_cxt->dev_param_cmd;

    if (0 != wlan_channel_to_freq(&channel))
    {
        return QAPI_ERROR;
    }

	memset(cmd, 0, sizeof(WMI_SET_PDEV_PARAM_CMD));
	cmd->pdev_param_id = WIFI_PARAM_SET_PDEV_CHANNEL;
	cmd->pdev_param_value = (uint32_t)channel;

	wmi_dev_cmd_send(WMI_SET_PDEV_PARAM_CMDID, device_id, cmd, sizeof(WMI_SET_PDEV_PARAM_CMD));
	
	if(p_cxt->wlan_set_param_block_mode) {
		p_cxt->param_id = WIFI_PARAM_SET_PDEV_CHANNEL;
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_PARAM, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }
	error = get_wlan_qapi_error();
	return error;
}

qapi_Status_t wlan_set_country_code(uint8_t device_id, uint8_t *country_code)
{
	qapi_Status_t error = QAPI_OK;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
	WMI_SET_PDEV_PARAM_CMD *cmd = &p_cxt->dev_param_cmd;
	
	if(country_code == NULL)
		return QAPI_ERROR;
		
	memset(cmd, 0, sizeof(WMI_SET_PDEV_PARAM_CMD));
	cmd->pdev_param_id = WIFI_PARAM_SET_PDEV_COUNTRY_CODE;
	cmd->pdev_param_value = country_code[0] | country_code[1] << 8 | country_code[2] << 16;

	wmi_dev_cmd_send(WMI_SET_PDEV_PARAM_CMDID, device_id, cmd, sizeof(WMI_SET_PDEV_PARAM_CMD));

	if(p_cxt->wlan_set_param_block_mode) {
		p_cxt->param_id = WIFI_PARAM_SET_PDEV_COUNTRY_CODE;
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_PARAM, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }
	error = get_wlan_qapi_error();
	return error;
}

qapi_Status_t wlan_set_phy_mode(uint8_t device_id, uint32_t phy_mode)
{
	qapi_Status_t error = QAPI_OK;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
	WMI_SET_PDEV_PARAM_CMD *cmd = &p_cxt->dev_param_cmd;
		
	memset(cmd, 0, sizeof(WMI_SET_PDEV_PARAM_CMD));
	cmd->pdev_param_id = WIFI_PARAM_SET_PHYMODE;
	cmd->pdev_param_value = phy_mode;

	wmi_dev_cmd_send(WMI_SET_PDEV_PARAM_CMDID, device_id, cmd, sizeof(WMI_SET_PDEV_PARAM_CMD));
	
	if(p_cxt->wlan_set_param_block_mode) {
		p_cxt->param_id = WIFI_PARAM_SET_PHYMODE;
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_PARAM, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }
	error = get_wlan_qapi_error();
	return error;
}

int32_t wlan_set_11n_ht(uint8_t  __attribute__((__unused__)) device_id, uint8_t htconfig)
{
	int32_t error = QAPI_OK;
	WMI_SET_HT_CAP_CMD *cmd;
	cmd = malloc(sizeof(WMI_SET_HT_CAP_CMD));
	if(cmd == NULL)
		return QAPI_ERROR;

	memset(cmd, 0, sizeof(WMI_SET_HT_CAP_CMD));
    do {
        if (QAPI_WLAN_11N_DISABLED_E != htconfig) {
            cmd->enable = 1;
            cmd->short_GI_20MHz = 1;
            cmd->max_ampdu_len_exp = 2;
            if (QAPI_WLAN_11N_HT40_E == htconfig) {
                cmd->chan_width_40M_supported = 1;
                cmd->short_GI_40MHz    = 1;
                cmd->intolerance_40MHz = 0;
            }
        }

        if(QAPI_OK != wmi_cmd_send(WMI_SET_HT_CAP_CMDID, cmd, sizeof(WMI_SET_HT_CAP_CMD))) {
            error = QAPI_ERROR;
            break;
        }
    } while (0);

	free(cmd);
	return error;
}

qapi_Status_t wlan_set_op_mode(uint8_t mode)
{
	qapi_Status_t status = QAPI_OK;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    WMI_CONNECT_CMD *p_connect_cmd = &p_cxt->connect_cmd;

	if(((p_cxt->opmode == WHAL_M_AP) && (mode == DEV_MODE_AP_E))
		|| ((p_cxt->opmode == WHAL_M_STA) && (mode == DEV_MODE_STATION_E))
		#ifdef NT_FN_CONCURRENCY
		|| ((p_cxt->conc_mode == WHAL_M_AP_STA) && (mode == DEV_MODE_AP_STA_E))
		#endif
		)
		return status;

	qapi_WLAN_Disconnect(0);

	p_connect_cmd->networkType = mode;
	status = wmi_set_op_mode();
	if(status == QAPI_OK) {
		qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
		#ifdef NT_FN_CONCURRENCY
		if(mode == DEV_MODE_AP_STA_E) {
			p_cxt->conc_mode = WHAL_M_AP_STA;
		}
		else {
			p_cxt->conc_mode = WHAL_M_NO_CONC;
		}
		#endif
		if(mode == DEV_MODE_AP_E)
			p_cxt->opmode = WHAL_M_AP;
		else if(mode == DEV_MODE_STATION_E)
			p_cxt->opmode = WHAL_M_STA;
		qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
	}
	return status;
}

qapi_Status_t wlan_get_mac_address(uint8_t __attribute__((__unused__)) device_ID, uint8_t mac_addr[__QAPI_WLAN_MAC_LEN])
{
	extern devh_t *gdevp;
	memscpy(mac_addr, __QAPI_WLAN_MAC_LEN, gdevp->ic_myaddr, __QAPI_WLAN_MAC_LEN);
	return QAPI_OK;
}

qapi_Status_t wlan_get_power_mode(uint8_t __attribute__((__unused__)) device_ID, uint8_t *powermode)
{
	extern devh_t *gdevp;
	extern uint8_t get_currently_enabled_powersave(devh_t *dev);
	if(powermode == NULL)
		return QAPI_ERROR;
	*powermode = get_currently_enabled_powersave(gdevp);
	return QAPI_OK;
}

qapi_Status_t wlan_get_phy_mode(uint8_t *phymode)
{
	extern devh_t *gdevp;
	if(phymode == NULL)
		return QAPI_ERROR;
	*phymode = (uint8_t)(gdevp->pDevCmn->ic_phymode);
	return QAPI_OK;
}

qapi_Status_t wlan_sta_get_rssi(uint8_t device_ID, uint8_t *rssi)
{
	qapi_Status_t ret = QAPI_ERROR;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
	if(rssi == NULL)
		return ret;

	ret = wmi_wlan_get_statistics(device_ID);
	if(ret == QAPI_OK)
		*rssi = p_cxt->rssi;
	return ret;
}

qapi_Status_t wlan_sta_get_reg_info(qapi_WLAN_Reg_Evt_t *regulatory)
{
	qapi_Status_t ret = QAPI_ERROR;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
	if(regulatory == NULL)
		return ret;

	ret = wmi_wlan_get_regulatory();
	if(ret == QAPI_OK) {
                memscpy(regulatory,sizeof(qapi_WLAN_Reg_Evt_t),&(p_cxt->reg_result),sizeof(qapi_WLAN_Reg_Evt_t));
        }
	return ret;
}

qapi_Status_t wlan_set_ap_beacon_inteval(uint8_t device_ID, uint32_t beacon_interval)
{
	qapi_Status_t error = QAPI_OK;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
	WMI_SET_PDEV_PARAM_CMD *cmd = &p_cxt->dev_param_cmd;
		
	memset(cmd, 0, sizeof(WMI_SET_PDEV_PARAM_CMD));
	cmd->pdev_param_id = WIFI_PARAM_SET_AP_BCN_INTERVAL;
	cmd->pdev_param_value = beacon_interval;

	wmi_dev_cmd_send(WMI_SET_PDEV_PARAM_CMDID, device_ID, cmd, sizeof(WMI_SET_PDEV_PARAM_CMD));
	if(p_cxt->wlan_set_param_block_mode) {
		p_cxt->param_id = WIFI_PARAM_SET_AP_BCN_INTERVAL;
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_PARAM, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }
	error = get_wlan_qapi_error();
	return error;
}

qapi_Status_t wlan_set_ap_dtim_period(uint8_t device_ID, uint32_t dtim_period)
{
	qapi_Status_t error = QAPI_OK;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
	WMI_SET_PDEV_PARAM_CMD *cmd = &p_cxt->dev_param_cmd;
		
	memset(cmd, 0, sizeof(WMI_SET_PDEV_PARAM_CMD));
	cmd->pdev_param_id = WIFI_PARAM_SET_AP_DTIM;
	cmd->pdev_param_value = dtim_period;

	wmi_dev_cmd_send(WMI_SET_PDEV_PARAM_CMDID, device_ID, cmd, sizeof(WMI_SET_PDEV_PARAM_CMD));
	if(p_cxt->wlan_set_param_block_mode) {
		p_cxt->param_id = WIFI_PARAM_SET_AP_DTIM;
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_PARAM, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }
	error = get_wlan_qapi_error();
	return error;
}

qapi_Status_t wlan_set_ap_inactivity(uint8_t device_ID, uint32_t inactivity_time)
{
	qapi_Status_t error = QAPI_OK;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
	WMI_SET_PDEV_PARAM_CMD *cmd = &p_cxt->dev_param_cmd;
		
	memset(cmd, 0, sizeof(WMI_SET_PDEV_PARAM_CMD));
	cmd->pdev_param_id = WIFI_PARAM_SET_AP_INACTIVITY;
	cmd->pdev_param_value = inactivity_time;

	wmi_dev_cmd_send(WMI_SET_PDEV_PARAM_CMDID, device_ID, cmd, sizeof(WMI_SET_PDEV_PARAM_CMD));
	if(p_cxt->wlan_set_param_block_mode) {
		p_cxt->param_id = WIFI_PARAM_SET_AP_INACTIVITY;
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_PARAM, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }
	error = get_wlan_qapi_error();
	return error;
}

qapi_Status_t wlan_set_ap_hidden(uint8_t device_ID, uint8_t hidden)
{
	qapi_Status_t error = QAPI_OK;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
	WMI_SET_PDEV_PARAM_CMD *cmd = &p_cxt->dev_param_cmd;
		
	memset(cmd, 0, sizeof(WMI_SET_PDEV_PARAM_CMD));
	cmd->pdev_param_id = WIFI_PARAM_SET_AP_HIDDEN;
	cmd->pdev_param_value = hidden;

	wmi_dev_cmd_send(WMI_SET_PDEV_PARAM_CMDID, device_ID, cmd, sizeof(WMI_SET_PDEV_PARAM_CMD));
	if(p_cxt->wlan_set_param_block_mode) {
		p_cxt->param_id = WIFI_PARAM_SET_AP_HIDDEN;
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_PARAM, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }
	error = get_wlan_qapi_error();
	return error;
}

qapi_Status_t wlan_set_agg_cfg(uint8_t device_ID, uint16_t tx_tid_mask, uint16_t rx_tid_mask)
{
	qapi_Status_t error = QAPI_OK;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
	WMI_SET_PDEV_PARAM_CMD *cmd = &p_cxt->dev_param_cmd;
    uint32_t mask = tx_tid_mask | (rx_tid_mask <<16);
		
	memset(cmd, 0, sizeof(WMI_SET_PDEV_PARAM_CMD));
	cmd->pdev_param_id = WIFI_PARAM_SET_ALLOW_AGGR;
	cmd->pdev_param_value = mask;
    wmi_dev_cmd_send(WMI_SET_PDEV_PARAM_CMDID, device_ID, cmd, sizeof(WMI_SET_PDEV_PARAM_CMD));

	if(p_cxt->wlan_set_param_block_mode) {
        p_cxt->param_id = WIFI_PARAM_SET_ALLOW_AGGR;
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_PARAM, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }
	error = get_wlan_qapi_error();
	return error;
}

qapi_Status_t wlan_set_amsdu_rx(uint8_t device_ID, uint8_t enable)
{
	qapi_Status_t error = QAPI_OK;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
	WMI_SET_PDEV_PARAM_CMD *cmd = &p_cxt->dev_param_cmd;
		
	memset(cmd, 0, sizeof(WMI_SET_PDEV_PARAM_CMD));
	cmd->pdev_param_id = WIFI_PARAM_SET_AMSDU_RX;
	cmd->pdev_param_value = enable;
    wmi_dev_cmd_send(WMI_SET_PDEV_PARAM_CMDID, device_ID, cmd, sizeof(WMI_SET_PDEV_PARAM_CMD));

	if(p_cxt->wlan_set_param_block_mode) {
        p_cxt->param_id = WIFI_PARAM_SET_AMSDU_RX;
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_PARAM, QURT_SIGNAL_ATTR_CLEAR_MASK);
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }
	error = get_wlan_qapi_error();
	return error;
}

