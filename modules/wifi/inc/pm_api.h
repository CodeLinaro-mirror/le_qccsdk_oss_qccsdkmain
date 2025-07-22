/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef _PM_API_H_
#define _PM_API_H_
#include "wifi_cmn.h"
#include "wlan_dev.h"
#include "nt_common.h"

// macro to convert beacon interval TU to uS (each tu is 2^10 uS)
#define NT_BEACON_TBTT_CALC(bcn_intvl_tu) ((bcn_intvl_tu) << 10)

/****************************************************************
 *                  PM API definitions                          *
 ****************************************************************/

/*
 * Macro definitions
 */

#ifdef POWER_SAVE
#define NT_SET_POWERMODE(dev, mode, module) pmSetPowerMode(dev, mode, module)
#else
#define NT_SET_POWERMODE(dev, mode, module)
#endif  // POWER_SAVE

#define NT_SET_POWERMODE_EXT(dev, mode, module)                      \
    {                                                                \
        PM_STRUCT *pPmStruct = NULL;                                 \
        PM_CMN_STRUCT *pPmCmnStruct = NULL;                          \
                                                                     \
        if (dev)                                                     \
            pPmStruct = (PM_STRUCT *)dev->pPmStruct;                 \
        if (pPmStruct)                                               \
            pPmCmnStruct = (PM_CMN_STRUCT *)pPmStruct->pPmCmnStruct; \
                                                                     \
        if (pPmStruct && pPmCmnStruct) {                             \
            NT_SET_POWERMODE(dev, mode, module);                     \
        }                                                            \
    }

/* Infrastrucure mode */
#define PM_MAX_SERVICE_INTERVAL 0xFFFFFFFF

/*
 * Data structure defintions
 */

/* Infrastructure mode */

/* WLAN power state */
typedef enum {
    PM_POWER_MODE_SLEEP = 1,
    PM_POWER_MODE_AWAKE = 2,
} PM_POWER_MODE;

typedef struct {
    TimerHandle_t osTimer;
    uint8_t bScheduled;
} PM_TIMER;

typedef enum {
    PM_AWAKE_EVENT = 1,
    PM_SLEEP_EVENT = 2,
    PM_FAKESLEEP_EVENT = 3,
#define PM_MAX_EVENTS (PM_FAKESLEEP_EVENT + 1)
} PM_EVENT;

/* PM PS state with respect to AP or GO */
typedef enum {
    PM_PS_STATE_SLEEP = 1,
    PM_PS_STATE_AWAKE = 2,
} PM_PS_STATE;

typedef void (*PM_EVENT_HANDLER)(devh_t *pDev, PM_EVENT, nt_status_t);

/* Module id for tracking purpose */
typedef enum {
    PM_MODULE_ID_CSERV = 1,
    /*For McKinley 1.0, LPL uses the module ID from MLME to register for event
     * notification. This is beacuse of the MAX module id limit set. FYI*/
    PM_MODULE_ID_MLME = 2,
    PM_MODULE_ID_TXRX = 3,
    PM_MODULE_ID_PM = 4,
    PM_MODULE_ID_BT_COEX = 5,
    PM_MODULE_ID_CO = 6,
    PM_MODULE_ID_DC = 7,
    PM_MODULE_ID_RO = 8,
    PM_MODULE_ID_CM = 9,
    PM_MODULE_ID_RRM = 10,
    PM_MODULE_ID_AP = 11,
    PM_MODULE_ID_KEYMGMT = 12,
    PM_MODULE_ID_DEPRECATED = 13,
    PM_MODULE_ID_MISC = 14,
    PM_MODULE_ID_DFS = 15,
    PM_MODULE_ID_TIMER = 16,
    PM_MODULE_ID_P2P = 17,
    PM_MODULE_ID_LPL = 18,
    PM_MODULE_ID_SCHED = 19,
    PM_MODULE_ID_HS_WFF = 20,
    PM_MODULE_ID_HOST = 21,
    /*Adding some reserves for future modules. Kindly add comment here
     if using any of the reserved IDs. Using RSV2 as lock module*/
    PM_MODULE_ID_RSV3 = 22,
    PM_MODULE_ID_RSV6 = 23,
    PM_MODULE_ID_MAX = 24
} PM_MODULE_ID;

/* Tim policy to use during FULL APSD */
typedef enum {
    PM_IGNORE_TIM_FULL_APSD = 1,
    PM_PROCESS_TIM_FULL_APSD = 2,
    PM_IGNORE_TIM_SIMULATED_APSD = 3,
    PM_PROCESS_TIM_SIMULATED_APSD = 4,
} PM_APSD_TIM_POLICY;

/* ALL_BEACON filter policy during sleep */
typedef enum { PM_DISALLOW_ALL_BEACONS = 1, PM_ALLOW_ALL_BEACONS = 2 } PM_ALL_BEACON_FILTER_POLICY;

/* Policy to determnine whether TX should wakeup WLAN if sleeping */
typedef enum { PM_TX_WAKEUP_UPON_SLEEP = 1, PM_TX_DONT_WAKEUP_UPON_SLEEP = 2 } PM_TX_WAKEUP_POLICY_UPON_SLEEP;

/*
 * Policy to determnine whether power save failure event should be sent to
 * host during scanning
 */
typedef enum {
    PM_SEND_POWER_SAVE_FAIL_EVENT_ALWAYS = 1,
    PM_IGNORE_POWER_SAVE_FAIL_EVENT_DURING_SCAN = 2,
} PM_POWER_SAVE_FAIL_EVENT_POLICY;

typedef enum {
    PM_AP_PS_DISABLE = 1,
    PM_AP_PS_ATH = 2,
} PM_AP_PS_TYPE;

typedef struct {
    PM_AP_PS_TYPE psType;
    uint32_t idleTime;   /* msec */
    uint32_t psPeriod;   /* in usec */
    uint8_t sleepPeriod; /* in 10 msec */
} PM_AP_CONFIG_PARAMS;

/*
 * Extra flags used in receive processing
 */
typedef enum {
    PM_RECV_IS_AGGR = 0x1,
    PM_RECV_MORE_AGGR = 0x2,
    PM_RX_CHAIN_HIGH = 0x4,
} PM_RECV_FLAGS;

/*
 * Function defintions
 */

/* Infrastructre mode */
void pmAPSDEnable(devh_t *, NT_BOOL, uint32_t, uint32_t);
NT_BOOL pmIsAPSDEnabled(devh_t *);
void pmBmissNotify(devh_t *);
void pmSetVoicePktSize(devh_t *, uint32_t);

/* AP mode */
uint32_t pmApGetPSPeriod(devh_t *);
void pmApSetPmParams(devh_t *, PM_AP_CONFIG_PARAMS *);

/* common */
uint32_t pmGetPMBit(devh_t *);
void pmChanOpRequest(devh_t *, NT_BOOL);
void pmChanOp(devh_t *, PM_MODULE_ID);
uint32_t pmGetStats(devh_t *, uint8_t *, uint32_t);
void pmConnectionNotify(devh_t *, NT_BOOL, WHAL_OPMODE, NT_BOOL, NT_BOOL, uint32_t /*, WLAN_PHY_MODE*/);
void release_osTimer(devh_t *dev);
PM_POWER_MODE pmGetPowerMode(devh_t *, NT_BOOL *);
nt_status_t pmSetPowerMode(devh_t *, PM_POWER_MODE, PM_MODULE_ID);
void pmSetPowerModeFP(devh_t *, PM_POWER_MODE, PM_MODULE_ID);
nt_status_t pmFakeSleep(devh_t *, NT_BOOL, uint32_t, NT_BOOL, PM_MODULE_ID);
nt_status_t pmEnable(devh_t *, NT_BOOL);
void pmMyBeaconCallback(devh_t *, NT_BOOL, uint8_t *, NT_BOOL);
void pmSetAllBeaconPolicy(devh_t *, PM_ALL_BEACON_FILTER_POLICY);
void pmRegisterEventHandler(devh_t *, PM_EVENT_HANDLER, uint32_t, PM_MODULE_ID);
nt_status_t pmSBAHandler(devh_t *);
PM_PS_STATE pmGetPSState(devh_t *);
NT_BOOL pmIsInSleepState(devh_t *dev);

#ifdef NT_FN_WMM_PS_STA
/**@breif function sets the threshold value for pspoll
 * @params threshold_value: intended threshold value for pspoll
 * @return none
 */
void nt_pm_pspoll_threshold_set(devh_t *dev, uint8_t threshold_value);
#endif

#endif  // _PM_API_H_
