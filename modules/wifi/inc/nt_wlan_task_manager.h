/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef WFM_INC_NT_SME_MLME_TASK_MANAGER_H_
#define WFM_INC_NT_SME_MLME_TASK_MANAGER_H_

#include "nt_osal.h"

#define NT_MAX_QUEUE_SIZE          100
#define NT_MAX_QUEUE_WAIT_TIME     portMAX_DELAY
#define NT_MAX_SEMAPHORE_WAIT_TIME 3000

extern qurt_pipe_t msg_wfm_wmi_id;
void nt_devcfg_prot_wifi_app();
void nt_devcfg_wlan_init();
void nt_devcfg_logger_init();
void nt_wlan_task(void *pvParameters);
/**
 * brief  : creates wlan task and queue related to it.
 * params : none
 * return : nt_pass on successful creation of task and queue, nt_fail on failure to do so.
 */
BaseType_t nt_create_wlan_task(uint8 is_ftm);

#ifdef CONFIG_WMI_EVENT
BaseType_t nt_create_wlan_evt_task(uint8 is_ftm);
#endif

#endif /* WFM_INC_NT_SME_MLME_TASK_MANAGER_H_ */
