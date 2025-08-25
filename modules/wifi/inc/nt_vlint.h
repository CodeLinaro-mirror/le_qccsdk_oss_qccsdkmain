/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
* @file nt_vlint.h
* @brief advanced wrapper for vlint.h
*========================================================================*/

#ifndef _VLINT_H_
#define _VLINT_H_

/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#ifdef FERMION_CONFIG_HCF

#include <stdio.h>
#include <sys/types.h>
#include "vlint.h"
#include "vlint_private.h"
#include "nt_devcfg.h"
#include "wlan_dev.h"
#include "nt_devcfg_structure.h"
#include "nt_logger_api.h"
#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "nt_osal.h"
#include <assert.h>

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define MIBKEY_NULL 0

/*-------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
typedef uint8_t *VLDATA;

typedef struct mibvalue {
    uint16_t keyid;
    uint16_t length;
    VLDATA data_array;
} MIB_KEYVAL;

typedef enum {
    CONFIG_STATE_READ_HEADER = 1, /* Reading the file header, assumed 0 */
    CONFIG_STATE_READ_DESC,       /* Reading the KEYVAL header */
    CONFIG_STATE_READ_DATA        /* Reading the KEYVAL data */
} config_state_t;

typedef struct nt_config {
    config_state_t config_state;
    MIB_KEYVAL keyval;
} nt_config_t;

/*-------------------------------------------------------------------------	 
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/
/*
 * @brief stores the data from the data structure.
 * @param control, data
 * @return This function does not return anything
 *
 */
extern void nt_config_install(MIB_KEYVAL *control, uint8_t *data);

#endif  // FERMION_CONFIG_HCF
#endif  //_VLINT_H_
