/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @brief Earbud Context holder function definitions
 *=======================================================================*/

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include "fwconfig_cmn.h"
#include "nt_flags.h"

#ifdef EB_OFFLOADS
#include "wifi_fw_eb_offloads.h"
#include "hal_int_modules.h"

#include "fermion_hw_reg.h"
#include "nt_common.h"
#include "nt_logger_api.h"
#include "ieee80211_defs.h"
#include "wifi_fw_mgmt_api.h"
/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------
 * Global Data Definitions
 * ----------------------------------------------------------------------*/
static eb_offlods_t g_eb_offload = {EB_LOCATION_MAX};
static bool g_xpan_support = FALSE;
/*------------------------------------------------------------------------
 *  Function Declarations and Definitions
 * ----------------------------------------------------------------------*/

/*
 * @brief  Recieve qcnie data from frame and call appropritate functions
 * @param  qcnie_info        : EDCA/PIFS params
 * @return e_edca_params_error_msg
 */
#ifdef FEATURE_RATE_AND_EDCA_CONFIG
e_edca_params_error_msg api_parse_eb_info(uint8_t *qcnie_info)
{
    if (qcnie_info == NULL)
        return EDCA_IE_STATUS_ERR;

    NT_LOG_WMI_INFO("api_parse_eb_info ", qcnie_info[0], qcnie_info[1], qcnie_info[2]);

    uint8_t *qcnie_frame = qcnie_info;

    if (*qcnie_frame++ != TLV_EDCA_PIFS_PARAM_ATTR_ID) {
        return EDCA_IE_STATUS_MISMATHCED_PARAM_ID;
    }
    uint8_t length = *qcnie_frame++;
    uint8_t param_type = *qcnie_frame++;
    if (param_type == EDCA_PARAM_TYPE_RBO_SEPARATION && length == EDCA_TLV_RBO_SEPARATION_LEN) {
        wlan_edca_pifs_param_ie_t edca_ie;
        edca_ie.edca_pifs_param.eparam.acvo_aifsn = (qcnie_frame[0] & (EDCA_AIFSN_MASK));
        edca_ie.edca_pifs_param.eparam.acvo_acm = (qcnie_frame[0] & (EDCA_ACM_MASK));
        edca_ie.edca_pifs_param.eparam.acvo_aci = (qcnie_frame[0] & (EDCA_ACI_MASK));
        edca_ie.edca_pifs_param.eparam.acvo_cwmin = (qcnie_frame[1] & (EDCA_CW_MIN_MASK));
        edca_ie.edca_pifs_param.eparam.acvo_cwmax = (qcnie_frame[1] & (EDCA_CW_MAX_MASK));
        qcnie_frame += 2;  // moving frame to txoplimit field
        memscpy(&edca_ie.edca_pifs_param.eparam.acvo_txoplimit, sizeof(uint16_t), qcnie_frame, sizeof(uint16_t));

        hal_mod_wmmparams_add_rbo_access(
            edca_ie.edca_pifs_param.eparam.acvo_aifsn, edca_ie.edca_pifs_param.eparam.acvo_cwmin,
            edca_ie.edca_pifs_param.eparam.acvo_cwmax, edca_ie.edca_pifs_param.eparam.acvo_txoplimit);

        NT_LOG_MLM_INFO("Aifsn, acvo_acm, acvo_aci ", edca_ie.edca_pifs_param.eparam.acvo_aifsn,
                        edca_ie.edca_pifs_param.eparam.acvo_acm, edca_ie.edca_pifs_param.eparam.acvo_aci);
        NT_LOG_MLM_INFO("cw_min, cw_max ", edca_ie.edca_pifs_param.eparam.acvo_cwmin,
                        edca_ie.edca_pifs_param.eparam.acvo_cwmax, 0);
        return EDCA_IE_STATUS_OK;
    } else if (param_type == EDCA_PARAM_TYPE_CONTENTION_FREE && length == EDCA_TLV_CONTENTION_FREE_LEN) {
        uint8_t offset;
        switch (wifi_fw_offload_get_eb_location()) {
            case EB_LEFT:
                offset = qcnie_frame[1];
                break;
            case EB_RIGHT:
                offset = qcnie_frame[2];
                break;
            default:
                (void)offset;
                NT_LOG_MLM_ERR("EB Location not recieved: ", 0, 0, 0);
                return EDCA_IE_STATUS_MISSING_LOCATION;
        }
        hal_mod_wmmparams_add_pifs(offset);
        return EDCA_IE_STATUS_OK;
    } else {
        return EDCA_IE_STATUS_MISMATHCED_PARAMS;
    }
}

#endif
/*
 * @brief  Store EB location
 * @param  wifi_fw_offload_set_eb_location : eb location
 * @return void
 */

void wifi_fw_offload_set_eb_location(uint8_t eb_location)
{
    if (eb_location >= EB_LOCATION_MAX) {
        NT_LOG_WMI_ERR("Wrong EB Location recieved: ", eb_location, 0, 0);
    } else {
        g_eb_offload.eb_location = eb_location;
        NT_LOG_WMI_ERR("EB location ", eb_location, 0, 0);
    }
}

/*
 * @brief  updates whether XPAN supported or not
 * @param  wifi_fw_offload_set_xpan_support : xpan_support
 * @return void
 */

void wifi_fw_offload_set_xpan_support(bool xpan_support)
{
    g_xpan_support = xpan_support;
    NT_LOG_WMI_ERR("Xpan support", xpan_support, 0, 0);
}

/*
 * @brief  return global value of eb location
 * @param  none
 * @return eb location
 */

uint8_t wifi_fw_offload_get_eb_location()
{
    return g_eb_offload.eb_location;
}

/*
 * @brief  return global value of xpan_enable
 * @param  none
 * @return is_xpan_supported or not
 */

bool wifi_fw_offload_get_xpan_support()
{
    return g_xpan_support;
}

#endif /*EB_OFFLOADS*/
