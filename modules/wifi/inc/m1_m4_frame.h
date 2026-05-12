/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __M1_M4_FRAME_H__
#define __M1_M4_FRAME_H__

#include "suppl_auth.h"

void parse_and_decode_m1(INFO_PARAMS *param);
NT_BOOL parse_and_decode_m3(INFO_PARAMS *param, uint8_t *mic);
NT_BOOL verify_mic(uint8_t ver, uint8_t *buf, uint16_t len, uint8_t *mic, uint8_t *kck);
void create_eap_m2(INFO_PARAMS *param, uint8_t *buf, uint8_t *sz);
void create_eap_m4(INFO_PARAMS *param, uint8_t *buf, uint8_t *sz);
void generate_ptk(INFO_PARAMS *param, uint8_t *pmk, uint8_t pmk_len, uint8_t *n1, uint8_t *n2, uint8_t *addr1,
                  uint8_t *addr2, uint8_t *ptk, uint16_t auth, uint8_t ucipher);
void sec_dump_frame(char *pmsg, uint8_t *fr, uint16_t sz);
uint8_t sec_get_frm_info(uint8_t *frm, uint16_t sz, uint16_t auth);
bool decrypt_key(INFO_PARAMS *param, uint8_t *keyiv, uint8_t *encrypted_data, uint16_t keydatalen);

NT_BOOL parse_and_decode_m2(INFO_PARAMS *param, uint8_t *mic);
void create_eap_m1(INFO_PARAMS *param, uint8_t *data, uint8_t *sz);
void create_eap_m3(INFO_PARAMS *param, uint8_t *data, uint8_t *sz);
void create_eap_m5(INFO_PARAMS *param, uint8_t *data, uint8_t *sz);
#ifdef NT_FN_RMF
uint8_t create_encrypted_key(INFO_PARAMS *param, uint16_t gtk_len, uint16_t igtk_len, uint8_t *result);
#else
uint8_t create_encrypted_key(INFO_PARAMS *param, uint16_t gtk_len, uint8_t *result);
#endif

#endif /* __M1_M4_FRAME_H__ */
