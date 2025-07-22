/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*
* @file coex_wlan.h
* @brief Coex wlan event handler params and struct definitions
*========================================================================*/
#ifndef _COEX_WLAN_EVENT_HANDLER_H_
#define _COEX_WLAN_EVENT_HANDLER_H_
/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nt_osal.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "nt_common.h"
#if defined(SUPPORT_COEX)
#include "wlan_dev.h"

#define COEX_APPEND_TO_STA_LIST(VdevID) \
    (coex_add_to_wlanlist(&gpBtCoexWlanInfoDev->NumOfWlanSTA, gpBtCoexWlanInfoDev->pWlanSTAList, VdevID))
#define COEX_REMOVE_FROM_STA_LIST(VdevID) \
    (coex_remove_from_wlanlist(&gpBtCoexWlanInfoDev->NumOfWlanSTA, gpBtCoexWlanInfoDev->pWlanSTAList, VdevID))
#define VDEV_ID_INVALID 0xFF

enum {
    COEX_WLAN_NONE,
    COEX_WLAN_2GHZ,
    COEX_WLAN_5GHZ,
    COEX_WLAN_6GHZ,
};

void coex_vdev_notification_update_manager(devh_t *vdev, uint8_t stimulus);
uint8_t coex_determine_vdev_state(uint8_t vdev_id);
void coex_add_to_wlanlist(uint8_t *pNumofWlanInMode, uint8_t *pWlanModeList, uint8_t VdevID);
void coex_remove_from_wlanlist(uint8_t *pNumofWlanInMode, uint8_t *pWlanModeList, uint8_t VdevID);
void coex_update_vdev(channel_t *pChan, uint8_t vdev_id, devh_t *pVdev);
void coex_reset_vdev(uint8_t vdev_id, uint8_t Stimulus);
void coex_add_to_list(uint8_t *pNumOfItems, void *pList, const void *pItem, uint8_t MaxNum, uint8_t NumOfBytesInItem);
void coex_remove_from_list(uint8_t *pNumOfItems, void *pList, const void *pItem,
                           __attribute__((__unused__)) uint8_t MaxNum, uint8_t NumOfBytesInItem, void *pInvalidItem);
void coex_set_freq_range(channel_t *pChan);

#endif  // #if defined(SUPPORT_COEX)
#endif  // _COEX_WLAN_EVENT_HANDLER_H_
