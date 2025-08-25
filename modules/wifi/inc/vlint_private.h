/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @file nt_vlint_private.h
 * @brief macros related to vlint unpacking is stored here.
 *========================================================================*/

#ifndef VLINT_PRIVATE_H
#define VLINT_PRIVATE_H

/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#ifdef FERMION_CONFIG_HCF
#include <limits.h>
#include "vlint.h"
#include <stdint.h>
#include <string.h>
#include "nt_logger_api.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "nt_osal.h"
#include <stdio.h>
#include "assert.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/

/* B-103266 Enable modified VLINT definition in CS-213274-SP */
#define VL_DATA_USE_SIGN_BIT 1 /* Use sign bit in first octet to reduce the storage requirements for uints */

/* The VLINT format bitmask and (unshifted) values. */
#define VL_FORMAT_MASK  (0x80)
#define VL_FORMAT_OCTET (0x00) /* Single octet */
#define VL_FORMAT_VAR   (0x80) /* 2 or more octets */

#define VL_SIGN_MASK (0x40)

#define VL_TYPE_MASK         (0x20)
#define VL_TYPE_OCTET_STRING (0x20)
#define VL_TYPE_INTEGER      (0x00)

/** Data mask for VL_FORMAT_OCTET format. */
#define VL_FORMAT_OCTET_DATA_MASK (0x7f)

/* Data length mask for VL_FORMAT_VAR format. */
#define VL_FORMAT_VAR_DATALEN_MASK (0x1f)

#endif /* FERMION_CONFIG_HCF */
#endif /* VLINT_PRIVATE_H */
