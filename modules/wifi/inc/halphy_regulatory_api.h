/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * @file halphy_regulatory_api.h
 * @brief API for regulatory Functions and types
 * ======================================================================*/

#ifndef _HALPHY_REGULATORY_API_H_
#define _HALPHY_REGULATORY_API_H_

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include <stdint.h>
#include <stdbool.h>
#include <halphy_regulatory.h>
#ifdef SUPPORT_REGULATORY

/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define REGULATORY_CHAN_VALID       0
#define REGULATORY_CHAN_DISABLED    (1 << 0)
#define REGULATORY_CHAN_NO_IR       (1 << 1)
#define REGULATORY_CHAN_RADAR       (1 << 3)
#define REGULATORY_CHAN_NO_OFDM     (1 << 6)
#define REGULATORY_CHAN_INDOOR_ONLY (1 << 9)

#define REGULATORY_CHAN_NO_20MHZ (1 << 11)
#define REGULATORY_CHAN_NO_10MHZ (1 << 12)
#define REGULATORY_CHAN_AFC      (1 << 13)

#define REGULATORY_PHYMODE_NO11A  (1 << 0)
#define REGULATORY_PHYMODE_NO11B  (1 << 1)
#define REGULATORY_PHYMODE_NO11G  (1 << 2)
#define REGULATORY_CHAN_NO11N     (1 << 3)
#define REGULATORY_PHYMODE_NO11AC (1 << 4)
#define REGULATORY_PHYMODE_NO11AX (1 << 5)
#define REGULATORY_PHYMODE_NO11BE (1 << 6)

//#define IS_REG_CHANNEL_ACTIVE_SCAN(flag) (!((flag) & ( REGULATORY_CHAN_DISABLED | REGULATORY_CHAN_NO_IR |
//REGULATORY_CHAN_INDOOR_ONLY)))

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
typedef struct {
    uint16_t start_freq;
    uint16_t end_freq;
    uint8_t reg_power;
    uint8_t ant_gain;
    uint16_t flag_info;
    uint32_t psd_power_info; /* bits 0     - whether PSD power,
                              bits 15:1  - reserved
                              bits 31:16 - maximum PSD EIRP (dBm/MHz) */
    uint16_t max_bw;         /* Maximum BW */
} phyrf_reg_rule_struct;

typedef struct {
    uint32_t status_code; /* WMI_REG_SET_CC_STATUS_CODE */
    uint8_t alpha[3];
    uint8_t ctl_value[2];                          // Idx 0 for 2G, Idx 1 for 5G
    uint8_t ctl_6g_value[3 * WMI_REG_CLIENT_MAX];  // SP,VLP,LPI for STA and SAP
    uint16_t country_id;
    uint32_t domain_code;
    uint8_t countrycode_type;
    uint8_t num_2g_reg_rules;
    uint8_t num_5g_reg_rules;
    uint8_t domain_code_6g_super_id;
    uint8_t num_6g_reg_rules_sp[WMI_REG_CLIENT_MAX];
    uint8_t num_6g_reg_rules_lpi[WMI_REG_CLIENT_MAX];
    uint8_t num_6g_reg_rules_vlp[WMI_REG_CLIENT_MAX];
    /* followed by wmi_regulatory_rule_ext struct TLV array. First 2G, then 5G, then 6G */
} phyrf_reg_exchange_struct;

typedef enum {
    WMI_COUNTRYCODE_ALPHA2,
    WMI_COUNTRYCODE_COUNTRY_ID,
    WMI_COUNTRYCODE_DOMAIN_CODE,
} WMI_COUNTRYCODE_TYPE;

typedef enum {
    WMI_REG_SET_CC_STATUS_PASS = 0,
    WMI_REG_CURRENT_ALPHA2_NOT_FOUND = 1,
    WMI_REG_INIT_ALPHA2_NOT_FOUND = 2,
    WMI_REG_SET_CC_CHANGE_NOT_ALLOWED = 3,
    WMI_REG_SET_CC_STATUS_NO_MEMORY = 4,
    WMI_REG_SET_CC_STATUS_FAIL = 5,
} SET_CC_STATUS_CODE;

// CTL values from the regulatory database
typedef enum regulatory_ctl {
    CTL_11B = 0x01,
    CTL_11G = 0x02,
    FCC = 0x10,
    ETSI = 0x30,
    MKK = 0x40,
    CTL_KOR = 0x50,
    CTL_CHN = 0x60,
    CTL_USER_DEF = 0x70,
    APL = 0X80,
    NO_CTL = 0xFF,
} regulatory_ctl_t;

/*
typedef struct {
    uint32_t  freq_info;        bits 15:0  = u16 start_freq,
                                bits 31:16 = u16 end_freq
                                (both in MHz units)
                                use same MACRO as wmi_regulatory_rule_struct
    uint32_t  bw_pwr_info;      bits 15:0  = u16 max_bw (MHz units),
                                bits 23:16 = u8 reg_power (dBm units),
                                bits 31:24 = u8 ant_gain (dB units)
                                use same MACRO as wmi_regulatory_rule_struct
    uint32_t  flag_info;        bits 15:0  = u16 flags,
                                bits 31:16 reserved
                                use same MACRO as wmi_regulatory_rule_struct
    uint32_t  psd_power_info;   bits 0     - whether PSD power,
                                bits 15:1  - reserved
                                bits 31:16 - maximum PSD EIRP (dBm/MHz)
} phyrf_reg_rule_struct;
*/

/*------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/
uint8_t halphy_get_regdb_version(void);
uint8_t ctl_val_from_bdf_cc(uint16_t chan);
uint8_t halphy_get_regdb_sub_version(void);
uint8_t halphy_get_regdb_customer_version(void);
bool halphy_regdb_init(uint8_t *p_regdb_data);
uint16_t halphy_regulatory_IsCountryCodeValid_internal(uint8_t *alpha);
uint8_t halphy_get_regdb_customer_version(void);
void halphy_regulatory_GetDefaultCountryCodeNewRegRules(phyrf_reg_exchange_struct *p_reg_info, void *reg_rules);
void halphy_regulatory_GetCountryCodeNewRegRules(uint8_t *alpha, phyrf_reg_exchange_struct *p_reg_info,
                                                 void *reg_rules);
uint16_t halphy_regulatory_get_country_id(void);

#endif /* SUPPORT_REGULATORY */
#endif /* _HALPHY_REGULATORY_API_H_ */
