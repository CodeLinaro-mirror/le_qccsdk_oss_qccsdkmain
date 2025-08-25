/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================

* @file fwconfig_wlan.h
* @brief feature flag definitions of NT code base required for Fermion
* ======================================================================*/
#ifndef _FWCONFIG_WLAN_H_
#define _FWCONFIG_WLAN_H_
/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
/* None*/
//#include "autoconf.h"
#ifndef CONFIG_SOC_QCC730V2
#define CONFIG_SOC_QCC730V2 1
#endif
#ifndef CONFIG_WMI_EVENT
#define CONFIG_WMI_EVENT 1  // wifi-mix
#endif
#ifndef CONFIG_6G_BAND
#define CONFIG_6G_BAND 1  // wifi-only
#endif
#if (FERMION_CHIP_VERSION == 2)
#include "fwconfig_QCP7321.h"
#else
#include "fwconfig_QCP5321.h"
#endif

#endif
