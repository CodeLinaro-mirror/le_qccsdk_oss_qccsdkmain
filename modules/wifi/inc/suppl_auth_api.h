/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//#ifdef  SECURITY

#ifndef __SUPPL_AUTH_API_H__
#define __SUPPL_AUTH_API_H__

#include <stdint.h>
//#include "nt_common.h"
#include "wifi_cmn.h"
#include "wmi.h"
#include "if_ethersubr.h"

typedef enum { SUPPLICANT_FN = 1, AUTHENTICATOR_FN } SUPPL_AUTH_FN;

typedef enum {
    SUPPL_STATUS_FAIL_UNDEF = 0,
    SUPPL_STATUS_SUCCESS,
    SUPPL_STATUS_FAIL_RECV_M1,
    SUPPL_STATUS_FAIL_RECV_M3,
    SUPPL_STATUS_TKIP_CM_START,
    SUPPL_STATUS_TKIP_CM_STOP,
    SUPPL_STATUS_MIC_FIALURE,
} SUPPL_STATUS;

typedef enum {
    SUPPL_AUTH_EVENT_RECV_M1 = 0xB0,
    SUPPL_AUTH_EVENT_RECV_M2,
    SUPPL_AUTH_EVENT_RECV_M3,
    SUPPL_AUTH_EVENT_RECV_M4,
    SUPPL_AUTH_EVENT_RECV_MICERR,
    SUPPL_AUTH_EVENT_TIMEOUT,
    SUPPL_AUTH_EVENT_DISCONNECTED,
} SUPPL_AUTH_EVENT;

typedef uint16_t (*LL_HDR_FN)(void *pdev, uint8_t *buf, uint8_t *peer, uint8_t rekey_flag);
typedef void (*SUPPL_AUTH_COMPL_FN)(void *ctxt, uint8_t *peer, WMI_ADD_CIPHER_KEY_CMD *key, SUPPL_STATUS status);

typedef struct _supp_auth_config_t {
    SUPPL_AUTH_FN side;
    uint8_t hdr_len;
    uint16_t auth;
    uint8_t ucipher;
    uint8_t mcipher;
    uint8_t *peer;
    uint8_t *pmk;
    uint8_t *gtk;
#if ((defined NT_FN_RMF) || (defined NT_FN_WPA3))
    uint8_t *igtk;
#endif
    LL_HDR_FN hdr_fn;
    SUPPL_AUTH_COMPL_FN compl_fn;
    uint8_t *rsn_ie;
    uint8_t pmk_len;
} SUPP_AUTH_CONFIG_T;

/* flag used to indicate use WEP long cipher */
#define WEP_CRYPT_LONG_FLAG 0x10

/* Ref. IEEE 802.1X 2010, section 11.3 */
typedef struct {
    uint8_t version;
    uint8_t type;
    uint16_t length;
} NT_EAPOL_HEADER;

#define EAPOL_PACKET_TYPE_EAP    0x00
#define EAPOL_PACKET_TYPE_START  0x01
#define EAPOL_PACKET_TYPE_LOGOFF 0x02
#define EAPOL_PACKET_TYPE_KEY    0x03

void *suppl_auth_init(void *pdev, uint8_t *hwaddr);
void suppl_auth_deinit(void *supp_auth_inst);
nt_status_t suppl_del_auth_ctxt(void *ctxt, uint8_t *peer);
void suppl_auth_init_auth(void *suppl_ctxt, SUPP_AUTH_CONFIG_T config, void *calling_ctxt);
nt_status_t suppl_auth_proc_recv_frm(void *ctxt, uint8_t *sa, uint8_t *frm, uint16_t sz, uint32_t evt);
void suppl_auth_disconnect_evt(void *ctxt, uint8_t *sa);

#endif /* __SUPPL_AUTH_API_H__ */
//#endif  /* SECURITY */
