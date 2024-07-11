

/*
 * This file contains the definitions of the WMI protocol specified in the
 * Wireless Module Interface (WMI).  It includes definitions of all the
 * commands and events. Commands are messages from the host to the WM.
 * Events and Replies are messages from the WM to the host.
 *
 * Ownership of correctness in regards to commands
 * belongs to the host driver and the WMI is not required to validate
 * parameters for value, proper range, or any other checking.
 *
 */

#ifndef _WMI_H_
#define _WMI_H_

#include <stdio.h>
#include "wifi_cmn.h"
#include "wlan_defs.h"
#include "nt_common.h"
#include "ieee80211_defs.h"
#include "neutrino_startpack.h"
#include "wifi_cmn.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"


#define WMI_MAX_KEY_LEN       32
#define WMI_MAX_SSID_LEN      32
#define WMI_PASSPHRASE_LEN    64
#define WMI_PMK_LEN           32
#define WMI_GMK_LEN           32
#define WMI_CHANNEL_NUM_MAX   TOT_MAX_CHANNEL_INDEX + 1

typedef PREPACK struct {
	uint8_t *wur_buffer;
	uint8_t wur_buffer_len;
}POSTPACK WMI_WURID_CMD;

#define WMI_MIN_KEY_INDEX   0
#define WMI_MAX_KEY_INDEX   3
#define WPS_UUID_LEN       16
/*
 * List of Commnands
 */
typedef enum {
    WMI_CONNECT_CMDID = 0x0001,
    WMI_DISCONNECT_CMDID,
    WMI_START_SCAN_CMDID,
    WMI_SET_BG_SCAN_PARAMS_CMDID,
    WMI_GET_ROAM_TBL_CMDID,
    WMI_SET_PROBED_SSID_CMDID,
    WMI_GET_CHANNEL_LIST_CMDID,
    WMI_SET_CHANNEL_PARAMS_CMDID,
	WMI_SET_POWER_PARAMS_CMDID,
    WMI_SET_POWERSAVE_TIMERS_POLICY_CMDID, //0x0A
    WMI_ADD_CIPHER_KEY_CMDID,
    WMI_SET_TKIP_COUNTERMEASURES_CMDID,
    WMI_RSSI_THRESHOLD_PARAMS_CMDID,
    WMI_SET_ACCESS_PARAMS_CMDID,
    WMI_SET_MAX_SP_LEN_CMDID,

    WMI_SET_WMM_CMDID,
    WMI_SET_WMM_TXOP_CMDID,

    /*
     * AP mode commands
     */
    WMI_AP_CONFIG_COMMIT_CMDID,
    WMI_AP_SET_MLME_CMDID,
    WMI_AP_SET_COUNTRY_CMDID, //0x14=20
    WMI_AP_SET_DTIM_CMDID,

    WMI_ALLOW_AGGR_CMDID,
    WMI_ADDBA_REQ_CMDID,
    WMI_DELBA_REQ_CMDID,
    WMI_SET_HT_CAP_CMDID,
    WMI_SET_HT_OP_CMDID,

    WMI_SET_CHANNEL_CMDID,
    WMI_SET_PASSPHRASE_CMDID,

    WMI_AP_SET_APSD_CMDID,
    WMI_AP_APSD_BUFFERED_TRAFFIC_CMDID, //0x1E=30
    /* 802.11w cmd */
    WMI_SET_RSN_CAP_CMDID,
	/*WPA IE enable*/
	WMI_EN_WPA_OUI_CMDID,
	WMI_SEND_BC_FRM,
    WMI_WNM_SLEEP_CMDID,
    WMI_WNM_TIM_CMDID,

    WMI_GET_COUNTRY_CODE_CMDID,
    WMI_GET_WLAN_PROD_STATS_CMDID,

    WMI_WNM_CONFIG_CMDID,
    WMI_WNM_MAX_IDLE_PERIOD_CMDID,
    WMI_WNM_SLEEP_RESPONSE_CMDID, //0x28-40

    WMI_SET_STA_COUNTRY_CODE_CMDID,

    WMI_GET_STA_DTIM_CMDID,

	//AWS Support
	WMI_WLAN_ON_CMDID,
	WMI_WLAN_OFF_CMDID,
	WMI_SET_MODE_CMDID,
	WMI_CONFIG_AP_CMDID,
	WMI_WLAN_TARGET_RESET_CMDID,
	WMI_DPM_DATA_AVAIL_NOTIFICATION_CMDID,
	WMI_RX_MGMT_FRAME_CMDID,
	WMI_RX_EAP_FRAME_CMDID, //0x32=50
	WMI_PRE_BEACON_INTR_CMDID,
	WMI_MIC_ERROR_INTR_CMDID,
	WMI_BAD_DECRYPT_ERROR_INTR_CMDID,
	WMI_STA_IDLE_TIMER_CMDID,
	WMI_SET_FORCE_DTIM,
	WMI_SET_WEP_KEY_IND_DEF_CMDID,
	WMI_GET_WLAN_80211_STAT_CMDID,
	WMI_GET_RETURN_STATUS_CMDID,
	WMI_SET_IP_PRECEDENCE_CMDID,
	WMI_SET_CONFIG_CMDID, //0x3c=60
	WMI_WUR_INFO_CMDID,
	WMI_WUR_TSF_MATCH,
	WMI_WUR_TSF_BEACON_MATCH,
/*	WMI_WUR_WAKE_UP_FRAME_RECEIVED,
	WMI_WUR_VENDOR_FRAME_RECEIVED,*/
	WMI_WUR_FRAME_RECEIVED,
	WMI_GET_WUR_CMDID,
	WMI_SET_VENDOR_CMDID,
	WMI_GET_CONFIG_CMDID,
	WMI_NTENABLE_CMDID,
	WMI_RA_CFG_CMDID,
    WMI_IMPS_CONFIG_CMDID, //0x46=70
	WMI_POWER_SLEEP_MODE_CMDID,				// to select clockgated or mcu sleep
	WMI_TIMEDOUT_HNDL_CMDID,
	WMI_WUR_STATS_CMDID,					///< to dispaly wur statistics
	WMI_WUR_ERROR_FRAME_RECEIVED_CMDID,		///< to receive error frame
	WMI_SET_BEACON_MISS_THRESHOLD,          /**< wmi id for setting beacon miss threshold*/
	WMI_WUR_BEACON_MISS_CMDID,				///< to handle beacon miss

//#ifdef NT_FN_FTM
	// FTM Support
	WMI_SET_LOCATION_ON,
	WMI_SET_LOCATION_OFF,
	WMI_SET_FTM_ON,
	WMI_SET_FTM_OFF, //0x50=80
	WMI_SEND_FTM_FRAME,
	WMI_SET_LOCATION_CONFIGURE,
	WMI_SET_RTT_CFG,
	WMI_RTT_T2_AVAIL,		/*t2_capture*/
	WMI_RTT_T4_AVAIL,		/*t4_capture*/
        WMI_FTM_INITIAL_CFG,

	WMI_SET_STA_UAPSD_CMDID,
	WMI_SET_AP_UAPSD_CMDID,
	WMI_SET_WUR_ENABLE_CMDID,				//?< to enable/disable wur feature
	WMI_PUT_DEVICE_INTO_SLEEP,              /**< 0x5a=90 used to put device back into sleep when supporting uapsd */
	WMI_UAPSD_TRIGGER_TIMER_ENABLE,         /**< start or stop uapsd trigger timer */
	WMI_DPM_HAL_STATS_CMDID,			/** to show wifi statistics*/
	WMI_SET_PROTECTION_CMDID,
	WMI_EOSP_INTERRUPT_PROCESS_CMDID,              /**< used to put device back into sleep when supporting uapsd */
	WMI_UAPSD_TRIGGER_TIMER_ENABLE_CMDID,         /**< start or stop uapsd trigger timer */
	WMI_OMPS_CMDID,							//	To goto omps mode
	WMI_SET_WNM_ENABLE_CMDID,        	 	/**< to enable/disable WNM feature */
	WMI_SET_BSS_IDLE_TIME_CMDID,        	/**< Set bss idle time */
	WMI_SET_SLEEP_TIME_CMDID,				/**< Set sleep time interval for wnm sta */
	WMI_SET_WNM_DTIM_STATE_CMDID, //0x64=100
	WMI_SHOW_WNM_STATS_CMDID,				/**< Display wnm stats */
	WMI_DPM_DATA_AVAIL_INTR_BY_AP_CMDID,	/**< Data available interrupt from AP */
	WMI_DPM_DATA_RCV_NOTIFICATION_CMDID,
	WMI_BMPS_EXIT_CMDID,
	WMI_WPS_START_CMDID,
	WMI_GET_WPS_STATUS_CMDID,
//#ifdef CONFIG_P2P_FW_OFFLOAD
	WMI_P2P_PERSISTENT_PROFILE_CMDID,
//#endif
	WMI_WPS_SET_CONFIG_CMDID,
	WMI_WPS_SET_CONFIG_STATE_CMDID,
	WMI_ENABLE_XPA, //0x6e=110
	WMI_ENABLE_AUTO_BA,
	WMI_SET_TWT_RESP_CMDID,				/**< Set sleep time interval for twt sta */
	WMI_SET_TWT_ENABLE_CMDID,				/**< Set sleep time interval for wnm sta */
	WMI_SHOW_TWT_STATS_CMDID,			/**< Display twt stats */
	WMI_WAKEUP_TWT_CMDID,
	WMI_SLEEP_TWT_CMDID,
	WMI_SET_CONFIGURE_TWT_CMDID,
	WMI_SET_TWT_ALIGNMENT_CMDID,
	WMI_SET_TWT_DTIM_STATE_CMDID,
	WMI_PKT_THLD_NOTIFY_CMID, //0x78=120
	WMI_SET_RATE,
	WMI_CFG_RATE_IDX,
    WMI_UNIT_TEST_CMDID,
	WMI_IF_ADD_CMDID,
	WMI_SET_PDEV_PARAM_CMDID,
	WMI_SCAN_STOP_CMDID,
	WMI_PDEV_UTF_CMDID,
	WMI_MODE_CMDID,
	WMI_COEX_RRM_LO_THRESH_EVENT,
	WMI_COEX_RRM_HI_THRESH_EVENT, //0x82=130
	WMI_COEX_CRIT_PROTO_START,
	WMI_COEX_CRIT_PROTO_STOP,
	WMI_COEX_GPM_RX_EVENT,
	WMI_COEX_GPM_FULL_EVENT,
	WMI_COEX_PROCESS_BAND_CHANGE,
    WMI_TWT_SETUP_CMDID,
    WMI_TWT_TEARDOWN_CMDID,
    WMI_TWT_STATUS_CMDID,
    WMI_PHYDBGDUMP_CMD,
    WMI_UPDATE_BI_CMDID, //0x8c=140
    WMI_SET_RESET_WAKELOCK_CMDID,
    WMI_PERIODIC_TSF_SYNC_CMDID,
	WMI_HANDLE_ECSA_TIMER_CMDID,
	WMI_HANDLE_ECSA_TIMER_AP_TEST_CMDID,
    WMI_F2A_PULSE_ON_TWT_WAKEUP_CMDID,
    WMI_ENTER_IMPS_CMDID,
    WMI_UPDATE_BMTT_CMDID,
    WMI_CXC_CMDID,
    WMI_TX_COMPLETE_WQ_PROCESSING_CMD,
    WMI_TX_COMPLETE_TMR_EXPIRY_CMD, //0x96=150
	WMI_PERIODIC_TRAFFIC_SETUP_CMDID, /* Setup periodic traffic params and start periodic wake and sleep session */
	WMI_PERIODIC_TRAFFIC_STATUS_CMDID, /* Get periodic traffic params e.g wake interval, next_sp_tsf, session id and traffic type*/
	WMI_PERIODIC_TRAFFIC_TEARDOWN_CMDID, /* Teardown periodic traffic session */
	WMI_PERIODIC_TRAFFIC_PRINT_PM_STATS_CMDID, /* Print useful power stats in periodic wake session for debugging */
    WMI_PERIODIC_TRAFFIC_WAKEUP_CMDID, /* process periodic traffic wake up and start SP */
	WMI_CLK_LATENCY_CMDID, /* Set the clock latency during the various sleep modes */
	WMI_SLP_CLK_CAL_CFG_CMDID,
	WMI_BMPS_ENABLE_CMDID,
	WMI_BMPS_IGNORE_BCMC_CMDID,
	WMI_BMPS_TIMING_CFG_CMDID, //0xa0=160
	WMI_IMPS_CFG_CMDID,
	WMI_GET_STATISTICS_CMDID,
    WMI_SLEEP_CLK_CAL_DONE_CMDID, /* Updates the xocnt based on current temperature from the sleep clock cal results */
    WMI_SLP_CLK_CAL_ACT_CMDID, /* To enable/disable slp clk cal in active mode */
    WMI_GET_REGULATORY_CMDID,
    WMI_HALPHY_ANI_PERIODIC_CB_CMDID, /* Handles ANI periodic monitor activities at poll timer expiry */
    WMI_HALPHY_TEMP_CMP_PERIODIC_CB_CMDID, /*  Handles periodic temperature monitor activities for SCPC compensation*/
    WMI_GET_RATE,
    WMI_CMD_MAX, /* Note: This cmd should be the last in the WMI_COMMAND_ID ENUM */
} WMI_COMMAND_ID;

#ifdef CONFIG_WMI_EVENT
typedef enum {
    WMI_DISCONNECT_EVTID = 0x1,
    WMI_CONNECT_FAIL_EVTID,
    WMI_CONNECT_SUCCESS_EVTID,
    WMI_ROAM_SUCCESS_EVTID,
    WMI_WIFI_EN_EVTID,
    WMI_WIFI_DIS_EVTID,
    WMI_WIFI_SET_MODE_EVTID,
    WMI_SCAN_START_EVTID,
    WMI_SCAN_STOP_EVTID,
    WMI_SCAN_COMP_EVTID,
    WMI_COEX_EVTID, //0x0A
    WMI_TWT_SETUP_EVTID,
    WMI_TWT_TEARDOWN_EVTID,
    WMI_TWT_STATUS_EVTID,
    WMI_WUR_EVTID,
    WMI_IP_DHCP_SUCCESS_EVTID,
    WMI_IP_ADDR_READY_EVTID,
    WMI_IP_PING_EVTID,
    WMI_NETIF_ADD_EVTID,
    WMI_SET_PARAM_EVENT_ID,
    WMI_REPORT_STATISTICS_EVTID,
    WMI_REGULATORY_EVTID,
    WMI_SET_RATE_EVTID,
    WMI_GET_RATE_EVTID,
    WMI_CHAN_SWITCH_EVTID,
	WMI_SCAN_RESULT_EVTID,
    WMI_MAX_EVTID,
} WMI_EVENTT_ID;

#define WMI_LOCAL_EVT_FLAG_HEAP    0x00000001

typedef struct {
    void* pwmi_evt_data;
    WMI_EVENTT_ID wmi_evt_id;
    uint32_t flag;
} wmi_evt_struct_t;

typedef PREPACK struct
{
    uint8_t reserved1;
    uint8_t netif_id;
    uint8_t dhcp_type; /* check ringif_dhcp_type */
    uint8_t ip_ver;    /* check ringif_ipaddr_type */

    uint32_t ipv4_addr;     /* If ip_ver is IP_VER_V4 */
    uint32_t ipv6_addr[4];  /* If ip_ver is IP_VER_V6 */
} POSTPACK WMI_IP_DDR_EVT;

typedef struct {
    uint8_t net_id; /* network interface id */
    uint8_t status; /* status code */
}POSTPACK WMI_NETIF_ADD_EVT;

typedef struct {
    int32_t status; /* status code for success or faliure */
    int32_t reason_code; /* reason code for status */
    uint32_t assoc_id; /* association id */
    uint16_t channel_frequency; /*frequency of current channel*/
    uint8_t bssid[IEEE80211_ADDR_LEN]; /* bssid of the ap joined */
    ssid_t ssid; /*ssid of joind AP */
}POSTPACK WMI_JOIN_EVT;

typedef struct {
    uint32_t assoc_id; /* association id */
    int32_t reason; /* status code */
}POSTPACK WMI_DISC_EVT;
#endif

typedef enum {
	INVALID_CRYPT       = 0x00,
    NONE_CRYPT          = 0x01,
    WEP_CRYPT           = 0x02,
    TKIP_CRYPT          = 0x04,
    AES_CRYPT           = 0x08,
	BIP_CRYPT			= 0x10,
} CRYPTO_TYPE;

/* Each of these timeout event id should correspond with the
 * timeout handlers defined in wmi_timer_disp_hnd_t
 */

typedef enum  //@Wmi generic timedout handler events
{
	cm_connect_timeout_evntid = 0x0,
	pm_prob_res_timout_evntid,
	dc_deterministic_scan_timeout_evntid,
	dc_min_chdwell_timeout_evntid,
	ap_check_sta_inactivity_evntid,
#ifdef NT_FN_AMPDU
	TXRXAddBaRespTimeout_evntid,
#endif //NT_FN_AMPDU
	TXRX_reset_mic_err_cnt_timeout_handler_evntid,
	TXRX_enable_assoc_timeout_handler_evntid,
	wlan_beacon_bmiss_handler_evntid,
	pmIdleTimeoutFunc_evntid,
	pmPspollTimeoutFunc_evntid,
#ifdef NT_FN_RA
	ra_inspect_rates_evntid,
#endif //NT_FN_RA
#if ((defined NT_FN_RMF) || (defined NT_FN_WPA3))
	mlme_assoc_comeback_timeout_handler_evntid,
#endif
	mlme_mgmt_rsp_timeout_handler_evntid,
	sec_tkip_cm_timeout_evntid,
	sec_timeout_evntid,
#ifdef NT_FN_ROAMING
	ro_periodic_search_timeout_evntid,			//Roaming periodic search timeout event
#endif //NT_FN_ROAMING

#ifdef NT_FN_WNM_POWERSAVE_MODE
	nt_wnm_bss_idle_timeout_cb_evntid,			///< bss idle timeout event id for wmi
#endif	/*  NT_FN_WNM_POWERSAVE_MODE */
#ifdef NT_FN_WMM_PS_STA
	nt_be_trigger_tmr_expiry_cb_evntid,
	nt_bk_trigger_tmr_expiry_cb_evntid,
	nt_vi_trigger_tmr_expiry_cb_evntid,
	nt_vo_trigger_tmr_expiry_cb_evntid,
	nt_be_eosp_tmr_expiry_cb_evntid,
	nt_bk_eosp_tmr_expiry_cb_evntid,
	nt_vi_eosp_tmr_expiry_cb_evntid,
	nt_vo_eosp_tmr_expiry_cb_evntid,
#endif //NT_FN_WMM_PS_STA
#ifdef NT_FN_FTM_2016V
	nt_ftm_partial_tsf_bur_dur_rep_start_asap_cb_evntid,
	nt_ftm_partial_tsf_bur_start_non_asap_timeout_msg_cb_evntid,
	nt_ftm_tsf_corrected_burst_period_expiry_asap_cb_evntid,
	nt_ftm_burst_period_expiry_cb_evntid,
	nt_ftm_burst_duration_expiry_cb_evntid,
#endif //NT_FN_FTM_2016V
#ifdef NT_FN_FTM
	nt_ftm_min_delta_expiry_cb_evntid,
#endif //NT_FN_FTM
#ifdef CONFIG_CHANNEL_SCHEDULER
        co_op_dwell_timeout_eventid,
        co_scheduler_timeout_eventid,
        co_pm_event_timeout_eventid,
        co_fake_sleep_timeout_eventid,
        co_wait_for_cts_timeout_eventid,
#endif /* CONFIG_CHANNEL_SCHEDULER */
#ifdef SUPPORT_TWT_AP
    twt_ap_sp_end_timeout,
    twt_ap_sp_start_timeout,
#endif
#ifdef SUPPORT_COEX
	wlan_dhcp_timeout_handler_evntid,
	coex_prio_timeout_eventid,
#ifdef PLATFORM_FERMION
	coex_mci_recovery_timeout_eventid,
#ifndef EMULATION_BUILD
	coex_wsi_state_timeout_eventid,
	cxc_reset_bt_event_timeout_eventid,
#endif
#endif
#endif
#ifdef SUPPORT_TWT_STA
    twt_sta_twt_setup_timeout,
#endif
#ifdef FEATURE_PERIODIC_WAKE_SLEEP
	/*Periodic traffic idle timer timeout event*/
	periodicTrafficIdleTimer_eventid,
#endif
	invalid_evntid = 0xff
}wmi_tmdout_evnthndl_t;

/*
 * Connect Command
 */
typedef enum {
    INFRA_NETWORK       = 0x01,				//STA Mode
    AP_NETWORK          = 0x10,				//AP Mode
#ifdef NT_FN_CONCURRENCY
	AP_STA_NETWORK		= 0x11,				//AP_STA Concurrency Mode
#endif //NT_FN_CONCURRENCY
    NETWORK_CONNECTED_USING_WPS = 0x20
} NETWORK_TYPE;

typedef enum {
    OPEN_AUTH           = 0x01,
    SHARED_AUTH         = 0x02,
    LEAP_AUTH           = 0x04,  /* different from IEEE_AUTH_MODE definitions */
	SAE_AUTH			= 0x08
} DOT11_AUTH_MODE;

typedef enum {
    WMI_NONE_AUTH           = 0x01,
    WMI_WPA_AUTH            = 0x02,
    WMI_WPA2_AUTH           = 0x04,
    WMI_WPA_PSK_AUTH        = 0x08,
    WMI_WPA2_PSK_AUTH       = 0x10,
    WMI_WPA_AUTH_CCKM       = 0x20,
    WMI_WPA2_AUTH_CCKM      = 0x40,
    WMI_WPA3_SHA256_AUTH 	= 0x80,
} AUTH_MODE;

typedef PREPACK struct {
    uint8_t     networkType;
    uint8_t     dot11AuthMode;
    uint8_t     authMode;
    uint8_t     pairwiseCryptoType;
    uint8_t     pairwiseCryptoLen;
    uint8_t     groupCryptoType;
    uint8_t     groupCryptoLen;
    uint8_t     ssidLength;
    unsigned char     ssid[WMI_MAX_SSID_LEN + 1];
    uint8_t     num_channels;                       /* how many channels to scan */
    uint8_t     channel_list[WMI_CHANNEL_NUM_MAX];  /* channels in Mhz */
    uint8_t     bssid[IEEE80211_ADDR_LEN];
    uint32_t    ctrl_flags;
#ifdef SUPPORT_5GHZ
    uint8_t     wlan_mode;
#endif
} POSTPACK WMI_CONNECT_CMD;

typedef PREPACK struct {
    unsigned char countryCode[3];
} POSTPACK WMI_AP_SET_COUNTRY_CMD;


typedef PREPACK struct
{
	   int8_t enableDisc;
	   int32_t servicePeriod;
	   int32_t serviceInterval;
	   int32_t beaconPeriod;
	   int8_t beaconOffset;
	   int8_t wurMcEnable;
	   int8_t wurEnable;
	   uint16_t ven_id_1;
	   uint16_t ven_id_2;
} POSTPACK WMI_SET_CONFIG_CMD;

/**
 * WMI_WNM_CONFIG_CMD
 */
typedef PREPACK struct
{
	   int8_t wnm_enable;			///< wnm enable flag
	   uint16_t bss_max_idle_time;	///< bss idle time
	   uint16_t sleep_interval;		///< sleep time
	   uint8_t wnm_dtim_enable_disable_auto;
} POSTPACK WMI_WNM_CONFIG_CMD;

/**
 * WMI_TWT_CONFIG_CMD
 */
typedef struct
{
	uint16_t resp_type;		///< response type of twt
	uint8_t twt_type;		//individual/broadcast
	uint64_t twt_wake_interval;	//span of time between first wake up and next wake up
	uint32_t twt_min_wake_duration;//span of time between point of wake up and point of sleep
	uint8_t twt_wakeup_type; //implicit/explicit
	uint8_t twt_flow_type;//announced /unannounced
	uint16_t twt_alignment;
	uint8_t twt_dtim_enable_disable_auto;
}WMI_TWT_CONFIG_CMD;

typedef PREPACK struct
{
	   void* get_wur_cfg_inf;
	   void* get_wmm_cfg_inf;
	   void* get_rmf_cfg_inf;
} POSTPACK WMI_GET_CONFIG_CMD;

//#ifdef SUPPORT_UNIT_TEST_CMD

#define WMI_UNIT_TEST_ARGS_MAX (64-1)  //As control interface support 256 bytes, Hence 256/4 args

typedef PREPACK struct
{
    uint8_t vdev_id;
    uint8_t module_id;
    uint8_t num_args;
    uint8_t reserved;
    uint32_t args[WMI_UNIT_TEST_ARGS_MAX];
}POSTPACK WMI_UNIT_TEST_CMD;

//#endif

typedef PREPACK struct
{
	   uint16_t id;
	   uint16_t sec_id;
	   uint16_t type;
	   uint32_t data;
	   uint8_t wur_prot_flg;
	   uint8_t sta_mac[IEEE80211_ADDR_LEN];
} POSTPACK WMI_WUR_INF_CMD;

typedef PREPACK struct {
    uint8_t dtim;
} POSTPACK WMI_AP_SET_DTIM_CMD;

/*
 * WMI_RECONNECT_CMDID
 */
typedef PREPACK struct {
    uint16_t    channel;                    /* hint */
    uint8_t     bssid[IEEE80211_ADDR_LEN];         /* mandatory if set */
} POSTPACK WMI_RECONNECT_CMD;

/*
 * WMI_SET_PASSPHRASE_CMDID
 */
typedef PREPACK struct {
    unsigned char ssid[WMI_MAX_SSID_LEN + 1];
    uint8_t passphrase[WMI_PASSPHRASE_LEN + 1];
    uint8_t ssid_len;
    uint8_t passphrase_len;
} POSTPACK WMI_SET_PASSPHRASE_CMD;

/*
 * WMI_ADD_CIPHER_KEY_CMDID
 */
typedef enum {
    PAIRWISE_USAGE      = 0x00,
    GROUP_USAGE         = 0x01,
    TX_USAGE            = 0x02     /* default Tx Key - Static WEP only */
} KEY_USAGE;

typedef PREPACK struct {
    uint8_t     keyIndex;
    uint8_t     keyType;
    uint8_t     keyUsage;           /* KEY_USAGE */
    uint8_t     keyLength;
    uint8_t     key[WMI_MAX_KEY_LEN];
    uint8_t    key_macaddr[IEEE80211_ADDR_LEN];
} POSTPACK WMI_ADD_CIPHER_KEY_CMD;

/*
 * WMI_SET_TKIP_COUNTERMEASURES_CMDID
 */
typedef enum {
    WMI_TKIP_CM_DISABLE = 0x0,
    WMI_TKIP_CM_ENABLE  = 0x1,
} WMI_TKIP_CM_CONTROL;


typedef PREPACK struct {
    uint8_t  cm_en;                     /* WMI_TKIP_CM_CONTROL */
} POSTPACK WMI_SET_TKIP_COUNTERMEASURES_CMD;

#define WMI_PMKID_LEN 16

/*
 * WMI_START_SCAN_CMD
 */

typedef enum {
    WMI_LONG_SCAN  = 0,// scan all ssids
    WMI_SHORT_SCAN = 1 //scan specific ssids
} WMI_SCAN_TYPE;

typedef PREPACK struct {
    uint8_t  auth_mode;                          /* authentication mode */
    uint8_t  crypto_type;                        /* encryption type */
    uint8_t  group_crypto;                       /* group-crypto type*/
    uint8_t  probe_type;                         /* active or passive scan */
    uint8_t  scan_type;                          /* all ssid or specific ssid */
    ssid_t   ssid[5];                               /* profile ssid */
    uint8_t  num_channels;                       /* how many channels to scan */
    uint8_t  channel_list[WMI_CHANNEL_NUM_MAX];  /* channels in Mhz */
    uint8_t  cnt_prof;
    NT_BOOL  scan_only;                          /* do only scan - no connection */
#ifdef SUPPORT_RING_IF
    uint32_t chdwell_active_duration;
    uint32_t chdwell_passive_duration;
#endif
} POSTPACK WMI_START_SCAN_CMD;

typedef PREPACK struct {
	uint8_t      bg_scan_status;         /*enabled or disabled*/
	int32_t      bg_period;              /* frequency in seconds */
    uint8_t      full_scan_freq;         /* full scan frequency */
    uint8_t      probe_type;
    uint8_t      trigger_type;           /* rssi or rate or mixed or periodic */
    uint8_t      rssi_min_thresh;        /* minimum rssi threshold */
    uint8_t      rssi_max_thresh;        /* maximum rssi threshold */
    uint16_t     rate_min_thresh;        /* minimum rate threshold */
    uint16_t     rate_max_thresh;        /* maximum rate threshold */
    uint8_t      ssid;
    uint8_t      num_ch;
} POSTPACK WMI_BACKGROUND_SCAN_CMD;

/*
 * WMI_SET_PROBED_SSID_CMDID
 */
#define MAX_PROBED_SSID_INDEX   4

typedef enum {
    DISABLE_SSID_FLAG  = 0,                  /* disables entry */
    SPECIFIC_SSID_FLAG = 0x01,               /* probes specified ssid */
    ANY_SSID_FLAG      = 0x02,               /* probes for any ssid */
	PROFILE_SSID_FLAG  = 0x10,               /* Profile SSid Flag */
} WMI_SSID_FLAG;


typedef PREPACK struct {
	uint8_t  auth_mode;                          /* authentication mode */
	uint8_t  crypto_type;
    uint8_t  flag;                         /* WMI_SSID_FLG */
    ssid_t   ssid;                               /* profile ssid */
} POSTPACK WMI_PROBED_SSID_CMD;

typedef PREPACK struct {
    uint8_t             descType;
    uint8_t             keyInfo[2]; /* big endian */
    uint8_t             pmkid_valid;
    uint8_t             rsrv[4];
    uint8_t             pmkid[WMI_PMKID_LEN];
} POSTPACK WMI_RX_EAPOL_KEY;

typedef PREPACK struct {
	int8_t power_save_mode;
	int8_t status;
} POSTPACK WMI_POWER_SAVE;

typedef PREPACK struct {
    uint16_t    idle_period;             /* msec */
    uint16_t    pspoll_number;
    uint16_t    dtim_policy;
    uint16_t    tx_wakeup_policy;
    uint16_t    num_tx_to_wakeup;
    uint16_t    ps_fail_event_policy;
} POSTPACK WMI_POWER_PARAMS_CMD;

typedef PREPACK struct {
    uint8_t uapsd_enable; /*enable/disable AP UAPSD*/
} POSTPACK WMI_AP_UAPSD_CMD;
typedef PREPACK struct {
    uint8_t tid;              /* tid to which we want to start sending QOS NULL*/
    uint8_t enable;           /* enable or disable transmitting QOS NULL*/
    uint8_t trigger_period_length;   /* trigger time period length */
} POSTPACK WMI_STA_UAPSD_TRIGGER_TIMER_CMD;

typedef PREPACK struct {
    uint16_t    psPollTimeout;          /* msec */
    uint16_t    triggerTimeout;         /* msec */
    uint32_t    apsdTimPolicy;      /* TIM behavior with  ques APSD enabled. Default is IGNORE_TIM_ALL_QUEUES_APSD */
    uint32_t    simulatedAPSDTimPolicy;      /* TIM behavior with  simulated APSD enabled. Default is PROCESS_TIM_SIMULATED_APSD */
} POSTPACK WMI_POWERSAVE_TIMERS_POLICY_CMD;

/*
 * WMI_SET_MAX_SP_LEN_CMDID
 */
typedef enum {
    DELIVER_ALL_PKT = 0x0,
    DELIVER_2_PKT = 0x1,
    DELIVER_4_PKT = 0x2,
    DELIVER_6_PKT = 0x3,
} APSD_SP_LEN_TYPE;

/*
 * WMI_SET_STA_UAPSD_CMDID
 */
typedef PREPACK struct {
    uint8_t    staUapsd;
} POSTPACK WMI_SET_STA_UAPSD_CMD;

/*
 * WMI_SET_CHANNEL_PARAMS_CMDID
 */
typedef enum {
    WMI_11B_MODE  = 0x1,
    WMI_11G_MODE  = 0x2,
    WMI_11GN_MODE  = 0x3,
	WMI_11GONLY_MODE = 0x4
} WMI_PHY_MODE;

typedef enum {
    WMI_IGNORE_BARKER_IN_ERP = 0,
    WMI_DONOT_IGNORE_BARKER_IN_ERP
} WMI_PREAMBLE_POLICY;

/* Max number of STA */
#define AP_MAX_NUM_STA          4

/*
 * Disconnect Event
 */
typedef enum {
    NO_NETWORK_AVAIL   = 0x01,
    LOST_LINK          = 0x02,     /* bmiss */
    DISCONNECT_CMD     = 0x03,
    BSS_DISCONNECTED   = 0x04,
    AUTH_FAILED        = 0x05,
    ASSOC_FAILED       = 0x06,
    NO_RESOURCES_AVAIL = 0x07,
    CSERV_DISCONNECT   = 0x08,
    INVALID_PROFILE    = 0x0a,
    DOT11H_CHANNEL_SWITCH = 0x0b,
    PROFILE_MISMATCH   = 0x0c,
    CONNECTION_EVICTED = 0x0d,
    IBSS_MERGE         = 0x0f,
    EXCESS_TX_RETRY    = 0x10,     /* TX frames failed after excessive retries */
    SEC_HS_TO_RECV_M1  = 0x11,     /* Security 4-way handshake timed out waiting for M1 */
    SEC_HS_TO_RECV_M3  = 0x12,     /* Security 4-way handshake timed out waiting for M3 */
    TKIP_COUNTERMEASURES = 0x13,
#if ((defined NT_FN_RMF) || (defined NT_FN_WPA3))
	SA_QUERY_FAILURE   = 0x14,     /* SA Query Failed */
#endif //NT_FN_RMF
	MIC_FAILURE_4WAY_HANDSHAKE = 0x15,  /* MIC failure in 4-way handshake */
	RECEIVED_DISASSOC  = 0x16,
	RECEIVED_DEAUTH	   = 0x17,
#ifdef FEATURE_STA_ECSA
	CHANNEL_SWITCH_SUCCESS	= 0x18, /* Channel switch succesful*/
	CHANNEL_SWITCH_FAILED	= 0x19, /* Channel switch failed*/
#endif
    FOURWAY_HANDSHAKE_TIMEOUT  = 0x20,

} WMI_DISCONNECT_REASON;

/*
 * Connect Event
 */
typedef enum {
	RECEIVED_ASSOC_RESP			= 0x01, /* Recieved Assoc Response */
	FOURWAY_HANDSHAKE_SUCCESS	= 0x02, /* 4way Handshake completed */
}WMI_CONNECT_REASON;

#ifdef NT_FN_DEBUG_STATS
typedef PREPACK struct {
    uint32_t   power_save_failure_cnt;
    uint16_t   stop_tx_failure_cnt;
    uint16_t   atim_tx_failure_cnt;
    uint16_t   atim_rx_failure_cnt;
    uint16_t   bcn_rx_failure_cnt;
}POSTPACK pm_stats_t;
#endif //NT_FN_DEBUG_STATS

#if defined(NT_FN_PRODUCTION_STATS) || defined(NT_FN_DEBUG_STATS)
typedef struct{
#ifdef NT_FN_PRODUCTION_STATS
       uint16_t bmps_enter_count;
       uint16_t bmps_exit_count;
       uint16_t bmps_exit_due_to_tim;
       uint16_t bmps_exit_due_to_sta_data_availablility;
       uint16_t bmps_exit_due_to_beacon_miss;
       uint16_t bmps_exit_due_to_broadcast_msg;
#endif //NT_FN_PRODUCTION_STATS
#ifdef NT_FN_DEBUG_STATS
       uint32_t bmps_count_minimal_code_entered;
       uint32_t bmps_count_minimal_code_sleep_immediate;
       uint32_t total_awake_time;
       uint32_t total_sleep_time;
       uint64_t last_bmps_exit_time;
       uint64_t last_bmps_enter_time;
       uint64_t max_awake_time;
       uint64_t min_awake_time;
       uint32_t max_sleep_time;
       uint32_t min_sleep_time;
#endif //NT_FN_DEBUG_STATS
}pm_statistics_t;

typedef struct{
#ifdef NT_FN_PRODUCTION_STATS
       uint16_t imps_enter_count;
       uint16_t imps_exit_count;
#endif //NT_FN_PRODUCTION_STATS
#ifdef NT_FN_DEBUG_STATS
       uint32_t total_awake_time;
       uint32_t total_sleep_time;
       uint32_t last_imps_exit_time;
       uint32_t last_imps_enter_time;
       uint32_t avg_connection_time;
#endif //NT_FN_DEBUG_STATS
}imps_stats_t;

/* @struct	: wnm_ps_struct_stats_ap_t
 * @brief	: parameter for wnm power save statistics for ap
 * */
typedef struct{
#ifdef NT_FN_DEBUG_STATS
	uint16_t wnm_power_save_enter_count;		///< count for enter into wnm power save mode
	uint16_t wnm_power_save_exit_count;			///< count for exit from wnm power save mode
#endif //NT_FN_DEBUG_STATS
#ifdef NT_FN_PRODUCTION_STATS
	uint16_t wnm_power_save_total_enter_sleep_mode_req_frame_recv;	///<total enter sleep mode req recv
	uint16_t wnm_power_save_total_enter_sleep_mode_resp_frame_sent;	///<total enter sleep mode resp sent
	uint16_t wnm_power_save_total_exit_sleep_mode_req_frame_recv;	///<total exit sleep mode req recv
	uint16_t wnm_power_save_total_exit_sleep_mode_resp_frame_sent;	///<total exit sleep mode resp sent
#endif //NT_FN_PRODUCTION_STATS
}wnm_ps_struct_stats_ap_t;

/* @struct	: wnm_ps_struct_stats_sta_t
 * @brief	: parameter for wnm power save statistics for sta side
 * */
typedef struct{
#ifdef NT_FN_PRODUCTION_STATS
	uint16_t wnm_power_save_enter_count;		                ///< count for enter into wnm power save mode
	uint16_t wnm_power_save_exit_count_due_to_TIM_from_ap;		///< count for exit from wnm mode due to tim update from ap
	uint16_t wnm_power_save_exit_count_due_to_sta_data_avail;	///< count for exit from wnm mode due to data availability from sta
#endif //NT_FN_PRODUCTION_STATS
#ifdef NT_FN_DEBUG_STATS
	uint16_t wnm_power_save_total_enter_sleep_mode_req_frame_sent;		///<total enter sleep mode req sent
	uint16_t wnm_power_save_total_enter_sleep_mode_resp_frame_received;	///<total enter sleep mode resp received
	uint16_t wnm_power_save_total_exit_sleep_mode_req_frame_sent;		///<total exit sleep mode req sent
	uint16_t wnm_power_save_total_exit_sleep_mode_resp_frame_received;	///<total exit sleep mode resp recv
#endif //NT_FN_DEBUG_STATS
}wnm_ps_struct_stats_sta_t;

/* @struct	: wur_struct_stats_sta_t
 * @brief	: parameter for wur statistics for sta side
 * */
typedef struct{
#ifdef NT_FN_PRODUCTION_STATS
       uint16_t wur_enter_count;	                      ///< count for enter into wur mode
       uint16_t wur_exit_count;		                      ///< count for exit from wur mode
       uint16_t wur_wakeup_count_due_to_ap_wakeup_cmd;	  ///< count for wur wakeup frame when ap send wakeup frame
       uint16_t wur_wakeup_count_due_to_sta_wakeup_cmd;	  ///< count for wur wakeup frame when sta send wakeup frame
       uint16_t wur_wakeup_count_due_to_data_availability;	///< count for wur wakeup if data available at sta side
       uint16_t wur_wakeup_count_due_to_error_frame;
       uint16_t wur_wakeup_count_due_beacon_miss;
#endif //NT_FN_PRODUCTION_STATS
#ifdef NT_FN_DEBUG_STATS
       uint16_t wur_vendor_count;	///< count for received wur_vendor frame
       uint16_t wur_mode_setup_req_tx_count;	///< count for mode setup request
       uint16_t wur_mode_setup_resp_rx_count;	///< count for mode setup response
       uint16_t wur_mode_suspend_req_tx_count;	///< count for mode suspend request
       uint16_t wur_mode_suspend_resp_rx_count;	///< count for mode suspend response
       uint16_t wur_mode_enter_req_tx_count;	///< count for wur mode enter
#endif //NT_FN_DEBUG_STATS
}wur_struct_stats_sta_t;

/* @struct	: wur_struct_stats_ap_t
 * @brief	: parameter for wur statistics for ap
 * */
typedef struct{
#ifdef NT_FN_PRODUCTION_STATS
       uint16_t wur_wakeup_frame_sent_count;	///< count for wur wakeup frame send from ap
       uint16_t wur_mode_enter_req_rx_count;	///< count for enter mode recv request
#endif //NT_FN_PRODUCTION_STATS
#ifdef NT_FN_DEBUG_STATS
       uint16_t wur_vendor_frame_sent_count;	///< count for wur vendor frame send from ap
       uint16_t wur_mode_setup_req_rx_count;	///< count for mode setup request receive
       uint16_t wur_mode_setup_resp_tx_count;	///< count for mode setup response sent
       uint16_t wur_mode_suspend_req_rx_count;	///< count for mode suspend request receive
       uint16_t wur_mode_suspend_resp_tx_count;	///< count for mode suspend response sent
       uint16_t wur_beacon_frame_sent_count;	///< count for wur beacon send from ap
#endif
}wur_struct_stats_ap_t;
#endif //NT_FN_PRODUCTION_STATS || NT_FN_DEBUG_STATS

#ifdef NT_FN_WMM_PS_STA
#if defined(NT_FN_DEBUG_STATS) || defined(NT_FN_PRODUCTION_STATS)
typedef struct uapsd_stats_s{
#ifdef NT_FN_PRODUCTION_STATS
	uint16_t qos_nulls_sent_to_vo;
	uint16_t qos_nulls_sent_to_vi;
	uint16_t qos_nulls_sent_to_bk;
	uint16_t qos_nulls_sent_to_be;
	uint16_t eosp_tmr_expired_cnt_for_vo;
	uint16_t eosp_tmr_expired_cnt_for_vi;
	uint16_t eosp_tmr_expired_cnt_for_bk;
	uint16_t eosp_tmr_expired_cnt_for_be;
	uint16_t more_bits_set_cnt;
#endif //NT_FN_PRODUCTION_STATS
#ifdef NT_FN_DEBUG_STATS
	uint16_t total_trigger_timer_Cnt;
	uint16_t total_eosp_interrupts_raised;
	uint16_t received_qos_data_cnt;
#endif //NT_FN_DEBUG_STATS
}uapsd_stats_t;
#endif //defined(NT_FN_DEBUG_STATS) || defined(NT_FN_PRODUCTION_STATS)
#endif //NT_FN_WMM_PS_STA

#ifdef NT_FN_PRODUCTION_STATS
typedef PREPACK struct wlan_prod_stats_s {
	pm_statistics_t		pm_stats;
	imps_stats_t 		imps_stats;
#ifdef NT_FN_WNM_POWERSAVE_MODE
	wnm_ps_struct_stats_ap_t  wnm_stats_ap;
	wnm_ps_struct_stats_sta_t wnm_stats_sta;
#endif //#ifdef NT_FN_WNM_POWERSAVE_MODE
#ifdef NT_FN_WMM_PS_STA
	uapsd_stats_t 	uapsd_sta;
#endif //NT_FN_WMM_PS_STA
#if (defined NT_FN_WUR_AP) || (defined NT_FN_WUR_STA)
	wur_struct_stats_sta_t wur_sta;
	wur_struct_stats_ap_t  wur_ap;
#endif //#if (defined NT_FN_WUR_AP) || (defined NT_FN_WUR_STA)
}POSTPACK wlan_prod_stats_t ;
#endif //NT_FN_PRODUCTION_STATS

#define MAX_REGULATORY_RULES    17

typedef PREPACK struct wlan_reg_rule_s {
    uint16_t    start_freq;
    uint16_t    end_freq;
    uint8_t     reg_power;
    uint8_t     ant_gain;
    uint16_t    flag_info;
    uint32_t    psd_power_info;
    uint16_t    max_bw;
}POSTPACK wlan_reg_rule_t ;

typedef PREPACK struct wlan_regulatory_s {
	uint8_t alpha[3];
	uint8_t num_2g_reg_rules;
    uint8_t num_5g_reg_rules;
	wlan_reg_rule_t reg_rules[MAX_REGULATORY_RULES];
}POSTPACK wlan_regulatory_t ;

typedef PREPACK struct wlan_cserv_stats_s {
	uint8_t status;
	uint8_t	cs_rssi;
}POSTPACK wlan_cserv_stats_t ;

typedef PREPACK struct {
	uint8_t ch_num;
    int32_t roam_util;
    uint8_t bssid[IEEE80211_ADDR_LEN];
    ssid_t  ssid;
    int8_t  rssi;
    int8_t  last_rssi;
    int8_t  util;
    int8_t  bias;
    uint8_t  ucipher;
#ifdef SUPPORT_RING_IF
    uint8_t wlan_mode;
#endif
} POSTPACK WMI_BSS_ROAM_INFO;

#define WLAN_NUM_ROAMING_CANDIDATES   16

typedef PREPACK struct {
    uint16_t  roamMode;
    uint16_t  numEntries;
    WMI_BSS_ROAM_INFO bssRoamInfo[WLAN_NUM_ROAMING_CANDIDATES];
} POSTPACK WMI_TARGET_ROAM_TBL;

/*
 * WMI_SET_REASSOC_MODE_CMDID
 *
 * Set authentication mode
 */
typedef enum {
    REASSOC_DO_DISASSOC = 0x00,
    REASSOC_DONOT_DISASSOC = 0x01,
	REASSOC_D0_ASSOC = 0x02,
} WMI_REASSOC_MODE;

#ifdef NT_FN_WMM
typedef enum {
    WMI_WMM_DISABLED = 0,
    WMI_WMM_ENABLED
} WMI_WMM_STATUS;

typedef PREPACK struct {
    uint8_t    status;
    uint8_t    ip_precedence[8];
}POSTPACK WMI_SET_WMM_CMD;
#endif //NT_FN_WMM

typedef PREPACK struct {
    uint8_t    status;
}POSTPACK WMI_SET_QOS_SUPP_CMD;


typedef enum {
    WMI_TXOP_DISABLED = 0,
    WMI_TXOP_ENABLED
} WMI_TXOP_CFG;

#ifdef ATH_KF
typedef PREPACK struct {
    uint8_t    txopEnable;
}POSTPACK WMI_SET_WMM_TXOP_CMD;

typedef PREPACK struct {
    NT_BOOL configured;
    uint8_t keepaliveInterval;
} POSTPACK WMI_GET_KEEPALIVE_CMD;
#endif //ATH_KF

/*
 * Add Application specified IE to a management frame
 */
#define WMI_MAX_IE_LEN  255
#define WMI_MAX_LARGE_IE_LEN  510

typedef PREPACK struct {
    uint8_t mgmtFrmType;  /* one of WMI_MGMT_FRAME_TYPE */
    uint8_t ieLen;    /* Length  of the IE that should be added to the MGMT frame */
    uint8_t ieInfo[1];
} POSTPACK WMI_SET_APPIE_CMD;

/*
 * WMI_SET_AKMP_PARAMS_CMD
 */

#define WMI_AKMP_MULTI_PMKID_EN   0x000001

typedef PREPACK struct {
    uint32_t    akmpInfo;
} POSTPACK WMI_SET_AKMP_PARAMS_CMD;


typedef PREPACK struct {
    uint8_t pmkid[WMI_PMKID_LEN];
} POSTPACK WMI_PMKID;


/*
 * WMI_SET_PMKID_LIST_CMD
 */
#define WMI_MAX_PMKID_CACHE   8

typedef PREPACK struct {
    uint32_t    numPMKID;
    WMI_PMKID   pmkidList[WMI_MAX_PMKID_CACHE];
} POSTPACK WMI_SET_PMKID_LIST_CMD;

/*
 * WMI_GET_PMKID_LIST_CMD  Reply
 * Following the Number of PMKIDs is the list of PMKIDs
 */
typedef PREPACK struct {
    uint32_t    numPMKID;
    uint8_t     bssidList[IEEE80211_ADDR_LEN][1];
    WMI_PMKID   pmkidList[1];
} POSTPACK WMI_PMKID_LIST_REPLY;


typedef PREPACK struct {
    uint16_t oldChannel;
    uint32_t newChannel;
} POSTPACK WMI_CHANNEL_CHANGE_EVENT;

typedef PREPACK struct {
    uint32_t version;
} POSTPACK WMI_WLAN_VERSION_EVENT;


/* WMI_ADDBA_REQ_EVENTID */
typedef PREPACK struct {
    uint8_t     tid;
    uint8_t     win_sz;
    uint16_t    st_seq_no;
    uint8_t     status;         /* f/w response for ADDBA Req; OK(0) or failure(!=0) */
    uint8_t     sta_id;         //xerus
    uint8_t     amsdu_status;   //xerus
} POSTPACK WMI_ADDBA_REQ_EVENT;

/* WMI_ADDBA_RESP_EVENTID */
typedef PREPACK struct {
    uint8_t     tid;
    uint8_t     status;         /* OK(0), failure (!=0) */
    uint16_t    amsdu_sz;       /* Three values: Not supported(0), 3839, 8k */
} POSTPACK WMI_ADDBA_RESP_EVENT;

/* WMI_DELBA_EVENTID
 * f/w received a DELBA for peer and processed it.
 * Host is notified of this
 */
typedef PREPACK struct {
    uint8_t     tid;
    uint8_t     is_peer_initiator;
    uint16_t    reason_code;
    uint8_t     sta_id;          //xerus
} POSTPACK WMI_DELBA_EVENT;

#ifdef ATH_KF
/* WMI_ALLOW_AGGR_CMDID
 * Configures tid's to allow ADDBA negotiations
 * on each tid, in each direction
 */
typedef PREPACK struct {
    uint16_t    tx_allow_aggr;  /* 16-bit mask to allow uplink ADDBA negotiation - bit position indicates tid*/
    uint16_t    rx_allow_aggr;  /* 16-bit mask to allow donwlink ADDBA negotiation - bit position indicates tid*/
} POSTPACK WMI_ALLOW_AGGR_CMD;
#endif //ATH_KF

/* WMI_ADDBA_REQ_CMDID
 * f/w starts performing ADDBA negotiations with peer
 * on the given tid
 */
typedef PREPACK struct {
    uint8_t     tid;
} POSTPACK WMI_ADDBA_REQ_CMD;

/* WMI_DELBA_REQ_CMDID
 * f/w would teardown BA with peer.
 * is_send_initiator indicates if it's or tx or rx side
 */
typedef PREPACK struct {
    uint8_t     tid;
    uint8_t     is_sender_initiator;

} POSTPACK WMI_DELBA_REQ_CMD;

/*
 * ------- AP Mode definitions --------------
 */

#define MCAST_AID               0xFF /* Spl. AID used to set DTIM flag in the beacons */
#define DEF_AP_COUNTRY_CODE     "US "

/* AP mode disconnect reasons */
#define AP_DISCONNECT_STA_LEFT      101
#define AP_DISCONNECT_FROM_HOST     102
#define AP_DISCONNECT_COMM_TIMEOUT  103

/*
 * Used with WMI_AP_SET_MLME_CMDID
 */
typedef PREPACK struct {
    uint8_t    mac[IEEE80211_ADDR_LEN];
    uint16_t   reason;              /* 802.11 reason code */
    uint8_t    cmd;                 /* operation to perform */
/* MLME Commands */
#define WMI_AP_MLME_ASSOC       1   /* associate station */
#define WMI_AP_DISASSOC         2   /* disassociate station */
#define WMI_AP_DEAUTH           3   /* deauthenticate station */
#define WMI_AP_MLME_AUTHORIZE   4   /* authorize station */
#define WMI_AP_MLME_UNAUTHORIZE 5   /* unauthorize station */
} POSTPACK WMI_AP_SET_MLME_CMD;

typedef struct {
    NT_BOOL flag;
    uint16_t rsvd;
    uint16_t aid;
} WMI_AP_SET_PVB_CMD;

#define WMI_DISABLE_REGULATORY_CODE "FF"



/* user selected ldpc and stbc infor are passed as bit field
 * in enable param along 11n enable status from host to
 * firmware. Meaning of each bit set is defined  below.
 * Currently  5 bits are used for LDPC and STBC in u8 enable.
 * this is only for MDM9x25 project, rest is similar to old
 * */
#define WMI_HTCAP_11N_ENABLE            (1 << 0)
#define WMI_HTCAP_LDPC_CODING           (1 << 1)
#define WMI_HTCAP_RX_STBC_1SS           (1 << 2)
#define WMI_HTCAP_RX_STBC_2SS           (1 << 3)
#define WMI_HTCAP_RX_STBC_3SS           (1 << 4)
#define WMI_HTCAP_TX_STBC               (1 << 5)
#define WMI_REQUIRE_HT                  (1 << 7)

typedef PREPACK struct {
    uint8_t  band; /* specifies which band to apply these values */
    uint8_t  enable; /* allows 11n to be disabled on a per band basis */
    uint8_t  chan_width_40M_supported;
    uint8_t  short_GI_20MHz;
    uint8_t  short_GI_40MHz;
    uint8_t  intolerance_40MHz;
    uint8_t  max_ampdu_len_exp;
} POSTPACK WMI_SET_HT_CAP_CMD;

typedef PREPACK struct {
    uint8_t   sta_chan_width;
    uint8_t   ap_ht_info;  /* b0:b1 - ht_opmode */
} POSTPACK WMI_SET_HT_OP_CMD;

typedef PREPACK struct {
    uint8_t     ch_index; 	// Channel index */
    uint8_t 	dev_id; 	// AP or STA dev id for concurrency mode
} POSTPACK WMI_SET_CHANNEL_CMD;

#ifdef NT_FN_XPA
typedef PREPACK struct  {
uint8_t enable_xpa;
} POSTPACK WMI_XPA_CMD;
#endif //NT_FN_XPA

#ifdef NT_FN_WPS
typedef enum {
	WPS_EN     = 0x1,
	WPS_EN_INT = 0x2
}WPS_CONFIG_METHODS;

typedef PREPACK struct {
    uint8_t ssid[WMI_MAX_SSID_LEN + 1];
    uint8_t macaddress[IEEE80211_ADDR_LEN];
    uint16_t channel;
    uint8_t ssid_len;
} POSTPACK WPS_SCAN_LIST_ENTRY;

#define WPS_PIN_LEN (8)
#define MAX_LIST_COUNT 8

typedef PREPACK struct {
    uint8_t pin[WPS_PIN_LEN+1];
    uint8_t pin_length;
}POSTPACK WPS_PIN;
typedef enum {
    WPS_PIN_MODE = 0x1,
    WPS_PBC_MODE = 0x2
} WPS_MODE;

typedef enum _WPS_OPER_MODE{
    WPS_REGISTRAR_ROLE    = 0x1,
    WPS_ENROLLEE_ROLE     = 0x2,
    WPS_AP_ENROLLEE_ROLE  = 0x3
} WPS_OPER_MODE;

typedef struct {
	uint8_t categ;
	uint8_t sub_categ;
}WMI_WPS_DEV_TYPE;

typedef PREPACK struct {
	WPS_SCAN_LIST_ENTRY   ssid_info;
    WPS_MODE              config_mode; /* WPS_MODE PIN or PBC*/
    WPS_PIN               wps_pin;

    uint8_t               timeout;     /* in Seconds appx. 2 minutes*/
    WPS_OPER_MODE         role;        /* WPS_OPER_MOD registrar or enrollee*/
    uint8_t               ctl_flag;    /* WPS_START_CTRL_FLAG */
    uint8_t  		     *device_name;
    uint8_t   		      uuid[WPS_UUID_LEN];
    uint16_t  		      config_methods;
    uint16_t  		      dev_name_len;
    WMI_WPS_DEV_TYPE      pri_dev_type;
    uint8_t     	      device_mode ;
    uint8_t  			  devId ;

} POSTPACK WMI_WPS_START_CMD;

/* WMI_WPS_SET_CONFIG_STATE_CMD */
typedef PREPACK struct {
    int32_t mode;
} POSTPACK  WMI_WPS_SET_CONFIG_STATE_CMD;

typedef enum {
    WPS_DO_CONNECT_AFTER_CRED_RECVD = 0x1
} WPS_START_CTRL_FLAG;

typedef enum {
    WPS_STATUS_SUCCESS = 0x0,
    WPS_STATUS_FAILURE = 0x1,
    WPS_STATUS_IDLE = 0x2,
    WPS_STATUS_IN_PROGRESS  = 0x3
} WPS_STATUS;

typedef PREPACK struct {
    uint8_t  wps_status;  /* WPS_STATUS */
    uint8_t  wps_state;
} POSTPACK WMI_WPS_GET_STATUS_EVENT;

typedef enum {
    WPS_ERROR_INVALID_START_INFO  = 0x1,
    WPS_ERROR_MULTIPLE_PBC_SESSIONS,
    WPS_ERROR_WALKTIMER_TIMEOUT,
    WPS_ERROR_M2D_RCVD,
    WPS_ERROR_PWD_AUTH_FAIL,
    WPS_ERROR_CANCELLED,
    WPS_ERROR_INVALID_PIN
} WPS_ERROR_CODE;

/* Authentication Type Flags */
#define WPS_CRED_AUTH_OPEN    0x0001
#define WPS_CRED_AUTH_WPAPSK  0x0002
#define WPS_CRED_AUTH_SHARED  0x0004
#define WPS_CRED_AUTH_WPA     0x0008
#define WPS_CRED_AUTH_WPA2    0x0010
#define WPS_CRED_AUTH_WPA2PSK 0x0020

/* Encryption Type Flags */
#define WPS_CRED_ENCR_NONE 0x0001
#define WPS_CRED_ENCR_WEP  0x0002
#define WPS_CRED_ENCR_TKIP 0x0004
#define WPS_CRED_ENCR_AES  0x0008

typedef enum {
    WPS_AUTH_TYPE_OPEN    = 0x0001,
    WPS_AUTH_TYPE_WPAPSK  = 0x0002,
    WPS_AUTH_TYPE_SHARED  = 0x0004,
    WPS_AUTH_TYPE_WPA     = 0x0008,
    WPS_AUTH_TYPE_WPA2    = 0x0010,
    WPS_AUTH_TYPE_WPA2PSK = 0x0020
} WPS_AUTH_TYPE;

typedef enum {
    WPS_ENCR_TYPE_NONE = 0x0001,
    WPS_ENCR_TYPE_WEP  = 0x0002,
    WPS_ENCR_TYPE_TKIP = 0x0004,
    WPS_ENCR_TYPE_AES  = 0x0008,
} WPS_ENCR_TYPE;

typedef PREPACK struct {
  uint16_t ap_channel;
  uint8_t  ssid[WMI_MAX_SSID_LEN + 1];
  uint8_t  ssid_len;
  uint16_t auth_type;
  uint16_t encr_type;
  uint8_t  key_idx;
  uint8_t  key[64];
  uint8_t  key_len;
  uint8_t  mac_addr[IEEE80211_ADDR_LEN];
} POSTPACK WPS_CREDENTIAL;

typedef PREPACK struct _WMI_WPS_PROFILE_EVENT{
  uint8_t status;      /* WPS_STATUS */
  uint8_t error_code;  /* WPS_ERROR_CODE */
  uint8_t cred_flag;
  uint8_t disassoc_flag ;
  uint8_t role ;
  uint8_t wps_state;
  WPS_CREDENTIAL  credential;
  uint8_t  peer_dev_addr[IEEE80211_ADDR_LEN];
} POSTPACK WMI_WPS_PROFILE_EVENT;

typedef PREPACK struct {
    WPS_CREDENTIAL  credential;
}POSTPACK WMI_P2P_PERSISTENT_PROFILE_CMD;

/*typedef struct {
	uint8_t  		 *device_name;
	uint8_t   		 uuid[WPS_UUID_LEN];
	uint16_t  		 config_methods;
	uint16_t  		 dev_name_len;
	WMI_WPS_DEV_TYPE pri_dev_type;
	WPS_OPER_MODE    dev_wps_mode;

}WMI_WPS_SET_CONFIG_CMD;*/

/* WPS Commands AND Events DEFINITION END */
#endif//NT_FN_WPS

#ifdef ATH_KF
typedef enum {
    WMI_AP_APSD_DISABLED = 0,
    WMI_AP_APSD_ENABLED
} WMI_AP_APSD_STATUS;

typedef PREPACK struct {
    uint8_t    enable;
} POSTPACK WMI_AP_SET_APSD_CMD;

typedef enum {
    WMI_AP_APSD_NO_DELIVERY_FRAMES_FOR_THIS_TRIGGER =  0x1,
} WMI_AP_APSD_BUFFERED_TRAFFIC_FLAGS;

typedef PREPACK struct {
    uint16_t   aid;
    uint16_t   bitmap;
    uint32_t   flags;
} POSTPACK WMI_AP_APSD_BUFFERED_TRAFFIC_CMD;

/** Reporting the WLAN STATISTICS */
typedef PREPACK struct{
	uint32_t unicast_tx_pkts;
	uint32_t unicast_rx_pkts;
	uint32_t multicast_tx_pkts;
	uint32_t multicast_rx_pkts;
	uint32_t broadcast_tx_pkts;
	uint32_t broadcast_rx_pkts;
	uint32_t unicast_non_null_tx_pkts;	// Unicast TX Packets excluding NULL and Qos NULL pkts
	uint32_t unicast_non_null_rx_pkts;	// Unicast RX Packets excluding NULL and Qos NULL pkts
	uint32_t unicast_filtered_accepted_tx_pkts;
	uint32_t unicast_filtered_accepted_rx_pkts;
	uint32_t multicast_filtered_accepted_tx_pkts;
	uint32_t multicast_filtered_accepted_rx_pkts;
	uint32_t broadcast_filtered_accepted_tx_pkts;
	uint32_t broadcast_filtered_accepted_rx_pkts;
	uint32_t unicast_filtered_rejected_tx_pkts;
	uint32_t unicast_filtered_rejected_rx_pkts;
	uint32_t multicast_filtered_rejected_tx_pkts;
	uint32_t multicast_filtered_rejected_rx_pkts;
	uint32_t broadcast_filtered_rejected_tx_pkts;
	uint32_t broadcast_filtered_rejected_rx_pkts;
	uint32_t null_tx_pkts;
	uint32_t null_rx_pkts;
	uint32_t qos_null_tx_pkts;
	uint32_t qos_null_rx_pkts;
	uint32_t ps_poll_tx_pkts;
	uint32_t ps_poll_rx_pkts;
	uint32_t tx_retry_cnt;
	uint32_t beacon_miss_cnt;
	uint32_t beacons_received_cnt;
	uint32_t beacon_resync_failure_cnt;
	uint32_t curr_early_wakeup_adj_in_ms;
	uint32_t avg_early_wakeup_adj_in_ms;
	uint32_t early_termination_cnt;
	uint32_t uapsd_trigger_rx_cnt;
	uint32_t uapsd_trigger_tx_cnt;
	uint32_t rx_amsdu_pkts;
}POSTPACK WMI_DEVICE_WLAN_STATS;

typedef PREPACK struct
{
	uint8_t htc_inf_cur_cnt;
	uint8_t htc_inf_reaped_cnt;
	uint8_t mac_inf_cur_cnt;
	uint8_t mac_inf_reaped_cnt;
	uint8_t fw_inf_cur_cnt;
	uint8_t fw_inf_reaped_cnt;
	uint8_t free_buf_cnt;
	uint8_t mgmt_buf_cnt;
	uint8_t smmgmt_buf_cnt;
	uint8_t num_txbuf_queued;
	uint8_t num_rxbuf_queued;
	uint8_t reserved;
}POSTPACK WMI_COMMON_WLAN_TXRX_BUFFER_INFO;

typedef PREPACK struct {
	uint32_t total_active_time_in_ms;
	uint32_t total_powersave_time_in_ms;
	WMI_COMMON_WLAN_TXRX_BUFFER_INFO txrx_buf_info;
} POSTPACK WMI_COMMON_WLAN_STATS;

typedef PREPACK struct {
    uint16_t        wmi_event_missed_last;
    uint16_t        reserved;
    uint32_t        wmi_event_missed_bitmap;
    uint32_t        wmi_event_missed_cnt;
} POSTPACK WMI_DEVICE_WLAN_STATS_EXT2;

typedef PREPACK struct {
	WMI_DEVICE_WLAN_STATS devWlanStats;
	WMI_COMMON_WLAN_STATS commonWlanStats;
//	WMI_DEVICE_WLAN_STATS_EXT devWlanStat_ext;
    WMI_DEVICE_WLAN_STATS_EXT2  devWlanStat_ext2;
}POSTPACK WMI_WLAN_STATS;
#endif //ATH_KF

/*
 * Frame Types
 */
typedef enum {
    WMI_FRAME_BEACON = 0,
    WMI_FRAME_PROBE_REQ,
    WMI_FRAME_PROBE_RESP,
    WMI_FRAME_ASSOC_REQ,
    WMI_FRAME_ASSOC_RESP,
    WMI_NUM_MGMT_FRAME
} WMI_MGMT_FRAME_TYPE;

#define AGGRX_CFG_INVAL 0xff

#ifdef ATH_KF
typedef PREPACK struct {
    uint16_t cmdType;                           /*Control WNM sleep feature*/
    uint16_t duration;                         /*WNM Sleep duration*/
} POSTPACK WMI_WNM_SLEEP_CMD;

typedef PREPACK struct {
    uint16_t enable;                            /*1- Enable/ 0- disable TIM broadcast*/
    uint16_t period;                           /*TIM sleep period*/
} POSTPACK WMI_WNM_TIM_CMD;

typedef PREPACK struct {
    uint8_t enable;   /* Enable/disable 11v */
} POSTPACK WMI_WNM_CONFIG_CMD;

typedef PREPACK struct {
    uint16_t period;     /* Max idle period */
    uint16_t pKeepAlive; /* AP accepts only protected keep alives or data frames */
} POSTPACK WMI_WNM_MAX_IDLE_PERIOD_CMD;

typedef PREPACK struct {
    uint8_t response;   /* WNM sleep response */
} POSTPACK WMI_WNM_SLEEP_RESPONSE_CMD;

typedef PREPACK struct {
    uint16_t cmd;        /* WNM command */
    uint16_t response;   /* WNM command response */
} POSTPACK WMI_WNM_EVENT;

typedef PREPACK struct {
    uint32_t    channel;
	uint8_t		tbtt_cnt;
} WMI_CHANNEL_SWITCH_CMD;

/*
 * WMI_GET_STA_DTIM_CMDID
 */
typedef PREPACK struct {
    uint32_t     dtim;                  /* DTIM number */
} POSTPACK WMI_GET_STA_DTIM_CMD;
#endif //ATH_KF

/**
 * stats type
 */
typedef enum{
	debug_stats = 0,
	production_stats,
}device_stats_type;

/**
 * Operations on stats
 */
typedef enum{
	stats_read = 0,
	stats_clear,
}stats_operation_type;

typedef PREPACK struct {
	device_stats_type type : 1;
	stats_operation_type operation_type : 1;
	void *buff_ptr;
} POSTPACK WMI_DPM_STATS_CMD;
#define IP_PRECEDENCE_CNT   8
typedef PREPACK struct {
    uint8_t  ip_precedence[IP_PRECEDENCE_CNT]; /* specifies which band to apply these values */
} POSTPACK WMI_SET_IP_PRECEDENCE_CMD;

/*** network detail indexes ***/
typedef enum{
	wapp_profiles = 0,
	wapp_security,
	wapp_ssid,
	wapp_password,
	wapp_devicemode,
	wapp_conf,
	wapp_psm,
}wapp_nw_config_details_indx;

typedef PREPACK struct
{
uint8_t ra_ON;
uint8_t set_ra_staid;
uint8_t set_rate_config_p_rate ;
uint8_t set_rate_config_s_rate ;
uint8_t set_rate_config_t_rate ;
} POSTPACK SET_RATE_CFG;

#ifdef NT_FN_FTM_11V
typedef PREPACK struct {
	uint8_t conn_id;						  /* station id to which ftm frames to be sent from ap and viceversa */
	uint32_t ftm_period;                      /*period for sending ftm frames in ms*/
	uint16_t ftm_count;
} POSTPACK WMI_FTM_CMD;
#endif // NT_FN_FTM_11V

typedef PREPACK struct {
        uint8_t network_type; /* AP or STA*/
        uint8_t dhcp_type; /* static 0 /dynamic 1 */
        union {
        	uint32_t ipv4_addr;
        	uint32_t ipv6_addr[4];
        };    /* ipv4/ipv6 address */
        uint8_t ip_ver; /* ipv4 /ipv6 see ip_type*/
        uint32_t gateway; /* gateway IP  used only in case of static dhcp */
        uint32_t netmask; /* netmask used only in case of static dhcp */

} POSTPACK WMI_IF_ADD_CMD;

typedef PREPACK struct {
	uint8_t *tlv20_buff; /* pointer to buffer which has TLV2.0 encoded message */
	uint16_t buff_len; /* buffer length having TLV2.0 encoded message */
} POSTPACK WMI_PDEV_UTF_CMD;

typedef PREPACK struct {
	app_mode_id_t requested_mode; /* 0 for MM 1 for FTM */
} POSTPACK WMI_MODE_CMD;

typedef PREPACK struct {
	uint32_t pdev_param_id;
	uint32_t pdev_param_value;
} POSTPACK WMI_SET_PDEV_PARAM_CMD;

typedef PREPACK struct {
	uint8_t scan_id;
}POSTPACK WMI_SCAN_STOP_CMD;

#define MAX_SCAN_SSID 15

typedef struct{
    uint16_t chan_freq;  //Channel frequency in MHz
    uint8_t bssid[IEEE80211_ADDR_LEN];
    ssid_t  ssid;
    uint8_t  security_mode;
    int8_t  rssi;
    uint8_t wlan_mode;
}ap_info;

typedef PREPACK struct{
    uint8_t enable;
    uint32_t slp_time;
    uint32_t recnx_wait;
    uint32_t cmd_proc_wait;
    uint32_t cnx_wait;
} POSTPACK WMI_IMPS_CFG;

typedef PREPACK struct{
    uint8_t enable;
} POSTPACK WMI_SLP_CLK_CAL_CFG;

typedef PREPACK struct{
    uint8_t enable;
} POSTPACK WMI_BMPS_IGNORE_BCMC;

typedef PREPACK struct{
    uint8_t enable;
} POSTPACK WMI_BMPS_ENABLE;

typedef PREPACK struct{
    uint32_t time;
} POSTPACK WMI_BMPS_IDLE_TIME;

typedef PREPACK struct{
    uint32_t pre_bcn_wkup;
    uint32_t bcn_wait_time;
    uint32_t tele_pre_bcn_inc;
    uint32_t tele_bcn_wait_inc;
} POSTPACK WMI_BMPS_TIMING_CFG;

typedef PREPACK struct{
    uint8_t enable;
} POSTPACK WMI_SLP_CLK_CAL_ACT;

typedef PREPACK struct{
    uint8_t num_entries;
    uint8_t scan_id;
    ap_info scan_bss_info[MAX_SCAN_SSID];
} POSTPACK SCAN_RESULT;

#ifdef SUPPORT_SAP_POWERSAVE
typedef PREPACK struct {
    uint32_t next_tbtt_hi;
    uint32_t next_tbtt_lo;
    uint16_t beacon_multiplier;
    uint16_t freq;
} POSTPACK WMI_UPDATE_BI_CMD;
#endif /* SUPPORT_SAP_POWERSAVE */

#ifdef SUPPORT_BEACON_MISS_THRESHOLD_TIME
typedef PREPACK struct {
    uint32_t beacon_miss_thr_time_us;
} POSTPACK WMI_UPDATE_BMTT_CMD;
#endif /* SUPPORT_BEACON_MISS_THRESHOLD_TIME */

typedef enum
{
    WMI_WAKELOCK_RESET_CMD,
    WMI_WAKELOCK_SET_CMD,
}wmi_wakelock_type;

typedef PREPACK struct{
    uint8_t status;
    uint8_t param_id;
} POSTPACK SET_PDEV_PARAM_RESULT;

typedef PREPACK struct {
	uint8_t netif_id; /*Network interface id of the connected interface */
	uint8_t status;
	uint8_t reason;
    uint16_t new_chan_freq;
} POSTPACK chan_switch_event;

#ifdef CONFIG_WMI_EVENT
typedef enum {
    WIFI_PARAM_SET_PDEV_CHANNEL = 0,
    WIFI_PARAM_SET_AP_BCN_INTERVAL = 1,
    WIFI_PARAM_SET_PHYMODE = 2,
    WIFI_PARAM_SET_PDEV_COUNTRY_CODE = 3,
    WIFI_PARAM_SET_LOGGER_ATTACHED = 4,
    WIFI_PARAM_SET_BA_TIMEOUT = 5,
    WIFI_PARAM_SET_EB_LOCATION = 6,
    WIFI_PARAM_SET_AP_DTIM = 7,
    WIFI_PARAM_SET_AP_INACTIVITY = 8,
    WIFI_PARAM_SET_AP_HIDDEN = 9,
    WIFI_PARAM_SET_ALLOW_AGGR = 10,
    WIFI_PARAM_SET_AMSDU_RX = 11,
    WIFI_PARAM_SET_STA_DTIM = 12,
}param_id;
enum {
    WIFI_STATUS_SUCCESS,
    WIFI_STATUS_FALIURE,
    WIFI_STATUS_INVALID_NETIF,
    WIFI_STATUS_INVALID_LEN,
    WIFI_STATUS_INVALID_ARGS,
    WIFI_STATUS_MAX,
};
typedef enum
{
    HOST_LOGGER_DETACHED = 0,
    HOST_LOGGER_ATTACHED = 1,
} host_logger_state;
#endif
#endif /* _WMI_H_ */
