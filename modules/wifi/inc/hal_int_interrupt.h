/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _HAL_INT_INTERRUPT_H_

#define _HAL_INT_INTERRUPT_H_

#include <stdint.h>

void wlan_ccu_fiq_handler(void);
void combined_fiq_handler(void);
void dpu_fiq_handler(void);
void dpu_error_handler(void);
void rxp_group_fiq_handler(void);
void tpe_fiq_handler(void);
void phy_fiq_fiq_handler(void);
void phy_fiq_irq_handler(void);

#if (defined NT_FN_WUR_STA)
void wur_cpu_int(void);
#endif

extern void (*mic_error_interrupt_cb)(void);
extern void (*bad_decrypt_error_interrupt_cb)(void);
extern void (*eosp_interrupt_cb)(uint8_t tid, uint8_t more_bit, uint8_t staid);
extern void (*more_bit_interrupt_cb)(void);
extern void (*nt_rtt_t4_capture_interrupt_cb)();
extern void (*nt_rtt_t2_capture_interrupt_cb)();

extern void (*tx_bd_complete_interrupt_cb)();
extern void (*bmu_error_interrupt_cb)(void);
extern void (*rpe_error_interrupt_cb)(void);
extern void (*phy_hif_fiq_interrupt_cb)(void);
extern void (*phy_fiq_irq_interrupt_cb)(void);
extern void (*phy_fiq_fiq_interrupt_cb)(void);
extern void (*txp_error_interrupt_cb)(void);
extern void (*dbr_incorrect_length_interrupt_cb)(void);
extern void (*dbr_gam_error_interrupt_cb)(void);
extern void (*ahb2phy_timeout_error1_interrupt_cb)(void);
extern void (*ahb2phy_timeout_error2_interrupt_cb)(void);
extern void (*ack_ba_mdbit_low_interrupt_cb)(void);

typedef struct halmac_err_cnt {
    uint32_t ahb2phy_to_err1_cnt;
    uint32_t ahb2phy_to_err2_cnt;
    uint32_t bmu_err_cnt;
    uint32_t dbr_len_err_cnt;
    uint32_t dbr_gam_err_cnt;
    uint32_t dpu_mic_err_cnt;
    uint32_t dpu_internal_err_cnt;
    uint32_t dxe_err_cnt[12];
    uint32_t rpe_err_cnt;
    uint32_t txp_err_cnt;
} halmac_err_cnt;

extern halmac_err_cnt g_halmac_err_cnt;

#if (defined NT_FN_WUR_STA)
/*! @function : wur_packet_avaliable_interrupt_cb
 * 	@Brief	:	call back funtion for received wur frame
 * 	@Param	:	received frame byte
 * */
extern void (*wur_packet_avaliable_interrupt_cb)(uint64_t payload);
extern void (*wake_main_radio_interrupt_cb)();
extern void (*wur_beacon_miss_interrupt_cb)();
extern void (*wur_crc_packet_error_interrupt_cb)();
#endif

#endif  //_HAL_INT_INTERRUPT_H_
