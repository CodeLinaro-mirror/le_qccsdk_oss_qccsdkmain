/* 
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/*
 * To enter into scan mode call the nt_hal_scan_begin.
 * nt_hal_scan_t is the structure to restore the register back up.
 * Who ever calling this function they have allocate the memory for this structure and pass the address of that
 * structure as argument to nt_hal_scan_begin Call the hal_scan_channel_switch to switch the channel. To come out of the
 * scan mode call the nt_hal_scan_end.
 */

#ifndef _HAL_INT_SCAN_H_
#define _HAL_INT_SCAN_H_

#include <stdint.h>
#include "nt_common.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"

/* types of frames that SCAN module recognizes and can send during scan begin */
#define HAL_SCAN_FRMTYPE_NO_FRAME 0
#define HAL_SCAN_FRMTYPE_DATA_NULL 1
#define HAL_SCAN_FRMTYPE_QOS_NULL 2
#define HAL_SCAN_FRMTYPE_CTS2SELF 3
#define HAL_SCAN_FRMTYPE_MAX 4

/* maximum number of channel support by during switch channel*/
#ifdef CONFIG_WIFILIB_6GHZ
#define HAL_SCAN_MAX_CHANNEL 76
#elif defined(SUPPORT_5GHZ)
#define HAL_SCAN_MAX_CHANNEL 52
#else
#define HAL_SCAN_MAX_CHANNEL 12
#endif /* CONFIG_WIFILIB_6GHZ */

/* It is not defined in ht_hw have to define here */
#define HAL_RXP_CFG_FLT_TYPE_SUBTYPE_RX_DISABLE0_MASK 0XFFFFFFFF
#define HAL_RXP_CFG_FLT_TYPE_SUBTYPE_RX_DISABLE1_MASK 0XFFFFFFFF
#define HAL_RXP_ACTION_FRAME_FILTER_OFFSET 0x34
#define HAL_RXP_BEACON_FRAME_FILTER_OFFSET 0x20
#define HAL_RXP_PROBE_RESPONSE_FRAME_FILTER_OFFSET 0x14
#define HAL_RXP_PROBE_REQUEST_FRAME_FILTER_OFFSET 0x10
#define HAL_RXP_CFG_ACTION_FLT_ENABLE_OFSET 0xD
#define HAL_RXP_CFG_BEACON_FLT_ENABLE_OFFSET 0X8
#define HAL_RXP_CFG_PROBE_FLT_ENABLE_OFSET 0x5
#define HAL_RXP_CFG_PROBE_REQUEST_FLT_ENABLE_OFSET 0x4
#define HAL_RXP_CFG_ACK_FLT_ENABLE_OFFSET 0x1D
#define HAL_RXP_SW_BD_CH_NUM_SET (16)

/* @brief to restored the register values and store back again that value to registers */
typedef struct nt_hal_scan_s {
    uint8_t r_channel; // restore channel number
    uint8_t r_phyband; // restore phyband
    uint16_t r_resv;
    uint32_t r_rxp_flt0;       // restore rxp filter disable0
    uint32_t r_rxp_flt1;       // restore rxp filter disable1
    uint32_t r_bo_mapping1;    // restore back off engine mapping1
    uint32_t r_bo_mapping2;    // restore back off engine mapping2
    uint32_t r_rxp_beacon_flt; // restore rxp beacon filter
    uint32_t r_rxp_probe_flt;  // restore rxp probe filter

} nt_hal_scan_t;

/*
 * @brief: Disable all the transmission STAID and QID except Probe response STAID and QID and send production frames
 * @param parameter1: NO_Frame - 0, Data NULL - 1 ,QOS NULL - 2, CTS to Self -3 PM bit - 1
 * @param parameter2: reg_bkp to restored the register values
 * @param parameter3: To pass the current channel number and restored
 * @returns: Frame type is not less than hal scan max frame or reg_bkup equal to NULL return NT_EPARAM otherwise return
 * NT_OK
 */
nt_status_t nt_hal_scan_begin(uint8_t frmtype, nt_hal_scan_t *reg_bkup, uint8_t chnum);

/*
 * @brief:This function is used to disable the tx and rx
 * @param : None
 * @returns: None
 */
void nt_hal_tx_rx_disable(void);

/*
 * @brief:Enable the tx and rx and set RXP to scan mode
 * @param parameter1: channel to switch
 * @returns: channel number is not less than max channel return NT_EPARAM or return NT_OK
 */
nt_status_t nt_hal_tx_rx_enable(uint8_t chnum);

/*
 * @brief:Restored the RXP mode. Send DATA_NULL to AP or CTS2SELF after finished scan and send production frames
 * @param parameter1: NO_Frame - 0, Data NULL - 1 ,QOS NULL - 2, CTS to Self -3 PM bit - 0
 * @param parameter2: reg_bkp to restored the register values
 * @returns: Frame type is not less than hal scan max frame or reg_bkup equal to NULL return NT_EPARAM otherwise return
 * NT_OK
 */
nt_status_t nt_hal_scan_end(uint8_t frmtype, nt_hal_scan_t *reg_bkup, NT_BOOL connected);

#ifdef SUPPORT_BMU_ERROR_RECOVERY
/*
 * @brief: This function is used to back-up TX/RX state and disable TX and RX before BMU recovery
 * @param : None
 * @returns: None
 */
void nt_hal_tx_rx_disable_pre_bmu_recovery(void);

/*
 * @brief: Set the TX/RX state post BMU recovery to the same state that was present before disable
 * @param chnum: channel to switch
 * @returns: None
 */
void nt_hal_tx_rx_enable_post_bmu_recovery(uint8_t chnum);
#endif /* SUPPORT_BMU_ERROR_RECOVERY */

#endif //_HAL_INT_SCAN_H_
