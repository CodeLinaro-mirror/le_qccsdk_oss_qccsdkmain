/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * @brief Redb structure
 *=======================================================================*/

#ifndef _HALPHY_REGDB_STRUCT_H_
#define _HALPHY_REGDB_STRUCT_H_

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#ifdef SUPPORT_REGULATORY
#include <stdint.h>
/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/

#define MAX_REG_RULES           10
#define REG_ALPHA2_LEN          2
#define NUM_MAX_6G_CLIENT_TYPES 2

/**
 * enum dfs_region - DFS region
 * @DFS_UNINIT_REGION: un-initialized region
 * @DFS_FCC_REGION: FCC region
 * @DFS_ETSI_REGION: ETSI region
 * @DFS_MKK_REGION: MKK region
 * @DFS_CN_REGION: China region
 * @DFS_KR_REGION: Korea region
 * @DFS_UNDEF_REGION: Undefined region
 */

typedef enum {
    DFS_UNINIT_REG = 0,
    DFS_FCC_REG = 1,
    DFS_ETSI_REG = 2,
    DFS_MKK_REG = 3,
    DFS_CN_REG = 4,
    DFS_KR_REG = 5,
    DFS_MKKN_REG = 6,
    DFS_UNDEF_REG = 0xFFFF,
} DFS_REG;

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
/*
NOTE: DO NOT EDIT THE FOLLOWING STRUCTURES. THEY ARE MATCHED TO BDF STRUCTURES.
      IF THEY NEED EDITING, PLEASE EDIT REGULATORY STRUCTURES IN "bdf_template.h" FILE AS WELL.
*/

typedef struct {
    uint16_t start_freq;  // Start Frequency
    uint16_t end_freq;    // End Frequency
    uint16_t max_bw;      // maximum BW
    uint16_t flags;
    uint8_t reg_power;  // regulatory power
    uint8_t ctl_val;    // sub band CTL
    uint8_t pad[2];     // ADDED TO MATCH BDF STRUCTURE
} REGULATORY_RULE;      // Regulatory rule's structure for 2G and 5G

typedef struct {
    uint16_t start_freq;  // Start Frequency
    uint16_t end_freq;    // End Frequency
    uint16_t max_bw;      // maximum BW
    uint16_t flags;
    uint8_t psd_power;  // For tx power Total Tx power = psd(in db)+ 10log(BW)
    uint8_t tx_power_eirp;
    uint8_t max_psd_eirp;
    uint8_t pad;
} REGULATORY_RULE_6G;  // Regulatory rule's structure

typedef struct {
    uint8_t ctl_val;        // Conformance Test Limit Value
    uint8_t min_bw;         // minimum BW
    uint8_t ant_gain;       // antenna gain
    uint8_t num_reg_rules;  // number of regulatory rules
    uint16_t max_bw;
    uint8_t reg_rule_id[MAX_REG_RULES];  // array of reg-rules indices
} REG_DOMAIN_6G;

typedef struct {
    uint8_t super_dmn_id;
    uint8_t dmn_id_6g_ap_sp;
    uint8_t dmn_id_6g_ap_lpi;
    uint8_t dmn_id_6g_ap_vlp;
    uint8_t dmn_id_6g_client_sp[NUM_MAX_6G_CLIENT_TYPES];
    uint8_t dmn_id_6g_client_lpi[NUM_MAX_6G_CLIENT_TYPES];
    uint8_t dmn_id_6g_client_vlp[NUM_MAX_6G_CLIENT_TYPES];
    uint8_t pad[2];
} SUPER_DOMAIN_SUBSET;

typedef struct {
    uint16_t dmn_6g_id_internal;
    uint16_t dmn_6g_id_code;
} SUBDOMAIN_6G_MAP;

typedef struct {
    uint8_t ctl_val;                     // Conformance Test Limit Value
    uint8_t dfs_region;                  // dfs region
    uint8_t min_bw;                      // minimum BW
    uint8_t ant_gain;                    // antenna gain
    uint8_t num_reg_rules;               // number of regulatory rules
    uint8_t pad;                         // ADDED TO MATCH BDF STRUCTURE
    uint8_t reg_rule_id[MAX_REG_RULES];  // array of reg-rules indices
} REG_DOMAIN;                            // Regulatory domain values common for 2g and 5g

typedef struct {
    uint16_t country_code;     // first key
    uint16_t reg_dmn_pair_id;  // reg domain pair value
    uint8_t super_dmn_6g_id;
    uint8_t alpha2[REG_ALPHA2_LEN + 1];      // second key
    uint8_t alpha2_11d[REG_ALPHA2_LEN + 1];  // may not be unique
    uint8_t max_bw_2g;                       // maximum 2G BW
    uint8_t max_bw_5g;                       // maximum 5G BW
    uint8_t max_bw_6g;
    uint8_t phymode_bitmap;    // phy modes not available: 11a/11b/11g/11n/11ac/11ax
    uint8_t pad;               // ADDED TO MATCH BDF STRUCTURE
} COUNTRY_CODE_TO_REG_DOMAIN;  // Main lookup table for country code and reg domain

typedef struct {
    uint16_t reg_dmn_pair_id;  // Regulatory domain pair value
    uint8_t dmn_id_5g;         // 5g Reg domain Ex: FCC1, ETSI1, APL2 etc.
    uint8_t dmn_id_2g;         // 2g Reg domain Ex: FCCA, WORLD, MKKA etc.
} REG_DOMAIN_PAIR;             // Common Reg domain for respective 2g and 5g reg domains

// REGULATORY DATABASE DEFINITIONS (FOR SPLIT BDF)
#define NUM_REG_COUNTRIES        240
#define NUM_REG_DOMAIN_PAIRS     200
#define NUM_REG_RULES_2G         20
#define NUM_REG_RULES_5G         150
#define NUM_REG_DOMAINS_2G       25
#define NUM_REG_DOMAINS_5G       100
#define NUM_REG_RULES_6G         150
#define NUM_REG_DOMAINS_6G       80
#define NUM_SUBDOMAIN_6G_MAP     150
#define NUM_SUPER_DOMAIN_SUBSETS 50
#define REGULATORY_DB_FUTURE     1072

typedef struct regDbBdf {
    uint16_t checksum;                                                   // 2B
    COUNTRY_CODE_TO_REG_DOMAIN regDbAllCountries[NUM_REG_COUNTRIES];     // 12*240 = 2880B
    REG_DOMAIN_PAIR regDbRegDmnPairs[NUM_REG_DOMAIN_PAIRS];              // 4*200  = 800B
    REGULATORY_RULE regDbRegRule2g[NUM_REG_RULES_2G];                    // 12*20  = 240B
    REGULATORY_RULE regDbRegRule5g[NUM_REG_RULES_5G];                    // 12*150 = 1800B
    REGULATORY_RULE_6G regDbRegRule6g[NUM_REG_RULES_6G];                 // 12*150 = 1800B
    REG_DOMAIN regDbRegDomains2g[NUM_REG_DOMAINS_2G];                    // 16*25  = 150B
    REG_DOMAIN regDbRegDomains5g[NUM_REG_DOMAINS_5G];                    // 16*100 = 600B
    REG_DOMAIN_6G regDbRegDomains6g[NUM_REG_DOMAINS_6G];                 // 16*80  = 1280B
    SUPER_DOMAIN_SUBSET regDbSuperDmnSubsets[NUM_SUPER_DOMAIN_SUBSETS];  // 9*50   = 450B
    SUBDOMAIN_6G_MAP regDbSubDomain6gMap[NUM_SUBDOMAIN_6G_MAP];          // 4*150  = 600B
    uint8_t regDbEnable;                                                 // 1B
    uint8_t regDbVersion;                                                // 1B
    uint8_t regDbSubVer;                                                 // 1B
    uint8_t regDbCustomVer;                                              // 1B
    uint8_t regulatorytDbFuture[REGULATORY_DB_FUTURE - 4];               // 1074B
} REGDB_STRUCT;

#endif /* SUPPORT_REGULATORY */
#endif /* _HALPHY_REGDB_STRUCT_H_ */
