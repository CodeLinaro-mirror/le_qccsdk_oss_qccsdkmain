/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * @file halphy_regulatory.h
 * @brief Function header and defines for Regulatory data base
 * ======================================================================*/

#ifndef _HALPHY_REGULATORY_H_
#define _HALPHY_REGULATORY_H_

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include <stdint.h>
#include <stdbool.h>
#include "halphy_regulatory_db_struct.h"
#ifdef SUPPORT_REGULATORY
/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define INVALID_REG_DOMAIN   0xFF
#define INVALID_SUPER_DOMAIN 0xFF
#define INVALID_COUNTRY_CODE 0xFFFF
#define INVALID_6G_SUBDOMAIN 0xFFFF
#define US_COUNTRY_ID        840
#define CHINA_COUNTRY_ID     156
#define WORLD_MODE           96
#define WMI_REG_CLIENT_MAX   2

#define RD_FCC_AP  0xE8
#define RD_Korea   0xF4
#define RD_Brazil  0xF3
#define RD_UK      0xF2
#define RD_EU_6G   0xF1
#define RD_FCC_STA 0xEA

#define FCC_RD_6G    0x10016
#define Korea_RD_6G  0x40048
#define BRAZIL_RD_6G 0x700ED
#define UK_RD_6G     0x30037
#define EU_RD_6G     0x20000

#define NUM_6G_POWER_MODES 3
/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/

typedef struct regulatory_rule_s {
    uint16_t start_freq; /* Start Frequency */
    uint16_t end_freq;   /* End Frequency */
    uint16_t max_bw;     /* Maximum BW */
    uint16_t flags;      /* Channel properties */
    uint8_t reg_power;   /* Regulatory power */
    uint8_t ant_gain;
    uint16_t rsvd2;
} regulatory_rule_t;

typedef struct regulatory_rule_s_6g {
    uint16_t start_freq; /* Start Frequency */
    uint16_t end_freq;   /* End Frequency */
    uint16_t max_bw;     /* Maximum BW */
    uint16_t flags;
    uint8_t psd_power;
    uint8_t tx_power_eirp;
    uint8_t max_psd_eirp;
    uint8_t ant_gain; /* Channel properties */
} regulatory_rule_6g_t;

typedef struct phyrf_operating_ctl_domain_s {
    uint8_t ctl_2g;
    uint8_t ctl_5g;
    uint8_t ctl_6g[NUM_6G_POWER_MODES];
} phyrf_operating_ctl_domain_t;

#endif /* SUPPORT_REGULATORY */
#endif /* _HALPHY_REGULATORY_H_ */
