/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/


#ifndef  __ECSA_H__
#define  __ECSA_H__

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "wlan_mlme.h"
#ifdef FEATURE_STA_ECSA

#define ECSA_TSF_OFFSET 0

typedef enum
{
	ECSA_STOP,  		//0 ECSA request has not been recieved or has been fulfilled
	ECSA_PENDING, 		//1 ECSA request is getting processed and channel change isnt complete
	ECSA_START,			//2 ECSA request is recieved
	ECSA_PENDING_TWT,
} ecsa_state_t;


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

void ecsa_channel_change();


/*==========================================================================
   * FUNCTION:  nt_recv_ecsa_action_frame()
   *
   * DESCRIPTION:
   * This api parses the data recieved in ECSA frame and stores it in ecsa_ctx
   * Depending on whether STA is in TWT mode or non TWT mode, changes channel
   * PARAMETERS:
   * 1. dev - device pointer
   * 2. p_frm - frame pointer
   * 3. conn - connection structure
   * RETURN VALUE:
   * MLME_SM_STATUS - MLME_SM_OK or MLME_SM_ERR

============================================================================*/

MLME_SM_STATUS nt_recv_ecsa_action_frame(devh_t *dev, uint8_t *p_frm, conn_t *conn);



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

#define CS_MODE_STOP_TRAFFIC 1

#define START_6G_HE_OPERATING_CLASS 131
#define START_6G_HE160_OPERATING_CLASS	134

#endif //FEATURE_STA_ECSA
#endif	// __ECSA_H__
