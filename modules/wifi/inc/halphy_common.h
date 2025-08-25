/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * @file halphy_rx.h
 * @brief MACROs, FLAGs, utility functions header for Halphy
 * ======================================================================*/

#ifndef _HALPHY_COMMON_H_
#define _HALPHY_COMMON_H_

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>

/* this flag is to dump the calibration registers after calibration or restore */

//#define HALPHY_DUMP_CAL_REG

#ifdef _WIN32
#include <stdio.h>

#define NT_LOG_PRINT(MODNAME, LVL, msg, ...) \
    {                                        \
        printf(msg, __VA_ARGS__);            \
        printf("\n");                        \
    }
#include <Windows.h>
#define nt_osal_calloc(count, size) calloc(count, size)
#define nt_osal_free_memory(ptr)    free(ptr)
#else
#include "wifi_cmn.h"
#include <nt_flags.h>
#include <nt_logger_api.h>
#include <nt_osal.h>
#endif

#include "nt_common.h"
#include "component_bdf_def.h"
/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
typedef enum freq_band /* WLAN freq bands */
{ PHYRF_5G = 0,
  PHYRF_2G = 1,
  PHYRF_6G = 2,
} freq_band_t;

typedef enum mod_type /* WLAN OFDM modulation type */
{ BPSK = 0,           /* 6M, 9M, MCS0 */
  QPSK = 1,           /* 12M, 18M, MCS1, MCS2 */
  QAM16 = 2,          /* 24M, MCS3 */
  CCK = 3,            /* all 11B modulation */
} mod_type_t;
/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#ifdef PLATFORM_FERMION
#define NUM_CBC_CHANNELS CHLIST_CBC_NUM
#else
#define NUM_CBC_CHANNELS 2
#endif
#ifndef _WIN32
//#define NT_FN_SOCPM_CTRL // Enable Halphy PDC APIs
#endif

#define MID_CHANNEL_2_4GHZ 7
#define DEFAULT_CHANNEL    2412

/* Halphy Cal Algorithm version. Change this if there is any update in Cal Alogorithm,
 * So that CalDB stored in RRAM get updated*/
#define HALPHY_CAL_VERSION 32

#define MAX(x, y)                 (((x) > (y)) ? (x) : (y))
#define MIN(x, y)                 (((x) < (y)) ? (x) : (y))
#define PHYRF_GET_XG(freq)        (((freq) > 5940) ? PHYRF_6G : (((freq) > 4800) ? PHYRF_5G : PHYRF_2G))
#define DEFAULT_2G_FREQ           2412
#define DEFAULT_5G_FREQ           4920
#define DEFAULT_6G_FREQ           5955
#define TXFIR_FINEGAIN_OFFSET_MAX 63
#define TXFIR_FINEGAIN_OFFSET_MIN 0

#define TEMP_RECAL_TEMP_LO 0  /* low tempeature falling below this tempeature re-calibration would be triggered */
#define TEMP_RECAL_TEMP_HI 60 /* high tempeature falling above this tempeature re-calibration would be triggered */
#define ROOM_TEMP          25 /* room tempeature */
#define CH12_FREQ          2467
#define CH13_FREQ          2472
/*------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/

void halphy_compute_checksum(uint8_t *pData, uint32_t length, uint16_t *checksum_out);

#endif /* _HALPHY_COMMON_H_ */
