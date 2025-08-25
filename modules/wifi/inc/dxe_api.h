/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DXE_API_H_
#define DXE_API_H_

#include "nt_common.h"

#define NT_DXE_XFR_HOST_TO_HOST 0
#define NT_DXE_XFR_BMU_TO_BMU   1
#define NT_DXE_XFR_HOST_TO_BMU  2
#define NT_DXE_XFR_BMU_TO_HOST  3

typedef enum {
    NDXE_SUCCESS,
    NDXE_FAIL,
    NDXE_INVAILD_PARAMS,
    NDXE_NO_PKTS_AVAILABLE,
    NDXE_NO_FREE_DESC,
    NDXE_INVALID_CHANNEL,
    NDXE_NO_MEM_AVAILABLE,
    NDXE_ERROR_INTERRUPT,
    NDXE_REPEATED,
    NDXE_BUFF_ALLOC_FAILED,
} eRet_t;

/*build error with enum-conversion under ARM-GCC 2021.10
merged with e_dpm_dxe_ch_t defined in data_path.h and change e_dpm_dxe_ch_t to be typedef
*/
typedef enum {
    TX_BK = 0,       // DXE_CHANNEL_0
    TX_BE,           // DXE_CHANNEL_1
    TX_VI,           // DXE_CHANNEL_2
    TX_VO,           // DXE_CHANNEL_3
    TX_MGMT,         // DXE_CHANNEL_4
    RX_MGMT,         // DXE_CHANNEL_5
    RX_DATA,         // DXE_CHANNEL_6
    RRAM_WRITE_DXE,  // DXE_CHANNEL_7
    HW_ENC_DEC_1,    // DXE_CHANNEL_8
    HW_ENC_DEC_2,    // DXE_CHANNEL_9
    H2H,             // DXE_CHANNEL_10
    DXE_CHANNEL_0 = TX_BK,
    DXE_CHANNEL_1,
    DXE_CHANNEL_2,
    DXE_CHANNEL_3,
    DXE_CHANNEL_4,
    DXE_CHANNEL_5,
    DXE_CHANNEL_6,
    DXE_CHANNEL_7, /* used for RRAM write */
    DXE_CHANNEL_8,
    DXE_CHANNEL_9,
    DXE_CHANNEL_10,
    DXE_CHANNEL_11,

    // Set to upper bound, not a real channel
    DXE_CHANNEL_MAX,

    DXE_CHANNEL_NONE
} e_dxe_channel;

typedef enum {
    NT_DXE_BUF_HEAP,
    NT_DXE_BUF_PBUF,

    // Set to upper bound
    NT_DXE_BUF_MAX,
} e_dxe_buf_type;

typedef struct DxeChannelCfg {
    e_dxe_channel channel;
    uint32_t nDescs;      // Number of URBs for USB or descriptors for DXE that can be queued for transfer at one time
    uint32_t refWQ;       // Reference WQ - for H2B and B2H only
    uint32_t xfrType;     // H2B(Tx), B2H(Rx), H2H(SRAM<->HostMem R/W)
    uint32_t chPriority;  // Channel Priority 7(Highest) - 0(Lowest)
    uint32_t chk_size;
    uint32_t bmuThdSel;
    uint8_t buffer_type;
    uint8_t bdPresent;  // 1 = BD attached to frames for this pipe
    uint8_t BDTXIdx;    // BD Template Index for H2B Transfer
    uint8_t useshortdescfmt;
    void (*cbfn)(uint32_t arg);
    uint32_t arg;
} dxe_channel_cfg_t, *p_dxe_channel_cfg_t;

eRet_t nt_ndxe_init();
void nt_ndxe_deinit(void);
#ifdef MEM_CPY_VIA_DXE
void nt_dxe_cpy_done_handler(uint32_t tx_type);
void *nt_dxe_memcpy(void *dst, const void *src, uint32_t length);
#endif
void nt_ndxe_config_channel(e_dxe_channel channel, p_dxe_channel_cfg_t pcfg);
eRet_t nt_ndxe_write_frame_to_transfer(e_dxe_channel channel, const void *frame, uint32_t length, void *h2hdst);
eRet_t nt_ndxe_get_single_received_frame(e_dxe_channel channel, void **frame);
void nt_dxe_interrupt_handler(void);
uint32_t nt_dxe_get_pending_pkt_count(e_dxe_channel channel);
void nt_ndxe_channel_change(e_dxe_channel channel, uint8_t amsdu);
void nt_ndxe_channel_change_notify(uint8_t amsdu);
void *nt_dxe_get_last_tx_pkt(e_dxe_channel channel);
void *nt_dxe_get_last_rx_pkt(e_dxe_channel channel);
uint32_t nt_dxe_get_dxe_timestamp(e_dxe_channel channel);
uint32_t hal_dxe_suspend();
uint32_t hal_dxe_resume();
void nt_hal_wait_until_dxe_channel_avail(void);
#ifdef SUPPORT_BMU_ERROR_RECOVERY
/* Store the DXE state and suspend DXE before BMU recovery */
void hal_dxe_abort_pre_bmu_recovery(void);
/* Restore the DXE state to the value from before DXE suspend for BMU recovery */
void hal_dxe_restore_post_bmu_recovery(void);
#endif /* SUPPORT_BMU_ERROR_RECOVERY */
#ifdef NT_TST_TIME_STAMP_ENABLE
void *nt_dxe_get_tm_tx_pkt(e_dxe_channel channel);
#endif
void nt_dxe_update_intr_cnt(e_dxe_channel channel);
#endif /* DXE_API_H_ */
