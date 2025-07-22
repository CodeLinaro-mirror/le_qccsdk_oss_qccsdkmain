/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*
* @file coex_bt.h
* @brief Coex bt params and struct definitions
*========================================================================*/
#ifndef _COEX_ISO_H_
#define _COEX_ISO_H_
/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nt_osal.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "nt_common.h"

#if defined(SUPPORT_COEX)
#include "coex_gpm.h"
#include "coex_mci.h"
#include "coex_sched.h"
#include "coex_freerun.h"
/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define COEX_ISO_MAX_GROUPS            (4) /* Spec allows upto 16 groups, but for BT controller has max 4. */
#define COEX_ISO_DEF_PRIO              (219)
#define COEX_ISO_BURST_NUM_HIGH_THRESH (2)
#define COEX_ISO_FLUSH_TO_HIGH_THRESH  (2) /* number of attempts */
#define COEX_ISO_BT_DUR_THRSH_SMALL    (4000)
#define COEX_ISO_BT_DUR_THRSH_MED      (7668)
#define COEX_ISO_WLAN_DUR_THRSH_SMALL  (5000)
#define COEX_ISO_WLAN_DUR_THRSH_MED    (17000)

#define COEX_ISO_MIN_WLAN_DUR_FOR_FREERUN (4000)

#define COEX_ISO_INTRVL_BM_BT_SMALL   (0x20)
#define COEX_ISO_INTRVL_BM_BT_MED     (0x10)
#define COEX_ISO_INTRVL_BM_BT_LONG    (0x08)
#define COEX_ISO_INTRVL_BM_WLAN_SMALL (0x04)
#define COEX_ISO_INTRVL_BM_WLAN_MED   (0x02)
#define COEX_ISO_INTRVL_BM_WLAN_LONG  (0x01)

#define COEX_ISO_BT_BITMAP_LENGTH (100)
#define COEX_ISO_MIN_WIFI_DUR_MS  (3)

#define COEX_ISO_INTRVL_SLOT_SIZE_US (625)
#define COEX_ISO_MAX_NSE             (32)
#define COEX_ISO_INVALID_BT_DUR      (0xFF)

#define COEX_ISO_LL_LATENCY_THRESH_MS      (20)
#define COEX_ISO_LL_ISO_INTVL_THRESH_SLOTS (0x18) /* 15ms ISO interval */
#define COEX_ISO_LL_FT_THRESH              (0)
#define COEX_ISO_LL_MIN_HP_DC_THRESH       (60)

#define COEX_ISO_LL_MIN_HP_NSE (1)

#define COEX_ISO_LL_CP_BW_THRESH     (55)
#define COEX_ISO_LL_HP_BW_THRESH     (55)
#define COEX_ISO_LL_MP_BW_THRESH     (25)
#define COEX_ISO_LL_MIN_HP_BW_THRESH (50)

#define COEX_ISO_MIN_HQ_LATENCY_THRESH_MS (60)
#define COEX_ISO_MIN_HQ_FT                (3)
#define COEX_ISO_MIN_HQ_ISO_INTVL_SLOTS   (0x18) /* 15ms ISO interval */

/* Percentage threshold values for following ISO conf
 * ISO interval - 10 ms
 * Sub interval(2 CIS) ~ 2 ms*/
#define COEX_ISO_BT_BW_DEFAULT_THRESH (0)  /* 0 NSE */
#define COEX_ISO_BT_BW_MIN_THRESH     (30) /* 1 NSE */
#define COEX_ISO_BT_BW_MED_THRESH     (42) /* 2 NSE */
#define COEX_ISO_BT_BW_MAX_THRESH     (84) /* 4 NSE */

#define BTCOEX_ISO_DEFAULT_PRIO (0)
/* Below DATA, RETRX and CTL weights of ISO */
#define BTCOEX_ISO_DATA_LOW_PRIO  (1U)
#define BTCOEX_ISO_DATA_MED_PRIO  (19U)
#define BTCOEX_ISO_DATA_HIGH_PRIO (124U)
#define BTCOEX_ISO_DATA_CRIT_PRIO (252U)  // BTCOEX_ISO_DATA_HIGH_PRIO + 128

#define BTCOEX_ISO_RETRX_LOW_PRIO  (1U)
#define BTCOEX_ISO_RETRX_MED_PRIO  (19U)
#define BTCOEX_ISO_RETRX_HIGH_PRIO (124U)
#define BTCOEX_ISO_RETRX_CRIT_PRIO (252U)  // BTCOEX_ISO_RETRX_HIGH_PRIO + 128

#define BTCOEX_ISO_CTL_LOW_PRIO  (1U)
#define BTCOEX_ISO_CTL_MED_PRIO  (19U)
#define BTCOEX_ISO_CTL_HIGH_PRIO (76U)
#define BTCOEX_ISO_CTL_CRIT_PRIO (204U)  // BTCOEX_ISO_CTL_HIGH_PRIO + 128

#define ceil(x, y) ((x / y) + (((x % y) == 0) ? 0 : 1))

#define BTCOEX_ISO_RETRX_MED_WHC_PRIO (93U)

#define COEX_SET_ISO_WGHT(pkt_type, pkt_level, priority) iso_wghts[pkt_type].prio[pkt_level] = priority

typedef enum {
    COEX_ISO_ERR_STORE_GRP_INFO = 0,
    COEX_ISO_ERR_GRP_NOT_FOUND = 1,
    COEX_ISO_INVALID_GRP_ID = 2,
} E_COEX_ISO_ERR_CODE;

typedef enum {
    E_COEX_ISO_PROPERTY_INT_SEQ = 0,
    E_COEX_ISO_PROPERTY_CIS_BIS = 1,
    E_COEX_ISO_PROPERTY_BN = 2,
    E_COEX_ISO_PROPERTY_FT = 3,
    E_COEX_ISO_PROPERTY_COUNT
} E_COEX_ISO_PROPERTY;

typedef enum {
    E_COEX_ISO_STREAM_TYPE_SEQUENTIAL = 0,
    E_COEX_ISO_STREAM_TYPE_INTERLEAVED = 1,
} E_COEX_ISO_STREAM_TYPE;

typedef enum {
    E_COEX_ISO_STREAM_TRANSPORT_TYPE_CIS = 0,
    E_COEX_ISO_STREAM_TRANSPORT_TYPE_BIS = 1,
    E_COEX_ISO_STREAM_TRANSPORT_TYPE_MAX
} E_COEX_ISO_STREAM_TRANSPORT_TYPE;

typedef enum {
    E_COEX_ISO_CONFIG_DATA = 0,
    E_COEX_ISO_CONFIG_RETRX = 1,
    E_COEX_ISO_CONFIG_CONTROL = 2,
    E_COEX_ISO_CONFIG_MAX
} E_COEX_ISO_CONFIG;

typedef enum {
    E_COEX_ISO_LEVEL_LOW = 0,
    E_COEX_ISO_LEVEL_MED = 1,
    E_COEX_ISO_LEVEL_HIGH = 2,
    E_COEX_ISO_LEVEL_CRIT = 3,
    E_COEX_ISO_LEVEL_MAX
} E_COEX_ISO_PACKET_LEVEL;

typedef enum {
    E_COEX_ISO_CODEC_TRANSPARENT = 0,
    E_COEX_ISO_CODEC_LC3 = 1,
    E_COEX_ISO_CODEC_LC3Q = 2,
    E_COEX_ISO_CODEC_APTX = 3,
    E_COEX_ISO_CODEC_MAX
} E_COEX_ISO_CODEC_TYPE;

/* Local profile detected by CoEx based on stream type, latency and codec type*/
typedef enum {
    E_COEX_ISO_LOC_PROFILE_BCAST = 0,
    E_COEX_ISO_LOC_PROFILE_UCAST_AUDIO = 1,
    E_COEX_ISO_LOC_PROFILE_STEREO_REC = 2,
    E_COEX_ISO_LOC_PROFILE_UCAST_VOICE = 3,
    E_COEX_ISO_LOC_Q2Q_GAMING = 4,
    E_COEX_ISO_LOC_Q2Q_GAMING_VBC = 5,
    E_COEX_ISO_LOC_APTX_HQ = 6,
    E_COEX_ISO_LOC_PROFILE_UCAST_HQ_AUDIO = 7,
    E_COEX_ISO_LOC_PROFILE_MAX
} E_COEX_ISO_LOC_PROFILE_TYPE;

typedef enum {
    E_COEX_ISO_PROFILE_UNKNOWN = 0,
    E_COEX_ISO_PROFILE_AUDIO = 1,
    E_COEX_ISO_PROFILE_VOICE = 2,
    E_COEX_ISO_PROFILE_STEREO_REC = 3,
    E_COEX_ISO_PROFILE_MAX
} E_COEX_ISO_PROFILE_TYPE;

typedef enum {
    E_COEX_HS_DISCONNECTED = 0,      /* CIS Disconnected */
    E_COEX_HS_CIS_ESTABLISHED = 1,   /* CIS Established */
    E_COEX_HS_CIS_DISCONNECTING = 2, /* XPAN Connection established */
} E_COEX_HS_CIS_STATE;

typedef enum {
    COEX_REASON_NONE = 0,
    COEX_REASON_CIS_STATE_CHANGE = 1, /* Update XPAN conf only when CIS state is changed */
    COEX_REASON_WLAN_CONNECTING = 2,  /* Update XPAN conf after WLAN connection */
} XPAN_CONF_OVERRIDE_REASON;

typedef struct _coex_iso_wghts {
    uint8_t prio[4];
} coex_iso_wghts;

typedef struct _coex_xpan_iso_prio_perc_thrsh {
    uint8_t cp_perc_thresh;
    uint8_t hp_perc_thresh;
    uint8_t mp_perc_thresh;
} coex_xpan_iso_prio_perc_thrsh;

typedef struct _coex_iso_qos_le_info {
    uint8_t codec_type;
    uint8_t reason;
    uint16_t latency_ms;
    uint8_t tx_flush_to;
    uint8_t rx_flush_to;
    uint16_t M2S_SDU_size;
    uint16_t S2M_SDU_size;
    uint8_t vbc_en;
} coex_iso_qos_le_info;

typedef struct _coex_iso_link {
    uint8_t id;
    uint8_t grpid;
    uint8_t stream_idx;
    E_BT_PROFILE profile;
    uint8_t tx_flush_to;
    uint8_t rx_flush_to;
    uint8_t role;
    // uint8_t                                 data_rate;
    uint8_t sub_events; /* Number of sub-events. */
    E_COEX_BT_LINK_OP link_op;
    uint8_t burst_num;
    uint32_t sub_interval; /* micro seconds. */
    uint32_t prio_mask;
    // whal_coex_sched_entry                   sched_info;    /* sched info entry . */
    struct _coex_iso_group *grp;
    uint64_t subevent_mask;
} coex_iso_link;

typedef struct _coex_iso_group {
    uint8_t id;
    uint8_t hp_bm;
    uint8_t active_links;
    uint8_t num_links;
    uint8_t is_hp_duration_allowed;
    uint8_t is_hp_duration_valid;
    E_COEX_ISO_STREAM_TYPE strm_type;
    E_COEX_ISO_STREAM_TRANSPORT_TYPE strm_tranp_type;
    uint32_t duration;            /* In units of COEX_ISO_INTRVL_SLOT_SIZE_US */
    uint32_t interval;            /* In units of COEX_ISO_INTRVL_SLOT_SIZE_US */
    uint32_t prio_mask;           /* combined priority mask of all links */
    uint32_t hp_duration_us;      /* In units of us */
    uint32_t prev_hp_duration_us; /* previous hp duration */
    coex_iso_link links[MAX_LOCAL_BT_LINK];
    uint32_t max_bt_duty_cycle;
    uint32_t hp_bt_duty_cycle;
    uint8_t codec_type;
    uint16_t latency_ms;
    uint32_t hp_duration_allowed_by_coex_us;
    uint8_t tx_flush_to;
    uint8_t rx_flush_to;
    uint16_t M2S_SDU_size;
    uint16_t S2M_SDU_size;
    uint8_t is_priority_update_gpm_sent;
    uint8_t tx_burst_no;
    uint8_t rx_burst_no;
    E_COEX_ISO_PROFILE_TYPE profile_type;
    coex_iso_qos_le_info qos_le_info;
    E_COEX_ISO_LOC_PROFILE_TYPE loc_profile_type;
    uint32_t iso_pkt_prio[E_COEX_ISO_CONFIG_MAX]; /* To cache the current priority of ISO packets */

} coex_iso_group;

typedef struct _coex_xpan_transition_conf {
    uint8_t XPAN_transition_in_progress;
    uint8_t WlanWeightGroup;
    coex_xpan_iso_prio_perc_thrsh *prio_perc_thresh;
    uint8_t conf_override_reason;
} coex_xpan_transition_conf;

typedef struct _coex_iso_schedule {
    uint32_t schedule_start_time; /* Start time of first BT window in US */
    uint32_t schedule_end_time;   /* End time of last WLAN window in US */
    uint32_t bt_duration;         /* BT duration in micro seconds */
    uint32_t wl_duration;         /* WL duration in micro seconds */
} coex_iso_schedule;

typedef struct _coex_iso_settings {
    coex_iso_group grp_info[COEX_ISO_MAX_GROUPS];
    uint8_t remote_grpid[COEX_ISO_MAX_GROUPS]; /* remote to local group id mapping */
    uint32_t largest_intrvl_grp;               /* Index of the group that has largest interval. */
    uint32_t eff_bt_dur;                       /* Effective BT duration considering legacy policy. */
    uint32_t eff_wlan_dur;                     /* Effective WLAN duration considering legacy policy. */
    coex_iso_schedule curr_sched;
    uint8_t num_iso_grp;
    uint8_t num_iso_links;
} coex_iso_settings;

void coex_iso_init(void);
void coex_iso_topology_reset(void);
uint8_t coex_iso_process_link_info_msg(coex_gpm_iso_link_info *link_info);
uint8_t coex_iso_process_group_info_msg(coex_gpm_iso_grp_info *grp_info);
uint8_t coex_iso_process_priority_mask_msg(coex_gpm_le_iso_priority_mask *prio_mask);
uint8_t coex_iso_process_qos_le_info_msg(coex_gpm_qos_le_iso_info *qos_le_info);
uint8_t coex_iso_get_schedule(coex_iso_schedule *iso_schedule);
void coex_iso_set_loc_profile_type(coex_iso_group *grp);
void coex_iso_detect_profile_type(coex_iso_group *grp);
uint8_t coex_iso_get_intervals(uint32_t *wlan, uint32_t *bt);
uint32_t coex_iso_generate_interval_bitmap(void);
uint8_t coex_iso_get_net_policy(uint8_t legacy_policy, uint32_t wlan_intrvl, uint32_t bt_intrvl);
void coex_iso_update_pkt_priority(uint8_t grpid);
void coex_iso_send_priority_update_and_subevent_mask_gpm(coex_iso_group *grp, uint64_t mask);
uint8_t coex_iso_get_schedule_for_profile(E_COEX_ISO_LOC_PROFILE_TYPE loc_profile_type,
                                          coex_iso_schedule *iso_schedule);
coex_iso_link *coex_get_iso_link();
uint8_t coex_iso_exists();
uint8_t coex_iso_get_min_ft(coex_iso_group *grp);
uint64_t coex_iso_get_subevent_mask(coex_iso_link *iso_link);
#ifdef SUPPORT_XPAN_COEX
coex_xpan_iso_prio_perc_thrsh *coex_get_XPAN_iso_prio_thresholds();
void coex_XPAN_iso_conf_update(coex_mac *mac);
void coex_XPAN_iso_conf_update_5g(coex_mac *coex_mac);
uint8_t is_XPAN_transition_ongoing();
uint8_t coex_iso_get_XPAN_wlan_weight();
uint64_t coex_iso_XPAN_get_subevent_mask(E_COEX_ISO_LOC_PROFILE_TYPE profile_type, coex_iso_link *iso_link);
void coex_iso_update_XPAN_cis_state(uint8_t cis_state);
#endif

#endif /* SUPPORT_COEX */
#endif /* #ifndef _COEX_ISO_H_ */
