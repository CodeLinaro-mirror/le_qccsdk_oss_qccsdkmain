/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __WLAN_DEFS_H__
#define __WLAN_DEFS_H__

#include "if_ethersubr.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
/*
 * This file contains WLAN definitions that may be used across both
 * Host and Target software.
 */

typedef enum {
    MODE_11B = 0,       /* 11b Mode */
    MODE_11G = 1,       /* 11b/g Mode */
    MODE_11NG_HT20 = 2, /* 11b/g/n HT20 Mode */
#ifdef SUPPORT_5GHZ
    MODE_11A_ONLY = 3,    /* 11A only mode (no HT)*/
    MODE_11A_HT20 = 4,    /* 5GHZ mode with HT20*/
    MODE_11ABGN_HT20 = 5, /* 5GHZ mode with CCK rates*/
#endif
    MODE_UNKNOWN,
    MODE_MAX = MODE_UNKNOWN,
} WLAN_PHY_MODE;

#define IS_MODE_11B(mode)  ((mode) == MODE_11B)
#define IS_MODE_11G(mode)  ((mode) == MODE_11G)
#define IS_MODE_11GN(mode) ((mode) == MODE_11NG_HT20)
#ifdef SUPPORT_5GHZ
#define IS_MODE_11ABGN(mode) ((mode) == MODE_11ABGN_HT20)
#endif
#define IS_11G_RATE(rs) \
    (((rs & 0x7F) == 12) || ((rs & 0x7F) == 18) || ((rs & 0x7F) == 24) || ((rs & 0x7F) == 36) || ((rs & 0x7F) == 48))

#if (defined SUPPORT_5GHZ) && (defined PLATFORM_FERMION)
#define DEFAULT_PHYMODE MODE_11A_HT20
#define DEFAULT_FREQ    5180
#else
#define DEFAULT_PHYMODE MODE_11NG_HT20
#define DEFAULT_FREQ    2422
#endif

#ifdef CONFIG_WIFILIB_6GHZ
#define TOT_6GHZ_CHANNELS 24
#else
#define TOT_6GHZ_CHANNELS 0
#endif /* CONFIG_WIFILIB_6GHZ */

#ifdef SUPPORT_5GHZ
#define TOT_5GHZ_CHANNELS ((33) + TOT_6GHZ_CHANNELS)
#else
#define TOT_5GHZ_CHANNELS 0
#endif /* SUPPORT_5GHZ */

#define TOT_2GHZ_CHANNELS 11

#define TOT_2GHZ_MAX_CHANNEL_INDEX (TOT_2GHZ_CHANNELS - (1))
#define TOT_5GHZ_MAX_CHANNEL_INDEX ((TOT_5GHZ_CHANNELS + TOT_2GHZ_CHANNELS) - 1)

#ifdef SUPPORT_5GHZ
#define TOT_MAX_CHANNEL_INDEX TOT_5GHZ_MAX_CHANNEL_INDEX
#else
#define TOT_MAX_CHANNEL_INDEX TOT_2GHZ_MAX_CHANNEL_INDEX
#endif /* SUPPORT_5GHZ */

#ifdef SUPPORT_5GHZ
#define IS_MODE_11G(mode)         ((mode) == MODE_11G)
#define IS_MODE_11A_ONLY(mode)    ((mode) == MODE_11A_ONLY)
#define IS_MODE_11A_HT20(mode)    ((mode) == MODE_11A_HT20)
#define IS_MODE_11ABGN_HT20(mode) ((mode) == MODE_11ABGN_HT20)

#define PHYMODE_IS_5G(mode) (mode == MODE_11A_ONLY || mode == MODE_11A_HT20 || mode == MODE_11ABGN_HT20)
#define PHYMODE_IS_2G(mode) (mode == MODE_11B || mode == MODE_11G || mode == MODE_11NG_HT20 || mode == MODE_11ABGN_HT20)
#endif /* SUPPORT_5GHZ */

#endif /* __WLANDEFS_H__ */
