/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @file fwconfig_cmn.h
 * @brief feature flag definitions of NT code base required for Fermion
 * ======================================================================*/
#ifndef _FWCONFIG_CMN_H_
#define _FWCONFIG_CMN_H_
/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
/* None*/
#include "autoconf.h"
#if (FERMION_CHIP_VERSION == 2)
#include "fwconfig_QCP7321.h"
#else
#include "fwconfig_QCP5321.h"
#endif

#endif
