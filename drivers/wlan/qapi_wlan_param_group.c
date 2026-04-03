/* 
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "wlan_drv.h"
#include "wlan_qapi_helper.h"
#ifdef CONFIG_WLAN_8021X
#include "wlan_8021x_cxt.h"
#include "suppl_auth.h"

extern wlan_8021x_global_t *g_wl8021x_global;
extern int wpa_debug_level;
#endif

/* Should be called under protection of p_cxt->wlan_qapi_cxt_mutex */
static void _wlan_set_wep(void)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    WMI_CONNECT_CMD *p_connect_cmd = &p_cxt->connect_cmd;

    p_connect_cmd->dot11AuthMode = OPEN_AUTH;
    p_connect_cmd->authMode = WMI_NONE_AUTH;
    p_connect_cmd->pairwiseCryptoType = WEP_CRYPT;
}

qapi_Status_t qapi_WLAN_Set_Param(uint8_t __attribute__((__unused__)) device_ID, uint16_t group_ID, uint16_t param_ID,
                                  const void *data, uint32_t length,
                                  qapi_WLAN_Wait_For_Status_e __attribute__((__unused__)) wait_For_Status)
{
    qapi_Status_t ret = QAPI_OK;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    if (gp_wlan_qapi_cxt->wlanEnabled == false) {
        warn_printf("wlan is not enabled\n");
        return QAPI_ERROR;
    }

    switch (group_ID) {
    case __QAPI_WLAN_PARAM_GROUP_WIRELESS: {
        switch (param_ID) {
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID: {
            if (!data || !length) {
                warn_printf("clear connect ssid\n");
            } else {
                #if 0
                if (length > __QAPI_WLAN_MAX_SSID_LEN) {
                    PRINT_ERR_INVALID_PARAM1("length", length);
                    ret = QAPI_WLAN_ERR_EINVAL;
                    break;
                }
                #endif
            }
            qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
            wlan_set_connect_ssid((unsigned char *)data, (uint8_t)length);
            qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_BSSID: {
            if (!data || !length) {
                warn_printf("clear connect bssid\n");
            } else {
                if (length != __QAPI_WLAN_MAC_LEN) {
                    PRINT_ERR_INVALID_PARAM1("length", length);
                    ret = QAPI_WLAN_ERR_EINVAL;
                    break;
                }
            }
            qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
            wlan_set_connect_bssid((uint8_t *)data, (uint8_t)length);
            qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_BSSID */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_CHANNEL: {
            uint16_t channel = ((uint16_t *)data)[0];
            qbool_t is_6g_index = (qbool_t)((uint32_t *)data)[1];
            if (is_6g_index == TRUE || is_6g_index == FALSE) {
                ret = wlan_set_channel(device_ID, channel, is_6g_index);
            } else {
                PRINT_ERR_INVALID_PARAM1("is_6g_index", is_6g_index);
                ret = QAPI_WLAN_ERR_EINVAL;
            }
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_CHANNEL */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_PHY_MODE: {
            qapi_WLAN_Phy_Mode_e phy_mode = *((qapi_WLAN_Phy_Mode_e *)data);
            ret = wlan_set_phy_mode(device_ID, (uint32_t)phy_mode);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_PHY_MODE */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_11N_HT: {
            qapi_WLAN_11n_HT_Config_t config = *(qapi_WLAN_11n_HT_Config_t *)data;
            ret = wlan_set_11n_ht(device_ID, (uint8_t)config.htconfig, config.sgi, config.mpdu_density);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_11N_HT */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE: {
            uint8_t mode = *((uint8_t *)data);
            ret = (qapi_Status_t)wlan_set_op_mode(mode);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_COUNTRY_CODE: {
            uint8_t *country_code = (uint8_t *)data;
            ret = wlan_set_country_code(device_ID, country_code);
            if (ret == QAPI_OK)
                memscpy(p_cxt->country_code, 3, (char *)country_code, 3);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_COUNTRY_CODE */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_BEACON_INTERVAL_IN_TU: {
            uint32_t beacon_interval = *((uint32_t *)data);
            ret = wlan_set_ap_beacon_inteval(device_ID, beacon_interval);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_BEACON_INTERVAL_IN_TU */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_DTIM_INTERVAL: {
            uint32_t dtim_period = *((uint32_t *)data);
            ret = wlan_set_ap_dtim_period(device_ID, dtim_period);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_DTIM_INTERVAL */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_INACTIVITY_TIME_IN_MINS: {
            uint32_t inactivity_time = *((uint32_t *)data);
            ret = wlan_set_ap_inactivity(device_ID, inactivity_time);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_INACTIVITY_TIME_IN_MINS */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_ENABLE_HIDDEN_MODE: {
            uint8_t hidden = *((uint8_t *)data);
            ret = wlan_set_ap_hidden(device_ID, hidden);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_ENABLE_HIDDEN_MODE */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_ALLOW_TX_RX_AGGR_SET_TID: {
            qapi_WLAN_Aggregation_Params_t *paggr = (qapi_WLAN_Aggregation_Params_t *)data;
            ret = wlan_set_agg_cfg(device_ID, paggr->tx_TID_Mask, paggr->rx_TID_Mask);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_ALLOW_TX_RX_AGGR_SET_TID */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_AMSDU_RX: {
            uint8_t enable = *((uint8_t *)data);
            ret = wlan_set_amsdu_rx(device_ID, enable);
			break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_AMSDU_RX */
		}
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_LISTEN_INTERVAL_IN_TU: {
            qapi_WLAN_Listen_Interval_Params_t *listen_interval = (qapi_WLAN_Listen_Interval_Params_t *) data;
            ret = wlan_set_sta_slptime(device_ID, listen_interval->time, listen_interval->round_type);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_LISTEN_INTERVAL_IN_TU */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_APP_IE: {
            qapi_WLAN_App_Ie_Params_t *ie_param = (qapi_WLAN_App_Ie_Params_t *)data;
            ret = wlan_set_appie(ie_param);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_APP_IE */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_RTS: {
            uint32_t enable = *((uint32_t *)data);
            ret = wlan_set_rts_cts(device_ID, enable);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_RTS */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_RTS_RATE_2G: {
            uint32_t rate = *((uint32_t *)data);
            ret = wlan_set_rts_rate(device_ID, rate);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_RTS_RATE_2G */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_EDCA_PARAM: {
            qapi_WLAN_Edca_Params_t edca_para = *((qapi_WLAN_Edca_Params_t *)data);
            ret = wlan_set_edca_param(device_ID, edca_para.qid, edca_para.aifsn, edca_para.cw_min, edca_para.cw_max,
                                      edca_para.txop_limit);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_CONTENTION_WINDOW */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_PER_UPPER_THRESHOLD: {
            uint32_t threshold = *((uint32_t *)data);
            ret = wlan_set_per_upper_threshold(device_ID, threshold);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_PER_UPPER_THRESHOLD */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_BA_WINDOW: {
            qapi_WLAN_BA_Window_Params_t ba_win = *((qapi_WLAN_BA_Window_Params_t *)data);
            ret = wlan_set_ba_win_size(device_ID, ba_win.ack_timeout, ba_win.delay);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_BA_WINDOW */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_SLOT_TIME: {
            uint32_t slot_time = *((uint32_t *)data);
            ret = wlan_set_slot_time(device_ID, slot_time);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_SLOT_TIME */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_EDCCA_THRESHOLD: {
            uint8_t edcca_threshold = *((uint8_t *)data);
            ret = wlan_set_edcca_threshold(device_ID, edcca_threshold);
            break;
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_TX_POWER_IN_DBM: {
            qapi_WLAN_Set_Txpower_Params_t txPwr_Params = *((qapi_WLAN_Set_Txpower_Params_t *)data);
            ret = wlan_set_tx_power(txPwr_Params);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_TX_POWER_IN_DBM */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_BMISS_CONFIG: {
            uint8_t bmiss_threshold = *((uint8_t *)data);
            ret = wlan_set_bmiss_threshold(device_ID, bmiss_threshold);
            break;
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_RSP_RATE: {
            ret = (qapi_Status_t)wlan_set_rsp_rate(device_ID, (*(uint8_t *)data));
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_RSP_RATE */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_BA_WINDOW_SIZE: {
			qapi_WLAN_BA_Window_Size_t ba_size = *((qapi_WLAN_BA_Window_Size_t *) data);
            ret = wlan_set_ba_window_size(device_ID, ba_size.tx_size, ba_size.rx_size);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_BA_WINDOW_SIZE */
        }
        case  __QAPI_WLAN_PARAM_GROUP_WIRELESS_PROTECTION_MODE: {
            uint32_t enable = *((uint32_t *)data);
            ret = wlan_set_cts_to_self(device_ID, enable);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_PROTECTION_MODE */
        } 
        default: /* __QAPI_WLAN_PARAM_GROUP_WIRELESS + param_ID */
            PRINT_ERR_INVALID_PARAM1("param_ID", param_ID);
            ret = QAPI_WLAN_ERR_EINVAL;
        }
        break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS */
    }
    case __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY: {
        switch (param_ID) {
#ifdef CONFIG_WLAN_8021X
        case __QAPI_WLAN_PARAM_GROUP_SECURITY_PMK: {
            if (!data || !length) {
                warn_printf("clear passphrase\n");
            } else {
                if (length > __QAPI_WLAN_PASSPHRASE_LEN) {
                    PRINT_ERR_INVALID_PARAM1("length", length);
                    ret = QAPI_WLAN_ERR_EINVAL;
                    break;
                }
            }
            qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
            wlan_sec_set_pmk(device_ID, (uint8_t*) data, length);
            qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
            break; /* __QAPI_WLAN_PARAM_GROUP_SECURITY_PMK */
        }
#endif
        case __QAPI_WLAN_PARAM_GROUP_SECURITY_PASSPHRASE: {
            if (!data || !length) {
                warn_printf("clear passphrase\n");
            } else {
                if (length > __QAPI_WLAN_PASSPHRASE_LEN) {
                    PRINT_ERR_INVALID_PARAM1("length", length);
                    ret = QAPI_WLAN_ERR_EINVAL;
                    break;
                }
            }
            qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
            wlan_set_passphrase((uint8_t *)data, (uint8_t)length);
            qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
            break; /* __QAPI_WLAN_PARAM_GROUP_SECURITY_PASSPHRASE */
        }
        case __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE: {
            if (!data || !length) {
                warn_printf("clear authMode\n");
            }
            WMI_CONNECT_CMD *p_cmd = &p_cxt->connect_cmd;
            qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
            if (!data || !length) {
                wlan_clear_privacy();
                info_printf("clear dot11AuthMode/authMode as open\n");
            } else {
                qapi_WLAN_Auth_Mode_e e_wpa_ver = (qapi_WLAN_Auth_Mode_e)(*(uint32_t *)data);
                info_printf("set e_wpa_ver=%d\n", e_wpa_ver);
                switch (e_wpa_ver) {
                case QAPI_WLAN_AUTH_NONE_E:
                    wlan_clear_privacy();
                    break;
                case QAPI_WLAN_AUTH_WEP_E:
                    _wlan_set_wep();
                    break;
                case QAPI_WLAN_AUTH_WPA_PSK_E:
                    p_cmd->dot11AuthMode = OPEN_AUTH;
                    p_cmd->authMode = WMI_WPA_PSK_AUTH;
                    break;
                case QAPI_WLAN_AUTH_WPA2_PSK_E:
                    p_cmd->dot11AuthMode = OPEN_AUTH;
                    p_cmd->authMode = WMI_WPA2_PSK_AUTH;
                    break;
                case QAPI_WLAN_AUTH_WPA3_SAE_E:
                    p_cmd->dot11AuthMode = SAE_AUTH;
                    p_cmd->authMode = WMI_WPA3_SHA256_AUTH;
                    break;
                case QAPI_WLAN_AUTH_WPA2_SAE_MIXED_E:
                    p_cmd->dot11AuthMode = (SAE_AUTH | OPEN_AUTH);
                    p_cmd->authMode = (WMI_WPA3_SHA256_AUTH | WMI_WPA2_PSK_AUTH);
                    break;
                case QAPI_WLAN_AUTH_WPA_WPA2_MIXED_E:
                    p_cmd->dot11AuthMode = OPEN_AUTH;
                    p_cmd->authMode = (WMI_WPA2_PSK_AUTH | WMI_WPA_PSK_AUTH);
                    break;
                case QAPI_WLAN_AUTH_WPA_WPA2_SAE_MIXED_E:
                    p_cmd->dot11AuthMode = (SAE_AUTH | OPEN_AUTH);
                    p_cmd->authMode = (WMI_WPA3_SHA256_AUTH | WMI_WPA2_PSK_AUTH | WMI_WPA_PSK_AUTH);
                    break;
#ifdef CONFIG_WLAN_8021X
                case QAPI_WLAN_AUTH_WPA_E:
                    p_cmd->dot11AuthMode = OPEN_AUTH;
                    p_cmd->authMode = WMI_WPA_AUTH;
                    break;
                case QAPI_WLAN_AUTH_WPA2_E:
                    p_cmd->dot11AuthMode = OPEN_AUTH;
                    p_cmd->authMode = WMI_WPA2_AUTH;
                    break;
                case QAPI_WLAN_AUTH_WPA3_EAP_ONLY_E:
                    p_cmd->dot11AuthMode = OPEN_AUTH;
                    p_cmd->authMode = WMI_WPA3_ENTERPRISE_ONLY_AUTH;
                    break;
                case QAPI_WLAN_AUTH_WPA3_EAP_TRANSITION_E:
                    p_cmd->dot11AuthMode = OPEN_AUTH;
                    p_cmd->authMode = (WMI_WPA2_AUTH | WMI_WPA2_SHA256_AUTH);
                    break;
#endif
                default:
                    PRINT_ERR_INVALID_PARAM1("e_wpa_ver", e_wpa_ver);
                    ret = QAPI_WLAN_ERR_EINVAL;
                    break;
                }
#ifdef CONFIG_WLAN_8021X
                if (AUTH_IS_8021X(p_cmd->authMode)) {
                    wlan_8021x_set_auth_mode(p_cmd->authMode);
                }
#endif
            }
            qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
            break; /* __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE */
        }
        case __QAPI_WLAN_PARAM_GROUP_SECURITY_ENCRYPTION_TYPE: {
            if (!data || !length) {
                PRINT_ERR_INVALID_PARAM;
                ret = QAPI_WLAN_ERR_EINVAL;
                break;
            }
            qapi_WLAN_Crypt_Type_e e_cipher = (qapi_WLAN_Crypt_Type_e)(*(uint32_t *)data);
            if (e_cipher >= QAPI_WLAN_CRYPT_INVALID_E) {
                PRINT_ERR_INVALID_PARAM1("e_cipher", e_cipher);
                ret = QAPI_WLAN_ERR_EINVAL;
                break;
            }
            WMI_CONNECT_CMD *p_cmd = &p_cxt->connect_cmd;
            qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
            info_printf("set e_cipher=%d\n", e_cipher);
            switch (e_cipher) {
            case QAPI_WLAN_CRYPT_NONE_E:
                wlan_clear_privacy();
                break;
            case QAPI_WLAN_CRYPT_WEP_CRYPT_E:
                _wlan_set_wep();
                break;
            case QAPI_WLAN_CRYPT_TKIP_CRYPT_E:
                p_cmd->pairwiseCryptoType = TKIP_CRYPT;
                p_cmd->groupCryptoType = TKIP_CRYPT;
                break;
            case QAPI_WLAN_CRYPT_AES_CRYPT_E:
                p_cmd->pairwiseCryptoType = AES_CRYPT;
                p_cmd->groupCryptoType = TKIP_CRYPT | AES_CRYPT;
                break;
            case QAPI_WLAN_CRYPT_AUTO:
                p_cmd->pairwiseCryptoType = TKIP_CRYPT | AES_CRYPT;
                p_cmd->groupCryptoType = TKIP_CRYPT | AES_CRYPT;
                break;
            default:
                PRINT_ERR_INVALID_PARAM1("e_cipher", e_cipher);
                ret = QAPI_WLAN_ERR_EINVAL;
                break;
            }
            qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
            break; /* __QAPI_WLAN_PARAM_GROUP_SECURITY_ENCRYPTION_TYPE */
        }
#ifdef CONFIG_WPS
        case __QAPI_WLAN_PARAM_GROUP_SECURITY_WPS_CREDENTIALS: {
            ret = (qapi_Status_t)wlan_wps_set_credentials(device_ID, (qapi_WLAN_WPS_Credentials_t *)data);
            break; /* __QAPI_WLAN_PARAM_GROUP_SECURITY_WPS_CREDENTIALS */
        }
#endif
#ifdef CONFIG_WLAN_8021X
        case __QAPI_WLAN_PARAM_GROUP_SECURITY_PMKID: {
            ret = (qapi_Status_t)wlan_sec_set_pmkid(device_ID, (qapi_WLAN_Set_PMKID_Params_t*) data);
            break;
        }
        case __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_METHOD:
        case __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_IDENTITY:
        case __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_USERNAME:
        case __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_PASSWORD:
        case __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_CA_CER:
        case __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_CER:
        case __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_PRIVATE_KEY:
        case __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_NO_SERVER_AUTH: {
            ret = wlan_8021x_set_param(device_ID, param_ID, data, length, g_wl8021x_global);
            break; /* __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_START ~ __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_END */
        }
        case __QAPI_WLAN_PARAM_GROUP_SECURITY_DEBUG_LEVEL:
            info_printf("%s wpa dbglevel=%d=>%d\n", __FUNCTION__, wpa_debug_level, *((int *)data));
            wpa_debug_level = *((int *)data);
            ret = QAPI_OK;
            break;
#endif
        default: /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY + param_ID */
            PRINT_ERR_INVALID_PARAM1("param_ID", param_ID);
            ret = QAPI_WLAN_ERR_EINVAL;
        }
        break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY */
    }

#ifdef CONFIG_ENABLE_P2P_MODE
    case __QAPI_WLAN_PARAM_GROUP_P2P: {
        switch (param_ID) {
        case __QAPI_WLAN_PARAM_GROUP_P2P_CONFIG_PARAMS: {
            qapi_WLAN_P2P_Config_Params_t *p2pConfig = (qapi_WLAN_P2P_Config_Params_t *)data;

            /* The listen channel can be one of the social channels only */
            if ((p2pConfig->listen_Chan != 1) && (p2pConfig->listen_Chan != 6) && (p2pConfig->listen_Chan != 11)) {
                return QAPI_ERROR;
            }
            ret = wlan_p2p_set_config(device_ID, p2pConfig);
            break;
        }

        case __QAPI_WLAN_PARAM_GROUP_P2P_OPPS_PARAMS: {
            ret = wlan_p2p_set_opps(device_ID, (qapi_WLAN_P2P_Opps_Params_t *)data);
            break;
        }
        case __QAPI_WLAN_PARAM_GROUP_P2P_NOA_PARAMS: {
            ret = wlan_p2p_set_noa(device_ID, (qapi_WLAN_P2P_Noa_Params_t *)data);
            break;
        }

        case __QAPI_WLAN_PARAM_GROUP_P2P_GO_PARAMS: {
            qapi_WLAN_P2P_Go_Params_t *go = (qapi_WLAN_P2P_Go_Params_t *)data;
            ret = wlan_p2p_set_pass_ssid(device_ID, go);
            break;
        }

        case __QAPI_WLAN_PARAM_GROUP_P2P_LISTEN_CHANNEL: {
            qapi_WLAN_P2P_Listen_Channel_t *listen_channel = (qapi_WLAN_P2P_Listen_Channel_t *)data;
            /* The listen channel can be one of the social channels only */
            if ((listen_channel->channel != 1) && (listen_channel->channel != 6) && (listen_channel->channel != 11)) {
                return QAPI_ERROR;
            }
            ret = wlan_p2p_set(device_ID, WMI_P2P_CONFID_LISTEN_CHANNEL, data, length);
            break;
        }
        case __QAPI_WLAN_PARAM_GROUP_P2P_SSID_POSTFIX: {
            ret = wlan_p2p_set(device_ID, WMI_P2P_CONFID_SSID_POSTFIX, data, length);
            break;
        }
        case __QAPI_WLAN_PARAM_GROUP_P2P_INTRA_BSS: {
            ret = wlan_p2p_set(device_ID, WMI_P2P_CONFID_INTRA_BSS, data, length);
            break;
        }
        case __QAPI_WLAN_PARAM_GROUP_P2P_CONCURRENT_MODE: {
            ret = wlan_p2p_set(device_ID, WMI_P2P_CONFID_CONCURRENT_MODE, data, length);
            break;
        }
        case __QAPI_WLAN_PARAM_GROUP_P2P_GO_INTENT: {
            ret = wlan_p2p_set(device_ID, WMI_P2P_CONFID_GO_INTENT, data, length);
            break;
        }
        case __QAPI_WLAN_PARAM_GROUP_P2P_DEV_NAME: {
            ret = wlan_p2p_set(device_ID, WMI_P2P_CONFID_DEV_NAME, data, length);
            break;
        }
        case __QAPI_WLAN_PARAM_GROUP_P2P_OP_MODE: {
            ret = wlan_p2p_set(device_ID, WMI_P2P_CONFID_P2P_OPMODE, data, length);
            break;
        }
        case __QAPI_WLAN_PARAM_GROUP_P2P_CCK_RATES: {
            ret = wlan_p2p_set(device_ID, WMI_P2P_CONFID_CCK_RATES, data, length);
            break;
        }
        case __QAPI_WLAN_PARAM_GROUP_P2P_DISCOVERABLE_INTERVAL: {
            ret = wlan_p2p_set(device_ID, WMI_P2P_CONFID_DISC_INT, data, length);
            break;
        }

        default: /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY + param_ID */
            PRINT_ERR_INVALID_PARAM1("param_ID", param_ID);
            ret = QAPI_WLAN_ERR_EINVAL;
        }
        break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY */
    }
#endif // CONFIG_ENABLE_P2P_MODE

    default: /* group_ID */
        PRINT_ERR_INVALID_PARAM1("group_ID", group_ID);
        ret = QAPI_WLAN_ERR_EINVAL;
        break;
    } /* group_ID */
    return ret;
}

qapi_Status_t qapi_WLAN_Get_Param(uint8_t __attribute__((__unused__)) device_ID, uint16_t group_ID, uint16_t param_ID,
                                  void *data, uint32_t *length)

{
    qapi_Status_t ret = QAPI_OK;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    if (gp_wlan_qapi_cxt->wlanEnabled == false) {
        warn_printf("wlan is not enabled\n");
        return QAPI_ERROR;
    }

    if (!data || !length || !*length) {
        return QAPI_WLAN_ERR_EINVAL;
    }

    switch (group_ID) {
    case __QAPI_WLAN_PARAM_GROUP_WIRELESS: {
        switch (param_ID) {
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE: {
            qapi_WLAN_DEV_Mode_e *mode = (qapi_WLAN_DEV_Mode_e *)data;
            if (*length < sizeof(qapi_WLAN_DEV_Mode_e)) {
                return QAPI_WLAN_ERR_EINVAL;
            }
            if (p_cxt->opmode == WHAL_M_AP)
                *mode = DEV_MODE_AP_E;
            else
                *mode = DEV_MODE_STATION_E;
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_CONCURRENCY_MODE: {
            qapi_WLAN_DEV_Mode_e *conc_mode = (qapi_WLAN_DEV_Mode_e *)data;
            if (*length < sizeof(qapi_WLAN_DEV_Mode_e)) {
                return QAPI_WLAN_ERR_EINVAL;
            }
            if (p_cxt->conc_mode == WHAL_M_AP_STA)
                *conc_mode = DEV_MODE_AP_STA_E;
            else
                *conc_mode = DEV_MODE_NO_CONC_E;
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_CONCURRENCY_MODE */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_MAC_ADDRESS: {
            if (*length < __QAPI_WLAN_MAC_LEN) {
                return QAPI_WLAN_ERR_EINVAL;
            }
            wlan_get_mac_address(device_ID, data);
            // memcpy(data, p_cxt->dev_common->devp[device_ID]->ic_myaddr, __QAPI_WLAN_MAC_LEN);//TODO
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_MAC_ADDRESS */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_POWER_MODE_PARAMS: {
            uint8_t *powermode = (uint8_t *)data;
            if (*length < sizeof(uint8_t)) {
                return QAPI_WLAN_ERR_EINVAL;
            }
            wlan_get_power_mode(device_ID, powermode);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_POWER_MODE_PARAMS */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_PHY_MODE: {
            uint8_t *phymode = (uint8_t *)data;
            if (*length < sizeof(uint8_t)) {
                return QAPI_WLAN_ERR_EINVAL;
            }
            wlan_get_phy_mode(phymode);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_PHY_MODE */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_COUNTRY_CODE: {
            char *country_code = (char *)data;
            if (*length < 4) {
                return QAPI_WLAN_ERR_EINVAL;
            }
            memscpy(country_code, 3, p_cxt->country_code, 3);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_COUNTRY_CODE */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_RSSI: {
            uint8_t *rssi = (uint8_t *)data;
            if (*length < sizeof(uint8_t)) {
                return QAPI_WLAN_ERR_EINVAL;
            }
            ret = wlan_sta_get_rssi(device_ID, rssi);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_RSSI */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_LISTEN_INTERVAL_IN_TU: {
            uint32_t *interval = (uint32_t *)data;
            if (*length < sizeof(uint32_t)) {
                return QAPI_WLAN_ERR_EINVAL;
            }
            ret = wlan_get_sta_slptime(interval);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_LISTEN_INTERVAL_IN_TU */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_RTS: {
            uint32_t *enable = (uint32_t *)data;
            if (*length < sizeof(uint32_t)) {
                return QAPI_WLAN_ERR_EINVAL;
            }
            wlan_get_rts_cts(enable);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_RTS */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_RTS_RATE_2G: {
            uint32_t *rate = (uint32_t *)data;
            if (*length < sizeof(uint32_t)) {
                return QAPI_WLAN_ERR_EINVAL;
            }
            ret = wlan_get_rts_rate(rate);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_RTS_RATE_2G */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_EDCA_PARAM: {
            qapi_WLAN_Edca_Params_t *edca_para = (qapi_WLAN_Edca_Params_t *)data;
            if (*length < sizeof(uint32_t)) {
                return QAPI_WLAN_ERR_EINVAL;
            }
            ret = wlan_get_edca_param(edca_para->qid, &edca_para->aifsn, &edca_para->cw_min, &edca_para->cw_max,
                                      &edca_para->txop_limit);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_CONTENTION_WINDOW */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_PER_UPPER_THRESHOLD: {
            uint32_t *threshold = (uint32_t *)data;
            if (*length < sizeof(uint32_t)) {
                return QAPI_WLAN_ERR_EINVAL;
            }
            wlan_get_per_upper_threshold(threshold);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_PER_UPPER_THRESHOLD */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_BA_WINDOW: {
            qapi_WLAN_BA_Window_Params_t *ba_win = (qapi_WLAN_BA_Window_Params_t *)data;
            if (*length < sizeof(uint32_t)) {
                return QAPI_WLAN_ERR_EINVAL;
            }
            wlan_get_ba_win_size(&ba_win->ack_timeout, &ba_win->delay);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_BA_WINDOW */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_SLOT_TIME: {
            uint32_t *slot_time = (uint32_t *)data;
            if (*length < sizeof(uint32_t)) {
                return QAPI_WLAN_ERR_EINVAL;
            }
            wlan_get_slot_time(slot_time);
            break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_SLOT_TIME */
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_EDCCA_THRESHOLD: {
            uint8_t *edcca_threshold = (uint8_t *)data;
            if (*length < sizeof(uint8_t)) {
                return QAPI_WLAN_ERR_EINVAL;
            }
            wlan_get_edcca_threshold(edcca_threshold);
            break;
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_TX_POWER_IN_DBM: {
            qapi_WLAN_Get_Power_Evt_t *power = (qapi_WLAN_Get_Power_Evt_t *)data;
            wlan_get_tx_power(power);
            break;
        }
        case __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_BMISS_CONFIG: {
            uint8_t *bmiss_threshold = (uint8_t *)data;
            if (*length < sizeof(uint8_t)) {
                return QAPI_WLAN_ERR_EINVAL;
            }
            wlan_get_bmiss_threshold(bmiss_threshold);
            break;
        }
        default: /* __QAPI_WLAN_PARAM_GROUP_WIRELESS + param_ID */
            PRINT_ERR_INVALID_PARAM1("param_ID", param_ID);
            ret = QAPI_WLAN_ERR_EINVAL;
        }
        break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS */
    }
    case __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY: {
        switch (param_ID) {
        case __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE: {
            uint8_t authMode = p_cxt->connect_cmd.authMode;
            uint8_t pairwiseCryptoType = p_cxt->connect_cmd.pairwiseCryptoType;
            qapi_WLAN_Auth_Mode_e *p_e_wpa_ver = (qapi_WLAN_Auth_Mode_e *)data;
            if (*length < sizeof(qapi_WLAN_Auth_Mode_e)) {
                return QAPI_WLAN_ERR_EINVAL;
            }
            switch (authMode) {
            case WMI_NONE_AUTH:
                if (pairwiseCryptoType == NONE_CRYPT) {
                    *p_e_wpa_ver = QAPI_WLAN_CRYPT_NONE_E;
                } else if (pairwiseCryptoType == WEP_CRYPT) {
                    *p_e_wpa_ver = QAPI_WLAN_AUTH_WEP_E;
                }
                *length = sizeof(qapi_WLAN_Auth_Mode_e);
                break;
            case WMI_WPA_PSK_AUTH:
                *p_e_wpa_ver = QAPI_WLAN_AUTH_WPA_PSK_E;
                *length = sizeof(qapi_WLAN_Auth_Mode_e);
                break;
            case WMI_WPA2_PSK_AUTH:
                *p_e_wpa_ver = QAPI_WLAN_AUTH_WPA2_PSK_E;
                *length = sizeof(qapi_WLAN_Auth_Mode_e);
                break;
            default:
                // skip
                break;
            }
            break; /* __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE */
        }
        default: /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY + param_ID */
            PRINT_ERR_INVALID_PARAM1("param_ID", param_ID);
            ret = QAPI_WLAN_ERR_EINVAL;
        }
        break; /* __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY */
    }

#ifdef CONFIG_ENABLE_P2P_MODE
    case __QAPI_WLAN_PARAM_GROUP_P2P: {
        switch (param_ID) {
        case __QAPI_WLAN_PARAM_GROUP_P2P_NODE_LIST: {
            qapi_WLAN_P2P_Node_List_Params_t *pNodeList = (qapi_WLAN_P2P_Node_List_Params_t *)data;
            ret = wlan_p2p_get_node_list(device_ID, pNodeList);
            break;
        }
        case __QAPI_WLAN_PARAM_GROUP_P2P_NETWORK_LIST: {
            qapi_WLAN_P2P_Network_List_Params_t *pNetworkList = (qapi_WLAN_P2P_Network_List_Params_t *)data;
            ret = wlan_p2p_get_network_list(device_ID, pNetworkList->network_List_Buffer, pNetworkList->buffer_Length);
            break;
        }
        default:
            PRINT_ERR_INVALID_PARAM1("param_ID", param_ID);
            ret = QAPI_WLAN_ERR_EINVAL;
            break;
        }
        break;
    }
#endif // CONFIG_ENABLE_P2P_MODE

    default: /* group_ID */
        PRINT_ERR_INVALID_PARAM1("group_ID", group_ID);
        ret = QAPI_WLAN_ERR_EINVAL;
        break;
    } /* group_ID */
    return ret;
}
