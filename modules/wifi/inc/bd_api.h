/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CORE_WIFI_DPM_INC_BD_API_H_
#define CORE_WIFI_DPM_INC_BD_API_H_
#include "data_path.h"
#include "bd.h"

#ifdef NT_FN_RRAM_PERF_BUILD
__attribute__((section(".perf_nc_txt"))) void nt_dpm_prefill_bd_mgmt(p_ndpA ad);
__attribute__((section(".perf_nc_txt"))) void nt_dpm_prefill_bd_data(p_ndpA ad);
__attribute__((section(".perf_tx_txt"))) p_dpm_bd_tx_template nt_nwlan_dpm_get_prefilled_bd(
    p_ndpA ad, e_bd_template_type_t bd_type);
__attribute__((section(".perf_nc_txt"))) uint32_t nt_append_bd_to_eap_frame(void *frame, uint32_t length,
                                                                            void **ret_frame, uint8_t rate);
__attribute__((section(".perf_nc_txt"))) uint32_t nt_get_pdu_from_rcvd_packet(void *frame, void **ret_frame);
#ifdef FEATURE_TX_COMPLETE
__attribute__((section(".perf_nc_txt"))) uint32_t nt_append_bd_to_data_frame(void *frame, uint32_t length,
                                                                             void **ret_frame, NT_BOOL tx_enabled);
__attribute__((section(".perf_nc_txt"))) uint32_t nt_append_bd_to_frame(void *frame, uint32_t length, void **ret_frame,
                                                                        NT_BOOL tx_enabled, uint8_t rate);
#else
__attribute__((section(".perf_nc_txt"))) uint32_t nt_append_bd_to_frame(void *frame, uint32_t length, void **ret_frame,
                                                                        uint8_t rate);
#endif
#else
void nt_dpm_prefill_bd_mgmt(p_ndpA ad);
void nt_dpm_prefill_bd_data(p_ndpA ad);
p_dpm_bd_tx_template nt_nwlan_dpm_get_prefilled_bd(p_ndpA ad, e_bd_template_type_t bd_type);
#ifdef FEATURE_TX_COMPLETE
uint32_t nt_append_bd_to_data_frame(void *frame, uint32_t length, void **ret_frame, NT_BOOL tx_enabled);
uint32_t nt_append_bd_to_frame(void *frame, uint32_t length, void **ret_frame, NT_BOOL tx_enabled, uint8_t rate);
#else
uint32_t nt_append_bd_to_frame(void *frame, uint32_t length, void **ret_frame, uint8_t rate);
#endif

#ifdef NT_RMF_TEST
/*
 * To append BD to Frame used for STA Attack scenario.
 * Parameters:
 * Param1:frame:      frame pointer.
 * Param2:length:     length of the frame.
 * Param3:ret_frame:  return frame pointer
 * Param4:testvar:    testvar used for frame manipulation
 * */
uint32_t nt_append_bd_to_frame_test(void *frame, uint32_t length, void **ret_frame, uint8_t testvar);
#endif

uint32_t nt_append_bd_to_eap_frame(void *frame, uint32_t length, void **ret_frame, uint8_t rate);
uint32_t nt_get_pdu_from_rcvd_packet(void *frame, void **ret_frame);
#endif

#endif /* CORE_WIFI_DPM_INC_BD_API_H_ */
