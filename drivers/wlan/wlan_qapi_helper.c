/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "wlan_drv.h"
#include "wlan_qapi_helper.h"
#include "wlan_ra.h"
#include "wmi_api.h"
#include "safeAPI.h"

#ifdef CONFIG_6GHZ
/*11 for 2G and 30 for 5G and 24 for 6G */
#define SCAN_LIST_NUM_CHANNELS 68
#elif defined(SUPPORT_5GHZ)
/*11 for 2G and 33 for 5G*/
#define SCAN_LIST_NUM_CHANNELS 44
#else
/*11 for 2G*/
#define SCAN_LIST_NUM_CHANNELS 11 
#endif /* CONFIG_6GHZ */

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
    p_connect_cmd->num_channels = SCAN_LIST_NUM_CHANNELS;
	for (int i=0; i<p_connect_cmd->num_channels; i++) {
		p_connect_cmd->channel_list[i] = i;
	}
    p_connect_cmd->wlan_mode = MODE_11ABGN_HT20;
}

int32_t wlan_channel_to_freq(uint16_t *channel, qbool_t is_6g_index)
{
    if (NULL == channel)
    {
        return -1;
    }
    if(*channel < 1 || *channel > 173)
    {
      return -1;
    }
	if (is_6g_index) 
	{
		*channel = __QAPI_WLAN_6G_CHAN_FREQ_1 + ((*channel - 1)) * 5;
	} else 
	{
		if (*channel < 27) {
			if(*channel == 14)
				*channel = __QAPI_WLAN_CHAN_FREQ_14;
			else
				*channel = __QAPI_WLAN_CHAN_FREQ_1 + ((*channel - 1) * 5);
		} else {
			*channel = (5000 + (*channel * 5));
		}
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
    else if (*channel < 5955)
    {
        *channel -= __QAPI_WLAN_CHAN_FREQ_36;
        *channel = 36 + (*channel / 5); // since in 11a channel 36 is the starting number
    }
	else
	{
        *channel -= __QAPI_WLAN_6G_CHAN_FREQ_1;
        *channel = (*channel / 5) + 1; // since in 11ax channel 1 is the starting number			   
	}
    return 0;
}

qapi_Status_t wlan_set_channel(uint8_t device_id, uint16_t channel, qbool_t is_6g_index)
{
	qapi_Status_t error = QAPI_OK;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
	WMI_SET_PDEV_PARAM_CMD *cmd = &p_cxt->dev_param_cmd;

    if (0 != wlan_channel_to_freq(&channel, is_6g_index))
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

qapi_Status_t wlan_set_sta_slptime(uint8_t device_ID, uint16_t time, uint16_t round_type)
{
	qapi_Status_t error = QAPI_OK;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
	WMI_SET_PDEV_PARAM_CMD *cmd = &p_cxt->dev_param_cmd;

	memset(cmd, 0, sizeof(WMI_SET_PDEV_PARAM_CMD));
	cmd->pdev_param_id = WIFI_PARAM_SET_STA_DTIM;
	cmd->pdev_param_value = time | (round_type << 16);
	wmi_dev_cmd_send(WMI_SET_PDEV_PARAM_CMDID, device_ID, cmd, sizeof(WMI_SET_PDEV_PARAM_CMD));

	if(p_cxt->wlan_set_param_block_mode) {
		p_cxt->param_id = WIFI_PARAM_SET_STA_DTIM;
		qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_PARAM, QURT_SIGNAL_ATTR_CLEAR_MASK);
	} else {
		log_printf("unblock mode, should check WMI cmd done in event cb\n");
	}
	error = get_wlan_qapi_error();
	return error;
}

qapi_Status_t wlan_get_sta_slptime(uint32_t *listen_interval)
{
	extern devh_t *gdevp;
	extern uint16_t wlan_get_listen_interval(devh_t *dev, uint16_t beaconInterval);
	uint16_t ni_intval = gdevp->bss->ni_intval;

	if (gdevp->ifState == IF_UP)
		*listen_interval = (uint32_t)wlan_get_listen_interval(gdevp, ni_intval) * ni_intval;
	else
		*listen_interval = (uint32_t)wlan_get_listen_interval(gdevp, 100) * 100;
	return QAPI_OK;
}

qapi_Status_t wlan_clear_mgmt_frame_queue(void)
{
	WMI_MGMT_FRAME_RECV_MSG mgmt_frame;
	WMI_MGMT_FRAME_FILTER *p_mgmt_filter = &(gp_wlan_qapi_cxt->mgmt_filter);
	

	if (NULL == p_mgmt_filter->recv_queue)
	{
	    return QAPI_OK;
	}
    
	while (qurt_pipe_receive_timed(p_mgmt_filter->recv_queue, &mgmt_frame, 0) == NT_QUEUE_SUCCESS)
	{
		nt_osal_free_memory(mgmt_frame.frame);
	}

    
    return QAPI_OK;
}

qapi_Status_t wlan_recv_mgmt_frame(uint8_t *buffer, uint32_t buffer_len, uint32_t *frame_len, uint32_t timeout)
{    
    qapi_Status_t ret = QAPI_WLAN_ERROR;
	WMI_MGMT_FRAME_RECV_MSG mgmt_frame;
	WMI_MGMT_FRAME_FILTER *p_mgmt_filter = &(gp_wlan_qapi_cxt->mgmt_filter);
	
	if (qurt_pipe_receive_timed(p_mgmt_filter->recv_queue, &mgmt_frame, timeout) == NT_QUEUE_SUCCESS)
	{
		memscpy(buffer, buffer_len, mgmt_frame.frame, mgmt_frame.frame_len);
		*frame_len = mgmt_frame.frame_len;
		nt_osal_free_memory(mgmt_frame.frame);
		ret = QAPI_OK;
	}
	else
	{
	    ret = QAPI_WLAN_ERR_QOSAL_EVENT_TIMEOUT;
	}
    
    return ret;
}

qapi_Status_t wlan_set_appie(qapi_WLAN_App_Ie_Params_t *ie_params)
{
    qapi_Status_t error = QAPI_OK;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    WMI_SET_APPIE_CMD *cmd = &p_cxt->appie_cmd;
    if(cmd == NULL)
    {
        return QAPI_ERROR;
    }
    memset(cmd, 0, sizeof(WMI_SET_APPIE_CMD));

    /* Application IE is a hex number starting with 0xdd.
     * Hex number 0xdd of length 1 will remove the already added IE. */
    if((ie_params->ie_Len < 1) || (ie_params->ie_Len > WMI_MAX_APP_IE_LEN) || !ie_params->ie_Info)
    {
        log_printf("%s:%d: IE length %d is out of the range of 1 and 64.\n", __func__, __LINE__, ie_params->ie_Len);
        return QAPI_ERROR;
    }
    /* The length must be not less than 5 as a valid application information element
     * at least has element ID, length and OUI per 802.11 spec.
     */
    if((ie_params->ie_Len > 1) && (ie_params->ie_Len < 5))
    {
        log_printf("%s:%d: IE length %d is less than 5 bytes.\n", __func__, __LINE__, ie_params->ie_Len);
        return QAPI_ERROR;
    }

    if (ie_params->ie_Info[0] != 0xdd)
    {
        log_printf("%s:%d: Application specified information element must start with 'dd'.\n", __func__, __LINE__);
        return QAPI_ERROR;
    }

	/* The length in application information element should be the length of OUI and Vendor-specific content*/
	if ((ie_params->ie_Len > 1) &&(ie_params->ie_Info[1] != (ie_params->ie_Len -2)))
    {
        log_printf("%s:%d: The length in application information element is not correct.\n", __func__, __LINE__);
        return QAPI_ERROR;
    }
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    cmd->mgmtFrmType = ie_params->mgmt_Frame_Type;
    cmd->ieLen = ie_params->ie_Len;
     
    memscpy(cmd->ieInfo, ie_params->ie_Len, ie_params->ie_Info, ie_params->ie_Len);
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);

    wmi_cmd_send(WMI_SET_APPIE_CMDID, cmd, sizeof(WMI_SET_APPIE_CMD));

    if (p_cxt->wlan_set_param_block_mode) 
    {
        p_cxt->param_id = WIFI_PARAM_SET_APP_IE;
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_PARAM, QURT_SIGNAL_ATTR_CLEAR_MASK);
        log_printf("set appie: block mode, WMI cmd done\n");
    } 
    else 
    {
        log_printf("set appie: unblock mode, should check WMI cmd done in event cb\n");
    }

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    error = get_wlan_qapi_error();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);

    return error;  
}

qapi_Status_t wlan_set_rts_cts(uint8_t device_ID, uint32_t enable)
{
	qapi_Status_t error = QAPI_OK;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
	WMI_SET_PDEV_PARAM_CMD *cmd = &p_cxt->dev_param_cmd;

	memset(cmd, 0, sizeof(WMI_SET_PDEV_PARAM_CMD));
	cmd->pdev_param_id = WIFI_PARAM_SET_RTS_CTS;
	cmd->pdev_param_value = enable;

	wmi_dev_cmd_send(WMI_SET_PDEV_PARAM_CMDID, device_ID, cmd, sizeof(WMI_SET_PDEV_PARAM_CMD));

	if(p_cxt->wlan_set_param_block_mode) {
		p_cxt->param_id = WIFI_PARAM_SET_RTS_CTS;
		qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_PARAM, QURT_SIGNAL_ATTR_CLEAR_MASK);
	} else {
		log_printf("unblock mode, should check WMI cmd done in event cb\n");
	}
	error = get_wlan_qapi_error();
	return error;
}

qapi_Status_t wlan_get_rts_cts(uint32_t *enable)
{
	volatile hal_tpe_sta_desc_t* desc = ((volatile hal_tpe_sta_desc_t *) ((uint32_t)&_ln_RAM_start_addr_hw_desc__ + HAL_MMAP_TPE_DESC_OFST)) + STA_MODE;
	if (desc->rate_params_20Mhz[0].protection_mode)
		*enable = 1;
	else
		*enable = 0;
	return QAPI_OK;
}

qapi_Status_t wlan_set_rts_rate(uint8_t device_ID, uint32_t rate)
{
	qapi_Status_t error = QAPI_OK;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
	WMI_SET_PDEV_PARAM_CMD *cmd = &p_cxt->dev_param_cmd;

	if (rate > 1)
		return QAPI_ERR_INVALID_PARAM;

	memset(cmd, 0, sizeof(WMI_SET_PDEV_PARAM_CMD));
	cmd->pdev_param_id = WIFI_PARAM_SET_RTS_RATE_2G;
	cmd->pdev_param_value = rate;

	wmi_dev_cmd_send(WMI_SET_PDEV_PARAM_CMDID, device_ID, cmd, sizeof(WMI_SET_PDEV_PARAM_CMD));

	if(p_cxt->wlan_set_param_block_mode) {
		p_cxt->param_id = WIFI_PARAM_SET_RTS_RATE_2G;
		qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_PARAM, QURT_SIGNAL_ATTR_CLEAR_MASK);
	} else {
		log_printf("unblock mode, should check WMI cmd done in event cb\n");
	}
	error = get_wlan_qapi_error();
	return error;
}

qapi_Status_t wlan_get_rts_rate(uint32_t *rate)
{
	nt_hal_fix_rts_rate(0, (uint32_t)rate);

	if (*rate == HAL_RT_IDX_11B_LONG_1_MBPS)
		*rate = 1;
	else if (*rate == HAL_RT_IDX_11A_6_MBPS)
		*rate = 6;
	else if (*rate == HAL_RT_IDX_11A_12_MBPS)
		*rate = 12;
	else
		return QAPI_ERR_INVALID_PARAM;
	return QAPI_OK;
}

qapi_Status_t wlan_set_cw_size(uint8_t device_ID, uint8_t qid, uint16_t cw_min, uint16_t cw_max)
{
	qapi_Status_t error = QAPI_OK;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
	WMI_SET_PDEV_PARAM_CMD *cmd = &p_cxt->dev_param_cmd;
	qapi_WLAN_Contention_Window_Params_t cw_para;

	if ((qid >= 8) && (qid != 0xff))
		return QAPI_ERR_INVALID_PARAM;

	cw_para.qid = qid;
	cw_para.cw_min = cw_min;
	cw_para.cw_max = cw_max;

	memset(cmd, 0, sizeof(WMI_SET_PDEV_PARAM_CMD));
	cmd->pdev_param_id = WIFI_PARAM_SET_CW_SIZE;
	cmd->pdev_param_value = (uint32_t)&cw_para;

	wmi_dev_cmd_send(WMI_SET_PDEV_PARAM_CMDID, device_ID, cmd, sizeof(WMI_SET_PDEV_PARAM_CMD));

	if(p_cxt->wlan_set_param_block_mode) {
		p_cxt->param_id = WIFI_PARAM_SET_CW_SIZE;
		qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_PARAM, QURT_SIGNAL_ATTR_CLEAR_MASK);
	} else {
		log_printf("unblock mode, should check WMI cmd done in event cb\n");
	}
	error = get_wlan_qapi_error();
	return error;
}

qapi_Status_t wlan_get_cw_size(uint8_t qid, uint16_t *cw_min, uint16_t *cw_max)
{
	uint32_t value;
	if (qid > 7 && qid != 0xff)
		return QAPI_ERR_INVALID_PARAM;

	if (qid == 0xff)
		value = HAL_REG_RD(QWLAN_MTU_SW_CW_MIN_CW_MAX_0_REG);
	else
		value = HAL_REG_RD(QWLAN_MTU_SW_CW_MIN_CW_MAX_0_REG + 4 * qid);

	*cw_min = (uint16_t)value;
	*cw_max = (uint16_t)(value >> 16);

	return QAPI_OK;
}

qapi_Status_t wlan_set_per_upper_threshold(uint8_t device_ID, uint32_t threshold)
{
	qapi_Status_t error = QAPI_OK;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
	WMI_SET_PDEV_PARAM_CMD *cmd = &p_cxt->dev_param_cmd;

	if (threshold > 100)
		return QAPI_ERR_INVALID_PARAM;

	memset(cmd, 0, sizeof(WMI_SET_PDEV_PARAM_CMD));
	cmd->pdev_param_id = WIFI_PARAM_SET_PER_UPPER_THRESHOLD;
	cmd->pdev_param_value = threshold;

	wmi_dev_cmd_send(WMI_SET_PDEV_PARAM_CMDID, device_ID, cmd, sizeof(WMI_SET_PDEV_PARAM_CMD));

	if(p_cxt->wlan_set_param_block_mode) {
		p_cxt->param_id = WIFI_PARAM_SET_PER_UPPER_THRESHOLD;
		qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_PARAM, QURT_SIGNAL_ATTR_CLEAR_MASK);
	} else {
		log_printf("unblock mode, should check WMI cmd done in event cb\n");
	}
	error = get_wlan_qapi_error();
	return error;
}

qapi_Status_t wlan_get_per_upper_threshold(uint32_t *threshold)
{
	extern devh_t *gdevp;
	nt_rate_context_t *pRateCtrl = (nt_rate_context_t *)gdevp->pRateCtrl;
	*threshold = pRateCtrl->perUpperThresh;

	return QAPI_OK;
}

qapi_Status_t wlan_set_ba_win_size(uint8_t device_ID, uint16_t ack_timeout, uint16_t delay)
{
	qapi_Status_t error = QAPI_OK;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
	WMI_SET_PDEV_PARAM_CMD *cmd = &p_cxt->dev_param_cmd;

	if (ack_timeout >= 4096|| delay >= 64)
		return QAPI_ERR_INVALID_PARAM;

	memset(cmd, 0, sizeof(WMI_SET_PDEV_PARAM_CMD));
	cmd->pdev_param_id = WIFI_PARAM_SET_BA_WIN_SIZE;
	cmd->pdev_param_value = (ack_timeout << 16) | delay;

	wmi_dev_cmd_send(WMI_SET_PDEV_PARAM_CMDID, device_ID, cmd, sizeof(WMI_SET_PDEV_PARAM_CMD));

	if(p_cxt->wlan_set_param_block_mode) {
		p_cxt->param_id = WIFI_PARAM_SET_BA_WIN_SIZE;
		qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_PARAM, QURT_SIGNAL_ATTR_CLEAR_MASK);
	} else {
		log_printf("unblock mode, should check WMI cmd done in event cb\n");
	}
	error = get_wlan_qapi_error();
	return error;
}

qapi_Status_t wlan_get_ba_win_size(uint16_t *ack_timeout, uint16_t *delay)
{
	uint32_t value;
	value = HAL_REG_RD(QWLAN_MTU_SW_MTU_BCN_SLOT_USEC_SIFS_LIMIT_REG);
	value &= (uint32_t)QWLAN_MTU_SW_MTU_BCN_SLOT_USEC_SIFS_LIMIT_SW_MTU_EARLY_PKT_DET_MISS_LIMIT_MASK;
	*ack_timeout = (uint16_t)(value >> QWLAN_MTU_SW_MTU_BCN_SLOT_USEC_SIFS_LIMIT_SW_MTU_EARLY_PKT_DET_MISS_LIMIT_OFFSET);

	value = HAL_REG_RD(QWLAN_AGC_D_FIRANDCAL_REG);
	value &= (uint32_t)QWLAN_AGC_D_FIRANDCAL_DELAY_MASK;
	*delay = (uint16_t)(value >> QWLAN_AGC_D_FIRANDCAL_DELAY_OFFSET);
	return QAPI_OK;
}

qapi_Status_t wlan_set_slot_time(uint8_t device_ID, uint32_t slot_time)
{
	qapi_Status_t error = QAPI_OK;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
	WMI_SET_PDEV_PARAM_CMD *cmd = &p_cxt->dev_param_cmd;

	if (slot_time != 9 && slot_time != 20)
		return QAPI_ERR_INVALID_PARAM;

	memset(cmd, 0, sizeof(WMI_SET_PDEV_PARAM_CMD));
	cmd->pdev_param_id = WIFI_PARAM_SET_SLOT_TIME;
	cmd->pdev_param_value = slot_time;

	wmi_dev_cmd_send(WMI_SET_PDEV_PARAM_CMDID, device_ID, cmd, sizeof(WMI_SET_PDEV_PARAM_CMD));

	if(p_cxt->wlan_set_param_block_mode) {
		p_cxt->param_id = WIFI_PARAM_SET_SLOT_TIME;
		qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_PARAM, QURT_SIGNAL_ATTR_CLEAR_MASK);
	} else {
		log_printf("unblock mode, should check WMI cmd done in event cb\n");
	}
	error = get_wlan_qapi_error();
	return error;
}

qapi_Status_t wlan_get_slot_time(uint32_t *slot_time)
{
	uint32_t value;
	value = HAL_REG_RD(QWLAN_MTU_SW_MTU_BCN_SLOT_USEC_SIFS_LIMIT_REG);
	value &= (uint32_t)QWLAN_MTU_SW_MTU_BCN_SLOT_USEC_SIFS_LIMIT_SW_MTU_BCN_SLOT_LIMIT_MASK;
	*slot_time = (value >> QWLAN_MTU_SW_MTU_BCN_SLOT_USEC_SIFS_LIMIT_SW_MTU_BCN_SLOT_LIMIT_OFFSET);
	return QAPI_OK;
}
