/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __ECSA_H__
#define __ECSA_H__

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "wlan_mlme.h"
#if defined(FEATURE_STA_ECSA) || defined(FEATURE_AP_ECSA)

#define ECSA_TSF_OFFSET                0
#define CS_MODE_CONTINUE_TRAFFIC       0
#define CS_MODE_STOP_TRAFFIC           1
#define VENDOR_IE_TARGET_TSF_OFFSET    6
#define START_6G_HE_OPERATING_CLASS    131
#define START_6G_HE160_OPERATING_CLASS 134

typedef enum {
    ECSA_STOP,            // 0 ECSA request has not been recieved or has been fulfilled
    ECSA_PENDING,         // 1 ECSA request is getting processed and channel change isnt complete
    ECSA_START,           // 2 ECSA request is recieved
    ECSA_PENDING_TWT,     // 3 ECSA request when twt is active
    ECSA_PENDING_PTSM,    // 4 ECSA request when periodic traffic is active
    ECSA_CHANNEL_CHANGE,  // 5 ECSA channel change
} ecsa_state_t;

/*===============================================================
    * This enum is used to handle DFS channel specific ECSA requirement *
    *
    * in DFS channel post chan switch we should wait for first beacon then enable TX.
    * some AP send disassoc so post disconnect reset the CSA flags.
    * some AP does CAC so we hot BMISS counter so disconnect and reset the CSA flags.
=================================================================*/
typedef enum {
    ECSA_NON_DFS_CH,
    ECSA_DFS_CH,
    ECSA_DFS_CS_COMP,
    ECSA_DFS_ENABLE_TX,
} ecsa_dfs_cs_t;

/*==========================================================================
   * FUNCTION:  ecsa_init(devh_t *dev)
   *
   * DESCRIPTION:
   * This function initialize the ECSA context member of device
   * PARAMETERS:
   * 1. devh_t *dev - device pointer
============================================================================*/
void ecsa_init(devh_t *dev);

/*==========================================================================
   * FUNCTION:  ecsa_deinit(devh_t *dev)
   *
   * DESCRIPTION:
   * This function de-initialize the ECSA context member of device
   * PARAMETERS:
   * 1. devh_t *dev - device pointer
============================================================================*/
void ecsa_deinit(devh_t *dev);

/*==========================================================================
   * FUNCTION:  is_channel_valid(devh_t* dev, uint16_t freq)
   *
   * DESCRIPTION:
   * This function check the channel frequency is support by device
   * PARAMETERS:
   * 1. devh_t *dev - device pointer
   * 2. uint16_t freq - channel frequency
   * RETURN VALUE:
   * NT_BOOL TRUE or FALSE
============================================================================*/

NT_BOOL is_channel_valid(devh_t *dev, uint16_t freq);

/*==========================================================================
   * FUNCTION:  ecsa_active_with_blocking_traffic()
   *
   * DESCRIPTION:
   * Function returns true if ecsa is active and traffic is blocked
   * returns false if ecsa is not active, or if its active with traffic not blocked
   * PARAMETERS:
   * void
   * RETURN VALUE:
   * Bool TRUE or FALSE

============================================================================*/
NT_BOOL is_ecsa_active_with_blocking_traffic();

/*==========================================================================
   * FUNCTION:  is_ecsa_state_start()
   *
   * DESCRIPTION:
   *  This function checks if ecsa is started.
   * PARAMETERS:
   * 1. dev - device pointer
   * RETURN VALUE:
   * NT_BOOL TRUE or FALSE

============================================================================*/

NT_BOOL is_ecsa_state_start(devh_t *dev);

/*==========================================================================
   * FUNCTION:  ecsa_channel_change()
   *
   * DESCRIPTION:
   * changes channel according to ecsa_ctx.
   * Stops wakelock if it was set
   * Restart dpm traffic
   * PARAMETERS:
   * 1. dev - device pointer

============================================================================*/

void ecsa_channel_change(devh_t *dev);

/*==========================================================================
   * FUNCTION:  ecsa_data_available(uint8_t __unused status)
   *
   * DESCRIPTION:
   * This API is a callback function for nt_dpm_stop_handler() function
   * PARAMETERS:
   * 1. status - not used in this api
============================================================================*/

void ecsa_data_available(uint8_t __unused status);

/*==========================================================================
   * FUNCTION:  ecsa_data_stop_start_cb(uint8_t __unused status)
   *
   * DESCRIPTION:
   * This API is a callback function for nt_dpm_stop_handler() function,
   * It is called when dpm is stopped or restarted again
   * PARAMETERS:
   * 1. status - not used in this api
============================================================================*/

void ecsa_data_stop_start_cb(uint8_t __unused status);

/*==========================================================================
   * FUNCTION:  ecsa_state_stop(devh_t *dev)
   *
   * DESCRIPTION:
   * This function resets the ECSA state and restarts data_path and resets wakelock
   * It is called when dpm is stopped or restarted again
   * PARAMETERS:
   * 1. devh_t *dev - device pointer
============================================================================*/

void ecsa_state_stop(devh_t *dev);

/*==========================================================================
   * FUNCTION:  ecsa_send_completed_event()
   *
   * DESCRIPTION:
   * sends back a complete event once channel switch process is completed
   * PARAMETERS:
   * 1. devh_t *dev - device pointer
   * RETURN VALUE:
   * void

============================================================================*/

void ecsa_send_completed_event(devh_t *dev, uint16_t freq);

#if defined(FEATURE_STA_ECSA)

/*==========================================================================
   * FUNCTION:  ecsa_sta_timer_cb()
   *
   * DESCRIPTION:
   * This function is a callback after target tsf timer runs out.
   * It is invoked in non TWT case.
   * It calls function to change channel

============================================================================*/

void ecsa_sta_timer_cb();

/*==========================================================================
   * FUNCTION:  nt_recv_ecsa_ie()
   *
   * DESCRIPTION:
   * This api parses the data recieved in ECSA frame and stores it in ecsa_ctx
   * Depending on whether STA is in TWT mode or non TWT mode, changes channel
   * PARAMETERS:
   * 1. dev - device pointer
   * 2. ecsa_ie - frame pointer
   * RETURN VALUE:
   * MLME_SM_STATUS - MLME_SM_OK or MLME_SM_ERR

============================================================================*/

MLME_SM_STATUS nt_recv_ecsa_ie(devh_t *dev, uint8_t *ecsa_ie);

/*==========================================================================
   * FUNCTION:  nt_recv_csa_ie()
   *
   * DESCRIPTION:
   * This api parses the data recieved in CSA frame and stores it in ecsa_ctx
   * Depending on whether STA is in TWT mode or non TWT mode, changes channel
   * PARAMETERS:
   * 1. dev - device pointer
   * 2. csa_ie - frame pointer
   * RETURN VALUE:
   * MLME_SM_STATUS - MLME_SM_OK or MLME_SM_ERR
============================================================================*/

MLME_SM_STATUS nt_recv_csa_ie(devh_t *dev, uint8_t *csa_ie);

/*==========================================================================
   * FUNCTION:  ecsa_sta_state_start()
   *
   * DESCRIPTION:
   * This api parses the data recieved in ECSA frame and stores it in ecsa_ctx
   * Depending on whether STA is in TWT mode or non TWT mode, changes channel
   * PARAMETERS:
   * 1. dev - device pointer
   * RETURN VALUE:
   * MLME_SM_STATUS - MLME_SM_OK or MLME_SM_ERR

============================================================================*/

MLME_SM_STATUS ecsa_sta_state_start(devh_t *dev, uint32_t period);

/*==========================================================================
   * FUNCTION:  ecsa_sp_check()
   *
   * DESCRIPTION:
   * Check if current sp is where channel change is expected to happen
   * Channel is changed if current sp = expected sp
   * No change if ECSA is not pending or some later SP has reaches target tsf
   * PARAMETERS:
   * 1. dev - device pointer
   * 2. next tbtt
   * RETURN VALUE:
   * void

============================================================================*/

void ecsa_sp_check(devh_t *dev, uint64_t next_tbtt);

#endif  // FEATURE_STA_ECSA
#if defined(FEATURE_AP_ECSA)
/*==========================================================================
   * FUNCTION:  ecsa_set_type(int type)
   *
   * DESCRIPTION:
   * set the ECSA type to CSA or ECSA in AP mode
   * PARAMETERS:
   * 1. int type - 0 CSA default, 1 ECSA
   * RETURN VALUE:
   * void
============================================================================*/
void ecsa_set_type(int type);

/*==========================================================================
   * FUNCTION:  ecsa_ap_chan_switch(uint8_t mode,uint8_t count,uint8_t ch_no,uint8_t is_6g)
   *
   * DESCRIPTION:
   * trigger a channel switch with CSA/ECSA element in AP mode
   * PARAMETERS:
   * 1. uint8_t mode - channel switch mode
   * 2. uint8_t count - channel switch count
   * 3. uint8_t ch_no - new channel number
   * 4. is_6g - 0 not 6G, else 6G
   * RETURN VALUE:
   * uint8_t - 0 channel switch success, else fail
============================================================================*/
uint8_t ecsa_ap_chan_switch(uint8_t mode, uint8_t count, uint8_t ch_no, uint8_t is_6g);

/*==========================================================================
   * FUNCTION:  ieee80211_add_ecsa_ie(uint8_t *frm, ecsa_ctx_t *ecsa_ctx)
   *
   * DESCRIPTION:
   * add CSA/ECSA element in beacon frame in AP mode
   * PARAMETERS:
   * 1. uint8_t *frm - pointer to frame buffer
   * 2. ecsa_ctx_t *ecsa_ctx - pointer to structure containing ecsa context
   * RETURN VALUE:
   * uint8_t * - pointer to new frame buffer
============================================================================*/
uint8_t *ieee80211_add_ecsa_ie(uint8_t *frm, ecsa_ctx_t *ecsa_ctx);

/*==========================================================================
   * FUNCTION:  ieee80211_add_ecsa_ie(uint8_t *frm, ecsa_ctx_t *ecsa_ctx)
   *
   * DESCRIPTION:
   * add csa/ecsa element in action frame in AP mode
   * PARAMETERS:
   * 1. uint8_t *frm - pointer to frame buffer
   * 2. ecsa_ctx_t *ecsa_ctx - pointer to structure containing ecsa context
   * RETURN VALUE:
   * uint8_t * - length of frame
============================================================================*/
uint16_t ieee80211_add_ecsa_action(uint8_t *frm, ecsa_ctx_t *ecsa_ctx, uint8_t type);
#endif

#endif
#endif  // __ECSA_H__
