/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//
// This file contains the api for the WLAN TxRx QOS module.
//
// $Id:
//
//

#ifndef _TXRX_QOS_API_H_
#define _TXRX_QOS_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "txrx_cfg_api.h"
#include "wmi.h"

#define BE_ACM   0
#define BE_AIFSN 3
#define BE_CWMIN 4
#define BE_CWMAX 10
#define BE_TXOP  0

#define BK_ACM   0
#define BK_AIFSN 7
#define BK_CWMIN 4
#define BK_CWMAX 10
#define BK_TXOP  0

#define VI_ACM   0
#define VI_AIFSN 2
#define VI_CWMIN 3
#define VI_CWMAX 4
#define VI_TXOP  94

#define VO_ACM   0
#define VO_AIFSN 2
#define VO_CWMIN 2
#define VO_CWMAX 3
#define VO_TXOP  47

struct devh_s;

void *txrx_qos_init(devh_t *dev);
void txrx_context_deinit(void *txrx_context_inst);
void txrx_qos_update_queueparams(struct devh_s *dev, struct chanAccParams *newParams, NT_BOOL forceUpdate);
void txrx_qos_process_tspec(struct devh_s *dev, WMM_TSPEC_IE *wmmTspec, uint8_t status);
void txrx_qos_update_totalMediumTime(struct devh_s *dev);
WMM_TSPEC_INFO *txrx_qos_getWmmTspecConfig(struct devh_s *dev, uint8_t ac);
struct apsdConfig *txrx_qos_getUapsdConfig(struct devh_s *);
void txrx_qos_reset_WMM_states(struct devh_s *dev);
void txrx_qos_process_tsm_req(devh_t *dev, uint8_t *frm, uint8_t *efrm);
nt_status_t txrx_qos_process_delts(devh_t *dev, conn_t *conn, uint8_t *frm);

#ifdef ATH_KF
void txrx_qos_ResetPstreamMappings(struct devh_s *dev);
void txrx_qos_SetACParamsCmd(struct devh_s *dev, uint8_t *buffer);
void txrx_qos_SetMaxSpLenCmd(struct devh_s *dev, WMI_SET_MAX_SP_LEN_CMD *pCmd);
void txrx_qos_CreatePstreamCmd(struct devh_s *dev, WMI_CREATE_PSTREAM_CMD *pCmd);
void txrx_qos_DeletePstreamCmd(struct devh_s *dev, WMI_DELETE_PSTREAM_CMD *pCmd);
void txrx_qos_getTspecParams(struct devh_s *dev, WMI_CREATE_PSTREAM_CMD *pCmd, WMM_TSPEC_INFO *pTspec);
NT_BOOL txrx_qos_getWmmTspecAcm(struct devh_s *dev, uint8_t ac);
void txrx_qos_SetWmmCmd(struct devh_s *dev, uint8_t status);
void txrx_qos_SetWmmTxop(struct devh_s *dev, uint8_t cfg);
#endif
uint8_t *txrx_qos_GetApsdConfigInWmmIe(struct devh_s *dev);
#ifdef ATH_KF
struct wmmParams *txrx_qos_getACSpec(uint32_t wmm_ac);

uint8_t txrx_qos_NumpStreamCreated(void);
void txrx_qos_SetStaUAPSDCmd(struct devh_s *dev, WMI_SET_STA_UAPSD_CMD *pCmd);
#endif  // ATH_KF

#ifdef __cplusplus
}
#endif

#endif /* _TXRX_QOS_API_H_ */
