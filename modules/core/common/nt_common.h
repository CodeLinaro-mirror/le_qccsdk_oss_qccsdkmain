/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @brief common definitions and types used across the NT code base
 */

#ifndef _NT_COMMON_H_
#define _NT_COMMON_H_

#include <stdint.h>
#ifndef _WIN32
#include "FreeRTOS.h"
#endif
#include "wifi_cmn.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"
// status codes ----------------------------------------------------------------------------------
// 31:16 : bit map of supported modules
// 15:12 : arbitrary use by module (eg sub-module defs)
// 11:0  : actual error code
typedef uint32_t nt_status_t;

/**********Task sizes**********/

#define MLME_TASK_STACK_SIZE      (1200)
#define WIFI_MNGR_TASK_STACK_SIZE (700)
#define AP_TASK_STACK_SIZE        (200 + 200)
#define STA_TASK_STACK_SIZE       (200 + 200)

/**********Task priorities********/

#define MLME_TASK_PRIORITY      7
#define WIFI_MNGR_TASK_PRIORITY 3
#define AP_TASK_PRIORITY        4
#define STA_TASK_PRIORITY       4

/********************************/

#define NT_STATUS_CODE_MASK   0x00000FFF
#define NT_STATUS_MODULE_MASK 0xFFFF0000
#define NT_STATUS_SUBMOD_MASK 0x0000F000

#define NT_STATUS_CODE_GET(x)   ((x)&NT_STATUS_CODE_MASK)
#define NT_STATUS_MODULE_GET(x) ((x)&NT_STATUS_MODULE_MASK)
#define NT_STATUS_SUBMOD_GET(x) ((x)&NT_STATUS_SUBMOD_MASK)

#define NT_STATUS_MAKE(code, mod, submod) \
    (NT_STATUS_CODE_GET(code) | NT_STATUS_MODULE_GET(mod) | NT_STATUS_SUBMOD_GET(submod))

#define NT_DEFAULT_QOS_DATA_TIMER_PERIOD 10
#define NT_DEFAULT_TRIGGER_TIMER_PERIOD  50

/** UAPSD specific */
#define NT_TOTAL_ACCESS_CATEGORIES  4
#define NT_TIDS_PER_ACCESS_CATEGORY 2
#define NT_AC_DELIVERY_ENABLE_CHECK(notified_ac, txrx_struc) \
    ((1 << (notified_ac)) &                                  \
     txrx_struc->sc_apsdConfig.deliveryEnabled)  // check if notification received AC is uapsd enabled AC
#define NT_EXTRACT_SERVICE_PERIOD(apsd_info) \
    ((((apsd_info >> IEEE80211_QOSINFO_MAXSP_S) & IEEE80211_QOSINFO_MAXSP_M) * 2))
#define NT_MASKED_UAPSD_DELIVERY_DATA(apsd_info) (apsd_info & IEEE80211_QOSINFO_UAPSD_FLAGS_M)
#define NT_MASKED_UAPSD_TRIGGER_DATA(apsd_info)  (apsd_info & IEEE80211_QOSINFO_UAPSD_FLAGS_M)
#define NT_DEFAULT_PSPOLL_TIMER_TIMEOUT_VAL      1000

/** Frame/packet common macros. */

#define NT_MAC_ADDR_SIZE  6  /** MAC address size 6 Bytes. */
#define NT_IPV4_ADDR_SIZE 4  /** IPv4 address size 4 Bytes. */
#define NT_IPV6_ADDR_SIZE 16 /** IPv6 address size 16 Bytes. */

/** Definitions for error constants. */

#define NT_FAIL      -1 /** FAILUE */
#define NT_OK        0  /** No error, everything OK. */
#define NT_EPARAM    1  /** Parameter errors,  Illegal argument. */
#define NT_ERANGE    2  /** Out of range.(Total number exceeds the set maximum) */
#define NT_ENOMEM    3  /** Not enough memory. */
#define NT_EINVFRM   4  /** Invalid frame. */
#define NT_ENORES    5  /** Resource allocation failed.	*/
#define NT_EINVSTATE 6  /** State machine error. */
#define NT_EPENDING  7  /** Event pending. */
#define NT_ECONSUMED 8  /** Object consumed. */
#define NT_ETXFAIL   9  /** Transmit failure. */
#define NT_ERXFAIL   10 /** Receive failure. */
#define NT_ETIMEOUT  11 /** Timeout occurred. */
#define NT_EINVPTR   12 /** Invalid pointer. */
#define NT_EADDRUSED 13 /** Address in use.	*/
#define NT_EFAIL     14 /** Operation fail */

#define NT_ECANCELED 15 /** Action/Connection aborted.	*/
#define NT_ECONNRST  16 /** Connection reset. */
#define NT_ECONN     17 /** Not connected. */
#define NT_ECONNCLSD 18 /** Connection closed. */
#define NT_EISCONN   19 /** Already connected. */

#define NT_EIF         20 /** Low-level netif error. */
#define NT_ERTE        21 /** Routing problem. */
#define NT_EINPROGRESS 22 /** Operation in progress. */

#define NT_EINVIPADDR 23 /** Invalid IP address format received. */
#define NT_EQUEUEFAIL 24 /** Queue send/receive failed. */
#define NT_ESEMPFAIL  25 /** Semaphore give/take failed. */

#define NT_SAEFAIL 26 /** SAE Auth failures */

#define NT_ERRMAX 27 /** Error max - keep this as last entry */

/**Queues definitions****/

#define NT_MANAGEMENT_FRAME_QUEUE 9

#if (NT_CHIP_VERSION == 2)

/**
 * Return Status
 */
#define FAILURE 0  // return 0 when the callback fails
#define PASS    1  // return 1 when the callback is success

#endif  //#if (NT_CHIP_VERSION==2)
/**
 *  register definitions -----------------------------------------------------------------------   //vinod
 */
// write data to particular location and read from data particular location.
#define HAL_REG_WR(_reg, _val) *((volatile unsigned long *)(_reg)) = ((unsigned long)(_val))
#define HAL_REG_RD(_reg)       *((volatile unsigned long *)(_reg))
#define NT_REG_WR(_reg, _val)  HAL_REG_WR(_reg, _val)
#define NT_REG_RD(_reg)        HAL_REG_RD(_reg)

// Register Read and Write
#define HW_REG_WR(_reg, _val)                  \
    (*((volatile uint32_t *)(_reg))) = (_val); \
    __asm volatile("dsb" ::: "memory")
#define HW_REG_RD(_reg) (*((volatile uint32_t *)(_reg)))

#define CHECK_BIT_SET(value, pos) (value & (1 << pos))
// logging control -----------------------------------------------------------------------------

// compile time options/configs ---------------------------------------------------------------

typedef _Bool NT_BOOL;

/* enum application mode */
typedef enum app_mode_id {
    APP_MODE_MM = 0,
    APP_MODE_FTM = 1,
    APP_MODE_MAX = 2,
} app_mode_id_t;

typedef enum module_id_t {
    NT_STATUS_MOD_HAL = 1,
    NT_STATUS_MOD_SME = 2,
    NT_STATUS_MOD_MLM = 3,
    NT_STATUS_MOD_DPM = 4,
    NT_STATUS_MOD_WFM = 5,
    NT_STATUS_MOD_WMI = 6,
    NT_STATUS_MOD_HALPHY = 7,
    NT_STATUS_MOD_CLI = 8,
    NT_STATUS_MOD_SYSTEM = 9,
    NT_STATUS_MOD_WIFI_APP = 10,
    NT_STATUS_MOD_COMMON = 11,
    NT_STATUS_MOD_SECURITY = 12,
    NT_STATUS_MOD_HAL_API = 13,
    NT_STATUS_MOD_COMMISIONING_APP = 14,
    NT_STATUS_MOD_ONBOARDING_APP = 15,
    NT_STATUS_MOD_WPS = 16,
    NT_STATUS_MOD_AWS = 17,
    NT_STATUS_MOD_COEX = 18,
    NT_STATUS_MOD_PDC = 19,
    NT_STATUS_MOD_RCLI = 20,
    NT_STATUS_MOD_HOSTED_APP = 21,
    NT_STATUS_MOD_TLS = 22,
    NT_STATUS_MOD_DEV_CFG = 23,
    NT_STATUS_MOD_SOCPM = 24,
    NT_STATUS_MOD_WPM = 25,
    NT_STATUS_MOD_RA = 26,
    NT_STATUS_MOD_ANI = 27,
    NT_STATUS_MOD_FTM = 28,
    NT_MAX_MODULE_ID
} module_id;

#endif  // _NT_COMMON_H_
