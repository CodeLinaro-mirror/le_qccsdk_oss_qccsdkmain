/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _WLAN_FRAMEGEN_H_
#define _WLAN_FRAMEGEN_H_

#include <stdint.h>
#include "wifi_cmn.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"

#define WLAN_TX_MGMT_FRAME_SIZE 500  // NEUTRINO FIX-ME: Earlier defined as 4096
#define WLAN_TX_CTS_FRAME_SIZE  512  // NEUTRINO FIX-ME: define the correct length
#define MGTBLEN                 380  /*Maximum IE len supported + IEEE80211 header len*/
#define INIT_DIALOG_ID_VALUE    1

typedef struct {
    uint16_t status;
    uint16_t alg;
    uint16_t seqNo;
    uint8_t *chlng;
#ifdef NT_FN_WPA3
    uint8_t *anti_clogging_token;
    uint16_t send_confirm;
    uint8_t *scalar;
    uint8_t *ff_element;
    uint8_t *confirm;
#endif  // NT_FN_WPA3
} auth_frm_arg_t;

typedef struct {
    uint16_t status;
    uint8_t aid;
} assoc_resp_frm_arg_t;

typedef struct {
    uint32_t arg;
    uint8_t *actionElem;
} ieee80211h_frm_arg_t;

typedef struct {
    uint32_t arg;
    uint16_t len;
    uint8_t *buf;
} generic_buf;

#if (defined NT_FN_WUR_AP) || (defined NT_FN_WUR_STA)
typedef struct {
    uint32_t frm_category;
    uint8_t wur_action;
    uint8_t wur_action_type;

} wur_buf_t;
#endif

#ifdef NT_FN_WNM_POWERSAVE_MODE
/* @struct	: wnm_ps_buf_t
 * @brief	: parameter for wnm sleep mode action frame
 * */
typedef struct {
    uint32_t frm_category;    ///< catagory for wnm sleep mode
    uint8_t wnm_action;       ///< action for wnm sleep mode req/resp
    uint8_t wnm_action_type;  ///< action type i.e exit/enter for wnm sleep mode
} wnm_ps_buf_t;
#endif /* NT_FN_WNM_POWERSAVE_MODE */

#if (defined SUPPORT_STA_TWT_RENEG) || (defined SUPPORT_AP_TWT_RENEG)
typedef enum {
    RSSI_TYPE,
    TWT_SI_TYPE,
    TWT_SP_TYPE,
    TWT_EB_OFFSET_TYPE,
    TWT_EFFECTIVE_NEW_PARAM_TSF_VALUE_TYPE,
} VS_AF_TYPE;

#define TWT_SI_SP_UPDATE                                                   \
    ((1 << TWT_SI_TYPE) | (1 << TWT_SP_TYPE) | (1 << TWT_EB_OFFSET_TYPE) | \
     (1 << TWT_EFFECTIVE_NEW_PARAM_TSF_VALUE_TYPE))
#define TWT_EB_OFFSET_UPDATE ((1 << TWT_EB_OFFSET_TYPE) | (1 << TWT_EFFECTIVE_NEW_PARAM_TSF_VALUE_TYPE))
#define TWT_TSF_UPDATE       (1 << TWT_EFFECTIVE_NEW_PARAM_TSF_VALUE_TYPE)

typedef struct {
    uint8_t type;
    uint8_t length;
    int16_t rssi;
} __attribute__((packed)) vs_af_rssi_type_t;

typedef struct {
    uint8_t type;
    uint8_t length;
    uint16_t twt_wake_exponent : 5, twt_wake_mentissa_lo : 11;
    uint8_t twt_wake_mentissa_hi : 5, rsvd : 3;
} __attribute__((packed)) vs_af_twt_si_type_t;

typedef struct {
    uint8_t type;
    uint8_t length;
    uint16_t twt_wake_tu : 1,       // Wake Duration Unit (0 = 256 𝜇s, 1 = 1 TU (1.024 ms))
        twt_min_wake_duration : 8,  // Bits 1 to 8: Nominal Minimum TWT Wake Duration
        rsvd : 7;
} __attribute__((packed)) vs_af_twt_sp_type_t;

typedef struct {
    uint8_t type;
    uint8_t length;
    uint32_t offset_from_tsf;
} __attribute__((packed)) vs_af_twt_eb_offset_type_t;

typedef struct {
    uint8_t type;
    uint8_t length;
    /* This is effective TSF for the new TWT SI/SP params received */
    uint64_t effective_tsf;
} __attribute__((packed)) vs_af_twt_effective_tsf_type_t;

#endif

#ifdef SUPPORT_AP_TWT_RENEG
typedef struct {
    uint8_t category;
    uint8_t qcn_oui[3];
    uint8_t sequence_num;
    vs_af_rssi_type_t rssi_type;
    vs_af_twt_si_type_t twt_si_type;
    vs_af_twt_sp_type_t twt_sp_type;
    vs_af_twt_eb_offset_type_t twt_eb_offset_type;
    vs_af_twt_effective_tsf_type_t twt_effective_tsf_type;
} twt_reneg_buf_t;
#endif

#ifdef NT_FN_TWT
/* @struct	: twt_buf_t
 * @brief	: parameter for twt action frame
 * */
typedef struct {
    uint32_t frm_category;    ///< catagory for twt
    uint8_t twt_action;       ///< action for twt i.e req/resp
    uint8_t twt_action_type;  ///< action type for twt req i.e enter suspend
} twt_buf_t;
#endif  // NT_FN_TWT
typedef struct {
    union {
        uint32_t arg;
        auth_frm_arg_t auth_frm_arg;
        ieee80211h_frm_arg_t ieee80211h_frm_arg;
        generic_buf tlv;
        assoc_resp_frm_arg_t assoc_resp_arg;
#if (defined NT_FN_WUR_AP) || (defined NT_FN_WUR_STA)
        wur_buf_t wur_buf;
#endif
#ifdef NT_FN_WNM_POWERSAVE_MODE
        wnm_ps_buf_t wnm_buf;  ///< wnm parameter for management frame
#endif                         /* NT_FN_WNM_POWERSAVE_MODE */
#ifdef NT_FN_TWT
        twt_buf_t twt_buf;  ///< twt parameter for management frame
#endif                      /* NT_FN_TWT */
#ifdef SUPPORT_AP_TWT_RENEG
        twt_reneg_buf_t twt_reneg_buf;
#endif
    } s;

#ifdef FEATURE_TX_COMPLETE
    pfn_callback callback;
    void *params;
#endif  // FEATURE_TX_COMPLETE
} mgmt_frm_arg_t;

typedef struct ie_info_s {
    uint8_t ieLen;
    uint8_t ieInfo[WMI_MAX_IE_LEN];
} ie_info_t;

void FRMGEN_SetAppieCmd(devh_t *dev, WMI_SET_APPIE_CMD *buffer);

void FRMGEN_GetAppie(devh_t *dev, uint16_t id, ie_info_t **pIe);

uint8_t *FRMGEN_ProbeReq(devh_t *dev, uint8_t *dst_addr, uint8_t *bssid, ssid_t *ssid_info, uint16_t *msg_len);
uint8_t *FRMGEN_QosDataNull(devh_t *dev, uint8_t *dst_addr, uint8_t *bssid, uint16_t *msg_len, uint8_t qos);

uint8_t *FRMGEN_BeaconProbe(devh_t *dev, bss_t *bss, uint8_t type, uint8_t *pAddr1, uint16_t *msg_len);
uint8_t *FRMGEN_CreateMgmt(devh_t *dev, int32_t type, uint8_t *pAddr1, conn_t *conn, mgmt_frm_arg_t *mgmt_frm_arg,
                           uint16_t *msg_len);
uint8_t *FRMGEN_Pspoll(devh_t *dev, uint8_t wmmAC, uint32_t cb_flags, uint16_t sendCompId, uint16_t assocId,
                       uint8_t *pDstAddr);
uint8_t *FRMGEN_NullData(devh_t *dev, uint8_t wmmAC, uint32_t cb_flags, uint8_t sendCompId, conn_t *conn, int bQos);
void *FRMGEN_Init(devh_t *dev);

uint8_t *FRMGEN_CtsToSelf(devh_t *dev, uint16_t dur);
uint8_t *FRMGEN_GenerateBar(devh_t *dev, uint8_t *dest, uint32_t cb_flags, uint16_t sendCompId, uint8_t tid,
                            uint16_t seq_num);
void FRMGEN_ie_module_init();
ie_info_t *FRMGEN_alloc_ie();
void FRMGEN_deinit(void *FRMGEN_inst);
void FRMGEN_free_ie(ie_info_t *pIe);
void FRMGEN_set_ie(devh_t *dev, uint16_t id, ie_info_t *pIe);
#if ((defined NT_FN_RMF) || (defined NT_FN_WPA3))
void FRMGEN_mmie_mic(devh_t *dev, uint8_t *frmbody_start, uint8_t frm_body_len, uint8_t *mic, uint8_t *bufPtr);
#endif

uint8_t *FRMGEN_CfEnd(devh_t *dev);
#if (defined NT_FN_WUR_AP) || (defined NT_FN_WUR_STA)
uint16_t FRMGEN_create_modeset_req(uint8_t *frm, int8_t action_field, int8_t action_type);
uint8_t *FRMGEN_teardown(int8_t action_field);
#endif
#ifdef NT_FN_WUR_AP
uint16_t FRMGEN_create_modeset_res(devh_t *dev, uint8_t *frm, int8_t action, int8_t action_type);
uint8_t *FRMGEN_wakeup_indication(int8_t action_field);
uint8_t *FRMGEN_wur_beacon_frame(devh_t *dev, uint8_t *frm, uint16_t txid);
uint8_t FRMGEN_wur_wakeup_frame(devh_t *dev, uint8_t *frm, uint16_t id_t);
uint8_t *FRMGEN_wur_vendor_specific_frame(uint16_t id_t);
#endif

#ifdef NT_FN_WNM_POWERSAVE_MODE
/**
 *	@Func 	:	FRMGEN_create_wnm_sleep_mode_req
 *	@Brief 	: 	This api create wnm sleep mode response frame
 *	@Param	:	frm - pointer to frame buffer
 *	@Param	:	action - wnm action i.e sleep req/resp
 *	@Param	:	action type - type of action frame i.e enter/exit
 *	@Return	:	length of frame
 */
uint16_t FRMGEN_create_wnm_sleep_mode_req(devh_t *dev, uint8_t *frm, int8_t action, int8_t action_type);

/**
 *	@Func 	:	FRMGEN_create_wnm_sleep_mode_resp
 *	@Brief 	: 	This api create wnm sleep mode response frame
 *	@Param	:	frm - pointer to frame buffer
 *	@Param	:	action - wnm action i.e sleep req/resp
 *	@Param	:	action type - type of action frame  i.e enter/exit
 *	@Return	:	length of frame
 */
uint16_t FRMGEN_create_wnm_sleep_mode_resp(uint8_t *frm, int8_t action, int8_t action_type);
#endif /* NT_FN_WNM_POWERSAVE_MODE */

#ifdef NT_FN_TWT
/**
 *	@Func 	:	FRMGEN_create_twt_req
 *	@Brief 	: 	This api create twt request frame
 *	@Param	:	frm - pointer to frame buffer
 *	@Param	:	action - wnm action frame
 *	@Param	:	action type - type of action frame i.e enter/exit
 *	@Return	:	length of frame
 */
uint16_t FRMGEN_create_twt_req(devh_t *dev, uint8_t *frm, int8_t action, int8_t action_type);
/**
 *	@Func 	:	FRMGEN_create_twt_resp
 *	@Brief 	: 	This api create twt response frame
 *	@Param	:	frm - pointer to frame buffer
 *	@Param	:	action type - type of action frame
 *	@Return	:	length of frame
 */
uint16_t FRMGEN_create_twt_resp(devh_t *dev, uint8_t *frm, int8_t action, int8_t action_type);

/**
 *	@Func 	:	FRMGEN_create_twt_brodcast_req
 *	@Brief 	: 	This api create twt request frame
 *	@Param	:	frm - pointer to frame buffer
 *	@Param	:	action - wnm action frame
 *	@Param	:	action type - type of action frame i.e enter/exit
 *	@Return	:	length of frame
 */
uint16_t FRMGEN_create_twt_brodcast_req(devh_t *dev, uint8_t *frm, int8_t action, int8_t action_type);
/**
 *	@Func 	:	FRMGEN_create_twt_brodcast_resp
 *	@Brief 	: 	This api create twt response frame
 *	@Param	:	frm - pointer to frame buffer
 *	@Param	:	action type - type of action frame
 *	@Return	:	length of frame
 */
uint16_t FRMGEN_create_twt_brodcast_resp(devh_t *dev, uint8_t *frm, int8_t action, int8_t action_type);

/**
 *  @Func   :   FRMGEN_create_twt_teardown
 *  @Brief  :   This api create twt teardown frame
 *  @Param  :   frm - pointer to frame buffer
 *  @Param  :   action type - type of action frame
 *  @Return :   length of frame
 */
uint16_t FRMGEN_create_twt_teardown(devh_t *dev, uint8_t *frm, int8_t action, int8_t action_type, uint8_t is_btwt);

#endif  // NT_FN_TWT

/**
 *  @Func   :   FRMGEN_get_dialog_token
 *  @Brief  :   This api gets the dialog tokens for action frame.
 *  @Param  :   None
 *  @Return :   The current available dialog token. returns dialog_id between 1 to 255
 */

uint8_t FRMGEN_get_dialog_token(devh_t *dev);
#endif /* _WLAN_FRAMEGEN_H_ */
