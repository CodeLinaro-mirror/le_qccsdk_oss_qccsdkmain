/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef DATA_PATH_H_
#define DATA_PATH_H_

#include <sys/_stdint.h>

#include "bd.h"
#include "dpm_ip.h"
#include "defines.h"
#include "wifi_cmn.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"  /* Resolve the NT_TST_TIME_STAMP_ENABLE. */
#include "nt_common.h" /* Resolve the NT_MAC_ADDR_SIZE/NT_MAX_SSID_LEN/NT_IPV4_ADDR_SIZE */
#include "data_path_sys.h"
#include "nt_osal.h"
#include "lwip/apps/nt_dhcps.h"

#ifdef SUPPORT_RING_IF
#include "stdbool.h"
#endif

#define NT_MAX_NO_OF_STA_ENTRY 4 /** Maximum sta entry allowed in data path adapter. */
#define NT_MAX_NO_OF_DEVICES   2 /** Maximum device entry allowed in data path adapter. */
#define NT_MAX_NO_OF_TID       8 /** Maximum TID support for BA session. */

#define NT_MAC_MC_IPV6_CHECK \
    2 /** IPv6 we are checking only 1st 16-bits (2 bytes) as multicast over Ethernet prepends 33-33. */
#define NT_MAC_MC_IPV4_CHECK \
    3 /** IPv4 we are checking only 1st 24-bits (3 bytes) as multicast over Ethernet prepends 01-00-5E. */
#define NT_MAC_MC_ETH_CHECK 3 /** Ethernet Multicast Addresses prepends 01-80-C2. */

#define NT_DPM_SNAP_HEADER_SIZE 8  /** SNAP Hearde 3 bytes OUI 3 bytes EtherType 2 bytes (Total = 8 bytes). */
#define NT_DPM_ETH_HDR_SIZE     14 /** Ethernet header size 14 bytes sizeof(ethernet_header_t). */

#define NT_DPM_TASK_STACK_SIZE 400 /** Data_path task stack size. */
#define NT_DPM_TASK_PRIORITY   6   /** Data_path task priority. */

#define NT_TX_BUFFER_OFFSET 0x68 /** fixed off-set for packet start in bd for data frame. */

#define NT_DPM_MAC_MTU_SIZE 2304 /** 802.11 MTU Size */

#define NT_DPM_MULTICAST_BIT 1 /** Multicast bit */

#define NT_CONVERT_TO_DBM(rssi) ((rssi)-100)
#ifdef PLATFORM_FERMION
#define NT_DPM_GET_RSSI(phystat) (halphy_rssi_correction)(((phystat)&0xFF000000) >> 24)
#else /* PLATFORM_FERMION */
#define NT_DPM_GET_RSSI(phystat) (((phystat)&0xFF000000) >> 24)
#endif /* PLATFORM_FERMION */

/*****************************************EDIT BELOW***************************************************/

/** DXE Channel Configuration macro. */

#define NT_DXE_TX_NO_DESC     3  /** Number of descriptors for DXE that can be queued for transfer at one time. */
#define NT_DXE_RX_NO_DESC     3  /** Number of descriptors for DXE that can be queued for receive at one time. */
#define NT_DXE_REF_WQ_TX      6  /** DXE Reference WQ TX */
#define NT_DXE_REF_WQ_RX      11 /** DXE Reference WQ RX */
#define NT_DXE_CH_PRIO        4  /** DXE channel priority */
#define NT_DXE_BD_STATE       1  /** BD status i.e. BD attached to frames for this pipe */
#define NT_DXE_BD_TX_IDX      0  /** BD Template Index for H2B Transfer. */
#define NT_DXE_MAX_CHUNK_SIZE 0  /** Maximum chunk size. */
#define NT_DXE_BMU_THRESH_TX  5  /** BMU Threshold for TX. */
#define NT_DXE_BMU_THRESH_RX  6  /** BMU Threshold for RX. */
#define NT_DXE_USE_SHORT_DESC 0  /** Enable short desc format if 0 disabled and 1 enabled. */

#define NT_DXE_TX_NO_DESC_BK                                                                                          \
    NT_DXE_TX_NO_DESC /** Number of descriptors for DXE that can be queued for transfer at one time for Background AC \
                       */
#define NT_DXE_TX_NO_DESC_BE                                                                                           \
    NT_DXE_TX_NO_DESC /** Number of descriptors for DXE that can be queued for transfer at one time for Best effort AC \
                       */
#define NT_DXE_TX_NO_DESC_VI \
    NT_DXE_TX_NO_DESC /** Number of descriptors for DXE that can be queued for transfer at one time for Video AC  */
#define NT_DXE_TX_NO_DESC_VO \
    NT_DXE_TX_NO_DESC /** Number of descriptors for DXE that can be queued for transfer at one time for Voice AC  */

#define NT_DXE_CH_PRIO_BK 3 /** DXE channel priority for Background AC */
#define NT_DXE_CH_PRIO_BE 4 /** DXE channel priority Best effort AC */
#define NT_DXE_CH_PRIO_VI 5 /** DXE channel priority Video AC */
#define NT_DXE_CH_PRIO_VO 6 /** DXE channel priority Voice AC*/

// Frame Type definitions

#define NT_MGMT_FRAME 0x0
#define NT_CTRL_FRAME 0x1
#define NT_DATA_FRAME 0x2

// Control frame subtype definitions
#define NT_CTRL_RR         3
#define NT_CTRL_BR_POLL    4
#define NT_CTRL_NDPA       5
#define NT_CTRL_WRAPPER    7
#define NT_CTRL_BAR        8
#define NT_CTRL_BA         9
#define NT_CTRL_PS_POLL    10
#define NT_CTRL_RTS        11
#define NT_CTRL_CTS        12
#define NT_CTRL_ACK        13
#define NT_CTRL_CF_END     14
#define NT_CTRL_CF_END_ACK 15

// Data frame subtype definitions
#define NT_DATA_DATA              0
#define NT_DATA_DATA_ACK          1
#define NT_DATA_DATA_POLL         2
#define NT_DATA_DATA_ACK_POLL     3
#define NT_DATA_NULL              4
#define NT_DATA_NULL_ACK          5
#define NT_DATA_NULL_POLL         6
#define NT_DATA_NULL_ACK_POLL     7
#define NT_DATA_QOS_DATA          8
#define NT_DATA_QOS_DATA_ACK      9
#define NT_DATA_QOS_DATA_POLL     10
#define NT_DATA_QOS_DATA_ACK_POLL 11
#define NT_DATA_QOS_NULL          12
#define NT_DATA_QOS_NULL_ACK      13
#define NT_DATA_QOS_NULL_POLL     14
#define NT_DATA_QOS_NULL_ACK_POLL 15

#define NT_DATA_QOS_MASK  8
#define NT_DATA_NULL_MASK 4
#define NT_DATA_POLL_MASK 2
#define NT_DATA_ACK_MASK  1

// Management frame subtype definitions
#define NT_MGMT_ASSOC_REQ     0x0
#define NT_MGMT_ASSOC_RSP     0x1
#define NT_MGMT_REASSOC_REQ   0x2
#define NT_MGMT_REASSOC_RSP   0x3
#define NT_MGMT_PROBE_REQ     0x4
#define NT_MGMT_PROBE_RSP     0x5
#define NT_MGMT_BEACON        0x8
#define NT_MGMT_ATIM          0x9
#define NT_MGMT_DISASSOC      0xA
#define NT_MGMT_AUTH          0xB
#define NT_MGMT_DEAUTH        0xC
#define NT_MGMT_ACTION        0xD
#define NT_MGMT_ACTION_NO_ACK 0xE
#define NT_MGMT_RESERVED15    0xF

// Action frame categories
#define NT_ACTION_SPECTRUM_MGMT 0
#define NT_ACTION_QOS_MGMT      1
#define NT_ACTION_DLP           2
#define NT_ACTION_BLKACK        3
#define NT_ACTION_WME           17
#define NT_ACTION_VHT           21

// QoS management action codes
#define NT_QOS_ADD_TS_REQ 0
#define NT_QOS_ADD_TS_RSP 1
#define NT_QOS_DEL_TS_REQ 2
#define NT_QOS_SCHEDULE   3

// Spectrum management action codes
#define NT_ACTION_MEASURE_REQUEST_ID 0
#define NT_ACTION_MEASURE_REPORT_ID  1
#define NT_ACTION_TPC_REQUEST_ID     2
#define NT_ACTION_TPC_REPORT_ID      3
#define NT_ACTION_CHANNEL_SWITCH_ID  4

// Measurement Request/Report Type
#define NT_BASIC_MEASUREMENT_TYPE 0
#define NT_CCA_MEASUREMENT_TYPE   1
#define NT_RPI_MEASUREMENT_TYPE   2

// DLP action frame types
#define NT_DLP_REQ      0
#define NT_DLP_RSP      1
#define NT_DLP_TEARDOWN 2

// block acknowledgement action frame types
#define NT_BLKACK_ADD_REQ 0
#define NT_BLKACK_ADD_RSP 1
#define NT_BLKACK_DEL     2

// VHT Action field values
#define NT_ACTION_VHT_CBF              0
#define NT_ACTION_VHT_GID_MGMT         1
#define NT_ACTION_VHT_OPER_MODE_NOTIFY 2

#define NT_MAX_RANDOM_LENGTH 2306

#define DEFAULT_TIMEOUT 10 /* seconds */
/******************************************************************************************************/

#define SEQNO_MAX        (4096)
#define SEQNO_DIFF(a, b) (((a) >= (b)) ? ((a) - (b)) : ((a) + (SEQNO_MAX) - (b)))
#define IS_BROADCAST_MULTICAST(addr) \
    ((!memcmp(addr, bc_add, NT_MAC_ADDR_SIZE)) || ((addr[0] & NT_DPM_MULTICAST_BIT) == 0x1))

#define dp_htonl(x)                                                                   \
    ((((x) & (uint32_t)0x000000ffUL) << 24) | (((x) & (uint32_t)0x0000ff00UL) << 8) | \
     (((x) & (uint32_t)0x00ff0000UL) >> 8) | (((x) & (uint32_t)0xff000000UL) >> 24))

#define dp_ntohl(x) dp_htonl(x)

#define dp_htons(x) ((uint16_t)((((x) & (uint16_t)0x00ffU) << 8) | (((x) & (uint16_t)0xff00U) >> 8)))
#define dp_ntohs(x) dp_htons(x)

#define SIZEOF_PBUF LWIP_MEM_ALIGN_SIZE(sizeof(struct pbuf))

// Max Rx rate index which we receive from BD template
#define HAL_MAX_RX_RATEINDEX (32)

#define GET_RX_RATE_FROM_BD(bd)   (phy_rates[(bd)->rateIndex])
#define GET_MPDU_LEN_FROM_BD(bd)  ((bd)->mpduLength)
#define DPM_TO_COEX_QUEUE_TIMEOUT 0
#define DPM_ALLOWED_BCMC_NUM      3

#if 0
/* 			Rate Index and Rate (in Kbps) mapping
			-------------------------------------
   0 - 1000		8 - 6000		16 - 6500		24 - 7222
   1 - 2000		9 - 9000		17 - 13000		25 - 14444
   2 - 5500		10 - 12000		18 - 19500		26 - 21667
   3 - 11000	11 - 18000		19 - 26000		27 - 28889
   4 - 1000		12 - 24000		20 - 39000		28 - 43333
   5 - 2000		13 - 36000		21 - 52000		29 - 57778
   6 - 5500		14 - 48000		22 - 58500		30 - 65000
   7 - 11000	15 - 54000		23 - 65000		31 - 72222
*/

static const uint32_t phy_rates[] = {
										1000, 2000, 5500, 11000,
										1000, 2000, 5500, 11000,

										6000, 9000, 12000, 18000,
										24000, 36000, 48000, 54000,

										6500, 13000, 19500, 26000,
										39000, 52000, 58500, 65000,
										7222, 14444, 21667, 28889,
										43333, 57778, 65000, 72222
								};
#endif
enum {
    OWN_STA_ENTRY,
    OWN_AP_ENTRY,
    PEER_STA_ENTRY,
    PEER_AP_ENTRY,
};

/// Frame control field format (2 bytes)
typedef struct MacFrameCtl {
    uint8_t protVer : 2;
    uint8_t type : 2;
    uint8_t subType : 4;

    uint8_t toDS : 1;
    uint8_t fromDS : 1;
    uint8_t moreFrag : 1;
    uint8_t retry : 1;
    uint8_t powerMgmt : 1;
    uint8_t moreData : 1;
    uint8_t wep : 1;
    uint8_t order : 1;
} __attribute__((packed)) mac_frame_ctrl_t;

/// Sequence control field
typedef struct MacSeqCtl {
    uint8_t fragNum : 4;
    uint8_t seqNumLo : 4;
    uint8_t seqNumHi : 8;
} __attribute__((packed)) mac_seq_ctrl_t;

/// QoS control field
typedef struct MacQosCtl {
    uint8_t tid : 4;
    uint8_t esop_txopUnit : 1;
    uint8_t ackPolicy : 2;
    uint8_t amsdu : 1;

    uint8_t txop : 8;
} __attribute__((packed)) mac_qos_ctrl_t;

/// 3 address MAC data header format (24/26 bytes)
typedef struct MacDataHdr {
    mac_frame_ctrl_t fc;              // 2 bytes
    uint8_t durationLo;               // 1 byte
    uint8_t durationHi;               // 1 byte
    uint8_t addr1[NT_MAC_ADDR_SIZE];  // 6 byte
    uint8_t addr2[NT_MAC_ADDR_SIZE];  // 6 byte
    uint8_t addr3[NT_MAC_ADDR_SIZE];  // 6 byte
    mac_seq_ctrl_t seqControl;        // 2 bytes
    mac_qos_ctrl_t qosControl;        // 2 bytes
} __attribute__((packed, aligned(4))) mac_data_hdr_t, *p_mac_data_hdr;

typedef struct MacMgmtHdr {
    mac_frame_ctrl_t fc;
    uint8_t durationLo;
    uint8_t durationHi;
    uint8_t addr1[NT_MAC_ADDR_SIZE];
    uint8_t addr2[NT_MAC_ADDR_SIZE];
    uint8_t addr3[NT_MAC_ADDR_SIZE];
    mac_seq_ctrl_t seqControl;
} __attribute__((packed)) mac_mgmt_hdr_t, *p_mac_mgmt_hdr;

typedef struct MacCtrlHdr {
    mac_frame_ctrl_t fc;
    uint8_t durationLo;
    uint8_t durationHi;
    uint8_t addr1[NT_MAC_ADDR_SIZE];
    uint8_t addr2[NT_MAC_ADDR_SIZE];
} __attribute__((packed)) mac_ctrl_hdr_t, *p_mac_ctrl_hdr;

typedef struct basic_bar_info {
    uint16_t frag_num : 4;
    uint16_t ssn : 12;
} __attribute__((packed)) basic_bar_info_t;

typedef struct bar_ctrl {
    uint8_t bar_ack_policy : 1;
    uint8_t multi_tid : 1;
    uint8_t compressed_bitmap : 1;
    uint8_t rsvd1 : 5;
    uint8_t rsvd2 : 4;
    uint8_t tid_info : 4;
} __attribute__((packed)) bar_ctrl_t;

typedef struct bar_frame {
    mac_frame_ctrl_t fc;
    uint8_t durationLo;
    uint8_t durationHi;
    uint8_t ra[NT_MAC_ADDR_SIZE];
    uint8_t ta[NT_MAC_ADDR_SIZE];
    bar_ctrl_t bar_ctrl;
    union {
        basic_bar_info_t bar_info;
    };
} __attribute__((packed)) bar_frame_t;

enum {
    ENQUEUE_PKT,
    PROCESS_PKT,
    DROP_PKT,
};

enum {
    STA_DEVICE,
    AP_DEVICE,

    MAX_ROLE,
};

#if defined(FEATURE_TX_COMPLETE)
typedef struct {
    void *src_callback;
    void *params;
    uint32_t status;
} dpm_txc_struct_t;
#endif

typedef struct device {
    uint8_t role;
#ifdef LWIP
    struct netif *netif;
#endif
    uint8_t mac_address[NT_MAC_ADDR_SIZE];
    uint8_t occupied;
    uint8_t dev_id;
    uint8_t ip_address[NT_IPV4_ADDR_SIZE];

    /* list of sta entries */
    void *dev_sta;

    uint32_t rx_rate_index_counter[HAL_MAX_RX_RATEINDEX];
} device_t;

#define NT_DPM_BA_RECIPIENT 0
#define NT_DPM_BA_INITIATOR 1
#define NT_DPM_BA_DISABLED  0xFF

//#define DPM_BA_REORDER_TIMEOUT 20
#define DPM_BA_REORDER_TIMEOUT (pnDpA->blockack_timeout_ms)

typedef struct seq_num {
    uint16_t value : 12;
} seq_num_t;

typedef struct dpm_mlme_stats_s {
    uint16_t received_qos_data_cnt;

} dpm_mlme_stats_t;

typedef struct ba_param_s {
    struct node *reorder_head_node;
    uint32_t ba_window_size;
    uint32_t ba_enable;
    uint8_t ba_tid;
    uint8_t sta_id;
    uint8_t badirection;
    uint16_t last_indi_seq_num : 12;
    uint16_t wind_start : 12;
    uint32_t reorder_pkt_count;
    TimerHandle_t reorder_timer_handle;
    struct node *tnode_head;
    struct node *tnode_tail;
    nt_osal_semaphore_handle_t reorder_mutex;
    uint32_t timer_period;
    uint8_t timer_start;
} ba_param_t;

typedef struct dpm_dynamic_ba {
    uint8_t ba_enabled;
#define DPM_ADD_BA_PKT_THRESHOLD 2
#define DPM_DEL_BA_PKT_THRESHOLD 8
    uint8_t pkt_threshold;
    uint32_t pkt_timestamp;
    uint32_t delay_threshold;
} dpm_dyn_ba_t;

typedef struct sta_entry {
    uint8_t mac_addr[NT_MAC_ADDR_SIZE];
    uint8_t bssid[NT_MAC_ADDR_SIZE];
    uint8_t sta_id;
    uint8_t hal_sta_idx;  // HAL STA descriptor index
    uint8_t inUse;
#define NON_HT_STA 0
#define HT_STA     1
    uint8_t sta_type;
#define NON_QOS_STA 0
#define QOS_STA     1
    uint8_t qos_sta;
    uint8_t s_mode;
    device_t *dev;
    uint8_t rmf;
    ba_param_t ba_params[NT_MAX_NO_OF_TID];
    uint32_t last_tx_rx_time;
    dpm_mlme_stats_t dpm_mlme_stats;
    uint16_t rx_rate;
    dpm_dyn_ba_t dyn_ba[NT_MAX_NO_OF_TID];
    uint8_t rate_index;
    float rate;
} sta_entry_t;

enum {
    AC_BK,
    AC_BE,
    AC_VI,
    AC_VO,

    MAX_AC_NUM,
};

#define DPM_MAX_TID 8
typedef struct nt_dpm_tid_to_ac_mapping {
    uint8_t ac_per_tid[DPM_MAX_TID];
} nt_dpm_tid_to_ac_mapping_t;

#if NT_FN_DPM_WMM
#define NT_DPM_WMM_DISABLE 0
#define NT_DPM_WMM_ENABLE  1

#define BTQM_QUEUE_LEN_CHECK 4

struct pkt_node {
    struct pkt_node *next;
    void *pbuf;
    uint32_t len;
};

#define WMM_QUEUE_CSL_PERCENT 10
#define MAX_BURST_HEAP_SIZE   MEM_SIZE / 4

typedef struct nt_dpm_wmm_queue {
    struct pkt_node *pkt_queue_head;
    uint32_t queue_count;
    uint32_t queue_length;
    uint32_t burst_length;
    uint32_t csl;
    uint32_t bsl;
    uint32_t total_pkts_enqueued;
    uint32_t total_pkts_dequeued;
    uint32_t missed_pkts;
    uint32_t max_queue_count;
    uint32_t max_queue_len;
    uint32_t queue_full_count1;
    uint32_t queue_full_count2;
    uint32_t tx_allocation_failed;

    uint32_t dxe_busy;
    uint32_t pkt_notify;
    uint32_t pkt_notify_from_isr;
} nt_dpm_wmm_queue_t;
#endif

enum {
    AP_MODE,
    STA_MODE,
    CONCURRENT_MODE,

    MAX_MODE_VALUE,
};

enum { ENC_NONE = 0, ENC_WEP40, ENC_WEP104, ENC_TKIP, ENC_AES };

#ifndef PLATFORM_FERMION
typedef enum dpm_dxe_ch_assignments {
    TX_BK,
    TX_BE,
    TX_VI,
    TX_VO,
    TX_MGMT,
    RX_MGMT,
    RX_DATA,
    HW_ENC_DEC_1,
    HW_ENC_DEC_2,
    H2H,
} e_dpm_dxe_ch_t;
#endif

typedef enum { MGMT_BD_TEMPLATE, DATA_BD_TEMPLATE, MAX_BD_TEMPLATE } e_bd_template_type_t;

enum {
    NOTIFY_TX_BK_DONE,
    NOTIFY_TX_BE_DONE,
    NOTIFY_TX_VI_DONE,
    NOTIFY_TX_VO_DONE,
    NOTIFY_RX_DATA,
    NOTIFY_H2H,
    NOTIFY_STOP_DP,
    NOTIFY_STOP_RX,
    NOTIFY_START_DP,
#if NT_FN_DPM_WMM
    NOTIFY_PKT_AVAIL_BK,
    NOTIFY_PKT_AVAIL_BE,
    NOTIFY_PKT_AVAIL_VI,
    NOTIFY_PKT_AVAIL_VO,
#endif
    NOTIFY_CHANNEL_CHANGE_HT,
    NOTIFY_CHANNEL_CHANGE_NON_HT,
    NOTIFY_CHANNEL_CHNAGE_ADD_BA,
    NOTIFY_CHANNEL_CHNAGE_DEL_BA,
#if defined(SUPPORT_HIGH_RES_TIMER)
    NOTIFY_TIMER_EXPIRY,
#if defined(HRES_TIMER_UNIT_TEST)
    NOTIFY_TMR_TEST_CMD,
#endif
#endif
    NOTIFY_PAUSE_WMM,
    NOTIFY_UNPAUSE_WMM,
#if defined(FEATURE_TX_COMPLETE)
    NOTIFY_TXC,
#endif
};

struct psdp {
    uint8_t dpm_sleep;
    uint8_t dpm_sw_pend;
    void *ps_pbuf;
    void (*cb_fn)(uint8_t);
    void (*data_available_cb)(uint8_t);
};

#ifdef NT_PKT_THLD_NOTIFICATION
struct pkt_thld_notify {
    uint32_t pkt_thld;
    void (*callbck)(void);
    uint32_t pkt_count;
    uint8_t enable;
};
#endif
/** Data path related stats */
#if ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS) || \
     (defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS))
struct nt_dp_stats {
#if (defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS)
    uint32_t BK_xmit;       /* Transmitted packets for Background traffic BK. */
    uint32_t BE_xmit;       /* Transmitted packets for Best effort BE. */
    uint32_t VI_xmit;       /* Transmitted packets for Video VI. */
    uint32_t VO_xmit;       /* Transmitted packets for Voice VO. */
    uint16_t mc_xmit;       /* Transmitted multicast packets. */
    uint16_t bc_xmit;       /* Transmitted broadcast packets. */
    uint16_t mgmt_xmit;     /* Transmitted management packets. */
    uint16_t eap_xmit;      /* Transmitted eap packets. */
    uint32_t recv;          /* Received packets. */
    uint16_t recv_bc;       /* Received broadcast packets. */
    uint16_t recv_mgmt;     /* Received management packets. */
    uint16_t recv_bar;      /* Received bar frame. */
    uint16_t recv_eap;      /* Received eap frame. */
    uint32_t fwd_up;        /* Forward packets upward to stack. */
    uint32_t fwd_down;      /* Forward packets downward to stack. */
    uint16_t tx_drop;       /* Dropped packets tx. */
    uint16_t rx_drop;       /* Dropped packets rx. */
    uint16_t lenerr;        /* Invalid length error. */
    uint16_t rx_memerr;     /* Out of memory error. */
    uint16_t wmm_dequeue;   /* Packets enqueued to WMM Queue */
    uint16_t wmm_enqueue;   /* Packets dequeued from WMM Queue */
    uint16_t pkt2dxe;       /* Packets sent to Dxe */
    uint16_t dxe_fail;      /* Packets failed to transmit from Dxe */
    uint32_t reorder_enque; /* tx reorder queue*/
    uint32_t reorder_deque; /* rx reorder queue*/
    uint16_t ipv4_xmit;     /* tx ip4*/
    uint16_t ipv6_xmit;     /* tx ip6*/
    uint16_t ipv4_recv;     /* rx ip4*/
    uint16_t ipv6_recv;     /* rx ip6*/
#endif                      // ( NT_FN_AP_HAL_DPH_DEBUG_STATS)||( NT_FN_STA_HAL_DPH_DEBUG_STATS)
#ifdef NT_FN_STA_HAL_DPH_PRODUCTION_STATS
    uint16_t recv_mc;    /* Received multicast packets. */
    uint16_t recv_amsdu; /* Received amsdu frame. */
    uint16_t recv_msdu;  /* Received msdu frame. */
#endif                   // NT_FN_STA_HAL_DPH_PRODUCTION_STATS
#ifdef NT_FN_AP_HAL_DPH_PRODUCTION_STATS
    uint16_t tx_amsdu; /* transmitted amsdu frame. */
    uint16_t tx_msdu;  /* transmitted msdu frame. */
#endif                 // NT_FN_AP_HAL_DPH_PRODUCTION_STATS
};
#endif /* ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS)||(defined \
          NT_FN_AP_HAL_DPH_PRODUCTION_STATS)||(defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS)) */

#define DPM_HT_ENABLE  1
#define DPM_HT_DISABLE 0

#define HASH_ENTRY(addr) (addr[0] | addr[1] | addr[2] | addr[3] | addr[4] | addr[5])
#define MAC_EQUAL(addr1, addr2)                                                                              \
    ((addr1[0] == addr2[0]) && (addr1[1] == addr2[1]) && (addr1[2] == addr2[2]) && (addr1[3] == addr2[3]) && \
     (addr1[4] == addr2[4]) && (addr1[5] == addr2[5]))
#define IP_EQUAL(ip1, ip2) ((ip1[0] == ip2[0]) && (ip1[1] == ip2[1]) && (ip1[2] == ip2[2]) && (ip1[3] == ip2[3]))

typedef struct ndpA {
    volatile dpm_bd_tx_template_t tx_bd_templates[MAX_BD_TEMPLATE];
    sta_entry_t sta_entry[NT_MAX_NO_OF_STA_ENTRY];
    device_t neutrino_device[NT_MAX_NO_OF_DEVICES];

    struct psdp ps_ctrl;

    nt_dpm_tid_to_ac_mapping_t tid_to_ac_map;

    uint8_t sta_ht_map;

#if NT_FN_DPM_WMM
    uint8_t wmm_enable;
    uint8_t wfq_enable;
    nt_dpm_wmm_queue_t wmm_queue[MAX_AC_NUM];
    uint8_t active_queues;
    void *tx_buf[MAX_AC_NUM];
    uint32_t buf_len[MAX_AC_NUM];
    uint32_t total_burst_len;
    uint32_t total_csl;
#endif

#if ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS) || \
     (defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS))
    struct nt_dp_stats dp_stats[NT_MAX_NO_OF_STA_ENTRY];
    uint16_t tx_mgmt;  // pre assoc mgmt frms
#endif                 /* ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS)||(defined \
                          NT_FN_AP_HAL_DPH_PRODUCTION_STATS)||(defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS))s */
    uint8_t rssi_data_pckt;

#ifdef NT_PKT_THLD_NOTIFICATION
    struct pkt_thld_notify ptn;
#endif
    uint8_t auto_ba_traffic_enable;
#define DPM_AUTO_BA_BE_ENABLED    0
#define DPM_AUTO_BA_BK_ENABLED    1
#define DPM_AUTO_BA_BE_BK_ENABLED 2
    uint8_t auto_ba_ac_type;
    uint16_t blockack_timeout_ms;  // block ack timeout in ms
} ndpA_t, *p_ndpA;

struct node {
    uint16_t seqnum : 12;
    struct node *next;
    void *pbuf;
    uint32_t timestamp;
    struct node *next_tnode;
    struct node *prev_tnode;
};

extern p_ndpA pnDpA;

#ifdef NT_FN_RRAM_PERF_BUILD
__attribute__((section(".perf_nc_txt"))) void nt_nwlan_dpm_dxe_init(void);
__attribute__((section(".perf_tx_txt"))) device_t *nt_dpm_find_dev(p_ndpA ad, uint8_t *mac_addr);
__attribute__((section(".perf_cm_txt"))) sta_entry_t *nt_dpm_find_sta_entry(p_ndpA ad, uint8_t *mac_addr);
__attribute__((section(".perf_rx_txt"))) void nt_dpm_reorder_timeout(TimerHandle_t handle);
__attribute__((section(".perf_nc_txt"))) void nt_dpm_reorder_queue_flush(p_ndpA ad, uint8_t staid, uint8_t batid);
__attribute__((section(".perf_rx_txt"))) void nt_dpm_process_rx_data(p_ndpA ad);
__attribute__((section(".perf_rx_txt"))) NT_BOOL nt_dpm_config_ba_timeout(uint16_t timeout_ms);
__attribute__((section(".perf_tx_txt"))) void nt_dpm_tx_done_handler(uint32_t tx_type);
__attribute__((section(".perf_rx_txt"))) void nt_dpm_rx_handler(uint32_t rx_type);
__attribute__((section(".perf_tx_txt"))) nt_status_t nt_dpm_process_packet(p_ndpA ad, void *buf, uint32_t len);
__attribute__((section(".perf_nc_txt"))) void nt_dpm_dev_init(device_t *dev);
__attribute__((section(".perf_nc_txt"))) void nt_dpm_dev_deinit(device_t *dev);
__attribute__((section(".perf_nc_txt"))) void nt_dpm_init();
__attribute__((section(".perf_nc_txt"))) void nt_dpm_deinit();
__attribute__((section(".perf_nc_txt"))) uint8_t nt_dpm_stop(p_ndpA ad);
__attribute__((section(".perf_nc_txt"))) uint8_t nt_dpm_pause_wmm(p_ndpA ad);
__attribute__((section(".perf_nc_txt"))) uint8_t nt_dpm_unpause_wmm(p_ndpA ad);
__attribute__((section(".perf_nc_txt"))) uint8_t nt_dpm_stop_rx(void);
__attribute__((section(".perf_nc_txt"))) uint8_t nt_dpm_start(p_ndpA ad);
__attribute__((section(".perf_nc_txt"))) uint32_t nt_dpm_create_data_path_task();
__attribute__((section(".perf_nc_txt"))) void nt_dpm_remove_data_path_task();
#if NT_FN_DPM_WMM
__attribute__((section(".perf_tx_txt"))) nt_status_t nt_dpm_add_to_wmm_queue(p_ndpA ad, void *buf, uint32_t len);
__attribute__((section(".perf_tx_txt"))) void nt_dpm_wmm_process_packet(p_ndpA ad, uint8_t ac);
__attribute__((section(".perf_nc_txt"))) void nt_dpm_wmm_init(p_ndpA ad);
__attribute__((section(".perf_tx_txt"))) void nt_dpm_packet_available_notify(uint8_t ac);
__attribute__((section(".perf_tx_txt"))) void nt_dpm_tickle_all_wmm_queues();

__attribute__((section(".perf_tx_txt"))) void nt_dpm_packet_available_notify_from_isr(uint8_t ac);
#endif
__attribute__((section(".perf_rx_txt"))) void nt_dpm_rx_notify();
__attribute__((section(".perf_nc_txt"))) void nt_ndxe_channel_change_notify(uint8_t amsdu);
__attribute__((section(".perf_nc_txt"))) NT_BOOL nt_dpm_is_paused();
#if defined(FEATURE_TX_COMPLETE)
__attribute__((section(".perf_tx_txt"))) void nt_dpm_process_txc(dpm_txc_struct_t *buffer);
#endif

uint32_t nt_send_data_packet(uint32_t len, uint8_t ch, void *dst_addr);
void nt_send_broadcast_data_packet(uint32_t len, uint8_t ch, uint8_t sid, uint8_t qid);
void nt_send_multicast_data_packet(uint32_t len, uint8_t ch, uint8_t sid, uint8_t qid);
void nt_dpm_send_ac_pck(uint8_t tid, uint32_t *arr_size, uint16_t total_pkts, uint8_t sta_id);
__attribute__((section(".perf_rx_txt"))) uint8_t nt_dpm_process_bar_frames(p_ndpA ad, uint8_t *frame);

__attribute__((section(".perf_nc_txt"))) sta_entry_t *nt_dpm_find_sta_entry_from_staidx(p_ndpA ad, uint8_t sta_id);
__attribute__((section(".perf_tx_txt"))) sta_entry_t *nt_dpm_find_sta_entry_for_eth_pkt(p_ndpA ad,
                                                                                        uint8_t *src_mac_addr,
                                                                                        uint8_t *dest_mac_addr);
__attribute__((section(".perf_cm_txt"))) void *nt_dpm_allocate_buffer(uint32_t length);
__attribute__((section(".perf_cm_txt"))) void nt_dpm_free_buffer(void *buf);
__attribute__((section(".perf_cm_txt"))) void *nt_dpm_calloc_buffer(uint32_t num, uint32_t size);
__attribute__((section(".perf_cm_txt"))) void nt_dpm_realloc_buffer(void *buf, uint32_t length);
__attribute__((section(".perf_nc_txt"))) void nt_dpm_notify_network_to_set_linkup(struct netif *netif);
__attribute__((section(".perf_nc_txt"))) void nt_dpm_notify_network_to_set_linkdown(struct netif *netif);
#ifdef MEM_CPY_VIA_DXE
__attribute__((section(".perf_cm_txt"))) void *nt_dpm_memcpy(void *dst, const void *src, uint32_t length);
#endif

/*data path task fun declaration*/
__attribute__((section(".perf_nc_txt"))) void nt_dpm_unpause_wmm_handler(void (*cbptr)(uint8_t));
__attribute__((section(".perf_nc_txt"))) void nt_dpm_pause_wmm_handler(void (*cbptr)(uint8_t),
                                                                       void (*data_available_cb)(uint8_t));

__attribute__((section(".perf_nc_txt"))) void nt_dpm_start_handler(void (*cbptr)(uint8_t));
__attribute__((section(".perf_nc_txt"))) void nt_dpm_stop_handler(void (*cbptr)(uint8_t),
                                                                  void (*data_available_cb)(uint8_t));
__attribute__((section(".perf_nc_txt"))) void nt_dpm_stop_rx_handler(void (*cbptr)(uint8_t));
__attribute__((section(".perf_nc_txt"))) void nt_dpm_stop_handler_from_isr(void (*cbptr)(uint8_t),
                                                                           void (*data_available_cb)(uint8_t));
uint8_t nt_dpm_more_bit_check(void *frame);
__attribute__((section(".perf_nc_txt"))) void nt_dpm_ack_modify(uint8_t ack_bit);

__attribute__((section(".perf_nc_txt"))) void nt_dpm_tid_modify(uint8_t tid);
void nt_dpm_send_amsdu_pkt(uint32_t no_of_msdu, uint32_t *size_array, uint8_t tid, uint8_t time);
#else
void nt_nwlan_dpm_dxe_init(void);
device_t *nt_dpm_find_dev(p_ndpA ad, uint8_t *mac_addr);
sta_entry_t *nt_dpm_find_sta_entry(p_ndpA ad, uint8_t *mac_addr);
void nt_dpm_reorder_timeout(TimerHandle_t handle);
void nt_dpm_reorder_queue_flush(p_ndpA ad, uint8_t staid, uint8_t batid);
void nt_dpm_process_rx_data(p_ndpA ad);
NT_BOOL nt_dpm_config_ba_timeout(uint16_t timeout_ms);
void nt_dpm_clear_rx_stats(device_t *dev);
void nt_dpm_tx_done_handler(uint32_t tx_type);
void nt_dpm_rx_handler(uint32_t rx_type);
nt_status_t nt_dpm_process_packet(p_ndpA ad, void *buf, uint32_t len);
#ifdef SUPPORT_RING_IF
void nt_dpm_dev_init(device_t *dev, void *buffer);
#else
void nt_dpm_dev_init(device_t *dev);
#endif
void nt_dpm_dev_deinit(device_t *dev);
void nt_dpm_init();
void nt_dpm_deinit();
uint8_t nt_dpm_stop(p_ndpA ad);
uint8_t nt_dpm_stop_rx(void);
uint8_t nt_dpm_start(p_ndpA ad);
uint8_t nt_dpm_pause_wmm(p_ndpA ad);
uint8_t nt_dpm_unpause_wmm(p_ndpA ad);
uint32_t nt_dpm_create_data_path_task();
void nt_dpm_remove_data_path_task();
#if NT_FN_DPM_WMM
#ifdef SUPPORT_RING_IF
nt_status_t nt_dpm_add_to_wmm_queue(p_ndpA ad, void *buf, uint32_t len, uint8_t qos_for_non_ip);
#else
nt_status_t nt_dpm_add_to_wmm_queue(p_ndpA ad, void *buf, uint32_t len);
#endif
void nt_dpm_wmm_process_packet(p_ndpA ad, uint8_t ac);
void nt_dpm_wmm_init(p_ndpA ad);
void nt_dpm_packet_available_notify(uint8_t ac);
void nt_dpm_tickle_all_wmm_queues();
void nt_dpm_packet_available_notify_from_isr(uint8_t ac);
#endif
void nt_dpm_rx_notify();
void nt_ndxe_channel_change_notify(uint8_t amsdu);
NT_BOOL nt_dpm_is_paused();
#if defined(FEATURE_TX_COMPLETE)
void nt_dpm_process_txc(dpm_txc_struct_t *buffer);
#endif

#ifdef SUPPORT_RING_IF
bool is_ip_packet(ethernet_header_t *p_eth_hdr);
#endif

uint32_t nt_send_data_packet(uint32_t len, uint8_t ch, void *dst_addr);
void nt_send_broadcast_data_packet(uint32_t len, uint8_t ch, uint8_t sid, uint8_t qid);
void nt_send_multicast_data_packet(uint32_t len, uint8_t ch, uint8_t sid, uint8_t qid);
void nt_dpm_send_ac_pck(uint8_t tid, uint32_t *arr_size, uint16_t total_pkts, uint8_t sta_id);
uint8_t nt_dpm_process_bar_frames(p_ndpA ad, uint8_t *frame);

sta_entry_t *nt_dpm_find_sta_entry_from_staidx(p_ndpA ad, uint8_t sta_id);
sta_entry_t *nt_dpm_find_sta_entry_for_eth_pkt(p_ndpA ad, uint8_t *src_mac_addr, uint8_t *dest_mac_addr);
void *nt_dpm_allocate_buffer_pool(uint32_t length);
void *nt_dpm_allocate_buffer(uint32_t length);
void nt_dpm_free_buffer(void *buf);
void *nt_dpm_calloc_buffer(uint32_t num, uint32_t size);
void nt_dpm_realloc_buffer(void *buf, uint32_t length);
void nt_dpm_notify_network_to_set_linkup(struct netif *netif);
void nt_dpm_notify_network_to_set_linkdown(struct netif *netif);
#ifdef MEM_CPY_VIA_DXE
void *nt_dpm_memcpy(void *dst, const void *src, uint32_t length);
#endif

/*data path task fun declaration*/
void nt_dpm_start_handler(void (*cbptr)(uint8_t));
void nt_dpm_unpause_wmm_handler(void (*cbptr)(uint8_t));
void nt_dpm_pause_wmm_handler(void (*cbptr)(uint8_t), void (*data_available_cb)(uint8_t));
void nt_dpm_stop_handler(void (*cbptr)(uint8_t), void (*data_available_cb)(uint8_t));
void nt_dpm_stop_handler_from_isr(void (*cbptr)(uint8_t), void (*data_available_cb)(uint8_t));
uint8_t nt_dpm_more_bit_check(void *frame);
void nt_dpm_ack_modify(uint8_t ack_bit);

void nt_dpm_tid_modify(uint8_t tid);
void nt_dpm_send_amsdu_pkt(uint32_t no_of_msdu, uint32_t *size_array, uint8_t tid, uint8_t time);
#endif

#ifdef NT_TST_TIME_STAMP_ENABLE
void nt_dpm_tm_reset();
void nt_dpm_tm_node_reset(uint8_t ts);
void nt_dpm_read_ts(uint8_t ts, void *buf);
void nt_dpm_udp_tm_tx_process(uint32_t *payload, uint32_t udp_buf_size);
#endif
#ifdef SUPPORT_COEX
void nt_dpm_set_data_rx_reorder_timeout(uint32_t new_time);
#endif

#ifdef NT_FN_FTM
/*
 *  @brief  : To get delta value of t3 and t2 from bd header
 *  @param  : none
 *  @return : 32 bit delta value of t3 and t2
 */
uint16_t get_t3_t2_delta_value();

/*
 *  @brief  : To get absolute value of t2 from bd header
 *  @param  : none
 *  @return : 32 bit absolute value of t2
 */
uint32_t get_t2_absolute_value();

/*
 *  @brief  : To get FAC from bd header
 *  @param  : none
 *  @return : 16 bit FAC value
 */
uint16_t get_fac_value_bd();
#endif

#ifdef NT_TST_TIME_STAMP_ENABLE
#define SIZE_OF_STATS 15

typedef struct tm_node_struct {
    uint8_t valid;
    uint32_t value;
    uint32_t offset;
} tm_node_t;

typedef struct tm_struct {
    tm_node_t tx_stat[SIZE_OF_STATS];
    tm_node_t rx_stat[SIZE_OF_STATS];
    uint32_t tx_marker;
    uint32_t tx_reset_count;
    uint32_t tx_max_time;
    uint32_t tx_min_time;
    uint32_t tx_average_time;
    uint32_t tx_count;

    uint32_t rx_marker;
    uint32_t rx_max_time;
    uint32_t rx_min_time;
    uint32_t rx_average_time;
    uint32_t rx_count;

    uint8_t tm_start;

    uint32_t tx_node_reset_count;
    uint32_t rx_node_reset_count;

    uint32_t udp_tx_node_reset_count;
    uint32_t dpm_tx_node_reset_count;

    tm_node_t tx_stat_min[SIZE_OF_STATS];
    tm_node_t rx_stat_min[SIZE_OF_STATS];

    tm_node_t tx_stat_max[SIZE_OF_STATS];
    tm_node_t rx_stat_max[SIZE_OF_STATS];
} tm_struct_t;

enum {
    PING_SEND,
    UDP_SEND_PKT,
    LOW_LVL_OUTPUT,
    PKT_FROM_STACK,
    TX_PROCESS_PACKET,
    HDR_TRANS,
    DXE_TX,
    DXE_CALL_DONE,
    DXE_TX_INTERRUPT,
};

enum {
    RXP_TIMESTAMP,
    DXE_START,
    DXE_END,
    RX_INTERRUPT,
    PROCESS_RX_DATA,
    SANITY_CHECK,
    HDR_RX_TRANS,
    FWD_PKT_TO_STACK,
    ETH_PKT_TO_STACK,
    NETIF_INPUT,
    LWIP_INPKT,
    UDP_OUTPUT,
    PING_RCV,
};

enum {
    TX_TIMESTAMP,
    RX_TIMESTAMP,
    FULL_TIMESTAMP,
};

extern tm_struct_t nt_dpm_tm;

#endif /* NT_TST_TIME_STAMP_ENABLE */

#ifdef NT_FN_DPM_DEBUG
#define DEBUG_CODE 1
#endif

#if DEBUG_CODE
#define DEBUG_BARQ_SIZE 400
typedef struct dbg_node_s {
    uint32_t timestamp;
    uint16_t seqnum;
} dbg_node_t;
extern volatile dbg_node_t seqin[DEBUG_BARQ_SIZE];
extern volatile dbg_node_t seqout[DEBUG_BARQ_SIZE];
#endif

NT_BOOL nt_dpm_is_tx_pending_wmm_queues(uint8_t wmi_dev_id);
NT_BOOL nt_dpm_is_tx_pending_nonwmm_queues(uint8_t wmi_dev_id);
NT_BOOL nt_dpm_is_tx_pending_in_tx_buffers_or_queues(uint8_t wmi_dev_id);

/*
 * @brief Check if Tx is busy.
 * @param : none
 * @return : True: when HW is busy in tx
 *        : else return FALSE
 */
NT_BOOL nt_dpm_is_tx_busy(void);

#ifdef WAR_RESTORE_DPU_DEFAULT_WQ_12_ON_EXIT_FROM_BMPS
/*
 * @brief restore DPU routing post WLAN wake-up to get mgmt frames in correct WQ
 * @param : none
 * @return : none
 */
void nt_dpm_restore_dpu_default_wq_routing_post_wakeup(void);
#endif

NT_BOOL nt_dpm_sw_is_tx_busy(void);

void nt_dpm_sw_set_tx_status(NT_BOOL status);

#endif /* DATA_PATH_H_ */
