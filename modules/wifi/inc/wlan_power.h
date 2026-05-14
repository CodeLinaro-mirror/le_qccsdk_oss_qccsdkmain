/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _WLAN_POWER_H_
#define _WLAN_POWER_H_

#include "nt_common.h"
#include "wifi_cmn.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "pm_api.h"
#include "wmi.h"
#include "nt_imps.h"
#if (defined NT_FN_WUR_AP) || (defined NT_FN_WUR_STA)
#include "nt_wur.h"
#endif
#ifdef NT_FN_WNM_POWERSAVE_MODE
#include "nt_wnm_power_save.h"
#endif /* NT_FN_WNM_POWERSAVE_MODE */
#ifdef NT_FN_TWT
#include "nt_twt.h"
#endif /*  NT_FN_TWT */

#ifdef FEATURE_PERIODIC_WAKE_SLEEP
#include "nt_periodic_wake_infra.h"
#endif /* FEATURE_PERIODIC_WAKE_SLEEP */
#include "wlan_qpower.h"
#include "hal_int_powersave.h"
#include "halphy_api.h"

/**********************************************************
 *      Power management module internal defintion file   *
 **********************************************************/

/*
 * Macro definitions
 */

/*surendar : need to remove below 2 line once the socpm completed*/
#define BMPS_SLEEP 1
#define IMPS_SLEEP 2

/* Infrastructure */
#define MAX_PSPOLL_TIMEOUT             50    /*ms*/
#define MAX_IDLE_TIME                  30000 /*ms*/
#define MAX_BEACON_MIX                 5
#define MAC_HEADER_OFFSET              32
#define BCN_TSTAMP_SIZE                sizeof(uint64_t) /*8bytes*/
#define BCN_INTV_OFFSET                sizeof(struct ieee80211_frame) + BCN_TSTAMP_SIZE
#define TIM                            5
#define TIM_LEN                        4
#define TIM_IE_TRAFFIC_INDICATOR_MASK  0x1
#define TIM_IE_TRAFFIC_INDICATOR_SHIFT 0
#define TIM_IE_BITMAP_CTL_MASK         0xFE
#define TIM_IE_BITMAP_CTL_SHIFT        1

/*
 * Data structure definitions
 */

#define CURRENT_POWER_SAVE_BMPS_MASK 1
#define CURRENT_POWER_SAVE_IMPS_MASK 2
#define CURRENT_POWER_SAVE_WUR_MASK  3
#define CURRENT_POWER_SAVE_WNM_MASK  4

/*Power Save Policy*/
#define DEFAULT_POLICY_TO_STOP_LOCATION_MANAGER_WHEN_ENTER_TO_PS 1
#define DEFAULT_POLICY_TO_STOP_BG_SCAN_WHEN_ENTERING_PW_SAVE     1

#define NT_PM_BMPS_OFFSET  (0)
#define NT_PM_UAPSD_OFFSET (1)
#define NT_PM_WNM_OFFSET   (2)
#define NT_PM_TWT_OFFSET   (3)
#define NT_PM_WUR_OFFSET   (4)

#define WB_TO_BCN_RX_START_US        (450) /* Estimated latency from warm boot to beacon rx window start */
#define SLP_TIME_CALC_TO_AON_PRGM_US (165) /* Estimated latency from sleep time calculation to aon programming */

#define BMPS_PRE_BCN_LOWER_BOUND_US 200
#define BMPS_PRE_BCN_UPPER_BOUND_US 15000

#define BMPS_IDLE_TIMER_LOWER_BOUND_MS 1
#define BMPS_IDLE_TIMER_UPPER_BOUND_MS 300000

#define BMPS_NUM_IDLE_INTERVALS_LOWER_BOUND 1
#define BMPS_NUM_IDLE_INTERVALS_UPPER_BOUND 255

#ifdef SUPPORT_SAP_POWERSAVE
#define PM_INVALID_NEXT_TBTT (uint64_t) - 1
#endif

#ifdef SUPPORT_BEACON_MISS_THRESHOLD_TIME
#define BMTT_LOWER_BOUND_LIMIT_US 1000000
#define BMTT_UPPER_BOUND_LIMIT_US 20000000
#endif /* SUPPORT_BEACON_MISS_THRESHOLD_TIME */

#if !defined(PLATFORM_FERMION)
// enable RRI backups count, if higher than MAX, don't perform backups any more
#define _MLM_INC_N_RRI_BKUP_COUNT
// max number of times backups are performed
#define _MLM_N_BKUP_MAX 5
#endif /* PLATFORM_FERMION */

// additional delay to account for time for s/w overheads after sleep time is calculated
#define _MLM_SLP_DELAY_COMP_US 1000
// min sleep time in us, if next tbtt is less that this, delay wake by additional bi
#define _MLM_SLP_MIN_US 40000

#define LOWER32BIT(__val) ((uint32_t)(((__val)&0xFFFFFFFFull)))

#define UPPER32BIT(__val) ((uint32_t)((((__val) >> 32) & 0xFFFFFFFFull)))

/* This macro is used to calculate sleep time */
#define TIME_DIFF_SLEEP_CAL_U64(a, b) (TIME_IS_GREATER_U64(a, b) ? TIME_DIFF_U64(a, b) : 0)

/* Beacon tsf start from 24th octet */
#define BCN_TSF_START_OCTET 24

/* Time taken to process the event in NT wlan thread, till */
#define SLEEP_EXIT_TO_DPM_START_TIME_US 240
/* Time taken to process the event in NT wlan thread and data path start */
#define DATAPATH_START_TIME_US 125
/* Time taken to transmit PM null frame */
#define PM_NULL_FRAME_TX_TIME_US 230
/* Time taken in HALPHY operations post sleep exit for TX readiness */
#define HALPHY_SET_CHA_AND_RATE_TBL_RESTORE_TIME_US 980
/* Time taken to restart background services and execute post wake CBs */
#define POST_WAKE_BG_SERVICE_RESTART_US 1100

/* Time taken in first rri table restore */
#define MCU_SLEEP_FIRST_RRI_RESTORE_US 150
/* Time taken in second rri table restore */
#define MCU_SLEEP_SEC_RRI_RESTORE_US 150
/* Time taken in light rri table restore */
#define LIGHT_SLEEP_RRI_RESTORE_US 450
/* Time overhead for non polled RRI restore */
#define NON_POLLED_RRI_OVERHEAD_US 15
/* Time overhead for HDM rri restore for first table */
#define HDM_RRI_FIRST_TABLE_OVERHEAD_US 12
/* Time overhead for HDM rri restore for light table
 * (light_sleep_restore_time - cpu_boot_to_min_cb_time) + base_hdm_rri_overhead */
#define HDM_RRI_LIGHT_TABLE_OVERHEAD_US 125

/* portion of BMPS SW W2S time from sleep registration to sleep entry  */
#define BMPS_SW_W2S_SLPREG_TO_SLP_TIME_US (1000)

/* Time for TWT min callback excluding the time for RRI restoration */
#define TWT_MIN_CB_NO_RRI_TIME_US (21)
/* Time taken for necessary processing and checks before SP start */
#define TWT_SP_START_PROCESSING_TIME_US (110)
#if defined(EMULATION_BUILD)
#define TWT_CLK_GATED_PRE_WAKE_TIME_US (1000)
#define TWT_MCU_SLP_PRE_WAKE_TIME_US   (6000)
#if defined(SUPPORT_LIGHT_SLEEP_FOR_TWT)
#define TWT_LIGHT_SLEEP_PRE_WAKE_TIME_US (2000)
#endif /*SUPPORT_LIGHT_SLEEP_FOR_TWT*/
#else  /* EMULATION_BUILD */
#define TWT_CLK_GATED_PRE_WAKE_TIME_US (2300)
#endif /* EMULATION_BUILD */

#ifdef FEATURE_PERIODIC_WAKE_SLEEP
/* Estimated time for PTSM min callback excluding the time for RRI restoration.
 * To be profiled and refined */
#define PTSM_MIN_CB_NO_RRI_TIME_US 15
/* Estimated time for necessary processing and checks before SP start.
 * To be profiled and refined */
#define PTSM_SP_START_PROCESSING_TIME_US 150
#endif /* FEATURE_PERIODIC_WAKE_SLEEP */

typedef enum {
    PM_MODE_BMPS = 0,  // BMPS power save mode
    PM_MODE_IMPS,
#if (defined NT_FN_WUR_AP) || (defined NT_FN_WUR_STA)
    PM_MODE_WUR,  // WUR power save mode
#endif
#ifdef NT_FN_WNM_POWERSAVE_MODE
    PM_MODE_WNM,  // WNM power save mode
#endif            /* NT_FN_WNM_POWERSAVE_MODE */
#ifdef NT_FN_TWT
    PM_MODE_TWT,  // TWT mode
#endif            /* NT_FN_TWT */
#ifdef FEATURE_PERIODIC_WAKE_SLEEP
    PM_MODE_PTSM, /* Periodic traffic sleep mode */
#endif            /* FEATURE_PERIODIC_WAKE_SLEEP */
} NT_PM_TYPE;

typedef enum { NT_PM_Normal = 0, NT_PM_LowPower, NT_PM_AlwaysOn, NT_PM_Not_Supported } NT_PM_Mode;

typedef enum { nt_ftm = 0, nt_bg_sacan } NT_PM_SERVICE_ID;

typedef enum {
    PM_STATE_SLEEP = 1,
    PM_STATE_AWAKE = 2,
    PM_STATE_FAKE_SLEEP = 3,
} PM_STATE;

typedef enum {
    PM_STATE_TRANSITION_DONE = 0,
    PM_STATE_TRANSITION_TO_AWAKE = 1,
    PM_STATE_TRANSITION_TO_AWAKE_SEND_NULL = 2,
    PM_STATE_TRANSITION_TO_SLEEP = 3,
    PM_STATE_TRANSITION_TO_SLEEP_NULL_SENT = 4,
} PM_STATE_TRANSITION;

typedef enum {
    WHAL_PM_UNDEFINED = 0,
    WHAL_PM_AWAKE = 1,        /* fully wake up chip */
    WHAL_PM_FAKE_SLEEP = 2,   /* wake up chip but set PM on all self generat
e frame */
    WHAL_PM_FULL_SLEEP = 3,   /* fully sleep */
    WHAL_PM_NETWORK_SLEEP = 4 /* sleep but will listen for beacon*/
} WHAL_POWER_MODE;

typedef enum {
    PM_RRI_NONE = 0,
    PM_RRI_MAC_DOWN_LIGHT,  /* MAC turned off with light sleep config */
    PM_RRI_MAC_DOWN_MCUSLP, /* MAC turned off with MCU sleep config */
    PM_RRI_RX_READY,        /* RRI first list restored */
    PM_RRI_SECOND_RESTORE,  /* RRI second list restoration is in progress */
    PM_RRI_LIGHT_RESTORE,   /* RRI light list restoration is in progress */
    PM_RRI_TXRX_READY,      /* full restore (first+second or light) completed */
} PM_RRI_MAC_STATE;

typedef enum {
    EXIT_REASON_NONE,
    EXIT_REASON_BEACON_MISS, /*BMISS threshold in BMPS*/
    EXIT_REASON_TIM_UC,      /*TIM unicast data*/
    EXIT_REASON_TIM_BC,      /*TIM multicase/broadcast data*/
#ifdef SUPPORT_DATAPATH_FLUSH_BEFORE_BMPS_SLEEP
    EXIT_REASON_DATA_PATH,         /*BMPS exit due to datapath flush tops-down wake*/
#endif                             /* SUPPORT_DATAPATH_FLUSH_BEFORE_BMPS_SLEEP */
    EXIT_REASON_CSA,               /*When CSA bit is set in DTIM bcn*/
    EXIT_REASON_NEGATIVE_SLP_TIME, /*Negative slp time on attempt to slp back*/
    EXIT_REASON_EXT_INT,           /*External wakeup by interrupt*/
    EXIT_REASON_RTOS_TIME,         /*wakeup by RTOS*/
    EXIT_REASON_LIMIT,
} PROTOCOL_SLP_EXIT_REASON;

typedef enum {
    PM_EVENT_CONNECTION_STATE_CHANGE,
    PM_EVENT_PROTOCOL_SLEEP_CHANGE,
    PM_EVENT_CHANNEL_CHANGE,
    PM_EVENT_RRT_CHANGE, /* PM event when trigger an RRI resync on change in RRTs*/
} PM_EVENT_TYPE;

typedef enum {
    PS_POLICY_ALLOWED_SLEEP = 0,
    PS_POLICY_NOT_ALLOWED_SLEEP = 1,
} PS_POLICY;

/* PM AP Information */
#ifdef BMPS_AP
typedef struct {
    uint8_t bSBA;
    uint8_t bApPmSleep;
    uint8_t bApAwake;
    uint32_t lastActivity;
    uint32_t idleTime;
    PM_AP_PS_TYPE psType;
    PM_AP_CONFIG_PARAMS pmConfig;
} PM_AP_INFO;
#endif /* BMPS_AP */

/* PM config parameters */
typedef struct {
    uint32_t initial_idle_period_ms;
    uint32_t idle_timer_period_ms;
    uint8_t num_idle_intervals;  // minimum consecutive idle intervals with no change in DXE counters
    uint32_t numPspoll;
    uint32_t pspollTOValue;
    sleep_mode pm_sleep_mode;
    PM_TX_WAKEUP_POLICY_UPON_SLEEP txWakeupPolicy;
    uint32_t numTxToWakeup;
    uint32_t pspollTOMax;
    uint32_t imps_sleep_time;
    uint8_t acceptable_tx_count;
    uint8_t acceptable_rx_count;
    uint16_t max_bcn_rx_no_wake_limit;
    uint8_t force_dtim; // num of dtim interval
    uint16_t round;
    uint16_t bmps_re_enter_delay;
} PM_INFRA_STA_CONFIG_PARAMS;

/*The PM_ACTIVITY_POLICY Structure contains all other Activity policies for power saving mode*/
typedef struct {
    /* If 1 : kill(if BG scan in progress) and stop BG scan while entering to power save ;
     * 0 : wait to complete BG scan (if BG Sacan in progress ) and does not stop BG scan in power save
     * */
    uint8_t stop_bg_scan : 1;

    /* If 1 : kill(if Location manager in progress) and stop Location manager while entering to power save ;
     * 0 : wait to complete Location manager (if Location manager in progress ) and does not stop Location manager in
     * power save
     * */
    uint8_t stop_location_manager : 1;
} PM_ACTIVITY_POLICY;

typedef struct {
    uint16_t bmps_bcn_rx_count;          /* Beacon RX count for BMPS cycle */
    uint16_t bmps_bcn_bc_rx_count;       /* Beacon RX count for BMPS cycle */
    uint16_t bmps_bcn_dtim_count_zero;   /* Beacon RX count for BMPS cycle */
    uint16_t bmps_bcn_dtim_count_n_zero; /* Beacon RX count for BMPS cycle */
    uint16_t bmps_bcn_miss_count;        /* Beacon miss count for BMPS cycle */
    uint16_t bmps_running_neg_pad_count;
    /* Running bmiss count for early RX and beacon wait telescopic increment */
    uint16_t bmps_running_bmiss_count;
    /* Running bmiss count for early RX and beacon wait telescopic increment for debug*/
    uint16_t bmps_running_bmiss_count_history;
#ifdef WLAN_BMPS_TBTT_DEBUG
    uint64_t bmps_last_tsf;      /* Last TSF used in sleep time computation */
    uint64_t bmps_slp_us;        /* Last sleep time computed */
#endif                           /* WLAN_BMPS_TBTT_DEBUG */
    bool bmps_entry_in_progress; /* Reflects whether device is in the process of entering BMPS */
#ifdef BMPS_ENTRY_ABORT_ON_ACTIVITY_POST_ITO
    uint32_t final_ito_slot_rx_count; /* RX data count at final BMPS ITO slot */
    uint32_t final_ito_slot_tx_count; /* TX data count at final BMPS ITO slot */
    uint32_t post_dpm_stop_rx_count;  /* RX data count after DPM stop on BMPS entry */
    uint32_t post_dpm_stop_tx_count;  /* TX data count after DPM stop on BMPS entry */
#endif                                /* BMPS_ENTRY_ABORT_ON_ACTIVITY_POST_ITO */
} bmps_struct_t;

/* PM Dev structure*/
typedef struct {
    devh_t *dev;                         /* pointer to dev */
    PM_POWER_MODE powerMode;             /* current power mode */
    WHAL_POWER_MODE hwPowerMode;         /* Current H/W Power Mode */
    PM_STATE pmState;                    /* current pm state */
    PM_STATE_TRANSITION stateTransition; /* PM state transition */
    WHAL_OPMODE opMode;                  /* ad-hoc or infra mode */

    int8_t pmDisableCount;                                /* Ref. count to track PM Enable */
    int8_t powerModeWakeupCountFP;                        /* Ref. count to track power mode wakeups */
    int8_t pmStateWakeupCount;                            /* Ref. count to track PM state wakeups */
    int8_t pmGlobalDisableCount;                          /*Ref. count to track PM Enable of all the VI's*/
    int8_t fakeSleepEnable;                               /* Ref. count of fake sleep enabled */
    uint8_t bPmEnablePending;                             /* Enabling/Disabling PM */
    uint8_t powerModeModuleWakeupCount[PM_MODULE_ID_MAX]; /* per module power reference count */
    uint8_t bPmEnable;                                    /* PM Enabled or disabled */
    uint8_t bConnected;                                   /* conneced or disconnected */
    bool qos_null_with_pm0_sent;                          /* set to 1 when pm=0 sent */
    uint16_t powerModeWakeupCount;                        /* Ref. count to track per device power mode wakeups */
    nt_status_t fakeSleepStatus;                          /* current status of fake sleep state */
    PM_TIMER pspollTimer;                                 /* ps poll timer */
    PM_TIMER idleTimer;                                   /* idle timer */
    uint8_t idle_time_modified;
#ifdef NT_FN_WNM_POWERSAVE_MODE
    PM_TIMER bss_idle_timer; /* bss idle timer for wnm */
#endif                       /*  NT_FN_WNM_POWERSAVE_MODE */

#ifdef NT_FN_WMM_PS_STA
    uint16_t pspollTimerInterval; /* pspoll timer interval */
#endif                            // NT_FN_WMM_PS_STA
    uint32_t idleTimerInterval;   /* idle timer interval */
    uint32_t numPspoll;           /* number PS-Poll we need send before we fully awake */
    uint32_t numPspollLeft;       /* how many ps-poll left before we fully awake */
    uint32_t beaconInterval;      /* beacon interval */
    uint8_t is_prob_sent_from_pm; /*this falg will be set when pm send prob req to ap to make sure the connection
                                     availability*/
#ifdef NT_FN_DEBUG_STATS
    pm_stats_t psStats;      /* power saving stats */
#endif                       // NT_FN_DEBUG_STATS
    uint8_t bPspollSent;     /* PS Poll sent or not */
    uint8_t bWaitForTxDrain; /* Wait for H/W Tx queue to drain */
    PM_INFRA_STA_CONFIG_PARAMS pmConfig;
    uint16_t pspollTOCount; /* No. of consecutive PS timeout */
    PM_TX_WAKEUP_POLICY_UPON_SLEEP txWakeupPolicy;
    uint32_t numTxToWakeup;          /* No. of consecutive uplink frames sent in sleep, that will cause awake */
    uint32_t pspollTOValue;          /* how long we wait for PS-Poll frame */
    uint8_t bHwCtrlPmField;          /* H/W controls PM field */
    uint8_t bMiss;                   /* Beacon miss */
    uint8_t bForceSleep;             /* Set the power mode to sleep when the SM is still AWAKE */
    uint32_t lastDataActivity;       /* Time in MS of last data activity known to PM  */
    uint32_t lastLegacyDataActivity; /* Time in MS of non-APSD data activity known to PM  */
    NT_PM_Mode station_power_mode;
    uint32_t last_tx_count;
    uint32_t last_rx_count;
    uint32_t bcn_base_early_rx_us;     /* Base early RX time for beacon RX in powersave */
    uint32_t bcn_early_rx_step_us;     /* Step for telescopic increment of beacon early RX time */
    uint32_t beacon_wait_time_us;      /* Time to wait for beacon RX beyond TBTT in powersave */
    uint32_t beacon_wait_inc_step_us;  /* Step for telescopic increment of beacon wait time */
    uint8_t no_of_acceptable_bcn_miss; /* no.of continuous beacon miss acceptable in mini mlme*/
    uint64_t last_sleep_time;
    uint8_t bmps_enabled;     /* BMPS Enabled or disabled */
    uint8_t bmps_log_enabled; /* BMPS log enabled or disabled */
    bool bmps_pwr_opt_enabled; /* BMPS power optimization enabled or disabled */
    bool bmps_compress_qos_null_enabled; /* BMPS power optimization enabled or disabled */
    uint8_t bmps_rx_filter_enabled;
    // IMPS_STRUCT imps_struct;
    uint64_t next_dtim_tbtt_time_us; /*next dtim tbtt time value in microseconds*/
#if defined(NT_FN_PRODUCTION_STATS) || defined(NT_FN_DEBUG_STATS)
    pm_statistics_t pm_statistics;
    // imps_stats_t imps_statistics;
#endif  // NT_FN_PRODUCTION_STATS or NT_FN_DEBUG_STATS
    NT_PM_TYPE pm_type;
#if (defined NT_FN_WUR_AP) || (defined NT_FN_WUR_STA)
    WUR_STRUCT_T wur_struct;
#if defined(NT_FN_PRODUCTION_STATS) || defined(NT_FN_DEBUG_STATS)
    wur_struct_stats_ap_t wur_stats_ap;                    /*! wur statistics for ap  */
#endif                                                     // NT_FN_PRODUCTION_STATS || NT_FN_DEBUG_STATS
    uint32_t sta_count;                                    /*! no of sta assoc with ap */
    wur_struct_sta_list_t wur_sta_list[WUR_MAX_STA_COUNT]; /*! supported wur sta list with mac add and wur id */
#if defined(NT_FN_PRODUCTION_STATS) || defined(NT_FN_DEBUG_STATS)
    wur_struct_stats_sta_t wur_stats_sta; /*! wur statistics for sta  */
#endif                                    // NT_FN_PRODUCTION_STATS or NT_FN_DEBUG_STATS
#endif

#if defined(NT_FN_WNM_POWERSAVE_MODE)
    wnm_ps_struct_stats_ap_t wnm_ps_stats_ap; /*! wnm power save for ap  */
#endif

#ifdef NT_FN_WNM_POWERSAVE_MODE
    wnm_ps_struct_t wnm_ps_struct;
    wnm_ps_struct_stats_sta_t wnm_ps_stats_sta; /*! wnm power save statistics for sta  */
#endif                                          /* NT_FN_WNM_POWERSAVE_MODE */
#ifdef NT_FN_TWT
    twt_struct_t twt_struct;
    twt_struct_stats_sta_t twt_stats_sta; /*! twt statistics for sta  */
    twt_struct_stats_ap_t twt_stats_ap;   /*! twt stats for ap  */
    twt_persist_t twt_persist;
#endif                                    /* NT_FN_TWT */
    uint8_t slp_clk_sel;                  /* Sleep clk selection for AON */
    uint8_t pm_enter_sleep_state;         /*! Sta state sleep or awake  */
    uint8_t pm_currently_running_service; /*! to store currently running service bit-0 : FTM ; bit-1 : BG Scan*/
    PM_POWER_MODE pm_last_power_mode;     /*! last power mode*/
    PM_ACTIVITY_POLICY pm_activity_policy;
    uint8_t pre_slp_hooks_complete; /* To ensure pre sleep hooks were completed when undoing changes on sleep abort */
    uint8_t slp_aborted;            /* To check if sleep abort or wake up when restarting services */
#ifdef SUPPORT_BEACON_MISS_THRESHOLD_TIME
    uint32_t beacon_miss_thr_time_us; /* To update beacon miss threshold value to calculate number of bmiss value */
#endif                                /* SUPPORT_BEACON_MISS_THRESHOLD_TIME */
#ifdef SUPPORT_SAP_POWERSAVE
    uint64_t sap_ps_next_tbtt;
    uint16_t beacon_multiplier;
    NT_BOOL bi_updated;
    uint16_t freq_after_wakeup;
    NT_BOOL sap_ps_enabled;
#endif /* SUPPORT_SAP_POWERSAVE */
    NT_BOOL wlan_state_off;
    PM_RRI_MAC_STATE rri_state;

#if defined(SUPPORT_HDM_INITIATED_RRI)
    uint8_t hdm_triggered_rri_support_enabled; /* feature enable flag from INI */
    uint8_t hdm_triggered_rri_enable;          /* feature enable flag for HAL layer */
    HAL_RRI_LIST_TYPE hdm_triggered_rri_list;  /* RRI list address programmed to HDM */
    uint8_t hdm_triggered_rri_in_progress;     /* flag to indicate HDM RRI is in progress */
    uint8_t hdm_triggered_rri_done;            /* flag to indicate  RRI is done by HW */
    uint32_t hdm_triggered_rri_pass_count;     /*count that keeps track of how many times HDM RRI completed on time*/
    uint32_t
        hdm_triggered_rri_fail_count; /*count that keeps track of how many times HW failed to complete RRI on time*/
    uint32_t hdm_triggered_rri_no_start_count; /*count that keeps track of HDM RRI fail to start on time*/
#endif                                         /*SUPPORT_HDM_INITIATED_RRI*/
    bmps_struct_t bmps_struct;                 /* bmps structure */
#ifdef FEATURE_PERIODIC_WAKE_SLEEP
    nt_periodic_wake_struct_t periodic_wake_struct; /* periodic wake structure*/
#endif                                              /* FEATURE_PERIODIC_WAKE_SLEEP */
    uint64_t last_rx_dtim_tsf;                      /*last dtim rx tsf*/
    uint32_t total_slp_bcn_recv_count;              /* number of beacons received while in mcu sleep */
    uint32_t total_slp_bcn_miss_count;              /* number of beacons missed while in mcu sleep */
    chan_activity_t chan_stats;                     /* Channel activities structure for qpower feature*/
    PROTOCOL_SLP_EXIT_REASON sleep_exit_reason;     /* protocol sleep exit reason */
    uint16_t dxe_tx_count_after_awake;              /* tx count after waking up */
    bool  first_qos_null_pm1_sent;
    bool  bmps_timer_registered_when_awake;
    bool  mybeacon_received;
    PS_POLICY powersave_policy;
} PM_STRUCT;

/**
 * @brief: A static common structure for device agnostic configurations
 */
typedef struct {
    IMPS_STRUCT_CTX_t imps_struct_ctx;
    imps_stats_t imps_statistics;
#ifdef SUPPORT_SW_NON_POLLED_RRI
    uint8_t non_polled_rri_enable;      /* feature enable flag */
    uint64_t non_polled_rri_start_time; /* timestamp to track timeout */
    uint32_t non_polled_rri_pass_count; /* count of how many times non polled RRI completed on time */
    uint32_t non_polled_rri_fail_count; /* count of how many times non polled RRI failed to complete on time */
#endif                                  /* SUPPORT_SW_NON_POLLED_RRI */
    NT_BOOL wlan_state_off;             /* tracks whether MAC is OFF/ON. considered ON when RX ready or TX/RX ready */
    PM_RRI_MAC_STATE rri_state;         /* tracks current RRI state of MAC */
} ppm_common_t;
extern ppm_common_t g_ppm_common_struct;

#define PM_IS_TIMER_SCHEDULED(timer) ((timer)->bScheduled == 1)
#define PM_TIMER_RUNNING(timer)      ((timer)->bScheduled = (timer)->repeat)

#define PM_TRANSECTION_IN_PROGRESS(pPmStruct) ((pPmStruct)->pm_enter_sleep_state == PM_STATE_AWAKE)

#define PM_PROB_SENT_BY_PM(dev) (((PM_STRUCT *)dev->pPmStruct)->is_prob_sent_from_pm)

#ifdef SUPPORT_SAP_POWERSAVE
#define PM_BI_UPDATED(dev) (((PM_STRUCT *)dev->pPmStruct)->bi_updated)
#endif

#define PM_IS_WLAN_STATE_ON(pPmStruct)                ((pPmStruct)->wlan_state_off == FALSE)
#define PM_SET_WLAN_STATE_OFF(pPmStruct)              ((pPmStruct)->wlan_state_off = TRUE)
#define PM_SET_WLAN_STATE_ON(pPmStruct)               ((pPmStruct)->wlan_state_off = FALSE)
#define PM_COMMON_SET_WLAN_STATE_OFF(pPmCommonStruct) ((pPmCommonStruct)->wlan_state_off = TRUE)
#define PM_COMMON_SET_WLAN_STATE_ON(pPmCommonStruct)  ((pPmCommonStruct)->wlan_state_off = FALSE)

#define PM_SET_RRI_STATE(pPmStruct, new_state)              ((pPmStruct)->rri_state = (new_state))
#define PM_GET_RRI_STATE(pPmStruct)                         ((pPmStruct)->rri_state)
#define PM_COMMON_SET_RRI_STATE(pPmCommonStruct, new_state) ((pPmCommonStruct)->rri_state = (new_state))

/* Set protocol sleep exit reason in PM struct */
#define PM_SET_SLEEP_EXIT_REASON(pPmStruct, reason) ((pPmStruct)->sleep_exit_reason = (reason))

/* Get protocol sleep exit reason from PM struct */
#define PM_GET_SLEEP_EXIT_REASON(pPmStruct) ((pPmStruct)->sleep_exit_reason)

void *pmInit(devh_t *);
void pmIdleTimeoutFunc(TimerHandle_t timer_handle);
void pmImpsTimeoutFunc(TimerHandle_t timer_handle);
void pmPspollTimeoutFunc(TimerHandle_t timer_handle);
void pm_deinit(void *pm_inst);
void set_sleep_exit_reason(void);
// static void pmInfraPmEnable(devh_t *, uint8_t);
/*suren : This function is used for renter to sleep mode after send the ps-poll frame to ap*/
// static void pmCancelPspollState(devh_t *);
/*suren : If Rx/Tx not happened in previous idle timer we need to cancel idle timer*/
// static void pmCancelIdleTimer(devh_t *);
void pmEndStateTransition(devh_t *);
nt_status_t pmInfraStateTransitionToSleep(devh_t *, _Bool);
void pmSendPspollOrNull(devh_t *);
void pmInfraConnectionNotify(devh_t *, uint8_t);
// static void pmInfraMyBeaconCallback(devh_t *, _Bool, uint8_t *, _Bool);
// static void pmInfraStaDataPreSendNotify(devh_t *, void *, uint32_t *, conn_t *,uint32_t, _Bool);
// static void pmInfraStaDataPostSendNotify(devh_t *, void *, uint32_t *, conn_t *,uint32_t);
// static void pmInfraStaDataSendCompleteNotify(devh_t *, void *, uint32_t *,conn_t *, uint32_t, _Bool, nt_status_t);
nt_status_t pmDataQueuesEmptyCallback(devh_t *, uint8_t);
// static void pmInfraStaDataReceiveNotify(devh_t *, uint32_t, _Bool, _Bool, _Bool,uint32_t, conn_t *, _Bool, uint32_t);
void pmInfraStaSetPmParams(devh_t *, PM_INFRA_STA_CONFIG_PARAMS *);
void pmSetPowerSaveMode(devh_t *dev, NT_PM_Mode mode);
void nt_process_pm_dph_data_available_interrupt();
void nt_bmps_wake_up_notification_sta(void);
uint64_t nt_process_beacon(devh_t *dev);
void nt_bmps_wakeup_callback(soc_wkup_reason wkup_reason);
#ifndef FEATURE_FPCI
void nt_bmps_pre_sleep_hooks(devh_t *dev);
void nt_bmps_post_wake_hooks(devh_t *dev);
void nt_bmps_sleep_abort_hooks(devh_t *dev);
#endif /* FEATURE_FPCI */
void nt_bmps_enter_sleep_callback();
nt_status_t nt_enter_power_save_mode();
void nt_pm_delay(uint32_t delay_count);
void nt_wnm_state_transition_to_awake();
uint64_t bmps_compute_slp_time(PM_STRUCT *pPmStruct, uint64_t bcn_tstamp, uint16_t bcn_intvl, uint8_t dtim_count);

/* AP mode */
#ifdef BMPS_AP
#ifdef CONFIG_AP_POWER_SAVE
static void pmApStateTransitionToAwake(devh_t *, uint32_t);
static void pmApStateTransitionToSleep(devh_t *);
#endif /* CONFIG_AP_POWER_SAVE */
static void pmApPmEnable(devh_t *, uint8_t);
static void pmApMgmtReceiveNotify(devh_t *, uint8_t *, conn_t *, uint8_t);
static void pmApDataReceiveNotify(devh_t *, uint32_t, _Bool, _Bool, _Bool, uint32_t, conn_t *, _Bool, uint32_t);
static void pmApDataPreSendNotify(devh_t *, void *, uint32_t *, conn_t *, uint32_t, _Bool);
static void pmApMyBeaconCallback(devh_t *, _Bool, uint8_t *, _Bool);
static nt_status_t pmApSBAHandler(devh_t *);
static nt_status_t pmApNDPHandler(devh_t *);
static void pmApConnectionNotify(devh_t *, _Bool);

#endif /* BMPS_AP */
uint64_t wakeup_beaconProcess(__unused uint32_t wkup_delay);
/* common */
void pmInitTimer(PM_TIMER *, void *timer_id, uint32_t timeout_value, void *pFunction);
void pmTimeout(PM_TIMER *);
void pmUnTimeout(PM_TIMER *);
void pmDeleteTimer(PM_TIMER *);
nt_status_t nt_pm_stop_all_bg_services(devh_t *);
nt_status_t nt_pm_start_all_bg_services(devh_t *);
void pmInfraStateTransitionToAwake(devh_t *dev, NT_BOOL bSendNull, NT_BOOL bStartIdleTimer);
/*
 *  @brief : to get currently enabled power save
 *  @param : None
 *  @return : 8bit value. (bit-0 : bmps ; bit-1 : imps ; bit-2 : wur ; bit-3 : wnm ;)
 */
uint8_t get_currently_enabled_powersave(devh_t *dev);
/*static void pmInfraPauseResumeSM(devh_t *, _Bool);*/

/*
 *  @brief : to check any of the wlan power save protocols enabled in INI
 *  @param : None
 *  @return : BOOL. TRUE if any wlan power save protocol enabled else FALSE
 */
NT_BOOL is_wlan_power_save_enabled();

/*
 *  @brief : All services have to call this function to notify power manager about the service begin and end
 *  @param : dev :  device structure; service_id : service unique id; service_statue : begin/end
 *  @return : status
 */
nt_status_t nt_wpm_service_notify(devh_t *dev, NT_PM_SERVICE_ID service_id, NT_BOOL service_state);

/*
 *  @brief : To set default power save policy for all other activities
 *  @param : dev :  device structure;
 *  @return : none
 */
void nt_wpm_set_default_activity_policy(devh_t *dev);

/**
 *  @func   nt_beacon_check_sum
 *  @brief  This function is used to calculate checksum for the input buffer.
 *  @Param  frm : input buffer frame
 *  @Param  len : lenth of the  buffer
 *  @Param  dev : output c_sum
 *  @Return none
 */
void nt_beacon_check_sum(uint8_t *frm, uint16_t len, uint8_t *c_sum);

/**
 *  @func   nt_pm_stop_dpm
 *  @brief  This function is used to call stop dpm handler.
 *  @Param  data_available_notify_cb : notify to power manager  about data availability on DPM
 *  @Return none
 */
void nt_pm_stop_dpm(void *data_available_notify_cb);

/**
 *  @func   nt_pm_start_dpm
 *  @brief  This function is used to call start dpm handler.
 *  @Return none
 */
void nt_pm_start_dpm();

/**
 *  @func   nt_pm_pause_wmm
 *  @brief  This function is used to call wmm pause handler.
 *  @Param  data_available_notify_cb : notify to power manager  about data availability on DPM
 *  @Return none
 */
void nt_pm_pause_wmm(void *data_available_notify_cb);

/**
 *  @func   nt_pm_unpause_wmm
 *  @brief  This function is used to call wmm unpause handler.
 *  @Return none
 */
void nt_pm_unpause_wmm();

/**
 * @func nt_pm_prob_res_recv_notify
 * @brief This function is used to notify pm about prob resp recv.
 * @Return none
 */
void nt_pm_prob_res_recv_notify();

/**
 * @func nt_pm_prob_res_timout_cb
 * @brief If power save exit due to beacon miss, Pm will send prob req to current ap to make sure the connection
 * availability and wait for certain timeout. if prob res received before timout then, pm stop timer. else start to
 * reconnect process.
 * @Return none
 */
void nt_pm_prob_res_timout_cb(TimerHandle_t timer_handle);
/**
 * @func nt_pm_send_prob_req_to_check_is_ap_alive
 * @brief If power save exit due to beacon miss, Pm will call this function to send prob req to make sure the connection
 * availability.
 * @Return none
 */
void nt_pm_send_prob_req_to_check_is_ap_alive(devh_t *dev);

/**
 * @func nt_pm_set_force_dtim
 * @brief to set force DTIm value is force_dtim value > 0
 * @Return none
 */
void nt_pm_set_force_dtim(devh_t *dev, uint32_t forced_dtim);

NT_BOOL pmIsSOCWakeFromTwtSleep(void);

#if defined(NT_FN_DEBUG_STATS) || defined(NT_FN_PRODUCTION_STATS)
void *nt_wpm_get_power_save_stats(devh_t *dev);
#endif  // NT_FN_PRODUCTION_STATS or NT_FN_DEBUG_STATS

#ifdef SUPPORT_BEACON_MISS_THRESHOLD_TIME
/**
 * @func nt_update_beacon_miss_threshold
 * @brief to update beacon miss threshold time only if BMTT is supported
 * @Return none
 */
void nt_update_beacon_miss_threshold(devh_t *dev);
#endif /* SUPPORT_BEACON_MISS_THRESHOLD_TIME */

/*
 * @brief  Helps to notify the NT_WLAN task to do tops down notify from other threads
 * @param  : dev : device structure pointer
 * @return : NONE
 */
void nt_pm_notify_top_down_wakeup(devh_t *dev);

/*
 * @brief  Helps to check the BMPS status
 * @param  : NONE
 * @return NT_BOOL -> TRUE: if BMPS enabled; FALSE: Otherwise
 */
NT_BOOL nt_pm_is_bmps_enabled(void);

/*
 * @brief  Helps to configure BMPS/IMPS through APP
 * @param  : NONE
 * @return NONE
 */
void nt_enable_bmps_imps(void);
/*IMPS APIS end*/

#ifdef PMU_REG_RETENTION_STATUS_FOR_SOC_SLP
/*
 * @brief  Helps to get the PMU_DIG_TOP_REG retention status in last soc sleep and reset the status.
 * @param  : none
 * @return PMU_DIG_TOP_REG retention status
 * Limitation: we can call this API at one place only because it also reset the reg retention status.
 */
uint32_t nt_pm_get_pmu_dtop_reg_retention_status(void);

/*
 * @brief  Helps to set and get the PMU_DIG_TOP_REG retention status in current soc sleep.
 * @param  : soc_sleep_mode
 * @return : PMU_DIG_TOP_REG retention status
 */
uint32_t nt_pm_set_and_get_pmu_dtop_reg_retention_status(sleep_mode soc_slp_mode);
#endif /* PMU_REG_RETENTION_STATUS_FOR_SOC_SLP */

/**
 * @brief:  The function is used to initialise ppm_common_struct members
 *
 * @param: None
 * @return: None
 */
void ppm_common_init(void);

/*
 * @brief  : Returns the pre sleep callbacks completion status
 * @param  : None
 * @return : pPmStruct->pre_slp_hooks_complete
 */
uint8_t nt_pm_get_pre_slp_cb_complete_status(void);

/**
 * @brief Calculate early RX time for beacon reception during power save
 * @Param  : dev : device structure pointer
 * @Return : Early RX time computed
 */
uint32_t nt_pm_get_bmps_beacon_early_rx(devh_t *dev);

/*
 * @brief  : Returns the pre sleep callbacks completion status
 * @param  : event -> Event to take action for
 * @return : None
 */
void nt_pm_process_event(devh_t *dev, PM_EVENT_TYPE event);

#if defined(SUPPORT_SAP_POWERSAVE)
NT_BOOL xpan_sap_lp_get_mode(void);
void xpan_sap_lp_set_mode(NT_BOOL ps_value);
#endif

/*
 * @brief  : Checks and ensures RRI TX/RX readiness
 * @param  : dev -> device structure pointer
 * @return : None
 */
void nt_pm_enforce_rri_readiness(devh_t *dev);

#ifdef SUPPORT_SW_NON_POLLED_RRI
/*
 * @brief  : Returns whether non polled RRI triger is allowed
 * @param  : None
 * @return : TRUE if non polled RRI is allowed
 */
uint8_t nt_pm_is_non_polled_rri_allowed(void);

/*
 * @brief  : Initiates non polled RRI restore for specfied table
 * @param  : type -> RRI list to trigger restoration for
 * @return : None
 */
void nt_pm_initiate_non_polled_rri(HAL_RRI_LIST_TYPE list);

/*
 * @brief  : Returns whether non polled RRI completed
 * @param  : list -> RRI list to check completion for
 * @return : TRUE if non polled RRI completed
 */
uint8_t nt_pm_check_non_polled_rri_complete(HAL_RRI_LIST_TYPE list);

/*
 * @brief  : Synchronization point for non polled RRI completion
 * @param  : dev -> device structure pointer
 * @return : None
 */
void nt_pm_sync_non_polled_rri_completion(devh_t *dev);
#endif /* SUPPORT_SW_NON_POLLED_RRI */

#if defined(SUPPORT_HDM_INITIATED_RRI)
/*
 * @brief  : Returns whether HDM triggered RRI completed
 * @param  : pPmStruct -> pointer to PM struct
 * @return : TRUE if HDM triggered RRI completed
 */
uint8_t nt_pm_check_hdm_rri_complete(PM_STRUCT *pPmStruct);
#endif /* SUPPORT_HDM_INITIATED_RRI */

/*
 * @brief  Compute sleep to wake overhead time to be compensated for BMPS
 * @param  : pPmStruct -> Pointer to PM struct
 * @return : Compensation time in microseconds
 */
uint64_t bmps_compute_s2w_compensation_time(PM_STRUCT *pPmStruct);

/*
 * @brief  Compute sleep to wake overhead time to be compensated for TWT
 * @param  : pPmStruct -> Pointer to PM struct
 * @param  : mode -> sleep mode
 * @return : Compensation time in microseconds
 */
uint64_t twt_compute_s2w_compensation_time(PM_STRUCT *pPmStruct, sleep_mode mode);

#ifdef FEATURE_PERIODIC_WAKE_SLEEP
/*
 * @brief  Compute sleep to wake overhead time to be compensated for PTSM
 * @param  : pPmStruct -> Pointer to PM struct
 * @param  : mode -> sleep mode
 * @return : Compensation time in microseconds
 */
uint64_t ptsm_compute_s2w_compensation_time(PM_STRUCT *pPmStruct, sleep_mode mode);
#endif /* FEATURE_PERIODIC_WAKE_SLEEP */

/*
 * @brief  Perform necessary halphy restoration on exit form powersave
 * @param  : dev -> device structure pointer
 * @param  : profile -> calibration profile to restore
 * @return : None
 */
void nt_wpm_wakeup_channel_restore(devh_t *dev, halphy_cal_profile_t profile);

uint8_t nt_pm_check_if_wkup_from_network_activity(PM_STRUCT *pPmStruct);

void pm_save_active_sleep_time_record(devh_t *dev, uint32_t sleep_time, uint32_t active_time, BMPS_STATS_TYPE type);
void pm_save_tx_rx_counts_during_certain_period(devh_t *dev);
void pm_statistic_init(pm_statistics_t * pm_statistics);
void pm_statistic_deinit(pm_statistics_t * pm_statistics);

static inline void pm_set_powersave_policy(devh_t *dev, PS_POLICY policy)
{
    if (dev && dev->pPmStruct) {
        PM_STRUCT *pPmStruct = (PM_STRUCT *)dev->pPmStruct;
        pPmStruct->powersave_policy = policy;
    }
}
#endif  // _WLAN_POWER_H_
