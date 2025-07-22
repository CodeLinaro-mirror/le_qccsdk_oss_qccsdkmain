/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*============================================================================
 * @file wifi_fw_mgmt_api.h
 * @brief Wifi Firmware wlan connection/data related APIs/Defs to be shared with Apps
 *=============================================================================*/
#ifndef WIFI_FW_MGMT_API_H
#define WIFI_FW_MGMT_API_H

/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include "wifi_fw_cmn_api.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define MAX_SSID_LEN          32
#define MAX_SCAN_BSSID        5
#define MAX_SCAN_RESULT       4
#define MAX_CHAN_NUM          11  // 21
#define MAX_CHAN_NUM_ADJUSTED (4 * ((MAX_CHAN_NUM + 1) / 4))
#define INVALID_ASSOC_ID      0xFFFF
#define WIFI_PASSPHRASE_LEN   64
#define MAX_FTM_TLV_LEN       1564 /* maximum size of TLV2.0 cmd / rsp */
#define UNIT_TEST_MAX_NUM_ARG 50

/*--------------------------------------------------------------------
 * Type Decleration
 *--------------------------------------------------------------------*/
typedef enum {
    WLAN_ENABLE_CMD = 0,
    WLAN_DISABLE_CMD,                    // 1
    WLAN_IF_ADD_CMD,                     // 2
    WLAN_SCAN_CMD,                       // 3
    WLAN_JOIN_CMD,                       // 4
    WLAN_DISCONNECT_CMD,                 // 5
    WLAN_PDEV_UTF_CMD,                   // 6
    WLAN_MODE_CMD,                       // 7
    WLAN_SET_PDEV_PARAM_CMD,             // 8
    WLAN_UNIT_TEST_CMD,                  // 9
    WLAN_SCAN_STOP_CMD,                  // 10
    WLAN_TWT_SETUP_CMD,                  // 11
    WLAN_TWT_TEARDOWN_CMD,               // 12
    WLAN_TWT_STATUS_CMD,                 // 13
    WLAN_PHYDBGDUMP_CMD,                 // 14
    WLAN_UPDATE_BI_CMD,                  // 15
    WLAN_SET_RESET_WAKELOCK_CMD,         // 16
    WLAN_PERIODIC_TSF_SYNC_CMD,          // 17
    WLAN_F2A_PULSE_ON_TWT_WAKEUP_CMD,    // 18
    WLAN_UPDATE_BMTT_CMD,                // 19
    WLAN_COEX_EVT_STATUS,                // 20
    WLAN_PERIODIC_TRAFFIC_SETUP_CMD,     // 21
    WLAN_PERIODIC_TRAFFIC_STATUS_CMD,    // 22
    WLAN_PERIODIC_TRAFFIC_TEARDOWN_CMD,  // 23
    WLAN_CLK_LATENCY_CMD,                // 24

    /* TBD: Need to add more command as part of another FR */
    WLAN_INVALID_CMD,
} wifi_api_cmds;

typedef enum {
    WLAN_ENABLE_EVT = 0,
    WLAN_DISABLE_EVT,                  // 1
    WLAN_IF_ADD_COMP_EVT,              // 2
    WLAN_SCAN_COMP_EVT,                // 3
    WLAN_JOIN_COMP_EVT,                // 4
    WLAN_DISCONNECT_EVT,               // 5
    WLAN_PDEV_UTF_EVT,                 // 6
    WLAN_MODE_EVT,                     // 7
    WLAN_SET_PDEV_PARAMS_EVT,          // 8
    WLAN_UNIT_TEST_EVT,                // 9
    WLAN_SCAN_START_EVT,               // 10
    WLAN_SCAN_STOP_EVT,                // 11
    WLAN_TWT_SETUP_EVT,                // 12
    WLAN_TWT_TEARDOWN_EVT,             // 13
    WLAN_TWT_STATUS_EVT,               // 14
    WLAN_PHYDBGDUMP_EVT,               // 15
    WLAN_UPDATE_BI_EVT,                // 16
    WLAN_SET_RESET_WAKELOCK_EVT,       // 17
    WLAN_PERIODIC_TSF_SYNC_START_EVT,  // 18
    WLAN_PERIODIC_TSF_SYNC_EVT,        // 19

    WLAN_F2A_PULSE_ON_TWT_WAKEUP_EVT,  // 20
    WLAN_UPDATE_BMTT_EVT,              // 21

    WLAN_COEX_EVT,                       // 22
    WLAN_PERIODIC_TRAFFIC_SETUP_EVT,     // 23
    WLAN_PERIODIC_TRAFFIC_STATUS_EVT,    // 24
    WLAN_PERIODIC_TRAFFIC_TEARDOWN_EVT,  // 25
    WLAN_CLK_LATENCY_EVT,                // 26

    /*/* TBD: Need to add more events as part of another FR */
    WLAN_INVALID_EVT,
} wifi_api_events;

typedef enum {
    WIFI_FOURWAY_HANDSHAKE_SUCCESS,
    WIFI_RECEIVED_ASSOC_RESP,
    WIFI_NO_NETWORK_AVAIL,
    WIFI_LOST_LINK,
    WIFI_DISCONNECT_CMD,
    WIFI_BSS_DISCONNECTED,
    WIFI_AUTH_FAILED,
    WIFI_ASSOC_FAILED,
    WIFI_SEC_HS_TO_RECV_M1,
    WIFI_SEC_HS_TO_RECV_M3,
    WIFI_RECEIVED_DEAUTH,
    WIFI_RECEIVED_DISASSOC,
    WIFI_IEEE80211_REASON_MIC_FAILURE,
    WIFI_REASON_UNKNOWN,
    WIFI_INVALID_REASON,
    WIFI_ECSA_SUCCESS,
    WIFI_ECSA_FAILURE,
} disconn_reason;

#ifndef CONFIG_WMI_EVENT
typedef enum {
    WIFI_PARAM_SET_PDEV_CHANNEL = 0,
    WIFI_PARAM_SET_AP_BCN_INTERVAL = 1,
    WIFI_PARAM_SET_PHYMODE = 2,
    WIFI_PARAM_SET_PDEV_COUNTRY_CODE = 3,
    WIFI_PARAM_SET_LOGGER_ATTACHED = 4,
    WIFI_PARAM_SET_BA_TIMEOUT = 5,
    WIFI_PARAM_SET_EB_LOCATION = 6,
} param_id;

enum {
    WIFI_STATUS_SUCCESS,
    WIFI_STATUS_FALIURE,
    WIFI_STATUS_INVALID_NETIF,
    WIFI_STATUS_INVALID_LEN,
    WIFI_STATUS_INVALID_ARGS,
    WIFI_STATUS_MAX,
};

typedef enum {
    HOST_LOGGER_DETACHED = 0,
    HOST_LOGGER_ATTACHED = 1,
} host_logger_state;
#endif

typedef enum {
    WLAN_MODE_11B,
    WLAN_MODE_11G,
    WLAN_MODE_11NG_HT20,
    WLAN_MODE_11A_ONLY,
    WLAN_MODE_11A_HT20,
    WLAN_MODE_11ABGN_HT20,
    WLAN_MODE_UNKNOWN,
    WLAN_MODE_MAX = WLAN_MODE_UNKNOWN,
} phymode;

/*Location of eb*/
typedef enum {
    EB_LEFT,
    EB_RIGHT,
    EB_LOCATION_MAX,  // should be last to signify undefined
} eb_location;

typedef struct {
    uint8_t ssid_len;
    uint8_t reserved[Reserve_24_BIT];
    uint8_t ssid[MAX_SSID_LEN];
} ssid_info;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ssid_info)
WIFI_FW_STRUCT_SIZE_SYNC(ssid_info, (4 + MAX_SSID_LEN))

typedef struct wifi_bss_info {
    uint16_t channel; /* channel in mhz */
    uint8_t bssid[IEEE80211_ADDR_LENGTH];
    ssid_info ssid;
    uint8_t security_mode;
    int8_t rssi;
    uint8_t wlan_mode;
    uint8_t reserved;
} wifi_bss_info_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wifi_bss_info_t)
WIFI_FW_STRUCT_SIZE_SYNC(wifi_bss_info_t, 48)

typedef struct {
    uint16_t msg_id;    /* The message id identifying the wlan cmd */
    uint8_t network_id; /* network_id associated with the command */
    uint8_t reserved;   /* reserved */
} wlan_cmd_hdr;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_cmd_hdr)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_cmd_hdr, 4)

typedef struct {
    uint16_t msg_id;    /* The message id identifying the wlan evt */
    uint8_t network_id; /* network_id associated with the command */
    uint8_t status;     /* over all status */
} wlan_evt_hdr;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_evt_hdr)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_evt_hdr, 4)

typedef struct {
    wlan_cmd_hdr cmd_hdr;      /* contains the message id, length and network id */
    uint32_t pdev_param_id;    /* header containg info about set_param */
    uint32_t pdev_param_value; /* param value to be set*/
} PACKSTRUCT wlan_set_pdev_param_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_set_pdev_param_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_set_pdev_param_cmd_t, 12)

typedef struct {
    wlan_cmd_hdr cmd_hdr;
    uint8_t bssid[IEEE80211_ADDR_LENGTH]; /* bssid of the ap to join */
    uint16_t reserved_for_bssid;
    ssid_info ssid;                              /*< SSID of the Wi-Fi network to join. */
    uint8_t password[WIFI_PASSPHRASE_LEN];       /* Password needed to join the AP. */
    uint8_t group_password[WIFI_PASSPHRASE_LEN]; /* Password needed to join the AP. */
    uint8_t password_len;
    uint8_t grp_passwd_len;
    uint8_t security_mode; /* Wi-Fi Security. Defines the wifi security type see */
    uint8_t wlan_mode;
    uint32_t ctrl_flags;
    uint16_t channel; /* channel in mhz */
    uint16_t reserved;
} PACKSTRUCT wlan_join_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_join_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_join_cmd_t, 188)

typedef struct {
    wlan_cmd_hdr cmd_hdr; /* contains the message id, length and network id */

    uint8_t chan_list[MAX_CHAN_NUM]; /* list of channels to scan*/
#if (MAX_CHAN_NUM_ADJUSTED != MAX_CHAN_NUM)
    uint8_t reserved[MAX_CHAN_NUM_ADJUSTED - MAX_CHAN_NUM];
#endif

    uint8_t num_chan;          /* no of channels to scan */
    uint8_t num_ssid;          /*number of ssid in ssid list*/
    uint16_t chdwell_duration; /*channel dwell duration can be active or passive duration based on probe type*/

    ssid_info ssid[MAX_SCAN_BSSID]; /*list of ssid to scan */

    uint8_t security_mode; /* security mode */
    uint8_t probe_type;    /* Active(1) or passive(2) scan */
    uint8_t ctrl_flag;     /* scan related flags*/
    uint8_t reserved2;
} PACKSTRUCT wlan_scan_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_scan_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_scan_cmd_t, (MAX_CHAN_NUM_ADJUSTED + MAX_SCAN_BSSID * 36 + 12))

typedef struct {
    wlan_cmd_hdr cmd_hdr; /* contains the message id, length and network id */
    uint16_t assoc_id;
    uint16_t reserved;
} PACKSTRUCT wlan_disconnect_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_disconnect_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_disconnect_cmd_t, 8)

typedef struct {
    wlan_cmd_hdr cmd_hdr; /* contains the message id, length and network id */

    uint8_t reserved1;
    uint8_t network_type; /* AP or STA*/
    uint8_t dhcp_type;    /* check ringif_dhcp_type */
    uint8_t ip_ver;       /* check ringif_ipaddr_type */

    uint32_t ipv4_addr;    /* If ip_ver is IP_VER_V4 */
    uint32_t ipv6_addr[4]; /* If ip_ver is IP_VER_V6 */
    uint32_t gateway;      /* gateway IP  used only in case of static dhcp */
    uint32_t netmask;      /* netmask used only in case of static dhcp */

} PACKSTRUCT wlan_if_add_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_if_add_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_if_add_cmd_t, 36)

typedef struct {
    wlan_cmd_hdr cmd_hdr;                /* contains the message id, length and network id */
    uint8_t tlv20_buff[MAX_FTM_TLV_LEN]; /* max length of TLV2.0 buffer */
} PACKSTRUCT wlan_pdev_utf_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_pdev_utf_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_pdev_utf_cmd_t, (MAX_FTM_TLV_LEN + 4))

typedef struct {
    wlan_cmd_hdr cmd_hdr;   /* contains the message id, length and network id */
    uint8_t requested_mode; /* 0 for MM 1 for FTM */
    uint8_t reserved[Reserve_24_BIT];
} PACKSTRUCT wlan_mode_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_mode_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_mode_cmd_t, 8)

typedef struct {
    wlan_cmd_hdr cmd_hdr; /* contains the message id, length and network id */
    uint8_t scan_id;
    uint8_t reserved[Reserve_24_BIT];
} PACKSTRUCT wlan_scan_stop_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_scan_stop_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_scan_stop_cmd_t, 8)

typedef struct {
    wlan_cmd_hdr cmd_hdr; /* contains the message id, length and network id */
    uint32_t next_tbtt_low;
    uint32_t next_tbtt_high;
    uint16_t beacon_multiplier;
    uint16_t freq;
} PACKSTRUCT wlan_update_bi_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_update_bi_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_update_bi_cmd_t, 16)

typedef struct {
    wlan_cmd_hdr cmd_hdr; /* contains the message id, length and network id */
    uint8_t enable;
    uint8_t reserved[Reserve_24_BIT];
} PACKSTRUCT wlan_f2a_pulse_on_twt_wakeup_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_f2a_pulse_on_twt_wakeup_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_f2a_pulse_on_twt_wakeup_cmd_t, 8)

typedef struct {
    wlan_cmd_hdr cmd_hdr; /* contains the message id, length and network id */
    uint32_t beacon_miss_thr_time_us;
} PACKSTRUCT wlan_update_bmtt_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_update_bmtt_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_update_bmtt_cmd_t, 8)

typedef enum {
    WLAN_PERIODC_TRAFFIC_EVT_STATUS_OK = 0,      /* Operation Successful */
    WLAN_PERIODC_TRAFFIC_EVT_INVALID_PARAM,      /* Invalid param */
    WLAN_PERIODC_TRAFFIC_EVT_NO_RESOURCE,        /* i.e No memory to allocate one more session */
    WLAN_PERIODC_TRAFFIC_EVT_FW_NOT_READY,       /* FW not ready or feature not enabled */
    WLAN_PERIODC_TRAFFIC_EVT_NO_ACK,             /* AP did not ACK the request/response frame */
    WLAN_PERIODC_TRAFFIC_EVT_NO_RESPONSE,        /* peer AP did not send the response frame */
    WLAN_PERIODC_TRAFFIC_EVT_TWT_ACTIVE,         /* twt interface is up */
    WLAN_PERIODC_TRAFFIC_EVT_AP_DISCONNECT,      /* STA is disconnected with AP */
    WLAN_PERIODC_TRAFFIC_EVT_NULL_POINTER,       /* When NULL point is passed */
    WLAN_PERIODC_TRAFFIC_EVT_TEARDOWN_ON_CMD_RX, /* Teardown event when we receive teardown cmd */
    WLAN_PERIODC_TRAFFIC_EVT_UNKNOWN_ERROR,      /* Unknown Error */
                                                 /* Add more Error code here */
} wlan_periodic_traffic_evt_status_t;

typedef enum {
    WLAN_PERIODIC_TRAFFIC_AUDIO = 0, /* Audio traffic */
    WLAN_PERIODIC_TRAFFIC_VOICE,     /* Voice traffic */
    WLAN_PERIODIC_TRAFFIC_INVALID,   /* Invalid traffic type */
} wlan_periodic_traffic_type_t;

typedef enum {
    WLAN_PERIODIC_TRAFFIC_SESSION_ID = 0,     /* Valid session id */
    WLAN_PERIODIC_TRAFFIC_SESSION_ID_INVALID, /* Invalid session id */
} wlan_periodic_traffic_session_id_t;

typedef struct {
    wlan_cmd_hdr cmd_hdr;   /* contains the message id, length and network id */
    uint32_t wake_interval; /* wake interval, Audio: 70ms, 130ms Voice: 30ms*/
    uint32_t start_tsf_lo;  /* periodic traffic start tsf lower bits */
    uint32_t start_tsf_hi;  /* periodic traffic start tsf higher bits */
    uint8_t traffic_type;   /* periodic traffic type: Audio, Voice */
    uint8_t session_id;     /* periodic traffic session id: valid = 0, rest = invalid */
    uint16_t reserved;      /* reserved for future use cases */
} PACKSTRUCT wlan_periodic_traffic_setup_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_periodic_traffic_setup_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_periodic_traffic_setup_cmd_t, 20)

typedef struct {
    wlan_cmd_hdr cmd_hdr; /* contains the message id, length and network id */
    uint8_t traffic_type; /* periodic traffic type: Audio, Voice */
    uint8_t session_id;   /* periodic traffic session id: valid = 0, rest = invalid */
    uint16_t reserved;    /* reserved for future use cases */
} PACKSTRUCT wlan_periodic_traffic_status_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_periodic_traffic_status_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_periodic_traffic_status_cmd_t, 8)

typedef struct {
    wlan_cmd_hdr cmd_hdr; /* contains the message id, length and network id */
    uint8_t traffic_type; /* periodic traffic type: Audio, Voice */
    uint8_t session_id;   /* periodic traffic session id: valid = 0, rest = invalid */
    uint16_t reserved;    /* reserved for future use cases */
} PACKSTRUCT wlan_periodic_traffic_teardown_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_periodic_traffic_teardown_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_periodic_traffic_teardown_cmd_t, 8)

typedef struct {
    wlan_cmd_hdr cmd_hdr; /* contains the message id, length and network id */
    uint32_t clk_latency; /* clock latency */
} PACKSTRUCT wlan_clk_latency_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_clk_latency_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_clk_latency_cmd_t, 8)

/*-----------------------------------Wlan Events-------------------------------------*/

typedef struct {
    wlan_evt_hdr evt_hdr; /* contains the message id, length and network id */
    uint32_t reserved;
} PACKSTRUCT wlan_disable_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_disable_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_disable_evt_t, 8)

typedef struct {
    wlan_evt_hdr evt_hdr; /* contains the message id, length and network id */
    uint8_t network_id;   /* netif id  for the interface */
    uint8_t reserved[Reserve_24_BIT];
} PACKSTRUCT wlan_if_add_comp_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_if_add_comp_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_if_add_comp_evt_t, 8)

typedef struct {
    wlan_evt_hdr evt_hdr; /* contains the message id, length and network id */
    uint8_t scan_id;      /* identifier for specific scan */
    uint8_t reserved[Reserve_24_BIT];
} PACKSTRUCT wlan_scan_start_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_scan_start_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_scan_start_evt_t, 8)

typedef struct {
    wlan_evt_hdr evt_hdr;     /* contains the message id, length and network id */
    uint8_t num_bss_curr_evt; /* no of bss in current event */
    uint8_t num_bss_rem;      /* no of bss remaining */
    uint8_t scan_id;
    uint8_t reserved;
    wifi_bss_info_t scan_bss_info[MAX_SCAN_RESULT]; /* bss info */
} PACKSTRUCT wlan_scan_comp_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_scan_comp_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_scan_comp_evt_t, (8 + MAX_SCAN_RESULT * sizeof(wifi_bss_info_t)))

typedef struct {
    wlan_evt_hdr evt_hdr; /* contains the message id, length and network id */
    uint8_t scan_id;      /* identifier for specific scan */
    uint8_t reserved[Reserve_24_BIT];
} PACKSTRUCT wlan_scan_stop_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_scan_stop_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_scan_stop_evt_t, 8)

typedef struct {
    wlan_evt_hdr evt_hdr;                 /* contains the message id, length and network id */
    uint8_t bssid[IEEE80211_ADDR_LENGTH]; /* bssid of the ap joined */
    uint16_t reserved_for_bssid;
    ssid_info ssid;         /*ssid of joind AP */
    uint16_t assoc_id;      /* association id */
    uint8_t host_initiated; /* Specify whether join is host initiated or not*/
    uint8_t reason_code;
    uint16_t channel_frequency;
    uint16_t reserved;
} PACKSTRUCT wlan_join_comp_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_join_comp_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_join_comp_evt_t, 56)

typedef struct {
    wlan_evt_hdr evt_hdr;                    /* contains the message id, length and network id */
    uint8_t mac_addr[IEEE80211_ADDR_LENGTH]; /* the assigned mac address */
    uint8_t num_networks;                    /* no of vdev supported */
    uint8_t reserved;                        /* reserved */
    uint32_t cap_info;                       /*capibility info bitmap*/
    uint32_t cap_info2;
} PACKSTRUCT wlan_enable_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_enable_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_enable_evt_t, 20)

typedef struct {
    wlan_evt_hdr evt_hdr;   /* contains the message id, length and network id */
    uint16_t assoc_id;      /* association id */
    uint16_t reason_code;   /* specify the type of disconnection */
    uint8_t host_initiated; /* Specify whether join is host initiated or not*/
    uint8_t reserved[Reserve_24_BIT];
} PACKSTRUCT wlan_disconnect_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_disconnect_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_disconnect_evt_t, 12)

typedef struct {
    wlan_evt_hdr evt_hdr; /* contains the message id, length and network id */
    uint8_t param_id;
    uint8_t reserved[Reserve_24_BIT];
} PACKSTRUCT wlan_set_pdev_param_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_set_pdev_param_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_set_pdev_param_evt_t, 8)

typedef struct {
    wlan_evt_hdr evt_hdr; /* contains the message id, length and network id */
    uint8_t actual_mode;  /* 0 for MM 1 for FTM */
    uint8_t reserved[Reserve_24_BIT];
} PACKSTRUCT wlan_mode_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_mode_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_mode_evt_t, 8)

typedef struct {
    wlan_cmd_hdr cmd_hdr;
    uint16_t reserved_1;
    uint8_t dialog_id;         /* unique identifier between WLAN Firmware and App to representing a twt session > 0 */
    uint8_t negotiation_type;  /* 0: Individual TWT, 1: Broadcast TWT */
    uint32_t wake_duration;    /* wake duration for device. Its TWT SP (Service period)in ms */
    uint32_t wake_interval;    /* Time between wake duration. Its TWT SI (Service Interval) in ms */
    uint32_t twt_start_tsf_lo; /* Its lower 32 bit of Start TSF. App to specify when first twt starts, TWT Offset tsf.
                                  When its set to 0 (hi and lo), FW will decide the value */
    uint32_t twt_start_tsf_hi; /* Its higher 32 bit of start tsf. */
    uint8_t flow_type;         /* 0: Announced, 1: unannounced */
    uint8_t trigger_type;      /* 0: non-triggered, 1: triggered twt */
    uint16_t reserved_2;
} PACKSTRUCT wlan_twt_setup_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_twt_setup_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_twt_setup_cmd_t, 28)

typedef struct {
    wlan_evt_hdr evt_hdr;
    uint16_t reserved_1;
    uint8_t dialog_id;         /* unique identifier between WLAN Firmware and App to representing a twt session > 0 */
    uint8_t negotiation_type;  /* 0: Individual TWT, 1: Broadcast TWT */
    uint32_t wake_duration;    /* wake duration for device. Its TWT SP (Service period)in ms */
    uint32_t wake_interval;    /* Time between wake duration. Its TWT SI (Service Interval) in ms */
    uint32_t twt_start_tsf_lo; /* Its lower 32 bit of Start TSF. App to specify when first twt starts, TWT Offset tsf.
                                  When its set to 0 (hi and lo), FW will decide the value */
    uint32_t twt_start_tsf_hi; /* Its higher 32 bit of start tsf. */
    uint8_t flow_type;         /* 0: Announced, 1: unannounced */
    uint8_t trigger_type;      /* 0: non-triggered, 1: triggered twt */
    uint8_t reason_code;       /* reason code wlan_twt_evt_status_t */
    uint8_t host_initiated;    /* host initiated or unsolicited */
    uint32_t offset_from_tsf;  /* wake tsf -> offset from the effective tsf - field required for SUPPORT_STA_TWT_RENEG
                                  feature */
} PACKSTRUCT wlan_twt_setup_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_twt_setup_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_twt_setup_evt_t, 32)

typedef struct {
    wlan_cmd_hdr cmd_hdr;
    uint16_t reserved_1;
    uint8_t dialog_id;  /* unique identifier between WLAN Firmware and App to representing a twt session > 0 */
    uint8_t reserved_2; /* reserved */
} PACKSTRUCT wlan_twt_teardown_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_twt_teardown_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_twt_teardown_cmd_t, 8)

typedef struct {
    wlan_evt_hdr evt_hdr;
    uint8_t reserved_1;
    uint8_t host_initiated;
    uint8_t dialog_id;   /* unique identifier between WLAN Firmware and App to representing a twt session > 0 */
    uint8_t reason_code; /* reason code wlan_twt_evt_status_t */
} PACKSTRUCT wlan_twt_teardown_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_twt_teardown_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_twt_teardown_evt_t, 8)

typedef struct {
    wlan_evt_hdr evt_hdr;
    uint32_t address;
    uint32_t length;
} PACKSTRUCT wlan_phydbgdump_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_phydbgdump_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_phydbgdump_evt_t, 12)

typedef struct {
    wlan_cmd_hdr cmd_hdr;
    uint16_t reserved_1;
    uint8_t dialog_id;  /* unique identifier between WLAN Firmware and App to representing a twt session > 0 */
    uint8_t reserved_2; /* reserved */
} PACKSTRUCT wlan_twt_status_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_twt_status_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_twt_status_cmd_t, 8)

typedef struct {
    wlan_evt_hdr evt_hdr;
    uint16_t reserved_1;
    uint8_t dialog_id;         /* unique identifier between WLAN Firmware and App to representing a twt session > 0 */
    uint8_t negotiation_type;  /* 0: Broadcast TWT, 1: Individual TWT */
    uint32_t wake_duration;    /* wake duration for device. Its TWT SP (Service period)in ms */
    uint32_t wake_interval;    /* Time between wake duration. Its TWT SI (Service Interval) in ms */
    uint32_t twt_start_tsf_lo; /* Its lower 32 bit of Start TSF. App to specify when first twt starts, TWT Offset tsf.
                                  When its set to 0 (hi and lo), FW will decide the value */
    uint32_t twt_start_tsf_hi; /* Its higher 32 bit of start tsf. */
    uint8_t flow_type;         /* 0: Announced, 1: unannounced */
    uint8_t trigger_type;      /* 0: non-triggered, 1: triggered twt */
    uint8_t reason_code;       /* reason code wlan_twt_evt_status_t */
    uint8_t reserved_2;        /* reserved */
} PACKSTRUCT wlan_twt_status_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_twt_status_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_twt_status_evt_t, 28)

typedef struct {
    wlan_cmd_hdr cmd_hdr;
    uint32_t time_period_ms; /* time period within which to do TSF sync*/
} PACKSTRUCT wlan_periodic_tsf_sync_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_periodic_tsf_sync_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_periodic_tsf_sync_cmd_t, 8)

typedef struct {
    wlan_evt_hdr evt_hdr;
} PACKSTRUCT wlan_periodic_tsf_sync_start_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_periodic_tsf_sync_start_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_periodic_tsf_sync_start_evt_t, 4)

typedef struct {
    wlan_evt_hdr evt_hdr;
    uint32_t tsf_lo;
    uint32_t tsf_hi;
} PACKSTRUCT wlan_periodic_tsf_sync_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_periodic_tsf_sync_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_periodic_tsf_sync_evt_t, 12)

typedef enum {
    WLAN_TWT_EVT_STATUS_OK = 0,
    WLAN_TWT_EVT_DIALOG_ID_NOT_EXIST = 1,
    WLAN_TWT_EVT_INVALID_PARAM = 2,
    WLAN_TWT_EVT_NO_RESOURCE = 3,
    WLAN_TWT_EVT_FW_NOT_READY = 4,
    WLAN_TWT_EVT_NO_ACK = 5,
    WLAN_TWT_EVT_NO_RESPONSE = 6,
    WLAN_TWT_EVT_DENIED = 7,
    WLAN_TWT_EVT_UNKNOWN_ERROR = 8,
    WLAN_TWT_EVT_STA_NOT_ASSOCIATED = 9,
    WLAN_TWT_EVT_RENEGOTIATION_SUCCESS = 10,
} wlan_twt_evt_status_t;

typedef struct {
    wlan_cmd_hdr cmd_hdr;
    uint8_t vdev_id;
    uint8_t module_id;
    uint8_t num_args;
    uint8_t reserved;
    uint32_t args[UNIT_TEST_MAX_NUM_ARG];
} PACKSTRUCT wlan_unit_test_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_unit_test_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_unit_test_cmd_t, 208)

typedef struct {
    wlan_evt_hdr evt_hdr;
} PACKSTRUCT wlan_unit_test_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_unit_test_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_unit_test_evt_t, 4)

typedef struct {
    wlan_evt_hdr evt_hdr;                /* contains the message id, length and network id */
    uint8_t tlv20_buff[MAX_FTM_TLV_LEN]; /* max length of TLV2.0 buffer */
} PACKSTRUCT wlan_pdev_utf_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_pdev_utf_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_pdev_utf_evt_t, (4 + MAX_FTM_TLV_LEN))

typedef struct {
    wlan_evt_hdr evt_hdr; /* contains the message id, length and network id */
} PACKSTRUCT wlan_update_bi_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_update_bi_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_update_bi_evt_t, 4)

typedef enum {
    WIFI_WAKELOCK_TYPE_RESET,
    WIFI_WAKELOCK_TYPE_SET,
} wakelock_type;

typedef struct {
    wlan_cmd_hdr cmd_hdr; /* contains the message id, length and network id */
    uint8_t wakelock_command_type;
    uint8_t reserved[Reserve_24_BIT];
} PACKSTRUCT wlan_set_reset_wakelock_cmd_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_set_reset_wakelock_cmd_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_set_reset_wakelock_cmd_t, 8)

typedef struct {
    wlan_evt_hdr evt_hdr; /* contains the message id, length and network id */
    uint8_t wakelock_event_type;
    uint8_t reserved[Reserve_24_BIT];
} PACKSTRUCT wlan_set_reset_wakelock_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_set_reset_wakelock_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_set_reset_wakelock_evt_t, 8)

typedef struct {
    wlan_evt_hdr evt_hdr; /* contains the message id, length and network id */
    uint8_t status;
    uint8_t reserved[Reserve_24_BIT];
} PACKSTRUCT wlan_f2a_pulse_on_twt_wakeup_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_f2a_pulse_on_twt_wakeup_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_f2a_pulse_on_twt_wakeup_evt_t, 8)

typedef struct {
    wlan_evt_hdr evt_hdr; /* contains the message id, length and network id */
} PACKSTRUCT wlan_update_bmtt_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_update_bmtt_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_update_bmtt_evt_t, 4)

typedef struct {
    wlan_evt_hdr evt_hdr; /* contains the message id, length and network id */
    uint8_t event_type;
    uint8_t reserved[Reserve_24_BIT];
} PACKSTRUCT wlan_cxc_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_cxc_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_cxc_evt_t, 8)

typedef struct {
    wlan_cmd_hdr cmd_hdr; /* contains the message id, length and network id */
    uint8_t status;
    uint8_t reserved[Reserve_24_BIT];
} PACKSTRUCT wlan_cxc_evt_status_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_cxc_evt_status_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_cxc_evt_status_t, 8)

typedef struct {
    wlan_evt_hdr evt_hdr;   /* contains the message id, status and network id */
    uint32_t wake_interval; /* wake interval, Audio: 70ms, 130ms Voice: 30ms*/
    uint32_t start_tsf_lo;  /* periodic traffic start tsf lower bits */
    uint32_t start_tsf_hi;  /* periodic traffic start tsf higher bits */
    uint8_t traffic_type;   /* periodic traffic type: Audio, Voice */
    uint8_t session_id;     /* periodic traffic session id: valid = 0, rest = invalid */
    uint8_t status;         /* periodic traffic setup status */
    uint8_t reserved;       /* reserved for future use cases */
} PACKSTRUCT wlan_periodic_traffic_setup_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_periodic_traffic_setup_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_periodic_traffic_setup_evt_t, 20)

typedef struct {
    wlan_evt_hdr evt_hdr;    /* contains the message id, status and network id */
    uint8_t teardown_reason; /* periodic traffic teardown reason */
    uint8_t traffic_type;    /* periodic traffic type: Audio, Voice */
    uint8_t session_id;      /* periodic traffic session id: valid = 0, rest = invalid */
    uint8_t reserved;        /* reserved for future use cases */
} PACKSTRUCT wlan_periodic_traffic_teardown_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_periodic_traffic_teardown_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_periodic_traffic_teardown_evt_t, 8)

typedef struct {
    wlan_evt_hdr evt_hdr;    /* contains the message id, status and network id */
    uint32_t wake_interval;  /* wake interval, Audio: 70ms, 130ms Voice: 30ms*/
    uint32_t next_sp_tsf_lo; /* periodic traffic start tsf lower bits */
    uint32_t next_sp_tsf_hi; /* periodic traffic start tsf higher bits */
    uint8_t traffic_type;    /* periodic traffic type: Audio, Voice */
    uint8_t session_id;      /* periodic traffic session id: valid = 0, rest = invalid */
    uint16_t reserved;       /* reserved for future use cases */
} PACKSTRUCT wlan_periodic_traffic_status_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_periodic_traffic_status_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_periodic_traffic_status_evt_t, 20)

typedef struct {
    wlan_evt_hdr evt_hdr; /* contains the message id, status and network id */
} PACKSTRUCT wlan_clk_latency_evt_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wlan_clk_latency_evt_t)
WIFI_FW_STRUCT_SIZE_SYNC(wlan_clk_latency_evt_t, 4)

#endif /* INC_NT_WIFI_API_LIB_H_ */
