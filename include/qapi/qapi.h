/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __QAPI_H__ // [
#define __QAPI_H__

/**
 * @file qapi.h
 *
 * @brief QAPI interface definition
 *
 * @details This file provides the base type definitions used by the QAPI.
 *          This includes the basic integer types (based on stdint.h and
 *          stddef.h) and a basic boolean type.
 */

/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include "qapi_types.h"
#include "qapi_version.h"
#include "qapi_status.h"

#if CONFIG_QCCSDK_CONSOLE // [
  #include "qapi_console.h"
#endif // ] if CONFIG_QCCSDK_CONSOLE

#ifdef I2C_QAPI // [
  #include "qapi_i2c.h"
#endif // ] if I2C_QAPI

#if CONFIG_WLAN
    #include "qapi_wlan.h"
#endif

/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/

/* Function Definitions to be added later */

#endif // ] #ifndef __QAPI_H__
