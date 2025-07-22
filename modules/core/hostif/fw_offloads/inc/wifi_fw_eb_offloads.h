/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*========================================================================
 * @file wifi_fw_eb_offload_api.h
 * @brief Earbud context Holder param and struct definitions
 * ======================================================================*/
#ifndef WIFI_FW_EB_OFFLOADS_H
#define WIFI_FW_EB_OFFLOADS_H
/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include "wlan_mlme.h"

#ifdef EB_OFFLOADS
#include "wifi_fw_mgmt_api.h"

/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
/*Value set in QCN_IE frame for distinguishing channel access method */
#define EDCA_PARAM_TYPE_RBO_SEPARATION  0
#define EDCA_PARAM_TYPE_CONTENTION_FREE 1

#define EDCA_TLV_RBO_SEPARATION_LEN  5
#define EDCA_TLV_CONTENTION_FREE_LEN 4

#define EDCA_AIFSN_MASK  0x0f
#define EDCA_ACM_MASK    0x10
#define EDCA_ACI_MASK    0x60
#define EDCA_CW_MIN_MASK 0x0f
#define EDCA_CW_MAX_MASK 0xf0

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/

/*Global structure to store offloaded data*/
typedef struct eb_offlods {
    uint8_t eb_location;
} eb_offlods_t;

/*Arguments sent as part of qcnie for EDCA_PARAM_TYPE_RBO_SEPARATION case*/
typedef struct edca_param {
    uint8_t acvo_aifsn;  // bit 0:4
    uint8_t acvo_acm;    // bit 5
    uint8_t acvo_aci;    // bit 6
    // uint8_t unused;            //bit 7
    uint8_t acvo_cwmin;       // bit 8:11
    uint8_t acvo_cwmax;       // bit 12:15
    uint16_t acvo_txoplimit;  // bit 16:31
} edca_param_t;

/*Arguments sent as part of qcnie for EDCA_PARAM_TYPE_CONTENTION_FREE case*/
typedef struct pifs_param {
    uint8_t sap_pifs_offset;  // bit 0:7
    uint8_t leb_pifs_offset;  // bit 8:15
    uint8_t reb_pifs_offset;  // bit 16:23
} pifs_param_t;

/*Structure holding data sent through QCN_IE*/
typedef struct wlan_edca_pifs_param_ie {
    uint8_t edca_param_type;
    union edca_pifs_param {
        edca_param_t eparam; /* edca_param_type = 0 */
        pifs_param_t pparam; /* edca_param_type = 1 */
    } edca_pifs_param;
} wlan_edca_pifs_param_ie_t;

/*Enum indicating type of error while parsing EDCA info from QCN_IE*/
typedef enum {
    EDCA_IE_STATUS_MISMATHCED_PARAMS,
    EDCA_IE_STATUS_MISMATHCED_PARAM_ID,
    EDCA_IE_STATUS_MISSING_LOCATION,
    EDCA_IE_STATUS_OK,
    EDCA_IE_STATUS_ERR,
} e_edca_params_error_msg;

/*------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/

/*
 * @brief  Store EB location
 * @param  wifi_fw_offload_set_eb_location : eb location
 * @return void
 */
void wifi_fw_offload_set_eb_location(uint8_t eb_location);
/*
 * @brief  updates whether XPAN supported or not
 * @param  wifi_fw_offload_set_xpan_support : xpan_support
 * @return void
 */

void wifi_fw_offload_set_xpan_support(bool xpan_support);
/*
 * @brief  return global value of eb location
 * @param  none
 * @return eb location
 */
uint8_t wifi_fw_offload_get_eb_location();
/*
 * @brief  return global value of xpan_enable
 * @param  none
 * @return is_xpan_supported or not
 */
bool wifi_fw_offload_get_xpan_support();

#ifdef FEATURE_RATE_AND_EDCA_CONFIG
/*
 * @brief  Recieve qcnie data from frame and call appropritate functions
 * @param  qcnie_info        : EDCA/PIFS params
 * @return e_edca_params_error_msg
 */
e_edca_params_error_msg api_parse_eb_info(uint8_t *qcnie_info);
#endif

#endif /*WIFI_FW_EB_OFFLOADS_H*/
#endif /*EB_OFFLOADS*/
