/* 
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef _WLAN_DEV_H_
#define _WLAN_DEV_H_

#ifdef __cplusplus
extern "C" {
#endif

//#error wlan_dev called
#include <stdint.h>
#include "osapi.h"
#include "wifi_cmn.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "nt_common.h"
#include "ieee80211.h"
#include "wmi.h"
#include "wlan_conn.h"
#include "wlan_bss.h"
#ifdef SUPPORT_REGULATORY
#include "halphy_regulatory_api.h"
#endif /* SUPPORT_REGULATORY */
#include "wreg_api.h"
#include "wlan_ap.h"
#include "hal_api_sys.h"
#include "hal_int_scan.h"
#include "txrx_api.h"
#include "tx_aggr_api.h"
#include "mlme_al.h"
#include "iot_wifi.h"
#include "nt_logger_api.h"
//#include "sae.h"
//#include "wmi.h"

#ifdef NT_DEBUG
#ifndef SUPPORT_FERMION_LOGGER
#define WLAN_CFG_INC_DBG_PRINT // include debug prints
#endif                         // SUPPORT_FERMION_LOGGER
#endif

#ifdef WLAN_CFG_INC_DBG_PRINT
#include "uart.h"
#include <string.h> // strlen
#define WLAN_DEBUG(x) UART_Send(x, strlen(x))
#else
#define WLAN_DEBUG(x)
#endif

#ifdef WLAN_CFG_INC_DBG_PRINT
void wlan_dev_dbg0_print(const char *s, const char *fn, const uint32_t ln);
#define WLAN_DBG0_PRINT(str) wlan_dev_dbg0_print(str, __func__, __LINE__)
void wlan_dev_dbg1_print(const char *s, const uint32_t a1, const char *fn, const uint32_t ln);
#define WLAN_DBG1_PRINT(str, a1) wlan_dev_dbg1_print(str, a1, __func__, __LINE__)
void wlan_dev_dbg2_print(const char *s, const uint32_t a1, const uint32_t a2, const char *fn, const uint32_t ln);
#define WLAN_DBG2_PRINT(str, a1, a2) wlan_dev_dbg2_print(str, a1, a2, __func__, __LINE__)
void wlan_dev_dbg3_print(const char *s, const uint32_t a1, const uint32_t a2, const uint32_t a3, const char *fn,
                         const uint32_t ln);
#define WLAN_DBG3_PRINT(str, a1, a2, a3) wlan_dev_dbg3_print(str, a1, a2, a3, __func__, __LINE__)
void wlan_dev_mac_addr_print(const char *s, const uint8_t *a1, const char *fn, const uint32_t ln);
#define WLAN_DBG_MAC_ADDR_PRINT(str, a1) wlan_dev_mac_addr_print(str, a1, __func__, __LINE__)
void wlan_dev_arr_print(const char *pmsg, const uint8_t *ptr, const uint16_t len, const char *fn, const uint32_t ln);
#define WLAN_DBG_ARR_PRINT(str, ptr, len) wlan_dev_arr_print(str, ptr, len, __func__, __LINE__)
void wlan_dev_str_print(const char *pmsg, const uint8_t *ptr, const uint16_t len, const char *fn, const uint32_t ln);
#define WLAN_DBG_STR_PRINT(str, ptr, len) wlan_dev_str_print(str, ptr, len, __func__, __LINE__)
#elif defined(SUPPORT_FERMION_LOGGER)
// For these macros, the users are not expected to use any format specifiers inside. So adding them manually
#define LOG_SPECIFIER_3 " %d %d %d"
#define LOG_SPECIFIER_2 " %d %d"
#define LOG_SPECIFIER_1 " %d"
#define LOG_STRING_SPECIFIER " %s"
#define WLAN_DBG_PRINT(msg, p1, p2, p3)                                                                                \
    do {                                                                                                               \
        WLAN_DBG_PRINT_FORMAT_CHK(msg)                                                                                 \
        DEBUG_LOG_STRING(log_fmt, msg "" LOG_SPECIFIER_3);                                                             \
        log_to_buffer_variadic(log_fmt, 3, p1, p2, p3);                                                                \
    } while (0)
#define WLAN_DBG0_PRINT(msg)                                                                                           \
    do {                                                                                                               \
        WLAN_DBG_PRINT_FORMAT_CHK(msg)                                                                                 \
        DEBUG_LOG_STRING(log_fmt, msg);                                                                                \
        log_to_buffer_variadic(log_fmt, 0);                                                                            \
    } while (0)

#define WLAN_DBG1_PRINT(msg, p1)                                                                                       \
    do {                                                                                                               \
        WLAN_DBG_PRINT_FORMAT_CHK(msg)                                                                                 \
        DEBUG_LOG_STRING(log_fmt, msg "" LOG_SPECIFIER_1);                                                             \
        log_to_buffer_variadic(log_fmt, 1, p1);                                                                        \
    } while (0)

#define WLAN_DBG2_PRINT(msg, p1, p2)                                                                                   \
    do {                                                                                                               \
        WLAN_DBG_PRINT_FORMAT_CHK(msg)                                                                                 \
        DEBUG_LOG_STRING(log_fmt, msg "" LOG_SPECIFIER_2);                                                             \
        log_to_buffer_variadic(log_fmt, 2, p1, p2);                                                                    \
    } while (0)

#define WLAN_DBG3_PRINT(msg, p1, p2, p3)                                                                               \
    do {                                                                                                               \
        WLAN_DBG_PRINT_FORMAT_CHK(msg)                                                                                 \
        DEBUG_LOG_STRING(log_fmt, msg "" LOG_SPECIFIER_3);                                                             \
        log_to_buffer_variadic(log_fmt, 3, p1, p2, p3);                                                                \
    } while (0)

#define WLAN_DBG_MAC_ADDR_PRINT(msg, ptr)                                                                              \
    do {                                                                                                               \
        WLAN_DBG_PRINT_FORMAT_CHK(msg)                                                                                 \
        DEBUG_LOG_STRING(log_fmt, msg "" LOG_STRING_SPECIFIER);                                                        \
        log_arr_to_buffer(log_fmt, (uint8_t *)ptr, 6);                                                                 \
    } while (0)

#define WLAN_DBG_ARR_PRINT(msg, ptr, len)                                                                              \
    {                                                                                                                  \
        WLAN_DBG_PRINT_FORMAT_CHK(msg)                                                                                 \
        DEBUG_LOG_STRING(log_fmt, msg "" LOG_STRING_SPECIFIER);                                                        \
        log_arr_to_buffer(log_fmt, (uint8_t *)ptr, len);                                                               \
    }                                                                                                                  \
    while (0)

#define WLAN_DBG_STR_PRINT(msg, ptr, len)                                                                              \
    do {                                                                                                               \
        WLAN_DBG_PRINT_FORMAT_CHK(msg)                                                                                 \
        DEBUG_LOG_STRING(log_fmt, msg "" LOG_STRING_SPECIFIER);                                                        \
        log_dynamic_string_to_buffer_with_len(log_fmt, (char *)ptr, len);                                              \
    } while (0)

#else
#define WLAN_DBG_PRINT(str, a1, a2, a3)
#define WLAN_DBG0_PRINT(str)
#define WLAN_DBG1_PRINT(str, a1)
#define WLAN_DBG2_PRINT(str, a1, a2)
#define WLAN_DBG3_PRINT(str, a1, a2, a3)
#define WLAN_DBG_MAC_ADDR_PRINT(str, a1)
#define WLAN_DBG_ARR_PRINT(str, ptr, len)
#define WLAN_DBG_STR_PRINT(str, ptr, len)
#endif

#ifdef SUPPORT_5GHZ
#ifdef CONFIG_WIFILIB_6GHZ
#define IEEE_FREQ_6GHZ_LOW_BOUND 5955
#define IEEE_FREQ_6GHZ_HIGH_BOUND 6415
#define IEEE_FREQ_6GHZ_OFFSET 5950
#endif
#define IEEE_FREQ_5GHZ_LOW_BOUND 4900
#define IEEE_FREQ_5GHZ_HIGH_BOUND 5920
#define IEEE_FREQ_5GHZ_OFFSET 5000
#define IEEE_FREQ_2GHZ_LOW_BOUND 2412
#define IEEE_FREQ_2GHZ_HIGH_BOUND 3000
#define CHANNEL_FREQ_INVALID 0xFFFF
#define CHANNEL_LIST_END 0x0
#define IEEE_CHAN_5GHZ_HIGH_BOUND 184
#endif /* SUPPORT_5GHZ */
#define DC_CHANNEL_INDEX_INVALID 255
#ifdef SUPPORT_EVENT_HANDLERS
#endif /* SUPPORT_EVENT_HANDLERS */
typedef void (*CSERV_COMPLETION_CB)(void *arg, nt_status_t status);

typedef enum {
    WHAL_M_STA = 0,     /* infrastructure station */
    WHAL_M_AP = 1,      /* infrastructure AP */
    WHAL_M_AP_STA = 2,  /* AP_STA Concurrency */
    WHAL_M_NO_CONC = 3, /* Concurrency Off */
    WHAL_M_NONE = 0xFF,
} WHAL_OPMODE;

typedef struct _tsf_offset {
    NT_BOOL offset_negative;
    uint64_t offset;
} TSF_OFFSET;

typedef long (*TimDispatchTab)();

#define WLAN_MAC_ADDR_FOR_AP "@@@@@@"
#define WLAN_MAC_ADDR_FOR_STA "BBBBBB"

#define WLAN_MANIPULATED_MAC_ADDR_FOR_AP "######"

#define WLAN_BROADCAST_ADDR (uint8_t *)"\xff\xff\xff\xff\xff\xff"
#define POOL_CLEAR_MEM_ON_FREE 0x1

typedef struct conn_profile_s {
    ssid_t ssid;
    NETWORK_TYPE wlan_opmode;
    DOT11_AUTH_MODE wlan_dot11_authmode;
    AUTH_MODE wlan_authmode;
    int32_t bssid_set;
    uint8_t bssid[IEEE80211_ADDR_LEN];

    char countryCode[3]; /* Country Code */
    uint8_t ap_dtim;
    uint8_t ap_dtim_period;

    CRYPTO_TYPE pairwise_cipher;
    uint8_t pairwise_cipher_len;
    CRYPTO_TYPE group_cipher;
    uint8_t group_cipher_len;
    uint32_t flags;
    uint32_t grp_cipher_flag; // temporary flag for decision of group-ciphers for STA
    uint16_t akm_type;         /* Used for WPA2 assoc to differentiate SHA1 and SHA256,
                                 Be cautious when used for other AUTH type's, value may invalid */
} conn_profile_t;

#ifdef NT_FN_WMM_PS_STA
typedef struct {
    uint8_t tid : 3;
    uint8_t more_bit : 1;
    uint8_t sta_id;
} eosp_interrupt_struct;
#endif // NT_FN_WMM_PS_STA

#define GET_PROFILE_SSID(dev) (&(CONN_PROFILE_PTR(dev)->ssid))

#define _MAX_TIDS 8

/*
 * Channels are specified by frequency.
 */
typedef struct {
    uint16_t channel;  /* setting in Mhz, input parameter */
    int8_t maxRDPower; /* max transmit power, input parameter*/
    uint8_t wlanMode;  /* wlan phy mode, input parameter */
} WHAL_CHANNEL;

/*
 * Channels are specified by frequency and attributes.
 */
typedef struct channel {
    WHAL_CHANNEL ch_halchan;
    uint32_t ch_attrib; /* see below - wouldn't dare call them "flags" */
} channel_t;

#ifdef SUPPORT_EVENT_HANDLERS
typedef enum _wlan_dev_event_type {
    WLAN_DEV_EVENT_START,
    WLAN_DEV_EVENT_STOP,
    WLAN_DEV_EVENT_PAUSE,
    WLAN_DEV_EVENT_UNPAUSE,
    WLAN_DEV_EVENT_CONN_PEER,
    WLAN_DEV_EVENT_PEER_CRIT_PROTO_HINT,
    WLAN_DEV_EVENT_DELETE_PEER,
    WLAN_DEV_EVENT_TWT_SETUP,
    WLAN_DEV_EVENT_TWT_TEARDOWN,
} wlan_dev_event_type;

typedef struct _wlan_dev_event {
    uint32_t type;
    channel_t *channel;
    bss_t *bss;
} wlan_dev_event;

typedef struct wlan_dev_ev_handler_info dev_ev_handler_info;
#endif /* SUPPORT_EVENT_HANDLERS */

#define ch_freq ch_halchan.channel
#define ch_wlanMode ch_halchan.wlanMode
#define ch_tx_pwr ch_halchan.maxRDPower

#ifdef SUPPORT_REGULATORY
/*
 * Channel Attributes. Bits 0-15 is reserved for regulatory attributes.
 */
#define CHANNEL_REG_ATTRIB_MASK 0x0000FFFF        /* Bits 0-15 belong to reg */
#define CHANNEL_ACTIVE 0x80000000                 /* XXX channel use allowed */
#define CHANNEL_USER_DISABLE 0x02000000           /* disabled by user */
#define CHANNEL_IF_DISABLE 0x04000000             /* disabled - interference */
#define CHANNEL_RECVD_BEACON_PROBERESP 0x08000000 /* Channel with activity */
#define CHANNEL_AP_DISABLE 0x10000000             /* disabled by AP */
#define CHANNEL_RECVD_AP_CHAN_REPORT 0x20000000   /* part of AP channel report */
#define CHANNEL_REG_DISABLE 0x40000000            /*channel disabled by regulatory information*/
/* Bit 16-31 for channel attributes related to HT */
#define CHANNEL_REG_HT_ATTRIB_MASK 0xFFFF0000 /* Bit 16-31 HT realted */
#else
/*
 * Channel Attributes. Bits 0-7 is reserved for regulatory attributes.
 */
#define CHANNEL_REG_ATTRIB_MASK 0x00FF        /* Bits 0-7 belong to reg */
#define CHANNEL_ACTIVE 0x8000                 /* XXX channel use allowed */
#define CHANNEL_USER_DISABLE 0x0200           /* disabled by user */
#define CHANNEL_IF_DISABLE 0x0400             /* disabled - interference */
#define CHANNEL_RECVD_BEACON_PROBERESP 0x0800 /* Channel with activity */
#define CHANNEL_AP_DISABLE 0x1000             /* disabled by AP */
#define CHANNEL_RECVD_AP_CHAN_REPORT 0x2000   /* part of AP channel report */
#define CHANNEL_REG_DISABLE 0x4000            /*channel disabled by regulatory information*/
/* Bit 16-31 for channel attributes related to HT */
#define CHANNEL_REG_HT_ATTRIB_MASK 0xFFFF0000 /* Bit 16-31 HT realted */
#endif

#define CHANNEL_PHY_MODE(ch) ((ch)->ch_attrib & CHANNEL_WLANMODE_MASK)
#define CHANNEL_IS_11A(ch) (CHANNEL_PHY_MODE(ch) == MODE_11A_ONLY)
#define CHANNEL_IS_11G(ch) (CHANNEL_PHY_MODE(ch) == MODE_11G)
#define CHANNEL_IS_11B(ch) ((CHANNEL_PHY_MODE(ch) == MODE_11B) || CHANNEL_IS_11G(ch))
#ifdef SUPPORT_5GHZ
#ifdef CONFIG_WIFILIB_6GHZ
#define FREQ_IS_5G(freq)                                                                                               \
    (((freq > IEEE_FREQ_5GHZ_LOW_BOUND) && (freq <= IEEE_FREQ_5GHZ_HIGH_BOUND)) ||                                     \
     ((freq >= IEEE_FREQ_6GHZ_LOW_BOUND) && (freq <= IEEE_FREQ_6GHZ_HIGH_BOUND)))
#else
#define FREQ_IS_5G(freq) (((freq > IEEE_FREQ_5GHZ_LOW_BOUND) && (freq <= IEEE_FREQ_5GHZ_HIGH_BOUND)))
#endif /* CONFIG_WIFILIB_6GHZ */
#define FREQ_IS_2G(freq) ((freq >= IEEE_FREQ_2GHZ_LOW_BOUND) && (freq <= IEEE_FREQ_2GHZ_HIGH_BOUND))
#define CHANNEL_IS_2GHZ(ch) ((CHANNEL_IS_11G(ch) || CHANNEL_IS_11B(ch)) && FREQ_IS_2G((ch)->ch_freq))
//#define CHANNEL_IS_5GHZ(ch)     (ch >= IEEE_CHAN_5GHZ_OFFSET)
#define CHANNEL_IS_5GHZ(ch) (PHYMODE_IS_5G(ch->ch_wlanMode) && FREQ_IS_5G((ch)->ch_freq))
#else
#define CHANNEL_IS_2GHZ(ch) (CHANNEL_IS_11G(ch) || CHANNEL_IS_11B(ch))
#endif

#define CHANNEL_IS_ACTIVE(ch) (((ch)->ch_attrib & CHANNEL_ACTIVE) == CHANNEL_ACTIVE)

#ifdef SUPPORT_REGULATORY
#define REGULATORY_FLAG                                                                                                \
    (REGULATORY_CHAN_DISABLED | REGULATORY_CHAN_NO_IR | REGULATORY_CHAN_RADAR | REGULATORY_CHAN_INDOOR_ONLY)
#define CHANNEL_CAN_PROBE(ch) (((ch)->ch_attrib & (CHANNEL_ACTIVE | REGULATORY_FLAG)) == CHANNEL_ACTIVE)
#else
#define CHANNEL_CAN_PROBE(ch)                                                                                          \
    ((((ch)->ch_attrib & (CHANNEL_ACTIVE | CHANNEL_ACTIVE_SCAN)) == (CHANNEL_ACTIVE | CHANNEL_ACTIVE_SCAN)))
#endif /* SUPPORT_REGULATORY */
#define CHANNEL_HAS_ACTIVITY(ch) (((ch)->ch_attrib & CHANNEL_RECVD_BEACON_PROBERESP) == CHANNEL_RECVD_BEACON_PROBERESP)
#ifdef NT_FN_PRODUCTION_STATS
#define DEV_IEEE_STATS_INC(dev, stats) ((dev)->ic_stats.stats++)
#endif // NT_FN_PRODUCTION_STATS
#if defined(NT_FN_DEBUG_STATS) || defined(NT_FN_PRODUCTION_STATS)
#define DEV_UAPSD_STATS_INC(dev, stats) ((dev)->uapsd_stats_struct.stats++)
#endif // NT_FN_DEBUG_STATS || NT_FN_PRODUCTION_STATS

#define BEACON_BUFFER_SZ 512
#define WEP_CACHE_SIZE 256

#define WLAN_CIPHER_WEP_INV 0
#define WLAN_CIPHER_WEP40 1
#define WLAN_CIPHER_WEP104 2

#define WLAN_CIPHER_WEP40_LEN 5
#define WLAN_CIPHER_WEP104_LEN 13
#define WLAN_MAX_NUM_WEP_KEYS 4

enum if_state {
    IF_RESET = 0,
    IF_DOWN,
    IF_STARTING,
    IF_UP,
};

#define mic_err_flag_reset 0
#define mic_err_flag_set 1

#define VBM_SIZE (AP_MAX_NUM_STA / 8) + 1

#define BMISS_CHECK_MAC_BLOCK_TIMER_PERIOD 10
#define BMISS_CHECK_MAC_BLOCK_DURATION 5

/* Define feature flag bit mask, which use low word */
#define FEATURE_TSRS_ENABLED 0x00000004
#define FEATURE_QOS_SUPPORT_ENABLED 0x00000001

/* Define status flag bit mask, which use high word */
#define STATUS_AP_MCAST_PENDING 0x00400000

#define NONCE_LEN 32
#define GTK_LEN 32

#if ((defined NT_FN_RMF) || (defined NT_FN_WPA3))
#define IGTK_LEN 16
#endif // NT_FN_RMF

#ifdef NT_FN_WPS
/* Define status flag bit mask, wich use high word */
#define STATUS_WSC_SET 0x00010000 // To inform the WSC registration progress to target
//#define STATUS_HW_RESOURCE_OWNER            0x00020000 // STA-STA Concurrency Value 1 - Uses HW logic, Value 0- Uses
// SW logic #define STATUS_PROMISCUOUS_MODE             0x00040000 // 1 == promiscuous mode active, 0 == filtered mode
//<default> #define STATUS_SEND_DOT11_HEADER_TO_HOST    0x00080000 // 1 == send .11 headers to host #define
// STATUS_DEFRAG_ON_HOST               0x00100000 // 1 == defragmentation performed by host. #define
// STATUS_SEND_DECRYPT_ERROR_TO_HOST   0x00200000 // 1 == send decrypt err frames to host #define
// STATUS_AP_MCAST_PENDING             0x00400000 #define STATUS_GO_AUTH_PENDING     0x00800000 // 1 == indicated there
// is pending AUTH for p2p GO #define STATUS_DISCONNECT_PENDING           0x01000000 // 1 == disconnect is pending for
// p2p client due to GO absence #define STATUS_MCC_MEDIA_STREAM_ENABLED     0x10000000 // 1 == media stream is enabled.
// 0
//=== media stream is disabled
#endif // NT_FN_WPS

typedef struct {
    uint8_t uapsd_trigger_tid : 3; /** < tids to which qos null frames need to be sent */
} trigger_tid_list;

#define NT_CONN_TBL_AP_IDX 0
#define NT_CONN_TBL_STA_IDX 1
#define NT_CONN_TBL_CONN_START_IDX 2
#define NT_CONN_TBL_MAX_ENTRIES (AP_MAX_NUM_STA + 2)

#define VALID_AID(aid) (((aid) >= (NT_CONN_TBL_CONN_START_IDX) || ((aid) < NT_CONN_TBL_MAX_ENTRIES)) ? TRUE : FALSE)
#define VALID_STA_IDX_FOR_AP_MODE(sta_idx)                                                                             \
    ((((sta_idx) >= NT_CONN_TBL_CONN_START_IDX) || ((sta_idx) < NT_CONN_TBL_MAX_ENTRIES)) ? TRUE : FALSE)
#define VALID_STA_IDX_FOR_STA_MODE(sta_idx) (((sta_idx) == NT_CONN_TBL_STA_IDX) ? TRUE : FALSE)

#define NT_MAX_DEVICES 2
#define NT_DEV_AP_ID 0
#define NT_DEV_STA_ID 1
#define NT_DEFAULT_HAL_STA_ID 2
#define NT_DEV_INV_ID 0xFF

#ifdef ENABLE_TSF_SYNC_STATS
#define TSF_SYNC_MIN_SUPPORTED_PERIOD_MS 100
#define TSF_SYNC_TOLERANCE 10 // percent above which tsf sync update will be flagged out of time period

typedef struct tsf_periodic_sync_stats_s {
    uint64_t tsf_sync_enable_start_time;
    uint16_t probe_req_beacon_miss_count;
    uint16_t beacon_miss_count;
    uint16_t probe_req_timer_exp_count;
    uint16_t timer_exp_count;
    uint16_t probe_req_dropped_count;
    uint16_t probe_req_requeued_count;
    uint16_t probe_req_queue_hw_fail;
    uint16_t tsf_updated_from_beacon;
    uint16_t tsf_updated_from_probe_rsp;
    uint16_t tsf_within_time_period;
    uint16_t tsf_within_ten_percent_err;
    uint16_t tsf_out_of_time_period;
} tsf_periodic_sync_stats_t;
#endif

#ifdef SUPPORT_PERIODIC_TSF_SYNC
#define TSF_SYNC_NUM_MARGIN_SPS 1 // no of SP's before timeout SP to send probe req for tsf sync

typedef enum {
    PR_NO_QUEUE = 0,
    PR_WAIT_BCN,    /* probe req not to be queued if beacon is expected */
    PR_PENDING,     /*probe req to be queued in twt start*/
    PR_ADDED_TO_HW, /*probe req succesffuly queued to hw*/
    PR_SUCCESS,     /*probe req send succeeded*/
    PR_FAILED,      /*probe req send failed*/
} tsf_sync_probe_req_state_t;

typedef enum {
    TSF_SYNC_DISABLED = 0,
    TSF_SYNC_ENABLED,
} tsf_sync_enable_state_t;

typedef struct tsf_periodic_sync_ctx_s {
    tsf_sync_enable_state_t enabled;            /* If feature is enabled run time */
    tsf_sync_probe_req_state_t state_probe_req; /* 0- PENDING, 1- PR_ADDED_TO_HW 2- PR_SENT*/
    uint32_t time_period_ms;                    /* time period milli seconds */
    uint64_t last_sync_time;                    /* Last time TSF sync event was sent to Host*/
    uint64_t next_beacon_time;                  /* exp time of next beacon */
    uint64_t last_beacon_time;                  /* last time of beacon */
    uint8_t probe_req_retry_count;              /* no of times probe req is retried */
    uint8_t num_margin_sps;                     /* no. of SPs before the actual sync time, Probe req to be started */
#ifdef ENABLE_TSF_SYNC_STATS
    tsf_periodic_sync_stats_t *p_tsf_sync_stats;
#endif
} tsf_periodic_sync_ctx_t;
#endif

typedef struct anti_param_s {
    uint8_t rts_enable;
    uint8_t rts_rate;
    uint8_t threshold;
    uint8_t slot_time;
    uint8_t qid;
    uint8_t aifsn;
    uint16_t cw_min;
    uint16_t cw_max;
    uint16_t txop_limit;
    uint16_t ack_timeout;
    uint16_t delay;
    uint8_t cts_to_self_enable;
} anti_param_t;

#if defined(FEATURE_STA_ECSA) || defined(FEATURE_AP_ECSA)
typedef struct ecsa_ctx_s {
    NT_BOOL qcn_ie_added;         /*check whether qcn_ie has been sent through assoc*/
    uint8_t state;                /*check if ecsa request is pending, 0 = stopped ,1 = pending,2 = start*/
    uint8_t is_dfs;               /*If DFS channel then TX should be enabled in new channel post beacon RX */
    uint8_t channel_switch_mode;  /*mode 1 = No data transmission, 0 = transmission*/
    uint8_t new_op_class;         /*new operating class to switch*/
    uint8_t new_channel_no;       /*new channel number to swtich*/
    uint16_t new_channel_freq;    /*new channel freq to swtich*/
    uint8_t channel_switch_count; /*channel switch count = 0 for immediate switch, 1 for deferred*/
    uint32_t target_tsf;          /*lower 32 bits for target time at which channel switch should occur*/
    uint32_t rcv_tsf;             /*lower 32 bits for time at which CSA/ECSA frame received */
    nt_osal_timer_handle_t ecsa_timer;
} ecsa_ctx_t;
#endif /*FEATURE_STA_ECSA*/

#ifdef FM_PMK_CACHING
#define NT_PMKINFO_MAX 1
#define NT_PMKSA_MAX 5

typedef struct pmkinfo_s {
    uint8_t pmk[WMI_PMK_LEN];
    uint8_t pmkid[SAE_PMKID_LEN];
} pmkinfo_t;

typedef struct pmksa_s {
    uint8_t ni_macaddr[IEEE80211_ADDR_LEN]; /* peer mac address */
    uint8_t num;
    uint8_t replace_idx;
    pmkinfo_t pmkinfo[NT_PMKINFO_MAX];
} pmksa_t;

typedef struct pmkcaching_s {
    uint8_t num;
    uint8_t replace_idx;
    pmksa_t pmksa[NT_PMKSA_MAX];
} pmkcaching_t;
#endif /*FM_PMK_CACHING*/

typedef struct wpa3_tdi_ctx_s {
    unsigned char ssid[WMI_MAX_SSID_LEN + 1];
    uint8_t ssid_len;
    uint8_t mode;
    uint32_t last_update_time;
} wpa3_tdi_ctx_t;

typedef struct dev_common_s {
    uint8_t conc_mode;      // AP_STA concurrency or no concurrency
    uint8_t active_dev_cnt; // Active device count
    struct devh_s *devp[NT_MAX_DEVICES];

    WLAN_PHY_MODE ic_phymode;
    uint8_t enable_xpa;

    uint8_t chanlistsize;
    channel_t *chanlist;
    uint8_t dev_channels_supported; /*Total number of supported channels*/

    conn_t *connTbl;
#ifdef SUPPORT_REGULATORY
    phyrf_reg_rule_struct *Reg_Rules;
    phyrf_reg_exchange_struct *reg_info;
#endif /* SUPPORT_REGULATORY */

    /*for getting tx power*/
    wlan_tx_power_t power;
    void *p2p_ctx_pool;
} dev_common_t;

typedef enum {
    SUBTYPE_NONE = 0,
    SUBTYPE_BT,
    SUBTYPE_P2PDEV,
    SUBTYPE_P2PCLIENT,
    SUBTYPE_P2PGO,
    SUBTYPE_P2PDEV_DEDICATE,
    SUBTYPE_FW_P2PDEV,
    SUBTYPE_FW_P2PCLIENT,
    SUBTYPE_FW_P2PGO,
} NETWORK_SUBTYPE;

#define SUB_OPMODE_NONE SUBTYPE_NONE
#define SUB_OPMODE_BT SUBTYPE_BT
#define SUB_OPMODE_P2PDEV SUBTYPE_P2PDEV
#define SUB_OPMODE_P2P_CLIENT SUBTYPE_P2PCLIENT
#define SUB_OPMODE_P2P_GO SUBTYPE_P2PGO
#define SUB_OPMODE_P2PDEV_DEDICATE SUBTYPE_P2PDEV_DEDICATE
#define SUB_OPMODE_FW_P2PDEV SUBTYPE_FW_P2PDEV
#define SUB_OPMODE_FW_P2P_CLIENT SUBTYPE_FW_P2PCLIENT
#define SUB_OPMODE_FW_P2P_GO SUBTYPE_FW_P2PGO

typedef struct devh_s {
    dev_common_t *pDevCmn; /* device common pointer*/
    uint8_t devId;         /* device Id */
    uint8_t ic_opmode;     /* WHAL_OPMODE  AP or STA  */
    uint8_t ic_subopmode;
    uint8_t ifState;
    uint8_t ic_myaddr[IEEE80211_ADDR_LEN];
    uint8_t numConn; // max number of connections supported
    uint8_t lastAid; // last Aid assigned to connecting station

    uint32_t ic_caps;
    uint32_t ic_flags;
    uint32_t ic_flags2;
    uint32_t regCode;

    uint32_t tbtt_offset;
    uint8_t curr_chindex;

    struct bss_s *bssToConnect; /* BSS that we want to conn*/
    struct bss_s *bss;
    struct bss_s *bssList; /* Roaming candidates */
    conn_t *bssConn;

    void *umlme_info;
    void *frmgen_info;
    void *pWlanBeaconStruct;

    void *pPmStruct;
    void *pcm_struct;
    void *p_txrx_ctx; /* TXRX context */
    void *pdc_struct;
    void *pco_struct;

    WMI_PREAMBLE_POLICY preamblePolicy;

    conn_profile_t conn_profile;

    nt_hal_bss_t halBssInfo;   /* HAL BSS Structure */
    nt_hal_scan_t halScanInfo; /* HAL Backup Register values */
#ifdef NT_FN_PRODUCTION_STATS
    struct ieee80211_stats ic_stats; /* private statistics */
#endif                               // NT_FN_PRODUCTION_STATS

    AGGR_CFG aggr_cfg;

    struct ieee80211_rateset ic_sup_rates[MODE_MAX];

    uint8_t pmk[WMI_PMK_LEN];
    uint8_t pmk_len;

    uint8_t gmk[WMI_GMK_LEN];
    uint16_t gmk_len;
    uint8_t g_nonce[NONCE_LEN];
    uint8_t gtk[GTK_LEN];
#if ((defined NT_FN_RMF) || (defined NT_FN_WPA3))
    uint8_t igtk[IGTK_LEN];
    uint16_t igtk_keyix;
    uint8_t ipn[6];
#endif

    uint8_t configTSCPerTid;

    void *pro_struct;
    void *km;

    void *suppl_auth_ctxt;

#ifdef NT_FN_WPS
    void *wps_ctx;
#endif // NT_FN_WPS
    uint8_t passphrase[WMI_PASSPHRASE_LEN];
    uint8_t passphrase_len;
    unsigned char ssid[WMI_MAX_SSID_LEN + 1];
    uint8_t ssid_len;
    void *p2p_fw_ctx;
    void *p2p_ctx;
    bool p2p_fw_profile;
    TXRX_MODE_EXT phyModeExt;

    /**
     * sae_pwe - SAE mechanism for PWE derivation
     * 0 = hunting-and-pecking loop only
     * 1 = hash-to-element only
     * 2 = both hunting-and-pecking loop and hash-to-element enabled
     */
    int sae_pwe;
    struct sae_pt *pt;

    /**
     * sae_password - SAE password
     *
     * This parameter can be used to set a password for SAE. By default, the
     * passphrase value is used if this separate parameter is not used, but
     * passphrase follows the WPA-PSK constraints (8..63 characters) even
     * though SAE passwords do not have such constraints.
     */
    char *sae_password;

    /**
     * sae_password_id - SAE password identifier
     *
     * This parameter can be used to identify a specific SAE password. If
     * not included, the default SAE password is used instead.
     */
    char *sae_password_id;

    void *pRateCtrl;

    void *pApDevStruct;
    uint32_t apPsBitmapCtrl;
    uint32_t apPvb[VBM_SIZE]; /* Partial Virtual Bitmap - AP mode */
    uint32_t apPwrSaveBitmap;

    uint8_t dtim_count;
    uint8_t dtim_period;

#ifdef NT_FN_HT
    /* ht_cap_ie_info and ht_op_ie_info stores the contents that will be used in
     * assoc_req frames for STA mode and assoc_resp frames for AP mode.
     * Also the values stored will be used to dictate device behavior.
     */
    struct ieee80211_ie_htcap ht_cap_ie_info;
    struct ieee80211_ie_htinfo ht_op_ie_info;
    /* if ht_allowed == 0 then HT is not allowed by the host or internally for that band */
    uint8_t ht_allowed;
#endif // NT_FN_HT

    uint16_t rsn_cap;
    //#ifdef NT_FN_DEBUG_STATS
    //   WMI_DEVICE_WLAN_STATS       devWlanStats;
    //#endif //NT_FN_DEBUG_STATS
    void *wmi_event_filter;
    uint8_t mic_err_cnt; /* mic-error counter */
    TimerHandle_t mic_err_timer;
    TimerHandle_t tkip_wait_timer;

    uint16_t phyInt_fiq_fiq_count;
    uint16_t phyInt_fiq_irq_count;

#ifdef NT_FN_WMM_PS_STA
    TimerHandle_t qos_null_trigger_timer_hndl[4]; /**< timer used for sending qos null frames when supporting uapsd*/
    TimerHandle_t eosp_intr_wait_timer_hndl[4];   /**< timer used for waiting to received buffered qos data at sta side
                                                     after sending qos null frame*/
    trigger_tid_list triggering_tids[4];
    uint8_t trigger_timer_run_status : 4; /**< trigger timer running status indication flag */
#if defined(NT_FN_DEBUG_STATS) || defined(NT_FN_PRODUCTION_STATS)
    uapsd_stats_t uapsd_stats_struct;
#endif                              // NT_FN_DEBUG_STATS || NT_FN_PRODUCTION_STATS
    uint8_t pspoll_sent_status : 1; /**< flag maintained to check pspoll sent status**/
#endif                              // NT_FN_WMM_PS_STA
    uint8_t mic_err_dsbl_auth;
#ifdef NT_FN_PROTECTION
    uint8_t wifi_prot_mode;
#endif // NT_FN_PROTECTION
    struct ieee80211_ie_ext_cap_filed nt_eie_xcp_field;
#ifdef NT_FN_FTM
    void *pftmStruct;
#endif // NT_FN_FTM
#ifdef SUPPORT_EVENT_HANDLERS
    dev_ev_handler_info *ev_handler_info;
#endif /* SUPPORT_EVENT_HANDLERS */
#ifdef SUPPORT_PERIODIC_TSF_SYNC
    tsf_periodic_sync_ctx_t tsf_sync_ctx;
#endif /*SUPPORT_PERIODIC_TSF_SYNC*/
#if defined(FEATURE_STA_ECSA) || defined(FEATURE_AP_ECSA)
    ecsa_ctx_t *ecsa_ctx;
#endif
    struct anti_param_s anti_param;

#ifdef FM_PMK_CACHING
    pmkcaching_t pmkcaching;
#endif

    wpa3_tdi_ctx_t wpa3_tdi_ctx;
    bool is_pmk_set;

} devh_t;

typedef void (*RO_CSERV_COMPLETION_CB)(devh_t *dev, void *arg, nt_status_t status);

extern devh_t *gdevp;
extern dev_common_t *gpDevCommon;

#ifdef SUPPORT_EVENT_HANDLERS
/* Enter module name to register event handler */
enum dev_event_cb_modules {
    DEV_EVENT_CB_BT_COEX,
    DEV_EVENT_CB_MAX_EVENT_HANDLERS,
};

typedef void (*wlan_dev_event_handler)(devh_t *dev, wlan_dev_event *event);
struct wlan_dev_ev_handler_info {
    uint16_t num_ev_handlers;
    void (*evhandlers[DEV_EVENT_CB_MAX_EVENT_HANDLERS])(devh_t *dev, wlan_dev_event *event);
};
#endif /* SUPPORT_EVENT_HANDLERS */

#define AP_BG_NO_ERP 0 /* Must be 0, dont change */
#define AP_BG_SCANING 1
#define AP_BG_ERP 2

#define DEV_NO_PROTECTION_MODE 0
#define DEV_CTS_PROTECTION_MODE 1
#define DEV_RTS_CTS_PROTECTION_MODE 2

#define CONN_PROFILE_PTR(dev) (&((dev)->conn_profile))

#define INCR_IEEE80211_SEQNO(_x) ((_x) = ((_x) + 1) & 0xFFF)

#define DEV_GET_CHANNEL_NUM_MAX(dev) (dev)->chanlistsize
#define DEV_SET_CHANNEL_NUM_MAX(dev, num) (dev)->chanlistsize = num
#define DEV_GET_CHANNEL(dev, index) (&((dev)->chanlist[(index)]))

#define PROFILE_IS_VALID(prof) (((prof)->ssid.ssid_len > 0) || (!IEEE80211_ADDR_NULL((prof)->bssid)))
#define PROFILE_IS_INFRA(prof) (PROFILE_IS_VALID(prof) && ((prof)->wlan_opmode & INFRA_NETWORK))
#define PROFILE_IS_AP(prof) (PROFILE_IS_VALID(prof) && ((prof)->wlan_opmode & AP_NETWORK))

#define SSID_IS_VALID(ssidp) ((ssidp)->ssid_len > 0)

#define GET_BSS_FROM_DEVH(dev) ((dev)->bss)
#define GET_SSID_PTR(dev) ((CONN_PROFILE_PTR(dev))->ssid.ssid)
#define GET_SSID_LEN(dev) ((CONN_PROFILE_PTR(dev))->ssid.ssid_len)

#define CIPHER_TYPE_PROFILE(dev) ((CONN_PROFILE_PTR(dev))->pairwise_cipher)

#define DEVICE_RESET(dev) ((dev)->ifState == IF_RESET)
#define DEVICE_INITIALIZED(dev) ((dev)->ifState == IF_DOWN)
#define DEVICE_CONNECTED(dev) ((dev)->ifState == IF_UP)
#define DEVICE_DISCONNECTED(dev) ((dev)->ifState == IF_DOWN)
#define DEVICE_STARTING(dev) ((dev)->ifState == IF_STARTING)

/*
 * Profile Related Macros
 */

#define PROFILE_IS_WPA(prof) ((prof)->wlan_authmode & (WMI_WPA_AUTH | WMI_WPA_PSK_AUTH | WMI_WPA2_AUTH| WMI_WPA2_PSK_AUTH | WMI_WPA2_SHA256_AUTH | WMI_WPA3_SHA256_AUTH | WMI_WPA3_ENTERPRISE_ONLY_AUTH))

#define PROFILE_HAS_PRIVACY_ENABLED(prof) ((prof)->pairwise_cipher != NONE_CRYPT)

void wlan_init(dev_common_t **pDevCmn);
NT_BOOL wlan_deinit(dev_common_t **devref);
NT_BOOL wlan_dev_deinit(devh_t **devref);
devh_t *wlan_main_device_init(dev_common_t *pDevCmn, uint8_t mode);
void nt_wlan_init_dev_macaddr(devh_t *);
void nt_wlan_init_dev_macaddr_for_conc(dev_common_t *pDevCmn);
void nt_wlan_set_channel_idx(uint8_t chIdx);
uint8_t nt_wlan_chck_ch_idx(uint8_t curr_chindex, NT_BOOL flag);
void *nt_wlan_get_global_dev_addr(void *);

#ifdef FM_PMK_CACHING
void nt_wlan_save_pmk_info(devh_t *dev, conn_t *conn);
void nt_wlan_clear_pmk_info(devh_t *dev);
pmksa_t *nt_wlan_find_pmksa_by_addr(devh_t *dev, uint8_t *pAddr);
uint8_t *nt_wlan_find_pmk_by_pmkid(devh_t *dev, uint8_t *pAddr, uint8_t *pmkid);
void nt_wlan_clear_pmksa_by_addr(devh_t *dev, uint8_t *pAddr);
void nt_wlan_set_sae_pmk(devh_t *dev, uint8_t *pAddr, uint8_t *pmkid);
#endif /*FM_PMK_CACHING*/

#ifdef __cplusplus
}
#endif

#endif /* _WLAN_DEV_H_ */
